//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>

CF_EXTERN_C_BEGIN

typedef const struct CF_BRIDGED_TYPE(NSNumber) __CFBoolean *CFBooleanRef;

CF_EXPORT const CFBooleanRef kCFBooleanTrue;
CF_EXPORT const CFBooleanRef kCFBooleanFalse;

CF_EXPORT CFTypeID CFBooleanGetTypeID(void);
CF_EXPORT Boolean CFBooleanGetValue(CFBooleanRef boolean);

typedef CF_ENUM(CFIndex, CFNumberType) {
    kCFNumberSInt8Type = 1,
    kCFNumberSInt16Type = 2,
    kCFNumberSInt32Type = 3,
    kCFNumberSInt64Type = 4,
    kCFNumberFloat32Type = 5,
    kCFNumberFloat64Type = 6,
    kCFNumberCharType = 7,
    kCFNumberShortType = 8,
    kCFNumberIntType = 9,
    kCFNumberLongType = 10,
    kCFNumberLongLongType = 11,
    kCFNumberFloatType = 12,
    kCFNumberDoubleType = 13,
    kCFNumberCFIndexType = 14,
    kCFNumberNSIntegerType = 15,
    kCFNumberCGFloatType = 16,
    kCFNumberMaxType = 16
};

typedef const struct CF_BRIDGED_TYPE(NSNumber) __CFNumber *CFNumberRef;

CF_EXPORT const CFNumberRef kCFNumberPositiveInfinity;
CF_EXPORT const CFNumberRef kCFNumberNegativeInfinity;
CF_EXPORT const CFNumberRef kCFNumberNaN;

CF_EXPORT CFTypeID CFNumberGetTypeID(void);
CF_EXPORT CFNumberRef CFNumberCreate(CFAllocatorRef allocator, CFNumberType theType, const void *valuePtr);
CF_EXPORT CFNumberType CFNumberGetType(CFNumberRef number);
CF_EXPORT CFIndex CFNumberGetByteSize(CFNumberRef number);
CF_EXPORT Boolean CFNumberIsFloatType(CFNumberRef number);
CF_EXPORT Boolean CFNumberGetValue(CFNumberRef number, CFNumberType theType, void *valuePtr);
CF_EXPORT CFComparisonResult CFNumberCompare(CFNumberRef number, CFNumberRef otherNumber, void *context);

CF_EXTERN_C_END
