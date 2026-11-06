//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFGreek.h"
#include "CoreFoundation/StringEncoding/OCFSingleByte.h"

// Inferred organization: the Greek converters (ISO Latin Greek, Mac Greek, DOS Greek and Windows Greek,
// 0x1c03057b4-0x1c0305e1c) are contiguous in the iOS Simulator __text section and are kept in one translation
// unit with their tables (0x1c046fd30-0x1c04708e0, __TEXT,__const) and converter records
// (0x2227abf60-0x2227ac080,
// __DATA_CONST,__const). The native source path is not in the evidence. __CFIsValidCombiningCharacterForGreek
// (0x1c0305950), which lies in the same range, is restored here in native order; __CFPrecomposeGreekCharacter
// (0x1c0305e1c) and its map are restored here, before the records. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446:
// unread. Every converter ignores its flags. The from-Unicode tables hold (unit, byte) pairs sorted by unit;
// each search reads the byte as the low byte of the second halfword (LDRB at offset 2). Every precompose
// function passes (character, numChars, &used) to __CFPrecomposeGreekCharacter and returns the count it
// stored in used. Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.

// Native ISOLatinGreek_from_uni, 0x1c046fd30 (__TEXT,__const, 91 four-byte entries). Non-external.
static const uint16_t ISOLatinGreek_from_uni[91][2] = {
    {0x00a3, 0x00a3}, {0x00a6, 0x00a6}, {0x00a7, 0x00a7}, {0x00a8, 0x00a8}, {0x00a9, 0x00a9},
    {0x00ab, 0x00ab}, {0x00ac, 0x00ac}, {0x00ad, 0x00ad}, {0x00b0, 0x00b0}, {0x00b1, 0x00b1},
    {0x00b2, 0x00b2}, {0x00b3, 0x00b3}, {0x00b7, 0x00b7}, {0x00bb, 0x00bb}, {0x00bd, 0x00bd},
    {0x02bc, 0x00a2}, {0x02bd, 0x00a1}, {0x0384, 0x00b4}, {0x0385, 0x00b5}, {0x0386, 0x00b6},
    {0x0388, 0x00b8}, {0x0389, 0x00b9}, {0x038a, 0x00ba}, {0x038c, 0x00bc}, {0x038e, 0x00be},
    {0x038f, 0x00bf}, {0x0390, 0x00c0}, {0x0391, 0x00c1}, {0x0392, 0x00c2}, {0x0393, 0x00c3},
    {0x0394, 0x00c4}, {0x0395, 0x00c5}, {0x0396, 0x00c6}, {0x0397, 0x00c7}, {0x0398, 0x00c8},
    {0x0399, 0x00c9}, {0x039a, 0x00ca}, {0x039b, 0x00cb}, {0x039c, 0x00cc}, {0x039d, 0x00cd},
    {0x039e, 0x00ce}, {0x039f, 0x00cf}, {0x03a0, 0x00d0}, {0x03a1, 0x00d1}, {0x03a3, 0x00d3},
    {0x03a4, 0x00d4}, {0x03a5, 0x00d5}, {0x03a6, 0x00d6}, {0x03a7, 0x00d7}, {0x03a8, 0x00d8},
    {0x03a9, 0x00d9}, {0x03aa, 0x00da}, {0x03ab, 0x00db}, {0x03ac, 0x00dc}, {0x03ad, 0x00dd},
    {0x03ae, 0x00de}, {0x03af, 0x00df}, {0x03b0, 0x00e0}, {0x03b1, 0x00e1}, {0x03b2, 0x00e2},
    {0x03b3, 0x00e3}, {0x03b4, 0x00e4}, {0x03b5, 0x00e5}, {0x03b6, 0x00e6}, {0x03b7, 0x00e7},
    {0x03b8, 0x00e8}, {0x03b9, 0x00e9}, {0x03ba, 0x00ea}, {0x03bb, 0x00eb}, {0x03bc, 0x00ec},
    {0x03bd, 0x00ed}, {0x03be, 0x00ee}, {0x03bf, 0x00ef}, {0x03c0, 0x00f0}, {0x03c1, 0x00f1},
    {0x03c2, 0x00f2}, {0x03c3, 0x00f3}, {0x03c4, 0x00f4}, {0x03c5, 0x00f5}, {0x03c6, 0x00f6},
    {0x03c7, 0x00f7}, {0x03c8, 0x00f8}, {0x03c9, 0x00f9}, {0x03ca, 0x00fa}, {0x03cb, 0x00fb},
    {0x03cc, 0x00fc}, {0x03cd, 0x00fd}, {0x03ce, 0x00fe}, {0x2015, 0x00af}, {0x2018, 0x00a1},
    {0x2019, 0x00a2},
};

// The 95 entries map bytes 0xa1-0xff; two padding bytes follow up to __CFGreekPrecompMap.
// Native ISOLatinGreek_to_uni, 0x1c046fe9c (__TEXT,__const, 95 uint16 entries). Non-external.
static const uint16_t ISOLatinGreek_to_uni[95] = {
    0x2018, 0x2019, 0x00a3, 0xfffd, 0xfffd, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0xfffd, 0x00ab, 0x00ac,
    0x00ad, 0xfffd, 0x2015, 0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x0384, 0x0385, 0x0386, 0x00b7, 0x0388,
    0x0389, 0x038a, 0x00bb, 0x038c, 0x00bd, 0x038e, 0x038f, 0x0390, 0x0391, 0x0392, 0x0393, 0x0394,
    0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f, 0x03a0,
    0x03a1, 0xfffd, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7, 0x03a8, 0x03a9, 0x03aa, 0x03ab, 0x03ac,
    0x03ad, 0x03ae, 0x03af, 0x03b0, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, 0x03b8,
    0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf, 0x03c0, 0x03c1, 0x03c2, 0x03c3, 0x03c4,
    0x03c5, 0x03c6, 0x03c7, 0x03c8, 0x03c9, 0x03ca, 0x03cb, 0x03cc, 0x03cd, 0x03ce, 0xfffd,
};

// Native MacGreek_from_uni, 0x1c0470024 (__TEXT,__const, 128 four-byte entries). Non-external.
static const uint16_t MacGreek_from_uni[128][2] = {
    {0x00a0, 0x00ca}, {0x00a3, 0x0092}, {0x00a5, 0x00b4}, {0x00a6, 0x009b}, {0x00a7, 0x00ac},
    {0x00a8, 0x008c}, {0x00a9, 0x00a9}, {0x00ab, 0x00c7}, {0x00ac, 0x00c2}, {0x00ad, 0x00ff},
    {0x00ae, 0x00a8}, {0x00b0, 0x00ae}, {0x00b1, 0x00b1}, {0x00b2, 0x0082}, {0x00b3, 0x0084},
    {0x00b7, 0x00af}, {0x00b9, 0x0081}, {0x00bb, 0x00c8}, {0x00bd, 0x0097}, {0x00c4, 0x0080},
    {0x00c9, 0x0083}, {0x00d6, 0x0085}, {0x00dc, 0x0086}, {0x00df, 0x00a7}, {0x00e0, 0x0088},
    {0x00e2, 0x0089}, {0x00e4, 0x008a}, {0x00e7, 0x008d}, {0x00e8, 0x008f}, {0x00e9, 0x008e},
    {0x00ea, 0x0090}, {0x00eb, 0x0091}, {0x00ee, 0x0094}, {0x00ef, 0x0095}, {0x00f4, 0x0099},
    {0x00f6, 0x009a}, {0x00f7, 0x00d6}, {0x00f9, 0x009d}, {0x00fb, 0x009e}, {0x00fc, 0x009f},
    {0x0153, 0x00cf}, {0x0384, 0x008b}, {0x0385, 0x0087}, {0x0386, 0x00cd}, {0x0388, 0x00ce},
    {0x0389, 0x00d7}, {0x038a, 0x00d8}, {0x038c, 0x00d9}, {0x038e, 0x00da}, {0x038f, 0x00df},
    {0x0390, 0x00fd}, {0x0391, 0x00b0}, {0x0392, 0x00b5}, {0x0393, 0x00a1}, {0x0394, 0x00a2},
    {0x0395, 0x00b6}, {0x0396, 0x00b7}, {0x0397, 0x00b8}, {0x0398, 0x00a3}, {0x0399, 0x00b9},
    {0x039a, 0x00ba}, {0x039b, 0x00a4}, {0x039c, 0x00bb}, {0x039d, 0x00c1}, {0x039e, 0x00a5},
    {0x039f, 0x00c3}, {0x03a0, 0x00a6}, {0x03a1, 0x00c4}, {0x03a3, 0x00aa}, {0x03a4, 0x00c6},
    {0x03a5, 0x00cb}, {0x03a6, 0x00bc}, {0x03a7, 0x00cc}, {0x03a8, 0x00be}, {0x03a9, 0x00bf},
    {0x03aa, 0x00ab}, {0x03ab, 0x00bd}, {0x03ac, 0x00c0}, {0x03ad, 0x00db}, {0x03ae, 0x00dc},
    {0x03af, 0x00dd}, {0x03b0, 0x00fe}, {0x03b1, 0x00e1}, {0x03b2, 0x00e2}, {0x03b3, 0x00e7},
    {0x03b4, 0x00e4}, {0x03b5, 0x00e5}, {0x03b6, 0x00fa}, {0x03b7, 0x00e8}, {0x03b8, 0x00f5},
    {0x03b9, 0x00e9}, {0x03ba, 0x00eb}, {0x03bb, 0x00ec}, {0x03bc, 0x00ed}, {0x03bd, 0x00ee},
    {0x03be, 0x00ea}, {0x03bf, 0x00ef}, {0x03c0, 0x00f0}, {0x03c1, 0x00f2}, {0x03c2, 0x00f7},
    {0x03c3, 0x00f3}, {0x03c4, 0x00f4}, {0x03c5, 0x00f9}, {0x03c6, 0x00e6}, {0x03c7, 0x00f8},
    {0x03c8, 0x00e3}, {0x03c9, 0x00f6}, {0x03ca, 0x00fb}, {0x03cb, 0x00fc}, {0x03cc, 0x00de},
    {0x03cd, 0x00e0}, {0x03ce, 0x00f1}, {0x2013, 0x00d0}, {0x2015, 0x00d1}, {0x2018, 0x00d4},
    {0x2019, 0x00d5}, {0x201c, 0x00d2}, {0x201d, 0x00d3}, {0x2020, 0x00a0}, {0x2022, 0x0096},
    {0x2026, 0x00c9}, {0x2030, 0x0098}, {0x20ac, 0x009c}, {0x2122, 0x0093}, {0x2248, 0x00c5},
    {0x2260, 0x00ad}, {0x2264, 0x00b2}, {0x2265, 0x00b3},
};

// Native MacGreek_to_uni, 0x1c0470224 (__TEXT,__const, 128 uint16 entries). Non-external.
static const uint16_t MacGreek_to_uni[128] = {
    0x00c4, 0x00b9, 0x00b2, 0x00c9, 0x00b3, 0x00d6, 0x00dc, 0x0385, 0x00e0, 0x00e2, 0x00e4, 0x0384, 0x00a8,
    0x00e7, 0x00e9, 0x00e8, 0x00ea, 0x00eb, 0x00a3, 0x2122, 0x00ee, 0x00ef, 0x2022, 0x00bd, 0x2030, 0x00f4,
    0x00f6, 0x00a6, 0x20ac, 0x00f9, 0x00fb, 0x00fc, 0x2020, 0x0393, 0x0394, 0x0398, 0x039b, 0x039e, 0x03a0,
    0x00df, 0x00ae, 0x00a9, 0x03a3, 0x03aa, 0x00a7, 0x2260, 0x00b0, 0x00b7, 0x0391, 0x00b1, 0x2264, 0x2265,
    0x00a5, 0x0392, 0x0395, 0x0396, 0x0397, 0x0399, 0x039a, 0x039c, 0x03a6, 0x03ab, 0x03a8, 0x03a9, 0x03ac,
    0x039d, 0x00ac, 0x039f, 0x03a1, 0x2248, 0x03a4, 0x00ab, 0x00bb, 0x2026, 0x00a0, 0x03a5, 0x03a7, 0x0386,
    0x0388, 0x0153, 0x2013, 0x2015, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x0389, 0x038a, 0x038c, 0x038e,
    0x03ad, 0x03ae, 0x03af, 0x03cc, 0x038f, 0x03cd, 0x03b1, 0x03b2, 0x03c8, 0x03b4, 0x03b5, 0x03c6, 0x03b3,
    0x03b7, 0x03b9, 0x03be, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03bf, 0x03c0, 0x03ce, 0x03c1, 0x03c3, 0x03c4,
    0x03b8, 0x03c9, 0x03c2, 0x03c7, 0x03c5, 0x03b6, 0x03ca, 0x03cb, 0x0390, 0x03b0, 0x00ad,
};

// Native DOSGreek_from_uni, 0x1c0470324 (__TEXT,__const, 128 four-byte entries). Non-external.
static const uint16_t DOSGreek_from_uni[128][2] = {
    {0x00a0, 0x00ff}, {0x00b0, 0x00f8}, {0x00b1, 0x00f1}, {0x00b2, 0x00fd}, {0x00b7, 0x00fa},
    {0x00f7, 0x00f6}, {0x0386, 0x00ea}, {0x0388, 0x00eb}, {0x0389, 0x00ec}, {0x038a, 0x00ed},
    {0x038c, 0x00ee}, {0x038e, 0x00ef}, {0x038f, 0x00f0}, {0x0391, 0x0080}, {0x0392, 0x0081},
    {0x0393, 0x0082}, {0x0394, 0x0083}, {0x0395, 0x0084}, {0x0396, 0x0085}, {0x0397, 0x0086},
    {0x0398, 0x0087}, {0x0399, 0x0088}, {0x039a, 0x0089}, {0x039b, 0x008a}, {0x039c, 0x008b},
    {0x039d, 0x008c}, {0x039e, 0x008d}, {0x039f, 0x008e}, {0x03a0, 0x008f}, {0x03a1, 0x0090},
    {0x03a3, 0x0091}, {0x03a4, 0x0092}, {0x03a5, 0x0093}, {0x03a6, 0x0094}, {0x03a7, 0x0095},
    {0x03a8, 0x0096}, {0x03a9, 0x0097}, {0x03aa, 0x00f4}, {0x03ab, 0x00f5}, {0x03ac, 0x00e1},
    {0x03ad, 0x00e2}, {0x03ae, 0x00e3}, {0x03af, 0x00e5}, {0x03b1, 0x0098}, {0x03b2, 0x0099},
    {0x03b3, 0x009a}, {0x03b4, 0x009b}, {0x03b5, 0x009c}, {0x03b6, 0x009d}, {0x03b7, 0x009e},
    {0x03b8, 0x009f}, {0x03b9, 0x00a0}, {0x03ba, 0x00a1}, {0x03bb, 0x00a2}, {0x03bc, 0x00a3},
    {0x03bd, 0x00a4}, {0x03be, 0x00a5}, {0x03bf, 0x00a6}, {0x03c0, 0x00a7}, {0x03c1, 0x00a8},
    {0x03c2, 0x00aa}, {0x03c3, 0x00a9}, {0x03c4, 0x00ab}, {0x03c5, 0x00ac}, {0x03c6, 0x00ad},
    {0x03c7, 0x00ae}, {0x03c8, 0x00af}, {0x03c9, 0x00e0}, {0x03ca, 0x00e4}, {0x03cb, 0x00e8},
    {0x03cc, 0x00e6}, {0x03cd, 0x00e7}, {0x03ce, 0x00e9}, {0x207f, 0x00fc}, {0x2219, 0x00f9},
    {0x221a, 0x00fb}, {0x2248, 0x00f7}, {0x2264, 0x00f3}, {0x2265, 0x00f2}, {0x2500, 0x00c4},
    {0x2502, 0x00b3}, {0x250c, 0x00da}, {0x2510, 0x00bf}, {0x2514, 0x00c0}, {0x2518, 0x00d9},
    {0x251c, 0x00c3}, {0x2524, 0x00b4}, {0x252c, 0x00c2}, {0x2534, 0x00c1}, {0x253c, 0x00c5},
    {0x2550, 0x00cd}, {0x2551, 0x00ba}, {0x2552, 0x00d5}, {0x2553, 0x00d6}, {0x2554, 0x00c9},
    {0x2555, 0x00b8}, {0x2556, 0x00b7}, {0x2557, 0x00bb}, {0x2558, 0x00d4}, {0x2559, 0x00d3},
    {0x255a, 0x00c8}, {0x255b, 0x00be}, {0x255c, 0x00bd}, {0x255d, 0x00bc}, {0x255e, 0x00c6},
    {0x255f, 0x00c7}, {0x2560, 0x00cc}, {0x2561, 0x00b5}, {0x2562, 0x00b6}, {0x2563, 0x00b9},
    {0x2564, 0x00d1}, {0x2565, 0x00d2}, {0x2566, 0x00cb}, {0x2567, 0x00cf}, {0x2568, 0x00d0},
    {0x2569, 0x00ca}, {0x256a, 0x00d8}, {0x256b, 0x00d7}, {0x256c, 0x00ce}, {0x2580, 0x00df},
    {0x2584, 0x00dc}, {0x2588, 0x00db}, {0x258c, 0x00dd}, {0x2590, 0x00de}, {0x2591, 0x00b0},
    {0x2592, 0x00b1}, {0x2593, 0x00b2}, {0x25a0, 0x00fe},
};

// Native DOSGreek_to_uni, 0x1c0470524 (__TEXT,__const, 128 uint16 entries). Non-external.
static const uint16_t DOSGreek_to_uni[128] = {
    0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d,
    0x039e, 0x039f, 0x03a0, 0x03a1, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7, 0x03a8, 0x03a9, 0x03b1, 0x03b2,
    0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, 0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
    0x03c0, 0x03c1, 0x03c3, 0x03c2, 0x03c4, 0x03c5, 0x03c6, 0x03c7, 0x03c8, 0x2591, 0x2592, 0x2593, 0x2502,
    0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510, 0x2514,
    0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550,
    0x256c, 0x2567, 0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c,
    0x2588, 0x2584, 0x258c, 0x2590, 0x2580, 0x03c9, 0x03ac, 0x03ad, 0x03ae, 0x03ca, 0x03af, 0x03cc, 0x03cd,
    0x03cb, 0x03ce, 0x0386, 0x0388, 0x0389, 0x038a, 0x038c, 0x038e, 0x038f, 0x00b1, 0x2265, 0x2264, 0x03aa,
    0x03ab, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0,
};

// Native WindowsGreek_from_uni, 0x1c0470624 (__TEXT,__const, 111 four-byte entries). Non-external.
static const uint16_t WindowsGreek_from_uni[111][2] = {
    {0x00a0, 0x00a0}, {0x00a3, 0x00a3}, {0x00a4, 0x00a4}, {0x00a5, 0x00a5}, {0x00a6, 0x00a6},
    {0x00a7, 0x00a7}, {0x00a8, 0x00a8}, {0x00a9, 0x00a9}, {0x00ab, 0x00ab}, {0x00ac, 0x00ac},
    {0x00ad, 0x00ad}, {0x00ae, 0x00ae}, {0x00b0, 0x00b0}, {0x00b1, 0x00b1}, {0x00b2, 0x00b2},
    {0x00b3, 0x00b3}, {0x00b5, 0x00b5}, {0x00b6, 0x00b6}, {0x00b7, 0x00b7}, {0x00bb, 0x00bb},
    {0x00bd, 0x00bd}, {0x0192, 0x0083}, {0x0384, 0x00b4}, {0x0385, 0x00a1}, {0x0386, 0x00a2},
    {0x0388, 0x00b8}, {0x0389, 0x00b9}, {0x038a, 0x00ba}, {0x038c, 0x00bc}, {0x038e, 0x00be},
    {0x038f, 0x00bf}, {0x0390, 0x00c0}, {0x0391, 0x00c1}, {0x0392, 0x00c2}, {0x0393, 0x00c3},
    {0x0394, 0x00c4}, {0x0395, 0x00c5}, {0x0396, 0x00c6}, {0x0397, 0x00c7}, {0x0398, 0x00c8},
    {0x0399, 0x00c9}, {0x039a, 0x00ca}, {0x039b, 0x00cb}, {0x039c, 0x00cc}, {0x039d, 0x00cd},
    {0x039e, 0x00ce}, {0x039f, 0x00cf}, {0x03a0, 0x00d0}, {0x03a1, 0x00d1}, {0x03a3, 0x00d3},
    {0x03a4, 0x00d4}, {0x03a5, 0x00d5}, {0x03a6, 0x00d6}, {0x03a7, 0x00d7}, {0x03a8, 0x00d8},
    {0x03a9, 0x00d9}, {0x03aa, 0x00da}, {0x03ab, 0x00db}, {0x03ac, 0x00dc}, {0x03ad, 0x00dd},
    {0x03ae, 0x00de}, {0x03af, 0x00df}, {0x03b0, 0x00e0}, {0x03b1, 0x00e1}, {0x03b2, 0x00e2},
    {0x03b3, 0x00e3}, {0x03b4, 0x00e4}, {0x03b5, 0x00e5}, {0x03b6, 0x00e6}, {0x03b7, 0x00e7},
    {0x03b8, 0x00e8}, {0x03b9, 0x00e9}, {0x03ba, 0x00ea}, {0x03bb, 0x00eb}, {0x03bc, 0x00ec},
    {0x03bd, 0x00ed}, {0x03be, 0x00ee}, {0x03bf, 0x00ef}, {0x03c0, 0x00f0}, {0x03c1, 0x00f1},
    {0x03c2, 0x00f2}, {0x03c3, 0x00f3}, {0x03c4, 0x00f4}, {0x03c5, 0x00f5}, {0x03c6, 0x00f6},
    {0x03c7, 0x00f7}, {0x03c8, 0x00f8}, {0x03c9, 0x00f9}, {0x03ca, 0x00fa}, {0x03cb, 0x00fb},
    {0x03cc, 0x00fc}, {0x03cd, 0x00fd}, {0x03ce, 0x00fe}, {0x2013, 0x0096}, {0x2014, 0x0097},
    {0x2015, 0x00af}, {0x2018, 0x0091}, {0x2019, 0x0092}, {0x201a, 0x0082}, {0x201c, 0x0093},
    {0x201d, 0x0094}, {0x201e, 0x0084}, {0x2020, 0x0086}, {0x2021, 0x0087}, {0x2022, 0x0095},
    {0x2026, 0x0085}, {0x2030, 0x0089}, {0x2039, 0x008b}, {0x203a, 0x009b}, {0x20ac, 0x0080},
    {0x2122, 0x0099},
};

// Native WindowsGreek_to_uni, 0x1c04707e0 (__TEXT,__const, 128 uint16 entries). Non-external.
static const uint16_t WindowsGreek_to_uni[128] = {
    0x20ac, 0xfffd, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0xfffd, 0x2030, 0xfffd, 0x2039, 0xfffd,
    0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0xfffd, 0x2122,
    0xfffd, 0x203a, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x00a0, 0x0385, 0x0386, 0x00a3, 0x00a4, 0x00a5, 0x00a6,
    0x00a7, 0x00a8, 0x00a9, 0xfffd, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x2015, 0x00b0, 0x00b1, 0x00b2, 0x00b3,
    0x0384, 0x00b5, 0x00b6, 0x00b7, 0x0388, 0x0389, 0x038a, 0x00bb, 0x038c, 0x00bd, 0x038e, 0x038f, 0x0390,
    0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d,
    0x039e, 0x039f, 0x03a0, 0x03a1, 0xfffd, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7, 0x03a8, 0x03a9, 0x03aa,
    0x03ab, 0x03ac, 0x03ad, 0x03ae, 0x03af, 0x03b0, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7,
    0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf, 0x03c0, 0x03c1, 0x03c2, 0x03c3, 0x03c4,
    0x03c5, 0x03c6, 0x03c7, 0x03c8, 0x03c9, 0x03ca, 0x03cb, 0x03cc, 0x03cd, 0x03ce, 0xfffd,
};

// Native __CFToISOLatinGreek, 0x1c03057b4. Non-external. Units above 0xa0 are searched only in U+00A3-U+2019
// (the 32-bit test unit - 0x201a >= 0xffffe089).
uint32_t __CFToISOLatinGreek(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0xa0) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x201a >= UINT32_C(0xffffe089)) {
        int low = 0;
        int high = 90;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = ISOLatinGreek_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)ISOLatinGreek_from_uni[middle][1];
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

// Native __CFFromISOLatinGreek, 0x1c030582c. Non-external. Bytes up to 0xa0 map to themselves; the unit is
// stored before the U+FFFD test.
uint32_t __CFFromISOLatinGreek(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if (byte > 0xa0) {
        value = ISOLatinGreek_to_uni[byte - 0xa1];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToISOLatinGreekPrecompose, 0x1c0305858. Non-external. The ISOLatinGreek lookup is inlined with
// the range test on the low halfword ((uint16_t)(unit - 0x201a) >= 0xe089); a zero byte fails, and success
// returns the helper's used count.
OCFIndex __CFToISOLatinGreekPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    OCFIndex used;
    uint16_t precomposed = __CFPrecomposeGreekCharacter(character, numChars, &used);
    uint8_t byte;
    if (precomposed <= 0xa0) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed - 0x201a) < 0xe089) {
            return 0;
        }
        int low = 0;
        int high = 90;
        for (;;) {
            int middle = low + ((high - low) >> 1);
            uint32_t key = ISOLatinGreek_from_uni[middle][0];
            if (precomposed >= key) {
                if (precomposed <= key) {
                    byte = (uint8_t)ISOLatinGreek_from_uni[middle][1];
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
    return used;
}

// Native __CFIsValidCombiningCharacterForGreek, 0x1c0305950. Non-external. Evidence:
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json. A tail call of
// CFStringEncodingIsValidCombiningCharacterForLatin1.
bool __CFIsValidCombiningCharacterForGreek(uint16_t character) {
    return OCFStringEncodingIsValidCombiningCharacterForLatin1(character);
}

// Native __CFToMacGreek, 0x1c0305954. Non-external. Units above 0x7f are searched only in U+00A0-U+2265 (the
// 32-bit test unit - 0x2266 >= 0xffffde3a).
uint32_t __CFToMacGreek(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x2266 >= UINT32_C(0xffffde3a)) {
        int low = 0;
        int high = 127;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = MacGreek_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)MacGreek_from_uni[middle][1];
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

// Native __CFFromMacGreek, 0x1c03059cc. Non-external. The unit is stored before the U+FFFD test.
uint32_t __CFFromMacGreek(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = MacGreek_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToMacGreekPrecompose, 0x1c03059f4. Non-external. The MacGreek lookup is inlined with the range
// test on the low halfword ((uint16_t)(unit - 0x2266) >= 0xde3a); a zero byte fails, and success returns the
// helper's used count.
OCFIndex __CFToMacGreekPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    OCFIndex used;
    uint16_t precomposed = __CFPrecomposeGreekCharacter(character, numChars, &used);
    uint8_t byte;
    if (precomposed <= 0x7f) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed - 0x2266) < 0xde3a) {
            return 0;
        }
        int low = 0;
        int high = 127;
        for (;;) {
            int middle = low + ((high - low) >> 1);
            uint32_t key = MacGreek_from_uni[middle][0];
            if (precomposed >= key) {
                if (precomposed <= key) {
                    byte = (uint8_t)MacGreek_from_uni[middle][1];
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
    return used;
}

// Native __CFToDOSGreek, 0x1c0305aec. Non-external. Units above 0x7f are searched only in U+00A0-U+25A0 (the
// 32-bit test unit - 0x25a1 >= 0xffffdaff).
uint32_t __CFToDOSGreek(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x25a1 >= UINT32_C(0xffffdaff)) {
        int low = 0;
        int high = 127;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = DOSGreek_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)DOSGreek_from_uni[middle][1];
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

// Native __CFFromDOSGreek, 0x1c0305b64. Non-external. The unit is stored before the U+FFFD test.
uint32_t __CFFromDOSGreek(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = DOSGreek_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToDOSGreekPrecompose, 0x1c0305b8c. Non-external. The DOSGreek lookup is inlined with the range
// test on the low halfword ((uint16_t)(unit - 0x25a1) >= 0xdaff); a zero byte fails, and success returns the
// helper's used count.
OCFIndex __CFToDOSGreekPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    OCFIndex used;
    uint16_t precomposed = __CFPrecomposeGreekCharacter(character, numChars, &used);
    uint8_t byte;
    if (precomposed <= 0x7f) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed - 0x25a1) < 0xdaff) {
            return 0;
        }
        int low = 0;
        int high = 127;
        for (;;) {
            int middle = low + ((high - low) >> 1);
            uint32_t key = DOSGreek_from_uni[middle][0];
            if (precomposed >= key) {
                if (precomposed <= key) {
                    byte = (uint8_t)DOSGreek_from_uni[middle][1];
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
    return used;
}

// Native __CFToWindowsGreek, 0x1c0305c84. Non-external. Units above 0x7f are searched only in U+00A0-U+2122
// (the 32-bit test unit - 0x2123 >= 0xffffdf7d).
uint32_t __CFToWindowsGreek(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x2123 >= UINT32_C(0xffffdf7d)) {
        int low = 0;
        int high = 110;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = WindowsGreek_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)WindowsGreek_from_uni[middle][1];
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

// Native __CFFromWindowsGreek, 0x1c0305cfc. Non-external. The unit is stored before the U+FFFD test.
uint32_t __CFFromWindowsGreek(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = WindowsGreek_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToWindowsGreekPrecompose, 0x1c0305d24. Non-external. The WindowsGreek lookup is inlined with the
// range test on the low halfword ((uint16_t)(unit - 0x2123) >= 0xdf7d); a zero byte fails, and success
// returns the helper's used count.
OCFIndex __CFToWindowsGreekPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    OCFIndex used;
    uint16_t precomposed = __CFPrecomposeGreekCharacter(character, numChars, &used);
    uint8_t byte;
    if (precomposed <= 0x7f) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed - 0x2123) < 0xdf7d) {
            return 0;
        }
        int low = 0;
        int high = 110;
        for (;;) {
            int middle = low + ((high - low) >> 1);
            uint32_t key = WindowsGreek_from_uni[middle][0];
            if (precomposed >= key) {
                if (precomposed <= key) {
                    byte = (uint8_t)WindowsGreek_from_uni[middle][1];
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
    return used;
}

// A precomposition map entry (8 bytes): the base unit above the combining unit as a 32-bit key, the
// precomposed unit, and two zero bytes. PrecompositionEntry is a descriptive name.
typedef struct {
    uint32_t key;
    uint16_t value;
    uint16_t reserved;
} PrecompositionEntry;

// Native __CFGreekPrecompMap, 0x1c046ff5c (__TEXT,__const, 25 entries sorted by key). Non-external.
static const PrecompositionEntry __CFGreekPrecompMap[25] = {
    {0x003d0338, 0x2260, 0}, {0x00a80301, 0x0385, 0}, {0x03080301, 0x0344, 0}, {0x03910301, 0x0386, 0},
    {0x03950301, 0x0388, 0}, {0x03970301, 0x0389, 0}, {0x03990301, 0x038a, 0}, {0x03990308, 0x03aa, 0},
    {0x039f0301, 0x038c, 0}, {0x03a50301, 0x038e, 0}, {0x03a50308, 0x03ab, 0}, {0x03a90301, 0x038f, 0},
    {0x03b10301, 0x03ac, 0}, {0x03b50301, 0x03ad, 0}, {0x03b70301, 0x03ae, 0}, {0x03b90301, 0x03af, 0},
    {0x03b90308, 0x03ca, 0}, {0x03bf0301, 0x03cc, 0}, {0x03c50301, 0x03cd, 0}, {0x03c50308, 0x03cb, 0},
    {0x03c90301, 0x03ce, 0}, {0x03ca0301, 0x0390, 0}, {0x03cb0301, 0x03b0, 0}, {0x03d20301, 0x03d3, 0},
    {0x03d20308, 0x03d4, 0},
};

// Native __CFPrecomposeGreekCharacter, 0x1c0305e1c. Non-external. *usedChars is set to 2 first. A base unit
// up to 0x7f is answered by CFStringEncodingPrecomposeLatinCharacter(character, 2, NULL) (a tail call). A key
// (character[0] << 16) | character[1] above 0x03d20308, or one the binary search of __CFGreekPrecompMap does
// not find, answers U+FFFD. With at least three characters and U+0301 third, a found U+03CB becomes U+03B0
// and U+03CA becomes U+0390, with *usedChars 3.
uint16_t __CFPrecomposeGreekCharacter(const uint16_t *character, OCFIndex numChars, OCFIndex *usedChars) {
    *usedChars = 2;
    if (character[0] <= 0x7F) {
        return OCFStringEncodingPrecomposeLatinCharacter(character, 2, NULL);
    }
    uint32_t key = ((uint32_t)character[0] << 16) | character[1];
    if (key > 0x3D20308) {
        return 0xFFFD;
    }
    const PrecompositionEntry *map = __CFGreekPrecompMap;
    const PrecompositionEntry *low = map;
    const PrecompositionEntry *high = map + 24;
    do {
        const PrecompositionEntry *middle =
            low + ((uintptr_t)((const uint8_t *)high - (const uint8_t *)low) >> 4);
        if (key < middle->key) {
            high = middle - 1;
        } else if (key > middle->key) {
            low = middle + 1;
        } else {
            uint16_t value = middle->value;
            if (numChars < 3 || character[2] != 0x301) {
                return value;
            }
            if (value == 0x3CB) {
                *usedChars = 3;
                return 0x3B0;
            }
            if (value == 0x3CA) {
                *usedChars = 3;
                return 0x390;
            }
            return value;
        }
    } while (low <= high);
    return 0xFFFD;
}

// Native __CFConverterISOLatinGreek, 0x2227abf60 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterISOLatinGreek = {
    .toBytes.cheapEightBit = __CFToISOLatinGreek,
    .toUnicode.cheapEightBit = __CFFromISOLatinGreek,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 3,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToISOLatinGreekPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForGreek,
};

// Native __CFConverterMacGreek, 0x2227abfa8 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterMacGreek = {
    .toBytes.cheapEightBit = __CFToMacGreek,
    .toUnicode.cheapEightBit = __CFFromMacGreek,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 3,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToMacGreekPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForGreek,
};

// Native __CFConverterDOSGreek, 0x2227abff0 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterDOSGreek = {
    .toBytes.cheapEightBit = __CFToDOSGreek,
    .toUnicode.cheapEightBit = __CFFromDOSGreek,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToDOSGreekPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForGreek,
};

// Native __CFConverterWindowsGreek, 0x2227ac038 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterWindowsGreek = {
    .toBytes.cheapEightBit = __CFToWindowsGreek,
    .toUnicode.cheapEightBit = __CFFromWindowsGreek,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 3,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToWindowsGreekPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForGreek,
};
