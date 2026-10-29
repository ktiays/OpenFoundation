//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include <OpenCoreFoundation/Base/OCFBase.h>

// Native __CFRangeMake: 0x1c0150dec, iOS Simulator 27.0 (24A434), arm64.
// CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device comparison: __CFRangeMake, 0x18072d198, iOS 27.0.1 (24A446), arm64e,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; same register return; confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationBase.md.
OCFRange __OCFRangeMake(OCFIndex location, OCFIndex length) {
    OCFRange result;
    result.location = location;
    result.length = length;
    return result;
}
