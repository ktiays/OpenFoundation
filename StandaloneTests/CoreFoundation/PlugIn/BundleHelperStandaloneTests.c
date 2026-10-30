//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/BundleHelperStandaloneTests.h"
#include "CoreFoundation/PlugIn/OCFBundle.h"

#include <stdio.h>

// Native _CFBundleNormalizedPath 0x1c016a39c, iOS Simulator 24A434 UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Device comparison: 0x18063e8d4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationBundleHelperStandaloneTests(void) {
    size_t cases = 0;
    const char *preboot = "/private/preboot/Cryptexes/OS/System/Library";
    const char *cryptex = "/System/Cryptexes/OS/usr/lib";
    const char *plain = "/System/Library/Frameworks";
    const char *shortened = "/System/Cryptexes/OS";
    if (_CFBundleNormalizedPath(NULL) != NULL || _CFBundleNormalizedPath(preboot) != preboot + 29 ||
        _CFBundleNormalizedPath(cryptex) != cryptex + 20 || _CFBundleNormalizedPath(plain) != plain ||
        _CFBundleNormalizedPath(shortened) != shortened) {
        fputs("Bundle path standalone mismatch.\n", stderr);
        return 1;
    }
    cases += 5;
    printf("Bundle path standalone cases passed: %zu.\n", cases);
    return 0;
}
