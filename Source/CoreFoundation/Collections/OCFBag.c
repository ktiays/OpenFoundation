//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Collections/OCFBag.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFBasicHash.h"
#include "CoreFoundation/String/OCFString.h"

#include <string.h>

// Inferred organization: __CFBagDeallocate (0x1c014f8b0) through CFBagRemoveValue (0x1c0150240) lie in one
// cluster of the iOS Simulator __text section and are kept in one translation unit with the bag class record
// and callback structure; the native source path is not in the evidence. Functions of the cluster outside the
// scope are not restored. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; pseudocode used as a
// guide only); iOS Device 24A446: unread. Reading: not yet written; evidence
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json (CFBagCreateCopy:
// iphone-simulator-corpus-delta5.json).

static void __CFBagDeallocate(OCFTypeRef cf);
static Boolean __CFBagEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFBagHash(OCFTypeRef cf);
static OCFStringRef __CFBagCopyDescription(OCFTypeRef cf);

// Native kCFTypeBagCallBacks, 0x222429170 (__DATA_CONST,__const). External. Version 0,
// __CFTypeCollectionRetain, __CFTypeCollectionRelease, CFCopyDescription, CFEqual, CFHash.
const OCFBagCallBacks kOCFTypeBagCallBacks = {
    0,
    __CFTypeCollectionRetain,
    __CFTypeCollectionRelease,
    OCFCopyDescription,
    OCFEqual,
    OCFHash,
};

// Native __CFBagClass, 0x2224291d0 (__DATA_CONST,__const). Hidden (was private external); version 1, name
// "CFBag", finalize, equal, hash and the debug description.
const OCFRuntimeClass __CFBagClass = {
    1,
    "CFBag",
    NULL,
    NULL,
    __CFBagDeallocate,
    __CFBagEqual,
    __CFBagHash,
    NULL,
    __CFBagCopyDescription,
    NULL,
    NULL,
    0,
};

static uint64_t infoOf(OCFTypeRef cf) {
    return __atomic_load_n(&((OCFRuntimeBase *)cf)->info, __ATOMIC_ACQUIRE);
}

// Native __CFBagDeallocate, 0x1c014f8b0. Non-external. A tail call.
static void __CFBagDeallocate(OCFTypeRef cf) {
    __CFBasicHashDeallocate(cf);
}

// Native __CFBagEqual, 0x1c014f8b4. Non-external. A tail call.
static Boolean __CFBagEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    return __CFBasicHashEqual(cf1, cf2);
}

// Native __CFBagHash, 0x1c014f8b8. Non-external. A tail call.
static OCFHashCode __CFBagHash(OCFTypeRef cf) {
    return __CFBasicHashHash(cf);
}

// Native __CFBagCopyDescription, 0x1c014f8bc. Non-external. A tail call.
static OCFStringRef __CFBagCopyDescription(OCFTypeRef cf) {
    return __CFBasicHashCopyDescription(cf);
}

// Native __CFBagCreateGeneric, 0x1c014f97c. Non-external. A linear table with counts (flags 0x2002) whose
// value and key callbacks are both the bag's callbacks (NULL callbacks give NULL); no indirect key.
static BasicHashRef __CFBagCreateGeneric(OCFAllocatorRef allocator, const OCFBagCallBacks *callBacks) {
    OCFBagRetainCallBack retain = NULL;
    OCFBagReleaseCallBack release = NULL;
    OCFBagCopyDescriptionCallBack copyDescription = NULL;
    OCFBagEqualCallBack equal = NULL;
    OCFBagHashCallBack hash = NULL;
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
    return CFBasicHashCreate(allocator, 0x2002, &callbacks);
}

// Native CFBagCreateMutable, 0x1c014fa08. External. A generic bag given type 4; the capacity is unused.
OCFMutableBagRef
OCFBagCreateMutable(OCFAllocatorRef allocator, OCFIndex capacity, const OCFBagCallBacks *callBacks) {
    (void)capacity;
    BasicHashRef ht = __CFBagCreateGeneric(allocator, callBacks);
    if (ht != NULL) {
        _OCFRuntimeSetInstanceTypeIDAndIsa(ht, 4);
        if (__OCFOASafe) {
            __OCFSetLastAllocationEventName(ht, "CFBag (mutable)");
        }
    }
    return (OCFMutableBagRef)ht;
}

// Native CFBagCreateCopy, 0x1c014fa64. External. A bag that is not an Objective-C object (neither a tagged
// pointer nor __CF_IS_OBJC for type 4) is copied with CFBasicHashCreateCopy. Otherwise its values are read
// with CFBasicHashGetCount and CFBasicHashGetElements (applied to the object as natively, without Objective-C
// dispatch; the count is read twice) into a 256-entry stack buffer, of which the native code zeroes the first
// 64 entries (512 bytes), or, for 257 values or more, into a kCFAllocatorSystemDefault allocation (8 bytes
// per value, type 0xC0040B8AA526D, freed afterwards); a generic bag with kCFTypeBagCallBacks gets the count
// as its capacity (when nonzero) and every value added. The copy (when created) is marked immutable (info bit
// 6, a compare-and-swap loop), given type 4 and, with allocation statistics on, the event name "CFBag
// (immutable)".
OCFBagRef OCFBagCreateCopy(OCFAllocatorRef allocator, OCFBagRef theBag) {
    BasicHashRef ht;
    if ((OF_WORD_SHIFT((uintptr_t)theBag, 63)) != 0 || __CF_IS_OBJC(4, theBag) != 0) {
        OCFIndex numValues = CFBasicHashGetCount((BasicHashRef)theBag);
        uintptr_t buffer[256];
        memset(buffer, 0, 512);
        uintptr_t *list;
        if (numValues < 0x101) {
            list = buffer;
        } else {
            list = OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, numValues << 3, 0xC0040B8AA526D, 0);
        }
        CFBasicHashGetElements((BasicHashRef)theBag, CFBasicHashGetCount((BasicHashRef)theBag), NULL, list);
        ht = __CFBagCreateGeneric(allocator, &kOCFTypeBagCallBacks);
        if (numValues >= 1 && ht != NULL) {
            CFBasicHashSetCapacity(ht, numValues);
            for (OCFIndex idx = 0; idx < numValues; idx++) {
                CFBasicHashAddValue(ht, list[idx], list[idx]);
            }
        }
        if (numValues >= 0x101) {
            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, list);
        }
        if (ht == NULL) {
            return NULL;
        }
    } else {
        ht = CFBasicHashCreateCopy(allocator, (BasicHashRef)theBag);
        if (ht == NULL) {
            return NULL;
        }
    }
    uint64_t *info = &((OCFRuntimeBase *)ht)->info;
    uint64_t expected = __atomic_load_n(info, __ATOMIC_ACQUIRE);
    while (!__atomic_compare_exchange_n(
        info, &expected, expected | 0x40, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    )) {
    }
    _OCFRuntimeSetInstanceTypeIDAndIsa(ht, 4);
    if (__OCFOASafe == 1) {
        __OCFSetLastAllocationEventName(ht, "CFBag (immutable)");
    }
    return (OCFBagRef)ht;
}

// Native CFBagGetCountOfValue, 0x1c014fe68. External. A tail call (no Objective-C dispatch).
OCFIndex OCFBagGetCountOfValue(OCFBagRef theBag, const void *value) {
    return CFBasicHashGetCountOfKey((BasicHashRef)theBag, (uintptr_t)value);
}

// Native CFBagApplyFunction, 0x1c014ff68, with __CFBagApplyFunction_block_invoke (0x1c014ffe4). External. The
// applier gets each value and the context once per occurrence.
void OCFBagApplyFunction(OCFBagRef theBag, OCFBagApplierFunction applier, void *context) {
    CFBasicHashApply((BasicHashRef)theBag, ^Boolean(BasicHashBucket bucket) {
      for (uintptr_t remaining = bucket.count; remaining != 0;) {
          remaining--;
          applier((const void *)bucket.value, context);
      }
      return true;
    });
}

// Native CFBagAddValue, 0x1c015012c. External. An immutable bag is logged at level 3 (the add then halts in
// CFBasicHashAddValue).
void OCFBagAddValue(OCFMutableBagRef theBag, const void *value) {
    if ((infoOf(theBag) & 0x40) != 0) {
        OCFLog(
            3,
            OF_STR("%s(): immutable collection %p given to mutating function"),
            "void CFBagAddValue(CFMutableBagRef, const void *)",
            theBag
        );
    }
    CFBasicHashAddValue((BasicHashRef)theBag, (uintptr_t)value, (uintptr_t)value);
}

// Native CFBagRemoveValue, 0x1c0150240. External. An immutable bag is logged at level 3 (the removal then
// halts in CFBasicHashRemoveValue).
void OCFBagRemoveValue(OCFMutableBagRef theBag, const void *value) {
    if ((infoOf(theBag) & 0x40) != 0) {
        OCFLog(
            3,
            OF_STR("%s(): immutable collection %p given to mutating function"),
            "void CFBagRemoveValue(CFMutableBagRef, const void *)",
            theBag
        );
    }
    CFBasicHashRemoveValue((BasicHashRef)theBag, (uintptr_t)value);
}
