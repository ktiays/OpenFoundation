//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <stdint.h>

// Signed 128-bit value as emit128 and its tables read it, iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified: the signed high word at offset 0,
// the unsigned low word at offset 8. Int128Parts and its field names are local descriptive spellings.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
typedef struct {
    int64_t high;
    uint64_t low;
} Int128Parts;

// Original non-external name is preserved; no process-shared symbol is exported.
OF_EXTERN void emit128(char *buffer, const Int128Parts *value, uint8_t forcePlus);

// CFNumber.h (iOS Simulator SDK interface), spelled OCF.
typedef const struct __OCFBoolean *OCFBooleanRef;

OF_EXPORT const OCFBooleanRef kOCFBooleanTrue;
OF_EXPORT const OCFBooleanRef kOCFBooleanFalse;

OF_EXPORT OCFTypeID OCFBooleanGetTypeID(void);
OF_EXPORT Boolean OCFBooleanGetValue(OCFBooleanRef boolean);

typedef enum : OCFIndex {
    kOCFNumberSInt8Type = 1,
    kOCFNumberSInt16Type = 2,
    kOCFNumberSInt32Type = 3,
    kOCFNumberSInt64Type = 4,
    kOCFNumberFloat32Type = 5,
    kOCFNumberFloat64Type = 6,
    kOCFNumberCharType = 7,
    kOCFNumberShortType = 8,
    kOCFNumberIntType = 9,
    kOCFNumberLongType = 10,
    kOCFNumberLongLongType = 11,
    kOCFNumberFloatType = 12,
    kOCFNumberDoubleType = 13,
    kOCFNumberCFIndexType = 14,
    kOCFNumberNSIntegerType = 15,
    kOCFNumberCGFloatType = 16,
    kOCFNumberMaxType = 16,
} OCFNumberType;

typedef const struct __OCFNumber *OCFNumberRef;

OF_EXPORT const OCFNumberRef kOCFNumberPositiveInfinity;
OF_EXPORT const OCFNumberRef kOCFNumberNegativeInfinity;
OF_EXPORT const OCFNumberRef kOCFNumberNaN;

OF_EXPORT OCFTypeID OCFNumberGetTypeID(void);
OF_EXPORT OCFNumberRef
OCFNumberCreate(OCFAllocatorRef allocator, OCFNumberType theType, const void *valuePtr);
OF_EXPORT OCFNumberType OCFNumberGetType(OCFNumberRef number);
OF_EXPORT OCFIndex OCFNumberGetByteSize(OCFNumberRef number);
OF_EXPORT Boolean OCFNumberIsFloatType(OCFNumberRef number);
OF_EXPORT Boolean OCFNumberGetValue(OCFNumberRef number, OCFNumberType theType, void *valuePtr);
OF_EXPORT OCFComparisonResult OCFNumberCompare(OCFNumberRef number, OCFNumberRef otherNumber, void *context);

// Exported by the reference image; not part of the SDK interface. The canonical type without the 128-bit
// mapping CFNumberGetType applies (17 stays 17).
OF_EXPORT OCFNumberType _OCFNumberGetType2(OCFNumberRef number);

// The static booleans (external natively as __kCFBooleanTrue and __kCFBooleanFalse).
OF_EXPORT const OCFRuntimeBase __kOCFBooleanTrue;
OF_EXPORT const OCFRuntimeBase __kOCFBooleanFalse;

// The CFBoolean (type 21) and CFNumber (type 22) class records, for the builtin class table.
OF_EXTERN const OCFRuntimeClass __CFBooleanClass;
OF_EXTERN const OCFRuntimeClass __CFNumberClass;

// The CFBoolean formatting-description callback (was private external), iOS Simulator 24A434, 0x1c01abab8.
// The other CFBoolean callbacks are non-external and static in Number/OCFNumber.c.
OF_EXTERN OCFStringRef __CFBooleanCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions);

// Original non-external names are preserved (the hash and description callbacks are also reached from the
// runtime for tagged numbers).
OF_EXTERN OCFHashCode __CFNumberHash(OCFTypeRef cf);
OF_EXTERN OCFStringRef __CFNumberCopyDescription(OCFTypeRef cf);
OF_EXTERN OCFStringRef __CFNumberCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions);
OF_EXTERN OCFStringRef __CFNumberCopyFormattingDescriptionAsFloat64(OCFTypeRef cf);
// Native __CFNumberCreateFormattingDescription, 0x1c01abcc8 (was a private external); signature from the
// register use (the formatting options in X2 are not read).
OF_EXTERN OCFStringRef
__CFNumberCreateFormattingDescription(OCFAllocatorRef allocator, OCFTypeRef cf, const void *formatOptions);
