//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/OCFDateComponents.h"

#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"

#include <assert.h>

// Inferred organization: the CFDateComponents functions family J restores lie in one iOS Simulator __text
// region (__CFDateComponentsCopyDescriptionInner 0x1c022fa54 through CFDateComponentsIsValidDateInCalendar
// 0x1c0230680, with OUTLINED_FUNCTION_0_29 at 0x1c0230938 and the .cold fragments at 0x1c030cbd8-0x1c030ccec)
// with the unit's class callbacks (__CFDateComponentsDeallocate 0x1c022fd74, __CFDateComponentsEqual,
// __CFDateComponentsHash, __CFDateComponentsCopyDescription 0x1c02300a4, their .cold fragments at
// 0x1c030cb10-0x1c030cbb0) and CFDateComponentsGetTypeID (0x1c0230120), restored from
// Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json, and the leap-month and repeated-day accessors
// (0x1c022f9fc-0x1c022fa3c), which are outside the evidence and not restored. The native source path is
// CFDateComponents.c (the file name of the unit's assertions). Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full;
// the constant strings resolved from the ADRP/ADD encodings and read with Local/Scratch/cf-core/macho.py);
// iOS Device 24A446: unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.
//
// The assertions (__assert_rtn with the function name, "CFDateComponents.c", the line and "NULL !=
// dateComp", OUTLINED_FUNCTION_0_29 loading the last two) are the unit's .cold.1/.cold.2 fragments. The
// function name is the native __func__, spelled with OF_NAME_STRING so that each naming mode names the
// function as it is built.

// kCFUndefined of the unit fields (NSDateComponentUndefined, 0x7fffffffffffffff).
#define kUndefinedComponent ((OCFIndex)0x7fffffffffffffff)

// The repeated-day unit as the native code compares it: 0xffffffff80000000 (1 << 31 sign-extended).
#define kRepeatedDayUnit ((OCFCalendarUnit)(int64_t)INT32_MIN)

// The leap-month unit (0x40000000).
#define kLeapMonthUnit ((OCFCalendarUnit)0x40000000)

// Native __CFDateComponentsCopyDescriptionInner, 0x1c022fa54. Hidden (was a private external).
// A new mutable string (system default allocator, no maximum length) gets "{", then, each only when set
// (the calendar and time zone when not NULL, the values when not kCFUndefined), in this order:
// "\n    Calendar: %@", "\n    TimeZone: %@", "\n    Era: %ld", "\n    Calendar Year: %ld",
// "\n    Day of Year: %ld", "\n    Month: %ld", "\n    Leap Month: %ld", "\n    Day: %ld",
// "\n    Repeated Day: %ld", "\n    Hour: %ld", "\n    Minute: %ld", "\n    Second: %ld",
// "\n    Nanosecond: %ld", "\n    Quarter: %ld", "\n    Year for Week of Year: %ld", "\n    Week of Year:
// %ld",
// "\n    Week of Month: %ld", "\n    Week (obsolete): %ld", "\n    Weekday: %ld" and
// "\n    Weekday Ordinal: %ld" (the constant strings 0x2227b4280-0x2227b4500). No closing brace is appended.
OCFStringRef __CFDateComponentsCopyDescriptionInner(OCFDateComponentsRef dateComp) {
    OCFMutableStringRef result = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    OCFStringAppend(result, OF_STR("{"));
    if (dateComp->calendar != NULL) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Calendar: %@"), dateComp->calendar);
    }
    if (dateComp->timeZone != NULL) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    TimeZone: %@"), dateComp->timeZone);
    }
    if (dateComp->era != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Era: %ld"), dateComp->era);
    }
    if (dateComp->year != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Calendar Year: %ld"), dateComp->year);
    }
    if (dateComp->dayOfYear != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Day of Year: %ld"), dateComp->dayOfYear);
    }
    if (dateComp->month != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Month: %ld"), dateComp->month);
    }
    if (dateComp->leapMonth != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Leap Month: %ld"), dateComp->leapMonth);
    }
    if (dateComp->day != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Day: %ld"), dateComp->day);
    }
    if (dateComp->repeatedDay != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Repeated Day: %ld"), dateComp->repeatedDay);
    }
    if (dateComp->hour != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Hour: %ld"), dateComp->hour);
    }
    if (dateComp->minute != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Minute: %ld"), dateComp->minute);
    }
    if (dateComp->second != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Second: %ld"), dateComp->second);
    }
    if (dateComp->nanosecond != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Nanosecond: %ld"), dateComp->nanosecond);
    }
    if (dateComp->quarter != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Quarter: %ld"), dateComp->quarter);
    }
    if (dateComp->yearForWeekOfYear != kUndefinedComponent) {
        OCFStringAppendFormat(
            result, NULL, OF_STR("\n    Year for Week of Year: %ld"), dateComp->yearForWeekOfYear
        );
    }
    if (dateComp->weekOfYear != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Week of Year: %ld"), dateComp->weekOfYear);
    }
    if (dateComp->weekOfMonth != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Week of Month: %ld"), dateComp->weekOfMonth);
    }
    if (dateComp->week != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Week (obsolete): %ld"), dateComp->week);
    }
    if (dateComp->weekday != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Weekday: %ld"), dateComp->weekday);
    }
    if (dateComp->weekdayOrdinal != kUndefinedComponent) {
        OCFStringAppendFormat(result, NULL, OF_STR("\n    Weekday Ordinal: %ld"), dateComp->weekdayOrdinal);
    }
    return result;
}

static void __CFDateComponentsDeallocate(OCFTypeRef cf);
static Boolean __CFDateComponentsEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFDateComponentsHash(OCFTypeRef cf);
static OCFStringRef __CFDateComponentsCopyDescription(OCFTypeRef cf);

// Native __CFDateComponentsClass, 0x2227a56f0 (read with Local/Scratch/cf-core/macho.py). Hidden (was a
// private external). Version 0, name "CFDateComponents" (0x1c048c13c), finalize, equal, hash and the debug
// description; no init, copy, formatting description, reclaim, refcount or alignment.
const OCFRuntimeClass __CFDateComponentsClass = {
    0,
    "CFDateComponents",
    NULL,
    NULL,
    __CFDateComponentsDeallocate,
    __CFDateComponentsEqual,
    __CFDateComponentsHash,
    NULL,
    __CFDateComponentsCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CFDateComponentsDeallocate, 0x1c022fd74, with .cold.1 (0x1c030cb10, line 152, "NULL != cf").
// Non-external. NULL fails the assertion; the calendar, then the time zone, is released when present.
static void __CFDateComponentsDeallocate(OCFTypeRef cf) {
    if (cf == NULL) {
        __assert_rtn("__CFDateComponentsDeallocate", "CFDateComponents.c", 152, "NULL != cf");
    }
    const struct __OCFDateComponents *dateComp = cf;
    if (dateComp->calendar != NULL) {
        OCFRelease(dateComp->calendar);
    }
    if (dateComp->timeZone != NULL) {
        OCFRelease(dateComp->timeZone);
    }
}

// Native __CFDateComponentsEqual, 0x1c022fdb8, with .cold.2 (0x1c030cb60, line 20, "NULL != cf1") and .cold.1
// (0x1c030cb38, line 21, "NULL != cf2"). Non-external. The values are compared in the native order (era,
// year, quarter, month, day, hour, minute, second, nanosecond, week, week of year, day of year, week of
// month, year for week of year, weekday, weekday ordinal); the leap month also matches when one side is
// kCFUndefined and the other 0; then the repeated day. A calendar or time zone present on one side only
// differs; present on both, CFEqual decides.
static Boolean __CFDateComponentsEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    if (cf1 == NULL) {
        __assert_rtn("__CFDateComponentsEqual", "CFDateComponents.c", 20, "NULL != cf1");
    }
    if (cf2 == NULL) {
        __assert_rtn("__CFDateComponentsEqual", "CFDateComponents.c", 21, "NULL != cf2");
    }
    const struct __OCFDateComponents *first = cf1;
    const struct __OCFDateComponents *second = cf2;
    if (first->era != second->era || first->year != second->year || first->quarter != second->quarter ||
        first->month != second->month || first->day != second->day || first->hour != second->hour ||
        first->minute != second->minute || first->second != second->second ||
        first->nanosecond != second->nanosecond || first->week != second->week ||
        first->weekOfYear != second->weekOfYear || first->dayOfYear != second->dayOfYear ||
        first->weekOfMonth != second->weekOfMonth || first->yearForWeekOfYear != second->yearForWeekOfYear ||
        first->weekday != second->weekday || first->weekdayOrdinal != second->weekdayOrdinal) {
        return false;
    }
    OCFIndex firstLeapMonth = first->leapMonth;
    OCFIndex secondLeapMonth = second->leapMonth;
    bool leapMonthsMatch;
    if (firstLeapMonth == kUndefinedComponent) {
        leapMonthsMatch = secondLeapMonth == 0 || firstLeapMonth == secondLeapMonth;
    } else if (firstLeapMonth == 0 && secondLeapMonth == kUndefinedComponent) {
        leapMonthsMatch = true;
    } else {
        leapMonthsMatch = firstLeapMonth == secondLeapMonth;
    }
    if (!leapMonthsMatch) {
        return false;
    }
    if (first->repeatedDay != second->repeatedDay) {
        return false;
    }
    if (first->calendar != NULL) {
        if (second->calendar == NULL) {
            return false;
        }
        if (!OCFEqual(first->calendar, second->calendar)) {
            return false;
        }
    } else if (second->calendar != NULL) {
        return false;
    }
    if (first->timeZone == NULL) {
        return second->timeZone == NULL;
    }
    if (second->timeZone == NULL) {
        return false;
    }
    return OCFEqual(first->timeZone, second->timeZone) ? true : false;
}

// Native __CFDateComponentsHash, 0x1c022ff84, with .cold.1 (0x1c030cb88, line 59, "NULL != cf").
// Non-external. The calendar and time zone hashes (0 when absent) are combined with XOR; kCFUndefined counts
// as 0 for the month, day, hour, minute, second, day of year and the two years; the other fields are used as
// stored. The sum keeps the native association (64-bit wrapping arithmetic):
// month * 2678437 + hashes + (hour * 3607 + minute * 61) + second
// + (year + yearForWeekOfYear) * 32832013 + (day + dayOfYear) * 86413
// + 32 * (weekOfYear * 41 + weekOfMonth * 11 - weekday + 8 * weekday + 3 * weekdayOrdinal + quarter).
static OCFHashCode __CFDateComponentsHash(OCFTypeRef cf) {
    if (cf == NULL) {
        __assert_rtn("__CFDateComponentsHash", "CFDateComponents.c", 59, "NULL != cf");
    }
    const struct __OCFDateComponents *dateComp = cf;
    uint64_t calendarHash = 0;
    if (dateComp->calendar != NULL) {
        calendarHash = OCFHash(dateComp->calendar);
    }
    uint64_t timeZoneHash = 0;
    if (dateComp->timeZone != NULL) {
        timeZoneHash = OCFHash(dateComp->timeZone);
    }
    uint64_t hashes = timeZoneHash ^ calendarHash;
    uint64_t year = dateComp->year == kUndefinedComponent ? 0 : (uint64_t)dateComp->year;
    uint64_t day = dateComp->day == kUndefinedComponent ? 0 : (uint64_t)dateComp->day;
    uint64_t second = dateComp->second == kUndefinedComponent ? 0 : (uint64_t)dateComp->second;
    uint64_t yearForWeekOfYear =
        dateComp->yearForWeekOfYear == kUndefinedComponent ? 0 : (uint64_t)dateComp->yearForWeekOfYear;
    uint64_t dayOfYear = dateComp->dayOfYear == kUndefinedComponent ? 0 : (uint64_t)dateComp->dayOfYear;
    uint64_t years = yearForWeekOfYear + year;
    uint64_t month = dateComp->month == kUndefinedComponent ? 0 : (uint64_t)dateComp->month * 2678437;
    uint64_t days = dayOfYear + day;
    uint64_t weeks = (uint64_t)dateComp->weekOfMonth * 11 + (uint64_t)dateComp->weekOfYear * 41;
    uint64_t hour = dateComp->hour == kUndefinedComponent ? 0 : (uint64_t)dateComp->hour * 3607;
    uint64_t minute = dateComp->minute == kUndefinedComponent ? 0 : (uint64_t)dateComp->minute * 61;
    uint64_t weekday = (uint64_t)dateComp->weekday;
    uint64_t small = (weeks - weekday) + (weekday << 3);
    small = small + ((uint64_t)dateComp->weekdayOrdinal * 3 + (uint64_t)dateComp->quarter);
    uint64_t result = month + hashes;
    result = result + (hour + minute);
    result = result + second;
    result = years * 32832013 + result;
    result = days * 86413 + result;
    return result + (small << 5);
}

// Native __CFDateComponentsCopyDescription, 0x1c02300a4, with .cold.1 (0x1c030cbb0, line 143, "NULL != cf").
// Non-external. "<CFDateComponents %p [%p]>%@" with the object, its allocator and the inner description,
// formatted with the system default allocator; the inner description is released.
static OCFStringRef __CFDateComponentsCopyDescription(OCFTypeRef cf) {
    if (cf == NULL) {
        __assert_rtn("__CFDateComponentsCopyDescription", "CFDateComponents.c", 143, "NULL != cf");
    }
    OCFStringRef inner = __CFDateComponentsCopyDescriptionInner((OCFDateComponentsRef)cf);
    OCFStringRef result = OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault,
        NULL,
        OF_STR("<CFDateComponents %p [%p]>%@"),
        cf,
        OCFGetAllocator(cf),
        inner
    );
    OCFRelease(inner);
    return result;
}

// Native CFDateComponentsGetTypeID, 0x1c0230120. External. The builtin type ID 66.
OCFTypeID OCFDateComponentsGetTypeID(void) {
    return 66;
}

// Native CFDateComponentsCreate, 0x1c0230128. External.
// A NULL allocator is CFAllocatorGetDefault(). A type-66 instance with 0xA0 extra bytes and no category; on
// success the calendar and time zone are cleared and every value field (0x20-0xAF) is set to kCFUndefined.
OCFDateComponentsRef OCFDateComponentsCreate(OCFAllocatorRef allocator) {
    if (allocator == NULL) {
        allocator = OCFAllocatorGetDefault();
    }
    struct __OCFDateComponents *dateComp =
        (struct __OCFDateComponents *)_OCFRuntimeCreateInstance(allocator, 66, 0xA0, NULL);
    if (dateComp != NULL) {
        dateComp->calendar = NULL;
        dateComp->timeZone = NULL;
        dateComp->era = kUndefinedComponent;
        dateComp->year = kUndefinedComponent;
        dateComp->month = kUndefinedComponent;
        dateComp->leapMonth = kUndefinedComponent;
        dateComp->repeatedDay = kUndefinedComponent;
        dateComp->day = kUndefinedComponent;
        dateComp->hour = kUndefinedComponent;
        dateComp->minute = kUndefinedComponent;
        dateComp->second = kUndefinedComponent;
        dateComp->week = kUndefinedComponent;
        dateComp->weekday = kUndefinedComponent;
        dateComp->weekdayOrdinal = kUndefinedComponent;
        dateComp->quarter = kUndefinedComponent;
        dateComp->weekOfMonth = kUndefinedComponent;
        dateComp->weekOfYear = kUndefinedComponent;
        dateComp->yearForWeekOfYear = kUndefinedComponent;
        dateComp->nanosecond = kUndefinedComponent;
        dateComp->dayOfYear = kUndefinedComponent;
    }
    return dateComp;
}

// Native CFDateComponentsCreateCopy, 0x1c0230174, with .cold.1 (0x1c030cbd8: "Out of memory", stored and
// BRK #1). External.
// A new instance from CFDateComponentsCreate with the allocator (NULL crashes); the source's calendar and
// time zone are copied (CFDateComponentsCopy... and Set..., then released) when present, then the values are
// copied in the native order (0x20-0x8F in 16-byte pairs, 0xA8, the 0x90 pair, then 0xA0). The source is
// not checked for NULL here.
OCFDateComponentsRef OCFDateComponentsCreateCopy(OCFAllocatorRef allocator, OCFDateComponentsRef dateComp) {
    OCFDateComponentsRef result = OCFDateComponentsCreate(allocator);
    if (result == NULL) {
        crashWithMessage("Out of memory");
    }
    OCFCalendarRef calendar = OCFDateComponentsCopyCalendar(dateComp);
    if (calendar != NULL) {
        OCFDateComponentsSetCalendar(result, calendar);
        OCFRelease(calendar);
    }
    OCFTimeZoneRef timeZone = OCFDateComponentsCopyTimeZone(dateComp);
    if (timeZone != NULL) {
        OCFDateComponentsSetTimeZone(result, timeZone);
        OCFRelease(timeZone);
    }
    result->era = dateComp->era;
    result->year = dateComp->year;
    result->month = dateComp->month;
    result->leapMonth = dateComp->leapMonth;
    result->repeatedDay = dateComp->repeatedDay;
    result->day = dateComp->day;
    result->hour = dateComp->hour;
    result->minute = dateComp->minute;
    result->second = dateComp->second;
    result->week = dateComp->week;
    result->weekday = dateComp->weekday;
    result->weekdayOrdinal = dateComp->weekdayOrdinal;
    result->quarter = dateComp->quarter;
    result->weekOfMonth = dateComp->weekOfMonth;
    result->dayOfYear = dateComp->dayOfYear;
    result->weekOfYear = dateComp->weekOfYear;
    result->yearForWeekOfYear = dateComp->yearForWeekOfYear;
    result->nanosecond = dateComp->nanosecond;
    return result;
}

// Native CFDateComponentsCopyCalendar, 0x1c0230244, with .cold.1 (0x1c030cbec, line 377). External.
// NULL fails the assertion; the calendar is retained with CFRetain (a tail call) unless it is NULL.
OCFCalendarRef OCFDateComponentsCopyCalendar(OCFDateComponentsRef dateComp) {
    if (dateComp == NULL) {
        __assert_rtn(
            OF_NAME_STRING(OCFDateComponentsCopyCalendar), "CFDateComponents.c", 377, "NULL != dateComp"
        );
    }
    OCFCalendarRef calendar = dateComp->calendar;
    if (calendar == NULL) {
        return NULL;
    }
    return (OCFCalendarRef)OCFRetain(calendar);
}

// Native CFDateComponentsSetCalendar, 0x1c0230264, with .cold.1 (0x1c030cc10, line 386). External.
// NULL fails the assertion. A calendar CFEqual to the current one (both present) changes nothing; otherwise
// the current one is released and cleared, and a new calendar is stored as a copy (_CFCalendarCreateCopy with
// the system default allocator), given the components' time zone first when there is one.
void OCFDateComponentsSetCalendar(OCFDateComponentsRef dateComp, OCFCalendarRef calendar) {
    if (dateComp == NULL) {
        __assert_rtn(
            OF_NAME_STRING(OCFDateComponentsSetCalendar), "CFDateComponents.c", 386, "NULL != dateComp"
        );
    }
    OCFCalendarRef current = dateComp->calendar;
    if (calendar != NULL && current != NULL) {
        if (OCFEqual(current, calendar)) {
            return;
        }
        current = dateComp->calendar;
    }
    if (current != NULL) {
        OCFRelease(current);
        dateComp->calendar = NULL;
    }
    if (calendar == NULL) {
        return;
    }
    OCFCalendarRef copy = _CFCalendarCreateCopy(kOCFAllocatorSystemDefault, calendar);
    OCFTimeZoneRef timeZone = dateComp->timeZone;
    if (timeZone != NULL) {
        OCFCalendarSetTimeZone(copy, timeZone);
    }
    dateComp->calendar = copy;
}

// Native CFDateComponentsCopyTimeZone, 0x1c02302e8, with .cold.1 (0x1c030cc34, line 407). External.
// NULL fails the assertion; the time zone is retained with CFRetain (a tail call) unless it is NULL.
OCFTimeZoneRef OCFDateComponentsCopyTimeZone(OCFDateComponentsRef dateComp) {
    if (dateComp == NULL) {
        __assert_rtn(
            OF_NAME_STRING(OCFDateComponentsCopyTimeZone), "CFDateComponents.c", 407, "NULL != dateComp"
        );
    }
    OCFTimeZoneRef timeZone = dateComp->timeZone;
    if (timeZone == NULL) {
        return NULL;
    }
    return (OCFTimeZoneRef)OCFRetain(timeZone);
}

// Native CFDateComponentsSetTimeZone, 0x1c0230308, with .cold.1 (0x1c030cc58, line 416). External.
// NULL fails the assertion. A time zone CFEqual to the current one (both present) changes nothing; otherwise
// the current one is released and cleared, and a new one is retained, stored and, when the components have a
// calendar, set on it (CFCalendarSetTimeZone, a tail call).
void OCFDateComponentsSetTimeZone(OCFDateComponentsRef dateComp, OCFTimeZoneRef timeZone) {
    if (dateComp == NULL) {
        __assert_rtn(
            OF_NAME_STRING(OCFDateComponentsSetTimeZone), "CFDateComponents.c", 416, "NULL != dateComp"
        );
    }
    OCFTimeZoneRef current = dateComp->timeZone;
    if (timeZone != NULL && current != NULL) {
        if (OCFEqual(current, timeZone)) {
            return;
        }
        current = dateComp->timeZone;
    }
    if (current != NULL) {
        OCFRelease(current);
        dateComp->timeZone = NULL;
    }
    if (timeZone == NULL) {
        return;
    }
    dateComp->timeZone = (OCFTimeZoneRef)OCFRetain(timeZone);
    OCFCalendarRef calendar = dateComp->calendar;
    if (calendar != NULL) {
        OCFCalendarSetTimeZone(calendar, timeZone);
    }
}

// Native CFDateComponentsGetValue, 0x1c0230384, with .cold.1 (0x1c030cc7c, line 243). External.
// NULL fails the assertion. The unit selects its field (signed comparisons natively); any other unit
// answers kCFUndefined.
OCFIndex OCFDateComponentsGetValue(OCFDateComponentsRef dateComp, OCFCalendarUnit unit) {
    if (dateComp == NULL) {
        __assert_rtn(
            OF_NAME_STRING(OCFDateComponentsGetValue), "CFDateComponents.c", 243, "NULL != dateComp"
        );
    }
    switch (unit) {
        case 0x2:
            return dateComp->era;
        case 0x4:
            return dateComp->year;
        case 0x8:
            return dateComp->month;
        case kLeapMonthUnit:
            return dateComp->leapMonth;
        case kRepeatedDayUnit:
            return dateComp->repeatedDay;
        case 0x10:
            return dateComp->day;
        case 0x20:
            return dateComp->hour;
        case 0x40:
            return dateComp->minute;
        case 0x80:
            return dateComp->second;
        case 0x100:
            return dateComp->week;
        case 0x200:
            return dateComp->weekday;
        case 0x400:
            return dateComp->weekdayOrdinal;
        case 0x800:
            return dateComp->quarter;
        case 0x1000:
            return dateComp->weekOfMonth;
        case 0x2000:
            return dateComp->weekOfYear;
        case 0x4000:
            return dateComp->yearForWeekOfYear;
        case 0x8000:
            return dateComp->nanosecond;
        case 0x10000:
            return dateComp->dayOfYear;
        default:
            return kUndefinedComponent;
    }
}

// Native CFDateComponentsSetValue, 0x1c02304fc, with .cold.1 (0x1c030cca0, line 311). External.
// NULL fails the assertion. The unit selects the field that stores the value; any other unit changes
// nothing.
void OCFDateComponentsSetValue(OCFDateComponentsRef dateComp, OCFCalendarUnit unit, OCFIndex value) {
    if (dateComp == NULL) {
        __assert_rtn(
            OF_NAME_STRING(OCFDateComponentsSetValue), "CFDateComponents.c", 311, "NULL != dateComp"
        );
    }
    switch (unit) {
        case 0x2:
            dateComp->era = value;
            break;
        case 0x4:
            dateComp->year = value;
            break;
        case 0x8:
            dateComp->month = value;
            break;
        case kLeapMonthUnit:
            dateComp->leapMonth = value;
            break;
        case kRepeatedDayUnit:
            dateComp->repeatedDay = value;
            break;
        case 0x10:
            dateComp->day = value;
            break;
        case 0x20:
            dateComp->hour = value;
            break;
        case 0x40:
            dateComp->minute = value;
            break;
        case 0x80:
            dateComp->second = value;
            break;
        case 0x100:
            dateComp->week = value;
            break;
        case 0x200:
            dateComp->weekday = value;
            break;
        case 0x400:
            dateComp->weekdayOrdinal = value;
            break;
        case 0x800:
            dateComp->quarter = value;
            break;
        case 0x1000:
            dateComp->weekOfMonth = value;
            break;
        case 0x2000:
            dateComp->weekOfYear = value;
            break;
        case 0x4000:
            dateComp->yearForWeekOfYear = value;
            break;
        case 0x8000:
            dateComp->nanosecond = value;
            break;
        case 0x10000:
            dateComp->dayOfYear = value;
            break;
        default:
            break;
    }
}

// Native CFDateComponentsIsValidDate, 0x1c023066c. External. Without a calendar, false; otherwise
// CFDateComponentsIsValidDateInCalendar with it (a tail call). The components are not checked for NULL here.
Boolean OCFDateComponentsIsValidDate(OCFDateComponentsRef dateComp) {
    OCFCalendarRef calendar = dateComp->calendar;
    if (calendar == NULL) {
        return false;
    }
    return OCFDateComponentsIsValidDateInCalendar(dateComp, calendar);
}

// Native CFDateComponentsIsValidDateInCalendar, 0x1c0230680, with .cold.2 (0x1c030ccec, line 445, "NULL !=
// dateComp") and .cold.1 (0x1c030ccc4, line 446, "NULL != inCalendar"). External.
// A set nanosecond above 999,999,999 (unsigned, so negative values too) is invalid. The calendar is copied
// (_CFCalendarCreateCopy with the system default allocator). When the nanosecond is set and nonzero it is
// cleared around CFCalendarCreateDateFromComponents and restored; otherwise the date is made directly. No
// date answers true. Otherwise the date's components for the units 0x17EFE are compared with every set field
// except the week (0x68) and nanosecond (0xA0) fields, in the native order (0x20 through 0x90, then 0xA8 and
// finally 0x98); any difference answers false. The date, the date's components and the calendar copy are
// released.
Boolean OCFDateComponentsIsValidDateInCalendar(OCFDateComponentsRef dateComp, OCFCalendarRef inCalendar) {
    if (dateComp == NULL) {
        __assert_rtn(
            OF_NAME_STRING(OCFDateComponentsIsValidDateInCalendar),
            "CFDateComponents.c",
            445,
            "NULL != dateComp"
        );
    }
    if (inCalendar == NULL) {
        __assert_rtn(
            OF_NAME_STRING(OCFDateComponentsIsValidDateInCalendar),
            "CFDateComponents.c",
            446,
            "NULL != inCalendar"
        );
    }
    OCFIndex nanosecond = dateComp->nanosecond;
    if (nanosecond != kUndefinedComponent && (uint64_t)nanosecond > 999999999) {
        return false;
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFCalendarRef calendar = _CFCalendarCreateCopy(allocator, inCalendar);
    OCFDateRef date;
    if ((uint64_t)nanosecond - 1 >= 0x7ffffffffffffffe) {
        date = OCFCalendarCreateDateFromComponents(allocator, calendar, dateComp);
    } else {
        dateComp->nanosecond = 0;
        date = OCFCalendarCreateDateFromComponents(allocator, calendar, dateComp);
        dateComp->nanosecond = nanosecond;
    }
    if (date == NULL) {
        OCFRelease(calendar);
        return true;
    }
    OCFDateComponentsRef dateComponents =
        OCFCalendarCreateDateComponentsFromDate(allocator, calendar, 0x17EFE, date);
    Boolean result;
    if ((dateComp->era != kUndefinedComponent && dateComponents->era != dateComp->era) ||
        (dateComp->year != kUndefinedComponent && dateComponents->year != dateComp->year) ||
        (dateComp->month != kUndefinedComponent && dateComponents->month != dateComp->month) ||
        (dateComp->leapMonth != kUndefinedComponent && dateComponents->leapMonth != dateComp->leapMonth) ||
        (dateComp->repeatedDay != kUndefinedComponent &&
         dateComponents->repeatedDay != dateComp->repeatedDay) ||
        (dateComp->day != kUndefinedComponent && dateComponents->day != dateComp->day) ||
        (dateComp->hour != kUndefinedComponent && dateComponents->hour != dateComp->hour) ||
        (dateComp->minute != kUndefinedComponent && dateComponents->minute != dateComp->minute) ||
        (dateComp->second != kUndefinedComponent && dateComponents->second != dateComp->second) ||
        (dateComp->weekday != kUndefinedComponent && dateComponents->weekday != dateComp->weekday) ||
        (dateComp->weekdayOrdinal != kUndefinedComponent &&
         dateComponents->weekdayOrdinal != dateComp->weekdayOrdinal) ||
        (dateComp->quarter != kUndefinedComponent && dateComponents->quarter != dateComp->quarter) ||
        (dateComp->weekOfMonth != kUndefinedComponent &&
         dateComponents->weekOfMonth != dateComp->weekOfMonth) ||
        (dateComp->weekOfYear != kUndefinedComponent && dateComponents->weekOfYear != dateComp->weekOfYear) ||
        (dateComp->dayOfYear != kUndefinedComponent && dateComponents->dayOfYear != dateComp->dayOfYear)) {
        result = false;
    } else if (dateComp->yearForWeekOfYear == kUndefinedComponent) {
        result = true;
    } else {
        result = dateComponents->yearForWeekOfYear == dateComp->yearForWeekOfYear;
    }
    OCFRelease(date);
    OCFRelease(dateComponents);
    OCFRelease(calendar);
    return result;
}
