//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Base/UtilitiesTests.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <stdio.h>

typedef OCFBuildVersion (*BuildVersionFunction)(uint64_t, uint32_t);

typedef bool (*PlatformLinkedFunction)(uint64_t, uint32_t);

typedef bool (*LinkedFunction)(uint64_t);

static uint64_t nextRandom(uint64_t *state) {
    uint64_t value = (*state += UINT64_C(0x9e3779b97f4a7c15));
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

static int
compare(BuildVersionFunction native, BuildVersionFunction replacement, uint64_t version, uint32_t platform) {
    OCFBuildVersion expected = native(version, platform);
    OCFBuildVersion actual = replacement(version, platform);
    if (expected.platform != actual.platform || expected.version != actual.version) {
        fprintf(
            stderr,
            "_CFBuildVersionForCFSystemVersion mismatch: version 0x%llx, platform %u.\n",
            (unsigned long long)version,
            platform
        );
        return 1;
    }
    return 0;
}

static int
resolvePair(const char *nativeName, const char *replacementName, void **native, void **replacement) {
    int status = resolveReferenceFunction("CoreFoundation", OF_REFERENCE_CF_UUID, nativeName, native);
    if (status == 0) {
        status = resolveReplacementFunction("OpenCoreFoundation", replacementName, replacement);
    }
    if (status == 0 && *native == *replacement) {
        fputs("Setup failure: utilities reference resolved to the replacement.\n", stderr);
        status = 77;
    }
    return status;
}

// Native _CFPlatformExecutableLinkedOnOrAfter 0x1c0256c34 and _CFExecutableLinkedOnOrAfter 0x1c0256c8c
// (external), iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x180703f48 and 0x18060bfb0, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence:
// verified for the bodies, medium for the identity of their out-of-image branch targets.
// Both sides ask the same process's dyld, so the answers are those of this test tool's linked SDK. The
// result bytes are compared over the version set of the build-version suite with platforms 0..15 and
// random platforms, then random versions; accepted (true) answers are counted to keep both outcomes
// visible.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
static int runLinkedOnOrAfterTests(const uint64_t *values, size_t count, uint64_t *state, size_t *cases) {
    void *nativeFunction = NULL;
    void *replacementFunction = NULL;
    int status = resolvePair(
        OF_NAME_STRING(_CFPlatformExecutableLinkedOnOrAfter),
        OF_NAME_STRING(_OCFPlatformExecutableLinkedOnOrAfter),
        &nativeFunction,
        &replacementFunction
    );
    if (status != 0) {
        return status;
    }
    PlatformLinkedFunction nativePlatform = (PlatformLinkedFunction)nativeFunction;
    PlatformLinkedFunction replacementPlatform = (PlatformLinkedFunction)replacementFunction;
    status = resolvePair(
        OF_NAME_STRING(_CFExecutableLinkedOnOrAfter),
        OF_NAME_STRING(_OCFExecutableLinkedOnOrAfter),
        &nativeFunction,
        &replacementFunction
    );
    if (status != 0) {
        return status;
    }
    LinkedFunction nativeLinked = (LinkedFunction)nativeFunction;
    LinkedFunction replacementLinked = (LinkedFunction)replacementFunction;
    size_t accepted = 0;
    size_t total = 0;
    for (size_t index = 0; index < count + 200000; ++index) {
        uint64_t version = index < count ? values[index] : nextRandom(state);
        if (index >= count && (version & 1)) {
            version &= 0xffffffff;
        }
        bool expected = nativeLinked(version);
        bool actual = replacementLinked(version);
        if (expected != actual) {
            fprintf(
                stderr,
                "_CFExecutableLinkedOnOrAfter mismatch: version 0x%llx.\n",
                (unsigned long long)version
            );
            return 1;
        }
        accepted += expected;
        ++total;
        for (uint32_t platform = 0; platform < 18; ++platform) {
            uint32_t value = platform < 16 ? platform : (uint32_t)nextRandom(state);
            expected = nativePlatform(version, value);
            actual = replacementPlatform(version, value);
            if (expected != actual) {
                fprintf(
                    stderr,
                    "_CFPlatformExecutableLinkedOnOrAfter mismatch: version 0x%llx, platform %u.\n",
                    (unsigned long long)version,
                    value
                );
                return 1;
            }
            accepted += expected;
            ++total;
        }
    }
    *cases += total;
    printf(
        "Linked-on-or-after differential cases passed: %zu, %zu true (two functions; finite coverage).\n",
        total,
        accepted
    );
    return 0;
}

// Native _CFBuildVersionForCFSystemVersion 0x1c0256b10 (external), iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device 24A446: 0x18060c1c8,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Both 32-bit result words are compared. Versions 0..4095, the macOS-version boundaries around
// 10.15.1, 10.16 and 11.0 and the 32-bit edges, each with bits above 32 clear and set, run over platforms
// 0..63 and random platforms; random 64-bit versions follow.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationUtilitiesDifferentialTests(void) {
    void *nativeFunction = NULL;
    void *replacementFunction = NULL;
    int status = resolveReferenceFunction(
        "CoreFoundation",
        OF_REFERENCE_CF_UUID,
        OF_NAME_STRING(_CFBuildVersionForCFSystemVersion),
        &nativeFunction
    );
    if (status == 0) {
        status = resolveReplacementFunction(
            "OpenCoreFoundation", OF_NAME_STRING(_OCFBuildVersionForCFSystemVersion), &replacementFunction
        );
    }
    if (status == 0 && nativeFunction == replacementFunction) {
        fputs("Setup failure: build-version reference resolved to the replacement.\n", stderr);
        status = 77;
    }
    if (status != 0) {
        return status;
    }
    BuildVersionFunction native = (BuildVersionFunction)nativeFunction;
    BuildVersionFunction replacement = (BuildVersionFunction)replacementFunction;
    size_t cases = 0;
    uint64_t state = UINT64_C(0xb01d5e7e75a0c0de);
    uint64_t values[4096 + 64];
    size_t count = 0;
    for (uint64_t version = 0; version < 4096; ++version) {
        values[count++] = version;
    }
    const uint64_t edges[] = {
        0xa0f00,
        0xa0f01,
        0xa0f02,
        0xa1000,
        0xa0fff,
        0xb0000,
        0xaffff,
        0xb0001,
        0xc0000,
        0xd0000,
        0xffff0000,
        0xfffffffe,
        0xffffffff,
        0x7fffffff,
        0x80000000
    };
    for (size_t index = 0; index < sizeof(edges) / sizeof(edges[0]); ++index) {
        values[count++] = edges[index];
    }
    for (size_t index = 0; index < count; ++index) {
        for (uint32_t high = 0; high < 3; ++high) {
            uint64_t version = values[index];
            if (high == 1) {
                version |= UINT64_C(1) << 32;
            } else if (high == 2) {
                version |= nextRandom(&state) << 32;
            }
            for (uint32_t platform = 0; platform < 64; ++platform) {
                if (compare(native, replacement, version, platform) != 0) {
                    return 1;
                }
                ++cases;
            }
            for (uint32_t round = 0; round < 8; ++round) {
                if (compare(native, replacement, version, (uint32_t)nextRandom(&state)) != 0) {
                    return 1;
                }
                ++cases;
            }
        }
    }
    for (uint32_t round = 0; round < 1000000; ++round) {
        uint64_t bits = nextRandom(&state);
        uint32_t platform = (bits & 1) ? (uint32_t)(bits >> 32) : (uint32_t)(bits >> 32) % 12;
        if (compare(native, replacement, nextRandom(&state), platform) != 0) {
            return 1;
        }
        ++cases;
    }
    printf("Build-version differential cases passed: %zu (one function; finite coverage).\n", cases);
    return runLinkedOnOrAfterTests(values, count, &state, &cases);
}
