//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdint.h>

typedef double OCFTimeInterval;
typedef OCFTimeInterval OCFAbsoluteTime;

OF_EXPORT const OCFTimeInterval kOCFAbsoluteTimeIntervalSince1970;

OF_EXPORT OCFAbsoluteTime OCFAbsoluteTimeGetCurrent(void);

// Scalar ABI verified in CFGregorianDateIsValid, iOS Simulator 24A434,
// 0x1c018dca8, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
typedef struct {
    int32_t year;
    int8_t month;
    int8_t day;
    int8_t hour;
    int8_t minute;
    double second;
} OCFGregorianDate;

OF_EXPORT uint8_t OCFGregorianDateIsValid(OCFGregorianDate gdate, OCFOptionFlags unitFlags);

// Original non-external name is preserved; no process-shared symbol is exported. The 8-bit month and
// day follow the callers, which sign-extend 8-bit fields (SBFX #8) into W1 and W2.
OF_EXTERN double __CFAbsoluteFromYMD(int64_t year, int8_t month, int8_t day);

OF_EXTERN OCFStringRef __CFDateCopyDescription(OCFTypeRef cf);

// CFDate.h (iOS Simulator SDK interface), spelled OCF. CFDateGetTypeID (0x1c0280240) is outside the scope.
typedef const struct __OCFDate *OCFDateRef;

typedef const struct __OCFTimeZone *OCFTimeZoneRef;

// Called by the property-list units; not restored on this branch.
OF_EXPORT OCFTypeID OCFDateGetTypeID(void);
OF_EXPORT OCFGregorianDate OCFAbsoluteTimeGetGregorianDate(OCFAbsoluteTime at, OCFTimeZoneRef tz);
OF_EXPORT OCFAbsoluteTime OCFGregorianDateGetAbsoluteTime(OCFGregorianDate gdate, OCFTimeZoneRef tz);

OF_EXPORT OCFDateRef OCFDateCreate(OCFAllocatorRef allocator, OCFAbsoluteTime at);
OF_EXPORT OCFAbsoluteTime OCFDateGetAbsoluteTime(OCFDateRef theDate);

// The CFDate class record (type 42), for the builtin class table; the type is OCFRuntimeClass
// (Runtime/OCFRuntime.h, a private header this installed header cannot include).
OF_EXTERN const struct __OCFRuntimeClass __CFDateClass;

// The time-stamp-register rates (__CFTSRRate, ticks per second, and its reciprocal __CF1_TSRRate) the TSR
// conversions read, and the once predicate and global block of CFDateGetTypeID that compute them. The
// predicate and block are function-local natively; they are shared here (local descriptive names) because the
// TSR conversions inline the same dispatch_once.
OF_EXTERN double __CFTSRRate;
OF_EXTERN double __CF1_TSRRate;
OF_EXTERN intptr_t dateTypeIDInitOnce;
#ifdef __BLOCKS__
OF_EXTERN void (^const dateTypeIDInitBlock)(void);
#endif
