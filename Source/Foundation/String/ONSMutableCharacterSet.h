//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSCharacterSet.h"

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

#ifdef __OBJC__
// Native class NSMutableCharacterSet (__DATA,__objc_data 0x22e6d6980, external), an NSCharacterSet subclass;
// class_ro_t 0x222849b68: flags 0, instance start and size 8, no ivars (confidence: verified). Restored as
// for NSCharacterSet.
__attribute__((visibility("default")))
@interface ONSMutableCharacterSet : ONSCharacterSet

+ (id)characterSetWithCharactersInString:(id)aString;
- (id)init;
- (id)copyWithZone:(struct _NSZone *)zone;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
- (void)addCharactersInRange:(ONSRange)aRange;
- (void)removeCharactersInRange:(ONSRange)aRange;
- (void)addCharactersInString:(id)aString;
- (void)removeCharactersInString:(id)aString;
- (void)formUnionWithCharacterSet:(id)otherSet;
- (void)formIntersectionWithCharacterSet:(id)otherSet;
- (void)invert;
- (BOOL)isMutable;
+ (id)whitespaceCharacterSet;
+ (id)whitespaceAndNewlineCharacterSet;
+ (id)decimalDigitCharacterSet;
+ (id)alphanumericCharacterSet;
- (id)initWithCoder:(id)coder;
+ (BOOL)supportsSecureCoding;

@end

// Native NSCFCharacterSet, 0x22e6d69f8, was private external. class_ro_t 0x222849bf8
// gives superclass NSMutableCharacterSet, instance start and size 8, and no additional ivars.
__attribute__((visibility("hidden")))
@interface ONSCFCharacterSet : ONSMutableCharacterSet
- (Class)classForArchiver;
@end

#endif
