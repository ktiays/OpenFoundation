//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Date/OCFTimeZone.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSTimeZone (__DATA,__objc_data 0x22e6ce580, external), an NSObject subclass; class_ro_t
// 0x2227c54c0: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The method lists are in the shared cache's
// optimized method lists, outside the extracted image; the methods come from the IDA capture
// (Objective-C/OCFNSTimeZone.m). NSTimeInterval is spelled double. The class is external natively (default
// visibility). Only the methods other units send are declared.
__attribute__((visibility("default")))
@interface ONSTimeZone : NSObject

+ (id)defaultTimeZone;
+ (id)timeZoneForSecondsFromGMT:(NSInteger)seconds;
- (id)name;
- (id)data;
- (NSInteger)secondsFromGMTForDate:(id)date;
- (id)abbreviationForDate:(id)date;
- (BOOL)isDaylightSavingTimeForDate:(id)date;
- (double)daylightSavingTimeOffsetForDate:(id)date;
- (id)nextDaylightSavingTimeTransitionAfterDate:(id)date;
- (NSInteger)secondsFromGMT;
- (id)abbreviation;
- (BOOL)isDaylightSavingTime;
- (BOOL)isEqualToTimeZone:(id)aTimeZone;
- (id)initWithName:(id)tzName;
- (id)initWithName:(id)tzName data:(id)aData;

@end

// The placeholder class of this image whose static instance +[NSTimeZone allocWithZone:] answers
// (__NSPlaceholderTimeZone, restored in Objective-C/OCFNSPlaceholderTimeZone.m), declared for the prefix
// tool.
@class __ONSPlaceholderTimeZone;
#endif
