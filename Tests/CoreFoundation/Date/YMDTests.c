//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Date/YMDTests.h"
#include "CoreFoundation/Date/OCFDate.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <stdio.h>
#include <string.h>

typedef double (*AbsoluteFromYMDFunction)(int64_t, int8_t, int8_t);

static uint64_t nextRandom(uint64_t *state) {
    uint64_t value = (*state += UINT64_C(0x9e3779b97f4a7c15));
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

static int compare(AbsoluteFromYMDFunction native, int64_t year, int8_t month, int8_t day) {
    double expected = native(year, month, day);
    double actual = __CFAbsoluteFromYMD(year, month, day);
    if (memcmp(&expected, &actual, sizeof(double)) != 0) {
        fprintf(
            stderr,
            "__CFAbsoluteFromYMD mismatch: year %lld, month %d, day %d.\n",
            (long long)year,
            month,
            day
        );
        return 1;
    }
    return 0;
}

// Native __CFAbsoluteFromYMD 0x1c018dee0 (non-external; resolved by IDA offset), iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device 24A446: 0x1806d56e4,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Results are compared as bit patterns. Years -4000..4000 (relative to 2001) run over every 8-bit month
// with a random day; years -60..60 over every month and day; random years up to 2^40 in magnitude and
// the 64-bit extremes over random months and days.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationYMDDifferentialTests(void) {
    void *function = NULL;
    int status = resolveReferenceAddress(
        "CoreFoundation", OF_REFERENCE_CF_UUID, UINT64_C(0x1c018dee0), UINT64_C(0x1c014a000), &function
    );
    if (status == 0) {
        status = checkInternalImplementation((const void *)__CFAbsoluteFromYMD);
    }
    if (status == 0 && function == (void *)__CFAbsoluteFromYMD) {
        fputs("Setup failure: __CFAbsoluteFromYMD reference resolved to the replacement.\n", stderr);
        status = 77;
    }
    if (status != 0) {
        return status;
    }
    AbsoluteFromYMDFunction native = (AbsoluteFromYMDFunction)function;
    size_t cases = 0;
    uint64_t state = UINT64_C(0x7d0a1b2c3d4e5f60);
    for (int64_t year = -4000; year <= 4000; ++year) {
        for (int month = -128; month < 128; ++month) {
            if (compare(native, year, (int8_t)month, (int8_t)nextRandom(&state)) != 0) {
                return 1;
            }
            ++cases;
        }
    }
    for (int64_t year = -60; year <= 60; ++year) {
        for (int month = -128; month < 128; ++month) {
            for (int day = -128; day < 128; ++day) {
                if (compare(native, year, (int8_t)month, (int8_t)day) != 0) {
                    return 1;
                }
                ++cases;
            }
        }
    }
    const int64_t extremes[] = {INT64_MIN, INT64_MIN + 1, INT64_MIN + 399, INT64_MAX, INT64_MAX - 399};
    for (size_t index = 0; index < sizeof(extremes) / sizeof(extremes[0]); ++index) {
        for (int month = -128; month < 128; ++month) {
            if (compare(native, extremes[index], (int8_t)month, (int8_t)nextRandom(&state)) != 0) {
                return 1;
            }
            ++cases;
        }
    }
    for (uint32_t round = 0; round < 1000000; ++round) {
        uint64_t bits = nextRandom(&state);
        int64_t year = (int64_t)(nextRandom(&state) >> (24 + (bits & 15)));
        if (bits & 16) {
            year = -year;
        }
        if (compare(native, year, (int8_t)(bits >> 8), (int8_t)(bits >> 16)) != 0) {
            return 1;
        }
        ++cases;
    }
    printf("__CFAbsoluteFromYMD differential cases passed: %zu (one function; finite coverage).\n", cases);
    return 0;
}
