//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>

#include <stdint.h>

// The MacTypes subset CFBase.h declares when MacTypes.h is absent (iOS Simulator SDK interface).
#if !defined(__MACTYPES__) && !defined(_OS_OSTYPES_H)
typedef unsigned char Boolean;
typedef unsigned char UInt8;
typedef signed char SInt8;
typedef unsigned short UInt16;
typedef signed short SInt16;
typedef unsigned int UInt32;
typedef signed int SInt32;
typedef uint64_t UInt64;
typedef int64_t SInt64;
typedef SInt32 OSStatus;
typedef float Float32;
typedef double Float64;
typedef unsigned short UniChar;
typedef unsigned long UniCharCount;
typedef UInt32 UTF32Char;
typedef UInt16 UTF16Char;
typedef UInt8 UTF8Char;
#endif

typedef unsigned long OCFTypeID;
typedef unsigned long long OCFAllocatorTypeID;
typedef signed long OCFIndex;
typedef unsigned long OCFHashCode;
typedef unsigned long OCFOptionFlags;

typedef const void *OCFTypeRef;
typedef const struct __OCFString *OCFStringRef;
typedef struct __OCFString *OCFMutableStringRef;
typedef const struct __OCFAllocator *OCFAllocatorRef;
typedef const struct __OCFNull *OCFNullRef;

typedef enum : OCFIndex {
    kOCFCompareLessThan = -1L,
    kOCFCompareEqualTo = 0,
    kOCFCompareGreaterThan = 1,
} OCFComparisonResult;

enum {
    kOCFNotFound = -1,
};

typedef OCFComparisonResult (*OCFComparatorFunction)(const void *val1, const void *val2, void *context);

typedef struct {
    OCFIndex location;
    OCFIndex length;
} OCFRange;

OF_EXPORT OCFRange __OCFRangeMake(OCFIndex location, OCFIndex length);

typedef const void *(*OCFAllocatorRetainCallBack)(const void *info);
typedef void (*OCFAllocatorReleaseCallBack)(const void *info);
typedef OCFStringRef (*OCFAllocatorCopyDescriptionCallBack)(const void *info);
typedef void *(*OCFAllocatorAllocateCallBack)(OCFIndex allocSize, OCFOptionFlags hint, void *info);
typedef void *(*OCFAllocatorReallocateCallBack)(void *ptr, OCFIndex newsize, OCFOptionFlags hint, void *info);
typedef void (*OCFAllocatorDeallocateCallBack)(void *ptr, void *info);
typedef OCFIndex (*OCFAllocatorPreferredSizeCallBack)(OCFIndex size, OCFOptionFlags hint, void *info);

typedef struct {
    OCFIndex version;
    void *info;
    OCFAllocatorRetainCallBack retain;
    OCFAllocatorReleaseCallBack release;
    OCFAllocatorCopyDescriptionCallBack copyDescription;
    OCFAllocatorAllocateCallBack allocate;
    OCFAllocatorReallocateCallBack reallocate;
    OCFAllocatorDeallocateCallBack deallocate;
    OCFAllocatorPreferredSizeCallBack preferredSize;
} OCFAllocatorContext;

OF_EXPORT const OCFAllocatorRef kOCFAllocatorDefault;
OF_EXPORT const OCFAllocatorRef kOCFAllocatorSystemDefault;
OF_EXPORT const OCFAllocatorRef kOCFAllocatorMalloc;
OF_EXPORT const OCFAllocatorRef kOCFAllocatorMallocZone;
OF_EXPORT const OCFAllocatorRef kOCFAllocatorNull;

OF_EXPORT OCFAllocatorRef OCFAllocatorGetDefault(void);
OF_EXPORT OCFAllocatorRef OCFAllocatorCreate(OCFAllocatorRef allocator, OCFAllocatorContext *context);
OF_EXPORT void OCFAllocatorDeallocate(OCFAllocatorRef allocator, void *ptr);

OF_EXPORT void *OCFAllocatorAllocateTyped(
    OCFAllocatorRef allocator, OCFIndex size, OCFAllocatorTypeID descriptor, OCFOptionFlags hint
);
OF_EXPORT OCFIndex
OCFAllocatorGetPreferredSizeForSize(OCFAllocatorRef allocator, OCFIndex size, OCFOptionFlags hint);

OF_EXPORT OCFTypeID OCFGetTypeID(OCFTypeRef cf);
OF_EXPORT OCFStringRef OCFCopyTypeIDDescription(OCFTypeID type_id);
OF_EXPORT OCFTypeRef OCFRetain(OCFTypeRef cf);
OF_EXPORT void OCFRelease(OCFTypeRef cf);
OF_EXPORT OCFTypeRef OCFAutorelease(OCFTypeRef arg);
OF_EXPORT OCFIndex OCFGetRetainCount(OCFTypeRef cf);
OF_EXPORT Boolean OCFEqual(OCFTypeRef cf1, OCFTypeRef cf2);
OF_EXPORT OCFHashCode OCFHash(OCFTypeRef cf);
OF_EXPORT OCFStringRef OCFCopyDescription(OCFTypeRef cf);
OF_EXPORT OCFAllocatorRef OCFGetAllocator(OCFTypeRef cf);

OF_EXPORT OCFTypeID OCFNullGetTypeID(void);
OF_EXPORT const OCFNullRef kOCFNull;
