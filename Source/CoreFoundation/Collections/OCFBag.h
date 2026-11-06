//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Runtime/OCFRuntime.h"

// Public types with the iOS Simulator SDK's CFBag.h signatures.
typedef const void *(*OCFBagRetainCallBack)(OCFAllocatorRef allocator, const void *value);
typedef void (*OCFBagReleaseCallBack)(OCFAllocatorRef allocator, const void *value);
typedef OCFStringRef (*OCFBagCopyDescriptionCallBack)(const void *value);
typedef Boolean (*OCFBagEqualCallBack)(const void *value1, const void *value2);
typedef OCFHashCode (*OCFBagHashCallBack)(const void *value);

typedef struct {
    OCFIndex version;
    OCFBagRetainCallBack retain;
    OCFBagReleaseCallBack release;
    OCFBagCopyDescriptionCallBack copyDescription;
    OCFBagEqualCallBack equal;
    OCFBagHashCallBack hash;
} OCFBagCallBacks;

typedef void (*OCFBagApplierFunction)(const void *value, void *context);

typedef const struct __OCFBag *OCFBagRef;
typedef struct __OCFBag *OCFMutableBagRef;

OF_EXPORT const OCFBagCallBacks kOCFTypeBagCallBacks;

OF_EXPORT OCFMutableBagRef
OCFBagCreateMutable(OCFAllocatorRef allocator, OCFIndex capacity, const OCFBagCallBacks *callBacks);
OF_EXPORT OCFBagRef OCFBagCreateCopy(OCFAllocatorRef allocator, OCFBagRef theBag);
OF_EXPORT OCFIndex OCFBagGetCountOfValue(OCFBagRef theBag, const void *value);
OF_EXPORT void OCFBagApplyFunction(OCFBagRef theBag, OCFBagApplierFunction applier, void *context);
OF_EXPORT void OCFBagAddValue(OCFMutableBagRef theBag, const void *value);
OF_EXPORT void OCFBagRemoveValue(OCFMutableBagRef theBag, const void *value);

// The class record (type 4), defined by this unit for the builtin table.
OF_EXTERN const OCFRuntimeClass __CFBagClass;
