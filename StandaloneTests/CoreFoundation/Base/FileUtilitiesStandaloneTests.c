//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Base/FileUtilitiesStandaloneTests.h"
#include "CoreFoundation/Base/OCFFileUtilities.h"

#include <stdio.h>

static int checkPath(const char *text, OCFIndex start, OCFIndex lengthAfterDeleting, int line) {
    uint16_t units[32];
    OCFIndex length = 0;
    while (text[length] != '\0') {
        units[length] = (uint8_t)text[length];
        ++length;
    }
    if (_CFStartOfLastPathComponent(units, length) != start ||
        _CFLengthAfterDeletingLastPathComponent(units, length) != lengthAfterDeleting) {
        fprintf(stderr, "File-utility path standalone mismatch at source line %d.\n", line);
        return 1;
    }
    return 0;
}

// Native _CFGetSlash 0x1c019cefc, _CFStartOfLastPathComponent 0x1c019d374 and
// _CFLengthAfterDeletingLastPathComponent 0x1c019d458, iOS Simulator 24A434
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Device comparison: 0x18066f0b8,
// 0x180698360, 0x18069ab0c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified for the
// inline bodies.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationFileUtilitiesStandaloneTests(void) {
    size_t cases = 0;
    if (_CFGetSlash() != '/') {
        fputs("File-utility path standalone mismatch: _CFGetSlash.\n", stderr);
        return 1;
    }
    ++cases;

    static const struct {
        const char *text;
        OCFIndex start;
        OCFIndex lengthAfterDeleting;
    } kCases[] = {
        {"", 0, 0},
        {"/", 0, 0},
        {"a/", 0, 0},
        {"/a", 1, 1},
        {"/a/", 1, 1},
        {"/usr/lib", 5, 4},
        {"/usr/lib/", 5, 4},
        {"usr", 0, 0},
        {"C:", 0, 0},
        {"C:x", 2, 2},
        {"C:/x", 3, 3},
        {"c:/x/y", 5, 4},
        {"[:/x", 3, 2},
        {"1:x", 0, 0},
    };

    for (size_t index = 0; index < sizeof(kCases) / sizeof(kCases[0]); ++index) {
        if (checkPath(kCases[index].text, kCases[index].start, kCases[index].lengthAfterDeleting, __LINE__) !=
            0) {
            fprintf(stderr, "Case \"%s\".\n", kCases[index].text);
            return 1;
        }
        ++cases;
    }
    printf("File-utility path standalone cases passed: %zu.\n", cases);
    return 0;
}
