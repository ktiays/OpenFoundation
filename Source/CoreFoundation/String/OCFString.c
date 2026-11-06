//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFString.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/String/OCFIndirectTaggedPointerString.h"
#include "CoreFoundation/String/OCFStringByteStream.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/String/OCFStringFormatConfiguration.h"
#include "CoreFoundation/String/OCFTaggedPointerString.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingDatabase.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"
#include "CoreFoundation/PlugIn/OCFBundle.h"

#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <os/log.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unicode/uchar.h>

// Native __CFStringValidateFormatSpecBuffers, iOS Simulator 24A434, 0x1c02242fc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1806ba570, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// indexes the record by a counter instead of advancing a cursor and authenticates its return; the
// comparisons, calls and the 0x18800000000 type mask agree.
// For each record of specs, the width (offset 26) and precision (offset 25) argument numbers are
// clamped to -1 when negative, -2 takes the next sequential argument number, and the main argument
// number (offset 24) takes the next one when it is -1. nextArgumentIndex is the first sequential
// number; only its low byte reaches a comparison. A width or precision argument must be found in
// expectedSpecs with type 0x21 and a size at most 4. The main argument must be found; its type must
// equal the record's type or be one of the accepted pairs below, and the two sizes must agree after
// sizes up to 4 are raised to 4. Every comparison uses the low byte of the 16-bit size and type.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool __CFStringValidateFormatSpecBuffers(
    const FormatSpec *specs,
    int32_t count,
    const FormatSpec *expectedSpecs,
    int32_t expectedCount,
    OCFIndex nextArgumentIndex
) {
    for (int32_t index = 0; index < count; ++index) {
        const FormatSpec *spec = &specs[index];
        int8_t width = spec->widthArgNum > -1 ? spec->widthArgNum : -1;
        if (spec->widthArgNum == -2) {
            width = (int8_t)nextArgumentIndex;
            ++nextArgumentIndex;
        }
        int8_t precision = spec->precArgNum > -1 ? spec->precArgNum : -1;
        if (spec->precArgNum == -2) {
            precision = (int8_t)nextArgumentIndex;
            ++nextArgumentIndex;
        }
        int8_t argument = spec->mainArgNum;
        if (argument == -1) {
            argument = (int8_t)nextArgumentIndex;
            ++nextArgumentIndex;
        }
        // The native function clears one 32-bit stack word holding both results once per record.
        int16_t type = 0;
        int16_t size = 0;
        if (width != -1) {
            if (!__CFStringFindTypeAndSizeForArgumentIndex(
                    expectedSpecs, expectedCount, width, &type, &size
                )) {
                return false;
            }
            if ((uint8_t)type != 0x21 || (uint8_t)size > 4) {
                return false;
            }
        }
        if (precision != -1) {
            if (!__CFStringFindTypeAndSizeForArgumentIndex(
                    expectedSpecs, expectedCount, precision, &type, &size
                )) {
                return false;
            }
            if ((uint8_t)type != 0x21 || (uint8_t)size > 4) {
                return false;
            }
        }
        if (!__CFStringFindTypeAndSizeForArgumentIndex(
                expectedSpecs, expectedCount, argument, &type, &size
            )) {
            return false;
        }
        uint8_t specType = (uint8_t)spec->type;
        uint8_t foundType = (uint8_t)type;
        if (specType != foundType) {
            if (specType <= 0x26) {
                if (specType == 0x23) {
                    if ((unsigned)(foundType - 0x25) >= 4 && foundType != 0x21) {
                        return false;
                    }
                } else if (specType != 0x26 || foundType != 0x23) {
                    return false;
                }
            } else if (specType == 0x27 || specType == 0x28) {
                if (foundType > 0x28 || ((UINT64_C(1) << foundType) & UINT64_C(0x18800000000)) == 0) {
                    return false;
                }
            } else if (specType != 0x29 || foundType != 0x21) {
                return false;
            }
        }
        uint8_t specSize = (uint8_t)spec->size;
        uint8_t foundSize = (uint8_t)size;
        if (specSize <= 4) {
            specSize = 4;
        }
        if (foundSize <= 4) {
            foundSize = 4;
        }
        if (specSize != foundSize) {
            return false;
        }
    }
    return true;
}

// Native __CFStringFindTypeAndSizeForArgumentIndex, iOS Simulator 24A434, 0x1c022452c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1806d64a8, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// indexes records by a counter; the comparisons, sequential numbering and stores agree.
// The records are numbered as in __CFStringValidateFormatSpecBuffers from a 64-bit counter that starts
// at zero; each assigned number is the counter's low byte, sign-extended. argumentIndex is compared in
// 64 bits (CMP X2, Wn, SXTB). A width or precision match stores type 0x21 and size 3; a main-argument
// match copies the record's 16-bit type and then its size. The first matching record wins and the out
// parameters are untouched when none matches.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool __CFStringFindTypeAndSizeForArgumentIndex(
    const FormatSpec *specs, int32_t count, OCFIndex argumentIndex, int16_t *type, int16_t *size
) {
    OCFIndex nextArgumentIndex = 0;
    for (int32_t index = 0; index < count; ++index) {
        const FormatSpec *spec = &specs[index];
        int8_t width = spec->widthArgNum > -1 ? spec->widthArgNum : -1;
        if (spec->widthArgNum == -2) {
            width = (int8_t)nextArgumentIndex;
            ++nextArgumentIndex;
        }
        int8_t precision = spec->precArgNum > -1 ? spec->precArgNum : -1;
        if (spec->precArgNum == -2) {
            precision = (int8_t)nextArgumentIndex;
        }
        if (argumentIndex == width || argumentIndex == precision) {
            *type = 0x21;
            *size = 3;
            return true;
        }
        if (spec->precArgNum == -2) {
            ++nextArgumentIndex;
        }
        int8_t argument = spec->mainArgNum;
        if (argument == -1) {
            argument = (int8_t)nextArgumentIndex;
        }
        if (argumentIndex == argument) {
            *type = spec->type;
            *size = spec->size;
            return true;
        }
        if (spec->mainArgNum == -1) {
            ++nextArgumentIndex;
        }
    }
    return false;
}

// The find-results array callbacks (non-external natively), defined below in native order.
static const void *__rangeRetain(OCFAllocatorRef allocator, const void *value);
static void __rangeRelease(OCFAllocatorRef allocator, const void *value);
static OCFStringRef __rangeCopyDescription(const void *value);

// Native __rangeEqual, iOS Simulator 24A434, 0x1c0218d64, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; no direct caller (an address-taken callback beside __rangeRetain,
// __rangeRelease and __rangeCopyDescription).
// iOS Device 24A446: 0x180764320, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions.
// Compares two 16-byte ranges: both 64-bit words must be equal (LDP, CMP, CCMP, CSET).
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool __rangeEqual(const void *first, const void *second) {
    const OCFRange *firstRange = first;
    const OCFRange *secondRange = second;
    return firstRange->location == secondRange->location && firstRange->length == secondRange->length;
}

// Inferred organization: the CFString core functions from __CFStringInitializeSystemEncoding (0x1c02085f4)
// through _CFStringHasStrongOrWeakRTL (0x1c0225954), with their cold fragments (0x1c030c484-0x1c030ca4c) and
// __CFStringHandleOutOfMemory (0x1c030c440), are contiguous in the iOS Simulator __text section with the
// formatter and the other CFString functions of the image, and are kept in this translation unit in native
// address order; the native source path is not in the evidence. Functions of the same native unit restored by
// other work lists (__CFStringCopyFormattingDescription, the formatter) are declared in OCFString.h and
// defined elsewhere. Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read end to end; medium-confidence parts are named at their definitions); iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/CoreFoundationString.md.

static const char *
_CFStrGetSpecialCaseHandlingLanguageIdentifierForLocale(OCFLocaleRef locale, bool collatorOnly);
static Boolean __CFStringFillCharacterSetInlineBuffer(
    OCFCharacterSetInlineBuffer *buffer, OCFStringCompareFlags compareOptions
);
static OCFIndex __CFStringFoldCharacterClusterAtIndex(
    UTF32Char character,
    OCFStringInlineBuffer *buffer,
    OCFIndex index,
    OCFOptionFlags flags,
    const uint8_t *langCode,
    UTF32Char *outCharacters,
    OCFIndex *consumedLength,
    bool *insufficientBufferSpace
);
static UTF32Char __CFStringGetLongCharacterFromInlineBuffer(
    OCFStringInlineBuffer *buffer, OCFIndex length, OCFIndex idx, OCFRange *readRange
);
static void __CFStrDeallocateMutableContents(OCFMutableStringRef str, void *buffer);
static void *__CFStrAllocateMutableContents(OCFMutableStringRef str, size_t size);
static Boolean CFStrIsUnicode(OCFStringRef str);
static void __CFStringChangeSizeMultiple(
    OCFMutableStringRef str,
    const OCFRange *deleteRanges,
    OCFIndex numDeleteRanges,
    OCFIndex insertLength,
    Boolean makeUnicode
);

// libdyld (supplied): the SDK-version query, with the build-version pair in X0 as OCFBuildVersion.
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);
// libdyld's private immutable-memory query (supplied; no SDK declaration).
extern bool _dyld_is_memory_immutable(const void *address, size_t length);
static Boolean __CFStringEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static void __CFStringGetLineOrParagraphBounds(
    OCFStringRef string,
    OCFRange range,
    OCFIndex *lineBeginIndex,
    OCFIndex *lineEndIndex,
    OCFIndex *contentsEndIndex,
    Boolean includeLineEndings
);
static void __CFStringDeallocate(OCFTypeRef cf);
static OCFStringRef __CFStringCopyDescription(OCFTypeRef cf);
static OCFStringRef __CFStringSlowPathCopyBundleUnloadingProtectedString(OCFStringRef string);
// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call passes 0x10000.
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);
static OCFRange _CFStringInlineBufferGetComposedRange(
    OCFStringInlineBuffer *buffer, OCFIndex start, OCFIndex type, const uint8_t *bmpBitmap, uint32_t csetType
);
static bool _CFStringIsJoiningPulliAtIndex(
    OCFStringInlineBuffer *buffer, OCFIndex index, OCFIndex rangeLocation, OCFIndex rangeLength
);
static bool __CFStringIsRegionalIndicatorAtIndex(OCFStringInlineBuffer *buffer, OCFIndex index);

// The component record __CFStringGetExtendedPictographicSequenceComponent fills (five 64-bit words; callers
// initialize it to {-1, 0, -1, -1, -1}): the component's location and length, then the indexes of its
// extending character, its zero-width joiner and its pictograph (-1: none). Written only on success. The type
// and field names are local descriptive spellings.
typedef struct {
    OCFIndex location;
    OCFIndex length;
    OCFIndex extendIndex;
    OCFIndex joinerIndex;
    OCFIndex pictographIndex;
} PictographicSequenceComponent;

static bool __CFStringGetExtendedPictographicSequenceComponent(
    OCFStringInlineBuffer *buffer, OCFIndex length, OCFIndex index, PictographicSequenceComponent *component
);
// libicucore interfaces CFStringFindWithOptionsAndLocale calls (ICU C API; supplied, never reimplemented).
// The collator and string-search handles are opaque; UErrorCode, UCollationStrength and UScriptCode are
// 32-bit integers (a failure is a status above zero, as every native check reads it).
extern void *ucol_open(const char *loc, int32_t *status);
extern void ucol_close(void *coll);
extern void ucol_setStrength(void *coll, int32_t strength);
extern void *usearch_openFromCollator(
    const UniChar *pattern,
    int32_t patternLength,
    const UniChar *text,
    int32_t textLength,
    const void *collator,
    void *breakiter,
    int32_t *status
);
extern int32_t usearch_first(void *strsrch, int32_t *status);
extern int32_t usearch_last(void *strsrch, int32_t *status);
extern int32_t usearch_getMatchedLength(const void *strsrch);
extern void usearch_close(void *strsrch);
extern int8_t u_isdigit(int32_t c);
extern int32_t uscript_getScript(int32_t codepoint, int32_t *err);
// libsystem_platform's feature-flag query (private SPI, os/feature_private.h); supplied.
extern bool _os_feature_enabled_impl(const char *domain, const char *feature);
// ICU (libicucore), supplied: UCharCategory and UProperty are int-sized enumerations; UBool is int8_t.
extern int8_t u_hasBinaryProperty(int32_t c, int32_t which);

// Native __CFDefaultSystemEncoding, 0x22e6d303c (__DATA,__bss, one byte). Non-external. The system encoding
// is always kCFStringEncodingMacRoman (0) once set, so the native variable is a one-byte "set" flag (STRB of
// 1); it is also read by __CFStringAppendFormatCore (String/OCFStringFormat.c), hence hidden.
bool __CFDefaultSystemEncoding;

// Native __CFIsReadyToInitializeSystemEncoding, 0x22e6d3038, and __CFStringIsBeingInitialized, 0x22e6d303d
// (__DATA,__bss, one byte each). Non-external.
static bool __CFIsReadyToInitializeSystemEncoding;

static bool __CFStringIsBeingInitialized;

// Native __CFDefaultFileSystemEncoding, 0x22e6d3040 (__DATA,__bss, one byte). Non-external. Like the system
// encoding, a one-byte "set" flag: the file-system encoding is always kCFStringEncodingUTF8.
static bool __CFDefaultFileSystemEncoding;

// Native __CFDefaultEightBitStringEncoding, 0x22e6cf9b8 (__DATA,__data). External; initially
// kCFStringEncodingInvalidId.
OCFStringEncoding __OCFDefaultEightBitStringEncoding = 0xffffffff;

// Native __CFMutableCharToUniCharTable, 0x22e6cf9c8 (__DATA,__data, 256 UniChars: the identity). Non-external
// natively; hidden here because __CFSetCharToUniCharFunc (another unit) rewrites it.
UniChar __CFMutableCharToUniCharTable[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c,
    0x000d, 0x000e, 0x000f, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019,
    0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026,
    0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032, 0x0033,
    0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 0x0040,
    0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d,
    0x004e, 0x004f, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a,
    0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074,
    0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 0x0080, 0x0081,
    0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e,
    0x008f, 0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009a, 0x009b,
    0x009c, 0x009d, 0x009e, 0x009f, 0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8,
    0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af, 0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5,
    0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf, 0x00c0, 0x00c1, 0x00c2,
    0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc,
    0x00dd, 0x00de, 0x00df, 0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9,
    0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6,
    0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff,
};

// Native __CFCharToUniCharTable, 0x22e6cfbc8 (__DATA,__data). Hidden (was private external). Initially
// __CFMutableCharToUniCharTable.
const UniChar *__CFCharToUniCharTable = __CFMutableCharToUniCharTable;

// Native __CFTaggedStringClass, 0x22e6d4250 (__DATA,__common). Hidden (was private external). Nonzero once
// inline tagged strings may be created; no writer is in the evidence (an Objective-C unit outside this
// family).
uintptr_t __CFTaggedStringClass;

// Native charToSixBitLookup, 0x1c0385982 (__TEXT,__const, 256 bytes; this unit's copy, byte-identical to the
// indirect-tagged unit's). Non-external.
static const uint8_t charToSixBitLookup[256] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x2e, 0x07, 0x3c,
    0x1d, 0x1e, 0x28, 0x1f, 0x1c, 0x2b, 0x2c, 0x30, 0x2a, 0x31, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x32, 0x2f, 0x1b, 0x21, 0x35, 0x33, 0x3a, 0x3d, 0x0d, 0x3b, 0x34, 0x27, 0x13, 0x22, 0x29,
    0x2d, 0xff, 0x18, 0x14, 0x17, 0x26, 0x37, 0x36, 0x3f, 0x3e, 0xff, 0xff, 0xff, 0xff, 0xff, 0x38,
    0xff, 0x08, 0x20, 0x0e, 0x0a, 0x00, 0x11, 0x1a, 0x15, 0x01, 0x16, 0x12, 0x02, 0x06, 0x0b, 0x03,
    0x09, 0xff, 0x05, 0x0c, 0x04, 0x10, 0x23, 0x24, 0x19, 0x25, 0x39, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

// The compiled constant "" at 0x2227ac180 (__DATA,__cfstring: the constant-string class, 0x7c8, "", 0), which
// __CFStringCreateImmutableFunnel3 and the bundle-unloading copy reference directly.
static const OCFConstantString emptyString = {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "", 0};

// Native kCFEmptyString, 0x2227ac180's pointer at 0x2227a52f8 (__DATA_CONST,__const). External.
const OCFStringRef kOCFEmptyString = (OCFStringRef)&emptyString;

// Native __CFStringClass, 0x2227a5300 (__DATA_CONST,__const). Hidden (was private external). Version 1, type
// 7 in the builtin table; copy, formatting description (_CFNonObjCStringCreateCopy and
// __CFStringCopyFormattingDescription), finalize, equal, hash and debug description; no init, reclaim or
// reference-count callbacks.
const OCFRuntimeClass __CFStringClass = {
    1,
    "CFString",
    NULL,
    (OCFTypeRef (*)(OCFAllocatorRef, OCFTypeRef))(void *)_CFNonObjCStringCreateCopy,
    __CFStringDeallocate,
    __CFStringEqual,
    __OCFStringHash,
    (OCFStringRef (*)(OCFTypeRef, const void *))(void *)__CFStringCopyFormattingDescription,
    __CFStringCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __ASCII_LOWERCASE_TABLE, 0x1c0385a82 (__TEXT,__const, 128 bytes). Non-external. Bytes 0x00-0x7f
// with 'A'-'Z' mapped to 'a'-'z'; read only by CFStringCompareWithOptionsAndLocale's eight-bit ASCII path.
static const uint8_t __ASCII_LOWERCASE_TABLE[128] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
};

// Whether the character extends a grapheme cluster for the emoji rules: the zero-width joiner, ICU's
// Grapheme_Extend or Emoji_Modifier, or the grapheme cluster break SpacingMark (10), tested in that order.
static inline bool isClusterExtendingCharacter(UTF32Char character) {
    return character == 0x200D || u_hasBinaryProperty((UChar32)character, UCHAR_GRAPHEME_EXTEND) ||
           u_hasBinaryProperty((UChar32)character, UCHAR_EMOJI_MODIFIER) ||
           u_getIntPropertyValue((UChar32)character, UCHAR_GRAPHEME_CLUSTER_BREAK) == U_GCB_SPACING_MARK;
}

// The transcoding-hint lengths, indexed by character - 0xF860.
// Native __CFTranscodingHintLength, 0x1c0385972 (__TEXT,__const, 16 bytes). Non-external.
static const uint8_t __CFTranscodingHintLength[16] = {2, 3, 4, 4, 4, 4, 4, 2, 2, 2, 2, 4, 0, 0, 0, 0};

// One find result as CFStringCreateArrayWithFindResults stores it in its CFData: the range and the data that
// holds it (24 bytes, the array value __rangeRetain retains through offset 0x10). FindResult is a local
// descriptive spelling.
typedef struct {
    OCFRange range;
    OCFDataRef data;
} FindResult;

// The array callbacks of the find results (SDK CFArrayCallBacks layout). RangeArrayCallBacks is a local
// descriptive spelling.
typedef struct {
    OCFIndex version;
    const void *(*retain)(OCFAllocatorRef allocator, const void *value);
    void (*release)(OCFAllocatorRef allocator, const void *value);
    OCFStringRef (*copyDescription)(const void *value);
    bool (*equal)(const void *value1, const void *value2);
} RangeArrayCallBacks;

// Native anonymous constant, 0x2227a5390 (__DATA_CONST,__const, 40 bytes): version 0, __rangeRetain,
// __rangeRelease, __rangeCopyDescription, __rangeEqual. Non-external.
static const RangeArrayCallBacks kFindResultsArrayCallBacks = {
    0,
    __rangeRetain,
    __rangeRelease,
    __rangeCopyDescription,
    __rangeEqual,
};

// Native sUseOuterDirectionalOverride, 0x22e6cf9bc (__DATA,__data, one byte, 1). Non-external; read with
// LDARB.
static uint8_t sUseOuterDirectionalOverride = 1;

// Native appendParagraphRangeInfo.sArrayOfStatisticalWritingDirectionParagraphKeys, 0x22e6d30e8, and
// appendParagraphRangeInfo.onceToken, 0x22e6d30f0 (__DATA,__bss). Non-external statics of the inlined
// appendParagraphRangeInfo.
static const void **sArrayOfStatisticalWritingDirectionParagraphKeys;

static dispatch_once_t appendParagraphRangeInfoOnceToken;

// Native notFoundRange, 0x1c0385b08 (__TEXT,__const): {kCFNotFound, 0}. Non-external.
static const OCFRange notFoundRange = {kOCFNotFound, 0};

// Build versions passed to dyld_program_sdk_at_least (any platform): 0x07e70901 for the negative-length
// crash, 0x07e90000 for the null-byte crash.
static const OCFBuildVersion kNegativeLengthVersion = {0xffffffff, 0x07e70901};
static const OCFBuildVersion kNullByteEncodingVersion = {0xffffffff, 0x07e90000};

// Native __CFStringInitializeSystemEncoding, 0x1c02085f4. Hidden (was private external). Marks the system
// encoding as ready and tail-calls CFStringGetSystemEncoding.
void __CFStringInitializeSystemEncoding(void) {
    __CFIsReadyToInitializeSystemEncoding = true;
    OCFStringGetSystemEncoding();
}

// Native CFStringGetSystemEncoding, 0x1c0208604. External. Always returns kCFStringEncodingMacRoman (0).
// Before the system encoding is ready only the character-to-Unicode function is installed (from the MacRoman
// converter's to-Unicode callback, when none is set). Once ready, the first call (guarded by
// __CFStringIsBeingInitialized) marks the encoding set, asks for the MacRoman converter (a second time when
// the first answer is NULL; the second answer is dereferenced unchecked) and installs its to-Unicode callback
// when the converter is of the cheap eight-bit class (1), else NULL; the guard is cleared only on that path's
// end.
OCFStringEncoding OCFStringGetSystemEncoding(void) {
    if (__CFDefaultSystemEncoding) {
        return 0;
    }
    if (!__CFIsReadyToInitializeSystemEncoding) {
        if (__OCFCharToUniCharFunc == NULL) {
            const StringEncodingConverterRecord *converter = CFStringEncodingGetConverter(0);
            __CFSetCharToUniCharFunc(converter->toUnicode.cheapEightBit);
        }
        return 0;
    }
    if (__CFStringIsBeingInitialized) {
        return 0;
    }
    __CFStringIsBeingInitialized = true;
    const StringEncodingConverterRecord *converter = CFStringEncodingGetConverter(0);
    __CFDefaultSystemEncoding = true;
    if (converter == NULL) {
        converter = CFStringEncodingGetConverter(0);
    }
    if (converter->encodingClass == 1) {
        __CFSetCharToUniCharFunc(converter->toUnicode.cheapEightBit);
    } else {
        __CFSetCharToUniCharFunc(NULL);
    }
    __CFStringIsBeingInitialized = false;
    return 0;
}

// Native __CFStringMtbl, 0x1c02086bc. External. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json. Info bit 0 (the mutable bit) of an acquire load
// of the info word.
Boolean __OCFStringMtbl(OCFStringRef string) {
    return __atomic_load_n(&((const OCFRuntimeBase *)string)->info, __ATOMIC_ACQUIRE) & 1;
}

// Native __CFStringIsCF, 0x1c02086cc. External. A CF string (CF_IS_OBJC_CHECKING_CONSTANT_STRING answers 0,
// with *isConstant set for a constant string) reports its mutable bit, its length and whether it is
// eight-bit, and answers true; anything else answers false and leaves the three outputs untouched.
Boolean __OCFStringIsCF(
    OCFStringRef string, Boolean *isMutable, OCFIndex *length, Boolean *isEightBit, bool *isConstant
) {
    if (CF_IS_OBJC_CHECKING_CONSTANT_STRING(7, string, isConstant) != 0) {
        return false;
    }
    *isMutable = (Boolean)(stringInfo(string) & kStringInfoMutable);
    *length = stringLength(string);
    *isEightBit = (Boolean)(1 & ~(stringInfo(string) >> 4));
    return true;
}

// Native __CFStringIsEightBit, 0x1c02087a4 (corpus delta capture 7). External. Whether info bit 4 is clear
// (acquire load; BIC of 1 with the word shifted right by four), with no type check.
Boolean __OCFStringIsEightBit(OCFStringRef string) {
    return (Boolean)(1 & ~(stringInfo(string) >> 4));
}

// Native CFStringFileSystemEncoding, 0x1c02087b8. External. Marks the file-system encoding set and returns
// kCFStringEncodingUTF8.
OCFStringEncoding OCFStringFileSystemEncoding(void) {
    if (!__CFDefaultFileSystemEncoding) {
        __CFDefaultFileSystemEncoding = true;
    }
    return 0x8000100;
}

// Native CFStringGetMaximumSizeForEncoding, 0x1c02087d8. External. The Unicode-family variant is
// (encoding - 0x08000100) rotated right by 26: 1, 4 and 5 (the UTF-32 forms) need four bytes per character, 0
// (UTF-8) three; otherwise the low twelve bits select: MacRoman, ISO Latin 1, Windows Latin 1, ASCII and
// NextStep Latin one byte, UTF-16 two, non-lossy ASCII six, and any other encoding its converter's maximum
// bytes per character (the length itself when there is no converter). A product that would exceed CFIndex is
// kCFNotFound (-1); the comparisons are the native ones (high bits for x4 and x2, signed limits otherwise).
OCFIndex OCFStringGetMaximumSizeForEncoding(OCFIndex length, OCFStringEncoding encoding) {
    uint32_t difference = encoding + 0xF7FFFF00;
    uint32_t variant = (difference >> 26) | (difference << 6);
    if (variant - 4 < 2 || variant == 1) {
        return ((uint64_t)length >> 61) != 0 ? -1 : (OCFIndex)((uint64_t)length << 2);
    }
    if (variant == 0) {
        return length <= 0x2AAAAAAAAAAAAAAA ? length * 3 : -1;
    }
    switch (encoding & 0xFFF) {
        case 0:
        case 0x201:
        case 0x500:
        case 0x600:
        case 0xB01:
            return length;
        case 0x100:
            return ((uint64_t)length >> 62) != 0 ? -1 : (OCFIndex)((uint64_t)length << 1);
        case 0xBFF:
            return length <= 0x1555555555555555 ? length * 6 : -1;
        default:
            break;
    }
    const StringEncodingConverterRecord *converter = CFStringEncodingGetConverter(encoding);
    if (converter == NULL) {
        return length;
    }
    uint64_t maxBytesPerChar = converter->maxBytesPerChar;
    OCFIndex limit = (OCFIndex)(UINT64_C(0x7FFFFFFFFFFFFFFF) / maxBytesPerChar);
    return length <= limit ? (OCFIndex)((uint64_t)length * maxBytesPerChar) : -1;
}

// Native __CFStringComputeEightBitStringEncoding, 0x1c02088d4. External. Reentrant calls (guarded by the
// function-static __CFStringIsBeingInitialized2) answer ASCII. Executables linked on or after version 9 use
// ASCII (stored). Otherwise 0 is returned while the string system is being initialized or not yet ready to
// initialize the system encoding; an already computed encoding is returned as is; else the system encoding
// decides: an invalid answer returns ASCII unstored, any other stores and returns 0 (the native code stores
// the constant 0, CFStringGetSystemEncoding's only valid answer).
OCFStringEncoding __OCFStringComputeEightBitStringEncoding(void) {
    // Native __CFStringComputeEightBitStringEncoding.__CFStringIsBeingInitialized2, 0x22e6d3041
    // (__DATA,__bss).
    static bool __CFStringIsBeingInitialized2;
    if (__CFStringIsBeingInitialized2) {
        return 0x600;
    }
    __CFStringIsBeingInitialized2 = true;
    bool linkedOnOrAfter = _OCFExecutableLinkedOnOrAfter(9);
    __CFStringIsBeingInitialized2 = false;
    if (linkedOnOrAfter) {
        __OCFDefaultEightBitStringEncoding = 0x600;
        return 0x600;
    }
    if (__CFStringIsBeingInitialized) {
        return 0;
    }
    if (!__CFIsReadyToInitializeSystemEncoding) {
        return 0;
    }
    if (__OCFDefaultEightBitStringEncoding != 0xffffffff) {
        return __OCFDefaultEightBitStringEncoding;
    }
    if (OCFStringGetSystemEncoding() == 0xffffffff) {
        return 0x600;
    }
    __OCFDefaultEightBitStringEncoding = 0;
    return 0;
}

// The inline buffer __CFStringEqual builds over its eight-bit string: zeroed (0xb8 bytes), the string and the
// range (0, length), the direct UTF-16 pointer from CFStringGetCharactersPtr, and when there is none an
// inlined CFStringGetCStringPtr(string, kCFStringEncodingASCII): the contents (after any length byte, bit 2
// of the info word) when the eight-bit encoding is ASCII (the native code tests it twice) and the string is
// eight-bit or has a null byte, else NULL.
static void equalInitInlineBuffer(OCFStringRef string, OCFStringInlineBuffer *buffer, OCFIndex length) {
    memset(buffer, 0, sizeof(*buffer));
    buffer->theString = string;
    buffer->bufferedRangeEnd = 0;
    buffer->rangeToBuffer.length = length;
    buffer->directUniCharBuffer = OCFStringGetCharactersPtr(string);
    const char *cString = NULL;
    if (buffer->directUniCharBuffer == NULL) {
        if (stringEightBitEncoding() == 0x600 || stringEightBitEncoding() == 0x600) {
            if (stringIsEightBit(string) || stringHasNullByte(string)) {
                cString = (const char *)stringContents(string) + ((stringInfo(string) >> 2) & 1);
            }
        }
    }
    buffer->bufferedRangeStart = 0;
    buffer->bufferedRangeEnd = 0;
    buffer->directCStringBuffer = cString;
}

// Native _CFStringEqual, 0x1c0208a1c (corpus delta capture 7). Hidden (was private external). A tail call
// of the class's equal callback, __CFStringEqual.
Boolean _CFStringEqual(OCFStringRef first, OCFStringRef second) {
    return __CFStringEqual(first, second);
}

// Native __CFStringEqual, 0x1c0208a20. Non-external (the class's equal callback). Lengths first; then both
// contents pointers skip a length byte by bit 2 of the info word (also for mutable strings). Two eight-bit
// strings compare with memcmp, two Unicode strings character by character, and a mixed pair maps the
// eight-bit side through an inline buffer (whose direct C-string path sign-extends each byte before
// truncating it to a UniChar).
static Boolean __CFStringEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    OCFStringRef string1 = cf1;
    OCFStringRef string2 = cf2;
    const uint8_t *contents1 = stringContents(string1);
    const uint8_t *contents2 = stringContents(string2);
    OCFIndex length = stringLength(string1);
    if (length != stringLength(string2)) {
        return false;
    }
    contents1 += (stringInfo(string1) >> 2) & 1;
    contents2 += (stringInfo(string2) >> 2) & 1;
    if (stringIsEightBit(string1) && stringIsEightBit(string2)) {
        return memcmp(contents1, contents2, (size_t)length) == 0;
    }
    if (stringIsEightBit(string1)) {
        OCFStringInlineBuffer buffer;
        equalInitInlineBuffer(string1, &buffer, length);
        const UniChar *characters2 = (const UniChar *)contents2;
        for (OCFIndex index = 0; index < length; ++index) {
            if (characters2[index] != stringGetCharacterFromInlineBuffer(&buffer, index)) {
                return false;
            }
        }
        return true;
    }
    if (stringIsEightBit(string2)) {
        OCFStringInlineBuffer buffer;
        equalInitInlineBuffer(string2, &buffer, length);
        const UniChar *characters1 = (const UniChar *)contents1;
        for (OCFIndex index = 0; index < length; ++index) {
            if (characters1[index] != stringGetCharacterFromInlineBuffer(&buffer, index)) {
                return false;
            }
        }
        return true;
    }
    const UniChar *characters1 = (const UniChar *)contents1;
    const UniChar *characters2 = (const UniChar *)contents2;
    for (OCFIndex index = 0; index < length; ++index) {
        if (characters1[index] != characters2[index]) {
            return false;
        }
    }
    return true;
}

// The four-character step of the native string hash: result * 67503105 + c3 + (((c0 * 257 + c1) * 257 + c2) *
// 257), the parenthesized part computed in 32 bits (W registers; it wraps for characters above 0xFF).
static uint64_t hashFourCharacters(uint64_t result, uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3) {
    uint32_t partial = c0 + (c0 << 8);
    partial += c1;
    partial += partial << 8;
    partial += c2;
    partial += partial << 8;
    return result * 0x4060401 + c3 + partial;
}

// The eight-bit string hash, inlined natively into CFStringHashCString and __CFStringHash: each byte maps
// through
// __CFCharToUniCharTable. Up to 96 characters every character is hashed (four at a time, then one at a time
// with result * 257 + c); longer strings hash the first 32, the 32 starting at length / 2 - 16 and the
// last 32. The result starts as the length and ends as result + (result << (length & 31)).
static OCFHashCode hashEightBitCharacters(const uint8_t *bytes, OCFIndex length) {
    uint64_t result = (uint64_t)length;
    const UniChar *table = __CFCharToUniCharTable;
    if (length < 0x61) {
        const uint8_t *end = bytes + length;
        const uint8_t *cursor = bytes;
        if (length >= 4) {
            const uint8_t *end4 = bytes + (length & ~(OCFIndex)3);
            do {
                result = hashFourCharacters(
                    result, table[cursor[0]], table[cursor[1]], table[cursor[2]], table[cursor[3]]
                );
                cursor += 4;
            } while (cursor < end4);
        }
        while (cursor < end) {
            result = result + (result << 8);
            result += table[*cursor];
            ++cursor;
        }
    } else {
        for (OCFIndex offset = 0; offset < 32; offset += 4) {
            const uint8_t *cursor = bytes + offset;
            result = hashFourCharacters(
                result, table[cursor[0]], table[cursor[1]], table[cursor[2]], table[cursor[3]]
            );
        }
        const uint8_t *middle = bytes + ((uint64_t)length >> 1) - 16;
        for (OCFIndex offset = 0; offset < 32; offset += 4) {
            const uint8_t *cursor = middle + offset;
            result = hashFourCharacters(
                result, table[cursor[0]], table[cursor[1]], table[cursor[2]], table[cursor[3]]
            );
        }
        const uint8_t *end = bytes + length;
        for (const uint8_t *cursor = end - 32; cursor < end; cursor += 4) {
            result = hashFourCharacters(
                result, table[cursor[0]], table[cursor[1]], table[cursor[2]], table[cursor[3]]
            );
        }
    }
    return (OCFHashCode)((result << (length & 0x1F)) + result);
}

// The UTF-16 string hash of __CFStringHash: the same sampling as hashEightBitCharacters over UniChars.
static OCFHashCode hashUnicodeCharacters(const UniChar *characters, OCFIndex length) {
    uint64_t result = (uint64_t)length;
    if (length < 0x61) {
        const UniChar *end = characters + length;
        const UniChar *cursor = characters;
        if (length * 2 >= 8) {
            const UniChar *end4 =
                (const UniChar *)((const uint8_t *)characters + ((length * 2) & ~(OCFIndex)7));
            do {
                result = hashFourCharacters(result, cursor[0], cursor[1], cursor[2], cursor[3]);
                cursor += 4;
            } while (cursor < end4);
        }
        while (cursor < end) {
            result = result + (result << 8);
            result += *cursor;
            ++cursor;
        }
    } else {
        for (OCFIndex offset = 0; offset < 32; offset += 4) {
            const UniChar *cursor = characters + offset;
            result = hashFourCharacters(result, cursor[0], cursor[1], cursor[2], cursor[3]);
        }
        const UniChar *middle = (const UniChar *)((const uint8_t *)characters + (length & ~(OCFIndex)1)) - 16;
        for (OCFIndex offset = 0; offset < 32; offset += 4) {
            const UniChar *cursor = middle + offset;
            result = hashFourCharacters(result, cursor[0], cursor[1], cursor[2], cursor[3]);
        }
        const UniChar *end = characters + length;
        for (const UniChar *cursor = end - 32; cursor < end; cursor += 4) {
            result = hashFourCharacters(result, cursor[0], cursor[1], cursor[2], cursor[3]);
        }
    }
    return (OCFHashCode)((result << (length & 0x1F)) + result);
}

// Native CFStringHashISOLatin1CString, 0x1c0209140. External. iOS Device divergence: unread.
// The bytes contribute their unsigned Latin-1 values directly.
OCFHashCode OCFStringHashISOLatin1CString(const uint8_t *bytes, OCFIndex length) {
    uint64_t result = (uint64_t)length;
    if (length < 97) {
        OCFIndex index = 0;
        for (; index + 4 <= length; index += 4) {
            const uint8_t *p = bytes + index;
            result = hashFourCharacters(result, p[0], p[1], p[2], p[3]);
        }
        for (; index < length; ++index) {
            result = 257 * result + bytes[index];
        }
    } else {
        const OCFIndex locations[] = {0, (length >> 1) - 16, length - 32};
        for (size_t section = 0; section < 3; ++section) {
            for (OCFIndex offset = 0; offset < 32; offset += 4) {
                const uint8_t *p = bytes + locations[section] + offset;
                result = hashFourCharacters(result, p[0], p[1], p[2], p[3]);
            }
        }
    }
    return (result << (length & 31)) + result;
}

// Native CFStringHashCString, 0x1c02092c8. External. The eight-bit string hash of len bytes.
OCFHashCode OCFStringHashCString(const UInt8 *bytes, OCFIndex len) {
    return hashEightBitCharacters(bytes, len);
}

// Native CFStringHashNSString, 0x1c0209620. External. iOS Device divergence: unread.
// All Objective-C fetches occur before hashing. The assembly uses one 96-unit stack buffer.
OCFHashCode OCFStringHashNSString(OCFStringRef string) {
    OCFIndex length = ((OCFIndex (*)(id, SEL))objc_msgSend)((id)string, sel_getUid("length"));
    void (*getCharacters)(id, SEL, UniChar *, OCFRange) =
        (void (*)(id, SEL, UniChar *, OCFRange))objc_msgSend;
    SEL selector = sel_getUid("getCharacters:range:");
    UniChar characters[96];
    OCFIndex count = length;
    if (length < 97) {
        getCharacters((id)string, selector, characters, (OCFRange){0, length});
    } else {
        getCharacters((id)string, selector, characters, (OCFRange){0, 32});
        getCharacters((id)string, selector, characters + 32, (OCFRange){(length >> 1) - 16, 32});
        getCharacters((id)string, selector, characters + 64, (OCFRange){length - 32, 32});
        count = 96;
    }

    uint64_t result = (uint64_t)length;
    OCFIndex index = 0;
    for (; index + 4 <= count; index += 4) {
        const UniChar *p = characters + index;
        result = hashFourCharacters(result, p[0], p[1], p[2], p[3]);
    }
    for (; index < count; ++index) {
        result = 257 * result + characters[index];
    }
    return (result << (length & 31)) + result;
}

// Native __CFStringHash, 0x1c0209774. External (the class's hash callback). Eight-bit contents (after any
// length byte, bit 2 of the info word) use the eight-bit hash, UTF-16 contents the UTF-16 hash.
OCFHashCode __OCFStringHash(OCFTypeRef cf) {
    OCFStringRef string = cf;
    const uint8_t *contents = stringContents(string);
    OCFIndex length = stringLength(string);
    if (stringIsEightBit(string)) {
        return hashEightBitCharacters(contents + ((stringInfo(string) >> 2) & 1), length);
    }
    return hashUnicodeCharacters((const UniChar *)contents, length);
}

// Native _CFNonObjCStringCreateCopy, 0x1c0209b3c. Hidden (was private external); the class's copy callback.
// An immutable string (info bit 0 clear) whose object allocator (inline __CFGetAllocator) is the requested
// one (alloc, or for NULL the per-thread default from _CFGetTSD(1), else kCFAllocatorSystemDefault) is
// retained with _CFNonObjCRetain (a tail call) when its contents are inline ((info & 0x60) == 0), when info
// bit 5 is set, or when __CFRuntimeIsImmortal answers true. Otherwise a copy is made with
// __CFStringCreateImmutableFunnel3 from the original alloc (NULL stays NULL): Unicode contents (info bit 4)
// with 2 * length bytes, kCFStringEncodingUnicode and options 2; eight-bit contents from the contents past
// any length byte (info bit 2), length bytes in the eight-bit encoding and options 0. Both pass a
// contentsDeallocator of -1 and no converter flags. The length is read as stringLength reads it, also for
// Unicode contents with a length byte (LDRB of the first content byte, as natively).
OCFStringRef _CFNonObjCStringCreateCopy(OCFAllocatorRef alloc, OCFStringRef string) {
    if ((stringInfo(string) & kStringInfoMutable) == 0) {
        OCFAllocatorRef requested = alloc;
        if (alloc == NULL) {
            OCFAllocatorRef threadDefault = (OCFAllocatorRef)_OCFGetTSD(1);
            requested = threadDefault == NULL ? kOCFAllocatorSystemDefault : threadDefault;
        }
        if (requested == stringObjectAllocator(string)) {
            if ((stringInfo(string) & kStringInfoContentsMask) == 0 || (stringInfo(string) & 0x20) != 0 ||
                __CFRuntimeIsImmortal(string)) {
                return (OCFStringRef)_OCFNonObjCRetain(string);
            }
        }
    }
    if (stringIsUnicode(string)) {
        const void *contents = stringContents(string);
        OCFIndex length = stringLength(string);
        return __CFStringCreateImmutableFunnel3(
            alloc, contents, length << 1, 0x100, 2, (OCFAllocatorRef)(intptr_t)-1, 0
        );
    }
    const uint8_t *contents = stringContents(string);
    uint64_t info = stringInfo(string);
    OCFIndex length = stringLength(string);
    OCFIndex skip = (OCFIndex)((info >> 2) & 1);
    OCFStringEncoding encoding = stringEightBitEncoding();
    return __CFStringCreateImmutableFunnel3(
        alloc, contents + skip, length, encoding, 0, (OCFAllocatorRef)(intptr_t)-1, 0
    );
}

// Native __CFStringDeallocate, 0x1c0209d30. Non-external (the class's finalize callback). Inline contents
// need nothing. Contents kinds with bit 5 (0x20 and 0x60) are freed when the pointer is not NULL: mutable
// contents through __CFStrDeallocateMutableContents, 0x60 contents with the stored deallocator (then
// released), 0x20 contents with the object's allocator. A mutable string then releases its stored contents
// allocator when its kind is 0x60.
static void __CFStringDeallocate(OCFTypeRef cf) {
    OCFStringRef string = cf;
    if ((stringInfo(string) & kStringInfoContentsMask) == 0) {
        return;
    }
    uint8_t info = (uint8_t)stringInfo(string);
    if ((stringInfo(string) & kStringInfoContentsFreeWithObjectAllocator) != 0) {
        void *contents = (void *)stringContents(string);
        if (contents != NULL) {
            if ((info & kStringInfoMutable) != 0) {
                __CFStrDeallocateMutableContents((OCFMutableStringRef)string, contents);
            } else {
                if ((~stringInfo(string) & kStringInfoContentsMask) == 0) {
                    OCFAllocatorRef deallocator = *stringContentsDeallocatorSlot(string);
                    OCFAllocatorDeallocate(deallocator, contents);
                    OCFRelease(deallocator);
                    return;
                }
                OCFAllocatorDeallocate(stringObjectAllocator(string), contents);
                return;
            }
        } else if ((info & kStringInfoMutable) == 0) {
            return;
        }
    } else if ((info & kStringInfoMutable) == 0) {
        return;
    }
    if ((~stringInfo(string) & kStringInfoContentsMask) == 0) {
        OCFRelease(string->variants.notInlineMutable.contentsAllocator);
    }
}

// Native __CFStringCopyDescription, 0x1c0209e98. Non-external (the class's debug description callback).
// Formatted with the system default allocator: the object, its allocator and the object again.
static OCFStringRef __CFStringCopyDescription(OCFTypeRef cf) {
    OCFStringRef string = cf;
    return OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault,
        NULL,
        OF_STR("<CFString %p [%p]>{contents = \"%@\"}"),
        string,
        stringObjectAllocator(string),
        string
    );
}

// Native CFStringGetTypeID, 0x1c0209f48. External.
OCFTypeID OCFStringGetTypeID(void) {
    return 7;
}

// The encodings whose eight-bit contents are ASCII-compatible, as __CFStringCreateImmutableFunnel3 inlines
// the test (by bits 8-15 of the encoding; shifts of one are 32-bit for groups 10 and 11 and 64-bit for group
// 0). CFStringCreateExternalRepresentation compiles the same sets as a switch on the second byte.
static bool encodingIsSupersetOfASCII(OCFStringEncoding encoding) {
    switch ((encoding >> 8) & 0xFF) {
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

// Whether no byte has bit 7 set, in the native block order: 32 bytes (two 128-bit loads), 16, 8, 4, then
// single bytes.
static bool bytesAreASCII(const uint8_t *bytes, OCFIndex length) {
    uint64_t remaining = (uint64_t)length;
    const uint8_t *cursor = bytes;
    while (remaining > 0x1F) {
        uint64_t words[4];
        memcpy(words, cursor, sizeof(words));
        cursor += 32;
        if (((words[0] | words[1] | words[2] | words[3]) & 0x8080808080808080) != 0) {
            return false;
        }
        remaining -= 32;
    }
    while (remaining > 0xF) {
        uint64_t words[2];
        memcpy(words, cursor, sizeof(words));
        cursor += 16;
        if (((words[1] | words[0]) & 0x8080808080808080) != 0) {
            return false;
        }
        remaining -= 16;
    }
    while (remaining > 7) {
        uint64_t word;
        memcpy(&word, cursor, sizeof(word));
        cursor += 8;
        if ((word & 0x8080808080808080) != 0) {
            return false;
        }
        remaining -= 8;
    }
    while (remaining > 3) {
        uint32_t word;
        memcpy(&word, cursor, sizeof(word));
        cursor += 4;
        if ((word & 0x80808080) != 0) {
            return false;
        }
        remaining -= 4;
    }
    while (remaining != 0) {
        if ((int8_t)*cursor < 0) {
            return false;
        }
        ++cursor;
        --remaining;
    }
    return true;
}

// Native __CFStringCreateImmutableFunnel3, 0x1c0209f50 (with .cold.1 at 0x1c030c484 and .cold.2 at
// 0x1c030c498). Hidden (was private external). options: 0x1 possibly external format, 0x2 try to reduce
// Unicode, 0x4 bytes start with a length byte, 0x8 bytes are null-terminated, 0x30 no copy (0x10 the public
// no-copy functions, 0x20 the disallow-ROM variant). contentsDeallocator -1 means the allocator itself; NULL
// the default allocator.
// - Fewer bytes than the length byte: a crash (.cold.2) for executables linked on or after 0x07e70901, else
// NULL.
// - A null-terminated request in a Unicode-family encoding (variant 0 or 3-7 of (encoding - 0x100) rotated by
//   26): a crash (.cold.1) for executables linked on or after 0x07e90000.
// - The first 0x200 bytes of the decoding buffer are zeroed. No bytes with the system default allocator (also
//   when the default allocator is the system default): the given bytes are freed (no copy) and the retained
//   empty constant string is returned.
// - Eight-bit storage is used directly for the eight-bit encoding (other than ASCII) and for ASCII-compatible
//   encodings whose bytes are all ASCII; UTF-16 directly unless possibly external. UTF-16 that may be reduced
//   and is all ASCII is narrowed into the local buffer (below 0x3f0 bytes) or a typed allocation, with a
//   length byte below 0x200 bytes and a null byte; when that allocation fails the string stays UTF-16 with
//   the byte count set to the buffer size (kept native behavior). Everything else is decoded with
//   __CFStringDecodeByteStream3; a decoded UTF-16 buffer that the decoder allocated with the same allocator
//   is shrunk with __CFSafelyReallocateWithAllocatorTyped and adopted without copying.
// - ASCII contents of up to eleven characters become inline tagged strings while __CFTaggedStringClass is
// set.
// - Otherwise the object is created with inline contents (a length byte up to 0xff eight-bit bytes, else an
//   explicit length; a null byte for eight-bit contents) or external contents (kind 0x20, 0x40 or 0x60 by the
//   deallocator, which a 0x60 string retains).
// A buffer the decoder allocated is freed at the end, also on the tagged path (where the native code passes
// NULL, so a heap buffer would leak; tagged contents never need one).
OCFStringRef __CFStringCreateImmutableFunnel3(
    OCFAllocatorRef alloc,
    const void *bytes,
    OCFIndex numBytes,
    OCFStringEncoding encoding,
    uint8_t options,
    OCFAllocatorRef contentsDeallocator,
    UInt32 converterFlags
) {
    OCFIndex hasLengthByte = (options >> 2) & 1;
    bool freeClientBytes = (options & 0x30) != 0;
    bool noCopy = freeClientBytes;
    if (numBytes < hasLengthByte) {
        if (dyld_program_sdk_at_least(kNegativeLengthVersion)) {
            crashWithMessage("CFString cannot be created from a negative number of bytes");
        }
        return NULL;
    }
    OCFIndex decodeLength = numBytes - hasLengthByte;
    bool nullByte = ((options >> 3) & 1) != 0;
    if (nullByte) {
        uint32_t difference = encoding - 0x100;
        uint32_t variant = (difference >> 26) | (difference << 6);
        if ((variant - 3 < 5 || variant == 0) && dyld_program_sdk_at_least(kNullByteEncodingVersion)) {
            crashWithMessage(
                "CFStringCreateWithCString can only be called with an c-string compatible encoding"
            );
        }
    }
    VarWidthCharBuffer vBuf;
    memset(&vBuf, 0, 0x200);
    if (alloc == NULL) {
        alloc = _OCFGetTSD(1);
        if (alloc == NULL) {
            alloc = kOCFAllocatorSystemDefault;
        }
    }
    OCFAllocatorRef deallocator = alloc;
    if (contentsDeallocator != (OCFAllocatorRef)(intptr_t)-1) {
        if (contentsDeallocator != NULL) {
            deallocator = contentsDeallocator;
        } else {
            deallocator = _OCFGetTSD(1);
            if (deallocator == NULL) {
                deallocator = kOCFAllocatorSystemDefault;
            }
        }
    }
    if (numBytes == 0) {
        if (alloc == kOCFAllocatorSystemDefault || ((alloc == NULL || alloc == kOCFAllocatorDefault) &&
                                                    kOCFAllocatorSystemDefault == OCFAllocatorGetDefault())) {
            if (freeClientBytes && deallocator != kOCFAllocatorNull) {
                OCFAllocatorDeallocate(deallocator, (void *)bytes);
            }
            return OCFRetain(kOCFEmptyString);
        }
    }
    vBuf.shouldFreeChars = false;
    bool eightBitCompatible = false;
    if (encoding != 0x100) {
        OCFStringEncoding eightBit = stringEightBitEncoding();
        if (encoding != 0x600 && encoding == eightBit) {
            eightBitCompatible = true;
        } else if (encodingIsSupersetOfASCII(encoding)) {
            eightBitCompatible = bytesAreASCII(bytes, numBytes);
        }
    }
    const uint8_t *source = bytes;
    OCFIndex sourceLength = numBytes;
    OCFIndex contentLength;
    OCFStringEncoding storedEncoding = encoding;
    bool lengthByte = hasLengthByte != 0;
    bool sourceLengthByte;
    bool direct = encoding == 0x100 ? (options & 0x1) == 0 : eightBitCompatible;
    bool inlineContents;
    bool useNullByte;
    OCFIndex extraBytes;
    if (direct) {
        if (encoding == 0x100 && (options & 0x2) != 0) {
            OCFIndex characterCount = (OCFIndex)((uint64_t)numBytes >> 1);
            const UniChar *characters = bytes;
            bool reducible = true;
            if ((uint64_t)numBytes >= 2) {
                for (OCFIndex index = 0; index < characterCount; ++index) {
                    if (characters[index] > 0x7F) {
                        reducible = false;
                        break;
                    }
                }
            }
            if (reducible) {
                bool shortString = (uint64_t)numBytes < 0x200;
                OCFIndex size = characterCount + (shortString ? 1 : 0) + 1;
                uint8_t *buffer;
                bool allocated = true;
                if ((uint64_t)size < 0x3F0) {
                    buffer = vBuf.localBuffer;
                } else {
                    buffer = OCFAllocatorAllocateTyped(alloc, size, 0x100004077774924, 0);
                    if (__OCFOASafe) {
                        __OCFSetLastAllocationEventName(buffer, "CFString (store)");
                    }
                    if (buffer == NULL) {
                        allocated = false;
                    }
                }
                if (allocated) {
                    uint8_t *cursor = buffer;
                    if ((uint64_t)numBytes <= 0x1FF) {
                        *cursor++ = (uint8_t)characterCount;
                    }
                    if ((uint64_t)numBytes > 0x1FF || (uint64_t)numBytes >= 2) {
                        for (OCFIndex index = 0; index != characterCount; ++index) {
                            cursor[index] = (uint8_t)characters[index];
                        }
                    }
                    cursor[characterCount] = 0;
                    if (freeClientBytes && deallocator != kOCFAllocatorNull) {
                        OCFAllocatorDeallocate(deallocator, (void *)bytes);
                    }
                    noCopy = (uint64_t)size > 0x3EF;
                    sourceLength = characterCount + (shortString ? 1 : 0);
                    contentLength = sourceLength - (shortString ? 1 : 0);
                    nullByte = true;
                    storedEncoding = 0x600;
                    source = buffer;
                    deallocator = alloc;
                    lengthByte = shortString;
                    sourceLengthByte = shortString;
                    goto tryTaggedString;
                }
                sourceLength = size;
            }
            storedEncoding = 0x100;
        }
        contentLength = sourceLength - (lengthByte ? 1 : 0);
        sourceLengthByte = lengthByte;
        if (eightBitCompatible) {
            goto tryTaggedString;
        }
        goto createObject;
    }
    {
        Boolean useClientsMemory = false;
        vBuf.allocator = kOCFAllocatorSystemDefault;
        vBuf.chars.ascii = NULL;
        if (!__OCFStringDecodeByteStream3(
                source + hasLengthByte,
                decodeLength,
                encoding,
                false,
                &vBuf,
                &useClientsMemory,
                converterFlags
            )) {
            return NULL;
        }
        bool isASCII = vBuf.isASCII != 0;
        storedEncoding = isASCII ? 0x600 : 0x100;
        if (useClientsMemory) {
            sourceLengthByte = hasLengthByte != 0;
        } else {
            sourceLength = vBuf.numChars << (isASCII ? 0 : 1);
            if (freeClientBytes && deallocator != kOCFAllocatorNull) {
                OCFAllocatorDeallocate(deallocator, (void *)bytes);
            }
            if (vBuf.shouldFreeChars && alloc == vBuf.allocator && !isASCII) {
                vBuf.shouldFreeChars = false;
                source = __OCFSafelyReallocateWithAllocatorTyped(
                    vBuf.allocator, vBuf.chars.unicode, sourceLength, 0x1A416876, 0, NULL
                );
                useNullByte = false;
                lengthByte = false;
                sourceLengthByte = false;
                deallocator = alloc;
                contentLength = sourceLength;
                goto createExternalObject;
            }
            noCopy = false;
            nullByte = false;
            sourceLengthByte = false;
            deallocator = alloc;
            source = vBuf.chars.ascii;
        }
        contentLength = sourceLength - (sourceLengthByte ? 1 : 0);
        lengthByte = sourceLengthByte;
        if (!isASCII) {
            goto createObject;
        }
    }
tryTaggedString:
    if (__CFTaggedStringClass != 0) {
        const uint8_t *characters = source + (sourceLengthByte ? 1 : 0);
        uintptr_t tagged = 0;
        if (contentLength <= 7) {
            uint64_t word = 0;
            __builtin___memmove_chk(&word, characters, (size_t)contentLength, sizeof(word));
            if ((word & 0x8080808080808080) == 0) {
                tagged = stringEncodeTaggedPointer(
                    ((uintptr_t)contentLength << 3) | (word << 7) | 0x8000000000000002
                );
            }
        } else if ((uint64_t)contentLength <= 0xB) {
            uint64_t code = 0;
            bool encoded = true;
            if ((uint64_t)contentLength < 0xA) {
                for (OCFIndex index = 0; index != contentLength; ++index) {
                    uint64_t sixBit = charToSixBitLookup[characters[index]];
                    if (sixBit > 0x3F) {
                        encoded = false;
                        break;
                    }
                    code = sixBit | (code << 6);
                }
            } else {
                for (OCFIndex index = 0; index != contentLength; ++index) {
                    uint64_t fiveBit = charToSixBitLookup[characters[index]];
                    if (fiveBit > 0x1F) {
                        encoded = false;
                        break;
                    }
                    code = fiveBit | (code << 5);
                }
            }
            if (encoded) {
                tagged = stringEncodeTaggedPointer(
                    (code << 7) | ((uintptr_t)contentLength << 3) | 0x8000000000000002
                );
            }
        }
        if (tagged != 0) {
            if (noCopy && deallocator != kOCFAllocatorNull) {
                OCFAllocatorDeallocate(deallocator, (void *)source);
            }
            if (vBuf.shouldFreeChars) {
                OCFAllocatorDeallocate(vBuf.allocator, NULL);
            }
            return (OCFStringRef)tagged;
        }
    }
createObject:
    if (noCopy) {
        useNullByte = nullByte;
        goto createExternalObject;
    }
    useNullByte = nullByte;
    {
        OCFIndex size;
        if (lengthByte) {
            size = sourceLength;
        } else if (storedEncoding != 0x100 && sourceLength <= 0xFF) {
            size = sourceLength + 1;
            lengthByte = true;
        } else {
            lengthByte = false;
            size = sourceLength + 8;
        }
        useNullByte = storedEncoding == 0x100 ? useNullByte : true;
        extraBytes = size + (useNullByte ? 1 : 0);
        inlineContents = true;
        goto createInstance;
    }
createExternalObject:
    inlineContents = false;
    {
        OCFIndex size = (deallocator == kOCFAllocatorNull || deallocator == alloc) ? 8 : 16;
        extraBytes = lengthByte ? size : size + 8;
    }
createInstance:;
    OCFStringRef string = (OCFStringRef)_OCFRuntimeCreateInstance(alloc, 7, extraBytes, NULL);
    if (string == NULL) {
        if (deallocator != kOCFAllocatorNull && !inlineContents) {
            OCFAllocatorDeallocate(deallocator, (void *)source);
        }
        if (vBuf.shouldFreeChars) {
            OCFAllocatorDeallocate(vBuf.allocator, (void *)source);
        }
        return NULL;
    }
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName((void *)string, "CFString (immutable)");
    }
    uint64_t kind;
    if (inlineContents) {
        kind = 0;
    } else if (deallocator == alloc) {
        kind = kStringInfoContentsFreeWithObjectAllocator;
    } else if (deallocator == kOCFAllocatorNull) {
        kind = kStringInfoContentsNotFreed;
    } else {
        kind = kStringInfoContentsCustomFree;
    }
    stringSetInfoBits(string, kStringInfoContentsMask, kind);
    stringSetInfoBits(string, kStringInfoUnicode, storedEncoding == 0x100 ? kStringInfoUnicode : 0);
    stringSetInfoBits(string, kStringInfoNullByte, (uint64_t)(useNullByte ? 1 : 0) << 3);
    stringSetInfoBits(string, kStringInfoLengthByte, (uint64_t)(lengthByte ? 1 : 0) << 2);
    if (!lengthByte) {
        OCFIndex storedLength = (OCFIndex)((uint64_t)contentLength >> (storedEncoding == 0x100 ? 1 : 0));
        uintptr_t offset = (stringInfo(string) & kStringInfoContentsMask) == 0 ? 0x10 : 0x18;
        *(OCFIndex *)((uintptr_t)string + offset) = storedLength;
    }
    if (inlineContents) {
        uint8_t *contents = (uint8_t *)stringContents(string);
        if (!(lengthByte ? sourceLengthByte : true)) {
            *contents++ = (uint8_t)sourceLength;
        }
        memmove(contents, source, (size_t)sourceLength);
        if (useNullByte) {
            contents[sourceLength] = 0;
        }
    } else {
        struct __OCFString *object = (struct __OCFString *)string;
        object->variants.notInlineImmutable1.buffer = (void *)source;
        if ((~stringInfo(string) & kStringInfoContentsMask) == 0) {
            OCFRetain(deallocator);
            *stringContentsDeallocatorSlot(string) = deallocator;
        }
    }
    if (vBuf.shouldFreeChars) {
        OCFAllocatorDeallocate(vBuf.allocator, (void *)source);
    }
    return string;
}

// Native CFStringCreateWithCString, 0x1c020ab48. External. strlen bytes with a null byte (options 0x8), the
// allocator as contents deallocator (-1).
OCFStringRef OCFStringCreateWithCString(OCFAllocatorRef alloc, const char *cStr, OCFStringEncoding encoding) {
    return __CFStringCreateImmutableFunnel3(
        alloc, cStr, (OCFIndex)strlen(cStr), encoding, 0x8, (OCFAllocatorRef)(intptr_t)-1, 0
    );
}

// Native CFStringCreateWithCStringNoCopy, 0x1c020abb4. External. As CFStringCreateWithCString without copying
// (options 0x18).
OCFStringRef OCFStringCreateWithCStringNoCopy(
    OCFAllocatorRef alloc, const char *cStr, OCFStringEncoding encoding, OCFAllocatorRef contentsDeallocator
) {
    return __CFStringCreateImmutableFunnel3(
        alloc, cStr, (OCFIndex)strlen(cStr), encoding, 0x18, contentsDeallocator, 0
    );
}

// Native _CFStringCreateWithNullTerminatedBytesNoCopy, 0x1c020ac08. External. iOS Device divergence: unread.
OCFStringRef _OCFStringCreateWithNullTerminatedBytesNoCopy(
    OCFAllocatorRef alloc,
    const uint8_t *bytes,
    OCFIndex numBytes,
    OCFStringEncoding encoding,
    OCFAllocatorRef contentsDeallocator
) {
    return __CFStringCreateImmutableFunnel3(alloc, bytes, numBytes, encoding, 0x18, contentsDeallocator, 0);
}

// Native CFStringCreateWithCharacters, 0x1c020ac18. External. UTF-16 that may be reduced (options 0x2).
OCFStringRef OCFStringCreateWithCharacters(OCFAllocatorRef alloc, const UniChar *chars, OCFIndex numChars) {
    return __CFStringCreateImmutableFunnel3(
        alloc, chars, numChars << 1, 0x100, 0x2, (OCFAllocatorRef)(intptr_t)-1, 0
    );
}

// Native CFStringCreateWithCharactersNoCopy, 0x1c020ac30. External. UTF-16 without copying (options 0x10; not
// reduced).
OCFStringRef OCFStringCreateWithCharactersNoCopy(
    OCFAllocatorRef alloc, const UniChar *chars, OCFIndex numChars, OCFAllocatorRef contentsDeallocator
) {
    return __CFStringCreateImmutableFunnel3(alloc, chars, numChars << 1, 0x100, 0x10, contentsDeallocator, 0);
}

// Native CFStringCreateWithBytes, 0x1c020ac48. External. options 0x2, plus 0x1 for an external
// representation.
OCFStringRef OCFStringCreateWithBytes(
    OCFAllocatorRef alloc,
    const UInt8 *bytes,
    OCFIndex numBytes,
    OCFStringEncoding encoding,
    Boolean isExternalRepresentation
) {
    return __CFStringCreateImmutableFunnel3(
        alloc,
        bytes,
        numBytes,
        encoding,
        isExternalRepresentation != 0 ? 0x3 : 0x2,
        (OCFAllocatorRef)(intptr_t)-1,
        0
    );
}

// Native CFStringCreateWithBytesNoCopy, 0x1c020ac74. External. options 0x12, plus 0x1 for an external
// representation.
OCFStringRef OCFStringCreateWithBytesNoCopy(
    OCFAllocatorRef alloc,
    const UInt8 *bytes,
    OCFIndex numBytes,
    OCFStringEncoding encoding,
    Boolean isExternalRepresentation,
    OCFAllocatorRef contentsDeallocator
) {
    return __CFStringCreateImmutableFunnel3(
        alloc, bytes, numBytes, encoding, isExternalRepresentation != 0 ? 0x13 : 0x12, contentsDeallocator, 0
    );
}

// Native _CFStringCreateWithBytesNoCopyDisallowROM, 0x1c020ac88. Hidden (was private external). options 0x22,
// plus 0x1 for an external representation.
OCFStringRef _CFStringCreateWithBytesNoCopyDisallowROM(
    OCFAllocatorRef alloc,
    const UInt8 *bytes,
    OCFIndex numBytes,
    OCFStringEncoding encoding,
    Boolean isExternalRepresentation,
    OCFAllocatorRef contentsDeallocator
) {
    return __CFStringCreateImmutableFunnel3(
        alloc, bytes, numBytes, encoding, isExternalRepresentation != 0 ? 0x23 : 0x22, contentsDeallocator, 0
    );
}

// Native CFStringCreateMutable, 0x1c020ae70. External. 32 extra bytes (the contents allocator slot at 0x30 is
// outside the object; it is written only for contents kind 0x60, which this function never sets). Kind 0x20,
// eight-bit, mutable; no buffer, zero length and capacity; the bits word is (maxLength << 4) | fixed
// capacity, or a desired capacity of 32 (0x200) when maxLength is 0.
OCFMutableStringRef OCFStringCreateMutable(OCFAllocatorRef alloc, OCFIndex maxLength) {
    if (alloc == NULL) {
        alloc = _OCFGetTSD(1);
        if (alloc == NULL) {
            alloc = kOCFAllocatorSystemDefault;
        }
    }
    struct __OCFString *string = (struct __OCFString *)_OCFRuntimeCreateInstance(alloc, 7, 0x20, NULL);
    if (string == NULL) {
        return string;
    }
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName(string, "CFString (mutable)");
    }
    stringSetInfoBits(string, kStringInfoContentsMask, kStringInfoContentsFreeWithObjectAllocator);
    stringSetInfoBits(string, kStringInfoUnicode, 0);
    stringSetInfoBits(string, kStringInfoMutable, kStringInfoMutable);
    string->variants.notInlineMutable.buffer = NULL;
    if ((stringInfo(string) & kStringInfoContentsMask) != 0) {
        string->variants.notInlineMutable.length = 0;
    } else {
        string->variants.inline1.length = 0;
    }
    string->variants.notInlineMutable.capacity = 0;
    string->variants.notInlineMutable.bits =
        maxLength == 0
            ? 0x200
            : (((uint64_t)maxLength << kStringMutableDesiredCapacityShift) | kStringMutableFixedCapacity);
    if ((~stringInfo(string) & kStringInfoContentsMask) == 0) {
        OCFRetain(alloc);
        string->variants.notInlineMutable.contentsAllocator = alloc;
    }
    return string;
}

// Native CFStringCreateCopy, 0x1c020e2d8. External. A tagged string is returned itself (not retained); an
// Objective-C string receives -copy; a CF string goes to _CFNonObjCStringCreateCopy.
OCFStringRef OCFStringCreateCopy(OCFAllocatorRef alloc, OCFStringRef theString) {
    if (stringIsTagged(theString)) {
        return theString;
    }
    if (__CF_IS_OBJC(7, theString) != 0) {
        return ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)theString, sel_getUid("copy"));
    }
    return _CFNonObjCStringCreateCopy(alloc, theString);
}

// Native CFStringCreateWithSubstring, 0x1c020e5d4. External. Tagged and Objective-C strings receive
// -_createSubstringWithRange:. The whole string goes to _CFNonObjCStringCreateCopy. Eight-bit contents are
// passed from location (then past any length byte, bit 2 of the info word) in the eight-bit encoding with no
// options; UTF-16 contents with options 0x2.
OCFStringRef OCFStringCreateWithSubstring(OCFAllocatorRef alloc, OCFStringRef str, OCFRange range) {
    if (stringIsTagged(str) || __CF_IS_OBJC(7, str) != 0) {
        return ((OCFStringRef (*)(id, SEL, OCFRange))objc_msgSend)(
            (id)str, sel_getUid("_createSubstringWithRange:"), range
        );
    }
    if (range.location == 0 && range.length == stringLength(str)) {
        return _CFNonObjCStringCreateCopy(alloc, str);
    }
    if (stringIsEightBit(str)) {
        const uint8_t *contents = (const uint8_t *)stringContents(str) + range.location;
        OCFIndex skip = (OCFIndex)((stringInfo(str) >> 2) & 1);
        OCFStringEncoding encoding = stringEightBitEncoding();
        return __CFStringCreateImmutableFunnel3(
            alloc, contents + skip, range.length, encoding, 0, (OCFAllocatorRef)(intptr_t)-1, 0
        );
    }
    const UniChar *characters = (const UniChar *)stringContents(str) + range.location;
    return __CFStringCreateImmutableFunnel3(
        alloc, characters, range.length << 1, 0x100, 0x2, (OCFAllocatorRef)(intptr_t)-1, 0
    );
}

// Native _CFStringCopyBundleUnloadingProtectedString, 0x1c020e908. Hidden (was private external). A tagged
// string is returned itself. ASCII contents (_CFStringGetCStringPtrInternal with null termination and BMP
// required) that dyld reports immutable are kept for a constant string (returned itself, not retained) or
// wrapped without copying (a tagged or kCFAllocatorNull string); anything else takes the slow path. The
// length is read before the C-string pointer is tested.
OCFStringRef _CFStringCopyBundleUnloadingProtectedString(OCFStringRef string) {
    if (stringIsTagged(string)) {
        return string;
    }
    const char *cString = _CFStringGetCStringPtrInternal(string, 0x600, true, true);
    OCFIndex length = OCFStringGetLength(string);
    if (cString != NULL && _dyld_is_memory_immutable(cString, (size_t)length)) {
        if (string->base.isa == (uintptr_t)__OCFConstantStringClassReferencePtr) {
            return string;
        }
        return _OCFStringCreateWithASCIIBytesNoCopyOrIndirectTagged(
            kOCFAllocatorSystemDefault, (const UInt8 *)cString, length
        );
    }
    return __CFStringSlowPathCopyBundleUnloadingProtectedString(string);
}

// Native CFStringGetCStringPtr, 0x1c020e9bc. External. _CFStringGetCStringPtrInternal requiring null
// termination and BMP (a tail call).
const char *OCFStringGetCStringPtr(OCFStringRef theString, OCFStringEncoding encoding) {
    return _CFStringGetCStringPtrInternal(theString, encoding, true, true);
}

// Native CFStringGetLength, 0x1c020e9c8. External. Tagged strings: tag index 2 (inline) reads
// __NSTaggedPointerStringGetLength, tag index 0 (indirect) the 16-bit length from
// _CFIndirectTaggedPointerStringGetContents, any other tag receives -length (the native code searches the
// permutation table twice). Objective-C strings receive -length.
OCFIndex OCFStringGetLength(OCFStringRef theString) {
    if (stringIsTagged(theString)) {
        unsigned tag = stringTagIndex(theString);
        if (tag == kStringTagInline) {
            return _ONSTaggedPointerStringGetLength(theString);
        }
        if (tag == kStringTagIndirect) {
            uint16_t length = 0;
            _OCFIndirectTaggedPointerStringGetContents(theString, &length);
            return length;
        }
        return ((OCFIndex (*)(id, SEL))objc_msgSend)((id)theString, sel_getUid("length"));
    }
    if (__CF_IS_OBJC(7, theString) != 0) {
        return ((OCFIndex (*)(id, SEL))objc_msgSend)((id)theString, sel_getUid("length"));
    }
    return stringLength(theString);
}

// Native __CFStringSlowPathCopyBundleUnloadingProtectedString, 0x1c020eb78. Non-external. A copy owned by the
// system default allocator: the empty constant string (not retained) for an empty string; else the contents
// in the string's fastest encoding when _CFStringGetCStringPtrInternal(fastest, no null termination, BMP) has
// them, or UTF-16 from CFStringGetCharactersPtr (both with options 0x2); else CFStringGetBytes into a buffer
// of CFStringGetMaximumSizeForEncoding bytes (malloc above 0x100 bytes with type 0x100004077774924, a zeroed
// stack buffer otherwise, none for 0) converted the same way, or CFStringCreateMutableCopy when nothing
// converts.
static OCFStringRef __CFStringSlowPathCopyBundleUnloadingProtectedString(OCFStringRef string) {
    OCFIndex length = OCFStringGetLength(string);
    if (length == 0) {
        return kOCFEmptyString;
    }
    OCFStringEncoding encoding = OCFStringGetFastestEncoding(string);
    const char *cString = _CFStringGetCStringPtrInternal(string, encoding, false, true);
    if (cString != NULL) {
        return __CFStringCreateImmutableFunnel3(
            kOCFAllocatorSystemDefault, cString, length, encoding, 0x2, (OCFAllocatorRef)(intptr_t)-1, 0
        );
    }
    const UniChar *characters = OCFStringGetCharactersPtr(string);
    if (characters != NULL) {
        return __CFStringCreateImmutableFunnel3(
            kOCFAllocatorSystemDefault, characters, length << 1, 0x100, 0x2, (OCFAllocatorRef)(intptr_t)-1, 0
        );
    }
    OCFIndex size = OCFStringGetMaximumSizeForEncoding(length, encoding);
    OCFIndex usedLength = 0;
    OCFStringRef result;
    if ((uint64_t)size > 0x100) {
        UInt8 *buffer = malloc_type_malloc((size_t)size, 0x100004077774924);
        if (OCFStringGetBytes(string, (OCFRange){0, length}, encoding, 0, false, buffer, size, &usedLength) !=
            0) {
            result = __CFStringCreateImmutableFunnel3(
                kOCFAllocatorSystemDefault,
                buffer,
                usedLength,
                encoding,
                0x2,
                (OCFAllocatorRef)(intptr_t)-1,
                0
            );
        } else {
            result = (OCFStringRef)OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, string);
        }
        free(buffer);
        return result;
    }
    UInt8 stackBuffer[size == 0 ? 1 : size];
    UInt8 *buffer = NULL;
    if (size != 0) {
        buffer = stackBuffer;
        bzero(buffer, (size_t)size);
    }
    if (OCFStringGetBytes(string, (OCFRange){0, length}, encoding, 0, false, buffer, size, &usedLength) !=
        0) {
        result = __CFStringCreateImmutableFunnel3(
            kOCFAllocatorSystemDefault, buffer, usedLength, encoding, 0x2, (OCFAllocatorRef)(intptr_t)-1, 0
        );
    } else {
        result = (OCFStringRef)OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, string);
    }
    return result;
}

// Native CFStringCreateMutableWithExternalCharactersNoCopy, 0x1c020f2d4. External. The contents kind follows
// the deallocator: NULL 0x20 (the object's allocator), kCFAllocatorNull 0x40, any other 0x60 (with a
// 0x28-byte body instead of 0x20 for the contents allocator). A Unicode mutable string with no buffer, zero
// length, capacity 0 and bits 0x200 (the 16-byte constant at 0x1c0384140) is created; a 0x60 string first
// stores the retained allocator, then the bits word gains external mutable (0x4, a plain read-modify-write),
// and a 0x60 string replaces the stored allocator by the retained deallocator. The characters are then
// installed with CFStringSetExternalCharactersNoCopy.
OCFMutableStringRef OCFStringCreateMutableWithExternalCharactersNoCopy(
    OCFAllocatorRef alloc,
    UniChar *chars,
    OCFIndex numChars,
    OCFIndex capacity,
    OCFAllocatorRef externalCharactersAllocator
) {
    uint64_t kindCode;
    if (externalCharactersAllocator == NULL) {
        kindCode = 1;
    } else {
        kindCode = externalCharactersAllocator != kOCFAllocatorNull ? 3 : 2;
    }
    if (alloc == NULL) {
        alloc = _OCFGetTSD(1);
        if (alloc == NULL) {
            alloc = kOCFAllocatorSystemDefault;
        }
    }
    struct __OCFString *string =
        (struct __OCFString *)_OCFRuntimeCreateInstance(alloc, 7, kindCode == 3 ? 0x28 : 0x20, NULL);
    if (string == NULL) {
        return string;
    }
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName(string, "CFString (mutable)");
    }
    stringSetInfoBits(string, kStringInfoContentsMask, kindCode << 5);
    stringSetInfoBits(string, kStringInfoUnicode, kStringInfoUnicode);
    stringSetInfoBits(string, kStringInfoMutable, kStringInfoMutable);
    string->variants.notInlineMutable.buffer = NULL;
    if ((stringInfo(string) & kStringInfoContentsMask) != 0) {
        string->variants.notInlineMutable.length = 0;
    } else {
        string->variants.inline1.length = 0;
    }
    string->variants.notInlineMutable.capacity = 0;
    string->variants.notInlineMutable.bits = 0x200;
    if ((~stringInfo(string) & kStringInfoContentsMask) == 0) {
        OCFRetain(alloc);
        string->variants.notInlineMutable.contentsAllocator = alloc;
    }
    string->variants.notInlineMutable.bits |= kStringMutableExternal;
    if ((~stringInfo(string) & kStringInfoContentsMask) == 0) {
        OCFRelease(string->variants.notInlineMutable.contentsAllocator);
        OCFRetain(externalCharactersAllocator);
        string->variants.notInlineMutable.contentsAllocator = externalCharactersAllocator;
    }
    OCFStringSetExternalCharactersNoCopy(string, chars, numChars, capacity);
    return string;
}

// Native CFStringSetExternalCharactersNoCopy, 0x1c020f4a0 (with .cold.1 at 0x1c030c5b4 and its outlined
// fragments OUTLINED_FUNCTION_1_19, _0_27 and _2_16). External. An immutable string logs a fault through
// _CFOSLog ("CFString: %s(): Expect mutable string" with the native function signature; os_log_type_enabled
// with type 0x11, a 12-byte buffer, _os_log_fault_impl) and changes nothing. Otherwise the buffer and length
// are stored, the capacity becomes capacity * 2 bytes and the bits word gains capacity provided externally
// (0x8, a plain read-modify-write).
void OCFStringSetExternalCharactersNoCopy(
    OCFMutableStringRef theString, UniChar *chars, OCFIndex length, OCFIndex capacity
) {
    if ((stringInfo(theString) & kStringInfoMutable) == 0) {
        os_log_t log = _CFOSLog();
        os_log_fault(
            log,
            "CFString: %s(): Expect mutable string",
            "void CFStringSetExternalCharactersNoCopy(CFMutableStringRef, UniChar *, CFIndex, CFIndex)"
        );
        return;
    }
    theString->variants.notInlineMutable.buffer = chars;
    if ((stringInfo(theString) & kStringInfoContentsMask) != 0) {
        theString->variants.notInlineMutable.length = length;
    } else {
        theString->variants.inline1.length = length;
    }
    uint64_t bits = theString->variants.notInlineMutable.bits | kStringMutableCapacityProvidedExternally;
    theString->variants.notInlineMutable.capacity = capacity << 1;
    theString->variants.notInlineMutable.bits = bits;
}

// Native CFStringCreateMutableCopy, 0x1c020f510. External. Tagged and Objective-C strings receive
// -mutableCopy (the allocator and maximum length are not passed). A new mutable string is sized with
// __CFStringChangeSizeMultiple (one empty range at 0, the source length, Unicode when the non-empty source
// answers CFStrIsUnicode). Eight-bit contents are filled with CFStringGetBytes in the eight-bit encoding
// (into the contents past any length byte), UTF-16 contents with CFStringGetCharacters when the buffer is not
// NULL. When the new string is the source itself, a system-default copy of the source is read and released
// afterwards.
OCFMutableStringRef
OCFStringCreateMutableCopy(OCFAllocatorRef alloc, OCFIndex maxLength, OCFStringRef theString) {
    if (stringIsTagged(theString) || __CF_IS_OBJC(7, theString) != 0) {
        return ((OCFMutableStringRef (*)(id, SEL))objc_msgSend)((id)theString, sel_getUid("mutableCopy"));
    }
    OCFMutableStringRef newString = OCFStringCreateMutable(alloc, maxLength);
    OCFStringRef copy = NULL;
    if (theString == newString) {
        copy = OCFStringCreateCopy(kOCFAllocatorSystemDefault, theString);
        theString = copy;
    }
    OCFIndex length = OCFStringGetLength(theString);
    Boolean makeUnicode = length >= 1 ? CFStrIsUnicode(theString) != 0 : false;
    OCFRange range = {0, 0};
    __CFStringChangeSizeMultiple(newString, &range, 1, length, makeUnicode);
    if (stringIsEightBit(newString)) {
        uint8_t *contents = (uint8_t *)stringContents(newString);
        OCFStringEncoding encoding = stringEightBitEncoding();
        OCFIndex skip = (OCFIndex)((stringInfo(newString) >> 2) & 1);
        OCFStringGetBytes(
            theString, (OCFRange){0, length}, encoding, 0, false, contents + skip, length, NULL
        );
    } else {
        UniChar *contents = (UniChar *)stringContents(newString);
        if (contents != NULL) {
            OCFStringGetCharacters(theString, (OCFRange){0, length}, contents);
        }
    }
    if (copy != NULL) {
        OCFRelease(copy);
    }
    return newString;
}

// Native _CFStringGetLength2, 0x1c020f724. External. The CF length, without dispatch.
OCFIndex _OCFStringGetLength2(OCFStringRef string) {
    return stringLength(string);
}

// Native CFStringGetCharacterAtIndex, 0x1c020f790. External. Tagged and Objective-C strings receive
// -characterAtIndex:. Eight-bit contents (past any length byte, bit 2 of the info word) map through
// __CFCharToUniCharTable; no range check.
UniChar OCFStringGetCharacterAtIndex(OCFStringRef theString, OCFIndex idx) {
    if (stringIsTagged(theString) || __CF_IS_OBJC(7, theString) != 0) {
        return ((UniChar (*)(id, SEL, OCFIndex))objc_msgSend)(
            (id)theString, sel_getUid("characterAtIndex:"), idx
        );
    }
    const uint8_t *contents = stringContents(theString);
    if (stringIsEightBit(theString)) {
        contents += (stringInfo(theString) >> 2) & 1;
        return __CFCharToUniCharTable[contents[idx]];
    }
    return ((const UniChar *)contents)[idx];
}

// Native _CFStringCheckAndGetCharacterAtIndex, 0x1c020f844 (corpus delta capture 7). External. CF strings
// only (no dispatch). A negative index, for programs linked before the 0x07E30901 SDKs
// (dyld_program_sdk_at_least with 0x7E30901FFFFFFFF false), stores 0 and answers 0 (success); otherwise a
// negative index or one at or beyond the length answers 3. Eight-bit contents (past any length byte) map
// through __CFCharToUniCharTable; the character is stored and 0 answered.
int _OCFStringCheckAndGetCharacterAtIndex(OCFStringRef string, OCFIndex index, UniChar *character) {
    if (index < 0 && !dyld_program_sdk_at_least((OCFBuildVersion){0xFFFFFFFF, 0x07E30901})) {
        *character = 0;
        return 0;
    }
    const uint8_t *contents = stringContents(string);
    if (index < 0 || index >= stringLength(string)) {
        return 3;
    }
    if (stringIsEightBit(string)) {
        contents += (stringInfo(string) >> 2) & 1;
        *character = __CFCharToUniCharTable[contents[index]];
        return 0;
    }
    *character = ((const UniChar *)contents)[index];
    return 0;
}

// Native CFStringGetCharacters, 0x1c020f950. External. Tagged and Objective-C strings receive
// -getCharacters:range:. Eight-bit contents from range.location (then past any length byte) are widened with
// __CFStrConvertBytesToUnicode, UTF-16 contents copied with memmove (both tail calls); no range check.
void OCFStringGetCharacters(OCFStringRef theString, OCFRange range, UniChar *buffer) {
    if (stringIsTagged(theString) || __CF_IS_OBJC(7, theString) != 0) {
        ((void (*)(id, SEL, UniChar *, OCFRange))objc_msgSend)(
            (id)theString, sel_getUid("getCharacters:range:"), buffer, range
        );
        return;
    }
    const uint8_t *contents = stringContents(theString);
    if (stringIsEightBit(theString)) {
        OCFIndex skip = (OCFIndex)((stringInfo(theString) >> 2) & 1);
        __OCFStrConvertBytesToUnicode(contents + range.location + skip, buffer, range.length);
        return;
    }
    memmove(buffer, (const UniChar *)contents + range.location, (size_t)range.length << 1);
}

// Native _CFStringCheckAndGetCharacters, 0x1c020fa2c (corpus delta capture 7). External. CF strings only (no
// dispatch). A range whose end (location + length, signed) passes the length answers 3. Eight-bit contents
// from the location (then past any length byte) are widened with __CFStrConvertBytesToUnicode, UTF-16
// contents copied with memmove; 0 is answered. The location is not checked.
int _OCFStringCheckAndGetCharacters(
    OCFStringRef string, OCFIndex location, OCFIndex length, UniChar *buffer
) {
    const uint8_t *contents = stringContents(string);
    if (location + length > stringLength(string)) {
        return 3;
    }
    if (stringIsEightBit(string)) {
        OCFIndex skip = (OCFIndex)((stringInfo(string) >> 2) & 1);
        __OCFStrConvertBytesToUnicode(contents + location + skip, buffer, length);
    } else {
        memmove(buffer, (const UniChar *)contents + location, (size_t)length << 1);
    }
    return 0;
}

// The eight-bit content bytes after the length byte: the contents pointer plus info bit 2 (UBFX #2, #1; the
// native code adds the raw bit, not the (info & 5) == 4 test).
static inline const uint8_t *stringEightBitBytes(OCFStringRef string) {
    const uint8_t *contents = stringContents(string);
    return contents + ((stringInfo(string) >> 2) & 1);
}

// The tag of a tagged pointer with the extended tag decoded, as CFStringGetCharactersPtr inlines it: the full
// 64-bit obfuscator, and for a missing permutation entry (index 7) the extended tag ((index | value) >> 55,
// low byte) + 8.
static inline unsigned stringTaggedPointerTag(OCFTypeRef cf) {
    uintptr_t pointer = (uintptr_t)cf;
    uintptr_t obfuscator = (~pointer & 0xC000000000000007) == 0 ? 0 : objc_debug_taggedpointer_obfuscator;
    uintptr_t value = obfuscator ^ pointer;
    uintptr_t index = 0;
    while (index != 7 && (value & 7) != objc_debug_tag60_permutations[index]) {
        ++index;
    }
    if (index == 7) {
        return (uint8_t)((index | value) >> 55) + 8;
    }
    return (unsigned)index;
}

// Native CFStringGetBytes, 0x1c020fd4c. External.
// A tagged pointer with a nonzero lossByte, an Unicode CF string, and a CF string whose bytes cannot be
// returned as `encoding` go to __CFStringEncodeByteStream (a tail call; generatingExternalFile is
// isExternalRepresentation). An Objective-C object with a nonzero lossByte also goes there. An inline tagged
// string (tag index 2) answers ISO Latin 1 (0x201), UTF-8 (0x8000100) and ASCII (0x600) itself through
// _NSTaggedPointerStringGetLength (no buffer) or _NSTaggedPointerStringGetBytes (into a 16-byte local), when
// the range fits (length - range.length >= 0 and range.location <= that difference). Every other case sends
// -getBytes:maxLength:usedLength:encoding:options:range:remainingRange: with the NSStringEncoding of
// `encoding`, options 2 for an external representation (else 0) and the range as remaining range; the result
// is the remaining range's location minus range.location when the send answers YES, else 0. The eight-bit
// copy is min(range.length, maxBufLen) bytes when there is a buffer; without one the result is range.length.
OCFIndex OCFStringGetBytes(
    OCFStringRef theString,
    OCFRange range,
    OCFStringEncoding encoding,
    UInt8 lossByte,
    Boolean isExternalRepresentation,
    UInt8 *buffer,
    OCFIndex maxBufLen,
    OCFIndex *usedBufLen
) {
    OCFRange remaining;
    if (stringIsTagged(theString)) {
        if (lossByte != 0) {
            goto encode;
        }
        remaining = range;
        if (stringTagIndex(theString) == kStringTagInline &&
            (encoding == 0x201 || encoding == 0x8000100 || encoding == 0x600)) {
            if (buffer == NULL) {
                OCFIndex difference = _ONSTaggedPointerStringGetLength(theString) - range.length;
                if (difference < 0 || range.location > difference) {
                    goto message;
                }
                goto stored;
            }
            UInt8 bytes[16];
            OCFIndex difference = _ONSTaggedPointerStringGetBytes(theString, bytes) - range.length;
            if (difference < 0 || range.location > difference) {
                goto message;
            }
            if (range.length >= maxBufLen) {
                range.length = maxBufLen;
            }
            memmove(buffer, bytes + range.location, range.length);
            goto stored;
        }
        goto message;
    }
    if (__CF_IS_OBJC(7, theString) != 0) {
        if (lossByte != 0) {
            goto encode;
        }
        remaining = range;
        goto message;
    }
    if (stringIsUnicode(theString)) {
        goto encode;
    }
    if (stringEightBitEncoding() != encoding) {
        if (stringEightBitEncoding() != 0x600 || !stringEncodingAcceptsASCIIContents(encoding)) {
            goto encode;
        }
    }
    if (buffer != NULL) {
        if (range.length >= maxBufLen) {
            range.length = maxBufLen;
        }
        const uint8_t *contents = stringContents(theString);
        memmove(buffer, contents + range.location + ((stringInfo(theString) >> 2) & 1), range.length);
    }
stored:
    if (usedBufLen != NULL) {
        *usedBufLen = range.length;
    }
    return range.length;
message:
    {
        unsigned long nsEncoding = OCFStringConvertEncodingToNSStringEncoding(encoding);
        bool converted = ((bool (*)(
            id, SEL, void *, OCFIndex, OCFIndex *, unsigned long, unsigned long, OCFRange, OCFRange *
        ))objc_msgSend)(
            (id)theString,
            sel_getUid("getBytes:maxLength:usedLength:encoding:options:range:remainingRange:"),
            buffer,
            maxBufLen,
            usedBufLen,
            nsEncoding,
            isExternalRepresentation ? 2 : 0,
            range,
            &remaining
        );
        return converted ? remaining.location - range.location : 0;
    }
encode:
    return __OCFStringEncodeByteStream(
        theString,
        range.location,
        range.length,
        isExternalRepresentation,
        encoding,
        (char)lossByte,
        buffer,
        maxBufLen,
        usedBufLen
    );
}

// Native CFStringGetPascalStringPtr, 0x1c0210120. External.
// NULL for a tagged pointer, an Objective-C object, a string without info bit 2 (the raw bit; TBZ #2), an
// Unicode string, or an encoding that does not accept the eight-bit contents. A string with a length byte
// ((info & 5) == 4) answers its contents; any other string answers them only when its length equals the first
// content byte (re-testing (info & 5) == 4 first, as natively).
const unsigned char *OCFStringGetPascalStringPtr(OCFStringRef theString, OCFStringEncoding encoding) {
    if (stringIsTagged(theString)) {
        return NULL;
    }
    if (__CF_IS_OBJC(7, theString) != 0) {
        return NULL;
    }
    if ((stringInfo(theString) & kStringInfoLengthByte) == 0) {
        return NULL;
    }
    if (stringIsUnicode(theString)) {
        return NULL;
    }
    if (stringEightBitEncoding() != encoding) {
        if (stringEightBitEncoding() != 0x600 || !stringEncodingAcceptsASCIIContents(encoding)) {
            return NULL;
        }
    }
    const unsigned char *contents = stringContents(theString);
    if (stringHasLengthByte(theString)) {
        return contents;
    }
    OCFIndex length;
    if (stringHasLengthByte(theString)) {
        length = contents[0];
    } else if ((stringInfo(theString) & kStringInfoContentsMask) != 0) {
        length = theString->variants.notInlineImmutable1.length;
    } else {
        length = theString->variants.inline1.length;
    }
    return length == contents[0] ? contents : NULL;
}

// Native _CFNonObjCStringGetCStringPtr, 0x1c0210350. Hidden (was private external). A tail call (B) to
// _CFStringGetCStringPtrInternal with the same three arguments and requiresBMP false (MOV W3, #0).
const char *
_CFNonObjCStringGetCStringPtr(OCFStringRef str, OCFStringEncoding encoding, Boolean requiresNullTermination) {
    return _CFStringGetCStringPtrInternal(str, encoding, requiresNullTermination, false);
}

// Native _CFStringGetCStringPtrInternal, 0x1c0210358. Non-external.
// The third argument is requiresNullTermination; the fourth (named requiresBMP in the shared prototype)
// decides whether Objective-C objects are dispatched: when it is set, an inline tagged string (tag index 2)
// answers NULL at once and tagged pointers and Objective-C objects receive -_fastCStringContents: with the
// third argument. When it is clear, every object is read as a CF string (a tagged pointer would be
// dereferenced; no native caller passes one so). The encoding must be the eight-bit encoding, or the
// eight-bit encoding ASCII and `encoding` accept ASCII contents; a NULL string answers NULL. Without required
// termination, eight-bit contents are answered; otherwise, and for Unicode contents, the null-byte bit (0x8)
// must be set, and the contents (plus info bit 2) are answered even when they are Unicode (native behavior).
const char *_CFStringGetCStringPtrInternal(
    OCFStringRef str, OCFStringEncoding encoding, Boolean requiresNullTermination, Boolean requiresBMP
) {
    if (stringIsTagged(str) && requiresBMP && stringTagIndex(str) == kStringTagInline) {
        return NULL;
    }
    if (encoding != stringEightBitEncoding()) {
        if (stringEightBitEncoding() != 0x600) {
            return NULL;
        }
        if (!stringEncodingAcceptsASCIIContents(encoding)) {
            return NULL;
        }
    }
    if (str == NULL) {
        return NULL;
    }
    if (requiresBMP && (stringIsTagged(str) || __CF_IS_OBJC(7, str) != 0)) {
        return ((const char *(*)(id, SEL, bool))objc_msgSend)(
            (id)str, sel_getUid("_fastCStringContents:"), requiresNullTermination != 0
        );
    }
    if (requiresNullTermination || stringIsUnicode(str)) {
        if (!stringHasNullByte(str)) {
            return NULL;
        }
    }
    return (const char *)stringEightBitBytes(str);
}

// Native _CFStringGetCStringPtrWithOptions, 0x1c02105d4. External. A tail call (B) to
// _CFStringGetCStringPtrInternal with the same arguments.
const char *_OCFStringGetCStringPtrWithOptions(
    OCFStringRef string, OCFStringEncoding encoding, Boolean requiresNullTermination, Boolean requiresBMP
) {
    return _CFStringGetCStringPtrInternal(string, encoding, requiresNullTermination, requiresBMP);
}

// Native _CFStringGetUTF8PtrWithOptions, 0x1c02105d8. Hidden (was private external).
// Arguments as read from the assembly and both callers (___CFStringContainsNullCharacter and
// ___CFStringEncodeByteStream pass (string, &length, 0, 1)): the string, the length out pointer (X1, written
// only on the CF path), requiresNullTermination (W2) and the Objective-C dispatch flag (W3, as in
// _CFStringGetCStringPtrInternal). The eight-bit encoding must be ASCII, and the ASCII-contents test is
// applied to the eight-bit encoding itself (always accepted once it is ASCII). Tagged pointers and
// Objective-C objects receive -_fastUTF8StringContents:utf8Length: when the dispatch flag is set; an inline
// tagged string answers NULL first.
const char *_CFStringGetUTF8PtrWithOptions(
    OCFStringRef string, OCFIndex *utf8Length, Boolean requiresNullTermination, Boolean requiresBMP
) {
    if (stringIsTagged(string) && requiresBMP && stringTagIndex(string) == kStringTagInline) {
        return NULL;
    }
    if (stringEightBitEncoding() != 0x600) {
        return NULL;
    }
    bool accepted = stringEncodingAcceptsASCIIContents(stringEightBitEncoding());
    if (string == NULL || !accepted) {
        return NULL;
    }
    if (requiresBMP && (stringIsTagged(string) || __CF_IS_OBJC(7, string) != 0)) {
        return ((const char *(*)(id, SEL, bool, OCFIndex *))objc_msgSend)(
            (id)string,
            sel_getUid("_fastUTF8StringContents:utf8Length:"),
            requiresNullTermination != 0,
            utf8Length
        );
    }
    if (requiresNullTermination || stringIsUnicode(string)) {
        if (!stringHasNullByte(string)) {
            return NULL;
        }
    }
    *utf8Length = stringLength(string);
    return (const char *)stringEightBitBytes(string);
}

// Native CFStringGetCharactersPtr, 0x1c02108b8. External.
// Tagged pointers with tag 0 or 2 (indirect and inline tagged strings; the full tag with the extended tag
// decoded, tested as (tag & ~2) == 0) answer NULL; other tagged pointers and Objective-C objects receive
// -_fastCharacterContents. A CF string answers its contents when they are Unicode.
const UniChar *OCFStringGetCharactersPtr(OCFStringRef theString) {
    if (stringIsTagged(theString)) {
        if ((stringTaggedPointerTag(theString) & ~2u) == 0) {
            return NULL;
        }
    } else if (__CF_IS_OBJC(7, theString) == 0) {
        if (!stringIsUnicode(theString)) {
            return NULL;
        }
        return stringContents(theString);
    }
    return ((const UniChar *(*)(id, SEL))objc_msgSend)((id)theString, sel_getUid("_fastCharacterContents"));
}

// Native CFStringGetCString, 0x1c0210c8c. External.
// A buffer size below 1 answers false without writing. Tagged pointers and Objective-C objects receive
// -_getCString:maxLength:encoding: with bufferSize - 1. A CF string whose eight-bit contents can be returned
// as `encoding` is copied with memmove and terminated when its length is below bufferSize; otherwise
// __CFStringEncodeByteStream converts the whole string (no external representation, loss byte 0) into
// bufferSize - 1 bytes and succeeds only when every character was converted. Failures write a zero first
// byte.
Boolean
OCFStringGetCString(OCFStringRef theString, char *buffer, OCFIndex bufferSize, OCFStringEncoding encoding) {
    OCFIndex maximum = bufferSize - 1;
    if (maximum < 0) {
        return false;
    }
    if (stringIsTagged(theString) || __CF_IS_OBJC(7, theString) != 0) {
        return ((bool (*)(id, SEL, char *, OCFIndex, OCFStringEncoding))objc_msgSend)(
            (id)theString, sel_getUid("_getCString:maxLength:encoding:"), buffer, maximum, encoding
        );
    }
    const uint8_t *contents = stringContents(theString);
    OCFIndex length = stringLength(theString);
    if (stringIsEightBit(theString)) {
        bool direct = stringEightBitEncoding() == encoding;
        if (!direct && stringEightBitEncoding() == 0x600) {
            direct = stringEncodingAcceptsASCIIContents(encoding);
        }
        if (direct) {
            if (length >= bufferSize) {
                buffer[0] = 0;
                return false;
            }
            memmove(buffer, contents + ((stringInfo(theString) >> 2) & 1), length);
            buffer[length] = 0;
            return true;
        }
    }
    OCFIndex usedLength = 0;
    if (__OCFStringEncodeByteStream(
            theString, 0, length, false, encoding, 0, (UInt8 *)buffer, maximum, &usedLength
        ) == length) {
        buffer[usedLength] = 0;
        return true;
    }
    buffer[0] = 0;
    return false;
}

// The grapheme-extend bitmap test of the comparison: the BMP bitmap or the character's plane (set 0x6e), the
// bit of the character's low 16 bits.
static inline bool stringIsGraphemeExtend(UTF32Char character, const uint8_t *graphemeBMP) {
    const uint8_t *bitmap = graphemeBMP;
    if (character >= 0x10000) {
        bitmap = OCFUniCharGetBitmapPtrForPlane(0x6E, character >> 16);
    }
    if (bitmap == NULL) {
        return false;
    }
    return ((bitmap[(uint16_t)character >> 3] >> (character & 7)) & 1) != 0;
}

// The surrogate step of the main loop: a character whose bits 10-15 are 0xd800 (AND #0xFC00, also for a
// folded 32-bit character) is combined with a following low surrogate as ((character << 10) & 0x36ffc00) +
// low - 0x35fdc00 (32-bit), and the used length becomes 2.
static inline UTF32Char stringCombineSurrogates(
    UTF32Char character, OCFStringInlineBuffer *buffer, OCFIndex index, OCFIndex *usedLength
) {
    if ((character & 0xFC00) != 0xD800 || index < -1 || index + 1 >= buffer->rangeToBuffer.length) {
        return character;
    }
    UniChar low = stringGetCharacterFromInlineBuffer(buffer, index + 1);
    if ((low >> 10) != 0x37) {
        return character;
    }
    *usedLength = 2;
    return ((character << 10) & 0x36FFC00) + low - 0x35FDC00;
}

// The case step of the main loop for a character read from an inline buffer: 'A'-'Z' gains 0x20 unless a
// language code is present and the character is 'I', or forced ordering is on and no result is recorded yet.
static inline UTF32Char stringCompareFoldCase(
    UniChar character, OCFOptionFlags options, const uint8_t *langCode, OCFComparisonResult compareResult
) {
    if ((options & kOCFCompareCaseInsensitive) == 0 || (UniChar)(character - 'A') > 25) {
        return character;
    }
    if (langCode != NULL && character == 'I') {
        return 'I';
    }
    if (compareResult != kOCFCompareEqualTo || (options & kOCFCompareForcedOrdering) == 0) {
        return character | 0x20;
    }
    return character;
}

// The trailing skip of ignorable characters after the main loop, for one string: stop at ASCII when no
// ignored-character set is in use; combine a surrogate pair (16-bit test here); skip grapheme-extend
// characters when diacritics are ignored, then members of the ignored set.
static inline OCFIndex stringSkipIgnorable(
    OCFStringInlineBuffer *buffer,
    OCFIndex index,
    OCFIndex limit,
    bool diacriticsInsensitive,
    bool hasIgnoredCharacters,
    OCFCharacterSetInlineBuffer *ignoredCharacters,
    const uint8_t *graphemeBMP
) {
    while (index < limit) {
        UTF32Char character = stringGetCharacterFromInlineBuffer(buffer, index);
        if (character < 0x80 && !hasIgnoredCharacters) {
            break;
        }
        if ((character >> 10) == 0x36 && index >= -1 && index + 1 < buffer->rangeToBuffer.length) {
            UniChar low = stringGetCharacterFromInlineBuffer(buffer, index + 1);
            if ((low >> 10) == 0x37) {
                character = (character << 10) + low - 0x35FDC00;
            }
        }
        if (!diacriticsInsensitive || !stringIsGraphemeExtend(character, graphemeBMP)) {
            if (!hasIgnoredCharacters || !stringCharacterSetIsMember(ignoredCharacters, character)) {
                break;
            }
        }
        index += character < 0x10000 ? 1 : 2;
    }
    return index;
}

// Native CFStringCompareWithOptionsAndLocale, 0x1c0210fa8. External.
// Order of work, as natively: the length of the second string; kCFCompareLocalized without a locale copies
// the current locale (released before every return that follows, when it is not NULL); the language code of
// _CFStrGetSpecialCaseHandlingLanguageIdentifierForLocale(locale, true); the zeroed inline buffers and
// character-set buffer; __CFStringFillCharacterSetInlineBuffer. Equality options are 0x191 (case, nonliteral,
// diacritic, width) or a filled set.
// Without kCFCompareNumerically, a locale or a filled set, both strings' _CFStringGetCStringPtrInternal(...,
// eight-bit encoding, 0, 1) are tried: with both, nonliteral and numeric are cleared from the options; an
// ASCII eight-bit encoding without forced ordering compares case-insensitively through
// __ASCII_LOWERCASE_TABLE (byte & 0x7f) or with memcmp; otherwise without case or diacritic insensitivity
// memcmp, and with them an ASCII byte loop that records the forced-ordering result, folds 'A'-'Z' when
// case-insensitive, and leaves to the main loop (at the first non-ASCII byte) with its index and result.
// Length differences on these paths are 32-bit ((int)length - (int)length2). Without equality options and
// with neither C-string pointer, two CFStringGetCharactersPtr buffers are compared as UTF-16 units with a
// 64-bit difference. The main loop reads both strings through inline buffers initialized with
// CFStringGetCharactersPtr and _CFStringGetCStringPtrInternal(..., 0x600, 0, 1); with a locale, forced
// ordering clears the diacritic option (and diacritic insensitivity). Numeric runs of ASCII digits compare as
// unsigned 64-bit values. Folding
// (__CFStringFoldCharacterClusterAtIndex) that reports insufficient buffer space records the end of the
// character cluster (CFStringGetRangeOfCharacterClusterAtIndex(string, index, 1) on the original string with
// the inline-buffer index); no further fold is attempted on that side until its index reaches that end, and
// characters that differ while neither side may fold count as equal (native behavior, kept).
// Reading of the whole body from the assembly; the inlined inline-buffer reads match
// stringGetCharacterFromInlineBuffer (byte reads sign-extend, then truncate to 16 bits).
OCFComparisonResult OCFStringCompareWithOptionsAndLocale(
    OCFStringRef theString1,
    OCFStringRef theString2,
    OCFRange rangeToCompare,
    OCFStringCompareFlags compareOptions,
    OCFLocaleRef locale
) {
    OCFIndex str2UsedLen = 0;
    OCFIndex str1UsedLen = 0;
    OCFIndex length = rangeToCompare.length;
    OCFIndex location = rangeToCompare.location;
    OCFIndex string2Len = OCFStringGetLength(theString2);
    bool freeLocale = locale == NULL && ((compareOptions >> 5) & 1) != 0;
    if (freeLocale) {
        locale = OCFLocaleCopyCurrent();
    }
    OCFOptionFlags equalityOptions = compareOptions & 0x191;
    const uint8_t *langCode = NULL;
    if (locale != NULL) {
        langCode = (const uint8_t *)_CFStrGetSpecialCaseHandlingLanguageIdentifierForLocale(locale, true);
    }
    UTF32Char strBuf2[64];
    UTF32Char strBuf1[64];
    OCFStringInlineBuffer inlineBuf2;
    OCFStringInlineBuffer inlineBuf1;
    memset(&inlineBuf2, 0, sizeof(inlineBuf2));
    memset(&inlineBuf1, 0, sizeof(inlineBuf1));
    OCFCharacterSetInlineBuffer csetBuffer;
    memset(&csetBuffer, 0, sizeof(csetBuffer));
    bool hasIgnoredCharacters = __CFStringFillCharacterSetInlineBuffer(&csetBuffer, compareOptions) != 0;
    bool equality = equalityOptions != 0 || hasIgnoredCharacters;
    OCFComparisonResult compareResult = kOCFCompareEqualTo;
    OCFIndex startIndex = 0;
    OCFOptionFlags options = compareOptions;

    if ((compareOptions & kOCFCompareNumerically) == 0 && locale == NULL && !hasIgnoredCharacters) {
        OCFStringEncoding eightBitEncoding = stringEightBitEncoding();
        const uint8_t *str1Bytes =
            (const uint8_t *)_CFStringGetCStringPtrInternal(theString1, eightBitEncoding, false, true);
        const uint8_t *str2Bytes =
            (const uint8_t *)_CFStringGetCStringPtrInternal(theString2, eightBitEncoding, false, true);
        OCFIndex factor = 1;
        if (str1Bytes != NULL && str2Bytes != NULL) {
            options = compareOptions & ~(OCFOptionFlags)0x50;
            if ((compareOptions & kOCFCompareForcedOrdering) == 0 && eightBitEncoding == 0x600) {
                if ((compareOptions & kOCFCompareCaseInsensitive) != 0) {
                    OCFIndex count = length < string2Len ? length : string2Len;
                    int difference = 0;
                    const uint8_t *bytes1 = str1Bytes + location;
                    if (count != 0) {
                        do {
                            difference = __ASCII_LOWERCASE_TABLE[*bytes1++ & 0x7F] -
                                         __ASCII_LOWERCASE_TABLE[*str2Bytes++ & 0x7F];
                            if (difference != 0) {
                                break;
                            }
                        } while (--count != 0);
                    }
                    if (difference == 0) {
                        difference = (int)length - (int)string2Len;
                    }
                    return difference > 0 ? kOCFCompareGreaterThan
                                          : (difference < 0 ? kOCFCompareLessThan : kOCFCompareEqualTo);
                }
            } else if (
                (compareOptions & (kOCFCompareCaseInsensitive | kOCFCompareDiacriticInsensitive)) != 0
            ) {
                OCFIndex limitLength = length < string2Len ? length : string2Len;
                str1Bytes += location;
                OCFIndex index = 0;
                if (limitLength >= 1) {
                    for (; index < limitLength; ++index) {
                        uint32_t str1Char = str1Bytes[index];
                        uint32_t str2Char = str2Bytes[index];
                        if (str1Char == str2Char) {
                            continue;
                        }
                        if (((str1Char | str2Char) & 0x80) != 0) {
                            str1Bytes = NULL;
                            break;
                        }
                        if (compareResult == kOCFCompareEqualTo && ((compareOptions >> 9) & 1) != 0) {
                            compareResult =
                                str1Char >= str2Char ? kOCFCompareGreaterThan : kOCFCompareLessThan;
                        }
                        if ((compareOptions & kOCFCompareCaseInsensitive) != 0) {
                            if (str1Char - 'A' < 26) {
                                str1Char |= 0x20;
                            }
                            if (str2Char - 'A' < 26) {
                                str2Char |= 0x20;
                            }
                        }
                        if (str1Char != str2Char) {
                            return str1Char >= str2Char ? kOCFCompareGreaterThan : kOCFCompareLessThan;
                        }
                    }
                }
                startIndex = index;
                if (index == limitLength) {
                    int difference = (int)length - (int)string2Len;
                    if (difference == 0) {
                        return compareResult;
                    }
                    return difference >= 0 ? kOCFCompareGreaterThan : kOCFCompareLessThan;
                }
            }
        } else if (!equality && str1Bytes == NULL && str2Bytes == NULL) {
            const UniChar *characters1 = OCFStringGetCharactersPtr(theString1);
            const UniChar *characters2 = OCFStringGetCharactersPtr(theString2);
            str1Bytes = (const uint8_t *)characters1;
            str2Bytes = (const uint8_t *)characters2;
            factor = 2;
            if (characters1 != NULL && characters2 != NULL) {
                OCFIndex difference = length - string2Len;
                OCFIndex count = length < string2Len ? length : string2Len;
                if (count >= 1) {
                    const UniChar *unit1 = characters1 + location;
                    const UniChar *end1 = unit1 + count;
                    UniChar first;
                    UniChar second;
                    do {
                        first = *unit1++;
                        second = *characters2++;
                    } while (first == second && unit1 < end1);
                    if (first != second) {
                        difference = (OCFIndex)first - (OCFIndex)second;
                    }
                }
                return difference == 0 ? kOCFCompareEqualTo : ((difference >> 63) | 1);
            }
        }
        // The memcmp path (also reached with a negative limit after the byte loop, whose first pointer is
        // then already offset by the location; kept as native).
        if (str1Bytes != NULL && str2Bytes != NULL) {
            OCFIndex count = length < string2Len ? length : string2Len;
            int difference = memcmp(str1Bytes + factor * location, str2Bytes, (size_t)(factor * count));
            if (difference == 0) {
                difference = (int)length - (int)string2Len;
            }
            return difference > 0 ? kOCFCompareGreaterThan
                                  : (difference < 0 ? kOCFCompareLessThan : kOCFCompareEqualTo);
        }
    }

    const uint8_t *graphemeBMP = OCFUniCharGetBitmapPtrForPlane(0x6E, 0);
    inlineBuf1.theString = theString1;
    inlineBuf1.rangeToBuffer = rangeToCompare;
    inlineBuf1.directUniCharBuffer = OCFStringGetCharactersPtr(theString1);
    inlineBuf1.directCStringBuffer = inlineBuf1.directUniCharBuffer != NULL
                                         ? NULL
                                         : _CFStringGetCStringPtrInternal(theString1, 0x600, false, true);
    inlineBuf1.bufferedRangeStart = 0;
    inlineBuf1.bufferedRangeEnd = 0;
    inlineBuf2.theString = theString2;
    inlineBuf2.rangeToBuffer.location = 0;
    inlineBuf2.rangeToBuffer.length = string2Len;
    inlineBuf2.directUniCharBuffer = OCFStringGetCharactersPtr(theString2);
    inlineBuf2.directCStringBuffer = inlineBuf2.directUniCharBuffer != NULL
                                         ? NULL
                                         : _CFStringGetCStringPtrInternal(theString2, 0x600, false, true);
    inlineBuf2.bufferedRangeStart = 0;
    inlineBuf2.bufferedRangeEnd = 0;

    bool diacriticsInsensitive;
    OCFIndex str1LocalizedIndex;
    if (locale != NULL) {
        diacriticsInsensitive = (compareOptions & kOCFCompareForcedOrdering) == 0 &&
                                (compareOptions & kOCFCompareDiacriticInsensitive) != 0;
        if ((compareOptions & kOCFCompareForcedOrdering) != 0) {
            options &= ~(OCFOptionFlags)kOCFCompareDiacriticInsensitive;
        }
        str1LocalizedIndex = startIndex;
    } else {
        diacriticsInsensitive = (compareOptions & kOCFCompareDiacriticInsensitive) != 0;
        str1LocalizedIndex = 0;
    }
    OCFIndex str2LocalizedIndex = str1LocalizedIndex;
    bool caseInsensitive = (compareOptions & kOCFCompareCaseInsensitive) != 0;
    bool numerically = (compareOptions & kOCFCompareNumerically) != 0;
    bool forceOrdering = (compareOptions & kOCFCompareForcedOrdering) != 0;
    OCFIndex str1Index = startIndex;
    OCFIndex str2Index = startIndex;
    OCFIndex strBuf1Len = 0;
    OCFIndex strBuf2Len = 0;
    OCFIndex strBuf1Index = 0;
    OCFIndex strBuf2Index = 0;
    OCFIndex forcedIndex1 = 0;
    OCFIndex forcedIndex2 = 0;
    OCFIndex cluster1End = 0;
    OCFIndex cluster2End = 0;
    bool numericEquivalence = false;
    OCFComparisonResult result;

    while (str1Index < length && str2Index < string2Len) {
        UTF32Char str1Char;
        UTF32Char str2Char;
        if (strBuf1Len == 0) {
            str1Char = stringCompareFoldCase(
                stringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index),
                compareOptions,
                langCode,
                compareResult
            );
            str1UsedLen = 1;
        } else {
            str1Char = strBuf1[strBuf1Index++];
        }
        if (strBuf2Len == 0) {
            str2Char = stringCompareFoldCase(
                stringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index),
                compareOptions,
                langCode,
                compareResult
            );
            str2UsedLen = 1;
        } else {
            str2Char = strBuf2[strBuf2Index++];
        }

        if (strBuf2Len == 0 && numerically && strBuf1Len == 0 && str1Char - '0' <= 9 && str2Char - '0' <= 9) {
            uint64_t intValue1 = 0;
            uint64_t intValue2 = 0;
            OCFIndex str1NumRangeIndex = str1Index;
            OCFIndex str2NumRangeIndex = str2Index;
            do {
                intValue1 = intValue1 * 10 + (str1Char - '0');
                str1Char = stringGetCharacterFromInlineBuffer(&inlineBuf1, ++str1Index);
            } while ((UniChar)(str1Char - '0') < 10);
            do {
                intValue2 = intValue2 * 10 + (str2Char - '0');
                str2Char = stringGetCharacterFromInlineBuffer(&inlineBuf2, ++str2Index);
            } while ((UniChar)(str2Char - '0') < 10);
            if (intValue1 != intValue2) {
                if (freeLocale && locale != NULL) {
                    OCFRelease(locale);
                }
                return intValue1 >= intValue2 ? kOCFCompareGreaterThan : kOCFCompareLessThan;
            }
            if (forceOrdering && compareResult == kOCFCompareEqualTo) {
                OCFIndex digits1 = str1Index - str1NumRangeIndex;
                OCFIndex digits2 = str2Index - str2NumRangeIndex;
                if (digits1 != digits2) {
                    compareResult = digits1 < digits2 ? kOCFCompareLessThan : kOCFCompareGreaterThan;
                    numericEquivalence = true;
                    forcedIndex2 = str2NumRangeIndex;
                    forcedIndex1 = str1NumRangeIndex;
                }
            }
            continue;
        }

        if (str1Char != str2Char) {
            if (!equality) {
                if (locale != NULL) {
                    result = _CFCompareStringsWithLocale(
                        &inlineBuf1,
                        (OCFRange){str1Index, length - str1Index},
                        &inlineBuf2,
                        (OCFRange){str2Index, string2Len - str2Index},
                        options,
                        locale
                    );
                    goto releaseAndReturn;
                }
                return str1Char >= str2Char ? kOCFCompareGreaterThan : kOCFCompareLessThan;
            }
            OCFComparisonResult characterResult =
                str1Char < str2Char ? kOCFCompareLessThan : kOCFCompareGreaterThan;
            if (compareResult == kOCFCompareEqualTo && forceOrdering) {
                compareResult = characterResult;
                forcedIndex2 = str2LocalizedIndex;
                forcedIndex1 = str1LocalizedIndex;
            }
            if (str1Char <= 0x7F && str2Char <= 0x7F && !hasIgnoredCharacters) {
                if (locale != NULL) {
                    result = _CFCompareStringsWithLocale(
                        &inlineBuf1,
                        (OCFRange){str1Index, length - str1Index},
                        &inlineBuf2,
                        (OCFRange){str2Index, string2Len - str2Index},
                        options,
                        locale
                    );
                    goto releaseAndReturn;
                }
                if (!caseInsensitive) {
                    return characterResult;
                }
            }

            str1Char = stringCombineSurrogates(str1Char, &inlineBuf1, str1Index, &str1UsedLen);
            str2Char = stringCombineSurrogates(str2Char, &inlineBuf2, str2Index, &str2UsedLen);

            if (hasIgnoredCharacters) {
                if (stringCharacterSetIsMember(&csetBuffer, str1Char)) {
                    if (strBuf1Len > 0 && strBuf1Index == strBuf1Len) {
                        strBuf1Len = 0;
                    }
                    if (strBuf1Len == 0) {
                        str1Index += str1UsedLen;
                    }
                    if (strBuf2Len > 0) {
                        --strBuf2Index;
                    }
                    continue;
                }
                if (stringCharacterSetIsMember(&csetBuffer, str2Char)) {
                    if (strBuf2Len > 0 && strBuf2Index == strBuf2Len) {
                        strBuf2Len = 0;
                    }
                    if (strBuf2Len == 0) {
                        str2Index += str2UsedLen;
                    }
                    if (str1Index < length && strBuf1Len > 0) {
                        --strBuf1Index;
                    }
                    continue;
                }
            }

            if (diacriticsInsensitive && str1Index > 0) {
                bool str1Skip = false;
                bool str2Skip = false;
                if (strBuf1Len == 0 && stringIsGraphemeExtend(str1Char, graphemeBMP)) {
                    str1Char = str2Char;
                    str1Skip = true;
                }
                if (strBuf2Len == 0 && stringIsGraphemeExtend(str2Char, graphemeBMP)) {
                    str2Char = str1Char;
                    str2Skip = true;
                }
                if (str1Skip != str2Skip) {
                    if (str1Skip) {
                        str2Index -= str2UsedLen;
                    }
                    if (str2Skip) {
                        str1Index -= str1UsedLen;
                    }
                }
            }

            if (str1Char != str2Char) {
                if (strBuf1Len == 0) {
                    if (cluster1End == 0 || cluster1End == str1Index) {
                        bool insufficientBufferSpace = false;
                        strBuf1Len = __CFStringFoldCharacterClusterAtIndex(
                            str1Char,
                            &inlineBuf1,
                            str1Index,
                            options,
                            langCode,
                            strBuf1,
                            &str1UsedLen,
                            &insufficientBufferSpace
                        );
                        if (strBuf1Len > 0) {
                            strBuf1Index = 1;
                            str1Char = strBuf1[0];
                        }
                        cluster1End = 0;
                        if (insufficientBufferSpace == 1) {
                            OCFRange cluster =
                                OCFStringGetRangeOfCharacterClusterAtIndex(theString1, str1Index, 1);
                            cluster1End = cluster.location + cluster.length;
                        }
                    }
                    if (strBuf1Len == 0 && strBuf2Len >= 1) {
                        goto localizedResult;
                    }
                }
                if (strBuf2Len == 0) {
                    if (strBuf1Len == 0 || str1Char != str2Char) {
                        if (cluster2End == 0 || cluster2End == str2Index) {
                            bool insufficientBufferSpace = false;
                            OCFIndex folded = __CFStringFoldCharacterClusterAtIndex(
                                str2Char,
                                &inlineBuf2,
                                str2Index,
                                options,
                                langCode,
                                strBuf2,
                                &str2UsedLen,
                                &insufficientBufferSpace
                            );
                            if (folded > 0) {
                                strBuf2Index = 1;
                                str2Char = strBuf2[0];
                            }
                            if (folded == 0 || str1Char != str2Char) {
                                goto localizedResult;
                            }
                            strBuf2Len = folded;
                            cluster2End = 0;
                            if (insufficientBufferSpace == 1) {
                                OCFRange cluster =
                                    OCFStringGetRangeOfCharacterClusterAtIndex(theString2, str2Index, 1);
                                cluster2End = cluster.location + cluster.length;
                            }
                        }
                    }
                    if (strBuf2Len == 0) {
                        goto advance;
                    }
                }
            }

            if (strBuf1Len >= 1 && strBuf2Len >= 1) {
                while (strBuf1Index < strBuf1Len && strBuf2Index < strBuf2Len) {
                    if (strBuf1[strBuf1Index] != strBuf2[strBuf2Index]) {
                        break;
                    }
                    ++strBuf1Index;
                    ++strBuf2Index;
                }
                if (strBuf1Index < strBuf1Len && strBuf2Index < strBuf2Len) {
                    str1Char = strBuf1[strBuf1Index];
                    str2Char = strBuf2[strBuf2Index];
                    goto localizedResult;
                }
            }
        }

    advance:
        if (strBuf1Len > 0 && strBuf1Index == strBuf1Len) {
            strBuf1Len = 0;
        }
        if (strBuf2Len > 0 && strBuf2Index == strBuf2Len) {
            strBuf2Len = 0;
        }
        if (strBuf1Len == 0) {
            str1Index += str1UsedLen;
        }
        if (strBuf2Len == 0) {
            str2Index += str2UsedLen;
        }
        if (strBuf1Len == 0 && strBuf2Len == 0) {
            str1LocalizedIndex = str1Index;
            str2LocalizedIndex = str2Index;
        }
        continue;

    localizedResult:
        if (locale != NULL) {
            result = _CFCompareStringsWithLocale(
                &inlineBuf1,
                (OCFRange){str1LocalizedIndex, length - str1LocalizedIndex},
                &inlineBuf2,
                (OCFRange){str2LocalizedIndex, string2Len - str2LocalizedIndex},
                options,
                locale
            );
            goto releaseAndReturn;
        }
        return str1Char >= str2Char ? kOCFCompareGreaterThan : kOCFCompareLessThan;
    }

    if (hasIgnoredCharacters || diacriticsInsensitive) {
        str1Index = stringSkipIgnorable(
            &inlineBuf1,
            str1Index,
            length,
            diacriticsInsensitive,
            hasIgnoredCharacters,
            &csetBuffer,
            graphemeBMP
        );
        if (str2Index < string2Len) {
            str2Index = stringSkipIgnorable(
                &inlineBuf2,
                str2Index,
                string2Len,
                diacriticsInsensitive,
                hasIgnoredCharacters,
                &csetBuffer,
                graphemeBMP
            );
        }
    }

    if (locale != NULL && !numericEquivalence && compareResult != kOCFCompareEqualTo && str1Index == length &&
        str2Index == string2Len) {
        compareResult = _CFCompareStringsWithLocale(
            &inlineBuf1,
            (OCFRange){forcedIndex1, length - forcedIndex1},
            &inlineBuf2,
            (OCFRange){forcedIndex2, string2Len - forcedIndex2},
            options,
            locale
        );
    }
    if (locale != NULL && freeLocale) {
        OCFRelease(locale);
    }
    if (str1Index < length) {
        return kOCFCompareGreaterThan;
    }
    if (str2Index < string2Len) {
        return kOCFCompareLessThan;
    }
    return compareResult;

releaseAndReturn:
    if (freeLocale) {
        OCFRelease(locale);
    }
    return result;
}

// The CFUniChar bitmap membership test the native code inlines: bit (character & 7) of byte
// ((character & 0xFFFF) >> 3).
static inline bool isMemberOfBitmap(const uint8_t *bitmap, UTF32Char character) {
    return ((bitmap[(uint16_t)character >> 3] >> (character & 7)) & 1) != 0;
}

// The inlined CFUniCharGetCombiningPropertyForCharacter: a zero page byte means class 0 (callers branch on
// the page byte itself, which this helper reports through *hasPage).
static inline uint8_t combiningClassForCharacter(const uint8_t *bitmap, UniChar character, bool *hasPage) {
    uint8_t page = bitmap[character >> 8];
    *hasPage = page != 0;
    if (page == 0) {
        return 0;
    }
    return bitmap[((uint32_t)page << 8) - 0x100 + 0x100 + (character & 0xFF)];
}

// Native _CFStrGetSpecialCaseHandlingLanguageIdentifierForLocale, 0x1c0212d9c. Non-external. The native C
// name has one leading underscore (nm _CFStrGetSpecialCaseHandlingLanguageIdentifierForLocale); the
// temporary prelude spells the callers' prototype with two, which the merge renames.
// A locale that needs no special case handling answers NULL. The cached locale is read under the lock, but
// the cached identifier is read after the unlock. The identifier is the locale's collator identifier
// (__kCFLocaleCollatorID) when collatorOnly is set, else the locale identifier; shorter than two characters
// answers NULL. Its first two UTF-8 bytes (through _CFStringGetCStringPtrInternal(identifier, UTF-8, true,
// true), else CFStringGetBytes of the range {0, 2}, which must produce exactly two bytes) select "az", "el",
// "lt", "nl" or "tr"; any other pair answers NULL without updating the cache. A match is cached under the
// lock (locale, then identifier). The returned pointers are the image's C string literals.
const char *_CFStrGetSpecialCaseHandlingLanguageIdentifierForLocale(OCFLocaleRef locale, bool collatorOnly) {
    static OCFLocaleRef lastLocale = NULL;
    static const char *lastLangID = NULL;
    static os_unfair_lock lock = OS_UNFAIR_LOCK_INIT;
    if (__CFLocaleGetDoesNotRequireSpecialCaseHandling(locale)) {
        return NULL;
    }
    os_unfair_lock_lock_with_options(&lock, 0x10000);
    OCFLocaleRef cachedLocale = lastLocale;
    os_unfair_lock_unlock(&lock);
    if (cachedLocale != NULL && cachedLocale == locale) {
        return lastLangID;
    }
    OCFStringRef identifier;
    if (collatorOnly) {
        identifier = (OCFStringRef)OCFLocaleGetValue(locale, __kCFLocaleCollatorID);
    } else {
        identifier = OCFLocaleGetIdentifier(locale);
    }
    if (OCFStringGetLength(identifier) < 2) {
        return NULL;
    }
    UInt8 buffer[2];
    const char *langID = _CFStringGetCStringPtrInternal(identifier, 0x8000100, true, true);
    if (langID == NULL) {
        OCFIndex usedBufLen = 0;
        langID = (const char *)buffer;
        OCFRange range = {0, 2};
        if (OCFStringGetBytes(identifier, range, 0x8000100, 0, false, buffer, 2, &usedBufLen) != 2) {
            return NULL;
        }
    }
    const char *result;
    uint8_t first = (uint8_t)langID[0];
    uint8_t second = (uint8_t)langID[1];
    if (first <= 'k') {
        if (first == 'a') {
            if (second != 'z') {
                return NULL;
            }
            result = "az";
        } else if (first == 'e') {
            if (second != 'l') {
                return NULL;
            }
            result = "el";
        } else {
            return NULL;
        }
    } else if (first == 'l') {
        if (second != 't') {
            return NULL;
        }
        result = "lt";
    } else if (first == 'n') {
        if (second != 'l') {
            return NULL;
        }
        result = "nl";
    } else if (first == 't') {
        if (second != 'r') {
            return NULL;
        }
        result = "tr";
    } else {
        return NULL;
    }
    os_unfair_lock_lock_with_options(&lock, 0x10000);
    lastLocale = locale;
    lastLangID = result;
    os_unfair_lock_unlock(&lock);
    return result;
}

// Native __CFStringFillCharacterSetInlineBuffer, 0x1c0212fa8. Non-external.
// Only when compareOptions has bit 16 (0x10000) set: the shared inverted copy of the predefined set 10
// (alphanumerics, CFCharacterSetGetPredefined(10) copied with the system default allocator and inverted) is
// published once with a compare-and-swap (CASAL against NULL; the loser releases its copy) and the buffer is
// initialized from it. Answers bit 16. The buffer's fields are not read here, so the
// OCFCharacterSetInlineBuffer layout is not confirmed by this function.
Boolean __CFStringFillCharacterSetInlineBuffer(
    OCFCharacterSetInlineBuffer *buffer, OCFStringCompareFlags compareOptions
) {
    static OCFCharacterSetRef nonAlnumChars = NULL;
    if ((compareOptions & 0x10000) != 0) {
        if (__atomic_load_n(&nonAlnumChars, __ATOMIC_RELAXED) == NULL) {
            OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
            OCFMutableCharacterSetRef set =
                OCFCharacterSetCreateMutableCopy(allocator, OCFCharacterSetGetPredefined(10));
            OCFCharacterSetInvert(set);
            OCFCharacterSetRef expected = NULL;
            if (!__atomic_compare_exchange_n(
                    &nonAlnumChars,
                    &expected,
                    (OCFCharacterSetRef)set,
                    false,
                    __ATOMIC_SEQ_CST,
                    __ATOMIC_SEQ_CST
                )) {
                OCFRelease(set);
            }
        }
        OCFCharacterSetInitInlineBuffer(__atomic_load_n(&nonAlnumChars, __ATOMIC_RELAXED), buffer);
    }
    return (Boolean)((compareOptions >> 16) & 1);
}

// Native __CFStringFoldCharacterClusterAtIndex, 0x1c0213038. Non-external.
// The native function takes eight arguments (no output capacity: 64 UTF-32 characters are assumed,
// decomposition passes 64, and 63 after the first character). Flags: 0x1 case-insensitive, 0x10 non-literal,
// 0x80 diacritic-insensitive, 0x100 width-insensitive; 0x90 selects decomposition and the final non-base
// filtering. Bitmaps: 9 canonical decomposable, 0x6e grapheme extend, 0x67 has non-self lowercase, 0x6a has
// non-self case folding (plane-0 bitmaps cached in function statics). The Turkic dotted capital I is handled
// for "tr" and "az". Case folding maps through an eight-unit UTF-16 buffer (CFUniCharMapCaseTo context 3,
// flags 0) whose returned count is used as a 32-bit value. In the final filtering, a Turkic capital I skips
// its first loop pass without advancing the index (the same character is examined again with the flag
// cleared), as natively.
OCFIndex __CFStringFoldCharacterClusterAtIndex(
    UTF32Char character,
    OCFStringInlineBuffer *buffer,
    OCFIndex index,
    OCFOptionFlags flags,
    const uint8_t *langCode,
    UTF32Char *outCharacters,
    OCFIndex *consumedLength,
    bool *insufficientBufferSpace
) {
    static const uint8_t *decompBMP = NULL;
    static const uint8_t *graphemeBMP = NULL;
    static const uint8_t *lowerBMP = NULL;
    static const uint8_t *caseFoldBMP = NULL;
    if (character == 0) {
        return 0;
    }
    OCFIndex filteredLength = 0;
    bool isTurkikCapitalI = false;
    uint32_t planeNo = character >> 16;
    if (decompBMP == NULL) {
        decompBMP = OCFUniCharGetBitmapPtrForPlane(9, 0);
        graphemeBMP = OCFUniCharGetBitmapPtrForPlane(0x6e, 0);
    }
    OCFIndex currentIndex = index + (planeNo != 0 ? 2 : 1);
    if (character <= 0x7F) {
        if (langCode != NULL && character == 'I') {
            planeNo = 0;
            goto decompose;
        }
        if ((flags & 1) != 0 && character - 'A' <= 25) {
            character |= 0x20;
            *outCharacters = character;
            filteredLength = 1;
        }
        goto filter;
    }
    if ((flags & 0x100) != 0 && character - 0xFF00 <= 0xEF) {
        filteredLength = 1;
        OCFUniCharCompatibilityDecompose(&character, 1, 1);
        *outCharacters = character;
    } else if (planeNo != 0) {
        goto decompose;
    }
    planeNo = 0;
    if ((character & 0xFC00) == 0xD800) {
        UniChar lowSurrogate = stringGetCharacterFromInlineBuffer(buffer, currentIndex);
        if ((lowSurrogate >> 10) == 0x37) {
            character = ((character & 0xFFFF) << 10) + lowSurrogate - 0x35FDC00;
            ++currentIndex;
            planeNo = character >> 16;
        }
    }

decompose:
    if ((flags & 0x90) != 0) {
        const uint8_t *bitmap = planeNo != 0 ? OCFUniCharGetBitmapPtrForPlane(9, planeNo) : decompBMP;
        if (bitmap != NULL && isMemberOfBitmap(bitmap, character)) {
            UTF32Char original = character;
            filteredLength = OCFUniCharDecomposeCharacter(character, outCharacters, 64);
            character = *outCharacters;
            if ((flags & 0x80) != 0 && character < 0x510) {
                filteredLength = 1;
            } else if ((flags & 0x10) != 0) {
                if (insufficientBufferSpace != NULL && filteredLength == 0) {
                    *insufficientBufferSpace = true;
                }
            } else {
                filteredLength = 0;
                character = original;
            }
        }
    }

    if ((flags & 1) == 0) {
        isTurkikCapitalI = false;
        goto filter;
    }
    {
        bool keepNonBase = (flags & 0x80) == 0 || character > 0x50F;
        if (lowerBMP == NULL) {
            lowerBMP = OCFUniCharGetBitmapPtrForPlane(0x67, 0);
            caseFoldBMP = OCFUniCharGetBitmapPtrForPlane(0x6a, 0);
        }
        if (langCode != NULL && character == 'I') {
            bool turkik =
                (langCode[0] == 'a' && langCode[1] == 'z') || (langCode[0] == 't' && langCode[1] == 'r');
            if (turkik && langCode[2] == 0) {
                if (filteredLength >= 2) {
                    if (outCharacters[1] == 0x307) {
                        if (filteredLength != 2) {
                            memmove(outCharacters + 1, outCharacters + 2, 4 * filteredLength - 8);
                        }
                        *outCharacters = 'i';
                        character = 'i';
                        isTurkikCapitalI = true;
                        --filteredLength;
                        goto filter;
                    }
                } else if (stringGetCharacterFromInlineBuffer(buffer, currentIndex) == 0x307) {
                    *outCharacters = 'i';
                    character = 'i';
                    ++currentIndex;
                    isTurkikCapitalI = true;
                    filteredLength = 1;
                    goto filter;
                }
            }
        }
        const uint8_t *bitmap = planeNo != 0 ? OCFUniCharGetBitmapPtrForPlane(0x67, planeNo) : lowerBMP;
        if (bitmap == NULL || !isMemberOfBitmap(bitmap, character)) {
            bitmap = planeNo != 0 ? OCFUniCharGetBitmapPtrForPlane(0x6a, planeNo) : caseFoldBMP;
            if (bitmap == NULL || !isMemberOfBitmap(bitmap, character)) {
                isTurkikCapitalI = false;
                goto filter;
            }
        }
        UniChar caseFoldBuffer[8];
        uint32_t bufferLength = (uint32_t)OCFUniCharMapCaseTo(character, caseFoldBuffer, 8, 3, 0, langCode);
        const UniChar *bufferLimit = caseFoldBuffer + bufferLength;
        if (filteredLength > 0) {
            --filteredLength;
        }
        if (filteredLength >= 1 && (bufferLength & 0xFFFFFFFE) != 0) {
            OCFIndex totalScalarLength = 0;
            if (bufferLength != 0) {
                const UniChar *bufferP = caseFoldBuffer;
                do {
                    const UniChar *next = bufferP + 1;
                    if ((*bufferP & 0xFC00) == 0xD800 && next < bufferLimit &&
                        (bufferP[1] & 0xFC00) == 0xDC00) {
                        next = bufferP + 2;
                    }
                    ++totalScalarLength;
                    bufferP = next;
                } while (bufferP < bufferLimit);
            }
            memmove(outCharacters + totalScalarLength, outCharacters + 1, filteredLength * sizeof(UTF32Char));
        }
        if (bufferLength != 0) {
            const UniChar *bufferP = caseFoldBuffer;
            UTF32Char *outCharactersP = outCharacters;
            do {
                const uint8_t *nonBaseBitmap;
                UniChar unit = *bufferP;
                const UniChar *next = bufferP + 1;
                character = unit;
                if ((unit & 0xFC00) == 0xD800 && next < bufferLimit && (*next & 0xFC00) == 0xDC00) {
                    bufferP += 2;
                    character = ((UTF32Char)unit << 10) + 0xFCA02400 + *next;
                    nonBaseBitmap = OCFUniCharGetBitmapPtrForPlane(0x6e, character >> 16);
                } else {
                    nonBaseBitmap = graphemeBMP;
                    bufferP = next;
                }
                if (nonBaseBitmap == NULL || keepNonBase || !isMemberOfBitmap(nonBaseBitmap, character)) {
                    *outCharactersP++ = character;
                    ++filteredLength;
                }
            } while (bufferP < bufferLimit);
        }
        isTurkikCapitalI = false;
    }

filter:
    if ((flags & 0x90) != 0) {
        bool keepNonBase = (flags & 0x80) == 0 || character > 0x50F;
        if (filteredLength == 0) {
            *outCharacters = character;
            if (!keepNonBase) {
                filteredLength = 0;
                goto extend;
            }
            UTF32Char nextCharacter = stringGetCharacterFromInlineBuffer(buffer, currentIndex);
            const uint8_t *nonBaseBitmap = graphemeBMP;
            const uint8_t *decompBitmap = decompBMP;
            if (currentIndex >= 0 && currentIndex < buffer->rangeToBuffer.length &&
                (nextCharacter >> 10) == 0x36 && currentIndex + 1 < buffer->rangeToBuffer.length) {
                UniChar lowSurrogate = stringGetCharacterFromInlineBuffer(buffer, currentIndex + 1);
                if ((lowSurrogate >> 10) == 0x37) {
                    nextCharacter = (nextCharacter << 10) + lowSurrogate - 0x35FDC00;
                    nonBaseBitmap = OCFUniCharGetBitmapPtrForPlane(0x6e, nextCharacter >> 16);
                    decompBitmap = OCFUniCharGetBitmapPtrForPlane(9, nextCharacter >> 16);
                }
            }
            if (nonBaseBitmap == NULL) {
                filteredLength = 0;
                goto extend;
            }
            if (!isMemberOfBitmap(nonBaseBitmap, nextCharacter)) {
                filteredLength = 0;
                keepNonBase = false;
                goto extend;
            }
            if ((flags & 0x80) != 0 && nextCharacter < 0x510) {
                filteredLength = 1;
            } else if (decompBitmap != NULL && isMemberOfBitmap(decompBitmap, nextCharacter)) {
                OCFIndex decomposedLength =
                    OCFUniCharDecomposeCharacter(nextCharacter, outCharacters + 1, 63);
                filteredLength = decomposedLength + 1;
                if (insufficientBufferSpace != NULL && decomposedLength == 0) {
                    filteredLength = 1;
                    *insufficientBufferSpace = true;
                }
            } else {
                outCharacters[1] = nextCharacter;
                filteredLength = 2;
            }
            keepNonBase = true;
            currentIndex += nonBaseBitmap == graphemeBMP ? 1 : 2;
        }
        if (filteredLength > 63) {
            goto overflow;
        }

    extend:
        for (;;) {
            const uint8_t *nonBaseBitmap = graphemeBMP;
            const uint8_t *decompBitmap = decompBMP;
            if (currentIndex < 0 || currentIndex >= buffer->rangeToBuffer.length) {
                character = 0;
            } else {
                UniChar unit = stringGetCharacterFromInlineBuffer(buffer, currentIndex);
                character = unit;
                if ((unit >> 10) == 0x36 && currentIndex + 1 < buffer->rangeToBuffer.length) {
                    UniChar lowSurrogate = stringGetCharacterFromInlineBuffer(buffer, currentIndex + 1);
                    if ((lowSurrogate >> 10) == 0x37) {
                        character = ((character & 0xFFFF) << 10) + lowSurrogate - 0x35FDC00;
                        nonBaseBitmap = OCFUniCharGetBitmapPtrForPlane(0x6e, character >> 16);
                        decompBitmap = OCFUniCharGetBitmapPtrForPlane(9, (uint16_t)(character >> 16));
                    }
                }
            }
            if (!isTurkikCapitalI) {
                if (nonBaseBitmap == NULL || !isMemberOfBitmap(nonBaseBitmap, character)) {
                    goto sort;
                }
                if (keepNonBase) {
                    if (decompBitmap != NULL && isMemberOfBitmap(decompBitmap, character)) {
                        OCFIndex decomposedLength = OCFUniCharDecomposeCharacter(
                            character, outCharacters + filteredLength, 64 - filteredLength
                        );
                        filteredLength += decomposedLength;
                        if (decomposedLength == 0) {
                            goto overflow;
                        }
                    } else {
                        outCharacters[filteredLength++] = character;
                    }
                } else if ((uint64_t)filteredLength <= 1) {
                    filteredLength = 1;
                }
                currentIndex += nonBaseBitmap == graphemeBMP ? 1 : 2;
            }
            isTurkikCapitalI = false;
            if (filteredLength >= 64) {
                goto overflow;
            }
        }

    overflow:
        if (insufficientBufferSpace != NULL) {
            *insufficientBufferSpace = true;
        }

    sort:
        if (filteredLength >= 2) {
            UTF32Char *sortCharacters = outCharacters + filteredLength - 1;
            OCFIndex sortLength = 1;
            while (outCharacters < sortCharacters) {
                UTF32Char sortCharacter = *sortCharacters;
                const uint8_t *nonBaseBitmap;
                if ((sortCharacter >> 16) != 0) {
                    nonBaseBitmap = OCFUniCharGetBitmapPtrForPlane(0x6e, sortCharacter >> 16);
                } else {
                    nonBaseBitmap = graphemeBMP;
                }
                if (nonBaseBitmap == NULL || !isMemberOfBitmap(nonBaseBitmap, sortCharacter)) {
                    break;
                }
                --sortCharacters;
                ++sortLength;
            }
            if (sortLength >= 2) {
                OCFUniCharPrioritySort(sortCharacters, sortLength);
            }
        }
    }
    if (consumedLength != NULL && filteredLength >= 1) {
        *consumedLength = currentIndex - index;
    }
    return filteredLength;
}

// Native CFStringGetRangeOfCharacterClusterAtIndex, 0x1c0213c0c. External.
// The cluster type is compared in 64 bits (1 grapheme: grapheme-extend set 0x6e, otherwise the non-base set
// 8; types up to 2 join graphemes; 4 is backward deletion for the composed-range helper). An index at or past
// the length answers {kCFNotFound, 0}; an untagged, CF, eight-bit string answers {charIndex, 1}. The inline
// buffer is zeroed and then set up natively (directCStringBuffer through
// _CFStringGetCStringPtrInternal(string, ASCII, true, true), not CFStringGetCStringPtr); bitmap 5 is the
// letter set; combClassBMP is the cached combining-class property data (property 0, plane 0). After the
// composed range come, in order: grapheme joiners (U+034F, viramas of combining class 9 with the Tamil pulli
// rule, U+200D) backward and forward, transcoding hints (U+F860-U+F86F), regional-indicator pairs, the ICU
// emoji rules (extended pictographic sequences, prepend and extending characters, at most 64 code units
// each), a cluster of one character preceded by U+200D, and a trailing U+200D. CoreFoundation defect: kept.
// When the backward joiner walk meets a low surrogate it combines it with the preceding high surrogate as
// (low << 10) + high - 0x35FDC00 (the two units swapped), so the letter test of that pair reads an unrelated
// code point (iOS Simulator 0x1c02144b4; confidence: verified). The result is well defined and kept. The
// pictographic scan's start index is an uninitialized native variable that is only used when extending
// characters were found before the cluster; it is initialized to zero here (no observable difference).
OCFRange OCFStringGetRangeOfCharacterClusterAtIndex(OCFStringRef string, OCFIndex charIndex, OCFIndex type) {
    static const uint8_t *combClassBMP = NULL;
    OCFIndex length = OCFStringGetLength(string);
    uint32_t csetType = type == 1 ? 0x6e : 8;
    if (charIndex >= length) {
        OCFRange notFound = {kOCFNotFound, 0};
        return notFound;
    }
    OCFStringInlineBuffer stringBuffer;
    memset(&stringBuffer, 0, sizeof(stringBuffer));
    if (!stringIsTagged(string) && __CF_IS_OBJC(7, string) == 0 &&
        (stringInfo(string) & kStringInfoUnicode) == 0) {
        OCFRange single = {charIndex, 1};
        return single;
    }
    const uint8_t *bmpBitmap = OCFUniCharGetBitmapPtrForPlane(csetType, 0);
    const uint8_t *letterBMP = OCFUniCharGetBitmapPtrForPlane(5, 0);
    if (combClassBMP == NULL) {
        combClassBMP = OCFUniCharGetUnicodePropertyDataForPlane(0, 0);
    }
    stringBuffer.theString = string;
    stringBuffer.rangeToBuffer.location = 0;
    stringBuffer.rangeToBuffer.length = length;
    stringBuffer.directUniCharBuffer = OCFStringGetCharactersPtr(string);
    stringBuffer.directCStringBuffer = NULL;
    if (stringBuffer.directUniCharBuffer == NULL) {
        stringBuffer.directCStringBuffer = _CFStringGetCStringPtrInternal(string, 0x600, true, true);
    }
    stringBuffer.bufferedRangeStart = 0;
    stringBuffer.bufferedRangeEnd = 0;

    OCFRange range =
        _CFStringInlineBufferGetComposedRange(&stringBuffer, charIndex, type, bmpBitmap, csetType);
    OCFIndex location = range.location;
    OCFIndex rangeLength = range.length;
    OCFIndex current = location;

    if (type <= 2) {
        UTF32Char character = 0;
        const uint8_t *letter = letterBMP;
        bool joins = false;
        if (location >= 0 && location < stringBuffer.rangeToBuffer.length) {
            character = stringGetCharacterFromInlineBuffer(&stringBuffer, location);
            if (rangeLength >= 2 && (character & 0xFC00) == 0xD800 &&
                location + 1 < stringBuffer.rangeToBuffer.length) {
                UniChar otherSurrogate = stringGetCharacterFromInlineBuffer(&stringBuffer, location + 1);
                if ((otherSurrogate >> 10) == 0x37) {
                    character = (character << 10) + otherSurrogate - 0x35FDC00;
                    letter = OCFUniCharGetBitmapPtrForPlane(5, character >> 16);
                }
            }
            joins = character == 0x200D;
        }
        if (!joins && letter != NULL && isMemberOfBitmap(letter, character)) {
            joins = true;
        }
        if (joins) {
            // Backward over joiners.
            OCFIndex otherLocation = location;
            for (;;) {
                current = otherLocation;
                if (current < 2) {
                    break;
                }
                UniChar previous = stringGetCharacterFromInlineBuffer(&stringBuffer, current - 1);
                bool hasPage;
                if (previous != 0x34F) {
                    if (previous == 0xBCD &&
                        !_CFStringIsJoiningPulliAtIndex(&stringBuffer, current - 1, 0, length)) {
                        break;
                    }
                    if (combClassBMP != NULL &&
                        combiningClassForCharacter(combClassBMP, previous, &hasPage) == 9) {
                        otherLocation = current - 1;
                    } else {
                        if (previous != 0x200D) {
                            break;
                        }
                        UniChar beforeJoiner = stringGetCharacterFromInlineBuffer(&stringBuffer, current - 2);
                        bool isJoiner;
                        if (beforeJoiner == 0x34F) {
                            isJoiner = true;
                        } else {
                            if (combClassBMP == NULL) {
                                break;
                            }
                            uint8_t combiningClass =
                                combiningClassForCharacter(combClassBMP, beforeJoiner, &hasPage);
                            if (!hasPage) {
                                break;
                            }
                            isJoiner = combiningClass == 9;
                        }
                        if (current == 2) {
                            break;
                        }
                        otherLocation = current - 2;
                        if (!isJoiner) {
                            break;
                        }
                    }
                } else {
                    otherLocation = current - 1;
                }
                OCFRange otherRange = _CFStringInlineBufferGetComposedRange(
                    &stringBuffer, otherLocation - 1, type, bmpBitmap, csetType
                );
                otherLocation = otherRange.location;
                UTF32Char otherCharacter = 0;
                const uint8_t *otherLetter = letterBMP;
                if (otherLocation >= 0 && otherLocation < stringBuffer.rangeToBuffer.length) {
                    otherCharacter = stringGetCharacterFromInlineBuffer(&stringBuffer, otherLocation);
                    if ((otherCharacter >> 10) == 0x37 && otherLocation != 0 &&
                        otherLocation <= stringBuffer.rangeToBuffer.length) {
                        UniChar highSurrogate =
                            stringGetCharacterFromInlineBuffer(&stringBuffer, otherLocation - 1);
                        if ((highSurrogate >> 10) == 0x36) {
                            otherCharacter = ((otherCharacter & 0xFFFF) << 10) + highSurrogate - 0x35FDC00;
                            otherLetter = OCFUniCharGetBitmapPtrForPlane(5, otherCharacter >> 16);
                            otherLocation = otherLocation - 1;
                        }
                    }
                }
                if (otherLetter == NULL || !isMemberOfBitmap(otherLetter, otherCharacter)) {
                    break;
                }
            }

            // Forward over joiners.
            OCFIndex newLength = location - current + rangeLength;
            OCFIndex end = rangeLength + location;
            if (newLength < 2 || end >= length) {
                rangeLength = newLength;
            } else {
                for (;;) {
                    UniChar previous = 0;
                    if (end >= 1 && end <= stringBuffer.rangeToBuffer.length) {
                        previous = stringGetCharacterFromInlineBuffer(&stringBuffer, end - 1);
                    }
                    if (previous != 0x34F && previous != 0x200D) {
                        bool hasPage;
                        if (combClassBMP == NULL ||
                            combiningClassForCharacter(combClassBMP, previous, &hasPage) != 9) {
                            break;
                        }
                        if (previous == 0xBCD &&
                            !_CFStringIsJoiningPulliAtIndex(&stringBuffer, end - 1, 0, length)) {
                            break;
                        }
                    }
                    UTF32Char next = 0;
                    const uint8_t *nextLetter = letterBMP;
                    if (end >= 0 && end < stringBuffer.rangeToBuffer.length) {
                        OCFIndex position = end;
                        next = stringGetCharacterFromInlineBuffer(&stringBuffer, end);
                        if (next == 0x200D) {
                            position = end + 1;
                            next = 0;
                            if (position < stringBuffer.rangeToBuffer.length) {
                                next = stringGetCharacterFromInlineBuffer(&stringBuffer, position);
                            }
                        }
                        if (position < stringBuffer.rangeToBuffer.length && (next >> 10) == 0x36 &&
                            position + 1 < stringBuffer.rangeToBuffer.length) {
                            UniChar lowSurrogate =
                                stringGetCharacterFromInlineBuffer(&stringBuffer, position + 1);
                            if ((lowSurrogate >> 10) == 0x37) {
                                next = ((next & 0xFFFF) << 10) + lowSurrogate - 0x35FDC00;
                                nextLetter = OCFUniCharGetBitmapPtrForPlane(5, next >> 16);
                            }
                        }
                        end = position;
                    }
                    if (nextLetter == NULL || !isMemberOfBitmap(nextLetter, next)) {
                        break;
                    }
                    OCFRange nextRange =
                        _CFStringInlineBufferGetComposedRange(&stringBuffer, end, type, bmpBitmap, csetType);
                    end = nextRange.location + nextRange.length;
                    if (end >= length) {
                        break;
                    }
                }
                rangeLength = end - current;
            }
        }
    }

    // Transcoding hints.
    {
        OCFIndex end = current + rangeLength;
        OCFIndex index = (end > 5 ? end : 5) - 5;
        while (index <= current) {
            UniChar character = stringGetCharacterFromInlineBuffer(&stringBuffer, index);
            if ((character & 0xFFF0) == 0xF860) {
                OCFIndex hintLength = __CFTranscodingHintLength[(uint32_t)character - 0xF860];
                if (index + hintLength + 1 >= end) {
                    if (index + hintLength < length) {
                        OCFIndex otherIndex = index + hintLength + 1;
                        OCFIndex next = index + 1;
                        while (next <= index + hintLength) {
                            OCFRange nextRange = _CFStringInlineBufferGetComposedRange(
                                &stringBuffer, next, type, bmpBitmap, csetType
                            );
                            next = nextRange.location + nextRange.length;
                            if (next > otherIndex) {
                                otherIndex = nextRange.location;
                                break;
                            }
                        }
                        current = index;
                        rangeLength = otherIndex - index;
                    }
                    break;
                }
            }
            ++index;
        }
    }

    // Regional indicators.
    if (rangeLength == 2) {
        if (__CFStringIsRegionalIndicatorAtIndex(&stringBuffer, current)) {
            OCFIndex indicatorStart = current;
            if (current >= 2) {
                OCFIndex previous;
                do {
                    previous = indicatorStart - 2;
                    if (!__CFStringIsRegionalIndicatorAtIndex(&stringBuffer, previous)) {
                        break;
                    }
                    indicatorStart = previous;
                } while (previous + 2 >= 4);
            }
            if (current > indicatorStart && ((current - indicatorStart) & 3) != 0) {
                rangeLength = 4;
                current = current - 2;
            } else if (current + 4 <= length) {
                rangeLength = __CFStringIsRegionalIndicatorAtIndex(&stringBuffer, current + 2) ? 4 : 2;
            } else {
                rangeLength = 2;
            }
        } else {
            rangeLength = 2;
        }
    }

    // Emoji sequences.
    if (current >= 0 && current < length) {
        OCFRange readRange;
        UTF32Char character =
            __CFStringGetLongCharacterFromInlineBuffer(&stringBuffer, length, current, &readRange);
        OCFIndex extendLength = 0;
        OCFIndex extendStart = 0;
        bool scanned = true;
        while (isClusterExtendingCharacter(character)) {
            extendStart = readRange.location;
            if (extendStart == 0) {
                scanned = false;
                break;
            }
            extendLength += readRange.length;
            if (extendLength >= 64) {
                scanned = false;
                break;
            }
            character = __CFStringGetLongCharacterFromInlineBuffer(
                &stringBuffer, length, extendStart - 1, &readRange
            );
        }
        if (scanned) {
            PictographicSequenceComponent component = {-1, 0, -1, -1, -1};
            OCFIndex scanIndex = readRange.location;
            OCFIndex scanLength = readRange.length;
            OCFIndex sequenceStart;
            OCFIndex sequenceLength = 0;
            if (__CFStringGetExtendedPictographicSequenceComponent(
                    &stringBuffer, length, scanIndex, &component
                )) {
                do {
                    sequenceStart = component.location;
                    sequenceLength += component.length;
                    scanIndex = sequenceStart - 1;
                    if (component.joinerIndex == -1) {
                        break;
                    }
                } while (__CFStringGetExtendedPictographicSequenceComponent(
                    &stringBuffer, length, scanIndex, &component
                ));
                scanLength = 0;
            } else {
                sequenceStart = scanIndex;
            }
            readRange.location = scanIndex;
            readRange.length = scanLength;
            bool hasExtend = extendLength > 0;
            bool noSequence;
            OCFIndex prependLength = 0;
            bool proceed = true;
            if (component.extendIndex == -1 && component.joinerIndex == -1) {
                noSequence = sequenceLength == 0;
                if (extendLength >= 1 && noSequence) {
                    proceed = false;
                } else {
                    UTF32Char prepend = __CFStringGetLongCharacterFromInlineBuffer(
                        &stringBuffer, length, scanIndex, &readRange
                    );
                    if (u_getIntPropertyValue((UChar32)prepend, UCHAR_GRAPHEME_CLUSTER_BREAK) ==
                        U_GCB_PREPEND) {
                        scanIndex = readRange.location;
                        prependLength = readRange.length;
                    }
                }
            } else {
                sequenceStart = component.pictographIndex;
                sequenceLength += component.location - sequenceStart;
                scanIndex = sequenceStart + 1;
                noSequence = sequenceLength == 0;
                if (extendLength >= 1 && noSequence) {
                    proceed = false;
                }
            }
            if (proceed && (prependLength != 0 || !noSequence)) {
                OCFIndex forward;
                if (noSequence) {
                    forward = scanIndex + prependLength;
                    sequenceLength = 0;
                    sequenceStart = forward;
                } else {
                    forward = sequenceStart + sequenceLength;
                }
                if (__CFStringGetExtendedPictographicSequenceComponent(
                        &stringBuffer, length, forward, &component
                    )) {
                    do {
                        if (sequenceLength >= 1 && component.joinerIndex == -1) {
                            break;
                        }
                        sequenceLength += component.length;
                        forward += component.length;
                    } while (__CFStringGetExtendedPictographicSequenceComponent(
                        &stringBuffer, length, forward, &component
                    ));
                }
                OCFIndex sequenceEnd = sequenceLength + sequenceStart;
                bool extendsFurther = sequenceEnd < extendStart + extendLength;
                OCFIndex extraExtend = extendsFurther ? extendLength : 0;
                OCFIndex limit = (extendsFurther ? extendStart : sequenceEnd) + extraExtend;
                OCFIndex trailingIndex = hasExtend ? limit : forward;
                OCFIndex trailingLength = hasExtend ? extraExtend : 0;
                if (trailingIndex < length) {
                    UTF32Char trailing = __CFStringGetLongCharacterFromInlineBuffer(
                        &stringBuffer, length, trailingIndex, &readRange
                    );
                    while (isClusterExtendingCharacter(trailing)) {
                        trailingLength += readRange.length;
                        trailing = __CFStringGetLongCharacterFromInlineBuffer(
                            &stringBuffer, length, readRange.location + readRange.length, &readRange
                        );
                        if (trailingLength >= 64) {
                            break;
                        }
                    }
                }
                if (sequenceLength >= 1) {
                    OCFIndex newStart = prependLength > 0 ? scanIndex : sequenceStart;
                    OCFIndex newLength = sequenceLength + (prependLength > 0 ? prependLength : 0) +
                                         (trailingLength > 0 ? trailingLength : 0);
                    if (current >= newStart && current + rangeLength <= newLength + newStart) {
                        rangeLength = newLength;
                        current = newStart;
                    }
                }
            }
        }
    }

    // A one-character cluster after a zero-width joiner joins the preceding composed range.
    if (current >= 1 && rangeLength == 1) {
        if (stringGetCharacterFromInlineBuffer(&stringBuffer, current) == 0x200D) {
            OCFRange otherRange =
                _CFStringInlineBufferGetComposedRange(&stringBuffer, current - 1, type, bmpBitmap, csetType);
            if (current == otherRange.location + otherRange.length) {
                rangeLength = otherRange.length + 1;
                current = otherRange.location;
            } else {
                rangeLength = 1;
            }
        } else {
            rangeLength = 1;
        }
    }
    OCFIndex after = current + rangeLength;
    if (after < length && stringGetCharacterFromInlineBuffer(&stringBuffer, after) == 0x200D) {
        ++rangeLength;
    }
    OCFRange result = {current, rangeLength};
    return result;
}

// Native CFStringCompareWithOptions, 0x1c0214e0c. External. A tail call with a NULL locale.
OCFComparisonResult OCFStringCompareWithOptions(
    OCFStringRef theString1,
    OCFStringRef theString2,
    OCFRange rangeToCompare,
    OCFStringCompareFlags compareOptions
) {
    return OCFStringCompareWithOptionsAndLocale(theString1, theString2, rangeToCompare, compareOptions, NULL);
}

// Native CFStringCompare, 0x1c0214e14. External. Compares the whole first string ({0, CFStringGetLength}).
OCFComparisonResult
OCFStringCompare(OCFStringRef theString1, OCFStringRef theString2, OCFStringCompareFlags compareOptions) {
    OCFRange range = {0, OCFStringGetLength(theString1)};
    return OCFStringCompareWithOptionsAndLocale(theString1, theString2, range, compareOptions, NULL);
}

// The inline-buffer initialization these callers compile: CFStringInitInlineBuffer with CFStringGetCStringPtr
// expanded to _CFStringGetCStringPtrInternal(str, kCFStringEncodingASCII, false, true). The callers also
// clear the character buffer and the buffered range before use (STP Q0/XZR); the buffers below are
// zero-initialized for that.
static inline void initInlineBuffer(OCFStringRef string, OCFStringInlineBuffer *buffer, OCFRange range) {
    buffer->theString = string;
    buffer->rangeToBuffer = range;
    buffer->directUniCharBuffer = OCFStringGetCharactersPtr(string);
    buffer->directCStringBuffer = buffer->directUniCharBuffer != NULL
                                      ? NULL
                                      : _CFStringGetCStringPtrInternal(string, 0x600, false, true);
    buffer->bufferedRangeStart = 0;
    buffer->bufferedRangeEnd = 0;
}

// CFCharacterSetInlineBufferIsLongCharacterMember (CFCharacterSetPriv.h), as CFStringFindCharacterFromSet
// compiles it: flags bit 0 compact bitmap, bit 1 no bitmap available, bit 2 inverted. Inside
// [rangeStart, rangeLimit) a character above 0xFFFF or a set without a bitmap asks
// _CFCharacterSetIsLongCharacterMemberForInline; a NULL bitmap answers "member" unless compact; a compact
// bitmap indexes 256-byte planes (0 none, 0xFF all, else a 32-byte segment at 256 + 32 * (value - 1)).
// Outside the range the answer is the inverted flag.
static inline bool
csetInlineBufferIsLongCharacterMember(OCFCharacterSetInlineBuffer *buffer, UTF32Char character) {
    bool isInverted = (buffer->flags & 4) != 0;
    if (character >= buffer->rangeStart && character < buffer->rangeLimit) {
        if (character > 0xFFFF || (buffer->flags & 2) != 0) {
            return _OCFCharacterSetIsLongCharacterMemberForInline(buffer, character) != 0;
        }
        if (buffer->bitmap == NULL) {
            if ((buffer->flags & 1) == 0) {
                isInverted = !isInverted;
            }
        } else if ((buffer->flags & 1) == 0) {
            if ((buffer->bitmap[character >> 3] & (1 << (character & 7))) != 0) {
                isInverted = !isInverted;
            }
        } else {
            uint8_t value = buffer->bitmap[character >> 8];
            if (value == 0xFF) {
                isInverted = !isInverted;
            } else if (value > 0) {
                const uint8_t *segment = buffer->bitmap + (256 + (32 * (value - 1)));
                character &= 0xFF;
                if ((segment[character >> 3] & (1 << (character % 8))) != 0) {
                    isInverted = !isInverted;
                }
            }
        }
    }
    return isInverted;
}

// CFStringGetCharactersPtrFromInlineBuffer (CFStringInlineBuffer private inline) as the localized-search
// path compiles it: NULL outside the buffered string's range or for more than 64 characters without direct
// UTF-16 contents; a refill starts at the requested location, ends at the earlier of 64 characters later and
// the range end, and widens eight-bit contents as signed chars (LDRSB, STRH).
static inline const UniChar *
findGetCharactersPtrFromInlineBuffer(OCFStringInlineBuffer *buf, OCFRange range) {
    if (range.location < 0 || range.location + range.length > buf->rangeToBuffer.length) {
        return NULL;
    }
    if (buf->directUniCharBuffer != NULL) {
        return buf->directUniCharBuffer + buf->rangeToBuffer.location + range.location;
    }
    if (range.length > 64) {
        return NULL;
    }
    if (range.location + range.length > buf->bufferedRangeEnd || range.location < buf->bufferedRangeStart) {
        buf->bufferedRangeStart = range.location;
        buf->bufferedRangeEnd = buf->bufferedRangeStart + 64;
        if (buf->bufferedRangeEnd > buf->rangeToBuffer.length) {
            buf->bufferedRangeEnd = buf->rangeToBuffer.length;
        }
        OCFIndex location = buf->rangeToBuffer.location + buf->bufferedRangeStart;
        OCFIndex length = buf->bufferedRangeEnd - buf->bufferedRangeStart;
        if (buf->directCStringBuffer != NULL) {
            UniChar *bufPtr = buf->buffer;
            while (length-- != 0) {
                *bufPtr++ = (UniChar)buf->directCStringBuffer[location++];
            }
        } else {
            OCFRange refill = {location, length};
            OCFStringGetCharacters(buf->theString, refill, buf->buffer);
        }
    }
    return buf->buffer + (range.location - buf->bufferedRangeStart);
}

// CFUniCharIsMemberOfBitmap on a plane bitmap: the low 16 bits of the character index it (UBFX #3, #13).
static inline bool findIsMemberOfBitmap(UTF32Char character, const uint8_t *bitmap) {
    return (bitmap[(character & 0xFFFF) >> 3] & (1 << (character & 7))) != 0;
}

// The surrogate-pair combination as compiled: ((high << 10) & 0x36FFC00) - 0x35FDC00 + low, which equals
// ((high - 0xD800) << 10) + (low - 0xDC00) + 0x10000 for a high surrogate.
static inline UTF32Char findCombineSurrogates(UTF32Char high, UniChar low) {
    return ((high << 10) & 0x36FFC00) + 0xFCA02400 + low;
}

// The character at index with an immediately following low surrogate combined (index + 1 outside the buffered
// range reads as 0 and is not combined).
static inline UTF32Char findLongCharacterAtIndex(OCFStringInlineBuffer *buf, OCFIndex index) {
    UTF32Char character = stringGetCharacterFromInlineBuffer(buf, index);
    if ((character & 0xFC00) == 0xD800) {
        UniChar other = stringGetCharacterFromInlineBuffer(buf, index + 1);
        if ((other >> 10) == 0x37) {
            character = findCombineSurrogates(character, other);
        }
    }
    return character;
}

// The ASCII case fold of the non-eight-bit loops: A-Z gain 0x20, except 'I' while a language identifier is in
// effect (kept for the language's own folding).
static inline UTF32Char
findFoldASCII(UTF32Char character, OCFStringCompareFlags options, const uint8_t *langCode) {
    if ((options & kOCFCompareCaseInsensitive) != 0 && (UniChar)(character - 'A') <= 25) {
        if (character != 'I' || langCode == NULL) {
            return character | 0x20;
        }
        return 'I';
    }
    return character;
}

// Native CFStringFindWithOptionsAndLocale, 0x1c0214e5c. External.
// Read from the assembly (12,532 bytes); the Hex-Rays output drops the arguments of every
// __CFStringFoldCharacterClusterAtIndex call, the CFStringGetRangeOfCharacterClusterAtIndex calls after a
// fold that reports insufficient buffer space and the cluster-limit checks; the assembly is followed
// throughout.
// - Returns false unless the pattern and the range are non-empty and the pattern fits the range or length
//   variants are possible (kCFCompareCaseInsensitive, kCFCompareNonliteral, kCFCompareDiacriticInsensitive
//   (0x91) or a non-empty ignorable set from __CFStringFillCharacterSetInlineBuffer).
// - Both strings are read through inline buffers over [0, location + length) and [0, pattern length) whose
//   eight-bit direct contents are the ASCII C-string pointers; the language identifier is the locale's, or
//   the current locale's with kCFCompareLocalized (released after use).
// - fromLoc/toLoc/delta follow the classic rules; equality options are length variants or
//   kCFCompareWidthInsensitive.
// - With eight-bit contents in __CFDefaultEightBitStringEncoding for both strings, bytes are compared
//   directly; a differing non-ASCII byte (or 'I' with a language identifier) is folded, other ASCII bytes
//   are case-folded, ignorable characters are skipped, and folded runs of different lengths are completed
//   from the following characters.
// - Otherwise, without equality options, a literal UTF-16 comparison runs.
// - Otherwise, unless option bit 16 (0x10000) is set and when either string holds Unicode contents, a pattern
//   with an ASCII digit-class character (u_isdigit) or whose first non-Common character is Bengali,
//   Devanagari or Gujarati (script mask 0x8410) is searched with an ICU "root@collation=search" collator
//   (strength 0 with kCFCompareCaseInsensitive, else 2 with kCFCompareLocalized, else 3); a usable ICU result
//   is final. Then the general loop runs: it folds character clusters (caching the last fold of each string
//   by index), skips ignorable characters and, with kCFCompareDiacriticInsensitive, non-base characters, and
//   after a fold that reports insufficient buffer space refolds only at the cluster limit (the cluster end;
//   searching backwards in the subject string, max(cluster location, 2) - 1).
// CoreFoundation defect (kept): in the eight-bit path, when the pattern's folded run is shorter than the
// subject's, the continuation characters are read from the pattern at the subject index plus one
// (stringToFind at str1Index + 1, LDRH [base, X27 LSL #1, #2] at 0x1c02157c8) while the fold is asked about
// the pattern index plus one; the read is bounds-checked by the inline buffer (out of range reads 0), so the
// behavior is well defined and is kept. Also in that path the continuation characters are compared with the
// first folded character of the other string (strBuf1[0] / strBuf2[0], not the character just read), as
// natively.
// Unread: __CFStringFoldCharacterClusterAtIndex's output capacity is constant-propagated; the 64-entry
// buffers match the native frame (256 bytes each at var_270 and var_170); confidence: medium.
Boolean OCFStringFindWithOptionsAndLocale(
    OCFStringRef theString,
    OCFStringRef stringToFind,
    OCFRange rangeToSearch,
    OCFStringCompareFlags searchOptions,
    OCFLocaleRef locale,
    OCFRange *result
) {
    OCFIndex findStrLen = OCFStringGetLength(stringToFind);
    OCFCharacterSetInlineBuffer csetBuffer;
    memset(&csetBuffer, 0, sizeof(csetBuffer));
    Boolean hasIgnoredChars = __CFStringFillCharacterSetInlineBuffer(&csetBuffer, searchOptions);
    bool lengthVariants = (searchOptions & 0x91) != 0 ? true : hasIgnoredChars != 0;
    if (findStrLen < 1 || rangeToSearch.length < 1 ||
        !(findStrLen <= rangeToSearch.length || lengthVariants)) {
        return false;
    }
    OCFCharacterSetInlineBuffer *ignoredChars = hasIgnoredChars ? &csetBuffer : NULL;
    OCFStringInlineBuffer inlineBuf1;
    OCFStringInlineBuffer inlineBuf2;
    memset(&inlineBuf1, 0, sizeof(inlineBuf1));
    memset(&inlineBuf2, 0, sizeof(inlineBuf2));
    UTF32Char strBuf1[64];
    UTF32Char strBuf2[64];

    OCFStringEncoding eightBitEncoding = __OCFDefaultEightBitStringEncoding;
    if (eightBitEncoding == 0xFFFFFFFF) {
        eightBitEncoding = __OCFStringComputeEightBitStringEncoding();
    }
    const uint8_t *str1Bytes =
        (const uint8_t *)_CFStringGetCStringPtrInternal(theString, eightBitEncoding, false, true);
    const uint8_t *str2Bytes =
        (const uint8_t *)_CFStringGetCStringPtrInternal(stringToFind, eightBitEncoding, false, true);
    OCFIndex maxStr1Index = rangeToSearch.location + rangeToSearch.length;
    const uint8_t *langCode;
    if (locale != NULL) {
        langCode = (const uint8_t *)_CFStrGetSpecialCaseHandlingLanguageIdentifierForLocale(locale, true);
    } else if ((searchOptions & kOCFCompareLocalized) != 0) {
        OCFLocaleRef currentLocale = OCFLocaleCopyCurrent();
        langCode =
            (const uint8_t *)_CFStrGetSpecialCaseHandlingLanguageIdentifierForLocale(currentLocale, true);
        OCFRelease(currentLocale);
    } else {
        langCode = NULL;
    }
    OCFRange range1 = {0, maxStr1Index};
    initInlineBuffer(theString, &inlineBuf1, range1);
    OCFRange range2 = {0, findStrLen};
    initInlineBuffer(stringToFind, &inlineBuf2, range2);

    OCFIndex fromLoc;
    OCFIndex toLoc;
    OCFIndex step = lengthVariants ? 1 : findStrLen;
    if ((searchOptions & kOCFCompareBackwards) != 0) {
        fromLoc = maxStr1Index - step;
        toLoc = ((searchOptions & kOCFCompareAnchored) != 0 && !lengthVariants) ? fromLoc
                                                                                : rangeToSearch.location;
    } else {
        fromLoc = rangeToSearch.location;
        toLoc = (searchOptions & kOCFCompareAnchored) != 0 ? fromLoc : maxStr1Index - step;
    }
    bool equalityOptions = (searchOptions & kOCFCompareWidthInsensitive) != 0 || lengthVariants;
    OCFOptionFlags anchorOptions = searchOptions & (kOCFCompareBackwards | kOCFCompareAnchored);
    OCFIndex delta = fromLoc <= toLoc ? 1 : -1;
    bool caseInsensitive = (searchOptions & kOCFCompareCaseInsensitive) != 0;

    if (str1Bytes != NULL && str2Bytes != NULL) {
        // The eight-bit path.
        for (;;) {
            OCFIndex str1Index = fromLoc;
            OCFIndex str2Index = 0;
            if (str1Index < maxStr1Index) {
                do {
                    uint8_t str2Byte;
                    OCFIndex strBuf1Len;
                    int8_t str1Signed;
                    UniChar str1Character = 0;
                    for (;;) {
                        uint8_t str1Byte = str1Bytes[str1Index];
                        str2Byte = str2Bytes[str2Index];
                        if (str1Byte == str2Byte) {
                            goto eightBitMatched;
                        }
                        if (!equalityOptions) {
                            goto eightBitCompared;
                        }
                        str1Signed = (int8_t)str1Byte;
                        if (str1Signed < 0 || (langCode != NULL && str1Signed == 'I')) {
                            str1Character = stringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index);
                            strBuf1Len = __CFStringFoldCharacterClusterAtIndex(
                                str1Character,
                                &inlineBuf1,
                                str1Index,
                                searchOptions,
                                langCode,
                                strBuf1,
                                NULL,
                                NULL
                            );
                            if (strBuf1Len <= 0) {
                                strBuf1[0] = str1Character;
                                strBuf1Len = 1;
                            }
                        } else {
                            if (caseInsensitive && (unsigned)(str1Signed - 'A') < 26) {
                                str1Signed = (int8_t)(str1Signed | 0x20);
                            }
                            strBuf1[0] = (uint8_t)str1Signed;
                            strBuf1Len = 1;
                        }
                        if (ignoredChars == NULL ||
                            (anchorOptions != kOCFCompareAnchored && str1Index == fromLoc)) {
                            break;
                        }
                        UTF32Char member = str1Signed < 0 ? str1Character : (uint8_t)str1Signed;
                        if (!csetInlineBufferIsLongCharacterMember(ignoredChars, member)) {
                            break;
                        }
                        ++str1Index;
                        if (str1Index >= maxStr1Index) {
                            goto eightBitCompared;
                        }
                    }
                    OCFIndex strBuf2Len;
                    int8_t str2Signed = (int8_t)str2Byte;
                    UniChar str2Character = 0;
                    if (str2Signed < 0 || (langCode != NULL && str2Signed == 'I')) {
                        str2Character = stringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index);
                        strBuf2Len = __CFStringFoldCharacterClusterAtIndex(
                            str2Character,
                            &inlineBuf2,
                            str2Index,
                            searchOptions,
                            langCode,
                            strBuf2,
                            NULL,
                            NULL
                        );
                        if (strBuf2Len <= 0) {
                            strBuf2[0] = str2Character;
                            strBuf2Len = 1;
                        }
                    } else {
                        if (caseInsensitive && (unsigned)(str2Signed - 'A') < 26) {
                            str2Signed = (int8_t)(str2Signed | 0x20);
                        }
                        strBuf2[0] = (uint8_t)str2Signed;
                        strBuf2Len = 1;
                    }
                    if (ignoredChars != NULL) {
                        UTF32Char member = str2Signed < 0 ? str2Character : (uint8_t)str2Signed;
                        if (csetInlineBufferIsLongCharacterMember(ignoredChars, member)) {
                            goto eightBitAdvancePattern;
                        }
                    }
                    if (strBuf1Len == 1 && strBuf2Len == 1) {
                        if (strBuf1[0] != strBuf2[0]) {
                            goto eightBitCompared;
                        }
                        goto eightBitMatched;
                    }
                    if (!caseInsensitive && strBuf1Len != strBuf2Len) {
                        goto eightBitCompared;
                    }
                    OCFIndex common = strBuf1Len < strBuf2Len ? strBuf1Len : strBuf2Len;
                    if (memcmp(strBuf1, strBuf2, 4 * common) != 0) {
                        goto eightBitCompared;
                    }
                    if (strBuf1Len < strBuf2Len) {
                        if (strBuf2Len + str1Index > maxStr1Index) {
                            goto eightBitCompared;
                        }
                        const UTF32Char *pending = strBuf2 + strBuf1Len;
                        const UTF32Char *end = strBuf2 + strBuf2Len;
                        do {
                            UniChar next = stringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index + 1);
                            if (__CFStringFoldCharacterClusterAtIndex(
                                    next,
                                    &inlineBuf1,
                                    str1Index + 1,
                                    searchOptions,
                                    langCode,
                                    strBuf1,
                                    NULL,
                                    NULL
                                ) > 0) {
                                goto eightBitCompared;
                            }
                            if (*pending++ != strBuf1[0]) {
                                goto eightBitCompared;
                            }
                            ++str1Index;
                        } while (pending < end);
                    } else if (strBuf2Len < strBuf1Len) {
                        if (strBuf1Len + str2Index > findStrLen) {
                            goto eightBitCompared;
                        }
                        const UTF32Char *pending = strBuf1 + strBuf2Len;
                        const UTF32Char *end = strBuf1 + strBuf1Len;
                        do {
                            // CoreFoundation defect (kept): reads the pattern at str1Index + 1 (see above).
                            UniChar next = stringGetCharacterFromInlineBuffer(&inlineBuf2, str1Index + 1);
                            if (__CFStringFoldCharacterClusterAtIndex(
                                    next,
                                    &inlineBuf2,
                                    str2Index + 1,
                                    searchOptions,
                                    langCode,
                                    strBuf2,
                                    NULL,
                                    NULL
                                ) > 0) {
                                goto eightBitCompared;
                            }
                            if (*pending++ != strBuf2[0]) {
                                goto eightBitCompared;
                            }
                            ++str2Index;
                        } while (pending < end);
                    }
                eightBitMatched:
                    ++str1Index;
                eightBitAdvancePattern:
                    ++str2Index;
                } while (str1Index < maxStr1Index && str2Index < findStrLen);
            }
        eightBitCompared:
            if (str1Index == maxStr1Index && ignoredChars != NULL && str2Index < findStrLen) {
                do {
                    UniChar character = stringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index);
                    if (!csetInlineBufferIsLongCharacterMember(ignoredChars, character)) {
                        break;
                    }
                    ++str2Index;
                } while (str2Index != findStrLen);
            }
            if (str2Index == findStrLen) {
                if (ignoredChars != NULL && anchorOptions == (kOCFCompareBackwards | kOCFCompareAnchored) &&
                    str1Index < maxStr1Index) {
                    do {
                        UniChar character = stringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index);
                        if (!csetInlineBufferIsLongCharacterMember(ignoredChars, character)) {
                            break;
                        }
                        ++str1Index;
                    } while (str1Index != maxStr1Index);
                }
                if (anchorOptions == (kOCFCompareBackwards | kOCFCompareAnchored) &&
                    str1Index != maxStr1Index) {
                    return false;
                }
                if (result != NULL) {
                    result->location = fromLoc;
                    result->length = str1Index - fromLoc;
                }
                return true;
            }
            if (fromLoc == toLoc) {
                return false;
            }
            fromLoc += delta;
        }
    }

    if (!equalityOptions) {
        // The literal path.
        for (;;) {
            OCFIndex str1Index = fromLoc;
            OCFIndex str2Index = 0;
            while (stringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index) ==
                   stringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index)) {
                ++str1Index;
                if (++str2Index == findStrLen) {
                    if (result != NULL) {
                        result->location = fromLoc;
                        result->length = findStrLen;
                    }
                    return true;
                }
            }
            if (fromLoc == toLoc) {
                return false;
            }
            fromLoc += delta;
        }
    }

    const uint8_t *nonBaseBMP = OCFUniCharGetBitmapPtrForPlane(0x6e, 0);
    const uint8_t *combClassBMP = OCFUniCharGetUnicodePropertyDataForPlane(0, 0);
    if ((searchOptions & 0x10000) == 0) {
        // The pattern is scanned through a copy of its inline buffer (a 184-byte copy, LDP/STP Q).
        OCFStringInlineBuffer scanBuffer = inlineBuf2;
        if (CFStrIsUnicode(theString) || CFStrIsUnicode(stringToFind)) {
            int32_t scriptError = 0;
            OCFIndex index = 0;
            for (;;) {
                UniChar character = stringGetCharacterFromInlineBuffer(&scanBuffer, index);
                if (u_isdigit(character) != 0) {
                    goto searchWithCollator;
                }
                int32_t script = uscript_getScript(character, &scriptError);
                if (scriptError > 0) {
                    goto generalSearch;
                }
                if (script != 0) {
                    if ((uint32_t)script <= 15 && ((1u << script) & 0x8410) != 0) {
                        goto searchWithCollator;
                    }
                    goto generalSearch;
                }
                ++index;
                if (findStrLen == index) {
                    goto generalSearch;
                }
            }
        searchWithCollator:;
            int32_t status = 0;
            int32_t textLength = (int32_t)rangeToSearch.length;
            OCFRange textRange = {(searchOptions & kOCFCompareBackwards) != 0 ? toLoc : fromLoc, textLength};
            const UniChar *text = findGetCharactersPtrFromInlineBuffer(&inlineBuf1, textRange);
            OCFRange patternRange = {0, (int32_t)findStrLen};
            const UniChar *pattern = findGetCharactersPtrFromInlineBuffer(&inlineBuf2, patternRange);
            if (pattern != NULL && text != NULL) {
                void *collator = ucol_open("root@collation=search", &status);
                if (status <= 0) {
                    int32_t strength = (searchOptions & kOCFCompareLocalized) != 0 ? 2 : 3;
                    ucol_setStrength(collator, caseInsensitive ? 0 : strength);
                    void *search = usearch_openFromCollator(
                        pattern, (int32_t)findStrLen, text, textLength, collator, NULL, &status
                    );
                    if (status <= 0) {
                        int32_t position = (searchOptions & kOCFCompareBackwards) != 0
                                               ? usearch_last(search, &status)
                                               : usearch_first(search, &status);
                        if (status < 1) {
                            Boolean found;
                            if (position < 0) {
                                found = false;
                            } else {
                                int32_t matchedLength = usearch_getMatchedLength(search);
                                if ((searchOptions & kOCFCompareAnchored) != 0) {
                                    if ((searchOptions & kOCFCompareBackwards) != 0) {
                                        found = (int32_t)((uint32_t)matchedLength + (uint32_t)position) ==
                                                textLength;
                                    } else {
                                        found = position == 0;
                                    }
                                } else {
                                    found = true;
                                }
                                if (result != NULL && found) {
                                    result->location = rangeToSearch.location + (uint32_t)position;
                                    result->length = matchedLength;
                                    found = true;
                                }
                            }
                            usearch_close(search);
                            ucol_close(collator);
                            return found;
                        }
                        usearch_close(search);
                    }
                    ucol_close(collator);
                }
            }
        }
    }

generalSearch:;
    // The general loop. The str1 fold cache (start, length, consumed length, cluster limit), the used lengths
    // and the buffer cursors persist across starting locations; the str2 cache restarts with each location.
    bool forwardAnchor = anchorOptions == kOCFCompareAnchored;
    bool localeCaseFolding = caseInsensitive && langCode != NULL;
    bool backwardsSearch = fromLoc > toLoc;
    OCFIndex strBuf1Index = 0;
    OCFIndex strBuf2Index = 0;
    OCFIndex str1UsedLen = 0;
    OCFIndex str2UsedLen = 0;
    OCFIndex cacheStart1 = 0;
    OCFIndex cacheLen1 = 0;
    OCFIndex cacheUsed1 = 0;
    OCFIndex clusterLimit1 = 0;
    UTF32Char str1Char = 0;
    UTF32Char str2Char = 0;
    for (;;) {
        OCFIndex str1Index = fromLoc;
        OCFIndex str2Index = 0;
        OCFIndex strBuf1Len = 0;
        OCFIndex strBuf2Len = 0;
        OCFIndex cacheStart2 = 0;
        OCFIndex cacheLen2 = 0;
        OCFIndex cacheUsed2 = 0;
        OCFIndex clusterLimit2 = 0;
        for (;;) {
            if (strBuf1Len != 0) {
                str1Char = strBuf1[strBuf1Index++];
            } else {
                str1Char = findFoldASCII(
                    stringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index), searchOptions, langCode
                );
                str1UsedLen = 1;
            }
            if (strBuf2Len != 0) {
                str2Char = strBuf2[strBuf2Index++];
            } else {
                str2Char = findFoldASCII(
                    stringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index), searchOptions, langCode
                );
                str2UsedLen = 1;
            }
            if (str1Char == str2Char) {
                goto advance;
            }
            if (!((str1Char | str2Char) > 0x7F || ignoredChars != NULL || localeCaseFolding)) {
                goto compared;
            }
            if ((str1Char & 0xFC00) == 0xD800) {
                UniChar other = stringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index + 1);
                if ((other >> 10) == 0x37) {
                    str1Char = findCombineSurrogates(str1Char, other);
                    str1UsedLen = 2;
                }
            }
            if ((str2Char & 0xFC00) == 0xD800) {
                UniChar other = stringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index + 1);
                if ((other >> 10) == 0x37) {
                    str2Char = findCombineSurrogates(str2Char, other);
                    str2UsedLen = 2;
                }
            }
            if (ignoredChars != NULL) {
                if ((str1Index != fromLoc || forwardAnchor) && str1Index < maxStr1Index &&
                    csetInlineBufferIsLongCharacterMember(ignoredChars, str1Char)) {
                    if (strBuf1Index == strBuf1Len && strBuf1Len > 0) {
                        strBuf1Len = 0;
                    }
                    if (strBuf1Len == 0) {
                        str1Index += str1UsedLen;
                    }
                    if (strBuf2Len > 0) {
                        --strBuf2Index;
                    }
                    continue;
                }
                if (csetInlineBufferIsLongCharacterMember(ignoredChars, str2Char)) {
                    if (strBuf2Index == strBuf2Len && strBuf2Len > 0) {
                        strBuf2Len = 0;
                    }
                    if (strBuf2Len == 0) {
                        str2Index += str2UsedLen;
                    }
                    if (strBuf1Len > 0) {
                        --strBuf1Index;
                    }
                    if (str2Index < findStrLen) {
                        continue;
                    }
                    goto compared;
                }
            }
            if ((searchOptions & kOCFCompareDiacriticInsensitive) != 0 && str1Index > fromLoc) {
                // A non-base character on one side only is matched by undoing the other side's advance.
                bool str1IsNonBase = false;
                if (strBuf1Len == 0) {
                    const uint8_t *bitmap = str1Char < 0x10000
                                                ? nonBaseBMP
                                                : OCFUniCharGetBitmapPtrForPlane(0x6e, str1Char >> 16);
                    if (bitmap != NULL && findIsMemberOfBitmap(str1Char, bitmap)) {
                        str1Char = str2Char;
                        str1IsNonBase = true;
                    }
                }
                bool str2IsNonBase = false;
                if (strBuf2Len == 0) {
                    const uint8_t *bitmap = str2Char < 0x10000
                                                ? nonBaseBMP
                                                : OCFUniCharGetBitmapPtrForPlane(0x6e, str2Char >> 16);
                    if (bitmap != NULL && findIsMemberOfBitmap(str2Char, bitmap)) {
                        str2Char = str1Char;
                        str2IsNonBase = true;
                    }
                }
                if (str1IsNonBase != str2IsNonBase) {
                    if (str1IsNonBase) {
                        str2Index -= str2UsedLen;
                    }
                    if (str2IsNonBase) {
                        str1Index -= str1UsedLen;
                    }
                }
            }
            if (str1Char != str2Char) {
                bool foldPattern = false;
                if (strBuf1Len == 0) {
                    if (clusterLimit1 != 0 && clusterLimit1 != str1Index) {
                        // Inside a cluster too large to fold: no fold until the cluster limit.
                        strBuf1Len = 0;
                        foldPattern = true;
                        if (strBuf2Len > 0) {
                            goto compared;
                        }
                    } else {
                        OCFIndex cacheIndex = str1Index - cacheStart1;
                        strBuf1Index = cacheIndex + 1;
                        if (cacheLen1 >= 1 && str1Index >= cacheStart1 &&
                            str1Index < cacheStart1 + cacheUsed1 && strBuf1Index < cacheLen1) {
                            clusterLimit1 = 0;
                            strBuf1Len = cacheLen1;
                            str1Char = strBuf1[cacheIndex];
                        } else {
                            // The consumed length is passed in its stack slot, which holds the current used
                            // length (STP X4, X5 before the call).
                            OCFIndex consumed = str1UsedLen;
                            bool insufficientBufferSpace = false;
                            cacheLen1 = __CFStringFoldCharacterClusterAtIndex(
                                str1Char,
                                &inlineBuf1,
                                str1Index,
                                searchOptions,
                                langCode,
                                strBuf1,
                                &consumed,
                                &insufficientBufferSpace
                            );
                            if (cacheLen1 > 0) {
                                strBuf1Index = 1;
                                str1Char = strBuf1[0];
                            }
                            str1UsedLen = consumed;
                            if (insufficientBufferSpace) {
                                OCFRange cluster =
                                    OCFStringGetRangeOfCharacterClusterAtIndex(theString, str1Index, 1);
                                OCFIndex clusterEnd = cluster.location + cluster.length;
                                OCFIndex clusterStart = (cluster.location > 2 ? cluster.location : 2) - 1;
                                clusterLimit1 = backwardsSearch ? clusterStart : clusterEnd;
                            } else {
                                clusterLimit1 = 0;
                            }
                            cacheUsed1 = str1UsedLen;
                            cacheStart1 = str1Index;
                            strBuf1Len = cacheLen1;
                            foldPattern = cacheLen1 == 0;
                            if (cacheLen1 == 0 && strBuf2Len > 0) {
                                goto compared;
                            }
                        }
                    }
                }
                if (strBuf2Len == 0) {
                    foldPattern = foldPattern || str1Char != str2Char;
                    if (!foldPattern) {
                        goto advance;
                    }
                    if (clusterLimit2 != 0 && clusterLimit2 != str2Index) {
                        if (str1Char != str2Char) {
                            goto compared;
                        }
                        goto advance;
                    }
                    OCFIndex cacheIndex = str2Index - cacheStart2;
                    strBuf2Index = cacheIndex + 1;
                    if (cacheLen2 >= 1 && str2Index >= cacheStart2 && str2Index < cacheStart2 + cacheUsed2 &&
                        strBuf2Index < cacheLen2) {
                        if (str1Char != strBuf2[cacheIndex]) {
                            goto compared;
                        }
                        clusterLimit2 = 0;
                        strBuf2Len = cacheLen2;
                    } else {
                        OCFIndex consumed = str2UsedLen;
                        bool insufficientBufferSpace = false;
                        cacheLen2 = __CFStringFoldCharacterClusterAtIndex(
                            str2Char,
                            &inlineBuf2,
                            str2Index,
                            searchOptions,
                            langCode,
                            strBuf2,
                            &consumed,
                            &insufficientBufferSpace
                        );
                        str2UsedLen = consumed;
                        if (insufficientBufferSpace) {
                            OCFRange cluster =
                                OCFStringGetRangeOfCharacterClusterAtIndex(stringToFind, str2Index, 1);
                            clusterLimit2 = cluster.location + cluster.length;
                        } else {
                            clusterLimit2 = 0;
                        }
                        if (cacheLen2 == 0 || str1Char != strBuf2[0]) {
                            goto compared;
                        }
                        cacheUsed2 = str2UsedLen;
                        cacheStart2 = str2Index;
                        strBuf2Index = 1;
                        strBuf2Len = cacheLen2;
                    }
                }
            }
            if (strBuf1Len >= 1 && strBuf2Len >= 1) {
                while (strBuf1Index < strBuf1Len && strBuf2Index < strBuf2Len) {
                    if (strBuf1[strBuf1Index] != strBuf2[strBuf2Index]) {
                        goto compared;
                    }
                    ++strBuf1Index;
                    ++strBuf2Index;
                }
                if (strBuf1Index < strBuf1Len && strBuf2Index < strBuf2Len) {
                    goto compared;
                }
            }
        advance:
            if (strBuf1Index == strBuf1Len && strBuf1Len > 0) {
                strBuf1Len = 0;
            }
            if (strBuf2Index == strBuf2Len && strBuf2Len > 0) {
                strBuf2Len = 0;
            }
            if (strBuf1Len == 0) {
                str1Index += str1UsedLen;
            }
            if (strBuf2Len == 0) {
                str2Index += str2UsedLen;
            }
            if (str2Index >= findStrLen) {
                break;
            }
        }
    compared:
        if (str1Index == maxStr1Index && ignoredChars != NULL && str2Index < findStrLen) {
            do {
                UTF32Char character = findLongCharacterAtIndex(&inlineBuf2, str2Index);
                if (!csetInlineBufferIsLongCharacterMember(ignoredChars, character)) {
                    break;
                }
                str2Index += character >= 0x10000 ? 2 : 1;
            } while (str2Index < findStrLen);
        }
        if (str2Index != findStrLen) {
            goto nextLocation;
        }
        if (strBuf1Len >= 1) {
            // A partially consumed fold matches only when its rest is non-base (diacritic-insensitive, first
            // folded character at most U+050F); planes above the BMP are tested against the canonical
            // decomposable set (9).
            if ((searchOptions & kOCFCompareDiacriticInsensitive) == 0 || strBuf1[0] > 0x50F) {
                goto nextLocation;
            }
            while (strBuf1Index < strBuf1Len) {
                UTF32Char character = strBuf1[strBuf1Index];
                const uint8_t *bitmap =
                    character < 0x10000 ? nonBaseBMP : OCFUniCharGetBitmapPtrForPlane(9, character >> 16);
                if (bitmap == NULL || !findIsMemberOfBitmap(character, bitmap)) {
                    break;
                }
                ++strBuf1Index;
            }
            if (strBuf1Index != strBuf1Len) {
                goto nextLocation;
            }
            str1Index += str1UsedLen;
            strBuf1Index = strBuf1Len;
        }
        if ((searchOptions & (kOCFCompareDiacriticInsensitive | kOCFCompareNonliteral)) != 0 &&
            str1Index < maxStr1Index) {
            UTF32Char character = stringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index);
            const uint8_t *bitmap = nonBaseBMP;
            if ((character & 0xFC00) == 0xD800) {
                UniChar other = stringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index + 1);
                if ((other >> 10) == 0x37) {
                    character = findCombineSurrogates(character, other);
                    bitmap = OCFUniCharGetBitmapPtrForPlane(0x6e, character >> 16);
                }
            }
            if (bitmap != NULL && findIsMemberOfBitmap(character, bitmap)) {
                if ((searchOptions & kOCFCompareDiacriticInsensitive) == 0) {
                    goto nextLocation;
                }
                if (character < 0x10000) {
                    // The backward scan's loop condition is only the location test (as natively).
                    OCFIndex index = str1Index;
                    UniChar previous;
                    do {
                        --index;
                        previous = stringGetCharacterFromInlineBuffer(&inlineBuf1, index);
                    } while (rangeToSearch.location < index);
                    if (previous <= 0x50F && ++str1Index < maxStr1Index) {
                        for (;;) {
                            UniChar next = stringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index);
                            if (nonBaseBMP == NULL || !findIsMemberOfBitmap(next, nonBaseBMP)) {
                                break;
                            }
                            if (++str1Index >= maxStr1Index) {
                                break;
                            }
                        }
                    }
                }
            } else if ((searchOptions & kOCFCompareDiacriticInsensitive) == 0) {
                // A match must not end inside a grapheme cluster: U+034F around the boundary, a zero-width
                // joiner, a Hangul jamo (U+1100-U+11F9) or a virama (combining class 9) before it.
                UniChar other = stringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index - 1);
                bool joins = character == 0x34F || other == 0x34F || other == 0x200D ||
                             (UniChar)(other - 0x1100) < 0xFA;
                if (!joins && combClassBMP != NULL) {
                    uint8_t page = combClassBMP[other >> 8];
                    joins = page != 0 && combClassBMP[256 + (page - 1) * 256 + (other & 0xFF)] == 9;
                }
                if (joins) {
                    OCFRange cluster =
                        OCFStringGetRangeOfCharacterClusterAtIndex(theString, str1Index - 1, 1);
                    if (str1Index < cluster.location + cluster.length) {
                        goto nextLocation;
                    }
                }
            }
        }
        if (ignoredChars != NULL && anchorOptions == (kOCFCompareBackwards | kOCFCompareAnchored) &&
            str1Index < maxStr1Index) {
            do {
                UTF32Char character = findLongCharacterAtIndex(&inlineBuf1, str1Index);
                if (!csetInlineBufferIsLongCharacterMember(ignoredChars, character)) {
                    break;
                }
                str1Index += character >= 0x10000 ? 2 : 1;
            } while (str1Index < maxStr1Index);
        }
        if (anchorOptions == (kOCFCompareBackwards | kOCFCompareAnchored) && str1Index != maxStr1Index) {
            return false;
        }
        if (result != NULL) {
            result->location = fromLoc;
            result->length = str1Index - fromLoc;
        }
        return true;
    nextLocation:
        if (fromLoc == toLoc) {
            return false;
        }
        fromLoc += delta;
    }
}

// Native CFStringFindWithOptions, 0x1c0218b00. External. A tail call with a NULL locale.
Boolean OCFStringFindWithOptions(
    OCFStringRef theString,
    OCFStringRef stringToFind,
    OCFRange rangeToSearch,
    OCFStringCompareFlags searchOptions,
    OCFRange *result
) {
    return OCFStringFindWithOptionsAndLocale(
        theString, stringToFind, rangeToSearch, searchOptions, NULL, result
    );
}

// Native CFStringCreateArrayWithFindResults, 0x1c0218b0c. External.
// The search end is truncated to 32 bits (MOV W20, W8) before the remaining length is computed in 64 bits;
// the result data grows to capacity * 2 + 8 entries; the array is created with capacity count * 8 (LSL #3)
// and a stack copy of the callbacks, receives a pointer to each entry, and the data is released afterwards.
OCFArrayRef OCFStringCreateArrayWithFindResults(
    OCFAllocatorRef alloc,
    OCFStringRef theString,
    OCFStringRef stringToFind,
    OCFRange rangeToSearch,
    OCFStringCompareFlags compareOptions
) {
    if (alloc == NULL) {
        alloc = _OCFGetTSD(1);
        if (alloc == NULL) {
            alloc = kOCFAllocatorSystemDefault;
        }
    }
    if (rangeToSearch.length < 1) {
        return NULL;
    }
    OCFIndex offset = 0;
    OCFMutableDataRef rangeData = NULL;
    OCFIndex capacity = 0;
    OCFIndex count = 0;
    FindResult *entry = NULL;
    uint64_t endIndex = (uint32_t)(rangeToSearch.location + rangeToSearch.length);
    do {
        OCFRange foundRange = {0, 0};
        if (!OCFStringFindWithOptionsAndLocale(
                theString, stringToFind, rangeToSearch, compareOptions, NULL, &foundRange
            )) {
            if (count == 0) {
                return NULL;
            }
            break;
        }
        OCFIndex foundEnd = foundRange.location + foundRange.length;
        if ((compareOptions & kOCFCompareBackwards) == 0) {
            rangeToSearch.length = (OCFIndex)(endIndex - (uint64_t)foundEnd);
            rangeToSearch.location = foundEnd;
        } else {
            rangeToSearch.length = foundRange.location - rangeToSearch.location;
        }
        if (count >= capacity) {
            if (rangeData == NULL) {
                rangeData = OCFDataCreateMutable(alloc, 0);
            }
            capacity = capacity * 2 + 8;
            OCFDataSetLength(rangeData, capacity * 24);
            entry = (FindResult *)(OCFDataGetMutableBytePtr(rangeData) + offset);
        }
        entry->range = foundRange;
        entry->data = rangeData;
        ++entry;
        ++count;
        offset += 24;
    } while (rangeToSearch.length > 0);
    RangeArrayCallBacks callBacks = kFindResultsArrayCallBacks;
    OCFDataSetLength(rangeData, count * 24);
    const uint8_t *bytes = OCFDataGetMutableBytePtr(rangeData);
    OCFMutableArrayRef array = OCFArrayCreateMutable(alloc, count * 8, (const OCFArrayCallBacks *)&callBacks);
    do {
        OCFArrayAppendValue(array, bytes);
        bytes += 24;
    } while (--count != 0);
    OCFRelease(rangeData);
    return array;
}

// Native __rangeRetain, 0x1c0218cf8. Non-external; the find-results array's retain callback. Retains the
// array value's object at offset 0x10 and answers the value itself.
static const void *__rangeRetain(OCFAllocatorRef allocator, const void *value) {
    (void)allocator;
    OCFRetain(*(OCFTypeRef const *)((const uint8_t *)value + 0x10));
    return value;
}

// Native __rangeRelease, 0x1c0218d20 (corpus delta capture 7). Non-external; the find-results array's
// release callback. Releases the array value's object at offset 0x10 (a tail call).
static void __rangeRelease(OCFAllocatorRef allocator, const void *value) {
    (void)allocator;
    OCFRelease(*(OCFTypeRef const *)((const uint8_t *)value + 0x10));
}

// Native __rangeCopyDescription, 0x1c0218d28 (corpus delta capture 7). Non-external; the find-results array's
// description callback. "{%ld, %ld}" of the value's first two words, on kCFAllocatorSystemDefault.
static OCFStringRef __rangeCopyDescription(const void *value) {
    const OCFIndex *range = value;
    return OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault, NULL, OF_STR("{%ld, %ld}"), range[0], range[1]
    );
}

// Native CFStringFind, 0x1c0218d7c. External. Searches the whole string with a NULL locale; not found is
// {kCFNotFound, 0}.
OCFRange
OCFStringFind(OCFStringRef theString, OCFStringRef stringToFind, OCFStringCompareFlags compareOptions) {
    OCFRange result = {0, 0};
    OCFRange range = {0, OCFStringGetLength(theString)};
    if (!OCFStringFindWithOptionsAndLocale(theString, stringToFind, range, compareOptions, NULL, &result)) {
        OCFRange notFound = {kOCFNotFound, 0};
        return notFound;
    }
    return result;
}

// Native CFStringHasPrefix, 0x1c0218e14. External. An anchored search of the whole string (options 8).
Boolean OCFStringHasPrefix(OCFStringRef theString, OCFStringRef prefix) {
    OCFRange range = {0, OCFStringGetLength(theString)};
    return OCFStringFindWithOptionsAndLocale(theString, prefix, range, kOCFCompareAnchored, NULL, NULL);
}

// Native CFStringHasSuffix, 0x1c0218e54. External. A backwards anchored search of the whole string (options
// 0xC).
Boolean OCFStringHasSuffix(OCFStringRef theString, OCFStringRef suffix) {
    OCFRange range = {0, OCFStringGetLength(theString)};
    return OCFStringFindWithOptionsAndLocale(
        theString, suffix, range, kOCFCompareBackwards | kOCFCompareAnchored, NULL, NULL
    );
}

// Whether the character extends a composed sequence: the zero-width joiner, the halfwidth katakana voiced
// marks U+FF9E-U+FF9F, the transcoding-hint range U+F870-U+F87F (with the plane bits), the Fitzpatrick
// modifiers U+1F3FB-U+1F3FF, membership in the cluster bitmap, or a tag character U+E0020-U+E007F, tested in
// that order.
static inline bool extendsComposedSequence(UTF32Char character, bool isMember) {
    return character == 0x200D || (character & 0xFFFFFFFE) == 0xFF9E || (character & 0x1FFFF0) == 0xF870 ||
           character - 0x1F3FB < 5 || isMember || character - 0xE0020 <= 0x5F;
}

// Hangul syllable types as the composed-range code numbers them: 0 leading jamo (below U+1160), 1 vowel jamo
// (below U+11A8), 2 trailing jamo (below U+AC00), 3 LV syllable ((character - U+AC00) % 28 == 0), 4 LVT
// syllable.
static inline uint8_t hangulTypeForCharacter(UTF32Char character) {
    if ((character >> 5) < 0x8B) {
        return 0;
    }
    if ((character >> 3) < 0x235) {
        return 1;
    }
    if ((character >> 10) < 0x2B) {
        return 2;
    }
    return ((character - 0xAC00) % 28) == 0 ? 3 : 4;
}

// Native _CFStringInlineBufferGetComposedRange, 0x1c0218e94. Non-external.
// type 4 (backward deletion) does not combine characters in U+0530-U+194F. The backward walk extends over
// low/high surrogate pairs and characters for which extendsComposedSequence holds (the bitmap of the
// character's plane, csetType, for non-BMP characters); a pair at index 1 stops the walk at 0. Hangul
// syllables then extend backward and forward by syllable type, and finally the forward walk extends over the
// same characters as the backward walk; a NUL character stops either forward walk. The range ends at the
// last examined index.
static OCFRange _CFStringInlineBufferGetComposedRange(
    OCFStringInlineBuffer *buffer, OCFIndex start, OCFIndex type, const uint8_t *bmpBitmap, uint32_t csetType
) {
    OCFIndex end = start + 1;
    UTF32Char character = 0;
    const uint8_t *bitmap = bmpBitmap;
    uint8_t hangulType;
    if (start < 0) {
        goto trailing;
    }
    if (start < buffer->rangeToBuffer.length) {
        character = stringGetCharacterFromInlineBuffer(buffer, start);
        if (type == 4 && (uint16_t)((uint16_t)(character - 0x1950) >> 5) > 0x75E) {
            goto hangul;
        }
        if ((character >> 10) == 0x36 && end < buffer->rangeToBuffer.length) {
            UniChar otherSurrogate = stringGetCharacterFromInlineBuffer(buffer, end);
            if ((otherSurrogate >> 10) == 0x37) {
                end = start + 2;
                character = (character << 10) + otherSurrogate - 0x35FDC00;
                bitmap = OCFUniCharGetBitmapPtrForPlane(csetType, character >> 16);
            }
        }
    }
    if (start == 0) {
        goto hangul;
    }
    for (;;) {
        OCFIndex position;
        if (type == 4 && (character - 0x530) >> 5 < 0xA1) {
            goto hangul;
        }
        if ((character >> 16) != 0) {
            position = start;
        } else {
            position = start;
            bitmap = bmpBitmap;
            if ((character & 0xFC00) == 0xDC00 && start <= buffer->rangeToBuffer.length) {
                UniChar otherSurrogate = stringGetCharacterFromInlineBuffer(buffer, start - 1);
                if ((otherSurrogate >> 10) == 0x36) {
                    position = start - 1;
                    character = character + ((UTF32Char)otherSurrogate << 10) - 0x35FDC00;
                    bitmap = OCFUniCharGetBitmapPtrForPlane(csetType, character >> 16);
                    if (position == 0) {
                        start = 0;
                        goto hangul;
                    }
                }
            }
        }
        bool isMember = bitmap != NULL && isMemberOfBitmap(bitmap, character);
        if (!extendsComposedSequence(character, isMember)) {
            start = position;
            goto hangul;
        }
        start = position - 1;
        character = 0;
        if (position <= buffer->rangeToBuffer.length) {
            character = stringGetCharacterFromInlineBuffer(buffer, start);
        }
        if (position <= 1) {
            goto hangul;
        }
    }

hangul:
    if (character - 0x1100 >= 0xFA && ((character - 0xAC00) >> 4) > 0x2BA) {
        goto trailing;
    }
    hangulType = hangulTypeForCharacter(character);
    if (start >= 1) {
        uint8_t previousType = hangulType;
        for (;;) {
            if (start > buffer->rangeToBuffer.length) {
                break;
            }
            UniChar previous = stringGetCharacterFromInlineBuffer(buffer, start - 1);
            if ((uint16_t)((uint16_t)(previous - 0x1100) >> 4) > 0xC6A) {
                break;
            }
            if ((uint16_t)(previous + 0x5400) > 0x65F9) {
                break;
            }
            if (previousType == 2) {
                if ((uint32_t)previous - 0x1160 < 0x43) {
                    previousType = 1;
                } else if ((previous >> 10) < 0x2B) {
                    if ((previous >> 5) < 0x8B) {
                        break;
                    }
                    previousType = 2;
                } else {
                    previousType = (((uint32_t)previous - 0xAC00) % 28) == 0 ? 3 : 4;
                }
            } else if (previousType == 1) {
                if ((previous >> 5) < 0x8B) {
                    previousType = 0;
                } else if ((previous >> 10) < 0x2B) {
                    if (previous > 0x11A2) {
                        break;
                    }
                    previousType = 1;
                } else {
                    if ((((uint32_t)previous - 0xAC00) % 28) != 0) {
                        break;
                    }
                    previousType = 3;
                }
            } else {
                if ((previous >> 5) > 0x8A) {
                    break;
                }
                previousType = 0;
            }
            --start;
            if ((uint64_t)(start + 1) <= 1) {
                start = 0;
                break;
            }
        }
    }
    if (end < buffer->rangeToBuffer.length) {
        do {
            UniChar next = stringGetCharacterFromInlineBuffer(buffer, end);
            if (next == 0) {
                break;
            }
            if ((uint32_t)next - 0x1100 >= 0xFA && (uint16_t)((uint16_t)(next + 0x5400) >> 4) > 0x2BA) {
                break;
            }
            if (hangulType == 1 || hangulType == 3) {
                if ((uint32_t)next - 0x1160 > 0x99) {
                    break;
                }
                hangulType = (next >> 3) >= 0x235 ? 2 : 1;
            } else if (hangulType == 2 || hangulType == 4) {
                if ((uint32_t)next - 0x11A8 >= 0x52) {
                    break;
                }
                hangulType = 2;
            } else {
                if ((next >> 5) < 0x8B) {
                    hangulType = 0;
                } else if ((next >> 3) < 0x235) {
                    hangulType = 1;
                } else {
                    if ((next >> 10) < 0x2B) {
                        break;
                    }
                    hangulType = (((uint32_t)next - 0xAC00) % 28) == 0 ? 3 : 4;
                }
            }
            ++end;
        } while (end < buffer->rangeToBuffer.length);
    }

trailing:;
    OCFIndex position;
    for (;;) {
        position = end;
        if (position < 0 || position >= buffer->rangeToBuffer.length) {
            break;
        }
        UTF32Char next = stringGetCharacterFromInlineBuffer(buffer, position);
        if (next == 0) {
            break;
        }
        if (type == 4 && (uint16_t)((uint16_t)(next - 0x530) >> 5) < 0xA1) {
            break;
        }
        OCFIndex step = 1;
        const uint8_t *nextBitmap = bmpBitmap;
        if ((next >> 10) == 0x36 && position + 1 < buffer->rangeToBuffer.length) {
            UniChar otherSurrogate = stringGetCharacterFromInlineBuffer(buffer, position + 1);
            if ((otherSurrogate >> 10) == 0x37) {
                next = (next << 10) + otherSurrogate - 0x35FDC00;
                nextBitmap = OCFUniCharGetBitmapPtrForPlane(csetType, next >> 16);
                step = 2;
            }
        }
        bool isMember = nextBitmap != NULL && isMemberOfBitmap(nextBitmap, next);
        end = position + step;
        if (!extendsComposedSequence(next, isMember)) {
            break;
        }
    }
    OCFRange range = {start, position - start};
    return range;
}

// The inlined CFStringGetCharactersFromInlineBuffer (UniChar from the direct buffer; from the buffered
// characters for the overlapping part, the rest from the direct C string, sign-extended, or
// CFStringGetCharacters).
static inline void
stringGetCharactersFromInlineBuffer(OCFStringInlineBuffer *buf, OCFRange desiredRange, UniChar *outBuf) {
    if (buf->directUniCharBuffer != NULL) {
        memmove(
            outBuf,
            buf->directUniCharBuffer + buf->rangeToBuffer.location + desiredRange.location,
            desiredRange.length * sizeof(UniChar)
        );
        return;
    }
    if (desiredRange.location >= buf->bufferedRangeStart && desiredRange.location < buf->bufferedRangeEnd) {
        OCFIndex bufLen = desiredRange.length;
        if (bufLen > buf->bufferedRangeEnd - desiredRange.location) {
            bufLen = buf->bufferedRangeEnd - desiredRange.location;
        }
        memmove(
            outBuf, buf->buffer + (desiredRange.location - buf->bufferedRangeStart), bufLen * sizeof(UniChar)
        );
        outBuf += bufLen;
        desiredRange.location += bufLen;
        desiredRange.length -= bufLen;
    } else {
        OCFIndex desiredRangeMax = desiredRange.location + desiredRange.length;
        if (desiredRangeMax > buf->bufferedRangeStart && desiredRangeMax < buf->bufferedRangeEnd) {
            desiredRange.length = buf->bufferedRangeStart - desiredRange.location;
            memmove(
                outBuf + desiredRange.length,
                buf->buffer,
                (desiredRangeMax - buf->bufferedRangeStart) * sizeof(UniChar)
            );
        }
    }
    if (desiredRange.length >= 1) {
        if (buf->directCStringBuffer != NULL) {
            const char *cString =
                buf->directCStringBuffer + buf->rangeToBuffer.location + desiredRange.location;
            while (desiredRange.length-- != 0) {
                *outBuf++ = (UniChar)*cString++;
            }
        } else {
            OCFRange range = {buf->rangeToBuffer.location + desiredRange.location, desiredRange.length};
            OCFStringGetCharacters(buf->theString, range, outBuf);
        }
    }
}

// Native _CFStringIsJoiningPulliAtIndex, 0x1c02199f4. Non-external.
// For an index strictly inside (rangeLocation, rangeLocation + rangeLength), up to four characters from
// index - 1 are copied into a zeroed buffer and compared with the Tamil conjuncts KA VIRAMA SSA (three
// units), SHA VIRAMA RA II and SA VIRAMA RA II. The native copy into the eight-byte buffer goes through
// __memmove_chk (size 8), which cannot fail for at most four units.
static bool _CFStringIsJoiningPulliAtIndex(
    OCFStringInlineBuffer *buffer, OCFIndex index, OCFIndex rangeLocation, OCFIndex rangeLength
) {
    if (index <= rangeLocation || index >= rangeLocation + rangeLength) {
        return false;
    }
    OCFIndex location = index - 1;
    OCFIndex count = rangeLocation + rangeLength - location;
    if (count >= 4) {
        count = 4;
    }
    UniChar characters[4] = {0, 0, 0, 0};
    OCFRange range = {location, count};
    stringGetCharactersFromInlineBuffer(buffer, range, characters);
    if (characters[0] == 0x0B95 && characters[1] == 0x0BCD && characters[2] == 0x0BB7) {
        return true;
    }
    if (characters[0] == 0x0BB6 && characters[1] == 0x0BCD && characters[2] == 0x0BB0 &&
        characters[3] == 0x0BC0) {
        return true;
    }
    return characters[0] == 0x0BB8 && characters[1] == 0x0BCD && characters[2] == 0x0BB0 &&
           characters[3] == 0x0BC0;
}

// Native __CFStringIsRegionalIndicatorAtIndex, 0x1c0219bcc. Non-external. A regional indicator is U+D83C
// followed by U+DDE6-U+DDFF; a negative or out-of-range index answers false.
static bool __CFStringIsRegionalIndicatorAtIndex(OCFStringInlineBuffer *buffer, OCFIndex index) {
    if (index < 0) {
        return false;
    }
    if (stringGetCharacterFromInlineBuffer(buffer, index) != 0xD83C) {
        return false;
    }
    UniChar low = stringGetCharacterFromInlineBuffer(buffer, index + 1);
    return (uint16_t)(low + 0x221A) < 0x1A;
}

// Native CFStringGetRangeOfComposedCharactersAtIndex, 0x1c0219d48. External. A tail call with cluster type 2.
OCFRange OCFStringGetRangeOfComposedCharactersAtIndex(OCFStringRef theString, OCFIndex theIndex) {
    return OCFStringGetRangeOfCharacterClusterAtIndex(theString, theIndex, 2);
}

// The eight-bit encoding as every caller reads it: __CFDefaultEightBitStringEncoding, computed on first use
// (LDR, CMN #1, BL __CFStringComputeEightBitStringEncoding).
static inline OCFStringEncoding eightBitStringEncoding(void) {
    OCFStringEncoding encoding = __OCFDefaultEightBitStringEncoding;
    if (encoding == (OCFStringEncoding)-1) {
        encoding = __OCFStringComputeEightBitStringEncoding();
    }
    return encoding;
}

// The length-byte skip the mutation functions add to eight-bit contents: info bit 2 (UBFX #2, #1), without
// the mutable test of stringHasLengthByte.
static inline uintptr_t skipAnyLengthByte(OCFStringRef string) {
    return (uintptr_t)((stringInfo(string) >> 2) & 1);
}

// The replacement every CFStringInsert/Replace/ReplaceAll/Append call compiles (the native static inline
// __CFStringReplace, which has no symbol): a replacement equal to the string is first copied with the system
// default allocator; the size change makes the string Unicode when the replacement has characters and
// CFStrIsUnicode says so; eight-bit contents receive CFStringGetBytes in the eight-bit encoding at
// range.location plus the length-byte skip (no NULL test), Unicode contents receive CFStringGetCharacters
// when the contents pointer is not NULL; the copy is released last.
static inline void replaceInRange(OCFMutableStringRef string, OCFRange range, OCFStringRef replacement) {
    OCFStringRef copy = NULL;
    if (replacement == string) {
        copy = OCFStringCreateCopy(kOCFAllocatorSystemDefault, replacement);
        replacement = copy;
    }
    OCFIndex replacementLength = OCFStringGetLength(replacement);
    Boolean makeUnicode = replacementLength >= 1 && CFStrIsUnicode(replacement) != 0;
    __CFStringChangeSizeMultiple(string, &range, 1, replacementLength, makeUnicode);
    if (stringIsUnicode(string)) {
        UniChar *contents = (UniChar *)stringContents(string);
        if (contents != NULL) {
            OCFStringGetCharacters(replacement, (OCFRange){0, replacementLength}, contents + range.location);
        }
    } else {
        uint8_t *contents = (uint8_t *)stringContents(string);
        OCFStringEncoding encoding = eightBitStringEncoding();
        OCFStringGetBytes(
            replacement,
            (OCFRange){0, replacementLength},
            encoding,
            0,
            false,
            contents + range.location + skipAnyLengthByte(string),
            replacementLength,
            NULL
        );
    }
    if (copy != NULL) {
        OCFRelease(copy);
    }
}

static bool isObjCString(OCFStringRef string) {
    return stringIsTagged(string) || __CF_IS_OBJC(7, string) != 0;
}

// Native CFStringFindCharacterFromSet, 0x1c0219d50. External. No Objective-C dispatch: every string goes
// through the inline buffer. The search runs from the range's first index (last with kCFCompareBackwards) to
// its last (first), or only at the start with kCFCompareAnchored. A surrogate (0xD800-0xDFFF) is paired with
// its neighbour in the search direction; the neighbour's index is computed in 32 bits and sign-extended (ADD
// W8, W21, W26; SXTW), as natively. A valid pair is tested as one long character and reported as a two-unit
// range at the lower index; an invalid pair moves past the neighbour.
Boolean OCFStringFindCharacterFromSet(
    OCFStringRef theString,
    OCFCharacterSetRef theSet,
    OCFRange rangeToSearch,
    OCFStringCompareFlags searchOptions,
    OCFRange *result
) {
    OCFIndex length = OCFStringGetLength(theString);
    if (rangeToSearch.length == 0 || rangeToSearch.location + rangeToSearch.length > length) {
        return false;
    }
    OCFIndex last = rangeToSearch.location + rangeToSearch.length - 1;
    OCFIndex fromLoc;
    OCFIndex toLoc;
    if ((searchOptions & kOCFCompareBackwards) != 0) {
        fromLoc = last;
        toLoc = rangeToSearch.location;
    } else {
        fromLoc = rangeToSearch.location;
        toLoc = last;
    }
    if ((searchOptions & kOCFCompareAnchored) != 0) {
        toLoc = fromLoc;
    }
    OCFIndex step = fromLoc <= toLoc ? 1 : -1;
    OCFStringInlineBuffer stringBuffer = {0};
    OCFCharacterSetInlineBuffer csetBuffer = {0};
    initInlineBuffer(theString, &stringBuffer, rangeToSearch);
    OCFCharacterSetInitInlineBuffer(theSet, &csetBuffer);
    OCFIndex cnt = fromLoc;
    while (true) {
        UniChar ch = stringGetCharacterFromInlineBuffer(&stringBuffer, cnt - rangeToSearch.location);
        if (ch >= 0xD800 && ch <= 0xDFFF) {
            OCFIndex otherCharIndex = (int32_t)(cnt + step);
            if ((fromLoc > toLoc && otherCharIndex < toLoc) || (fromLoc <= toLoc && otherCharIndex > toLoc)) {
                return false;
            }
            UniChar lowChar =
                stringGetCharacterFromInlineBuffer(&stringBuffer, otherCharIndex - rangeToSearch.location);
            UniChar highChar;
            if (cnt < otherCharIndex) {
                highChar = ch;
            } else {
                highChar = lowChar;
                lowChar = ch;
            }
            if ((highChar & 0xFC00) == 0xD800 && (lowChar & 0xFC00) == 0xDC00 &&
                csetInlineBufferIsLongCharacterMember(
                    &csetBuffer, ((UTF32Char)highChar << 10) + lowChar + 0xFCA02400
                )) {
                if (result != NULL) {
                    *result = (OCFRange){cnt < otherCharIndex ? cnt : otherCharIndex, 2};
                }
                return true;
            }
            cnt = otherCharIndex;
            if (otherCharIndex == toLoc) {
                return false;
            }
        } else if (csetInlineBufferIsLongCharacterMember(&csetBuffer, ch)) {
            if (result != NULL) {
                *result = (OCFRange){cnt, 1};
            }
            return true;
        } else if (cnt == toLoc) {
            return false;
        }
        cnt += step;
    }
}

// Native CFStringGetLineBounds, 0x1c021a15c (corpus delta capture 7). External. Tagged and Objective-C
// strings receive -getLineStart:end:contentsEnd:forRange:; CF strings go to
// __CFStringGetLineOrParagraphBounds with the line endings U+2028 and U+0085 included (both tail calls).
void OCFStringGetLineBounds(
    OCFStringRef theString,
    OCFRange range,
    OCFIndex *lineBeginIndex,
    OCFIndex *lineEndIndex,
    OCFIndex *contentsEndIndex
) {
    if (stringIsTagged(theString) || __CF_IS_OBJC(7, theString) != 0) {
        ((void (*)(id, SEL, OCFIndex *, OCFIndex *, OCFIndex *, OCFRange))objc_msgSend)(
            (id)theString,
            sel_getUid("getLineStart:end:contentsEnd:forRange:"),
            lineBeginIndex,
            lineEndIndex,
            contentsEndIndex,
            range
        );
        return;
    }
    __CFStringGetLineOrParagraphBounds(
        theString, range, lineBeginIndex, lineEndIndex, contentsEndIndex, true
    );
}

// The separator test both scans of __CFStringGetLineOrParagraphBounds inline: characters from 0x0E to 0x84
// are never separators; LF, CR and U+2029 always are; U+2028 and U+0085 only with includeLineEndings.
static inline bool lineSeparatorCharacter(UniChar character, Boolean includeLineEndings) {
    if ((uint16_t)(character - 14) < 0x77) {
        return false;
    }
    if (character == 10 || character == 13 || character == 0x2029) {
        return true;
    }
    return includeLineEndings && (character == 0x2028 || character == 0x85);
}

// The inline-buffer initialization of __CFStringGetLineOrParagraphBounds: the whole string, with the
// direct C-string pointer of _CFStringGetCStringPtrInternal(str, kCFStringEncodingASCII, false, false)
// inlined natively (the eight-bit encoding must be ASCII, read twice; eight-bit contents or a null byte; past
// any length byte).
static inline void lineInitInlineBuffer(OCFStringRef string, OCFStringInlineBuffer *buffer, OCFIndex length) {
    buffer->theString = string;
    buffer->rangeToBuffer = (OCFRange){0, length};
    buffer->directUniCharBuffer = OCFStringGetCharactersPtr(string);
    buffer->directCStringBuffer = buffer->directUniCharBuffer != NULL
                                      ? NULL
                                      : _CFStringGetCStringPtrInternal(string, 0x600, false, false);
    buffer->bufferedRangeStart = 0;
    buffer->bufferedRangeEnd = 0;
}

// The reads of the end scan: 0xFFFF outside the buffered range (the SDK's
// __CFStringGetCharacterFromInlineBufferAux), else CFStringGetCharacterFromInlineBuffer.
static inline UniChar lineGetCharacterAux(OCFStringInlineBuffer *buffer, OCFIndex idx) {
    if (idx < 0 || idx >= buffer->rangeToBuffer.length) {
        return 0xFFFF;
    }
    return stringGetCharacterFromInlineBuffer(buffer, idx);
}

// Native __CFStringGetLineOrParagraphBounds, 0x1c021a1f8 (corpus delta capture 7). Non-external. The length
// is read inline. With lineBeginIndex: location 0 answers 0; otherwise, over an inline buffer of the whole
// string (reads outside it answer 0), a LF at the location preceded by a CR steps back past both, and the
// scan walks back from the location - 1 to the character after the first separator (0 when none). With
// lineEndIndex or contentsEndIndex: a fresh inline buffer (reads outside it answer 0xFFFF) is read at the
// range's last character (the location itself for an empty range): a LF ends the contents there, or one
// earlier with a separator length of 2 when a CR precedes it; otherwise the scan walks forward to the first
// separator (a CR followed by a LF has length 2, any other separator 1) or to the string's length
// (separator length 0). contentsEndIndex receives the end of the contents, then lineEndIndex that plus the
// separator length.
static void __CFStringGetLineOrParagraphBounds(
    OCFStringRef string,
    OCFRange range,
    OCFIndex *lineBeginIndex,
    OCFIndex *lineEndIndex,
    OCFIndex *contentsEndIndex,
    Boolean includeLineEndings
) {
    OCFStringInlineBuffer buffer = {0};
    OCFIndex length = stringLength(string);
    if (lineBeginIndex != NULL) {
        OCFIndex start = 0;
        if (range.location != 0) {
            lineInitInlineBuffer(string, &buffer, length);
            OCFIndex idx = range.location;
            UniChar ch = stringGetCharacterFromInlineBuffer(&buffer, idx);
            idx--;
            if (ch == 10 && stringGetCharacterFromInlineBuffer(&buffer, idx) == 13) {
                idx--;
            }
            while (true) {
                if (idx < 0) {
                    start = 0;
                    break;
                }
                if (lineSeparatorCharacter(
                        stringGetCharacterFromInlineBuffer(&buffer, idx), includeLineEndings
                    )) {
                    start = idx + 1;
                    break;
                }
                idx--;
            }
        }
        *lineBeginIndex = start;
    }
    if (lineEndIndex == NULL && contentsEndIndex == NULL) {
        return;
    }
    lineInitInlineBuffer(string, &buffer, length);
    OCFIndex idx = range.location + range.length - (range.length != 0);
    OCFIndex endOfContents;
    OCFIndex lineSeparatorLength = 1;
    UniChar ch = lineGetCharacterAux(&buffer, idx);
    if (ch == 10) {
        endOfContents = idx;
        if (lineGetCharacterAux(&buffer, idx - 1) == 13) {
            lineSeparatorLength = 2;
            endOfContents = idx - 1;
        }
    } else {
        while (true) {
            if (lineSeparatorCharacter(ch, includeLineEndings)) {
                endOfContents = idx;
                if (ch == 13 && lineGetCharacterAux(&buffer, idx + 1) == 10) {
                    lineSeparatorLength = 2;
                }
                break;
            }
            if (idx >= length) {
                endOfContents = length;
                lineSeparatorLength = 0;
                break;
            }
            idx++;
            ch = lineGetCharacterAux(&buffer, idx);
        }
    }
    if (contentsEndIndex != NULL) {
        *contentsEndIndex = endOfContents;
    }
    if (lineEndIndex != NULL) {
        *lineEndIndex = endOfContents + lineSeparatorLength;
    }
}

// Native CFStringCreateByCombiningStrings, 0x1c021ab10. External. A separator that is a tagged pointer or an
// Objective-C object makes the result Unicode; an empty array gives __CFStringCreateImmutableFunnel3 with no
// bytes (Unicode, tryToReduceUnicode, ALLOCATORSFREEFUNC, the allocator as passed); one element is
// CFStringCreateCopy. Otherwise the default allocator replaces NULL, the store (length + 1 bytes when every
// string is an eight-bit CF string, else 2 * length) is allocated with type ID 0x100004077774924 and named
// "CFString (store)"; an allocation failure is __CFStringHandleOutOfMemory. The separator is converted once
// and its first copy is reused. An eight-bit result is NUL-terminated and created from strlen of the store
// (hasNullByte | noCopy); a Unicode result from 2 * length bytes (noCopy); both free the store with the
// allocator.
OCFStringRef
OCFStringCreateByCombiningStrings(OCFAllocatorRef alloc, OCFArrayRef theArray, OCFStringRef separatorString) {
    OCFIndex numStrings = OCFArrayGetCount(theArray);
    bool isSeparatorCF = false;
    bool canBeEightbit = false;
    if (!isObjCString(separatorString)) {
        canBeEightbit = stringIsEightBit(separatorString);
        isSeparatorCF = true;
    }
    if (numStrings == 1) {
        return OCFStringCreateCopy(alloc, OCFArrayGetValueAtIndex(theArray, 0));
    }
    if (numStrings == 0) {
        return __CFStringCreateImmutableFunnel3(alloc, NULL, 0, 0x100, 2, (OCFAllocatorRef)-1, 0);
    }
    if (alloc == NULL) {
        OCFAllocatorRef current = _OCFGetTSD(1);
        alloc = current != NULL ? current : kOCFAllocatorSystemDefault;
    }
    OCFIndex numChars = OCFStringGetLength(separatorString) * (numStrings - 1);
    for (OCFIndex idx = 0; idx < numStrings; ++idx) {
        OCFStringRef string = OCFArrayGetValueAtIndex(theArray, idx);
        OCFIndex stringLengthValue = OCFStringGetLength(string);
        if (isObjCString(string) || stringIsUnicode(string)) {
            canBeEightbit = false;
        }
        numChars += stringLengthValue;
    }
    OCFIndex unicodeBytes = numChars << 1;
    uint8_t *buffer =
        OCFAllocatorAllocateTyped(alloc, canBeEightbit ? numChars + 1 : unicodeBytes, 0x100004077774924, 0);
    if (buffer == NULL) {
        __CFStringHandleOutOfMemory();
    }
    if (__OCFOASafe == 1) {
        __OCFSetLastAllocationEventName(buffer, "CFString (store)");
    }
    OCFIndex separatorLength = OCFStringGetLength(separatorString);
    uint8_t *bufPtr = buffer;
    if (numStrings >= 1) {
        uintptr_t charShift = canBeEightbit ? 0 : 1;
        OCFIndex separatorBytes = separatorLength << charShift;
        const uint8_t *separatorFill = NULL;
        for (OCFIndex idx = 0; idx < numStrings; ++idx) {
            if (idx != 0) {
                if (separatorFill != NULL) {
                    memmove(bufPtr, separatorFill, separatorBytes);
                } else {
                    if (!isSeparatorCF) {
                        OCFStringGetCharacters(
                            separatorString,
                            (OCFRange){0, OCFStringGetLength(separatorString)},
                            (UniChar *)bufPtr
                        );
                    } else if (canBeEightbit || stringIsUnicode(separatorString)) {
                        const uint8_t *contents = stringContents(separatorString);
                        memmove(bufPtr, contents + skipAnyLengthByte(separatorString), separatorBytes);
                    } else {
                        const uint8_t *contents = stringContents(separatorString);
                        uintptr_t skip = skipAnyLengthByte(separatorString);
                        __OCFStrConvertBytesToUnicode(
                            contents + skip, (UniChar *)bufPtr, stringLength(separatorString)
                        );
                    }
                    separatorFill = bufPtr;
                }
                bufPtr += separatorBytes;
            }
            OCFStringRef string = OCFArrayGetValueAtIndex(theArray, idx);
            if (isObjCString(string)) {
                OCFIndex valueLength = OCFStringGetLength(string);
                OCFStringGetCharacters(string, (OCFRange){0, valueLength}, (UniChar *)bufPtr);
                bufPtr += valueLength << 1;
            } else {
                const uint8_t *contents = stringContents(string);
                OCFIndex valueBytes = stringLength(string) << charShift;
                if (canBeEightbit || stringIsUnicode(string)) {
                    memmove(bufPtr, contents + skipAnyLengthByte(string), valueBytes);
                } else {
                    uintptr_t skip = skipAnyLengthByte(string);
                    __OCFStrConvertBytesToUnicode(contents + skip, (UniChar *)bufPtr, stringLength(string));
                }
                bufPtr += valueBytes;
            }
        }
    }
    if (canBeEightbit) {
        *bufPtr = 0;
        OCFStringEncoding encoding = eightBitStringEncoding();
        return __CFStringCreateImmutableFunnel3(
            alloc, buffer, (OCFIndex)strlen((const char *)buffer), encoding, 0x18, alloc, 0
        );
    }
    return __CFStringCreateImmutableFunnel3(alloc, buffer, unicodeBytes, 0x100, 0x10, alloc, 0);
}

// Native CFStringCreateArrayBySeparatingStrings, 0x1c021b048. External. Without a match the result is a
// one-element array of the string itself (CFArrayCreate with kCFTypeArrayCallBacks); otherwise a mutable
// array of capacity count + 2 receives the substrings between the found ranges (each created, appended and
// released) and the remainder after the last range; the find results are released.
OCFArrayRef OCFStringCreateArrayBySeparatingStrings(
    OCFAllocatorRef alloc, OCFStringRef theString, OCFStringRef separatorString
) {
    const void *values[1] = {theString};
    OCFIndex length = OCFStringGetLength(theString);
    OCFArrayRef separatorRanges =
        OCFStringCreateArrayWithFindResults(alloc, theString, separatorString, (OCFRange){0, length}, 0);
    if (separatorRanges == NULL) {
        return OCFArrayCreate(alloc, values, 1, &kOCFTypeArrayCallBacks);
    }
    OCFIndex numRanges = OCFArrayGetCount(separatorRanges);
    OCFMutableArrayRef array = OCFArrayCreateMutable(alloc, numRanges + 2, &kOCFTypeArrayCallBacks);
    OCFIndex startIndex = 0;
    for (OCFIndex idx = 0; idx < numRanges; ++idx) {
        const OCFRange *currentRange = OCFArrayGetValueAtIndex(separatorRanges, idx);
        OCFStringRef substring = OCFStringCreateWithSubstring(
            alloc, theString, (OCFRange){startIndex, currentRange->location - startIndex}
        );
        OCFArrayAppendValue(array, substring);
        OCFRelease(substring);
        startIndex = currentRange->location + currentRange->length;
    }
    OCFStringRef substring =
        OCFStringCreateWithSubstring(alloc, theString, (OCFRange){startIndex, length - startIndex});
    OCFArrayAppendValue(array, substring);
    OCFRelease(substring);
    OCFRelease(separatorRanges);
    return array;
}

// Native CFStringCreateFromExternalRepresentation, 0x1c021b1d8. External. iOS Device divergence: unread.
OCFStringRef OCFStringCreateFromExternalRepresentation(
    OCFAllocatorRef alloc, OCFDataRef data, OCFStringEncoding encoding
) {
    const uint8_t *bytes = OCFDataGetBytePtr(data);
    OCFIndex length = OCFDataGetLength(data);
    return __CFStringCreateImmutableFunnel3(alloc, bytes, length, encoding, 3, (OCFAllocatorRef)-1, 0);
}

// Native CFStringCreateExternalRepresentation, 0x1c021b234. External. An eight-bit CF string whose encoding
// is the requested one (or ASCII, for an ASCII-superset request) becomes CFDataCreate of its bytes with the
// allocator as passed. Otherwise NULL becomes the default allocator; UTF-16/UTF-32 requests (low twelve bits
// 0x100, the encoding 0x100 or one in 0x8000101-0x1C000100) size the store as (length + 1) * 2, or * 4 when
// bit 27 is set; other requests take CFStringGetMaximumSizeForEncoding and, for a multi-byte encoding and a
// CF string, either CFStringEncodingByteLengthForCharacters (Unicode contents, flag 0x100) or a sizing pass
// of
// __CFStringEncodeByteStream (a 32-bit result; failing when it differs from the length and the loss byte is
// 0 or the result is 0); an all-ASCII eight-bit string then becomes CFDataCreate of its bytes. The store is
// allocated with type ID 0x100004077774924 (no NULL test) and named "CFData (store)", encoded, and wrapped
// without copying, freed with the allocator.
OCFDataRef OCFStringCreateExternalRepresentation(
    OCFAllocatorRef alloc, OCFStringRef theString, OCFStringEncoding encoding, UInt8 lossByte
) {
    OCFIndex usedLength = 0;
    OCFIndex guessedByteLength = 0;
    OCFIndex length;
    if (isObjCString(theString)) {
        length = OCFStringGetLength(theString);
    } else {
        length = stringLength(theString);
        if (stringIsEightBit(theString) &&
            (eightBitStringEncoding() == encoding ||
             (eightBitStringEncoding() == 0x600 && encodingIsSupersetOfASCII(encoding)))) {
            const uint8_t *contents = stringContents(theString);
            uintptr_t skip = skipAnyLengthByte(theString);
            return OCFDataCreate(alloc, contents + skip, stringLength(theString));
        }
    }
    if (alloc == NULL) {
        OCFAllocatorRef current = _OCFGetTSD(1);
        alloc = current != NULL ? current : kOCFAllocatorSystemDefault;
    }
    if ((encoding & 0xFFF) == 0x100 && (encoding == 0x100 || (uint32_t)(encoding + 0xF7FFFEFF) >> 26 <= 4)) {
        guessedByteLength = (length + 1) << ((encoding & 0x8000000) != 0 ? 2 : 1);
    } else {
        guessedByteLength = OCFStringGetMaximumSizeForEncoding(length, encoding);
        if (!stringIsTagged(theString) && guessedByteLength > length && __CF_IS_OBJC(7, theString) == 0) {
            if (stringIsUnicode(theString)) {
                OCFIndex aLength = CFStringEncodingByteLengthForCharacters(
                    encoding, 0x100, (const UniChar *)stringContents(theString), stringLength(theString)
                );
                if (aLength > 0) {
                    guessedByteLength = aLength;
                }
            } else {
                SInt32 result = (SInt32)__OCFStringEncodeByteStream(
                    theString, 0, length, true, encoding, (char)lossByte, NULL, INTPTR_MAX, &guessedByteLength
                );
                if (result != length && (lossByte == 0 || result == 0)) {
                    return NULL;
                }
                if (guessedByteLength == length && stringIsEightBit(theString) &&
                    encodingIsSupersetOfASCII(encoding)) {
                    const uint8_t *contents = stringContents(theString);
                    uintptr_t skip = skipAnyLengthByte(theString);
                    return OCFDataCreate(alloc, contents + skip, stringLength(theString));
                }
            }
        }
    }
    uint8_t *bytes = OCFAllocatorAllocateTyped(alloc, guessedByteLength, 0x100004077774924, 0);
    if (__OCFOASafe == 1) {
        __OCFSetLastAllocationEventName(bytes, "CFData (store)");
    }
    SInt32 result = (SInt32)__OCFStringEncodeByteStream(
        theString, 0, length, true, encoding, (char)lossByte, bytes, guessedByteLength, &usedLength
    );
    if (result != length && (lossByte == 0 || result == 0)) {
        OCFAllocatorDeallocate(alloc, bytes);
        return NULL;
    }
    return OCFDataCreateWithBytesNoCopy(alloc, bytes, usedLength, alloc);
}

// Native CFStringGetSmallestEncoding, 0x1c021b960 (corpus delta capture 7). External. Tagged and
// Objective-C strings answer -_smallestEncodingInCFStringEncoding (a tail call). An eight-bit string answers
// the eight-bit encoding (computed when still unset). A Unicode string is measured with
// __CFStringEncodeByteStream (no buffer, no loss byte, maximum INT64_MAX) in the eight-bit encoding: a full
// conversion answers the eight-bit encoding (reread; computed when unset, a tail call). Otherwise, when the
// eight-bit encoding (computed when unset) is the system encoding (MacRoman: the flag byte's bit 0 means 0,
// else CFStringGetSystemEncoding runs and the byte minus one is used) kCFStringEncodingUnicode (0x100) is
// answered; else a second measure in the system encoding (0 when the flag is set, else after another
// CFStringGetSystemEncoding the byte minus one) answers the system encoding (read the same way) when it
// converts the whole string, kCFStringEncodingUnicode otherwise.
OCFStringEncoding OCFStringGetSmallestEncoding(OCFStringRef theString) {
    if (stringIsTagged(theString) || __CF_IS_OBJC(7, theString) != 0) {
        return ((OCFStringEncoding (*)(id, SEL))objc_msgSend)(
            (id)theString, sel_getUid("_smallestEncodingInCFStringEncoding")
        );
    }
    OCFStringEncoding encoding;
    if (stringIsEightBit(theString)) {
        encoding = __OCFDefaultEightBitStringEncoding;
    } else {
        OCFIndex length = stringLength(theString);
        OCFIndex converted = __OCFStringEncodeByteStream(
            theString, 0, length, false, stringEightBitEncoding(), 0, NULL, INT64_MAX, NULL
        );
        encoding = __OCFDefaultEightBitStringEncoding;
        if (converted != length) {
            if (encoding == 0xffffffff) {
                encoding = __OCFStringComputeEightBitStringEncoding();
            }
            OCFStringEncoding systemEncoding;
            if ((__CFDefaultSystemEncoding & 1) != 0) {
                if (encoding == 0) {
                    return 0x100;
                }
                systemEncoding = 0;
            } else {
                OCFStringGetSystemEncoding();
                if (encoding == (OCFStringEncoding)(__CFDefaultSystemEncoding - 1)) {
                    return 0x100;
                }
                if ((__CFDefaultSystemEncoding & 1) != 0) {
                    systemEncoding = 0;
                } else {
                    OCFStringGetSystemEncoding();
                    systemEncoding = (OCFStringEncoding)(__CFDefaultSystemEncoding - 1);
                }
            }
            if (__OCFStringEncodeByteStream(
                    theString, 0, length, false, systemEncoding, 0, NULL, INT64_MAX, NULL
                ) != length) {
                return 0x100;
            }
            if ((__CFDefaultSystemEncoding & 1) != 0) {
                return 0;
            }
            OCFStringGetSystemEncoding();
            return (OCFStringEncoding)(__CFDefaultSystemEncoding - 1);
        }
    }
    if (encoding == 0xffffffff) {
        return __OCFStringComputeEightBitStringEncoding();
    }
    return encoding;
}

// Native CFStringGetFastestEncoding, 0x1c021bb3c. External. Objective-C strings answer
// -_fastestEncodingInCFStringEncoding; CF strings the eight-bit encoding or kCFStringEncodingUnicode.
OCFStringEncoding OCFStringGetFastestEncoding(OCFStringRef theString) {
    if (isObjCString(theString)) {
        return ((OCFStringEncoding (*)(id, SEL))objc_msgSend)(
            (id)theString, sel_getUid("_fastestEncodingInCFStringEncoding")
        );
    }
    if (stringIsUnicode(theString)) {
        return 0x100;
    }
    return eightBitStringEncoding();
}

// Native CFStringGetIntValue, 0x1c021bba8. External. Scans from index 0 of an inline buffer over the whole
// string with no locale (32-bit result); 0 when the scan fails.
SInt32 OCFStringGetIntValue(OCFStringRef str) {
    SInt32 idx = 0;
    OCFStringInlineBuffer buf = {0};
    initInlineBuffer(str, &buf, (OCFRange){0, OCFStringGetLength(str)});
    SInt32 result = 0;
    return __CFStringScanInteger(&buf, NULL, &idx, false, &result) ? result : 0;
}

// Native CFStringGetDoubleValue, 0x1c021bc78. External. As CFStringGetIntValue with __CFStringScanDouble;
// +0.0 when the scan fails (FCSEL with a zero register).
double OCFStringGetDoubleValue(OCFStringRef str) {
    SInt32 idx = 0;
    OCFStringInlineBuffer buf = {0};
    initInlineBuffer(str, &buf, (OCFRange){0, OCFStringGetLength(str)});
    double result = 0.0;
    return __CFStringScanDouble(&buf, NULL, &idx, &result) ? result : 0.0;
}

// Native CFStringInsert, 0x1c021bd4c. External. Objective-C strings answer -insertString:atIndex:. An
// immutable CF string logs a fault through _CFOSLog (CFStringInsert.cold.1, 0x1c030c5f0, with
// OUTLINED_FUNCTION_0_27/1_19/2_16: _os_log_fault_impl, type 0x11, a 12-byte argument buffer holding the
// function name) and is left unchanged.
void OCFStringInsert(OCFMutableStringRef str, OCFIndex idx, OCFStringRef insertedStr) {
    if (isObjCString(str)) {
        ((void (*)(id, SEL, OCFStringRef, OCFIndex))objc_msgSend)(
            (id)str, sel_getUid("insertString:atIndex:"), insertedStr, idx
        );
        return;
    }
    if (!stringIsMutable(str)) {
        os_log_fault(
            _CFOSLog(),
            "CFString: %s(): Expect mutable string",
            "void CFStringInsert(CFMutableStringRef, CFIndex, CFStringRef)"
        );
        return;
    }
    replaceInRange(str, (OCFRange){idx, 0}, insertedStr);
}

// Native CFStringDelete, 0x1c021bfb0. External. Objective-C strings answer -deleteCharactersInRange:; an
// immutable CF string logs the fault of CFStringDelete.cold.1 (0x1c030c62c); a mutable one shrinks over the
// range.
void OCFStringDelete(OCFMutableStringRef theString, OCFRange range) {
    if (isObjCString(theString)) {
        ((void (*)(id, SEL, OCFRange))objc_msgSend)(
            (id)theString, sel_getUid("deleteCharactersInRange:"), range
        );
        return;
    }
    if (!stringIsMutable(theString)) {
        os_log_fault(
            _CFOSLog(),
            "CFString: %s(): Expect mutable string",
            "void CFStringDelete(CFMutableStringRef, CFRange)"
        );
        return;
    }
    __CFStringChangeSizeMultiple(theString, &range, 1, 0, false);
}

// Native CFStringReplace, 0x1c021c0a4. External. Objective-C strings answer
// -replaceCharactersInRange:withString:; an immutable CF string logs the fault of CFStringReplace.cold.1
// (0x1c030c668).
void OCFStringReplace(OCFMutableStringRef theString, OCFRange range, OCFStringRef replacement) {
    if (isObjCString(theString)) {
        ((void (*)(id, SEL, OCFRange, OCFStringRef))objc_msgSend)(
            (id)theString, sel_getUid("replaceCharactersInRange:withString:"), range, replacement
        );
        return;
    }
    if (!stringIsMutable(theString)) {
        os_log_fault(
            _CFOSLog(),
            "CFString: %s(): Expect mutable string",
            "void CFStringReplace(CFMutableStringRef, CFRange, CFStringRef)"
        );
        return;
    }
    replaceInRange(theString, range, replacement);
}

// Native CFStringReplaceAll, 0x1c021c310. External. Objective-C strings answer -setString:; an immutable CF
// string logs the fault of CFStringReplaceAll.cold.1 (0x1c030c6a4). The current length is read before the
// replacement is copied.
void OCFStringReplaceAll(OCFMutableStringRef theString, OCFStringRef replacement) {
    if (isObjCString(theString)) {
        ((void (*)(id, SEL, OCFStringRef))objc_msgSend)((id)theString, sel_getUid("setString:"), replacement);
        return;
    }
    if (!stringIsMutable(theString)) {
        os_log_fault(
            _CFOSLog(),
            "CFString: %s(): Expect mutable string",
            "void CFStringReplaceAll(CFMutableStringRef, CFStringRef)"
        );
        return;
    }
    replaceInRange(theString, (OCFRange){0, stringLength(theString)}, replacement);
}

// Native CFStringAppend, 0x1c021c5c8. External. Objective-C strings answer -appendString:; an immutable CF
// string logs the fault of CFStringAppend.cold.1 (0x1c030c6e0).
void OCFStringAppend(OCFMutableStringRef theString, OCFStringRef appendedString) {
    if (isObjCString(theString)) {
        ((void (*)(id, SEL, OCFStringRef))objc_msgSend)(
            (id)theString, sel_getUid("appendString:"), appendedString
        );
        return;
    }
    if (!stringIsMutable(theString)) {
        os_log_fault(
            _CFOSLog(),
            "CFString: %s(): Expect mutable string",
            "void CFStringAppend(CFMutableStringRef, CFStringRef)"
        );
        return;
    }
    replaceInRange(theString, (OCFRange){stringLength(theString), 0}, appendedString);
}

// Native CFStringAppendCharacters, 0x1c021c888. External. Objective-C strings answer
// -appendCharacters:length:; an immutable CF string logs the fault of CFStringAppendCharacters.cold.1
// (0x1c030c71c). Unicode contents receive the characters with memmove; eight-bit contents stay eight-bit when
// every appended character is below 0x80 (each stored as its low byte after the length-byte skip) and become
// Unicode otherwise.
void OCFStringAppendCharacters(OCFMutableStringRef theString, const UniChar *chars, OCFIndex numChars) {
    if (isObjCString(theString)) {
        ((void (*)(id, SEL, const UniChar *, OCFIndex))objc_msgSend)(
            (id)theString, sel_getUid("appendCharacters:length:"), chars, numChars
        );
        return;
    }
    if (!stringIsMutable(theString)) {
        os_log_fault(
            _CFOSLog(),
            "CFString: %s(): Expect mutable string",
            "void CFStringAppendCharacters(CFMutableStringRef, const UniChar *, CFIndex)"
        );
        return;
    }
    OCFIndex strLength = stringLength(theString);
    OCFRange range = {strLength, 0};
    if (stringIsUnicode(theString)) {
        __CFStringChangeSizeMultiple(theString, &range, 1, numChars, true);
        memmove((UniChar *)stringContents(theString) + strLength, chars, numChars * sizeof(UniChar));
        return;
    }
    bool isASCII = true;
    for (OCFIndex idx = 0; isASCII && idx < numChars; ++idx) {
        isASCII = chars[idx] < 0x80;
    }
    __CFStringChangeSizeMultiple(theString, &range, 1, numChars, !isASCII);
    if (!isASCII) {
        memmove((UniChar *)stringContents(theString) + strLength, chars, numChars * sizeof(UniChar));
        return;
    }
    uint8_t *contents = (uint8_t *)stringContents(theString);
    uintptr_t skip = skipAnyLengthByte(theString);
    if (numChars >= 1) {
        contents += strLength + skip;
        for (OCFIndex idx = 0; idx < numChars; ++idx) {
            contents[idx] = (uint8_t)chars[idx];
        }
    }
}

// Native __CFStringAppendBytes, 0x1c021cb50. External. ASCII or the eight-bit encoding append the bytes as
// they are; kCFStringEncodingUnicode appends length / 2 characters, demoted to bytes when all are below 0x80;
// any other encoding is decoded by __CFStringDecodeByteStream3 into a variable-width buffer that uses the
// default allocator (alwaysUnicode: the receiver's info bit 4, read even for an Objective-C receiver) and a
// failure returns. An Objective-C receiver gets -_cfAppendCString:length: for bytes or
// -appendCharacters:length: for characters. A CF receiver is not tested for mutability; it grows (Unicode
// when the appended text or the receiver is Unicode) and receives the text; decoded storage that the decoder
// allocated is freed with the default allocator.
// CoreFoundation defect: 0x1c021cc1c and 0x1c021cc48 return after the Objective-C send without freeing the
// decoded storage (a leak whenever the decoder allocated); the restoration frees it on that path too.
// Confidence: verified (both sends branch to the epilogue at 0x1c021cf0c).
void __OCFStringAppendBytes(
    OCFMutableStringRef string, const char *cStr, OCFIndex appendedLength, OCFStringEncoding encoding
) {
    bool appendedIsUnicode = false;
    bool freeCStrWhenDone = false;
    bool demoteAppendedUnicode = false;
    VarWidthCharBuffer vBuf = {0};
    if (encoding == 0x600 || encoding == eightBitStringEncoding()) {
        // The bytes are appended as they are; appendedLength counts them.
    } else if (encoding == 0x100) {
        const UniChar *chars = (const UniChar *)cStr;
        OCFIndex length = (OCFIndex)((uint64_t)appendedLength >> 1);
        bool isASCII = true;
        for (OCFIndex idx = 0; isASCII && idx < length; ++idx) {
            isASCII = chars[idx] < 0x80;
        }
        demoteAppendedUnicode = isASCII;
        appendedIsUnicode = !isASCII;
        appendedLength = length;
    } else {
        Boolean usingPassedInMemory = false;
        OCFAllocatorRef current = _OCFGetTSD(1);
        vBuf.allocator = current != NULL ? current : kOCFAllocatorSystemDefault;
        vBuf.chars.unicode = NULL;
        if (!__OCFStringDecodeByteStream3(
                (const UInt8 *)cStr,
                appendedLength,
                encoding,
                (Boolean)((stringInfo(string) >> 4) & 1),
                &vBuf,
                &usingPassedInMemory,
                0
            )) {
            return;
        }
        appendedLength = vBuf.numChars;
        appendedIsUnicode = !vBuf.isASCII;
        cStr = (const char *)vBuf.chars.ascii;
        freeCStrWhenDone = vBuf.shouldFreeChars && !usingPassedInMemory;
    }
    if (isObjCString(string)) {
        if (!demoteAppendedUnicode && !appendedIsUnicode) {
            ((void (*)(id, SEL, const char *, OCFIndex))objc_msgSend)(
                (id)string, sel_getUid("_cfAppendCString:length:"), cStr, appendedLength
            );
        } else {
            ((void (*)(id, SEL, const char *, OCFIndex))objc_msgSend)(
                (id)string, sel_getUid("appendCharacters:length:"), cStr, appendedLength
            );
        }
    } else {
        OCFIndex strLength = stringLength(string);
        OCFRange range = {strLength, 0};
        Boolean makeUnicode = appendedIsUnicode ? true : (Boolean)((stringInfo(string) >> 4) & 1);
        __CFStringChangeSizeMultiple(string, &range, 1, appendedLength, makeUnicode);
        if (stringIsUnicode(string)) {
            UniChar *contents = (UniChar *)stringContents(string) + strLength;
            if (appendedIsUnicode) {
                memmove(contents, cStr, appendedLength * sizeof(UniChar));
            } else {
                __OCFStrConvertBytesToUnicode((const UInt8 *)cStr, contents, appendedLength);
            }
        } else if (demoteAppendedUnicode) {
            uint8_t *contents = (uint8_t *)stringContents(string);
            uintptr_t skip = skipAnyLengthByte(string);
            if (appendedLength >= 1) {
                contents += strLength + skip;
                const UniChar *chars = (const UniChar *)cStr;
                for (OCFIndex idx = 0; idx < appendedLength; ++idx) {
                    contents[idx] = (uint8_t)chars[idx];
                }
            }
        } else {
            uint8_t *contents = (uint8_t *)stringContents(string) + strLength;
            memmove(contents + skipAnyLengthByte(string), cStr, appendedLength);
        }
    }
    if (freeCStrWhenDone) {
        OCFAllocatorRef current = _OCFGetTSD(1);
        OCFAllocatorDeallocate(current != NULL ? current : kOCFAllocatorSystemDefault, (void *)cStr);
    }
}

// Native CFStringAppendCString, 0x1c021cf94. External. No Objective-C dispatch: the receiver's info word is
// tested for mutability first (kept: read for any receiver), an immutable string logs the fault of
// CFStringAppendCString.cold.1 (0x1c030c794), and a mutable one takes __CFStringAppendBytes with strlen.
void OCFStringAppendCString(OCFMutableStringRef theString, const char *cStr, OCFStringEncoding encoding) {
    if (!stringIsMutable(theString)) {
        os_log_fault(
            _CFOSLog(),
            "CFString: %s(): Expect mutable string",
            "void CFStringAppendCString(CFMutableStringRef, const char *, CFStringEncoding)"
        );
        return;
    }
    __OCFStringAppendBytes(theString, cStr, (OCFIndex)strlen(cStr), encoding);
}

// Native CFStringFindAndReplace, 0x1c021d0f0. External. Objective-C strings answer
// -replaceOccurrencesOfString:withString:options:range:. The matches are collected first (a 62-range stack
// buffer, then storage grown to 2 * capacity + 8 ranges through __CFSafelyReallocateWithAllocatorTyped with
// the system default allocator and type ID 0x1000040451B5BE8 in the fourth argument register, the stack
// ranges copied over once); the search end is location + length truncated to an unsigned 32-bit value (ADD
// W22, W25, W27). Backward matches are reversed with 32-bit indexes. Mutability is tested only then: an
// immutable string logs the fault of CFStringFindAndReplace.cold.1 (0x1c030c80c) and still answers the match
// count. The replacement is stored once and copied to every other range, each offset by the earlier length
// changes.
OCFIndex OCFStringFindAndReplace(
    OCFMutableStringRef theString,
    OCFStringRef stringToFind,
    OCFStringRef replacementString,
    OCFRange rangeToSearch,
    OCFStringCompareFlags compareOptions
) {
    if (isObjCString(theString)) {
        return ((OCFIndex (*)(id, SEL, OCFStringRef, OCFStringRef, OCFStringCompareFlags, OCFRange))
                    objc_msgSend)(
            (id)theString,
            sel_getUid("replaceOccurrencesOfString:withString:options:range:"),
            stringToFind,
            replacementString,
            compareOptions,
            rangeToSearch
        );
    }
    OCFRange foundRange = {0, 0};
    if (rangeToSearch.length < 1) {
        return 0;
    }
    OCFAllocatorRef alloc = kOCFAllocatorSystemDefault;
    OCFRange rangeBuffer[62];
    OCFRange *ranges = rangeBuffer;
    OCFIndex foundCount = 0;
    OCFIndex capacity = 62;
    uint32_t endIndex = (uint32_t)(rangeToSearch.location + rangeToSearch.length);
    while (true) {
        if (!OCFStringFindWithOptionsAndLocale(
                theString, stringToFind, rangeToSearch, compareOptions, NULL, &foundRange
            )) {
            if (foundCount == 0) {
                return 0;
            }
            break;
        }
        if ((compareOptions & kOCFCompareBackwards) != 0) {
            rangeToSearch.length = foundRange.location - rangeToSearch.location;
        } else {
            rangeToSearch.location = foundRange.location + foundRange.length;
            rangeToSearch.length = (OCFIndex)endIndex - rangeToSearch.location;
        }
        if (foundCount >= capacity) {
            capacity = 2 * capacity + 8;
            bool wasLocal = ranges == rangeBuffer;
            OCFRange *grown = __OCFSafelyReallocateWithAllocatorTyped(
                alloc, wasLocal ? NULL : ranges, capacity * 16, 0x1000040451B5BE8, 0, NULL
            );
            ranges = grown;
            if (wasLocal) {
                memmove(grown, rangeBuffer, 0x3E0);
            }
        }
        ranges[foundCount] = foundRange;
        ++foundCount;
        if (rangeToSearch.length <= 0) {
            break;
        }
    }
    if ((compareOptions & kOCFCompareBackwards) != 0) {
        for (int i = 0, j = (int)foundCount - 1; i < j; ++i, --j) {
            OCFRange temp = {0, 0};
            temp = ranges[i];
            ranges[i] = ranges[j];
            ranges[j] = temp;
        }
    }
    if (!stringIsMutable(theString)) {
        os_log_fault(
            _CFOSLog(),
            "CFString: %s(): Expect mutable string",
            "CFIndex CFStringFindAndReplace(CFMutableStringRef, CFStringRef, CFStringRef, CFRange, "
            "CFStringCompareFlags)"
        );
    } else {
        OCFStringRef copy = NULL;
        if (replacementString == theString) {
            copy = OCFStringCreateCopy(alloc, replacementString);
            replacementString = copy;
        }
        OCFIndex replacementLength = OCFStringGetLength(replacementString);
        Boolean makeUnicode = replacementLength >= 1 && CFStrIsUnicode(replacementString) != 0;
        __CFStringChangeSizeMultiple(theString, ranges, foundCount, replacementLength, makeUnicode);
        if (stringIsUnicode(theString)) {
            UniChar *contents = (UniChar *)stringContents(theString);
            UniChar *firstReplacement = contents + ranges[0].location;
            OCFStringGetCharacters(replacementString, (OCFRange){0, replacementLength}, firstReplacement);
            for (OCFIndex cnt = 1; cnt < foundCount; ++cnt) {
                contents += replacementLength - ranges[cnt - 1].length;
                memmove(
                    contents + ranges[cnt].location, firstReplacement, replacementLength * sizeof(UniChar)
                );
            }
        } else {
            uint8_t *contents = (uint8_t *)stringContents(theString);
            uint8_t *firstReplacement = contents + ranges[0].location;
            uintptr_t skip = skipAnyLengthByte(theString);
            OCFStringEncoding encoding = eightBitStringEncoding();
            OCFStringGetBytes(
                replacementString,
                (OCFRange){0, replacementLength},
                encoding,
                0,
                false,
                firstReplacement + skip,
                replacementLength,
                NULL
            );
            uintptr_t secondSkip = skipAnyLengthByte(theString);
            if (foundCount != 1) {
                contents += secondSkip;
                for (OCFIndex cnt = 1; cnt < foundCount; ++cnt) {
                    contents += replacementLength - ranges[cnt - 1].length;
                    memmove(contents + ranges[cnt].location, firstReplacement + skip, replacementLength);
                }
            }
        }
        if (copy != NULL) {
            OCFRelease(copy);
        }
    }
    if (ranges != rangeBuffer) {
        OCFAllocatorDeallocate(alloc, ranges);
    }
    return foundCount;
}

// Native __CFStringCheckAndReplace, 0x1c021d564 (corpus delta capture 7). External. CF strings only (no
// dispatch). An immutable string answers 1, a NULL replacement 2, and a range whose end (unsigned) is below
// its location or beyond the length 3. A replacement that is the string itself is replaced by a
// system-default copy (released at the end). The range becomes the replacement's length
// (__CFStringChangeSizeMultiple, Unicode when the non-empty replacement answers CFStrIsUnicode); eight-bit
// contents are filled with CFStringGetBytes in the eight-bit encoding at the location (past any length
// byte), UTF-16 contents with CFStringGetCharacters when the buffer is not NULL. 0 is answered.
int __OCFStringCheckAndReplace(
    OCFMutableStringRef string, OCFIndex location, OCFIndex length, OCFStringRef replacement
) {
    if (!stringIsMutable(string)) {
        return 1;
    }
    if (replacement == NULL) {
        return 2;
    }
    uint64_t end = (uint64_t)location + (uint64_t)length;
    if (end < (uint64_t)location || end > (uint64_t)stringLength(string)) {
        return 3;
    }
    OCFStringRef copy = NULL;
    if (replacement == string) {
        copy = OCFStringCreateCopy(kOCFAllocatorSystemDefault, replacement);
        replacement = copy;
    }
    OCFIndex replacementLength = OCFStringGetLength(replacement);
    Boolean makeUnicode = replacementLength >= 1 ? CFStrIsUnicode(replacement) != 0 : false;
    OCFRange range = {location, length};
    __CFStringChangeSizeMultiple(string, &range, 1, replacementLength, makeUnicode);
    if (stringIsEightBit(string)) {
        uint8_t *contents = (uint8_t *)stringContents(string);
        OCFStringEncoding encoding = stringEightBitEncoding();
        OCFIndex skip = (OCFIndex)((stringInfo(string) >> 2) & 1);
        OCFStringGetBytes(
            replacement,
            (OCFRange){0, replacementLength},
            encoding,
            0,
            false,
            contents + location + skip,
            replacementLength,
            NULL
        );
    } else {
        UniChar *contents = (UniChar *)stringContents(string);
        if (contents != NULL) {
            OCFStringGetCharacters(replacement, (OCFRange){0, replacementLength}, contents + location);
        }
    }
    if (copy != NULL) {
        OCFRelease(copy);
    }
    return 0;
}

// Native CFStringPad, 0x1c021d7d4. External. Objective-C strings answer -_cfPad:length:padIndex:; an
// immutable CF string logs the fault of CFStringPad.cold.1 (0x1c030c848). A shorter target length truncates;
// a longer one appends copies of the pad string starting at indexIntoPad (Unicode when the receiver or the
// pad is Unicode, and always for an Objective-C pad).
void OCFStringPad(
    OCFMutableStringRef theString, OCFStringRef padString, OCFIndex length, OCFIndex indexIntoPad
) {
    if (isObjCString(theString)) {
        ((void (*)(id, SEL, OCFStringRef, OCFIndex, OCFIndex))objc_msgSend)(
            (id)theString, sel_getUid("_cfPad:length:padIndex:"), padString, length, indexIntoPad
        );
        return;
    }
    if (!stringIsMutable(theString)) {
        os_log_fault(
            _CFOSLog(),
            "CFString: %s(): Expect mutable string",
            "void CFStringPad(CFMutableStringRef, CFStringRef, CFIndex, CFIndex)"
        );
        return;
    }
    OCFIndex originalLength = stringLength(theString);
    if (length < originalLength) {
        OCFRange range = {length, originalLength - length};
        __CFStringChangeSizeMultiple(theString, &range, 1, 0, false);
        return;
    }
    if (originalLength >= length) {
        return;
    }
    OCFIndex padRemaining = length - originalLength;
    OCFIndex padStringLength;
    bool isUnicode;
    if (isObjCString(padString)) {
        padStringLength = OCFStringGetLength(padString);
        isUnicode = true;
    } else {
        padStringLength = stringLength(padString);
        isUnicode = stringIsUnicode(theString) ? true : stringIsUnicode(padString);
    }
    unsigned charShift = isUnicode ? 1 : 0;
    OCFRange range = {originalLength, 0};
    __CFStringChangeSizeMultiple(theString, &range, 1, padRemaining, isUnicode);
    uint8_t *contents = (uint8_t *)stringContents(theString);
    uintptr_t skip = skipAnyLengthByte(theString);
    OCFIndex padLength = padStringLength - indexIntoPad;
    padLength = padRemaining < padLength ? padRemaining : padLength;
    contents += (originalLength << charShift) + skip;
    do {
        if (isUnicode) {
            OCFStringGetCharacters(padString, (OCFRange){indexIntoPad, padLength}, (UniChar *)contents);
        } else {
            OCFStringEncoding encoding = eightBitStringEncoding();
            OCFStringGetBytes(
                padString,
                (OCFRange){indexIntoPad, padLength},
                encoding,
                0,
                false,
                contents,
                padRemaining,
                NULL
            );
        }
        contents += padLength << charShift;
        padRemaining -= padLength;
        indexIntoPad = 0;
        padLength = padRemaining < padLength ? padRemaining : padStringLength;
    } while (padRemaining > 0);
}

// Native CFStringLowercase, 0x1c021e314. External. The info word is read before the Objective-C dispatch
// (kept: LDAR at 0x1c021e350 precedes the tagged-pointer test, so a tagged receiver faults there).
// Objective-C strings answer -_cfLowercase:; an immutable CF string logs the fault of
// CFStringLowercase.cold.1 (0x1c030c8fc). A locale gives a language code through
// _CFStrGetSpecialCaseHandlingLanguageIdentifierForLocale(locale, false). Without one, eight-bit contents
// map 'A'-'Z' in place until a byte above 0x7F; the rest is converted to Unicode and mapped character by
// character with CFUniCharMapCaseTo (type 0, an eight-unit buffer; conditional flags for a language code or
// U+03A3), adjusting the string for mappings of other lengths. After a BMP character maps to nothing the
// contents pointer is reloaded.
void OCFStringLowercase(OCFMutableStringRef theString, OCFLocaleRef locale) {
    uint64_t initialInfo = stringInfo(theString);
    if (isObjCString(theString)) {
        ((void (*)(id, SEL, OCFLocaleRef))objc_msgSend)((id)theString, sel_getUid("_cfLowercase:"), locale);
        return;
    }
    if (!stringIsMutable(theString)) {
        os_log_fault(
            _CFOSLog(),
            "CFString: %s(): Expect mutable string",
            "void CFStringLowercase(CFMutableStringRef, CFLocaleRef)"
        );
        return;
    }
    bool isEightBit = (initialInfo & kStringInfoUnicode) == 0;
    OCFIndex length = stringLength(theString);
    const uint8_t *langCode = NULL;
    if (locale != NULL) {
        langCode = (const uint8_t *)_CFStrGetSpecialCaseHandlingLanguageIdentifierForLocale(locale, false);
    }
    OCFIndex currentIndex = 0;
    if (langCode == NULL && isEightBit) {
        uint8_t *contents = (uint8_t *)stringContents(theString);
        contents += skipAnyLengthByte(theString);
        for (; currentIndex < length; ++currentIndex) {
            uint8_t ch = contents[currentIndex];
            if ((unsigned)(ch - 'A') <= 25) {
                contents[currentIndex] = ch | 0x20;
            } else if ((ch & 0x80) != 0) {
                break;
            }
        }
    }
    if (currentIndex >= length) {
        return;
    }
    if (isEightBit) {
        OCFRange range = {0, 0};
        __CFStringChangeSizeMultiple(theString, &range, 1, 0, true);
    }
    UniChar *contents = (UniChar *)stringContents(theString);
    UniChar mappedCharacters[8];
    uint32_t flags = 0;
    for (; currentIndex < length; ++currentIndex) {
        UTF32Char currentChar = contents[currentIndex];
        if ((currentChar & 0xFC00) == 0xD800 && currentIndex + 1 < length &&
            (contents[currentIndex + 1] & 0xFC00) == 0xDC00) {
            currentChar = (currentChar << 10) + contents[currentIndex + 1] + 0xFCA02400;
        }
        if (langCode != NULL || currentChar == 0x3A3) {
            flags = CFUniCharGetConditionalCaseMappingFlags(
                currentChar, contents, currentIndex, length, 0, langCode, flags
            );
        } else {
            flags = 0;
        }
        OCFIndex mappedLength = OCFUniCharMapCaseTo(currentChar, mappedCharacters, 8, 0, flags, langCode);
        if (mappedLength >= 1) {
            contents[currentIndex] = mappedCharacters[0];
        }
        if (currentChar >= 0x10000) {
            if (mappedLength == 2) {
                contents[++currentIndex] = mappedCharacters[1];
            } else if (mappedLength == 1) {
                OCFRange range = {currentIndex + 1, 1};
                __CFStringChangeSizeMultiple(theString, &range, 1, 0, true);
                contents = (UniChar *)stringContents(theString);
                --length;
            } else if (mappedLength == 0) {
                OCFRange range = {currentIndex, 2};
                __CFStringChangeSizeMultiple(theString, &range, 1, 0, true);
                contents = (UniChar *)stringContents(theString);
                length -= 2;
            } else {
                OCFRange range = {currentIndex + 1, 0};
                __CFStringChangeSizeMultiple(theString, &range, 1, mappedLength - 2, true);
                contents = (UniChar *)stringContents(theString);
                memmove(
                    contents + currentIndex + 1, mappedCharacters + 1, (mappedLength - 1) * sizeof(UniChar)
                );
                length += mappedLength - 2;
                currentIndex += mappedLength - 1;
            }
        } else if (mappedLength == 0) {
            OCFRange range = {currentIndex, 1};
            __CFStringChangeSizeMultiple(theString, &range, 1, 0, true);
            contents = (UniChar *)stringContents(theString);
            --length;
        } else if (mappedLength >= 2) {
            OCFRange range = {currentIndex + 1, 0};
            __CFStringChangeSizeMultiple(theString, &range, 1, mappedLength - 1, true);
            contents = (UniChar *)stringContents(theString);
            memmove(contents + currentIndex + 1, mappedCharacters + 1, (mappedLength - 1) * sizeof(UniChar));
            length += mappedLength - 1;
            currentIndex += mappedLength - 1;
        }
    }
}

// Native CFStringUppercase, 0x1c021e824. External. As CFStringLowercase with -_cfUppercase:, the fault of
// CFStringUppercase.cold.1 (0x1c030c938), case-mapping type 1, the eight-bit fast path mapping 'a'-'z' on
// signed bytes (LDRSB) until a negative byte, and conditional flags only for a language code.
void OCFStringUppercase(OCFMutableStringRef theString, OCFLocaleRef locale) {
    uint64_t initialInfo = stringInfo(theString);
    if (isObjCString(theString)) {
        ((void (*)(id, SEL, OCFLocaleRef))objc_msgSend)((id)theString, sel_getUid("_cfUppercase:"), locale);
        return;
    }
    if (!stringIsMutable(theString)) {
        os_log_fault(
            _CFOSLog(),
            "CFString: %s(): Expect mutable string",
            "void CFStringUppercase(CFMutableStringRef, CFLocaleRef)"
        );
        return;
    }
    bool isEightBit = (initialInfo & kStringInfoUnicode) == 0;
    OCFIndex length = stringLength(theString);
    const uint8_t *langCode = NULL;
    if (locale != NULL) {
        langCode = (const uint8_t *)_CFStrGetSpecialCaseHandlingLanguageIdentifierForLocale(locale, false);
    }
    OCFIndex currentIndex = 0;
    if (langCode == NULL && isEightBit) {
        int8_t *contents = (int8_t *)stringContents(theString);
        contents += skipAnyLengthByte(theString);
        for (; currentIndex < length; ++currentIndex) {
            int32_t ch = contents[currentIndex];
            if ((uint32_t)(ch - 'a') <= 25) {
                contents[currentIndex] = (int8_t)(ch - 0x20);
            } else if (ch < 0) {
                break;
            }
        }
    }
    if (currentIndex >= length) {
        return;
    }
    if (isEightBit) {
        OCFRange range = {0, 0};
        __CFStringChangeSizeMultiple(theString, &range, 1, 0, true);
    }
    UniChar *contents = (UniChar *)stringContents(theString);
    UniChar mappedCharacters[8];
    uint32_t flags = 0;
    for (; currentIndex < length; ++currentIndex) {
        UTF32Char currentChar = contents[currentIndex];
        if ((currentChar & 0xFC00) == 0xD800 && currentIndex + 1 < length &&
            (contents[currentIndex + 1] & 0xFC00) == 0xDC00) {
            currentChar = (currentChar << 10) + contents[currentIndex + 1] + 0xFCA02400;
        }
        if (langCode != NULL) {
            flags = CFUniCharGetConditionalCaseMappingFlags(
                currentChar, contents, currentIndex, length, 1, langCode, flags
            );
        } else {
            flags = 0;
        }
        OCFIndex mappedLength = OCFUniCharMapCaseTo(currentChar, mappedCharacters, 8, 1, flags, langCode);
        if (mappedLength >= 1) {
            contents[currentIndex] = mappedCharacters[0];
        }
        if (currentChar >= 0x10000) {
            if (mappedLength == 2) {
                contents[++currentIndex] = mappedCharacters[1];
            } else if (mappedLength == 1) {
                OCFRange range = {currentIndex + 1, 1};
                __CFStringChangeSizeMultiple(theString, &range, 1, 0, true);
                contents = (UniChar *)stringContents(theString);
                --length;
            } else if (mappedLength == 0) {
                OCFRange range = {currentIndex, 2};
                __CFStringChangeSizeMultiple(theString, &range, 1, 0, true);
                contents = (UniChar *)stringContents(theString);
                length -= 2;
            } else {
                OCFRange range = {currentIndex + 1, 0};
                __CFStringChangeSizeMultiple(theString, &range, 1, mappedLength - 2, true);
                contents = (UniChar *)stringContents(theString);
                memmove(
                    contents + currentIndex + 1, mappedCharacters + 1, (mappedLength - 1) * sizeof(UniChar)
                );
                length += mappedLength - 2;
                currentIndex += mappedLength - 1;
            }
        } else if (mappedLength == 0) {
            OCFRange range = {currentIndex, 1};
            __CFStringChangeSizeMultiple(theString, &range, 1, 0, true);
            contents = (UniChar *)stringContents(theString);
            --length;
        } else if (mappedLength >= 2) {
            OCFRange range = {currentIndex + 1, 0};
            __CFStringChangeSizeMultiple(theString, &range, 1, mappedLength - 1, true);
            contents = (UniChar *)stringContents(theString);
            memmove(contents + currentIndex + 1, mappedCharacters + 1, (mappedLength - 1) * sizeof(UniChar));
            length += mappedLength - 1;
            currentIndex += mappedLength - 1;
        }
    }
}

// Native CFStringCapitalize, 0x1c021ed30. External. iOS Device divergence: unread.
// .cold.1 at 0x1c030c974 folds in the fault-log fragments OUTLINED_FUNCTION_0_27,
// OUTLINED_FUNCTION_1_19 and OUTLINED_FUNCTION_2_16. The initial info load precedes the tagged test (kept).
void OCFStringCapitalize(OCFMutableStringRef theString, OCFLocaleRef locale) {
    uint64_t initialInfo = stringInfo(theString);
    if (isObjCString(theString)) {
        ((void (*)(id, SEL, OCFLocaleRef))objc_msgSend)((id)theString, sel_getUid("_cfCapitalize:"), locale);
        return;
    }
    if (!stringIsMutable(theString)) {
        os_log_fault(
            _CFOSLog(),
            "CFString: %s(): Expect mutable string",
            "void CFStringCapitalize(CFMutableStringRef, CFLocaleRef)"
        );
        return;
    }

    OCFIndex length = stringLength(theString);
    const uint8_t *caseIgnorable = OCFUniCharGetBitmapPtrForPlane(0x6d, 0);
    const uint8_t *language = NULL;
    if (locale != NULL) {
        language = (const uint8_t *)_CFStrGetSpecialCaseHandlingLanguageIdentifierForLocale(locale, false);
    }
    bool eightBit = (initialInfo & kStringInfoUnicode) == 0;
    bool precedingLetter = false;
    OCFIndex index = 0;
    if (language == NULL && eightBit) {
        uint8_t *bytes = (uint8_t *)stringContents(theString) + skipAnyLengthByte(theString);
        for (; index < length; ++index) {
            uint8_t byte = bytes[index];
            if (byte >= 0x80) {
                break;
            }
            if (byte >= 'A' && byte <= 'Z') {
                bytes[index] = byte | (precedingLetter ? 0x20 : 0);
                precedingLetter = true;
            } else if (byte >= 'a' && byte <= 'z') {
                bytes[index] = byte - (precedingLetter ? 0 : 0x20);
                precedingLetter = true;
            } else if (caseIgnorable == NULL || (caseIgnorable[byte >> 3] & (1u << (byte & 7))) == 0) {
                precedingLetter = false;
            }
        }
    }
    if (index >= length) {
        return;
    }
    if (eightBit) {
        OCFRange range = {0, 0};
        __CFStringChangeSizeMultiple(theString, &range, 1, 0, true);
    }

    UniChar *contents = (UniChar *)stringContents(theString);
    uint32_t flags = 0;
    for (; index < length; ++index) {
        UTF32Char character = contents[index];
        if ((character & 0xfc00) == 0xd800 && index + 1 < length &&
            (contents[index + 1] & 0xfc00) == 0xdc00) {
            character = (character << 10) + contents[index + 1] + 0xfca02400;
        }
        uint32_t mapping = precedingLetter ? 0 : 2;
        if (language != NULL || (character == 0x3a3 && precedingLetter)) {
            flags = CFUniCharGetConditionalCaseMappingFlags(
                character, contents, index, length, mapping, language, flags
            );
        } else {
            flags = 0;
        }
        UniChar mapped[8];
        OCFIndex count = OCFUniCharMapCaseTo(character, mapped, 8, mapping, flags, language);
        if (count >= 1) {
            contents[index] = mapped[0];
        }
        OCFIndex originalCount = character >= 0x10000 ? 2 : 1;
        if (count < originalCount) {
            OCFRange removed = {index + count, originalCount - count};
            __CFStringChangeSizeMultiple(theString, &removed, 1, 0, true);
            contents = (UniChar *)stringContents(theString);
            length -= originalCount - count;
        } else if (count > originalCount) {
            OCFRange insertion = {index + 1, 0};
            __CFStringChangeSizeMultiple(theString, &insertion, 1, count - originalCount, true);
            contents = (UniChar *)stringContents(theString);
            memmove(contents + index + 1, mapped + 1, (size_t)(count - 1) * sizeof(UniChar));
            length += count - originalCount;
            index += count - 1;
        } else if (count == 2) {
            contents[++index] = mapped[1];
        }

        bool ignorable;
        if (character >= 0x10000) {
            ignorable = (OCFUniCharIsMemberOf(character, 0x6d) & 1) != 0;
        } else {
            ignorable =
                caseIgnorable != NULL && (caseIgnorable[character >> 3] & (1u << (character & 7))) != 0;
        }
        if (!ignorable) {
            precedingLetter = (OCFUniCharIsMemberOf(character, 7) & 1) != 0 ||
                              (OCFUniCharIsMemberOf(character, 6) & 1) != 0;
        }
    }
}

// Native CFStringGetFileSystemRepresentation, 0x1c0221cc0. External. A non-empty string, CF or Objective-C,
// always receives -getFileSystemRepresentation:maxLength: (there is no CF fast path); an empty string writes
// a single NUL when maxBufLen is not zero.
Boolean OCFStringGetFileSystemRepresentation(OCFStringRef string, char *buffer, OCFIndex maxBufLen) {
    if (OCFStringGetLength(string) != 0) {
        return ((Boolean (*)(id, SEL, char *, OCFIndex))objc_msgSend)(
            (id)string, sel_getUid("getFileSystemRepresentation:maxLength:"), buffer, maxBufLen
        );
    }
    if (maxBufLen == 0) {
        return false;
    }
    buffer[0] = 0;
    return true;
}

// Native _CFStringCreateByParsingMarkdownAndInflectingIfNeeded, 0x1c0221d78. External. The string is parsed
// only for a bundle whose identifier starts with "com.apple." (kCFCompareAnchored over the whole identifier)
// and when the string contains "^["; the third argument is passed through as the localization. Otherwise the
// string is retained and returned.
OCFStringRef _OCFStringCreateByParsingMarkdownAndInflectingIfNeeded(
    OCFStringRef string, OCFBundleRef bundle, OCFStringRef localization
) {
    if (bundle != NULL) {
        OCFStringRef identifier = OCFBundleGetIdentifier(bundle);
        if (identifier != NULL) {
            OCFRange range = {0, OCFStringGetLength(identifier)};
            if (OCFStringFindWithOptionsAndLocale(identifier, OF_STR("com.apple."), range, 8, NULL, NULL)) {
                OCFRange found = OCFStringFind(string, OF_STR("^["), 0);
                if (found.location != kOCFNotFound) {
                    const void *configuration = _OCFStringGetFormatSpecifierConfiguration(string);
                    return _CFStringCreateWithMarkdownAndConfiguration(
                        string, configuration, bundle, localization
                    );
                }
            }
        }
    }
    OCFRetain(string);
    return string;
}

// Native CFStringCopyStatisticalWritingDirections, 0x1c0221e54. Hidden (was private external). An array of
// one dictionary per paragraph (keys Location, Length, WritingDirection, HasMixedDirectionality and the six
// NumberOf... counts) from a per-character state machine over u_charDirection classes;
// appendParagraphRangeInfo and the inline-buffer accessors are inlined. Facts read from the assembly:
// - Class EN (2) for U+06F0-U+06F9 is treated as AN (5). Characters are read with
//   __CFStringGetLongCharacterFromInlineBuffer; the inline buffer's C-string pointer comes from
//   _CFStringGetCStringPtrInternal(string, ASCII, true, true).
// - A CR whose next unit is LF ends the paragraph with both (the character length becomes 2); any other unit
// read
//   after a CR is kept and classified by the next iteration without being read again, and an LF kept that way
//   is skipped.
// - The embedding stack holds the raw characters (no callbacks); "considerWordDirectionality" (0, 1, 0xff)
// and the
//   isolate flag follow the embeddings and isolates, and any other value is logged at level 2.
// - When the number range is started from the neutral range, the character length is added twice (0x1c0222568
//   and 0x1c0222834), kept.
// - The paragraph direction is 0 for a first character U+200E, 1 for U+200F, else from the first counted run
// and
//   the "LanguageAwareStringStatisticalLTR"/"RTL" features, where 0x100 means a right-to-left share of the
//   strong counts of at least 0.4 ((RTL numbers + RTL words) / ((LTR numbers + LTR words) + (RTL numbers +
//   RTL words))). The share's 0.15 bit (0x10000) is computed natively but never read; NumberOfWeakNumbers is
//   never incremented; the run start (0xa8), the number end (0xd0) and the byte at 0x50 are stored natively
//   but never read, and are not kept here.
// - The paragraph keys are created once (.cold.1, dispatch_once with
// ___appendParagraphRangeInfo_block_invoke,
//   0x1c0225d90: malloc_type_calloc(10, 8, 0x80040B8603338) filled with the ten constant strings).
// Folds the compiler-outlined piece _CFStringCopyStatisticalWritingDirections.cold.1 (0x1c030ca28).
OCFArrayRef CFStringCopyStatisticalWritingDirections(OCFStringRef string) {
    OCFIndex length = OCFStringGetLength(string);
    if (length == 0) {
        return OCFArrayCreate(kOCFAllocatorSystemDefault, NULL, 0, &kOCFTypeArrayCallBacks);
    }
    OCFStringInlineBuffer buffer;
    memset(buffer.buffer, 0, sizeof(buffer.buffer));
    buffer.theString = string;
    buffer.rangeToBuffer.location = 0;
    buffer.rangeToBuffer.length = length;
    buffer.directUniCharBuffer = OCFStringGetCharactersPtr(string);
    buffer.directCStringBuffer = NULL;
    if (buffer.directUniCharBuffer == NULL) {
        buffer.directCStringBuffer = _CFStringGetCStringPtrInternal(string, 0x600, true, true);
    }
    buffer.bufferedRangeStart = 0;
    buffer.bufferedRangeEnd = 0;

    // Counts in key order: left-to-right numbers, right-to-left numbers, weak numbers, left-to-right words,
    // right-to-left words, weak words.
    OCFIndex counts[6] = {0, 0, 0, 0, 0, 0};
    uint8_t hasMixedDirectionality = 0;
    uint8_t pendingMixedDirectionality = 0;
    OCFRange neutralRange = notFoundRange;
    OCFRange numberRange = notFoundRange;
    OCFIndex wordLength = 0;
    OCFIndex paragraphLocation = 0;
    OCFIndex paragraphLength = 0;
    int32_t firstCharacter = 0;
    uint8_t paragraphDirection = 0xFE;
    uint8_t wordDirection = 0xFF;
    uint8_t numberDirection = 0xFF;
    uint8_t considerWordDirectionality = 0xFF;
    uint8_t insideIsolate = 0;
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableArrayRef result = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
    OCFMutableArrayRef embeddings = OCFArrayCreateMutable(allocator, 0, NULL);
    uint8_t useOuterOverride = __atomic_load_n(&sUseOuterDirectionalOverride, __ATOMIC_ACQUIRE) & 1;
    uint8_t leftToRightFeature = _os_feature_enabled_impl("Foundation", "LanguageAwareStringStatisticalLTR");
    uint8_t rightToLeftFeature = _os_feature_enabled_impl("Foundation", "LanguageAwareStringStatisticalRTL");
    uint8_t arabicIndicDigitsFeature =
        _os_feature_enabled_impl("Foundation", "LanguageAwareStringStatisticalArabicIndicDigits");
    if (length >= 1) {
        UTF32Char ch = 0;
        OCFIndex index = 0;
        int32_t direction;
        int32_t cls;
        OCFIndex charLen;
        // Every path assigns these before reading them; the initial values only silence the compiler.
        uint32_t lookahead = 0;
        bool paragraphEnd = false;
        bool countRun = false;
        bool resetNeutral = false;
        bool resetNumber = false;
        uint8_t directionChanged = 0;
        bool insideWord = false;
        OCFIndex savedWordLength = 0;
        OCFIndex count = 0;
    loopTop:
        if (ch != 0) {
            ++index;
            direction = u_charDirection((int32_t)ch);
            if (ch == 10) {
                lookahead = 0;
                goto nextCharacter;
            }
        } else {
            OCFRange range;
            ch = __CFStringGetLongCharacterFromInlineBuffer(&buffer, length, index, &range);
            index = range.length + range.location;
            direction = u_charDirection((int32_t)ch);
        }
        charLen = ch >= 0x10000 ? 2 : 1;
        cls = direction == 2 ? (ch - 0x6f0 < 10 ? 5 : 2) : direction;
        countRun = true;
        if ((uint32_t)(cls - 7) > 15) {
            goto defaultClass;
        }
        lookahead = 0;
        paragraphEnd = false;
        resetNeutral = true;
        resetNumber = true;
        switch (cls) {
            case 7:
                goto paragraphSeparator;
            case 8:
                goto tail;
            case 9:
                goto whiteSpace;
            case 10:
            case 18:
                goto otherNeutral;
            case 11:
            case 12:
            case 14:
            case 15:
                goto embeddingOrOverride;
            case 16:
            case 22:
                goto popDirectional;
            case 19:
            case 20:
            case 21:
                goto isolate;
            default:
                goto defaultClass;
        }

    embeddingOrOverride:
        savedWordLength = wordLength;
        insideWord = savedWordLength != 0;
        OCFArrayAppendValue(embeddings, (const void *)(uintptr_t)ch);
        if (useOuterOverride == 1 && OCFArrayGetCount(embeddings) != 1) {
            goto unchangedOverride;
        }
        if (ch > 0x202C) {
            if (ch == 0x202E) {
                goto rightToLeftOverride;
            }
            if (ch != 0x202D) {
                goto unchangedOverride;
            }
            goto leftToRightOverride;
        }
        if (ch == 0x202A) {
            goto leftToRightOverride;
        }
        if (ch != 0x202B) {
            goto unchangedOverride;
        }
        goto rightToLeftOverride;

    isolate:
        {
            savedWordLength = wordLength;
            insideWord = savedWordLength != 0;
            OCFArrayAppendValue(embeddings, (const void *)(uintptr_t)ch);
            if (useOuterOverride == 1 && OCFArrayGetCount(embeddings) != 1) {
                goto unchangedOverride;
            }
        }
    enterIsolate:
        {
            uint8_t previousIsolate = insideIsolate;
            considerWordDirectionality = 0xFF;
            insideIsolate = 1;
            if (savedWordLength != 0) {
                goto overrideInsideWord;
            }
            directionChanged = previousIsolate ^ 1;
            goto accumulateWord;
        }

    otherNeutral:
        if (u_charType((int32_t)ch) != 0x13 || wordLength == 0) {
            goto notIdeographic;
        }
        lookahead = 0;
        paragraphEnd = false;
        goto countAndResetNeutral;

    popDirectional:
        {
            savedWordLength = wordLength;
            insideWord = savedWordLength != 0;
            count = OCFArrayGetCount(embeddings);
            if (count == 0) {
                goto unchangedOverride;
            }
            uint32_t last = (uint32_t)(uintptr_t)OCFArrayGetValueAtIndex(embeddings, count - 1);
            if (cls == 16) {
                uint32_t offset = last - 0x202A;
                if (offset > 4 || offset == 2) {
                    goto unchangedOverride;
                }
            } else if (cls == 22 && last - 0x2066 >= 3) {
                goto unchangedOverride;
            }
            OCFArrayRemoveValueAtIndex(embeddings, count - 1);
            if (count == 1) {
                considerWordDirectionality = 0xFF;
                insideIsolate = 0;
                countRun = true;
                if (savedWordLength != 0) {
                    goto overrideInsideWordCounted;
                }
                directionChanged = 1;
                goto resetNumberAndAccumulate;
            }
            if (useOuterOverride == 1 && OCFArrayGetCount(embeddings) != 0) {
                goto unchangedOverride;
            }
            uint32_t previous = (uint32_t)(uintptr_t)OCFArrayGetValueAtIndex(embeddings, count - 2);
            if (previous > 0x202C) {
                if (previous == 0x202E) {
                    goto rightToLeftOverride;
                }
                if (previous != 0x202D) {
                    resetNumber = true;
                    goto enterIsolate;
                }
                resetNumber = true;
                goto leftToRightOverride;
            }
            if (previous == 0x202A) {
                resetNumber = true;
                goto leftToRightOverride;
            }
            if (previous != 0x202B) {
                resetNumber = true;
                goto enterIsolate;
            }
            goto rightToLeftOverride;
        }

    leftToRightOverride:
        directionChanged = considerWordDirectionality != 0;
        considerWordDirectionality = 0;
        insideIsolate = 0;
        if (savedWordLength == 0) {
            goto accumulateWord;
        }
        goto overrideInsideWord;

    rightToLeftOverride:
        directionChanged = considerWordDirectionality != 1;
        considerWordDirectionality = (uint8_t)resetNumber;
        insideIsolate = 0;
        if (savedWordLength == 0) {
            goto accumulateWord;
        }
    overrideInsideWord:
        lookahead = 0;
        paragraphEnd = false;
        countRun = true;
        resetNeutral = true;
        goto tail;

    whiteSpace:
        lookahead = 0;
        countRun = true;
        if (ch > 0x2006) {
            uint32_t offset = ch - 0x2007;
            if (offset > 0x28) {
                paragraphEnd = lookahead != 0;
                goto tail;
            }
            if (((UINT64_C(1) << offset) & UINT64_C(0x10000000003)) != 0) {
                goto defaultClass;
            }
            if (((UINT64_C(1) << offset) & UINT64_C(0x600000000)) != 0) {
                goto paragraphSeparator;
            }
            paragraphEnd = lookahead != 0;
            goto resetNeutralFlag;
        }
        if (ch - 10 < 4 || ch == 0x85) {
            goto paragraphSeparator;
        }
        if (ch == 0xA0) {
            goto defaultClass;
        }
        paragraphEnd = lookahead != 0;
        goto tail;

    defaultClass:
        if (u_hasBinaryProperty((int32_t)ch, 0x11) == 0) {
            goto notIdeographic;
        }
        if (wordLength == 0) {
            goto ideographicWordStart;
        }
        lookahead = 0;
        paragraphEnd = false;
        index -= charLen;
        countRun = true;
        resetNeutral = true;
        resetNumber = true;
        charLen = 0;
        goto tail;

    notIdeographic:
        directionChanged = 0;
        resetNumber = false;
        goto accumulateWord;

    unchangedOverride:
        countRun = true;
        if (!insideWord) {
            directionChanged = 0;
            goto resetNumberAndAccumulate;
        }
    overrideInsideWordCounted:
        lookahead = 0;
        paragraphEnd = false;
        resetNeutral = true;
        resetNumber = true;
        goto tail;

    resetNumberAndAccumulate:
        resetNumber = true;
    accumulateWord:
        wordLength += charLen;
        if (OCFArrayGetCount(embeddings) != 0 || (directionChanged & 1) != 0) {
            goto directionalOverride;
        }
        if ((uint32_t)cls > 22) {
            goto clearNotCounted;
        }
        lookahead = 0;
        paragraphEnd = false;
        countRun = false;
        resetNeutral = resetNumber;
        switch (cls) {
            case 0:
                goto leftToRight;
            case 1:
            case 13:
                goto rightToLeft;
            case 2:
            case 5:
                goto number;
            case 3:
                goto numberSeparator;
            case 4:
                goto numberTerminator;
            case 6:
                goto commonSeparator;
            case 7:
            case 8:
            case 10:
            case 17:
            case 18:
                goto clearNotCounted;
            default:
                goto tail;
        }

    rightToLeft:
        if (wordDirection == 1) {
            goto clearNotCounted;
        }
        if (wordDirection != 0xFF) {
            goto markMixed;
        }
        resetNeutral = true;
        wordDirection = 1;
        if (numberDirection == 0xFF) {
            lookahead = 0;
            paragraphEnd = false;
            countRun = false;
            goto resetNumberFlag;
        }
        lookahead = 0;
        paragraphEnd = false;
        countRun = false;
        resetNumber = true;
        numberDirection = 1;
        goto tail;

    directionalOverride:
        if (considerWordDirectionality == 0xFF) {
            if (insideIsolate != 1) {
                goto notCounted;
            }
            lookahead = 0;
            paragraphEnd = false;
            countRun = false;
            resetNeutral = resetNumber;
            pendingMixedDirectionality = wordDirection != 0xFF;
            wordDirection = 0xFF;
            numberDirection = 0xFF;
            goto tail;
        }
        if (considerWordDirectionality == 1) {
            pendingMixedDirectionality = wordDirection != 1;
            if (wordDirection != 0xFF) {
                goto notCounted;
            }
            wordDirection = 1;
            if (numberDirection == 0xFF) {
                goto notCounted;
            }
            lookahead = 0;
            paragraphEnd = false;
            countRun = false;
            resetNeutral = resetNumber;
            numberDirection = 1;
            goto tail;
        }
        if (considerWordDirectionality != 0) {
            OCFLog(
                2,
                OF_STR("Unknown value for considerWordDirectionality: %d"),
                (int)(int8_t)considerWordDirectionality
            );
            goto notCounted;
        }
        pendingMixedDirectionality = wordDirection != 0;
        if (wordDirection != 0xFF) {
            goto notCounted;
        }
        wordDirection = 0;
        if (numberDirection == 0xFF) {
            goto notCounted;
        }
        lookahead = 0;
        paragraphEnd = false;
        countRun = false;
        resetNeutral = resetNumber;
        numberDirection = 0;
        goto tail;

    notCounted:
        lookahead = 0;
        paragraphEnd = false;
        countRun = false;
    copyResetNumber:
        resetNeutral = resetNumber;
        goto tail;

    ideographicWordStart:
        wordLength = charLen;
        if (cls == 0) {
            lookahead = wordDirection;
            if (lookahead == 0) {
                goto ideographicCounted;
            }
            if (lookahead != 0xFF) {
                goto ideographicMixed;
            }
            wordDirection = 0;
            if (numberDirection == 0xFF) {
                lookahead = 0;
                goto ideographicCounted;
            }
            lookahead = 0;
            paragraphEnd = false;
            countRun = true;
            numberDirection = 0;
            goto resetNeutralAndNumberFlags;
        }
        if (cls != 13 && cls != 1) {
            lookahead = 0;
            goto ideographicCounted;
        }
        if (wordDirection == 1) {
            lookahead = 0;
            goto ideographicCounted;
        }
        if (wordDirection != 0xFF) {
            goto ideographicMixed;
        }
        countRun = true;
        wordDirection = 1;
        if (numberDirection == 0xFF) {
            lookahead = 0;
            paragraphEnd = false;
            goto resetNeutralAndNumberFlags;
        }
        lookahead = 0;
        paragraphEnd = false;
        resetNeutral = true;
        numberDirection = 1;
        goto resetNumberFlag;

    ideographicCounted:
        paragraphEnd = false;
        countRun = true;
        goto resetNeutralAndNumberFlags;

    ideographicMixed:
        lookahead = 0;
        paragraphEnd = false;
        countRun = true;
        pendingMixedDirectionality = 1;
        goto resetNeutralAndNumberFlags;

    number:
        {
            OCFIndex location = numberRange.location;
            resetNeutral = resetNumber;
            if (location == kOCFNotFound) {
                location = neutralRange.location;
                if (location == kOCFNotFound) {
                    location = index - charLen;
                    numberRange.location = location;
                    resetNeutral = resetNumber;
                } else {
                    numberRange.location = location;
                    numberRange.length = neutralRange.length + charLen;
                    resetNeutral = true;
                }
            }
            lookahead = 0;
            numberRange.length = numberRange.length + charLen;
            if (cls == 5 && numberDirection == 0xFF && (arabicIndicDigitsFeature & 1) != 0) {
                lookahead = 0;
                paragraphEnd = false;
                countRun = false;
                numberDirection = 1;
                goto tail;
            }
            paragraphEnd = lookahead != 0;
            countRun = lookahead != 0;
            goto tail;
        }

    numberSeparator:
        if (numberRange.location != kOCFNotFound) {
            goto extendNumber;
        }
        if (neutralRange.location != kOCFNotFound) {
            goto extendNeutral;
        }
        goto startNeutral;

    commonSeparator:
        if (numberRange.location != kOCFNotFound) {
            goto extendNumber;
        }
        if (neutralRange.location == kOCFNotFound) {
            goto startNeutral;
        }
        lookahead = 0;
        paragraphEnd = false;
        countRun = false;
        goto resetNeutralFlag;

    leftToRight:
        lookahead = wordDirection;
        if (lookahead == 0) {
            goto notCountedReset;
        }
        if (lookahead != 0xFF) {
            goto markMixed;
        }
        wordDirection = 0;
        if (numberDirection == 0xFF) {
            goto clearNotCounted;
        }
        lookahead = 0;
        paragraphEnd = false;
        countRun = false;
        resetNeutral = true;
        numberDirection = 0;
        goto resetNumberFlag;

    clearNotCounted:
        lookahead = 0;
    notCountedReset:
        paragraphEnd = false;
        countRun = false;
        goto resetNeutralAndNumberFlags;

    numberTerminator:
        if (numberRange.location == kOCFNotFound) {
            if (neutralRange.location == kOCFNotFound) {
                goto notCounted;
            }
            goto extendNeutral;
        }
    extendNumber:
        lookahead = 0;
        paragraphEnd = false;
        countRun = false;
        resetNeutral = resetNumber;
        numberRange.length += charLen;
        goto tail;

    markMixed:
        lookahead = 0;
        paragraphEnd = false;
        countRun = false;
        resetNeutral = true;
        pendingMixedDirectionality = 1;
        goto resetNumberFlag;

    extendNeutral:
        lookahead = 0;
        paragraphEnd = false;
        countRun = false;
        resetNeutral = resetNumber;
        neutralRange.length += charLen;
        goto tail;

    startNeutral:
        lookahead = 0;
        paragraphEnd = false;
        countRun = false;
        neutralRange.location = index - charLen;
        neutralRange.length = neutralRange.length + charLen;
        goto copyResetNumber;

    paragraphSeparator:
        lookahead = 0;
        paragraphEnd = true;
        if (ch != 0xD || (uint64_t)index >= (uint64_t)length) {
            goto countAndResetNeutral;
        }
        if (index >= buffer.rangeToBuffer.length) {
            lookahead = 0;
            goto paragraphEndCounted;
        }
        lookahead = stringGetCharacterFromInlineBuffer(&buffer, index);
        if (lookahead != 10) {
            goto paragraphEndCounted;
        }
        ++index;
        paragraphEnd = true;
        countRun = true;
        charLen = 2;
        goto resetNeutralFlag;

    paragraphEndCounted:
        paragraphEnd = true;
    countAndResetNeutral:
        countRun = true;
    resetNeutralFlag:
        resetNeutral = true;
        goto tail;

    resetNeutralAndNumberFlags:
        resetNeutral = true;
    resetNumberFlag:
        resetNumber = true;

    tail:
        if (paragraphLength == 0) {
            firstCharacter = (int32_t)ch;
        }
        paragraphLength += charLen;
        if (index == length) {
            if (neutralRange.location != kOCFNotFound) {
                neutralRange = notFoundRange;
            }
            countRun = true;
            paragraphEnd = true;
            goto resetNumberRange;
        }
        if (neutralRange.location == kOCFNotFound) {
            resetNeutral = false;
        }
        if (resetNeutral) {
            neutralRange = notFoundRange;
            if (resetNumber) {
                goto resetNumberRange;
            }
            goto countCheck;
        }
        if (resetNumber) {
            goto resetNumberRange;
        }
        goto countCheck;

    resetNumberRange:
        if (numberRange.location != kOCFNotFound) {
            numberRange = notFoundRange;
            if (countRun) {
                goto countWord;
            }
            goto nextIndex;
        }
    countCheck:
        if (!countRun) {
            goto nextIndex;
        }
    countWord:
        if (wordDirection == 1) {
            ++counts[4];
            goto rightToLeftRun;
        }
        if (wordDirection == 0) {
            ++counts[3];
            goto leftToRightRun;
        }
        if (numberDirection == 1) {
            ++counts[1];
            goto rightToLeftRun;
        }
        if (numberDirection == 0) {
            ++counts[0];
            goto leftToRightRun;
        }
        if (wordLength < 1) {
            goto paragraphCheck;
        }
        ++counts[5];
        goto mixedCheck;

    leftToRightRun:
        if (paragraphDirection == 0xFE) {
            paragraphDirection = 0;
        }
        goto wordCheck;

    rightToLeftRun:
        if (paragraphDirection == 0xFE) {
            paragraphDirection = 1;
        }
    wordCheck:
        if (wordLength < 1) {
            goto paragraphCheck;
        }
    mixedCheck:
        if ((pendingMixedDirectionality & 1) != 0) {
            hasMixedDirectionality = 1;
        }
    paragraphCheck:
        if (!paragraphEnd) {
            goto resetWord;
        }
        {
            uint64_t share = 0;
            bool computeShare;
            if (counts[0] != 0 || counts[1] != 0 || counts[3] != 0) {
                share = 1;
                computeShare = (counts[4] | counts[1]) != 0;
            } else {
                share = counts[4] != 0;
                computeShare = counts[4] != 0;
            }
            if (computeShare) {
                double rightToLeft = (double)counts[1] + (double)counts[4];
                double total = ((double)counts[0] + (double)counts[3]) + rightToLeft;
                double ratio = rightToLeft / total;
                share = (ratio >= 0.4 ? 0x100 : 0) | (ratio >= 0.15 ? 0x10000 : 0) | share;
            }
            int8_t writingDirection;
            if (firstCharacter == 0x200E) {
                writingDirection = 0;
            } else if (firstCharacter == 0x200F) {
                writingDirection = 1;
            } else if (paragraphDirection == 0) {
                if (leftToRightFeature == 1) {
                    writingDirection = (share & 0x100) != 0 ? 1 : (int8_t)((share & 1) - 1);
                } else {
                    writingDirection = 0;
                }
            } else if (paragraphDirection == 1) {
                if (rightToLeftFeature != 1 || (share & 0x100) != 0) {
                    writingDirection = 1;
                } else {
                    writingDirection = (int8_t)((share & 1) - 1);
                }
            } else {
                writingDirection = (share & 0x100) != 0 ? 1 : (int8_t)((share & 1) - 1);
            }
            dispatch_once(&appendParagraphRangeInfoOnceToken, ^{
              const void **keys = malloc_type_calloc(10, 8, 0x80040B8603338);
              sArrayOfStatisticalWritingDirectionParagraphKeys = keys;
              keys[0] = OF_STR("Location");
              keys[1] = OF_STR("Length");
              keys[2] = OF_STR("WritingDirection");
              keys[3] = OF_STR("HasMixedDirectionality");
              keys[4] = OF_STR("NumberOfLeftToRightNumbers");
              keys[5] = OF_STR("NumberOfRightToLeftNumbers");
              keys[6] = OF_STR("NumberOfWeakNumbers");
              keys[7] = OF_STR("NumberOfLeftToRightWords");
              keys[8] = OF_STR("NumberOfRightToLeftWords");
              keys[9] = OF_STR("NumberOfWeakWords");
            });
            // kCFNumberCFIndexType (14), kCFNumberSInt8Type (1), kCFNumberIntType (9).
            OCFNumberRef location = OCFNumberCreate(allocator, 14, &paragraphLocation);
            OCFNumberRef paragraphLengthNumber = OCFNumberCreate(allocator, 14, &paragraphLength);
            OCFNumberRef directionNumber = OCFNumberCreate(allocator, 1, &writingDirection);
            int32_t mixed = hasMixedDirectionality;
            OCFNumberRef mixedNumber = OCFNumberCreate(allocator, 9, &mixed);
            OCFNumberRef leftToRightNumbers = OCFNumberCreate(allocator, 14, &counts[0]);
            OCFNumberRef rightToLeftNumbers = OCFNumberCreate(allocator, 14, &counts[1]);
            OCFNumberRef weakNumbers = OCFNumberCreate(allocator, 14, &counts[2]);
            OCFNumberRef leftToRightWords = OCFNumberCreate(allocator, 14, &counts[3]);
            OCFNumberRef rightToLeftWords = OCFNumberCreate(allocator, 14, &counts[4]);
            OCFNumberRef weakWords = OCFNumberCreate(allocator, 14, &counts[5]);
            const void *values[10] = {
                location,
                paragraphLengthNumber,
                directionNumber,
                mixedNumber,
                leftToRightNumbers,
                rightToLeftNumbers,
                weakNumbers,
                leftToRightWords,
                rightToLeftWords,
                weakWords,
            };
            OCFDictionaryRef paragraph = OCFDictionaryCreate(
                allocator,
                sArrayOfStatisticalWritingDirectionParagraphKeys,
                values,
                10,
                &kOCFCopyStringDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
            OCFRelease(location);
            OCFRelease(paragraphLengthNumber);
            OCFRelease(directionNumber);
            OCFRelease(mixedNumber);
            OCFRelease(leftToRightNumbers);
            OCFRelease(rightToLeftNumbers);
            OCFRelease(weakNumbers);
            OCFRelease(leftToRightWords);
            OCFRelease(rightToLeftWords);
            OCFRelease(weakWords);
            OCFArrayAppendValue(result, paragraph);
            OCFRelease(paragraph);
            memset(counts, 0, sizeof(counts));
            paragraphLocation = index;
            paragraphLength = 0;
            firstCharacter = 0;
            paragraphDirection = 0xFE;
            hasMixedDirectionality = 0;
        }
    resetWord:
        wordLength = 0;
        wordDirection = 0xFF;
        numberDirection = 0xFF;
        pendingMixedDirectionality = 0;
    nextIndex:
        index -= lookahead == 10;
    nextCharacter:
        ch = lookahead;
        if (index < length) {
            goto loopTop;
        }
    }
    OCFArrayRef copy = OCFRetain(result);
    OCFRelease(result);
    result = NULL;
    OCFRelease(embeddings);
    return copy;
}

// Native __CFStringGetLongCharacterFromInlineBuffer, 0x1c0222e44. Non-external.
// The character at idx, combined with the following low surrogate (high surrogate below length - 1) or the
// preceding high surrogate (low surrogate after index 0). *readRange receives the code units read; an index
// outside [0, length) answers 0 with {-1, 0}.
UTF32Char __CFStringGetLongCharacterFromInlineBuffer(
    OCFStringInlineBuffer *buffer, OCFIndex length, OCFIndex idx, OCFRange *readRange
) {
    UTF32Char character = 0;
    OCFIndex readLength = 0;
    OCFIndex readLocation = -1;
    if (idx >= 0 && idx < length) {
        UniChar unit = stringGetCharacterFromInlineBuffer(buffer, idx);
        character = unit;
        readLength = 1;
        readLocation = idx;
        if ((unit & 0xFC00) == 0xD800 && idx < length - 1) {
            UniChar low = stringGetCharacterFromInlineBuffer(buffer, idx + 1);
            if ((low >> 10) == 0x37) {
                character = (character << 10) + low - 0x35FDC00;
                readLength = 2;
            }
        } else if (idx != 0 && (unit & 0xFC00) == 0xDC00 && idx <= buffer->rangeToBuffer.length) {
            UniChar high = stringGetCharacterFromInlineBuffer(buffer, idx - 1);
            if ((high >> 10) == 0x36) {
                character = character + ((UTF32Char)high << 10) - 0x35FDC00;
                readLength = 2;
                readLocation = idx - 1;
            }
        }
    }
    readRange->location = readLocation;
    readRange->length = readLength;
    return character;
}

// Native __CFStrDeallocateMutableContents, 0x1c02230e8. Non-external. The contents allocator is the stored
// one (0x30) for contents kind 0x60, else the object's allocator. For a mutable string the info word is
// loaded once more and discarded (LDAR XZR), as natively.
void __CFStrDeallocateMutableContents(OCFMutableStringRef str, void *buffer) {
    OCFAllocatorRef allocator = stringMutableContentsAllocator(str);
    if ((stringInfo(str) & kStringInfoMutable) != 0) {
        (void)stringInfo(str);
    }
    OCFAllocatorDeallocate(allocator, buffer);
}

// Native CFStrIsUnicode, 0x1c022314c. Non-external. Tagged pointers and Objective-C strings answer
// -_encodingCantBeStoredInEightBitCFString; a CF string answers its Unicode bit (info bit 4).
Boolean CFStrIsUnicode(OCFStringRef str) {
    if (stringIsTagged(str) || __CF_IS_OBJC(7, str) != 0) {
        return ((Boolean (*)(id, SEL))objc_msgSend)(
            (id)str, sel_getUid("_encodingCantBeStoredInEightBitCFString")
        );
    }
    return (stringInfo(str) >> 4) & 1;
}

// Native _objc_getTaggedPointerTag, 0x1c0223198. Non-external; this unit's copy of libobjc's inline tag
// decoder. The obfuscator is skipped for pointers with bits 63, 62 and 0-2 set; the low three bits of the
// decoded value are searched in the first seven entries of objc_debug_tag60_permutations; not found (index 7)
// answers the extended tag 8 + bits 55-62 of the decoded value.
uint32_t _objc_getTaggedPointerTag(const void *ptr) {
    uintptr_t pointer = (uintptr_t)ptr;
    uintptr_t decoded =
        ((~pointer & 0xC000000000000007) == 0 ? 0 : objc_debug_taggedpointer_obfuscator) ^ pointer;
    uintptr_t index = 0;
    do {
        if ((decoded & 7) == objc_debug_tag60_permutations[index]) {
            break;
        }
        ++index;
    } while (index != 7);
    if ((index & 7) == 7) {
        return (uint32_t)(((index | decoded) >> 55) & 0xFF) + 8;
    }
    return (uint32_t)(index & 7);
}

// Native __CFStringGetExtendedPictographicSequenceComponent, 0x1c02231f8. Non-external.
// An index not below length (unsigned) answers false. The backward scan from index collects extending
// characters (Grapheme_Extend or Emoji_Modifier; at most 64), one zero-width joiner and one
// Extended_Pictographic character, recording the location of each; a pictograph ends the component (with the
// extend span when no joiner was seen). Without a pictograph, a component that saw an extending character or
// a joiner is scanned forward for its pictograph, extending characters (refused after a joiner; at most 63)
// and one joiner. The record is written only on success.
static bool __CFStringGetExtendedPictographicSequenceComponent(
    OCFStringInlineBuffer *buffer, OCFIndex length, OCFIndex index, PictographicSequenceComponent *component
) {
    if ((uint64_t)index >= (uint64_t)length) {
        return false;
    }
    OCFRange readRange;
    OCFIndex count = 0;
    OCFIndex total = 0;
    OCFIndex location = -1;
    OCFIndex pictograph = -1;
    OCFIndex joiner = -1;
    OCFIndex extend = -1;
    OCFIndex position = index;
    for (;;) {
        UTF32Char character =
            __CFStringGetLongCharacterFromInlineBuffer(buffer, length, position, &readRange);
        if (u_hasBinaryProperty((UChar32)character, UCHAR_GRAPHEME_EXTEND) ||
            u_hasBinaryProperty((UChar32)character, UCHAR_EMOJI_MODIFIER)) {
            if (count > 63) {
                ++count;
                break;
            }
            location = readRange.location;
            extend = location;
            ++count;
        } else if (character == 0x200D) {
            if (extend != -1 || joiner != -1) {
                break;
            }
            extend = -1;
            location = readRange.location;
            joiner = location;
        } else if (u_hasBinaryProperty((UChar32)character, UCHAR_EXTENDED_PICTOGRAPHIC)) {
            if (pictograph != -1 || joiner != -1 || extend != -1) {
                break;
            }
            location = readRange.location;
            joiner = -1;
            pictograph = location;
        } else {
            break;
        }
        total += readRange.length;
        position = location - 1;
        readRange.location = position;
        if (position < 0) {
            break;
        }
    }
    if (pictograph == -1) {
        if ((joiner & extend) == -1) {
            return false;
        }
        position = location + total;
        if (position >= length) {
            return false;
        }
        for (;;) {
            UTF32Char character =
                __CFStringGetLongCharacterFromInlineBuffer(buffer, length, position, &readRange);
            if (u_hasBinaryProperty((UChar32)character, UCHAR_GRAPHEME_EXTEND) ||
                u_hasBinaryProperty((UChar32)character, UCHAR_EMOJI_MODIFIER)) {
                if (joiner != -1) {
                    return false;
                }
                if (count > 62) {
                    return false;
                }
                ++count;
                joiner = -1;
                pictograph = -1;
            } else if (character == 0x200D) {
                if (joiner != -1) {
                    return false;
                }
                pictograph = -1;
                joiner = readRange.location;
            } else if (u_hasBinaryProperty((UChar32)character, UCHAR_EXTENDED_PICTOGRAPHIC)) {
                pictograph = readRange.location;
            } else {
                return false;
            }
            total += readRange.length;
            position = readRange.location + readRange.length;
            readRange.location = position;
            readRange.length = 0;
            if (pictograph != -1 || position >= length) {
                break;
            }
        }
        if (pictograph == -1) {
            return false;
        }
        if (count > 63) {
            return false;
        }
    } else if (extend != -1 && joiner == -1) {
        total += extend - pictograph;
        location = pictograph;
    }
    component->location = location;
    component->length = total;
    component->extendIndex = extend;
    component->joinerIndex = joiner;
    component->pictographIndex = pictograph;
    return true;
}

// The size step of the inlined __CFStrNewCapacity: the contents allocator's preferred size for contents kind
// 0x60, else malloc_good_size; a negative answer becomes -1 (CMN #1; CSINV ..., GT).
static OCFIndex mutableContentsGoodSize(OCFMutableStringRef str, uint64_t size) {
    OCFIndex result;
    if ((~stringInfo(str) & kStringInfoContentsMask) == 0) {
        result =
            OCFAllocatorGetPreferredSizeForSize(str->variants.notInlineMutable.contentsAllocator, size, 0);
    } else {
        result = (OCFIndex)malloc_good_size(size);
    }
    return result > -1 ? result : -1;
}

// One deferred block move of the inlined rearrangeBlocks: source offset, byte length and shift (24 bytes).
typedef struct {
    OCFIndex sourceOffset;
    OCFIndex length;
    OCFIndex shift;
} DeferredBlockMove;

// Native __CFStringChangeSizeMultiple, 0x1c0223490. Non-external. __CFStrNewCapacity and rearrangeBlocks are
// inlined; copyBlocks is called. Facts read from the assembly:
// - The new length is curLength + insertLength - deleteRanges[0].length for one range, else
//   curLength + insertLength * count minus every range length (a do-while over count; count 0 is never
//   passed).
// - A zero new length keeps the buffer only when the recomputed capacity (desired capacity, preferred size)
//   equals the current one; the recomputation happens only for a nonzero capacity whose half is nonzero and
//   that was not provided externally. A kept buffer of capacity 2 or more gets the length and null bytes
//   (two zero bytes); a NULL buffer with such a capacity is .cold.1 ("String had a capacity but NULL buffer
//   pointer").
// - Otherwise the first capacity computation shrinks only below half the capacity; the retry after a failed
//   allocation recomputes whenever the request differs from the capacity (unless provided externally) and
//   without extra room (max(request, desired capacity * character size)).
// - A new buffer is allocated when there are no contents, when the capacity changes, or when nonempty
// eight-bit
//   contents become Unicode. __CFStringHandleOutOfMemory is called on overflow, on a capacity of -1 and on a
//   failed retry; execution continues after it as natively.
// - In-place rearrangement defers blocks that move forward (positive shift) in a ten-entry stack array that
//   grows to count * 2 + 8 entries through the system default allocator (type ID 0x1000040504FFAC1) and moves
//   them in reverse when a block that does not move forward follows, and at the end.
// The fifth argument of __CFSafelyReallocateWithAllocatorTyped is passed in X3 (0x1000040504FFAC1) and the
// sixth in X4 (0), by position, whatever the restored prototype names them.
void __CFStringChangeSizeMultiple(
    OCFMutableStringRef str,
    const OCFRange *deleteRanges,
    OCFIndex numDeleteRanges,
    OCFIndex insertLength,
    Boolean makeUnicode
) {
    uint8_t *curContents;
    if ((stringInfo(str) & kStringInfoContentsMask) != 0) {
        curContents = str->variants.notInlineMutable.buffer;
    } else {
        uintptr_t offset =
            (stringInfo(str) & (kStringInfoMutable | kStringInfoLengthByte)) == kStringInfoLengthByte
                ? 0
                : sizeof(OCFIndex);
        curContents = (uint8_t *)&str->variants + offset;
    }
    OCFIndex curLength;
    if (curContents == NULL) {
        curLength = 0;
    } else if ((stringInfo(str) & (kStringInfoMutable | kStringInfoLengthByte)) == kStringInfoLengthByte) {
        curLength = curContents[0];
    } else if ((stringInfo(str) & kStringInfoContentsMask) != 0) {
        curLength = str->variants.notInlineMutable.length;
    } else {
        curLength = str->variants.inline1.length;
    }
    OCFIndex newLength;
    if (numDeleteRanges == 1) {
        newLength = curLength + insertLength - deleteRanges[0].length;
    } else {
        newLength = insertLength * numDeleteRanges + curLength;
        const OCFRange *range = deleteRanges;
        OCFIndex remaining = numDeleteRanges;
        do {
            newLength -= range->length;
            ++range;
        } while (--remaining != 0);
    }
    if (newLength == 0) {
        OCFIndex curCapacity = str->variants.notInlineMutable.capacity;
        bool keepBuffer = true;
        if (curCapacity != 0) {
            uint64_t bits = str->variants.notInlineMutable.bits;
            if (curCapacity / 2 != 0 && (bits & kStringMutableCapacityProvidedExternally) == 0) {
                OCFIndex newCapacity =
                    mutableContentsGoodSize(str, bits >> kStringMutableDesiredCapacityShift);
                keepBuffer = newCapacity == curCapacity;
            }
        }
        if (!keepBuffer) {
            if (curContents != NULL) {
                __CFStrDeallocateMutableContents(str, curContents);
            }
            str->variants.notInlineMutable.buffer = NULL;
            uint64_t bits = str->variants.notInlineMutable.bits;
            str->variants.notInlineMutable.capacity = 0;
            str->variants.notInlineMutable.bits = bits & ~(uint64_t)kStringMutableCapacityProvidedExternally;
            stringSetInfoBits(str, kStringInfoLengthByte, 0);
            stringSetInfoBits(str, kStringInfoNullByte, 0);
            if ((str->variants.notInlineMutable.bits & kStringMutableExternal) == 0) {
                stringSetInfoBits(str, kStringInfoUnicode, 0);
            }
        } else if ((str->variants.notInlineMutable.bits & kStringMutableExternal) == 0) {
            stringSetInfoBits(str, kStringInfoUnicode, 0);
            if (curCapacity >= 2) {
                if (curContents == NULL) {
                    crashWithMessage("String had a capacity but NULL buffer pointer");
                }
                stringSetInfoBits(str, kStringInfoLengthByte, kStringInfoLengthByte);
                stringSetInfoBits(str, kStringInfoNullByte, kStringInfoNullByte);
                *(uint16_t *)curContents = 0;
            } else {
                stringSetInfoBits(str, kStringInfoLengthByte, 0);
                stringSetInfoBits(str, kStringInfoNullByte, 0);
            }
        }
        if ((stringInfo(str) & kStringInfoContentsMask) != 0) {
            str->variants.notInlineMutable.length = 0;
        } else {
            str->variants.inline1.length = 0;
        }
        return;
    }

    uint64_t oldInfo = stringInfo(str);
    uint64_t oldIsUnicode = oldInfo & kStringInfoUnicode;
    OCFIndex newContentsOffset = 0;
    OCFIndex charSize = 2;
    uint64_t newIsUnicode = 1;
    uint64_t extraBytes = 0;
    if (!makeUnicode && oldIsUnicode == 0) {
        uint64_t external = str->variants.notInlineMutable.bits & kStringMutableExternal;
        newIsUnicode = external >> 2;
        newContentsOffset = (OCFIndex)(newIsUnicode ^ 1);
        charSize = external != 0 ? 2 : 1;
        extraBytes = external != 0 ? 0 : 2;
    }
    OCFIndex curCapacity = str->variants.notInlineMutable.capacity;
    if ((uint64_t)newLength > ((extraBytes ^ INT64_MAX) >> newIsUnicode)) {
        __CFStringHandleOutOfMemory();
    }
    uint64_t reqCapacity = ((uint64_t)newLength << newIsUnicode) + extraBytes;
    uint64_t anyCapacity = (uint64_t)curCapacity | reqCapacity;
    OCFIndex newCapacity = curCapacity;
    if (anyCapacity != 0 &&
        ((uint64_t)curCapacity < reqCapacity ||
         ((str->variants.notInlineMutable.bits & kStringMutableCapacityProvidedExternally) == 0 &&
          reqCapacity < (uint64_t)(curCapacity / 2)))) {
        if ((int64_t)reqCapacity < 0) {
            __CFStringHandleOutOfMemory();
            newCapacity = -1;
        } else {
            uint64_t extraRoom = (reqCapacity * 3 + 1) >> 1;
            uint64_t bits = str->variants.notInlineMutable.bits;
            uint64_t desired = (bits >> kStringMutableDesiredCapacityShift) * (uint64_t)charSize;
            uint64_t exact = desired > reqCapacity ? desired : reqCapacity;
            uint64_t size = (bits & kStringMutableFixedCapacity) == 0 ? extraRoom : exact;
            if (extraRoom < desired) {
                size = desired;
            }
            newCapacity = mutableContentsGoodSize(str, size);
            if (newCapacity == -1) {
                __CFStringHandleOutOfMemory();
            }
        }
    } else if (newCapacity == -1) {
        __CFStringHandleOutOfMemory();
    }

    uint8_t *newBase;
    bool allocated;
    uint64_t hasLengthByte;
    uint8_t *newContents;
    if (curContents != NULL && newCapacity == curCapacity &&
        !(newIsUnicode != 0 && curLength >= 1 && oldIsUnicode == 0)) {
        allocated = false;
        hasLengthByte = (stringInfo(str) >> 2) & 1;
        newContents = curContents + newContentsOffset;
        newBase = curContents;
    } else {
        newBase = __CFStrAllocateMutableContents(str, (size_t)newCapacity);
        if (newBase == NULL) {
            OCFIndex retryCapacity = curCapacity;
            bool overflow = false;
            if (anyCapacity != 0 &&
                ((uint64_t)curCapacity < reqCapacity ||
                 ((str->variants.notInlineMutable.bits & kStringMutableCapacityProvidedExternally) == 0 &&
                  (reqCapacity < (uint64_t)curCapacity || reqCapacity < (uint64_t)(curCapacity / 2))))) {
                if ((int64_t)reqCapacity < 0) {
                    overflow = true;
                } else {
                    uint64_t desired =
                        (str->variants.notInlineMutable.bits >> kStringMutableDesiredCapacityShift) *
                        (uint64_t)charSize;
                    uint64_t size = reqCapacity > desired ? reqCapacity : desired;
                    retryCapacity = mutableContentsGoodSize(str, size);
                }
            }
            newCapacity = retryCapacity;
            if (overflow || retryCapacity == -1) {
                __CFStringHandleOutOfMemory();
                newCapacity = -1;
            }
            newBase = __CFStrAllocateMutableContents(str, (size_t)newCapacity);
            if (newBase == NULL) {
                __CFStringHandleOutOfMemory();
                newBase = NULL;
            }
        }
        hasLengthByte = (stringInfo(str) >> 2) & 1;
        newContents = newBase + newContentsOffset;
        allocated = true;
    }
    OCFIndex finalCapacity = newCapacity;

    if (curContents != NULL) {
        Boolean srcIsUnicode = (oldInfo >> 4) & 1;
        const uint8_t *source = curContents + hasLengthByte;
        if ((oldIsUnicode >> 4) == (uint8_t)newIsUnicode) {
            if (newContents == source) {
                DeferredBlockMove localMoves[10];
                DeferredBlockMove *moves = localMoves;
                OCFIndex movesCapacity = 10;
                OCFIndex count = 0;
                OCFIndex shift = 0;
                bool dynamic = false;
                OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
                OCFIndex index = 0;
                do {
                    OCFIndex start = deleteRanges[index].location + deleteRanges[index].length;
                    OCFIndex blockLength;
                    if (index == numDeleteRanges - 1) {
                        blockLength = (curLength - start) * charSize;
                        if (blockLength == 0) {
                            break;
                        }
                    } else {
                        blockLength = (deleteRanges[index + 1].location - start) * charSize;
                    }
                    OCFIndex sourceOffset = start * charSize;
                    shift = (insertLength - deleteRanges[index].length) * charSize + shift;
                    if (shift > 0) {
                        if (count == movesCapacity) {
                            DeferredBlockMove *previous = moves;
                            movesCapacity = count * 2 + 8;
                            OCFIndex bytes = movesCapacity * 3 * 8;
                            if (dynamic) {
                                moves = __OCFSafelyReallocateWithAllocatorTyped(
                                    allocator, previous, bytes, 0x1000040504FFAC1, 0, NULL
                                );
                            } else {
                                DeferredBlockMove *grown =
                                    OCFAllocatorAllocateTyped(allocator, bytes, 0x1000040504FFAC1, 0);
                                memmove(grown, previous, (size_t)(count * 3 * 8));
                                dynamic = true;
                                moves = grown;
                            }
                        }
                        moves[count].sourceOffset = sourceOffset;
                        moves[count].length = blockLength;
                        moves[count].shift = shift;
                        ++count;
                    } else {
                        if (shift != 0 && blockLength != 0) {
                            memmove(
                                newContents + sourceOffset + shift,
                                newContents + sourceOffset,
                                (size_t)blockLength
                            );
                        }
                        if (count >= 1) {
                            for (OCFIndex pending = count; pending > 0; --pending) {
                                DeferredBlockMove *move = &moves[pending - 1];
                                if (move->shift != 0 && move->length != 0) {
                                    memmove(
                                        newContents + move->sourceOffset + move->shift,
                                        newContents + move->sourceOffset,
                                        (size_t)move->length
                                    );
                                }
                            }
                            count = 0;
                        }
                    }
                } while (++index != numDeleteRanges);
                for (OCFIndex pending = count; pending > 0; --pending) {
                    DeferredBlockMove *move = &moves[pending - 1];
                    if (move->shift != 0 && move->length != 0) {
                        memmove(
                            newContents + move->sourceOffset + move->shift,
                            newContents + move->sourceOffset,
                            (size_t)move->length
                        );
                    }
                }
                if (dynamic) {
                    OCFAllocatorDeallocate(allocator, moves);
                }
            } else {
                copyBlocks(
                    source,
                    newContents,
                    curLength,
                    srcIsUnicode,
                    srcIsUnicode,
                    deleteRanges,
                    numDeleteRanges,
                    insertLength
                );
            }
        } else if (newIsUnicode != 0) {
            copyBlocks(
                source, newContents, curLength, srcIsUnicode, 1, deleteRanges, numDeleteRanges, insertLength
            );
        }
        if (allocated && (stringInfo(str) & kStringInfoContentsFreeWithObjectAllocator) != 0) {
            __CFStrDeallocateMutableContents(str, curContents);
        }
    }

    if (newIsUnicode != 0) {
        if (oldIsUnicode == 0) {
            stringSetInfoBits(str, kStringInfoUnicode, kStringInfoUnicode);
        }
        if (hasLengthByte != 0) {
            stringSetInfoBits(str, kStringInfoLengthByte, 0);
            stringSetInfoBits(str, kStringInfoNullByte, 0);
        }
    } else {
        newContents[newLength] = 0;
        newBase[0] = newLength > 0xff ? 0 : (uint8_t)newLength;
        if (hasLengthByte == 0) {
            stringSetInfoBits(str, kStringInfoLengthByte, kStringInfoLengthByte);
            stringSetInfoBits(str, kStringInfoNullByte, kStringInfoNullByte);
        }
        if (oldIsUnicode != 0) {
            stringSetInfoBits(str, kStringInfoUnicode, 0);
        }
    }
    if ((stringInfo(str) & kStringInfoContentsMask) != 0) {
        str->variants.notInlineMutable.length = newLength;
    } else {
        str->variants.inline1.length = newLength;
    }
    if (allocated) {
        uint64_t bits = str->variants.notInlineMutable.bits;
        str->variants.notInlineMutable.capacity = finalCapacity;
        str->variants.notInlineMutable.bits = bits & ~(uint64_t)kStringMutableCapacityProvidedExternally;
        str->variants.notInlineMutable.buffer = newBase;
    }
}

// Native __CFStrAllocateMutableContents, 0x1c0223e08. Non-external. Type ID 0xC29B8678, hint 0; the event
// name "CFString (store)" is recorded when __CFOASafe is 1.
void *__CFStrAllocateMutableContents(OCFMutableStringRef str, size_t size) {
    OCFAllocatorRef allocator = stringMutableContentsAllocator(str);
    void *contents = OCFAllocatorAllocateTyped(allocator, (OCFIndex)size, 0xC29B8678, 0);
    if (__OCFOASafe) {
        __OCFSetLastAllocationEventName(contents, "CFString (store)");
    }
    return contents;
}

// Native copyBlocks, 0x1c0223ea8. Non-external. Copies the source around numRanges ranges (at least one; the
// loop is a do-while) into the destination, leaving insertLength units (shifted by dstIsUnicode) before each
// range's continuation. Positions are in bytes: source locations are shifted by srcIsUnicode. For each range,
// the bytes from the previous source position up to its location are copied when there is at least one, with
// memmove when the two widths are equal (a 32-bit comparison of the flags) and __CFStrConvertBytesToUnicode
// otherwise; the source position moves to (location + length) << srcIsUnicode and the destination position
// grows by (insertLength << dstIsUnicode) + (count << (srcIsUnicode != dstIsUnicode)), the count added even
// when it is not positive. The tail, (srcLength << srcIsUnicode) minus the source position, is copied the
// same way when positive (a tail call).
void copyBlocks(
    const uint8_t *srcStart,
    uint8_t *dstStart,
    OCFIndex srcLength,
    Boolean srcIsUnicode,
    Boolean dstIsUnicode,
    const OCFRange *ranges,
    OCFIndex numRanges,
    OCFIndex insertLength
) {
    OCFIndex srcLocationInBytes = 0;
    OCFIndex dstLocationInBytes = 0;
    OCFIndex insertBytes = insertLength << dstIsUnicode;
    unsigned convertShift = (uint32_t)srcIsUnicode != (uint32_t)dstIsUnicode;
    unsigned srcShift = srcIsUnicode;
    const OCFRange *range = ranges;
    do {
        OCFIndex location = range->location;
        OCFIndex srcBytesToCopy = (location << srcShift) - srcLocationInBytes;
        if (srcBytesToCopy >= 1) {
            if ((uint32_t)srcIsUnicode == (uint32_t)dstIsUnicode) {
                memmove(dstStart + dstLocationInBytes, srcStart + srcLocationInBytes, (size_t)srcBytesToCopy);
            } else {
                __OCFStrConvertBytesToUnicode(
                    srcStart + srcLocationInBytes, (UniChar *)(dstStart + dstLocationInBytes), srcBytesToCopy
                );
            }
        }
        srcLocationInBytes = (range->length + location) << srcShift;
        ++range;
        dstLocationInBytes = dstLocationInBytes + insertBytes + (srcBytesToCopy << convertShift);
    } while (--numRanges != 0);
    OCFIndex srcBytesToCopy = (srcLength << srcShift) - srcLocationInBytes;
    if (srcBytesToCopy <= 0) {
        return;
    }
    if ((uint32_t)srcIsUnicode == (uint32_t)dstIsUnicode) {
        memmove(dstStart + dstLocationInBytes, srcStart + srcLocationInBytes, (size_t)srcBytesToCopy);
        return;
    }
    __OCFStrConvertBytesToUnicode(
        srcStart + srcLocationInBytes, (UniChar *)(dstStart + dstLocationInBytes), srcBytesToCopy
    );
}

// Native __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT_WITH_CONTEXT__, 0x1c02253b0. Non-external.
// Calls function(object, context, "%@NSCONTEXT", replacing, isDeferred): X3 and X4 are not touched before the
// BLR, so the caller's replacing flag and deferral pointer reach the context function.
OCFStringRef __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT_WITH_CONTEXT__(
    ContextDescriptionFunction function,
    const void *object,
    const void *context,
    bool replacing,
    bool *isDeferred
) {
    return function((void *)object, context, OF_STR("%@NSCONTEXT"), replacing, isDeferred);
}

// Native __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT_WITH_LOCALE__, 0x1c02253dc. Non-external.
OCFStringRef __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT_WITH_LOCALE__(
    CopyDescriptionFunction function, const void *object, const void *formatOptions
) {
    return function((void *)object, formatOptions);
}

// Native __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT__, 0x1c02253fc. Non-external.
OCFStringRef __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT__(const void *object) {
    return OCFCopyDescription(object);
}

// Native _CFStringHasStrongOrWeakRTL, 0x1c0225954. Non-external. With the Foundation feature
// "LanguageAwareStringStatisticalLocalizedStringWithFormat" enabled: false for an empty range or a string
// whose fastest encoding is ASCII; otherwise each long character of the range (read through
// __CFStringGetLongCharacterFromInlineBuffer) is classified by u_charDirection: R (1), AL (13) answer true;
// AN (5) and the EN (2) digits U+06F0-U+06F9 answer true when
// "LanguageAwareStringStatisticalArabicIndicDigits" is enabled. Without the feature, the characters are
// tested against the CFUniChar bitmap 0x66 (plane 0, or the plane of a surrogate pair; a lone high surrogate
// is not tested). The inline buffer's C-string pointer comes from _CFStringGetCStringPtrInternal(string,
// ASCII, true, true).
Boolean _CFStringHasStrongOrWeakRTL(OCFStringRef string, OCFIndex location, OCFIndex length) {
    OCFStringInlineBuffer buffer;
    if (_os_feature_enabled_impl("Foundation", "LanguageAwareStringStatisticalLocalizedStringWithFormat")) {
        if (length == 0) {
            return false;
        }
        if (OCFStringGetFastestEncoding(string) == 0x600) {
            return false;
        }
        memset(buffer.buffer, 0, sizeof(buffer.buffer));
        buffer.theString = string;
        buffer.rangeToBuffer.location = location;
        buffer.rangeToBuffer.length = length;
        buffer.directUniCharBuffer = OCFStringGetCharactersPtr(string);
        buffer.directCStringBuffer = buffer.directUniCharBuffer != NULL
                                         ? NULL
                                         : _CFStringGetCStringPtrInternal(string, 0x600, true, true);
        buffer.bufferedRangeStart = 0;
        buffer.bufferedRangeEnd = 0;
        bool arabicIndicDigits =
            _os_feature_enabled_impl("Foundation", "LanguageAwareStringStatisticalArabicIndicDigits");
        if (length < 1) {
            return false;
        }
        OCFIndex index = 0;
        do {
            OCFRange range;
            UTF32Char character = __CFStringGetLongCharacterFromInlineBuffer(&buffer, length, index, &range);
            int32_t direction = u_charDirection((int32_t)character);
            if (direction > 4) {
                if (direction == 5) {
                    if (arabicIndicDigits) {
                        return true;
                    }
                } else if (direction == 13) {
                    return true;
                }
            } else if (direction == 2) {
                if (arabicIndicDigits && character - 0x6f0 <= 9) {
                    return true;
                }
            } else if (direction == 1) {
                return true;
            }
            index = range.location + range.length;
        } while (index < length);
        return false;
    }
    memset(buffer.buffer, 0, sizeof(buffer.buffer));
    const uint8_t *bitmap = OCFUniCharGetBitmapPtrForPlane(0x66, 0);
    buffer.theString = string;
    buffer.rangeToBuffer.location = location;
    buffer.rangeToBuffer.length = length;
    buffer.directUniCharBuffer = OCFStringGetCharactersPtr(string);
    buffer.directCStringBuffer =
        buffer.directUniCharBuffer != NULL ? NULL : _CFStringGetCStringPtrInternal(string, 0x600, true, true);
    buffer.bufferedRangeStart = 0;
    buffer.bufferedRangeEnd = 0;
    if (length < 1) {
        return false;
    }
    OCFIndex index = 0;
    do {
        UTF32Char character = stringGetCharacterFromInlineBuffer(&buffer, index);
        const uint8_t *planeBitmap = bitmap;
        if ((character >> 10) == 0x36) {
            if (index + 1 >= buffer.rangeToBuffer.length) {
                ++index;
                continue;
            }
            UniChar low = stringGetCharacterFromInlineBuffer(&buffer, index + 1);
            if ((low >> 10) != 0x37) {
                ++index;
                continue;
            }
            character = (character << 10) + low + 0xFCA02400;
            uint32_t plane = (character >> 16) & 0x1f;
            ++index;
            if (plane != 0) {
                planeBitmap = OCFUniCharGetBitmapPtrForPlane(0x66, plane);
            }
        }
        if (planeBitmap != NULL && ((planeBitmap[(character >> 3) & 0x1fff] >> (character & 7)) & 1) != 0) {
            return true;
        }
        ++index;
    } while (index < length);
    return false;
}

// Native __CFStringHandleOutOfMemory, 0x1c030c440. Hidden (was private external). Raises an NSMallocException
// through __CFRaiseMemoryException with the reason "Out of memory. We suggest restarting the application. If
// you have an unsaved document, create a backup copy in Finder, then try to save."; it never returns.
void __CFStringHandleOutOfMemory(void) {
    _CFRaiseMemoryException(OF_STR(
        "Out of memory. We suggest restarting the application. If you have an unsaved document, create a "
        "backup "
        "copy in Finder, then try to save."
    ));
}

// Native _CFBytesInASCII, 0x1c0208960. External. Whether no byte has its high bit set: 32 bytes at a time
// (two 16-byte vectors OR-ed), then 16, 8 and 4 at a time, then single bytes. Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device divergence: unread).
Boolean _OCFBytesInASCII(const void *data, OCFIndex length) {
    const uint8_t *bytes = data;
    uint64_t remaining = (uint64_t)length;
    while (remaining >= 32) {
        uint64_t words[4];
        memcpy(words, bytes, sizeof(words));
        bytes += 32;
        if (((words[0] | words[2]) | (words[1] | words[3])) & 0x8080808080808080) {
            return false;
        }
        remaining -= 32;
    }
    while (remaining >= 16) {
        uint64_t words[2];
        memcpy(words, bytes, sizeof(words));
        bytes += 16;
        if ((words[1] | words[0]) & 0x8080808080808080) {
            return false;
        }
        remaining -= 16;
    }
    while (remaining >= 8) {
        uint64_t word;
        memcpy(&word, bytes, sizeof(word));
        bytes += 8;
        if (word & 0x8080808080808080) {
            return false;
        }
        remaining -= 8;
    }
    while (remaining >= 4) {
        uint32_t word;
        memcpy(&word, bytes, sizeof(word));
        bytes += 4;
        if (word & 0x80808080) {
            return false;
        }
        remaining -= 4;
    }
    while (remaining != 0) {
        if ((int8_t)*bytes < 0) {
            return false;
        }
        bytes++;
        remaining--;
    }
    return true;
}

// Native CFStringHashCharacters, 0x1c02094ac. External. The UTF-16 hash __CFStringHash inlines
// (hashUnicodeCharacters): up to 96 characters every character, longer strings their first 32, the 32
// starting 16 before the middle, and their last 32. Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device divergence: unread).
OCFHashCode OCFStringHashCharacters(const UniChar *characters, OCFIndex length) {
    return hashUnicodeCharacters(characters, length);
}
