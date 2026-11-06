//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

// Native class NSAttributedStringMarkdownParsingOptions, 0x22e6de978, external. class_ro_t
// 0x2228636e8 gives instance start 8, size 40 and the ivars below. The superclass pointer
// 0x22e6b6088 is outside the image (libobjc NSObject, confidence: medium).
__attribute__((visibility("default")))
@interface ONSAttributedStringMarkdownParsingOptions : NSObject {
    @private
    BOOL _allowsExtendedAttributes;
    BOOL _appliesSourcePositionAttributes;
    NSInteger _interpretedSyntax;
    NSInteger _failurePolicy;
    ONSString *_languageCode;
}
@property(nonatomic) BOOL allowsExtendedAttributes;
@property(nonatomic) NSInteger interpretedSyntax;
@property(nonatomic) NSInteger failurePolicy;
@property(atomic, copy) ONSString *languageCode;
@property(nonatomic) BOOL appliesSourcePositionAttributes;
- (id)copyWithZone:(struct _NSZone *)zone;
@end
