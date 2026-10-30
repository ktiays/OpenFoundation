//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/StringHelperStandaloneTests.h"
#include "CoreFoundation/String/OCFString.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static FormatSpec makeSpec(int16_t type, int16_t size, int8_t main, int8_t precision, int8_t width) {
    FormatSpec spec;
    memset(&spec, 0, sizeof(spec));
    spec.type = type;
    spec.size = size;
    spec.mainArgNum = main;
    spec.precArgNum = precision;
    spec.widthArgNum = width;
    return spec;
}

static int fail(int line) {
    fprintf(stderr, "String helper standalone mismatch at source line %d.\n", line);
    return 1;
}

// Native __CFStringFindTypeAndSizeForArgumentIndex 0x1c022452c, __CFStringValidateFormatSpecBuffers
// 0x1c02242fc and __rangeEqual 0x1c0218d64, iOS Simulator 24A434 UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Device comparison: 0x1806d64a8, 0x1806ba570, 0x180764320,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationStringHelperStandaloneTests(void) {
    size_t cases = 0;
    if (sizeof(FormatSpec) != 32 || offsetof(FormatSpec, mainArgNum) != 24 ||
        offsetof(FormatSpec, widthArgNum) != 26) {
        return fail(__LINE__);
    }
    ++cases;
    // "%*.*d": width takes argument 0, precision 1, the value 2.
    FormatSpec star = makeSpec(0x22, 4, -1, -2, -2);
    int16_t type = 0;
    int16_t size = 0;
    if (!__CFStringFindTypeAndSizeForArgumentIndex(&star, 1, 1, &type, &size) || type != 0x21 || size != 3) {
        return fail(__LINE__);
    }
    ++cases;
    if (!__CFStringFindTypeAndSizeForArgumentIndex(&star, 1, 2, &type, &size) || type != 0x22 || size != 4) {
        return fail(__LINE__);
    }
    ++cases;
    type = 7;
    size = 9;
    if (__CFStringFindTypeAndSizeForArgumentIndex(&star, 1, 3, &type, &size) || type != 7 || size != 9) {
        return fail(__LINE__);
    }
    ++cases;
    // Explicit numbers and a second record: the first record that names the index wins.
    FormatSpec pair[2] = {makeSpec(0x25, 8, 1, -1, -1), makeSpec(0x26, 2, -1, -1, -1)};
    if (!__CFStringFindTypeAndSizeForArgumentIndex(pair, 2, 0, &type, &size) || type != 0x26 || size != 2) {
        return fail(__LINE__);
    }
    ++cases;
    // Validation: identical buffers pass; a '#' (0x23) record accepts 0x25..0x28 and 0x21.
    if (!__CFStringValidateFormatSpecBuffers(&star, 1, &star, 1, 0)) {
        return fail(__LINE__);
    }
    ++cases;
    FormatSpec hash = makeSpec(0x23, 2, 0, -1, -1);
    FormatSpec found = makeSpec(0x27, 0, 0, -1, -1);
    if (!__CFStringValidateFormatSpecBuffers(&hash, 1, &found, 1, 0)) {
        return fail(__LINE__);
    }
    ++cases;
    found.type = 0x29;
    if (__CFStringValidateFormatSpecBuffers(&hash, 1, &found, 1, 0)) {
        return fail(__LINE__);
    }
    ++cases;
    // Sizes above 4 must agree exactly; sizes up to 4 compare as 4.
    FormatSpec wide = makeSpec(0x23, 8, 0, -1, -1);
    found.type = 0x23;
    found.size = 4;
    if (__CFStringValidateFormatSpecBuffers(&wide, 1, &found, 1, 0)) {
        return fail(__LINE__);
    }
    ++cases;
    // A missing argument number fails; zero records pass.
    if (__CFStringValidateFormatSpecBuffers(&star, 1, &hash, 1, 0) ||
        !__CFStringValidateFormatSpecBuffers(&star, 0, &hash, 0, 0)) {
        return fail(__LINE__);
    }
    ++cases;
    OCFRange first = {3, 4};
    OCFRange second = {3, 4};
    OCFRange third = {3, 5};
    if (!__rangeEqual(&first, &second) || __rangeEqual(&first, &third)) {
        return fail(__LINE__);
    }
    ++cases;
    printf("String helper standalone cases passed: %zu.\n", cases);
    return 0;
}
