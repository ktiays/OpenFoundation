//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

@class ONSMutableAttributedString;

// Native _NSAttributedStringFromMarkdownCreatorConcrete, 0x22e6de9c8, non-external. class_ro_t
// 0x222863800 gives instance start 8, size 16. Superclass 0x22e6b6088 is NSObject (confidence: medium).
__attribute__((visibility("hidden")))
@interface _ONSAttributedStringFromMarkdownCreatorConcrete : NSObject {
    @private
    ONSMutableAttributedString *_result;
}
- (id)result;
- (void)appendString:(id)string with:(id)attributes;
- (void)appendString:(id)string;
- (void)finalizeWithLanguageIdentifier:(id)language;
- (id)parseExtendedAttributesFromData:(id)data error:(id *)error;
@end
