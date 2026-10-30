//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Base/FileUtilitiesTests.h"
#include "CoreFoundation/Base/OCFFileUtilities.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <stdio.h>

typedef uint16_t (*SlashFunction)(void);

typedef OCFIndex (*PathFunction)(const uint16_t *, OCFIndex);

static int resolveHidden(uintptr_t address, void *replacement, void **native) {
    int status = resolveReferenceAddress(
        "CoreFoundation", OF_REFERENCE_CF_UUID, address, UINT64_C(0x1c014a000), native
    );
    if (status == 0) {
        status = checkInternalImplementation(replacement);
    }
    if (status == 0 && *native == replacement) {
        fputs("Setup failure: path helper reference resolved to the replacement.\n", stderr);
        status = 77;
    }
    return status;
}

static int comparePath(
    PathFunction const native[2],
    PathFunction const replacement[2],
    const uint16_t *units,
    OCFIndex length,
    size_t *cases
) {
    for (size_t function = 0; function < 2; ++function) {
        OCFIndex expected = native[function](units, length);
        OCFIndex actual = replacement[function](units, length);
        if (expected != actual) {
            fprintf(
                stderr,
                "%s mismatch: length %ld, units %04x %04x, native %ld, replacement %ld.\n",
                function ? "_CFLengthAfterDeletingLastPathComponent" : "_CFStartOfLastPathComponent",
                (long)length,
                length > 0 ? units[0] : 0,
                length > 1 ? units[1] : 0,
                (long)expected,
                (long)actual
            );
            return 1;
        }
        ++*cases;
    }
    return 0;
}

// Native _CFGetSlash 0x1c019cefc, _CFStartOfLastPathComponent 0x1c019d374 and
// _CFLengthAfterDeletingLastPathComponent 0x1c019d458 (non-external; resolved by IDA offset), iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device 24A446:
// 0x18066f0b8, 0x180698360, 0x18069ab0c; UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified
// for the inline bodies. Returned indices are compared exactly.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationFileUtilitiesDifferentialTests(void) {
    void *slash = NULL;
    void *nativeFunctions[2] = {NULL, NULL};
    int status = resolveHidden(UINT64_C(0x1c019cefc), (void *)_CFGetSlash, &slash);
    if (status == 0) {
        status =
            resolveHidden(UINT64_C(0x1c019d374), (void *)_CFStartOfLastPathComponent, &nativeFunctions[0]);
    }
    if (status == 0) {
        status = resolveHidden(
            UINT64_C(0x1c019d458), (void *)_CFLengthAfterDeletingLastPathComponent, &nativeFunctions[1]
        );
    }
    if (status != 0) {
        return status;
    }
    PathFunction const native[2] = {(PathFunction)nativeFunctions[0], (PathFunction)nativeFunctions[1]};
    PathFunction const replacement[2] = {
        _CFStartOfLastPathComponent, _CFLengthAfterDeletingLastPathComponent
    };
    size_t cases = 0;
    if (((SlashFunction)slash)() != _CFGetSlash()) {
        fputs("_CFGetSlash mismatch.\n", stderr);
        return 1;
    }
    ++cases;
    // Every buffer of length 0..9 over '/', ':', 'C', 'z' and '[': slashes and drive prefixes at every
    // position. Lengths -1 and below give 0 without a read.
    static const uint16_t kUnits[5] = {'/', ':', 'C', 'z', '['};
    uint16_t units[10];
    for (OCFIndex length = 0; length <= 9; ++length) {
        size_t total = 1;
        for (OCFIndex index = 0; index < length; ++index) {
            total *= 5;
        }
        for (size_t value = 0; value < total; ++value) {
            size_t digits = value;
            for (OCFIndex index = 0; index < length; ++index) {
                units[index] = kUnits[digits % 5];
                digits /= 5;
            }
            status = comparePath(native, replacement, units, length, &cases);
            if (status != 0) {
                return status;
            }
        }
    }
    status = comparePath(native, replacement, units, -1, &cases);
    if (status == 0) {
        status = comparePath(native, replacement, units, INT64_MIN, &cases);
    }
    if (status != 0) {
        return status;
    }
    // Every first unit with ':' second, lengths 2..6 and the slash before, at or after index 2.
    for (uint32_t first = 0; first < 0x10000; ++first) {
        for (OCFIndex length = 2; length <= 6; ++length) {
            for (OCFIndex slashIndex = -1; slashIndex < length; ++slashIndex) {
                for (OCFIndex index = 0; index < length; ++index) {
                    units[index] = index == slashIndex ? '/' : 'x';
                }
                units[0] = slashIndex == 0 ? '/' : (uint16_t)first;
                units[1] = slashIndex == 1 ? '/' : ':';
                status = comparePath(native, replacement, units, length, &cases);
                if (status != 0) {
                    return status;
                }
            }
        }
    }
    printf("File-utility path differential cases passed: %zu (three functions; finite coverage).\n", cases);
    return 0;
}
