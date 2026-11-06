//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringInternal.h"
#import "Foundation/String/ONSPresentationIntent.h"
#include "CoreFoundation/String/OCFString.h"

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

// Native NSInlinePresentationIntentAttributeName, 0x2227d7b38. External.
static const OCFConstantString kInlinePresentationIntentName = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSInlinePresentationIntent", 26
};
id const ONSInlinePresentationIntentAttributeName = (id)&kInlinePresentationIntentName;

// Native NSPresentationIntentAttributeName, 0x2227daf10. External. The key spelling is fixed in both modes.
static const OCFConstantString kPresentationIntentName = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NS"
    "PresentationIntent",
    20
};
id const ONSPresentationIntentAttributeName = (id)&kPresentationIntentName;
