//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSCalendar (__DATA,__objc_data 0x22e6ce530, external), an NSObject subclass; class_ro_t
// 0x2227c5340: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The method lists are in the shared cache's
// optimized method lists, outside the extracted image; the methods come from the IDA capture
// (Objective-C/OCFNSCalendar.m). NSCalendarUnit and NSCalendarOptions are spelled NSUInteger, NSTimeInterval
// double, NSComparisonResult NSInteger and NSRange OCFRange; the enumeration block parameter is noescape (the
// native stack blocks carry no copy or dispose helpers, flags 0xc0000000). Only the methods other units send
// are declared. The class is external natively (default visibility).
__attribute__((visibility("default")))
@interface ONSCalendar : NSObject

+ (id)currentCalendar;
- (id)initWithCalendarIdentifier:(id)identifier;
- (id)calendarIdentifier;
- (id)locale;
- (void)setLocale:(id)locale;
- (id)timeZone;
- (void)setTimeZone:(id)timeZone;
- (NSUInteger)firstWeekday;
- (void)setFirstWeekday:(NSUInteger)weekday;
- (NSUInteger)minimumDaysInFirstWeek;
- (void)setMinimumDaysInFirstWeek:(NSUInteger)days;
- (id)gregorianStartDate;
- (void)setGregorianStartDate:(id)date;
- (id)dateFromComponents:(id)components;
- (id)components:(NSUInteger)unitFlags fromDate:(id)date;
- (NSInteger)component:(NSUInteger)unit fromDate:(id)date;
- (BOOL)rangeOfUnit:(NSUInteger)unit startDate:(id *)datep interval:(double *)tip forDate:(id)date;
- (NSInteger)compareDate:(id)date1 toDate:(id)date2 toUnitGranularity:(NSUInteger)unit;
- (BOOL)isDateInToday:(id)date;
- (BOOL)isDateInTomorrow:(id)date;
- (BOOL)isDateInYesterday:(id)date;
- (BOOL)isDate:(id)date1 inSameDayAsDate:(id)date2;
- (BOOL)nextWeekendStartDate:(id *)datep
                    interval:(double *)tip
                     options:(NSUInteger)options
                   afterDate:(id)date;
- (id)nextDateAfterDate:(id)date matchingComponents:(id)comps options:(NSUInteger)options;
- (void)
    enumerateDatesStartingAfterDate:(id)start
                 matchingComponents:(id)comps
                            options:(NSUInteger)opts
                         usingBlock:
                             (void(__attribute__((noescape)) ^)(id date, BOOL exactMatch, BOOL *stop))block;
- (id)components:(NSUInteger)unitFlags
        fromDate:(id)startingDate
          toDate:(id)resultDate
         options:(NSUInteger)opts;
- (id)dateByAddingComponents:(id)comps toDate:(id)date options:(NSUInteger)opts;

@end

// The Swift calendar class +[NSCalendar allocWithZone:] looks up by name (_NSSwiftCalendar, Foundation's,
// outside this image), declared so that the prefix tool maps its runtime name.
@class _ONSSwiftCalendar;
#endif
