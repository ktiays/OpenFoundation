//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/CharacterSet/CharacterSetHelperStandaloneTests.h"
#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Native chcompar 0x1c0174a34, iOS Simulator 24A434 UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Device comparison: 0x180718e44, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationCharacterSetHelperStandaloneTests(void) {
    size_t cases = 0;
    uint16_t low = 1;
    uint16_t high = 0xffff;
    if (chcompar(&low, &high) != 1 - 0xffff || chcompar(&high, &low) != 0xffff - 1 ||
        chcompar(&low, &low) != 0) {
        fputs("Character-set comparator standalone mismatch.\n", stderr);
        return 1;
    }
    cases += 3;
    uint16_t units[6] = {'z', 0x2028, 'a', 0, 0xffff, 'a'};
    qsort(units, 6, sizeof(uint16_t), chcompar);
    static const uint16_t kSorted[6] = {0, 'a', 'a', 'z', 0x2028, 0xffff};
    for (size_t index = 0; index < 6; ++index) {
        if (units[index] != kSorted[index]) {
            fputs("Character-set comparator standalone sort mismatch.\n", stderr);
            return 1;
        }
    }
    ++cases;
    printf("Character-set comparator standalone cases passed: %zu.\n", cases);
    return 0;
}
