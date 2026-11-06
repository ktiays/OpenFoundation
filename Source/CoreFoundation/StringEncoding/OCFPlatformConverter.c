//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFArabic.h"
#include "CoreFoundation/StringEncoding/OCFCyrillic.h"
#include "CoreFoundation/StringEncoding/OCFGreek.h"
#include "CoreFoundation/StringEncoding/OCFHebrew.h"
#include "CoreFoundation/StringEncoding/OCFJapanese.h"
#include "CoreFoundation/StringEncoding/OCFKorean.h"
#include "CoreFoundation/StringEncoding/OCFLatin2.h"
#include "CoreFoundation/StringEncoding/OCFLatin5.h"
#include "CoreFoundation/StringEncoding/OCFPlatformConverter.h"
#include "CoreFoundation/StringEncoding/OCFSimplifiedChinese.h"
#include "CoreFoundation/StringEncoding/OCFSingleByte.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingDatabase.h"
#include "CoreFoundation/StringEncoding/OCFSymbol.h"
#include "CoreFoundation/StringEncoding/OCFThai.h"
#include "CoreFoundation/StringEncoding/OCFTraditionalChinese.h"
#include "CoreFoundation/StringEncoding/OCFVietnamese.h"

#include <os/lock.h>

// Inferred organization: __CFStringEncodingConverterGetUncommonConverter (0x1c01cda44),
// CFStringEncodingDecomposeCharacter (0x1c01ceb4c), __CFStringEncodingGetExternalConverter (0x1c01cebc8) and
// the platform converter functions (0x1c01cecc4-0x1c01cece4) lie in one iOS Simulator __text region,
// interleaved with the per-thread data functions (0x1c01cea74-0x1c01ceaf8) of another track. The native
// source path is not in the evidence. The definitions added with the uncommon-converter switch: iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
//
// None of the three platform bodies reads an argument. The parameter lists follow the argument registers
// their only callers set up (CFStringEncodingUnicodeToBytes passes X0-X7,
// CFStringEncodingBytesToUnicode passes X0-X7, CFStringEncodingCharLengthForBytes tail-calls with its
// own arguments); their names and types beyond the register count are descriptive, not read.

// Native __CFICUBootstrap, 0x1c0375988 (__TEXT,__const, 72 bytes). Non-external. Every pointer slot is NULL;
// maxBytesPerChar 6, maxDecomposedCharLen 4, class 5 (ICU). __CFGetConverter replaces the to-bytes slot of
// its result with the ICU name.
static const StringEncodingConverterRecord __CFICUBootstrap = {
    .maxBytesPerChar = 6,
    .maxDecomposedCharLen = 4,
    .encodingClass = 5,
};

// Native __CFStringEncodingConverterGetUncommonConverter, 0x1c01cda44. Hidden (was private external).
// A compare tree and a 14-entry byte jump table (cases 0x202-0x20f; 0x203, 0x204 and 0x20c return NULL) over
// the 58 encodings below; every other value returns NULL. The case list was produced by evaluating the
// compare tree for every value below 0x10000 (Local/Scratch/cf-core-encodings/registry/uncommon.py); all
// comparisons are 32-bit equality or signed range tests, so no other value matches.
const StringEncodingConverterRecord *__CFStringEncodingConverterGetUncommonConverter(uint32_t encoding) {
    switch (encoding) {
        case 0x1:
            return &__CFConverterMacJapanese;
        case 0x2:
            return &__CFConverterMacTraditionalChinese;
        case 0x3:
            return &__CFConverterMacKorean;
        case 0x6:
            return &__CFConverterMacGreek;
        case 0x7:
            return &__CFConverterMacCyrillic;
        case 0x15:
            return &__CFConverterMacThai;
        case 0x19:
            return &__CFConverterMacSimplifiedChinese;
        case 0x1d:
            return &__CFConverterMacCentEuro;
        case 0x21:
            return &__CFConverterMacSymbol;
        case 0x22:
            return &__CFConverterMacDingbats;
        case 0x23:
            return &__CFConverterMacTurkish;
        case 0x24:
            return &__CFConverterMacCroatian;
        case 0x25:
            return &__CFConverterMacIcelandic;
        case 0x26:
            return &__CFConverterMacRomanian;
        case 0x98:
            return &__CFConverterMacCyrillic;
        case 0x202:
            return &__CFConverterISOLatin2;
        case 0x205:
            return &__CFConverterISOCyrillic;
        case 0x206:
            return &__CFConverterISOLatinArabic;
        case 0x207:
            return &__CFConverterISOLatinGreek;
        case 0x208:
            return &__CFConverterISOLatinHebrew;
        case 0x209:
            return &__CFConverterISOLatin5;
        case 0x20a:
            return &__CFConverterISOLatin6;
        case 0x20b:
            return &__CFConverterISOLatinThai;
        case 0x20d:
            return &__CFConverterISOLatin7;
        case 0x20e:
            return &__CFConverterISOLatin8;
        case 0x20f:
            return &__CFConverterISOLatin9;
        case 0x400:
            return &__CFConverterDOSLatinUS;
        case 0x405:
            return &__CFConverterDOSGreek;
        case 0x406:
            return &__CFConverterDOSBalticRim;
        case 0x410:
            return &__CFConverterDOSLatin1;
        case 0x412:
            return &__CFConverterDOSLatin2;
        case 0x414:
            return &__CFConverterDOSTurkish;
        case 0x416:
            return &__CFConverterDOSIcelandic;
        case 0x41b:
            return &__CFConverterDOSRussian;
        case 0x41d:
            return &__CFConverterDOSThai;
        case 0x420:
            return &__CFConverterDOSJapanese;
        case 0x422:
            return &__CFConverterDOSKorean;
        case 0x501:
            return &__CFConverterWinLatin2;
        case 0x502:
            return &__CFConverterWinCyrillic;
        case 0x503:
            return &__CFConverterWindowsGreek;
        case 0x504:
            return &__CFConverterWindowsLatin5;
        case 0x505:
            return &__CFConverterWindowsHebrew;
        case 0x506:
            return &__CFConverterWindowsArabic;
        case 0x507:
            return &__CFConverterWindowsBalticRim;
        case 0x508:
            return &__CFConverterWindowsVietnamese;
        case 0x820:
            return &__CFConverterISO_2022_JP;
        case 0x840:
            return &__CFConverterISO_2022_KR;
        case 0x920:
            return &__CFConverterEUC_JP;
        case 0x930:
            return &__CFConverterEUC_CN;
        case 0x940:
            return &__CFConverterEUC_KR;
        case 0xa01:
            return &__CFConverterShiftJIS;
        case 0xa02:
            return &__CFConverterKOI8R;
        case 0xa03:
            return &__CFConverterBig5;
        case 0xa04:
            return &__CFConverterMacRomanLatin1;
        case 0xa05:
            return &__CFConverterGB_HZ;
        case 0xbf0:
            return &__CFConverterShiftJIS_NTT_Docomo;
        case 0xbf1:
            return &__CFConverterShiftJIS_KDDI;
        case 0xbf2:
            return &__CFConverterShiftJIS_SoftBank;
        case 0xbf4:
            return &__CFConverterISO_2022_JP_KDDI;
        default:
            return NULL;
    }
}

// Native CFStringEncodingDecomposeCharacter, 0x1c01ceb4c. Hidden (was private external).
// One unit through CFUniCharDecompose: no consumed count, capacity 10, no reordering, UTF-16 output (format
// 0), HFS from flags bit 7. Returns the filled length.
OCFIndex CFStringEncodingDecomposeCharacter(uint32_t flags, uint16_t character, uint16_t *characters) {
    OCFIndex usedLength = 0;
    OCFUniCharDecompose(&character, 1, NULL, characters, 10, &usedLength, false, 0, ((flags >> 7) & 1) != 0);
    return usedLength;
}

// Native __CFStringEncodingGetExternalConverter, 0x1c01cebc8. Hidden (was private external).
// Function-local statics (native __CFStringEncodingGetExternalConverter.mappingTable and .mappingTableLock,
// __DATA,__bss): a dictionary created on first use with NULL callbacks, keyed by the zero-extended encoding,
// under an unfair lock. A missing or NULL entry is computed again: __CFICUBootstrap when
// __CFStringEncodingGetICUName has a name, otherwise NULL, and stored.
const StringEncodingConverterRecord *__CFStringEncodingGetExternalConverter(uint32_t encoding) {
    static struct __OCFDictionary *mappingTable = NULL;
    static os_unfair_lock mappingTableLock = OS_UNFAIR_LOCK_INIT;
    os_unfair_lock_lock(&mappingTableLock);
    const StringEncodingConverterRecord *converter = NULL;
    if (mappingTable != NULL) {
        converter = OCFDictionaryGetValue(mappingTable, (const void *)(uintptr_t)encoding);
    }
    if (converter == NULL) {
        converter = __CFStringEncodingGetICUName(encoding) == NULL ? NULL : &__CFICUBootstrap;
        if (mappingTable == NULL) {
            mappingTable = OCFDictionaryCreateMutable(NULL, 0, NULL, NULL);
        }
        OCFDictionarySetValue(mappingTable, (const void *)(uintptr_t)encoding, converter);
    }
    os_unfair_lock_unlock(&mappingTableLock);
    return converter;
}

// Native __CFStringEncodingPlatformUnicodeToBytes, iOS Simulator 24A434, 0x1c01cecc4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (body); parameter types inferred. Hidden
// (was private external). iOS Device 24A446: 0x18074ef20, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; identical instructions. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFStringEncodingPlatformUnicodeToBytes(
    uint32_t encoding,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    OCFIndex *usedCharLen,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)encoding;
    (void)flags;
    (void)characters;
    (void)numChars;
    (void)usedCharLen;
    (void)bytes;
    (void)maxByteLen;
    (void)usedByteLen;
    return 3;
}

// Native __CFStringEncodingPlatformBytesToUnicode, iOS Simulator 24A434, 0x1c01ceccc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (body); parameter types inferred. Hidden
// (was private external). iOS Device 24A446: 0x18074ef28, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; identical instructions. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFStringEncodingPlatformBytesToUnicode(
    uint32_t encoding,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    OCFIndex *usedByteLen,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
) {
    (void)encoding;
    (void)flags;
    (void)bytes;
    (void)numBytes;
    (void)usedByteLen;
    (void)characters;
    (void)maxCharLen;
    (void)usedCharLen;
    return 3;
}

// Native __CFStringEncodingPlatformCharLengthForBytes, iOS Simulator 24A434, 0x1c01cecd4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (body); parameter types inferred. Hidden
// (was private external). iOS Device 24A446: 0x18074ef30, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; identical instructions. The body clears all of X0 (MOV X0, #0), so the result is a
// 64-bit length. Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFStringEncodingPlatformCharLengthForBytes(
    uint32_t encoding, uint32_t flags, const uint8_t *bytes, OCFIndex numBytes
) {
    (void)encoding;
    (void)flags;
    (void)bytes;
    (void)numBytes;
    return 0;
}

// Native __CFStringEncodingPlatformByteLengthForCharacters, 0x1c01cecdc. Hidden (was private external).
// MOV X0, #0; the parameters follow CFStringEncodingByteLengthForCharacters' tail call.
OCFIndex __CFStringEncodingPlatformByteLengthForCharacters(
    uint32_t encoding, uint32_t flags, const uint16_t *characters, OCFIndex numChars
) {
    (void)encoding;
    (void)flags;
    (void)characters;
    (void)numChars;
    return 0;
}
