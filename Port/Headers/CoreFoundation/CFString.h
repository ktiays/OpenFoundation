//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFCharacterSet.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFLocale.h>
#include <stdarg.h>

CF_EXTERN_C_BEGIN

typedef UInt32 CFStringEncoding;

typedef CF_ENUM(CFStringEncoding, CFStringBuiltInEncodings) {
    kCFStringEncodingMacRoman = 0,
    kCFStringEncodingWindowsLatin1 = 0x0500,
    kCFStringEncodingISOLatin1 = 0x0201,
    kCFStringEncodingNextStepLatin = 0x0B01,
    kCFStringEncodingASCII = 0x0600,
    kCFStringEncodingUnicode = 0x0100,
    kCFStringEncodingUTF8 = 0x08000100,
    kCFStringEncodingNonLossyASCII = 0x0BFF,
    kCFStringEncodingUTF16 = 0x0100,
    kCFStringEncodingUTF16BE = 0x10000100,
    kCFStringEncodingUTF16LE = 0x14000100,
    kCFStringEncodingUTF32 = 0x0c000100,
    kCFStringEncodingUTF32BE = 0x18000100,
    kCFStringEncodingUTF32LE = 0x1c000100
};

#define kCFStringEncodingInvalidId (0xffffffffU)

// Constant strings: objc4's clang lays out the records as the restored CoreFoundation reads them
// (the class reference, the 64-bit info word at offset 8, the bytes and the length).
#if __has_builtin(__builtin___CFStringMakeConstantString)
    #define CFSTR(cStr) ((CFStringRef)__builtin___CFStringMakeConstantString("" cStr ""))
#else
    #error "CFSTR needs the compiler's constant CFString builtin"
#endif

typedef CF_OPTIONS(CFOptionFlags, CFStringCompareFlags) {
    kCFCompareCaseInsensitive = 1,
    kCFCompareBackwards = 4,
    kCFCompareAnchored = 8,
    kCFCompareNonliteral = 16,
    kCFCompareLocalized = 32,
    kCFCompareNumerically = 64,
    kCFCompareDiacriticInsensitive = 128,
    kCFCompareWidthInsensitive = 256,
    kCFCompareForcedOrdering = 512
};

CF_EXPORT CFTypeID CFStringGetTypeID(void);

CF_EXPORT CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding);
CF_EXPORT CFStringRef CFStringCreateWithBytes(
    CFAllocatorRef alloc,
    const UInt8 *bytes,
    CFIndex numBytes,
    CFStringEncoding encoding,
    Boolean isExternalRepresentation
);
CF_EXPORT CFStringRef CFStringCreateWithCharacters(CFAllocatorRef alloc, const UniChar *chars, CFIndex numChars);
CF_EXPORT CFStringRef CFStringCreateWithCStringNoCopy(
    CFAllocatorRef alloc,
    const char *cStr,
    CFStringEncoding encoding,
    CFAllocatorRef contentsDeallocator
);
CF_EXPORT CFStringRef CFStringCreateWithBytesNoCopy(
    CFAllocatorRef alloc,
    const UInt8 *bytes,
    CFIndex numBytes,
    CFStringEncoding encoding,
    Boolean isExternalRepresentation,
    CFAllocatorRef contentsDeallocator
);
CF_EXPORT CFStringRef CFStringCreateWithCharactersNoCopy(
    CFAllocatorRef alloc,
    const UniChar *chars,
    CFIndex numChars,
    CFAllocatorRef contentsDeallocator
);
CF_EXPORT CFStringRef CFStringCreateWithSubstring(CFAllocatorRef alloc, CFStringRef str, CFRange range);
CF_EXPORT CFStringRef CFStringCreateCopy(CFAllocatorRef alloc, CFStringRef theString);
CF_EXPORT CFStringRef CFStringCreateWithFormat(
    CFAllocatorRef alloc,
    CFDictionaryRef formatOptions,
    CFStringRef format,
    ...
) CF_FORMAT_FUNCTION(3, 4);
CF_EXPORT CFStringRef CFStringCreateWithFormatAndArguments(
    CFAllocatorRef alloc,
    CFDictionaryRef formatOptions,
    CFStringRef format,
    va_list arguments
) CF_FORMAT_FUNCTION(3, 0);
CF_EXPORT CFMutableStringRef CFStringCreateMutable(CFAllocatorRef alloc, CFIndex maxLength);
CF_EXPORT CFMutableStringRef CFStringCreateMutableCopy(CFAllocatorRef alloc, CFIndex maxLength, CFStringRef theString);

CF_EXPORT CFIndex CFStringGetLength(CFStringRef theString);
CF_EXPORT UniChar CFStringGetCharacterAtIndex(CFStringRef theString, CFIndex idx);
CF_EXPORT void CFStringGetCharacters(CFStringRef theString, CFRange range, UniChar *buffer);
CF_EXPORT Boolean CFStringGetCString(
    CFStringRef theString,
    char *buffer,
    CFIndex bufferSize,
    CFStringEncoding encoding
);
CF_EXPORT const char *CFStringGetCStringPtr(CFStringRef theString, CFStringEncoding encoding);
CF_EXPORT const UniChar *CFStringGetCharactersPtr(CFStringRef theString);
CF_EXPORT CFIndex CFStringGetBytes(
    CFStringRef theString,
    CFRange range,
    CFStringEncoding encoding,
    UInt8 lossByte,
    Boolean isExternalRepresentation,
    UInt8 *buffer,
    CFIndex maxBufLen,
    CFIndex *usedBufLen
);
CF_EXPORT CFStringRef CFStringCreateFromExternalRepresentation(
    CFAllocatorRef alloc,
    CFDataRef data,
    CFStringEncoding encoding
);
CF_EXPORT CFDataRef CFStringCreateExternalRepresentation(
    CFAllocatorRef alloc,
    CFStringRef theString,
    CFStringEncoding encoding,
    UInt8 lossByte
);
CF_EXPORT CFStringEncoding CFStringGetSmallestEncoding(CFStringRef theString);
CF_EXPORT CFStringEncoding CFStringGetFastestEncoding(CFStringRef theString);
CF_EXPORT CFStringEncoding CFStringGetSystemEncoding(void);
CF_EXPORT CFIndex CFStringGetMaximumSizeForEncoding(CFIndex length, CFStringEncoding encoding);
CF_EXPORT Boolean CFStringGetFileSystemRepresentation(CFStringRef string, char *buffer, CFIndex maxBufLen);
CF_EXPORT CFIndex CFStringGetMaximumSizeOfFileSystemRepresentation(CFStringRef string);
CF_EXPORT CFStringRef CFStringCreateWithFileSystemRepresentation(CFAllocatorRef alloc, const char *buffer);

CF_EXPORT CFComparisonResult CFStringCompareWithOptionsAndLocale(
    CFStringRef theString1,
    CFStringRef theString2,
    CFRange rangeToCompare,
    CFStringCompareFlags compareOptions,
    CFLocaleRef locale
);
CF_EXPORT CFComparisonResult CFStringCompareWithOptions(
    CFStringRef theString1,
    CFStringRef theString2,
    CFRange rangeToCompare,
    CFStringCompareFlags compareOptions
);
CF_EXPORT CFComparisonResult CFStringCompare(
    CFStringRef theString1,
    CFStringRef theString2,
    CFStringCompareFlags compareOptions
);
CF_EXPORT Boolean CFStringFindWithOptionsAndLocale(
    CFStringRef theString,
    CFStringRef stringToFind,
    CFRange rangeToSearch,
    CFStringCompareFlags searchOptions,
    CFLocaleRef locale,
    CFRange *result
);
CF_EXPORT Boolean CFStringFindWithOptions(
    CFStringRef theString,
    CFStringRef stringToFind,
    CFRange rangeToSearch,
    CFStringCompareFlags searchOptions,
    CFRange *result
);
CF_EXPORT CFArrayRef CFStringCreateArrayWithFindResults(
    CFAllocatorRef alloc,
    CFStringRef theString,
    CFStringRef stringToFind,
    CFRange rangeToSearch,
    CFStringCompareFlags compareOptions
);
CF_EXPORT CFRange CFStringFind(CFStringRef theString, CFStringRef stringToFind, CFStringCompareFlags compareOptions);
CF_EXPORT Boolean CFStringHasPrefix(CFStringRef theString, CFStringRef prefix);
CF_EXPORT Boolean CFStringHasSuffix(CFStringRef theString, CFStringRef suffix);
CF_EXPORT CFRange CFStringGetRangeOfComposedCharactersAtIndex(CFStringRef theString, CFIndex theIndex);
CF_EXPORT Boolean CFStringFindCharacterFromSet(
    CFStringRef theString,
    CFCharacterSetRef theSet,
    CFRange rangeToSearch,
    CFStringCompareFlags searchOptions,
    CFRange *result
);
CF_EXPORT void CFStringGetLineBounds(
    CFStringRef theString,
    CFRange range,
    CFIndex *lineBeginIndex,
    CFIndex *lineEndIndex,
    CFIndex *contentsEndIndex
);
CF_EXPORT CFStringRef CFStringCreateByCombiningStrings(
    CFAllocatorRef alloc,
    CFArrayRef theArray,
    CFStringRef separatorString
);
CF_EXPORT CFArrayRef CFStringCreateArrayBySeparatingStrings(
    CFAllocatorRef alloc,
    CFStringRef theString,
    CFStringRef separatorString
);
CF_EXPORT SInt32 CFStringGetIntValue(CFStringRef str);
CF_EXPORT double CFStringGetDoubleValue(CFStringRef str);

CF_EXPORT void CFStringAppend(CFMutableStringRef theString, CFStringRef appendedString);
CF_EXPORT void CFStringAppendCharacters(CFMutableStringRef theString, const UniChar *chars, CFIndex numChars);
CF_EXPORT void CFStringAppendCString(CFMutableStringRef theString, const char *cStr, CFStringEncoding encoding);
CF_EXPORT void CFStringAppendFormat(
    CFMutableStringRef theString,
    CFDictionaryRef formatOptions,
    CFStringRef format,
    ...
) CF_FORMAT_FUNCTION(3, 4);
CF_EXPORT void CFStringAppendFormatAndArguments(
    CFMutableStringRef theString,
    CFDictionaryRef formatOptions,
    CFStringRef format,
    va_list arguments
) CF_FORMAT_FUNCTION(3, 0);
CF_EXPORT void CFStringInsert(CFMutableStringRef str, CFIndex idx, CFStringRef insertedStr);
CF_EXPORT void CFStringDelete(CFMutableStringRef theString, CFRange range);
CF_EXPORT void CFStringReplace(CFMutableStringRef theString, CFRange range, CFStringRef replacement);
CF_EXPORT void CFStringReplaceAll(CFMutableStringRef theString, CFStringRef replacement);
CF_EXPORT CFIndex CFStringFindAndReplace(
    CFMutableStringRef theString,
    CFStringRef stringToFind,
    CFStringRef replacementString,
    CFRange rangeToSearch,
    CFStringCompareFlags compareOptions
);
CF_EXPORT void CFStringPad(
    CFMutableStringRef theString,
    CFStringRef padString,
    CFIndex length,
    CFIndex indexIntoPad
);
CF_EXPORT void CFStringLowercase(CFMutableStringRef theString, CFLocaleRef locale);
CF_EXPORT void CFStringUppercase(CFMutableStringRef theString, CFLocaleRef locale);
CF_EXPORT void CFStringCapitalize(CFMutableStringRef theString, CFLocaleRef locale);

CF_EXPORT Boolean CFStringIsEncodingAvailable(CFStringEncoding encoding);
CF_EXPORT CFStringRef CFStringGetNameOfEncoding(CFStringEncoding encoding);
CF_EXPORT unsigned long CFStringConvertEncodingToNSStringEncoding(CFStringEncoding encoding);
CF_EXPORT CFStringEncoding CFStringConvertNSStringEncodingToEncoding(unsigned long encoding);
CF_EXPORT CFStringRef CFStringConvertEncodingToIANACharSetName(CFStringEncoding encoding);
CF_EXPORT CFStringEncoding CFStringConvertIANACharSetNameToEncoding(CFStringRef theString);

CF_EXTERN_C_END
