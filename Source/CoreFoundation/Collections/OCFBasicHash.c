//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Collections/OCFBasicHash.h"

#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"

#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <os/lock.h>
#include <string.h>

// Inferred organization: __CFBasicHashGetCallbacks (0x1c02b6598) through
// __CFBasicHashGetPtrIndex_block_invoke (0x1c02bb090) are contiguous in the iOS Simulator __text section and
// are kept in one translation unit with their .cold fragments (0x1c0311f04-0x1c0311ff8) and static data; the
// native source path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; pseudocode used as a
// guide only); iOS Device 24A446: unread. Reading: not yet written; evidence
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json and iphone-simulator-corpus-delta4.json.
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_0_53 (0x1c02bb120).

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call passes 0x10000.
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

enum {
    kFlagHashStyleMask = 0x3,
    kFlagKeys = 0x4,
    kFlagCountsOffsetMask = 0x18,
    kFlagIndirectKeys = 0x8000,
    kCallbackWordSuppressRC = 0x2,
    kCallbackWordFastGrow = 0x4,
    kCallbackWordFinalized = 0x8,
    kInfoImmutable = 0x40,
};

// Bit positions of the 10-bit callback indexes in bucketWord and callbackWord.
enum {
    kRetainKeyShift = 24,
    kRetainValueShift = 34,
    kReleaseKeyShift = 44,
    kReleaseValueShift = 54,
    kKeyDescriptionShift = 4,
    kValueDescriptionShift = 14,
    kEquateKeysShift = 24,
    kEquateValuesShift = 34,
    kHashKeyShift = 44,
    kIndirectKeyShift = 54,
};

static const uintptr_t kEmptyMarker = 0;
static const uintptr_t kDeletedMarker = ~(uintptr_t)0;
static const uintptr_t kEscapedZero = 0xA7BAADB1;
static const uintptr_t kEscapedOnes = 0xA5BAADB9;

static void __CFBasicHashRehash(BasicHashRef ht, OCFIndex newItemCount);
static void __CFBasicHashIncSlotCount(BasicHashRef ht, OCFIndex index);
static void __CFBasicHashAddValue(BasicHashRef ht, OCFIndex index, uintptr_t stackKey, uintptr_t stackValue);
static void
__CFBasicHashReplaceValue(BasicHashRef ht, OCFIndex index, uintptr_t stackKey, uintptr_t stackValue);
static void __CFBasicHashRemoveValue(BasicHashRef ht, OCFIndex index);
static void __CFBasicHashDrain(BasicHashRef ht);
static int32_t CFBasicHashGetPtrIndex(const void *ptr);
static BasicHashBucket ___CFBasicHashFindBucket_Linear_Indirect(BasicHashRef ht, uintptr_t stackKey);
static BasicHashBucket ___CFBasicHashFindBucket_Double_Indirect(BasicHashRef ht, uintptr_t stackKey);
static BasicHashBucket ___CFBasicHashFindBucket_Exponential_Indirect(BasicHashRef ht, uintptr_t stackKey);
static BasicHashBucket ___CFBasicHashFindBucket_Linear(BasicHashRef ht, uintptr_t stackKey);
static BasicHashBucket ___CFBasicHashFindBucket_Double(BasicHashRef ht, uintptr_t stackKey);
static BasicHashBucket ___CFBasicHashFindBucket_Exponential(BasicHashRef ht, uintptr_t stackKey);
static OCFIndex ___CFBasicHashFindBucket_Linear_Indirect_NoCollision(BasicHashRef ht, uintptr_t stackKey);
static OCFIndex ___CFBasicHashFindBucket_Double_Indirect_NoCollision(BasicHashRef ht, uintptr_t stackKey);
static OCFIndex
___CFBasicHashFindBucket_Exponential_Indirect_NoCollision(BasicHashRef ht, uintptr_t stackKey);
static OCFIndex ___CFBasicHashFindBucket_Linear_NoCollision(BasicHashRef ht, uintptr_t stackKey);
static OCFIndex ___CFBasicHashFindBucket_Double_NoCollision(BasicHashRef ht, uintptr_t stackKey);
static OCFIndex ___CFBasicHashFindBucket_Exponential_NoCollision(BasicHashRef ht, uintptr_t stackKey);

// Native __CFBasicHashTableSizes, 0x1c046cb28 (__TEXT,__const, 64 signed words). Non-external. Bucket counts
// by table index; entries 45-63 are zero.
static const OCFIndex __CFBasicHashTableSizes[64] = {
    0,         3,         7,         13,        23,        41,         71,         127,        191,
    251,       383,       631,       1087,      1723,      2803,       4523,       7351,       11959,
    19447,     31231,     50683,     81919,     132607,    214519,     346607,     561109,     907759,
    1468927,   2376191,   3845119,   6221311,   10066421,  16287743,   26354171,   42641881,   68996069,
    111638519, 180634607, 292272623, 472907251, 765180413, 1238087663, 2003267557, 3241355263, 5244622819,
};

// Native __CFBasicHashTableCapacities, 0x1c046cd28 (__TEXT,__const, 64 signed words). Non-external. The
// largest used-bucket count each table index holds; entries 45-63 are zero.
static const OCFIndex __CFBasicHashTableCapacities[64] = {
    0,        3,         6,         11,        19,        32,        52,         85,         118,
    155,      237,       390,       672,       1065,      1732,      2795,       4543,       7391,
    12019,    19302,     31324,     50629,     81956,     132580,    214215,     346784,     561026,
    907847,   1468567,   2376414,   3844982,   6221390,   10066379,  16287773,   26354132,   42641916,
    68996399, 111638327, 180634415, 292272755, 472907503, 765180257, 1238087439, 2003267722, 3241355160,
};

// Native __CFBasicHashPrimitiveRoots, 0x1c046cf48 (__TEXT,__const, 64 words). Non-external. The exponential
// probe's multiplier by table index.
static const uintptr_t __CFBasicHashPrimitiveRoots[64] = {
    0, 2, 3,  2, 5,  6, 7, 3, 19, 6,  5, 3, 3,  3, 2, 5, 6, 3, 3, 6, 2, 3, 3, 3, 5, 10, 3, 3, 22, 3, 3, 3,
    5, 2, 22, 2, 11, 5, 5, 2, 3,  10, 2, 3, 10, 2, 3, 5, 3, 3, 2, 7, 2, 3, 3, 3, 2, 3,  5, 5, 2,  3, 2, 0,
};

// Native CFBasicHashCallBackPtrs (0x22e6d39f8, __DATA,__bss, 32 page pointers), CFBasicHashCallBackPtrsLock
// (0x22e6d3b00), CFBasicHashCallBackPtrsCount (0x22e6d3b04), CFBasicHashCallBackPtrsCapacity (0x22e6cff50,
// __DATA,__data, initially 32) and CFBasicHashGetPtrIndex.once (0x22e6d3af8). Non-external. The registered
// callback pointers in pages of 32; an object stores 10-bit indexes into them.
static void **CFBasicHashCallBackPtrs[32];
static os_unfair_lock CFBasicHashCallBackPtrsLock;
static int32_t CFBasicHashCallBackPtrsCount;
static int32_t CFBasicHashCallBackPtrsCapacity = 32;
static dispatch_once_t CFBasicHashGetPtrIndexOnce;

// Native __CFBasicHashClass, 0x2227a9cb8 (__DATA_CONST,__const). Hidden (was private external); version 1,
// name "CFBasicHash", finalize, equal, hash and the debug description.
const OCFRuntimeClass __CFBasicHashClass = {
    1,
    "CFBasicHash",
    NULL,
    NULL,
    __CFBasicHashDeallocate,
    __CFBasicHashEqual,
    __CFBasicHashHash,
    NULL,
    __CFBasicHashCopyDescription,
    NULL,
    NULL,
    0,
};

// The lookup result for a marker key or an empty table: index -1, no key, value or count (the 32 bytes at
// 0x1c046cf28 in __TEXT,__const).
static const BasicHashBucket kNotFoundBucket = {-1, 0, 0, 0};

// Inlined accessors and steps that recur in the native bodies (each native site expands them in place).

static uint64_t infoOf(BasicHashRef ht) {
    return __atomic_load_n(&ht->base.info, __ATOMIC_ACQUIRE);
}

static uint8_t bucketIndexOf(BasicHashRef ht) {
    return (uint8_t)(ht->bucketWord >> 16);
}

static void *callbackAt(uint64_t word, unsigned shift) {
    uint64_t index = (word >> shift) & 0x3FF;
    return CFBasicHashCallBackPtrs[index >> 5][index & 0x1F];
}

static void **slotAt(BasicHashRef ht, unsigned byteOffset) {
    return (void **)((uint8_t *)ht->pointers + byteOffset);
}

static uintptr_t unescapeStored(uintptr_t stored) {
    if (stored == kEscapedZero) {
        return 0;
    }
    if (stored == kEscapedOnes) {
        return ~(uintptr_t)0;
    }
    return stored;
}

static uintptr_t escapeStack(uintptr_t stack) {
    uintptr_t stored = stack == 0 ? kEscapedZero : stack;
    return stored == ~(uintptr_t)0 ? kEscapedOnes : stored;
}

// The count of a bucket by the counts width; 1 without a counts slot.
static uintptr_t countAt(BasicHashRef ht, OCFIndex index) {
    uint16_t flags = ht->flags;
    if ((flags & kFlagCountsOffsetMask) == 0) {
        return 1;
    }
    void *counts = *slotAt(ht, flags & kFlagCountsOffsetMask);
    switch ((flags >> 5) & 3) {
        case 0:
            return ((uint8_t *)counts)[index];
        case 1:
            return ((uint16_t *)counts)[index];
        case 2:
            return ((uint32_t *)counts)[index];
        default:
            return ((uint64_t *)counts)[index];
    }
}

// The allocator the retain and release callbacks receive: the system default for a tagged pointer or when
// info bit 7 is set, otherwise the allocator stored 16 bytes before the object.
static OCFAllocatorRef callbackAllocator(BasicHashRef ht) {
    if ((intptr_t)ht < 0) {
        return kOCFAllocatorSystemDefault;
    }
    if ((infoOf(ht) & 0x80) != 0) {
        return kOCFAllocatorSystemDefault;
    }
    return *(const OCFAllocatorRef *)((const uint8_t *)ht - 16);
}

static uintptr_t retainValue(BasicHashRef ht, uintptr_t stackValue) {
    BasicHashRetainCallback retain = callbackAt(ht->bucketWord, kRetainValueShift);
    if (retain != NULL && (ht->callbackWord & kCallbackWordSuppressRC) == 0) {
        stackValue = (uintptr_t)retain(callbackAllocator(ht), (const void *)stackValue);
    }
    return stackValue;
}

static uintptr_t retainKey(BasicHashRef ht, uintptr_t stackKey) {
    BasicHashRetainCallback retain = callbackAt(ht->bucketWord, kRetainKeyShift);
    if (retain != NULL && (ht->callbackWord & kCallbackWordSuppressRC) == 0) {
        stackKey = (uintptr_t)retain(callbackAllocator(ht), (const void *)stackKey);
    }
    return stackKey;
}

static void releaseStoredValue(BasicHashRef ht, uintptr_t stored) {
    uintptr_t stackValue = unescapeStored(stored);
    BasicHashReleaseCallback release = callbackAt(ht->bucketWord, kReleaseValueShift);
    if (release != NULL && (ht->callbackWord & kCallbackWordSuppressRC) == 0) {
        release(callbackAllocator(ht), (const void *)stackValue);
    }
}

static void releaseStoredKey(BasicHashRef ht, uintptr_t stored) {
    uintptr_t stackKey = unescapeStored(stored);
    BasicHashReleaseCallback release = callbackAt(ht->bucketWord, kReleaseKeyShift);
    if (release != NULL && (ht->callbackWord & kCallbackWordSuppressRC) == 0) {
        release(callbackAllocator(ht), (const void *)stackKey);
    }
}

// Stores a value word and releases the word it replaces unless that was empty or deleted.
static void storeValue(BasicHashRef ht, OCFIndex index, uintptr_t stored) {
    uintptr_t *values = ht->pointers[0];
    uintptr_t old = values[index];
    values[index] = stored;
    if (old + 1 >= 2) {
        releaseStoredValue(ht, old);
    }
}

// The key counterpart of storeValue; a table without a keys slot halts (BRK #1).
static void storeKey(BasicHashRef ht, OCFIndex index, uintptr_t stored) {
    if ((ht->flags & kFlagKeys) == 0) {
        __builtin_trap();
    }
    uintptr_t *keys = ht->pointers[1];
    uintptr_t old = keys[index];
    keys[index] = stored;
    if (old + 1 >= 2) {
        releaseStoredKey(ht, old);
    }
}

// The probe dispatch every lookup site inlines: an empty table finds nothing; style 0 halts.
static BasicHashBucket findBucket(BasicHashRef ht, uintptr_t stackKey) {
    if (bucketIndexOf(ht) == 0) {
        return kNotFoundBucket;
    }
    uint16_t flags = ht->flags;
    unsigned style = flags & kFlagHashStyleMask;
    if ((flags & kFlagIndirectKeys) != 0) {
        if (style <= 1) {
            if (style != 1) {
                __builtin_trap();
            }
            return ___CFBasicHashFindBucket_Linear_Indirect(ht, stackKey);
        }
        if (style == 2) {
            return ___CFBasicHashFindBucket_Double_Indirect(ht, stackKey);
        }
        return ___CFBasicHashFindBucket_Exponential_Indirect(ht, stackKey);
    }
    if (style <= 1) {
        if (style != 1) {
            __builtin_trap();
        }
        return ___CFBasicHashFindBucket_Linear(ht, stackKey);
    }
    if (style == 2) {
        return ___CFBasicHashFindBucket_Double(ht, stackKey);
    }
    return ___CFBasicHashFindBucket_Exponential(ht, stackKey);
}

// The free-bucket dispatch of the insertion paths: -1 for an empty table; style 0 halts.
static OCFIndex findFreeBucket(BasicHashRef ht, uintptr_t stackKey) {
    if (bucketIndexOf(ht) == 0) {
        return -1;
    }
    uint16_t flags = ht->flags;
    unsigned style = flags & kFlagHashStyleMask;
    if ((flags & kFlagIndirectKeys) != 0) {
        if (style <= 1) {
            if (style != 1) {
                __builtin_trap();
            }
            return ___CFBasicHashFindBucket_Linear_Indirect_NoCollision(ht, stackKey);
        }
        if (style == 2) {
            return ___CFBasicHashFindBucket_Double_Indirect_NoCollision(ht, stackKey);
        }
        return ___CFBasicHashFindBucket_Exponential_Indirect_NoCollision(ht, stackKey);
    }
    if (style <= 1) {
        if (style != 1) {
            __builtin_trap();
        }
        return ___CFBasicHashFindBucket_Linear_NoCollision(ht, stackKey);
    }
    if (style == 2) {
        return ___CFBasicHashFindBucket_Double_NoCollision(ht, stackKey);
    }
    return ___CFBasicHashFindBucket_Exponential_NoCollision(ht, stackKey);
}

static bool isMarker(uintptr_t stack) {
    return stack == kEscapedOnes || stack == kEscapedZero;
}

// Native __CFBasicHashGetCallbacks, 0x1c02b6598. Hidden (was private external). The ten registered callbacks
// of the object's indexes, in the order of BasicHashCallbacks.
BasicHashCallbacks __CFBasicHashGetCallbacks(BasicHashRef ht) {
    uint64_t bucketWord = ht->bucketWord;
    uint64_t callbackWord = ht->callbackWord;
    BasicHashCallbacks callbacks;
    callbacks.retainValue = (BasicHashRetainCallback)callbackAt(bucketWord, kRetainValueShift);
    callbacks.retainKey = (BasicHashRetainCallback)callbackAt(bucketWord, kRetainKeyShift);
    callbacks.releaseValue = (BasicHashReleaseCallback)callbackAt(bucketWord, kReleaseValueShift);
    callbacks.releaseKey = (BasicHashReleaseCallback)callbackAt(bucketWord, kReleaseKeyShift);
    callbacks.equateValues = (BasicHashEquateCallback)callbackAt(callbackWord, kEquateValuesShift);
    callbacks.equateKeys = (BasicHashEquateCallback)callbackAt(callbackWord, kEquateKeysShift);
    callbacks.hashKey = (BasicHashHashCallback)callbackAt(callbackWord, kHashKeyShift);
    callbacks.getIndirectKey = (BasicHashIndirectKeyCallback)callbackAt(callbackWord, kIndirectKeyShift);
    callbacks.copyValueDescription =
        (BasicHashDescriptionCallback)callbackAt(callbackWord, kValueDescriptionShift);
    callbacks.copyKeyDescription =
        (BasicHashDescriptionCallback)callbackAt(callbackWord, kKeyDescriptionShift);
    return callbacks;
}

// Native CFBasicHashGetBucket, 0x1c02b665c. Hidden (was private external). An empty or deleted bucket answers
// its index alone. The key is the keys slot's entry, else the indirect key of the value (the value itself
// without that callback), else the value.
BasicHashBucket CFBasicHashGetBucket(BasicHashRef ht, OCFIndex index) {
    BasicHashBucket result;
    result.key = 0;
    result.value = 0;
    result.count = 0;
    result.index = index;
    uintptr_t stored = ((uintptr_t *)ht->pointers[0])[index];
    if (stored - 1 >= (uintptr_t)-2) {
        result.key = 0;
        result.value = 0;
        result.count = 0;
        return result;
    }
    uint16_t flags = ht->flags;
    uintptr_t count = countAt(ht, index);
    result.value = unescapeStored(stored);
    result.count = count;
    uintptr_t key;
    if ((flags & kFlagKeys) != 0) {
        key = unescapeStored(((uintptr_t *)ht->pointers[1])[index]);
    } else if ((flags & kFlagIndirectKeys) != 0) {
        key = unescapeStored(stored);
        BasicHashIndirectKeyCallback indirect = callbackAt(ht->callbackWord, kIndirectKeyShift);
        if (indirect != NULL) {
            key = (uintptr_t)indirect((const void *)key);
        }
    } else {
        key = unescapeStored(stored);
    }
    result.key = key;
    return result;
}

// Native CFBasicHashFindBucket, 0x1c02b67b0. Hidden (was private external). A marker key or an empty table
// answers the not-found bucket.
BasicHashBucket CFBasicHashFindBucket(BasicHashRef ht, uintptr_t stackKey) {
    if (isMarker(stackKey)) {
        return kNotFoundBucket;
    }
    return findBucket(ht, stackKey);
}

// Native CFBasicHashSuppressRC, 0x1c02b6838. Hidden (was private external).
void CFBasicHashSuppressRC(BasicHashRef ht) {
    ht->callbackWord |= kCallbackWordSuppressRC;
}

// Native CFBasicHashUnsuppressRC, 0x1c02b6848. Hidden (was private external).
void CFBasicHashUnsuppressRC(BasicHashRef ht) {
    ht->callbackWord &= ~(uint64_t)kCallbackWordSuppressRC;
}

// Native CFBasicHashGetCount, 0x1c02b6858. Hidden (was private external). Without counts the used-bucket
// count; with counts the sum over every bucket.
OCFIndex CFBasicHashGetCount(BasicHashRef ht) {
    uint16_t flags = ht->flags;
    if ((flags & kFlagCountsOffsetMask) == 0) {
        return ht->usedBuckets;
    }
    OCFIndex size = __CFBasicHashTableSizes[bucketIndexOf(ht)];
    if (size < 1) {
        return 0;
    }
    OCFIndex result = 0;
    void *counts = *slotAt(ht, flags & kFlagCountsOffsetMask);
    unsigned width = (flags >> 5) & 3;
    OCFIndex index = 0;
    do {
        uintptr_t count;
        if (width > 1) {
            if (width == 2) {
                count = ((uint32_t *)counts)[index];
            } else {
                count = ((uint64_t *)counts)[index];
            }
        } else if (width != 0) {
            count = ((uint16_t *)counts)[index];
        } else {
            count = ((uint8_t *)counts)[index];
        }
        result += count;
        index++;
    } while (size != index);
    return result;
}

// Native CFBasicHashGetCountOfKey, 0x1c02b68f4. Hidden (was private external). Marker keys and an empty table
// count 0.
OCFIndex CFBasicHashGetCountOfKey(BasicHashRef ht, uintptr_t stackKey) {
    if (stackKey == kEscapedOnes || stackKey == kEscapedZero) {
        return 0;
    }
    if (ht->usedBuckets == 0) {
        return 0;
    }
    return findBucket(ht, stackKey).count;
}

// Native CFBasicHashApply, 0x1c02b6b98. Hidden (was private external). Visits occupied buckets in index order
// until the block answers false or every used bucket was visited.
void CFBasicHashApply(BasicHashRef ht, BasicHashApplyBlock block) {
    OCFIndex used = ht->usedBuckets;
    OCFIndex size = __CFBasicHashTableSizes[bucketIndexOf(ht)];
    if (used == 0 || size < 1) {
        return;
    }
    OCFIndex index = 1;
    for (;;) {
        BasicHashBucket bucket = CFBasicHashGetBucket(ht, index - 1);
        if (bucket.count != 0) {
            if (!block(bucket)) {
                return;
            }
            used -= 1;
        }
        if (used < 1 || index >= size) {
            return;
        }
        index++;
    }
}

// Native CFBasicHashesAreEqual, 0x1c02b6cec. Hidden (was private external). Equal counts, then every bucket
// of the first found in the second with the same count and, for keyed tables, an identical or equated value.
// The block is __CFBasicHashesAreEqual_block_invoke (0x1c02b6df8); it probes the second table with the
// inlined dispatch (no marker check) and equates with the first table's callback.
Boolean CFBasicHashesAreEqual(BasicHashRef ht1, BasicHashRef ht2) {
    OCFIndex count = CFBasicHashGetCount(ht1);
    if (count != CFBasicHashGetCount(ht2)) {
        return false;
    }
    if (count == 0) {
        return true;
    }
    __block Boolean equal = true;
    CFBasicHashApply(ht1, ^Boolean(BasicHashBucket bucket1) {
      BasicHashBucket bucket2 = findBucket(ht2, bucket1.key);
      if (bucket1.count == bucket2.count) {
          if ((ht1->flags & kFlagKeys) == 0) {
              return true;
          }
          if (bucket1.value == bucket2.value) {
              return true;
          }
          BasicHashEquateCallback equate = callbackAt(ht1->callbackWord, kEquateValuesShift);
          if (equate != NULL && equate((const void *)bucket1.value, (const void *)bucket2.value)) {
              return true;
          }
      }
      equal = false;
      return false;
    });
    return equal;
}

// Native CFBasicHashGetElements, 0x1c02b6f60. Hidden (was private external). Each occupied bucket contributes
// its count of copies (values to weakValues, keys to weakKeys, either may be NULL) up to bufferslen.
void CFBasicHashGetElements(
    BasicHashRef ht, OCFIndex bufferslen, uintptr_t *weakValues, uintptr_t *weakKeys
) {
    OCFIndex used = ht->usedBuckets;
    OCFIndex size = __CFBasicHashTableSizes[bucketIndexOf(ht)];
    if (used == 0 || size < 1 || bufferslen < 1) {
        return;
    }
    OCFIndex index = 0;
    OCFIndex offset = 0;
    do {
        BasicHashBucket bucket = CFBasicHashGetBucket(ht, index);
        if (bucket.count != 0) {
            used -= 1;
            if (offset < bufferslen) {
                uintptr_t remaining = bucket.count - 1;
                do {
                    if (weakValues != NULL) {
                        weakValues[offset] = bucket.value;
                    }
                    if (weakKeys != NULL) {
                        weakKeys[offset] = bucket.key;
                    }
                    offset++;
                } while (remaining-- != 0 && offset < bufferslen);
            }
        }
        if (used < 1) {
            return;
        }
        index++;
        if (index >= size) {
            return;
        }
    } while (offset < bufferslen);
}

// The layout of NSFastEnumerationState (a local name).
typedef struct {
    unsigned long state;
    void **itemsPtr;
    unsigned long *mutationsPtr;
    unsigned long extra[5];
} FastEnumerationState;

// Native __CFBasicHashFastEnumeration, 0x1c02b706c. Hidden (was private external). The first call (state 0)
// points the mutations pointer at the 16-bit mutation count (offset 16); every call sets the items pointer to
// the stack buffer. While buckets remain in use, the state is below the bucket count and fewer than count
// items were produced, each bucket's key (when occupied) is stored and the state advanced. The number of
// items is returned.
unsigned long
__CFBasicHashFastEnumeration(BasicHashRef ht, void *state, void *stackbuffer, unsigned long count) {
    FastEnumerationState *enumerationState = (FastEnumerationState *)state;
    OCFIndex index = (OCFIndex)enumerationState->state;
    if (index == 0) {
        enumerationState->mutationsPtr = (unsigned long *)&ht->mutations;
    }
    enumerationState->itemsPtr = (void **)stackbuffer;
    OCFIndex used = ht->usedBuckets;
    OCFIndex size = __CFBasicHashTableSizes[bucketIndexOf(ht)];
    OCFIndex produced = 0;
    if (used == 0 || index >= size || (OCFIndex)count < 1) {
        return (unsigned long)produced;
    }
    do {
        BasicHashBucket bucket = CFBasicHashGetBucket(ht, index);
        if (bucket.count != 0) {
            enumerationState->itemsPtr[produced] = (void *)bucket.key;
            produced++;
            used--;
        }
        enumerationState->state++;
        if (used < 1) {
            break;
        }
        index++;
        if (index >= size) {
            break;
        }
    } while (produced < (OCFIndex)count);
    return (unsigned long)produced;
}

// Native CFBasicHashSetCapacity, 0x1c02b7170. Hidden (was private external). An immutable object halts;
// growth beyond the used count bumps the mutation count and rehashes for the difference.
void CFBasicHashSetCapacity(BasicHashRef ht, OCFIndex capacity) {
    if ((infoOf(ht) & kInfoImmutable) != 0) {
        __builtin_trap();
    }
    OCFIndex newItemCount = capacity - (OCFIndex)ht->usedBuckets;
    if (newItemCount > 0) {
        ht->mutations++;
        __CFBasicHashRehash(ht, newItemCount);
    }
}

// The storage allocation __CFBasicHashRehash inlines: zeroed through the hint when the allocator honours it
// (type 0x5E33CA92, hint 1), otherwise allocated (type 0xFB0BB3CD, hint 0) and cleared with bzero. A failed
// allocation halts.
static void *allocateZeroedStore(BasicHashRef ht, size_t size) {
    OCFAllocatorRef allocator = OCFGetAllocator(ht);
    void *memory;
    if (__OCFAllocatorRespectsHintZeroWhenAllocating(allocator)) {
        memory = OCFAllocatorAllocateTyped(allocator, (OCFIndex)size, 0x5E33CA92, 1);
        if (memory == NULL) {
            __builtin_trap();
        }
    } else {
        memory = OCFAllocatorAllocateTyped(allocator, (OCFIndex)size, 0xFB0BB3CD, 0);
        if (memory == NULL) {
            __builtin_trap();
        }
        bzero(memory, size);
    }
    return memory;
}

// Native __CFBasicHashRehash, 0x1c02b71a0, with __CFBasicHashRehash.cold.1 (0x1c0311f04). Non-external. A
// nonzero newItemCount picks the first table index whose capacity holds max(newItemCount, 0) plus the used
// count (none halts), one further index when newItemCount is 1 and the object grows fast; zero keeps the
// index. The new stores replace the old ones (the deleted count clears), every live entry is reinserted
// without collision checks, and the old stores are freed. The hashes slot is never populated here: its old
// pointer is the constant NULL in the final deallocation.
static void __CFBasicHashRehash(BasicHashRef ht, OCFIndex newItemCount) {
    OCFIndex newIndex = bucketIndexOf(ht);
    uint8_t oldIndex = bucketIndexOf(ht);
    if (newItemCount != 0) {
        OCFIndex target = (newItemCount < 0 ? 0 : newItemCount) + (OCFIndex)ht->usedBuckets;
        OCFIndex index = 0;
        while (index < 64 && __CFBasicHashTableCapacities[index] < target) {
            index++;
        }
        if (index == 64) {
            __builtin_trap();
        }
        if (newItemCount == 1) {
            newIndex = index + ((ht->callbackWord & kCallbackWordFastGrow) != 0 ? 1 : 0);
        } else {
            newIndex = index;
        }
    }
    OCFIndex newSize = __CFBasicHashTableSizes[newIndex];
    OCFIndex oldSize = __CFBasicHashTableSizes[oldIndex];
    uintptr_t *newValues = NULL;
    uintptr_t *newKeys = NULL;
    void *newCounts = NULL;
    if (newSize >= 1) {
        OCFIndex bytes;
        if (__builtin_mul_overflow(newSize, 8, &bytes)) {
            crashWithMessage("overflow while growing CFBasicHash");
        }
        newValues = allocateZeroedStore(ht, (size_t)bytes);
        if (__OCFOASafe) {
            __OCFSetLastAllocationEventName(newValues, "CFBasicHash (value-store)");
        }
        if ((ht->flags & kFlagKeys) != 0) {
            newKeys = allocateZeroedStore(ht, (size_t)bytes);
            if (__OCFOASafe) {
                __OCFSetLastAllocationEventName(newKeys, "CFBasicHash (key-store)");
            }
        }
        uint16_t flags = ht->flags;
        if ((flags & kFlagCountsOffsetMask) != 0) {
            newCounts = allocateZeroedStore(ht, (size_t)newSize << ((flags >> 5) & 3));
            if (__OCFOASafe) {
                __OCFSetLastAllocationEventName(newCounts, "CFBasicHash (count-store)");
            }
        }
    }
    uintptr_t *oldValues = ht->pointers[0];
    ht->bucketWord = (ht->bucketWord & 0xFFFFFFFFFF000000) | ((uint64_t)(uint8_t)newIndex << 16);
    ht->pointers[0] = newValues;
    uint16_t flags = ht->flags;
    uintptr_t *oldKeys = NULL;
    if ((flags & kFlagKeys) != 0) {
        oldKeys = ht->pointers[1];
        ht->pointers[1] = newKeys;
    }
    void *oldCounts = NULL;
    if ((flags & kFlagCountsOffsetMask) != 0) {
        void **slot = slotAt(ht, flags & kFlagCountsOffsetMask);
        oldCounts = *slot;
        *slot = newCounts;
    }
    bool copyCounts = newCounts != NULL && oldCounts != NULL;
    for (OCFIndex index = 0; index < oldSize; index++) {
        uintptr_t stored = oldValues[index];
        if (stored - 1 > (uintptr_t)-3) {
            continue;
        }
        uintptr_t stackValue = unescapeStored(stored);
        uintptr_t stackKey = stackValue;
        if (oldKeys != NULL) {
            stackKey = unescapeStored(oldKeys[index]);
        }
        if ((ht->flags & kFlagIndirectKeys) != 0) {
            BasicHashIndirectKeyCallback indirect = callbackAt(ht->callbackWord, kIndirectKeyShift);
            stackKey = indirect != NULL ? (uintptr_t)indirect((const void *)stackValue) : stackValue;
        }
        OCFIndex newBucket = findFreeBucket(ht, stackKey);
        storeValue(ht, newBucket, escapeStack(stackValue));
        if (oldKeys != NULL) {
            storeKey(ht, newBucket, escapeStack(stackKey));
        }
        if (copyCounts) {
            switch ((ht->flags >> 5) & 3) {
                case 0:
                    ((uint8_t *)newCounts)[newBucket] = ((uint8_t *)oldCounts)[index];
                    break;
                case 1:
                    ((uint16_t *)newCounts)[newBucket] = ((uint16_t *)oldCounts)[index];
                    break;
                case 2:
                    ((uint32_t *)newCounts)[newBucket] = ((uint32_t *)oldCounts)[index];
                    break;
                default:
                    ((uint64_t *)newCounts)[newBucket] = ((uint64_t *)oldCounts)[index];
                    break;
            }
        }
    }
    OCFAllocatorRef allocator = OCFGetAllocator(ht);
    OCFAllocatorDeallocate(allocator, oldValues);
    OCFAllocatorDeallocate(allocator, oldKeys);
    OCFAllocatorDeallocate(allocator, oldCounts);
    OCFAllocatorDeallocate(allocator, NULL);
}

// Native CFBasicHashAddValue, 0x1c02b77e4. Hidden (was private external). An immutable object or a marker key
// or value halts. A present key bumps the mutation count and, below the count ceiling with counts, its count
// (true); without counts nothing is added (false). An absent key is inserted (true).
Boolean CFBasicHashAddValue(BasicHashRef ht, uintptr_t stackKey, uintptr_t stackValue) {
    if ((infoOf(ht) & kInfoImmutable) != 0) {
        __builtin_trap();
    }
    if (isMarker(stackKey) || isMarker(stackValue)) {
        __builtin_trap();
    }
    BasicHashBucket bucket = findBucket(ht, stackKey);
    if (bucket.count == 0) {
        __CFBasicHashAddValue(ht, bucket.index, stackKey, stackValue);
        return true;
    }
    ht->mutations++;
    if (bucket.count <= 0x7FFFFFFFFFFFFFFE && (ht->flags & kFlagCountsOffsetMask) != 0) {
        __CFBasicHashIncSlotCount(ht, bucket.index);
        return true;
    }
    return false;
}

// Native __CFBasicHashIncSlotCount, 0x1c02b79b8, with __CFBasicHashIncSlotCount.cold.1-.12 (0x1c0311f14-
// 0x1c0311fd0). Non-external. Increments a count below its width's ceiling (0x7E, 0x7FFE, 0x7FFFFFFE,
// 0x7FFFFFFFFFFFFFFE); otherwise widens the counts store one step (type 0x7DD6E35C, hint 0; a failed
// allocation or a widening beyond 8 bytes halts; a size overflow crashes) and retries.
// Folds the compiler-outlined pieces ___CFBasicHashIncSlotCount.cold.2 (0x1c0311f24),
// ___CFBasicHashIncSlotCount.cold.3 (0x1c0311f34), ___CFBasicHashIncSlotCount.cold.4 (0x1c0311f44),
// ___CFBasicHashIncSlotCount.cold.5 (0x1c0311f54), ___CFBasicHashIncSlotCount.cold.6 (0x1c0311f64),
// ___CFBasicHashIncSlotCount.cold.7 (0x1c0311f74), ___CFBasicHashIncSlotCount.cold.8 (0x1c0311f84),
// ___CFBasicHashIncSlotCount.cold.9 (0x1c0311f94), ___CFBasicHashIncSlotCount.cold.10 (0x1c0311fa4),
// ___CFBasicHashIncSlotCount.cold.11 (0x1c0311fb4), ___CFBasicHashIncSlotCount.cold.12 (0x1c0311fc4).
static void __CFBasicHashIncSlotCount(BasicHashRef ht, OCFIndex index) {
    for (;;) {
        uint16_t flags = ht->flags;
        void *counts = *slotAt(ht, flags & kFlagCountsOffsetMask);
        switch ((flags >> 5) & 3) {
            case 0: {
                uint8_t count = ((uint8_t *)counts)[index];
                if (count <= 0x7E) {
                    ((uint8_t *)counts)[index] = count + 1;
                    return;
                }
                break;
            }
            case 1: {
                uint16_t count = ((uint16_t *)counts)[index];
                if (count <= 0x7FFE) {
                    ((uint16_t *)counts)[index] = count + 1;
                    return;
                }
                break;
            }
            case 2: {
                uint32_t count = ((uint32_t *)counts)[index];
                if (count <= 0x7FFFFFFE) {
                    ((uint32_t *)counts)[index] = count + 1;
                    return;
                }
                break;
            }
            default: {
                uint64_t count = ((uint64_t *)counts)[index];
                if (count <= 0x7FFFFFFFFFFFFFFE) {
                    ((uint64_t *)counts)[index] = count + 1;
                    return;
                }
                break;
            }
        }
        OCFAllocatorRef allocator = OCFGetAllocator(ht);
        uint16_t current = ht->flags;
        void *newCounts;
        OCFIndex size;
        OCFIndex bytes;
        switch ((current >> 5) & 3) {
            case 0:
                ht->flags = (current & ~0x40) | 0x20;
                size = __CFBasicHashTableSizes[bucketIndexOf(ht)];
                if (__builtin_mul_overflow(size, 2, &bytes)) {
                    crashWithMessage("overflow while growing CFBasicHash");
                }
                newCounts = OCFAllocatorAllocateTyped(OCFGetAllocator(ht), bytes, 0x7DD6E35C, 0);
                if (newCounts == NULL) {
                    __builtin_trap();
                }
                if (__OCFOASafe) {
                    __OCFSetLastAllocationEventName(newCounts, "CFBasicHash (count-store)");
                }
                for (OCFIndex item = 0; item < size; item++) {
                    ((uint16_t *)newCounts)[item] = ((uint8_t *)counts)[item];
                }
                break;
            case 1:
                ht->flags = (current & 0xFF9F) | 0x40;
                size = __CFBasicHashTableSizes[bucketIndexOf(ht)];
                if (__builtin_mul_overflow(size, 4, &bytes)) {
                    crashWithMessage("overflow while growing CFBasicHash");
                }
                newCounts = OCFAllocatorAllocateTyped(OCFGetAllocator(ht), bytes, 0x7DD6E35C, 0);
                if (newCounts == NULL) {
                    __builtin_trap();
                }
                if (__OCFOASafe) {
                    __OCFSetLastAllocationEventName(newCounts, "CFBasicHash (count-store)");
                }
                for (OCFIndex item = 0; item < size; item++) {
                    ((uint32_t *)newCounts)[item] = ((uint16_t *)counts)[item];
                }
                break;
            case 2:
                ht->flags = current | 0x60;
                size = __CFBasicHashTableSizes[bucketIndexOf(ht)];
                if (__builtin_mul_overflow(size, 8, &bytes)) {
                    crashWithMessage("overflow while growing CFBasicHash");
                }
                newCounts = OCFAllocatorAllocateTyped(OCFGetAllocator(ht), bytes, 0x7DD6E35C, 0);
                if (newCounts == NULL) {
                    __builtin_trap();
                }
                if (__OCFOASafe) {
                    __OCFSetLastAllocationEventName(newCounts, "CFBasicHash (count-store)");
                }
                for (OCFIndex item = 0; item < size; item++) {
                    ((uint64_t *)newCounts)[item] = ((uint32_t *)counts)[item];
                }
                break;
            default:
                __builtin_trap();
        }
        *slotAt(ht, ht->flags & kFlagCountsOffsetMask) = newCounts;
        OCFAllocatorDeallocate(allocator, counts);
    }
}

// Native __CFBasicHashAddValue, 0x1c02b81ec. Non-external. Bumps the mutation count; when the table's
// capacity cannot take one more used bucket it rehashes for one item and finds a free bucket again; otherwise
// reusing a deleted bucket decrements the 16-bit deleted count. Retains the value then (keyed tables) the
// key, stores them, counts the bucket and increments the used count.
static void __CFBasicHashAddValue(BasicHashRef ht, OCFIndex index, uintptr_t stackKey, uintptr_t stackValue) {
    ht->mutations++;
    uint64_t bucketWord = ht->bucketWord;
    if (__CFBasicHashTableCapacities[(uint8_t)(bucketWord >> 16)] <
        (OCFIndex)(uint32_t)(ht->usedBuckets + 1)) {
        __CFBasicHashRehash(ht, 1);
        index = findFreeBucket(ht, stackKey);
    } else if (((uintptr_t *)ht->pointers[0])[index] == kDeletedMarker) {
        ht->bucketWord = (bucketWord & ~(uint64_t)0xFFFF) | (uint16_t)(bucketWord - 1);
    }
    stackValue = retainValue(ht, stackValue);
    if ((ht->flags & kFlagKeys) != 0) {
        stackKey = retainKey(ht, stackKey);
    }
    storeValue(ht, index, escapeStack(stackValue));
    if ((ht->flags & kFlagKeys) != 0) {
        storeKey(ht, index, escapeStack(stackKey));
    }
    if ((ht->flags & kFlagCountsOffsetMask) != 0) {
        __CFBasicHashIncSlotCount(ht, index);
    }
    ht->usedBuckets++;
}

// Native __CFBasicHashReplaceValue, 0x1c02b86fc. Non-external. Bumps the mutation count, retains the value
// then (keyed tables) the key, and stores them over the bucket's, releasing the old ones.
static void
__CFBasicHashReplaceValue(BasicHashRef ht, OCFIndex index, uintptr_t stackKey, uintptr_t stackValue) {
    ht->mutations++;
    stackValue = retainValue(ht, stackValue);
    if ((ht->flags & kFlagKeys) != 0) {
        stackKey = retainKey(ht, stackKey);
    }
    storeValue(ht, index, escapeStack(stackValue));
    if ((ht->flags & kFlagKeys) != 0) {
        storeKey(ht, index, escapeStack(stackKey));
    }
}

// Native CFBasicHashSetValue, 0x1c02b8954. Hidden (was private external). An immutable object or a marker key
// or value halts; a present key is replaced, an absent one added.
void CFBasicHashSetValue(BasicHashRef ht, uintptr_t stackKey, uintptr_t stackValue) {
    if ((infoOf(ht) & kInfoImmutable) != 0) {
        __builtin_trap();
    }
    if (isMarker(stackKey) || isMarker(stackValue)) {
        __builtin_trap();
    }
    BasicHashBucket bucket = findBucket(ht, stackKey);
    if (bucket.count != 0) {
        __CFBasicHashReplaceValue(ht, bucket.index, stackKey, stackValue);
    } else {
        __CFBasicHashAddValue(ht, bucket.index, stackKey, stackValue);
    }
}

// Native CFBasicHashRemoveValue, 0x1c02b8b20. Hidden (was private external). An immutable object halts; a
// marker key removes nothing (0). A count of 1 removes the bucket; a larger count bumps the mutation count
// and, below the ceiling with counts, decrements it. Answers the bucket's count before removal.
OCFIndex CFBasicHashRemoveValue(BasicHashRef ht, uintptr_t stackKey) {
    if ((infoOf(ht) & kInfoImmutable) != 0) {
        __builtin_trap();
    }
    if (stackKey == kEscapedOnes || stackKey == kEscapedZero) {
        return 0;
    }
    BasicHashBucket bucket = findBucket(ht, stackKey);
    if (bucket.count < 2) {
        if (bucket.count == 1) {
            __CFBasicHashRemoveValue(ht, bucket.index);
        }
    } else {
        ht->mutations++;
        uint16_t flags = ht->flags;
        if (bucket.count <= 0x7FFFFFFFFFFFFFFE && (flags & kFlagCountsOffsetMask) != 0) {
            void *counts = *slotAt(ht, flags & kFlagCountsOffsetMask);
            switch ((flags >> 5) & 3) {
                case 0:
                    ((uint8_t *)counts)[bucket.index]--;
                    break;
                case 1:
                    ((uint16_t *)counts)[bucket.index]--;
                    break;
                case 2:
                    ((uint32_t *)counts)[bucket.index]--;
                    break;
                default:
                    ((uint64_t *)counts)[bucket.index]--;
                    break;
            }
        }
    }
    return (OCFIndex)bucket.count;
}

// Native __CFBasicHashRemoveValue, 0x1c02b8d0c. Non-external. Marks the bucket deleted (releasing its value
// and key), decrements its count, the used count and increments the 16-bit deleted count. Shrinks
// (newItemCount -1) when the capacity two indexes down (five when growing fast; none below index 3 or 6)
// exceeds the used count; otherwise rehashes in place (0) when the deleted count was 0xFFFF or reaches a
// quarter of a table of at least 20 buckets.
static void __CFBasicHashRemoveValue(BasicHashRef ht, OCFIndex index) {
    ht->mutations++;
    storeValue(ht, index, kDeletedMarker);
    if ((ht->flags & kFlagKeys) != 0) {
        uintptr_t *keys = ht->pointers[1];
        uintptr_t old = keys[index];
        keys[index] = kDeletedMarker;
        if (old + 1 >= 2) {
            releaseStoredKey(ht, old);
        }
    }
    uint16_t flags = ht->flags;
    if ((flags & kFlagCountsOffsetMask) != 0) {
        void *counts = *slotAt(ht, flags & kFlagCountsOffsetMask);
        switch ((flags >> 5) & 3) {
            case 0:
                ((uint8_t *)counts)[index]--;
                break;
            case 1:
                ((uint16_t *)counts)[index]--;
                break;
            case 2:
                ((uint32_t *)counts)[index]--;
                break;
            default:
                ((uint64_t *)counts)[index]--;
                break;
        }
    }
    uint32_t used = ht->usedBuckets - 1;
    ht->usedBuckets = used;
    uint64_t bucketWord = ht->bucketWord;
    uint16_t deleted = (uint16_t)(bucketWord + 1);
    ht->bucketWord = (bucketWord & ~(uint64_t)0xFFFF) | deleted;
    uint8_t bucketIndex = (uint8_t)(bucketWord >> 16);
    if ((ht->callbackWord & kCallbackWordFastGrow) != 0) {
        if (bucketIndex >= 6 && __CFBasicHashTableCapacities[bucketIndex - 5] > (OCFIndex)used) {
            __CFBasicHashRehash(ht, -1);
            return;
        }
    } else {
        if (bucketIndex >= 3 && __CFBasicHashTableCapacities[bucketIndex - 2] > (OCFIndex)used) {
            __CFBasicHashRehash(ht, -1);
            return;
        }
    }
    if ((uint16_t)bucketWord == 0xFFFF) {
        __CFBasicHashRehash(ht, 0);
        return;
    }
    OCFIndex size = __CFBasicHashTableSizes[bucketIndex];
    if (size >= 20 && ((uintptr_t)size >> 2) <= deleted) {
        __CFBasicHashRehash(ht, 0);
    }
}

// Native CFBasicHashRemoveAllValues, 0x1c02b8f88. Hidden (was private external). An immutable object halts;
// a nonempty table drains.
void CFBasicHashRemoveAllValues(BasicHashRef ht) {
    if ((infoOf(ht) & kInfoImmutable) != 0) {
        __builtin_trap();
    }
    if (bucketIndexOf(ht) != 0) {
        __CFBasicHashDrain(ht);
    }
}

// Native __CFBasicHashDrain, 0x1c02b8fa8. Non-external. Detaches the stores, bumps the mutation count, clears
// the used count, deleted count and table index, releases every live value and (keyed tables) its key — the
// key without an empty check of its own — and frees the stores (the hashes slot as the constant NULL).
static void __CFBasicHashDrain(BasicHashRef ht) {
    OCFIndex size = __CFBasicHashTableSizes[bucketIndexOf(ht)];
    OCFAllocatorRef allocator = OCFGetAllocator(ht);
    uintptr_t *oldKeys = NULL;
    uintptr_t *oldValues = ht->pointers[0];
    ht->pointers[0] = NULL;
    uint16_t flags = ht->flags;
    if ((flags & kFlagKeys) != 0) {
        oldKeys = ht->pointers[1];
        ht->pointers[1] = NULL;
    }
    void *oldCounts = NULL;
    if ((flags & kFlagCountsOffsetMask) != 0) {
        void **slot = slotAt(ht, flags & kFlagCountsOffsetMask);
        oldCounts = *slot;
        *slot = NULL;
    }
    ht->mutations++;
    uint64_t bucketWord = ht->bucketWord;
    ht->usedBuckets = 0;
    ht->bucketWord = bucketWord & 0xFFFFFFFFFF000000;
    for (OCFIndex index = 0; index < size; index++) {
        uintptr_t stored = oldValues[index];
        if (stored - 1 > (uintptr_t)-3) {
            continue;
        }
        releaseStoredValue(ht, stored);
        if (oldKeys != NULL) {
            releaseStoredKey(ht, oldKeys[index]);
        }
    }
    OCFAllocatorDeallocate(allocator, oldValues);
    OCFAllocatorDeallocate(allocator, oldKeys);
    OCFAllocatorDeallocate(allocator, oldCounts);
    OCFAllocatorDeallocate(allocator, NULL);
}

// The object size CFBasicHashGetSize and the description report: 48 bytes, 56 with keys, 8 more with counts.
static OCFIndex objectSize(uint16_t flags) {
    OCFIndex size = (flags & kFlagKeys) == 0 ? 48 : 56;
    return (flags & kFlagCountsOffsetMask) != 0 ? size + 8 : size;
}

// Native CFBasicHashGetSize, 0x1c02b9194. Hidden (was private external). The object size, plus with total and
// a nonempty table the malloc sizes of the value, key and counts stores.
OCFIndex CFBasicHashGetSize(BasicHashRef ht, Boolean total) {
    OCFIndex size = objectSize(ht->flags);
    if (total && __CFBasicHashTableSizes[bucketIndexOf(ht)] >= 1) {
        size += (OCFIndex)malloc_size(ht->pointers[0]);
        uint16_t flags = ht->flags;
        if ((flags & kFlagKeys) != 0) {
            size += (OCFIndex)malloc_size(ht->pointers[1]);
            flags = ht->flags;
        }
        if ((flags & kFlagCountsOffsetMask) != 0) {
            size += (OCFIndex)malloc_size(ht->pointers[(flags >> 3) & 3]);
        }
    }
    return size;
}

// Native CFBasicHashCopyDescription, 0x1c02b9238. Hidden (was private external). The block is
// __CFBasicHashCopyDescription_block_invoke (0x1c02b9530): element descriptions come from the description
// callbacks when describeElements is set and the callback exists, otherwise "<%p>"; the entry format depends
// on keys and counts. The detail lines report "no" for the hash cache and strong values and keys and "custom"
// for the callbacks as constants, and NULL for the hashes pointer.
OCFMutableStringRef CFBasicHashCopyDescription(
    BasicHashRef ht, Boolean detailed, OCFStringRef prefix, OCFStringRef entryPrefix, Boolean describeElements
) {
    OCFMutableStringRef result = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    const char *mutability = (infoOf(ht) & kInfoImmutable) == 0 ? "mutable" : "immutable";
    uint16_t flags = ht->flags;
    const char *multi = (flags & kFlagCountsOffsetMask) == 0 ? "" : "multi";
    const char *kind = (flags & kFlagKeys) == 0 ? "set" : "dict";
    OCFStringAppendFormat(
        result,
        NULL,
        OF_STR("%@{type = %s %s%s, count = %ld,\n"),
        prefix,
        mutability,
        multi,
        kind,
        CFBasicHashGetCount(ht)
    );
    if (detailed) {
        OCFStringAppendFormat(
            result,
            NULL,
            OF_STR("%@hash cache = %s, strong values = %s, strong keys = %s, cb = %s,\n"),
            prefix,
            "no",
            "no",
            "no",
            "custom"
        );
        uint8_t bucketIndex = bucketIndexOf(ht);
        OCFStringAppendFormat(
            result,
            NULL,
            OF_STR("%@num bucket index = %d, num buckets = %ld, capacity = %ld, num buckets used = %u,\n"),
            prefix,
            (int)bucketIndex,
            __CFBasicHashTableSizes[bucketIndex],
            __CFBasicHashTableCapacities[bucketIndex],
            ht->usedBuckets
        );
        flags = ht->flags;
        int countsWidth = (flags & kFlagCountsOffsetMask) == 0 ? 0 : 1 << ((flags >> 5) & 3);
        OCFStringAppendFormat(
            result,
            NULL,
            OF_STR("%@counts width = %d, finalized = %s,\n"),
            prefix,
            countsWidth,
            (ht->callbackWord & kCallbackWordFinalized) == 0 ? "no" : "yes"
        );
        OCFIndex mutations = ht->mutations;
        OCFIndex deleted = (uint16_t)ht->bucketWord;
        OCFIndex size = objectSize(ht->flags);
        OCFStringAppendFormat(
            result,
            NULL,
            OF_STR("%@num mutations = %ld, num deleted = %ld, size = %ld, total size = %ld,\n"),
            prefix,
            mutations,
            deleted,
            size,
            CFBasicHashGetSize(ht, true)
        );
        flags = ht->flags;
        void *keys = (flags & kFlagKeys) != 0 ? ht->pointers[1] : NULL;
        void *counts =
            (flags & kFlagCountsOffsetMask) != 0 ? *slotAt(ht, flags & kFlagCountsOffsetMask) : NULL;
        OCFStringAppendFormat(
            result,
            NULL,
            OF_STR("%@values ptr = %p, keys ptr = %p, counts ptr = %p, hashes ptr = %p,\n"),
            prefix,
            ht->pointers[0],
            keys,
            counts,
            NULL
        );
    }
    OCFStringAppendFormat(result, NULL, OF_STR("%@entries =>\n"), prefix);
    CFBasicHashApply(ht, ^Boolean(BasicHashBucket bucket) {
      OCFStringRef valueDescription;
      OCFStringRef keyDescription;
      if (describeElements) {
          BasicHashDescriptionCallback copyValue = callbackAt(ht->callbackWord, kValueDescriptionShift);
          if (copyValue != NULL) {
              valueDescription = copyValue((const void *)bucket.value);
          } else {
              valueDescription = OCFStringCreateWithFormat(
                  kOCFAllocatorSystemDefault, NULL, OF_STR("<%p>"), (void *)bucket.value
              );
          }
          if ((ht->flags & kFlagKeys) == 0) {
              keyDescription = NULL;
          } else {
              BasicHashDescriptionCallback copyKey = callbackAt(ht->callbackWord, kKeyDescriptionShift);
              if (copyKey != NULL) {
                  keyDescription = copyKey((const void *)bucket.key);
              } else {
                  keyDescription = OCFStringCreateWithFormat(
                      kOCFAllocatorSystemDefault, NULL, OF_STR("<%p>"), (void *)bucket.key
                  );
              }
          }
      } else {
          valueDescription = OCFStringCreateWithFormat(
              kOCFAllocatorSystemDefault, NULL, OF_STR("<%p>"), (void *)bucket.value
          );
          if ((ht->flags & kFlagKeys) != 0) {
              keyDescription = OCFStringCreateWithFormat(
                  kOCFAllocatorSystemDefault, NULL, OF_STR("<%p>"), (void *)bucket.key
              );
          } else {
              keyDescription = NULL;
          }
      }
      uint16_t entryFlags = ht->flags;
      if ((entryFlags & kFlagKeys) != 0 && (entryFlags & kFlagCountsOffsetMask) != 0) {
          OCFStringAppendFormat(
              result,
              NULL,
              OF_STR("%@%ld : %@ = %@ (%ld)\n"),
              entryPrefix,
              bucket.index,
              keyDescription,
              valueDescription,
              (OCFIndex)bucket.count
          );
      } else if ((entryFlags & kFlagKeys) != 0) {
          OCFStringAppendFormat(
              result,
              NULL,
              OF_STR("%@%ld : %@ = %@\n"),
              entryPrefix,
              bucket.index,
              keyDescription,
              valueDescription
          );
      } else if ((entryFlags & kFlagCountsOffsetMask) != 0) {
          OCFStringAppendFormat(
              result,
              NULL,
              OF_STR("%@%ld : %@ (%ld)\n"),
              entryPrefix,
              bucket.index,
              valueDescription,
              (OCFIndex)bucket.count
          );
      } else {
          OCFStringAppendFormat(
              result, NULL, OF_STR("%@%ld : %@\n"), entryPrefix, bucket.index, valueDescription
          );
      }
      if (keyDescription != NULL) {
          OCFRelease(keyDescription);
      }
      if (valueDescription != NULL) {
          OCFRelease(valueDescription);
      }
      return true;
    });
    OCFStringAppendFormat(result, NULL, OF_STR("%@}\n"), prefix);
    return result;
}

// Native __CFBasicHashEqual, 0x1c02b9710. Hidden (was private external). A tail call.
Boolean __CFBasicHashEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    return CFBasicHashesAreEqual((BasicHashRef)cf1, (BasicHashRef)cf2);
}

// Native __CFBasicHashHash, 0x1c02b9714. Hidden (was private external). The element count (a tail call).
OCFHashCode __CFBasicHashHash(OCFTypeRef cf) {
    return (OCFHashCode)CFBasicHashGetCount((BasicHashRef)cf);
}

// Native __CFBasicHashCopyDescription, 0x1c02b9718. Hidden (was private external).
OCFStringRef __CFBasicHashCopyDescription(OCFTypeRef cf) {
    OCFMutableStringRef description =
        CFBasicHashCopyDescription((BasicHashRef)cf, false, OF_STR(""), OF_STR("\t"), true);
    OCFStringRef result = OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault,
        NULL,
        OF_STR("<CFBasicHash %p [%p]>%@"),
        cf,
        OCFGetAllocator(cf),
        description
    );
    OCFRelease(description);
    return result;
}

// Native __CFBasicHashDeallocate, 0x1c02b97a4. Hidden (was private external). A second finalization halts;
// the object is marked finalized and drained.
void __CFBasicHashDeallocate(OCFTypeRef cf) {
    BasicHashRef ht = (BasicHashRef)cf;
    uint64_t callbackWord = ht->callbackWord;
    if ((callbackWord & kCallbackWordFinalized) != 0) {
        __builtin_trap();
    }
    ht->callbackWord = callbackWord | kCallbackWordFinalized;
    __CFBasicHashDrain(ht);
}

static void setCallbackIndex(uint64_t *word, unsigned shift, int32_t index) {
    *word = (*word & ~((uint64_t)0x3FF << shift)) | ((uint64_t)((uint32_t)index & 0x3FF) << shift);
}

// Native CFBasicHashCreate, 0x1c02b97bc. Hidden (was private external). Creation flags: bit 0 keys, bit 1
// counts, bit 2 hashes, bits 6-12 the value/key options (flags bits 13-14, 9-12, 15), bits 13-14 the hash
// style, bit 15 fast growth. The instance is 32 bytes past the header, 40 with keys, 8 more each for counts
// and hashes. Conflicting options halt. Slots follow the values in the order keys, counts, hashes. The ten
// callbacks are registered in the order retain key, retain value, release key, release value, key
// description, value description, equate keys, equate values, hash key, indirect key.
BasicHashRef
CFBasicHashCreate(OCFAllocatorRef allocator, OCFOptionFlags flags, const BasicHashCallbacks *callbacks) {
    OCFIndex size = (flags & 1) == 0 ? 32 : 40;
    OCFOptionFlags countsFlag = flags & 2;
    OCFOptionFlags hashesFlag = flags & 4;
    BasicHashRef ht = (BasicHashRef)_OCFRuntimeCreateInstance(
        allocator, 3, (OCFIndex)(hashesFlag * 2 + countsFlag * 4) + size, NULL
    );
    if (ht == NULL) {
        return NULL;
    }
    uint16_t bits = (uint16_t)((ht->flags & ~3) | ((flags >> 13) & 3));
    ht->flags = bits;
    if ((flags & 0x8000) != 0) {
        ht->callbackWord |= kCallbackWordFastGrow;
    }
    bits = (uint16_t)(((flags << 7) & 0x6000) | (((flags >> 8) & 0xF) << 9) | ((flags >> 12) << 15) | bits);
    if ((flags & 0x1FC0) != 0) {
        ht->flags = bits;
    }
    ht->mutations = 1;
    if ((~bits & 0xA00) == 0 || (~bits & 0x2200) == 0 || (~bits & 0x1400) == 0 || (~bits & 0x4400) == 0 ||
        (~bits & 0x2800) == 0 || (~bits & 0x5000) == 0 || (~bits & 0x8400) == 0 || (~bits & 0x9000) == 0 ||
        bits >= 0xC000) {
        __builtin_trap();
    }
    unsigned slot = 1;
    if ((flags & 1) != 0) {
        bits |= kFlagKeys;
        ht->flags = bits;
        slot = 2;
    }
    if (countsFlag != 0) {
        bits = (uint16_t)((bits & ~0x18) | ((slot & 3) << 3));
        slot++;
        ht->flags = bits;
    }
    if (hashesFlag != 0) {
        bits = (uint16_t)((bits & 0xFE7F) | (slot << 7));
        ht->flags = bits;
    }
    setCallbackIndex(
        &ht->bucketWord, kRetainKeyShift, CFBasicHashGetPtrIndex((const void *)callbacks->retainKey)
    );
    setCallbackIndex(
        &ht->bucketWord, kRetainValueShift, CFBasicHashGetPtrIndex((const void *)callbacks->retainValue)
    );
    setCallbackIndex(
        &ht->bucketWord, kReleaseKeyShift, CFBasicHashGetPtrIndex((const void *)callbacks->releaseKey)
    );
    setCallbackIndex(
        &ht->bucketWord, kReleaseValueShift, CFBasicHashGetPtrIndex((const void *)callbacks->releaseValue)
    );
    setCallbackIndex(
        &ht->callbackWord,
        kKeyDescriptionShift,
        CFBasicHashGetPtrIndex((const void *)callbacks->copyKeyDescription)
    );
    setCallbackIndex(
        &ht->callbackWord,
        kValueDescriptionShift,
        CFBasicHashGetPtrIndex((const void *)callbacks->copyValueDescription)
    );
    setCallbackIndex(
        &ht->callbackWord, kEquateKeysShift, CFBasicHashGetPtrIndex((const void *)callbacks->equateKeys)
    );
    setCallbackIndex(
        &ht->callbackWord, kEquateValuesShift, CFBasicHashGetPtrIndex((const void *)callbacks->equateValues)
    );
    setCallbackIndex(
        &ht->callbackWord, kHashKeyShift, CFBasicHashGetPtrIndex((const void *)callbacks->hashKey)
    );
    setCallbackIndex(
        &ht->callbackWord, kIndirectKeyShift, CFBasicHashGetPtrIndex((const void *)callbacks->getIndirectKey)
    );
    return ht;
}

// Native CFBasicHashGetPtrIndex, 0x1c02b9a10, with CFBasicHashGetPtrIndex.cold.1 (0x1c0311fd4, the
// dispatch_once call) and __CFBasicHashGetPtrIndex_block_invoke (0x1c02bb090). Non-external. The first call
// allocates page 0 (malloc type 0x80040B8603338; failure halts) holding NULL, CFCopyDescription,
// __CFTypeCollectionRelease, __CFTypeCollectionRetain, CFEqual, CFHash, __CFStringCollectionCopy and NULL.
// Under the lock, answers a registered pointer's index or appends it (1024 entries halt; a full page count
// adds an unchecked 256-byte page).
static int32_t CFBasicHashGetPtrIndex(const void *ptr) {
    dispatch_once(&CFBasicHashGetPtrIndexOnce, ^{
      CFBasicHashCallBackPtrsLock = OS_UNFAIR_LOCK_INIT;
      void **page = malloc_type_malloc(0x100, 0x80040B8603338);
      CFBasicHashCallBackPtrs[0] = page;
      if (page == NULL) {
          __builtin_trap();
      }
      page[0] = NULL;
      page[1] = (void *)OCFCopyDescription;
      page[2] = (void *)__CFTypeCollectionRelease;
      page[3] = (void *)__CFTypeCollectionRetain;
      page[4] = (void *)OCFEqual;
      page[5] = (void *)OCFHash;
      page[6] = (void *)__CFStringCollectionCopy;
      page[7] = NULL;
      __atomic_store_n(&CFBasicHashCallBackPtrsCount, 8, __ATOMIC_RELEASE);
    });
    os_unfair_lock_lock_with_options(&CFBasicHashCallBackPtrsLock, 0x10000);
    int32_t index = 0;
    if (__atomic_load_n(&CFBasicHashCallBackPtrsCount, __ATOMIC_ACQUIRE) >= 1) {
        do {
            if (CFBasicHashCallBackPtrs[(uint32_t)index >> 5][index & 0x1F] == ptr) {
                os_unfair_lock_unlock(&CFBasicHashCallBackPtrsLock);
                return index;
            }
            index++;
        } while (index < __atomic_load_n(&CFBasicHashCallBackPtrsCount, __ATOMIC_ACQUIRE));
    }
    if (__atomic_load_n(&CFBasicHashCallBackPtrsCount, __ATOMIC_ACQUIRE) >= 1024) {
        __builtin_trap();
    }
    int32_t count = __atomic_load_n(&CFBasicHashCallBackPtrsCount, __ATOMIC_ACQUIRE);
    if (count == __atomic_load_n(&CFBasicHashCallBackPtrsCapacity, __ATOMIC_ACQUIRE)) {
        void **page = malloc_type_malloc(0x100, 0x80040B8603338);
        CFBasicHashCallBackPtrs[__atomic_load_n(&CFBasicHashCallBackPtrsCount, __ATOMIC_ACQUIRE) / 32] = page;
        __atomic_fetch_add(&CFBasicHashCallBackPtrsCapacity, 32, __ATOMIC_SEQ_CST);
    }
    index = __atomic_fetch_add(&CFBasicHashCallBackPtrsCount, 1, __ATOMIC_SEQ_CST);
    CFBasicHashCallBackPtrs[index / 32][index % 32] = (void *)ptr;
    os_unfair_lock_unlock(&CFBasicHashCallBackPtrsLock);
    return index;
}

// Native CFBasicHashCreateCopy, 0x1c02b9b3c, with CFBasicHashCreateCopy.cold.1 (0x1c0311fe8). Hidden (was
// private external). Allocates the stores with the given allocator (type 0xE20D7948, hint 0, not cleared; a
// failure answers NULL without freeing the stores already allocated), creates the instance, copies the
// mutation, flags, used, deleted/index and callback words (finalized cleared; mutations reset to 1), copies
// empty and deleted words as they are and retains every live value and key into the new stores, then copies
// the counts.
BasicHashRef CFBasicHashCreateCopy(OCFAllocatorRef allocator, BasicHashRef ht) {
    OCFIndex objectBytes = objectSize(ht->flags);
    OCFIndex size = __CFBasicHashTableSizes[bucketIndexOf(ht)];
    uintptr_t *newValues = NULL;
    uintptr_t *newKeys = NULL;
    void *newCounts = NULL;
    if (size >= 1) {
        OCFIndex bytes;
        if (__builtin_mul_overflow(size, 8, &bytes)) {
            crashWithMessage("overflow while growing CFBasicHash");
        }
        newValues = OCFAllocatorAllocateTyped(allocator, bytes, 0xE20D7948, 0);
        if (newValues == NULL) {
            return NULL;
        }
        if (__OCFOASafe) {
            __OCFSetLastAllocationEventName(newValues, "CFBasicHash (value-store)");
        }
        if ((ht->flags & kFlagKeys) != 0) {
            newKeys = OCFAllocatorAllocateTyped(allocator, bytes, 0xE20D7948, 0);
            if (newKeys == NULL) {
                return NULL;
            }
            if (__OCFOASafe) {
                __OCFSetLastAllocationEventName(newKeys, "CFBasicHash (key-store)");
            }
        }
        uint16_t flags = ht->flags;
        if ((flags & kFlagCountsOffsetMask) != 0) {
            newCounts = OCFAllocatorAllocateTyped(allocator, size << ((flags >> 5) & 3), 0xE20D7948, 0);
            if (newCounts == NULL) {
                return NULL;
            }
            if (__OCFOASafe) {
                __OCFSetLastAllocationEventName(newCounts, "CFBasicHash (count-store)");
            }
        }
    }
    BasicHashRef copy = (BasicHashRef)_OCFRuntimeCreateInstance(allocator, 3, objectBytes - 16, NULL);
    if (copy == NULL) {
        return NULL;
    }
    uint64_t callbackWord = ht->callbackWord;
    copy->mutations = ht->mutations;
    copy->flags = ht->flags;
    copy->usedBuckets = ht->usedBuckets;
    copy->bucketWord = ht->bucketWord;
    copy->callbackWord = callbackWord & ~(uint64_t)kCallbackWordFinalized;
    copy->mutations = 1;
    if (size == 0) {
        return copy;
    }
    uintptr_t *oldValues = ht->pointers[0];
    uint16_t flags = ht->flags;
    uintptr_t *oldKeys = (flags & kFlagKeys) != 0 ? ht->pointers[1] : NULL;
    void *oldCounts =
        (flags & kFlagCountsOffsetMask) != 0 ? *slotAt(ht, flags & kFlagCountsOffsetMask) : NULL;
    copy->pointers[0] = newValues;
    if (newKeys != NULL) {
        copy->pointers[(copy->flags & kFlagKeys) != 0 ? 1 : 0] = newKeys;
    }
    if (newCounts != NULL) {
        *slotAt(copy, copy->flags & kFlagCountsOffsetMask) = newCounts;
    }
    for (OCFIndex index = 0; index < size; index++) {
        uintptr_t stored = oldValues[index];
        if (stored - 1 > (uintptr_t)-3) {
            ((uintptr_t *)copy->pointers[0])[index] = stored;
            if (newKeys != NULL) {
                if ((copy->flags & kFlagKeys) == 0) {
                    __builtin_trap();
                }
                ((uintptr_t *)copy->pointers[1])[index] = stored;
            }
        } else {
            uintptr_t stackValue = retainValue(copy, unescapeStored(stored));
            ((uintptr_t *)copy->pointers[0])[index] = escapeStack(stackValue);
            if (newKeys != NULL) {
                uintptr_t stackKey = retainKey(copy, unescapeStored(oldKeys[index]));
                if ((copy->flags & kFlagKeys) == 0) {
                    __builtin_trap();
                }
                ((uintptr_t *)copy->pointers[1])[index] = escapeStack(stackKey);
            }
        }
    }
    if (newCounts != NULL && oldCounts != NULL) {
        memmove(newCounts, oldCounts, (size_t)size << ((copy->flags >> 5) & 3));
    }
    return copy;
}

// The probe sequences of the find functions, expanded in each (the native functions are separate copies of
// one template). Linear: start at hash % size, step 1. Double: step (hash / size) % size, or size - 1 for 0.
// Exponential: the double step scaled by successive powers of the table's primitive root, both reduced by
// size only when not already below it.
typedef enum {
    kProbeLinear,
    kProbeDouble,
    kProbeExponential,
} ProbeKind;

typedef struct {
    uintptr_t size;
    uintptr_t start;
    uintptr_t step;
    uintptr_t root;
    uintptr_t power;
    uintptr_t probe;
} ProbeState;

static inline __attribute__((always_inline)) ProbeState
beginProbe(BasicHashRef ht, uintptr_t stackKey, ProbeKind kind) {
    ProbeState state;
    uint8_t bucketIndex = bucketIndexOf(ht);
    state.size = (uintptr_t)__CFBasicHashTableSizes[bucketIndex];
    BasicHashHashCallback hashKey = callbackAt(ht->callbackWord, kHashKeyShift);
    uintptr_t hashCode = stackKey;
    if (hashKey != NULL) {
        hashCode = hashKey((const void *)stackKey);
    }
    state.start = hashCode % state.size;
    state.probe = state.start;
    state.step = 1;
    state.root = 0;
    state.power = 0;
    if (kind != kProbeLinear) {
        uintptr_t step = (hashCode / state.size) % state.size;
        state.step = step == 0 ? state.size - 1 : step;
    }
    if (kind == kProbeExponential) {
        state.root = __CFBasicHashPrimitiveRoots[bucketIndex];
        state.power = state.root;
    }
    return state;
}

static inline __attribute__((always_inline)) void advanceProbe(ProbeState *state, ProbeKind kind) {
    if (kind == kProbeExponential) {
        uintptr_t probe = state->power * state->step + state->start;
        if (probe >= state->size) {
            probe = probe % state->size;
        }
        state->probe = probe;
        uintptr_t power = state->power * state->root;
        if (power >= state->size) {
            power = power % state->size;
        }
        state->power = power;
    } else {
        uintptr_t next = state->probe + state->step;
        state->probe = state->size > next ? next : next - state->size;
    }
}

// The template of the six find functions: walks at most size buckets, remembering the first deleted one; an
// empty bucket ends the walk (the remembered deleted bucket or the empty one); a stored key (indirect: the
// indirect key of the stored value) identical to the probe key or equated with it is found; an exhausted walk
// answers the remembered deleted bucket or -1.
static inline __attribute__((always_inline)) BasicHashBucket
findBucketTemplate(BasicHashRef ht, uintptr_t stackKey, ProbeKind kind, bool indirect) {
    ProbeState state = beginProbe(ht, stackKey, kind);
    const uintptr_t *keys = (ht->flags & kFlagKeys) == 0 ? ht->pointers[0] : ht->pointers[1];
    OCFIndex deletedIndex = -1;
    BasicHashBucket result;
    for (uintptr_t remaining = state.size;;) {
        uintptr_t stored = keys[state.probe];
        if (stored == kDeletedMarker) {
            if (deletedIndex == -1) {
                deletedIndex = (OCFIndex)state.probe;
            }
        } else if (stored == kEmptyMarker) {
            result.index = deletedIndex == -1 ? (OCFIndex)state.probe : deletedIndex;
            result.key = 0;
            result.value = 0;
            result.count = 0;
            return result;
        } else {
            uintptr_t candidate = unescapeStored(stored);
            if (indirect) {
                BasicHashIndirectKeyCallback getIndirect = callbackAt(ht->callbackWord, kIndirectKeyShift);
                if (getIndirect != NULL) {
                    candidate = (uintptr_t)getIndirect((const void *)candidate);
                }
            }
            bool found = candidate == stackKey;
            if (!found) {
                BasicHashEquateCallback equate = callbackAt(ht->callbackWord, kEquateKeysShift);
                found = equate != NULL && equate((const void *)candidate, (const void *)stackKey);
            }
            if (found) {
                result.key = candidate;
                result.value = unescapeStored(((uintptr_t *)ht->pointers[0])[state.probe]);
                result.index = (OCFIndex)state.probe;
                result.count = countAt(ht, (OCFIndex)state.probe);
                return result;
            }
        }
        advanceProbe(&state, kind);
        if (--remaining == 0) {
            result.key = 0;
            result.value = 0;
            result.index = deletedIndex;
            result.count = 0;
            return result;
        }
    }
}

// The template of the six NoCollision functions (the indirect and direct copies are identical): the first
// empty bucket along the probe sequence, or -1.
static inline __attribute__((always_inline)) OCFIndex
findFreeBucketTemplate(BasicHashRef ht, uintptr_t stackKey, ProbeKind kind) {
    ProbeState state = beginProbe(ht, stackKey, kind);
    const uintptr_t *keys = (ht->flags & kFlagKeys) == 0 ? ht->pointers[0] : ht->pointers[1];
    for (uintptr_t remaining = state.size;;) {
        if (keys[state.probe] == kEmptyMarker) {
            return (OCFIndex)state.probe;
        }
        advanceProbe(&state, kind);
        if (--remaining == 0) {
            return -1;
        }
    }
}

// Native ___CFBasicHashFindBucket_Linear_Indirect, 0x1c02b9f00. Non-external.
static BasicHashBucket ___CFBasicHashFindBucket_Linear_Indirect(BasicHashRef ht, uintptr_t stackKey) {
    return findBucketTemplate(ht, stackKey, kProbeLinear, true);
}

// Native ___CFBasicHashFindBucket_Double_Indirect, 0x1c02ba118. Non-external.
static BasicHashBucket ___CFBasicHashFindBucket_Double_Indirect(BasicHashRef ht, uintptr_t stackKey) {
    return findBucketTemplate(ht, stackKey, kProbeDouble, true);
}

// Native ___CFBasicHashFindBucket_Exponential_Indirect, 0x1c02ba344. Non-external.
static BasicHashBucket ___CFBasicHashFindBucket_Exponential_Indirect(BasicHashRef ht, uintptr_t stackKey) {
    return findBucketTemplate(ht, stackKey, kProbeExponential, true);
}

// Native ___CFBasicHashFindBucket_Linear, 0x1c02ba5c0. Non-external.
static BasicHashBucket ___CFBasicHashFindBucket_Linear(BasicHashRef ht, uintptr_t stackKey) {
    return findBucketTemplate(ht, stackKey, kProbeLinear, false);
}

// Native ___CFBasicHashFindBucket_Double, 0x1c02ba7bc. Non-external.
static BasicHashBucket ___CFBasicHashFindBucket_Double(BasicHashRef ht, uintptr_t stackKey) {
    return findBucketTemplate(ht, stackKey, kProbeDouble, false);
}

// Native ___CFBasicHashFindBucket_Exponential, 0x1c02ba9cc. Non-external.
static BasicHashBucket ___CFBasicHashFindBucket_Exponential(BasicHashRef ht, uintptr_t stackKey) {
    return findBucketTemplate(ht, stackKey, kProbeExponential, false);
}

// Native ___CFBasicHashFindBucket_Linear_Indirect_NoCollision, 0x1c02bac18. Non-external.
static OCFIndex ___CFBasicHashFindBucket_Linear_Indirect_NoCollision(BasicHashRef ht, uintptr_t stackKey) {
    return findFreeBucketTemplate(ht, stackKey, kProbeLinear);
}

// Native ___CFBasicHashFindBucket_Double_Indirect_NoCollision, 0x1c02bacb8. Non-external.
static OCFIndex ___CFBasicHashFindBucket_Double_Indirect_NoCollision(BasicHashRef ht, uintptr_t stackKey) {
    return findFreeBucketTemplate(ht, stackKey, kProbeDouble);
}

// Native ___CFBasicHashFindBucket_Exponential_Indirect_NoCollision, 0x1c02bad6c. Non-external.
static OCFIndex
___CFBasicHashFindBucket_Exponential_Indirect_NoCollision(BasicHashRef ht, uintptr_t stackKey) {
    return findFreeBucketTemplate(ht, stackKey, kProbeExponential);
}

// Native ___CFBasicHashFindBucket_Linear_NoCollision, 0x1c02bae54. Non-external.
static OCFIndex ___CFBasicHashFindBucket_Linear_NoCollision(BasicHashRef ht, uintptr_t stackKey) {
    return findFreeBucketTemplate(ht, stackKey, kProbeLinear);
}

// Native ___CFBasicHashFindBucket_Double_NoCollision, 0x1c02baef4. Non-external.
static OCFIndex ___CFBasicHashFindBucket_Double_NoCollision(BasicHashRef ht, uintptr_t stackKey) {
    return findFreeBucketTemplate(ht, stackKey, kProbeDouble);
}

// Native ___CFBasicHashFindBucket_Exponential_NoCollision, 0x1c02bafa8. Non-external.
static OCFIndex ___CFBasicHashFindBucket_Exponential_NoCollision(BasicHashRef ht, uintptr_t stackKey) {
    return findFreeBucketTemplate(ht, stackKey, kProbeExponential);
}
