//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>

CF_EXTERN_C_BEGIN

typedef const void *(*CFDictionaryRetainCallBack)(CFAllocatorRef allocator, const void *value);
typedef void (*CFDictionaryReleaseCallBack)(CFAllocatorRef allocator, const void *value);
typedef CFStringRef (*CFDictionaryCopyDescriptionCallBack)(const void *value);
typedef Boolean (*CFDictionaryEqualCallBack)(const void *value1, const void *value2);
typedef CFHashCode (*CFDictionaryHashCallBack)(const void *value);

typedef struct {
    CFIndex version;
    CFDictionaryRetainCallBack retain;
    CFDictionaryReleaseCallBack release;
    CFDictionaryCopyDescriptionCallBack copyDescription;
    CFDictionaryEqualCallBack equal;
    CFDictionaryHashCallBack hash;
} CFDictionaryKeyCallBacks;

CF_EXPORT const CFDictionaryKeyCallBacks kCFTypeDictionaryKeyCallBacks;
CF_EXPORT const CFDictionaryKeyCallBacks kCFCopyStringDictionaryKeyCallBacks;

typedef struct {
    CFIndex version;
    CFDictionaryRetainCallBack retain;
    CFDictionaryReleaseCallBack release;
    CFDictionaryCopyDescriptionCallBack copyDescription;
    CFDictionaryEqualCallBack equal;
} CFDictionaryValueCallBacks;

CF_EXPORT const CFDictionaryValueCallBacks kCFTypeDictionaryValueCallBacks;

typedef void (*CFDictionaryApplierFunction)(const void *key, const void *value, void *context);

typedef const struct CF_BRIDGED_TYPE(NSDictionary) __CFDictionary *CFDictionaryRef;
typedef struct CF_BRIDGED_MUTABLE_TYPE(NSMutableDictionary) __CFDictionary *CFMutableDictionaryRef;

CF_EXPORT CFTypeID CFDictionaryGetTypeID(void);
CF_EXPORT CFDictionaryRef CFDictionaryCreate(
    CFAllocatorRef allocator,
    const void **keys,
    const void **values,
    CFIndex numValues,
    const CFDictionaryKeyCallBacks *keyCallBacks,
    const CFDictionaryValueCallBacks *valueCallBacks
);
CF_EXPORT CFDictionaryRef CFDictionaryCreateCopy(CFAllocatorRef allocator, CFDictionaryRef theDict);
CF_EXPORT CFMutableDictionaryRef CFDictionaryCreateMutable(
    CFAllocatorRef allocator,
    CFIndex capacity,
    const CFDictionaryKeyCallBacks *keyCallBacks,
    const CFDictionaryValueCallBacks *valueCallBacks
);
CF_EXPORT CFMutableDictionaryRef CFDictionaryCreateMutableCopy(
    CFAllocatorRef allocator,
    CFIndex capacity,
    CFDictionaryRef theDict
);
CF_EXPORT CFIndex CFDictionaryGetCount(CFDictionaryRef theDict);
CF_EXPORT Boolean CFDictionaryContainsKey(CFDictionaryRef theDict, const void *key);
CF_EXPORT const void *CFDictionaryGetValue(CFDictionaryRef theDict, const void *key);
CF_EXPORT Boolean CFDictionaryGetValueIfPresent(CFDictionaryRef theDict, const void *key, const void **value);
CF_EXPORT void CFDictionaryGetKeysAndValues(CFDictionaryRef theDict, const void **keys, const void **values);
CF_EXPORT void CFDictionaryApplyFunction(
    CFDictionaryRef theDict,
    CFDictionaryApplierFunction applier,
    void *context
);
CF_EXPORT void CFDictionaryAddValue(CFMutableDictionaryRef theDict, const void *key, const void *value);
CF_EXPORT void CFDictionarySetValue(CFMutableDictionaryRef theDict, const void *key, const void *value);
CF_EXPORT void CFDictionaryRemoveValue(CFMutableDictionaryRef theDict, const void *key);
CF_EXPORT void CFDictionaryRemoveAllValues(CFMutableDictionaryRef theDict);

CF_EXTERN_C_END
