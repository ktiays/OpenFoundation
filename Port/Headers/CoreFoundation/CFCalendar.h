//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDate.h>
#include <CoreFoundation/CFLocale.h>
#include <CoreFoundation/CFTimeZone.h>

CF_EXTERN_C_BEGIN

typedef CFStringRef CFCalendarIdentifier CF_STRING_ENUM;

CF_EXPORT const CFCalendarIdentifier kCFGregorianCalendar;

typedef struct CF_BRIDGED_MUTABLE_TYPE(NSCalendar) __CFCalendar *CFCalendarRef;

typedef CF_OPTIONS(CFOptionFlags, CFCalendarUnit) {
    kCFCalendarUnitEra = (1UL << 1),
    kCFCalendarUnitYear = (1UL << 2),
    kCFCalendarUnitMonth = (1UL << 3),
    kCFCalendarUnitDay = (1UL << 4),
    kCFCalendarUnitHour = (1UL << 5),
    kCFCalendarUnitMinute = (1UL << 6),
    kCFCalendarUnitSecond = (1UL << 7),
    kCFCalendarUnitWeekday = (1UL << 9),
    kCFCalendarUnitWeekdayOrdinal = (1UL << 10),
    kCFCalendarUnitQuarter = (1UL << 11),
    kCFCalendarUnitWeekOfMonth = (1UL << 12),
    kCFCalendarUnitWeekOfYear = (1UL << 13),
    kCFCalendarUnitYearForWeekOfYear = (1UL << 14)
};

CF_EXPORT CFTypeID CFCalendarGetTypeID(void);
CF_EXPORT CFCalendarRef CFCalendarCreateWithIdentifier(CFAllocatorRef allocator, CFCalendarIdentifier identifier);
CF_EXPORT CFCalendarIdentifier CFCalendarGetIdentifier(CFCalendarRef calendar);
CF_EXPORT void CFCalendarSetTimeZone(CFCalendarRef calendar, CFTimeZoneRef tz);
CF_EXPORT CFIndex CFCalendarGetFirstWeekday(CFCalendarRef calendar);
CF_EXPORT void CFCalendarSetFirstWeekday(CFCalendarRef calendar, CFIndex wkdy);
CF_EXPORT CFIndex CFCalendarGetMinimumDaysInFirstWeek(CFCalendarRef calendar);
CF_EXPORT void CFCalendarSetMinimumDaysInFirstWeek(CFCalendarRef calendar, CFIndex mwd);
CF_EXPORT Boolean CFCalendarComposeAbsoluteTime(
    CFCalendarRef calendar,
    CFAbsoluteTime *at,
    const char *componentDesc,
    ...
);

CF_EXTERN_C_END
