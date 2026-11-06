//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <dispatch/dispatch.h>
#include <stdbool.h>
#include <stdint.h>

// CFData.h (iOS Simulator SDK interface), spelled OCF.
typedef const struct __OCFData *OCFDataRef;
typedef struct __OCFData *OCFMutableDataRef;

OF_EXPORT OCFTypeID OCFDataGetTypeID(void);
// Native __CFDataMtbl, 0x1c01896a8 (external): the mutable bit.
OF_EXPORT Boolean __OCFDataMtbl(OCFDataRef data);
OF_EXPORT OCFDataRef OCFDataCreate(OCFAllocatorRef allocator, const UInt8 *bytes, OCFIndex length);
OF_EXPORT OCFDataRef OCFDataCreateWithBytesNoCopy(
    OCFAllocatorRef allocator, const UInt8 *bytes, OCFIndex length, OCFAllocatorRef bytesDeallocator
);
OF_EXPORT OCFDataRef OCFDataCreateCopy(OCFAllocatorRef allocator, OCFDataRef theData);
OF_EXPORT OCFMutableDataRef OCFDataCreateMutable(OCFAllocatorRef allocator, OCFIndex capacity);
OF_EXPORT OCFMutableDataRef
OCFDataCreateMutableCopy(OCFAllocatorRef allocator, OCFIndex capacity, OCFDataRef theData);
OF_EXPORT OCFIndex OCFDataGetLength(OCFDataRef theData);
OF_EXPORT const UInt8 *OCFDataGetBytePtr(OCFDataRef theData);
OF_EXPORT UInt8 *OCFDataGetMutableBytePtr(OCFMutableDataRef theData);
OF_EXPORT void OCFDataSetLength(OCFMutableDataRef theData, OCFIndex length);
OF_EXPORT void OCFDataAppendBytes(OCFMutableDataRef theData, const UInt8 *bytes, OCFIndex length);
OF_EXPORT void
OCFDataReplaceBytes(OCFMutableDataRef theData, OCFRange range, const UInt8 *newBytes, OCFIndex newLength);

// Native private externals, hidden with their native names (non-external in the pinned image).
OF_EXTERN OCFMutableDataRef _CFDataCreateFixedMutableWithBuffer(
    OCFAllocatorRef allocator, OCFIndex capacity, const UInt8 *buffer, OCFAllocatorRef deallocator
);

OF_EXTERN dispatch_data_t _CFDataCreateDispatchData(OCFDataRef data);
OF_EXTERN uint8_t *_CFDataGetBytePtrNonObjC(OCFDataRef data);

// The URL parameter is spelled with its structure tag (OCFURLRef, URL/OCFURL.h): the URL header includes the
// string header, which includes this one, as CFURL.h includes CFData.h.
struct __OCFURL;

OF_EXTERN OCFDataRef _CFDataCreateFromURL(const struct __OCFURL *url, OCFErrorRef *error);

// The CFData class record (type 20), for the builtin class table.
OF_EXTERN const OCFRuntimeClass __CFDataClass;
