//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Base/OCFBaseStandaloneTests.h"

#include <OpenCoreFoundation/Base/OCFBase.h>
#include "CoreFoundation/Base/OCFHashBytes.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

_Static_assert(sizeof(OCFIndex) == 8, "OCFIndex must preserve the arm64 scalar ABI.");
_Static_assert(sizeof(OCFHashCode) == 8, "OCFHashCode must preserve the arm64 result ABI.");
_Static_assert(sizeof(OCFOptionFlags) == 8, "OCFOptionFlags must preserve the arm64 scalar ABI.");
_Static_assert(sizeof(OCFRange) == 16, "OCFRange must occupy two arm64 integer registers.");
_Static_assert(offsetof(OCFRange, location) == 0, "OCFRange.location has the native offset.");
_Static_assert(offsetof(OCFRange, length) == 8, "OCFRange.length has the native offset.");

static const struct {
    OCFIndex length;
    OCFHashCode expected;
} kSequenceFixtures[] = {
    {0, 0},
    {1, 0},
    {2, 1},
    {3, 18},
    {4, 291},
    {5, 4660},
    {7, 1193046},
    {8, 19088743},
    {9, 36984424},
    {15, 179235518},
    {16, 183413583},
    {31, 5965230},
    {32, 95443711},
    {255, 214184366},
    {256, 205724447},
    {257, 70365488}
};

// Native __CFRangeMake: 0x1c0150dec; CFHashBytes: 0x1c02561ec.
// iOS Simulator 27.0 (24A434), arm64, CoreFoundation UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device comparison: __CFRangeMake 0x18072d198, CFHashBytes 0x180665d1c,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationBase.md.
// Fixed scalar outputs were confirmed by the identity-guarded probe retained under
// Local/Evidence/cf-base/NativeHashValues.txt; implementation facts come from IDA.
int runCoreFoundationBaseStandaloneTests(void) {
    const OCFIndex values[] = {
        LONG_MIN,
        LONG_MIN + 1,
        -0x100000000L,
        -1,
        0,
        1,
        0x7fffffffL,
        0x80000000L,
        0x100000000L,
        LONG_MAX - 1,
        LONG_MAX
    };
    size_t rangeCases = 0;
    for (size_t left = 0; left < sizeof(values) / sizeof(values[0]); ++left) {
        for (size_t right = 0; right < sizeof(values) / sizeof(values[0]); ++right) {
            OCFRange actual = __OCFRangeMake(values[left], values[right]);
            if (actual.location != values[left] || actual.length != values[right]) {
                fprintf(stderr, "__OCFRangeMake failed to preserve its scalar arguments.\n");
                return 1;
            }
            ++rangeCases;
        }
    }

    uint8_t bytes[512];
    uint8_t original[512];
    for (size_t index = 0; index < sizeof(bytes); ++index) {
        bytes[index] = (uint8_t)index;
        original[index] = (uint8_t)index;
    }
    size_t hashCases = 0;
    for (size_t index = 0; index < sizeof(kSequenceFixtures) / sizeof(kSequenceFixtures[0]); ++index) {
        OCFHashCode actual = OCFHashBytes(bytes, kSequenceFixtures[index].length);
        if (actual != kSequenceFixtures[index].expected) {
            fprintf(
                stderr,
                "OCFHashBytes sequence fixture %ld differed: %lu.\n",
                kSequenceFixtures[index].length,
                actual
            );
            return 1;
        }
        ++hashCases;
    }
    if (memcmp(bytes, original, sizeof(bytes)) != 0) {
        fprintf(stderr, "OCFHashBytes changed its input bytes.\n");
        return 1;
    }
    if (OCFHashBytes((const uint8_t *)"hello", 5) != 7258927 ||
        OCFHashBytes((const uint8_t *)"Foundation", 10) != 78412446) {
        fprintf(stderr, "OCFHashBytes text fixture differed.\n");
        return 1;
    }
    hashCases += 2;
    memset(bytes, 255, 32);
    if (OCFHashBytes(bytes, 32) != 4335) {
        fprintf(stderr, "OCFHashBytes high-byte fixture differed.\n");
        return 1;
    }
    ++hashCases;

    const OCFIndex noReadLengths[] = {0, -1, LONG_MIN, LONG_MAX, 0x80000000L, 0x100000000L, -0x100000000L};
    for (size_t index = 0; index < sizeof(noReadLengths) / sizeof(noReadLengths[0]); ++index) {
        if (OCFHashBytes(NULL, noReadLengths[index]) != 0) {
            fprintf(stderr, "OCFHashBytes changed a native no-read length result.\n");
            return 1;
        }
        ++hashCases;
    }

    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize < 512) {
        fprintf(stderr, "Setup failure: cannot establish a page size for byte-boundary fixtures.\n");
        return 77;
    }
    size_t mappingSize = (size_t)pageSize * 3;
    uint8_t *mapping = mmap(NULL, mappingSize, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mapping == MAP_FAILED) {
        fprintf(stderr, "Setup failure: cannot allocate byte-boundary fixtures.\n");
        return 77;
    }
    uint8_t *readable = mapping + pageSize;
    if (mprotect(readable, (size_t)pageSize, PROT_READ | PROT_WRITE) != 0) {
        munmap(mapping, mappingSize);
        fprintf(stderr, "Setup failure: cannot protect byte-boundary fixtures.\n");
        return 77;
    }
    for (unsigned placement = 0; placement < 2; ++placement) {
        for (size_t index = 0; index < sizeof(kSequenceFixtures) / sizeof(kSequenceFixtures[0]); ++index) {
            OCFIndex length = kSequenceFixtures[index].length;
            uint8_t *input = placement ? readable + pageSize - length : readable;
            for (OCFIndex byteIndex = 0; byteIndex < length; ++byteIndex) {
                input[byteIndex] = (uint8_t)byteIndex;
            }
            OCFHashCode actual = OCFHashBytes(input, length);
            if (actual != kSequenceFixtures[index].expected) {
                munmap(mapping, mappingSize);
                fprintf(stderr, "OCFHashBytes guarded fixture %ld differed.\n", length);
                return 1;
            }
            ++hashCases;
        }
    }
    munmap(mapping, mappingSize);
    printf(
        "CoreFoundation base standalone passed: %zu range cases, %zu hash cases.\n", rangeCases, hashCases
    );
    return 0;
}
