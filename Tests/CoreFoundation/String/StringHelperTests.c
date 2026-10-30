//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/StringHelperTests.h"
#include "CoreFoundation/String/OCFString.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef bool (*FindFunction)(const FormatSpec *, int32_t, OCFIndex, int16_t *, int16_t *);

typedef bool (*ValidateFunction)(const FormatSpec *, int32_t, const FormatSpec *, int32_t, OCFIndex);

typedef bool (*RangeEqualFunction)(const void *, const void *);

typedef struct {
    FindFunction find;
    ValidateFunction validate;
    RangeEqualFunction rangeEqual;
} StringFunctions;

// Accepted (true) results, reported so that coverage of the success paths stays visible.
static size_t acceptedFinds = 0;
static size_t acceptedValidations = 0;

enum {
    kMaximumSpecs = 300,
};

static uint64_t nextRandom(uint64_t *state) {
    uint64_t value = (*state += UINT64_C(0x9e3779b97f4a7c15));
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

static int resolveHidden(uintptr_t address, void *replacement, void **native) {
    int status = resolveReferenceAddress(
        "CoreFoundation", OF_REFERENCE_CF_UUID, address, UINT64_C(0x1c014a000), native
    );
    if (status == 0) {
        status = checkInternalImplementation(replacement);
    }
    if (status == 0 && *native == replacement) {
        fputs("Setup failure: string helper reference resolved to the replacement.\n", stderr);
        status = 77;
    }
    return status;
}

static int resolveFunctions(StringFunctions *native, StringFunctions *replacement) {
    void *function = NULL;
    int status =
        resolveHidden(UINT64_C(0x1c022452c), (void *)__CFStringFindTypeAndSizeForArgumentIndex, &function);
    if (status != 0) {
        return status;
    }
    native->find = (FindFunction)function;
    status = resolveHidden(UINT64_C(0x1c02242fc), (void *)__CFStringValidateFormatSpecBuffers, &function);
    if (status != 0) {
        return status;
    }
    native->validate = (ValidateFunction)function;
    status = resolveHidden(UINT64_C(0x1c0218d64), (void *)__rangeEqual, &function);
    if (status != 0) {
        return status;
    }
    native->rangeEqual = (RangeEqualFunction)function;
    replacement->find = __CFStringFindTypeAndSizeForArgumentIndex;
    replacement->validate = __CFStringValidateFormatSpecBuffers;
    replacement->rangeEqual = __rangeEqual;
    return 0;
}

static int compareFind(
    const StringFunctions *native,
    const StringFunctions *replacement,
    const FormatSpec *specs,
    int32_t count,
    OCFIndex argumentIndex,
    size_t *cases
) {
    int16_t expected[4] = {0x5a5a, 0x5a5a, 0x5a5a, 0x5a5a};
    int16_t actual[4] = {0x5a5a, 0x5a5a, 0x5a5a, 0x5a5a};
    bool expectedResult = native->find(specs, count, argumentIndex, &expected[1], &expected[2]);
    bool actualResult = replacement->find(specs, count, argumentIndex, &actual[1], &actual[2]);
    if (expectedResult != actualResult || memcmp(expected, actual, sizeof(expected)) != 0) {
        fprintf(
            stderr,
            "__CFStringFindTypeAndSizeForArgumentIndex mismatch: count %d, index %lld, first record "
            "%d/%d/%d.\n",
            count,
            (long long)argumentIndex,
            count > 0 ? specs[0].mainArgNum : 0,
            count > 0 ? specs[0].precArgNum : 0,
            count > 0 ? specs[0].widthArgNum : 0
        );
        return 1;
    }
    acceptedFinds += expectedResult;
    ++*cases;
    return 0;
}

static int compareValidate(
    const StringFunctions *native,
    const StringFunctions *replacement,
    const FormatSpec *specs,
    int32_t count,
    const FormatSpec *expectedSpecs,
    int32_t expectedCount,
    OCFIndex nextArgumentIndex,
    size_t *cases
) {
    bool expectedResult = native->validate(specs, count, expectedSpecs, expectedCount, nextArgumentIndex);
    bool actualResult = replacement->validate(specs, count, expectedSpecs, expectedCount, nextArgumentIndex);
    if (expectedResult != actualResult) {
        fprintf(
            stderr,
            "__CFStringValidateFormatSpecBuffers mismatch: counts %d/%d, next %lld, native %d.\n",
            count,
            expectedCount,
            (long long)nextArgumentIndex,
            expectedResult
        );
        return 1;
    }
    acceptedValidations += expectedResult;
    ++*cases;
    return 0;
}

static const int8_t kArgumentNumbers[] = {-128, -3, -2, -2, -1, -1, 0, 0, 1, 1, 2, 3, 4, 5, 7, 127};

static const int16_t kTypes[] = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x121, 0x0};

static void randomSpec(FormatSpec *spec, uint64_t *state, int structured) {
    uint64_t bits = nextRandom(state);
    memset(spec, (int)(bits >> 56), sizeof(*spec));
    if (structured) {
        size_t numbers = sizeof(kArgumentNumbers) / sizeof(kArgumentNumbers[0]);
        spec->mainArgNum = kArgumentNumbers[bits % numbers];
        spec->precArgNum = (bits >> 8) & 1 ? kArgumentNumbers[(bits >> 9) % numbers] : -1;
        spec->widthArgNum = (bits >> 16) & 1 ? kArgumentNumbers[(bits >> 17) % numbers] : -1;
        spec->type = kTypes[(bits >> 24) % (sizeof(kTypes) / sizeof(kTypes[0]))];
        spec->size = (int16_t)((bits >> 32) % 9 + ((bits >> 40) & 0x100));
    } else {
        spec->mainArgNum = (int8_t)bits;
        spec->precArgNum = (int8_t)(bits >> 8);
        spec->widthArgNum = (int8_t)(bits >> 16);
        spec->type = (int16_t)(bits >> 24);
        spec->size = (int16_t)(bits >> 40);
    }
}

// Native __CFStringFindTypeAndSizeForArgumentIndex 0x1c022452c, __CFStringValidateFormatSpecBuffers
// 0x1c02242fc and __rangeEqual 0x1c0218d64 (non-external; resolved by IDA offset), iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device 24A446: 0x1806d64a8,
// 0x1806ba570, 0x180764320; UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Results and both 16-bit out parameters, between canaries, are compared exactly.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationStringHelperDifferentialTests(void) {
    StringFunctions native;
    StringFunctions replacement;
    int status = resolveFunctions(&native, &replacement);
    if (status != 0) {
        return status;
    }
    size_t cases = 0;
    uint64_t state = UINT64_C(0x5eed0f0b16a0101);
    // One record: every byte triple of the three argument numbers, each index -3..3 and 0x100.
    FormatSpec spec;
    memset(&spec, 0, sizeof(spec));
    spec.type = 0x1234;
    spec.size = 0x5678;
    static const OCFIndex kIndices[] = {-3, -2, -1, 0, 1, 2, 3, 0x100};
    for (uint32_t bytes = 0; bytes < (UINT32_C(1) << 24); ++bytes) {
        spec.mainArgNum = (int8_t)bytes;
        spec.precArgNum = (int8_t)(bytes >> 8);
        spec.widthArgNum = (int8_t)(bytes >> 16);
        for (size_t index = 0; index < sizeof(kIndices) / sizeof(kIndices[0]); ++index) {
            if ((bytes & 0xf) != 0 && index >= 4) {
                continue;
            }
            status = compareFind(&native, &replacement, &spec, 1, kIndices[index], &cases);
            if (status != 0) {
                return status;
            }
        }
    }
    // Record arrays beyond 255 sequential numbers, so the low-byte counter wraps.
    FormatSpec *specs = calloc(kMaximumSpecs, sizeof(FormatSpec));
    FormatSpec *expectedSpecs = calloc(kMaximumSpecs, sizeof(FormatSpec));
    if (!specs || !expectedSpecs) {
        free(specs);
        free(expectedSpecs);
        fputs("Setup failure: string helper buffers.\n", stderr);
        return 77;
    }
    for (size_t round = 0; round < 40000 && status == 0; ++round) {
        int structured = (round & 3) != 0;
        int32_t count = (int32_t)(nextRandom(&state) % (round < 2000 ? kMaximumSpecs : 9));
        for (int32_t index = 0; index < count; ++index) {
            randomSpec(&specs[index], &state, structured);
            if (structured && (nextRandom(&state) & 1)) {
                specs[index].mainArgNum = -1;
            }
        }
        for (int attempt = 0; attempt < 24 && status == 0; ++attempt) {
            OCFIndex argumentIndex = (OCFIndex)(nextRandom(&state) % 300) - 130;
            status = compareFind(&native, &replacement, specs, count, argumentIndex, &cases);
        }
        if (status == 0) {
            status = compareFind(&native, &replacement, specs, -1, 0, &cases);
        }
    }
    // Validation: expected arrays are random; the validated arrays are copies with mutations, so both
    // accepting and rejecting paths occur.
    for (size_t round = 0; round < 1500000 && status == 0; ++round) {
        int32_t expectedCount = (int32_t)(nextRandom(&state) % 7);
        int structured = (round % 5) != 0;
        for (int32_t index = 0; index < expectedCount; ++index) {
            randomSpec(&expectedSpecs[index], &state, structured);
        }
        int32_t count = (int32_t)(nextRandom(&state) % 7);
        for (int32_t index = 0; index < count; ++index) {
            if (index < expectedCount && (nextRandom(&state) & 3) != 0) {
                specs[index] = expectedSpecs[index];
                uint64_t mutation = nextRandom(&state);
                if ((mutation & 7) == 0) {
                    specs[index].type = kTypes[(mutation >> 8) % (sizeof(kTypes) / sizeof(kTypes[0]))];
                } else if ((mutation & 7) == 1) {
                    specs[index].size = (int16_t)((mutation >> 8) % 9);
                }
            } else {
                randomSpec(&specs[index], &state, structured);
            }
        }
        OCFIndex next = (round & 7) == 0 ? (OCFIndex)nextRandom(&state) : (OCFIndex)(nextRandom(&state) % 4);
        status =
            compareValidate(&native, &replacement, specs, count, expectedSpecs, expectedCount, next, &cases);
    }
    // Every pair of record type low bytes and sizes 0..8, with one expected record of main argument 0.
    for (uint32_t pair = 0; pair < 0x10000 && status == 0; ++pair) {
        memset(specs, 0, sizeof(FormatSpec));
        memset(expectedSpecs, 0, sizeof(FormatSpec));
        specs[0].mainArgNum = 0;
        specs[0].precArgNum = -1;
        specs[0].widthArgNum = -1;
        specs[0].type = (int16_t)(pair & 0xff);
        expectedSpecs[0].mainArgNum = 0;
        expectedSpecs[0].precArgNum = -1;
        expectedSpecs[0].widthArgNum = -1;
        expectedSpecs[0].type = (int16_t)(pair >> 8);
        for (int16_t sizes = 0; sizes < 81 && status == 0; ++sizes) {
            specs[0].size = (int16_t)(sizes % 9 + ((pair & 0x100) ? 0x300 : 0));
            expectedSpecs[0].size = (int16_t)(sizes / 9);
            status = compareValidate(&native, &replacement, specs, 1, expectedSpecs, 1, 0, &cases);
        }
    }
    free(specs);
    free(expectedSpecs);
    if (status != 0) {
        return status;
    }
    // Range equality: random, equal and one-word-equal pairs.
    for (size_t round = 0; round < 1000000; ++round) {
        OCFRange first = {(OCFIndex)nextRandom(&state), (OCFIndex)nextRandom(&state)};
        OCFRange second = first;
        uint64_t choice = nextRandom(&state) % 4;
        if (choice == 1) {
            second.location ^= (OCFIndex)(UINT64_C(1) << (nextRandom(&state) % 64));
        } else if (choice == 2) {
            second.length ^= (OCFIndex)(UINT64_C(1) << (nextRandom(&state) % 64));
        } else if (choice == 3) {
            second.location = (OCFIndex)nextRandom(&state);
            second.length = (OCFIndex)nextRandom(&state);
        }
        if (native.rangeEqual(&first, &second) != replacement.rangeEqual(&first, &second)) {
            fputs("__rangeEqual mismatch.\n", stderr);
            return 1;
        }
        ++cases;
    }
    printf(
        "String helper differential cases passed: %zu (three functions; %zu lookups and %zu validations "
        "accepted; finite coverage).\n",
        cases,
        acceptedFinds,
        acceptedValidations
    );
    return 0;
}
