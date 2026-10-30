//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/BundleHelperTests.h"
#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef const char *(*NormalizedPathFunction)(const char *);

static int comparePath(NormalizedPathFunction native, const char *path, size_t *cases) {
    const char *expected = native(path);
    const char *actual = _CFBundleNormalizedPath(path);
    if (expected != actual) {
        fprintf(stderr, "_CFBundleNormalizedPath mismatch for \"%s\".\n", path ? path : "(null)");
        return 1;
    }
    ++*cases;
    return 0;
}

// Native _CFBundleNormalizedPath 0x1c016a39c (non-external; resolved by IDA offset), iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device 24A446:
// 0x18063e8d4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// The returned pointer is compared exactly.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationBundleHelperDifferentialTests(void) {
    void *function = NULL;
    int status = resolveReferenceAddress(
        "CoreFoundation", OF_REFERENCE_CF_UUID, UINT64_C(0x1c016a39c), UINT64_C(0x1c014a000), &function
    );
    if (status == 0) {
        status = checkInternalImplementation((const void *)_CFBundleNormalizedPath);
    }
    if (status == 0 && function == (void *)_CFBundleNormalizedPath) {
        fputs("Setup failure: bundle helper reference resolved to the replacement.\n", stderr);
        status = 77;
    }
    if (status != 0) {
        return status;
    }
    NormalizedPathFunction native = (NormalizedPathFunction)function;
    size_t cases = 0;
    status = comparePath(native, NULL, &cases);
    // Every prefix of both literals, each with every replacement of its last byte and every suffix.
    static const char *const kPrefixes[2] = {"/private/preboot/Cryptexes/OS/", "/System/Cryptexes/OS/"};
    static const char *const kSuffixes[4] = {"", "/", "System/Library/Frameworks/X.framework/X", "OS/"};
    char path[128];
    for (size_t prefix = 0; prefix < 2 && status == 0; ++prefix) {
        size_t length = strlen(kPrefixes[prefix]);
        for (size_t cut = 0; cut <= length && status == 0; ++cut) {
            for (uint32_t last = 0; last < 256 && status == 0; ++last) {
                for (size_t suffix = 0; suffix < 4 && status == 0; ++suffix) {
                    memcpy(path, kPrefixes[prefix], cut);
                    path[cut] = '\0';
                    if (cut > 0) {
                        path[cut - 1] = (char)last;
                        if (last == 0 && suffix != 0) {
                            continue;
                        }
                    }
                    strcat(path, kSuffixes[suffix]);
                    status = comparePath(native, path, &cases);
                }
            }
        }
    }
    static const char *const kPaths[] = {
        "",
        "/",
        "/private",
        "/private/preboot/Cryptexes/OS",
        "/System/Cryptexes/OS",
        "/system/cryptexes/os/",
        "/private/preboot/Cryptexes/OS//",
        "/System/Cryptexes/OS/System/Cryptexes/OS/",
    };
    for (size_t index = 0; index < sizeof(kPaths) / sizeof(kPaths[0]) && status == 0; ++index) {
        status = comparePath(native, kPaths[index], &cases);
    }
    if (status != 0) {
        return status;
    }
    printf("Bundle path differential cases passed: %zu (one function; finite coverage).\n", cases);
    return 0;
}
