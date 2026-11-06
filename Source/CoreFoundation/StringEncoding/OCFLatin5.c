//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFLatin5.h"
#include "CoreFoundation/StringEncoding/OCFSingleByte.h"

// Inferred organization: the ISO Latin5, Mac Turkish, DOS Turkish and Windows Latin5 converters
// (0x1c01dc148-0x1c01dc758) are contiguous in the iOS Simulator __text section and are kept in one
// translation unit with their tables and converter records; __CFIsValidCombiningCharacterForLatin5
// (0x1c01dc2f8), between them, is restored here
// in native order. The native source path is not in the evidence.
// Unless a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified; iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.

// Native ISOLatin5_from_uni, 0x1c0375cac (__TEXT,__const, 6 entries of {Unicode, byte}). Non-external.
static const uint16_t ISOLatin5_from_uni[6][2] = {
    {0x011e, 0x00d0},
    {0x011f, 0x00f0},
    {0x0130, 0x00dd},
    {0x0131, 0x00fd},
    {0x015e, 0x00de},
    {0x015f, 0x00fe},
};

// Native MacTurkish_from_uni, 0x1c0375cd0 (__TEXT,__const, 128 entries of {Unicode, byte}). Non-external.
static const uint16_t MacTurkish_from_uni[128][2] = {
    {0x00a0, 0x00ca}, {0x00a1, 0x00c1}, {0x00a2, 0x00a2}, {0x00a3, 0x00a3}, {0x00a5, 0x00b4},
    {0x00a7, 0x00a4}, {0x00a8, 0x00ac}, {0x00a9, 0x00a9}, {0x00aa, 0x00bb}, {0x00ab, 0x00c7},
    {0x00ac, 0x00c2}, {0x00ae, 0x00a8}, {0x00af, 0x00f8}, {0x00b0, 0x00a1}, {0x00b1, 0x00b1},
    {0x00b4, 0x00ab}, {0x00b5, 0x00b5}, {0x00b6, 0x00a6}, {0x00b7, 0x00e1}, {0x00b8, 0x00fc},
    {0x00ba, 0x00bc}, {0x00bb, 0x00c8}, {0x00bf, 0x00c0}, {0x00c0, 0x00cb}, {0x00c1, 0x00e7},
    {0x00c2, 0x00e5}, {0x00c3, 0x00cc}, {0x00c4, 0x0080}, {0x00c5, 0x0081}, {0x00c6, 0x00ae},
    {0x00c7, 0x0082}, {0x00c8, 0x00e9}, {0x00c9, 0x0083}, {0x00ca, 0x00e6}, {0x00cb, 0x00e8},
    {0x00cc, 0x00ed}, {0x00cd, 0x00ea}, {0x00ce, 0x00eb}, {0x00cf, 0x00ec}, {0x00d1, 0x0084},
    {0x00d2, 0x00f1}, {0x00d3, 0x00ee}, {0x00d4, 0x00ef}, {0x00d5, 0x00cd}, {0x00d6, 0x0085},
    {0x00d8, 0x00af}, {0x00d9, 0x00f4}, {0x00da, 0x00f2}, {0x00db, 0x00f3}, {0x00dc, 0x0086},
    {0x00df, 0x00a7}, {0x00e0, 0x0088}, {0x00e1, 0x0087}, {0x00e2, 0x0089}, {0x00e3, 0x008b},
    {0x00e4, 0x008a}, {0x00e5, 0x008c}, {0x00e6, 0x00be}, {0x00e7, 0x008d}, {0x00e8, 0x008f},
    {0x00e9, 0x008e}, {0x00ea, 0x0090}, {0x00eb, 0x0091}, {0x00ec, 0x0093}, {0x00ed, 0x0092},
    {0x00ee, 0x0094}, {0x00ef, 0x0095}, {0x00f1, 0x0096}, {0x00f2, 0x0098}, {0x00f3, 0x0097},
    {0x00f4, 0x0099}, {0x00f5, 0x009b}, {0x00f6, 0x009a}, {0x00f7, 0x00d6}, {0x00f8, 0x00bf},
    {0x00f9, 0x009d}, {0x00fa, 0x009c}, {0x00fb, 0x009e}, {0x00fc, 0x009f}, {0x00ff, 0x00d8},
    {0x011e, 0x00da}, {0x011f, 0x00db}, {0x0130, 0x00dc}, {0x0131, 0x00dd}, {0x0152, 0x00ce},
    {0x0153, 0x00cf}, {0x015e, 0x00de}, {0x015f, 0x00df}, {0x0178, 0x00d9}, {0x0192, 0x00c4},
    {0x02c6, 0x00f6}, {0x02c7, 0x00ff}, {0x02d8, 0x00f9}, {0x02d9, 0x00fa}, {0x02da, 0x00fb},
    {0x02db, 0x00fe}, {0x02dc, 0x00f7}, {0x02dd, 0x00fd}, {0x03a9, 0x00bd}, {0x03c0, 0x00b9},
    {0x2013, 0x00d0}, {0x2014, 0x00d1}, {0x2018, 0x00d4}, {0x2019, 0x00d5}, {0x201a, 0x00e2},
    {0x201c, 0x00d2}, {0x201d, 0x00d3}, {0x201e, 0x00e3}, {0x2020, 0x00a0}, {0x2021, 0x00e0},
    {0x2022, 0x00a5}, {0x2026, 0x00c9}, {0x2030, 0x00e4}, {0x2122, 0x00aa}, {0x2202, 0x00b6},
    {0x2206, 0x00c6}, {0x220f, 0x00b8}, {0x2211, 0x00b7}, {0x221a, 0x00c3}, {0x221e, 0x00b0},
    {0x222b, 0x00ba}, {0x2248, 0x00c5}, {0x2260, 0x00ad}, {0x2264, 0x00b2}, {0x2265, 0x00b3},
    {0x25ca, 0x00d7}, {0xf8a0, 0x00f5}, {0xf8ff, 0x00f0},
};

// Native MacTurkish_to_uni, 0x1c0375ed0 (__TEXT,__const, 128 UTF-16 units). Non-external.
static const uint16_t MacTurkish_to_uni[128] = {
    0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5,
    0x00e7, 0x00e9, 0x00e8, 0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 0x00f2, 0x00f4,
    0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6,
    0x00df, 0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8, 0x221e, 0x00b1, 0x2264, 0x2265,
    0x00a5, 0x00b5, 0x2202, 0x2211, 0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8, 0x00bf,
    0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5,
    0x0152, 0x0153, 0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 0x00ff, 0x0178, 0x011e,
    0x011f, 0x0130, 0x0131, 0x015e, 0x015f, 0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1,
    0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4, 0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9,
    0xf8a0, 0x02c6, 0x02dc, 0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7,
};

// Native DOSTurkish_from_uni, 0x1c0375fd0 (__TEXT,__const, 125 entries of {Unicode, byte}). Non-external.
static const uint16_t DOSTurkish_from_uni[125][2] = {
    {0x00a0, 0x00ff}, {0x00a1, 0x00ad}, {0x00a2, 0x00bd}, {0x00a3, 0x009c}, {0x00a4, 0x00cf},
    {0x00a5, 0x00be}, {0x00a6, 0x00dd}, {0x00a7, 0x00f5}, {0x00a8, 0x00f9}, {0x00a9, 0x00b8},
    {0x00aa, 0x00d1}, {0x00ab, 0x00ae}, {0x00ac, 0x00aa}, {0x00ad, 0x00f0}, {0x00ae, 0x00a9},
    {0x00af, 0x00ee}, {0x00b0, 0x00f8}, {0x00b1, 0x00f1}, {0x00b2, 0x00fd}, {0x00b3, 0x00fc},
    {0x00b4, 0x00ef}, {0x00b5, 0x00e6}, {0x00b6, 0x00f4}, {0x00b7, 0x00fa}, {0x00b8, 0x00f7},
    {0x00b9, 0x00fb}, {0x00ba, 0x00d0}, {0x00bb, 0x00af}, {0x00bc, 0x00ac}, {0x00bd, 0x00ab},
    {0x00be, 0x00f3}, {0x00bf, 0x00a8}, {0x00c0, 0x00b7}, {0x00c1, 0x00b5}, {0x00c2, 0x00b6},
    {0x00c3, 0x00c7}, {0x00c4, 0x008e}, {0x00c5, 0x008f}, {0x00c6, 0x0092}, {0x00c7, 0x0080},
    {0x00c8, 0x00d4}, {0x00c9, 0x0090}, {0x00ca, 0x00d2}, {0x00cb, 0x00d3}, {0x00cc, 0x00de},
    {0x00cd, 0x00d6}, {0x00ce, 0x00d7}, {0x00cf, 0x00d8}, {0x00d1, 0x00a5}, {0x00d2, 0x00e3},
    {0x00d3, 0x00e0}, {0x00d4, 0x00e2}, {0x00d5, 0x00e5}, {0x00d6, 0x0099}, {0x00d7, 0x00e8},
    {0x00d8, 0x009d}, {0x00d9, 0x00eb}, {0x00da, 0x00e9}, {0x00db, 0x00ea}, {0x00dc, 0x009a},
    {0x00df, 0x00e1}, {0x00e0, 0x0085}, {0x00e1, 0x00a0}, {0x00e2, 0x0083}, {0x00e3, 0x00c6},
    {0x00e4, 0x0084}, {0x00e5, 0x0086}, {0x00e6, 0x0091}, {0x00e7, 0x0087}, {0x00e8, 0x008a},
    {0x00e9, 0x0082}, {0x00ea, 0x0088}, {0x00eb, 0x0089}, {0x00ec, 0x00ec}, {0x00ed, 0x00a1},
    {0x00ee, 0x008c}, {0x00ef, 0x008b}, {0x00f1, 0x00a4}, {0x00f2, 0x0095}, {0x00f3, 0x00a2},
    {0x00f4, 0x0093}, {0x00f5, 0x00e4}, {0x00f6, 0x0094}, {0x00f7, 0x00f6}, {0x00f8, 0x009b},
    {0x00f9, 0x0097}, {0x00fa, 0x00a3}, {0x00fb, 0x0096}, {0x00fc, 0x0081}, {0x00ff, 0x00ed},
    {0x011e, 0x00a6}, {0x011f, 0x00a7}, {0x0130, 0x0098}, {0x0131, 0x008d}, {0x015e, 0x009e},
    {0x015f, 0x009f}, {0x2500, 0x00c4}, {0x2502, 0x00b3}, {0x250c, 0x00da}, {0x2510, 0x00bf},
    {0x2514, 0x00c0}, {0x2518, 0x00d9}, {0x251c, 0x00c3}, {0x2524, 0x00b4}, {0x252c, 0x00c2},
    {0x2534, 0x00c1}, {0x253c, 0x00c5}, {0x2550, 0x00cd}, {0x2551, 0x00ba}, {0x2554, 0x00c9},
    {0x2557, 0x00bb}, {0x255a, 0x00c8}, {0x255d, 0x00bc}, {0x2560, 0x00cc}, {0x2563, 0x00b9},
    {0x2566, 0x00cb}, {0x2569, 0x00ca}, {0x256c, 0x00ce}, {0x2580, 0x00df}, {0x2584, 0x00dc},
    {0x2588, 0x00db}, {0x2591, 0x00b0}, {0x2592, 0x00b1}, {0x2593, 0x00b2}, {0x25a0, 0x00fe},
};

// Native DOSTurkish_to_uni, 0x1c03761c4 (__TEXT,__const, 128 UTF-16 units). Non-external.
static const uint16_t DOSTurkish_to_uni[128] = {
    0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee,
    0x0131, 0x00c4, 0x00c5, 0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x0130, 0x00d6,
    0x00dc, 0x00f8, 0x00a3, 0x00d8, 0x015e, 0x015f, 0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x011e,
    0x011f, 0x00bf, 0x00ae, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb, 0x2591, 0x2592, 0x2593, 0x2502,
    0x2524, 0x00c1, 0x00c2, 0x00c0, 0x00a9, 0x2563, 0x2551, 0x2557, 0x255d, 0x00a2, 0x00a5, 0x2510, 0x2514,
    0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x00e3, 0x00c3, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550,
    0x256c, 0x00a4, 0x00ba, 0x00aa, 0x00ca, 0x00cb, 0x00c8, 0xfffd, 0x00cd, 0x00ce, 0x00cf, 0x2518, 0x250c,
    0x2588, 0x2584, 0x00a6, 0x00cc, 0x2580, 0x00d3, 0x00df, 0x00d4, 0x00d2, 0x00f5, 0x00d5, 0x00b5, 0xfffd,
    0x00d7, 0x00da, 0x00db, 0x00d9, 0x00ec, 0x00ff, 0x00af, 0x00b4, 0x00ad, 0x00b1, 0xfffd, 0x00be, 0x00b6,
    0x00a7, 0x00f7, 0x00b8, 0x00b0, 0x00a8, 0x00b7, 0x00b9, 0x00b3, 0x00b2, 0x25a0, 0x00a0,
};

// Native WindowsLatin5_from_uni, 0x1c03762c4 (__TEXT,__const, 121 entries of {Unicode, byte}). Non-external.
static const uint16_t WindowsLatin5_from_uni[121][2] = {
    {0x00a0, 0x00a0}, {0x00a1, 0x00a1}, {0x00a2, 0x00a2}, {0x00a3, 0x00a3}, {0x00a4, 0x00a4},
    {0x00a5, 0x00a5}, {0x00a6, 0x00a6}, {0x00a7, 0x00a7}, {0x00a8, 0x00a8}, {0x00a9, 0x00a9},
    {0x00aa, 0x00aa}, {0x00ab, 0x00ab}, {0x00ac, 0x00ac}, {0x00ad, 0x00ad}, {0x00ae, 0x00ae},
    {0x00af, 0x00af}, {0x00b0, 0x00b0}, {0x00b1, 0x00b1}, {0x00b2, 0x00b2}, {0x00b3, 0x00b3},
    {0x00b4, 0x00b4}, {0x00b5, 0x00b5}, {0x00b6, 0x00b6}, {0x00b7, 0x00b7}, {0x00b8, 0x00b8},
    {0x00b9, 0x00b9}, {0x00ba, 0x00ba}, {0x00bb, 0x00bb}, {0x00bc, 0x00bc}, {0x00bd, 0x00bd},
    {0x00be, 0x00be}, {0x00bf, 0x00bf}, {0x00c0, 0x00c0}, {0x00c1, 0x00c1}, {0x00c2, 0x00c2},
    {0x00c3, 0x00c3}, {0x00c4, 0x00c4}, {0x00c5, 0x00c5}, {0x00c6, 0x00c6}, {0x00c7, 0x00c7},
    {0x00c8, 0x00c8}, {0x00c9, 0x00c9}, {0x00ca, 0x00ca}, {0x00cb, 0x00cb}, {0x00cc, 0x00cc},
    {0x00cd, 0x00cd}, {0x00ce, 0x00ce}, {0x00cf, 0x00cf}, {0x00d1, 0x00d1}, {0x00d2, 0x00d2},
    {0x00d3, 0x00d3}, {0x00d4, 0x00d4}, {0x00d5, 0x00d5}, {0x00d6, 0x00d6}, {0x00d7, 0x00d7},
    {0x00d8, 0x00d8}, {0x00d9, 0x00d9}, {0x00da, 0x00da}, {0x00db, 0x00db}, {0x00dc, 0x00dc},
    {0x00df, 0x00df}, {0x00e0, 0x00e0}, {0x00e1, 0x00e1}, {0x00e2, 0x00e2}, {0x00e3, 0x00e3},
    {0x00e4, 0x00e4}, {0x00e5, 0x00e5}, {0x00e6, 0x00e6}, {0x00e7, 0x00e7}, {0x00e8, 0x00e8},
    {0x00e9, 0x00e9}, {0x00ea, 0x00ea}, {0x00eb, 0x00eb}, {0x00ec, 0x00ec}, {0x00ed, 0x00ed},
    {0x00ee, 0x00ee}, {0x00ef, 0x00ef}, {0x00f1, 0x00f1}, {0x00f2, 0x00f2}, {0x00f3, 0x00f3},
    {0x00f4, 0x00f4}, {0x00f5, 0x00f5}, {0x00f6, 0x00f6}, {0x00f7, 0x00f7}, {0x00f8, 0x00f8},
    {0x00f9, 0x00f9}, {0x00fa, 0x00fa}, {0x00fb, 0x00fb}, {0x00fc, 0x00fc}, {0x00ff, 0x00ff},
    {0x011e, 0x00d0}, {0x011f, 0x00f0}, {0x0130, 0x00dd}, {0x0131, 0x00fd}, {0x0152, 0x008c},
    {0x0153, 0x009c}, {0x015e, 0x00de}, {0x015f, 0x00fe}, {0x0160, 0x008a}, {0x0161, 0x009a},
    {0x0178, 0x009f}, {0x0192, 0x0083}, {0x02c6, 0x0088}, {0x02dc, 0x0098}, {0x2013, 0x0096},
    {0x2014, 0x0097}, {0x2018, 0x0091}, {0x2019, 0x0092}, {0x201a, 0x0082}, {0x201c, 0x0093},
    {0x201d, 0x0094}, {0x201e, 0x0084}, {0x2020, 0x0086}, {0x2021, 0x0087}, {0x2022, 0x0095},
    {0x2026, 0x0085}, {0x2030, 0x0089}, {0x2039, 0x008b}, {0x203a, 0x009b}, {0x20ac, 0x0080},
    {0x2122, 0x0099},
};

// Native WindowsLatin5_to_uni, 0x1c03764a8 (__TEXT,__const, 128 UTF-16 units). Non-external. The 80 bytes
// that follow it before the next symbol are not read by any function here.
static const uint16_t WindowsLatin5_to_uni[128] = {
    0x20ac, 0xfffd, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152,
    0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122,
    0x0161, 0x203a, 0x0153, 0xfffd, 0xfffd, 0x0178, 0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6,
    0x00a7, 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af, 0x00b0, 0x00b1, 0x00b2, 0x00b3,
    0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf, 0x00c0,
    0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd,
    0x00ce, 0x00cf, 0x011e, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da,
    0x00db, 0x00dc, 0x0130, 0x015e, 0x00df, 0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 0x011f, 0x00f1, 0x00f2, 0x00f3, 0x00f4,
    0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x0131, 0x015f, 0x00ff,
};

// Native __CFToISOLatin5, 0x1c01dc148. Non-external. Units 0xd0-0xff are stored directly unless their bit,
// (unit + 0x30) mod 64, is set in 0x600100006001 (U+00D0, U+00DD, U+00DE, U+00F0, U+00FD, U+00FE);
// U+011E-U+015F are searched first, and a unit there that is not in the table falls back to the same test.
uint32_t __CFToISOLatin5(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0xcf) {
        *byte = (uint8_t)character;
        return 1;
    }
    if (character >= 0x11e) {
        if (character > 0x15f) {
            return 0;
        }
        int low = 0;
        int high = 5;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = ISOLatin5_from_uni[middle][0];
            if (character >= entry) {
                if (character <= entry) {
                    *byte = (uint8_t)ISOLatin5_from_uni[middle][1];
                    return 1;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        } while (low <= high);
    }
    if (character > 0xff) {
        return 0;
    }
    if ((uint8_t)(character + 0x30) <= 0x2e &&
        ((UINT64_C(0x600100006001) >> ((character + 0x30) & 63)) & 1) != 0) {
        return 0;
    }
    *byte = (uint8_t)character;
    return 1;
}

// Native __CFFromISOLatin5, 0x1c01dc1e0. Non-external. Six bytes are remapped; every byte succeeds.
uint32_t __CFFromISOLatin5(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    switch (byte) {
        case 0xd0:
            value = 0x11e;
            break;
        case 0xdd:
            value = 0x130;
            break;
        case 0xde:
            value = 0x15e;
            break;
        case 0xf0:
            value = 0x11f;
            break;
        case 0xfd:
            value = 0x131;
            break;
        case 0xfe:
            value = 0x15f;
            break;
        default:
            break;
    }
    *character = value;
    return 1;
}

// Native __CFToISOLatin5Precompose, 0x1c01dc250. Non-external. The scratch byte is zeroed before
// CFStringEncodingPrecomposeLatinCharacter (NULL used-character count) runs; __CFToISOLatin5 receives the
// precomposed unit in both W0 (as flags) and W1, as in the native register setup.
OCFIndex __CFToISOLatin5Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    uint8_t byte = 0;
    uint16_t precomposed = OCFStringEncodingPrecomposeLatinCharacter(character, numChars, NULL);
    if (__CFToISOLatin5(precomposed, precomposed, &byte) == 0 || byte == 0) {
        return 0;
    }
    if (maxByteLen != 0) {
        *bytes = byte;
    }
    *usedByteLen = 1;
    return 2;
}

// Native __CFLatin5CombiningCharBitmap, 0x1c0375cc4 (__TEXT,__const, 3 32-bit words). Non-external.
static const uint32_t __CFLatin5CombiningCharBitmap[3] = {
    0xf3a80000,
    0x01000080,
    0x00000000,
};

// Native __CFIsValidCombiningCharacterForLatin5, 0x1c01dc2f8. Non-external. U+0300-U+035F test bit 31 - (unit
// mod 32) of word (unit - 0x300) / 32 in __CFLatin5CombiningCharBitmap (LSR by ~unit); other units are not
// combining.
bool __CFIsValidCombiningCharacterForLatin5(uint16_t character) {
    uint32_t offset = (uint32_t)character - 0x300;
    if (offset > 0x5F) {
        return false;
    }
    return ((__CFLatin5CombiningCharBitmap[offset >> 5] >> (~(uint32_t)character & 31)) & 1) != 0;
}

// Native __CFToMacTurkish, 0x1c01dc32c. Non-external.
uint32_t __CFToMacTurkish(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint16_t)(character + 0x700) < 0x7a0) {
        return 0;
    }
    int low = 0;
    int high = 127;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = MacTurkish_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)MacTurkish_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromMacTurkish, 0x1c01dc39c. Non-external. Byte 0xf5 decodes to U+FFFD (and fails) when flag bit
// 9 is set.
uint32_t __CFFromMacTurkish(uint32_t flags, uint8_t byte, uint16_t *character) {
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        if (byte == 0xf5 && (flags & 0x200) != 0) {
            value = 0xfffd;
        } else {
            value = MacTurkish_to_uni[byte & 0x7f];
        }
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToMacTurkishPrecompose, 0x1c01dc3d8. Non-external. CFStringEncodingPrecomposeLatinCharacter is
// called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToMacTurkishPrecompose(
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
        if ((uint16_t)(precomposed + 0x700) < 0x7a0) {
            return 0;
        }
        bool found = false;
        int low = 0;
        int high = 127;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = MacTurkish_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)MacTurkish_from_uni[middle][1];
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

// Native __CFToDOSTurkish, 0x1c01dc498. Non-external.
uint32_t __CFToDOSTurkish(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x25a1 < 0xffffdaffu) {
        return 0;
    }
    int low = 0;
    int high = 124;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = DOSTurkish_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)DOSTurkish_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromDOSTurkish, 0x1c01dc510. Non-external.
uint32_t __CFFromDOSTurkish(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = DOSTurkish_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToDOSTurkishPrecompose, 0x1c01dc538. Non-external. CFStringEncodingPrecomposeLatinCharacter is
// called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToDOSTurkishPrecompose(
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
        if ((uint16_t)(precomposed - 0x25a1) < 0xdaff) {
            return 0;
        }
        bool found = false;
        int low = 0;
        int high = 124;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = DOSTurkish_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)DOSTurkish_from_uni[middle][1];
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

// Native __CFToWindowsLatin5, 0x1c01dc5f8. Non-external.
uint32_t __CFToWindowsLatin5(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x2123 < 0xffffdf7du) {
        return 0;
    }
    int low = 0;
    int high = 120;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = WindowsLatin5_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)WindowsLatin5_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromWindowsLatin5, 0x1c01dc670. Non-external.
uint32_t __CFFromWindowsLatin5(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = WindowsLatin5_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToWindowsLatin5Precompose, 0x1c01dc698. Non-external. CFStringEncodingPrecomposeLatinCharacter
// is called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToWindowsLatin5Precompose(
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
        int high = 120;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = WindowsLatin5_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)WindowsLatin5_from_uni[middle][1];
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

// Native __CFConverterISOLatin5, 0x2227a3c50 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterISOLatin5 = {
    .toBytes.cheapEightBit = __CFToISOLatin5,
    .toUnicode.cheapEightBit = __CFFromISOLatin5,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToISOLatin5Precompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatin5,
};

// Native __CFConverterMacTurkish, 0x2227a3c98 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterMacTurkish = {
    .toBytes.cheapEightBit = __CFToMacTurkish,
    .toUnicode.cheapEightBit = __CFFromMacTurkish,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToMacTurkishPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatin5,
};

// Native __CFConverterDOSTurkish, 0x2227a3ce0 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterDOSTurkish = {
    .toBytes.cheapEightBit = __CFToDOSTurkish,
    .toUnicode.cheapEightBit = __CFFromDOSTurkish,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToDOSTurkishPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatin5,
};

// Native __CFConverterWindowsLatin5, 0x2227a3d28 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterWindowsLatin5 = {
    .toBytes.cheapEightBit = __CFToWindowsLatin5,
    .toUnicode.cheapEightBit = __CFFromWindowsLatin5,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToWindowsLatin5Precompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatin5,
};
