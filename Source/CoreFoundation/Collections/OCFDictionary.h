//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Runtime/OCFRuntime.h"

// Public types with the iOS Simulator SDK's CFDictionary.h signatures.
typedef const void *(*OCFDictionaryRetainCallBack)(OCFAllocatorRef allocator, const void *value);
typedef void (*OCFDictionaryReleaseCallBack)(OCFAllocatorRef allocator, const void *value);
typedef OCFStringRef (*OCFDictionaryCopyDescriptionCallBack)(const void *value);
typedef Boolean (*OCFDictionaryEqualCallBack)(const void *value1, const void *value2);
typedef OCFHashCode (*OCFDictionaryHashCallBack)(const void *value);

typedef struct {
    OCFIndex version;
    OCFDictionaryRetainCallBack retain;
    OCFDictionaryReleaseCallBack release;
    OCFDictionaryCopyDescriptionCallBack copyDescription;
    OCFDictionaryEqualCallBack equal;
    OCFDictionaryHashCallBack hash;
} OCFDictionaryKeyCallBacks;

typedef struct {
    OCFIndex version;
    OCFDictionaryRetainCallBack retain;
    OCFDictionaryReleaseCallBack release;
    OCFDictionaryCopyDescriptionCallBack copyDescription;
    OCFDictionaryEqualCallBack equal;
} OCFDictionaryValueCallBacks;

typedef void (*OCFDictionaryApplierFunction)(const void *key, const void *value, void *context);

typedef const struct __OCFDictionary *OCFDictionaryRef;
typedef struct __OCFDictionary *OCFMutableDictionaryRef;

OF_EXPORT const OCFDictionaryKeyCallBacks kOCFTypeDictionaryKeyCallBacks;
OF_EXPORT const OCFDictionaryKeyCallBacks kOCFCopyStringDictionaryKeyCallBacks;
OF_EXPORT const OCFDictionaryValueCallBacks kOCFTypeDictionaryValueCallBacks;

OF_EXPORT OCFTypeID OCFDictionaryGetTypeID(void);
OF_EXPORT OCFDictionaryRef OCFDictionaryCreate(
    OCFAllocatorRef allocator,
    const void **keys,
    const void **values,
    OCFIndex numValues,
    const OCFDictionaryKeyCallBacks *keyCallBacks,
    const OCFDictionaryValueCallBacks *valueCallBacks
);
OF_EXPORT OCFDictionaryRef OCFDictionaryCreateCopy(OCFAllocatorRef allocator, OCFDictionaryRef theDict);
OF_EXPORT OCFMutableDictionaryRef OCFDictionaryCreateMutable(
    OCFAllocatorRef allocator,
    OCFIndex capacity,
    const OCFDictionaryKeyCallBacks *keyCallBacks,
    const OCFDictionaryValueCallBacks *valueCallBacks
);
OF_EXPORT OCFMutableDictionaryRef
OCFDictionaryCreateMutableCopy(OCFAllocatorRef allocator, OCFIndex capacity, OCFDictionaryRef theDict);
OF_EXPORT OCFIndex OCFDictionaryGetCount(OCFDictionaryRef theDict);
OF_EXPORT Boolean OCFDictionaryContainsKey(OCFDictionaryRef theDict, const void *key);
OF_EXPORT const void *OCFDictionaryGetValue(OCFDictionaryRef theDict, const void *key);
OF_EXPORT Boolean
OCFDictionaryGetValueIfPresent(OCFDictionaryRef theDict, const void *key, const void **value);

// Native CFDictionaryGetKeyIfPresent, 0x1c01956e8. External (SPI).
OF_EXPORT Boolean
OCFDictionaryGetKeyIfPresent(OCFDictionaryRef theDict, const void *key, const void **actualKey);
OF_EXPORT void
OCFDictionaryGetKeysAndValues(OCFDictionaryRef theDict, const void **keys, const void **values);
OF_EXPORT void
OCFDictionaryApplyFunction(OCFDictionaryRef theDict, OCFDictionaryApplierFunction applier, void *context);
OF_EXPORT void OCFDictionaryAddValue(OCFMutableDictionaryRef theDict, const void *key, const void *value);
OF_EXPORT void OCFDictionarySetValue(OCFMutableDictionaryRef theDict, const void *key, const void *value);
OF_EXPORT void OCFDictionaryRemoveValue(OCFMutableDictionaryRef theDict, const void *key);

OF_EXPORT void OCFDictionaryRemoveAllValues(OCFMutableDictionaryRef theDict);

// External natively (no SDK declaration). The enumeration state has the layout of NSFastEnumerationState.
OF_EXPORT void _OCFDictionarySetCapacity(OCFMutableDictionaryRef theDict, OCFIndex capacity);
OF_EXPORT Boolean _OCFDictionaryIsMutable(OCFDictionaryRef theDict);
OF_EXPORT void _OCFDictionarySetKVOBit(OCFDictionaryRef theDict, OCFIndex bit);
OF_EXPORT unsigned long
_OCFDictionaryFastEnumeration(OCFDictionaryRef theDict, void *state, void *stackbuffer, unsigned long count);

// Original names, hidden (were private external).
OF_EXTERN OCFDictionaryRef __CFDictionaryCreateTransfer(
    OCFAllocatorRef allocator, const void **keys, const void **values, OCFIndex count
);
// Native __CFDictionaryGetKeyCallbacks and __CFDictionaryGetValueCallbacks; the records are returned by value
// (through X8).
OF_EXTERN OCFDictionaryKeyCallBacks __CFDictionaryGetKeyCallbacks(OCFDictionaryRef theDict);
OF_EXTERN OCFDictionaryValueCallBacks __CFDictionaryGetValueCallbacks(OCFDictionaryRef theDict);
OF_EXTERN void
CFDictionaryApply(OCFDictionaryRef theDict, void (^block)(const void *key, const void *value, Boolean *stop));

// The class record (type 18), defined by this unit for the builtin table.
OF_EXTERN const OCFRuntimeClass __CFDictionaryClass;

// The Foundation-present fast paths these functions try first (NSCFDictionary unit; hidden natively, were
// private external); declared here until their unit's header carries them. Signatures from their native
// readers' register use and their own entry code (confidence: medium).
OF_EXTERN OCFDictionaryRef __NSCFDictionaryCreateTransfer(
    OCFAllocatorRef allocator, const void **keys, const void **values, OCFIndex count
);
OF_EXTERN OCFDictionaryRef __NSCFDictionaryCreate(
    OCFAllocatorRef allocator,
    const void **keys,
    const void **values,
    OCFIndex numValues,
    const OCFDictionaryKeyCallBacks *keyCallBacks,
    const OCFDictionaryValueCallBacks *valueCallBacks
);
OF_EXTERN OCFMutableDictionaryRef __NSCFDictionaryCreateMutable(
    OCFAllocatorRef allocator,
    OCFIndex capacity,
    const OCFDictionaryKeyCallBacks *keyCallBacks,
    const OCFDictionaryValueCallBacks *valueCallBacks
);
