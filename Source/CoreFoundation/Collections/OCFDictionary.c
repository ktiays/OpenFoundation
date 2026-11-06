//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFBasicHash.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/message.h>
#include <malloc/malloc.h>
#include <objc/runtime.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: __CFDictionaryGetKeyCallbacks (0x1c0194ae0) through CFDictionaryRemoveAllValues
// (0x1c0195f58) lie in one cluster of the iOS Simulator __text section and are kept in one translation unit
// with the dictionary class record and callback structures; the native source path is not in the evidence.
// Functions of the cluster outside the scope are not restored. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full;
// pseudocode used as a guide only); iOS Device 24A446: unread. Reading: not yet written; evidence
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json and iphone-simulator-corpus-delta4.json.

static void __CFDictionaryDeallocate(OCFTypeRef cf);
static Boolean __CFDictionaryEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFDictionaryHash(OCFTypeRef cf);
static OCFStringRef __CFDictionaryCopyDescription(OCFTypeRef cf);
static BasicHashRef __CFDictionaryCreateGeneric(
    OCFAllocatorRef allocator,
    const OCFDictionaryKeyCallBacks *keyCallBacks,
    const OCFDictionaryValueCallBacks *valueCallBacks
);

// Native kCFTypeDictionaryKeyCallBacks (0x22279d850), kCFCopyStringDictionaryKeyCallBacks (0x22279d880) and
// kCFTypeDictionaryValueCallBacks (0x22279d8b0), __DATA_CONST,__const. External. Version 0;
// __CFTypeCollectionRetain (or __CFStringCollectionCopy), __CFTypeCollectionRelease, CFCopyDescription,
// CFEqual and (keys) CFHash.
const OCFDictionaryKeyCallBacks kOCFTypeDictionaryKeyCallBacks = {
    0,
    __CFTypeCollectionRetain,
    __CFTypeCollectionRelease,
    OCFCopyDescription,
    OCFEqual,
    OCFHash,
};

const OCFDictionaryKeyCallBacks kOCFCopyStringDictionaryKeyCallBacks = {
    0,
    (OCFDictionaryRetainCallBack)__CFStringCollectionCopy,
    __CFTypeCollectionRelease,
    OCFCopyDescription,
    OCFEqual,
    OCFHash,
};

const OCFDictionaryValueCallBacks kOCFTypeDictionaryValueCallBacks = {
    0,
    __CFTypeCollectionRetain,
    __CFTypeCollectionRelease,
    OCFCopyDescription,
    OCFEqual,
};

// Native __CFDictionaryClass, 0x22279d8d8 (__DATA_CONST,__const). Hidden (was private external); version 1,
// name "CFDictionary", finalize, equal, hash and the debug description.
const OCFRuntimeClass __CFDictionaryClass = {
    1,
    "CFDictionary",
    NULL,
    NULL,
    __CFDictionaryDeallocate,
    __CFDictionaryEqual,
    __CFDictionaryHash,
    NULL,
    __CFDictionaryCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CF_KVO_WillChangeSelector_hc (0x22e6d27e8), __CF_KVO_DidChangeSelector_hc (0x22e6d27f0),
// __CF_KVO_WillChangeKeysSelector_hc (0x22e6d27f8) and __CF_KVO_DidChangeKeysSelector_hc (0x22e6d2800),
// __DATA,__bss. Non-external. Registered on first use.
static SEL __CF_KVO_WillChangeSelector_hc;
static SEL __CF_KVO_DidChangeSelector_hc;
static SEL __CF_KVO_WillChangeKeysSelector_hc;
static SEL __CF_KVO_DidChangeKeysSelector_hc;

static const uint32_t kDictionaryTypeID = 18;

// Tagged pointers and Objective-C dictionaries go to their methods.
static bool isObjCDictionary(OCFTypeRef cf) {
    return (intptr_t)cf < 0 || __CF_IS_OBJC(kDictionaryTypeID, cf) != 0;
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

// Native __CFDictionaryGetKeyCallbacks, 0x1c0194ae0. Hidden (was private external). Version 0 and the key
// callbacks of the table: retain, release, key description, equal and hash.
OCFDictionaryKeyCallBacks __CFDictionaryGetKeyCallbacks(OCFDictionaryRef theDict) {
    BasicHashCallbacks callbacks = __CFBasicHashGetCallbacks((BasicHashRef)theDict);
    OCFDictionaryKeyCallBacks result;
    result.version = 0;
    result.retain = (OCFDictionaryRetainCallBack)callbacks.retainKey;
    result.release = (OCFDictionaryReleaseCallBack)callbacks.releaseKey;
    result.copyDescription = (OCFDictionaryCopyDescriptionCallBack)callbacks.copyKeyDescription;
    result.equal = (OCFDictionaryEqualCallBack)callbacks.equateKeys;
    result.hash = (OCFDictionaryHashCallBack)callbacks.hashKey;
    return result;
}

// Native __CFDictionaryGetValueCallbacks, 0x1c0194b64. Hidden (was private external). Version 0 and the value
// callbacks of the table: retain, release, value description and equal.
OCFDictionaryValueCallBacks __CFDictionaryGetValueCallbacks(OCFDictionaryRef theDict) {
    BasicHashCallbacks callbacks = __CFBasicHashGetCallbacks((BasicHashRef)theDict);
    OCFDictionaryValueCallBacks result;
    result.version = 0;
    result.retain = (OCFDictionaryRetainCallBack)callbacks.retainValue;
    result.release = (OCFDictionaryReleaseCallBack)callbacks.releaseValue;
    result.copyDescription = (OCFDictionaryCopyDescriptionCallBack)callbacks.copyValueDescription;
    result.equal = (OCFDictionaryEqualCallBack)callbacks.equateValues;
    return result;
}

// Native __CFDictionaryDeallocate, 0x1c0194be8. Non-external. A tail call.
static void __CFDictionaryDeallocate(OCFTypeRef cf) {
    __CFBasicHashDeallocate(cf);
}

// Native __CFDictionaryEqual, 0x1c0194bec. Non-external. A tail call.
static Boolean __CFDictionaryEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    return __CFBasicHashEqual(cf1, cf2);
}

// Native __CFDictionaryHash, 0x1c0194bf0. Non-external. A tail call.
static OCFHashCode __CFDictionaryHash(OCFTypeRef cf) {
    return __CFBasicHashHash(cf);
}

// Native __CFDictionaryCopyDescription, 0x1c0194bf4. Non-external. A tail call.
static OCFStringRef __CFDictionaryCopyDescription(OCFTypeRef cf) {
    return __CFBasicHashCopyDescription(cf);
}

// Native CFDictionaryGetTypeID, 0x1c0194bf8. External.
OCFTypeID OCFDictionaryGetTypeID(void) {
    return 18;
}

// Native __CFDictionaryCreateTransfer, 0x1c0194c00. Hidden (was private external). The Foundation path first;
// otherwise a keyed, linear table with the CFType callbacks, filled with retain and release suppressed (the
// table takes over the references), marked immutable and given type 18.
OCFDictionaryRef __CFDictionaryCreateTransfer(
    OCFAllocatorRef allocator, const void **keys, const void **values, OCFIndex count
) {
    OCFDictionaryRef result = __NSCFDictionaryCreateTransfer(allocator, keys, values, count);
    if (result != NULL) {
        return result;
    }
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
    BasicHashRef ht = CFBasicHashCreate(allocator, 0x2001, &callbacks);
    CFBasicHashSuppressRC(ht);
    if (count >= 1) {
        CFBasicHashSetCapacity(ht, count);
        do {
            uintptr_t key = (uintptr_t)*keys++;
            uintptr_t value = (uintptr_t)*values++;
            CFBasicHashAddValue(ht, key, value);
        } while (--count != 0);
    }
    CFBasicHashUnsuppressRC(ht);
    setImmutable(ht);
    _OCFRuntimeSetInstanceTypeIDAndIsa(ht, 18);
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName(ht, "CFDictionary (immutable)");
    }
    return (OCFDictionaryRef)ht;
}

// Native CFDictionaryCreate, 0x1c0194d50. External. The Foundation path first (same arguments); otherwise a
// generic table filled, marked immutable and given type 18.
OCFDictionaryRef OCFDictionaryCreate(
    OCFAllocatorRef allocator,
    const void **keys,
    const void **values,
    OCFIndex numValues,
    const OCFDictionaryKeyCallBacks *keyCallBacks,
    const OCFDictionaryValueCallBacks *valueCallBacks
) {
    OCFDictionaryRef result =
        __NSCFDictionaryCreate(allocator, keys, values, numValues, keyCallBacks, valueCallBacks);
    if (result != NULL) {
        return result;
    }
    BasicHashRef ht = __CFDictionaryCreateGeneric(allocator, keyCallBacks, valueCallBacks);
    if (ht == NULL) {
        return NULL;
    }
    if (numValues >= 1) {
        CFBasicHashSetCapacity(ht, numValues);
        do {
            uintptr_t key = (uintptr_t)*keys++;
            uintptr_t value = (uintptr_t)*values++;
            CFBasicHashAddValue(ht, key, value);
        } while (--numValues != 0);
    }
    setImmutable(ht);
    _OCFRuntimeSetInstanceTypeIDAndIsa(ht, 18);
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName(ht, "CFDictionary (immutable)");
    }
    return (OCFDictionaryRef)ht;
}

// Native __CFDictionaryCreateGeneric, 0x1c0194e3c. Non-external. A keyed, linear table (flags 0x2001) whose
// value callbacks come from valueCallBacks and key callbacks (with the hash) from keyCallBacks; either NULL
// gives NULL callbacks; no indirect key.
static BasicHashRef __CFDictionaryCreateGeneric(
    OCFAllocatorRef allocator,
    const OCFDictionaryKeyCallBacks *keyCallBacks,
    const OCFDictionaryValueCallBacks *valueCallBacks
) {
    BasicHashCallbacks callbacks;
    if (keyCallBacks != NULL) {
        callbacks.retainKey = keyCallBacks->retain;
        callbacks.releaseKey = keyCallBacks->release;
        callbacks.equateKeys = keyCallBacks->equal;
        callbacks.hashKey = keyCallBacks->hash;
        callbacks.copyKeyDescription = keyCallBacks->copyDescription;
    } else {
        callbacks.retainKey = NULL;
        callbacks.releaseKey = NULL;
        callbacks.equateKeys = NULL;
        callbacks.hashKey = NULL;
        callbacks.copyKeyDescription = NULL;
    }
    callbacks.getIndirectKey = NULL;
    if (valueCallBacks != NULL) {
        callbacks.retainValue = valueCallBacks->retain;
        callbacks.releaseValue = valueCallBacks->release;
        callbacks.equateValues = valueCallBacks->equal;
        callbacks.copyValueDescription = valueCallBacks->copyDescription;
    } else {
        callbacks.retainValue = NULL;
        callbacks.releaseValue = NULL;
        callbacks.equateValues = NULL;
        callbacks.copyValueDescription = NULL;
    }
    return CFBasicHashCreate(allocator, 0x2001, &callbacks);
}

// Native CFDictionaryCreateMutable, 0x1c0194ef4. External. The Foundation path first; otherwise a generic
// table with a positive capacity reserved (at most 1000) and type 18.
OCFMutableDictionaryRef OCFDictionaryCreateMutable(
    OCFAllocatorRef allocator,
    OCFIndex capacity,
    const OCFDictionaryKeyCallBacks *keyCallBacks,
    const OCFDictionaryValueCallBacks *valueCallBacks
) {
    OCFMutableDictionaryRef result =
        __NSCFDictionaryCreateMutable(allocator, capacity, keyCallBacks, valueCallBacks);
    if (result != NULL) {
        return result;
    }
    BasicHashRef ht = __CFDictionaryCreateGeneric(allocator, keyCallBacks, valueCallBacks);
    if (ht == NULL) {
        return NULL;
    }
    if (capacity >= 1) {
        CFBasicHashSetCapacity(ht, (uintptr_t)capacity < 1000 ? capacity : 1000);
    }
    _OCFRuntimeSetInstanceTypeIDAndIsa(ht, 18);
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName(ht, "CFDictionary (mutable)");
    }
    return (OCFMutableDictionaryRef)ht;
}

// Native CFDictionaryCreateCopy, 0x1c0194fa4. External. Objective-C dictionaries answer -copyWithZone: NULL;
// others a table copy, marked immutable and given type 18.
OCFDictionaryRef OCFDictionaryCreateCopy(OCFAllocatorRef allocator, OCFDictionaryRef theDict) {
    if (isObjCDictionary(theDict)) {
        return ((OCFDictionaryRef (*)(OCFTypeRef, SEL, void *))objc_msgSend)(
            theDict, sel_getUid("copyWithZone:"), NULL
        );
    }
    BasicHashRef ht = CFBasicHashCreateCopy(allocator, (BasicHashRef)theDict);
    if (ht != NULL) {
        setImmutable(ht);
        _OCFRuntimeSetInstanceTypeIDAndIsa(ht, 18);
        if (__OCFOASafe) {
            __OCFSetLastAllocationEventName(ht, "CFDictionary (immutable)");
        }
    }
    return (OCFDictionaryRef)ht;
}

// Native CFDictionaryCreateMutableCopy, 0x1c0195054. External. A CF dictionary is copied (capacity unused).
// An Objective-C dictionary answers -_cfMutableCopy for the system default allocator; otherwise its keys and
// values (256 on the stack, more in two system-default buffers of type 0xC0040B8AA526D, allocated values
// first) fill a table with the CFType callbacks. The native code clears the first 512 bytes of each 2048-byte
// stack buffer before use; the restoration repeats that.
OCFMutableDictionaryRef
OCFDictionaryCreateMutableCopy(OCFAllocatorRef allocator, OCFIndex capacity, OCFDictionaryRef theDict) {
    (void)capacity;
    BasicHashRef ht;
    if (!isObjCDictionary(theDict)) {
        ht = CFBasicHashCreateCopy(allocator, (BasicHashRef)theDict);
        if (ht == NULL) {
            return NULL;
        }
    } else {
        OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
        if (allocator == systemDefault || ((allocator == NULL || allocator == kOCFAllocatorDefault) &&
                                           systemDefault == OCFAllocatorGetDefault())) {
            return ((OCFMutableDictionaryRef (*)(OCFTypeRef, SEL))objc_msgSend)(
                theDict, sel_getUid("_cfMutableCopy")
            );
        }
        OCFIndex count = OCFDictionaryGetCount(theDict);
        const void *keysBuffer[256];
        const void *valuesBuffer[256];
        memset(valuesBuffer, 0, 512);
        memset(keysBuffer, 0, 512);
        const void **keys;
        const void **values;
        if (count >= 257) {
            values = OCFAllocatorAllocateTyped(systemDefault, count * 8, 0xC0040B8AA526D, 0);
            keys = OCFAllocatorAllocateTyped(systemDefault, count * 8, 0xC0040B8AA526D, 0);
        } else {
            keys = keysBuffer;
            values = valuesBuffer;
        }
        OCFDictionaryGetKeysAndValues(theDict, keys, values);
        ht = __CFDictionaryCreateGeneric(
            allocator, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
        if (count >= 1 && ht != NULL) {
            CFBasicHashSetCapacity(ht, count);
            for (OCFIndex index = 0; index < count; index++) {
                CFBasicHashAddValue(ht, (uintptr_t)keys[index], (uintptr_t)values[index]);
            }
        }
        if (keys != keysBuffer && keys != values) {
            OCFAllocatorDeallocate(systemDefault, keys);
        }
        if (values != valuesBuffer) {
            OCFAllocatorDeallocate(systemDefault, values);
        }
        if (ht == NULL) {
            return NULL;
        }
    }
    _OCFRuntimeSetInstanceTypeIDAndIsa(ht, 18);
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName(ht, "CFDictionary (mutable)");
    }
    return (OCFMutableDictionaryRef)ht;
}

// Native CFDictionaryGetCount, 0x1c0195364. External. Objective-C: -count.
OCFIndex OCFDictionaryGetCount(OCFDictionaryRef theDict) {
    if (isObjCDictionary(theDict)) {
        return ((OCFIndex (*)(OCFTypeRef, SEL))objc_msgSend)(theDict, sel_getUid("count"));
    }
    return CFBasicHashGetCount((BasicHashRef)theDict);
}

// Native CFDictionaryGetKeysAndValues, 0x1c01953a8. External. Objective-C: -getObjects:andKeys: (values,
// keys); otherwise the elements up to CFDictionaryGetCount.
void OCFDictionaryGetKeysAndValues(OCFDictionaryRef theDict, const void **keys, const void **values) {
    if (isObjCDictionary(theDict)) {
        ((void (*)(OCFTypeRef, SEL, const void **, const void **))objc_msgSend)(
            theDict, sel_getUid("getObjects:andKeys:"), values, keys
        );
        return;
    }
    CFBasicHashGetElements(
        (BasicHashRef)theDict, OCFDictionaryGetCount(theDict), (uintptr_t *)values, (uintptr_t *)keys
    );
}

// Native CFDictionaryContainsKey, 0x1c019546c. External. Objective-C: -containsKey:.
Boolean OCFDictionaryContainsKey(OCFDictionaryRef theDict, const void *key) {
    if (isObjCDictionary(theDict)) {
        return ((Boolean (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(
            theDict, sel_getUid("containsKey:"), key
        );
    }
    return CFBasicHashGetCountOfKey((BasicHashRef)theDict, (uintptr_t)key) > 0;
}

// Native CFDictionaryGetValue, 0x1c01954c4. External. Objective-C: -objectForKey:; otherwise the found value
// or NULL.
const void *OCFDictionaryGetValue(OCFDictionaryRef theDict, const void *key) {
    if (isObjCDictionary(theDict)) {
        return ((const void *(*)(OCFTypeRef, SEL, const void *))objc_msgSend)(
            theDict, sel_getUid("objectForKey:"), key
        );
    }
    BasicHashBucket bucket = CFBasicHashFindBucket((BasicHashRef)theDict, (uintptr_t)key);
    return bucket.count != 0 ? (const void *)bucket.value : NULL;
}

// Native CFDictionaryGetValueIfPresent, 0x1c0195580. External. Objective-C: -__getValue:forKey:.
Boolean OCFDictionaryGetValueIfPresent(OCFDictionaryRef theDict, const void *key, const void **value) {
    if (isObjCDictionary(theDict)) {
        return ((Boolean (*)(OCFTypeRef, SEL, const void **, const void *))objc_msgSend)(
            theDict, sel_getUid("__getValue:forKey:"), value, key
        );
    }
    BasicHashBucket bucket = CFBasicHashFindBucket((BasicHashRef)theDict, (uintptr_t)key);
    if (bucket.count == 0) {
        return false;
    }
    if (value != NULL) {
        *value = (const void *)bucket.value;
    }
    return true;
}

// Native CFDictionaryGetKeyIfPresent, 0x1c01956e8. External. No Objective-C dispatch: the stored key of the
// found bucket, for a CF dictionary only. iOS Simulator 24A434, confidence: verified.
// iOS Device divergence: unread. Reading: Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json.
Boolean OCFDictionaryGetKeyIfPresent(OCFDictionaryRef theDict, const void *key, const void **actualKey) {
    BasicHashBucket bucket = CFBasicHashFindBucket((BasicHashRef)theDict, (uintptr_t)key);
    if (bucket.count == 0) {
        return false;
    }
    if (actualKey != NULL) {
        *actualKey = (const void *)bucket.key;
    }
    return true;
}

// Native CFDictionaryApplyFunction, 0x1c0195768, with __CFDictionaryApplyFunction_block_invoke (0x1c0195850).
// External. Objective-C: -__apply:context:; otherwise the applier gets each key, value and context once.
void OCFDictionaryApplyFunction(
    OCFDictionaryRef theDict, OCFDictionaryApplierFunction applier, void *context
) {
    if (isObjCDictionary(theDict)) {
        ((void (*)(OCFTypeRef, SEL, OCFDictionaryApplierFunction, void *))objc_msgSend)(
            theDict, sel_getUid("__apply:context:"), applier, context
        );
        return;
    }
    CFBasicHashApply((BasicHashRef)theDict, ^Boolean(BasicHashBucket bucket) {
      applier((const void *)bucket.key, (const void *)bucket.value, context);
      return true;
    });
}

// Native CFDictionaryApply, 0x1c0195874, with __CFDictionaryApply_block_invoke (0x1c019594c). Hidden (was
// private external). Objective-C: -enumerateKeysAndObjectsWithOptions:usingBlock: with options 0; otherwise
// the block gets each key and value and a cleared stop flag, and a set flag ends the enumeration.
void CFDictionaryApply(
    OCFDictionaryRef theDict, void (^block)(const void *key, const void *value, Boolean *stop)
) {
    if (isObjCDictionary(theDict)) {
        ((void (*)(OCFTypeRef, SEL, uintptr_t, const void *))objc_msgSend)(
            theDict, sel_getUid("enumerateKeysAndObjectsWithOptions:usingBlock:"), 0, (const void *)block
        );
        return;
    }
    CFBasicHashApply((BasicHashRef)theDict, ^Boolean(BasicHashBucket bucket) {
      Boolean stop = false;
      block((const void *)bucket.key, (const void *)bucket.value, &stop);
      return !stop;
    });
}

// Native _CFDictionaryFastEnumeration, 0x1c01959b8. External. 0 for Objective-C dictionaries; otherwise the
// basic hash enumeration (a tail call).
unsigned long
_OCFDictionaryFastEnumeration(OCFDictionaryRef theDict, void *state, void *stackbuffer, unsigned long count) {
    if (isObjCDictionary(theDict)) {
        return 0;
    }
    return __CFBasicHashFastEnumeration((BasicHashRef)theDict, state, stackbuffer, count);
}

// Native _CFDictionaryIsMutable, 0x1c0195a20. External. False for Objective-C dictionaries; otherwise whether
// info bit 6 (immutable) is clear.
Boolean _OCFDictionaryIsMutable(OCFDictionaryRef theDict) {
    if (isObjCDictionary(theDict)) {
        return false;
    }
    return (infoOf(theDict) & 0x40) == 0;
}

// Native _CFDictionarySetCapacity, 0x1c0195a70. External. Ignored for Objective-C dictionaries.
void _OCFDictionarySetCapacity(OCFMutableDictionaryRef theDict, OCFIndex capacity) {
    if (isObjCDictionary(theDict)) {
        return;
    }
    CFBasicHashSetCapacity((BasicHashRef)theDict, capacity);
}

static SEL willChangeSelector(void) {
    SEL selector = __CF_KVO_WillChangeSelector_hc;
    if (selector == NULL) {
        selector = sel_registerName("willChangeValueForKey:");
        __CF_KVO_WillChangeSelector_hc = selector;
    }
    return selector;
}

static SEL didChangeSelector(void) {
    SEL selector = __CF_KVO_DidChangeSelector_hc;
    if (selector == NULL) {
        selector = sel_registerName("didChangeValueForKey:");
        __CF_KVO_DidChangeSelector_hc = selector;
    }
    return selector;
}

// Native _CFDictionarySetKVOBit, 0x1c0195ac8. External. Info bit 0 (observed) becomes bit 0 of bit, with a
// compare-and-swap loop; no Objective-C dispatch.
void _OCFDictionarySetKVOBit(OCFDictionaryRef theDict, OCFIndex bit) {
    uint64_t *info = &((OCFRuntimeBase *)theDict)->info;
    uint64_t expected = __atomic_load_n(info, __ATOMIC_ACQUIRE);
    while (!__atomic_compare_exchange_n(
        info,
        &expected,
        (expected & ~(uint64_t)1) | ((uint64_t)bit & 1),
        false,
        __ATOMIC_SEQ_CST,
        __ATOMIC_SEQ_CST
    )) {
    }
}

// Native CFDictionaryAddValue, 0x1c0195af4. External. Objective-C: -__addObject:forKey:. An immutable
// dictionary is logged at level 3 (the add then halts in CFBasicHashAddValue). With info bit 0 (observed) the
// key is bracketed by -willChangeValueForKey: and -didChangeValueForKey:.
void OCFDictionaryAddValue(OCFMutableDictionaryRef theDict, const void *key, const void *value) {
    if (isObjCDictionary(theDict)) {
        ((void (*)(OCFTypeRef, SEL, const void *, const void *))objc_msgSend)(
            theDict, sel_getUid("__addObject:forKey:"), value, key
        );
        return;
    }
    if ((infoOf(theDict) & 0x40) != 0) {
        OCFLog(
            3,
            OF_STR("%s(): immutable collection %p given to mutating function"),
            "void CFDictionaryAddValue(CFMutableDictionaryRef, const void *, const void *)",
            theDict
        );
    }
    if ((infoOf(theDict) & 1) != 0) {
        ((void (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(theDict, willChangeSelector(), key);
    }
    CFBasicHashAddValue((BasicHashRef)theDict, (uintptr_t)key, (uintptr_t)value);
    if ((infoOf(theDict) & 1) != 0) {
        ((void (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(theDict, didChangeSelector(), key);
    }
}

// Native CFDictionarySetValue, 0x1c0195d2c. External. As CFDictionaryAddValue with -__setObject:forKey: and
// CFBasicHashSetValue.
void OCFDictionarySetValue(OCFMutableDictionaryRef theDict, const void *key, const void *value) {
    if (isObjCDictionary(theDict)) {
        ((void (*)(OCFTypeRef, SEL, const void *, const void *))objc_msgSend)(
            theDict, sel_getUid("__setObject:forKey:"), value, key
        );
        return;
    }
    if ((infoOf(theDict) & 0x40) != 0) {
        OCFLog(
            3,
            OF_STR("%s(): immutable collection %p given to mutating function"),
            "void CFDictionarySetValue(CFMutableDictionaryRef, const void *, const void *)",
            theDict
        );
    }
    if ((infoOf(theDict) & 1) != 0) {
        ((void (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(theDict, willChangeSelector(), key);
    }
    CFBasicHashSetValue((BasicHashRef)theDict, (uintptr_t)key, (uintptr_t)value);
    if ((infoOf(theDict) & 1) != 0) {
        ((void (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(theDict, didChangeSelector(), key);
    }
}

// Native CFDictionaryRemoveValue, 0x1c0195e48. External. As CFDictionaryAddValue with -removeObjectForKey:
// and CFBasicHashRemoveValue.
void OCFDictionaryRemoveValue(OCFMutableDictionaryRef theDict, const void *key) {
    if (isObjCDictionary(theDict)) {
        ((void (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(
            theDict, sel_getUid("removeObjectForKey:"), key
        );
        return;
    }
    if ((infoOf(theDict) & 0x40) != 0) {
        OCFLog(
            3,
            OF_STR("%s(): immutable collection %p given to mutating function"),
            "void CFDictionaryRemoveValue(CFMutableDictionaryRef, const void *)",
            theDict
        );
    }
    if ((infoOf(theDict) & 1) != 0) {
        ((void (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(theDict, willChangeSelector(), key);
    }
    CFBasicHashRemoveValue((BasicHashRef)theDict, (uintptr_t)key);
    if ((infoOf(theDict) & 1) != 0) {
        ((void (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(theDict, didChangeSelector(), key);
    }
}

// Native CFDictionaryRemoveAllValues, 0x1c0195f58. External. Objective-C: -removeAllObjects. An immutable
// dictionary is logged at level 3. With info bit 0 (observed) the keys are copied (malloc_type_malloc, type
// 0xC0040B8AA526D) into a set (system default allocator, CFType callbacks; the buffer is freed) that is sent
// with -_willChangeValuesForKeys: before and -_didChangeValuesForKeys: after the removal; the set is released
// only after the second message (bit 0 read again).
void OCFDictionaryRemoveAllValues(OCFMutableDictionaryRef theDict) {
    if (isObjCDictionary(theDict)) {
        ((void (*)(OCFTypeRef, SEL))objc_msgSend)(theDict, sel_getUid("removeAllObjects"));
        return;
    }
    if ((infoOf(theDict) & 0x40) != 0) {
        OCFLog(
            3,
            OF_STR("%s(): immutable collection %p given to mutating function"),
            "void CFDictionaryRemoveAllValues(CFMutableDictionaryRef)",
            theDict
        );
    }
    OCFSetRef keys = NULL;
    if ((infoOf(theDict) & 1) != 0) {
        if (__CF_KVO_WillChangeKeysSelector_hc == NULL) {
            __CF_KVO_WillChangeKeysSelector_hc = sel_registerName("_willChangeValuesForKeys:");
        }
        OCFIndex count = OCFDictionaryGetCount(theDict);
        const void **buffer = malloc_type_malloc((size_t)(count * 8), 0xC0040B8AA526D);
        OCFDictionaryGetKeysAndValues(theDict, buffer, NULL);
        keys = OCFSetCreate(kOCFAllocatorSystemDefault, buffer, count, &kOCFTypeSetCallBacks);
        free(buffer);
        ((void (*)(OCFTypeRef, SEL, OCFSetRef))objc_msgSend)(
            theDict, __CF_KVO_WillChangeKeysSelector_hc, keys
        );
    }
    CFBasicHashRemoveAllValues((BasicHashRef)theDict);
    if ((infoOf(theDict) & 1) != 0) {
        SEL selector = __CF_KVO_DidChangeKeysSelector_hc;
        if (selector == NULL) {
            selector = sel_registerName("_didChangeValuesForKeys:");
            __CF_KVO_DidChangeKeysSelector_hc = selector;
        }
        ((void (*)(OCFTypeRef, SEL, OCFSetRef))objc_msgSend)(theDict, selector, keys);
        if (keys != NULL) {
            OCFRelease(keys);
        }
    }
}
