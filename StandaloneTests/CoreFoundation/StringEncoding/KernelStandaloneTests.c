//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "KernelStandaloneTests.h"

#include "CoreFoundation/StringEncoding/KernelCases.h"

#include <stdio.h>
#include <string.h>

int runStringEncodingKernelStandaloneTests(void) {
    size_t assertions = 0;
    for (size_t index = 0; index < sizeof(kUTF8KernelCases) / sizeof(kUTF8KernelCases[0]); ++index) {
        const UTF8KernelCase *test = &kUTF8KernelCases[index];
        uint8_t actual[40];
        uint8_t expected[40];
        memset(actual, 0xa5, sizeof(actual));
        memset(expected, 0xa5, sizeof(expected));
        if (test->capacity != 0) {
            memcpy(expected + 4, test->bytes, (size_t)test->used);
        }
        OCFIndex used = -123456789;
        OCFIndex consumed =
            __CFToUTF8(test->flags, test->input, test->count, actual + 4, test->capacity, &used);
        if (consumed != test->consumed || used != test->used ||
            memcmp(actual, expected, sizeof(actual)) != 0) {
            fprintf(
                stderr,
                "UTF8 standalone mismatch: %s, consumed %ld/%ld, used %ld/%ld.\n",
                test->name,
                consumed,
                test->consumed,
                used,
                test->used
            );
            return 1;
        }
        ++assertions;
    }
    const uint16_t lengthInput[] = {0x41, 0xa2, 0x20ac, 0xd83d, 0xde00, 0xd800, 0xdc00, 0xffff};
    if (__CFToUTF8Len(0, lengthInput, 0) != 0 || __CFToUTF8Len(0, lengthInput, 8) != 17 ||
        __CFToUTF8Len(UINT32_MAX, lengthInput, 8) != 17 || __CFToUTF8Len(0, lengthInput + 5, 1) != 3) {
        fprintf(stderr, "UTF8 length kernel standalone mismatch.\n");
        return 1;
    }
    assertions += 4;
    uint8_t byte = 0xa5;
    uint16_t character = 0xa55a;
    if (__CFToASCII(0, 0x80, &byte) != 0 || byte != 0xa5 || __CFFromASCII(0, 0x80, &character) != 0 ||
        character != 0xa55a || __CFToASCII(0, 0x7f, &byte) != 1 || byte != 0x7f ||
        __CFFromASCII(0, 0x7f, &character) != 1 || character != 0x7f) {
        fprintf(stderr, "ASCII kernel standalone mismatch.\n");
        return 1;
    }
    assertions += 4;
    byte = 0xa5;
    if (__CFToISOLatin1(0, 0x100, &byte) != 0 || byte != 0xa5 || __CFToISOLatin1(0, 0xff, &byte) != 1 ||
        byte != 0xff || __CFFromISOLatin1(0, 0xff, &character) != 1 || character != 0xff) {
        fprintf(stderr, "ISO Latin1 kernel standalone mismatch.\n");
        return 1;
    }
    assertions += 3;
    if (__CFToWinLatin1(0, 0x20ac, &byte) != 1 || byte != 0x80 ||
        __CFFromWinLatin1(0, 0x80, &character) != 1 || character != 0x20ac ||
        __CFFromWinLatin1(0, 0x81, &character) != 0 || character != 0xfffd ||
        __CFFromNextStepLatin(0, 0xff, &character) != 0 || character != 0xfffd) {
        fprintf(stderr, "Single-byte undefined mapping/write-order standalone mismatch.\n");
        return 1;
    }
    assertions += 4;
    if (__CFToDOSLatin1(0, 0x25a0, &byte) != 1 || byte != 0xfe ||
        __CFFromDOSLatin1(0, 0xfe, &character) != 1 || character != 0x25a0 ||
        __CFToMacRomanLatin1(0, 0xfb02, &byte) != 1 || byte != 0xfe ||
        __CFFromMacRomanLatin1(0, 0xfe, &character) != 1 || character != 0xfb02) {
        fprintf(stderr, "Single-byte map endpoint standalone mismatch.\n");
        return 1;
    }
    assertions += 4;
    printf("String encoding kernel standalone assertions passed: %zu.\n", assertions);
    return 0;
}
