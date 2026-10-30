//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Base/UtilitiesStandaloneTests.h"
#include "CoreFoundation/Base/OCFUtilities.h"

#include <stdio.h>

// Native _CFBuildVersionForCFSystemVersion 0x1c0256b10, iOS Simulator 24A434
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Device comparison: 0x18060c1c8,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified. _CFExecutableLinkedOnOrAfter 0x1c0256c8c
// (Device 0x18060bfb0) is checked at its two ends; its answer comes from this process's dyld.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationUtilitiesStandaloneTests(void) {
    struct {
        uint64_t systemVersion;
        uint32_t platform;
        uint32_t expectedPlatform;
        uint32_t expectedVersion;
        int line;
    } cases[] = {
        {0, 6, 2, 0, __LINE__},
        {4, 1, 1, 0, __LINE__},
        {6, 1, 1, 0x0a0600, __LINE__},
        {15, 6, 2, 0x0d0000, __LINE__},
        {16, 6, 2, 0x0e0000, __LINE__},
        {17, 6, 2, 0xffff0000, __LINE__},
        {7, 2, 2, 0, __LINE__},
        {12, 2, 2, 0xffff0000, __LINE__},
        {1003, 1, 1, 0, __LINE__},
        {1005, 1, 1, 0x0a0600, __LINE__},
        {1009, 1, 1, 0xffff0000, __LINE__},
        {1015, 1, 1, 0x0a0800, __LINE__},
        {1005, 6, 2, 0, __LINE__},
        {1016, 1, 1, 1016, __LINE__},
        {0x0a0f00, 6, 2, 0x0d0000, __LINE__},
        {0x0a0f01, 6, 2, 0x0d0200, __LINE__},
        {0x0b0000, 6, 2, 0x0e0000, __LINE__},
        {0x0c0000, 6, 2, 0xffff0000, __LINE__},
        {UINT64_C(0x123456789), 2, 2, 0x23456789, __LINE__},
    };

    size_t count = sizeof(cases) / sizeof(cases[0]);
    for (size_t index = 0; index < count; ++index) {
        OCFBuildVersion result =
            _OCFBuildVersionForCFSystemVersion(cases[index].systemVersion, cases[index].platform);
        if (result.platform != cases[index].expectedPlatform ||
            result.version != cases[index].expectedVersion) {
            fprintf(stderr, "Build-version standalone mismatch at source line %d.\n", cases[index].line);
            return 1;
        }
    }
    printf("Build-version standalone cases passed: %zu.\n", count);
    // Version 0 maps to build version 0 on the active platform, which every program's SDK reaches; a
    // version passed through as 0xfffffffe (version 0xfffffffe, the active platform) is above any SDK.
    if (!_OCFExecutableLinkedOnOrAfter(0) || _OCFExecutableLinkedOnOrAfter(UINT64_C(0xfffffffe))) {
        fputs("Linked-on-or-after standalone mismatch.\n", stderr);
        return 1;
    }
    puts("Linked-on-or-after standalone cases passed: 2.");
    return 0;
}
