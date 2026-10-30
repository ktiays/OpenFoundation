//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/CharacterSet/CharacterSetHelperTests.h"
#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <stdint.h>
#include <stdio.h>

typedef int (*CompareFunction)(const void *, const void *);

// Native chcompar 0x1c0174a34 (non-external; resolved by IDA offset), iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device 24A446: 0x180718e44,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// The 32-bit result is compared exactly for every first unit against every 257th second unit and
// for every second unit against every 257th first unit.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationCharacterSetHelperDifferentialTests(void) {
    void *function = NULL;
    int status = resolveReferenceAddress(
        "CoreFoundation", OF_REFERENCE_CF_UUID, UINT64_C(0x1c0174a34), UINT64_C(0x1c014a000), &function
    );
    if (status == 0) {
        status = checkInternalImplementation((const void *)chcompar);
    }
    if (status == 0 && function == (void *)chcompar) {
        fputs("Setup failure: character-set helper reference resolved to the replacement.\n", stderr);
        status = 77;
    }
    if (status != 0) {
        return status;
    }
    CompareFunction native = (CompareFunction)function;
    size_t cases = 0;
    for (uint32_t first = 0; first < 0x10000; ++first) {
        for (uint32_t second = first % 257; second < 0x10000; second += 257) {
            uint16_t pair[2][2] = {{(uint16_t)first, (uint16_t)second}, {(uint16_t)second, (uint16_t)first}};
            for (size_t order = 0; order < 2; ++order) {
                if (native(&pair[order][0], &pair[order][1]) != chcompar(&pair[order][0], &pair[order][1])) {
                    fprintf(stderr, "chcompar mismatch: %04x %04x.\n", pair[order][0], pair[order][1]);
                    return 1;
                }
                ++cases;
            }
        }
    }
    printf(
        "Character-set comparator differential cases passed: %zu (one function; finite coverage).\n", cases
    );
    return 0;
}
