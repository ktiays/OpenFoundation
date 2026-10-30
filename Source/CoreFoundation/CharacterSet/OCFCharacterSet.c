//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"

#include <stdint.h>

// Native chcompar, iOS Simulator 24A434, 0x1c0174a34, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; no direct caller (an address-taken comparator between
// CFCharacterSetCreateWithCharactersInString and CFCharacterSetCreateMutable).
// iOS Device 24A446: 0x180718e44, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions.
// Returns the first UTF-16 unit minus the second as a 32-bit int (LDRH, LDRH, SUB W0).
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int chcompar(const void *first, const void *second) {
    return (int)*(const uint16_t *)first - (int)*(const uint16_t *)second;
}
