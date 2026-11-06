//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFVietnamese.h"
#include "CoreFoundation/StringEncoding/OCFSingleByte.h"

// Inferred organization: the Windows Vietnamese converters (0x1c0160e7c-0x1c0160fdc) are contiguous in the
// iOS Simulator __text section and are kept in one translation unit with their tables and converter record;
// __CFIsValidCombiningCharacterForVietnamese (0x1c0160fdc), which follows them, is restored
// here with its bitmap, before the record.
// The native source path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.

// Native WindowsVietnamese_from_uni, 0x1c0372760 (__TEXT,__const, 119 entries of {Unicode, byte}).
// Non-external.
static const uint16_t WindowsVietnamese_from_uni[119][2] = {
    {0x00a0, 0x00a0}, {0x00a1, 0x00a1}, {0x00a2, 0x00a2}, {0x00a3, 0x00a3}, {0x00a4, 0x00a4},
    {0x00a5, 0x00a5}, {0x00a6, 0x00a6}, {0x00a7, 0x00a7}, {0x00a8, 0x00a8}, {0x00a9, 0x00a9},
    {0x00aa, 0x00aa}, {0x00ab, 0x00ab}, {0x00ac, 0x00ac}, {0x00ad, 0x00ad}, {0x00ae, 0x00ae},
    {0x00af, 0x00af}, {0x00b0, 0x00b0}, {0x00b1, 0x00b1}, {0x00b2, 0x00b2}, {0x00b3, 0x00b3},
    {0x00b4, 0x00b4}, {0x00b5, 0x00b5}, {0x00b6, 0x00b6}, {0x00b7, 0x00b7}, {0x00b8, 0x00b8},
    {0x00b9, 0x00b9}, {0x00ba, 0x00ba}, {0x00bb, 0x00bb}, {0x00bc, 0x00bc}, {0x00bd, 0x00bd},
    {0x00be, 0x00be}, {0x00bf, 0x00bf}, {0x00c0, 0x00c0}, {0x00c1, 0x00c1}, {0x00c2, 0x00c2},
    {0x00c4, 0x00c4}, {0x00c5, 0x00c5}, {0x00c6, 0x00c6}, {0x00c7, 0x00c7}, {0x00c8, 0x00c8},
    {0x00c9, 0x00c9}, {0x00ca, 0x00ca}, {0x00cb, 0x00cb}, {0x00cd, 0x00cd}, {0x00ce, 0x00ce},
    {0x00cf, 0x00cf}, {0x00d1, 0x00d1}, {0x00d3, 0x00d3}, {0x00d4, 0x00d4}, {0x00d6, 0x00d6},
    {0x00d7, 0x00d7}, {0x00d8, 0x00d8}, {0x00d9, 0x00d9}, {0x00da, 0x00da}, {0x00db, 0x00db},
    {0x00dc, 0x00dc}, {0x00df, 0x00df}, {0x00e0, 0x00e0}, {0x00e1, 0x00e1}, {0x00e2, 0x00e2},
    {0x00e4, 0x00e4}, {0x00e5, 0x00e5}, {0x00e6, 0x00e6}, {0x00e7, 0x00e7}, {0x00e8, 0x00e8},
    {0x00e9, 0x00e9}, {0x00ea, 0x00ea}, {0x00eb, 0x00eb}, {0x00ed, 0x00ed}, {0x00ee, 0x00ee},
    {0x00ef, 0x00ef}, {0x00f1, 0x00f1}, {0x00f3, 0x00f3}, {0x00f4, 0x00f4}, {0x00f6, 0x00f6},
    {0x00f7, 0x00f7}, {0x00f8, 0x00f8}, {0x00f9, 0x00f9}, {0x00fa, 0x00fa}, {0x00fb, 0x00fb},
    {0x00fc, 0x00fc}, {0x00ff, 0x00ff}, {0x0102, 0x00c3}, {0x0103, 0x00e3}, {0x0110, 0x00d0},
    {0x0111, 0x00f0}, {0x0152, 0x008c}, {0x0153, 0x009c}, {0x0178, 0x009f}, {0x0192, 0x0083},
    {0x01a0, 0x00d5}, {0x01a1, 0x00f5}, {0x01af, 0x00dd}, {0x01b0, 0x00fd}, {0x02c6, 0x0088},
    {0x02dc, 0x0098}, {0x0300, 0x00cc}, {0x0301, 0x00ec}, {0x0303, 0x00de}, {0x0309, 0x00d2},
    {0x0323, 0x00f2}, {0x2013, 0x0096}, {0x2014, 0x0097}, {0x2018, 0x0091}, {0x2019, 0x0092},
    {0x201a, 0x0082}, {0x201c, 0x0093}, {0x201d, 0x0094}, {0x201e, 0x0084}, {0x2020, 0x0086},
    {0x2021, 0x0087}, {0x2022, 0x0095}, {0x2026, 0x0085}, {0x2030, 0x0089}, {0x2039, 0x008b},
    {0x203a, 0x009b}, {0x20ab, 0x00fe}, {0x20ac, 0x0080}, {0x2122, 0x0099},
};

// Native WindowsVietnamese_to_uni, 0x1c037293c (__TEXT,__const, 128 UTF-16 units). Non-external.
static const uint16_t WindowsVietnamese_to_uni[128] = {
    0x20ac, 0xfffd, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0xfffd, 0x2039, 0x0152,
    0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122,
    0xfffd, 0x203a, 0x0153, 0xfffd, 0xfffd, 0x0178, 0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6,
    0x00a7, 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af, 0x00b0, 0x00b1, 0x00b2, 0x00b3,
    0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf, 0x00c0,
    0x00c1, 0x00c2, 0x0102, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x0300, 0x00cd,
    0x00ce, 0x00cf, 0x0110, 0x00d1, 0x0309, 0x00d3, 0x00d4, 0x01a0, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da,
    0x00db, 0x00dc, 0x01af, 0x0303, 0x00df, 0x00e0, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x0301, 0x00ed, 0x00ee, 0x00ef, 0x0111, 0x00f1, 0x0323, 0x00f3, 0x00f4,
    0x01a1, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x01b0, 0x20ab, 0x00ff,
};

// Native __CFToWindowsVietnamese, 0x1c0160e7c. Non-external.
uint32_t __CFToWindowsVietnamese(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x2123 < 0xffffdf7du) {
        return 0;
    }
    int low = 0;
    int high = 118;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = WindowsVietnamese_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)WindowsVietnamese_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromWindowsVietnamese, 0x1c0160ef4. Non-external.
uint32_t __CFFromWindowsVietnamese(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = WindowsVietnamese_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToWindowsVietnamesePrecompose, 0x1c0160f1c. Non-external.
// CFStringEncodingPrecomposeLatinCharacter is called with a NULL used-character count and two characters are
// reported consumed.
OCFIndex __CFToWindowsVietnamesePrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    uint16_t precomposed = OCFStringEncodingPrecomposeLatinCharacter(character, numChars, NULL);
    uint8_t byte = 0;
    if (precomposed <= 0x7f) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed - 0x2123) < 0xdf7d) {
            return 0;
        }
        bool found = false;
        int low = 0;
        int high = 118;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = WindowsVietnamese_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)WindowsVietnamese_from_uni[middle][1];
                    found = true;
                    break;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        } while (low <= high);
        if (!found) {
            return 0;
        }
    }
    if (byte == 0) {
        return 0;
    }
    if (maxByteLen != 0) {
        *bytes = byte;
    }
    *usedByteLen = 1;
    return 2;
}

// Native __CFVietnameseCombiningCharBitmap, 0x1c0372a3c (__TEXT,__const, 3 32-bit words). Non-external.
static const uint32_t __CFVietnameseCombiningCharBitmap[3] = {
    0xf2a00010,
    0x01000000,
    0x00000000,
};

// Native __CFIsValidCombiningCharacterForVietnamese, 0x1c0160fdc. Non-external. U+0300-U+035F test bit 31 -
// (unit mod 32) of word (unit - 0x300) / 32 in __CFVietnameseCombiningCharBitmap (LSR by ~unit); other units
// are not combining.
bool __CFIsValidCombiningCharacterForVietnamese(uint16_t character) {
    uint32_t offset = (uint32_t)character - 0x300;
    if (offset > 0x5F) {
        return false;
    }
    return ((__CFVietnameseCombiningCharBitmap[offset >> 5] >> (~(uint32_t)character & 31)) & 1) != 0;
}

// Native __CFConverterWindowsVietnamese, 0x22279c8a8 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterWindowsVietnamese = {
    .toBytes.cheapEightBit = __CFToWindowsVietnamese,
    .toUnicode.cheapEightBit = __CFFromWindowsVietnamese,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 1,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToWindowsVietnamesePrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForVietnamese,
};
