//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Locale/OCFCalendarEntryPoints.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSDateComponents (__DATA,__objc_data 0x22e6cde00, external), an NSObject subclass; class_ro_t
// 0x2227c3ba8: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). An instance carries its CFDateComponents in
// eight indexed bytes (__CFAllocateObject with 8 extra bytes, object_getIndexedIvars). The method lists are
// in the shared cache's optimized method lists, outside the extracted image; the methods come from the IDA
// capture (Objective-C/OCFNSDateComponents.m). NSCalendarUnit is spelled NSUInteger. The class is external
// natively (default visibility).
__attribute__((visibility("default")))
@interface ONSDateComponents : NSObject

- (OCFDateComponentsRef)_dateComponents;
- (id)_initWithCFDateComponents:(OCFDateComponentsRef)dateComponents;
- (id)calendar;
- (void)setCalendar:(id)calendar;
- (id)timeZone;
- (void)setTimeZone:(id)timeZone;
- (NSInteger)era;
- (void)setEra:(NSInteger)era;
- (NSInteger)year;
- (void)setYear:(NSInteger)year;
- (NSInteger)month;
- (void)setMonth:(NSInteger)month;
- (NSInteger)day;
- (void)setDay:(NSInteger)day;
- (NSInteger)hour;
- (void)setHour:(NSInteger)hour;
- (NSInteger)minute;
- (void)setMinute:(NSInteger)minute;
- (NSInteger)second;
- (void)setSecond:(NSInteger)second;
- (NSInteger)week;
- (void)setWeek:(NSInteger)week;
- (NSInteger)weekday;
- (void)setWeekday:(NSInteger)weekday;
- (NSInteger)weekdayOrdinal;
- (void)setWeekdayOrdinal:(NSInteger)weekdayOrdinal;
- (NSInteger)quarter;
- (void)setQuarter:(NSInteger)quarter;
- (NSInteger)nanosecond;
- (void)setNanosecond:(NSInteger)nanosecond;
- (NSInteger)weekOfYear;
- (void)setWeekOfYear:(NSInteger)weekOfYear;
- (NSInteger)weekOfMonth;
- (void)setWeekOfMonth:(NSInteger)weekOfMonth;
- (NSInteger)yearForWeekOfYear;
- (void)setYearForWeekOfYear:(NSInteger)yearForWeekOfYear;
- (NSInteger)dayOfYear;
- (void)setDayOfYear:(NSInteger)dayOfYear;
- (BOOL)isLeapMonth;
- (void)setLeapMonth:(BOOL)leapMonth;
- (BOOL)isRepeatedDay;
- (void)setRepeatedDay:(BOOL)repeatedDay;
- (BOOL)isLeapMonthSet;
- (BOOL)isRepeatedDaySet;
- (id)date;
- (void)setValue:(NSInteger)value forComponent:(NSUInteger)unit;
- (NSInteger)valueForComponent:(NSUInteger)unit;
- (BOOL)isValidDate;
- (BOOL)isValidDateInCalendar:(id)calendar;

@end
#endif
