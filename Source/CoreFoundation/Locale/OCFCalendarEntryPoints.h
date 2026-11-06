//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>
#include <OpenCoreFoundation/Date/OCFDate.h>

#include "CoreFoundation/Locale/OCFLocaleKeys.h"

// CFCalendar.h (iOS Simulator SDK interface), spelled OCF: the CFCalendar entry points that forward to
// NSCalendar, restored in Locale/OCFCalendarEntryPoints.c in native address order. CFCalendarRef is toll-free
// bridged to NSCalendar. Native kCFGregorianCalendar is declared by Locale/OCFLocaleKeys.h.
typedef struct __OCFCalendar *OCFCalendarRef;
typedef OCFStringRef OCFCalendarIdentifier;

// The calendar unit flags type (CFOptionFlags).
typedef OCFOptionFlags OCFCalendarUnit;

// The private CFDateComponents type (Locale/OCFDateComponents.h restores its unit).
typedef struct __OCFDateComponents *OCFDateComponentsRef;

// CFCalendarGetTypeID, CFCalendarSetTimeZone and the two component conversions are external like the SDK
// entry points (the conversions are private interfaces without an SDK declaration; their parameters are
// spelled from the native calls).
OF_EXPORT OCFTypeID OCFCalendarGetTypeID(void);
OF_EXPORT OCFCalendarRef
OCFCalendarCreateWithIdentifier(OCFAllocatorRef allocator, OCFCalendarIdentifier identifier);
OF_EXPORT OCFCalendarIdentifier OCFCalendarGetIdentifier(OCFCalendarRef calendar);
OF_EXPORT void OCFCalendarSetTimeZone(OCFCalendarRef calendar, OCFTimeZoneRef tz);
OF_EXPORT OCFIndex OCFCalendarGetFirstWeekday(OCFCalendarRef calendar);
OF_EXPORT void OCFCalendarSetFirstWeekday(OCFCalendarRef calendar, OCFIndex wkdy);
OF_EXPORT OCFIndex OCFCalendarGetMinimumDaysInFirstWeek(OCFCalendarRef calendar);
OF_EXPORT void OCFCalendarSetMinimumDaysInFirstWeek(OCFCalendarRef calendar, OCFIndex mwd);
OF_EXPORT OCFDateRef OCFCalendarCreateDateFromComponents(
    OCFAllocatorRef allocator, OCFCalendarRef calendar, OCFDateComponentsRef dateComp
);
OF_EXPORT OCFDateComponentsRef OCFCalendarCreateDateComponentsFromDate(
    OCFAllocatorRef allocator, OCFCalendarRef calendar, OCFCalendarUnit unitFlags, OCFDateRef date
);

// Native _CFCalendarCreateCopy, 0x1c01ced4c. Hidden (was a private external).
OF_EXTERN OCFCalendarRef _CFCalendarCreateCopy(OCFAllocatorRef allocator, OCFCalendarRef calendar);

// The Objective-C classes the entry points message or create: NSCalendar (Objective-C/OCFNSCalendar.m) and
// NSDateComponents (Objective-C/OCFNSDateComponents.m), declared for the naming switch.
#ifdef __OBJC__
@class ONSCalendar;
@class ONSDateComponents;
#endif
