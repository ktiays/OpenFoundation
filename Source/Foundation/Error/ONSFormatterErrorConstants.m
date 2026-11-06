//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Error/ONSErrorConstants.h"

#include "CoreFoundation/String/OCFString.h"

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Foundation/Number/Reading.md, round 2.

// Native NSMinimumValidValueErrorKey, 0x2227d1238. External. CFString 0x222824010.
static const OCFConstantString minimumValidValueKey = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSMinimumValidValue", 19
};
id const ONSMinimumValidValueErrorKey = (id)&minimumValidValueKey;

// Native NSMaximumValidValueErrorKey, 0x2227d1240. External. CFString 0x222824030.
static const OCFConstantString maximumValidValueKey = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSMaximumValidValue", 19
};
id const ONSMaximumValidValueErrorKey = (id)&maximumValidValueKey;
