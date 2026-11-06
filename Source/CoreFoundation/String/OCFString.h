//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Locale/OCFLocale.h"

#include <stdbool.h>
#include <stdint.h>

// Format-specification record as __CFStringFindTypeAndSizeForArgumentIndex and
// __CFStringValidateFormatSpecBuffers read it, iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for the offsets, widths and the
// 32-byte stride (ADD X8, X8, #0x20 per record); unread for bytes 4..23 and 27..31, which neither
// function touches. The size at offset 0 and the type at offset 2 are copied as 16-bit values (LDURH,
// STRH) and compared as their low bytes (LDURB). The three signed argument numbers at offsets 24..26
// take -1 (main) or -2 (width, precision) for "assign the next sequential argument"; the width,
// precision and main roles of offsets 26, 25 and 24 are inferred from that consumption order.
// FormatSpec and its field names are local descriptive spellings; the binary carries no type name.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
typedef struct {
    int16_t size;
    int16_t type;
    uint8_t unread0[20];
    int8_t mainArgNum;
    int8_t precArgNum;
    int8_t widthArgNum;
    uint8_t unread1[5];
} FormatSpec;

// Original non-external names are preserved; no process-shared symbol is exported.
OF_EXTERN bool __CFStringFindTypeAndSizeForArgumentIndex(
    const FormatSpec *specs, int32_t count, OCFIndex argumentIndex, int16_t *type, int16_t *size
);

OF_EXTERN bool __CFStringValidateFormatSpecBuffers(
    const FormatSpec *specs,
    int32_t count,
    const FormatSpec *expectedSpecs,
    int32_t expectedCount,
    OCFIndex nextArgumentIndex
);

OF_EXTERN bool __rangeEqual(const void *first, const void *second);

// A compiled constant string, as clang emits CFSTR literals into __DATA,__cfstring: the constant-string
// class, 0x7c8 (ASCII) or 0x7d0 (UTF-16), the bytes and the length (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified on the image's 2,376 literals).
// OCFConstantString is a local descriptive spelling. The restored sources build their constant strings with
// OF_STR so that they bind to the restored constant-string class in both naming modes.
typedef struct {
    uintptr_t isa;
    uint64_t info;
    const void *data;
    OCFIndex length;
} OCFConstantString;

_Static_assert(sizeof(OCFConstantString) == 32, "A compiled constant string is 32 bytes.");

// The constant-string class object (an Objective-C class; the isa of every constant string).
OF_EXPORT char __OCFConstantStringClassReference[];

#define OF_STR(literal)                                                                                      \
    ({                                                                                                       \
        static const OCFConstantString constantString = {                                                    \
            (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "" literal, sizeof(literal) - 1             \
        };                                                                                                   \
        (OCFStringRef) & constantString;                                                                     \
    })

OF_EXPORT OCFStringRef
OCFStringCreateWithFormat(OCFAllocatorRef alloc, OCFDictionaryRef formatOptions, OCFStringRef format, ...);

typedef UInt32 OCFStringEncoding;

// The built-in encodings the restored units name (CFStringBuiltInEncodings, SDK CFString.h values).
enum {
    kOCFStringEncodingISOLatin1 = 0x0201,
    kOCFStringEncodingASCII = 0x0600,
    kOCFStringEncodingUTF8 = 0x08000100,
};

OF_EXPORT OCFStringRef
OCFStringCreateWithCString(OCFAllocatorRef alloc, const char *cStr, OCFStringEncoding encoding);
OF_EXPORT OCFStringRef OCFStringCreateCopy(OCFAllocatorRef alloc, OCFStringRef theString);
OF_EXPORT void _OCFStringGetUserDefaultEncoding(uint32_t *scriptValue, uint32_t *regionValue);
OF_EXTERN void __CFStringInitializeSystemEncoding(void);
OF_EXTERN Boolean __NSInlineTaggedPointerStringIsEqualToTaggedPointer(OCFTypeRef string, OCFTypeRef other);
OF_EXTERN Boolean __NSIndirectTaggedPointerStringIsEqualToTaggedPointer(OCFTypeRef string, OCFTypeRef other);

// -----------------------------------------------------------------------------------------------------------
// CFString core family (cf-core string-core sub-track). Declarations below this line are added by the family;
// the declarations above are the coordinator's.
// Layouts read from the iOS Simulator image (24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC); confidence:
// verified for every offset and bit named here (CFStringGetLength, __CFStringCreateImmutableFunnel3,
// CFStringCreateMutable, __CFStringDeallocate, __CFStrAllocateMutableContents). Type, field and helper names
// are local descriptive spellings unless the SDK declares them.

#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <os/log.h>
#include <stddef.h>

typedef OCFOptionFlags OCFStringCompareFlags;

enum {
    kOCFCompareCaseInsensitive = 1,
    kOCFCompareBackwards = 4,
    kOCFCompareAnchored = 8,
    kOCFCompareNonliteral = 16,
    kOCFCompareLocalized = 32,
    kOCFCompareNumerically = 64,
    kOCFCompareDiacriticInsensitive = 128,
    kOCFCompareWidthInsensitive = 256,
    kOCFCompareForcedOrdering = 512,
};

// CFStringInlineBuffer (SDK layout, 0xb8 bytes). Units outside CFString compile the SDK's inline accessors
// (stringInitInlineBuffer and stringGetCharacterFromInlineBuffer are their local spellings; confirmed in
// __CFStringContainsNullCharacter and __CFStringEncodeByteStream, which call CFStringGetCharactersPtr and
// then CFStringGetCStringPtr(str, kCFStringEncodingASCII)). Inside the CFString unit the initialization calls
// _CFStringGetCStringPtrInternal(str, kCFStringEncodingASCII, false, true) instead (initInlineBuffer there).
typedef struct {
    UniChar buffer[64];
    OCFStringRef theString;
    const UniChar *directUniCharBuffer;
    const char *directCStringBuffer;
    OCFRange rangeToBuffer;
    OCFIndex bufferedRangeStart;
    OCFIndex bufferedRangeEnd;
} OCFStringInlineBuffer;

_Static_assert(sizeof(OCFStringInlineBuffer) == 0xb8, "The inline buffer is 184 bytes.");

// The variable-width character buffer of the byte-stream decoder: characters at 0, isASCII at 8,
// shouldFreeChars at 9, the allocator at 0x10, the character count at 0x18 and a 1,008-byte local buffer at
// 0x20 (the ASCII local-buffer limit is 0x3f0; __CFStringDecodeByteStream3,
// __CFStringCreateImmutableFunnel3).
typedef struct {
    union {
        uint8_t *ascii;
        UniChar *unicode;
    } chars;

    Boolean isASCII;
    Boolean shouldFreeChars;
    OCFAllocatorRef allocator;
    OCFIndex numChars;
    uint8_t localBuffer[1008];
} VarWidthCharBuffer;

_Static_assert(offsetof(VarWidthCharBuffer, allocator) == 0x10, "The allocator is at 0x10.");
_Static_assert(sizeof(VarWidthCharBuffer) == 0x410, "The variable-width buffer is 1,040 bytes.");

// The CFString object. Info bits 0-6: 0x1 mutable, 0x4 length byte, 0x8 null byte, 0x10 Unicode (UTF-16
// contents; clear: eight-bit contents in __CFDefaultEightBitStringEncoding), 0x60 the contents kind (0
// inline, 0x20 freed with the object's allocator, 0x40 not freed, 0x60 freed with the stored contents
// deallocator).
enum {
    kStringInfoMutable = 0x1,
    kStringInfoLengthByte = 0x4,
    kStringInfoNullByte = 0x8,
    kStringInfoUnicode = 0x10,
    kStringInfoContentsMask = 0x60,
    kStringInfoContentsFreeWithObjectAllocator = 0x20,
    kStringInfoContentsNotFreed = 0x40,
    kStringInfoContentsCustomFree = 0x60,
};

// The mutable variant's bits word (offset 0x28): bit 0 has gap (unused here), bit 1 fixed capacity, bit 2
// external mutable, bit 3 capacity provided externally, bits 4-63 the desired capacity.
enum {
    kStringMutableFixedCapacity = 0x2,
    kStringMutableExternal = 0x4,
    kStringMutableCapacityProvidedExternally = 0x8,
    kStringMutableDesiredCapacityShift = 4,
};

struct __OCFString {
    OCFRuntimeBase base;

    union {
        // Inline contents: an explicit length at 0x10 (unless the string has a length byte), then the bytes.
        struct {
            OCFIndex length;
        } inline1;

        // External immutable contents with an explicit length; the contents deallocator follows (0x20).
        struct {
            void *buffer;
            OCFIndex length;
            OCFAllocatorRef contentsDeallocator;
        } notInlineImmutable1;

        // External immutable contents with a length byte; the contents deallocator is at 0x18.
        struct {
            void *buffer;
            OCFAllocatorRef contentsDeallocator;
        } notInlineImmutable2;

        struct {
            void *buffer;
            OCFIndex length;
            OCFIndex capacity;
            uint64_t bits;
            OCFAllocatorRef contentsAllocator;
        } notInlineMutable;
    } variants;
};

_Static_assert(
    offsetof(struct __OCFString, variants.notInlineMutable.contentsAllocator) == 0x30,
    "The mutable contents allocator is at 0x30."
);

// Every native accessor reads the info word with LDAR; each helper performs one such load.
static inline uint64_t stringInfo(OCFStringRef string) {
    return __atomic_load_n(&string->base.info, __ATOMIC_ACQUIRE);
}

// Native flag updates load the info word (LDAR) and retry a compare-and-swap of (info & ~mask) | value.
static inline void stringSetInfoBits(OCFStringRef string, uint64_t mask, uint64_t value) {
    uint64_t *info = (uint64_t *)&string->base.info;
    uint64_t current = __atomic_load_n(info, __ATOMIC_ACQUIRE);
    while (!__atomic_compare_exchange_n(
        info, &current, (current & ~mask) | value, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    )) {
    }
}

static inline bool stringIsMutable(OCFStringRef string) {
    return (stringInfo(string) & kStringInfoMutable) != 0;
}

static inline bool stringIsEightBit(OCFStringRef string) {
    return (stringInfo(string) & kStringInfoUnicode) == 0;
}

static inline bool stringIsUnicode(OCFStringRef string) {
    return (stringInfo(string) & kStringInfoUnicode) != 0;
}

static inline bool stringIsInline(OCFStringRef string) {
    return (stringInfo(string) & kStringInfoContentsMask) == 0;
}

static inline bool stringHasNullByte(OCFStringRef string) {
    return (stringInfo(string) & kStringInfoNullByte) != 0;
}

// A length byte is used only by immutable strings: (info & 5) == 4.
static inline bool stringHasLengthByte(OCFStringRef string) {
    return (stringInfo(string) & (kStringInfoMutable | kStringInfoLengthByte)) == kStringInfoLengthByte;
}

static inline bool stringHasExplicitLength(OCFStringRef string) {
    return (stringInfo(string) & (kStringInfoMutable | kStringInfoLengthByte)) != kStringInfoLengthByte;
}

// Inline contents start at 0x10, after the explicit length when there is one (+8); external contents are the
// pointer at 0x10. The length byte, when present, is the first content byte.
static inline const void *stringContents(OCFStringRef string) {
    if ((stringInfo(string) & kStringInfoContentsMask) != 0) {
        return string->variants.notInlineImmutable1.buffer;
    }
    return (const uint8_t *)&string->variants + (stringHasExplicitLength(string) ? sizeof(OCFIndex) : 0);
}

// The length: the length byte when there is one, else 0x18 for external contents and 0x10 for inline ones.
static inline OCFIndex stringLength(OCFStringRef string) {
    if (stringHasLengthByte(string)) {
        return *(const uint8_t *)stringContents(string);
    }
    if ((stringInfo(string) & kStringInfoContentsMask) != 0) {
        return string->variants.notInlineImmutable1.length;
    }
    return string->variants.inline1.length;
}

// The stored contents deallocator of a 0x60 string: 0x18 with a length byte, else 0x20.
static inline OCFAllocatorRef *stringContentsDeallocatorSlot(OCFStringRef string) {
    uintptr_t offset = stringHasLengthByte(string) ? 0x18 : 0x20;
    return (OCFAllocatorRef *)((uintptr_t)string + offset);
}

// The object's allocator as the native code inlines __CFGetAllocator: the system default for a tagged pointer
// or an object whose info bit 7 is set, else the allocator stored 16 bytes before the object.
static inline OCFAllocatorRef stringObjectAllocator(OCFStringRef string) {
    if ((intptr_t)string < 0) {
        return kOCFAllocatorSystemDefault;
    }
    if ((int8_t)stringInfo(string) < 0) {
        return kOCFAllocatorSystemDefault;
    }
    return *(const OCFAllocatorRef *)((uintptr_t)string - 16);
}

// The allocator of mutable contents: the stored contents allocator (0x30) for an external mutable string
// (contents kind 0x60), else the object's allocator.
static inline OCFAllocatorRef stringMutableContentsAllocator(OCFStringRef string) {
    if ((~stringInfo(string) & kStringInfoContentsMask) == 0) {
        return string->variants.notInlineMutable.contentsAllocator;
    }
    return stringObjectAllocator(string);
}

// The tag-index search the native string functions inline for a tagged pointer: the obfuscator is skipped for
// pointers with bits 63, 62 and 0-2 set; the low three bits of the decoded value are looked up in the first
// seven entries of objc_debug_tag60_permutations. 2 is an inline tagged string (NSTaggedPointerString), 0 an
// indirect tagged string; 7 means not found (an extended tag).
extern uintptr_t objc_debug_taggedpointer_obfuscator;
extern const uint8_t objc_debug_tag60_permutations[8];

enum {
    kStringTagIndirect = 0,
    kStringTagInline = 2,
    kStringTagNotFound = 7,
};

static inline unsigned stringTagIndex(OCFTypeRef cf) {
    uintptr_t pointer = (uintptr_t)cf;
    uintptr_t obfuscator = (~pointer & 0xC000000000000007) == 0 ? 0 : objc_debug_taggedpointer_obfuscator;
    uintptr_t value = (obfuscator ^ pointer) & 7;
    unsigned index = 0;
    while (index != 7 && value != objc_debug_tag60_permutations[index]) {
        ++index;
    }
    return index;
}

static inline bool stringIsTagged(OCFTypeRef cf) {
    return (intptr_t)cf < 0;
}

// Public CFString interface (SDK CFString.h signatures, OCF spellings).
OF_EXPORT const OCFStringRef kOCFEmptyString;

OF_EXPORT OCFTypeID OCFStringGetTypeID(void);
OF_EXPORT OCFStringEncoding OCFStringGetSystemEncoding(void);
// Native __CFStringMtbl, 0x1c02086bc (external): the mutable bit.
OF_EXPORT Boolean __OCFStringMtbl(OCFStringRef string);
OF_EXPORT OCFIndex OCFStringGetMaximumSizeForEncoding(OCFIndex length, OCFStringEncoding encoding);
OF_EXPORT OCFStringRef OCFStringCreateWithCStringNoCopy(
    OCFAllocatorRef alloc, const char *cStr, OCFStringEncoding encoding, OCFAllocatorRef contentsDeallocator
);
OF_EXPORT OCFStringRef
OCFStringCreateWithCharacters(OCFAllocatorRef alloc, const UniChar *chars, OCFIndex numChars);
OF_EXPORT OCFStringRef OCFStringCreateWithCharactersNoCopy(
    OCFAllocatorRef alloc, const UniChar *chars, OCFIndex numChars, OCFAllocatorRef contentsDeallocator
);
OF_EXPORT OCFStringRef OCFStringCreateWithBytes(
    OCFAllocatorRef alloc,
    const UInt8 *bytes,
    OCFIndex numBytes,
    OCFStringEncoding encoding,
    Boolean isExternalRepresentation
);
OF_EXPORT OCFStringRef OCFStringCreateWithBytesNoCopy(
    OCFAllocatorRef alloc,
    const UInt8 *bytes,
    OCFIndex numBytes,
    OCFStringEncoding encoding,
    Boolean isExternalRepresentation,
    OCFAllocatorRef contentsDeallocator
);
OF_EXPORT OCFMutableStringRef OCFStringCreateMutable(OCFAllocatorRef alloc, OCFIndex maxLength);
OF_EXPORT OCFStringRef OCFStringCreateWithSubstring(OCFAllocatorRef alloc, OCFStringRef str, OCFRange range);
OF_EXPORT const char *OCFStringGetCStringPtr(OCFStringRef theString, OCFStringEncoding encoding);
OF_EXPORT OCFIndex OCFStringGetLength(OCFStringRef theString);
OF_EXPORT OCFMutableStringRef OCFStringCreateMutableWithExternalCharactersNoCopy(
    OCFAllocatorRef alloc,
    UniChar *chars,
    OCFIndex numChars,
    OCFIndex capacity,
    OCFAllocatorRef externalCharactersAllocator
);
OF_EXPORT void OCFStringSetExternalCharactersNoCopy(
    OCFMutableStringRef theString, UniChar *chars, OCFIndex length, OCFIndex capacity
);
OF_EXPORT OCFMutableStringRef
OCFStringCreateMutableCopy(OCFAllocatorRef alloc, OCFIndex maxLength, OCFStringRef theString);
OF_EXPORT UniChar OCFStringGetCharacterAtIndex(OCFStringRef theString, OCFIndex idx);
OF_EXPORT void OCFStringGetCharacters(OCFStringRef theString, OCFRange range, UniChar *buffer);
OF_EXPORT OCFIndex OCFStringGetBytes(
    OCFStringRef theString,
    OCFRange range,
    OCFStringEncoding encoding,
    UInt8 lossByte,
    Boolean isExternalRepresentation,
    UInt8 *buffer,
    OCFIndex maxBufLen,
    OCFIndex *usedBufLen
);
OF_EXPORT const unsigned char *
OCFStringGetPascalStringPtr(OCFStringRef theString, OCFStringEncoding encoding);
OF_EXPORT const UniChar *OCFStringGetCharactersPtr(OCFStringRef theString);
OF_EXPORT Boolean
OCFStringGetCString(OCFStringRef theString, char *buffer, OCFIndex bufferSize, OCFStringEncoding encoding);
OF_EXPORT OCFComparisonResult OCFStringCompareWithOptionsAndLocale(
    OCFStringRef theString1,
    OCFStringRef theString2,
    OCFRange rangeToCompare,
    OCFStringCompareFlags compareOptions,
    OCFLocaleRef locale
);
OF_EXPORT OCFComparisonResult OCFStringCompareWithOptions(
    OCFStringRef theString1,
    OCFStringRef theString2,
    OCFRange rangeToCompare,
    OCFStringCompareFlags compareOptions
);
OF_EXPORT OCFComparisonResult
OCFStringCompare(OCFStringRef theString1, OCFStringRef theString2, OCFStringCompareFlags compareOptions);
OF_EXPORT Boolean OCFStringFindWithOptionsAndLocale(
    OCFStringRef theString,
    OCFStringRef stringToFind,
    OCFRange rangeToSearch,
    OCFStringCompareFlags searchOptions,
    OCFLocaleRef locale,
    OCFRange *result
);
OF_EXPORT Boolean OCFStringFindWithOptions(
    OCFStringRef theString,
    OCFStringRef stringToFind,
    OCFRange rangeToSearch,
    OCFStringCompareFlags searchOptions,
    OCFRange *result
);
OF_EXPORT OCFArrayRef OCFStringCreateArrayWithFindResults(
    OCFAllocatorRef alloc,
    OCFStringRef theString,
    OCFStringRef stringToFind,
    OCFRange rangeToSearch,
    OCFStringCompareFlags compareOptions
);
OF_EXPORT OCFRange
OCFStringFind(OCFStringRef theString, OCFStringRef stringToFind, OCFStringCompareFlags compareOptions);
OF_EXPORT Boolean OCFStringHasPrefix(OCFStringRef theString, OCFStringRef prefix);
OF_EXPORT Boolean OCFStringHasSuffix(OCFStringRef theString, OCFStringRef suffix);
OF_EXPORT OCFRange OCFStringGetRangeOfComposedCharactersAtIndex(OCFStringRef theString, OCFIndex theIndex);
OF_EXPORT Boolean OCFStringFindCharacterFromSet(
    OCFStringRef theString,
    OCFCharacterSetRef theSet,
    OCFRange rangeToSearch,
    OCFStringCompareFlags searchOptions,
    OCFRange *result
);
OF_EXPORT OCFStringRef
OCFStringCreateByCombiningStrings(OCFAllocatorRef alloc, OCFArrayRef theArray, OCFStringRef separatorString);
OF_EXPORT OCFArrayRef OCFStringCreateArrayBySeparatingStrings(
    OCFAllocatorRef alloc, OCFStringRef theString, OCFStringRef separatorString
);
OF_EXPORT OCFStringRef
OCFStringCreateFromExternalRepresentation(OCFAllocatorRef alloc, OCFDataRef data, OCFStringEncoding encoding);
OF_EXPORT OCFStringRef _OCFStringCreateWithNullTerminatedBytesNoCopy(
    OCFAllocatorRef alloc,
    const uint8_t *bytes,
    OCFIndex numBytes,
    OCFStringEncoding encoding,
    OCFAllocatorRef contentsDeallocator
);
OF_EXPORT OCFDataRef OCFStringCreateExternalRepresentation(
    OCFAllocatorRef alloc, OCFStringRef theString, OCFStringEncoding encoding, UInt8 lossByte
);
OF_EXPORT OCFStringEncoding OCFStringGetFastestEncoding(OCFStringRef theString);
OF_EXPORT SInt32 OCFStringGetIntValue(OCFStringRef str);
OF_EXPORT double OCFStringGetDoubleValue(OCFStringRef str);
OF_EXPORT void OCFStringInsert(OCFMutableStringRef str, OCFIndex idx, OCFStringRef insertedStr);
OF_EXPORT void OCFStringDelete(OCFMutableStringRef theString, OCFRange range);
OF_EXPORT void OCFStringReplace(OCFMutableStringRef theString, OCFRange range, OCFStringRef replacement);
OF_EXPORT void OCFStringReplaceAll(OCFMutableStringRef theString, OCFStringRef replacement);
OF_EXPORT void OCFStringAppend(OCFMutableStringRef theString, OCFStringRef appendedString);
OF_EXPORT void
OCFStringAppendCharacters(OCFMutableStringRef theString, const UniChar *chars, OCFIndex numChars);
OF_EXPORT void
OCFStringAppendCString(OCFMutableStringRef theString, const char *cStr, OCFStringEncoding encoding);
OF_EXPORT OCFIndex OCFStringFindAndReplace(
    OCFMutableStringRef theString,
    OCFStringRef stringToFind,
    OCFStringRef replacementString,
    OCFRange rangeToSearch,
    OCFStringCompareFlags compareOptions
);
OF_EXPORT void
OCFStringPad(OCFMutableStringRef theString, OCFStringRef padString, OCFIndex length, OCFIndex indexIntoPad);
OF_EXPORT void OCFStringLowercase(OCFMutableStringRef theString, OCFLocaleRef locale);
OF_EXPORT void OCFStringUppercase(OCFMutableStringRef theString, OCFLocaleRef locale);
OF_EXPORT void OCFStringCapitalize(OCFMutableStringRef theString, OCFLocaleRef locale);
OF_EXPORT Boolean OCFStringGetFileSystemRepresentation(OCFStringRef string, char *buffer, OCFIndex maxBufLen);
OF_EXPORT OCFStringRef OCFStringCreateWithFileSystemRepresentation(OCFAllocatorRef alloc, const char *buffer);
OF_EXPORT OCFIndex OCFStringGetMaximumSizeOfFileSystemRepresentation(OCFStringRef string);
OF_EXPORT OCFStringEncoding OCFStringConvertIANACharSetNameToEncoding(OCFStringRef theString);
OF_EXPORT Boolean OCFStringIsEncodingAvailable(OCFStringEncoding encoding);
OF_EXPORT OCFStringRef OCFStringGetNameOfEncoding(OCFStringEncoding encoding);
OF_EXPORT OCFStringRef OCFStringConvertEncodingToIANACharSetName(OCFStringEncoding encoding);
OF_EXPORT OCFStringEncoding OCFStringGetSmallestEncoding(OCFStringRef theString);
OF_EXPORT void OCFStringGetLineBounds(
    OCFStringRef theString,
    OCFRange range,
    OCFIndex *lineBeginIndex,
    OCFIndex *lineEndIndex,
    OCFIndex *contentsEndIndex
);

// Exported names without an SDK declaration (prefix rule spellings). Signatures are read from the assembly by
// the defining unit.
OF_EXPORT OCFStringEncoding OCFStringFileSystemEncoding(void);
OF_EXPORT OCFStringEncoding __OCFStringComputeEightBitStringEncoding(void);
OF_EXPORT OCFHashCode OCFStringHashISOLatin1CString(const uint8_t *bytes, OCFIndex length);
OF_EXPORT OCFHashCode OCFStringHashCString(const UInt8 *bytes, OCFIndex len);
OF_EXPORT OCFHashCode OCFStringHashNSString(OCFStringRef string);
OF_EXPORT OCFHashCode __OCFStringHash(OCFTypeRef cf);
OF_EXPORT OCFIndex _OCFStringGetLength2(OCFStringRef string);
OF_EXPORT const char *_OCFStringGetCStringPtrWithOptions(
    OCFStringRef string, OCFStringEncoding encoding, Boolean requiresNullTermination, Boolean requiresBMP
);
OF_EXPORT OCFRange
OCFStringGetRangeOfCharacterClusterAtIndex(OCFStringRef string, OCFIndex charIndex, OCFIndex type);
OF_EXPORT void __OCFStringAppendBytes(
    OCFMutableStringRef string, const char *cStr, OCFIndex appendedLength, OCFStringEncoding encoding
);
OF_EXPORT void __OCFStrConvertBytesToUnicode(const UInt8 *bytes, UniChar *buffer, OCFIndex numChars);
// The checked entry points of the bridged string methods (__CFStringIsEightBit 0x1c02087a4,
// _CFStringCheckAndGetCharacterAtIndex 0x1c020f844, _CFStringCheckAndGetCharacters 0x1c020fa2c,
// __CFStringCheckAndReplace 0x1c021d564; the range arguments are passed as location and length, the
// registers of a CFRange). They answer 0 on success, 1 for an immutable string, 2 for a NULL replacement and
// 3 for a range out of bounds.
OF_EXPORT Boolean __OCFStringIsEightBit(OCFStringRef string);
OF_EXPORT int _OCFStringCheckAndGetCharacterAtIndex(OCFStringRef string, OCFIndex index, UniChar *character);
OF_EXPORT int
_OCFStringCheckAndGetCharacters(OCFStringRef string, OCFIndex location, OCFIndex length, UniChar *buffer);
OF_EXPORT int __OCFStringCheckAndReplace(
    OCFMutableStringRef string, OCFIndex location, OCFIndex length, OCFStringRef replacement
);
OF_EXPORT Boolean __OCFStringDecodeByteStream3(
    const UInt8 *bytes,
    OCFIndex len,
    OCFStringEncoding encoding,
    Boolean alwaysUnicode,
    VarWidthCharBuffer *buffer,
    Boolean *useClientsMemoryPtr,
    UInt32 converterFlags
);
OF_EXPORT OCFIndex __OCFStringEncodeByteStream(
    OCFStringRef string,
    OCFIndex rangeLoc,
    OCFIndex rangeLen,
    Boolean generatingExternalFile,
    OCFStringEncoding encoding,
    UInt8 lossByte,
    UInt8 *buffer,
    OCFIndex max,
    OCFIndex *usedBufLen
);
OF_EXPORT OCFIndex _ONSTaggedPointerStringGetLength(OCFStringRef string);
OF_EXPORT OCFIndex _ONSTaggedPointerStringGetBytes(OCFStringRef string, UInt8 *buffer);
OF_EXPORT const UInt8 *_OCFIndirectTaggedPointerStringGetContents(OCFStringRef string, uint16_t *length);
OF_EXPORT OCFStringRef _OCFStringCreateIndirectASCIITaggedPointerString(const UInt8 *bytes, OCFIndex length);
OF_EXPORT OCFStringRef _OCFStringCreateWithASCIIBytesNoCopyOrIndirectTagged(
    OCFAllocatorRef alloc, const UInt8 *bytes, OCFIndex length
);
OF_EXPORT OCFStringRef
_OCFStringCreateWithASCIICStringNoCopyOrIndirectTagged(OCFAllocatorRef alloc, const char *cString);
// The bundle parameter is spelled with its structure tag (OCFBundleRef, PlugIn/OCFBundle.h): the bundle
// header includes the URL header, which includes this one.
struct __OCFBundle;

OF_EXPORT OCFStringRef _OCFStringCreateByParsingMarkdownAndInflectingIfNeeded(
    OCFStringRef string, struct __OCFBundle *bundle, OCFStringRef localization
);

// The eight-bit encoding of non-Unicode contents (0xffffffff until computed); external natively as
// __CFDefaultEightBitStringEncoding.
OF_EXPORT OCFStringEncoding __OCFDefaultEightBitStringEncoding;

// Hidden (natively private external) names of the family.
OF_EXTERN const OCFRuntimeClass __CFStringClass;
OF_EXTERN uintptr_t __CFTaggedStringClass;
OF_EXTERN const UniChar *__CFCharToUniCharTable;
// The identity table __CFCharToUniCharTable initially points to (non-external natively; read by two units).
OF_EXTERN UniChar __CFMutableCharToUniCharTable[256];
OF_EXTERN OCFStringRef __CFStringCreateImmutableFunnel3(
    OCFAllocatorRef alloc,
    const void *bytes,
    OCFIndex numBytes,
    OCFStringEncoding encoding,
    uint8_t options,
    OCFAllocatorRef contentsDeallocator,
    UInt32 converterFlags
);
OF_EXTERN const char *_CFStringGetUTF8PtrWithOptions(
    OCFStringRef string, OCFIndex *utf8Length, Boolean requiresNullTermination, Boolean requiresBMP
);
OF_EXTERN OCFStringRef _CFStringCreateWithBytesNoCopyDisallowROM(
    OCFAllocatorRef alloc,
    const UInt8 *bytes,
    OCFIndex numBytes,
    OCFStringEncoding encoding,
    Boolean isExternalRepresentation,
    OCFAllocatorRef contentsDeallocator
);
OF_EXTERN OCFStringRef _CFStringCopyBundleUnloadingProtectedString(OCFStringRef string);
OF_EXTERN OCFArrayRef CFStringCopyStatisticalWritingDirections(OCFStringRef string);
// Takes no argument (X0 is never read) and never returns (it raises through __CFRaiseMemoryException), but
// the native callers keep their code after the call, as for a returning function.
OF_EXTERN void __CFStringHandleOutOfMemory(void);
OF_EXTERN OCFComparisonResult _CFCompareStringsWithLocale(
    OCFStringInlineBuffer *str1,
    OCFRange str1Range,
    OCFStringInlineBuffer *str2,
    OCFRange str2Range,
    OCFOptionFlags options,
    const void *compareLocale
);
OF_EXTERN Boolean __CFStringScanInteger(
    OCFStringInlineBuffer *buf, OCFTypeRef locale, SInt32 *indexPtr, Boolean doLonglong, void *result
);
OF_EXTERN Boolean
__CFStringScanDouble(OCFStringInlineBuffer *buf, OCFTypeRef locale, SInt32 *indexPtr, double *result);

// The SDK inline-buffer accessors (CFStringInitInlineBuffer, CFStringGetCharacterFromInlineBuffer).
static inline void stringInitInlineBuffer(OCFStringRef str, OCFStringInlineBuffer *buf, OCFRange range) {
    buf->theString = str;
    buf->rangeToBuffer = range;
    buf->directCStringBuffer = (buf->directUniCharBuffer = OCFStringGetCharactersPtr(str))
                                   ? NULL
                                   : OCFStringGetCStringPtr(str, 0x600);
    buf->bufferedRangeStart = buf->bufferedRangeEnd = 0;
}

static inline UniChar stringGetCharacterFromInlineBuffer(OCFStringInlineBuffer *buf, OCFIndex idx) {
    if (idx < 0 || idx >= buf->rangeToBuffer.length) {
        return 0;
    }
    if (buf->directUniCharBuffer) {
        return buf->directUniCharBuffer[idx + buf->rangeToBuffer.location];
    }
    if (buf->directCStringBuffer) {
        return (UniChar)(buf->directCStringBuffer[idx + buf->rangeToBuffer.location]);
    }
    if (idx >= buf->bufferedRangeEnd || idx < buf->bufferedRangeStart) {
        if ((buf->bufferedRangeStart = idx - 4) < 0) {
            buf->bufferedRangeStart = 0;
        }
        buf->bufferedRangeEnd = buf->bufferedRangeStart + 64;
        if (buf->bufferedRangeEnd > buf->rangeToBuffer.length) {
            buf->bufferedRangeEnd = buf->rangeToBuffer.length;
        }
        OCFRange range = {
            buf->rangeToBuffer.location + buf->bufferedRangeStart,
            buf->bufferedRangeEnd - buf->bufferedRangeStart
        };
        OCFStringGetCharacters(buf->theString, range, buf->buffer);
    }
    return buf->buffer[idx - buf->bufferedRangeStart];
}

// Native names of the CFString unit that its units call across files (non-external or was a private external
// natively); the ones not defined on this branch belong to other work lists of the family.
OF_EXTERN OCFStringRef _CFNonObjCStringCreateCopy(OCFAllocatorRef alloc, OCFStringRef string);
// Native _CFNonObjCStringGetCStringPtr, 0x1c0210350. Hidden (was private external).
OF_EXTERN const char *
_CFNonObjCStringGetCStringPtr(OCFStringRef str, OCFStringEncoding encoding, Boolean requiresNullTermination);
// Native _CFStringGetCStringPtrInternal, 0x1c0210358. Non-external; called by the string-core and formatting
// units.
OF_EXTERN const char *_CFStringGetCStringPtrInternal(
    OCFStringRef str, OCFStringEncoding encoding, Boolean requiresNullTermination, Boolean requiresBMP
);
// Native __CFDefaultSystemEncoding, 0x22e6d303c (__DATA,__bss, one byte). Non-external; a one-byte "set" flag
// (STRB of 1) written by CFStringGetSystemEncoding and read by __CFStringAppendFormatCore.
OF_EXTERN bool __CFDefaultSystemEncoding;
// _objc_getTaggedPointerTag: libobjc's inline tag decoder as compiled into the native string units (native
// __objc_getTaggedPointerTag, 0x1c0223198 in CFString.c and 0x1c022e2ec in CFStringEncodings.c, both
// non-external); returns the tag in W0, extended tags as 8 + (bits 55-62) (UBFX, ADD), so an unsigned 32-bit
// value.
OF_EXTERN uint32_t _objc_getTaggedPointerTag(const void *ptr);
// Native _CFStringEqual, 0x1c0208a1c (was a private external): the class's equal callback for two CF strings.
OF_EXTERN Boolean _CFStringEqual(OCFStringRef first, OCFStringRef second);
// Native _NSTaggedPointerStringEqualCFString, 0x1c015a340 (String/OCFTaggedPointerString.c), and
// _NSIndirectTaggedPointerStringIsEqualToCFString, 0x1c0206398 (String/OCFIndirectTaggedPointerString.c);
// both were private externals.
OF_EXTERN Boolean _NSTaggedPointerStringEqualCFString(OCFTypeRef taggedString, OCFStringRef string);
OF_EXTERN Boolean
_NSIndirectTaggedPointerStringIsEqualToCFString(OCFTypeRef taggedString, OCFStringRef string);
// copyBlocks (0x1c0223ea8, non-external, CFString.c), as __CFStringChangeSizeMultiple calls it.
OF_EXTERN void copyBlocks(
    const uint8_t *srcStart,
    uint8_t *dstStart,
    OCFIndex srcLength,
    Boolean srcIsUnicode,
    Boolean dstIsUnicode,
    const OCFRange *ranges,
    OCFIndex numRanges,
    OCFIndex insertLength
);

// -----------------------------------------------------------------------------------------------------------
// Family helpers and declarations of the individual units.
// Native __CFStringIsCF, 0x1c02086cc (external): whether string is a CF string object (no message dispatch),
// with its mutability, length and eight-bit flag; *isConstant is set for a compiled constant string.
OF_EXPORT Boolean __OCFStringIsCF(
    OCFStringRef string, Boolean *isMutable, OCFIndex *length, Boolean *isEightBit, bool *isConstant
);

// The eight-bit encoding as every native caller reads it inline: __CFDefaultEightBitStringEncoding, computed
// by
// __CFStringComputeEightBitStringEncoding while it is still kCFStringEncodingInvalidId (CMN W0, #1).
static inline OCFStringEncoding stringEightBitEncoding(void) {
    OCFStringEncoding encoding = __OCFDefaultEightBitStringEncoding;
    if (encoding == 0xffffffff) {
        encoding = __OCFStringComputeEightBitStringEncoding();
    }
    return encoding;
}

// libobjc's tagged-pointer encoding as the native string code inlines it: value ^ obfuscator is returned
// unobfuscated (value itself) when its bits 63, 62 and 0-2 are all set; otherwise the low three bits of the
// obfuscated value are replaced by their objc_debug_tag60_permutations entry.
static inline uintptr_t stringEncodeTaggedPointer(uintptr_t value) {
    uintptr_t obfuscated = objc_debug_taggedpointer_obfuscator ^ value;
    if ((~obfuscated & 0xC000000000000007) == 0) {
        return value;
    }
    return (obfuscated & ~(uintptr_t)7) | objc_debug_tag60_permutations[obfuscated & 7];
}

// The encoding test the native accessors inline when the eight-bit encoding is ASCII: whether eight-bit ASCII
// contents can be returned as bytes of `encoding`, by the encoding's second byte. 0x00: not 1, 4, 5, 0x21,
// 0x22 (mask 0x600000032) or 0x98; 0x01: only 0x8000100; 0x02: not 0x206; 0x06: only 0x600; 0x08 and 0x0c:
// never; 0x0a: not 0xa01, 0xa05, 0xa10 (mask 0x8011 from 0xa01); 0x0b: not 0xbf3, 0xbf4, 0xbff (mask 0x1003
// from 0xbf3); any other second byte: the encoding & 0xff00 is at most 0xc00. CFStringGetBytes,
// CFStringGetCString and CFStringGetPascalStringPtr branch exactly so; _CFStringGetCStringPtrInternal and
// _CFStringGetUTF8PtrWithOptions compute the same sets with different instructions (0xb: CMP #0xbff and an
// unsigned (encoding - 0xbf5) < 0xfffffffe; default: (encoding & 0xff00) < 0xc01).
static inline bool stringEncodingAcceptsASCIIContents(OCFStringEncoding encoding) {
    uint32_t group = (encoding >> 8) & 0xFF;
    switch (group) {
        case 0:
            if (encoding <= 0x22 && ((UINT64_C(1) << encoding) & UINT64_C(0x600000032)) != 0) {
                return false;
            }
            return encoding != 0x98;
        case 1:
            return encoding == 0x8000100;
        case 2:
            return encoding != 0x206;
        case 6:
            return encoding == 0x600;
        case 8:
        case 12:
            return false;
        case 10:
            if (encoding - 0xA01 > 0xF) {
                return true;
            }
            return ((1u << (encoding - 0xA01)) & 0x8011) == 0;
        case 11:
            if (encoding - 0xBF3 > 0xC) {
                return true;
            }
            return ((1u << (encoding - 0xBF3)) & 0x1003) == 0;
        default:
            return (encoding & 0xFF00) <= 0xC00;
    }
}

// CFCharacterSetInlineBufferIsLongCharacterMember as the native string functions inline it (read in
// CFStringCompareWithOptionsAndLocale): flags bit
// 0 compact bitmap, bit 1 no bitmap (ask _CFCharacterSetIsLongCharacterMemberForInline), bit 2 inverted.
// Outside [rangeStart, rangeLimit) the answer is the inverted bit; a non-BMP character or flag bit 1 asks the
// character-set function; without a bitmap the answer is (not compact) xor inverted; a compact bitmap is
// indexed by the character's high byte (0 none, 0xff all, else the 32-byte page at 32 * index + 224).
static inline bool stringCharacterSetIsMember(OCFCharacterSetInlineBuffer *buffer, UTF32Char character) {
    bool inverted = (buffer->flags & 4) != 0;
    if (character < buffer->rangeStart || character >= buffer->rangeLimit) {
        return inverted;
    }
    if ((character >> 16) != 0 || (buffer->flags & 2) != 0) {
        return _OCFCharacterSetIsLongCharacterMemberForInline(buffer, character) != 0;
    }
    if (buffer->bitmap == NULL) {
        return ((buffer->flags & 1) == 0) != inverted;
    }
    bool member;
    if ((buffer->flags & 1) == 0) {
        member = ((buffer->bitmap[character >> 3] >> (character & 7)) & 1) != 0;
    } else {
        uint8_t page = buffer->bitmap[character >> 8];
        if (page == 0) {
            member = false;
        } else if (page == 0xFF) {
            member = true;
        } else {
            member =
                ((buffer->bitmap[32 * page + ((character & 0xFF) >> 3) + 224] >> (character & 7)) & 1) != 0;
        }
    }
    return member != inverted;
}

// The find-results array callbacks __rangeRetain (0x1c0218cf8), __rangeRelease (0x1c0218d20) and
// __rangeCopyDescription (0x1c0218d28) are static in String/OCFString.c (non-external natively);
// __rangeEqual is declared above.
// The RTL test of the string formatter: non-external in the CFString unit and called by the formatter's code
// there.
OF_EXTERN Boolean _CFStringHasStrongOrWeakRTL(OCFStringRef string, OCFIndex location, OCFIndex length);

// Native _CFBytesInASCII, 0x1c0208960. External.
OF_EXPORT Boolean _OCFBytesInASCII(const void *bytes, OCFIndex length);

// Native CFStringHashCharacters, 0x1c02094ac. External.
OF_EXPORT OCFHashCode OCFStringHashCharacters(const UniChar *characters, OCFIndex length);
