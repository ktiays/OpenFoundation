//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/Locale/OCFLocale.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

// String formatting of the iOS Simulator CoreFoundation image (24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC): the format parser and formatter (__CFStringAppendFormatCore), format
// validation, localized numbers, the replacement metadata and the public
// CFStringCreateWithFormat/CFStringAppendFormat entry points.

// Constant-string records for initializers at file scope (OF_STR is an expression): an ASCII literal (info
// 0x7c8) or a UTF-16 literal (info 0x7d0), as clang emits a CFSTR literal with non-ASCII characters. OF_USTR
// is the UTF-16 counterpart of OF_STR.
#define ConstantASCIIString(literal)                                                                         \
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "" literal, sizeof(literal) - 1}
#define ConstantUTF16String(literal)                                                                         \
    {(uintptr_t)__OCFConstantStringClassReference, 0x7d0, u"" literal, sizeof(u"" literal) / 2 - 1}
#define OF_USTR(literal)                                                                                     \
    ({                                                                                                       \
        static const OCFConstantString constantString = ConstantUTF16String(literal);                        \
        (OCFStringRef) & constantString;                                                                     \
    })

// The 32-byte format-specification record the parser fills (iOS Simulator 24A434; confidence: verified for
// every offset and width the restored functions use; bytes 29..31 are unread). The shared FormatSpec in
// OCFString.h spells the same record with bytes 4..23 and 27..31 unread; FormatSpecRecord is a local
// descriptive spelling of the complete record. Types: 0x20 literal text, 0x21 integer, 0x22 floating point,
// 0x23 pointer, 0x25 object, 0x26 UTF-16 string, 0x27 C string, 0x28 Pascal string, 0x29 UTF-16 character,
// 0x2a %n, 0x2b an invalid or truncated specification, 0x2c a plural rule without a value type. Sizes: 1
// char, 2 short, 3 int (default), 4 long or pointer, 5 long double. Flags: 1 zero padding, 2 left
// justification, 4 plus sign, 8 space, 0x10 a %#@key@ or %[attribute]@ configuration, 0x20 localized number,
// 0x40 an attribute, 0x80 an unknown conversion kept as text. The argument numbers take -1 (none or main
// argument to assign) and -2 (a '*' width or precision to assign). numericStyle: 1 decimal, 2 scientific, 3
// general, 4 unsigned.
typedef struct {
    int16_t size;
    int16_t type;
    int32_t loc;
    int32_t len;
    int32_t widthArg;
    int32_t precArg;
    uint32_t flags;
    int8_t mainArgNum;
    int8_t precArgNum;
    int8_t widthArgNum;
    int8_t configDictIndex;
    uint8_t numericStyle;
    uint8_t unread[3];
} FormatSpecRecord;

_Static_assert(sizeof(FormatSpecRecord) == 32, "A format-specification record is 32 bytes.");
_Static_assert(sizeof(FormatSpecRecord) == sizeof(FormatSpec), "Both spellings describe one record.");

// The 16-byte argument value the formatter collects: the type and size of the specification that consumes it
// and the value (an integer, a double or a pointer). PrintValue is a local descriptive spelling.
typedef struct {
    int16_t type;
    int16_t size;
    uint32_t unread;

    union {
        int64_t int64Value;
        double doubleValue;
        const void *pointerValue;
    } value;
} PrintValue;

_Static_assert(sizeof(PrintValue) == 16, "A collected argument value is 16 bytes.");

typedef OCFStringRef (*CopyDescriptionFunction)(void *, const void *);
typedef OCFStringRef (*ContextDescriptionFunction)(void *, const void *, const void *, bool, bool *);

// The object-format-argument call-outs (non-external, defined in the CFString unit beside the string object
// functions, called by the formatter). ___CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT_WITH_CONTEXT__
// (0x1c02253b0) moves X1 and X2 to X0 and X1, loads "%@NSCONTEXT" into X2 and calls X0 with X3 and X4 live:
// the formatter's two call sites set W3 (0 or 1) and X4 (the deferral flag or 0) for the context function.
// ..._WITH_LOCALE__ (0x1c02253dc) calls X0 with X1 and X2.
OF_EXTERN OCFStringRef __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT_WITH_CONTEXT__(
    ContextDescriptionFunction function,
    const void *object,
    const void *context,
    bool replacing,
    bool *isDeferred
);
OF_EXTERN OCFStringRef __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT_WITH_LOCALE__(
    CopyDescriptionFunction function, const void *object, const void *formatOptions
);
OF_EXTERN OCFStringRef __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT__(const void *object);

// Isolate and statistical writing-direction helpers of the formatter (non-external natively, in the CFString
// unit's formatting range, 0x1c0225410-0x1c0225d90). specs points at the formatter's 32-byte specifier
// records (the location and length as 32-bit integers at offsets 4 and 8); the index is read as 32 bits
// (SXTW).
OF_EXTERN bool __CFShouldUseIsolatesForAppendingValue(
    OCFStringRef value,
    OCFMutableStringRef outputString,
    OCFIndex lastAppendedLength,
    const void *specs,
    OCFIndex specIndex,
    OCFIndex formatLength,
    OCFStringRef formatString
);
OF_EXTERN bool __CFShouldUseIsolatesForAppendingValueWithStatisticalWritingDirections(
    OCFStringRef value,
    OCFMutableStringRef outputString,
    bool *localeDirectionComputed,
    bool *localeIsRightToLeft,
    bool isDirectionalMark,
    OCFIndex lastAppendedLength,
    const void *specs,
    OCFIndex specIndex,
    OCFIndex formatLength,
    OCFStringRef formatString,
    OCFTypeRef locale
);
// direction 0 appends U+2066, 1 U+2067 and any other value U+2068.
OF_EXTERN void __CFAppendDirectionalIsolate(int32_t direction, OCFMutableStringRef outputString);

// Interfaces of families not yet restored on this branch
// SDK CFLocale.h (CFLocaleLanguageDirection is a CFIndex enumeration; kCFLocaleLanguageDirectionRightToLeft
// is 2). Defined by the CFLocale unit.

// Hidden (non-external natively; its callers include CFString units outside this family).
OF_EXTERN OCFStringRef __CFStringCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions);

OF_EXTERN bool __CFStringAppendFormatCore(
    OCFMutableStringRef outputString,
    CopyDescriptionFunction copyDescFunc,
    ContextDescriptionFunction contextDescFunc,
    OCFTypeRef formatOptions,
    OCFDictionaryRef formatConfiguration,
    OCFTypeRef implicitArgument,
    OCFStringRef validFormatSpecifiers,
    OCFStringRef formatString,
    OCFIndex initialArgPosition,
    OCFIndex nestingDepth,
    const PrintValue *originalValues,
    OCFIndex originalValuesSize,
    va_list args,
    OCFArrayRef *outReplacementMetadata,
    OCFErrorRef *errorPtr
);

OF_EXTERN OCFIndex __CFStringValidateFormat(
    OCFStringRef expected, OCFStringRef format, OCFIndex initialArgPosition, OCFErrorRef *errorPtr
);

// Public (CFString.h).
OF_EXPORT OCFStringRef OCFStringCreateWithFormatAndArguments(
    OCFAllocatorRef alloc, OCFDictionaryRef formatOptions, OCFStringRef format, va_list arguments
);
OF_EXPORT void OCFStringAppendFormat(
    OCFMutableStringRef theString, OCFDictionaryRef formatOptions, OCFStringRef format, ...
);
OF_EXPORT void OCFStringAppendFormatAndArguments(
    OCFMutableStringRef theString, OCFDictionaryRef formatOptions, OCFStringRef format, va_list arguments
);

// External SPI.
OF_EXPORT OCFStringRef OCFStringCreateStringWithValidatedFormat(
    OCFAllocatorRef alloc,
    OCFDictionaryRef formatOptions,
    OCFStringRef validFormatSpecifiers,
    OCFStringRef format,
    OCFErrorRef *errorPtr,
    ...
);
OF_EXPORT OCFStringRef _OCFStringCreateStringWithValidatedFormatAuxWithDesc(
    OCFAllocatorRef alloc,
    CopyDescriptionFunction copyDescFunc,
    ContextDescriptionFunction contextDescFunc,
    OCFDictionaryRef formatOptions,
    OCFTypeRef implicitArgument,
    OCFStringRef validFormatSpecifiers,
    OCFStringRef format,
    va_list arguments,
    OCFErrorRef *errorPtr
);
OF_EXPORT void _OCFStringAppendFormatAndArgumentsAux2(
    OCFMutableStringRef outputString,
    CopyDescriptionFunction copyDescFunc,
    ContextDescriptionFunction contextDescFunc,
    OCFDictionaryRef formatOptions,
    OCFTypeRef implicitArgument,
    OCFStringRef formatString,
    va_list arguments
);
OF_EXPORT OCFStringRef _OCFStringCreateWithFormatAndArgumentsAux2(
    OCFAllocatorRef alloc,
    CopyDescriptionFunction copyDescFunc,
    ContextDescriptionFunction contextDescFunc,
    OCFDictionaryRef formatOptions,
    OCFTypeRef implicitArgument,
    OCFStringRef format,
    va_list arguments
);
OF_EXPORT OCFStringRef _OCFStringCreateWithFormatAndArgumentsReturningMetadata(
    OCFAllocatorRef alloc,
    CopyDescriptionFunction copyDescFunc,
    ContextDescriptionFunction contextDescFunc,
    OCFDictionaryRef formatOptions,
    OCFDictionaryRef formatConfiguration,
    OCFTypeRef implicitArgument,
    OCFStringRef format,
    OCFArrayRef *outReplacementMetadata,
    va_list arguments
);
OF_EXPORT int32_t __OCFStringFindFormatSpecifiersInString(
    const uint8_t *cformat,
    const UniChar *uformat,
    OCFIndex formatLength,
    FormatSpecRecord *specs,
    OCFStringRef *configs,
    OCFIndex *configsCount
);

// The stringsdict keys of this unit (external; native kCFStringVariableWidthRuleType,
// kCFStringDeviceSpecificRuleType and kCFStringLocalizedFormatKey, __DATA_CONST,__const 0x2227aa268,
// 0x2227aa270 and 0x2227aa280).
OF_EXPORT const OCFStringRef kOCFStringVariableWidthRuleType;
OF_EXPORT const OCFStringRef kOCFStringDeviceSpecificRuleType;
OF_EXPORT const OCFStringRef kOCFStringLocalizedFormatKey;

OF_EXPORT const OCFStringRef _kOCFStringFormatMetadataReplacementIndexKey;
OF_EXPORT const OCFStringRef _kOCFStringFormatMetadataSpecifierRangeLocationInFormatStringKey;
OF_EXPORT const OCFStringRef _kOCFStringFormatMetadataSpecifierRangeLengthInFormatStringKey;
OF_EXPORT const OCFStringRef _kOCFStringFormatMetadataReplacementRangeLocationKey;
OF_EXPORT const OCFStringRef _kOCFStringFormatMetadataReplacementRangeLengthKey;
OF_EXPORT const OCFStringRef _kOCFStringFormatMetadataArgumentObjectKey;
OF_EXPORT const OCFStringRef _kOCFStringFormatMetadataAddedIsolatesKey;
