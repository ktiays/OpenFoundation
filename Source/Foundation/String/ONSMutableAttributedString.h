//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSAttributedString.h"

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

#ifdef __OBJC__
// Native class NSMutableAttributedString (__DATA,__objc_data 0x22e6d61b0, external), an NSAttributedString
// subclass; class_ro_t 0x222848160: flags 0, instance start and size 8, no ivars (confidence: verified).
__attribute__((visibility("default")))
@interface ONSMutableAttributedString : ONSAttributedString
+ (id)allocWithZone:(struct _NSZone *)zone;
- (id)mutableString;
- (void)addAttribute:(id)name value:(id)value range:(ONSRange)range;
- (void)addAttributes:(id)attributes range:(ONSRange)range;
- (void)removeAttribute:(id)name range:(ONSRange)range;
- (void)replaceCharactersInRange:(ONSRange)range withAttributedString:(id)string;
- (void)appendAttributedString:(id)string;
- (void)deleteCharactersInRange:(ONSRange)range;
- (void)beginEditing;
- (void)endEditing;
- (Class)classForCoder;
- (void)replaceCharactersInRange:(ONSRange)range withString:(id)string;
- (void)setAttributes:(id)attributes range:(ONSRange)range;
@end
#endif
