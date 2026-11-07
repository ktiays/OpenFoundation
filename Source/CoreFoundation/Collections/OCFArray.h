//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFSortFunctions.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Public CFArray interface, iOS Simulator SDK CFArray.h, spelled with the OCF prefix.
typedef const void *(*OCFArrayRetainCallBack)(OCFAllocatorRef allocator, const void *value);
typedef void (*OCFArrayReleaseCallBack)(OCFAllocatorRef allocator, const void *value);
typedef OCFStringRef (*OCFArrayCopyDescriptionCallBack)(const void *value);
typedef Boolean (*OCFArrayEqualCallBack)(const void *value1, const void *value2);

typedef struct {
    OCFIndex version;
    OCFArrayRetainCallBack retain;
    OCFArrayReleaseCallBack release;
    OCFArrayCopyDescriptionCallBack copyDescription;
    OCFArrayEqualCallBack equal;
} OCFArrayCallBacks;

typedef void (*OCFArrayApplierFunction)(const void *value, void *context);

typedef const struct __OCFArray *OCFArrayRef;
typedef struct __OCFArray *OCFMutableArrayRef;

// The CFArray object, iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// for every offset (the accesses of the functions in Collections/OCFArray.c). Info bits 0-1 are the storage
// type (0 immutable with inline values, 2 a mutable deque); bits 2-3 the callbacks (0 none, 1 the CFType
// callbacks, 3 a copy of custom callbacks at offset 48). An immutable array's values follow at offset 48, or
// at 88 after custom callbacks; a mutable array's values live in the deque at offset 40: its left index, its
// capacity, then the buckets. The field names are local descriptive spellings.
typedef struct {
    OCFIndex leftIdx;
    OCFIndex capacity;
    const void *buckets[];
} ArrayDeque;

struct __OCFArray {
    OCFRuntimeBase base;
    OCFIndex count;
    OCFIndex mutations;
    int32_t mutationsInProgress;
    ArrayDeque *store;
};

OF_ABI_ASSERT(sizeof(struct __OCFArray) == 48, "The native CFArray header is 48 bytes.");

OF_EXPORT const OCFArrayCallBacks kOCFTypeArrayCallBacks;

OF_EXPORT OCFTypeID OCFArrayGetTypeID(void);
OF_EXPORT OCFArrayRef OCFArrayCreate(
    OCFAllocatorRef allocator, const void **values, OCFIndex numValues, const OCFArrayCallBacks *callBacks
);
OF_EXPORT OCFArrayRef OCFArrayCreateCopy(OCFAllocatorRef allocator, OCFArrayRef theArray);
OF_EXPORT OCFMutableArrayRef
OCFArrayCreateMutable(OCFAllocatorRef allocator, OCFIndex capacity, const OCFArrayCallBacks *callBacks);
OF_EXPORT OCFMutableArrayRef
OCFArrayCreateMutableCopy(OCFAllocatorRef allocator, OCFIndex capacity, OCFArrayRef theArray);
OF_EXPORT OCFIndex OCFArrayGetCount(OCFArrayRef theArray);
OF_EXPORT Boolean OCFArrayContainsValue(OCFArrayRef theArray, OCFRange range, const void *value);
OF_EXPORT const void *OCFArrayGetValueAtIndex(OCFArrayRef theArray, OCFIndex idx);
OF_EXPORT void OCFArrayGetValues(OCFArrayRef theArray, OCFRange range, const void **values);
OF_EXPORT void
OCFArrayApplyFunction(OCFArrayRef theArray, OCFRange range, OCFArrayApplierFunction applier, void *context);
OF_EXPORT OCFIndex OCFArrayGetFirstIndexOfValue(OCFArrayRef theArray, OCFRange range, const void *value);
OF_EXPORT OCFIndex OCFArrayBSearchValues(
    OCFArrayRef theArray, OCFRange range, const void *value, OCFComparatorFunction comparator, void *context
);
OF_EXPORT void OCFArrayAppendValue(OCFMutableArrayRef theArray, const void *value);
OF_EXPORT void OCFArrayInsertValueAtIndex(OCFMutableArrayRef theArray, OCFIndex idx, const void *value);
OF_EXPORT void OCFArraySetValueAtIndex(OCFMutableArrayRef theArray, OCFIndex idx, const void *value);
OF_EXPORT void OCFArrayRemoveValueAtIndex(OCFMutableArrayRef theArray, OCFIndex idx);
OF_EXPORT void OCFArrayRemoveAllValues(OCFMutableArrayRef theArray);
OF_EXPORT void
OCFArrayReplaceValues(OCFMutableArrayRef theArray, OCFRange range, const void **newValues, OCFIndex newCount);
OF_EXPORT void OCFArrayExchangeValuesAtIndices(OCFMutableArrayRef theArray, OCFIndex idx1, OCFIndex idx2);
OF_EXPORT void OCFArraySortValues(
    OCFMutableArrayRef theArray, OCFRange range, OCFComparatorFunction comparator, void *context
);
OF_EXPORT void OCFArrayAppendArray(OCFMutableArrayRef theArray, OCFArrayRef otherArray, OCFRange otherRange);

// Private exported functions (no SDK declaration; signatures read from the native code and its call sites).
OF_EXPORT void _OCFArraySetCapacity(OCFMutableArrayRef array, OCFIndex cap);
OF_EXPORT void
_OCFArrayReplaceValues(OCFMutableArrayRef array, OCFRange range, const void **newValues, OCFIndex newCount);
OF_EXPORT Boolean _OCFArrayIsMutable(OCFArrayRef array);
OF_EXPORT const void *
_OCFArrayCheckAndGetValueAtIndex(OCFArrayRef array, OCFIndex index, Boolean *outOfBounds);
// The enumeration state has the layout of NSFastEnumerationState.
OF_EXPORT unsigned long
_OCFArrayFastEnumeration(OCFArrayRef array, void *state, void *stackbuffer, unsigned long count);

// Hidden functions (were private externals); native names preserved.
OF_EXTERN const OCFArrayCallBacks *__CFArrayGetCallBacks(OCFArrayRef array);
OF_EXTERN OCFArrayRef
__CFArrayCreateTransfer(OCFAllocatorRef allocator, const void **values, OCFIndex numValues);
OF_EXTERN OCFArrayRef __CFArrayCreate0(
    OCFAllocatorRef allocator, const void **values, OCFIndex numValues, const OCFArrayCallBacks *callBacks
);
OF_EXTERN OCFMutableArrayRef
__CFArrayCreateMutable0(OCFAllocatorRef allocator, OCFIndex capacity, const OCFArrayCallBacks *callBacks);
OF_EXTERN OCFArrayRef __CFArrayCreateCopy0(OCFAllocatorRef allocator, OCFArrayRef array);
OF_EXTERN OCFMutableArrayRef
__CFArrayCreateMutableCopy0(OCFAllocatorRef allocator, OCFIndex capacity, OCFArrayRef array);
// Native _CFNonObjCArrayGetCount, 0x1c014c0b0.
OF_EXTERN OCFIndex _CFNonObjCArrayGetCount(OCFArrayRef array);
OF_EXTERN void CFArrayApply(OCFArrayRef array, OCFRange range, void (^block)(const void *value, bool *stop));

// The builtin class record of type 19.
OF_EXTERN const OCFRuntimeClass __CFArrayClass;

// Native _CFCreateArrayStorage, 0x1c015b2ec (external), the temporary object-buffer storage of the native
// new_id_array pattern; natively at the head of the __NSArrayM unit, restored in Collections/OCFArray.c.
// *outCount receives the number of pointers the block holds.
OF_EXPORT void *_OCFCreateArrayStorage(size_t numPointers, Boolean zeroed, size_t *outCount);
