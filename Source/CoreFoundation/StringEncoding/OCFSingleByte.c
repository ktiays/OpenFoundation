//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFSingleByte.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"

// Inferred organization: two address clusters of the iOS Simulator __text section are kept in this
// translation unit with their tables and converter records: the Latin1 group (0x1c01688cc-0x1c0168fe4: the
// Latin1 combining-character test and precomposition, ASCII, ISO Latin1, MacRoman, Windows Latin1 and
// NextStep Latin) and the Latin supplement group (0x1c019ef84-0x1c019ffd8: ISO Latin6-9, Mac and DOS
// Icelandic, DOS and Windows Baltic Rim, MacRoman Latin1, DOS Latin US and DOS Latin1);
// __CFIsValidCombiningCharacterForLatinSupp (0x1c019f0e0), inside the second group, belongs to another
// family, and __CFToUTF8 (0x1c0168fe4), which follows the first, to OCFUTF8.c. The native source path is not
// in the evidence. Definitions added by the converter-registry restoration use the short form: unless a
// definition says otherwise, iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified; iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.

// Native __CFLatin1CombiningCharBitmap, 0x1c0372a7c (__TEXT,__const, 3 32-bit words). Non-external.
static const uint32_t __CFLatin1CombiningCharBitmap[3] = {
    0xfbb94010,
    0x01800000,
    0x00000000,
};

// Native macRoman_from_uni, 0x1c0372a88 (__TEXT,__const, 129 entries of {Unicode, byte}). Non-external.
static const uint16_t macRoman_from_uni[129][2] = {
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
    {0x0131, 0x00f5}, {0x0152, 0x00ce}, {0x0153, 0x00cf}, {0x0178, 0x00d9}, {0x0192, 0x00c4},
    {0x02c6, 0x00f6}, {0x02c7, 0x00ff}, {0x02d8, 0x00f9}, {0x02d9, 0x00fa}, {0x02da, 0x00fb},
    {0x02db, 0x00fe}, {0x02dc, 0x00f7}, {0x02dd, 0x00fd}, {0x03a9, 0x00bd}, {0x03c0, 0x00b9},
    {0x2013, 0x00d0}, {0x2014, 0x00d1}, {0x2018, 0x00d4}, {0x2019, 0x00d5}, {0x201a, 0x00e2},
    {0x201c, 0x00d2}, {0x201d, 0x00d3}, {0x201e, 0x00e3}, {0x2020, 0x00a0}, {0x2021, 0x00e0},
    {0x2022, 0x00a5}, {0x2026, 0x00c9}, {0x2030, 0x00e4}, {0x2039, 0x00dc}, {0x203a, 0x00dd},
    {0x2044, 0x00da}, {0x20ac, 0x00db}, {0x2122, 0x00aa}, {0x2126, 0x00bd}, {0x2202, 0x00b6},
    {0x2206, 0x00c6}, {0x220f, 0x00b8}, {0x2211, 0x00b7}, {0x221a, 0x00c3}, {0x221e, 0x00b0},
    {0x222b, 0x00ba}, {0x2248, 0x00c5}, {0x2260, 0x00ad}, {0x2264, 0x00b2}, {0x2265, 0x00b3},
    {0x25ca, 0x00d7}, {0xf8ff, 0x00f0}, {0xfb01, 0x00de}, {0xfb02, 0x00df},
};

// Native cp1252_from_uni, iOS Simulator 24A434, 0x1c0372c8c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18082eee8, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; captured table bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint16_t cp1252_from_uni[27][2] = {
    {0x0152, 0x008c}, {0x0153, 0x009c}, {0x0160, 0x008a}, {0x0161, 0x009a}, {0x0178, 0x009f},
    {0x017d, 0x008e}, {0x017e, 0x009e}, {0x0192, 0x0083}, {0x02c6, 0x0088}, {0x02dc, 0x0098},
    {0x2013, 0x0096}, {0x2014, 0x0097}, {0x2018, 0x0091}, {0x2019, 0x0092}, {0x201a, 0x0082},
    {0x201c, 0x0093}, {0x201d, 0x0094}, {0x201e, 0x0084}, {0x2020, 0x0086}, {0x2021, 0x0087},
    {0x2022, 0x0095}, {0x2026, 0x0085}, {0x2030, 0x0089}, {0x2039, 0x008b}, {0x203a, 0x009b},
    {0x20ac, 0x0080}, {0x2122, 0x0099},
};

// Native cp1252_to_uni, iOS Simulator 24A434, 0x1c0372cf8,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18082ef54, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; captured table bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint16_t cp1252_to_uni[32] = {
    0x20ac, 0xfffd, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160,
    0x2039, 0x0152, 0xfffd, 0x017d, 0xfffd, 0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022,
    0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0xfffd, 0x017e, 0x0178,
};

// Native nextstep_from_tab, iOS Simulator 24A434, 0x1c0372d38,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18082ef94, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; captured table bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint16_t nextstep_from_tab[127][2] = {
    {0x00a0, 0x0080}, {0x00a1, 0x00a1}, {0x00a2, 0x00a2}, {0x00a3, 0x00a3}, {0x00a4, 0x00a8},
    {0x00a5, 0x00a5}, {0x00a6, 0x00b5}, {0x00a7, 0x00a7}, {0x00a8, 0x00c8}, {0x00a9, 0x00a0},
    {0x00aa, 0x00e3}, {0x00ab, 0x00ab}, {0x00ac, 0x00be}, {0x00ae, 0x00b0}, {0x00af, 0x00c5},
    {0x00b1, 0x00d1}, {0x00b2, 0x00c9}, {0x00b3, 0x00cc}, {0x00b4, 0x00c2}, {0x00b5, 0x009d},
    {0x00b6, 0x00b6}, {0x00b7, 0x00b4}, {0x00b8, 0x00cb}, {0x00b9, 0x00c0}, {0x00ba, 0x00eb},
    {0x00bb, 0x00bb}, {0x00bc, 0x00d2}, {0x00bd, 0x00d3}, {0x00be, 0x00d4}, {0x00bf, 0x00bf},
    {0x00c0, 0x0081}, {0x00c1, 0x0082}, {0x00c2, 0x0083}, {0x00c3, 0x0084}, {0x00c4, 0x0085},
    {0x00c5, 0x0086}, {0x00c6, 0x00e1}, {0x00c7, 0x0087}, {0x00c8, 0x0088}, {0x00c9, 0x0089},
    {0x00ca, 0x008a}, {0x00cb, 0x008b}, {0x00cc, 0x008c}, {0x00cd, 0x008d}, {0x00ce, 0x008e},
    {0x00cf, 0x008f}, {0x00d0, 0x0090}, {0x00d1, 0x0091}, {0x00d2, 0x0092}, {0x00d3, 0x0093},
    {0x00d4, 0x0094}, {0x00d5, 0x0095}, {0x00d6, 0x0096}, {0x00d7, 0x009e}, {0x00d8, 0x00e9},
    {0x00d9, 0x0097}, {0x00da, 0x0098}, {0x00db, 0x0099}, {0x00dc, 0x009a}, {0x00dd, 0x009b},
    {0x00de, 0x009c}, {0x00df, 0x00fb}, {0x00e0, 0x00d5}, {0x00e1, 0x00d6}, {0x00e2, 0x00d7},
    {0x00e3, 0x00d8}, {0x00e4, 0x00d9}, {0x00e5, 0x00da}, {0x00e6, 0x00f1}, {0x00e7, 0x00db},
    {0x00e8, 0x00dc}, {0x00e9, 0x00dd}, {0x00ea, 0x00de}, {0x00eb, 0x00df}, {0x00ec, 0x00e0},
    {0x00ed, 0x00e2}, {0x00ee, 0x00e4}, {0x00ef, 0x00e5}, {0x00f0, 0x00e6}, {0x00f1, 0x00e7},
    {0x00f2, 0x00ec}, {0x00f3, 0x00ed}, {0x00f4, 0x00ee}, {0x00f5, 0x00ef}, {0x00f6, 0x00f0},
    {0x00f7, 0x009f}, {0x00f8, 0x00f9}, {0x00f9, 0x00f2}, {0x00fa, 0x00f3}, {0x00fb, 0x00f4},
    {0x00fc, 0x00f6}, {0x00fd, 0x00f7}, {0x00fe, 0x00fc}, {0x00ff, 0x00fd}, {0x0131, 0x00f5},
    {0x0141, 0x00e8}, {0x0142, 0x00f8}, {0x0152, 0x00ea}, {0x0153, 0x00fa}, {0x0192, 0x00a6},
    {0x02c6, 0x00c3}, {0x02c7, 0x00cf}, {0x02cb, 0x00c1}, {0x02d8, 0x00c6}, {0x02d9, 0x00c7},
    {0x02da, 0x00ca}, {0x02db, 0x00ce}, {0x02dc, 0x00c4}, {0x02dd, 0x00cd}, {0x2013, 0x00b1},
    {0x2014, 0x00d0}, {0x2019, 0x00a9}, {0x201a, 0x00b8}, {0x201c, 0x00aa}, {0x201d, 0x00ba},
    {0x201e, 0x00b9}, {0x2020, 0x00b2}, {0x2021, 0x00b3}, {0x2022, 0x00b7}, {0x2026, 0x00bc},
    {0x2030, 0x00bd}, {0x2039, 0x00ac}, {0x203a, 0x00ad}, {0x2044, 0x00a4}, {0xfb01, 0x00ae},
    {0xfb02, 0x00af}, {0xfffd, 0x00ff},
};

// Native NSToPrecompUnicodeTable, iOS Simulator 24A434, 0x1c0372f34,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18082f190, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; captured table bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint16_t NSToPrecompUnicodeTable[128] = {
    0x00a0, 0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc,
    0x00cd, 0x00ce, 0x00cf, 0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d9, 0x00da, 0x00db,
    0x00dc, 0x00dd, 0x00de, 0x00b5, 0x00d7, 0x00f7, 0x00a9, 0x00a1, 0x00a2, 0x00a3, 0x2044, 0x00a5, 0x0192,
    0x00a7, 0x00a4, 0x2019, 0x201c, 0x00ab, 0x2039, 0x203a, 0xfb01, 0xfb02, 0x00ae, 0x2013, 0x2020, 0x2021,
    0x00b7, 0x00a6, 0x00b6, 0x2022, 0x201a, 0x201e, 0x201d, 0x00bb, 0x2026, 0x2030, 0x00ac, 0x00bf, 0x00b9,
    0x02cb, 0x00b4, 0x02c6, 0x02dc, 0x00af, 0x02d8, 0x02d9, 0x00a8, 0x00b2, 0x02da, 0x00b8, 0x00b3, 0x02dd,
    0x02db, 0x02c7, 0x2014, 0x00b1, 0x00bc, 0x00bd, 0x00be, 0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5,
    0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00c6, 0x00ed, 0x00aa, 0x00ee, 0x00ef, 0x00f0, 0x00f1,
    0x0141, 0x00d8, 0x0152, 0x00ba, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00e6, 0x00f9, 0x00fa, 0x00fb,
    0x0131, 0x00fc, 0x00fd, 0x0142, 0x00f8, 0x0153, 0x00df, 0x00fe, 0x00ff, 0xfffd, 0xfffd,
};

// Native ISOLatin6_from_uni, 0x1c0373670 (__TEXT,__const, 95 entries of {Unicode, byte}). Non-external.
static const uint16_t ISOLatin6_from_uni[95][2] = {
    {0x00a7, 0x00a7}, {0x00ad, 0x00ad}, {0x00b0, 0x00b0}, {0x00b7, 0x00b7}, {0x00c1, 0x00c1},
    {0x00c2, 0x00c2}, {0x00c3, 0x00c3}, {0x00c4, 0x00c4}, {0x00c5, 0x00c5}, {0x00c6, 0x00c6},
    {0x00c9, 0x00c9}, {0x00cb, 0x00cb}, {0x00cd, 0x00cd}, {0x00ce, 0x00ce}, {0x00cf, 0x00cf},
    {0x00d0, 0x00d0}, {0x00d3, 0x00d3}, {0x00d4, 0x00d4}, {0x00d5, 0x00d5}, {0x00d6, 0x00d6},
    {0x00d8, 0x00d8}, {0x00da, 0x00da}, {0x00db, 0x00db}, {0x00dc, 0x00dc}, {0x00dd, 0x00dd},
    {0x00de, 0x00de}, {0x00df, 0x00df}, {0x00e1, 0x00e1}, {0x00e2, 0x00e2}, {0x00e3, 0x00e3},
    {0x00e4, 0x00e4}, {0x00e5, 0x00e5}, {0x00e6, 0x00e6}, {0x00e9, 0x00e9}, {0x00eb, 0x00eb},
    {0x00ed, 0x00ed}, {0x00ee, 0x00ee}, {0x00ef, 0x00ef}, {0x00f0, 0x00f0}, {0x00f3, 0x00f3},
    {0x00f4, 0x00f4}, {0x00f5, 0x00f5}, {0x00f6, 0x00f6}, {0x00f8, 0x00f8}, {0x00fa, 0x00fa},
    {0x00fb, 0x00fb}, {0x00fc, 0x00fc}, {0x00fd, 0x00fd}, {0x00fe, 0x00fe}, {0x0100, 0x00c0},
    {0x0101, 0x00e0}, {0x0104, 0x00a1}, {0x0105, 0x00b1}, {0x010c, 0x00c8}, {0x010d, 0x00e8},
    {0x0110, 0x00a9}, {0x0111, 0x00b9}, {0x0112, 0x00a2}, {0x0113, 0x00b2}, {0x0116, 0x00cc},
    {0x0117, 0x00ec}, {0x0118, 0x00ca}, {0x0119, 0x00ea}, {0x0122, 0x00a3}, {0x0123, 0x00b3},
    {0x0128, 0x00a5}, {0x0129, 0x00b5}, {0x012a, 0x00a4}, {0x012b, 0x00b4}, {0x012e, 0x00c7},
    {0x012f, 0x00e7}, {0x0136, 0x00a6}, {0x0137, 0x00b6}, {0x0138, 0x00ff}, {0x013b, 0x00a8},
    {0x013c, 0x00b8}, {0x0145, 0x00d1}, {0x0146, 0x00f1}, {0x014a, 0x00af}, {0x014b, 0x00bf},
    {0x014c, 0x00d2}, {0x014d, 0x00f2}, {0x0160, 0x00aa}, {0x0161, 0x00ba}, {0x0166, 0x00ab},
    {0x0167, 0x00bb}, {0x0168, 0x00d7}, {0x0169, 0x00f7}, {0x016a, 0x00ae}, {0x016b, 0x00be},
    {0x0172, 0x00d9}, {0x0173, 0x00f9}, {0x017d, 0x00ac}, {0x017e, 0x00bc}, {0x2015, 0x00bd},
};

// Native ISOLatin6_to_uni, 0x1c03737ec (__TEXT,__const, 95 UTF-16 units). Non-external.
static const uint16_t ISOLatin6_to_uni[95] = {
    0x0104, 0x0112, 0x0122, 0x012a, 0x0128, 0x0136, 0x00a7, 0x013b, 0x0110, 0x0160, 0x0166, 0x017d,
    0x00ad, 0x016a, 0x014a, 0x00b0, 0x0105, 0x0113, 0x0123, 0x012b, 0x0129, 0x0137, 0x00b7, 0x013c,
    0x0111, 0x0161, 0x0167, 0x017e, 0x2015, 0x016b, 0x014b, 0x0100, 0x00c1, 0x00c2, 0x00c3, 0x00c4,
    0x00c5, 0x00c6, 0x012e, 0x010c, 0x00c9, 0x0118, 0x00cb, 0x0116, 0x00cd, 0x00ce, 0x00cf, 0x00d0,
    0x0145, 0x014c, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x0168, 0x00d8, 0x0172, 0x00da, 0x00db, 0x00dc,
    0x00dd, 0x00de, 0x00df, 0x0101, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x012f, 0x010d,
    0x00e9, 0x0119, 0x00eb, 0x0117, 0x00ed, 0x00ee, 0x00ef, 0x00f0, 0x0146, 0x014d, 0x00f3, 0x00f4,
    0x00f5, 0x00f6, 0x0169, 0x00f8, 0x0173, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x0138,
};

// Native ISOLatin7_from_uni, 0x1c03738b8 (__TEXT,__const, 95 entries of {Unicode, byte}). Non-external.
static const uint16_t ISOLatin7_from_uni[95][2] = {
    {0x00a2, 0x00a2}, {0x00a3, 0x00a3}, {0x00a4, 0x00a4}, {0x00a6, 0x00a6}, {0x00a7, 0x00a7},
    {0x00a9, 0x00a9}, {0x00ab, 0x00ab}, {0x00ac, 0x00ac}, {0x00ad, 0x00ad}, {0x00ae, 0x00ae},
    {0x00b0, 0x00b0}, {0x00b1, 0x00b1}, {0x00b2, 0x00b2}, {0x00b3, 0x00b3}, {0x00b4, 0x00ff},
    {0x00b5, 0x00b5}, {0x00b6, 0x00b6}, {0x00b7, 0x00b7}, {0x00b9, 0x00b9}, {0x00bb, 0x00bb},
    {0x00bc, 0x00bc}, {0x00bd, 0x00bd}, {0x00be, 0x00be}, {0x00c4, 0x00c4}, {0x00c5, 0x00c5},
    {0x00c6, 0x00af}, {0x00c9, 0x00c9}, {0x00d3, 0x00d3}, {0x00d5, 0x00d5}, {0x00d6, 0x00d6},
    {0x00d7, 0x00d7}, {0x00d8, 0x00a8}, {0x00dc, 0x00dc}, {0x00df, 0x00df}, {0x00e4, 0x00e4},
    {0x00e5, 0x00e5}, {0x00e6, 0x00bf}, {0x00e9, 0x00e9}, {0x00f3, 0x00f3}, {0x00f5, 0x00f5},
    {0x00f6, 0x00f6}, {0x00f7, 0x00f7}, {0x00f8, 0x00b8}, {0x00fc, 0x00fc}, {0x0100, 0x00c2},
    {0x0101, 0x00e2}, {0x0104, 0x00c0}, {0x0105, 0x00e0}, {0x0106, 0x00c3}, {0x0107, 0x00e3},
    {0x010c, 0x00c8}, {0x010d, 0x00e8}, {0x0112, 0x00c7}, {0x0113, 0x00e7}, {0x0116, 0x00cb},
    {0x0117, 0x00eb}, {0x0118, 0x00c6}, {0x0119, 0x00e6}, {0x0122, 0x00cc}, {0x0123, 0x00ec},
    {0x012a, 0x00ce}, {0x012b, 0x00ee}, {0x012e, 0x00c1}, {0x012f, 0x00e1}, {0x0136, 0x00cd},
    {0x0137, 0x00ed}, {0x013b, 0x00cf}, {0x013c, 0x00ef}, {0x0141, 0x00d9}, {0x0142, 0x00f9},
    {0x0143, 0x00d1}, {0x0144, 0x00f1}, {0x0145, 0x00d2}, {0x0146, 0x00f2}, {0x014c, 0x00d4},
    {0x014d, 0x00f4}, {0x0156, 0x00aa}, {0x0157, 0x00ba}, {0x015a, 0x00da}, {0x015b, 0x00fa},
    {0x0160, 0x00d0}, {0x0161, 0x00f0}, {0x016a, 0x00db}, {0x016b, 0x00fb}, {0x0172, 0x00d8},
    {0x0173, 0x00f8}, {0x0179, 0x00ca}, {0x017a, 0x00ea}, {0x017b, 0x00dd}, {0x017c, 0x00fd},
    {0x017d, 0x00de}, {0x017e, 0x00fe}, {0x201c, 0x00b4}, {0x201d, 0x00a1}, {0x201e, 0x00a5},
};

// Native ISOLatin7_to_uni, 0x1c0373a34 (__TEXT,__const, 95 UTF-16 units). Non-external.
static const uint16_t ISOLatin7_to_uni[95] = {
    0x201d, 0x00a2, 0x00a3, 0x00a4, 0x201e, 0x00a6, 0x00a7, 0x00d8, 0x00a9, 0x0156, 0x00ab, 0x00ac,
    0x00ad, 0x00ae, 0x00c6, 0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x201c, 0x00b5, 0x00b6, 0x00b7, 0x00f8,
    0x00b9, 0x0157, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00e6, 0x0104, 0x012e, 0x0100, 0x0106, 0x00c4,
    0x00c5, 0x0118, 0x0112, 0x010c, 0x00c9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012a, 0x013b, 0x0160,
    0x0143, 0x0145, 0x00d3, 0x014c, 0x00d5, 0x00d6, 0x00d7, 0x0172, 0x0141, 0x015a, 0x016a, 0x00dc,
    0x017b, 0x017d, 0x00df, 0x0105, 0x012f, 0x0101, 0x0107, 0x00e4, 0x00e5, 0x0119, 0x0113, 0x010d,
    0x00e9, 0x017a, 0x0117, 0x0123, 0x0137, 0x012b, 0x013c, 0x0161, 0x0144, 0x0146, 0x00f3, 0x014d,
    0x00f5, 0x00f6, 0x00f7, 0x0173, 0x0142, 0x015b, 0x016b, 0x00fc, 0x017c, 0x017e, 0x00b4,
};

// Native ISOLatin8_from_uni, 0x1c0373af2 (__TEXT,__const, 95 entries of {Unicode, byte}). Non-external.
static const uint16_t ISOLatin8_from_uni[95][2] = {
    {0x00a3, 0x00a3}, {0x00a7, 0x00a7}, {0x00a9, 0x00a9}, {0x00ad, 0x00ad}, {0x00ae, 0x00ae},
    {0x00b6, 0x00b6}, {0x00c0, 0x00c0}, {0x00c1, 0x00c1}, {0x00c2, 0x00c2}, {0x00c3, 0x00c3},
    {0x00c4, 0x00c4}, {0x00c5, 0x00c5}, {0x00c6, 0x00c6}, {0x00c7, 0x00c7}, {0x00c8, 0x00c8},
    {0x00c9, 0x00c9}, {0x00ca, 0x00ca}, {0x00cb, 0x00cb}, {0x00cc, 0x00cc}, {0x00cd, 0x00cd},
    {0x00ce, 0x00ce}, {0x00cf, 0x00cf}, {0x00d1, 0x00d1}, {0x00d2, 0x00d2}, {0x00d3, 0x00d3},
    {0x00d4, 0x00d4}, {0x00d5, 0x00d5}, {0x00d6, 0x00d6}, {0x00d8, 0x00d8}, {0x00d9, 0x00d9},
    {0x00da, 0x00da}, {0x00db, 0x00db}, {0x00dc, 0x00dc}, {0x00dd, 0x00dd}, {0x00df, 0x00df},
    {0x00e0, 0x00e0}, {0x00e1, 0x00e1}, {0x00e2, 0x00e2}, {0x00e3, 0x00e3}, {0x00e4, 0x00e4},
    {0x00e5, 0x00e5}, {0x00e6, 0x00e6}, {0x00e7, 0x00e7}, {0x00e8, 0x00e8}, {0x00e9, 0x00e9},
    {0x00ea, 0x00ea}, {0x00eb, 0x00eb}, {0x00ec, 0x00ec}, {0x00ed, 0x00ed}, {0x00ee, 0x00ee},
    {0x00ef, 0x00ef}, {0x00f1, 0x00f1}, {0x00f2, 0x00f2}, {0x00f3, 0x00f3}, {0x00f4, 0x00f4},
    {0x00f5, 0x00f5}, {0x00f6, 0x00f6}, {0x00f8, 0x00f8}, {0x00f9, 0x00f9}, {0x00fa, 0x00fa},
    {0x00fb, 0x00fb}, {0x00fc, 0x00fc}, {0x00fd, 0x00fd}, {0x00ff, 0x00ff}, {0x010a, 0x00a4},
    {0x010b, 0x00a5}, {0x0120, 0x00b2}, {0x0121, 0x00b3}, {0x0174, 0x00d0}, {0x0175, 0x00f0},
    {0x0176, 0x00de}, {0x0177, 0x00fe}, {0x0178, 0x00af}, {0x1e02, 0x00a1}, {0x1e03, 0x00a2},
    {0x1e0a, 0x00a6}, {0x1e0b, 0x00ab}, {0x1e1e, 0x00b0}, {0x1e1f, 0x00b1}, {0x1e40, 0x00b4},
    {0x1e41, 0x00b5}, {0x1e56, 0x00b7}, {0x1e57, 0x00b9}, {0x1e60, 0x00bb}, {0x1e61, 0x00bf},
    {0x1e6a, 0x00d7}, {0x1e6b, 0x00f7}, {0x1e80, 0x00a8}, {0x1e81, 0x00b8}, {0x1e82, 0x00aa},
    {0x1e83, 0x00ba}, {0x1e84, 0x00bd}, {0x1e85, 0x00be}, {0x1ef2, 0x00ac}, {0x1ef3, 0x00bc},
};

// Native ISOLatin8_to_uni, 0x1c0373c6e (__TEXT,__const, 95 UTF-16 units). Non-external.
static const uint16_t ISOLatin8_to_uni[95] = {
    0x1e02, 0x1e03, 0x00a3, 0x010a, 0x010b, 0x1e0a, 0x00a7, 0x1e80, 0x00a9, 0x1e82, 0x1e0b, 0x1ef2,
    0x00ad, 0x00ae, 0x0178, 0x1e1e, 0x1e1f, 0x0120, 0x0121, 0x1e40, 0x1e41, 0x00b6, 0x1e56, 0x1e81,
    0x1e57, 0x1e83, 0x1e60, 0x1ef3, 0x1e84, 0x1e85, 0x1e61, 0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4,
    0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf, 0x0174,
    0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x1e6a, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc,
    0x00dd, 0x0176, 0x00df, 0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8,
    0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 0x0175, 0x00f1, 0x00f2, 0x00f3, 0x00f4,
    0x00f5, 0x00f6, 0x1e6b, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x0177, 0x00ff,
};

// Native ISOLatin9_from_uni, 0x1c0373d2c (__TEXT,__const, 92 entries of {Unicode, byte}). Non-external.
static const uint16_t ISOLatin9_from_uni[92][2] = {
    {0x00a5, 0x00a5}, {0x00a7, 0x00a7}, {0x00a9, 0x00a9}, {0x00aa, 0x00aa}, {0x00ab, 0x00ab},
    {0x00ac, 0x00ac}, {0x00ad, 0x00ad}, {0x00ae, 0x00ae}, {0x00af, 0x00af}, {0x00b0, 0x00b0},
    {0x00b1, 0x00b1}, {0x00b2, 0x00b2}, {0x00b3, 0x00b3}, {0x00b5, 0x00b5}, {0x00b6, 0x00b6},
    {0x00b7, 0x00b7}, {0x00b9, 0x00b9}, {0x00ba, 0x00ba}, {0x00bb, 0x00bb}, {0x00bf, 0x00bf},
    {0x00c0, 0x00c0}, {0x00c1, 0x00c1}, {0x00c2, 0x00c2}, {0x00c3, 0x00c3}, {0x00c4, 0x00c4},
    {0x00c5, 0x00c5}, {0x00c6, 0x00c6}, {0x00c7, 0x00c7}, {0x00c8, 0x00c8}, {0x00c9, 0x00c9},
    {0x00ca, 0x00ca}, {0x00cb, 0x00cb}, {0x00cc, 0x00cc}, {0x00cd, 0x00cd}, {0x00ce, 0x00ce},
    {0x00cf, 0x00cf}, {0x00d0, 0x00d0}, {0x00d1, 0x00d1}, {0x00d2, 0x00d2}, {0x00d3, 0x00d3},
    {0x00d4, 0x00d4}, {0x00d5, 0x00d5}, {0x00d6, 0x00d6}, {0x00d7, 0x00d7}, {0x00d8, 0x00d8},
    {0x00d9, 0x00d9}, {0x00da, 0x00da}, {0x00db, 0x00db}, {0x00dc, 0x00dc}, {0x00dd, 0x00dd},
    {0x00de, 0x00de}, {0x00df, 0x00df}, {0x00e0, 0x00e0}, {0x00e1, 0x00e1}, {0x00e2, 0x00e2},
    {0x00e3, 0x00e3}, {0x00e4, 0x00e4}, {0x00e5, 0x00e5}, {0x00e6, 0x00e6}, {0x00e7, 0x00e7},
    {0x00e8, 0x00e8}, {0x00e9, 0x00e9}, {0x00ea, 0x00ea}, {0x00eb, 0x00eb}, {0x00ec, 0x00ec},
    {0x00ed, 0x00ed}, {0x00ee, 0x00ee}, {0x00ef, 0x00ef}, {0x00f0, 0x00f0}, {0x00f1, 0x00f1},
    {0x00f2, 0x00f2}, {0x00f3, 0x00f3}, {0x00f4, 0x00f4}, {0x00f5, 0x00f5}, {0x00f6, 0x00f6},
    {0x00f7, 0x00f7}, {0x00f8, 0x00f8}, {0x00f9, 0x00f9}, {0x00fa, 0x00fa}, {0x00fb, 0x00fb},
    {0x00fc, 0x00fc}, {0x00fd, 0x00fd}, {0x00fe, 0x00fe}, {0x00ff, 0x00ff}, {0x0152, 0x00bc},
    {0x0153, 0x00bd}, {0x0160, 0x00a6}, {0x0161, 0x00a8}, {0x0178, 0x00be}, {0x017d, 0x00b4},
    {0x017e, 0x00b8}, {0x20ac, 0x00a4},
};

// Native ISOLatin9_to_uni, 0x1c0373e9c (__TEXT,__const, 92 UTF-16 units). Non-external.
static const uint16_t ISOLatin9_to_uni[92] = {
    0x20ac, 0x00a5, 0x0160, 0x00a7, 0x0161, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x017d, 0x00b5, 0x00b6, 0x00b7, 0x017e, 0x00b9, 0x00ba, 0x00bb,
    0x0152, 0x0153, 0x0178, 0x00bf, 0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
    0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf, 0x00d0, 0x00d1, 0x00d2, 0x00d3,
    0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb,
    0x00ec, 0x00ed, 0x00ee, 0x00ef, 0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
    0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff,
};

// Native MacIcelandic_from_uni, 0x1c0373f54 (__TEXT,__const, 128 entries of {Unicode, byte}). Non-external.
static const uint16_t MacIcelandic_from_uni[128][2] = {
    {0x00a0, 0x00ca}, {0x00a1, 0x00c1}, {0x00a2, 0x00a2}, {0x00a3, 0x00a3}, {0x00a5, 0x00b4},
    {0x00a7, 0x00a4}, {0x00a8, 0x00ac}, {0x00a9, 0x00a9}, {0x00aa, 0x00bb}, {0x00ab, 0x00c7},
    {0x00ac, 0x00c2}, {0x00ae, 0x00a8}, {0x00af, 0x00f8}, {0x00b0, 0x00a1}, {0x00b1, 0x00b1},
    {0x00b4, 0x00ab}, {0x00b5, 0x00b5}, {0x00b6, 0x00a6}, {0x00b7, 0x00e1}, {0x00b8, 0x00fc},
    {0x00ba, 0x00bc}, {0x00bb, 0x00c8}, {0x00bf, 0x00c0}, {0x00c0, 0x00cb}, {0x00c1, 0x00e7},
    {0x00c2, 0x00e5}, {0x00c3, 0x00cc}, {0x00c4, 0x0080}, {0x00c5, 0x0081}, {0x00c6, 0x00ae},
    {0x00c7, 0x0082}, {0x00c8, 0x00e9}, {0x00c9, 0x0083}, {0x00ca, 0x00e6}, {0x00cb, 0x00e8},
    {0x00cc, 0x00ed}, {0x00cd, 0x00ea}, {0x00ce, 0x00eb}, {0x00cf, 0x00ec}, {0x00d0, 0x00dc},
    {0x00d1, 0x0084}, {0x00d2, 0x00f1}, {0x00d3, 0x00ee}, {0x00d4, 0x00ef}, {0x00d5, 0x00cd},
    {0x00d6, 0x0085}, {0x00d8, 0x00af}, {0x00d9, 0x00f4}, {0x00da, 0x00f2}, {0x00db, 0x00f3},
    {0x00dc, 0x0086}, {0x00dd, 0x00a0}, {0x00de, 0x00de}, {0x00df, 0x00a7}, {0x00e0, 0x0088},
    {0x00e1, 0x0087}, {0x00e2, 0x0089}, {0x00e3, 0x008b}, {0x00e4, 0x008a}, {0x00e5, 0x008c},
    {0x00e6, 0x00be}, {0x00e7, 0x008d}, {0x00e8, 0x008f}, {0x00e9, 0x008e}, {0x00ea, 0x0090},
    {0x00eb, 0x0091}, {0x00ec, 0x0093}, {0x00ed, 0x0092}, {0x00ee, 0x0094}, {0x00ef, 0x0095},
    {0x00f0, 0x00dd}, {0x00f1, 0x0096}, {0x00f2, 0x0098}, {0x00f3, 0x0097}, {0x00f4, 0x0099},
    {0x00f5, 0x009b}, {0x00f6, 0x009a}, {0x00f7, 0x00d6}, {0x00f8, 0x00bf}, {0x00f9, 0x009d},
    {0x00fa, 0x009c}, {0x00fb, 0x009e}, {0x00fc, 0x009f}, {0x00fd, 0x00e0}, {0x00fe, 0x00df},
    {0x00ff, 0x00d8}, {0x0131, 0x00f5}, {0x0152, 0x00ce}, {0x0153, 0x00cf}, {0x0178, 0x00d9},
    {0x0192, 0x00c4}, {0x02c6, 0x00f6}, {0x02c7, 0x00ff}, {0x02d8, 0x00f9}, {0x02d9, 0x00fa},
    {0x02da, 0x00fb}, {0x02db, 0x00fe}, {0x02dc, 0x00f7}, {0x02dd, 0x00fd}, {0x03a9, 0x00bd},
    {0x03c0, 0x00b9}, {0x2013, 0x00d0}, {0x2014, 0x00d1}, {0x2018, 0x00d4}, {0x2019, 0x00d5},
    {0x201a, 0x00e2}, {0x201c, 0x00d2}, {0x201d, 0x00d3}, {0x201e, 0x00e3}, {0x2022, 0x00a5},
    {0x2026, 0x00c9}, {0x2030, 0x00e4}, {0x2044, 0x00da}, {0x20ac, 0x00db}, {0x2122, 0x00aa},
    {0x2202, 0x00b6}, {0x2206, 0x00c6}, {0x220f, 0x00b8}, {0x2211, 0x00b7}, {0x221a, 0x00c3},
    {0x221e, 0x00b0}, {0x222b, 0x00ba}, {0x2248, 0x00c5}, {0x2260, 0x00ad}, {0x2264, 0x00b2},
    {0x2265, 0x00b3}, {0x25ca, 0x00d7}, {0xf8ff, 0x00f0},
};

// Native MacIcelandic_to_uni, 0x1c0374154 (__TEXT,__const, 128 UTF-16 units). Non-external.
static const uint16_t MacIcelandic_to_uni[128] = {
    0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5,
    0x00e7, 0x00e9, 0x00e8, 0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 0x00f2, 0x00f4,
    0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 0x00dd, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6,
    0x00df, 0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8, 0x221e, 0x00b1, 0x2264, 0x2265,
    0x00a5, 0x00b5, 0x2202, 0x2211, 0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8, 0x00bf,
    0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5,
    0x0152, 0x0153, 0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 0x00ff, 0x0178, 0x2044,
    0x20ac, 0x00d0, 0x00f0, 0x00de, 0x00fe, 0x00fd, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1,
    0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4, 0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9,
    0x0131, 0x02c6, 0x02dc, 0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7,
};

// Native DOSIcelandic_from_uni, 0x1c0374254 (__TEXT,__const, 128 entries of {Unicode, byte}). Non-external.
static const uint16_t DOSIcelandic_from_uni[128][2] = {
    {0x00a0, 0x00ff}, {0x00a1, 0x00ad}, {0x00a3, 0x009c}, {0x00ab, 0x00ae}, {0x00ac, 0x00aa},
    {0x00b0, 0x00f8}, {0x00b1, 0x00f1}, {0x00b2, 0x00fd}, {0x00b5, 0x00e6}, {0x00b7, 0x00fa},
    {0x00bb, 0x00af}, {0x00bc, 0x00ac}, {0x00bd, 0x00ab}, {0x00bf, 0x00a8}, {0x00c1, 0x00a4},
    {0x00c4, 0x008e}, {0x00c5, 0x008f}, {0x00c6, 0x0092}, {0x00c7, 0x0080}, {0x00c9, 0x0090},
    {0x00cd, 0x00a5}, {0x00d0, 0x008b}, {0x00d3, 0x00a6}, {0x00d6, 0x0099}, {0x00d8, 0x009d},
    {0x00da, 0x00a7}, {0x00dc, 0x009a}, {0x00dd, 0x0097}, {0x00de, 0x008d}, {0x00df, 0x00e1},
    {0x00e0, 0x0085}, {0x00e1, 0x00a0}, {0x00e2, 0x0083}, {0x00e4, 0x0084}, {0x00e5, 0x0086},
    {0x00e6, 0x0091}, {0x00e7, 0x0087}, {0x00e8, 0x008a}, {0x00e9, 0x0082}, {0x00ea, 0x0088},
    {0x00eb, 0x0089}, {0x00ed, 0x00a1}, {0x00f0, 0x008c}, {0x00f3, 0x00a2}, {0x00f4, 0x0093},
    {0x00f6, 0x0094}, {0x00f7, 0x00f6}, {0x00f8, 0x009b}, {0x00fa, 0x00a3}, {0x00fb, 0x0096},
    {0x00fc, 0x0081}, {0x00fd, 0x0098}, {0x00fe, 0x0095}, {0x0192, 0x009f}, {0x0393, 0x00e2},
    {0x0398, 0x00e9}, {0x03a3, 0x00e4}, {0x03a6, 0x00e8}, {0x03a9, 0x00ea}, {0x03b1, 0x00e0},
    {0x03b4, 0x00eb}, {0x03b5, 0x00ee}, {0x03c0, 0x00e3}, {0x03c3, 0x00e5}, {0x03c4, 0x00e7},
    {0x03c6, 0x00ed}, {0x207f, 0x00fc}, {0x20a7, 0x009e}, {0x2219, 0x00f9}, {0x221a, 0x00fb},
    {0x221e, 0x00ec}, {0x2229, 0x00ef}, {0x2248, 0x00f7}, {0x2261, 0x00f0}, {0x2264, 0x00f3},
    {0x2265, 0x00f2}, {0x2310, 0x00a9}, {0x2320, 0x00f4}, {0x2321, 0x00f5}, {0x2500, 0x00c4},
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

// Native DOSBalticRim_from_uni, 0x1c0374454 (__TEXT,__const, 128 entries of {Unicode, byte}). Non-external.
static const uint16_t DOSBalticRim_from_uni[128][2] = {
    {0x00a0, 0x00ff}, {0x00a2, 0x0096}, {0x00a3, 0x009c}, {0x00a4, 0x009f}, {0x00a6, 0x00a7},
    {0x00a7, 0x00f5}, {0x00a9, 0x00a8}, {0x00ab, 0x00ae}, {0x00ac, 0x00aa}, {0x00ad, 0x00f0},
    {0x00ae, 0x00a9}, {0x00b0, 0x00f8}, {0x00b1, 0x00f1}, {0x00b2, 0x00fd}, {0x00b3, 0x00fc},
    {0x00b5, 0x00e6}, {0x00b6, 0x00f4}, {0x00b7, 0x00fa}, {0x00b9, 0x00fb}, {0x00bb, 0x00af},
    {0x00bc, 0x00ac}, {0x00bd, 0x00ab}, {0x00be, 0x00f3}, {0x00c4, 0x008e}, {0x00c5, 0x008f},
    {0x00c6, 0x0092}, {0x00c9, 0x0090}, {0x00d3, 0x00e0}, {0x00d5, 0x00e5}, {0x00d6, 0x0099},
    {0x00d7, 0x009e}, {0x00d8, 0x009d}, {0x00dc, 0x009a}, {0x00df, 0x00e1}, {0x00e4, 0x0084},
    {0x00e5, 0x0086}, {0x00e6, 0x0091}, {0x00e9, 0x0082}, {0x00f3, 0x00a2}, {0x00f5, 0x00e4},
    {0x00f6, 0x0094}, {0x00f7, 0x00f6}, {0x00f8, 0x009b}, {0x00fc, 0x0081}, {0x0100, 0x00a0},
    {0x0101, 0x0083}, {0x0104, 0x00b5}, {0x0105, 0x00d0}, {0x0106, 0x0080}, {0x0107, 0x0087},
    {0x010c, 0x00b6}, {0x010d, 0x00d1}, {0x0112, 0x00ed}, {0x0113, 0x0089}, {0x0116, 0x00b8},
    {0x0117, 0x00d3}, {0x0118, 0x00b7}, {0x0119, 0x00d2}, {0x0122, 0x0095}, {0x0123, 0x0085},
    {0x012a, 0x00a1}, {0x012b, 0x008c}, {0x012e, 0x00bd}, {0x012f, 0x00d4}, {0x0136, 0x00e8},
    {0x0137, 0x00e9}, {0x013b, 0x00ea}, {0x013c, 0x00eb}, {0x0141, 0x00ad}, {0x0142, 0x0088},
    {0x0143, 0x00e3}, {0x0144, 0x00e7}, {0x0145, 0x00ee}, {0x0146, 0x00ec}, {0x014c, 0x00e2},
    {0x014d, 0x0093}, {0x0156, 0x008a}, {0x0157, 0x008b}, {0x015a, 0x0097}, {0x015b, 0x0098},
    {0x0160, 0x00be}, {0x0161, 0x00d5}, {0x016a, 0x00c7}, {0x016b, 0x00d7}, {0x0172, 0x00c6},
    {0x0173, 0x00d6}, {0x0179, 0x008d}, {0x017a, 0x00a5}, {0x017b, 0x00a3}, {0x017c, 0x00a4},
    {0x017d, 0x00cf}, {0x017e, 0x00d8}, {0x2019, 0x00ef}, {0x201c, 0x00f2}, {0x201d, 0x00a6},
    {0x201e, 0x00f7}, {0x2219, 0x00f9}, {0x2500, 0x00c4}, {0x2502, 0x00b3}, {0x250c, 0x00da},
    {0x2510, 0x00bf}, {0x2514, 0x00c0}, {0x2518, 0x00d9}, {0x251c, 0x00c3}, {0x2524, 0x00b4},
    {0x252c, 0x00c2}, {0x2534, 0x00c1}, {0x253c, 0x00c5}, {0x2550, 0x00cd}, {0x2551, 0x00ba},
    {0x2554, 0x00c9}, {0x2557, 0x00bb}, {0x255a, 0x00c8}, {0x255d, 0x00bc}, {0x2560, 0x00cc},
    {0x2563, 0x00b9}, {0x2566, 0x00cb}, {0x2569, 0x00ca}, {0x256c, 0x00ce}, {0x2580, 0x00df},
    {0x2584, 0x00dc}, {0x2588, 0x00db}, {0x258c, 0x00dd}, {0x2590, 0x00de}, {0x2591, 0x00b0},
    {0x2592, 0x00b1}, {0x2593, 0x00b2}, {0x25a0, 0x00fe},
};

// Native DOSBalticRim_to_uni, 0x1c0374654 (__TEXT,__const, 128 UTF-16 units). Non-external.
static const uint16_t DOSBalticRim_to_uni[128] = {
    0x0106, 0x00fc, 0x00e9, 0x0101, 0x00e4, 0x0123, 0x00e5, 0x0107, 0x0142, 0x0113, 0x0156, 0x0157, 0x012b,
    0x0179, 0x00c4, 0x00c5, 0x00c9, 0x00e6, 0x00c6, 0x014d, 0x00f6, 0x0122, 0x00a2, 0x015a, 0x015b, 0x00d6,
    0x00dc, 0x00f8, 0x00a3, 0x00d8, 0x00d7, 0x00a4, 0x0100, 0x012a, 0x00f3, 0x017b, 0x017c, 0x017a, 0x201d,
    0x00a6, 0x00a9, 0x00ae, 0x00ac, 0x00bd, 0x00bc, 0x0141, 0x00ab, 0x00bb, 0x2591, 0x2592, 0x2593, 0x2502,
    0x2524, 0x0104, 0x010c, 0x0118, 0x0116, 0x2563, 0x2551, 0x2557, 0x255d, 0x012e, 0x0160, 0x2510, 0x2514,
    0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x0172, 0x016a, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550,
    0x256c, 0x017d, 0x0105, 0x010d, 0x0119, 0x0117, 0x012f, 0x0161, 0x0173, 0x016b, 0x017e, 0x2518, 0x250c,
    0x2588, 0x2584, 0x258c, 0x2590, 0x2580, 0x00d3, 0x00df, 0x014c, 0x0143, 0x00f5, 0x00d5, 0x00b5, 0x0144,
    0x0136, 0x0137, 0x013b, 0x013c, 0x0146, 0x0112, 0x0145, 0x2019, 0x00ad, 0x00b1, 0x201c, 0x00be, 0x00b6,
    0x00a7, 0x00f7, 0x201e, 0x00b0, 0x2219, 0x00b7, 0x00b9, 0x00b3, 0x00b2, 0x25a0, 0x00a0,
};

// Native WindowsBalticRim_from_uni, 0x1c0374754 (__TEXT,__const, 24 entries of {Unicode, byte}).
// Non-external.
static const uint16_t WindowsBalticRim_from_uni[24][2] = {
    {0x00a8, 0x008d}, {0x00af, 0x009d}, {0x00b4, 0x00b4}, {0x00b8, 0x008f}, {0x02c7, 0x008e},
    {0x02d9, 0x00ff}, {0x02db, 0x009e}, {0x2013, 0x0096}, {0x2014, 0x0097}, {0x2018, 0x0091},
    {0x2019, 0x0092}, {0x201a, 0x0082}, {0x201c, 0x0093}, {0x201d, 0x0094}, {0x201e, 0x0084},
    {0x2020, 0x0086}, {0x2021, 0x0087}, {0x2022, 0x0095}, {0x2026, 0x0085}, {0x2030, 0x0089},
    {0x2039, 0x008b}, {0x203a, 0x009b}, {0x20ac, 0x0080}, {0x2122, 0x0099},
};

// Native WindowsBalticRim_to_uni, 0x1c03747b4 (__TEXT,__const, 32 UTF-16 units). Non-external.
static const uint16_t WindowsBalticRim_to_uni[32] = {
    0x20ac, 0xfffd, 0x201a, 0xfffd, 0x201e, 0x2026, 0x2020, 0x2021, 0xfffd, 0x2030, 0xfffd,
    0x2039, 0xfffd, 0x00a8, 0x02c7, 0x00b8, 0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022,
    0x2013, 0x2014, 0xfffd, 0x2122, 0xfffd, 0x203a, 0xfffd, 0x00af, 0x02db, 0xfffd,
};

// Native MacRomanLatin1_from_uni, iOS Simulator 24A434, 0x1c03747f4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x180830a14, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; captured table bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint16_t MacRomanLatin1_from_uni[130][2] = {
    {0x00a0, 0x00a0}, {0x00a1, 0x00a1}, {0x00a2, 0x00a2}, {0x00a3, 0x00a3}, {0x00a4, 0x00a4},
    {0x00a5, 0x00a5}, {0x00a7, 0x00a7}, {0x00a8, 0x00a8}, {0x00a9, 0x00a9}, {0x00aa, 0x00aa},
    {0x00ab, 0x00ab}, {0x00ac, 0x00ac}, {0x00ae, 0x00ae}, {0x00af, 0x00af}, {0x00b0, 0x00b0},
    {0x00b1, 0x00b1}, {0x00b4, 0x00b4}, {0x00b5, 0x00b5}, {0x00b6, 0x00b6}, {0x00b7, 0x00b7},
    {0x00b8, 0x00b8}, {0x00ba, 0x00ba}, {0x00bb, 0x00bb}, {0x00bf, 0x00bf}, {0x00c0, 0x00c0},
    {0x00c1, 0x00c1}, {0x00c2, 0x00c2}, {0x00c3, 0x00c3}, {0x00c4, 0x00c4}, {0x00c5, 0x00c5},
    {0x00c6, 0x00c6}, {0x00c7, 0x00c7}, {0x00c8, 0x00c8}, {0x00c9, 0x00c9}, {0x00ca, 0x00ca},
    {0x00cb, 0x00cb}, {0x00cc, 0x00cc}, {0x00cd, 0x00cd}, {0x00ce, 0x00ce}, {0x00cf, 0x00cf},
    {0x00d1, 0x00d1}, {0x00d2, 0x00d2}, {0x00d3, 0x00d3}, {0x00d4, 0x00d4}, {0x00d5, 0x00d5},
    {0x00d6, 0x00d6}, {0x00d8, 0x00d8}, {0x00d9, 0x00d9}, {0x00da, 0x00da}, {0x00db, 0x00db},
    {0x00dc, 0x00dc}, {0x00df, 0x00df}, {0x00e0, 0x00e0}, {0x00e1, 0x00e1}, {0x00e2, 0x00e2},
    {0x00e3, 0x00e3}, {0x00e4, 0x00e4}, {0x00e5, 0x00e5}, {0x00e6, 0x00e6}, {0x00e7, 0x00e7},
    {0x00e8, 0x00e8}, {0x00e9, 0x00e9}, {0x00ea, 0x00ea}, {0x00eb, 0x00eb}, {0x00ec, 0x00ec},
    {0x00ed, 0x00ed}, {0x00ee, 0x00ee}, {0x00ef, 0x00ef}, {0x00f1, 0x00f1}, {0x00f2, 0x00f2},
    {0x00f3, 0x00f3}, {0x00f4, 0x00f4}, {0x00f5, 0x00f5}, {0x00f6, 0x00f6}, {0x00f7, 0x00f7},
    {0x00f8, 0x00f8}, {0x00f9, 0x00f9}, {0x00fa, 0x00fa}, {0x00fb, 0x00fb}, {0x00fc, 0x00fc},
    {0x00ff, 0x00ff}, {0x0131, 0x009e}, {0x0152, 0x0091}, {0x0153, 0x00a6}, {0x0178, 0x008d},
    {0x0192, 0x009f}, {0x02c6, 0x0096}, {0x02c7, 0x009d}, {0x02d8, 0x0098}, {0x02d9, 0x0099},
    {0x02da, 0x009a}, {0x02db, 0x009c}, {0x02dc, 0x0097}, {0x02dd, 0x009b}, {0x03a9, 0x0087},
    {0x03c0, 0x00bc}, {0x2013, 0x00ad}, {0x2014, 0x008b}, {0x2018, 0x008c}, {0x2019, 0x00b9},
    {0x201a, 0x0092}, {0x201c, 0x00b3}, {0x201d, 0x00b2}, {0x201e, 0x0093}, {0x2020, 0x00dd},
    {0x2021, 0x00fd}, {0x2022, 0x0080}, {0x2026, 0x008a}, {0x2030, 0x0094}, {0x2039, 0x00d0},
    {0x203a, 0x00f0}, {0x2044, 0x008e}, {0x20ac, 0x00db}, {0x2122, 0x0081}, {0x2126, 0x0087},
    {0x2202, 0x008f}, {0x2206, 0x0090}, {0x220f, 0x00bd}, {0x2211, 0x0085}, {0x221a, 0x0088},
    {0x221e, 0x0083}, {0x222b, 0x0086}, {0x2248, 0x0089}, {0x2260, 0x0082}, {0x2264, 0x00be},
    {0x2265, 0x0084}, {0x25ca, 0x00d7}, {0xf8ff, 0x0095}, {0xfb01, 0x00de}, {0xfb02, 0x00fe},
};

// Native MacRomanLatin1_to_uni, iOS Simulator 24A434, 0x1c03749fc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x180830c1c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; captured table bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint16_t MacRomanLatin1_to_uni[128] = {
    0x2022, 0x2122, 0x2260, 0x221e, 0x2265, 0x2211, 0x222b, 0x03a9, 0x221a, 0x2248, 0x2026, 0x2014, 0x2018,
    0x0178, 0x2044, 0x2202, 0x2206, 0x0152, 0x201a, 0x201e, 0x2030, 0xf8ff, 0x02c6, 0x02dc, 0x02d8, 0x02d9,
    0x02da, 0x02dd, 0x02db, 0x02c7, 0x0131, 0x0192, 0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x0153,
    0x00a7, 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x2013, 0x00ae, 0x00af, 0x00b0, 0x00b1, 0x201d, 0x201c,
    0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x2019, 0x00ba, 0x00bb, 0x03c0, 0x220f, 0x2264, 0x00bf, 0x00c0,
    0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd,
    0x00ce, 0x00cf, 0x2039, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x25ca, 0x00d8, 0x00d9, 0x00da,
    0x20ac, 0x00dc, 0x2020, 0xfb01, 0x00df, 0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 0x203a, 0x00f1, 0x00f2, 0x00f3, 0x00f4,
    0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x2021, 0xfb02, 0x00ff,
};

// Native DOSLatinUS_from_uni, 0x1c0374afc (__TEXT,__const, 128 entries of {Unicode, byte}). Non-external.
static const uint16_t DOSLatinUS_from_uni[128][2] = {
    {0x00a0, 0x00ff}, {0x00a1, 0x00ad}, {0x00a2, 0x009b}, {0x00a3, 0x009c}, {0x00a5, 0x009d},
    {0x00aa, 0x00a6}, {0x00ab, 0x00ae}, {0x00ac, 0x00aa}, {0x00b0, 0x00f8}, {0x00b1, 0x00f1},
    {0x00b2, 0x00fd}, {0x00b5, 0x00e6}, {0x00b7, 0x00fa}, {0x00ba, 0x00a7}, {0x00bb, 0x00af},
    {0x00bc, 0x00ac}, {0x00bd, 0x00ab}, {0x00bf, 0x00a8}, {0x00c4, 0x008e}, {0x00c5, 0x008f},
    {0x00c6, 0x0092}, {0x00c7, 0x0080}, {0x00c9, 0x0090}, {0x00d1, 0x00a5}, {0x00d6, 0x0099},
    {0x00dc, 0x009a}, {0x00df, 0x00e1}, {0x00e0, 0x0085}, {0x00e1, 0x00a0}, {0x00e2, 0x0083},
    {0x00e4, 0x0084}, {0x00e5, 0x0086}, {0x00e6, 0x0091}, {0x00e7, 0x0087}, {0x00e8, 0x008a},
    {0x00e9, 0x0082}, {0x00ea, 0x0088}, {0x00eb, 0x0089}, {0x00ec, 0x008d}, {0x00ed, 0x00a1},
    {0x00ee, 0x008c}, {0x00ef, 0x008b}, {0x00f1, 0x00a4}, {0x00f2, 0x0095}, {0x00f3, 0x00a2},
    {0x00f4, 0x0093}, {0x00f6, 0x0094}, {0x00f7, 0x00f6}, {0x00f9, 0x0097}, {0x00fa, 0x00a3},
    {0x00fb, 0x0096}, {0x00fc, 0x0081}, {0x00ff, 0x0098}, {0x0192, 0x009f}, {0x0393, 0x00e2},
    {0x0398, 0x00e9}, {0x03a3, 0x00e4}, {0x03a6, 0x00e8}, {0x03a9, 0x00ea}, {0x03b1, 0x00e0},
    {0x03b4, 0x00eb}, {0x03b5, 0x00ee}, {0x03c0, 0x00e3}, {0x03c3, 0x00e5}, {0x03c4, 0x00e7},
    {0x03c6, 0x00ed}, {0x207f, 0x00fc}, {0x20a7, 0x009e}, {0x2219, 0x00f9}, {0x221a, 0x00fb},
    {0x221e, 0x00ec}, {0x2229, 0x00ef}, {0x2248, 0x00f7}, {0x2261, 0x00f0}, {0x2264, 0x00f3},
    {0x2265, 0x00f2}, {0x2310, 0x00a9}, {0x2320, 0x00f4}, {0x2321, 0x00f5}, {0x2500, 0x00c4},
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

// Native DOSLatinUS_to_uni, 0x1c0374cfc (__TEXT,__const, 128 UTF-16 units). Non-external.
static const uint16_t DOSLatinUS_to_uni[128] = {
    0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee,
    0x00ec, 0x00c4, 0x00c5, 0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00d6,
    0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192, 0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa,
    0x00ba, 0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb, 0x2591, 0x2592, 0x2593, 0x2502,
    0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510, 0x2514,
    0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550,
    0x256c, 0x2567, 0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c,
    0x2588, 0x2584, 0x258c, 0x2590, 0x2580, 0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4,
    0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229, 0x2261, 0x00b1, 0x2265, 0x2264, 0x2320,
    0x2321, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0,
};

// Native DOSLatin1_from_uni, iOS Simulator 24A434, 0x1c0374dfc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18083101c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; captured table bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint16_t DOSLatin1_from_uni[128][2] = {
    {0x00a0, 0x00ff}, {0x00a1, 0x00ad}, {0x00a2, 0x00bd}, {0x00a3, 0x009c}, {0x00a4, 0x00cf},
    {0x00a5, 0x00be}, {0x00a6, 0x00dd}, {0x00a7, 0x00f5}, {0x00a8, 0x00f9}, {0x00a9, 0x00b8},
    {0x00aa, 0x00a6}, {0x00ab, 0x00ae}, {0x00ac, 0x00aa}, {0x00ad, 0x00f0}, {0x00ae, 0x00a9},
    {0x00af, 0x00ee}, {0x00b0, 0x00f8}, {0x00b1, 0x00f1}, {0x00b2, 0x00fd}, {0x00b3, 0x00fc},
    {0x00b4, 0x00ef}, {0x00b5, 0x00e6}, {0x00b6, 0x00f4}, {0x00b7, 0x00fa}, {0x00b8, 0x00f7},
    {0x00b9, 0x00fb}, {0x00ba, 0x00a7}, {0x00bb, 0x00af}, {0x00bc, 0x00ac}, {0x00bd, 0x00ab},
    {0x00be, 0x00f3}, {0x00bf, 0x00a8}, {0x00c0, 0x00b7}, {0x00c1, 0x00b5}, {0x00c2, 0x00b6},
    {0x00c3, 0x00c7}, {0x00c4, 0x008e}, {0x00c5, 0x008f}, {0x00c6, 0x0092}, {0x00c7, 0x0080},
    {0x00c8, 0x00d4}, {0x00c9, 0x0090}, {0x00ca, 0x00d2}, {0x00cb, 0x00d3}, {0x00cc, 0x00de},
    {0x00cd, 0x00d6}, {0x00ce, 0x00d7}, {0x00cf, 0x00d8}, {0x00d0, 0x00d1}, {0x00d1, 0x00a5},
    {0x00d2, 0x00e3}, {0x00d3, 0x00e0}, {0x00d4, 0x00e2}, {0x00d5, 0x00e5}, {0x00d6, 0x0099},
    {0x00d7, 0x009e}, {0x00d8, 0x009d}, {0x00d9, 0x00eb}, {0x00da, 0x00e9}, {0x00db, 0x00ea},
    {0x00dc, 0x009a}, {0x00dd, 0x00ed}, {0x00de, 0x00e8}, {0x00df, 0x00e1}, {0x00e0, 0x0085},
    {0x00e1, 0x00a0}, {0x00e2, 0x0083}, {0x00e3, 0x00c6}, {0x00e4, 0x0084}, {0x00e5, 0x0086},
    {0x00e6, 0x0091}, {0x00e7, 0x0087}, {0x00e8, 0x008a}, {0x00e9, 0x0082}, {0x00ea, 0x0088},
    {0x00eb, 0x0089}, {0x00ec, 0x008d}, {0x00ed, 0x00a1}, {0x00ee, 0x008c}, {0x00ef, 0x008b},
    {0x00f0, 0x00d0}, {0x00f1, 0x00a4}, {0x00f2, 0x0095}, {0x00f3, 0x00a2}, {0x00f4, 0x0093},
    {0x00f5, 0x00e4}, {0x00f6, 0x0094}, {0x00f7, 0x00f6}, {0x00f8, 0x009b}, {0x00f9, 0x0097},
    {0x00fa, 0x00a3}, {0x00fb, 0x0096}, {0x00fc, 0x0081}, {0x00fd, 0x00ec}, {0x00fe, 0x00e7},
    {0x00ff, 0x0098}, {0x0131, 0x00d5}, {0x0192, 0x009f}, {0x2017, 0x00f2}, {0x2500, 0x00c4},
    {0x2502, 0x00b3}, {0x250c, 0x00da}, {0x2510, 0x00bf}, {0x2514, 0x00c0}, {0x2518, 0x00d9},
    {0x251c, 0x00c3}, {0x2524, 0x00b4}, {0x252c, 0x00c2}, {0x2534, 0x00c1}, {0x253c, 0x00c5},
    {0x2550, 0x00cd}, {0x2551, 0x00ba}, {0x2554, 0x00c9}, {0x2557, 0x00bb}, {0x255a, 0x00c8},
    {0x255d, 0x00bc}, {0x2560, 0x00cc}, {0x2563, 0x00b9}, {0x2566, 0x00cb}, {0x2569, 0x00ca},
    {0x256c, 0x00ce}, {0x2580, 0x00df}, {0x2584, 0x00dc}, {0x2588, 0x00db}, {0x2591, 0x00b0},
    {0x2592, 0x00b1}, {0x2593, 0x00b2}, {0x25a0, 0x00fe},
};

// Native DOSLatin1_to_uni, iOS Simulator 24A434, 0x1c0374ffc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18083121c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; captured table bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint16_t DOSLatin1_to_uni[130] = {
    0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee,
    0x00ec, 0x00c4, 0x00c5, 0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00d6,
    0x00dc, 0x00f8, 0x00a3, 0x00d8, 0x00d7, 0x0192, 0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa,
    0x00ba, 0x00bf, 0x00ae, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb, 0x2591, 0x2592, 0x2593, 0x2502,
    0x2524, 0x00c1, 0x00c2, 0x00c0, 0x00a9, 0x2563, 0x2551, 0x2557, 0x255d, 0x00a2, 0x00a5, 0x2510, 0x2514,
    0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x00e3, 0x00c3, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550,
    0x256c, 0x00a4, 0x00f0, 0x00d0, 0x00ca, 0x00cb, 0x00c8, 0x0131, 0x00cd, 0x00ce, 0x00cf, 0x2518, 0x250c,
    0x2588, 0x2584, 0x00a6, 0x00cc, 0x2580, 0x00d3, 0x00df, 0x00d4, 0x00d2, 0x00f5, 0x00d5, 0x00b5, 0x00fe,
    0x00de, 0x00da, 0x00db, 0x00d9, 0x00fd, 0x00dd, 0x00af, 0x00b4, 0x00ad, 0x00b1, 0x2017, 0x00be, 0x00b6,
    0x00a7, 0x00f7, 0x00b8, 0x00b0, 0x00a8, 0x00b7, 0x00b9, 0x00b3, 0x00b2, 0x25a0, 0x00a0, 0x0000, 0x0000,
};

// Native __CFIdempotentCharToUniCharTable, 0x1c03a30d4 (__TEXT,__const, 256 UTF-16 units: the identity).
// External. Installed by __CFSetCharToUniCharFunc(NULL).
const uint16_t __OCFIdempotentCharToUniCharTable[256] = {
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

// Native __CFMacRomanCharToUnicharTable, 0x1c03a32d4 (__TEXT,__const, 256 UTF-16 units). External.
// __CFSetCharToUniCharFunc (0x1c022c1a4) also installs it for the MacRoman converter.
const uint16_t __OCFMacRomanCharToUnicharTable[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c,
    0x000d, 0x000e, 0x000f, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019,
    0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026,
    0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032, 0x0033,
    0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 0x0040,
    0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d,
    0x004e, 0x004f, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a,
    0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074,
    0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 0x00c4, 0x00c5,
    0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9,
    0x00e8, 0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 0x00f2, 0x00f4, 0x00f6, 0x00f5,
    0x00fa, 0x00f9, 0x00fb, 0x00fc, 0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 0x00ae,
    0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8, 0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5,
    0x2202, 0x2211, 0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8, 0x00bf, 0x00a1, 0x00ac,
    0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153,
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 0x00ff, 0x0178, 0x2044, 0x20ac, 0x2039,
    0x203a, 0xfb01, 0xfb02, 0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1, 0x00cb, 0x00c8,
    0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4, 0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6,
    0x02dc, 0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7,
};

// Native CFStringEncodingIsValidCombiningCharacterForLatin1, 0x1c01688cc. External. U+0300-U+035F test bit
// 31 - (unit mod 32) of word (unit - 0x300) / 32 in __CFLatin1CombiningCharBitmap (LSR by ~unit).
bool OCFStringEncodingIsValidCombiningCharacterForLatin1(uint16_t character) {
    uint32_t offset = (uint32_t)character - 0x300;
    if (offset > 0x5f) {
        return false;
    }
    return ((__CFLatin1CombiningCharBitmap[offset >> 5] >> (~(uint32_t)character & 31)) & 1) != 0;
}

// Native CFStringEncodingPrecomposeLatinCharacter, 0x1c0168900. External. A first unit that is a surrogate
// (high or low) is returned as is with one used character. Otherwise following units that are not surrogates
// and are members of character set 8 are composed with CFUniCharPrecomposeCharacter while the result is
// neither U+FFFD nor above U+FFFF. A single used character yields U+FFFD; a count below one yields U+FFFD
// without storing the used count.
uint16_t
OCFStringEncodingPrecomposeLatinCharacter(const uint16_t *character, OCFIndex numChars, OCFIndex *usedChars) {
    if (numChars < 1) {
        return 0xfffd;
    }
    uint32_t precomposed = character[0];
    if (((precomposed | 0x400) & 0xfc00) == 0xdc00) {
        if (usedChars != NULL) {
            *usedChars = 1;
        }
        return (uint16_t)precomposed;
    }
    OCFIndex used = 1;
    while (used != numChars) {
        uint16_t combining = character[used];
        if (((combining | 0x400) & 0xfc00) == 0xdc00) {
            break;
        }
        if (OCFUniCharIsMemberOf(combining, 8) == 0) {
            break;
        }
        uint32_t result = CFUniCharPrecomposeCharacter(precomposed, combining);
        if (result == 0xfffd || (result >> 16) != 0) {
            break;
        }
        ++used;
        precomposed = result;
    }
    if (usedChars != NULL) {
        *usedChars = used;
    }
    if (used > 1) {
        return (uint16_t)precomposed;
    }
    return 0xfffd;
}

// Native __CFToASCII, iOS Simulator 24A434, 0x1c01689f4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1806bf280, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; no scalar behavior difference found.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFToASCII(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
    }
    return character < 0x80;
}

// Native __CFFromASCII, iOS Simulator 24A434, 0x1c0168a0c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1807338bc, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; no scalar behavior difference found.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFFromASCII(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    if ((byte & 0x80) == 0) {
        *character = byte;
    }
    return (byte >> 7) ^ 1;
}

// Native __CFToISOLatin1, iOS Simulator 24A434, 0x1c0168a20,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18071cd3c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; no scalar behavior difference found.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFToISOLatin1(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0xff) {
        *byte = (uint8_t)character;
    }
    return character < 0x100;
}

// Native __CFFromISOLatin1, iOS Simulator 24A434, 0x1c0168a38,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1807338d0, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; no scalar behavior difference found.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFFromISOLatin1(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    *character = byte;
    return 1;
}

// Native __CFToISOLatin1Precompose, 0x1c0168a44. Non-external. No converter is called: a precomposed unit up
// to U+00FF with a nonzero low byte and at least two used characters is the byte, and the used-character
// count is the result.
OCFIndex __CFToISOLatin1Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    OCFIndex usedChars = 0;
    uint16_t precomposed = OCFStringEncodingPrecomposeLatinCharacter(character, numChars, &usedChars);
    if (precomposed > 0xff || (precomposed & 0xff) == 0 || usedChars < 2) {
        return 0;
    }
    if (maxByteLen != 0) {
        *bytes = (uint8_t)precomposed;
    }
    *usedByteLen = 1;
    return usedChars;
}

// Native __CFToMacRoman, 0x1c0168af8. Non-external.
uint32_t __CFToMacRoman(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint16_t)(character + 0x4fd) < 0x59d) {
        return 0;
    }
    int low = 0;
    int high = 128;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = macRoman_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)macRoman_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromMacRoman, 0x1c0168b68. Non-external. Every byte maps through __CFMacRomanCharToUnicharTable
// and succeeds.
uint32_t __CFFromMacRoman(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    *character = __OCFMacRomanCharToUnicharTable[byte];
    return 1;
}

// Native __CFToMacRomanPrecompose, 0x1c0168b80. Non-external. The used-character count from
// CFStringEncodingPrecomposeLatinCharacter, zeroed first, is the result; fewer than two used characters fail.
OCFIndex __CFToMacRomanPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    OCFIndex usedChars = 0;
    uint16_t precomposed = OCFStringEncodingPrecomposeLatinCharacter(character, numChars, &usedChars);
    uint8_t byte = 0;
    if (precomposed <= 0x7f) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed + 0x4fd) < 0x59d) {
            return 0;
        }
        bool found = false;
        int low = 0;
        int high = 128;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = macRoman_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)macRoman_from_uni[middle][1];
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
    if (byte == 0 || usedChars < 2) {
        return 0;
    }
    if (maxByteLen != 0) {
        *bytes = byte;
    }
    *usedByteLen = 1;
    return usedChars;
}

// Native __CFToWinLatin1, iOS Simulator 24A434, 0x1c0168c80,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x180733aa0, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; branch conditions and binary search agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFToWinLatin1(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character < 0x80 || (uint32_t)character - 160 <= 95) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 8483 >= UINT32_C(0xffffe02f)) {
        int low = 0;
        int high = 26;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = cp1252_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)cp1252_from_uni[middle][1];
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

// Native __CFFromWinLatin1, iOS Simulator 24A434, 0x1c0168d04,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x180733b28, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; output-before-failure behavior agrees.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFFromWinLatin1(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((int8_t)byte < -96) {
        value = cp1252_to_uni[byte & 0x1f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToWinLatin1Precompose, 0x1c0168d34. Non-external. The used-character count from
// CFStringEncodingPrecomposeLatinCharacter, zeroed first, is the result; fewer than two used characters fail.
OCFIndex __CFToWinLatin1Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    OCFIndex usedChars = 0;
    uint16_t precomposed = OCFStringEncodingPrecomposeLatinCharacter(character, numChars, &usedChars);
    uint8_t byte = 0;
    if (precomposed < 0x80 || (uint16_t)(precomposed - 0xa0) <= 0x5f) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed - 0x2123) < 0xe02f) {
            return 0;
        }
        bool found = false;
        int low = 0;
        int high = 26;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = cp1252_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)cp1252_from_uni[middle][1];
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
    if (byte == 0 || usedChars < 2) {
        return 0;
    }
    if (maxByteLen != 0) {
        *bytes = byte;
    }
    *usedByteLen = 1;
    return usedChars;
}

// Native __CFToNextStepLatin, iOS Simulator 24A434, 0x1c0168e4c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x180733c74, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; branch conditions and binary search agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFToNextStepLatin(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint16_t)(character + 2) >= 0xa2) {
        int low = 0;
        int high = 126;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = nextstep_from_tab[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)nextstep_from_tab[middle][1];
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

// Native __CFFromNextStepLatin, iOS Simulator 24A434, 0x1c0168ebc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x180733ce4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; output-before-failure behavior agrees.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFFromNextStepLatin(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = NSToPrecompUnicodeTable[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToNextStepLatinPrecompose, 0x1c0168ee4. Non-external. The used-character count from
// CFStringEncodingPrecomposeLatinCharacter, zeroed first, is the result; fewer than two used characters fail.
OCFIndex __CFToNextStepLatinPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)flags;
    OCFIndex usedChars = 0;
    uint16_t precomposed = OCFStringEncodingPrecomposeLatinCharacter(character, numChars, &usedChars);
    uint8_t byte = 0;
    if (precomposed <= 0x7f) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed + 2) < 0xa2) {
            return 0;
        }
        bool found = false;
        int low = 0;
        int high = 126;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = nextstep_from_tab[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)nextstep_from_tab[middle][1];
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
    if (byte == 0 || usedChars < 2) {
        return 0;
    }
    if (maxByteLen != 0) {
        *bytes = byte;
    }
    *usedByteLen = 1;
    return usedChars;
}

// Native __CFToISOLatin6, 0x1c019ef84. Non-external.
uint32_t __CFToISOLatin6(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0xa0) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x2016 < 0xffffe091u) {
        return 0;
    }
    int low = 0;
    int high = 94;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = ISOLatin6_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)ISOLatin6_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromISOLatin6, 0x1c019effc. Non-external.
uint32_t __CFFromISOLatin6(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if (byte > 0xa0) {
        value = ISOLatin6_to_uni[byte - 0xa1];
    }
    *character = value;
    return 1;
}

// Native __CFToISOLatin6Precompose, 0x1c019f020. Non-external. CFStringEncodingPrecomposeLatinCharacter is
// called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToISOLatin6Precompose(
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
    if (precomposed <= 0xa0) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed - 0x2016) < 0xe091) {
            return 0;
        }
        bool found = false;
        int low = 0;
        int high = 94;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = ISOLatin6_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)ISOLatin6_from_uni[middle][1];
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

// Native __CFLatinSuppCombiningCharBitmap, 0x1c03738ac (__TEXT,__const, 3 32-bit words). Non-external.
static const uint32_t __CFLatinSuppCombiningCharBitmap[3] = {
    0xf9a80000,
    0x01800080,
    0x00000000,
};

// Native __CFIsValidCombiningCharacterForLatinSupp, 0x1c019f0e0. Non-external. U+0300-U+035F test bit 31 -
// (unit mod 32) of word (unit - 0x300) / 32 in __CFLatinSuppCombiningCharBitmap (LSR by ~unit); other units
// are not combining.
bool __CFIsValidCombiningCharacterForLatinSupp(uint16_t character) {
    uint32_t offset = (uint32_t)character - 0x300;
    if (offset > 0x5F) {
        return false;
    }
    return ((__CFLatinSuppCombiningCharBitmap[offset >> 5] >> (~(uint32_t)character & 31)) & 1) != 0;
}

// Native __CFToISOLatin7, 0x1c019f114. Non-external.
uint32_t __CFToISOLatin7(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0xa0) {
        *byte = (uint8_t)character;
        return 1;
    }
    if (character == 0xa1 || character > 0x201e) {
        return 0;
    }
    int low = 0;
    int high = 94;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = ISOLatin7_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)ISOLatin7_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromISOLatin7, 0x1c019f190. Non-external.
uint32_t __CFFromISOLatin7(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if (byte > 0xa0) {
        value = ISOLatin7_to_uni[byte - 0xa1];
    }
    *character = value;
    return 1;
}

// Native __CFToISOLatin7Precompose, 0x1c019f1b4. Non-external. CFStringEncodingPrecomposeLatinCharacter is
// called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToISOLatin7Precompose(
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
    if (precomposed <= 0xa0) {
        byte = (uint8_t)precomposed;
    } else {
        if (precomposed == 0xa1 || precomposed > 0x201e) {
            return 0;
        }
        bool found = false;
        int low = 0;
        int high = 94;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = ISOLatin7_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)ISOLatin7_from_uni[middle][1];
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

// Native __CFToISOLatin8, 0x1c019f280. Non-external.
uint32_t __CFToISOLatin8(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0xa0) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x1ef4 < 0xffffe1afu) {
        return 0;
    }
    int low = 0;
    int high = 94;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = ISOLatin8_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)ISOLatin8_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromISOLatin8, 0x1c019f2f8. Non-external.
uint32_t __CFFromISOLatin8(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if (byte > 0xa0) {
        value = ISOLatin8_to_uni[byte - 0xa1];
    }
    *character = value;
    return 1;
}

// Native __CFToISOLatin8Precompose, 0x1c019f31c. Non-external. CFStringEncodingPrecomposeLatinCharacter is
// called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToISOLatin8Precompose(
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
    if (precomposed <= 0xa0) {
        byte = (uint8_t)precomposed;
    } else {
        if ((uint16_t)(precomposed - 0x1ef4) < 0xe1af) {
            return 0;
        }
        bool found = false;
        int low = 0;
        int high = 94;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = ISOLatin8_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)ISOLatin8_from_uni[middle][1];
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

// Native __CFToISOLatin9, 0x1c019f3dc. Non-external.
uint32_t __CFToISOLatin9(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0xa3) {
        *byte = (uint8_t)character;
        return 1;
    }
    if (character == 0xa4 || character > 0x20ac) {
        return 0;
    }
    int low = 0;
    int high = 91;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = ISOLatin9_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)ISOLatin9_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromISOLatin9, 0x1c019f458. Non-external.
uint32_t __CFFromISOLatin9(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if (byte > 0xa3) {
        value = ISOLatin9_to_uni[byte - 0xa4];
    }
    *character = value;
    return 1;
}

// Native __CFToISOLatin9Precompose, 0x1c019f47c. Non-external. CFStringEncodingPrecomposeLatinCharacter is
// called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToISOLatin9Precompose(
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
    if (precomposed <= 0xa3) {
        byte = (uint8_t)precomposed;
    } else {
        if (precomposed == 0xa4 || precomposed > 0x20ac) {
            return 0;
        }
        bool found = false;
        int low = 0;
        int high = 91;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = ISOLatin9_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)ISOLatin9_from_uni[middle][1];
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

// Native __CFToMacIcelandic, 0x1c019f548. Non-external.
uint32_t __CFToMacIcelandic(uint32_t flags, uint16_t character, uint8_t *byte) {
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
        uint32_t entry = MacIcelandic_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)MacIcelandic_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromMacIcelandic, 0x1c019f5b8. Non-external. Every byte succeeds.
uint32_t __CFFromMacIcelandic(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = MacIcelandic_to_uni[byte & 0x7f];
    }
    *character = value;
    return 1;
}

// Native __CFToMacIcelandicPrecompose, 0x1c019f5d8. Non-external. CFStringEncodingPrecomposeLatinCharacter is
// called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToMacIcelandicPrecompose(
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
            uint32_t entry = MacIcelandic_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)MacIcelandic_from_uni[middle][1];
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

// Native __CFToDOSIcelandic, 0x1c019f698. Non-external.
uint32_t __CFToDOSIcelandic(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x25a1 < 0xffffdaffu) {
        return 0;
    }
    int low = 0;
    int high = 127;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = DOSIcelandic_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)DOSIcelandic_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromDOSIcelandic, 0x1c019f710. Non-external. The native function decodes through
// DOSBalticRim_to_uni (ADRL at 0x1c019f718), not a DOS Icelandic table; the image has no DOSIcelandic_to_uni
// symbol. Kept as native behavior.
uint32_t __CFFromDOSIcelandic(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = DOSBalticRim_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToDOSIcelandicPrecompose, 0x1c019f738. Non-external. CFStringEncodingPrecomposeLatinCharacter is
// called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToDOSIcelandicPrecompose(
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
        int high = 127;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = DOSIcelandic_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)DOSIcelandic_from_uni[middle][1];
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

// Native __CFToDOSBalticRim, 0x1c019f7f8. Non-external.
uint32_t __CFToDOSBalticRim(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x25a1 < 0xffffdaffu) {
        return 0;
    }
    int low = 0;
    int high = 127;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = DOSBalticRim_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)DOSBalticRim_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromDOSBalticRim, 0x1c019f870. Non-external.
uint32_t __CFFromDOSBalticRim(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = DOSBalticRim_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToDOSBalticRimPrecompose, 0x1c019f898. Non-external. CFStringEncodingPrecomposeLatinCharacter is
// called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToDOSBalticRimPrecompose(
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
        int high = 127;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = DOSBalticRim_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)DOSBalticRim_from_uni[middle][1];
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

// Native __CFToWindowsBalticRim, 0x1c019f958. Non-external. U+00A8-U+2122 are searched in
// WindowsBalticRim_from_uni first. Units from U+00A0 that are not found there fail for U+00A1, U+00A5, U+00B4
// (bits of 0x80011 from U+00A1) and U+00FF, map U+00A0 to 0xa0, and are otherwise searched in
// ISOLatin7_from_uni up to U+201E.
uint32_t __CFToWindowsBalticRim(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if (character >= 0xa8 && character <= 0x2122) {
        int low = 0;
        int high = 23;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = WindowsBalticRim_from_uni[middle][0];
            if (character >= entry) {
                if (character <= entry) {
                    *byte = (uint8_t)WindowsBalticRim_from_uni[middle][1];
                    return 1;
                }
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        } while (low <= high);
    }
    if (character < 0xa0) {
        return 0;
    }
    if ((uint32_t)character - 0xa1 <= 0x13 && ((UINT32_C(1) << (character - 0xa1)) & 0x80011) != 0) {
        return 0;
    }
    if (character == 0xff) {
        return 0;
    }
    if (character < 0xa1) {
        *byte = 0xa0;
        return 1;
    }
    if (character == 0xa1 || character > 0x201e) {
        return 0;
    }
    int low = 0;
    int high = 94;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = ISOLatin7_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)ISOLatin7_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromWindowsBalticRim, 0x1c019fa6c. Non-external. Bytes 0x80-0x9f store their
// WindowsBalticRim_to_uni value and succeed, except the ten whose bit (byte mod 64) is set in 0x9501150a
// (0x81, 0x83, 0x88, 0x8a, 0x8c, 0x90, 0x98, 0x9a, 0x9c, 0x9f): those then store the byte itself and succeed.
// Bytes from 0xa0 first store U+FFFD; 0xa1 and 0xa5 then fail with it, 0xb4 stays U+00B4, 0xff becomes
// U+02D9, 0xa0 stores itself and the rest use ISOLatin7_to_uni.
uint32_t __CFFromWindowsBalticRim(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    if ((byte & 0x80) == 0) {
        *character = byte;
        return 1;
    }
    if (byte < 0xa0) {
        *character = WindowsBalticRim_to_uni[byte & 0x7f];
        if (((UINT64_C(1) << (byte & 63)) & 0x9501150a) == 0) {
            return 1;
        }
    } else {
        *character = 0xfffd;
    }
    uint16_t value = byte;
    if (byte > 0xb3) {
        if (byte == 0xb4) {
            *character = 0xb4;
            return 1;
        }
        if (byte == 0xff) {
            *character = 0x2d9;
            return 1;
        }
    } else if (byte == 0xa1 || byte == 0xa5) {
        return 0;
    }
    if (byte > 0xa0) {
        value = ISOLatin7_to_uni[byte - 0xa1];
    }
    *character = value;
    return 1;
}

// Native __CFToWindowsBalticRimPrecompose, 0x1c019fb18. Non-external. The scratch byte is zeroed before
// CFStringEncodingPrecomposeLatinCharacter (NULL used-character count) runs; __CFToWindowsBalticRim receives
// the precomposed unit in both W0 (as flags) and W1, as in the native register setup.
OCFIndex __CFToWindowsBalticRimPrecompose(
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
    if (__CFToWindowsBalticRim(precomposed, precomposed, &byte) == 0 || byte == 0) {
        return 0;
    }
    if (maxByteLen != 0) {
        *bytes = byte;
    }
    *usedByteLen = 1;
    return 2;
}

// Native __CFToMacRomanLatin1, iOS Simulator 24A434, 0x1c019fbc0,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18074713c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; branch conditions and binary search agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFToMacRomanLatin1(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint16_t)(character + 1277) >= 0x59d) {
        int low = 0;
        int high = 129;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = MacRomanLatin1_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)MacRomanLatin1_from_uni[middle][1];
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

// Native __CFFromMacRomanLatin1, iOS Simulator 24A434, 0x1c019fc30,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1807471ac, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; output-before-failure behavior agrees.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFFromMacRomanLatin1(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = MacRomanLatin1_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToMacRomanLatin1Precompose, 0x1c019fc58. Non-external. CFStringEncodingPrecomposeLatinCharacter
// is called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToMacRomanLatin1Precompose(
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
        if ((uint16_t)(precomposed + 0x4fd) < 0x59d) {
            return 0;
        }
        bool found = false;
        int low = 0;
        int high = 129;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = MacRomanLatin1_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)MacRomanLatin1_from_uni[middle][1];
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

// Native __CFToDOSLatinUS, 0x1c019fd18. Non-external.
uint32_t __CFToDOSLatinUS(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 0x25a1 < 0xffffdaffu) {
        return 0;
    }
    int low = 0;
    int high = 127;
    do {
        int middle = low + ((high - low) >> 1);
        uint32_t entry = DOSLatinUS_from_uni[middle][0];
        if (character >= entry) {
            if (character <= entry) {
                *byte = (uint8_t)DOSLatinUS_from_uni[middle][1];
                return 1;
            }
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    } while (low <= high);
    return 0;
}

// Native __CFFromDOSLatinUS, 0x1c019fd90. Non-external.
uint32_t __CFFromDOSLatinUS(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = DOSLatinUS_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToDOSLatinUSPrecompose, 0x1c019fdb8. Non-external. CFStringEncodingPrecomposeLatinCharacter is
// called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToDOSLatinUSPrecompose(
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
        int high = 127;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = DOSLatinUS_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)DOSLatinUS_from_uni[middle][1];
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

// Native __CFToDOSLatin1, iOS Simulator 24A434, 0x1c019fe78,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1807473fc, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; branch conditions and binary search agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFToDOSLatin1(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character <= 0x7f) {
        *byte = (uint8_t)character;
        return 1;
    }
    if ((uint32_t)character - 9633 >= UINT32_C(0xffffdaff)) {
        int low = 0;
        int high = 127;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t key = DOSLatin1_from_uni[middle][0];
            if (character >= key) {
                if (character <= key) {
                    *byte = (uint8_t)DOSLatin1_from_uni[middle][1];
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

// Native __CFFromDOSLatin1, iOS Simulator 24A434, 0x1c019fef0,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x180747474, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; output-before-failure behavior agrees.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFFromDOSLatin1(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    uint16_t value = byte;
    if ((byte & 0x80) != 0) {
        value = DOSLatin1_to_uni[byte & 0x7f];
    }
    *character = value;
    return value != 0xfffd;
}

// Native __CFToDOSLatin1Precompose, 0x1c019ff18. Non-external. CFStringEncodingPrecomposeLatinCharacter is
// called with a NULL used-character count and two characters are reported consumed.
OCFIndex __CFToDOSLatin1Precompose(
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
        int high = 127;
        do {
            int middle = low + ((high - low) >> 1);
            uint32_t entry = DOSLatin1_from_uni[middle][0];
            if (precomposed >= entry) {
                if (precomposed <= entry) {
                    byte = (uint8_t)DOSLatin1_from_uni[middle][1];
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

// Native __CFConverterASCII, 0x22279cb38 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterASCII = {
    .toBytes.cheapEightBit = __CFToASCII,
    .toUnicode.cheapEightBit = __CFFromASCII,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 1,
    .encodingClass = 1,
};

// Native __CFConverterISOLatin1, 0x22279cb80 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterISOLatin1 = {
    .toBytes.cheapEightBit = __CFToISOLatin1,
    .toUnicode.cheapEightBit = __CFFromISOLatin1,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 1,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToISOLatin1Precompose,
    .isValidCombiningChar = OCFStringEncodingIsValidCombiningCharacterForLatin1,
};

// Native __CFConverterMacRoman, 0x22279cbc8 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterMacRoman = {
    .toBytes.cheapEightBit = __CFToMacRoman,
    .toUnicode.cheapEightBit = __CFFromMacRoman,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 1,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToMacRomanPrecompose,
    .isValidCombiningChar = OCFStringEncodingIsValidCombiningCharacterForLatin1,
};

// Native __CFConverterWinLatin1, 0x22279cc10 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterWinLatin1 = {
    .toBytes.cheapEightBit = __CFToWinLatin1,
    .toUnicode.cheapEightBit = __CFFromWinLatin1,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 1,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToWinLatin1Precompose,
    .isValidCombiningChar = OCFStringEncodingIsValidCombiningCharacterForLatin1,
};

// Native __CFConverterNextStepLatin, 0x22279cc58 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterNextStepLatin = {
    .toBytes.cheapEightBit = __CFToNextStepLatin,
    .toUnicode.cheapEightBit = __CFFromNextStepLatin,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 1,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToNextStepLatinPrecompose,
    .isValidCombiningChar = OCFStringEncodingIsValidCombiningCharacterForLatin1,
};

// Native __CFConverterISOLatin6, 0x22279de88 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterISOLatin6 = {
    .toBytes.cheapEightBit = __CFToISOLatin6,
    .toUnicode.cheapEightBit = __CFFromISOLatin6,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToISOLatin6Precompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatinSupp,
};

// Native __CFConverterISOLatin7, 0x22279ded0 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterISOLatin7 = {
    .toBytes.cheapEightBit = __CFToISOLatin7,
    .toUnicode.cheapEightBit = __CFFromISOLatin7,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToISOLatin7Precompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatinSupp,
};

// Native __CFConverterISOLatin8, 0x22279df18 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterISOLatin8 = {
    .toBytes.cheapEightBit = __CFToISOLatin8,
    .toUnicode.cheapEightBit = __CFFromISOLatin8,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToISOLatin8Precompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatinSupp,
};

// Native __CFConverterISOLatin9, 0x22279df60 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterISOLatin9 = {
    .toBytes.cheapEightBit = __CFToISOLatin9,
    .toUnicode.cheapEightBit = __CFFromISOLatin9,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToISOLatin9Precompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatinSupp,
};

// Native __CFConverterMacIcelandic, 0x22279dfa8 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterMacIcelandic = {
    .toBytes.cheapEightBit = __CFToMacIcelandic,
    .toUnicode.cheapEightBit = __CFFromMacIcelandic,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToMacIcelandicPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatinSupp,
};

// Native __CFConverterDOSIcelandic, 0x22279dff0 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterDOSIcelandic = {
    .toBytes.cheapEightBit = __CFToDOSIcelandic,
    .toUnicode.cheapEightBit = __CFFromDOSIcelandic,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToDOSIcelandicPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatinSupp,
};

// Native __CFConverterDOSBalticRim, 0x22279e038 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterDOSBalticRim = {
    .toBytes.cheapEightBit = __CFToDOSBalticRim,
    .toUnicode.cheapEightBit = __CFFromDOSBalticRim,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToDOSBalticRimPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatinSupp,
};

// Native __CFConverterWindowsBalticRim, 0x22279e080 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterWindowsBalticRim = {
    .toBytes.cheapEightBit = __CFToWindowsBalticRim,
    .toUnicode.cheapEightBit = __CFFromWindowsBalticRim,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToWindowsBalticRimPrecompose,
    .isValidCombiningChar = __CFIsValidCombiningCharacterForLatinSupp,
};

// Native __CFConverterMacRomanLatin1, 0x22279e0c8 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterMacRomanLatin1 = {
    .toBytes.cheapEightBit = __CFToMacRomanLatin1,
    .toUnicode.cheapEightBit = __CFFromMacRomanLatin1,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToMacRomanLatin1Precompose,
    .isValidCombiningChar = OCFStringEncodingIsValidCombiningCharacterForLatin1,
};

// Native __CFConverterDOSLatinUS, 0x22279e110 (__DATA_CONST,__const, 72 bytes). Hidden (was private
// external).
const StringEncodingConverterRecord __CFConverterDOSLatinUS = {
    .toBytes.cheapEightBit = __CFToDOSLatinUS,
    .toUnicode.cheapEightBit = __CFFromDOSLatinUS,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToDOSLatinUSPrecompose,
    .isValidCombiningChar = OCFStringEncodingIsValidCombiningCharacterForLatin1,
};

// Native __CFConverterDOSLatin1, 0x22279e158 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
const StringEncodingConverterRecord __CFConverterDOSLatin1 = {
    .toBytes.cheapEightBit = __CFToDOSLatin1,
    .toUnicode.cheapEightBit = __CFFromDOSLatin1,
    .maxBytesPerChar = 1,
    .maxDecomposedCharLen = 2,
    .encodingClass = 1,
    .toBytesPrecompose = __CFToDOSLatin1Precompose,
    .isValidCombiningChar = OCFStringEncodingIsValidCombiningCharacterForLatin1,
};
