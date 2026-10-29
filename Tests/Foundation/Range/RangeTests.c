//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "Foundation/Range/RangeDifferentialTests.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <OpenFoundation/Range/ONSRange.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Native NSUnionRange: iOS Simulator 24A434, 0x1c05bd644, Foundation
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; iOS Device 24A446, 0x180f67520,
// C041F1D0-5EF1-30A3-BF96-830976173B56. Confidence: verified.
// Native NSIntersectionRange: same images, 0x1c05bd664 / 0x180e02318.
// Reading: Documentation/Reference/Research/FoundationRange.md.
// The full unsigned input domain is safe for these memory-free arithmetic leaves.
typedef ONSRange (*RangeOperation)(ONSRange range1, ONSRange range2);

typedef struct RangeFunctions {
    RangeOperation nativeUnion;
    RangeOperation replacementUnion;
    RangeOperation nativeIntersection;
    RangeOperation replacementIntersection;
} RangeFunctions;

static int resolveFunctions(RangeFunctions *functions) {
    void *address = NULL;
    int status = resolveReferenceFunction("Foundation", OF_REFERENCE_NS_UUID, "NSUnionRange", &address);
    if (status != 0) {
        return status;
    }
    functions->nativeUnion = (RangeOperation)address;
    status = resolveReferenceFunction("Foundation", OF_REFERENCE_NS_UUID, "NSIntersectionRange", &address);
    if (status != 0) {
        return status;
    }
    functions->nativeIntersection = (RangeOperation)address;
    status = resolveReplacementFunction("OpenFoundation", OF_NAME_STRING(ONSUnionRange), &address);
    if (status != 0) {
        return status;
    }
    functions->replacementUnion = (RangeOperation)address;
    status = resolveReplacementFunction("OpenFoundation", OF_NAME_STRING(ONSIntersectionRange), &address);
    if (status != 0) {
        return status;
    }
    functions->replacementIntersection = (RangeOperation)address;
    if (functions->nativeUnion == functions->replacementUnion ||
        functions->nativeIntersection == functions->replacementIntersection) {
        fputs("Setup failure: native and replacement range functions resolve to the same address.\n", stderr);
        return 77;
    }
    return 0;
}

static int compareFixture(const RangeFunctions *functions, const ONSUInteger values[4], size_t index) {
    ONSRange nativeFirst = {values[0], values[1]};
    ONSRange nativeSecond = {values[2], values[3]};
    ONSRange replacementFirst = {values[0], values[1]};
    ONSRange replacementSecond = {values[2], values[3]};
    ONSRange nativeUnion = functions->nativeUnion(nativeFirst, nativeSecond);
    ONSRange replacementUnion = functions->replacementUnion(replacementFirst, replacementSecond);
    ONSRange nativeIntersection = functions->nativeIntersection(nativeFirst, nativeSecond);
    ONSRange replacementIntersection =
        functions->replacementIntersection(replacementFirst, replacementSecond);
    if (memcmp(&nativeUnion, &replacementUnion, sizeof(nativeUnion)) != 0 ||
        memcmp(&nativeIntersection, &replacementIntersection, sizeof(nativeIntersection)) != 0) {
        fprintf(
            stderr,
            "Foundation range differential mismatch at fixture %zu: {%lu, %lu}, {%lu, %lu}.\n",
            index,
            values[0],
            values[1],
            values[2],
            values[3]
        );
        return 1;
    }
    return 0;
}

static uint64_t nextFixtureValue(uint64_t *state) {
    *state ^= *state >> 12;
    *state ^= *state << 25;
    *state ^= *state >> 27;
    return *state * UINT64_C(2685821657736338717);
}

int runFoundationRangeDifferentialTests(void) {
    RangeFunctions functions;
    int status = resolveFunctions(&functions);
    if (status != 0) {
        return status;
    }
    static const ONSUInteger kValues[] = {
        0,
        1,
        2,
        7,
        16,
        LONG_MAX - 1UL,
        LONG_MAX,
        LONG_MAX + 1UL,
        ULONG_MAX - 2,
        ULONG_MAX - 1,
        ULONG_MAX,
    };
    size_t count = sizeof(kValues) / sizeof(kValues[0]);
    size_t fixture = 0;
    for (size_t firstLocation = 0; firstLocation < count; ++firstLocation) {
        for (size_t firstLength = 0; firstLength < count; ++firstLength) {
            for (size_t secondLocation = 0; secondLocation < count; ++secondLocation) {
                for (size_t secondLength = 0; secondLength < count; ++secondLength) {
                    ONSUInteger values[] = {
                        kValues[firstLocation],
                        kValues[firstLength],
                        kValues[secondLocation],
                        kValues[secondLength]
                    };
                    status = compareFixture(&functions, values, fixture++);
                    if (status != 0) {
                        return status;
                    }
                }
            }
        }
    }
    uint64_t state = UINT64_C(0xc7505c69d23a9f81);
    for (size_t index = 0; index < 16384; ++index) {
        ONSUInteger values[4];
        for (size_t field = 0; field < 4; ++field) {
            values[field] = (ONSUInteger)nextFixtureValue(&state);
        }
        status = compareFixture(&functions, values, fixture++);
        if (status != 0) {
            return status;
        }
    }
    printf(
        "Foundation range differential passed: %zu independent fixtures, %zu exact comparisons across 2 "
        "APIs.\n",
        fixture,
        fixture * 2
    );
    return 0;
}
