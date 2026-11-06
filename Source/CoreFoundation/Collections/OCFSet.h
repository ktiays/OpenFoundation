//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Runtime/OCFRuntime.h"

// Public types with the iOS Simulator SDK's CFSet.h signatures.
typedef const void *(*OCFSetRetainCallBack)(OCFAllocatorRef allocator, const void *value);
typedef void (*OCFSetReleaseCallBack)(OCFAllocatorRef allocator, const void *value);
typedef OCFStringRef (*OCFSetCopyDescriptionCallBack)(const void *value);
typedef Boolean (*OCFSetEqualCallBack)(const void *value1, const void *value2);
typedef OCFHashCode (*OCFSetHashCallBack)(const void *value);

typedef struct {
    OCFIndex version;
    OCFSetRetainCallBack retain;
    OCFSetReleaseCallBack release;
    OCFSetCopyDescriptionCallBack copyDescription;
    OCFSetEqualCallBack equal;
    OCFSetHashCallBack hash;
} OCFSetCallBacks;

typedef void (*OCFSetApplierFunction)(const void *value, void *context);

typedef const struct __OCFSet *OCFSetRef;
typedef struct __OCFSet *OCFMutableSetRef;

OF_EXPORT const OCFSetCallBacks kOCFTypeSetCallBacks;
OF_EXPORT const OCFSetCallBacks kOCFCopyStringSetCallBacks;

OF_EXPORT OCFSetRef OCFSetCreate(
    OCFAllocatorRef allocator, const void **values, OCFIndex numValues, const OCFSetCallBacks *callBacks
);
OF_EXPORT OCFSetRef OCFSetCreateCopy(OCFAllocatorRef allocator, OCFSetRef theSet);
OF_EXPORT OCFMutableSetRef
OCFSetCreateMutable(OCFAllocatorRef allocator, OCFIndex capacity, const OCFSetCallBacks *callBacks);
OF_EXPORT OCFMutableSetRef
OCFSetCreateMutableCopy(OCFAllocatorRef allocator, OCFIndex capacity, OCFSetRef theSet);
OF_EXPORT OCFIndex OCFSetGetCount(OCFSetRef theSet);
OF_EXPORT Boolean OCFSetContainsValue(OCFSetRef theSet, const void *value);
OF_EXPORT const void *OCFSetGetValue(OCFSetRef theSet, const void *value);
OF_EXPORT void OCFSetGetValues(OCFSetRef theSet, const void **values);
OF_EXPORT void OCFSetApplyFunction(OCFSetRef theSet, OCFSetApplierFunction applier, void *context);
OF_EXPORT void _OCFSetSetCapacity(OCFMutableSetRef theSet, OCFIndex capacity);
OF_EXPORT void OCFSetAddValue(OCFMutableSetRef theSet, const void *value);
OF_EXPORT void OCFSetSetValue(OCFMutableSetRef theSet, const void *value);
OF_EXPORT void OCFSetRemoveValue(OCFMutableSetRef theSet, const void *value);
OF_EXPORT void OCFSetRemoveAllValues(OCFMutableSetRef theSet);
OF_EXPORT Boolean OCFSetGetValueIfPresent(OCFSetRef theSet, const void *candidate, const void **value);

// External natively (no SDK declaration). The enumeration state has the layout of NSFastEnumerationState.
OF_EXPORT Boolean _OCFSetIsMutable(OCFSetRef theSet);
OF_EXPORT unsigned long
_OCFSetFastEnumeration(OCFSetRef theSet, void *state, void *stackbuffer, unsigned long count);

// Original names, hidden (were private external).
OF_EXTERN OCFSetRef __CFSetCreateTransfer(OCFAllocatorRef allocator, const void **values, OCFIndex count);
OF_EXTERN void CFSetApply(OCFSetRef theSet, void (^block)(const void *value, Boolean *stop));
// Native __CFSetGetCallbacks; the record is returned by value (through X8).
OF_EXTERN OCFSetCallBacks __CFSetGetCallbacks(OCFSetRef theSet);

// The class record (type 17), defined by this unit for the builtin table.
OF_EXTERN const OCFRuntimeClass __CFSetClass;

// Native CFSetGetTypeID, 0x1c01eda30 (Collections/OCFSet.c). External.
OF_EXPORT OCFTypeID OCFSetGetTypeID(void);
