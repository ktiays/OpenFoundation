//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Locale/OCFCalendarEntryPoints.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"

// The CFDateComponents unit (native source CFDateComponents.c, the file name of its assertions), restored in
// Locale/OCFDateComponents.c. CFDateComponents is a private CoreFoundation type (builtin type ID 66) without
// an SDK header; the declarations spell the native signatures with OCF names.

// Native struct __CFDateComponents (176 bytes: the 16-byte header and 0xA0 extra bytes, read from
// CFDateComponentsCreate and the accessors): the calendar and time zone (retained), then one CFIndex per
// calendar unit, kCFUndefined (0x7fffffffffffffff, NSDateComponentUndefined) when unset. The field names
// are local descriptive spellings; the unit each field answers is in its comment.
struct __OCFDateComponents {
    OCFRuntimeBase base;
    OCFCalendarRef calendar;    // 0x10
    OCFTimeZoneRef timeZone;    // 0x18
    OCFIndex era;               // 0x20, kCFCalendarUnitEra (2)
    OCFIndex year;              // 0x28, kCFCalendarUnitYear (4)
    OCFIndex month;             // 0x30, kCFCalendarUnitMonth (8)
    OCFIndex leapMonth;         // 0x38, 0x40000000
    OCFIndex repeatedDay;       // 0x40, 0xffffffff80000000
    OCFIndex day;               // 0x48, kCFCalendarUnitDay (0x10)
    OCFIndex hour;              // 0x50, kCFCalendarUnitHour (0x20)
    OCFIndex minute;            // 0x58, kCFCalendarUnitMinute (0x40)
    OCFIndex second;            // 0x60, kCFCalendarUnitSecond (0x80)
    OCFIndex week;              // 0x68, kCFCalendarUnitWeek (0x100)
    OCFIndex weekday;           // 0x70, kCFCalendarUnitWeekday (0x200)
    OCFIndex weekdayOrdinal;    // 0x78, kCFCalendarUnitWeekdayOrdinal (0x400)
    OCFIndex quarter;           // 0x80, kCFCalendarUnitQuarter (0x800)
    OCFIndex weekOfMonth;       // 0x88, kCFCalendarUnitWeekOfMonth (0x1000)
    OCFIndex weekOfYear;        // 0x90, kCFCalendarUnitWeekOfYear (0x2000)
    OCFIndex yearForWeekOfYear; // 0x98, kCFCalendarUnitYearForWeekOfYear (0x4000)
    OCFIndex nanosecond;        // 0xa0, 0x8000
    OCFIndex dayOfYear;         // 0xa8, 0x10000
};

_Static_assert(sizeof(struct __OCFDateComponents) == 0xb0, "A CFDateComponents is 176 bytes.");

// Native CFDateComponentsCreate (0x1c0230128), CFDateComponentsCreateCopy (0x1c0230174),
// CFDateComponentsCopyCalendar (0x1c0230244), CFDateComponentsSetCalendar (0x1c0230264),
// CFDateComponentsCopyTimeZone (0x1c02302e8), CFDateComponentsSetTimeZone (0x1c0230308),
// CFDateComponentsGetValue (0x1c0230384), CFDateComponentsSetValue (0x1c02304fc), CFDateComponentsIsValidDate
// (0x1c023066c) and CFDateComponentsIsValidDateInCalendar (0x1c0230680); external.
OF_EXPORT OCFDateComponentsRef OCFDateComponentsCreate(OCFAllocatorRef allocator);
OF_EXPORT OCFDateComponentsRef
OCFDateComponentsCreateCopy(OCFAllocatorRef allocator, OCFDateComponentsRef dateComp);
OF_EXPORT OCFCalendarRef OCFDateComponentsCopyCalendar(OCFDateComponentsRef dateComp);
OF_EXPORT void OCFDateComponentsSetCalendar(OCFDateComponentsRef dateComp, OCFCalendarRef calendar);
OF_EXPORT OCFTimeZoneRef OCFDateComponentsCopyTimeZone(OCFDateComponentsRef dateComp);
OF_EXPORT void OCFDateComponentsSetTimeZone(OCFDateComponentsRef dateComp, OCFTimeZoneRef timeZone);
OF_EXPORT OCFIndex OCFDateComponentsGetValue(OCFDateComponentsRef dateComp, OCFCalendarUnit unit);
OF_EXPORT void OCFDateComponentsSetValue(OCFDateComponentsRef dateComp, OCFCalendarUnit unit, OCFIndex value);
OF_EXPORT Boolean OCFDateComponentsIsValidDate(OCFDateComponentsRef dateComp);
OF_EXPORT Boolean
OCFDateComponentsIsValidDateInCalendar(OCFDateComponentsRef dateComp, OCFCalendarRef inCalendar);

// Native __CFDateComponentsCopyDescriptionInner, 0x1c022fa54. Hidden (was a private external).
OF_EXTERN OCFStringRef __CFDateComponentsCopyDescriptionInner(OCFDateComponentsRef dateComp);

// The CFDateComponents class record (type 66), iOS Simulator 24A434, 0x2227a56f0 (__DATA_CONST,__const).
// Hidden (was a private external); original name preserved.
OF_EXTERN const OCFRuntimeClass __CFDateComponentsClass;

// Native CFDateComponentsGetTypeID, 0x1c0230120. External.
OF_EXPORT OCFTypeID OCFDateComponentsGetTypeID(void);
