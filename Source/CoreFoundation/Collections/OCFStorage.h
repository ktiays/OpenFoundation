//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"

#include <os/lock.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

// CFStorage is a private CoreFoundation interface (no SDK header); the signatures below are read from the
// native functions' register use, iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Ranges are passed by value in two registers.
typedef struct __OCFStorage *OCFStorageRef;

// A CFStorage node, iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for
// every offset (the accesses of the functions in Collections/OCFStorage.c): the byte count (offset 0), a
// 32-bit reference count changed with LDADD (8; zero marks the storage's inline root node, which is never
// retained or released), the frozen flag (byte 12), the leaf flag (byte 13), then a leaf's capacity and
// memory (16, 24) or an interior node's three children (16, 24, 32). The nodes are 40-byte allocations.
// StorageNode and its field names are local descriptive spellings.
typedef struct StorageNode {
    OCFIndex numBytes;
    uint32_t refCount;
    bool isFrozen;
    bool isLeaf;

    union {
        struct {
            OCFIndex capacityInBytes;
            uint8_t *memory;
        } leaf;

        struct {
            struct StorageNode *child[3];
        } notLeaf;
    } info;
} StorageNode;

OF_ABI_ASSERT(sizeof(StorageNode) == 40, "A native CFStorage node is 40 bytes.");
OF_ABI_ASSERT(__builtin_offsetof(StorageNode, isLeaf) == 13, "The leaf flag is byte 13.");
OF_ABI_ASSERT(__builtin_offsetof(StorageNode, info.leaf.memory) == 24, "Leaf memory at 24.");

// The CFStorage object (168 bytes after the header, _CFRuntimeCreateInstance with 0xa8): the value size
// (offset 16), the byte-to-value shift or -1 when the value size is not a power of two (24, 32 bits), the
// pthread mutex that serializes leaf-memory reallocation (32, initialized with the error-checking signature
// 0x32AAABA1), the always-frozen flag (byte 96), the cache of the last leaf found and its value range (104,
// 112, 120) under an os_unfair_lock (128), the maximum leaf capacity in bytes (136) and the inline root node
// (144). The field names are local descriptive spellings.
struct __OCFStorage {
    OCFRuntimeBase base;
    OCFIndex valueSize;
    uint32_t byteToValueShifter;
    pthread_mutex_t cacheReaderMemoryAllocationLock;
    bool alwaysFrozen;
    StorageNode *cacheNode;
    OCFRange cacheRange;
    os_unfair_lock cacheLock;
    OCFIndex maxLeafCapacity;
    StorageNode rootNode;
};

OF_ABI_ASSERT(sizeof(struct __OCFStorage) == 16 + 168, "The native CFStorage object is 184 bytes.");
OF_ABI_ASSERT(__builtin_offsetof(struct __OCFStorage, alwaysFrozen) == 0x60, "alwaysFrozen at 0x60.");
OF_ABI_ASSERT(__builtin_offsetof(struct __OCFStorage, cacheNode) == 0x68, "The cached node at 0x68.");
OF_ABI_ASSERT(__builtin_offsetof(struct __OCFStorage, cacheLock) == 0x80, "The cache lock at 0x80.");
OF_ABI_ASSERT(__builtin_offsetof(struct __OCFStorage, maxLeafCapacity) == 0x88, "The leaf capacity at 0x88.");
OF_ABI_ASSERT(__builtin_offsetof(struct __OCFStorage, rootNode) == 0x90, "The root node at 0x90.");

OF_EXPORT OCFStorageRef OCFStorageCreate(OCFAllocatorRef alloc, OCFIndex valueSizeInBytes);
OF_EXPORT OCFStorageRef OCFStorageCreateWithSubrange(OCFStorageRef storage, OCFRange range);
OF_EXPORT void OCFStorageInsertValues(OCFStorageRef storage, OCFRange range);
OF_EXPORT void OCFStorageReplaceValues(OCFStorageRef storage, OCFRange range, const void *values);
OF_EXPORT void OCFStorageDeleteValues(OCFStorageRef storage, OCFRange range);
OF_EXPORT OCFIndex OCFStorageGetCount(OCFStorageRef storage);
OF_EXPORT void *
OCFStorageGetValueAtIndex(OCFStorageRef storage, OCFIndex idx, OCFRange *validConsecutiveValueRange);
OF_EXPORT const void *
OCFStorageGetConstValueAtIndex(OCFStorageRef storage, OCFIndex idx, OCFRange *validConsecutiveValueRange);
OF_EXPORT void OCFStorageGetValues(OCFStorageRef storage, OCFRange range, void *values);

// The builtin class record of type 26.
OF_EXTERN const OCFRuntimeClass __CFStorageClass;
