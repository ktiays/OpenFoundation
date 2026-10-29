//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFStringEncodingDatabase.h"

#include <stdio.h>

// ICU (libicucore) is a library the native image links (/usr/lib/libicucore.A.dylib); it is supplied,
// never reimplemented. The SDK installs no ucnv.h, so the single called entry point is declared here.
// UErrorCode is a 32-bit enumeration; the native caller passes a 32-bit stack slot.
extern const char *ucnv_getAlias(const char *alias, uint16_t n, int32_t *pErrorCode);

// Inferred organization: __CFStringEncodingGetICUName (0x1c02ad994), __CFStringEncodingGetWindowsCodePage
// (0x1c02ae7c4) and __CFStringEncodingGetCanonicalName (0x1c02aea2c) lie in one iOS Simulator __text
// region with their sorted tables in __TEXT,__const and __DATA_CONST,__const. The native source path
// is not in the evidence.
// All tables below were decoded from the capture by Local/Evidence/cf-encoding-names/generate-tables.py,
// which also checks that the iOS Simulator and iOS Device images agree entry for entry.

// Native __CFKnownEncodingList, iOS Simulator 24A434, 0x1c046ca44 (__TEXT,__const, 98 entries; the
// searches read up to list + 0xc2), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// Non-external. iOS Device 24A446: 0x18097afb4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence:
// verified; the 98 entries agree. Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint16_t __CFKnownEncodingList[98] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d,
    0x000e, 0x000f, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a,
    0x001b, 0x001c, 0x001d, 0x001e, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x008c,
    0x0098, 0x00ec, 0x0400, 0x0405, 0x0406, 0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x0420, 0x0421, 0x0422, 0x0423, 0x0500, 0x0501, 0x0502,
    0x0503, 0x0504, 0x0505, 0x0506, 0x0507, 0x0508, 0x0510, 0x0600, 0x0628, 0x0632, 0x0820, 0x0821, 0x0822,
    0x0823, 0x0830, 0x0831, 0x0840, 0x0920, 0x0930, 0x0931, 0x0940, 0x0a01, 0x0a02, 0x0a03, 0x0a04, 0x0a05,
    0x0a06, 0x0a07, 0x0a08, 0x0a09, 0x0a10, 0x0b01, 0x0c02
};

// Native __CFWindowsCPList, iOS Simulator 24A434, 0x1c046c980 (__TEXT,__const, 98 entries parallel to
// __CFKnownEncodingList), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x18097aef0, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the 98
// entries agree. Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint16_t __CFWindowsCPList[98] = {
    0x2710, 0x2711, 0x2712, 0x2713, 0x2714, 0x2715, 0x2716, 0x2717, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2725, 0x0000, 0x0000, 0x0000, 0x2718, 0x0000,
    0x0000, 0x0000, 0x272d, 0x0000, 0x0000, 0x0000, 0x2761, 0x2762, 0x275f, 0x271a, 0x0000, 0x0000, 0x0000,
    0x2721, 0x0000, 0x01b5, 0x02e1, 0x0307, 0x0352, 0x0353, 0x0354, 0x0357, 0x0359, 0x035c, 0x035d, 0x035e,
    0x035f, 0x0360, 0x0361, 0x0362, 0x0365, 0x036a, 0x03a4, 0x03a8, 0x03b5, 0x03b6, 0x04e4, 0x04e2, 0x04e3,
    0x04e5, 0x04e6, 0x04e7, 0x04e8, 0x04e9, 0x04ea, 0x0551, 0x4e9f, 0x0000, 0xd698, 0xc42d, 0x0000, 0x0000,
    0x0000, 0xc433, 0x0000, 0xc431, 0xcadc, 0xcae0, 0xcaee, 0xcaed, 0x0000, 0x5182, 0x0000, 0x0000, 0xcec8,
    0x0000, 0x0000, 0x556a, 0x0000, 0x0000, 0x0000, 0x0025
};

// Native unnamed table of __CFStringEncodingGetWindowsCodePage, iOS Simulator 24A434, 0x1c046cb08
// (__TEXT,__const, seven entries read: index < 7), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified. iOS Device 24A446: 0x18097b078, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// the seven entries agree. kUnicodeWindowsCodePages is a local descriptive name. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint16_t kUnicodeWindowsCodePages[7] = {0x04b0, 0xfde8, 0xfde9, 0xfded, 0x04b1, 0x0000, 0xfdee};

// Native __CFCanonicalNameList, iOS Simulator 24A434, 0x2227a8cd8 (__DATA_CONST,__const, 98 pointers
// parallel to __CFKnownEncodingList), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// Non-external. iOS Device 24A446: 0x1e053ab10, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence:
// verified; chained-fixup targets decode to the same 98 strings (and the same null slots);
// authentication/diversity bits remain unread. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const char *const __CFCanonicalNameList[98] = {
    "macintosh",
    "japanese",
    "trad-chinese",
    "korean",
    "arabic",
    "hebrew",
    "greek",
    "cyrillic",
    "devanagari",
    "gurmukhi",
    "gujarati",
    "oriya",
    "bengali",
    "tamil",
    "telugu",
    "kannada",
    "malayalam",
    "sinhalese",
    "burmese",
    "khmer",
    "thai",
    "laotian",
    "georgian",
    "armenian",
    "simp-chinese",
    "tibetan",
    "mongolian",
    "ethiopic",
    "centraleurroman",
    "vietnamese",
    "symbol",
    "dingbats",
    "turkish",
    "croatian",
    "icelandic",
    "romanian",
    "celtic",
    "gaelic",
    "farsi",
    "ukrainian",
    "inuit",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "us-ascii",
    NULL,
    "gb18030",
    "iso-2022-jp",
    "iso-2022-jp-2",
    "iso-2022-jp-1",
    "iso-2022-jp-3",
    "iso-2022-cn",
    "iso-2022-cn-ext",
    "iso-2022-kr",
    "euc-jp",
    "gb2312",
    "euc-tw",
    "euc-kr",
    "shift_jis",
    "koi8-r",
    "big5",
    "roman-latin1",
    "hz-gb-2312",
    "big5-hkscs",
    "viscii",
    "koi8-u",
    NULL,
    "utf7-imap",
    "x-nextstep",
    "ibm037"
};

// Native unnamed table of __CFStringEncodingGetCanonicalName, iOS Simulator 24A434, 0x2227a9018
// (__DATA_CONST,__const, eight pointers read: index <= 7), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. iOS Device 24A446: 0x1e053ae20, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; chained-fixup targets decode to the same eight strings. kUnicodeCanonicalNames is a
// local descriptive name. Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const char *const kUnicodeCanonicalNames[8] = {
    "utf-16", "utf-7", "utf-8", "utf-32", "utf-16be", "utf-16le", "utf-32be", "utf-32le"
};

// Native __CFStringEncodingGetWindowsCodePage, iOS Simulator 24A434, 0x1c02ae7c4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden (was private external).
// iOS Device 24A446: 0x1807999d4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; identical
// instructions. The Unicode index is (encoding - 0x100) rotated right by 26 bits (EXTR). The binary search
// halves the byte distance with an arithmetic shift, which is the element midpoint; the native code also
// rejects a found byte offset of -2 (index -1), which no search result reaches.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFStringEncodingGetWindowsCodePage(uint32_t encoding) {
    if ((encoding & 0xf00) == 0x200) {
        return (encoding & 0xff) + 28590;
    }
    if ((encoding & 0xf00) == 0x100) {
        uint32_t value = encoding - 0x100;
        uint32_t index = (value >> 26) | (value << 6);
        if (index < 7) {
            return kUnicodeWindowsCodePages[index];
        }
        return 0;
    }
    uint32_t key = encoding & 0xfff;
    OCFIndex low = 0;
    OCFIndex high = 97;
    OCFIndex middle;
    for (;;) {
        middle = low + ((high - low) >> 1);
        if (key == __CFKnownEncodingList[middle]) {
            break;
        }
        if (key < __CFKnownEncodingList[middle]) {
            high = middle - 1;
        } else {
            low = middle + 1;
        }
        if (low > high) {
            return 0;
        }
    }
    if (middle == -1) {
        return 0;
    }
    return __CFWindowsCPList[middle];
}

// Native __CFStringEncodingGetCanonicalName, iOS Simulator 24A434, 0x1c02aea2c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden (was private external).
// iOS Device 24A446: 0x1806eb200, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: __CFStringEncodingGetCanonicalName at 0x1806eb200 tests the ISO 8859 group
// first and outlines the binary-search steps: _OUTLINED_FUNCTION_1_48 (0x180613ec0) sets the list
// bounds and the key encoding & 0xfff, _OUTLINED_FUNCTION_0_65 (0x18065cb54) forms the midpoint with
// the same arithmetic shift and compares its halfword, and _OUTLINED_FUNCTION_2_30 (0x180613938) forms
// both bound candidates and selects the low bound; the caller selects the high bound on the helper's
// flags. The search, tables, prefix selection and format strings agree with the iOS Simulator code.
// The group is ((encoding & 0xf00) - 0x100) >> 8 as an unsigned 32-bit value. The "x-mac-" prefix is
// chosen for a nonzero encoding in group 0x000 and for 0xa04. The result compares bufferSize with the
// sign-extended snprintf result.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
bool __CFStringEncodingGetCanonicalName(uint32_t encoding, char *buffer, OCFIndex bufferSize) {
    uint32_t group = ((encoding & 0xf00) - 0x100) >> 8;
    if (group - 3 < 2) {
        uint32_t key = encoding & 0xfff;
        OCFIndex low = 0;
        OCFIndex high = 97;
        OCFIndex middle;
        for (;;) {
            middle = low + ((high - low) >> 1);
            if (key == __CFKnownEncodingList[middle]) {
                break;
            }
            if (key < __CFKnownEncodingList[middle]) {
                high = middle - 1;
            } else {
                low = middle + 1;
            }
            if (low > high) {
                return false;
            }
        }
        if (middle == -1 || __CFWindowsCPList[middle] == 0) {
            return false;
        }
        const char *format = (encoding & 0xf00) == 0x400 ? "cp%d" : "windows-%d";
        return bufferSize > snprintf(buffer, (size_t)bufferSize, format, __CFWindowsCPList[middle]);
    }
    if (group == 1) {
        return bufferSize > snprintf(buffer, (size_t)bufferSize, "iso-8859-%u", encoding & 0xff);
    }
    if (group == 0) {
        uint32_t value = encoding - 0x100;
        uint32_t index = (value >> 26) | (value << 6);
        if (index > 7) {
            return false;
        }
        return bufferSize > snprintf(buffer, (size_t)bufferSize, "%s", kUnicodeCanonicalNames[index]);
    }
    uint32_t key = encoding & 0xfff;
    OCFIndex low = 0;
    OCFIndex high = 97;
    OCFIndex middle;
    for (;;) {
        middle = low + ((high - low) >> 1);
        if (key == __CFKnownEncodingList[middle]) {
            break;
        }
        if (key < __CFKnownEncodingList[middle]) {
            high = middle - 1;
        } else {
            low = middle + 1;
        }
        if (low > high) {
            return false;
        }
    }
    if (middle == -1 || __CFCanonicalNameList[middle] == NULL) {
        return false;
    }
    const char *prefix = ((encoding & 0xf00) == 0 && encoding != 0) || encoding == 0xa04 ? "x-mac-" : "";
    return bufferSize > snprintf(buffer, (size_t)bufferSize, "%s%s", prefix, __CFCanonicalNameList[middle]);
}

// Native __CFStringEncodingGetICUName, iOS Simulator 24A434, 0x1c02ad994,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden (was private external).
// iOS Device 24A446: 0x180798b1c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; identical
// control flow. One error code, zeroed once on entry, is shared by both ucnv_getAlias calls, as in the native
// code: a failure recorded by the first call reaches the second. The name buffer is 60 bytes (frame offsets
// 0xc-0x48).
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
const char *__CFStringEncodingGetICUName(uint32_t encoding) {
    int32_t errorCode = 0;
    char buffer[60];
    if (encoding == 0xa10) {
        return "IMAP-mailbox-name";
    }
    if ((encoding & 0xf00) != 0x100) {
        uint32_t codePage = __CFStringEncodingGetWindowsCodePage(encoding);
        if (codePage != 0 && snprintf(buffer, 60, "windows-%d", codePage) <= 59) {
            const char *name = ucnv_getAlias(buffer, 0, &errorCode);
            if (name != NULL) {
                return name;
            }
        }
    }
    if (__CFStringEncodingGetCanonicalName(encoding, buffer, 60)) {
        return ucnv_getAlias(buffer, 0, &errorCode);
    }
    return NULL;
}
