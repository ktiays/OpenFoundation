//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/URL/URLHelperStandaloneTests.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <stdio.h>

static int checkRange(OCFRange actual, OCFIndex location, OCFIndex length, int line) {
    if (actual.location != location || actual.length != length) {
        fprintf(stderr, "URL net-location standalone mismatch at source line %d.\n", line);
        return 1;
    }
    return 0;
}

// Native _netLocationRange 0x1c0244938, iOS Simulator 24A434 UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Device comparison: 0x180712ef4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationURLHelperStandaloneTests(void) {
    size_t cases = 0;
    // "http://user:pw@host:80/": scheme {0, 4}, user {7, 4}, password {12, 2}, host {15, 4}, port {20, 2}.
    const OCFRange all[5] = {{0, 4}, {7, 4}, {12, 2}, {15, 4}, {20, 2}};
    if (checkRange(_netLocationRange(0x1f, all), 7, 15, __LINE__) != 0) {
        return 1;
    }
    ++cases;
    // "http://host/": scheme and host only.
    const OCFRange hostOnly[2] = {{0, 4}, {7, 4}};
    if (checkRange(_netLocationRange(0x09, hostOnly), 7, 4, __LINE__) != 0) {
        return 1;
    }
    ++cases;
    if (checkRange(_netLocationRange(0x01, hostOnly), -1, 0, __LINE__) != 0 ||
        checkRange(_netLocationRange(0x20, hostOnly), -1, 0, __LINE__) != 0) {
        return 1;
    }
    cases += 2;
    // An absent (-1) user range is skipped; the host then starts the result.
    const OCFRange absentUser[3] = {{0, 4}, {-1, 0}, {9, 3}};
    if (checkRange(_netLocationRange(0x0b, absentUser), 9, 3, __LINE__) != 0) {
        return 1;
    }
    ++cases;
    printf("URL net-location standalone cases passed: %zu.\n", cases);
    return 0;
}
