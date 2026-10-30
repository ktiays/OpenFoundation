//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/URL/URLHelperTests.h"
#include "CoreFoundation/URL/OCFURL.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <stdio.h>

typedef OCFRange (*NetLocationFunction)(uint32_t, const OCFRange *);

static uint64_t nextRandom(uint64_t *state) {
    uint64_t value = (*state += UINT64_C(0x9e3779b97f4a7c15));
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

// Native _netLocationRange 0x1c0244938 (non-external; resolved by IDA offset), iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device 24A446: 0x180712ef4,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Both returned words are compared exactly. The function takes component flags and the packed range
// array, not URL text: every combination of the low 16 flag bits, with random high bits, is called over
// arrays that mix absent (-1) and present ranges.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationURLHelperDifferentialTests(void) {
    void *function = NULL;
    int status = resolveReferenceAddress(
        "CoreFoundation", OF_REFERENCE_CF_UUID, UINT64_C(0x1c0244938), UINT64_C(0x1c014a000), &function
    );
    if (status == 0) {
        status = checkInternalImplementation((const void *)_netLocationRange);
    }
    if (status == 0 && function == (void *)_netLocationRange) {
        fputs("Setup failure: URL helper reference resolved to the replacement.\n", stderr);
        status = 77;
    }
    if (status != 0) {
        return status;
    }
    NetLocationFunction native = (NetLocationFunction)function;
    size_t cases = 0;
    uint64_t state = UINT64_C(0x5eed0f0b16a0301);
    OCFRange ranges[16];
    for (uint32_t round = 0; round < 64; ++round) {
        for (size_t index = 0; index < 16; ++index) {
            uint64_t bits = nextRandom(&state);
            ranges[index].location = (bits & 3) == 0 ? -1 : (OCFIndex)(nextRandom(&state) % 200);
            ranges[index].length =
                (bits & 12) == 0 ? (OCFIndex)nextRandom(&state) : (OCFIndex)(bits >> 40) % 50;
            if (round >= 48) {
                ranges[index].location = (OCFIndex)nextRandom(&state);
            }
        }
        for (uint32_t low = 0; low < 0x10000; ++low) {
            uint32_t flags = low | (round & 1 ? (uint32_t)nextRandom(&state) & 0xffff0000u : 0);
            OCFRange expected = native(flags, ranges);
            OCFRange actual = _netLocationRange(flags, ranges);
            if (expected.location != actual.location || expected.length != actual.length) {
                fprintf(stderr, "_netLocationRange mismatch: flags 0x%x, round %u.\n", flags, round);
                return 1;
            }
            ++cases;
        }
    }
    printf("URL net-location differential cases passed: %zu (one function; finite coverage).\n", cases);
    return 0;
}
