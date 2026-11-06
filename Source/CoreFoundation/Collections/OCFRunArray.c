//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Collections/OCFRunArray.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"

#include <os/lock.h>

// Inferred organization: __CFRunArrayDeallocate (0x1c01db10c) through CFRunArrayReplace (0x1c01db898) form
// one cluster of the iOS Simulator __text section between the XML property list parser and CFLocale; it is
// kept in Collections/ (the run array is the attributed string's attribute storage). The native source path
// is not in the evidence. Restored: the class callbacks __CFRunArrayDeallocate, __CFRunArrayEqual,
// __CFRunArrayHash and __CFRunArrayCopyDescription, CFRunArrayGetTypeID, _CFRunArrayCreateWithGuts,
// __CFRunArrayMakeNewList, CFRunArrayCreate, CFRunArrayGetValueAtIndex, blockForLocation, CFRunArrayInsert,
// __CFRunArraySetBlockCapacity, CFRunArrayDelete and CFRunArrayReplace, and the class record of type 63. Not
// restored (not in the evidence): CFRunArrayGetCount (0x1c01db420) and CFRunArrayGetValueAtRunArrayIndex
// (0x1c01db4fc). Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; pseudocode used as a
// guide only); iOS Device 24A446: unread. Reading: not yet written; evidence
// Local/Evidence/cf-core/iphone-simulator-corpus-delta4.json and, for the class callbacks and
// CFRunArrayGetTypeID, iphone-simulator-corpus-delta5.json.

enum {
    kRunArrayTypeID = 63,
};

// The malloc type descriptor of every guts allocation.
static const OCFAllocatorTypeID kRunArrayGutsType = 0x1000040EED21634;

// Native runArrayLock, 0x22e6d2ea8 (__DATA,__bss). Non-external; guards the guts reference counts.
static os_unfair_lock runArrayLock;

static OCFIndex blockForLocation(RunArrayGuts *guts, OCFIndex location, OCFRange *effectiveRange);
static void __CFRunArraySetBlockCapacity(OCFRunArrayRef array, OCFIndex capacity);
static void __CFRunArrayDeallocate(OCFTypeRef cf);
static Boolean __CFRunArrayEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFRunArrayHash(OCFTypeRef cf);
static OCFStringRef __CFRunArrayCopyDescription(OCFTypeRef cf);

// Native __CFRunArrayClass, 0x2227a3aa8 (__DATA_CONST,__const). Hidden (was a private external); version 0,
// name "CFRunArray", finalize, equal, hash and the debug description (the record's words, read with
// macho.py).
const OCFRuntimeClass __CFRunArrayClass = {
    0,
    "CFRunArray",
    NULL,
    NULL,
    __CFRunArrayDeallocate,
    __CFRunArrayEqual,
    __CFRunArrayHash,
    NULL,
    __CFRunArrayCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CFRunArrayDeallocate, 0x1c01db10c. Non-external. Under the lock, guts with more than one
// reference lose one (the store and the unlock end the call). The last reference unlocks, releases every
// block's object (the count read again after each release) and frees the guts with the array's allocator
// (a tail call); the count of the freed guts is not written.
static void __CFRunArrayDeallocate(OCFTypeRef cf) {
    RunArrayGuts *guts = ((OCFRunArrayRef)cf)->guts;
    os_unfair_lock_lock(&runArrayLock);
    OCFIndex numRefs = guts->numRefs - 1;
    if (numRefs > 0) {
        guts->numRefs = numRefs;
        os_unfair_lock_unlock(&runArrayLock);
        return;
    }
    os_unfair_lock_unlock(&runArrayLock);
    for (OCFIndex idx = 0; idx < guts->numBlocks; idx++) {
        OCFRelease(guts->list[idx].obj);
    }
    OCFAllocatorDeallocate(OCFGetAllocator(cf), guts);
}

// Native __CFRunArrayEqual, 0x1c01db1ac. Non-external. Run arrays are never equal.
static Boolean __CFRunArrayEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    (void)cf1;
    (void)cf2;
    return false;
}

// Native __CFRunArrayHash, 0x1c01db1b4. Non-external. The total length.
static OCFHashCode __CFRunArrayHash(OCFTypeRef cf) {
    return (OCFHashCode)((OCFRunArrayRef)cf)->guts->length;
}

// Native __CFRunArrayCopyDescription, 0x1c01db1c0. Non-external. A mutable string of
// kCFAllocatorSystemDefault (no maximum) receives "%ld blocks used, total length %ld (%ld blocks, block %ld
// is at %ld)\n" with the block count, length, capacity, cached block and cached location, then " %ld %p
// %@\n" with each block's length and object twice (the count read again after each block). The strings are
// the __cfstring constants 0x2227b0ce0 and 0x2227b0d00.
static OCFStringRef __CFRunArrayCopyDescription(OCFTypeRef cf) {
    RunArrayGuts *guts = ((OCFRunArrayRef)cf)->guts;
    OCFMutableStringRef description = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    OCFStringAppendFormat(
        description,
        NULL,
        OF_STR("%ld blocks used, total length %ld (%ld blocks, block %ld is at %ld)\n"),
        guts->numBlocks,
        guts->length,
        guts->maxBlocks,
        guts->cachedBlock,
        guts->cachedLocation
    );
    for (OCFIndex idx = 0; idx < guts->numBlocks; idx++) {
        OCFStringAppendFormat(
            description,
            NULL,
            OF_STR(" %ld %p %@\n"),
            guts->list[idx].length,
            guts->list[idx].obj,
            guts->list[idx].obj
        );
    }
    return description;
}

// Native CFRunArrayGetTypeID, 0x1c01db284. External; always 63.
OCFTypeID OCFRunArrayGetTypeID(void) {
    return kRunArrayTypeID;
}

// Native _CFRunArrayCreateWithGuts, 0x1c01db28c. External. A run array (type 63, 8 extra bytes) shares the
// given guts (stored, then their reference count incremented under the lock), or gets new guts of two blocks
// (0x50 bytes on the array's allocator): no blocks, no length, cache 0, one reference.
OCFRunArrayRef _OCFRunArrayCreateWithGuts(OCFAllocatorRef allocator, RunArrayGuts *guts) {
    OCFRunArrayRef array = (OCFRunArrayRef)_OCFRuntimeCreateInstance(allocator, kRunArrayTypeID, 8, NULL);
    if (array == NULL) {
        return NULL;
    }
    if (guts != NULL) {
        array->guts = guts;
        os_unfair_lock_lock(&runArrayLock);
        array->guts->numRefs++;
        os_unfair_lock_unlock(&runArrayLock);
        return array;
    }
    RunArrayGuts *newGuts =
        (RunArrayGuts *)OCFAllocatorAllocateTyped(OCFGetAllocator(array), 0x50, kRunArrayGutsType, 0);
    newGuts->numBlocks = 0;
    newGuts->maxBlocks = 2;
    newGuts->cachedBlock = 0;
    newGuts->cachedLocation = 0;
    newGuts->numRefs = 1;
    newGuts->length = 0;
    array->guts = newGuts;
    return array;
}

// Native __CFRunArrayMakeNewList, 0x1c01db334. Non-external; called with runArrayLock held. New guts with the
// old capacity (2 without old guts; 48 + 16 per block on the array's allocator) receive the old blocks (each
// object retained; the count is read again after every block), the old block count, cache and length, and
// the old guts lose a reference (a plain decrement); without old guts they start empty. The new guts have one
// reference and replace the array's.
static void __CFRunArrayMakeNewList(OCFRunArrayRef array, RunArrayGuts *oldGuts) {
    OCFIndex capacity = oldGuts != NULL ? oldGuts->maxBlocks : 2;
    RunArrayGuts *newGuts = (RunArrayGuts *)OCFAllocatorAllocateTyped(
        OCFGetAllocator(array), capacity * 16 + 48, kRunArrayGutsType, 0
    );
    newGuts->maxBlocks = capacity;
    if (oldGuts != NULL) {
        OCFIndex numBlocks = oldGuts->numBlocks;
        for (OCFIndex idx = 0; idx < numBlocks; idx++) {
            newGuts->list[idx].length = oldGuts->list[idx].length;
            newGuts->list[idx].obj = OCFRetain(oldGuts->list[idx].obj);
            numBlocks = oldGuts->numBlocks;
        }
        newGuts->numBlocks = numBlocks;
        newGuts->cachedBlock = oldGuts->cachedBlock;
        newGuts->cachedLocation = oldGuts->cachedLocation;
        OCFIndex numRefs = oldGuts->numRefs;
        newGuts->length = oldGuts->length;
        oldGuts->numRefs = numRefs - 1;
    } else {
        newGuts->length = 0;
        newGuts->numBlocks = 0;
        newGuts->cachedBlock = 0;
        newGuts->cachedLocation = 0;
    }
    newGuts->numRefs = 1;
    array->guts = newGuts;
}

// Native CFRunArrayCreate, 0x1c01db418. External; new guts (a tail call).
OCFRunArrayRef OCFRunArrayCreate(OCFAllocatorRef allocator) {
    return _OCFRunArrayCreateWithGuts(allocator, NULL);
}

// Native CFRunArrayGetValueAtIndex, 0x1c01db42c. External. The object of the block holding the location; the
// block's range (when asked) comes from blockForLocation and its index through blockIndex (when not NULL).
// No bounds check.
OCFTypeRef OCFRunArrayGetValueAtIndex(
    OCFRunArrayRef array, OCFIndex location, OCFRange *effectiveRange, OCFIndex *blockIndex
) {
    OCFIndex block = blockForLocation(array->guts, location, effectiveRange);
    if (blockIndex != NULL) {
        *blockIndex = block;
    }
    return array->guts->list[block].obj;
}

// Native blockForLocation, 0x1c01db468. Non-external. The search starts at the cached block when the location
// lies beyond half the cached location (signed halving), else at block 0 and location 0; it walks backward
// while the block starts after the location (stopping at block 0), or forward while the next block starts at
// or before it. The block and its start become the cache; the range (when asked) is the start and the block's
// length.
static OCFIndex blockForLocation(RunArrayGuts *guts, OCFIndex location, OCFRange *effectiveRange) {
    OCFIndex start = guts->cachedLocation;
    OCFIndex block;
    if (location <= start / 2) {
        start = 0;
        block = 0;
    } else {
        block = guts->cachedBlock;
    }
    OCFIndex blockStart;
    if (start > location) {
        blockStart = start;
        do {
            blockStart -= guts->list[block - 1].length;
            if (block < 2) {
                block--;
                break;
            }
            block--;
        } while (blockStart > location);
    } else {
        block--;
        OCFIndex next = start;
        do {
            blockStart = next;
            next = blockStart + guts->list[block + 1].length;
            block++;
        } while (next <= location);
    }
    guts->cachedBlock = block;
    guts->cachedLocation = blockStart;
    if (effectiveRange != NULL) {
        effectiveRange->location = blockStart;
        effectiveRange->length = guts->list[block].length;
    }
    return block;
}

// Native CFRunArrayInsert, 0x1c01db52c. External. A zero length does nothing. Shared guts are copied under
// the lock first. At the end of the array the length joins the last block when its object is equal (moving
// the cached location when the cache is at or past that block), else a new block is appended (the first block
// of an empty array without the equality test). Inside the array the length joins the block holding the
// location when equal, or the previous block when the location starts the block and that object is equal
// (moving the cached location when the cache is at or past the block); otherwise a new block is inserted
// before the block (location at its start) or the block is split around a new one (location inside it, the
// tail retaining the block's object again). The total length grows by the inserted length.
void OCFRunArrayInsert(OCFRunArrayRef array, OCFRange range, OCFTypeRef obj) {
    if (range.length == 0) {
        return;
    }
    RunArrayGuts *guts = array->guts;
    os_unfair_lock_lock(&runArrayLock);
    if (guts->numRefs >= 2) {
        __CFRunArrayMakeNewList(array, guts);
        guts = array->guts;
    }
    os_unfair_lock_unlock(&runArrayLock);
    if (range.location == guts->length) {
        if (range.location < 1) {
            __CFRunArraySetBlockCapacity(array, 1);
            guts = array->guts;
            OCFTypeRef retained = OCFRetain(obj);
            guts->list[0].length = range.length;
            guts->list[0].obj = retained;
        } else {
            Boolean equal = OCFEqual(obj, guts->list[guts->numBlocks - 1].obj);
            OCFIndex numBlocks = guts->numBlocks;
            if (equal) {
                guts->list[numBlocks - 1].length += range.length;
                if (guts->cachedBlock >= numBlocks) {
                    guts->cachedLocation += range.length;
                }
                guts->length += range.length;
                return;
            }
            __CFRunArraySetBlockCapacity(array, numBlocks + 1);
            guts = array->guts;
            OCFTypeRef retained = OCFRetain(obj);
            numBlocks = guts->numBlocks;
            guts->list[numBlocks].length = range.length;
            guts->list[numBlocks].obj = retained;
            guts->numBlocks = numBlocks + 1;
            guts->length += range.length;
            return;
        }
        guts->numBlocks = guts->numBlocks + 1;
        guts->length += range.length;
        return;
    }
    OCFRange blockRange = {0, 0};
    OCFIndex block = blockForLocation(guts, range.location, &blockRange);
    RunArrayItem *item = &guts->list[block];
    if (OCFEqual(obj, item->obj)) {
        item->length += range.length;
        guts->length += range.length;
        return;
    }
    OCFIndex blockStart = blockRange.location;
    if (block >= 1 && blockStart == range.location && OCFEqual(obj, item[-1].obj)) {
        item[-1].length += range.length;
        if (guts->cachedBlock >= block) {
            guts->cachedLocation += range.length;
        }
        guts->length += range.length;
        return;
    }
    OCFIndex numBlocks = guts->numBlocks;
    OCFIndex tailOffset = blockStart - range.location;
    if (tailOffset == 0) {
        __CFRunArraySetBlockCapacity(array, numBlocks + 1);
        guts = array->guts;
        for (OCFIndex idx = guts->numBlocks; idx > block; idx--) {
            guts->list[idx] = guts->list[idx - 1];
        }
        OCFTypeRef retained = OCFRetain(obj);
        guts->list[block].length = range.length;
        guts->list[block].obj = retained;
        guts->numBlocks = guts->numBlocks + 1;
    } else {
        __CFRunArraySetBlockCapacity(array, numBlocks + 2);
        guts = array->guts;
        for (OCFIndex idx = guts->numBlocks + 1; idx >= block + 2; idx--) {
            guts->list[idx] = guts->list[idx - 2];
        }
        OCFTypeRef retained = OCFRetain(obj);
        guts->list[block + 1].length = range.length;
        guts->list[block + 1].obj = retained;
        guts->list[block].length = range.location - blockStart;
        OCFTypeRef tailObject = OCFRetain(guts->list[block].obj);
        guts->list[block + 2].length = tailOffset + blockRange.length;
        guts->list[block + 2].obj = tailObject;
        guts->numBlocks = guts->numBlocks + 2;
    }
    guts->length += range.length;
}

// Native __CFRunArraySetBlockCapacity, 0x1c01db7f8. Non-external. The capacity (at least 1) is applied only
// when the guts hold fewer blocks or more than twice as many (signed halving); the guts are then reallocated
// (__CFSafelyReallocateWithAllocatorTyped on the array's allocator, fetched first) for 4 * ((capacity + 3) /
// 3) blocks (unsigned division).
static void __CFRunArraySetBlockCapacity(OCFRunArrayRef array, OCFIndex capacity) {
    OCFIndex wanted = capacity > 1 ? capacity : 1;
    OCFIndex maxBlocks = array->guts->maxBlocks;
    if (maxBlocks >= wanted && maxBlocks / 2 <= wanted) {
        return;
    }
    uint64_t thirds = ((uint64_t)wanted + 3) / 3;
    OCFAllocatorRef allocator = OCFGetAllocator(array);
    RunArrayGuts *guts = __OCFSafelyReallocateWithAllocatorTyped(
        allocator, array->guts, (OCFIndex)((thirds << 6) | 0x30), kRunArrayGutsType, 0, NULL
    );
    array->guts = guts;
    guts->maxBlocks = (OCFIndex)(thirds << 2);
}

// Native CFRunArrayDelete, 0x1c01db88c. External; a replacement with nothing (a tail call).
void OCFRunArrayDelete(OCFRunArrayRef array, OCFRange range) {
    OCFRunArrayReplace(array, range, NULL, 0);
}

// Native CFRunArrayReplace, 0x1c01db898. External. A zero length does nothing; the new object counts only
// with a nonzero new length. Shared guts are copied under the lock first. The range's length leaves the
// total; the block holding the location loses what the range covers of it (released when emptied), and the
// following blocks lose the rest in turn (each released when emptied). The emptied run of blocks, from the
// first emptied block to the last, is then closed: the cache moves back over a cleared block it named; the
// new object joins the block before the run or the block after it when equal (adding its length to the
// total), or takes the run's first slot (retained); blocks equal across the closed run merge (the later one
// released); the remaining blocks move down and the capacity follows the new count. A new object not placed
// that way is inserted at the location afterwards.
void OCFRunArrayReplace(OCFRunArrayRef array, OCFRange range, OCFTypeRef newObject, OCFIndex newLength) {
    if (range.length == 0) {
        return;
    }
    RunArrayGuts *guts = array->guts;
    OCFTypeRef pending = newLength == 0 ? NULL : newObject;
    os_unfair_lock_lock(&runArrayLock);
    if (guts->numRefs >= 2) {
        __CFRunArrayMakeNewList(array, guts);
        guts = array->guts;
    }
    OCFRange blockRange = {0, 0};
    os_unfair_lock_unlock(&runArrayLock);
    OCFIndex block = blockForLocation(guts, range.location, &blockRange);
    RunArrayItem *item = &guts->list[block];
    guts->length -= range.length;
    OCFIndex remainingInBlock = blockRange.length - range.location + blockRange.location;
    OCFIndex removed = remainingInBlock < range.length ? remainingInBlock : range.length;
    item->length -= removed;
    if (item->length == 0) {
        OCFRelease(item->obj);
    }
    OCFIndex firstFree = item->length != 0 ? block + 1 : block;
    OCFIndex toRemove = range.length - removed;
    if (toRemove != 0) {
        RunArrayItem *next = &guts->list[block + 1];
        do {
            OCFIndex length = next->length;
            removed = toRemove < length ? toRemove : length;
            next->length = length - removed;
            if (length <= toRemove) {
                OCFRelease(next->obj);
            }
            next++;
            block++;
            toRemove -= removed;
        } while (toRemove != 0);
    }
    if (block != 0) {
        block -= guts->list[block].length != 0 ? 1 : 0;
    }
    if (firstFree > block) {
        goto insert;
    }
    if (firstFree >= 1) {
        if (firstFree == guts->cachedBlock) {
            guts->cachedBlock = firstFree - 1;
            guts->cachedLocation = guts->cachedLocation - guts->list[firstFree - 1].length;
        }
        if (pending == NULL) {
            goto mergeAcross;
        }
        if (OCFEqual(guts->list[firstFree - 1].obj, newObject)) {
            guts->list[firstFree - 1].length += newLength;
            guts->length += newLength;
            goto mergeAcross;
        }
    } else if (pending == NULL) {
        goto placed;
    }
    if (block + 1 < guts->numBlocks) {
        if (OCFEqual(guts->list[block + 1].obj, newObject)) {
            guts->list[block + 1].length += newLength;
            guts->length += newLength;
            if (firstFree < 1) {
                goto placed;
            }
            goto mergeAcross;
        }
    }
    if (firstFree < guts->numBlocks) {
        OCFTypeRef retained = OCFRetain(newObject);
        pending = NULL;
        guts->list[firstFree].length = newLength;
        guts->list[firstFree].obj = retained;
        firstFree++;
        guts->length += newLength;
    }
    goto close;
mergeAcross:
    if (block + 1 < guts->numBlocks && OCFEqual(guts->list[firstFree - 1].obj, guts->list[block + 1].obj)) {
        guts->list[firstFree - 1].length += guts->list[block + 1].length;
        OCFRelease(guts->list[block + 1].obj);
        pending = NULL;
        block = block + 1;
        goto close;
    }
placed:
    pending = NULL;
close:
    if (firstFree <= block) {
        OCFIndex numBlocks = guts->numBlocks;
        OCFIndex count = numBlocks - block - 1;
        if (count >= 1) {
            RunArrayItem *source = &guts->list[block + 1];
            RunArrayItem *destination = &guts->list[firstFree];
            do {
                *destination++ = *source++;
            } while (--count != 0);
            numBlocks = guts->numBlocks;
        }
        OCFIndex newCount = numBlocks + (firstFree - (block + 1));
        guts->numBlocks = newCount;
        __CFRunArraySetBlockCapacity(array, newCount);
    }
insert:
    if (pending != NULL) {
        OCFRunArrayInsert(array, (OCFRange){range.location, newLength}, pending);
    }
}
