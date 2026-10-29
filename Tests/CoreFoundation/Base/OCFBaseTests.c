//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Base/OCFBaseTests.h"

#include <OpenCoreFoundation/Base/OCFBase.h>
#include "CoreFoundation/Base/OCFHashBytes.h"
#include "Support/NativeReference.h"
#include "ReferencePins.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Native __CFRangeMake: 0x1c0150dec; CFHashBytes: 0x1c02561ec.
// iOS Simulator 27.0 (24A434), arm64, CoreFoundation UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device comparison: __CFRangeMake 0x18072d198, CFHashBytes 0x180665d1c,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationBase.md.
int runCoreFoundationBaseDifferentialTests(void) {
    void *nativeRangeAddress = NULL;
    void *replacementRangeAddress = NULL;
    void *nativeHashAddress = NULL;
    void *replacementHashAddress = NULL;
    int status = resolveReferenceFunction(
        "CoreFoundation", OF_REFERENCE_CF_UUID, OF_NAME_STRING(__CFRangeMake), &nativeRangeAddress
    );
    if (status != 0) {
        return status;
    }
    status = resolveReplacementFunction(
        "OpenCoreFoundation", OF_NAME_STRING(__OCFRangeMake), &replacementRangeAddress
    );
    if (status != 0) {
        return status;
    }
    status = resolveReferenceFunction(
        "CoreFoundation", OF_REFERENCE_CF_UUID, OF_NAME_STRING(CFHashBytes), &nativeHashAddress
    );
    if (status != 0) {
        return status;
    }
    status = resolveReplacementFunction(
        "OpenCoreFoundation", OF_NAME_STRING(OCFHashBytes), &replacementHashAddress
    );
    if (status != 0) {
        return status;
    }
    if (nativeRangeAddress == replacementRangeAddress || nativeHashAddress == replacementHashAddress) {
        fprintf(stderr, "Setup failure: a base comparison resolved the same implementation twice.\n");
        return 77;
    }
    typedef OCFRange (*RangeFunction)(OCFIndex, OCFIndex);
    typedef OCFHashCode (*HashFunction)(const uint8_t *, OCFIndex);
    RangeFunction nativeRange = (RangeFunction)nativeRangeAddress;
    RangeFunction replacementRange = (RangeFunction)replacementRangeAddress;
    HashFunction nativeHash = (HashFunction)nativeHashAddress;
    HashFunction replacementHash = (HashFunction)replacementHashAddress;

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
            OCFRange expected = nativeRange(values[left], values[right]);
            OCFRange actual = replacementRange(values[left], values[right]);
            if (memcmp(&expected, &actual, sizeof(actual)) != 0) {
                fprintf(stderr, "Range comparison differed for %ld, %ld.\n", values[left], values[right]);
                return 1;
            }
            ++rangeCases;
        }
    }

    uint8_t nativeBytes[4112];
    uint8_t replacementBytes[4112];
    uint8_t original[4112];
    size_t hashCases = 0;
    const OCFIndex largeLengths[] = {512, 1023, 1024, 1025, 2047, 2048, 2049, 4096};
    for (unsigned pattern = 0; pattern < 4; ++pattern) {
        for (size_t index = 0; index < sizeof(original); ++index) {
            uint8_t value;
            switch (pattern) {
                case 0:
                    value = (uint8_t)index;
                    break;
                case 1:
                    value = UINT8_MAX;
                    break;
                case 2:
                    value = 0;
                    break;
                default:
                    value = (uint8_t)((index * 37) ^ (index >> 3));
                    break;
            }
            nativeBytes[index] = value;
            replacementBytes[index] = value;
            original[index] = value;
        }
        for (size_t offset = 0; offset < 16; ++offset) {
            for (OCFIndex length = 0; length <= 257; ++length) {
                OCFHashCode expected = nativeHash(nativeBytes + offset, length);
                OCFHashCode actual = replacementHash(replacementBytes + offset, length);
                if (expected != actual) {
                    fprintf(
                        stderr,
                        "Byte hash differed: pattern %u, offset %zu, length %ld.\n",
                        pattern,
                        offset,
                        length
                    );
                    return 1;
                }
                ++hashCases;
            }
            for (size_t index = 0; index < sizeof(largeLengths) / sizeof(largeLengths[0]); ++index) {
                OCFIndex length = largeLengths[index];
                OCFHashCode expected = nativeHash(nativeBytes + offset, length);
                OCFHashCode actual = replacementHash(replacementBytes + offset, length);
                if (expected != actual) {
                    fprintf(
                        stderr,
                        "Large byte hash differed: pattern %u, offset %zu, length %ld.\n",
                        pattern,
                        offset,
                        length
                    );
                    return 1;
                }
                ++hashCases;
            }
        }
        if (memcmp(nativeBytes, original, sizeof(original)) != 0 ||
            memcmp(replacementBytes, original, sizeof(original)) != 0) {
            fprintf(stderr, "A byte hash implementation changed an input fixture.\n");
            return 1;
        }
    }
    const OCFIndex noReadLengths[] = {0, -1, LONG_MIN, LONG_MAX, 0x80000000L, 0x100000000L, -0x100000000L};
    for (size_t index = 0; index < sizeof(noReadLengths) / sizeof(noReadLengths[0]); ++index) {
        if (nativeHash(NULL, noReadLengths[index]) != replacementHash(NULL, noReadLengths[index])) {
            fprintf(stderr, "A native no-read byte hash length differed.\n");
            return 1;
        }
        ++hashCases;
    }
    printf(
        "CoreFoundation base differential passed: %zu range cases, %zu hash cases.\n", rangeCases, hashCases
    );
    return 0;
}
