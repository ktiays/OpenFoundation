//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>

CF_EXTERN_C_BEGIN

typedef double CFTimeInterval;
typedef CFTimeInterval CFAbsoluteTime;

CF_EXPORT CFAbsoluteTime CFAbsoluteTimeGetCurrent(void);

CF_EXPORT const CFTimeInterval kCFAbsoluteTimeIntervalSince1970;

typedef const struct CF_BRIDGED_TYPE(NSDate) __CFDate *CFDateRef;

CF_EXPORT CFTypeID CFDateGetTypeID(void);
CF_EXPORT CFDateRef CFDateCreate(CFAllocatorRef allocator, CFAbsoluteTime at);
CF_EXPORT CFAbsoluteTime CFDateGetAbsoluteTime(CFDateRef theDate);

typedef const struct CF_BRIDGED_TYPE(NSTimeZone) __CFTimeZone *CFTimeZoneRef;

typedef struct {
    SInt32 year;
    SInt8 month;
    SInt8 day;
    SInt8 hour;
    SInt8 minute;
    double second;
} CFGregorianDate;

typedef CF_OPTIONS(CFOptionFlags, CFGregorianUnitFlags) {
    kCFGregorianUnitsYears = (1UL << 0),
    kCFGregorianUnitsMonths = (1UL << 1),
    kCFGregorianUnitsDays = (1UL << 2),
    kCFGregorianUnitsHours = (1UL << 3),
    kCFGregorianUnitsMinutes = (1UL << 4),
    kCFGregorianUnitsSeconds = (1UL << 5),
    kCFGregorianAllUnits = 0x00FFFFFF
};

CF_EXPORT Boolean CFGregorianDateIsValid(CFGregorianDate gdate, CFOptionFlags unitFlags);
CF_EXPORT CFAbsoluteTime CFGregorianDateGetAbsoluteTime(CFGregorianDate gdate, CFTimeZoneRef tz);
CF_EXPORT CFGregorianDate CFAbsoluteTimeGetGregorianDate(CFAbsoluteTime at, CFTimeZoneRef tz);

CF_EXTERN_C_END
