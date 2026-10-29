//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Unicode/UnicodeStandaloneTests.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"

#include <stdio.h>
#include <string.h>

// Native raw Unicode primitives: 0x1c02512c4, 0x1c0251524, 0x1c0252910, 0x1c0252754.
// iOS Simulator 24A434 UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// Device comparison: 0x1806d523c, 0x1806400ec, 0x1806fc7b4, 0x18077483c,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
int runCoreFoundationUnicodeStandaloneTests(void) {
    size_t cases = 0;
    const uint32_t whitespace[] = {9, 32, 160, 5760, 0x2000, 0x200b, 8239, 8287, 12288};
    for (size_t index = 0; index < sizeof(whitespace) / sizeof(whitespace[0]); ++index) {
        if (!OCFUniCharIsMemberOf(whitespace[index], 2)) {
            fprintf(stderr, "Unicode standalone mismatch at source line %d.\n", __LINE__);
            return 1;
        }
        ++cases;
    }
    if (OCFUniCharIsMemberOf(10, 2) || !OCFUniCharIsMemberOf(10, 3) || !OCFUniCharIsMemberOf(0x41, 5) ||
        OCFUniCharIsMemberOf(0x41, 4)) {
        fprintf(stderr, "Unicode standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    cases += 4;
    uint32_t output[10];
    memset(output, 0xa5, sizeof(output));
    if (OCFUniCharDecomposeCharacter(0xe9, output + 1, 8) != 2 || output[1] != 0x65 || output[2] != 0x301) {
        fprintf(stderr, "Unicode standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    ++cases;
    memset(output, 0xa5, sizeof(output));
    if (OCFUniCharDecomposeCharacter(0x1fa, output + 1, 3) != 0 || output[1] != UINT32_C(0xa5a5a5a5)) {
        fprintf(stderr, "Unicode standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    ++cases;
    if (OCFUniCharDecomposeCharacter(0xac01, output + 1, 8) != 3 || output[1] != 0x1100 ||
        output[2] != 0x1161 || output[3] != 0x11a8) {
        fprintf(stderr, "Unicode standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    ++cases;
    if (OCFUniCharDecomposeCharacter(0xd7a4, output + 1, 8) != 2 || output[1] != 0x1113 ||
        output[2] != 0x1161) {
        fprintf(stderr, "Unicode standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    ++cases;
    const uint32_t scalars[] = {0x41, 0xe9, 0x1f600};
    const uint8_t expected[] = {0x41, 0xc3, 0xa9, 0xf0, 0x9f, 0x98, 0x80};
    uint8_t bytes[32];
    memset(bytes, 0xa5, sizeof(bytes));
    void *destination = bytes;
    OCFIndex used = 0;
    if (!OCFUniCharFillDestinationBuffer(scalars, 3, &destination, 32, &used, 2) || used != 7 ||
        destination != bytes + 7 || memcmp(bytes, expected, sizeof(expected)) != 0) {
        fprintf(stderr, "Unicode standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    ++cases;
    destination = bytes;
    used = 0;
    memset(bytes, 0xa5, sizeof(bytes));
    if (OCFUniCharFillDestinationBuffer(scalars, 3, &destination, 2, &used, 2) || destination != bytes ||
        used != 0 || bytes[0] != 0x41 || bytes[1] != 0xa5) {
        fprintf(stderr, "Unicode standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    ++cases;
    destination = 0;
    used = 0;
    if (!OCFUniCharFillDestinationBuffer(scalars, 3, &destination, 0, &used, 2) || destination != 0 ||
        used != 7) {
        fprintf(stderr, "Unicode standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    ++cases;
    printf("Unicode standalone cases passed: %zu (raw primitives).\n", cases);
    return 0;
}
