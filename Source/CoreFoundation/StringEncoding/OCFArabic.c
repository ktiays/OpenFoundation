//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFArabic.h"
#include "CoreFoundation/StringEncoding/OCFSingleByte.h"

// Inferred organization: the ISO and Windows Arabic converters (0x1c0156d5c-0x1c01571e8) are contiguous in
// the iOS Simulator __text section and are kept in one translation unit with their tables and converter
// records; __CFIsValidCombiningCharacterForArabic (0x1c0156fbc), between them, is
// restored here in native order. The
// native source path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.

// Native cp1256_from_uni, 0x1c0321708 (__TEXT,__const, 120 entries of {Unicode, byte}). Non-external.
static const uint16_t cp1256_from_uni[120][2] = {
    {0x00a0, 0x00a0}, {0x00a2, 0x00a2}, {0x00a3, 0x00a3}, {0x00a4, 0x00a4}, {0x00a5, 0x00a5},
    {0x00a6, 0x00a6}, {0x00a7, 0x00a7}, {0x00a8, 0x00a8}, {0x00a9, 0x00a9}, {0x00ab, 0x00ab},
    {0x00ac, 0x00ac}, {0x00ad, 0x00ad}, {0x00ae, 0x00ae}, {0x00af, 0x00af}, {0x00b0, 0x00b0},
    {0x00b1, 0x00b1}, {0x00b2, 0x00b2}, {0x00b3, 0x00b3}, {0x00b4, 0x00b4}, {0x00b5, 0x00b5},
    {0x00b6, 0x00b6}, {0x00b7, 0x00b7}, {0x00b8, 0x00b8}, {0x00b9, 0x00b9}, {0x00bb, 0x00bb},
    {0x00bc, 0x00bc}, {0x00bd, 0x00bd}, {0x00be, 0x00be}, {0x00d7, 0x00d7}, {0x00e0, 0x00e0},
    {0x00e2, 0x00e2}, {0x00e7, 0x00e7}, {0x00e8, 0x00e8}, {0x00e9, 0x00e9}, {0x00ea, 0x00ea},
    {0x00eb, 0x00eb}, {0x00ee, 0x00ee}, {0x00ef, 0x00ef}, {0x00f4, 0x00f4}, {0x00f7, 0x00f7},
    {0x00f9, 0x00f9}, {0x00fb, 0x00fb}, {0x00fc, 0x00fc}, {0x0152, 0x008c}, {0x0153, 0x009c},
    {0x0192, 0x0083}, {0x02c6, 0x0088}, {0x060c, 0x00a1}, {0x061b, 0x00ba}, {0x061f, 0x00bf},
    {0x0621, 0x00c1}, {0x0622, 0x00c2}, {0x0623, 0x00c3}, {0x0624, 0x00c4}, {0x0625, 0x00c5},
    {0x0626, 0x00c6}, {0x0627, 0x00c7}, {0x0628, 0x00c8}, {0x0629, 0x00c9}, {0x062a, 0x00ca},
    {0x062b, 0x00cb}, {0x062c, 0x00cc}, {0x062d, 0x00cd}, {0x062e, 0x00ce}, {0x062f, 0x00cf},
    {0x0630, 0x00d0}, {0x0631, 0x00d1}, {0x0632, 0x00d2}, {0x0633, 0x00d3}, {0x0634, 0x00d4},
    {0x0635, 0x00d5}, {0x0636, 0x00d6}, {0x0637, 0x00d8}, {0x0638, 0x00d9}, {0x0639, 0x00da},
    {0x063a, 0x00db}, {0x0640, 0x00dc}, {0x0641, 0x00dd}, {0x0642, 0x00de}, {0x0643, 0x00df},
    {0x0644, 0x00e1}, {0x0645, 0x00e3}, {0x0646, 0x00e4}, {0x0647, 0x00e5}, {0x0648, 0x00e6},
    {0x0649, 0x00ec}, {0x064a, 0x00ed}, {0x064b, 0x00f0}, {0x064c, 0x00f1}, {0x064d, 0x00f2},
    {0x064e, 0x00f3}, {0x064f, 0x00f5}, {0x0650, 0x00f6}, {0x0651, 0x00f8}, {0x0652, 0x00fa},
    {0x067e, 0x0081}, {0x0686, 0x008d}, {0x0698, 0x008e}, {0x06af, 0x0090}, {0x200c, 0x009d},
    {0x200d, 0x009e}, {0x200e, 0x00fd}, {0x200f, 0x00fe}, {0x2013, 0x0096}, {0x2014, 0x0097},
    {0x2018, 0x0091}, {0x2019, 0x0092}, {0x201a, 0x0082}, {0x201c, 0x0093}, {0x201d, 0x0094},
    {0x201e, 0x0084}, {0x2020, 0x0086}, {0x2021, 0x0087}, {0x2022, 0x0095}, {0x2026, 0x0085},
    {0x2030, 0x0089}, {0x2039, 0x008b}, {0x203a, 0x009b}, {0x20ac, 0x0080}, {0x2122, 0x0099},
};

// Native cp1256_to_uni, 0x1c03218e8 (__TEXT,__const, 128 UTF-16 units). Non-external.
static const uint16_t cp1256_to_uni[128] = {
    0x20ac, 0x067e, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0xfffd, 0x2039, 0x0152,
    0x0686, 0x0698, 0xfffd, 0x06af, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0xfffd, 0x2122,
    0xfffd, 0x203a, 0x0153, 0x200c, 0x200d, 0xfffd, 0x00a0, 0x060c, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6,
    0x00a7, 0x00a8, 0x00a9, 0xfffd, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af, 0x00b0, 0x00b1, 0x00b2, 0x00b3,
    0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x061b, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x061f, 0xfffd,
    0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, 0x0628, 0x0629, 0x062a, 0x062b, 0x062c, 0x062d,
    0x062e, 0x062f, 0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x00d7, 0x0637, 0x0638, 0x0639,
    0x063a, 0x0640, 0x0641, 0x0642, 0x0643, 0x00e0, 0x0644, 0x00e2, 0x0645, 0x0646, 0x0647, 0x0648, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x0649, 0x064a, 0x00ee, 0x00ef, 0x064b, 0x064c, 0x064d, 0x064e, 0x00f4,
    0x064f, 0x0650, 0x00f7, 0x0651, 0x00f9, 0x0652, 0x00fb, 0x00fc, 0x200e, 0x200f, 0xfffd,
};

// Native __CFToISOArabic, 0x1c0156d5c. Non-external. Units 0x63b-0x63f inside 0x621-0x652 fail; the rest map
// to unit - 0x60.
uint32_t __CFToISOArabic(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character < 0xa1) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x621 <= 0x31) {
        if ((uint8_t)(character - 0x3b) < 5) {
            return 0;
        }
        *byte = (uint8_t)(character - 0x60);
        return 1;
    }
    uint8_t value;
    if (character <= 0x60b) {
        if (character != 0xa4 && character != 0xad) {
            return 0;
        }
        value = (uint8_t)character;
    } else if (character == 0x60c) {
        value = 0xac;
    } else if (character == 0x61b) {
        value = 0xbb;
    } else if (character == 0x61f) {
        value = 0xbf;
    } else {
        return 0;
    }
    *byte = value;
    return 1;
}

// Native __CFFromISOArabic, 0x1c0156dec. Non-external. Bytes 0xdb-0xdf inside 0xc1-0xf2 fail; the rest map to
// byte + 0x560.
uint32_t __CFFromISOArabic(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    if (byte < 0xa1) {
        *character = byte;
        return 1;
    }
    uint16_t value;
    if (byte >= 0xc1) {
        if (byte > 0xf2 || (uint32_t)byte - 0xdb < 5) {
            return 0;
        }
        value = (uint16_t)(byte + 0x560);
    } else if (byte <= 0xac) {
        if (byte == 0xa4) {
            value = byte;
        } else if (byte == 0xac) {
            value = 0x60c;
        } else {
            return 0;
        }
    } else if (byte == 0xad) {
        value = byte;
    } else if (byte == 0xbb) {
        value = 0x61b;
    } else if (byte == 0xbf) {
        value = 0x61f;
    } else {
        return 0;
    }
    *character = value;
    return 1;
}

// Native __CFToISOArabicPrecompose, 0x1c0156e74. Non-external. The second unit is read without checking
// numChars. A pair that does not compose yields U+FFFD, which __CFToISOArabic rejects; flags (X0) reach
// __CFToISOArabic unchanged. The scratch byte is zeroed before the call.
OCFIndex __CFToISOArabicPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)numChars;
    uint16_t precomposed = 0xfffd;
    uint16_t combining = character[1];
    if (combining == 0x655) {
        if (character[0] == 0x627) {
            precomposed = 0x625;
        }
    } else if (combining == 0x654) {
        switch (character[0]) {
            case 0x627:
                precomposed = 0x623;
                break;
            case 0x648:
                precomposed = 0x624;
                break;
            case 0x64a:
                precomposed = 0x626;
                break;
            case 0x6c1:
                precomposed = 0x6c2;
                break;
            case 0x6d2:
                precomposed = 0x6d3;
                break;
            case 0x6d5:
                precomposed = 0x6c0;
                break;
            default:
                break;
        }
    } else if (combining == 0x653) {
        if (character[0] == 0x627) {
            precomposed = 0x622;
        }
    }
    uint8_t byte = 0;
    if (__CFToISOArabic(flags, precomposed, &byte) == 0 || byte == 0) {
        return 0;
    }
    if (maxByteLen != 0) {
        *bytes = byte;
    }
    *usedByteLen = 1;
    return 2;
}

// Native __CFIsValidCombiningCharacterForArabic, 0x1c0156fbc. Non-external. U+0653-U+0655 are combining;
// any other unit is answered by CFStringEncodingIsValidCombiningCharacterForLatin1 (a tail call).
bool __CFIsValidCombiningCharacterForArabic(uint16_t character) {
    if ((uint32_t)character - 0x653 < 3) {
        return true;
    }
    return OCFStringEncodingIsValidCombiningCharacterForLatin1(character);
}

// Native __CFToWinArabic, 0x1c0156fd4. Non-external.
uint32_t __CFToWinArabic(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x2123 < 0xffffdf7du) {
        return 0;
    }
    int low = 0;
    int high = 119;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = cp1256_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)cp1256_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromWinArabic, 0x1c015704c. Non-external.
uint32_t __CFFromWinArabic(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = cp1256_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToWinArabicPrecompose, 0x1c0157074. Non-external. A first unit below U+0080 goes through
// CFStringEncodingPrecomposeLatinCharacter (NULL used-character count); otherwise the second unit, read
// without checking numChars, selects an Arabic composition and any other pair fails. The composed Arabic unit
// is searched without the range gate.
OCFIndex __CFToWinArabicPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    uint16_t first = character[0];
    uint16_t value;
    bool searchTable = true;
    if (first < 0x80) {
        value = OCFStringEncodingPrecomposeLatinCharacter(character, numChars, NULL);
        if (value <= 0x7f) {
            searchTable = false;
        } else if ((uint16_t)(value - 0x2123) < 0xdf7d) {
            return 0;
        }
    } else {
        uint16_t combining = character[1];
        if (combining == 0x655) {
            if (first != 0x627) {
                return 0;
            }
            value = 0x625;
        } else if (combining == 0x654) {
            switch (first) {
                case 0x627:
                    value = 0x623;
                    break;
                case 0x648:
                    value = 0x624;
                    break;
                case 0x64a:
                    value = 0x626;
                    break;
                case 0x6c1:
                    value = 0x6c2;
                    break;
                case 0x6d2:
                    value = 0x6d3;
                    break;
                case 0x6d5:
                    value = 0x6c0;
                    break;
                default:
                    return 0;
            }
        } else if (combining == 0x653) {
            if (first != 0x627) {
                return 0;
            }
            value = 0x622;
        } else {
            return 0;
        }
    }
    if (searchTable) {
        bool found = false;
        int low = 0;
        int high = 119;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = cp1256_from_uni[middle][0];
            if (value >= entry) {
                if (value <= entry) {
                    value = cp1256_from_uni[middle][1];
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
    if ((value & 0xff) == 0) {
        return 0;
    }
    if (maxByteLen != 0) {
        *bytes = (uint8_t)value;
    }
    *usedByteLen = 1;
    return 2;
}

// Native __CFConverterISOLatinArabic, 0x222429828 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterISOLatinArabic = {
    .toBytes.cheapEightBit = __CFToISOArabic,
    .toUnicode.cheapEightBit = __CFFromISOArabic,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToISOArabicPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForArabic,
};

// Native __CFConverterWindowsArabic, 0x222429870 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterWindowsArabic = {
    .toBytes.cheapEightBit = __CFToWinArabic,
    .toUnicode.cheapEightBit = __CFFromWinArabic,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 1,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToWinArabicPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForArabic,
};
