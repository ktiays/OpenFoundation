//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFLatin2.h"

#include <stddef.h>

// Inferred organization: the Central European converters (ISO Latin2, DOS Latin2, Windows Latin2, Mac Central
// European, Mac Croatian and Mac Romanian, 0x1c02f2584-0x1c02f2f10) are contiguous in the iOS Simulator
// __text section and are kept in one translation unit with their tables (0x1c046e2a8-0x1c046f2a0,
// __TEXT,__const) and converter records (0x2227ab750-0x2227ab900, __DATA_CONST,__const). The native source
// path is not in the evidence. __CFIsValidCombiningCharacterForLatin2 (0x1c02f26d4), which lies in the same
// range, and its bitmap __CFLatin2CombiningCharBitmap (0x1c046e4e8) are restored here in native order. Unless
// a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified; iOS Device 24A446: unread. Every converter ignores its flags. The from-Unicode tables hold (unit,
// byte) pairs sorted by unit; each search reads the byte as the low byte of the second halfword (LDRB at
// offset 2). Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.

// Native latin2_from_uni, 0x1c046e2a8 (__TEXT,__const, 96 four-byte entries). Non-external.
static const uint16_t latin2_from_uni[96][2] = {
    {0x00a0, 0x00a0}, {0x00a4, 0x00a4}, {0x00a7, 0x00a7}, {0x00a8, 0x00a8}, {0x00ad, 0x00ad},
    {0x00b0, 0x00b0}, {0x00b4, 0x00b4}, {0x00b8, 0x00b8}, {0x00c1, 0x00c1}, {0x00c2, 0x00c2},
    {0x00c4, 0x00c4}, {0x00c7, 0x00c7}, {0x00c9, 0x00c9}, {0x00cb, 0x00cb}, {0x00cd, 0x00cd},
    {0x00ce, 0x00ce}, {0x00d3, 0x00d3}, {0x00d4, 0x00d4}, {0x00d6, 0x00d6}, {0x00d7, 0x00d7},
    {0x00da, 0x00da}, {0x00dc, 0x00dc}, {0x00dd, 0x00dd}, {0x00df, 0x00df}, {0x00e1, 0x00e1},
    {0x00e2, 0x00e2}, {0x00e4, 0x00e4}, {0x00e7, 0x00e7}, {0x00e9, 0x00e9}, {0x00eb, 0x00eb},
    {0x00ed, 0x00ed}, {0x00ee, 0x00ee}, {0x00f3, 0x00f3}, {0x00f4, 0x00f4}, {0x00f6, 0x00f6},
    {0x00f7, 0x00f7}, {0x00fa, 0x00fa}, {0x00fc, 0x00fc}, {0x00fd, 0x00fd}, {0x0102, 0x00c3},
    {0x0103, 0x00e3}, {0x0104, 0x00a1}, {0x0105, 0x00b1}, {0x0106, 0x00c6}, {0x0107, 0x00e6},
    {0x010c, 0x00c8}, {0x010d, 0x00e8}, {0x010e, 0x00cf}, {0x010f, 0x00ef}, {0x0110, 0x00d0},
    {0x0111, 0x00f0}, {0x0118, 0x00ca}, {0x0119, 0x00ea}, {0x011a, 0x00cc}, {0x011b, 0x00ec},
    {0x0139, 0x00c5}, {0x013a, 0x00e5}, {0x013d, 0x00a5}, {0x013e, 0x00b5}, {0x0141, 0x00a3},
    {0x0142, 0x00b3}, {0x0143, 0x00d1}, {0x0144, 0x00f1}, {0x0147, 0x00d2}, {0x0148, 0x00f2},
    {0x0150, 0x00d5}, {0x0151, 0x00f5}, {0x0154, 0x00c0}, {0x0155, 0x00e0}, {0x0158, 0x00d8},
    {0x0159, 0x00f8}, {0x015a, 0x00a6}, {0x015b, 0x00b6}, {0x015e, 0x00aa}, {0x015f, 0x00ba},
    {0x0160, 0x00a9}, {0x0161, 0x00b9}, {0x0162, 0x00de}, {0x0163, 0x00fe}, {0x0164, 0x00ab},
    {0x0165, 0x00bb}, {0x016e, 0x00d9}, {0x016f, 0x00f9}, {0x0170, 0x00db}, {0x0171, 0x00fb},
    {0x0179, 0x00ac}, {0x017a, 0x00bc}, {0x017b, 0x00af}, {0x017c, 0x00bf}, {0x017d, 0x00ae},
    {0x017e, 0x00be}, {0x02c7, 0x00b7}, {0x02d8, 0x00a2}, {0x02d9, 0x00ff}, {0x02db, 0x00b2},
    {0x02dd, 0x00bd},
};

// The 96 entries map bytes 0xa0-0xff.
// Native latin2_to_uni, 0x1c046e428 (__TEXT,__const, 96 uint16 entries). Non-external.
static const uint16_t latin2_to_uni[96] = {
    0x00a0, 0x0104, 0x02d8, 0x0141, 0x00a4, 0x013d, 0x015a, 0x00a7, 0x00a8, 0x0160, 0x015e, 0x0164,
    0x0179, 0x00ad, 0x017d, 0x017b, 0x00b0, 0x0105, 0x02db, 0x0142, 0x00b4, 0x013e, 0x015b, 0x02c7,
    0x00b8, 0x0161, 0x015f, 0x0165, 0x017a, 0x02dd, 0x017e, 0x017c, 0x0154, 0x00c1, 0x00c2, 0x0102,
    0x00c4, 0x0139, 0x0106, 0x00c7, 0x010c, 0x00c9, 0x0118, 0x00cb, 0x011a, 0x00cd, 0x00ce, 0x010e,
    0x0110, 0x0143, 0x0147, 0x00d3, 0x00d4, 0x0150, 0x00d6, 0x00d7, 0x0158, 0x016e, 0x00da, 0x0170,
    0x00dc, 0x00dd, 0x0162, 0x00df, 0x0155, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x013a, 0x0107, 0x00e7,
    0x010d, 0x00e9, 0x0119, 0x00eb, 0x011b, 0x00ed, 0x00ee, 0x010f, 0x0111, 0x0144, 0x0148, 0x00f3,
    0x00f4, 0x0151, 0x00f6, 0x00f7, 0x0159, 0x016f, 0x00fa, 0x0171, 0x00fc, 0x00fd, 0x0163, 0x02d9,
};

// Native __CFLatin2CombiningCharBitmap, 0x1c046e4e8 (__TEXT,__const, three 32-bit words, read with
// Local/Scratch/cf-core/macho.py). Non-external. Bit 31 - (unit & 31) of word (unit - 0x300) >> 5 marks a
// combining unit the Latin 2 converters accept.
static const uint32_t __CFLatin2CombiningCharBitmap[3] = {0xFBB80000, 0x01800080, 0x00000000};

// Native cp852_from_uni, 0x1c046e4f4 (__TEXT,__const, 128 four-byte entries). Non-external.
static const uint16_t cp852_from_uni[128][2] = {
    {0x00a0, 0x00ff}, {0x00a4, 0x00cf}, {0x00a7, 0x00f5}, {0x00a8, 0x00f9}, {0x00ab, 0x00ae},
    {0x00ac, 0x00aa}, {0x00ad, 0x00f0}, {0x00b0, 0x00f8}, {0x00b4, 0x00ef}, {0x00b8, 0x00f7},
    {0x00bb, 0x00af}, {0x00c1, 0x00b5}, {0x00c2, 0x00b6}, {0x00c4, 0x008e}, {0x00c7, 0x0080},
    {0x00c9, 0x0090}, {0x00cb, 0x00d3}, {0x00cd, 0x00d6}, {0x00ce, 0x00d7}, {0x00d3, 0x00e0},
    {0x00d4, 0x00e2}, {0x00d6, 0x0099}, {0x00d7, 0x009e}, {0x00da, 0x00e9}, {0x00dc, 0x009a},
    {0x00dd, 0x00ed}, {0x00df, 0x00e1}, {0x00e1, 0x00a0}, {0x00e2, 0x0083}, {0x00e4, 0x0084},
    {0x00e7, 0x0087}, {0x00e9, 0x0082}, {0x00eb, 0x0089}, {0x00ed, 0x00a1}, {0x00ee, 0x008c},
    {0x00f3, 0x00a2}, {0x00f4, 0x0093}, {0x00f6, 0x0094}, {0x00f7, 0x00f6}, {0x00fa, 0x00a3},
    {0x00fc, 0x0081}, {0x00fd, 0x00ec}, {0x0102, 0x00c6}, {0x0103, 0x00c7}, {0x0104, 0x00a4},
    {0x0105, 0x00a5}, {0x0106, 0x008f}, {0x0107, 0x0086}, {0x010c, 0x00ac}, {0x010d, 0x009f},
    {0x010e, 0x00d2}, {0x010f, 0x00d4}, {0x0110, 0x00d1}, {0x0111, 0x00d0}, {0x0118, 0x00a8},
    {0x0119, 0x00a9}, {0x011a, 0x00b7}, {0x011b, 0x00d8}, {0x0139, 0x0091}, {0x013a, 0x0092},
    {0x013d, 0x0095}, {0x013e, 0x0096}, {0x0141, 0x009d}, {0x0142, 0x0088}, {0x0143, 0x00e3},
    {0x0144, 0x00e4}, {0x0147, 0x00d5}, {0x0148, 0x00e5}, {0x0150, 0x008a}, {0x0151, 0x008b},
    {0x0154, 0x00e8}, {0x0155, 0x00ea}, {0x0158, 0x00fc}, {0x0159, 0x00fd}, {0x015a, 0x0097},
    {0x015b, 0x0098}, {0x015e, 0x00b8}, {0x015f, 0x00ad}, {0x0160, 0x00e6}, {0x0161, 0x00e7},
    {0x0162, 0x00dd}, {0x0163, 0x00ee}, {0x0164, 0x009b}, {0x0165, 0x009c}, {0x016e, 0x00de},
    {0x016f, 0x0085}, {0x0170, 0x00eb}, {0x0171, 0x00fb}, {0x0179, 0x008d}, {0x017a, 0x00ab},
    {0x017b, 0x00bd}, {0x017c, 0x00be}, {0x017d, 0x00a6}, {0x017e, 0x00a7}, {0x02c7, 0x00f3},
    {0x02d8, 0x00f4}, {0x02d9, 0x00fa}, {0x02db, 0x00f2}, {0x02dd, 0x00f1}, {0x2500, 0x00c4},
    {0x2502, 0x00b3}, {0x250c, 0x00da}, {0x2510, 0x00bf}, {0x2514, 0x00c0}, {0x2518, 0x00d9},
    {0x251c, 0x00c3}, {0x2524, 0x00b4}, {0x252c, 0x00c2}, {0x2534, 0x00c1}, {0x253c, 0x00c5},
    {0x2550, 0x00cd}, {0x2551, 0x00ba}, {0x2554, 0x00c9}, {0x2557, 0x00bb}, {0x255a, 0x00c8},
    {0x255d, 0x00bc}, {0x2560, 0x00cc}, {0x2563, 0x00b9}, {0x2566, 0x00cb}, {0x2569, 0x00ca},
    {0x256c, 0x00ce}, {0x2580, 0x00df}, {0x2584, 0x00dc}, {0x2588, 0x00db}, {0x2591, 0x00b0},
    {0x2592, 0x00b1}, {0x2593, 0x00b2}, {0x25a0, 0x00fe},
};

// Native cp852_to_uni, 0x1c046e6f4 (__TEXT,__const, 128 uint16 entries). Non-external.
static const uint16_t cp852_to_uni[128] = {
    0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x016f, 0x0107, 0x00e7, 0x0142, 0x00eb, 0x0150, 0x0151, 0x00ee,
    0x0179, 0x00c4, 0x0106, 0x00c9, 0x0139, 0x013a, 0x00f4, 0x00f6, 0x013d, 0x013e, 0x015a, 0x015b, 0x00d6,
    0x00dc, 0x0164, 0x0165, 0x0141, 0x00d7, 0x010d, 0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x0104, 0x0105, 0x017d,
    0x017e, 0x0118, 0x0119, 0x00ac, 0x017a, 0x010c, 0x015f, 0x00ab, 0x00bb, 0x2591, 0x2592, 0x2593, 0x2502,
    0x2524, 0x00c1, 0x00c2, 0x011a, 0x015e, 0x2563, 0x2551, 0x2557, 0x255d, 0x017b, 0x017c, 0x2510, 0x2514,
    0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x0102, 0x0103, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550,
    0x256c, 0x00a4, 0x0111, 0x0110, 0x010e, 0x00cb, 0x010f, 0x0147, 0x00cd, 0x00ce, 0x011b, 0x2518, 0x250c,
    0x2588, 0x2584, 0x0162, 0x016e, 0x2580, 0x00d3, 0x00df, 0x00d4, 0x0143, 0x0144, 0x0148, 0x0160, 0x0161,
    0x0154, 0x00da, 0x0155, 0x0170, 0x00fd, 0x00dd, 0x0163, 0x00b4, 0x00ad, 0x02dd, 0x02db, 0x02c7, 0x02d8,
    0x00a7, 0x00f7, 0x00b8, 0x00b0, 0x00a8, 0x02d9, 0x0171, 0x0158, 0x0159, 0x25a0, 0x00a0,
};

// Native cp1250_from_uni, 0x1c046e7f4 (__TEXT,__const, 42 four-byte entries). Non-external.
static const uint16_t cp1250_from_uni[42][2] = {
    {0x00a6, 0x00a6}, {0x00a9, 0x00a9}, {0x00ab, 0x00ab}, {0x00ac, 0x00ac}, {0x00ae, 0x00ae},
    {0x00b1, 0x00b1}, {0x00b5, 0x00b5}, {0x00b6, 0x00b6}, {0x00b7, 0x00b7}, {0x00bb, 0x00bb},
    {0x0104, 0x00a5}, {0x0105, 0x00b9}, {0x013d, 0x00bc}, {0x013e, 0x00be}, {0x015a, 0x008c},
    {0x015b, 0x009c}, {0x0160, 0x008a}, {0x0161, 0x009a}, {0x0164, 0x008d}, {0x0165, 0x009d},
    {0x0179, 0x008f}, {0x017a, 0x009f}, {0x017d, 0x008e}, {0x017e, 0x009e}, {0x02c7, 0x00a1},
    {0x2013, 0x0096}, {0x2014, 0x0097}, {0x2018, 0x0091}, {0x2019, 0x0092}, {0x201a, 0x0082},
    {0x201c, 0x0093}, {0x201d, 0x0094}, {0x201e, 0x0084}, {0x2020, 0x0086}, {0x2021, 0x0087},
    {0x2022, 0x0095}, {0x2026, 0x0085}, {0x2030, 0x0089}, {0x2039, 0x008b}, {0x203a, 0x009b},
    {0x20ac, 0x0080}, {0x2122, 0x0099},
};

// Native cp1250_to_uni, 0x1c046e89c (__TEXT,__const, 128 uint16 entries). Non-external.
static const uint16_t cp1250_to_uni[128] = {
    0x20ac, 0xfffd, 0x201a, 0xfffd, 0x201e, 0x2026, 0x2020, 0x2021, 0xfffd, 0x2030, 0x0160, 0x2039, 0x015a,
    0x0164, 0x017d, 0x0179, 0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0xfffd, 0x2122,
    0x0161, 0x203a, 0x015b, 0x0165, 0x017e, 0x017a, 0x00a0, 0x02c7, 0x02d8, 0x0141, 0x00a4, 0x0104, 0x00a6,
    0x00a7, 0x00a8, 0x00a9, 0x015e, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x017b, 0x00b0, 0x00b1, 0x02db, 0x0142,
    0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x0105, 0x015f, 0x00bb, 0x013d, 0x02dd, 0x013e, 0x017c, 0x0154,
    0x00c1, 0x00c2, 0x0102, 0x00c4, 0x0139, 0x0106, 0x00c7, 0x010c, 0x00c9, 0x0118, 0x00cb, 0x011a, 0x00cd,
    0x00ce, 0x010e, 0x0110, 0x0143, 0x0147, 0x00d3, 0x00d4, 0x0150, 0x00d6, 0x00d7, 0x0158, 0x016e, 0x00da,
    0x0170, 0x00dc, 0x00dd, 0x0162, 0x00df, 0x0155, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x013a, 0x0107, 0x00e7,
    0x010d, 0x00e9, 0x0119, 0x00eb, 0x011b, 0x00ed, 0x00ee, 0x010f, 0x0111, 0x0144, 0x0148, 0x00f3, 0x00f4,
    0x0151, 0x00f6, 0x00f7, 0x0159, 0x016f, 0x00fa, 0x0171, 0x00fc, 0x00fd, 0x0163, 0x02d9,
};

// Native maccenteuro_from_uni, 0x1c046e99c (__TEXT,__const, 128 four-byte entries). Non-external.
static const uint16_t maccenteuro_from_uni[128][2] = {
    {0x00a0, 0x00ca}, {0x00a3, 0x00a3}, {0x00a7, 0x00a4}, {0x00a8, 0x00ac}, {0x00a9, 0x00a9},
    {0x00ab, 0x00c7}, {0x00ac, 0x00c2}, {0x00ae, 0x00a8}, {0x00b0, 0x00a1}, {0x00b6, 0x00a6},
    {0x00bb, 0x00c8}, {0x00c1, 0x00e7}, {0x00c4, 0x0080}, {0x00c9, 0x0083}, {0x00cd, 0x00ea},
    {0x00d3, 0x00ee}, {0x00d4, 0x00ef}, {0x00d5, 0x00cd}, {0x00d6, 0x0085}, {0x00da, 0x00f2},
    {0x00dc, 0x0086}, {0x00dd, 0x00f8}, {0x00df, 0x00a7}, {0x00e1, 0x0087}, {0x00e4, 0x008a},
    {0x00e9, 0x008e}, {0x00ed, 0x0092}, {0x00f3, 0x0097}, {0x00f4, 0x0099}, {0x00f5, 0x009b},
    {0x00f6, 0x009a}, {0x00f7, 0x00d6}, {0x00fa, 0x009c}, {0x00fc, 0x009f}, {0x00fd, 0x00f9},
    {0x0100, 0x0081}, {0x0101, 0x0082}, {0x0104, 0x0084}, {0x0105, 0x0088}, {0x0106, 0x008c},
    {0x0107, 0x008d}, {0x010c, 0x0089}, {0x010d, 0x008b}, {0x010e, 0x0091}, {0x010f, 0x0093},
    {0x0112, 0x0094}, {0x0113, 0x0095}, {0x0116, 0x0096}, {0x0117, 0x0098}, {0x0118, 0x00a2},
    {0x0119, 0x00ab}, {0x011a, 0x009d}, {0x011b, 0x009e}, {0x0122, 0x00fe}, {0x0123, 0x00ae},
    {0x012a, 0x00b1}, {0x012b, 0x00b4}, {0x012e, 0x00af}, {0x012f, 0x00b0}, {0x0136, 0x00b5},
    {0x0137, 0x00fa}, {0x0139, 0x00bd}, {0x013a, 0x00be}, {0x013b, 0x00b9}, {0x013c, 0x00ba},
    {0x013d, 0x00bb}, {0x013e, 0x00bc}, {0x0141, 0x00fc}, {0x0142, 0x00b8}, {0x0143, 0x00c1},
    {0x0144, 0x00c4}, {0x0145, 0x00bf}, {0x0146, 0x00c0}, {0x0147, 0x00c5}, {0x0148, 0x00cb},
    {0x014c, 0x00cf}, {0x014d, 0x00d8}, {0x0150, 0x00cc}, {0x0151, 0x00ce}, {0x0154, 0x00d9},
    {0x0155, 0x00da}, {0x0156, 0x00df}, {0x0157, 0x00e0}, {0x0158, 0x00db}, {0x0159, 0x00de},
    {0x015a, 0x00e5}, {0x015b, 0x00e6}, {0x0160, 0x00e1}, {0x0161, 0x00e4}, {0x0164, 0x00e8},
    {0x0165, 0x00e9}, {0x016a, 0x00ed}, {0x016b, 0x00f0}, {0x016e, 0x00f1}, {0x016f, 0x00f3},
    {0x0170, 0x00f4}, {0x0171, 0x00f5}, {0x0172, 0x00f6}, {0x0173, 0x00f7}, {0x0179, 0x008f},
    {0x017a, 0x0090}, {0x017b, 0x00fb}, {0x017c, 0x00fd}, {0x017d, 0x00eb}, {0x017e, 0x00ec},
    {0x02c7, 0x00ff}, {0x2013, 0x00d0}, {0x2014, 0x00d1}, {0x2018, 0x00d4}, {0x2019, 0x00d5},
    {0x201a, 0x00e2}, {0x201c, 0x00d2}, {0x201d, 0x00d3}, {0x201e, 0x00e3}, {0x2020, 0x00a0},
    {0x2022, 0x00a5}, {0x2026, 0x00c9}, {0x2039, 0x00dc}, {0x203a, 0x00dd}, {0x2122, 0x00aa},
    {0x2202, 0x00b6}, {0x2206, 0x00c6}, {0x2211, 0x00b7}, {0x221a, 0x00c3}, {0x2260, 0x00ad},
    {0x2264, 0x00b2}, {0x2265, 0x00b3}, {0x25ca, 0x00d7},
};

// Native maccenteuro_to_uni, 0x1c046eb9c (__TEXT,__const, 128 uint16 entries). Non-external.
static const uint16_t maccenteuro_to_uni[128] = {
    0x00c4, 0x0100, 0x0101, 0x00c9, 0x0104, 0x00d6, 0x00dc, 0x00e1, 0x0105, 0x010c, 0x00e4, 0x010d, 0x0106,
    0x0107, 0x00e9, 0x0179, 0x017a, 0x010e, 0x00ed, 0x010f, 0x0112, 0x0113, 0x0116, 0x00f3, 0x0117, 0x00f4,
    0x00f6, 0x00f5, 0x00fa, 0x011a, 0x011b, 0x00fc, 0x2020, 0x00b0, 0x0118, 0x00a3, 0x00a7, 0x2022, 0x00b6,
    0x00df, 0x00ae, 0x00a9, 0x2122, 0x0119, 0x00a8, 0x2260, 0x0123, 0x012e, 0x012f, 0x012a, 0x2264, 0x2265,
    0x012b, 0x0136, 0x2202, 0x2211, 0x0142, 0x013b, 0x013c, 0x013d, 0x013e, 0x0139, 0x013a, 0x0145, 0x0146,
    0x0143, 0x00ac, 0x221a, 0x0144, 0x0147, 0x2206, 0x00ab, 0x00bb, 0x2026, 0x00a0, 0x0148, 0x0150, 0x00d5,
    0x0151, 0x014c, 0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 0x014d, 0x0154, 0x0155,
    0x0158, 0x2039, 0x203a, 0x0159, 0x0156, 0x0157, 0x0160, 0x201a, 0x201e, 0x0161, 0x015a, 0x015b, 0x00c1,
    0x0164, 0x0165, 0x00cd, 0x017d, 0x017e, 0x016a, 0x00d3, 0x00d4, 0x016b, 0x016e, 0x00da, 0x016f, 0x0170,
    0x0171, 0x0172, 0x0173, 0x00dd, 0x00fd, 0x0137, 0x017b, 0x0141, 0x017c, 0x0122, 0x02c7,
};

// Native maccroatian_from_uni, 0x1c046ec9c (__TEXT,__const, 128 four-byte entries). Non-external.
static const uint16_t maccroatian_from_uni[128][2] = {
    {0x00a0, 0x00ca}, {0x00a1, 0x00c1}, {0x00a2, 0x00a2}, {0x00a3, 0x00a3}, {0x00a7, 0x00a4},
    {0x00a8, 0x00ac}, {0x00a9, 0x00d9}, {0x00aa, 0x00bb}, {0x00ab, 0x00c7}, {0x00ac, 0x00c2},
    {0x00ae, 0x00a8}, {0x00af, 0x00f8}, {0x00b0, 0x00a1}, {0x00b1, 0x00b1}, {0x00b4, 0x00ab},
    {0x00b5, 0x00b5}, {0x00b6, 0x00a6}, {0x00b7, 0x00e1}, {0x00b8, 0x00fc}, {0x00ba, 0x00bc},
    {0x00bb, 0x00df}, {0x00bf, 0x00c0}, {0x00c0, 0x00cb}, {0x00c1, 0x00e7}, {0x00c2, 0x00e5},
    {0x00c3, 0x00cc}, {0x00c4, 0x0080}, {0x00c5, 0x0081}, {0x00c6, 0x00de}, {0x00c7, 0x0082},
    {0x00c8, 0x00e9}, {0x00c9, 0x0083}, {0x00ca, 0x00fd}, {0x00cb, 0x00fa}, {0x00cc, 0x00ed},
    {0x00cd, 0x00ea}, {0x00ce, 0x00eb}, {0x00cf, 0x00ec}, {0x00d1, 0x0084}, {0x00d2, 0x00f1},
    {0x00d3, 0x00ee}, {0x00d4, 0x00ef}, {0x00d5, 0x00cd}, {0x00d6, 0x0085}, {0x00d8, 0x00af},
    {0x00d9, 0x00f4}, {0x00da, 0x00f2}, {0x00db, 0x00f3}, {0x00dc, 0x0086}, {0x00df, 0x00a7},
    {0x00e0, 0x0088}, {0x00e1, 0x0087}, {0x00e2, 0x0089}, {0x00e3, 0x008b}, {0x00e4, 0x008a},
    {0x00e5, 0x008c}, {0x00e6, 0x00fe}, {0x00e7, 0x008d}, {0x00e8, 0x008f}, {0x00e9, 0x008e},
    {0x00ea, 0x0090}, {0x00eb, 0x0091}, {0x00ec, 0x0093}, {0x00ed, 0x0092}, {0x00ee, 0x0094},
    {0x00ef, 0x0095}, {0x00f1, 0x0096}, {0x00f2, 0x0098}, {0x00f3, 0x0097}, {0x00f4, 0x0099},
    {0x00f5, 0x009b}, {0x00f6, 0x009a}, {0x00f7, 0x00d6}, {0x00f8, 0x00bf}, {0x00f9, 0x009d},
    {0x00fa, 0x009c}, {0x00fb, 0x009e}, {0x00fc, 0x009f}, {0x0106, 0x00c6}, {0x0107, 0x00e6},
    {0x010c, 0x00c8}, {0x010d, 0x00e8}, {0x0110, 0x00d0}, {0x0111, 0x00f0}, {0x0131, 0x00f5},
    {0x0152, 0x00ce}, {0x0153, 0x00cf}, {0x0160, 0x00a9}, {0x0161, 0x00b9}, {0x017d, 0x00ae},
    {0x017e, 0x00be}, {0x0192, 0x00c4}, {0x02c6, 0x00f6}, {0x02c7, 0x00ff}, {0x02da, 0x00fb},
    {0x02dc, 0x00f7}, {0x03a9, 0x00bd}, {0x03c0, 0x00f9}, {0x2013, 0x00e0}, {0x2014, 0x00d1},
    {0x2018, 0x00d4}, {0x2019, 0x00d5}, {0x201a, 0x00e2}, {0x201c, 0x00d2}, {0x201d, 0x00d3},
    {0x201e, 0x00e3}, {0x2020, 0x00a0}, {0x2022, 0x00a5}, {0x2026, 0x00c9}, {0x2030, 0x00e4},
    {0x2039, 0x00dc}, {0x203a, 0x00dd}, {0x2044, 0x00da}, {0x20ac, 0x00db}, {0x2122, 0x00aa},
    {0x2202, 0x00b6}, {0x2206, 0x00b4}, {0x220f, 0x00b8}, {0x2211, 0x00b7}, {0x221a, 0x00c3},
    {0x221e, 0x00b0}, {0x222b, 0x00ba}, {0x2248, 0x00c5}, {0x2260, 0x00ad}, {0x2264, 0x00b2},
    {0x2265, 0x00b3}, {0x25ca, 0x00d7}, {0xf8ff, 0x00d8},
};

// Native maccroatian_to_uni, 0x1c046ee9c (__TEXT,__const, 128 uint16 entries). Non-external.
static const uint16_t maccroatian_to_uni[128] = {
    0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5,
    0x00e7, 0x00e9, 0x00e8, 0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 0x00f2, 0x00f4,
    0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6,
    0x00df, 0x00ae, 0x0160, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x017d, 0x00d8, 0x221e, 0x00b1, 0x2264, 0x2265,
    0x2206, 0x00b5, 0x2202, 0x2211, 0x220f, 0x0161, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x017e, 0x00f8, 0x00bf,
    0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x0106, 0x00ab, 0x010c, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5,
    0x0152, 0x0153, 0x0110, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 0xf8ff, 0x00a9, 0x2044,
    0x20ac, 0x2039, 0x203a, 0x00c6, 0x00bb, 0x2013, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x0107, 0x00c1,
    0x010d, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4, 0x0111, 0x00d2, 0x00da, 0x00db, 0x00d9,
    0x0131, 0x02c6, 0x02dc, 0x00af, 0x03c0, 0x00cb, 0x02da, 0x00b8, 0x00ca, 0x00e6, 0x02c7,
};

// Native macromanian_from_uni, 0x1c046ef9c (__TEXT,__const, 128 four-byte entries). Non-external.
static const uint16_t macromanian_from_uni[128][2] = {
    {0x00a0, 0x00ca}, {0x00a1, 0x00c1}, {0x00a2, 0x00a2}, {0x00a3, 0x00a3}, {0x00a5, 0x00b4},
    {0x00a7, 0x00a4}, {0x00a8, 0x00ac}, {0x00a9, 0x00a9}, {0x00aa, 0x00bb}, {0x00ab, 0x00c7},
    {0x00ac, 0x00c2}, {0x00ae, 0x00a8}, {0x00af, 0x00f8}, {0x00b0, 0x00a1}, {0x00b1, 0x00b1},
    {0x00b4, 0x00ab}, {0x00b5, 0x00b5}, {0x00b6, 0x00a6}, {0x00b7, 0x00e1}, {0x00b8, 0x00fc},
    {0x00ba, 0x00bc}, {0x00bb, 0x00c8}, {0x00bf, 0x00c0}, {0x00c0, 0x00cb}, {0x00c1, 0x00e7},
    {0x00c2, 0x00e5}, {0x00c3, 0x00cc}, {0x00c4, 0x0080}, {0x00c5, 0x0081}, {0x00c7, 0x0082},
    {0x00c8, 0x00e9}, {0x00c9, 0x0083}, {0x00ca, 0x00e6}, {0x00cb, 0x00e8}, {0x00cc, 0x00ed},
    {0x00cd, 0x00ea}, {0x00ce, 0x00eb}, {0x00cf, 0x00ec}, {0x00d1, 0x0084}, {0x00d2, 0x00f1},
    {0x00d3, 0x00ee}, {0x00d4, 0x00ef}, {0x00d5, 0x00cd}, {0x00d6, 0x0085}, {0x00d9, 0x00f4},
    {0x00da, 0x00f2}, {0x00db, 0x00f3}, {0x00dc, 0x0086}, {0x00df, 0x00a7}, {0x00e0, 0x0088},
    {0x00e1, 0x0087}, {0x00e2, 0x0089}, {0x00e3, 0x008b}, {0x00e4, 0x008a}, {0x00e5, 0x008c},
    {0x00e7, 0x008d}, {0x00e8, 0x008f}, {0x00e9, 0x008e}, {0x00ea, 0x0090}, {0x00eb, 0x0091},
    {0x00ec, 0x0093}, {0x00ed, 0x0092}, {0x00ee, 0x0094}, {0x00ef, 0x0095}, {0x00f1, 0x0096},
    {0x00f2, 0x0098}, {0x00f3, 0x0097}, {0x00f4, 0x0099}, {0x00f5, 0x009b}, {0x00f6, 0x009a},
    {0x00f7, 0x00d6}, {0x00f9, 0x009d}, {0x00fa, 0x009c}, {0x00fb, 0x009e}, {0x00fc, 0x009f},
    {0x00ff, 0x00d8}, {0x0102, 0x00ae}, {0x0103, 0x00be}, {0x0131, 0x00f5}, {0x0152, 0x00ce},
    {0x0153, 0x00cf}, {0x0178, 0x00d9}, {0x0192, 0x00c4}, {0x0218, 0x00af}, {0x0219, 0x00bf},
    {0x021a, 0x00de}, {0x021b, 0x00df}, {0x02c6, 0x00f6}, {0x02c7, 0x00ff}, {0x02d8, 0x00f9},
    {0x02d9, 0x00fa}, {0x02da, 0x00fb}, {0x02db, 0x00fe}, {0x02dc, 0x00f7}, {0x02dd, 0x00fd},
    {0x03a9, 0x00bd}, {0x03c0, 0x00b9}, {0x2013, 0x00d0}, {0x2014, 0x00d1}, {0x2018, 0x00d4},
    {0x2019, 0x00d5}, {0x201a, 0x00e2}, {0x201c, 0x00d2}, {0x201d, 0x00d3}, {0x201e, 0x00e3},
    {0x2020, 0x00a0}, {0x2021, 0x00e0}, {0x2022, 0x00a5}, {0x2026, 0x00c9}, {0x2030, 0x00e4},
    {0x2039, 0x00dc}, {0x203a, 0x00dd}, {0x2044, 0x00da}, {0x20ac, 0x00db}, {0x2122, 0x00aa},
    {0x2202, 0x00b6}, {0x2206, 0x00c6}, {0x220f, 0x00b8}, {0x2211, 0x00b7}, {0x221a, 0x00c3},
    {0x221e, 0x00b0}, {0x222b, 0x00ba}, {0x2248, 0x00c5}, {0x2260, 0x00ad}, {0x2264, 0x00b2},
    {0x2265, 0x00b3}, {0x25ca, 0x00d7}, {0xf8ff, 0x00f0},
};

// Four zero bytes follow the table up to the next symbol (0x1c046f2a0).
// Native macromanian_to_uni, 0x1c046f19c (__TEXT,__const, 128 uint16 entries). Non-external.
static const uint16_t macromanian_to_uni[128] = {
    0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5,
    0x00e7, 0x00e9, 0x00e8, 0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 0x00f2, 0x00f4,
    0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6,
    0x00df, 0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x0102, 0x0218, 0x221e, 0x00b1, 0x2264, 0x2265,
    0x00a5, 0x00b5, 0x2202, 0x2211, 0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x0103, 0x0219, 0x00bf,
    0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5,
    0x0152, 0x0153, 0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 0x00ff, 0x0178, 0x2044,
    0x20ac, 0x2039, 0x203a, 0x021a, 0x021b, 0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1,
    0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4, 0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9,
    0x0131, 0x02c6, 0x02dc, 0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7,
};

// Native __CFToISOLatin2, 0x1c02f2584. Non-external. Units above 0x7f are searched only in U+00A0-U+02DD (the
// 32-bit test unit - 0x2de >= 0xfffffdc2).
uint32_t __CFToISOLatin2(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x2de >= UINT32_C(0xfffffdc2)) {
        int low = 0;
        int high = 95;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = latin2_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)latin2_from_uni[middle][1];
                    return 1;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        } while (low <= high);
    }
    return 0;
}

// Native __CFFromISOLatin2, 0x1c02f25f4. Non-external. Bytes up to 0x9f map to themselves; always succeeds.
uint32_t __CFFromISOLatin2(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if (byte > 0x9f) {
        value = latin2_to_uni[byte - 0xa0];
    }
    *character = value;
    return 1;
}

// Native __CFToISOLatin2Precompose, 0x1c02f2618. Non-external. Calls
// CFStringEncodingPrecomposeLatinCharacter(character, numChars, NULL); the ISO Latin2 lookup is inlined with
// the range test on the low halfword ((uint16_t)(unit - 0x2de) >= 0xfdc2), and a zero byte fails.
OCFIndex __CFToISOLatin2Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    uint16_t precomposed = OCFStringEncodingPrecomposeLatinCharacter(character, numChars, NULL);
    uint8_t byte;
    if (precomposed <= 0x7f) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed - 0x2de) < 0xfdc2) {
            return 0;
        }
        int low = 0;
        int high = 95;
        for (;;) {
            int middle = low + ((high - low) >> 1);
            uint32_t key = latin2_from_uni[middle][0];
            if (precomposed >= key) {
                if (precomposed <= key) {
                    byte = (uint8_t)latin2_from_uni[middle][1];
                    break;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
            if (low > high) {
                return 0;
            }
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

// Native __CFIsValidCombiningCharacterForLatin2, 0x1c02f26d4. Non-external. Evidence:
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json. Units 0x300-0x35f test their bit in
// __CFLatin2CombiningCharBitmap, most significant bit first (word (unit - 0x300) >> 5, shifted right by 31 -
// (unit & 31)); any other unit is false.
bool __CFIsValidCombiningCharacterForLatin2(uint16_t character) {
    uint32_t offset = (uint32_t)character - 0x300;
    if (offset > 0x5F) {
        return false;
    }
    return (__CFLatin2CombiningCharBitmap[offset >> 5] >> (~(uint32_t)character & 31)) & 1;
}

// Native __CFToDOSLatin2, 0x1c02f2708. Non-external. Units outside U+00A0-U+25A0 (the 32-bit test
// unit - 0x25a1 >= 0xffffdaff) fail at once. A unit that the DOS Latin2 table does not hold is then searched
// in the ISO Latin2 table (latin2_from_uni, range U+00A0-U+02DD), as the native code falls through to that
// search.
uint32_t __CFToDOSLatin2(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x25a1 < UINT32_C(0xffffdaff)) {
        return 0;
    }
    int low = 0;
    int high = 127;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t key = cp852_from_uni[middle][0];
        if (character >= key) {
            if (character <= key) {
                *byte = (uint8_t)cp852_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    if ((uint32_t)character - 0x2de >= UINT32_C(0xfffffdc2)) {
        low = 0;
        high = 95;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = latin2_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)latin2_from_uni[middle][1];
                    return 1;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        } while (low <= high);
    }
    return 0;
}

// Native __CFFromDOSLatin2, 0x1c02f27c8. Non-external. The unit is stored before the U+FFFD test.
uint32_t __CFFromDOSLatin2(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = cp852_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToDOSLatin2Precompose, 0x1c02f27f0. Non-external. Calls
// CFStringEncodingPrecomposeLatinCharacter(character, numChars, NULL), then __CFToDOSLatin2 with the
// precomposed unit in both W0 (the flags) and W1; the byte scratch is zeroed first and a zero byte fails.
OCFIndex __CFToDOSLatin2Precompose(
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
    if (__CFToDOSLatin2(precomposed, precomposed, &byte) == 0 || byte == 0) {
        return 0;
    }
    if (maxByteLen != 0) {
        *bytes = byte;
    }
    *usedByteLen = 1;
    return 2;
}

// Native __CFToWinLatin2, 0x1c02f2898. Non-external. Units U+00A6-U+2122 are searched in the Windows table
// first; a unit below U+00A6, or one that table does not hold, is searched in the ISO Latin2 table
// (latin2_from_uni, range U+00A0-U+02DD). Units above U+2122 fail at once.
uint32_t __CFToWinLatin2(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    int low;
    int high;
    if (character >= 0xa6) {
        if (character > 0x2122) {
            return 0;
        }
        low = 0;
        high = 41;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = cp1250_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)cp1250_from_uni[middle][1];
                    return 1;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        } while (low <= high);
    }
    if ((uint32_t)character - 0x2de >= UINT32_C(0xfffffdc2)) {
        low = 0;
        high = 95;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = latin2_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)latin2_from_uni[middle][1];
                    return 1;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        } while (low <= high);
    }
    return 0;
}

// Native __CFFromWinLatin2, 0x1c02f2958. Non-external. The unit is stored before the U+FFFD test.
uint32_t __CFFromWinLatin2(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = cp1250_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToWinLatin2Precompose, 0x1c02f2980. Non-external. Calls
// CFStringEncodingPrecomposeLatinCharacter(character, numChars, NULL), then __CFToWinLatin2 with the
// precomposed unit in both W0 (the flags) and W1; the byte scratch is zeroed first and a zero byte fails.
OCFIndex __CFToWinLatin2Precompose(
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
    if (__CFToWinLatin2(precomposed, precomposed, &byte) == 0 || byte == 0) {
        return 0;
    }
    if (maxByteLen != 0) {
        *bytes = byte;
    }
    *usedByteLen = 1;
    return 2;
}

// Native __CFToMacCentEuro, 0x1c02f2a28. Non-external. Units above 0x7f are searched only in U+00A0-U+25CA
// (the 32-bit test unit - 0x25cb >= 0xffffdad5).
uint32_t __CFToMacCentEuro(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x25cb >= UINT32_C(0xffffdad5)) {
        int low = 0;
        int high = 127;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = maccenteuro_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)maccenteuro_from_uni[middle][1];
                    return 1;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        } while (low <= high);
    }
    return 0;
}

// Native __CFFromMacCentEuro, 0x1c02f2aa0. Non-external. Always succeeds.
uint32_t __CFFromMacCentEuro(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = maccenteuro_to_uni[byte & 0x7f];
    }
    *character = value;
    return 1;
}

// Native __CFToMacCentEuroPrecompose, 0x1c02f2ac0. Non-external. Calls
// CFStringEncodingPrecomposeLatinCharacter(character, numChars, NULL); the Mac Central European lookup is
// inlined with the range test on the low halfword ((uint16_t)(unit - 0x25cb) >= 0xdad5), and a zero byte
// fails.
OCFIndex __CFToMacCentEuroPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    uint16_t precomposed = OCFStringEncodingPrecomposeLatinCharacter(character, numChars, NULL);
    uint8_t byte;
    if (precomposed <= 0x7f) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed - 0x25cb) < 0xdad5) {
            return 0;
        }
        int low = 0;
        int high = 127;
        for (;;) {
            int middle = low + ((high - low) >> 1);
            uint32_t key = maccenteuro_from_uni[middle][0];
            if (precomposed >= key) {
                if (precomposed <= key) {
                    byte = (uint8_t)maccenteuro_from_uni[middle][1];
                    break;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
            if (low > high) {
                return 0;
            }
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

// Native __CFToMacCroatian, 0x1c02f2b80. Non-external. Units above 0x7f are searched only in U+00A0-U+F8FF
// ((uint16_t)(unit + 0x700) >= 0x7a0).
uint32_t __CFToMacCroatian(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint16_t)(character + 0x700) >= 0x7a0) {
        int low = 0;
        int high = 127;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = maccroatian_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)maccroatian_from_uni[middle][1];
                    return 1;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        } while (low <= high);
    }
    return 0;
}

// Native __CFFromMacCroatian, 0x1c02f2bf0. Non-external. Always succeeds.
uint32_t __CFFromMacCroatian(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = maccroatian_to_uni[byte & 0x7f];
    }
    *character = value;
    return 1;
}

// Native __CFToMacCroatianPrecompose, 0x1c02f2c10. Non-external. Calls
// CFStringEncodingPrecomposeLatinCharacter(character, numChars, NULL); the Mac Croatian lookup is inlined
// with the range test (uint16_t)(unit + 0x700) >= 0x7a0, and a zero byte fails.
OCFIndex __CFToMacCroatianPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    uint16_t precomposed = OCFStringEncodingPrecomposeLatinCharacter(character, numChars, NULL);
    uint8_t byte;
    if (precomposed <= 0x7f) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed + 0x700) < 0x7a0) {
            return 0;
        }
        int low = 0;
        int high = 127;
        for (;;) {
            int middle = low + ((high - low) >> 1);
            uint32_t key = maccroatian_from_uni[middle][0];
            if (precomposed >= key) {
                if (precomposed <= key) {
                    byte = (uint8_t)maccroatian_from_uni[middle][1];
                    break;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
            if (low > high) {
                return 0;
            }
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

// Native __CFToMacRomanian, 0x1c02f2cd0. Non-external; a standard eight-bit converter that reads only
// characters[0] and consumes one unit. Units above 0x7f are searched only in U+00A0-U+F8FF
// ((uint16_t)(unit + 0x700) >= 0x7a0).
uint32_t __CFToMacRomanian(uint32_t flags, const uint16_t *characters, OCFIndex numChars, uint8_t *byte) {
    (void)flags;
    (void)numChars;
    uint16_t character = characters[0];
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint16_t)(character + 0x700) >= 0x7a0) {
        int low = 0;
        int high = 127;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = macromanian_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)macromanian_from_uni[middle][1];
                    return 1;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        } while (low <= high);
    }
    return 0;
}

// Native __CFFromMacRomanian, 0x1c02f2d44. Non-external; a standard eight-bit converter that writes one unit,
// stored before the U+FFFD test.
uint32_t __CFFromMacRomanian(uint32_t flags, uint8_t byte, uint16_t *characters) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = macromanian_to_uni[byte & 0x7f];
    }
    *characters = value;
    return value != 0xfffd;
}

// Native __CFToMacRomanianLen, 0x1c02f2d6c. Non-external. Counts the units other than U+0326; the loop ends
// only when the count reaches zero (CBZ), so a negative count is not rejected.
OCFIndex __CFToMacRomanianLen(uint32_t flags, const uint16_t *characters, OCFIndex numChars) {
    (void)flags;
    OCFIndex length = 0;
    while (numChars != 0) {
        if (*characters++ != 0x326) {
            ++length;
        }
        --numChars;
    }
    return length;
}

// Native __CFFromMacRomanianLen, 0x1c02f2d8c. Non-external. Bytes 0xaf, 0xbf, 0xde and 0xdf (bits 0, 16, 47
// and 48 of 0x1800000010001, indexed by byte - 0xaf) count two units, every other byte one; the loop ends
// only when the count reaches zero (CBZ).
OCFIndex __CFFromMacRomanianLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes) {
    (void)flags;
    OCFIndex length = 0;
    while (numBytes != 0) {
        uint32_t index = (uint32_t)*bytes++ - 0xaf;
        if (index <= 0x30 && ((UINT64_C(1) << index) & UINT64_C(0x1800000010001)) != 0) {
            length += 2;
        } else {
            length += 1;
        }
        --numBytes;
    }
    return length;
}

// Native __CFToMacRomanianPrecompose, 0x1c02f2ddc. Non-external. With at least two units and U+0326
// (combining comma below) second, S, T, s and t map to 0xaf, 0xde, 0xbf and 0xdf and any other first unit
// fails without calling the Latin precomposer; otherwise CFStringEncodingPrecomposeLatinCharacter(character,
// numChars, NULL) is looked up inline with the range test (uint16_t)(unit + 0x700) >= 0x7a0, and a zero byte
// fails.
OCFIndex __CFToMacRomanianPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    uint8_t byte;
    if (numChars >= 2 && character[1] == 0x326) {
        switch (character[0]) {
            case 'S':
                byte = 0xaf;
                break;
            case 'T':
                byte = 0xde;
                break;
            case 's':
                byte = 0xbf;
                break;
            case 't':
                byte = 0xdf;
                break;
            default:
                return 0;
        }
        if (maxByteLen != 0) {
            *bytes = byte;
        }
        *usedByteLen = 1;
        return 2;
    }
    uint16_t precomposed = OCFStringEncodingPrecomposeLatinCharacter(character, numChars, NULL);
    if (precomposed <= 0x7f) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed + 0x700) < 0x7a0) {
            return 0;
        }
        int low = 0;
        int high = 127;
        for (;;) {
            int middle = low + ((high - low) >> 1);
            uint32_t key = macromanian_from_uni[middle][0];
            if (precomposed >= key) {
                if (precomposed <= key) {
                    byte = (uint8_t)macromanian_from_uni[middle][1];
                    break;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
            if (low > high) {
                return 0;
            }
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

// Native __CFConverterISOLatin2, 0x2227ab750 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterISOLatin2 = {
    .toBytes.cheapEightBit = __CFToISOLatin2,
    .toUnicode.cheapEightBit = __CFFromISOLatin2,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToISOLatin2Precompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatin2,
};

// Native __CFConverterDOSLatin2, 0x2227ab798 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterDOSLatin2 = {
    .toBytes.cheapEightBit = __CFToDOSLatin2,
    .toUnicode.cheapEightBit = __CFFromDOSLatin2,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToDOSLatin2Precompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatin2,
};

// Native __CFConverterWinLatin2, 0x2227ab7e0 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterWinLatin2 = {
    .toBytes.cheapEightBit = __CFToWinLatin2,
    .toUnicode.cheapEightBit = __CFFromWinLatin2,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToWinLatin2Precompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatin2,
};

// Native __CFConverterMacCentEuro, 0x2227ab828 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterMacCentEuro = {
    .toBytes.cheapEightBit = __CFToMacCentEuro,
    .toUnicode.cheapEightBit = __CFFromMacCentEuro,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToMacCentEuroPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatin2,
};

// Native __CFConverterMacCroatian, 0x2227ab870 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterMacCroatian = {
    .toBytes.cheapEightBit = __CFToMacCroatian,
    .toUnicode.cheapEightBit = __CFFromMacCroatian,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToMacCroatianPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatin2,
};

// Native __CFConverterMacRomanian, 0x2227ab8b8 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external); a standard eight-bit converter (class 2) with length callbacks.
const StringEncodingConverterRecord __CFConverterMacRomanian = {
    .toBytes.standardEightBit = __CFToMacRomanian,
    .toUnicode.standardEightBit = __CFFromMacRomanian,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 2,
    .toBytesLen = __CFToMacRomanianLen,
    .toUnicodeLen = __CFFromMacRomanianLen,
    .toBytesPrecompose = __CFToMacRomanianPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatin2,
};
