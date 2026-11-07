//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFSortFunctions.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"

#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <string.h>
#include <strings.h>

// Inferred organization: the native CFArray functions lie in three places of the iOS Simulator __text
// section:
// __CFArrayGetCallBacks (0x1c014b2b8) through CFArrayAppendArray (0x1c014d9e4), the creation entry points
// CFArrayCreateCopy (0x1c01cd544) through CFArrayCreateMutable (0x1c01cd860), interleaved with the
// __NSCFArray methods, and the out-of-line paths __CFArrayHandleOutOfMemory (0x1c0305eec) and the .cold
// entries. They are kept in one translation unit in native address order; the native source paths are not in
// the evidence. The second group inlines __FoundationPresent, whose statics and block belong to that group's
// translation unit. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (disassembly); iOS Device 24A446: unread.
// Reading: Local/Scratch/cf-core/tasks/ array-storage.txt (sub-track reading; no versioned reading yet);
// evidence of the later additions: Local/Evidence/cf-core/iphone-simulator-corpus-delta4.json.

// libobjc entry points the native image imports; the SDK installs no declaration for them.
extern id objc_alloc(Class cls);
extern Class objc_opt_class(id object);
extern BOOL objc_opt_isKindOfClass(id object, Class cls);

// The memory-exception path of the exception unit (non-external, was a private external); the native callers
// emit nothing after the call.
OF_EXTERN void _CFRaiseMemoryException(OCFStringRef reason) __attribute__((noreturn));

// The Foundation classes the Objective-C paths allocate or test (defined in the native CoreFoundation image).
extern char arrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSArray));
extern char mutableArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableArray));

// The storage types (info bits 0-1) and the callback kinds (info bits 2-3).
enum {
    kArrayImmutable = 0,
    kArrayDeque = 2,
    kArrayHasNullCallBacks = 0,
    kArrayHasCFTypeCallBacks = 1,
    kArrayHasCustomCallBacks = 3,
};

static void __CFArrayDeallocate(OCFTypeRef cf);
static Boolean __CFArrayEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFArrayHash(OCFTypeRef cf);
static OCFStringRef __CFArrayCopyDescription(OCFTypeRef cf);
static OCFArrayRef __CFArrayCreateInit(
    OCFAllocatorRef allocator, uint32_t flags, OCFIndex capacity, const OCFArrayCallBacks *callBacks
);
static void __CFArrayReleaseValues(OCFArrayRef array, OCFRange range, bool releaseStorageIfPossible);
static OCFComparisonResult __CFArrayCompareValues(const void *val1, const void *val2, void *context);
static void __CFArrayHandleOutOfMemory(OCFIndex numBytes) __attribute__((noreturn));

// Native __kCFNullArrayCallBacks, 0x1c0321278 (__TEXT,__const, 40 zero bytes). Non-external.
static const OCFArrayCallBacks __kCFNullArrayCallBacks = {0, NULL, NULL, NULL, NULL};

// Native kCFTypeArrayCallBacks, 0x222429088 (__DATA_CONST,__const). External; version 0,
// __CFTypeCollectionRetain,
// __CFTypeCollectionRelease, CFCopyDescription and CFEqual.
const OCFArrayCallBacks kOCFTypeArrayCallBacks = {
    0, __CFTypeCollectionRetain, __CFTypeCollectionRelease, OCFCopyDescription, OCFEqual
};

// Native __CFArrayClass, 0x2224290b0 (__DATA_CONST,__const). Hidden (was private external); version 1, name
// "CFArray", finalize, equal, hash and the debug description.
const OCFRuntimeClass __CFArrayClass = {
    1,
    "CFArray",
    NULL,
    NULL,
    __CFArrayDeallocate,
    __CFArrayEqual,
    __CFArrayHash,
    NULL,
    __CFArrayCopyDescription,
    NULL,
    NULL,
    0,
};

// The helpers below reproduce code the native functions inline at each use (no native symbols): the info word
// (an acquire load at every use), the storage type and callback kind, the bucket pointer, CF_IS_OBJC for type
// 19, __CFGetAllocator and the deque capacity rounding.
static uint64_t arrayInfo(OCFArrayRef array) {
    return __atomic_load_n(&array->base.info, __ATOMIC_ACQUIRE);
}

static uint32_t arrayType(OCFArrayRef array) {
    return (uint32_t)arrayInfo(array) & 3;
}

static uint32_t arrayCallBacksType(OCFArrayRef array) {
    return ((uint32_t)arrayInfo(array) >> 2) & 3;
}

// The first bucket: inline values after the header (and after custom callbacks), the deque's buckets from its
// left index, or NULL for any other storage type.
static const void **arrayBuckets(OCFArrayRef array) {
    switch (arrayType(array)) {
        case kArrayImmutable:
            return (const void **)((uint8_t *)array + ((~arrayInfo(array) & 0xC) == 0 ? 88 : 48));
        case kArrayDeque:
            return array->store->buckets + array->store->leftIdx;
        default:
            return NULL;
    }
}

static const void **arrayBucketAtIndex(OCFArrayRef array, OCFIndex idx) {
    if ((arrayInfo(array) & 1) != 0) {
        return NULL;
    }
    return arrayBuckets(array) + idx;
}

static bool isObjCArray(OCFTypeRef cf) {
    return (OF_WORD_SHIFT((uintptr_t)cf, 63)) != 0 || __CF_IS_OBJC(19, cf) != 0;
}

static OCFAllocatorRef arrayAllocator(OCFTypeRef cf) {
    if ((OF_WORD_SHIFT((uintptr_t)cf, 63)) != 0 || (arrayInfo(cf) & kOCFInfoDefaultAllocator) != 0) {
        return kOCFAllocatorSystemDefault;
    }
    return *(OCFAllocatorRef *)((const uint8_t *)cf - 16);
}

static OCFIndex dequeRoundUpCapacity(OCFIndex capacity) {
    if (capacity < 4) {
        return 4;
    }
    return flsl(capacity) > 62 ? INTPTR_MAX : (OCFIndex)1 << flsl(capacity);
}

static void setArrayFlags(OCFArrayRef array, uint64_t flags) {
    uint64_t *word = (uint64_t *)&array->base.info;
    uint64_t info = __atomic_load_n(word, __ATOMIC_ACQUIRE);
    while (!__atomic_compare_exchange_n(
        word, &info, (info & 0xFFFFFFFFFFFFFF80) | flags, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    )) {
    }
}

// Native __CFArrayGetCallBacks, 0x1c014b2b8. Hidden (was private external). No callbacks and the CFType
// callbacks are the static tables; any other kind is the copy at offset 48 of an immutable or deque array,
// NULL for another storage type.
const OCFArrayCallBacks *__CFArrayGetCallBacks(OCFArrayRef array) {
    switch (arrayCallBacksType(array)) {
        case kArrayHasNullCallBacks:
            return &__kCFNullArrayCallBacks;
        case kArrayHasCFTypeCallBacks:
            return &kOCFTypeArrayCallBacks;
        default:
            break;
    }
    switch (arrayType(array)) {
        case kArrayImmutable:
        case kArrayDeque:
            return (const OCFArrayCallBacks *)((const uint8_t *)array + sizeof(struct __OCFArray));
        default:
            return NULL;
    }
}

// Native __CFArrayDeallocate, 0x1c014b308. Non-external; a tail call releasing every value and the storage.
static void __CFArrayDeallocate(OCFTypeRef cf) {
    OCFArrayRef array = cf;
    __CFArrayReleaseValues(array, (OCFRange){0, array->count}, true);
}

// Native __CFArrayEqual, 0x1c014b318. Non-external. Equal counts and the same equal callback are required;
// values are compared by identity, then with the first array's equal callback (none: unequal).
static Boolean __CFArrayEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    OCFArrayRef array1 = cf1;
    OCFArrayRef array2 = cf2;
    if (array1 == array2) {
        return true;
    }
    OCFIndex cnt = array1->count;
    if (cnt != array2->count) {
        return false;
    }
    const OCFArrayCallBacks *cb1 = __CFArrayGetCallBacks(array1);
    const OCFArrayCallBacks *cb2 = __CFArrayGetCallBacks(array2);
    if (cb1->equal != cb2->equal) {
        return false;
    }
    for (OCFIndex idx = 0; idx < cnt; idx++) {
        const void *val1 = *arrayBucketAtIndex(array1, idx);
        const void *val2 = *arrayBucketAtIndex(array2, idx);
        if (val1 != val2) {
            if (cb1->equal == NULL) {
                return false;
            }
            if (!cb1->equal(val1, val2)) {
                return false;
            }
        }
    }
    return true;
}

// Native __CFArrayHash, 0x1c014b4d0. Non-external; the count.
static OCFHashCode __CFArrayHash(OCFTypeRef cf) {
    return (OCFHashCode)((OCFArrayRef)cf)->count;
}

// Native __CFArrayCopyDescription, 0x1c014b4d8. Non-external. The header line is written only for the
// immutable and deque types; each value uses the copyDescription callback when it answers a string, else its
// address.
static OCFStringRef __CFArrayCopyDescription(OCFTypeRef cf) {
    OCFArrayRef array = cf;
    OCFIndex cnt = array->count;
    OCFAllocatorRef allocator = arrayAllocator(array);
    OCFMutableStringRef result = OCFStringCreateMutable(allocator, 0);
    switch (arrayType(array)) {
        case kArrayImmutable:
            OCFStringAppendFormat(
                result,
                NULL,
                OF_STR("<CFArray %p [%p]>{type = immutable, count = %lu, values = (%s"),
                cf,
                allocator,
                cnt,
                cnt != 0 ? "\n" : ""
            );
            break;
        case kArrayDeque:
            OCFStringAppendFormat(
                result,
                NULL,
                OF_STR("<CFArray %p [%p]>{type = mutable-small, count = %lu, values = (%s"),
                cf,
                allocator,
                cnt,
                cnt != 0 ? "\n" : ""
            );
            break;
        default:
            break;
    }
    const OCFArrayCallBacks *cb = __CFArrayGetCallBacks(array);
    for (OCFIndex idx = 0; idx < cnt; idx++) {
        const void *val = *arrayBucketAtIndex(array, idx);
        OCFStringRef desc = NULL;
        if (cb->copyDescription != NULL) {
            desc = cb->copyDescription(val);
        }
        if (desc != NULL) {
            OCFStringAppendFormat(result, NULL, OF_STR("\t%lu : %@\n"), idx, desc);
            OCFRelease(desc);
        } else {
            OCFStringAppendFormat(result, NULL, OF_STR("\t%lu : <%p>\n"), idx, val);
        }
    }
    OCFStringAppend(result, OF_STR(")}"));
    return result;
}

// Native CFArrayGetTypeID, 0x1c014b6dc. External.
OCFTypeID OCFArrayGetTypeID(void) {
    return 19;
}

// Native __CFArrayCreateTransfer, 0x1c014b6e4. Hidden (was private external). An immutable array with the
// CFType callbacks (info flags 4) that takes over values without retaining them; NULL when numValues * 8 + 32
// does not fit an unsigned 64-bit size or when the instance cannot be created.
OCFArrayRef __CFArrayCreateTransfer(OCFAllocatorRef allocator, const void **values, OCFIndex numValues) {
    uint64_t size;
    if (__builtin_mul_overflow(numValues, (uint64_t)8, &size) || __builtin_add_overflow(size, 32, &size)) {
        return NULL;
    }
    OCFArrayRef array = _OCFRuntimeCreateInstance(allocator, 19, (OCFIndex)size, NULL);
    if (array == NULL) {
        return NULL;
    }
    setArrayFlags(array, 4);
    ((struct __OCFArray *)array)->count = numValues;
    memmove(arrayBuckets(array), values, (size_t)numValues * sizeof(const void *));
    if (__OCFOASafe == 1) {
        __OCFSetLastAllocationEventName((void *)array, "CFArray (immutable)");
    }
    return array;
}

// Native __CFArrayCreate0, 0x1c014b808. Hidden (was private external). The values are retained with the given
// allocator through the new array's callbacks, or copied as they are. The result of __CFArrayCreateInit is
// used without a NULL check, as natively.
OCFArrayRef __CFArrayCreate0(
    OCFAllocatorRef allocator, const void **values, OCFIndex numValues, const OCFArrayCallBacks *callBacks
) {
    OCFArrayRef result = __CFArrayCreateInit(allocator, kArrayImmutable, numValues, callBacks);
    const OCFArrayCallBacks *cb = __CFArrayGetCallBacks(result);
    const void **buckets = arrayBuckets(result);
    if (cb->retain != NULL) {
        for (OCFIndex idx = 0; idx < numValues; idx++) {
            buckets[idx] = cb->retain(allocator, values[idx]);
        }
    } else {
        for (OCFIndex idx = 0; idx < numValues; idx++) {
            buckets[idx] = values[idx];
        }
    }
    ((struct __OCFArray *)result)->count = numValues;
    return result;
}

// Native __CFArrayCreateInit, 0x1c014b948. Non-external. Callbacks with any slot set become kind 1 when they
// are kCFTypeArrayCallBacks or its four functions, else kind 3 (copied after the header). The instance has 32
// extra bytes (72 with custom callbacks) plus, when immutable, 8 per value; the deque type starts with one
// mutation, no mutation in progress and no store. Allocation-event names: "CFArray (immutable)", "CFArray
// (mutable-variable)". CoreFoundation defect: the native function forms the instance size in 32-bit registers
// (LSL W8, W2, #3; ADD W2, W9, W8), so an immutable capacity of 2^29 or more wraps and the instance is
// smaller than the values that
// __CFArrayCreate0 and __CFArrayCreateCopy0 then store; confidence: verified (disassembly). The restoration
// uses the exact size when the 32-bit sum would wrap for a positive capacity, and answers NULL (as for a
// failed allocation) when that size does not fit 64 bits; every other size is the native one.
static OCFArrayRef __CFArrayCreateInit(
    OCFAllocatorRef allocator, uint32_t flags, OCFIndex capacity, const OCFArrayCallBacks *callBacks
) {
    if (callBacks != NULL) {
        OCFArrayRetainCallBack retain = callBacks->retain;
        if (retain != NULL || callBacks->release != NULL || callBacks->copyDescription != NULL ||
            callBacks->equal != NULL) {
            if (callBacks == &kOCFTypeArrayCallBacks ||
                (retain == __CFTypeCollectionRetain && callBacks->release == __CFTypeCollectionRelease &&
                 callBacks->copyDescription == OCFCopyDescription && callBacks->equal == OCFEqual)) {
                flags |= 4;
            } else {
                flags |= 0xC;
            }
        }
    }
    uint32_t callBacksBits = flags & 0xC;
    uint32_t type = flags & 3;
    uint32_t base = callBacksBits == 0xC ? 0x48 : 0x20;
    uint32_t size = base + (type == kArrayImmutable ? (uint32_t)capacity << 3 : 0);
    OCFIndex extraBytes = size;
    if (type == kArrayImmutable && capacity > 0 && (uint64_t)capacity > (UINT32_MAX - base) >> 3) {
        if (__builtin_mul_overflow(capacity, 8, &extraBytes) ||
            __builtin_add_overflow(extraBytes, (OCFIndex)base, &extraBytes)) {
            return NULL;
        }
    }
    OCFArrayRef array = _OCFRuntimeCreateInstance(allocator, 19, extraBytes, NULL);
    if (array == NULL) {
        return NULL;
    }
    struct __OCFArray *memory = (struct __OCFArray *)array;
    setArrayFlags(array, flags);
    memory->count = 0;
    if (type == kArrayDeque) {
        if (__OCFOASafe == 1) {
            __OCFSetLastAllocationEventName(memory, "CFArray (mutable-variable)");
        }
        memory->mutations = 1;
        memory->mutationsInProgress = 0;
        memory->store = NULL;
    } else if (type == kArrayImmutable) {
        if (__OCFOASafe == 1) {
            __OCFSetLastAllocationEventName(memory, "CFArray (immutable)");
        }
    }
    if (callBacksBits == 0xC) {
        *(OCFArrayCallBacks *)__CFArrayGetCallBacks(array) = *callBacks;
    }
    return array;
}

// Native __CFArrayCreateMutable0, 0x1c014bb28. Hidden (was private external); a tail call with type 2.
OCFMutableArrayRef
__CFArrayCreateMutable0(OCFAllocatorRef allocator, OCFIndex capacity, const OCFArrayCallBacks *callBacks) {
    return (OCFMutableArrayRef)__CFArrayCreateInit(allocator, kArrayDeque, capacity, callBacks);
}

// Native __CFArrayCreateCopy0, 0x1c014bb38. Hidden (was private external). An Objective-C source copies with
// the CFType callbacks; every value is read with CFArrayGetValueAtIndex (an -objectAtIndex: message, also for
// a CF source) and retained with the given allocator. The new array is used without a NULL check, as
// natively.
OCFArrayRef __CFArrayCreateCopy0(OCFAllocatorRef allocator, OCFArrayRef array) {
    OCFIndex numValues = OCFArrayGetCount(array);
    const OCFArrayCallBacks *cb = isObjCArray(array) ? &kOCFTypeArrayCallBacks : __CFArrayGetCallBacks(array);
    OCFArrayRef result = __CFArrayCreateInit(allocator, kArrayImmutable, numValues, cb);
    cb = __CFArrayGetCallBacks(result);
    const void **buckets = arrayBuckets(result);
    for (OCFIndex idx = 0; idx < numValues; idx++) {
        const void *value = OCFArrayGetValueAtIndex(array, idx);
        if (cb->retain != NULL) {
            value = cb->retain(allocator, value);
        }
        buckets[idx] = value;
    }
    ((struct __OCFArray *)result)->count = numValues;
    return result;
}

// Native CFArrayGetCount, 0x1c014bd04. External. Objective-C arrays answer -count.
OCFIndex OCFArrayGetCount(OCFArrayRef theArray) {
    if (isObjCArray(theArray)) {
        return ((OCFIndex (*)(OCFArrayRef, SEL))objc_msgSend)(theArray, sel_getUid("count"));
    }
    return theArray->count;
}

// Native CFArrayGetValueAtIndex, 0x1c014bd48. External; a tail call of -objectAtIndex: for every array.
const void *OCFArrayGetValueAtIndex(OCFArrayRef theArray, OCFIndex idx) {
    return ((const void *(*)(OCFArrayRef, SEL, OCFIndex))objc_msgSend)(
        theArray, sel_getUid("objectAtIndex:"), idx
    );
}

// Native __CFArrayCreateMutableCopy0, 0x1c014bd50. Hidden (was private external). A zero capacity sets the
// deque capacity to the source count first; the values are appended one by one (CFArrayGetValueAtIndex, then
// CFArrayAppendValue).
OCFMutableArrayRef
__CFArrayCreateMutableCopy0(OCFAllocatorRef allocator, OCFIndex capacity, OCFArrayRef array) {
    OCFIndex numValues = OCFArrayGetCount(array);
    const OCFArrayCallBacks *cb = isObjCArray(array) ? &kOCFTypeArrayCallBacks : __CFArrayGetCallBacks(array);
    OCFMutableArrayRef result = (OCFMutableArrayRef)__CFArrayCreateInit(allocator, kArrayDeque, capacity, cb);
    if (capacity == 0) {
        _OCFArraySetCapacity(result, numValues);
    }
    for (OCFIndex idx = 0; idx < numValues; idx++) {
        OCFArrayAppendValue(result, OCFArrayGetValueAtIndex(array, idx));
    }
    return result;
}

// Native _CFArraySetCapacity, 0x1c014be70. External. Only CF deque arrays: the deque is reallocated for the
// rounded capacity (8 bytes per bucket after the 16-byte header, malloc type 0x1000040451B5BE8, the array's
// allocator), copying the old header and buckets, or created centered; a failed allocation is
// __CFArrayHandleOutOfMemory. Allocation-event name "CFArray (store-deque)".
// CoreFoundation defect: the native function copies the old deque's capacity of buckets into the new block
// whatever its size and keeps the left index, so a rounded capacity below the current one overflows the new
// block; confidence: verified (disassembly). The restoration leaves the deque unchanged in that case; any
// other capacity is handled natively.
void _OCFArraySetCapacity(OCFMutableArrayRef array, OCFIndex cap) {
    if (isObjCArray(array)) {
        return;
    }
    if (arrayType(array) != kArrayDeque) {
        return;
    }
    ArrayDeque *deque = array->store;
    OCFIndex capacity = dequeRoundUpCapacity(cap);
    OCFIndex size = (OCFIndex)((uint64_t)capacity * sizeof(const void *) + sizeof(ArrayDeque));
    OCFAllocatorRef allocator = arrayAllocator(array);
    if (deque == NULL) {
        deque = OCFAllocatorAllocateTyped(allocator, size, 0x1000040451B5BE8, 0);
        if (deque == NULL) {
            __CFArrayHandleOutOfMemory(size);
        }
        if (__OCFOASafe == 1) {
            __OCFSetLastAllocationEventName(deque, "CFArray (store-deque)");
        }
        deque->leftIdx = capacity / 2;
    } else {
        ArrayDeque *olddeque = deque;
        OCFIndex oldcap = deque->capacity;
        if (capacity < oldcap) {
            return;
        }
        deque = OCFAllocatorAllocateTyped(allocator, size, 0x1000040451B5BE8, 0);
        if (deque == NULL) {
            __CFArrayHandleOutOfMemory(size);
        }
        memmove(deque, olddeque, sizeof(ArrayDeque) + (size_t)oldcap * sizeof(const void *));
        OCFAllocatorDeallocate(allocator, olddeque);
        if (__OCFOASafe == 1) {
            __OCFSetLastAllocationEventName(deque, "CFArray (store-deque)");
        }
    }
    deque->capacity = capacity;
    array->store = deque;
}

// Native CFArrayAppendValue, 0x1c014bffc. External. Objective-C arrays answer -addObject:.
void OCFArrayAppendValue(OCFMutableArrayRef theArray, const void *value) {
    if (isObjCArray(theArray)) {
        ((void (*)(OCFMutableArrayRef, SEL, const void *))objc_msgSend)(
            theArray, sel_getUid("addObject:"), value
        );
        return;
    }
    _OCFArrayReplaceValues(theArray, (OCFRange){theArray->count, 0}, &value, 1);
}

// Native _CFNonObjCArrayGetCount, 0x1c014c0b0. Hidden (was a private external). The count, without dispatch.
OCFIndex _CFNonObjCArrayGetCount(OCFArrayRef array) {
    return array->count;
}

// Native CFArrayContainsValue, 0x1c014c1a4. External. Values are read with CFArrayGetValueAtIndex and matched
// by identity or the equal callback (the CFType callbacks for an Objective-C array).
Boolean OCFArrayContainsValue(OCFArrayRef theArray, OCFRange range, const void *value) {
    const OCFArrayCallBacks *cb =
        isObjCArray(theArray) ? &kOCFTypeArrayCallBacks : __CFArrayGetCallBacks(theArray);
    for (OCFIndex idx = 0; idx < range.length; idx++) {
        const void *item = OCFArrayGetValueAtIndex(theArray, range.location + idx);
        if (value == item || (cb->equal != NULL && cb->equal(value, item))) {
            return true;
        }
    }
    return false;
}

// Native _CFArrayCheckAndGetValueAtIndex, 0x1c014c28c. External. An index below 0 or not below the count sets
// *outOfBounds (when not NULL) and answers -1 as a pointer. Otherwise the value is read from the deque for a
// nonzero storage type and from the inline values for type 0; the info word is read once more beforehand and
// discarded (LDAR XZR), as natively.
const void *_OCFArrayCheckAndGetValueAtIndex(OCFArrayRef array, OCFIndex index, Boolean *outOfBounds) {
    if (index < 0 || index >= array->count) {
        if (outOfBounds != NULL) {
            *outOfBounds = true;
        }
        return (const void *)(intptr_t)-1;
    }
    (void)arrayInfo(array);
    const void **buckets;
    if ((arrayInfo(array) & 3) != 0) {
        buckets = array->store->buckets + array->store->leftIdx;
    } else {
        buckets = (const void **)((uint8_t *)array + ((~arrayInfo(array) & 0xC) == 0 ? 88 : 48));
    }
    return buckets[index];
}

// Native CFArrayGetValues, 0x1c014c2fc. External. Objective-C arrays answer -getObjects:range:; CF arrays
// copy with memmove from the immutable or deque buckets (nothing for another storage type).
void OCFArrayGetValues(OCFArrayRef theArray, OCFRange range, const void **values) {
    if (isObjCArray(theArray)) {
        ((void (*)(OCFArrayRef, SEL, const void **, OCFRange))objc_msgSend)(
            theArray, sel_getUid("getObjects:range:"), values, range
        );
        return;
    }
    if (range.length > 0) {
        switch (arrayType(theArray)) {
            case kArrayImmutable:
            case kArrayDeque:
                memmove(
                    values,
                    arrayBuckets(theArray) + range.location,
                    (size_t)range.length * sizeof(const void *)
                );
                break;
            default:
                break;
        }
    }
}

// Native __CFArrayFastEnumeration.const_mu, 0x1c03212a0 (__TEXT,__const). Non-external; the constant
// mutation count of immutable arrays (1).
static const unsigned long __CFArrayFastEnumerationConstMutations = 1;

// Native _CFArrayFastEnumeration, 0x1c014c3d0. External. An empty array, a storage type other than 0 or 2, or
// a state other than 0 produces nothing. The first call sets the state to 1 and points the mutations pointer
// at the constant 1 (immutable) or the mutation count (deque); the items pointer is set to the values (by the
// storage type read again; NULL for another type) and the whole count is returned.
unsigned long
_OCFArrayFastEnumeration(OCFArrayRef array, void *state, void *stackbuffer, unsigned long count) {
    (void)stackbuffer;
    (void)count;
    unsigned long *enumerationState = (unsigned long *)state;
    if (array->count == 0) {
        return 0;
    }
    uint32_t type = arrayType(array);
    if (type == kArrayDeque) {
        if (enumerationState[0] != 0) {
            return 0;
        }
        enumerationState[0] = 1;
        enumerationState[2] = (unsigned long)&array->mutations;
    } else {
        if (type != kArrayImmutable || enumerationState[0] != 0) {
            return 0;
        }
        enumerationState[0] = 1;
        enumerationState[2] = (unsigned long)&__CFArrayFastEnumerationConstMutations;
    }
    enumerationState[1] = (unsigned long)arrayBuckets(array);
    return (unsigned long)array->count;
}

// Native CFArrayApplyFunction, 0x1c014c4a4. External. No dispatch check; each value comes from
// CFArrayGetValueAtIndex.
void OCFArrayApplyFunction(
    OCFArrayRef theArray, OCFRange range, OCFArrayApplierFunction applier, void *context
) {
    for (OCFIndex idx = 0; idx < range.length; idx++) {
        applier(OCFArrayGetValueAtIndex(theArray, range.location + idx), context);
    }
}

// Native CFArrayApply, 0x1c014c508. Hidden (was private external). The block receives each value and a stop
// flag cleared before every call.
void CFArrayApply(OCFArrayRef array, OCFRange range, void (^block)(const void *value, bool *stop)) {
    for (OCFIndex idx = 0; idx < range.length; idx++) {
        const void *value = OCFArrayGetValueAtIndex(array, range.location + idx);
        bool stop = false;
        block(value, &stop);
        if (stop) {
            break;
        }
    }
}

// Native CFArrayGetFirstIndexOfValue, 0x1c014c5ac. External. As CFArrayContainsValue; -1 when absent.
OCFIndex OCFArrayGetFirstIndexOfValue(OCFArrayRef theArray, OCFRange range, const void *value) {
    const OCFArrayCallBacks *cb =
        isObjCArray(theArray) ? &kOCFTypeArrayCallBacks : __CFArrayGetCallBacks(theArray);
    for (OCFIndex idx = 0; idx < range.length; idx++) {
        const void *item = OCFArrayGetValueAtIndex(theArray, range.location + idx);
        if (value == item || (cb->equal != NULL && cb->equal(value, item))) {
            return range.location + idx;
        }
    }
    return -1;
}

// Native _CFArrayReplaceValues, 0x1c014c7a8. External. New values are retained first (into a 256-value stack
// buffer, or above 256 values a kCFAllocatorSystemDefault block of malloc type 0x80040B8603338 named "CFArray
// (temp)"); the mutation count is incremented and the replaced values released. Without a store, a deque for
// the future count is created (rounded capacity, centered, malloc type 0x1000040451B5BE8, no NULL check as
// natively). With a store and a changed count, the deque regions are repositioned inline
// (__CFArrayRepositionDequeRegions has no native symbol): with a 32-bit future count above the capacity, or a
// growing count and fewer than max(capacity >> 17, 4) free buckets, everything moves to a new deque
// recentered for that slack; otherwise the right part moves when it is the smaller one and the growth fits on
// the right (or the count shrinks), the left part moves when the count shrinks or the growth fits on the left
// and the left part is not larger, and both recenter otherwise, the vacated buckets cleared. The new values
// are then copied in; a missing store here is _CFArrayReplaceValues.cold.1 (0x1c0305f40, "CFArray expectation
// failed", BRK #1). The native code zeroes the first 512 bytes of the stack buffer before use. CoreFoundation
// defect: the native capacity test compares the capacity with the future count truncated to 32 bits (CMP X8,
// W24, UXTW), so a future count of 2^32 or more can take a repositioning path that writes past the deque;
// confidence: verified (disassembly). The restoration also reallocates when the capacity is below the full
// future count; counts below 2^32 behave natively.
void _OCFArrayReplaceValues(
    OCFMutableArrayRef array, OCFRange range, const void **newValues, OCFIndex newCount
) {
    const void *buffer[256];
    memset(buffer, 0, 512);
    OCFIndex cnt = array->count;
    const OCFArrayCallBacks *cb = __CFArrayGetCallBacks(array);
    OCFAllocatorRef allocator = arrayAllocator(array);
    OCFIndex futureCnt = cnt - range.length + newCount;
    const void **newv = newValues;
    if (cb->retain != NULL) {
        if (newCount > 256) {
            newv = OCFAllocatorAllocateTyped(
                kOCFAllocatorSystemDefault,
                (OCFIndex)((uint64_t)newCount * sizeof(const void *)),
                0x80040B8603338,
                0
            );
            if (newv != buffer && __OCFOASafe) {
                __OCFSetLastAllocationEventName(newv, "CFArray (temp)");
            }
        } else {
            newv = buffer;
        }
        for (OCFIndex idx = 0; idx < newCount; idx++) {
            newv[idx] = cb->retain(allocator, newValues[idx]);
        }
    }
    array->mutations++;
    if (range.length > 0) {
        __CFArrayReleaseValues(array, range, false);
    }
    ArrayDeque *deque = array->store;
    if (deque == NULL) {
        if (futureCnt >= 0) {
            OCFIndex capacity = dequeRoundUpCapacity(futureCnt);
            OCFIndex size = (OCFIndex)((uint64_t)capacity * sizeof(const void *) + sizeof(ArrayDeque));
            deque = OCFAllocatorAllocateTyped(allocator, size, 0x1000040451B5BE8, 0);
            if (__OCFOASafe == 1) {
                __OCFSetLastAllocationEventName(deque, "CFArray (store-deque)");
            }
            deque->leftIdx = (capacity - newCount) / 2;
            deque->capacity = capacity;
            array->store = deque;
        }
    } else if (range.length != newCount) {
        const void **buckets = deque->buckets;
        OCFIndex currentCnt = array->count;
        OCFIndex repositionedCnt = currentCnt - range.length + newCount;
        OCFIndex L = deque->leftIdx;
        OCFIndex A = range.location;
        OCFIndex B = currentCnt - range.location - range.length;
        OCFIndex C = range.length;
        OCFIndex numNewElems = newCount - C;
        OCFIndex wiggle = (OCFIndex)((uint64_t)deque->capacity >> 17);
        if ((uint64_t)wiggle <= 4) {
            wiggle = 4;
        }
        if ((uint64_t)deque->capacity < (uint64_t)(uint32_t)repositionedCnt ||
            deque->capacity < repositionedCnt ||
            (currentCnt < repositionedCnt && deque->capacity - currentCnt < wiggle)) {
            OCFIndex capacity = dequeRoundUpCapacity(repositionedCnt + wiggle);
            OCFIndex size = (OCFIndex)((uint64_t)capacity * sizeof(const void *) + sizeof(ArrayDeque));
            OCFAllocatorRef dequeAllocator = arrayAllocator(array);
            ArrayDeque *newDeque = OCFAllocatorAllocateTyped(dequeAllocator, size, 0x1000040451B5BE8, 0);
            if (__OCFOASafe == 1) {
                __OCFSetLastAllocationEventName(newDeque, "CFArray (store-deque)");
            }
            OCFIndex newL = (capacity - repositionedCnt) / 2;
            newDeque->leftIdx = newL;
            newDeque->capacity = capacity;
            const void **newBuckets = newDeque->buckets + newL;
            if (A > 0) {
                memmove(newBuckets, buckets + L, (size_t)A * sizeof(const void *));
            }
            if (B > 0) {
                memmove(newBuckets + A + newCount, buckets + L + A + C, (size_t)B * sizeof(const void *));
            }
            array->store = newDeque;
            OCFAllocatorDeallocate(dequeAllocator, deque);
        } else {
            OCFIndex R = deque->capacity - currentCnt - L;
            if ((numNewElems <= R || numNewElems < 0) && B < A) {
                const void **destination = buckets + L + A + newCount;
                if (B > 0) {
                    memmove(destination, buckets + L + A + C, (size_t)B * sizeof(const void *));
                }
                if (C - newCount > 0) {
                    bzero(destination + B, (size_t)(C - newCount) * sizeof(const void *));
                }
            } else if (numNewElems < 0 || !(L < numNewElems || B < A)) {
                OCFIndex newL = L - numNewElems;
                deque->leftIdx = newL;
                if (A > 0) {
                    memmove(buckets + newL, buckets + L, (size_t)A * sizeof(const void *));
                }
                if (numNewElems < 0) {
                    bzero(buckets + L, (size_t)-numNewElems * sizeof(const void *));
                }
            } else {
                OCFIndex slack = deque->capacity - currentCnt - numNewElems;
                OCFIndex newL = slack / 2 - slack / 4;
                OCFIndex oldC0 = L + A + C;
                OCFIndex newC0 = newL + A + newCount;
                deque->leftIdx = newL;
                if (newL < L) {
                    if (A > 0) {
                        memmove(buckets + newL, buckets + L, (size_t)A * sizeof(const void *));
                    }
                    if (B > 0) {
                        memmove(buckets + newC0, buckets + oldC0, (size_t)B * sizeof(const void *));
                    }
                    if (oldC0 - newC0 > 0) {
                        bzero(buckets + newC0 + B, (size_t)(oldC0 - newC0) * sizeof(const void *));
                    }
                } else {
                    if (B > 0) {
                        memmove(buckets + newC0, buckets + oldC0, (size_t)B * sizeof(const void *));
                    }
                    if (A > 0) {
                        memmove(buckets + newL, buckets + L, (size_t)A * sizeof(const void *));
                    }
                    if (newL - L > 0) {
                        bzero(buckets + L, (size_t)(newL - L) * sizeof(const void *));
                    }
                }
            }
        }
    }
    if (newCount > 0) {
        ArrayDeque *store = array->store;
        if (store == NULL) {
            crashWithMessage("CFArray expectation failed");
        }
        memmove(
            store->buckets + store->leftIdx + range.location, newv, (size_t)newCount * sizeof(const void *)
        );
    }
    array->count = futureCnt;
    if (newv != buffer && newv != newValues) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, newv);
    }
}

// Native CFArraySetValueAtIndex, 0x1c014cde4. External. Objective-C arrays answer -setObject:atIndex:; the
// index equal to the count appends through _CFArrayReplaceValues; otherwise the new value is retained,
// stored, the old one released, and the mutation count incremented.
void OCFArraySetValueAtIndex(OCFMutableArrayRef theArray, OCFIndex idx, const void *value) {
    if (isObjCArray(theArray)) {
        ((void (*)(OCFMutableArrayRef, SEL, const void *, OCFIndex))objc_msgSend)(
            theArray, sel_getUid("setObject:atIndex:"), value, idx
        );
        return;
    }
    if (idx == theArray->count) {
        _OCFArrayReplaceValues(theArray, (OCFRange){idx, 0}, &value, 1);
        return;
    }
    const OCFArrayCallBacks *cb = __CFArrayGetCallBacks(theArray);
    OCFAllocatorRef allocator = arrayAllocator(theArray);
    const void **bucket = arrayBucketAtIndex(theArray, idx);
    const void *newValue = value;
    if (cb->retain != NULL) {
        newValue = cb->retain(allocator, value);
    }
    const void *old = *bucket;
    *bucket = newValue;
    if (cb->release != NULL) {
        cb->release(allocator, old);
    }
    theArray->mutations++;
}

// Native CFArrayInsertValueAtIndex, 0x1c014cfd0. External. Objective-C arrays answer -insertObject:atIndex:.
void OCFArrayInsertValueAtIndex(OCFMutableArrayRef theArray, OCFIndex idx, const void *value) {
    if (isObjCArray(theArray)) {
        ((void (*)(OCFMutableArrayRef, SEL, const void *, OCFIndex))objc_msgSend)(
            theArray, sel_getUid("insertObject:atIndex:"), value, idx
        );
        return;
    }
    _OCFArrayReplaceValues(theArray, (OCFRange){idx, 0}, &value, 1);
}

// Native CFArrayExchangeValuesAtIndices, 0x1c014d098. External. Objective-C arrays answer
// -exchangeObjectAtIndex:withObjectAtIndex:.
void OCFArrayExchangeValuesAtIndices(OCFMutableArrayRef theArray, OCFIndex idx1, OCFIndex idx2) {
    if (isObjCArray(theArray)) {
        ((void (*)(OCFMutableArrayRef, SEL, OCFIndex, OCFIndex))objc_msgSend)(
            theArray, sel_getUid("exchangeObjectAtIndex:withObjectAtIndex:"), idx1, idx2
        );
        return;
    }
    const void **bucket1 = arrayBucketAtIndex(theArray, idx1);
    const void **bucket2 = arrayBucketAtIndex(theArray, idx2);
    const void *tmp = *bucket1;
    *bucket1 = *bucket2;
    *bucket2 = tmp;
    theArray->mutations++;
}

// Native CFArrayRemoveValueAtIndex, 0x1c014d1bc. External. Objective-C arrays answer -removeObjectAtIndex:.
void OCFArrayRemoveValueAtIndex(OCFMutableArrayRef theArray, OCFIndex idx) {
    if (isObjCArray(theArray)) {
        ((void (*)(OCFMutableArrayRef, SEL, OCFIndex))objc_msgSend)(
            theArray, sel_getUid("removeObjectAtIndex:"), idx
        );
        return;
    }
    _OCFArrayReplaceValues(theArray, (OCFRange){idx, 1}, NULL, 0);
}

// Native CFArrayRemoveAllValues, 0x1c014d218. External. Objective-C arrays answer -removeAllObjects.
void OCFArrayRemoveAllValues(OCFMutableArrayRef theArray) {
    if (isObjCArray(theArray)) {
        ((void (*)(OCFMutableArrayRef, SEL))objc_msgSend)(theArray, sel_getUid("removeAllObjects"));
        return;
    }
    __CFArrayReleaseValues(theArray, (OCFRange){0, theArray->count}, true);
    theArray->count = 0;
    theArray->mutations++;
}

// Native __CFArrayReleaseValues, 0x1c014d278. Non-external. Immutable arrays release and clear the range only
// with a release callback; deque arrays release (when there is a callback) and clear it when there is a
// store, and free the store and empty the array when asked to and the range is the whole array.
static void __CFArrayReleaseValues(OCFArrayRef array, OCFRange range, bool releaseStorageIfPossible) {
    const OCFArrayCallBacks *cb = __CFArrayGetCallBacks(array);
    switch (arrayType(array)) {
        case kArrayImmutable:
            if (cb->release != NULL && range.length > 0) {
                const void **buckets = arrayBuckets(array);
                OCFAllocatorRef allocator = arrayAllocator(array);
                for (OCFIndex idx = 0; idx < range.length; idx++) {
                    cb->release(allocator, buckets[idx + range.location]);
                }
                bzero(buckets + range.location, (size_t)range.length * sizeof(const void *));
            }
            break;
        case kArrayDeque: {
            ArrayDeque *deque = array->store;
            if (range.length > 0 && deque != NULL) {
                const void **buckets = arrayBuckets(array);
                if (cb->release != NULL) {
                    OCFAllocatorRef allocator = arrayAllocator(array);
                    for (OCFIndex idx = 0; idx < range.length; idx++) {
                        cb->release(allocator, buckets[idx + range.location]);
                    }
                }
                bzero(buckets + range.location, (size_t)range.length * sizeof(const void *));
            }
            if (releaseStorageIfPossible && range.location == 0 && array->count == range.length) {
                OCFAllocatorRef allocator = arrayAllocator(array);
                if (deque != NULL) {
                    OCFAllocatorDeallocate(allocator, deque);
                }
                ((struct __OCFArray *)array)->count = 0;
                ((struct __OCFArray *)array)->store = NULL;
            }
            break;
        }
        default:
            break;
    }
}

// Native _CFArrayIsMutable, 0x1c014d528. External. Reads the info word of any object it is given:
// CFArrayCreateCopy also asks it of Objective-C arrays, whose second word is then tested (native behavior
// kept).
Boolean _OCFArrayIsMutable(OCFArrayRef array) {
    return (arrayInfo(array) & 3) != 0;
}

// Native CFArrayReplaceValues, 0x1c014d53c. External. Objective-C arrays answer
// -replaceObjectsInRange:withObjects:count:.
void OCFArrayReplaceValues(
    OCFMutableArrayRef theArray, OCFRange range, const void **newValues, OCFIndex newCount
) {
    if (isObjCArray(theArray)) {
        ((void (*)(OCFMutableArrayRef, SEL, OCFRange, const void **, OCFIndex))objc_msgSend)(
            theArray, sel_getUid("replaceObjectsInRange:withObjects:count:"), range, newValues, newCount
        );
        return;
    }
    _OCFArrayReplaceValues(theArray, range, newValues, newCount);
}

// The comparator context of CFArraySortValues: the comparator and its context.
typedef struct {
    OCFComparatorFunction func;
    void *context;
} ArrayCompareContext;

// Native __CFArrayCompareValues, 0x1c014d5c8. Non-external; the CFQSortArray comparator over value slots.
static OCFComparisonResult __CFArrayCompareValues(const void *val1, const void *val2, void *context) {
    ArrayCompareContext *ctx = context;
    return ctx->func(*(const void *const *)val1, *(const void *const *)val2, ctx->context);
}

// Native CFArraySortValues, 0x1c014d5d8. External. Mutability is -isKindOfClass: NSMutableArray for
// Objective-C arrays, a non-zero storage type for CF arrays; the callbacks are the CFType ones for
// Objective-C arrays. A mutable array with exactly one of retain and release is bubble-sorted in place
// through CFArrayGetValueAtIndex and CFArrayExchangeValuesAtIndices (exchanging when comparator(next,
// current) is negative). Otherwise, from two values on, the values are copied out (a 256-value stack buffer,
// or a kCFAllocatorSystemDefault block of malloc type 0x80040B8603338), sorted with CFQSortArray and, when
// mutable, written back with CFArrayReplaceValues; an immutable array is sorted only in the copy. The native
// code zeroes the first 512 bytes of the stack buffer.
void OCFArraySortValues(
    OCFMutableArrayRef theArray, OCFRange range, OCFComparatorFunction comparator, void *context
) {
    bool isMutable;
    if (isObjCArray(theArray)) {
        isMutable = objc_opt_isKindOfClass((id)theArray, objc_opt_class((id)mutableArrayClass));
    } else {
        isMutable = (arrayInfo(theArray) & 3) != 0;
    }
    const OCFArrayCallBacks *cb =
        isObjCArray(theArray) ? &kOCFTypeArrayCallBacks : __CFArrayGetCallBacks(theArray);
    if (isMutable &&
        ((cb->retain != NULL && cb->release == NULL) || (cb->retain == NULL && cb->release != NULL))) {
        for (OCFIndex length = range.length; length >= 2; length--) {
            for (OCFIndex idx = range.location; idx < range.location - 1 + length; idx++) {
                const void *current = OCFArrayGetValueAtIndex(theArray, idx);
                const void *next = OCFArrayGetValueAtIndex(theArray, idx + 1);
                if (comparator(next, current, context) < 0) {
                    OCFArrayExchangeValuesAtIndices(theArray, idx, idx + 1);
                }
            }
        }
        return;
    }
    if (range.length < 2) {
        return;
    }
    const void *buffer[256];
    memset(buffer, 0, 512);
    const void **values = buffer;
    if ((uint64_t)range.length > 256) {
        values = OCFAllocatorAllocateTyped(
            kOCFAllocatorSystemDefault,
            (OCFIndex)((uint64_t)range.length * sizeof(const void *)),
            0x80040B8603338,
            0
        );
    }
    OCFArrayGetValues(theArray, range, values);
    ArrayCompareContext ctx = {comparator, context};
    OCFQSortArray(values, range.length, sizeof(const void *), __CFArrayCompareValues, &ctx);
    if (isMutable) {
        OCFArrayReplaceValues(theArray, range, values, range.length);
    }
    if (values != buffer) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, values);
    }
}

// Native CFArrayBSearchValues, 0x1c014d8bc. External. Values outside the range's first and last bound answer
// the range's end or start; otherwise a power-of-two probe sequence answers the index of the first value not
// less than the given one. The probe offsets are 32-bit (LSL W28, W26, W27), as natively: shifts are taken
// modulo 32.
OCFIndex OCFArrayBSearchValues(
    OCFArrayRef theArray, OCFRange range, const void *value, OCFComparatorFunction comparator, void *context
) {
    if (range.length <= 0) {
        return range.location;
    }
    const void *item = OCFArrayGetValueAtIndex(theArray, range.location + range.length - 1);
    if (comparator(item, value, context) < 0) {
        return range.location + range.length;
    }
    item = OCFArrayGetValueAtIndex(theArray, range.location);
    if (comparator(value, item, context) < 0) {
        return range.location;
    }
    int32_t lg = flsl(range.length) - 1;
    int32_t step = (int32_t)((uint32_t)1 << ((uint32_t)lg & 31));
    item = OCFArrayGetValueAtIndex(theArray, range.location + -1 + step);
    OCFIndex idx = comparator(item, value, context) < 0 ? range.length - step : -1;
    while (lg-- != 0) {
        step = (int32_t)((uint32_t)1 << ((uint32_t)lg & 31));
        item = OCFArrayGetValueAtIndex(theArray, range.location + idx + step);
        if (comparator(item, value, context) < 0) {
            idx += step;
        }
    }
    idx++;
    return idx + range.location;
}

// Native CFArrayAppendArray, 0x1c014d9e4. External. Each value of the other range is read with
// CFArrayGetValueAtIndex and appended with CFArrayAppendValue.
void OCFArrayAppendArray(OCFMutableArrayRef theArray, OCFArrayRef otherArray, OCFRange otherRange) {
    for (OCFIndex idx = otherRange.location; idx < otherRange.location + otherRange.length; idx++) {
        OCFArrayAppendValue(theArray, OCFArrayGetValueAtIndex(otherArray, idx));
    }
}

// Native _CFCreateArrayStorage, 0x1c015b2ec, with .cold.1 (0x1c03061ec, through OUTLINED_FUNCTION_1,
// 0x1c0160e68) and .cold.2 (0x1c03061fc). External. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json. Natively it heads the __NSArrayM unit
// (between -[NSTaggedPointerString uppercaseStringWithLocale:] and -[__NSArrayM count]; its .cold fragments
// sit before those of -[__NSArrayM _mutate]); that Objective-C unit is not restored on this branch, so the
// function joins the array family here (confidence: medium for the placement).
// A count of 2^61 or more crashes "failed to create array buffer"; malloc_good_size(count * 8) of 2^32 or
// more crashes "size unreasonably big". The block is malloc_type_calloc(1, size) when zeroed (tested as 32
// bits), else malloc_type_malloc(size), both with type 0x80040B8603338; size / 8 pointers are stored in
// *outCount before the NULL check, which crashes "failed to create array buffer".
void *_OCFCreateArrayStorage(size_t numPointers, Boolean zeroed, size_t *outCount) {
    if ((OF_WORD_SHIFT(numPointers, 61)) != 0) {
        crashWithMessage("failed to create array buffer");
    }
    size_t size = malloc_good_size(numPointers << 3);
    if (((uint64_t)size >> 32) != 0) {
        crashWithMessage("size unreasonably big");
    }
    void *storage;
    if (zeroed) {
        storage = malloc_type_calloc(1, size, 0x80040B8603338ULL);
    } else {
        storage = malloc_type_malloc(size, 0x80040B8603338ULL);
    }
    *outCount = size / sizeof(const void *);
    if (storage == NULL) {
        crashWithMessage("failed to create array buffer");
    }
    return storage;
}

// Native __FoundationPresent, inlined into the four creation entry points below: a dispatch_once (the inline
// predicate test, then OUTLINED_FUNCTION_0_20 at 0x1c01cd9c8 and the CFArrayCreate*.cold.1 thunks at
// 0x1c030b128-0x1c030b134) running ____FoundationPresent_block_invoke (0x1c01cd99c, non-external), which
// records whether objc_lookUpClass("NSXMLParser") answers a class. Statics: __FoundationPresent.static_init
// (0x22e6d2e30) and __FoundationPresent.present (0x22e6d2e28), one pair per native translation unit. The
// block invoke is also listed in another sub-track's task; it is part of this translation unit natively.
static bool __FoundationPresent(void) {
    static dispatch_once_t static_init;
    static bool present;
    dispatch_once(&static_init, ^{
      present = objc_lookUpClass("NSXMLParser") != NULL;
    });
    return present;
}

// _CFAllocatorIsSystemDefault, inlined into the creation entry points below: the system default allocator, or
// NULL or kCFAllocatorDefault while the default allocator is the system default.
static bool isSystemDefaultAllocator(OCFAllocatorRef allocator) {
    if (allocator == kOCFAllocatorSystemDefault) {
        return true;
    }
    if (allocator == NULL || allocator == kOCFAllocatorDefault) {
        return kOCFAllocatorSystemDefault == OCFAllocatorGetDefault();
    }
    return false;
}

// Native CFArrayCreateCopy, 0x1c01cd544. External. Objective-C sources, and CFType-callback CF sources with
// the system default allocator while Foundation is present, are copied by Foundation: -[NSArray
// initWithArray:copyItems:NO] when _CFArrayIsMutable answers true, else -copyWithZone:NULL. Otherwise
// __CFArrayCreateCopy0.
OCFArrayRef OCFArrayCreateCopy(OCFAllocatorRef allocator, OCFArrayRef theArray) {
    if (isObjCArray(theArray) || (__CFArrayGetCallBacks(theArray) == &kOCFTypeArrayCallBacks &&
                                  isSystemDefaultAllocator(allocator) && __FoundationPresent())) {
        if (_OCFArrayIsMutable(theArray)) {
            return ((OCFArrayRef (*)(id, SEL, OCFArrayRef, BOOL))objc_msgSend)(
                objc_alloc((Class)arrayClass), sel_getUid("initWithArray:copyItems:"), theArray, NO
            );
        }
        return ((OCFArrayRef (*)(OCFArrayRef, SEL, void *))objc_msgSend)(
            theArray, sel_getUid("copyWithZone:"), NULL
        );
    }
    return __CFArrayCreateCopy0(allocator, theArray);
}

// Native CFArrayCreateMutableCopy, 0x1c01cd69c. External. As CFArrayCreateCopy, with -[NSMutableArray
// initWithArray:copyItems:NO] (the capacity is not passed) or __CFArrayCreateMutableCopy0.
// Folds the compiler-outlined piece _CFArrayCreateMutableCopy.cold.1 (0x1c030b12c).
OCFMutableArrayRef
OCFArrayCreateMutableCopy(OCFAllocatorRef allocator, OCFIndex capacity, OCFArrayRef theArray) {
    if (isObjCArray(theArray) || (__CFArrayGetCallBacks(theArray) == &kOCFTypeArrayCallBacks &&
                                  isSystemDefaultAllocator(allocator) && __FoundationPresent())) {
        return ((OCFMutableArrayRef (*)(id, SEL, OCFArrayRef, BOOL))objc_msgSend)(
            objc_alloc((Class)mutableArrayClass), sel_getUid("initWithArray:copyItems:"), theArray, NO
        );
    }
    return __CFArrayCreateMutableCopy0(allocator, capacity, theArray);
}

// Native CFArrayCreate, 0x1c01cd784. External. The CFType callbacks with the system default allocator while
// Foundation is present create -[NSArray initWithObjects:count:]; otherwise __CFArrayCreate0.
// Folds the compiler-outlined piece _CFArrayCreate.cold.1 (0x1c030b130).
OCFArrayRef OCFArrayCreate(
    OCFAllocatorRef allocator, const void **values, OCFIndex numValues, const OCFArrayCallBacks *callBacks
) {
    if (callBacks == &kOCFTypeArrayCallBacks && isSystemDefaultAllocator(allocator) &&
        __FoundationPresent()) {
        return ((OCFArrayRef (*)(id, SEL, const void **, OCFIndex))objc_msgSend)(
            objc_alloc((Class)arrayClass), sel_getUid("initWithObjects:count:"), values, numValues
        );
    }
    return __CFArrayCreate0(allocator, values, numValues, callBacks);
}

// Native CFArrayCreateMutable, 0x1c01cd860. External. As CFArrayCreate, with -[NSMutableArray
// initWithCapacity:0] (the capacity is not passed) or __CFArrayCreateMutable0.
OCFMutableArrayRef
OCFArrayCreateMutable(OCFAllocatorRef allocator, OCFIndex capacity, const OCFArrayCallBacks *callBacks) {
    if (callBacks == &kOCFTypeArrayCallBacks && isSystemDefaultAllocator(allocator) &&
        __FoundationPresent()) {
        return ((OCFMutableArrayRef (*)(id, SEL, OCFIndex))objc_msgSend)(
            objc_alloc((Class)mutableArrayClass), sel_getUid("initWithCapacity:"), 0
        );
    }
    return __CFArrayCreateMutable0(allocator, capacity, callBacks);
}

// Native __CFArrayHandleOutOfMemory, 0x1c0305eec. Non-external. Raises a memory exception with the message
// "Attempt to allocate %ld bytes for CFArray failed" (created with kCFAllocatorSystemDefault, never
// released).
static void __CFArrayHandleOutOfMemory(OCFIndex numBytes) {
    OCFStringRef msg = OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault, NULL, OF_STR("Attempt to allocate %ld bytes for CFArray failed"), numBytes
    );
    _CFRaiseMemoryException(msg);
}
