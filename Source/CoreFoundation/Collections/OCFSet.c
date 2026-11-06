//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFBasicHash.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/message.h>
#include <objc/runtime.h>
#include <string.h>

// Inferred organization: __CFSetGetCallbacks (0x1c01ed99c) through CFSetRemoveAllValues (0x1c01ee7a8) lie in
// one cluster of the iOS Simulator __text section and are kept in one translation unit with the set class
// record and callback structures; the native source path is not in the evidence. Functions of the cluster
// outside the scope are not restored. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; pseudocode used as a
// guide only); iOS Device 24A446: unread. Reading: not yet written; evidence
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json and iphone-simulator-corpus-delta4.json.

static void __CFSetDeallocate(OCFTypeRef cf);
static Boolean __CFSetEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFSetHash(OCFTypeRef cf);
static OCFStringRef __CFSetCopyDescription(OCFTypeRef cf);
static BasicHashRef __CFSetCreateGeneric(OCFAllocatorRef allocator, const OCFSetCallBacks *callBacks);

// Native kCFTypeSetCallBacks (0x2227a43f8) and kCFCopyStringSetCallBacks (0x2227a4428), __DATA_CONST,__const.
// External. Version 0; __CFTypeCollectionRetain (or __CFStringCollectionCopy), __CFTypeCollectionRelease,
// CFCopyDescription, CFEqual, CFHash.
const OCFSetCallBacks kOCFTypeSetCallBacks = {
    0,
    __CFTypeCollectionRetain,
    __CFTypeCollectionRelease,
    OCFCopyDescription,
    OCFEqual,
    OCFHash,
};

const OCFSetCallBacks kOCFCopyStringSetCallBacks = {
    0,
    (OCFSetRetainCallBack)__CFStringCollectionCopy,
    __CFTypeCollectionRelease,
    OCFCopyDescription,
    OCFEqual,
    OCFHash,
};

// Native __CFSetClass, 0x2227a4458 (__DATA_CONST,__const). Hidden (was private external); version 1, name
// "CFSet", finalize, equal, hash and the debug description.
const OCFRuntimeClass __CFSetClass = {
    1,
    "CFSet",
    NULL,
    NULL,
    __CFSetDeallocate,
    __CFSetEqual,
    __CFSetHash,
    NULL,
    __CFSetCopyDescription,
    NULL,
    NULL,
    0,
};

static const uint32_t kSetTypeID = 17;

// Tagged pointers and Objective-C sets go to their methods.
static bool isObjCSet(OCFTypeRef cf) {
    return (intptr_t)cf < 0 || __CF_IS_OBJC(kSetTypeID, cf) != 0;
}

static uint64_t infoOf(OCFTypeRef cf) {
    return __atomic_load_n(&((OCFRuntimeBase *)cf)->info, __ATOMIC_ACQUIRE);
}

// The immutable bit (info bit 6), set with a compare-and-swap loop as natively.
static void setImmutable(OCFTypeRef cf) {
    uint64_t *info = &((OCFRuntimeBase *)cf)->info;
    uint64_t expected = __atomic_load_n(info, __ATOMIC_ACQUIRE);
    while (!__atomic_compare_exchange_n(
        info, &expected, expected | 0x40, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    )) {
    }
}

// Native __CFSetGetCallbacks, 0x1c01ed99c. Hidden (was private external). Version 0 and the key callbacks of
// the table: retain, release, key description, equal and hash.
OCFSetCallBacks __CFSetGetCallbacks(OCFSetRef theSet) {
    BasicHashCallbacks callbacks = __CFBasicHashGetCallbacks((BasicHashRef)theSet);
    OCFSetCallBacks result;
    result.version = 0;
    result.retain = (OCFSetRetainCallBack)callbacks.retainKey;
    result.release = (OCFSetReleaseCallBack)callbacks.releaseKey;
    result.copyDescription = (OCFSetCopyDescriptionCallBack)callbacks.copyKeyDescription;
    result.equal = (OCFSetEqualCallBack)callbacks.equateKeys;
    result.hash = (OCFSetHashCallBack)callbacks.hashKey;
    return result;
}

// Native __CFSetDeallocate, 0x1c01eda20. Non-external. A tail call.
static void __CFSetDeallocate(OCFTypeRef cf) {
    __CFBasicHashDeallocate(cf);
}

// Native __CFSetEqual, 0x1c01eda24. Non-external. A tail call.
static Boolean __CFSetEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    return __CFBasicHashEqual(cf1, cf2);
}

// Native __CFSetHash, 0x1c01eda28. Non-external. A tail call.
static OCFHashCode __CFSetHash(OCFTypeRef cf) {
    return __CFBasicHashHash(cf);
}

// Native __CFSetCopyDescription, 0x1c01eda2c. Non-external. A tail call.
static OCFStringRef __CFSetCopyDescription(OCFTypeRef cf) {
    return __CFBasicHashCopyDescription(cf);
}

// Native CFSetGetTypeID, 0x1c01eda30. External. The constant 17 (the builtin class number of __CFSetClass).
OCFTypeID OCFSetGetTypeID(void) {
    return 17;
}

// Native __CFSetCreateTransfer, 0x1c01eda38. Hidden (was private external). A linear set with the CFType
// callbacks, filled with retain and release suppressed (the set takes over the references), marked immutable
// and given type 17. Unlike the dictionary, no Foundation path and no NULL check of the created table.
OCFSetRef __CFSetCreateTransfer(OCFAllocatorRef allocator, const void **values, OCFIndex count) {
    BasicHashCallbacks callbacks = {
        __CFTypeCollectionRetain,
        __CFTypeCollectionRetain,
        __CFTypeCollectionRelease,
        __CFTypeCollectionRelease,
        OCFEqual,
        OCFEqual,
        OCFHash,
        NULL,
        OCFCopyDescription,
        OCFCopyDescription,
    };
    BasicHashRef ht = CFBasicHashCreate(allocator, 0x2000, &callbacks);
    CFBasicHashSuppressRC(ht);
    if (count >= 1) {
        CFBasicHashSetCapacity(ht, count);
        do {
            uintptr_t value = (uintptr_t)*values++;
            CFBasicHashAddValue(ht, value, value);
        } while (--count != 0);
    }
    CFBasicHashUnsuppressRC(ht);
    setImmutable(ht);
    _OCFRuntimeSetInstanceTypeIDAndIsa(ht, 17);
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName(ht, "CFSet (immutable)");
    }
    return (OCFSetRef)ht;
}

// Native CFSetCreate, 0x1c01edb68. External. A generic set filled with each value as key and value, marked
// immutable and given type 17.
OCFSetRef OCFSetCreate(
    OCFAllocatorRef allocator, const void **values, OCFIndex numValues, const OCFSetCallBacks *callBacks
) {
    BasicHashRef ht = __CFSetCreateGeneric(allocator, callBacks);
    if (ht == NULL) {
        return NULL;
    }
    if (numValues >= 1) {
        CFBasicHashSetCapacity(ht, numValues);
        do {
            uintptr_t value = (uintptr_t)*values++;
            CFBasicHashAddValue(ht, value, value);
        } while (--numValues != 0);
    }
    setImmutable(ht);
    _OCFRuntimeSetInstanceTypeIDAndIsa(ht, 17);
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName(ht, "CFSet (immutable)");
    }
    return (OCFSetRef)ht;
}

// Native __CFSetCreateGeneric, 0x1c01edc1c. Non-external. A linear set (flags 0x2000) whose value and key
// callbacks are both the set's callbacks (NULL callbacks give NULL); no indirect key.
static BasicHashRef __CFSetCreateGeneric(OCFAllocatorRef allocator, const OCFSetCallBacks *callBacks) {
    OCFSetRetainCallBack retain = NULL;
    OCFSetReleaseCallBack release = NULL;
    OCFSetCopyDescriptionCallBack copyDescription = NULL;
    OCFSetEqualCallBack equal = NULL;
    OCFSetHashCallBack hash = NULL;
    if (callBacks != NULL) {
        retain = callBacks->retain;
        release = callBacks->release;
        equal = callBacks->equal;
        hash = callBacks->hash;
        copyDescription = callBacks->copyDescription;
    }
    BasicHashCallbacks callbacks = {
        retain,
        retain,
        release,
        release,
        equal,
        equal,
        hash,
        NULL,
        copyDescription,
        copyDescription,
    };
    return CFBasicHashCreate(allocator, 0x2000, &callbacks);
}

// Native CFSetCreateMutable, 0x1c01edca8. External. A generic set given type 17; the capacity is unused.
OCFMutableSetRef
OCFSetCreateMutable(OCFAllocatorRef allocator, OCFIndex capacity, const OCFSetCallBacks *callBacks) {
    (void)capacity;
    BasicHashRef ht = __CFSetCreateGeneric(allocator, callBacks);
    if (ht != NULL) {
        _OCFRuntimeSetInstanceTypeIDAndIsa(ht, 17);
        if (__OCFOASafe) {
            __OCFSetLastAllocationEventName(ht, "CFSet (mutable)");
        }
    }
    return (OCFMutableSetRef)ht;
}

// Native CFSetCreateCopy, 0x1c01edd04. External. Objective-C sets answer -copyWithZone: NULL; others a table
// copy, marked immutable and given type 17.
OCFSetRef OCFSetCreateCopy(OCFAllocatorRef allocator, OCFSetRef theSet) {
    if (isObjCSet(theSet)) {
        return ((OCFSetRef (*)(OCFTypeRef, SEL, void *))objc_msgSend)(
            theSet, sel_getUid("copyWithZone:"), NULL
        );
    }
    BasicHashRef ht = CFBasicHashCreateCopy(allocator, (BasicHashRef)theSet);
    if (ht != NULL) {
        setImmutable(ht);
        _OCFRuntimeSetInstanceTypeIDAndIsa(ht, 17);
        if (__OCFOASafe) {
            __OCFSetLastAllocationEventName(ht, "CFSet (immutable)");
        }
    }
    return (OCFSetRef)ht;
}

// Native CFSetCreateMutableCopy, 0x1c01eddb4. External. A CF set is copied (capacity unused). An Objective-C
// set's values (256 on the stack, more in a system-default buffer of type 0xC0040B8AA526D) fill a set with
// the CFType callbacks. The native code clears the first 512 bytes of the 2048-byte stack buffer before use;
// the restoration repeats that.
OCFMutableSetRef OCFSetCreateMutableCopy(OCFAllocatorRef allocator, OCFIndex capacity, OCFSetRef theSet) {
    (void)capacity;
    BasicHashRef ht;
    if (!isObjCSet(theSet)) {
        ht = CFBasicHashCreateCopy(allocator, (BasicHashRef)theSet);
        if (ht == NULL) {
            return NULL;
        }
    } else {
        OCFIndex count = OCFSetGetCount(theSet);
        const void *valuesBuffer[256];
        memset(valuesBuffer, 0, 512);
        const void **values;
        if (count >= 257) {
            values = OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, count * 8, 0xC0040B8AA526D, 0);
        } else {
            values = valuesBuffer;
        }
        OCFSetGetValues(theSet, values);
        ht = __CFSetCreateGeneric(allocator, &kOCFTypeSetCallBacks);
        if (count >= 1 && ht != NULL) {
            CFBasicHashSetCapacity(ht, count);
            for (OCFIndex index = 0; index < count; index++) {
                CFBasicHashAddValue(ht, (uintptr_t)values[index], (uintptr_t)values[index]);
            }
        }
        if (values != valuesBuffer) {
            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, values);
        }
        if (ht == NULL) {
            return NULL;
        }
    }
    _OCFRuntimeSetInstanceTypeIDAndIsa(ht, 17);
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName(ht, "CFSet (mutable)");
    }
    return (OCFMutableSetRef)ht;
}

// Native CFSetGetCount, 0x1c01edf78. External. Objective-C: -count.
OCFIndex OCFSetGetCount(OCFSetRef theSet) {
    if (isObjCSet(theSet)) {
        return ((OCFIndex (*)(OCFTypeRef, SEL))objc_msgSend)(theSet, sel_getUid("count"));
    }
    return CFBasicHashGetCount((BasicHashRef)theSet);
}

// Native CFSetGetValues, 0x1c01edfbc. External. Objective-C: -getObjects:; otherwise the keys up to
// CFSetGetCount (no value buffer).
void OCFSetGetValues(OCFSetRef theSet, const void **values) {
    if (isObjCSet(theSet)) {
        ((void (*)(OCFTypeRef, SEL, const void **))objc_msgSend)(theSet, sel_getUid("getObjects:"), values);
        return;
    }
    CFBasicHashGetElements((BasicHashRef)theSet, OCFSetGetCount(theSet), NULL, (uintptr_t *)values);
}

// Native CFSetContainsValue, 0x1c01ee06c. External. Objective-C: -containsObject:.
Boolean OCFSetContainsValue(OCFSetRef theSet, const void *value) {
    if (isObjCSet(theSet)) {
        return ((Boolean (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(
            theSet, sel_getUid("containsObject:"), value
        );
    }
    return CFBasicHashGetCountOfKey((BasicHashRef)theSet, (uintptr_t)value) > 0;
}

// Native CFSetGetValue, 0x1c01ee0c4. External. Objective-C: -member:; otherwise the found value or NULL.
const void *OCFSetGetValue(OCFSetRef theSet, const void *value) {
    if (isObjCSet(theSet)) {
        return ((const void *(*)(OCFTypeRef, SEL, const void *))objc_msgSend)(
            theSet, sel_getUid("member:"), value
        );
    }
    BasicHashBucket bucket = CFBasicHashFindBucket((BasicHashRef)theSet, (uintptr_t)value);
    return bucket.count != 0 ? (const void *)bucket.value : NULL;
}

// Native CFSetGetValueIfPresent, 0x1c01ee180. External. Objective-C: -__getValue:forObj: (value, candidate);
// otherwise a found bucket stores its value through value (when not NULL).
Boolean OCFSetGetValueIfPresent(OCFSetRef theSet, const void *candidate, const void **value) {
    if (isObjCSet(theSet)) {
        return ((Boolean (*)(OCFTypeRef, SEL, const void **, const void *))objc_msgSend)(
            theSet, sel_getUid("__getValue:forObj:"), value, candidate
        );
    }
    BasicHashBucket bucket = CFBasicHashFindBucket((BasicHashRef)theSet, (uintptr_t)candidate);
    if (bucket.count == 0) {
        return false;
    }
    if (value != NULL) {
        *value = (const void *)bucket.value;
    }
    return true;
}

// Native CFSetApplyFunction, 0x1c01ee240, with __CFSetApplyFunction_block_invoke (0x1c01ee328). External.
// Objective-C: -__applyValues:context:; otherwise the applier gets each value and the context once.
void OCFSetApplyFunction(OCFSetRef theSet, OCFSetApplierFunction applier, void *context) {
    if (isObjCSet(theSet)) {
        ((void (*)(OCFTypeRef, SEL, OCFSetApplierFunction, void *))objc_msgSend)(
            theSet, sel_getUid("__applyValues:context:"), applier, context
        );
        return;
    }
    CFBasicHashApply((BasicHashRef)theSet, ^Boolean(BasicHashBucket bucket) {
      applier((const void *)bucket.value, context);
      return true;
    });
}

// Native CFSetApply, 0x1c01ee34c, with __CFSetApply_block_invoke (0x1c01ee424). Hidden (was private
// external). Objective-C: -enumerateObjectsWithOptions:usingBlock: with options 0; otherwise the block gets
// each bucket's key and a cleared stop flag, and a set flag ends the enumeration.
void CFSetApply(OCFSetRef theSet, void (^block)(const void *value, Boolean *stop)) {
    if (isObjCSet(theSet)) {
        ((void (*)(OCFTypeRef, SEL, uintptr_t, const void *))objc_msgSend)(
            theSet, sel_getUid("enumerateObjectsWithOptions:usingBlock:"), 0, (const void *)block
        );
        return;
    }
    CFBasicHashApply((BasicHashRef)theSet, ^Boolean(BasicHashBucket bucket) {
      Boolean stop = false;
      block((const void *)bucket.key, &stop);
      return !stop;
    });
}

// Native _CFSetFastEnumeration, 0x1c01ee48c. External. 0 for Objective-C sets; otherwise the basic hash
// enumeration (a tail call).
unsigned long _OCFSetFastEnumeration(OCFSetRef theSet, void *state, void *stackbuffer, unsigned long count) {
    if (isObjCSet(theSet)) {
        return 0;
    }
    return __CFBasicHashFastEnumeration((BasicHashRef)theSet, state, stackbuffer, count);
}

// Native _CFSetIsMutable, 0x1c01ee4f4. External. False for Objective-C sets; otherwise whether info bit 6
// (immutable) is clear.
Boolean _OCFSetIsMutable(OCFSetRef theSet) {
    if (isObjCSet(theSet)) {
        return false;
    }
    return (infoOf(theSet) & 0x40) == 0;
}

// Native _CFSetSetCapacity, 0x1c01ee544. External. iOS Device divergence: unread.
void _OCFSetSetCapacity(OCFMutableSetRef theSet, OCFIndex capacity) {
    if (!isObjCSet(theSet)) {
        CFBasicHashSetCapacity((BasicHashRef)theSet, capacity);
    }
}

// Native CFSetAddValue, 0x1c01ee58c. External. Objective-C: -addObject:. An immutable set is logged at level
// 3 (the add then halts in CFBasicHashAddValue).
void OCFSetAddValue(OCFMutableSetRef theSet, const void *value) {
    if (isObjCSet(theSet)) {
        ((void (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(theSet, sel_getUid("addObject:"), value);
        return;
    }
    if ((infoOf(theSet) & 0x40) != 0) {
        OCFLog(
            3,
            OF_STR("%s(): immutable collection %p given to mutating function"),
            "void CFSetAddValue(CFMutableSetRef, const void *)",
            theSet
        );
    }
    CFBasicHashAddValue((BasicHashRef)theSet, (uintptr_t)value, (uintptr_t)value);
}

// Native CFSetSetValue, 0x1c01ee69c. External. iOS Device divergence: unread.
void OCFSetSetValue(OCFMutableSetRef theSet, const void *value) {
    if (isObjCSet(theSet)) {
        ((void (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(theSet, sel_getUid("setObject:"), value);
        return;
    }
    if ((infoOf(theSet) & 0x40) != 0) {
        OCFLog(
            3,
            OF_STR("%s(): immutable collection %p given to mutating function"),
            "void CFSetSetValue(CFMutableSetRef, const void *)",
            theSet
        );
    }
    CFBasicHashSetValue((BasicHashRef)theSet, (uintptr_t)value, (uintptr_t)value);
}

// Native CFSetRemoveValue, 0x1c01ee724. External. Objective-C: -removeObject:. An immutable set is logged at
// level 3 (the removal then halts in CFBasicHashRemoveValue).
void OCFSetRemoveValue(OCFMutableSetRef theSet, const void *value) {
    if (isObjCSet(theSet)) {
        ((void (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(theSet, sel_getUid("removeObject:"), value);
        return;
    }
    if ((infoOf(theSet) & 0x40) != 0) {
        OCFLog(
            3,
            OF_STR("%s(): immutable collection %p given to mutating function"),
            "void CFSetRemoveValue(CFMutableSetRef, const void *)",
            theSet
        );
    }
    CFBasicHashRemoveValue((BasicHashRef)theSet, (uintptr_t)value);
}

// Native CFSetRemoveAllValues, 0x1c01ee7a8. External. Objective-C: -removeAllObjects. An immutable set is
// logged at level 3 (the removal then halts in CFBasicHashRemoveAllValues).
void OCFSetRemoveAllValues(OCFMutableSetRef theSet) {
    if (isObjCSet(theSet)) {
        ((void (*)(OCFTypeRef, SEL))objc_msgSend)(theSet, sel_getUid("removeAllObjects"));
        return;
    }
    if ((infoOf(theSet) & 0x40) != 0) {
        OCFLog(
            3,
            OF_STR("%s(): immutable collection %p given to mutating function"),
            "void CFSetRemoveAllValues(CFMutableSetRef)",
            theSet
        );
    }
    CFBasicHashRemoveAllValues((BasicHashRef)theSet);
}
