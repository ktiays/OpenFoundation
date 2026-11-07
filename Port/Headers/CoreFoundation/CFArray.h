//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>

CF_EXTERN_C_BEGIN

typedef const void *(*CFArrayRetainCallBack)(CFAllocatorRef allocator, const void *value);
typedef void (*CFArrayReleaseCallBack)(CFAllocatorRef allocator, const void *value);
typedef CFStringRef (*CFArrayCopyDescriptionCallBack)(const void *value);
typedef Boolean (*CFArrayEqualCallBack)(const void *value1, const void *value2);

typedef struct {
    CFIndex version;
    CFArrayRetainCallBack retain;
    CFArrayReleaseCallBack release;
    CFArrayCopyDescriptionCallBack copyDescription;
    CFArrayEqualCallBack equal;
} CFArrayCallBacks;

CF_EXPORT const CFArrayCallBacks kCFTypeArrayCallBacks;

typedef void (*CFArrayApplierFunction)(const void *value, void *context);

typedef const struct CF_BRIDGED_TYPE(NSArray) __CFArray *CFArrayRef;
typedef struct CF_BRIDGED_MUTABLE_TYPE(NSMutableArray) __CFArray *CFMutableArrayRef;

CF_EXPORT CFTypeID CFArrayGetTypeID(void);
CF_EXPORT CFArrayRef CFArrayCreate(
    CFAllocatorRef allocator,
    const void **values,
    CFIndex numValues,
    const CFArrayCallBacks *callBacks
);
CF_EXPORT CFArrayRef CFArrayCreateCopy(CFAllocatorRef allocator, CFArrayRef theArray);
CF_EXPORT CFMutableArrayRef CFArrayCreateMutable(
    CFAllocatorRef allocator,
    CFIndex capacity,
    const CFArrayCallBacks *callBacks
);
CF_EXPORT CFMutableArrayRef CFArrayCreateMutableCopy(
    CFAllocatorRef allocator,
    CFIndex capacity,
    CFArrayRef theArray
);
CF_EXPORT CFIndex CFArrayGetCount(CFArrayRef theArray);
CF_EXPORT Boolean CFArrayContainsValue(CFArrayRef theArray, CFRange range, const void *value);
CF_EXPORT CFIndex CFArrayGetFirstIndexOfValue(CFArrayRef theArray, CFRange range, const void *value);
CF_EXPORT const void *CFArrayGetValueAtIndex(CFArrayRef theArray, CFIndex idx);
CF_EXPORT void CFArrayGetValues(CFArrayRef theArray, CFRange range, const void **values);
CF_EXPORT void CFArrayApplyFunction(
    CFArrayRef theArray,
    CFRange range,
    CFArrayApplierFunction applier,
    void *context
);
CF_EXPORT CFIndex CFArrayBSearchValues(
    CFArrayRef theArray,
    CFRange range,
    const void *value,
    CFComparatorFunction comparator,
    void *context
);
CF_EXPORT void CFArrayAppendValue(CFMutableArrayRef theArray, const void *value);
CF_EXPORT void CFArrayInsertValueAtIndex(CFMutableArrayRef theArray, CFIndex idx, const void *value);
CF_EXPORT void CFArraySetValueAtIndex(CFMutableArrayRef theArray, CFIndex idx, const void *value);
CF_EXPORT void CFArrayRemoveValueAtIndex(CFMutableArrayRef theArray, CFIndex idx);
CF_EXPORT void CFArrayRemoveAllValues(CFMutableArrayRef theArray);
CF_EXPORT void CFArrayReplaceValues(
    CFMutableArrayRef theArray,
    CFRange range,
    const void **newValues,
    CFIndex newCount
);
CF_EXPORT void CFArrayExchangeValuesAtIndices(CFMutableArrayRef theArray, CFIndex idx1, CFIndex idx2);
CF_EXPORT void CFArraySortValues(
    CFMutableArrayRef theArray,
    CFRange range,
    CFComparatorFunction comparator,
    void *context
);
CF_EXPORT void CFArrayAppendArray(CFMutableArrayRef theArray, CFArrayRef otherArray, CFRange otherRange);

CF_EXTERN_C_END
