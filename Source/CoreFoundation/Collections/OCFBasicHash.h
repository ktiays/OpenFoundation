//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <stdbool.h>
#include <stdint.h>

// The CFBasicHash object, read from the iOS Simulator image (24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC); confidence: verified for every offset, width and bit named here.
// Type, field and constant names are local descriptive spellings; the binary carries none.
//
//   0x10  uint16  mutation count (1 after creation)
//   0x12  uint16  flags: bits 0-1 hash style (1 linear, 2 double, 3 exponential), bit 2 keys stored apart
//                 from values (pointer slot 1), bits 3-4 byte offset / 8 of the counts slot, bits 5-6 log2 of
//                 the count width in bytes, bits 7-8 hashes slot, bits 9-14 the value/key options of the
//                 creation flags (pairwise exclusive), bit 15 keys obtained through the indirect-key callback
//   0x14  uint32  used buckets
//   0x18  uint64  bits 0-15 deleted buckets, bits 16-23 bucket-table index, then 10-bit callback indexes:
//                 24 retain key, 34 retain value, 44 release key, 54 release value
//   0x20  uint64  bit 1 retain/release suppressed, bit 2 growth by one extra size step, bit 3 finalized,
//                 then 10-bit callback indexes: 4 key description, 14 value description, 24 equate keys,
//                 34 equate values, 44 hash key, 54 indirect key
//   0x28  pointer slots: values, then keys, counts and hashes as the flags place them
//
// Stored values and keys use 0 for an empty bucket and ~0 for a deleted one; the stack values 0 and ~0 are
// stored as 0xA7BAADB1 and 0xA5BAADB9.
typedef struct BasicHash {
    OCFRuntimeBase base;
    uint16_t mutations;
    uint16_t flags;
    uint32_t usedBuckets;
    uint64_t bucketWord;
    uint64_t callbackWord;
    void *pointers[];
} BasicHash;

_Static_assert(sizeof(BasicHash) == 40, "The native fixed part of a CFBasicHash is 40 bytes.");

typedef BasicHash *BasicHashRef;

// The callbacks CFBasicHashCreate registers, in the order its readers index them (ten pointers). Keys and
// values are pointer-sized words; the CF callback types are used so that collection callbacks need no cast.
typedef const void *(*BasicHashRetainCallback)(OCFAllocatorRef allocator, const void *stack);
typedef void (*BasicHashReleaseCallback)(OCFAllocatorRef allocator, const void *stack);
typedef Boolean (*BasicHashEquateCallback)(const void *collection, const void *stack);
typedef OCFHashCode (*BasicHashHashCallback)(const void *stack);
typedef const void *(*BasicHashIndirectKeyCallback)(const void *collection);
typedef OCFStringRef (*BasicHashDescriptionCallback)(const void *stack);

typedef struct {
    BasicHashRetainCallback retainValue;
    BasicHashRetainCallback retainKey;
    BasicHashReleaseCallback releaseValue;
    BasicHashReleaseCallback releaseKey;
    BasicHashEquateCallback equateValues;
    BasicHashEquateCallback equateKeys;
    BasicHashHashCallback hashKey;
    BasicHashIndirectKeyCallback getIndirectKey;
    BasicHashDescriptionCallback copyValueDescription;
    BasicHashDescriptionCallback copyKeyDescription;
} BasicHashCallbacks;

OF_ABI_ASSERT(sizeof(BasicHashCallbacks) == 80, "The native callback block is ten pointers.");

// A bucket as the lookup functions return it (32 bytes, returned through X8): its index (-1 when no bucket
// is free), the key and value as stack values, and the count (0 when the key is absent).
typedef struct {
    OCFIndex index;
    uintptr_t key;
    uintptr_t value;
    uintptr_t count;
} BasicHashBucket;

OF_ABI_ASSERT(sizeof(BasicHashBucket) == 32, "The native bucket is four words.");

// The enumeration callback of CFBasicHashApply: return false to stop.
typedef Boolean (^BasicHashApplyBlock)(BasicHashBucket bucket);

// The class record (type 3), defined by this unit for the builtin table.
OF_EXTERN const OCFRuntimeClass __CFBasicHashClass;

// Original names, hidden (were private external).
// Native __CFBasicHashGetCallbacks, 0x1c02b6598, and __CFBasicHashFastEnumeration, 0x1c02b706c. Hidden (were
// private external). The callbacks are returned by value (through X8); the enumeration state has the layout
// of NSFastEnumerationState.
OF_EXTERN BasicHashCallbacks __CFBasicHashGetCallbacks(BasicHashRef ht);
OF_EXTERN unsigned long
__CFBasicHashFastEnumeration(BasicHashRef ht, void *state, void *stackbuffer, unsigned long count);
OF_EXTERN BasicHashBucket CFBasicHashGetBucket(BasicHashRef ht, OCFIndex index);
OF_EXTERN BasicHashBucket CFBasicHashFindBucket(BasicHashRef ht, uintptr_t stackKey);
OF_EXTERN void CFBasicHashSuppressRC(BasicHashRef ht);
OF_EXTERN void CFBasicHashUnsuppressRC(BasicHashRef ht);
OF_EXTERN OCFIndex CFBasicHashGetCount(BasicHashRef ht);
OF_EXTERN OCFIndex CFBasicHashGetCountOfKey(BasicHashRef ht, uintptr_t stackKey);
OF_EXTERN void CFBasicHashApply(BasicHashRef ht, BasicHashApplyBlock block);
OF_EXTERN Boolean CFBasicHashesAreEqual(BasicHashRef ht1, BasicHashRef ht2);
OF_EXTERN void
CFBasicHashGetElements(BasicHashRef ht, OCFIndex bufferslen, uintptr_t *weakValues, uintptr_t *weakKeys);
OF_EXTERN void CFBasicHashSetCapacity(BasicHashRef ht, OCFIndex capacity);
OF_EXTERN Boolean CFBasicHashAddValue(BasicHashRef ht, uintptr_t stackKey, uintptr_t stackValue);
OF_EXTERN void CFBasicHashSetValue(BasicHashRef ht, uintptr_t stackKey, uintptr_t stackValue);
OF_EXTERN OCFIndex CFBasicHashRemoveValue(BasicHashRef ht, uintptr_t stackKey);
OF_EXTERN void CFBasicHashRemoveAllValues(BasicHashRef ht);
OF_EXTERN OCFIndex CFBasicHashGetSize(BasicHashRef ht, Boolean total);
OF_EXTERN OCFMutableStringRef CFBasicHashCopyDescription(
    BasicHashRef ht, Boolean detailed, OCFStringRef prefix, OCFStringRef entryPrefix, Boolean describeElements
);
OF_EXTERN Boolean __CFBasicHashEqual(OCFTypeRef cf1, OCFTypeRef cf2);
OF_EXTERN OCFHashCode __CFBasicHashHash(OCFTypeRef cf);
OF_EXTERN OCFStringRef __CFBasicHashCopyDescription(OCFTypeRef cf);
OF_EXTERN void __CFBasicHashDeallocate(OCFTypeRef cf);
OF_EXTERN BasicHashRef
CFBasicHashCreate(OCFAllocatorRef allocator, OCFOptionFlags flags, const BasicHashCallbacks *callbacks);
OF_EXTERN BasicHashRef CFBasicHashCreateCopy(OCFAllocatorRef allocator, BasicHashRef ht);
