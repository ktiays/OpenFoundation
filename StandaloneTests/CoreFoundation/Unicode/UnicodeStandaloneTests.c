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

// Native CFUniCharGetNumberOfPlanes 0x1c0251870, CFUniCharGetBitmapForPlane 0x1c02515a0,
// CFUniCharGetUnicodePropertyDataForPlane 0x1c025268c, CFUniCharPrioritySort 0x1c0253320.
// iOS Simulator 24A434 UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// Device comparison: 0x180715de4, 0x180715aa0, 0x18065673c, 0x1806fcc28,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
int runCoreFoundationUnicodePlaneStandaloneTests(void) {
    size_t cases = 0;
    if (CFUniCharGetNumberOfPlanes(1) != 15 || CFUniCharGetNumberOfPlanes(108) != 15 ||
        CFUniCharGetNumberOfPlanes(2) != 1 || CFUniCharGetNumberOfPlanes(12) != 17 ||
        CFUniCharGetNumberOfPlanes(4) != 2 || CFUniCharGetNumberOfPlanes(5) != 15) {
        fprintf(stderr, "Unicode plane standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    cases += 6;
    if (OCFUniCharGetUnicodePropertyDataForPlane(0, 0) == 0 ||
        OCFUniCharGetUnicodePropertyDataForPlane(0, 2) != 0 ||
        OCFUniCharGetUnicodePropertyDataForPlane(1, 13) != 0 ||
        OCFUniCharGetUnicodePropertyDataForPlane(1, 14) == 0 ||
        OCFUniCharGetUnicodePropertyDataForPlane(1, 15) != 0) {
        fprintf(stderr, "Unicode plane standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    cases += 5;
    // U+0301 and U+0316 have nonzero combining priorities in plane 0; a leading base letter is skipped.
    const uint8_t *priorities = OCFUniCharGetUnicodePropertyDataForPlane(0, 0);
    uint32_t marks[] = {0x61, 0x301, 0x316, 0x301};
    OCFUniCharPrioritySort(marks, 4);
    uint8_t previous = 0;
    for (size_t index = 1; index < 4; ++index) {
        uint32_t character = marks[index];
        uint8_t page = priorities[character >> 8];
        uint8_t priority = page ? priorities[page * 256 + (character & 0xff)] : 0;
        if (priority == 0 || priority < previous) {
            fprintf(stderr, "Unicode plane standalone mismatch at source line %d.\n", __LINE__);
            return 1;
        }
        previous = priority;
    }
    if (marks[0] != 0x61) {
        fprintf(stderr, "Unicode plane standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    cases += 4;
    static uint8_t bitmap[8192];
    if (CFUniCharGetBitmapForPlane(2, 0, bitmap, false) != 0 || !(bitmap[1] & 0x02) || (bitmap[1] & 0x04) ||
        !(bitmap[0x2000 >> 3] & 1)) {
        fprintf(stderr, "Unicode plane standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    if (CFUniCharGetBitmapForPlane(3, 0, bitmap, false) != 0 || !(bitmap[1] & 0x04)) {
        fprintf(stderr, "Unicode plane standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    if (CFUniCharGetBitmapForPlane(2, 1, bitmap, false) != 0xff ||
        CFUniCharGetBitmapForPlane(2, 1, bitmap, true) != 1 ||
        CFUniCharGetBitmapForPlane(12, 13, bitmap, false) != 1) {
        fprintf(stderr, "Unicode plane standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    cases += 5;
    printf("Unicode plane standalone cases passed: %zu (finite coverage).\n", cases);
    return 0;
}

static int expectMapping(
    uint32_t character,
    OCFIndex maxLength,
    uint32_t context,
    uint32_t flags,
    const char *language,
    OCFIndex expectedLength,
    const uint16_t *expected,
    int line
) {
    uint16_t converted[8] = {0};
    OCFIndex length =
        OCFUniCharMapCaseTo(character, converted, maxLength, context, flags, (const uint8_t *)language);
    if (length != expectedLength ||
        memcmp(converted, expected, (size_t)expectedLength * sizeof(uint16_t)) != 0) {
        fprintf(stderr, "Unicode case standalone mismatch at source line %d.\n", line);
        return 1;
    }
    return 0;
}

// Native CFUniCharMapCaseTo 0x1c02518e0, CFUniCharGetConditionalCaseMappingFlags 0x1c0252064,
// CFUniCharCompatibilityDecompose 0x1c0253068, __CFUniCharRecursivelyCompatibilityDecomposeCharacter
// 0x1c0253188. iOS Simulator 24A434 UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Device
// comparison: 0x1806cb400 (with cold bodies 0x1807c7c18, 0x1807c7cdc), 0x180774138, 0x180700fac, 0x180700d14,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified. Reading:
// Documentation/Reference/Research/CoreFoundationUnicode.md.
int runCoreFoundationUnicodeCaseStandaloneTests(void) {
    size_t cases = 0;
    static const uint16_t upperA[] = {0x41};
    static const uint16_t sharpS[] = {0x53, 0x53};
    static const uint16_t sharpSelf[] = {0xdf};
    static const uint16_t finalSigma[] = {0x3c2};
    static const uint16_t dotlessI[] = {0x131};
    static const uint16_t dottedI[] = {0x130};
    static const uint16_t upperJ[] = {0x4a};
    static const uint16_t lithuanianGrave[] = {0x69, 0x307, 0x300};
    static const uint16_t deseret[] = {0xd801, 0xdc28};
    static const uint16_t titleDz[] = {0x1c5};
    static const uint16_t upperNapostrophe[] = {0x2bc, 0x4e};
    if (expectMapping(0x61, 8, 1, 0, 0, 1, upperA, __LINE__) ||
        expectMapping(0xdf, 8, 1, 0, 0, 2, sharpS, __LINE__) ||
        expectMapping(0xdf, 2, 1, 0, 0, 1, sharpSelf, __LINE__) ||
        expectMapping(0x3a3, 8, 0, 1, 0, 1, finalSigma, __LINE__) ||
        expectMapping(0x49, 8, 0, 0, "tr", 1, dotlessI, __LINE__) ||
        expectMapping(0x69, 8, 1, 0, "az", 1, dottedI, __LINE__) ||
        expectMapping(0x6a, 8, 2, 0, "nl", 1, upperJ, __LINE__) ||
        expectMapping(0xcc, 8, 0, 0, "lt", 3, lithuanianGrave, __LINE__) ||
        expectMapping(0x10400, 8, 0, 0, 0, 2, deseret, __LINE__) ||
        expectMapping(0x1c4, 8, 2, 0, 0, 1, titleDz, __LINE__) ||
        expectMapping(0x149, 8, 2, 0, 0, 2, upperNapostrophe, __LINE__) ||
        expectMapping(0x301, 8, 1, 0x10, "el", 0, 0, __LINE__)) {
        return 1;
    }
    cases += 12;
    // Final sigma: cased before, nothing cased after; a following cased letter clears it.
    static const uint16_t sigmaEnd[] = {0x391, 0x3a3, 0x20};
    static const uint16_t sigmaMiddle[] = {0x391, 0x3a3, 0x391};
    static const uint16_t turkishDot[] = {0x49, 0x307};
    static const uint16_t dutchIJ[] = {0x49, 0x6a};
    if (CFUniCharGetConditionalCaseMappingFlags(0x3a3, sigmaEnd, 1, 3, 0, 0, 0) != 1 ||
        CFUniCharGetConditionalCaseMappingFlags(0x3a3, sigmaMiddle, 1, 3, 0, 0, 0) != 0 ||
        CFUniCharGetConditionalCaseMappingFlags(0x49, turkishDot, 0, 2, 0, (const uint8_t *)"tr", 0) != 4 ||
        CFUniCharGetConditionalCaseMappingFlags(0x49, dutchIJ, 0, 2, 2, (const uint8_t *)"nl", 0) != 8 ||
        CFUniCharGetConditionalCaseMappingFlags(0x391, sigmaEnd, 0, 3, 1, (const uint8_t *)"en", 0) != 0x10 ||
        CFUniCharGetConditionalCaseMappingFlags(0x41, sigmaEnd, 0, 3, 1, 0, 0) != 0) {
        fprintf(stderr, "Unicode case standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    cases += 6;
    // The Lithuanian scan tests the unit two places after a high surrogate and combines with the next one:
    // U+D834 U+DD85 (U+1D185, priority 230) combines only when a low surrogate follows it in range. With
    // only two units left, the CoreFoundation defect fix never reads buffer[length] (a low surrogate here).
    static const uint16_t marks[] = {0x69, 0xd834, 0xdd85, 0xdc00};
    static const uint16_t unpaired[] = {0x69, 0xd834, 0xdd85, 0x41};
    if (CFUniCharGetConditionalCaseMappingFlags(0x69, marks, 0, 3, 1, (const uint8_t *)"lt", 0) != 0 ||
        CFUniCharGetConditionalCaseMappingFlags(0x69, marks, 0, 4, 1, (const uint8_t *)"lt", 0) != 6 ||
        CFUniCharGetConditionalCaseMappingFlags(0x69, unpaired, 0, 4, 1, (const uint8_t *)"lt", 0) != 0) {
        fprintf(stderr, "Unicode case standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    cases += 3;
    uint32_t ligature[4] = {0x41, 0xfb01, 0x42, 0};
    uint32_t half[4] = {0xbd, 0, 0, 0};
    uint32_t plain[2] = {0x41, 0x301};
    if (OCFUniCharCompatibilityDecompose(ligature, 3, 4) != 4 || ligature[0] != 0x41 || ligature[1] != 0x66 ||
        ligature[2] != 0x69 || ligature[3] != 0x42 || OCFUniCharCompatibilityDecompose(half, 1, 2) != 0 ||
        OCFUniCharCompatibilityDecompose(half, 1, 3) != 3 || half[0] != 0x31 || half[1] != 0x2044 ||
        half[2] != 0x32 || OCFUniCharCompatibilityDecompose(plain, 2, 2) != 2 || plain[1] != 0x301 ||
        OCFUniCharCompatibilityDecompose(plain, 0, 0) != 0) {
        fprintf(stderr, "Unicode case standalone mismatch at source line %d.\n", __LINE__);
        return 1;
    }
    cases += 5;
    printf("Unicode case standalone cases passed: %zu (finite coverage).\n", cases);
    return 0;
}
