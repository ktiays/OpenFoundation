//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <stdint.h>

// The CFRunArray object (type 63, 8 extra bytes), iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for every offset named here (the accesses of
// Collections/OCFRunArray.c). The object holds its guts, which run arrays share copy-on-write: a reference
// count (changed under the unit's lock), the total length, the number of blocks and their capacity, the block
// and start location of the last lookup, then the blocks, each a length and a retained object. The type and
// field names are local descriptive spellings.
typedef struct {
    OCFIndex length;
    OCFTypeRef obj;
} RunArrayItem;

typedef struct {
    OCFIndex numRefs;
    OCFIndex length;
    OCFIndex numBlocks;
    OCFIndex maxBlocks;
    OCFIndex cachedBlock;
    OCFIndex cachedLocation;
    RunArrayItem list[];
} RunArrayGuts;

OF_ABI_ASSERT(sizeof(RunArrayGuts) == 48, "The native run array guts header is 48 bytes.");

typedef struct __OCFRunArray *OCFRunArrayRef;

struct __OCFRunArray {
    OCFRuntimeBase base;
    RunArrayGuts *guts;
};

// Exported natively (CFRunArray is SPI; no SDK declaration). Signatures from the native register use.
OF_EXPORT OCFTypeID OCFRunArrayGetTypeID(void);
OF_EXPORT OCFRunArrayRef _OCFRunArrayCreateWithGuts(OCFAllocatorRef allocator, RunArrayGuts *guts);
OF_EXPORT OCFRunArrayRef OCFRunArrayCreate(OCFAllocatorRef allocator);
OF_EXPORT OCFTypeRef OCFRunArrayGetValueAtIndex(
    OCFRunArrayRef array, OCFIndex location, OCFRange *effectiveRange, OCFIndex *blockIndex
);
OF_EXPORT void OCFRunArrayInsert(OCFRunArrayRef array, OCFRange range, OCFTypeRef obj);
OF_EXPORT void OCFRunArrayDelete(OCFRunArrayRef array, OCFRange range);
OF_EXPORT void
OCFRunArrayReplace(OCFRunArrayRef array, OCFRange range, OCFTypeRef newObject, OCFIndex newLength);

// The class record (type 63), defined by Collections/OCFRunArray.c for the builtin table.
OF_EXTERN const OCFRuntimeClass __CFRunArrayClass;
