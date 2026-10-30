//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/URL/OCFURL.h"

// Native _netLocationRange, iOS Simulator 24A434, 0x1c0244938, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external. Callers pass the range array of a URL (X19 + 0x38 in
// CFURLCopyNetLocation) or a local array (resolveAbsoluteURLStringBuffer); the function reads only
// that array.
// iOS Device 24A446: 0x180712ef4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// indexes the four collected ranges by a counter and authenticates its return; the results agree.
// Without any of flag bits 1..4 the result is {-1, 0}. Otherwise the ranges of bits 1..4 are collected
// in order, each at the index equal to the number of lower set bits ({-1, 0} for a clear bit; bit 4
// counts bits 0..3 in a loop). The first collected range whose location is not -1 starts the result;
// each later one sets length = location - start + its length, with 64-bit wraparound.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
OCFRange _netLocationRange(uint32_t flags, const OCFRange *ranges) {
    if ((flags & 0x1e) == 0) {
        return (OCFRange){-1, 0};
    }
    OCFRange components[4];
    for (uint32_t component = 0; component < 4; ++component) {
        uint32_t bit = UINT32_C(2) << component;
        if ((flags & bit) == 0) {
            components[component] = (OCFRange){-1, 0};
            continue;
        }
        uint32_t index = 0;
        for (uint32_t lower = 1; lower < bit; lower <<= 1) {
            if (flags & lower) {
                ++index;
            }
        }
        components[component] = ranges[index];
    }
    OCFRange result = {-1, 0};
    for (uint32_t component = 0; component < 4; ++component) {
        if (components[component].location == -1) {
            continue;
        }
        if (result.location == -1) {
            result = components[component];
        } else {
            result.length = (OCFIndex)((uint64_t)components[component].location - (uint64_t)result.location +
                                       (uint64_t)components[component].length);
        }
    }
    return result;
}
