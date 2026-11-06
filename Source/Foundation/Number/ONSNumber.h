//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Number/ONSValue.h"

#ifdef __OBJC__
// Native NSNumber, 0x22e6de400, external; superclass NSValue. class_ro_t 0x222862ae0: flags 0,
// instance start and size 8, no ivars. Foundation, iOS Simulator 24A434, arm64, UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device 24A446: unread.
// Reading: Source/Foundation/Number/Reading.md. Only the captured selectors are restored.
// The native protocol-list reference 0x23d890469 remains unread outside the extracted image.
// iOS Device divergence: this family remains unread; no device equivalence is claimed.
__attribute__((visibility("default")))
@interface ONSNumber : ONSValue

+ (id)numberWithChar:(char)value;
+ (id)numberWithUnsignedChar:(unsigned char)value;
+ (id)numberWithShort:(short)value;
+ (id)numberWithUnsignedShort:(unsigned short)value;
+ (id)numberWithInt:(int)value;
+ (id)numberWithUnsignedInt:(unsigned int)value;
+ (id)numberWithInteger:(NSInteger)value;
+ (id)numberWithUnsignedInteger:(NSUInteger)value;
+ (id)numberWithLong:(long)value;
+ (id)numberWithUnsignedLong:(unsigned long)value;
+ (id)numberWithLongLong:(long long)value;
+ (id)numberWithUnsignedLongLong:(unsigned long long)value;
+ (id)numberWithFloat:(float)value;
+ (id)numberWithDouble:(double)value;
+ (id)numberWithBool:(BOOL)value;
- (id)initWithChar:(char)value;
- (id)initWithUnsignedChar:(unsigned char)value;
- (id)initWithShort:(short)value;
- (id)initWithUnsignedShort:(unsigned short)value;
- (id)initWithInt:(int)value;
- (id)initWithUnsignedInt:(unsigned int)value;
- (id)initWithInteger:(NSInteger)value;
- (id)initWithUnsignedInteger:(NSUInteger)value;
- (id)initWithLong:(long)value;
- (id)initWithUnsignedLong:(unsigned long)value;
- (id)initWithLongLong:(long long)value;
- (id)initWithUnsignedLongLong:(unsigned long long)value;
- (id)initWithFloat:(float)value;
- (id)initWithDouble:(double)value;
- (id)initWithBool:(BOOL)value;
- (BOOL)boolValue;
- (char)charValue;
- (unsigned char)unsignedCharValue;
- (short)shortValue;
- (unsigned short)unsignedShortValue;
- (int)intValue;
- (unsigned int)unsignedIntValue;
- (NSInteger)integerValue;
- (NSUInteger)unsignedIntegerValue;
- (long)longValue;
- (unsigned long)unsignedLongValue;
- (long long)longLongValue;
- (unsigned long long)unsignedLongLongValue;
- (float)floatValue;
- (double)doubleValue;
- (NSInteger)compare:(id)otherNumber;
- (BOOL)isEqualToNumber:(id)number;
- (id)descriptionWithLocale:(id)locale;
- (id)redactedDescription;
- (id)stringValue;
- (BOOL)_allowsDirectEncoding;
- (BOOL)_getCString:(char *)buffer length:(int)length multiplier:(double)multiplier;

@end
#endif
