//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "EncodingNameStandaloneTests.h"

#include "CoreFoundation/StringEncoding/OCFPlatformConverter.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingDatabase.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"

#include <stdio.h>
#include <string.h>

static uint32_t identityToUnicode(uint32_t flags, uint8_t byte, uint16_t *character) {
    (void)flags;
    if (byte == 0) {
        return 0;
    }
    *character = byte;
    return 1;
}

static uint32_t identityToBytes(uint32_t flags, uint16_t character, uint8_t *byte) {
    (void)flags;
    if (character > 0xff) {
        return 0;
    }
    *byte = (uint8_t)character;
    return 1;
}

static uint32_t pairToUnicode(uint32_t flags, uint8_t byte, uint16_t *characters) {
    (void)flags;
    characters[0] = byte;
    characters[1] = 0x0301;
    return 2;
}

static int checkName(uint32_t encoding, const char *expected) {
    char buffer[64];
    memset(buffer, 0x5a, sizeof(buffer));
    bool result = __CFStringEncodingGetCanonicalName(encoding, buffer, sizeof(buffer));
    if (expected == NULL) {
        return result ? 1 : 0;
    }
    return !result || strcmp(buffer, expected) != 0;
}

// Expected values follow the readings of the native functions in
// Documentation/Reference/Research/CoreFoundationStringEncoding.md (iOS Simulator 24A434).
int runStringEncodingNameStandaloneTests(void) {
    size_t assertions = 0;

    const struct {
        uint32_t encoding;
        unsigned long expected;
    } conversions[] = {
        {0, 30},
        {0x1000, 30},
        {0x600, 1},
        {0x1600, 0x80001600},
        {0x8000100, 4},
        {0x100, 10},
        {0x10000100, 0x90000100},
        {0x201, 5},
        {0x500, 12},
        {0xbff, 7},
        {0xb01, 2},
        {0x1234, 0x80001234},
    };

    for (size_t index = 0; index < sizeof(conversions) / sizeof(conversions[0]); ++index) {
        if (OCFStringConvertEncodingToNSStringEncoding(conversions[index].encoding) !=
            conversions[index].expected) {
            fprintf(stderr, "NSStringEncoding standalone mismatch: %08x.\n", conversions[index].encoding);
            return 1;
        }
        ++assertions;
    }
    if (__CFStringEncodingGetWindowsCodePage(0x500) != 1252 ||
        __CFStringEncodingGetWindowsCodePage(0x201) != 28591 ||
        __CFStringEncodingGetWindowsCodePage(0x8000100) != 65001 ||
        __CFStringEncodingGetWindowsCodePage(0x1c000100) != 0 ||
        __CFStringEncodingGetWindowsCodePage(0x7ff) != 0 ||
        __CFStringEncodingGetWindowsCodePage(0x400) != 437) {
        fputs("Windows code page standalone mismatch.\n", stderr);
        return 1;
    }
    assertions += 6;

    const struct {
        uint32_t encoding;
        const char *expected;
    } names[] = {
        {0, "macintosh"},
        {1, "x-mac-japanese"},
        {0x500, "windows-1252"},
        {0x400, "cp437"},
        {0x201, "iso-8859-1"},
        {0x100, "utf-16"},
        {0x8000100, "utf-8"},
        {0x1c000100, "utf-32le"},
        {0x600, "us-ascii"},
        {0xa04, "x-mac-roman-latin1"},
        {0xa10, "utf7-imap"},
        {0x29, NULL},
        {0x601, NULL},
        {0x20000100, NULL},
    };

    for (size_t index = 0; index < sizeof(names) / sizeof(names[0]); ++index) {
        if (checkName(names[index].encoding, names[index].expected) != 0) {
            fprintf(stderr, "Canonical name standalone mismatch: %08x.\n", names[index].encoding);
            return 1;
        }
        ++assertions;
    }
    char small[8];
    memset(small, 0x5a, sizeof(small));
    if (__CFStringEncodingGetCanonicalName(0, small, 4) || memcmp(small, "mac\0", 4) != 0 ||
        small[4] != 0x5a) {
        fputs("Canonical name truncation standalone mismatch.\n", stderr);
        return 1;
    }
    ++assertions;
    const char *icu = __CFStringEncodingGetICUName(0xa10);
    if (icu == NULL || strcmp(icu, "IMAP-mailbox-name") != 0 || __CFStringEncodingGetICUName(0x29) != NULL ||
        __CFStringEncodingGetICUName(0x8000100) == NULL) {
        fputs("ICU name standalone mismatch.\n", stderr);
        return 1;
    }
    assertions += 3;
    uint16_t characters[8] = {0xa55a, 0xa55a, 0xa55a, 0xa55a, 0xa55a, 0xa55a, 0xa55a, 0xa55a};
    OCFIndex used = -1;
    if (__CFDefaultToUnicodeFallbackProc(NULL, 3, characters, 0, &used) != 1 || used != 1 ||
        characters[0] != 0xa55a || __CFDefaultToUnicodeFallbackProc(NULL, 3, characters, 1, &used) != 1 ||
        characters[0] != '?') {
        fputs("Fallback standalone mismatch.\n", stderr);
        return 1;
    }
    assertions += 2;

    static const struct {
        uint16_t characters[2];
        OCFIndex numChars;
        OCFIndex maxByteLen;
        OCFIndex result;
        OCFIndex used;
        const char *bytes;
    } toBytes[] = {
        {{0x0041, 0}, 1, 1, 1, 1, "\xc1"},   {{0x009f, 0}, 1, 1, 1, 1, "\x1f"},
        {{0x00c6, 0}, 1, 0, 1, 2, ""},       {{0x00c6, 0}, 1, 1, 1, 1, "A"},
        {{0x00c6, 0}, 1, 5, 1, 2, "AE"},     {{0x00be, 0}, 1, 0, 1, 3, ""},
        {{0x00be, 0}, 1, 4, 1, 3, "3/4"},    {{0x00a7, 0}, 1, 4, 1, 0, ""},
        {{0x00e9, 0}, 1, -1, 1, 0, ""},      {{0xd800, 0xdc00}, 2, 1, 2, 1, "?"},
        {{0xd800, 0xdc00}, 1, 1, 1, 1, "?"}, {{0xd800, 0xe000}, 2, 1, 1, 1, "?"},
        {{0xdc00, 0xdc01}, 2, 1, 2, 1, "?"}, {{0xdc01, 0xdc00}, 2, 1, 1, 1, "?"},
        {{0x2026, 0}, 1, 0, 1, 3, ""},       {{0x2026, 0}, 1, 3, 1, 3, "..."},
        {{0x2026, 0}, 1, 2, 1, 1, "?"},      {{0x3000, 0}, 1, 1, 1, 1, " "},
        {{0x2028, 0}, 1, 1, 1, 1, "\n"},     {{0x0100, 0}, 1, 1, 1, 1, "A"},
        {{0x4e00, 0}, 1, 1, 1, 1, "?"},
    };

    for (size_t index = 0; index < sizeof(toBytes) / sizeof(toBytes[0]); ++index) {
        uint8_t bytes[8];
        memset(bytes, 0xa5, sizeof(bytes));
        used = -1;
        OCFIndex result = __CFDefaultToBytesFallbackProc(
            toBytes[index].characters, toBytes[index].numChars, bytes, toBytes[index].maxByteLen, &used
        );
        size_t written = strlen(toBytes[index].bytes);
        if (result != toBytes[index].result || used != toBytes[index].used ||
            memcmp(bytes, toBytes[index].bytes, written) != 0 || bytes[written] != 0xa5) {
            fprintf(stderr, "To-bytes fallback standalone mismatch: %04x.\n", toBytes[index].characters[0]);
            return 1;
        }
        ++assertions;
    }
    if (__CFStringEncodingPlatformUnicodeToBytes(0, 0, NULL, 0, NULL, NULL, 0, NULL) != 3 ||
        __CFStringEncodingPlatformBytesToUnicode(0, 0, NULL, 0, NULL, NULL, 0, NULL) != 3 ||
        __CFStringEncodingPlatformCharLengthForBytes(0, 0, NULL, 0) != 0) {
        fputs("Platform converter standalone mismatch.\n", stderr);
        return 1;
    }
    assertions += 3;
    const StringEncodingConverterRecord identity = {
        .toBytes.cheapEightBit = identityToBytes,
        .toUnicode.cheapEightBit = identityToUnicode,
    };
    const uint8_t text[] = {0x41, 0xc0, 0x42, 0x00, 0x43};
    uint16_t unicode[12];
    memset(unicode, 0xa5, sizeof(unicode));
    used = -1;
    if (__CFToUnicodeCheapEightBitWrapper(&identity, 0, text, 5, unicode, 0, &used) != 3 || used != 3 ||
        unicode[0] != 0xa5a5) {
        fputs("Cheap eight-bit count standalone mismatch.\n", stderr);
        return 1;
    }
    ++assertions;
    // U+00C0 decomposes to U+0041 U+0300 through the canonical wrapper.
    if (__CFToCanonicalUnicodeCheapEightBitWrapper(&identity, 0, text, 3, unicode, 8, &used) != 3 ||
        used != 4 || unicode[0] != 0x41 || unicode[1] != 0x41 || unicode[2] != 0x300 || unicode[3] != 0x42 ||
        unicode[4] != 0xa5a5) {
        fputs("Canonical cheap eight-bit standalone mismatch.\n", stderr);
        return 1;
    }
    ++assertions;
    // A decomposition that does not fit returns at once and keeps the count stored before it.
    if (__CFToCanonicalUnicodeCheapEightBitWrapper(&identity, 0, text, 3, unicode, 2, &used) != 1 ||
        used != 1) {
        fputs("Canonical cheap eight-bit overflow standalone mismatch.\n", stderr);
        return 1;
    }
    ++assertions;
    const uint16_t wide[] = {0x41, 0x42, 0x100, 0x43};
    uint8_t narrow[8];
    memset(narrow, 0xa5, sizeof(narrow));
    if (__CFToBytesCheapEightBitWrapper(&identity, 0, wide, 4, narrow, 8, &used) != 2 || used != 2 ||
        narrow[0] != 0x41 || narrow[1] != 0x42 || narrow[2] != 0xa5) {
        fputs("Cheap eight-bit to-bytes standalone mismatch.\n", stderr);
        return 1;
    }
    ++assertions;
    const StringEncodingConverterRecord pairs = {.toUnicode.standardEightBit = pairToUnicode};
    memset(unicode, 0xa5, sizeof(unicode));
    if (__CFToUnicodeStandardEightBitWrapper(&pairs, 0, text, 3, unicode, 5, &used) != 2 || used != 4 ||
        unicode[0] != 0x41 || unicode[1] != 0x301 || unicode[2] != 0xc0 || unicode[4] != 0xa5a5) {
        fputs("Standard eight-bit standalone mismatch.\n", stderr);
        return 1;
    }
    ++assertions;
    printf("String-encoding name/converter standalone cases passed: %zu.\n", assertions);
    return 0;
}
