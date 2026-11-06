//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Collections/OCFStorage.h"

#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <mach/vm_page_size.h>
#include <os/lock.h>
#include <pthread.h>
#include <string.h>

// Inferred organization: the native CFStorage functions, __CFStorageGetCapacity (0x1c01fb428) through
// ____CFStorageEnumerateNodesInByteRangeWithBlock_block_invoke (0x1c01feb44), are contiguous in the iOS
// Simulator __text section, and __CFStorageClass lies in __DATA_CONST,__const (0x2227a4de0); they are one
// translation unit, kept here in native address order; the native source path is not in the evidence. The
// native unit has 33 functions; the 23 restored here are those of Configuration/Scope.json. The other ten,
// __CFStorageGetCapacity, __CFStorageGetValueSize, CFStorageGetTypeID, _CFStorageFastEnumeration,
// CFStorageApplyFunction and its block, CFStorageApplyBlock, __CFStorageEnumerateNodesInByteRangeWithBlock
// and its block, and __CFStorageSetAlwaysFrozen, lie between them and are outside the scope: no restored
// function calls them and none is a class callback. Unless a definition says otherwise: iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (IDA (corpus delta capture):
// each definition reconciled instruction by instruction with the IDA disassembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta.json; __CFStorageClass's bytes read from the pinned
// image); iOS Device 24A446: unread. Reading: not yet written.

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call passes 0x10000.
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

// The pair of nodes the insertion functions return in X0 and X1: the node that replaces the one inserted
// into, and the new sibling to place after it (or NULL). StorageDoubleNode is a local descriptive spelling.
typedef struct {
    StorageNode *child;
    StorageNode *sibling;
} StorageDoubleNode;

static OCFIndex __CFStorageGetNodeCapacity(StorageNode *node);
static void __CFStorageDeallocate(OCFTypeRef cf);
static Boolean __CFStorageEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFStorageHash(OCFTypeRef cf);
static OCFStringRef __CFStorageCopyDescription(OCFTypeRef cf);
static StorageDoubleNode __CFStorageInsertUnfrozen(
    OCFAllocatorRef allocator,
    OCFStorageRef storage,
    StorageNode *node,
    OCFIndex byteNum,
    OCFIndex size,
    OCFIndex absoluteByteNum
);
static StorageNode *
__CFStorageCreateNode(OCFAllocatorRef allocator, OCFStorageRef storage, bool isLeaf, OCFIndex numBytes);
static StorageNode *__CFStorageDeleteUnfrozen(
    OCFAllocatorRef allocator,
    OCFStorageRef storage,
    StorageNode *node,
    OCFIndex location,
    OCFIndex length,
    bool compact
);
static void __CFStorageClearRootNode(OCFStorageRef storage);
static void __CFStorageDescribeNode(StorageNode *node, OCFMutableStringRef str, OCFIndex level);
static void *__CFStorageFindByte(
    OCFStorageRef storage,
    StorageNode *node,
    OCFIndex byteNum,
    OCFIndex absoluteByteOffsetOfNode,
    StorageNode **resultNode,
    OCFRange *validConsecutiveByteRange,
    bool requireUnfreezing
);
static void __CFStorageDeallocateNode(OCFStorageRef storage, StorageNode *node);
static StorageDoubleNode __CFStorageInsertFrozen(
    OCFAllocatorRef allocator,
    OCFStorageRef storage,
    StorageNode *node,
    OCFIndex byteNum,
    OCFIndex size,
    OCFIndex absoluteByteNum
);
static StorageNode *__CFStorageDeleteFrozen(
    OCFAllocatorRef allocator, OCFStorageRef storage, StorageNode *node, OCFIndex location, OCFIndex length
);

// Native __CFStorageClass, 0x2227a4de0 (__DATA_CONST,__const). Hidden (was private external); version 1, name
// "CFStorage", finalize, equal, hash and the debug description.
const OCFRuntimeClass __CFStorageClass = {
    1,
    "CFStorage",
    NULL,
    NULL,
    __CFStorageDeallocate,
    __CFStorageEqual,
    __CFStorageHash,
    NULL,
    __CFStorageCopyDescription,
    NULL,
    NULL,
    0,
};

// The helpers below reproduce code the native functions inline at each use (no native symbols): the
// value/byte conversions (a shift when byteToValueShifter is not -1, else a multiplication or a signed
// division by the value size), the cache update and read under cacheLock (LDP/STP of the node and range
// between the lock calls), the node retain (a plain load of the count, then LDADD +1 when it is nonzero), the
// release (LDADD -1 when the count is nonzero; __CFStorageDeallocateNode when the old count was 1) and the
// leaf-memory reservation.
static OCFIndex valuesToBytes(OCFStorageRef storage, OCFIndex values) {
    if (storage->byteToValueShifter == UINT32_MAX) {
        return values * storage->valueSize;
    }
    return (OCFIndex)((uint64_t)values << storage->byteToValueShifter);
}

static OCFIndex bytesToValues(OCFStorageRef storage, OCFIndex bytes) {
    if (storage->byteToValueShifter == UINT32_MAX) {
        return bytes / storage->valueSize;
    }
    return bytes >> storage->byteToValueShifter;
}

static void setCache(OCFStorageRef storage, StorageNode *node, OCFIndex location, OCFIndex length) {
    os_unfair_lock_lock_with_options(&storage->cacheLock, 0x10000);
    storage->cacheNode = node;
    storage->cacheRange.location = location;
    storage->cacheRange.length = length;
    os_unfair_lock_unlock(&storage->cacheLock);
}

static StorageNode *readCache(OCFStorageRef storage, OCFRange *range) {
    os_unfair_lock_lock_with_options(&storage->cacheLock, 0x10000);
    StorageNode *node = storage->cacheNode;
    *range = storage->cacheRange;
    os_unfair_lock_unlock(&storage->cacheLock);
    return node;
}

static StorageNode *retainNode(StorageNode *node) {
    if (node->refCount != 0) {
        __atomic_fetch_add(&node->refCount, 1, __ATOMIC_RELAXED);
    }
    return node;
}

static void releaseNode(OCFStorageRef storage, StorageNode *node) {
    if (node->refCount != 0 && __atomic_fetch_sub(&node->refCount, 1, __ATOMIC_RELAXED) == 1) {
        __CFStorageDeallocateNode(storage, node);
    }
}

static void releaseNodeWithNullCheck(OCFStorageRef storage, StorageNode *node) {
    if (node != NULL) {
        releaseNode(storage, node);
    }
}

// A frozen child is thawed in place when this storage holds its only reference; returns whether the caller
// may mutate it.
static bool thawNodeDuringMutation(StorageNode *node) {
    if (node->isFrozen) {
        if (node->refCount != 1) {
            return false;
        }
        node->isFrozen = false;
    }
    return true;
}

// The leaf capacity for cap bytes: above half a page (vm_page_size as a signed value, halved with rounding
// toward zero), the page-rounded value computed in 32 bits and sign-extended, limited to maxLeafCapacity;
// otherwise cap rounded up to a multiple of 64 with signed division.
static OCFIndex roundUpLeafCapacity(OCFStorageRef storage, OCFIndex cap) {
    OCFIndex pageSize = (OCFIndex)vm_page_size;
    if (cap > pageSize / 2) {
        int32_t rounded = (int32_t)(((uint32_t)cap + (uint32_t)pageSize - 1) & -(uint32_t)pageSize);
        return storage->maxLeafCapacity < (OCFIndex)rounded ? storage->maxLeafCapacity : (OCFIndex)rounded;
    }
    return (cap + 63) / 64 * 64;
}

// Reserves the rounded capacity for a leaf (grows only, or any change when compacting), testing the capacity
// again under cacheReaderMemoryAllocationLock. The reallocation passes 0x338795ca in X3 and 0 in X4,
// positionally the hint and type ID parameters of the restored __CFSafelyReallocateWithAllocatorTyped, and no
// failure block.
static void allocLeafNodeMemory(
    OCFAllocatorRef allocator, OCFStorageRef storage, StorageNode *node, OCFIndex cap, bool compact
) {
    OCFIndex newCapacity = roundUpLeafCapacity(storage, cap);
    if (compact ? newCapacity == node->info.leaf.capacityInBytes
                : newCapacity <= node->info.leaf.capacityInBytes) {
        return;
    }
    pthread_mutex_lock(&storage->cacheReaderMemoryAllocationLock);
    if (compact ? newCapacity != node->info.leaf.capacityInBytes
                : newCapacity > node->info.leaf.capacityInBytes) {
        node->info.leaf.memory = __OCFSafelyReallocateWithAllocatorTyped(
            allocator, node->info.leaf.memory, newCapacity, 0x338795ca, 0, NULL
        );
        if (__OCFOASafe) {
            __OCFSetLastAllocationEventName(node->info.leaf.memory, "CFStorage (node bytes)");
        }
        node->info.leaf.capacityInBytes = newCapacity;
    }
    pthread_mutex_unlock(&storage->cacheReaderMemoryAllocationLock);
}

// Native __CFStorageGetNodeCapacity, 0x1c01fb468. Non-external. NULL is 0; a leaf's capacityInBytes;
// otherwise the sum over the children, accumulated as capacity(child[0]) + total + capacity(child[1]) and
// continued with child[2] as a loop (the native code eliminates the third recursive call).
static OCFIndex __CFStorageGetNodeCapacity(StorageNode *node) {
    OCFIndex total = 0;
    if (node == NULL) {
        return 0;
    }
    while (!node->isLeaf) {
        OCFIndex first = __CFStorageGetNodeCapacity(node->info.notLeaf.child[0]);
        OCFIndex second = __CFStorageGetNodeCapacity(node->info.notLeaf.child[1]);
        total = first + total + second;
        node = node->info.notLeaf.child[2];
        if (node == NULL) {
            return total;
        }
    }
    return node->info.leaf.capacityInBytes + total;
}

// Native __CFStorageDeallocate, 0x1c01fb4e4. Non-external; a tail call.
static void __CFStorageDeallocate(OCFTypeRef cf) {
    __CFStorageClearRootNode((OCFStorageRef)cf);
}

// Native __CFStorageEqual, 0x1c01fb4e8. Non-external. Equal counts and value sizes, then memcmp over the runs
// that CFStorageGetValueAtIndex reports for both storages (a run is refetched only when the index leaves it);
// the byte count is (run end - index) * valueSize, a multiplication even for power-of-two sizes.
static Boolean __CFStorageEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    OCFStorageRef storage1 = (OCFStorageRef)cf1;
    OCFStorageRef storage2 = (OCFStorageRef)cf2;
    OCFIndex count = bytesToValues(storage1, storage1->rootNode.numBytes);
    if (count != bytesToValues(storage2, storage2->rootNode.numBytes)) {
        return false;
    }
    OCFIndex valueSize = storage1->valueSize;
    if (valueSize != storage2->valueSize) {
        return false;
    }
    OCFIndex location = 0;
    OCFRange range1 = {0, 0};
    OCFRange range2 = {0, 0};
    const uint8_t *values1 = NULL;
    const uint8_t *values2 = NULL;
    while (location < count) {
        if (location >= range1.location + range1.length) {
            values1 = OCFStorageGetValueAtIndex(storage1, location, &range1);
        }
        if (location >= range2.location + range2.length) {
            values2 = OCFStorageGetValueAtIndex(storage2, location, &range2);
        }
        OCFIndex end2 = range2.location + range2.length;
        OCFIndex end1 = range1.location + range1.length;
        OCFIndex end = end2 < end1 ? end2 : end1;
        OCFIndex numBytes = (end - location) * valueSize;
        int result = memcmp(values1, values2, (size_t)numBytes);
        values1 += numBytes;
        values2 += numBytes;
        location = end;
        if (result != 0) {
            return false;
        }
    }
    return true;
}

// Native __CFStorageHash, 0x1c01fb668. Non-external; the count.
static OCFHashCode __CFStorageHash(OCFTypeRef cf) {
    OCFStorageRef storage = (OCFStorageRef)cf;
    return (OCFHashCode)bytesToValues(storage, storage->rootNode.numBytes);
}

// Native __CFStorageCopyDescription, 0x1c01fb68c. Non-external. A mutable string from the storage's
// allocator, the header line (count and the root's capacity in values) and the node tree from level 0. The
// shift (or the value size) is read once, before the capacity is summed, and converts both values.
static OCFStringRef __CFStorageCopyDescription(OCFTypeRef cf) {
    OCFStorageRef storage = (OCFStorageRef)cf;
    OCFAllocatorRef allocator = OCFGetAllocator(storage);
    OCFMutableStringRef result = OCFStringCreateMutable(allocator, 0);
    OCFIndex numBytes = storage->rootNode.numBytes;
    uint32_t shifter = storage->byteToValueShifter;
    OCFIndex count;
    OCFIndex capacity;
    if (shifter == UINT32_MAX) {
        OCFIndex valueSize = storage->valueSize;
        count = numBytes / valueSize;
        capacity = __CFStorageGetNodeCapacity(&storage->rootNode) / valueSize;
    } else {
        count = numBytes >> shifter;
        capacity = __CFStorageGetNodeCapacity(&storage->rootNode) >> shifter;
    }
    OCFStringAppendFormat(
        result,
        NULL,
        OF_STR("<CFStorage %p [%p]>[count = %lu, capacity = %lu]\n"),
        storage,
        allocator,
        count,
        capacity
    );
    __CFStorageDescribeNode(&storage->rootNode, result, 0);
    return result;
}

// Native CFStorageCreate, 0x1c01fb740. External. Type 26 with 168 extra bytes; the cache lock is cleared, the
// shift is the trailing-zero count when the size is a positive power of two (else -1), the allocation mutex
// gets the error-checking initializer signature, the leaf capacity is 0x3000 reduced to a multiple of a
// nonzero value size (signed remainder), and the root is an empty leaf.
OCFStorageRef OCFStorageCreate(OCFAllocatorRef alloc, OCFIndex valueSizeInBytes) {
    struct __OCFStorage *storage = (struct __OCFStorage *)_OCFRuntimeCreateInstance(alloc, 26, 168, NULL);
    if (storage == NULL) {
        return NULL;
    }
    storage->cacheLock = OS_UNFAIR_LOCK_INIT;
    storage->valueSize = valueSizeInBytes;
    if ((valueSizeInBytes & (valueSizeInBytes - 1)) == 0 && valueSizeInBytes > 0) {
        storage->byteToValueShifter = (uint32_t)__builtin_ctzl((unsigned long)valueSizeInBytes);
    } else {
        storage->byteToValueShifter = UINT32_MAX;
    }
    storage->cacheReaderMemoryAllocationLock = (pthread_mutex_t)PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
    storage->maxLeafCapacity = 0x3000;
    if (valueSizeInBytes != 0) {
        OCFIndex remainder = 0x3000 % valueSizeInBytes;
        if (remainder != 0) {
            storage->maxLeafCapacity = 0x3000 - remainder;
        }
    }
    storage->rootNode.isLeaf = true;
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName(storage, "CFStorage");
    }
    return storage;
}

// Native CFStorageCreateWithSubrange, 0x1c01fb818. External. Descends from the root while exactly one child
// intersects the byte range (offsets 0, child[0], child[0] + child[1]; a missing child[1] or child[2] counts
// zero bytes). A leaf is copied by inserting and replacing the values; an interior node is shared: its
// children are frozen and retained into the new root, then the values after and before the range are deleted.
OCFStorageRef OCFStorageCreateWithSubrange(OCFStorageRef storage, OCFRange range) {
    OCFStorageRef result = OCFStorageCreate(OCFGetAllocator(storage), storage->valueSize);
    if (range.length < 1) {
        return result;
    }
    OCFIndex byteLocation = valuesToBytes(storage, range.location);
    OCFIndex byteLength = valuesToBytes(storage, range.length);
    StorageNode *node = &((struct __OCFStorage *)storage)->rootNode;
    OCFIndex nodeOffset = 0;
    if (!node->isLeaf) {
        OCFIndex relativeLocation = byteLocation;
        do {
            StorageNode *children[3] = {
                node->info.notLeaf.child[0], node->info.notLeaf.child[1], node->info.notLeaf.child[2]
            };
            OCFIndex bytes1 = children[1] != NULL ? children[1]->numBytes : 0;
            OCFIndex bytes2 = children[2] != NULL ? children[2]->numBytes : 0;
            OCFIndex bytes0 = children[0]->numBytes;
            OCFIndex end = relativeLocation + byteLength;
            OCFIndex offsets[3] = {0, bytes0, bytes1 + bytes0};
            OCFIndex start0 = relativeLocation > 0 ? relativeLocation : 0;
            OCFIndex end0 = bytes0 < end ? bytes0 : end;
            OCFIndex start1 = bytes0 > relativeLocation ? bytes0 : relativeLocation;
            OCFIndex end1 = offsets[2] < end ? offsets[2] : end;
            OCFIndex start2 = offsets[2] > relativeLocation ? offsets[2] : relativeLocation;
            OCFIndex end2 = bytes2 + offsets[2] < end ? bytes2 + offsets[2] : end;
            bool in0 = end0 > start0;
            bool in1 = end1 > start1;
            bool in2 = end2 > start2;
            if ((OCFIndex)in1 + (OCFIndex)in0 + (OCFIndex)in2 != 1) {
                break;
            }
            OCFIndex index = in0 ? 0 : (in1 ? 1 : 2);
            node = children[index];
            relativeLocation -= offsets[index];
            nodeOffset += offsets[index];
        } while (!node->isLeaf);
    }
    if (node->isLeaf) {
        OCFStorageInsertValues(result, (OCFRange){0, range.length});
        if (node->info.leaf.memory != NULL) {
            OCFStorageReplaceValues(
                result, (OCFRange){0, range.length}, node->info.leaf.memory + (byteLocation - nodeOffset)
            );
        }
        return result;
    }
    OCFIndex numBytes = node->numBytes;
    result->rootNode.isLeaf = false;
    result->rootNode.numBytes = numBytes;
    result->rootNode.info.notLeaf.child[1] = NULL;
    result->rootNode.info.notLeaf.child[2] = NULL;
    result->rootNode.info.notLeaf.child[0] = NULL;
    for (int index = 0; index < 3; ++index) {
        StorageNode *child = node->info.notLeaf.child[index];
        if (child == NULL) {
            break;
        }
        child->isFrozen = true;
        retainNode(child);
        result->rootNode.info.notLeaf.child[index] = child;
    }
    OCFIndex nodeStart = bytesToValues(result, nodeOffset);
    OCFIndex nodeCount = bytesToValues(result, numBytes);
    OCFIndex front = range.location - nodeStart;
    OCFIndex afterRange = nodeStart - (range.location + range.length);
    OCFIndex tail = afterRange + nodeCount;
    if (tail >= 1) {
        OCFStorageDeleteValues(result, (OCFRange){-afterRange, tail});
    }
    if (front >= 1) {
        OCFStorageDeleteValues(result, (OCFRange){0, front});
    }
    return result;
}

// Native CFStorageInsertValues, 0x1c01fba90. External. Inserts at most maxLeafCapacity bytes (read once) per
// pass at the root; a sibling returned for the root moves the root's contents into a new node and makes the
// root an interior node over the two (clearing the cache when the root was a leaf).
void OCFStorageInsertValues(OCFStorageRef storage, OCFRange range) {
    OCFIndex numBytesToInsert = valuesToBytes(storage, range.length);
    OCFIndex byteNum = valuesToBytes(storage, range.location);
    OCFAllocatorRef allocator = OCFGetAllocator(storage);
    if (numBytesToInsert < 1) {
        return;
    }
    OCFIndex maxLeafCapacity = storage->maxLeafCapacity;
    do {
        OCFIndex insertThisTime = numBytesToInsert < maxLeafCapacity ? numBytesToInsert : maxLeafCapacity;
        StorageDoubleNode inserted = __CFStorageInsertUnfrozen(
            allocator, storage, &storage->rootNode, byteNum, insertThisTime, byteNum
        );
        if (inserted.sibling != NULL) {
            StorageNode *sibling = inserted.sibling;
            StorageNode *newNode = __CFStorageCreateNode(
                allocator, storage, storage->rootNode.isLeaf, storage->rootNode.numBytes
            );
            newNode->info = storage->rootNode.info;
            if (storage->rootNode.isLeaf) {
                setCache(storage, NULL, 0, 0);
                storage->rootNode.isLeaf = false;
            }
            storage->rootNode.info.notLeaf.child[0] = newNode;
            storage->rootNode.info.notLeaf.child[1] = sibling;
            storage->rootNode.info.notLeaf.child[2] = NULL;
            storage->rootNode.numBytes = sibling->numBytes + newNode->numBytes;
        }
        byteNum += insertThisTime;
        numBytesToInsert -= insertThisTime;
    } while (numBytesToInsert > 0);
}

// Native CFStorageReplaceValues, 0x1c01fbba4. External. Per run: the cached leaf when it is not frozen (its
// memory reserved first when absent) and holds the index, else __CFStorageFindByte with unfreezing, which
// updates the cache with the leaf's start and byte count; then memmove of the run's share of the values.
void OCFStorageReplaceValues(OCFStorageRef storage, OCFRange range, const void *values) {
    const uint8_t *source = values;
    while (range.length >= 1) {
        OCFRange cached;
        StorageNode *cachedNode = readCache(storage, &cached);
        uint8_t *destination = NULL;
        OCFRange valid = {0, 0};
        bool found = false;
        if (cachedNode != NULL && !cachedNode->isFrozen) {
            if (cachedNode->info.leaf.memory == NULL) {
                allocLeafNodeMemory(
                    OCFGetAllocator(storage), storage, cachedNode, cachedNode->numBytes, false
                );
            }
            if (range.location >= cached.location && range.location < cached.length + cached.location) {
                OCFIndex offset = valuesToBytes(storage, range.location - cached.location);
                if (cachedNode->info.leaf.memory != NULL) {
                    destination = cachedNode->info.leaf.memory + offset;
                    valid = cached;
                    found = true;
                }
            }
        }
        if (!found) {
            OCFRange byteRange = {0, 0};
            StorageNode *node = NULL;
            destination = __CFStorageFindByte(
                storage,
                &storage->rootNode,
                valuesToBytes(storage, range.location),
                0,
                &node,
                &byteRange,
                true
            );
            OCFIndex cacheLocation = 0;
            OCFIndex cacheLength = 0;
            if (node != NULL) {
                cacheLocation = bytesToValues(storage, byteRange.location);
                cacheLength = bytesToValues(storage, node->numBytes);
            }
            setCache(storage, node, cacheLocation, cacheLength);
            valid.location = bytesToValues(storage, byteRange.location);
            valid.length = bytesToValues(storage, byteRange.length);
        }
        OCFIndex remaining = valid.location - range.location + valid.length;
        OCFIndex count = range.length < remaining ? range.length : remaining;
        OCFIndex numBytes = valuesToBytes(storage, count);
        memmove(destination, source, (size_t)numBytes);
        source += numBytes;
        range.location += count;
        range.length -= count;
    }
}

// Native CFStorageDeleteValues, 0x1c01fbe7c. External. Clears the cache and deletes with compaction from the
// root. A NULL result clears the root (a tail call); a different node is moved into the root: a leaf's memory
// is taken over when the leaf is not frozen (the root's capacity stays 0, native behavior) or copied when it
// is, an interior node's children are retained (child[0] and child[1] without a NULL check). The returned
// node is then released.
void OCFStorageDeleteValues(OCFStorageRef storage, OCFRange range) {
    OCFAllocatorRef allocator = OCFGetAllocator(storage);
    OCFIndex byteLocation = valuesToBytes(storage, range.location);
    OCFIndex byteLength = valuesToBytes(storage, range.length);
    StorageNode *root = &storage->rootNode;
    setCache(storage, NULL, 0, 0);
    StorageNode *newRoot =
        __CFStorageDeleteUnfrozen(allocator, storage, root, byteLocation, byteLength, true);
    if (newRoot == NULL) {
        __CFStorageClearRootNode(storage);
        return;
    }
    if (newRoot != root) {
        OCFIndex numBytes = newRoot->numBytes;
        root->info.leaf.capacityInBytes = 0;
        root->numBytes = numBytes;
        root->isLeaf = newRoot->isLeaf;
        root->info.notLeaf.child[1] = NULL;
        root->info.notLeaf.child[2] = NULL;
        if (newRoot->isLeaf) {
            if (!newRoot->isFrozen) {
                root->info.leaf.memory = newRoot->info.leaf.memory;
                newRoot->info.notLeaf.child[0] = NULL;
                newRoot->info.notLeaf.child[1] = NULL;
                newRoot->info.notLeaf.child[2] = NULL;
            } else if (newRoot->info.leaf.memory != NULL) {
                allocLeafNodeMemory(allocator, storage, root, numBytes, false);
                memmove(root->info.leaf.memory, newRoot->info.leaf.memory, (size_t)newRoot->numBytes);
            }
        } else {
            root->info.notLeaf.child[0] = retainNode(newRoot->info.notLeaf.child[0]);
            root->info.notLeaf.child[1] = retainNode(newRoot->info.notLeaf.child[1]);
            if (newRoot->info.notLeaf.child[2] != NULL) {
                root->info.notLeaf.child[2] = retainNode(newRoot->info.notLeaf.child[2]);
            }
        }
    }
    releaseNode(storage, newRoot);
}

// Native CFStorageGetCount, 0x1c01fc0ec. External.
OCFIndex OCFStorageGetCount(OCFStorageRef storage) {
    return bytesToValues(storage, storage->rootNode.numBytes);
}

// Native CFStorageGetValueAtIndex, 0x1c01fc110. External. The cached leaf serves the index when it is not
// frozen (its memory reserved first when absent); the cached range is stored before its memory is tested
// again, and a leaf still without memory falls through to __CFStorageFindByte with unfreezing, which also
// updates the cache.
void *OCFStorageGetValueAtIndex(OCFStorageRef storage, OCFIndex idx, OCFRange *validConsecutiveValueRange) {
    OCFRange cached;
    StorageNode *cachedNode = readCache(storage, &cached);
    if (cachedNode != NULL && !cachedNode->isFrozen) {
        if (cachedNode->info.leaf.memory == NULL) {
            allocLeafNodeMemory(OCFGetAllocator(storage), storage, cachedNode, cachedNode->numBytes, false);
        }
        if (idx >= cached.location && idx < cached.length + cached.location) {
            if (validConsecutiveValueRange != NULL) {
                *validConsecutiveValueRange = cached;
            }
            uint8_t *memory = cachedNode->info.leaf.memory;
            OCFIndex offset = valuesToBytes(storage, idx - cached.location);
            if (memory != NULL) {
                return memory + offset;
            }
        }
    }
    OCFRange byteRange = {0, 0};
    StorageNode *node = NULL;
    void *result = __CFStorageFindByte(
        storage, &storage->rootNode, valuesToBytes(storage, idx), 0, &node, &byteRange, true
    );
    OCFIndex cacheLocation = 0;
    OCFIndex cacheLength = 0;
    if (node != NULL) {
        cacheLocation = bytesToValues(storage, byteRange.location);
        cacheLength = bytesToValues(storage, node->numBytes);
    }
    setCache(storage, node, cacheLocation, cacheLength);
    OCFIndex location = bytesToValues(storage, byteRange.location);
    OCFIndex length = bytesToValues(storage, byteRange.length);
    if (validConsecutiveValueRange != NULL) {
        validConsecutiveValueRange->location = location;
        validConsecutiveValueRange->length = length;
    }
    return result;
}

// Native CFStorageGetConstValueAtIndex, 0x1c01fc3a0. External. CFStorageGetValueAtIndex without the frozen
// test on the cached leaf and without unfreezing in __CFStorageFindByte.
const void *
OCFStorageGetConstValueAtIndex(OCFStorageRef storage, OCFIndex idx, OCFRange *validConsecutiveValueRange) {
    OCFRange cached;
    StorageNode *cachedNode = readCache(storage, &cached);
    if (cachedNode != NULL) {
        if (cachedNode->info.leaf.memory == NULL) {
            allocLeafNodeMemory(OCFGetAllocator(storage), storage, cachedNode, cachedNode->numBytes, false);
        }
        if (idx >= cached.location && idx < cached.length + cached.location) {
            if (validConsecutiveValueRange != NULL) {
                *validConsecutiveValueRange = cached;
            }
            uint8_t *memory = cachedNode->info.leaf.memory;
            OCFIndex offset = valuesToBytes(storage, idx - cached.location);
            if (memory != NULL) {
                return memory + offset;
            }
        }
    }
    OCFRange byteRange = {0, 0};
    StorageNode *node = NULL;
    const void *result = __CFStorageFindByte(
        storage, &storage->rootNode, valuesToBytes(storage, idx), 0, &node, &byteRange, false
    );
    OCFIndex cacheLocation = 0;
    OCFIndex cacheLength = 0;
    if (node != NULL) {
        cacheLocation = bytesToValues(storage, byteRange.location);
        cacheLength = bytesToValues(storage, node->numBytes);
    }
    setCache(storage, node, cacheLocation, cacheLength);
    OCFIndex location = bytesToValues(storage, byteRange.location);
    OCFIndex length = bytesToValues(storage, byteRange.length);
    if (validConsecutiveValueRange != NULL) {
        validConsecutiveValueRange->location = location;
        validConsecutiveValueRange->length = length;
    }
    return result;
}

// Native __CFStorageInsertUnfrozen, 0x1c01fc620. Non-external; returns the pair in X0 and X1. A leaf that
// fits grows in place. An overflowing leaf: appending creates a sibling of size bytes; inserting at 0 moves
// the leaf's bytes into a new sibling and keeps size bytes in place; otherwise the bytes after byteNum move
// to a new sibling, which takes the overflow beyond maxLeafCapacity when byteNum + size exceeds it (the leaf
// then keeps maxLeafCapacity bytes; the cache records its old byte count) or else the bytes from byteNum (the
// leaf keeps byteNum + size). An interior node inserts into the child that holds byteNum (an exact boundary
// goes left), thawing it or inserting frozen, places a returned sibling after it and splits into a new
// sibling when full. The cache is set to the leaf that receives the inserted bytes.
static StorageDoubleNode __CFStorageInsertUnfrozen(
    OCFAllocatorRef allocator,
    OCFStorageRef storage,
    StorageNode *node,
    OCFIndex byteNum,
    OCFIndex size,
    OCFIndex absoluteByteNum
) {
    if (node->isLeaf) {
        OCFIndex numBytes = node->numBytes;
        OCFIndex maxLeafCapacity = storage->maxLeafCapacity;
        OCFIndex newNumBytes = numBytes + size;
        if (newNumBytes <= maxLeafCapacity) {
            if (node->info.leaf.memory != NULL) {
                allocLeafNodeMemory(allocator, storage, node, newNumBytes, false);
                uint8_t *memory = node->info.leaf.memory;
                memmove(memory + byteNum + size, memory + byteNum, (size_t)(node->numBytes - byteNum));
                newNumBytes = node->numBytes + size;
            }
            node->numBytes = newNumBytes;
            setCache(
                storage,
                node,
                bytesToValues(storage, absoluteByteNum - byteNum),
                bytesToValues(storage, newNumBytes)
            );
            return (StorageDoubleNode){node, NULL};
        }
        if (byteNum == numBytes) {
            StorageNode *newNode = __CFStorageCreateNode(allocator, storage, true, size);
            OCFIndex cacheLocation = 0;
            OCFIndex cacheLength = 0;
            if (newNode != NULL) {
                cacheLocation = bytesToValues(storage, absoluteByteNum);
                cacheLength = bytesToValues(storage, newNode->numBytes);
            }
            setCache(storage, newNode, cacheLocation, cacheLength);
            return (StorageDoubleNode){node, newNode};
        }
        if (byteNum == 0) {
            StorageNode *newNode = __CFStorageCreateNode(allocator, storage, true, 0);
            newNode->numBytes = node->numBytes;
            newNode->info.leaf.capacityInBytes = node->info.leaf.capacityInBytes;
            newNode->info.leaf.memory = node->info.leaf.memory;
            node->numBytes = size;
            node->info.leaf.capacityInBytes = 0;
            node->info.leaf.memory = NULL;
            setCache(storage, node, bytesToValues(storage, absoluteByteNum), bytesToValues(storage, size));
            return (StorageDoubleNode){node, newNode};
        }
        OCFIndex insertionEnd = size + byteNum;
        if (insertionEnd > maxLeafCapacity) {
            StorageNode *newNode =
                __CFStorageCreateNode(allocator, storage, true, newNumBytes - maxLeafCapacity);
            if (node->info.leaf.memory != NULL) {
                allocLeafNodeMemory(
                    allocator, storage, newNode, node->numBytes + size - storage->maxLeafCapacity, false
                );
                memmove(
                    newNode->info.leaf.memory + (byteNum + size) - storage->maxLeafCapacity,
                    node->info.leaf.memory + byteNum,
                    (size_t)(node->numBytes - byteNum)
                );
                allocLeafNodeMemory(allocator, storage, node, storage->maxLeafCapacity, false);
            }
            setCache(
                storage,
                node,
                bytesToValues(storage, absoluteByteNum - byteNum),
                bytesToValues(storage, node->numBytes)
            );
            node->numBytes = storage->maxLeafCapacity;
            return (StorageDoubleNode){node, newNode};
        }
        StorageNode *newNode = __CFStorageCreateNode(allocator, storage, true, numBytes - byteNum);
        if (node->info.leaf.memory != NULL) {
            allocLeafNodeMemory(allocator, storage, newNode, node->numBytes - byteNum, false);
            memmove(
                newNode->info.leaf.memory,
                node->info.leaf.memory + byteNum,
                (size_t)(node->numBytes - byteNum)
            );
            allocLeafNodeMemory(allocator, storage, node, insertionEnd, false);
        }
        node->numBytes = insertionEnd;
        setCache(
            storage,
            node,
            bytesToValues(storage, absoluteByteNum - byteNum),
            bytesToValues(storage, insertionEnd)
        );
        return (StorageDoubleNode){node, newNode};
    }
    StorageNode **children = node->info.notLeaf.child;
    OCFIndex firstBytes = children[0]->numBytes;
    OCFIndex childIndex;
    OCFIndex relativeByteNum = byteNum - 1;
    if (byteNum <= firstBytes) {
        childIndex = 0;
    } else {
        relativeByteNum -= firstBytes;
        OCFIndex beyondSecond = relativeByteNum - children[1]->numBytes;
        if (beyondSecond >= 0) {
            childIndex = 2;
            relativeByteNum = beyondSecond;
        } else {
            childIndex = 1;
        }
    }
    StorageNode *child = children[childIndex];
    StorageDoubleNode inserted;
    if (thawNodeDuringMutation(child)) {
        inserted =
            __CFStorageInsertUnfrozen(allocator, storage, child, relativeByteNum + 1, size, absoluteByteNum);
    } else {
        inserted =
            __CFStorageInsertFrozen(allocator, storage, child, relativeByteNum + 1, size, absoluteByteNum);
    }
    if (child != inserted.child) {
        releaseNode(storage, child);
        children[childIndex] = inserted.child;
    }
    if (inserted.sibling == NULL) {
        node->numBytes += size;
        return (StorageDoubleNode){node, NULL};
    }
    if (children[2] == NULL) {
        if (byteNum <= firstBytes) {
            children[2] = children[1];
        }
        children[childIndex + 1] = inserted.sibling;
        node->numBytes += size;
        return (StorageDoubleNode){node, NULL};
    }
    StorageNode *newNode = __CFStorageCreateNode(allocator, storage, false, 0);
    if (childIndex == 1) {
        newNode->info.notLeaf.child[0] = inserted.sibling;
        newNode->info.notLeaf.child[1] = children[2];
    } else if (childIndex == 0) {
        newNode->info.notLeaf.child[0] = children[1];
        newNode->info.notLeaf.child[1] = children[2];
        children[1] = inserted.sibling;
    } else {
        newNode->info.notLeaf.child[0] = children[2];
        newNode->info.notLeaf.child[1] = inserted.sibling;
    }
    children[2] = NULL;
    node->numBytes = children[1]->numBytes + children[0]->numBytes;
    newNode->numBytes = newNode->info.notLeaf.child[1]->numBytes + newNode->info.notLeaf.child[0]->numBytes;
    return (StorageDoubleNode){node, newNode};
}

// Native __CFStorageCreateNode, 0x1c01fce50. Non-external. A 40-byte typed allocation (descriptor
// 0x01032040801afe6d) with reference count 1, the storage's always-frozen flag, the leaf flag and the byte
// count; child[2] is cleared only for an interior node, the first two words always.
static StorageNode *
__CFStorageCreateNode(OCFAllocatorRef allocator, OCFStorageRef storage, bool isLeaf, OCFIndex numBytes) {
    StorageNode *newNode =
        OCFAllocatorAllocateTyped(allocator, sizeof(StorageNode), 0x01032040801afe6dULL, 0);
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName(newNode, "CFStorage (node)");
    }
    newNode->refCount = 1;
    newNode->isFrozen = storage->alwaysFrozen;
    newNode->isLeaf = isLeaf;
    newNode->numBytes = numBytes;
    if (!isLeaf) {
        newNode->info.notLeaf.child[2] = NULL;
    }
    newNode->info.notLeaf.child[0] = NULL;
    newNode->info.notLeaf.child[1] = NULL;
    return newNode;
}

// Native __CFStorageDeleteUnfrozen, 0x1c01fcee8. Non-external. Deleting every byte returns NULL. A leaf
// shifts its bytes down and, when compacting and it has memory, resizes its memory to the rounded byte count
// (any change). An interior node collects, in order, each child untouched by the range (retained) or the
// result of deleting from it (thawed in place when possible, else deleted frozen; NULL results are dropped),
// releases its old children (child[0] without a NULL check) and returns the only collected child as
// collected, or takes the collected children. A node that remains is returned with a non-atomic increment of
// a nonzero count (LDR/ADD/STR).
static StorageNode *__CFStorageDeleteUnfrozen(
    OCFAllocatorRef allocator,
    OCFStorageRef storage,
    StorageNode *node,
    OCFIndex location,
    OCFIndex length,
    bool compact
) {
    OCFIndex newNumBytes = node->numBytes - length;
    if (newNumBytes == 0) {
        return NULL;
    }
    if (node->isLeaf) {
        node->numBytes = newNumBytes;
        uint8_t *memory = node->info.leaf.memory;
        if (memory != NULL) {
            memmove(memory + location, memory + location + length, (size_t)(newNumBytes - location));
            if (compact) {
                allocLeafNodeMemory(allocator, storage, node, node->numBytes, true);
            }
        }
    } else {
        StorageNode *newChildren[3] = {NULL, NULL, NULL};
        OCFIndex newChildCount = 0;
        OCFIndex childOffset = 0;
        OCFIndex rangeEnd = location + length;
        for (int index = 0; index < 3; ++index) {
            StorageNode *child = node->info.notLeaf.child[index];
            if (child == NULL) {
                break;
            }
            OCFIndex start = location > childOffset ? location : childOffset;
            OCFIndex childEnd = child->numBytes + childOffset;
            OCFIndex end = rangeEnd < childEnd ? rangeEnd : childEnd;
            OCFIndex overlap = end - start;
            if (overlap > 0) {
                StorageNode *newChild;
                if (thawNodeDuringMutation(child)) {
                    newChild = __CFStorageDeleteUnfrozen(
                        allocator, storage, child, start - childOffset, overlap, compact
                    );
                } else {
                    newChild =
                        __CFStorageDeleteFrozen(allocator, storage, child, start - childOffset, overlap);
                }
                if (newChild != NULL) {
                    newChildren[newChildCount++] = newChild;
                }
            } else {
                newChildren[newChildCount++] = retainNode(child);
            }
            childOffset = childEnd;
        }
        node->numBytes -= length;
        releaseNode(storage, node->info.notLeaf.child[0]);
        releaseNodeWithNullCheck(storage, node->info.notLeaf.child[1]);
        releaseNodeWithNullCheck(storage, node->info.notLeaf.child[2]);
        node->info.notLeaf.child[0] = NULL;
        node->info.notLeaf.child[1] = NULL;
        node->info.notLeaf.child[2] = NULL;
        if (newChildCount == 1) {
            return newChildren[0];
        }
        node->info.notLeaf.child[0] = newChildren[0];
        node->info.notLeaf.child[1] = newChildren[1];
        node->info.notLeaf.child[2] = newChildren[2];
    }
    if (node->refCount != 0) {
        node->refCount++;
    }
    return node;
}

// Native __CFStorageClearRootNode, 0x1c01fd200. Non-external. A leaf root's memory is deallocated (NULL
// passed through), an interior root's children are released with NULL checks; the root becomes an empty leaf
// with no capacity or memory (child[2]'s word is left as it was).
static void __CFStorageClearRootNode(OCFStorageRef storage) {
    OCFAllocatorRef allocator = OCFGetAllocator(storage);
    StorageNode *root = &storage->rootNode;
    if (root->isLeaf) {
        OCFAllocatorDeallocate(allocator, root->info.leaf.memory);
    } else {
        releaseNodeWithNullCheck(storage, root->info.notLeaf.child[0]);
        releaseNodeWithNullCheck(storage, root->info.notLeaf.child[1]);
        releaseNodeWithNullCheck(storage, root->info.notLeaf.child[2]);
    }
    root->isLeaf = true;
    root->numBytes = 0;
    root->info.leaf.capacityInBytes = 0;
    root->info.leaf.memory = NULL;
}

// Native CFStorageGetValues, 0x1c01fd2cc. External. CFStorageReplaceValues' run loop in the other direction:
// the cached leaf is used without a frozen test and __CFStorageFindByte does not unfreeze.
void OCFStorageGetValues(OCFStorageRef storage, OCFRange range, void *values) {
    uint8_t *destination = values;
    while (range.length >= 1) {
        OCFRange cached;
        StorageNode *cachedNode = readCache(storage, &cached);
        const uint8_t *source = NULL;
        OCFRange valid = {0, 0};
        bool found = false;
        if (cachedNode != NULL) {
            if (cachedNode->info.leaf.memory == NULL) {
                allocLeafNodeMemory(
                    OCFGetAllocator(storage), storage, cachedNode, cachedNode->numBytes, false
                );
            }
            if (range.location >= cached.location && range.location < cached.length + cached.location) {
                OCFIndex offset = valuesToBytes(storage, range.location - cached.location);
                if (cachedNode->info.leaf.memory != NULL) {
                    source = cachedNode->info.leaf.memory + offset;
                    valid = cached;
                    found = true;
                }
            }
        }
        if (!found) {
            OCFRange byteRange = {0, 0};
            StorageNode *node = NULL;
            source = __CFStorageFindByte(
                storage,
                &storage->rootNode,
                valuesToBytes(storage, range.location),
                0,
                &node,
                &byteRange,
                false
            );
            OCFIndex cacheLocation = 0;
            OCFIndex cacheLength = 0;
            if (node != NULL) {
                cacheLocation = bytesToValues(storage, byteRange.location);
                cacheLength = bytesToValues(storage, node->numBytes);
            }
            setCache(storage, node, cacheLocation, cacheLength);
            valid.location = bytesToValues(storage, byteRange.location);
            valid.length = bytesToValues(storage, byteRange.length);
        }
        OCFIndex remaining = valid.location - range.location + valid.length;
        OCFIndex count = range.length < remaining ? range.length : remaining;
        OCFIndex numBytes = valuesToBytes(storage, count);
        memmove(destination, source, (size_t)numBytes);
        destination += numBytes;
        range.location += count;
        range.length -= count;
    }
}

// Native __CFStorageDescribeNode, 0x1c01fdb8c. Non-external. Two spaces per level (the system encoding
// fetched for each), then the leaf or node line, then each present child at level + 1 (a missing child is
// skipped, not a stop).
static void __CFStorageDescribeNode(StorageNode *node, OCFMutableStringRef str, OCFIndex level) {
    for (OCFIndex count = 0; count < level; ++count) {
        OCFStringAppendCString(str, "  ", OCFStringGetSystemEncoding());
    }
    if (node->isLeaf) {
        OCFStringAppendFormat(
            str,
            NULL,
            OF_STR("Leaf %ld/%ld (%p) refcount: %u frozen: %s\n"),
            node->numBytes,
            node->info.leaf.capacityInBytes,
            node,
            node->refCount,
            node->isFrozen ? "yes" : "no"
        );
        return;
    }
    OCFStringAppendFormat(
        str,
        NULL,
        OF_STR("Node %ld (%p) refcount: %u frozen: %s\n"),
        node->numBytes,
        node,
        node->refCount,
        node->isFrozen ? "yes" : "no"
    );
    for (int index = 0; index < 3; ++index) {
        StorageNode *child = node->info.notLeaf.child[index];
        if (child != NULL) {
            __CFStorageDescribeNode(child, str, level + 1);
        }
    }
}

// Native __CFStorageFindByte, 0x1c01fdcb4. Non-external; the recursion over interior nodes is a loop. The
// child holding byteNum (strictly before a boundary) is chosen without NULL checks on child[1]; when
// unfreezing, a frozen child is thawed in place when this storage holds its only reference, or else replaced
// by a copy (a leaf copy gets the child's bytes when the child has memory; an interior copy retains the
// children, child[0] without a NULL check, and freezes them when the child is frozen) and the child is
// released. At the leaf the byte range and node are stored before the leaf's memory is reserved; the result
// points at byteNum in that memory.
static void *__CFStorageFindByte(
    OCFStorageRef storage,
    StorageNode *node,
    OCFIndex byteNum,
    OCFIndex absoluteByteOffsetOfNode,
    StorageNode **resultNode,
    OCFRange *validConsecutiveByteRange,
    bool requireUnfreezing
) {
    while (!node->isLeaf) {
        StorageNode **children = node->info.notLeaf.child;
        OCFIndex childIndex;
        OCFIndex relativeByteNum = byteNum - children[0]->numBytes;
        if (relativeByteNum < 0) {
            childIndex = 0;
            relativeByteNum = byteNum;
        } else {
            OCFIndex beyondSecond = relativeByteNum - children[1]->numBytes;
            if (beyondSecond < 0) {
                childIndex = 1;
            } else {
                childIndex = 2;
                relativeByteNum = beyondSecond;
            }
        }
        StorageNode *child = children[childIndex];
        if (requireUnfreezing && child->isFrozen) {
            if (child->refCount == 1) {
                child->isFrozen = false;
            } else {
                OCFAllocatorRef allocator = OCFGetAllocator(storage);
                StorageNode *copy = __CFStorageCreateNode(allocator, storage, child->isLeaf, child->numBytes);
                if (child->isLeaf) {
                    if (child->info.leaf.memory != NULL) {
                        allocLeafNodeMemory(allocator, storage, copy, copy->numBytes, false);
                        memmove(copy->info.leaf.memory, child->info.leaf.memory, (size_t)copy->numBytes);
                    }
                } else {
                    copy->info.notLeaf.child[0] = retainNode(child->info.notLeaf.child[0]);
                    if (child->info.notLeaf.child[1] != NULL) {
                        copy->info.notLeaf.child[1] = retainNode(child->info.notLeaf.child[1]);
                    }
                    if (child->info.notLeaf.child[2] != NULL) {
                        copy->info.notLeaf.child[2] = retainNode(child->info.notLeaf.child[2]);
                    }
                    if (child->isFrozen) {
                        copy->info.notLeaf.child[0]->isFrozen = true;
                        if (copy->info.notLeaf.child[1] != NULL) {
                            copy->info.notLeaf.child[1]->isFrozen = true;
                        }
                        if (copy->info.notLeaf.child[2] != NULL) {
                            copy->info.notLeaf.child[2]->isFrozen = true;
                        }
                    }
                }
                children[childIndex] = copy;
                releaseNode(storage, child);
                child = copy;
            }
        }
        absoluteByteOffsetOfNode = absoluteByteOffsetOfNode + byteNum - relativeByteNum;
        byteNum = relativeByteNum;
        node = child;
    }
    validConsecutiveByteRange->location = absoluteByteOffsetOfNode;
    validConsecutiveByteRange->length = node->numBytes;
    *resultNode = node;
    allocLeafNodeMemory(OCFGetAllocator(storage), storage, node, node->numBytes, false);
    return node->info.leaf.memory + byteNum;
}

// Native __CFStorageDeallocateNode, 0x1c01fe048. Non-external. A leaf's memory (when present) or the children
// (released with NULL checks), then the node, through the storage's allocator.
static void __CFStorageDeallocateNode(OCFStorageRef storage, StorageNode *node) {
    OCFAllocatorRef allocator = OCFGetAllocator(storage);
    if (node->isLeaf) {
        if (node->info.leaf.memory != NULL) {
            OCFAllocatorDeallocate(allocator, node->info.leaf.memory);
        }
    } else {
        releaseNodeWithNullCheck(storage, node->info.notLeaf.child[0]);
        releaseNodeWithNullCheck(storage, node->info.notLeaf.child[1]);
        releaseNodeWithNullCheck(storage, node->info.notLeaf.child[2]);
    }
    OCFAllocatorDeallocate(allocator, node);
}

// Native __CFStorageInsertFrozen, 0x1c01fe124. Non-external; returns the pair in X0 and X1. A frozen node is
// never changed. A leaf that fits is copied into a new leaf with the gap; an overflowing leaf: appending
// returns the node unretained with a new sibling of size bytes, inserting at 0 returns a new leaf before the
// retained node, and otherwise the bytes are split into a new leaf of maxLeafCapacity bytes and a new leaf of
// the overflow (the gap's bytes are left uninitialized). A NULL new leaf where the native code tests for one
// returns with a cleared cache. An interior node is copied: its children are retained and frozen into a
// four-slot array, the child holding byteNum is replaced by the recursive result and a returned sibling is
// placed after it, and four children split into two new interior nodes. CoreFoundation defect (fixed): the
// middle split reads the frozen leaf's memory without a NULL test (the fitting path tests it), so a frozen
// leaf whose memory was never reserved faults in the first memmove, which reads byteNum (at least 1) bytes
// from NULL; the two later copies read from NULL plus an offset as well. Reachable through the public API
// (CFStorageCreateWithSubrange shares and freezes never-accessed leaves; a later overflowing insertion into
// the middle of one crashes). Confidence: verified, IDA (corpus delta capture): 0x1c01fe718-0x1c01fe724,
// 0x1c01fe764-0x1c01fe774 and 0x1c01fe7ac-0x1c01fe7bc load the leaf's memory (offset 24) and call memmove
// with no test between. Divergence: the three copies are skipped when the leaf's memory is NULL, leaving the
// new leaves' bytes uninitialized, as unreserved memory already is.
static StorageDoubleNode __CFStorageInsertFrozen(
    OCFAllocatorRef allocator,
    OCFStorageRef storage,
    StorageNode *node,
    OCFIndex byteNum,
    OCFIndex size,
    OCFIndex absoluteByteNum
) {
    if (node->isLeaf) {
        OCFIndex numBytes = node->numBytes;
        OCFIndex maxLeafCapacity = storage->maxLeafCapacity;
        OCFIndex newNumBytes = numBytes + size;
        OCFIndex overflow = newNumBytes - maxLeafCapacity;
        if (newNumBytes <= maxLeafCapacity) {
            StorageNode *newNode = __CFStorageCreateNode(allocator, storage, true, newNumBytes);
            if (node->info.leaf.memory != NULL) {
                allocLeafNodeMemory(allocator, storage, newNode, newNumBytes, false);
                memmove(newNode->info.leaf.memory, node->info.leaf.memory, (size_t)byteNum);
                memmove(
                    newNode->info.leaf.memory + (byteNum + size),
                    node->info.leaf.memory + byteNum,
                    (size_t)(node->numBytes - byteNum)
                );
            } else if (newNode == NULL) {
                setCache(storage, NULL, 0, 0);
                return (StorageDoubleNode){NULL, NULL};
            }
            setCache(
                storage,
                newNode,
                bytesToValues(storage, absoluteByteNum - byteNum),
                bytesToValues(storage, newNode->numBytes)
            );
            return (StorageDoubleNode){newNode, NULL};
        }
        if (byteNum == numBytes) {
            StorageNode *newNode = __CFStorageCreateNode(allocator, storage, true, size);
            OCFIndex cacheLocation = 0;
            OCFIndex cacheLength = 0;
            if (newNode != NULL) {
                cacheLocation = bytesToValues(storage, absoluteByteNum);
                cacheLength = bytesToValues(storage, newNode->numBytes);
            }
            setCache(storage, newNode, cacheLocation, cacheLength);
            return (StorageDoubleNode){node, newNode};
        }
        if (byteNum == 0) {
            retainNode(node);
            StorageNode *newNode = __CFStorageCreateNode(allocator, storage, true, size);
            if (newNode == NULL) {
                setCache(storage, NULL, 0, 0);
                return (StorageDoubleNode){NULL, node};
            }
            setCache(
                storage,
                newNode,
                bytesToValues(storage, absoluteByteNum),
                bytesToValues(storage, newNode->numBytes)
            );
            return (StorageDoubleNode){newNode, node};
        }
        StorageNode *leftNode = __CFStorageCreateNode(allocator, storage, true, maxLeafCapacity);
        StorageNode *rightNode = __CFStorageCreateNode(allocator, storage, true, overflow);
        allocLeafNodeMemory(allocator, storage, leftNode, maxLeafCapacity, false);
        allocLeafNodeMemory(allocator, storage, rightNode, overflow, false);
        // CoreFoundation defect: the native code copies from a NULL leaf memory here; the copies are skipped
        // instead, as the fitting path above does natively.
        if (node->info.leaf.memory != NULL) {
            memmove(leftNode->info.leaf.memory, node->info.leaf.memory, (size_t)byteNum);
            OCFIndex gapEnd = size + byteNum;
            OCFIndex shiftedEnd = node->numBytes + size;
            OCFIndex leftStart = gapEnd > 0 ? gapEnd : 0;
            OCFIndex leftEnd = maxLeafCapacity < shiftedEnd ? maxLeafCapacity : shiftedEnd;
            OCFIndex leftLength = leftEnd > leftStart ? leftEnd - leftStart : 0;
            if (leftLength >= 1) {
                OCFIndex start = leftEnd > leftStart ? leftStart : 0;
                memmove(
                    leftNode->info.leaf.memory + start,
                    node->info.leaf.memory + start - size,
                    (size_t)leftLength
                );
            }
            OCFIndex rightStart = maxLeafCapacity > gapEnd ? maxLeafCapacity : gapEnd;
            OCFIndex rightEnd = newNumBytes < shiftedEnd ? newNumBytes : shiftedEnd;
            OCFIndex rightLength = rightEnd > rightStart ? rightEnd - rightStart : 0;
            if (rightLength >= 1) {
                OCFIndex start = rightEnd > rightStart ? rightStart : 0;
                memmove(
                    rightNode->info.leaf.memory + start - maxLeafCapacity,
                    node->info.leaf.memory + start - size,
                    (size_t)rightLength
                );
            }
        }
        setCache(
            storage,
            leftNode,
            bytesToValues(storage, absoluteByteNum - byteNum),
            bytesToValues(storage, leftNode->numBytes)
        );
        return (StorageDoubleNode){leftNode, rightNode};
    }
    StorageNode *newNode = __CFStorageCreateNode(allocator, storage, false, 0);
    StorageNode **children = node->info.notLeaf.child;
    OCFIndex firstBytes = children[0]->numBytes;
    OCFIndex childIndex;
    OCFIndex relativeByteNum = byteNum - 1;
    if (byteNum <= firstBytes) {
        childIndex = 0;
    } else {
        relativeByteNum -= firstBytes;
        OCFIndex beyondSecond = relativeByteNum - children[1]->numBytes;
        if (beyondSecond >= 0) {
            childIndex = 2;
            relativeByteNum = beyondSecond;
        } else {
            childIndex = 1;
        }
    }
    StorageDoubleNode inserted = __CFStorageInsertFrozen(
        allocator, storage, children[childIndex], relativeByteNum + 1, size, absoluteByteNum
    );
    StorageNode *newChildren[4] = {NULL, NULL, NULL, NULL};
    for (int index = 0; index < 3; ++index) {
        StorageNode *child = children[index];
        if (child != NULL) {
            retainNode(child);
            child->isFrozen = true;
        }
        newChildren[index] = child;
    }
    if (newChildren[childIndex] != inserted.child) {
        releaseNode(storage, newChildren[childIndex]);
        newChildren[childIndex] = inserted.child;
    }
    if (inserted.sibling != NULL) {
        if (childIndex != 2) {
            newChildren[3] = newChildren[2];
            if (byteNum <= firstBytes) {
                newChildren[2] = newChildren[1];
            }
        }
        newChildren[childIndex + 1] = inserted.sibling;
    }
    newNode->info.notLeaf.child[0] = newChildren[0];
    newNode->info.notLeaf.child[1] = newChildren[1];
    if (newChildren[3] != NULL) {
        StorageNode *siblingNode = __CFStorageCreateNode(allocator, storage, false, 0);
        siblingNode->info.notLeaf.child[0] = newChildren[2];
        siblingNode->info.notLeaf.child[1] = newChildren[3];
        newNode->numBytes = newChildren[1]->numBytes + newChildren[0]->numBytes;
        siblingNode->numBytes = newChildren[3]->numBytes + newChildren[2]->numBytes;
        return (StorageDoubleNode){newNode, siblingNode};
    }
    newNode->info.notLeaf.child[2] = newChildren[2];
    newNode->numBytes = node->numBytes + size;
    return (StorageDoubleNode){newNode, NULL};
}

// Native __CFStorageDeleteFrozen, 0x1c01fe890. Non-external. Deleting every byte returns NULL. A leaf is
// copied into a new leaf without the range (only when the frozen leaf has memory). An interior node collects
// each child untouched by the range (retained and frozen) or the result of deleting from it frozen (NULL
// results dropped); the only collected child is returned as is, else a new interior node takes the collected
// children (memcpy).
static StorageNode *__CFStorageDeleteFrozen(
    OCFAllocatorRef allocator, OCFStorageRef storage, StorageNode *node, OCFIndex location, OCFIndex length
) {
    if (node->isLeaf) {
        OCFIndex newNumBytes = node->numBytes - length;
        if (newNumBytes == 0) {
            return NULL;
        }
        StorageNode *newNode = __CFStorageCreateNode(allocator, storage, true, newNumBytes);
        if (node->info.leaf.memory == NULL) {
            return newNode;
        }
        allocLeafNodeMemory(allocator, storage, newNode, newNumBytes, false);
        OCFIndex rangeEnd = location + length;
        OCFIndex tail = node->numBytes - rangeEnd;
        if (location >= 1) {
            memmove(newNode->info.leaf.memory, node->info.leaf.memory, (size_t)location);
        }
        if (tail >= 1) {
            memmove(newNode->info.leaf.memory + location, node->info.leaf.memory + rangeEnd, (size_t)tail);
        }
        return newNode;
    }
    if (length == node->numBytes) {
        return NULL;
    }
    StorageNode *newChildren[3] = {NULL, NULL, NULL};
    OCFIndex newChildCount = 0;
    OCFIndex childOffset = 0;
    OCFIndex rangeEnd = location + length;
    for (int index = 0; index < 3; ++index) {
        StorageNode *child = node->info.notLeaf.child[index];
        if (child == NULL) {
            break;
        }
        OCFIndex start = location > childOffset ? location : childOffset;
        OCFIndex childEnd = child->numBytes + childOffset;
        OCFIndex end = rangeEnd < childEnd ? rangeEnd : childEnd;
        OCFIndex overlap = end - start;
        if (overlap > 0) {
            StorageNode *newChild =
                __CFStorageDeleteFrozen(allocator, storage, child, start - childOffset, overlap);
            if (newChild != NULL) {
                newChildren[newChildCount++] = newChild;
            }
        } else {
            retainNode(child);
            newChildren[newChildCount++] = child;
            child->isFrozen = true;
        }
        childOffset = childEnd;
    }
    if (newChildCount == 1) {
        return newChildren[0];
    }
    StorageNode *newNode = __CFStorageCreateNode(allocator, storage, false, 0);
    if (newChildCount != 0) {
        memcpy(newNode->info.notLeaf.child, newChildren, (size_t)newChildCount * sizeof(StorageNode *));
    }
    newNode->numBytes = node->numBytes - length;
    return newNode;
}
