//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSMutableAttributedString.h"
#import "Foundation/String/ONSMutableString.h"

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

// Native class NSMutableStringProxyForMutableAttributedString, 0x22e6d6250, external.
// class_ro_t 0x222848310 gives superclass NSMutableString, instance start 8 and size 16.
__attribute__((visibility("default")))
@interface ONSMutableStringProxyForMutableAttributedString : ONSMutableString {
    @private
    ONSMutableAttributedString *mutableAttributedString;
}
- (instancetype)initWithMutableAttributedString:(ONSMutableAttributedString *)string;
@end
