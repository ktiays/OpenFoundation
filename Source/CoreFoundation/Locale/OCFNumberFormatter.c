//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/OCFNumberFormatter.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Number/OCFBigNumber.h"
#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/String/OCFString.h"

#include <errno.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <xlocale.h>

// Inferred organization: the CFNumberFormatter functions restored here lie in one iOS Simulator __text region
// (__CFNumberFormatterDeallocate 0x1c01ae05c through __CFNumberFormatterApplySymbolPrefs 0x1c01b1590), with
// the unit's other functions (outside the scope) between them; the class record is at 0x2227a2620 and the
// property keys are in the locale-keys table (Locale/OCFLocaleKeys.c). The native source path is not in the
// evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; static tables read with
// Local/Scratch/cf-core/macho.py); iOS Device 24A446: unread. Reading: not yet written; the evidence is the
// assembly in Local/Evidence/cf-core/iphone-simulator-core-corpus.json.
//
// _CFAssertMismatchedTypeID is declared by PlugIn/OCFBundle.h (an interface of a family not yet restored).

// ICU (libicucore, linked by the native image); supplied, never reimplemented. Declared here because the SDK
// installs no unum.h. Enumerations are 32-bit; UBool is 8-bit; UNumberFormat is opaque.
extern void *unum_open(
    int32_t style,
    const UniChar *pattern,
    int32_t patternLength,
    const char *locale,
    void *parseErr,
    int32_t *status
);
extern void unum_close(void *format);
extern void *unum_clone(const void *format, int32_t *status);
extern void unum_applyPattern(
    void *format,
    int8_t localized,
    const UniChar *pattern,
    int32_t patternLength,
    void *parseError,
    int32_t *status
);
extern int32_t unum_getAttribute(const void *format, int32_t attr);
extern void unum_setAttribute(void *format, int32_t attr, int32_t newValue);
extern double unum_getDoubleAttribute(const void *format, int32_t attr);
extern void unum_setDoubleAttribute(void *format, int32_t attr, double newValue);
extern int32_t unum_getTextAttribute(
    const void *format, int32_t tag, UniChar *result, int32_t resultLength, int32_t *status
);
extern void unum_setTextAttribute(
    void *format, int32_t tag, const UniChar *newValue, int32_t newValueLength, int32_t *status
);
extern int32_t
unum_getSymbol(const void *format, int32_t symbol, UniChar *buffer, int32_t size, int32_t *status);
extern void
unum_setSymbol(void *format, int32_t symbol, const UniChar *value, int32_t length, int32_t *status);
extern int32_t unum_toPattern(
    const void *format, int8_t isPatternLocalized, UniChar *result, int32_t resultLength, int32_t *status
);
extern int32_t unum_formatDouble(
    const void *format, double number, UniChar *result, int32_t resultLength, void *pos, int32_t *status
);
extern int32_t unum_formatDecimal(
    const void *format,
    const char *number,
    int32_t length,
    UniChar *result,
    int32_t resultLength,
    void *pos,
    int32_t *status
);
extern int32_t unum_parseDecimal(
    const void *format,
    const UniChar *text,
    int32_t textLength,
    int32_t *parsePos,
    char *outBuf,
    int32_t outBufLength,
    int32_t *status
);
extern void unum_setContext(void *format, int32_t value, int32_t *status);
extern int32_t unum_getContext(const void *format, int32_t type, int32_t *status);

// The CFNumberFormatter instance, 88 bytes (_CFRuntimeCreateInstance extra bytes 0x48), read from the
// restored functions: the ICU formatter at 0x10, the locale at 0x18, the style at 0x20, the current format at
// 0x28, the default format at 0x30, the compressed format at 0x38, the multiplier number at 0x40, the zero
// symbol at 0x48, and three flag bytes: lenient parsing (0x50), a multiplier set through the multiplier key
// (0x51) and the character-direction option (0x52). Field names are local descriptive spellings; confidence:
// verified for the offsets named.
struct __OCFNumberFormatter {
    OCFRuntimeBase base;
    void *icuFormatter;
    OCFLocaleRef locale;
    OCFIndex style;
    OCFStringRef format;
    OCFStringRef defaultFormat;
    OCFStringRef compressedFormat;
    OCFNumberRef multiplier;
    OCFStringRef zeroSymbol;
    Boolean isLenient;
    Boolean userSetMultiplier;
    Boolean usesCharacterDirection;
};

_Static_assert(sizeof(struct __OCFNumberFormatter) == 0x58, "The native number formatter is 88 bytes.");
_Static_assert(offsetof(struct __OCFNumberFormatter, isLenient) == 0x50, "Lenient flag at 0x50.");

// kCFNumberSInt128Type, the literal 17 at the native comparison (the restored OCFNumberType ends at 16).
#define kNumberSInt128Type ((OCFNumberType)17)

// U_BUFFER_OVERFLOW_ERROR and U_UNSUPPORTED_ERROR, the literals 15 and 16 at the native uses.
#define kICUBufferOverflowError 15
#define kICUUnsupportedError 16

// The malloc type ID of the native UniChar buffers (0x1000040BDFB0063 at each call site).
#define kUniCharBufferTypeID 0x1000040BDFB0063ULL

// The arm64 FCVTZS conversion the native code uses: NaN converts to 0 and values out of range saturate (a C
// conversion would be undefined there).
static int64_t convertToInt64(double value) {
    if (value != value) {
        return 0;
    }
    if (value >= 0x1p63) {
        return INT64_MAX;
    }
    if (value < -0x1p63) {
        return INT64_MIN;
    }
    return (int64_t)value;
}

static void __CFNumberFormatterDeallocate(OCFTypeRef cf);
static OCFStringRef __CFNumberFormatterCopyDescription(OCFTypeRef cf);
static OCFStringRef
__CFNumberFormatterCreateCompressedString(OCFStringRef inString, Boolean isFormat, OCFRange *range);
static int32_t __CFNumberFormatterApplyPattern(OCFNumberFormatterRef formatter, OCFStringRef pattern);
static void __CFNumberFormatterApplySymbolPrefs(const void *key, const void *value, void *context);

// Native __CFNumberFormatterDeallocate, 0x1c01ae05c. Non-external. Closes the ICU formatter, then releases
// the locale and the fields at 0x28-0x48 in address order, each only when not NULL.
static void __CFNumberFormatterDeallocate(OCFTypeRef cf) {
    struct __OCFNumberFormatter *formatter = (struct __OCFNumberFormatter *)cf;
    if (formatter->icuFormatter != NULL) {
        unum_close(formatter->icuFormatter);
    }
    if (formatter->locale != NULL) {
        OCFRelease(formatter->locale);
    }
    if (formatter->format != NULL) {
        OCFRelease(formatter->format);
    }
    if (formatter->defaultFormat != NULL) {
        OCFRelease(formatter->defaultFormat);
    }
    if (formatter->compressedFormat != NULL) {
        OCFRelease(formatter->compressedFormat);
    }
    if (formatter->multiplier != NULL) {
        OCFRelease(formatter->multiplier);
    }
    if (formatter->zeroSymbol != NULL) {
        OCFRelease(formatter->zeroSymbol);
    }
}

// Native __CFNumberFormatterCopyDescription, 0x1c01ae0d4. Non-external. CFGetAllocator is called twice: first
// for the result's allocator, then for the second format argument.
static OCFStringRef __CFNumberFormatterCopyDescription(OCFTypeRef cf) {
    OCFAllocatorRef allocator = OCFGetAllocator(cf);
    OCFAllocatorRef described = OCFGetAllocator(cf);
    return OCFStringCreateWithFormat(allocator, NULL, OF_STR("<CFNumberFormatter %p [%p]>"), cf, described);
}

// Native __CFNumberFormatterClass, 0x2227a2620 (__DATA_CONST,__const). Hidden (was private external); version
// 0, name "CFNumberFormatter" (0x1c04837c0), the finalizer and the debug description only.
const OCFRuntimeClass __CFNumberFormatterClass = {
    0,
    "CFNumberFormatter",
    NULL,
    NULL,
    __CFNumberFormatterDeallocate,
    NULL,
    NULL,
    NULL,
    __CFNumberFormatterCopyDescription,
    NULL,
    NULL,
    0,
};

// The ICU styles for the CFNumberFormatter styles 0-10 (0x1c03757c4, eleven 32-bit values, __TEXT,__const;
// unnamed). Local descriptive name.
static const int32_t icuNumberFormatStyles[11] = {0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12};

// The AppleICUNumberFormatStrings keys for the styles 1-10 (0x2227a2680, ten constant-string pointers after
// __CFNumberFormatterClass; unnamed) and the ICU styles of the matching locale formats (0x1c03757f0, ten
// 32-bit values; unnamed). The key "0" (0x2227ac7e0) also stands for an index outside the table. Local
// descriptive names.
static const OCFConstantString formatStringsKey0 = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "0", 1
};
static const OCFConstantString formatStringsKey1 = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "1", 1
};
static const OCFConstantString formatStringsKey2 = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "2", 1
};
static const OCFConstantString formatStringsKey3 = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "3", 1
};
static const OCFConstantString formatStringsKey4 = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "4", 1
};
static const OCFConstantString formatStringsKey8 = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "8", 1
};
static const OCFConstantString formatStringsKey9 = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "9", 1
};
static const OCFConstantString formatStringsKey10 = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "10", 2
};
static const OCFStringRef formatStringsKeys[10] = {
    (OCFStringRef)&formatStringsKey1,
    (OCFStringRef)&formatStringsKey2,
    (OCFStringRef)&formatStringsKey3,
    (OCFStringRef)&formatStringsKey4,
    (OCFStringRef)&formatStringsKey0,
    (OCFStringRef)&formatStringsKey0,
    (OCFStringRef)&formatStringsKey0,
    (OCFStringRef)&formatStringsKey8,
    (OCFStringRef)&formatStringsKey9,
    (OCFStringRef)&formatStringsKey10,
};
static const int32_t formatStringsStyles[10] = {1, 2, 3, 4, 0, 0, 0, 10, 11, 12};

// The constant empty string (0x2227ac180) used as the identifier of a NULL locale.
static const OCFConstantString emptyLocaleIdentifier = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "", 0
};

// Native CFNumberFormatterCreateCopyWithICUFormat, 0x1c01ae128. Hidden (was a private external; native name).
// Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json. A NULL allocator is the thread
// default (TSD slot 1) or the system default. A new type-57 instance (0x48 extra bytes) receives only a clone
// of the formatter's ICU formatter (when it has one); every other field stays zero. A failed clone (status 1
// or more) closes the clone and still stores it, as natively.
OCFNumberFormatterRef
CFNumberFormatterCreateCopyWithICUFormat(OCFAllocatorRef allocator, OCFNumberFormatterRef formatter) {
    if (allocator == NULL) {
        allocator = _OCFGetTSD(1);
        if (allocator == NULL) {
            allocator = kOCFAllocatorSystemDefault;
        }
    }
    struct __OCFNumberFormatter *copy =
        (struct __OCFNumberFormatter *)_OCFRuntimeCreateInstance(allocator, 57, 0x48, NULL);
    if (copy != NULL && formatter->icuFormatter != NULL) {
        int32_t status = 0;
        void *icuFormatter = unum_clone(formatter->icuFormatter, &status);
        if (status >= 1) {
            // The closed clone is stored (STR X20 after unum_close). Not treated as a defect: libicucore's
            // unum_clone answers NULL whenever it reports a failure (ICU's interface, not read in IDA;
            // confidence: medium), so NULL is closed and stored.
            unum_close(icuFormatter);
        }
        copy->icuFormatter = icuFormatter;
    }
    return (OCFNumberFormatterRef)copy;
}

// The one-character pattern "#" (0x1c04a3a64, UTF-16) applied to the style-0 formatter.
static const UniChar noStylePattern[2] = {'#', 0};

// Native CFNumberFormatterCreate, 0x1c01ae1e4. External.
// A NULL allocator is the thread default (TSD slot 1) or the system default. The instance is type 57 with
// 0x48 extra bytes. A NULL locale is CFLocaleGetSystem(). Styles 11 and above are stored as 1; the ICU style
// comes from icuNumberFormatStyles. The locale identifier (the empty string for a NULL locale) is used as an
// ASCII C string pointer or copied to a 768-byte buffer; a failed copy or unum_open releases the instance and
// answers NULL. Style 0 applies the pattern "#" and sets the maximum integer digits (attribute 3) to 42 and
// the maximum fraction digits (6) to 0. The locale field retains the given locale, or holds
// CFLocaleGetSystem() unretained when the argument was NULL. Styles 1-4, 8 and 10 look up
// AppleICUNumberFormatStrings (a dictionary) by the style key; for a string value a temporary ICU formatter
// of the matching style gives a pattern that is replaced by the value inside the formatter's pattern (when
// found), and the result is applied. Every style then applies the AppleICUNumberSymbols dictionary through
// __CFNumberFormatterApplySymbolPrefs. Styles other than 5, 6, 7 and 9 store the ICU pattern as the format; a
// format is retained as the default format and compressed. Those styles also move an ICU multiplier
// (attribute 9) other than 1 into the multiplier field and reset ICU's to 1. Last, lenient parsing (attribute
// 19) is cleared and the capitalization context set to 0x100 with the status of the last ICU call that used
// it.
OCFNumberFormatterRef
OCFNumberFormatterCreate(OCFAllocatorRef allocator, OCFLocaleRef locale, OCFIndex style) {
    if (allocator == NULL) {
        allocator = _OCFGetTSD(1);
        if (allocator == NULL) {
            allocator = kOCFAllocatorSystemDefault;
        }
    }
    struct __OCFNumberFormatter *formatter =
        (struct __OCFNumberFormatter *)_OCFRuntimeCreateInstance(allocator, 57, 0x48, NULL);
    if (formatter == NULL) {
        return NULL;
    }
    if (locale == NULL) {
        locale = OCFLocaleGetSystem();
    }
    formatter->style = style;
    int32_t icuStyle;
    if ((uint64_t)style < 11) {
        icuStyle = icuNumberFormatStyles[style];
    } else {
        icuStyle = 1;
        formatter->style = 1;
    }
    OCFStringRef identifier =
        locale != NULL ? OCFLocaleGetIdentifier(locale) : (OCFStringRef)&emptyLocaleIdentifier;
    char buffer[768];
    const char *cLocale = OCFStringGetCStringPtr(identifier, kOCFStringEncodingASCII);
    if (cLocale == NULL) {
        cLocale = buffer;
        if (!OCFStringGetCString(identifier, buffer, 768, kOCFStringEncodingASCII)) {
            OCFRelease(formatter);
            return NULL;
        }
    }
    int32_t status = 0;
    formatter->icuFormatter = unum_open(icuStyle, NULL, 0, cLocale, NULL, &status);
    if (formatter->icuFormatter == NULL) {
        OCFRelease(formatter);
        return NULL;
    }
    if (style == 0) {
        status = 0;
        unum_applyPattern(formatter->icuFormatter, 0, noStylePattern, 1, NULL, &status);
        unum_setAttribute(formatter->icuFormatter, 3, 42);
        unum_setAttribute(formatter->icuFormatter, 6, 0);
    }
    formatter->locale = locale != NULL ? OCFRetain(locale) : OCFLocaleGetSystem();
    UniChar chars[768];
    OCFIndex currentStyle = formatter->style;
    if ((uint64_t)currentStyle > 9 || ((1ULL << currentStyle) & 0x2e1) == 0) {
        OCFTypeRef formatStrings = _CFLocaleGetAppleICUNumberFormatStringsPref(formatter->locale);
        if (formatStrings != NULL && OCFGetTypeID(formatStrings) == OCFDictionaryGetTypeID()) {
            uint64_t index = (uint64_t)currentStyle - 1;
            OCFStringRef key = index <= 9 ? formatStringsKeys[index] : (OCFStringRef)&formatStringsKey0;
            OCFTypeRef replacement = OCFDictionaryGetValue(formatStrings, key);
            if (replacement != NULL && OCFGetTypeID(replacement) == OCFStringGetTypeID()) {
                int32_t replacementStyle = index <= 9 ? formatStringsStyles[index] : 0;
                OCFStringRef localeIdentifier = OCFLocaleGetIdentifier(formatter->locale);
                const char *cIdentifier = OCFStringGetCStringPtr(localeIdentifier, kOCFStringEncodingASCII);
                char identifierBuffer[768];
                if (cIdentifier == NULL) {
                    cIdentifier =
                        OCFStringGetCString(localeIdentifier, identifierBuffer, 768, kOCFStringEncodingASCII)
                            ? identifierBuffer
                            : NULL;
                }
                int32_t localStatus = 0;
                void *localFormatter = unum_open(replacementStyle, NULL, 0, cIdentifier, NULL, &localStatus);
                if (localFormatter != NULL) {
                    localStatus = 0;
                    int32_t localLength = unum_toPattern(localFormatter, 0, chars, 768, &localStatus);
                    if (localStatus <= 0 && localLength <= 768) {
                        OCFStringRef localPattern =
                            OCFStringCreateWithCharacters(kOCFAllocatorSystemDefault, chars, localLength);
                        localStatus = 0;
                        int32_t length = unum_toPattern(formatter->icuFormatter, 0, chars, 768, &localStatus);
                        if (localStatus <= 0 && length <= 768) {
                            OCFMutableStringRef pattern =
                                OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
                            OCFStringAppendCharacters(pattern, chars, length);
                            OCFRange found = {0, 0};
                            if (OCFStringFindWithOptions(
                                    pattern, localPattern, (OCFRange){0, length}, 0, &found
                                )) {
                                OCFStringReplace(pattern, found, replacement);
                                __CFNumberFormatterApplyPattern(formatter, pattern);
                            }
                            OCFRelease(pattern);
                        }
                        OCFRelease(localPattern);
                    }
                    unum_close(localFormatter);
                }
            }
        }
    }
    OCFTypeRef symbols = _CFLocaleGetAppleICUNumberSymbolsPref(formatter->locale);
    if (symbols != NULL && OCFGetTypeID(symbols) == OCFDictionaryGetTypeID()) {
        OCFDictionaryApplyFunction(symbols, __CFNumberFormatterApplySymbolPrefs, formatter);
    }
    if ((uint64_t)formatter->style > 9 || ((1ULL << formatter->style) & 0x2e0) == 0) {
        status = 0;
        int32_t length = unum_toPattern(formatter->icuFormatter, 0, chars, 768, &status);
        if (status <= 0 && length <= 768) {
            formatter->format = OCFStringCreateWithCharacters(allocator, chars, length);
        }
    }
    if (formatter->format != NULL) {
        formatter->defaultFormat = OCFRetain(formatter->format);
        formatter->compressedFormat = __CFNumberFormatterCreateCompressedString(formatter->format, 1, NULL);
    }
    if ((uint64_t)formatter->style > 9 || ((1ULL << formatter->style) & 0x2e0) == 0) {
        int32_t multiplier = unum_getAttribute(formatter->icuFormatter, 9);
        if (multiplier != 1) {
            formatter->multiplier = OCFNumberCreate(allocator, kOCFNumberSInt32Type, &multiplier);
            unum_setAttribute(formatter->icuFormatter, 9, 1);
        }
    }
    unum_setAttribute(formatter->icuFormatter, 19, 0);
    unum_setContext(formatter->icuFormatter, 0x100, &status);
    return formatter;
}

// Native __CFNumberFormatterCreateCompressedString, 0x1c01ae6ec. Non-external.
// NULL for a NULL string. The range defaults to the whole string. Each character becomes a space when it is
// whitespace (the whitespace set is fetched for every character); quote characters toggle a quoted run when
// isFormat is set. A space outside quotes is kept only between two letters or between two decimal digits (the
// previous kept character and the following one). The following character is read while the next index is
// below the range length (not the range end), as natively; the loop ends at the range end. The result is a
// mutable string in the system default allocator.
static OCFStringRef
__CFNumberFormatterCreateCompressedString(OCFStringRef inString, Boolean isFormat, OCFRange *range) {
    if (inString == NULL) {
        return NULL;
    }
    OCFIndex location;
    OCFIndex length;
    if (range != NULL) {
        location = range->location;
        length = range->length;
    } else {
        length = OCFStringGetLength(inString);
        location = 0;
    }
    OCFMutableStringRef outString = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    OCFCharacterSetRef letters = OCFCharacterSetGetPredefined(kOCFCharacterSetLetter);
    OCFCharacterSetRef digits = OCFCharacterSetGetPredefined(kOCFCharacterSetDecimalDigit);
    if (length < 1) {
        return outString;
    }
    uint8_t inQuote = 0;
    UniChar previous = 0;
    OCFIndex end = location + length;
    OCFIndex index = location;
    do {
        UniChar character = OCFStringGetCharacterAtIndex(inString, index);
        OCFCharacterSetRef whitespace = OCFCharacterSetGetPredefined(kOCFCharacterSetWhitespace);
        UniChar ch = OCFCharacterSetIsCharacterMember(whitespace, character) ? ' ' : character;
        index++;
        UniChar next = index < length ? OCFStringGetCharacterAtIndex(inString, index) : 0;
        uint8_t isQuote = (isFormat != 0 && ch == '\'') ? 1 : 0;
        if (isQuote != inQuote || ch != ' ' ||
            (OCFCharacterSetIsCharacterMember(letters, previous) &&
             OCFCharacterSetIsCharacterMember(letters, next)) ||
            (OCFCharacterSetIsCharacterMember(digits, previous) &&
             OCFCharacterSetIsCharacterMember(digits, next))) {
            OCFStringAppendCharacters(outString, &ch, 1);
            previous = ch;
        }
        inQuote ^= isQuote;
    } while (index < end);
    return outString;
}

// Native CFNumberFormatterGetLocale, 0x1c01ae8e8. External. Type 57 is asserted (_CFAssertMismatchedTypeID).
OCFLocaleRef OCFNumberFormatterGetLocale(OCFNumberFormatterRef formatter) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != 57) {
        _CFAssertMismatchedTypeID(57, typeID);
    }
    return formatter->locale;
}

// Native CFNumberFormatterGetFormat, 0x1c01ae958. External.
// NULL for the styles 5, 6, 7 and 9 (style - 5 at most 4 with bit (style - 5) of 0x17). Otherwise the ICU
// pattern (up to 768 characters) replaces the stored format when it differs (CFEqual), with a new compressed
// format; the stored format is answered.
OCFStringRef OCFNumberFormatterGetFormat(OCFNumberFormatterRef formatter) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != 57) {
        _CFAssertMismatchedTypeID(57, typeID);
    }
    uint64_t offset = (uint64_t)formatter->style - 5;
    if (offset <= 4 && ((0x17u >> offset) & 1) != 0) {
        return NULL;
    }
    int32_t status = 0;
    UniChar chars[768];
    int32_t length = unum_toPattern(formatter->icuFormatter, 0, chars, 768, &status);
    if (status <= 0 && length <= 768) {
        OCFStringRef newString = OCFStringCreateWithCharacters(OCFGetAllocator(formatter), chars, length);
        if (newString != NULL) {
            if (formatter->format != NULL) {
                if (OCFEqual(newString, formatter->format)) {
                    OCFRelease(newString);
                    return formatter->format;
                }
                OCFRelease(formatter->format);
            }
            formatter->format = newString;
            if (formatter->compressedFormat != NULL) {
                OCFRelease(formatter->compressedFormat);
            }
            formatter->compressedFormat =
                __CFNumberFormatterCreateCompressedString(formatter->format, 1, NULL);
        }
    }
    return formatter->format;
}

// Native CFNumberFormatterSetFormat, 0x1c01aea90. External.
// Ignored for the styles 5, 6, 7 and 9, for a string equal to the stored format and for one longer than 1,024
// characters. A pattern that ICU rejects (status above zero) changes nothing; otherwise the ICU pattern (up
// to 768 characters) becomes the stored format and is compressed again.
void OCFNumberFormatterSetFormat(OCFNumberFormatterRef formatter, OCFStringRef formatString) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != 57) {
        _CFAssertMismatchedTypeID(57, typeID);
    }
    if ((uint64_t)formatter->style <= 9 && ((1ULL << formatter->style) & 0x2e0) != 0) {
        return;
    }
    OCFIndex length = OCFStringGetLength(formatString);
    if (formatter->format != NULL) {
        if (OCFEqual(formatter->format, formatString) || length >= 0x401) {
            return;
        }
    } else if (length > 0x400) {
        return;
    }
    if (__CFNumberFormatterApplyPattern(formatter, formatString) > 0) {
        return;
    }
    int32_t status = 0;
    UniChar chars[768];
    int32_t patternLength = unum_toPattern(formatter->icuFormatter, 0, chars, 768, &status);
    if (status > 0 || patternLength > 768) {
        return;
    }
    if (formatter->format != NULL) {
        OCFRelease(formatter->format);
    }
    formatter->format = OCFStringCreateWithCharacters(OCFGetAllocator(formatter), chars, patternLength);
    if (formatter->compressedFormat != NULL) {
        OCFRelease(formatter->compressedFormat);
    }
    formatter->compressedFormat = __CFNumberFormatterCreateCompressedString(formatter->format, 1, NULL);
}

// Native __CFNumberFormatterApplyPattern, 0x1c01aebec. Non-external; answers the ICU status.
// 16 (U_UNSUPPORTED_ERROR) for the styles 5, 6, 7 and 9. The pattern's characters come from the direct
// pointer or a buffer: malloc_type_malloc above 256 bytes (freed afterwards), otherwise a zeroed stack buffer
// (none for an empty string; the native stack area is sized to the length, here a fixed 128 characters).
// After unum_applyPattern, unless the multiplier was set through the multiplier key, the stored multiplier is
// released and replaced by ICU's multiplier (attribute 9) when that is not 1; ICU's multiplier is then reset
// to 1 (not when it was already 1).
static int32_t __CFNumberFormatterApplyPattern(OCFNumberFormatterRef formatter, OCFStringRef pattern) {
    uint64_t offset = (uint64_t)formatter->style - 5;
    if (offset <= 4 && ((0x17u >> offset) & 1) != 0) {
        return kICUUnsupportedError;
    }
    OCFIndex length = OCFStringGetLength(pattern);
    uint64_t size = (uint64_t)length << 1;
    UniChar stackBuffer[128];
    UniChar *buffer;
    if (size > 0x100) {
        buffer = malloc_type_malloc(size, kUniCharBufferTypeID);
    } else if (length >= 1) {
        buffer = stackBuffer;
        bzero(buffer, size);
    } else {
        buffer = NULL;
    }
    const UniChar *chars = OCFStringGetCharactersPtr(pattern);
    if (chars == NULL) {
        OCFStringGetCharacters(pattern, (OCFRange){0, length}, buffer);
        chars = buffer;
    }
    int32_t status = 0;
    unum_applyPattern(formatter->icuFormatter, 0, chars, (int32_t)length, NULL, &status);
    if (size > 0x100) {
        free(buffer);
    }
    if (!formatter->userSetMultiplier) {
        if (formatter->multiplier != NULL) {
            OCFRelease(formatter->multiplier);
        }
        formatter->multiplier = NULL;
        int32_t multiplier = unum_getAttribute(formatter->icuFormatter, 9);
        if (multiplier == 1) {
            return status;
        }
        formatter->multiplier =
            OCFNumberCreate(OCFGetAllocator(formatter), kOCFNumberSInt32Type, &multiplier);
    }
    unum_setAttribute(formatter->icuFormatter, 9, 1);
    return status;
}

// Native CFNumberFormatterCreateStringWithNumber, 0x1c01aed8c. External.
// A NULL allocator is the thread default (TSD slot 1) or the system default. CFGetTypeID(number) is called
// first; a CFNumber that is neither a tagged pointer nor an Objective-C object is read with its own type
// (_CFNumberGetType2) and formatted by CFNumberFormatterCreateStringWithValue. An Objective-C number answers
// the retained zero symbol when -doubleValue is 0.0 and one is set; otherwise the multiplier (1.0 when absent
// or unreadable, rounded down when its fractional part is below 2^-23) is passed to
// -_getCString:length:multiplier: with a 768-byte buffer (NO answers NULL), and that decimal string is
// formatted with unum_formatDecimal into a 769-character stack buffer from its second character, again into a
// system-default buffer of length + 2 characters on U_BUFFER_OVERFLOW_ERROR or 769 characters or more. A
// failing status answers NULL; the character-direction option with a right-to-left locale language puts
// U+200F before the result. The native code reads the CFNumber's value into the start of the character
// buffer; a separate local holds it here.
OCFStringRef OCFNumberFormatterCreateStringWithNumber(
    OCFAllocatorRef allocator, OCFNumberFormatterRef formatter, OCFNumberRef number
) {
    if (allocator == NULL) {
        allocator = _OCFGetTSD(1);
        if (allocator == NULL) {
            allocator = kOCFAllocatorSystemDefault;
        }
    }
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != 57) {
        _CFAssertMismatchedTypeID(57, typeID);
    }
    OCFTypeID numberTypeID = OCFGetTypeID(number);
    if ((intptr_t)number >= 0 && !__CF_IS_OBJC(numberTypeID, number)) {
        OCFNumberType numberType = _OCFNumberGetType2(number);
        _Alignas(16) uint8_t value[16];
        OCFNumberGetValue(number, numberType, value);
        return OCFNumberFormatterCreateStringWithValue(allocator, formatter, numberType, value);
    }
    double doubleValue = ((double (*)(id, SEL))objc_msgSend)((id)number, sel_getUid("doubleValue"));
    if (doubleValue == 0.0 && formatter->zeroSymbol != NULL) {
        return OCFRetain(formatter->zeroSymbol);
    }
    double multiplierValue = 1.0;
    double multiplier = 1.0;
    if (formatter->multiplier != NULL) {
        if (OCFNumberGetValue(formatter->multiplier, kOCFNumberFloat64Type, &multiplierValue)) {
            multiplier = multiplierValue;
        } else {
            multiplierValue = 1.0;
        }
    }
    double integral;
    if (modf(multiplier, &integral) < 0x1p-23) {
        multiplier = floor(multiplier);
        multiplierValue = multiplier;
    }
    char decimal[768];
    if (!((BOOL (*)(id, SEL, char *, int, double))objc_msgSend)(
            (id)number, sel_getUid("_getCString:length:multiplier:"), decimal, 0x300, multiplier
        )) {
        return NULL;
    }
    UniChar stackBuffer[769];
    UniChar *allocated = NULL;
    int32_t status = 0;
    OCFIndex length = unum_formatDecimal(
        formatter->icuFormatter, decimal, (int32_t)strlen(decimal), stackBuffer + 1, 0x300, NULL, &status
    );
    if (status == kICUBufferOverflowError || (int32_t)length >= 0x301) {
        length += 2;
        allocated =
            OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, length << 1, kUniCharBufferTypeID, 0);
        status = 0;
        length = unum_formatDecimal(
            formatter->icuFormatter,
            decimal,
            (int32_t)strlen(decimal),
            allocated + 1,
            (int32_t)length,
            NULL,
            &status
        );
    }
    OCFStringRef result = NULL;
    if (status <= 0) {
        UniChar *buffer = allocated != NULL ? allocated : stackBuffer;
        const UniChar *chars;
        if (formatter->usesCharacterDirection &&
            OCFLocaleGetLanguageCharacterDirection(OCFLocaleGetIdentifier(formatter->locale)) ==
                kOCFLocaleLanguageDirectionRightToLeft) {
            buffer[0] = 0x200f;
            length = length + 1;
            chars = buffer;
        } else {
            chars = buffer + 1;
        }
        result = OCFStringCreateWithCharacters(allocator, chars, length);
    }
    if (allocated != NULL) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, allocated);
    }
    return result;
}

// Native CFNumberFormatterCreateStringWithValue, 0x1c01af098. External.
// A NULL allocator is the thread default (TSD slot 1) or the system default. The multiplier is the stored
// number read as a double (1.0 when absent or unreadable); when its fractional part (modf) is below 2^-23 it
// is rounded down (FRINTM). Doubles (types 6, 13) and floats (5, 12, multiplied in double precision and
// narrowed) are formatted with unum_formatDouble; the integer types are multiplied in double precision
// (unless the multiplier is 1.0), truncated back to their width, put into a BigNum and formatted with
// unum_formatDecimal from its C string (no leading zeros, a leading plus): 64-bit (4, 10, 11, 14), 32-bit (3,
// 9), 16-bit (2, 8), 8-bit (1, 7) and 128-bit (17, high word first). Other types answer NULL. A zero value
// answers the retained zero symbol when one is set. The output goes to a 769-character stack buffer from its
// second character; a result of 769 characters or more, or U_BUFFER_OVERFLOW_ERROR, formats again into a
// system-default buffer of length + 2 characters (malloc type 0x1000040BDFB0063), freed afterwards. With the
// character-direction option and a right-to-left locale language, U+200F is put before the result.
OCFStringRef OCFNumberFormatterCreateStringWithValue(
    OCFAllocatorRef allocator, OCFNumberFormatterRef formatter, OCFNumberType numberType, const void *valuePtr
) {
    if (allocator == NULL) {
        allocator = _OCFGetTSD(1);
        if (allocator == NULL) {
            allocator = kOCFAllocatorSystemDefault;
        }
    }
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != 57) {
        _CFAssertMismatchedTypeID(57, typeID);
    }
    double multiplierValue = 1.0;
    double multiplier = 1.0;
    if (formatter->multiplier != NULL) {
        if (OCFNumberGetValue(formatter->multiplier, kOCFNumberFloat64Type, &multiplierValue)) {
            multiplier = multiplierValue;
        } else {
            multiplierValue = 1.0;
        }
    }
    double integral;
    if (modf(multiplier, &integral) < 0x1p-23) {
        multiplier = floor(multiplier);
        multiplierValue = multiplier;
    }
    int32_t status = 0;
    UniChar stackBuffer[769];
    UniChar *allocated = NULL;
    OCFIndex length;
    uint64_t type = (uint64_t)numberType;
    Boolean isDouble = type <= 13 && ((1ULL << type) & 0x2040) != 0;
    Boolean isFloat = type <= 13 && ((1ULL << type) & 0x1020) != 0;
    Boolean isInt64 = (type <= 13 && ((1ULL << type) & 0x810) != 0) || (type & ~4ULL) == 10;
    if (!isInt64 && (isFloat || isDouble)) {
        double value;
        if (isFloat) {
            float floatValue = *(const float *)valuePtr;
            if (floatValue == 0.0f && formatter->zeroSymbol != NULL) {
                return OCFRetain(formatter->zeroSymbol);
            }
            float scaled = (float)(multiplier * (double)floatValue);
            value = (double)(multiplier != 1.0 ? scaled : floatValue);
        } else {
            double doubleValue = *(const double *)valuePtr;
            if (doubleValue == 0.0 && formatter->zeroSymbol != NULL) {
                return OCFRetain(formatter->zeroSymbol);
            }
            double scaled = doubleValue * multiplier;
            value = multiplier != 1.0 ? scaled : doubleValue;
        }
        length = unum_formatDouble(formatter->icuFormatter, value, stackBuffer + 1, 0x300, NULL, &status);
        if (status == kICUBufferOverflowError || (int32_t)length >= 0x301) {
            length += 2;
            allocated =
                OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, length << 1, kUniCharBufferTypeID, 0);
            status = 0;
            length = unum_formatDouble(
                formatter->icuFormatter, value, allocated + 1, (int32_t)length, NULL, &status
            );
        }
    } else {
        BigNum number = {{0, 0, 0, 0, 0}, 0, 0};
        if (isInt64) {
            int64_t value = *(const int64_t *)valuePtr;
            if (value == 0 && formatter->zeroSymbol != NULL) {
                return OCFRetain(formatter->zeroSymbol);
            }
            int64_t scaled = (int64_t)(multiplier * (double)value);
            _OCFBigNumInitWithInt64(&number, multiplier != 1.0 ? scaled : value);
        } else if (type == 1 || type == 7) {
            uint8_t value = *(const uint8_t *)valuePtr;
            if (value == 0 && formatter->zeroSymbol != NULL) {
                return OCFRetain(formatter->zeroSymbol);
            }
            int32_t scaled = (int32_t)(multiplier * (double)(int8_t)value);
            _OCFBigNumInitWithInt8(&number, (int8_t)(multiplier != 1.0 ? scaled : value));
        } else if (type == 2 || type == 8) {
            uint16_t value = *(const uint16_t *)valuePtr;
            if (value == 0 && formatter->zeroSymbol != NULL) {
                return OCFRetain(formatter->zeroSymbol);
            }
            int32_t scaled = (int32_t)(multiplier * (double)(int16_t)value);
            _OCFBigNumInitWithInt16(&number, (int16_t)(multiplier != 1.0 ? scaled : value));
        } else if (type == 3 || type == 9) {
            int32_t value = *(const int32_t *)valuePtr;
            if (value == 0 && formatter->zeroSymbol != NULL) {
                return OCFRetain(formatter->zeroSymbol);
            }
            int32_t scaled = (int32_t)(multiplier * (double)value);
            _OCFBigNumInitWithInt32(&number, multiplier != 1.0 ? scaled : value);
        } else if (numberType == kNumberSInt128Type) {
            // CFSInt128Struct: the signed high word at offset 0, the low word at offset 8.
            int64_t high = ((const int64_t *)valuePtr)[0];
            uint64_t low = ((const uint64_t *)valuePtr)[1];
            __int128 value = (__int128)(((unsigned __int128)(uint64_t)high << 64) | low);
            if ((high | (int64_t)low) == 0 && formatter->zeroSymbol != NULL) {
                return OCFRetain(formatter->zeroSymbol);
            }
            __int128 scaled = (__int128)(multiplier * (double)value);
            _OCFBigNumInitWithInt128(&number, multiplier != 1.0 ? scaled : value);
        } else {
            return NULL;
        }
        char decimal[768];
        _OCFBigNumToCString(&number, 0, 1, decimal, 0x300);
        status = 0;
        length = unum_formatDecimal(
            formatter->icuFormatter, decimal, (int32_t)strlen(decimal), stackBuffer + 1, 0x300, NULL, &status
        );
        if (status == kICUBufferOverflowError || (int32_t)length >= 0x301) {
            length += 2;
            allocated =
                OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, length << 1, kUniCharBufferTypeID, 0);
            status = 0;
            length = unum_formatDecimal(
                formatter->icuFormatter,
                decimal,
                (int32_t)strlen(decimal),
                allocated + 1,
                (int32_t)length,
                NULL,
                &status
            );
        }
    }
    OCFStringRef result = NULL;
    if (status <= 0) {
        UniChar *buffer = allocated != NULL ? allocated : stackBuffer;
        const UniChar *chars;
        if (formatter->usesCharacterDirection &&
            OCFLocaleGetLanguageCharacterDirection(OCFLocaleGetIdentifier(formatter->locale)) ==
                kOCFLocaleLanguageDirectionRightToLeft) {
            buffer[0] = 0x200f;
            length = length + 1;
            chars = buffer;
        } else {
            chars = buffer + 1;
        }
        result = OCFStringCreateWithCharacters(allocator, chars, length);
    }
    if (allocated != NULL) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, allocated);
    }
    return result;
}

// Native CFNumberFormatterCreateNumberFromString, 0x1c01af618. External.
// A NULL allocator is the thread default (TSD slot 1) or the system default. The string is first parsed as a
// 64-bit integer over the given range (the whole string without one) with the multiplier field cleared for
// the call and restored after it. A multiplier m (1.0 when unreadable) must then divide the integer v exactly
// or the integer parse is abandoned: |m| must lie in [1, |v|] (unsigned |v|; NaN fails); an integral m
// divides with a zero remainder (SDIV: INT64_MIN / -1 wraps as the instruction does); otherwise m is scaled
// by 2^k for the first k >= 0 that makes it integral, that integer is doubled (k counting) while it stays
// within
// (-2^50, 2^50), the doubled value is subtracted from v while v stays within [-2^53, 2^53), counting 2^k per
// subtraction, and the rest must divide with fmod 0 (the quotient truncated and added to the count). The
// -1.0 and 1.0 branches after the integral test are unreachable (both values are integral) and kept. The
// integer is answered when kCFNumberFormatterParseIntegersOnly is set or the parse ended at the string's
// length (the range is written back); otherwise, without that option, the string is parsed as a double with
// the caller's range pointer.
OCFNumberRef OCFNumberFormatterCreateNumberFromString(
    OCFAllocatorRef allocator,
    OCFNumberFormatterRef formatter,
    OCFStringRef string,
    OCFRange *rangep,
    OCFOptionFlags options
) {
    if (allocator == NULL) {
        allocator = _OCFGetTSD(1);
        if (allocator == NULL) {
            allocator = kOCFAllocatorSystemDefault;
        }
    }
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != 57) {
        _CFAssertMismatchedTypeID(57, typeID);
    }
    OCFRange range;
    if (rangep != NULL) {
        range = *rangep;
    } else {
        range.location = 0;
        range.length = OCFStringGetLength(string);
    }

    union {
        int64_t integer;
        double floating;
    } value;

    OCFNumberRef multiplierNumber = formatter->multiplier;
    formatter->multiplier = NULL;
    Boolean parsed =
        OCFNumberFormatterGetValueFromString(formatter, string, &range, kOCFNumberSInt64Type, &value);
    formatter->multiplier = multiplierNumber;
    if (parsed) {
        Boolean exact = true;
        if (multiplierNumber != NULL) {
            int64_t integer = value.integer;
            double multiplierValue = 1.0;
            double multiplier = OCFNumberGetValue(multiplierNumber, kOCFNumberFloat64Type, &multiplierValue)
                                    ? multiplierValue
                                    : 1.0;
            uint64_t magnitude = integer < 0 ? 0 - (uint64_t)integer : (uint64_t)integer;
            double absMultiplier = fabs(multiplier);
            double integral;
            if (!(absMultiplier <= (double)magnitude) || absMultiplier < 1.0) {
                exact = false;
            } else if (modf(multiplier, &integral) == 0.0) {
                int64_t divisor = convertToInt64(multiplier);
                if (divisor == -1) {
                    value.integer = (int64_t)(0 - (uint64_t)integer);
                } else if (integer % divisor != 0) {
                    exact = false;
                } else {
                    value.integer = integer / divisor;
                }
            } else if (multiplier == -1.0) {
                value.integer = (int64_t)(0 - (uint64_t)integer);
            } else if (multiplier != 1.0) {
                uint32_t exponent = UINT32_MAX;
                double scaledIntegral;
                do {
                    exponent += 1;
                } while (modf(scalbn(multiplier, (int)exponent), &scaledIntegral) != 0.0);
                uint64_t step = (uint64_t)convertToInt64(scaledIntegral);
                uint64_t doubled;
                if (step + 0x3FFFFFFFFFFFFULL > 0x7FFFFFFFFFFFEULL) {
                    doubled = step;
                } else {
                    uint64_t next;
                    do {
                        doubled = step << 1;
                        exponent += 1;
                        next = 0x3FFFFFFFFFFFFULL + (step << 1);
                        step = doubled;
                    } while (next < 0x7FFFFFFFFFFFFULL);
                }
                uint64_t remaining = (uint64_t)integer;
                uint64_t count = 0;
                if (remaining + 0xFFE0000000000000ULL <= 0xFFC0000000000000ULL) {
                    uint64_t unit = 1ULL << (exponent & 63);
                    do {
                        count += unit;
                        remaining -= doubled;
                    } while (remaining + 0xFFE0000000000000ULL < 0xFFC0000000000001ULL);
                }
                double rest = (double)(int64_t)remaining;
                if (fmod(rest, multiplier) != 0.0) {
                    exact = false;
                } else {
                    value.integer = (int64_t)(count + (uint64_t)convertToInt64(rest / multiplier));
                }
            }
        }
        if (exact) {
            OCFIndex parsedLength = range.length;
            OCFIndex stringLength = OCFStringGetLength(string);
            if ((options & 1) != 0 || parsedLength == stringLength) {
                if (rangep != NULL) {
                    *rangep = range;
                }
                return OCFNumberCreate(allocator, kOCFNumberSInt64Type, &value);
            }
        }
    }
    if ((options & 1) != 0) {
        return NULL;
    }
    if (!OCFNumberFormatterGetValueFromString(formatter, string, rangep, kOCFNumberFloat64Type, &value)) {
        return NULL;
    }
    return OCFNumberCreate(allocator, kOCFNumberFloat64Type, &value);
}

// Native CFNumberFormatterGetValueFromString, 0x1c01af8dc. External.
// With lenient parsing the string is compressed (__CFNumberFormatterCreateCompressedString with the range
// pointer) and parsed from its start; otherwise it is retained and parsed over the range (the whole string
// without one) after its leading whitespace (kCFCharacterSetWhitespace, fetched before the length test). A
// zero symbol, compressed the same way when lenient, equal to the string (CFStringCompare without options)
// skips ICU: the value is 0 and the parse position the string's length. At most 1024 characters are parsed,
// from the string's direct buffer when it has one, else from a stack copy (an empty string without a direct
// buffer is released and answers false). The multiplier field is cleared for the parse; when lenient the
// compressed format is applied before and the format after (each application's multiplier released), and
// the range length is found by matching each parsed character, in order, against the original string with
// whitespace read as a space. unum_parseDecimal runs with integer-only parsing (attribute 0) set for the
// integer number types; its decimal string is read with strtoll_l (integer types, ICU success and a
// nonempty result) and then strtod_l (ICU success and a nonempty result), each setting status 3 on errno or
// trailing characters. The restored multiplier divides both results (the integer through double, FCVTZS). The
// value is stored when it fits the requested type (integers by sign extension; floats against FLT_MAX and
// DBL_MAX, NaN passing); other types answer false.
Boolean OCFNumberFormatterGetValueFromString(
    OCFNumberFormatterRef formatter,
    OCFStringRef string,
    OCFRange *rangep,
    OCFNumberType numberType,
    void *valuePtr
) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != 57) {
        _CFAssertMismatchedTypeID(57, typeID);
    }
    OCFStringRef parsedString = formatter->isLenient
                                    ? __CFNumberFormatterCreateCompressedString(string, false, rangep)
                                    : OCFRetain(string);
    OCFIndex length;
    OCFIndex location;
    if (formatter->isLenient) {
        length = OCFStringGetLength(parsedString);
        location = 0;
    } else {
        if (rangep != NULL) {
            location = rangep->location;
            length = rangep->length;
        } else {
            length = OCFStringGetLength(parsedString);
            location = 0;
        }
        OCFCharacterSetRef whitespace = OCFCharacterSetGetPredefined(kOCFCharacterSetWhitespace);
        if (length >= 1) {
            while (OCFCharacterSetIsCharacterMember(
                whitespace, OCFStringGetCharacterAtIndex(parsedString, location)
            )) {
                location += 1;
                if (length-- <= 1) {
                    length = 0;
                    break;
                }
            }
        }
    }
    Boolean differsFromZeroSymbol;
    if (formatter->zeroSymbol != NULL) {
        OCFStringRef zeroSymbol =
            formatter->isLenient
                ? __CFNumberFormatterCreateCompressedString(formatter->zeroSymbol, false, NULL)
                : OCFRetain(formatter->zeroSymbol);
        differsFromZeroSymbol = OCFStringCompare(parsedString, zeroSymbol, 0) != kOCFCompareEqualTo;
        OCFRelease(zeroSymbol);
    } else {
        differsFromZeroSymbol = true;
    }
    OCFIndex clamped = length < 0 ? 0 : length;
    OCFIndex parseLength = clamped < 0x400 ? clamped : 0x400;
    const UniChar *direct = OCFStringGetCharactersPtr(parsedString);
    uint32_t bufferLength = (length >= 1 && direct == NULL) ? (uint32_t)parseLength : 1;
    UniChar *stackBuffer = __builtin_alloca(((bufferLength << 1) + 15) & 0x1FF0);
    const UniChar *chars;
    if (direct != NULL) {
        chars = formatter->isLenient ? direct : direct + location;
    } else {
        if (length < 1) {
            if (parsedString != NULL) {
                OCFRelease(parsedString);
            }
            return false;
        }
        OCFStringGetCharacters(parsedString, (OCFRange){location, parseLength}, stackBuffer);
        chars = stackBuffer;
    }
    OCFNumberRef multiplier = formatter->multiplier;
    formatter->multiplier = NULL;
    if (formatter->isLenient) {
        __CFNumberFormatterApplyPattern(formatter, formatter->compressedFormat);
        if (formatter->multiplier != NULL) {
            OCFRelease(formatter->multiplier);
        }
        formatter->multiplier = NULL;
    }
    Boolean isInteger = (uint64_t)numberType <= 14 && ((1ULL << numberType) & 0x4F9E) != 0;
    unum_setAttribute(formatter->icuFormatter, 0, isInteger);
    int32_t status = 0;
    int32_t parsePosition = 0;
    int64_t integerValue = 0;
    double doubleValue = 0.0;
    if (differsFromZeroSymbol) {
        char decimal[1024];
        bzero(decimal, 0x400);
        int32_t decimalLength = unum_parseDecimal(
            formatter->icuFormatter, chars, (int32_t)parseLength, &parsePosition, decimal, 1024, &status
        );
        if (status < 1 && decimalLength > 0 && isInteger) {
            char *end = NULL;
            errno = 0;
            integerValue = strtoll_l(decimal, &end, 10, NULL);
            if (errno != 0 || *end != 0) {
                status = 3;
            }
        }
        if (status <= 0 && decimalLength >= 1) {
            char *end = NULL;
            errno = 0;
            doubleValue = strtod_l(decimal, &end, NULL);
            if (errno != 0 || *end != 0) {
                status = 3;
            }
        }
    } else {
        parsePosition = (int32_t)OCFStringGetLength(parsedString);
    }
    if (formatter->isLenient) {
        if (rangep != NULL) {
            OCFIndex cursor = rangep->location;
            if (parsePosition >= 1) {
                OCFIndex end = rangep->length + cursor;
                OCFIndex matched = 0;
                while (cursor < end) {
                    OCFIndex index = cursor;
                    Boolean found = false;
                    while (true) {
                        UniChar parsedCharacter = chars[matched];
                        UniChar character = OCFStringGetCharacterAtIndex(string, index);
                        OCFCharacterSetRef whitespace =
                            OCFCharacterSetGetPredefined(kOCFCharacterSetWhitespace);
                        UniChar compared =
                            OCFCharacterSetIsCharacterMember(whitespace, character) ? 0x20 : character;
                        cursor = index + 1;
                        if (parsedCharacter == compared) {
                            found = true;
                            break;
                        }
                        index += 1;
                        if (end == cursor) {
                            break;
                        }
                    }
                    if (!found) {
                        cursor = end;
                        break;
                    }
                    matched += 1;
                    if (matched >= parsePosition) {
                        break;
                    }
                }
            }
            rangep->length = cursor - rangep->location;
        }
        __CFNumberFormatterApplyPattern(formatter, formatter->format);
        if (formatter->multiplier != NULL) {
            OCFRelease(formatter->multiplier);
        }
    } else if (rangep != NULL) {
        rangep->length = location - rangep->location + parsePosition;
    }
    formatter->multiplier = multiplier;
    OCFRelease(parsedString);
    if (status > 0) {
        return false;
    }
    if (formatter->multiplier != NULL) {
        double multiplierValue = 1.0;
        double divisor = OCFNumberGetValue(formatter->multiplier, kOCFNumberFloat64Type, &multiplierValue)
                             ? multiplierValue
                             : 1.0;
        integerValue = convertToInt64((double)integerValue / divisor);
        doubleValue = doubleValue / divisor;
    }
    switch (numberType) {
        case kOCFNumberSInt64Type:
        case kOCFNumberLongType:
        case kOCFNumberLongLongType:
        case kOCFNumberCFIndexType:
            *(int64_t *)valuePtr = integerValue;
            return true;
        case kOCFNumberSInt32Type:
        case kOCFNumberIntType:
            if (integerValue != (int32_t)integerValue) {
                return false;
            }
            *(int32_t *)valuePtr = (int32_t)integerValue;
            return true;
        case kOCFNumberSInt8Type:
        case kOCFNumberCharType:
            if (integerValue != (int8_t)integerValue) {
                return false;
            }
            *(int8_t *)valuePtr = (int8_t)integerValue;
            return true;
        case kOCFNumberSInt16Type:
        case kOCFNumberShortType:
            if (integerValue != (int16_t)integerValue) {
                return false;
            }
            *(int16_t *)valuePtr = (int16_t)integerValue;
            return true;
        case kOCFNumberFloat32Type:
        case kOCFNumberFloatType:
            if (fabs(doubleValue) > 3.4028234663852886e38) {
                return false;
            }
            *(float *)valuePtr = (float)doubleValue;
            return true;
        case kOCFNumberFloat64Type:
        case kOCFNumberDoubleType:
            if (fabs(doubleValue) > 1.7976931348623157e308) {
                return false;
            }
            *(double *)valuePtr = doubleValue;
            return true;
        default:
            return false;
    }
}

// Native CFNumberFormatterSetProperty, 0x1c01afec8. External.
// After the property is set without a refresh, executables linked on or after version 6 refresh the format
// through CFNumberFormatterGetFormat (its result is not used).
void OCFNumberFormatterSetProperty(OCFNumberFormatterRef formatter, OCFStringRef key, OCFTypeRef value) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != 57) {
        _CFAssertMismatchedTypeID(57, typeID);
    }
    _CFNumberFormatterSetPropertyNoRefresh(formatter, key, value);
    if (_OCFExecutableLinkedOnOrAfter(6)) {
        OCFNumberFormatterGetFormat(formatter);
    }
}

// Native _CFNumberFormatterSetPropertyNoRefresh, 0x1c01aff3c. Hidden (was private external).
// For the styles 5, 6, 7 and 9 only the lenient key is handled. Keys are compared by pointer in native order.
// String values are copied (the length clamped to 768 characters) into a 768-character buffer and set as ICU
// text attributes (currency code 5, padding character 4, prefixes and suffixes 0-3) or symbols
// (unum_setSymbol). Booleans compare with kCFBooleanTrue. The zero symbol and the multiplier are retained
// (NULL clears them; setting the multiplier records whether one was given) and the previous value released
// after the store. The rounding increment is read as a double (attribute 12); the integer keys are read as
// 32-bit values and set through _CFNumberFormatterSetInt32PropertyNoRefresh. The default-format key and
// unknown keys do nothing. ICU statuses are not examined.
void _CFNumberFormatterSetPropertyNoRefresh(
    OCFNumberFormatterRef formatter, OCFStringRef key, OCFTypeRef value
) {
    int32_t status = 0;
    OCFIndex style = formatter->style;
    if ((uint64_t)(style - 8) >= (uint64_t)-3 || style == 9) {
        if (kOCFNumberFormatterIsLenientKey != key) {
            return;
        }
    }
    int32_t intValue = 0;
    double doubleValue = 0.0;
    UniChar buffer[768];
    int32_t textAttribute = -1;
    int32_t symbol = -1;
    if (kOCFNumberFormatterCurrencyCodeKey == key) {
        textAttribute = 5;
    } else if (kOCFNumberFormatterDecimalSeparatorKey == key) {
        symbol = 0;
    } else if (kOCFNumberFormatterCurrencyDecimalSeparatorKey == key) {
        symbol = 10;
    } else if (kOCFNumberFormatterAlwaysShowDecimalSeparatorKey == key) {
        unum_setAttribute(formatter->icuFormatter, 2, kOCFBooleanTrue == value);
        return;
    } else if (kOCFNumberFormatterGroupingSeparatorKey == key) {
        symbol = 1;
    } else if (kOCFNumberFormatterUseGroupingSeparatorKey == key) {
        unum_setAttribute(formatter->icuFormatter, 1, kOCFBooleanTrue == value);
        return;
    } else if (kOCFNumberFormatterPercentSymbolKey == key) {
        symbol = 3;
    } else if (kOCFNumberFormatterZeroSymbolKey == key) {
        OCFStringRef previous = formatter->zeroSymbol;
        formatter->zeroSymbol = value != NULL ? OCFRetain(value) : NULL;
        if (previous != NULL) {
            OCFRelease(previous);
        }
        return;
    } else if (kOCFNumberFormatterNaNSymbolKey == key) {
        symbol = 15;
    } else if (kOCFNumberFormatterInfinitySymbolKey == key) {
        symbol = 14;
    } else if (kOCFNumberFormatterMinusSignKey == key) {
        symbol = 6;
    } else if (kOCFNumberFormatterPlusSignKey == key) {
        symbol = 7;
    } else if (kOCFNumberFormatterCurrencySymbolKey == key) {
        symbol = 8;
    } else if (kOCFNumberFormatterExponentSymbolKey == key) {
        symbol = 11;
    } else if (kOCFNumberFormatterPaddingCharacterKey == key) {
        textAttribute = 4;
    } else if (kOCFNumberFormatterDefaultFormatKey == key) {
        return;
    } else if (kOCFNumberFormatterMultiplierKey == key) {
        OCFNumberRef previous = formatter->multiplier;
        formatter->multiplier = value != NULL ? OCFRetain(value) : NULL;
        formatter->userSetMultiplier = value != NULL;
        if (previous != NULL) {
            OCFRelease(previous);
        }
        return;
    } else if (kOCFNumberFormatterPositivePrefixKey == key) {
        textAttribute = 0;
    } else if (kOCFNumberFormatterPositiveSuffixKey == key) {
        textAttribute = 1;
    } else if (kOCFNumberFormatterNegativePrefixKey == key) {
        textAttribute = 2;
    } else if (kOCFNumberFormatterNegativeSuffixKey == key) {
        textAttribute = 3;
    } else if (kOCFNumberFormatterPerMillSymbolKey == key) {
        symbol = 12;
    } else if (kOCFNumberFormatterInternationalCurrencySymbolKey == key) {
        symbol = 9;
    } else if (kOCFNumberFormatterCurrencyGroupingSeparatorKey == key) {
        symbol = 17;
    } else if (kOCFNumberFormatterIsLenientKey == key) {
        Boolean lenient = kOCFBooleanTrue == value;
        formatter->isLenient = lenient;
        unum_setAttribute(formatter->icuFormatter, 19, lenient);
        return;
    } else if (kOCFNumberFormatterUseSignificantDigitsKey == key) {
        unum_setAttribute(formatter->icuFormatter, 16, kOCFBooleanTrue == value);
        return;
    } else if (kOCFNumberFormatterRoundingIncrementKey == key) {
        OCFNumberGetValue(value, kOCFNumberDoubleType, &doubleValue);
        unum_setDoubleAttribute(formatter->icuFormatter, 12, doubleValue);
        return;
    } else if (kOCFNumberFormatterUsesCharacterDirectionKey == key) {
        formatter->usesCharacterDirection = value == kOCFBooleanTrue;
        return;
    } else if (
        kOCFNumberFormatterMinIntegerDigitsKey == key || kOCFNumberFormatterMaxIntegerDigitsKey == key ||
        kOCFNumberFormatterMinFractionDigitsKey == key || kOCFNumberFormatterMaxFractionDigitsKey == key ||
        kOCFNumberFormatterGroupingSizeKey == key || kOCFNumberFormatterSecondaryGroupingSizeKey == key ||
        kOCFNumberFormatterRoundingModeKey == key || kOCFNumberFormatterFormatWidthKey == key ||
        kOCFNumberFormatterPaddingPositionKey == key || kOCFNumberFormatterMinSignificantDigitsKey == key ||
        kOCFNumberFormatterMaxSignificantDigitsKey == key || kOCFNumberFormatterMinGroupingDigitsKey == key
    ) {
        OCFNumberGetValue(value, kOCFNumberSInt32Type, &intValue);
        _CFNumberFormatterSetInt32PropertyNoRefresh(formatter, key, intValue);
        return;
    } else {
        return;
    }
    OCFIndex length = OCFStringGetLength(value);
    OCFIndex count = length < 0x300 ? length : 0x300;
    OCFStringGetCharacters(value, (OCFRange){0, count}, buffer);
    if (textAttribute >= 0) {
        unum_setTextAttribute(formatter->icuFormatter, textAttribute, buffer, (int32_t)count, &status);
    } else {
        unum_setSymbol(formatter->icuFormatter, symbol, buffer, (int32_t)count, &status);
    }
}

// Native _CFNumberFormatterSetInt32PropertyNoRefresh, 0x1c01b0940. Hidden (was private external).
// The formatting-context key (CFEqual with the literal "kCFNumberFormatterFormattingContextKey") sets the ICU
// display context to the value. The integer keys (compared by pointer) set their ICU attributes: minimum and
// maximum integer digits 4 and 3, fraction digits 7 and 6, grouping size 10, secondary grouping size 15,
// rounding mode 11, format width 13, padding position 14, significant digits 17 and 18, and minimum grouping
// digits 22, where -1 (any value at or above 0xfffffffe as unsigned) is passed as -2.
void _CFNumberFormatterSetInt32PropertyNoRefresh(
    OCFNumberFormatterRef formatter, OCFStringRef key, int32_t value
) {
    int32_t status = 0;
    if (OCFEqual(OF_STR("kCFNumberFormatterFormattingContextKey"), key)) {
        unum_setContext(formatter->icuFormatter, value, &status);
        return;
    }
    int32_t attribute;
    if (kOCFNumberFormatterMinIntegerDigitsKey == key) {
        attribute = 4;
    } else if (kOCFNumberFormatterMaxIntegerDigitsKey == key) {
        attribute = 3;
    } else if (kOCFNumberFormatterMinFractionDigitsKey == key) {
        attribute = 7;
    } else if (kOCFNumberFormatterMaxFractionDigitsKey == key) {
        attribute = 6;
    } else if (kOCFNumberFormatterGroupingSizeKey == key) {
        attribute = 10;
    } else if (kOCFNumberFormatterSecondaryGroupingSizeKey == key) {
        attribute = 15;
    } else if (kOCFNumberFormatterRoundingModeKey == key) {
        attribute = 11;
    } else if (kOCFNumberFormatterFormatWidthKey == key) {
        attribute = 13;
    } else if (kOCFNumberFormatterPaddingPositionKey == key) {
        attribute = 14;
    } else if (kOCFNumberFormatterMinSignificantDigitsKey == key) {
        attribute = 17;
    } else if (kOCFNumberFormatterMaxSignificantDigitsKey == key) {
        attribute = 18;
    } else if (kOCFNumberFormatterMinGroupingDigitsKey == key) {
        attribute = 22;
        if ((uint32_t)value >= 0xfffffffeu) {
            value = -2;
        }
    } else {
        return;
    }
    unum_setAttribute(formatter->icuFormatter, attribute, value);
}

// Native CFNumberFormatterCopyProperty, 0x1c01b0c84. External.
// The formatting-context key (CFEqual with "kCFNumberFormatterFormattingContextKey") answers the ICU
// capitalization context (type 1) as a 32-bit CFNumber. For the styles 5, 6, 7 and 9 only the lenient key is
// answered. Keys are compared by pointer in native order. Symbols and text attributes are read into a
// 768-character buffer: NULL when ICU reports an error or more than 768 characters. An empty currency code
// asks a temporary currency-style (2) ICU formatter for the locale (an ASCII identifier, NULL when it does
// not convert); its own status is not examined, and the result needs 1-768 characters. Booleans answer
// retained kCFBooleanTrue or kCFBooleanFalse; the zero symbol, default format and multiplier answer the
// retained field (NULL when absent). Integer attributes answer 32-bit CFNumbers (minimum grouping digits:
// ICU's -2 answers -1); the rounding increment a double CFNumber. Unknown keys answer NULL.
OCFTypeRef OCFNumberFormatterCopyProperty(OCFNumberFormatterRef formatter, OCFStringRef key) {
    int32_t intValue = 0;
    double doubleValue = 0.0;
    int32_t status = 0;
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != 57) {
        _CFAssertMismatchedTypeID(57, typeID);
    }
    if (OCFEqual(OF_STR("kCFNumberFormatterFormattingContextKey"), key)) {
        intValue = unum_getContext(formatter->icuFormatter, 1, &status);
        return OCFNumberCreate(OCFGetAllocator(formatter), kOCFNumberSInt32Type, &intValue);
    }
    OCFIndex style = formatter->style;
    if ((uint64_t)(style - 8) >= (uint64_t)-3 || style == 9) {
        if (kOCFNumberFormatterIsLenientKey != key) {
            return NULL;
        }
    }
    UniChar chars[768];
    int32_t symbol = -1;
    int32_t textAttribute = -1;
    int32_t attribute = -1;
    int32_t booleanAttribute = -1;
    OCFTypeRef field = NULL;
    if (kOCFNumberFormatterCurrencyCodeKey == key) {
        OCFIndex length = unum_getTextAttribute(formatter->icuFormatter, 5, chars, 768, &status);
        if (status <= 0 && length == 0) {
            OCFStringRef identifier = OCFLocaleGetIdentifier(formatter->locale);
            const char *cIdentifier = OCFStringGetCStringPtr(identifier, kOCFStringEncodingASCII);
            char buffer[768];
            if (cIdentifier == NULL) {
                cIdentifier = buffer;
                if (!OCFStringGetCString(identifier, buffer, 768, kOCFStringEncodingASCII)) {
                    return NULL;
                }
            }
            int32_t localStatus = 0;
            void *currencyFormatter = unum_open(2, NULL, 0, cIdentifier, NULL, &localStatus);
            if (currencyFormatter != NULL) {
                length = unum_getTextAttribute(currencyFormatter, 5, chars, 768, &localStatus);
                unum_close(currencyFormatter);
            } else {
                length = 0;
            }
        }
        if (status > 0) {
            return NULL;
        }
        if ((uint64_t)(length - 1) <= 0x2ff) {
            return OCFStringCreateWithCharacters(OCFGetAllocator(formatter), chars, length);
        }
        return NULL;
    } else if (kOCFNumberFormatterDecimalSeparatorKey == key) {
        symbol = 0;
    } else if (kOCFNumberFormatterCurrencyDecimalSeparatorKey == key) {
        symbol = 10;
    } else if (kOCFNumberFormatterAlwaysShowDecimalSeparatorKey == key) {
        booleanAttribute = 2;
    } else if (kOCFNumberFormatterGroupingSeparatorKey == key) {
        symbol = 1;
    } else if (kOCFNumberFormatterUseGroupingSeparatorKey == key) {
        booleanAttribute = 1;
    } else if (kOCFNumberFormatterPercentSymbolKey == key) {
        symbol = 3;
    } else if (kOCFNumberFormatterZeroSymbolKey == key) {
        field = formatter->zeroSymbol;
        return field != NULL ? OCFRetain(field) : NULL;
    } else if (kOCFNumberFormatterNaNSymbolKey == key) {
        symbol = 15;
    } else if (kOCFNumberFormatterInfinitySymbolKey == key) {
        symbol = 14;
    } else if (kOCFNumberFormatterMinusSignKey == key) {
        symbol = 6;
    } else if (kOCFNumberFormatterPlusSignKey == key) {
        symbol = 7;
    } else if (kOCFNumberFormatterCurrencySymbolKey == key) {
        symbol = 8;
    } else if (kOCFNumberFormatterExponentSymbolKey == key) {
        symbol = 11;
    } else if (kOCFNumberFormatterMinIntegerDigitsKey == key) {
        attribute = 4;
    } else if (kOCFNumberFormatterMaxIntegerDigitsKey == key) {
        attribute = 3;
    } else if (kOCFNumberFormatterMinFractionDigitsKey == key) {
        attribute = 7;
    } else if (kOCFNumberFormatterMaxFractionDigitsKey == key) {
        attribute = 6;
    } else if (kOCFNumberFormatterGroupingSizeKey == key) {
        attribute = 10;
    } else if (kOCFNumberFormatterSecondaryGroupingSizeKey == key) {
        attribute = 15;
    } else if (kOCFNumberFormatterRoundingModeKey == key) {
        attribute = 11;
    } else if (kOCFNumberFormatterRoundingIncrementKey == key) {
        doubleValue = unum_getDoubleAttribute(formatter->icuFormatter, 12);
        return OCFNumberCreate(OCFGetAllocator(formatter), kOCFNumberDoubleType, &doubleValue);
    } else if (kOCFNumberFormatterFormatWidthKey == key) {
        attribute = 13;
    } else if (kOCFNumberFormatterPaddingPositionKey == key) {
        attribute = 14;
    } else if (kOCFNumberFormatterPaddingCharacterKey == key) {
        textAttribute = 4;
    } else if (kOCFNumberFormatterDefaultFormatKey == key) {
        field = formatter->defaultFormat;
        return field != NULL ? OCFRetain(field) : NULL;
    } else if (kOCFNumberFormatterMultiplierKey == key) {
        field = formatter->multiplier;
        return field != NULL ? OCFRetain(field) : NULL;
    } else if (kOCFNumberFormatterPositivePrefixKey == key) {
        textAttribute = 0;
    } else if (kOCFNumberFormatterPositiveSuffixKey == key) {
        textAttribute = 1;
    } else if (kOCFNumberFormatterNegativePrefixKey == key) {
        textAttribute = 2;
    } else if (kOCFNumberFormatterNegativeSuffixKey == key) {
        textAttribute = 3;
    } else if (kOCFNumberFormatterPerMillSymbolKey == key) {
        symbol = 12;
    } else if (kOCFNumberFormatterInternationalCurrencySymbolKey == key) {
        symbol = 9;
    } else if (kOCFNumberFormatterCurrencyGroupingSeparatorKey == key) {
        symbol = 17;
    } else if (kOCFNumberFormatterIsLenientKey == key) {
        return OCFRetain(formatter->isLenient == 0 ? kOCFBooleanFalse : kOCFBooleanTrue);
    } else if (kOCFNumberFormatterUseSignificantDigitsKey == key) {
        booleanAttribute = 16;
    } else if (kOCFNumberFormatterMinSignificantDigitsKey == key) {
        attribute = 17;
    } else if (kOCFNumberFormatterMaxSignificantDigitsKey == key) {
        attribute = 18;
    } else if (kOCFNumberFormatterMinGroupingDigitsKey == key) {
        int32_t digits = unum_getAttribute(formatter->icuFormatter, 22);
        intValue = digits != -2 ? digits : -1;
        return OCFNumberCreate(OCFGetAllocator(formatter), kOCFNumberSInt32Type, &intValue);
    } else {
        return NULL;
    }
    if (booleanAttribute >= 0) {
        int32_t enabled = unum_getAttribute(formatter->icuFormatter, booleanAttribute);
        return OCFRetain(enabled == 0 ? kOCFBooleanFalse : kOCFBooleanTrue);
    }
    if (attribute >= 0) {
        intValue = unum_getAttribute(formatter->icuFormatter, attribute);
        return OCFNumberCreate(OCFGetAllocator(formatter), kOCFNumberSInt32Type, &intValue);
    }
    int32_t length;
    if (symbol >= 0) {
        length = unum_getSymbol(formatter->icuFormatter, symbol, chars, 768, &status);
    } else {
        length = unum_getTextAttribute(formatter->icuFormatter, textAttribute, chars, 768, &status);
    }
    if (status > 0 || length > 768) {
        return NULL;
    }
    return OCFStringCreateWithCharacters(OCFGetAllocator(formatter), chars, length);
}

// Native __CFNumberFormatterApplySymbolPrefs, 0x1c01b1590. Non-external; the CFDictionaryApplierFunction
// CFNumberFormatterCreate passes for the AppleICUNumberSymbols dictionary (the context is the formatter).
// For a string key (the symbol number, CFStringGetIntValue) and a string value: an empty value sets the empty
// symbol; otherwise up to 768 characters are copied to a stack buffer when the string has no direct character
// pointer. The length passed to unum_setSymbol is the full length with a direct pointer and the clamped
// length after a copy, as natively. The ICU status starts at 0 and is not examined.
static void __CFNumberFormatterApplySymbolPrefs(const void *key, const void *value, void *context) {
    if (OCFGetTypeID(key) != OCFStringGetTypeID()) {
        return;
    }
    if (OCFGetTypeID(value) != OCFStringGetTypeID()) {
        return;
    }
    int32_t symbol = OCFStringGetIntValue(key);
    int32_t status = 0;
    void *format = ((struct __OCFNumberFormatter *)context)->icuFormatter;
    OCFIndex length = OCFStringGetLength(value);
    if (length < 1) {
        unum_setSymbol(format, symbol, (const UniChar *)u"", 0, &status);
        return;
    }
    OCFIndex clamped = (size_t)length < 0x300 ? length : 0x300;
    UniChar buffer[clamped];
    const UniChar *characters = OCFStringGetCharactersPtr(value);
    if (characters == NULL) {
        OCFStringGetCharacters(value, (OCFRange){0, clamped}, buffer);
        characters = buffer;
        length = clamped;
    }
    unum_setSymbol(format, symbol, characters, (int32_t)length, &status);
}
