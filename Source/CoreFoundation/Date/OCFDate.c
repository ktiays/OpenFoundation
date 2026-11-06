//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include <OpenCoreFoundation/Date/OCFDate.h>

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Date/OCFDatePrivate.h"
#include "CoreFoundation/Date/OCFTimeZone.h"
#include "CoreFoundation/Notification/OCFNotificationCenter.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <dispatch/dispatch.h>
#include <mach/mach_time.h>
#include <math.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <time.h>

static void __CFYMDFromAbsolute(int64_t absolute, int64_t *year, uint8_t *month, uint8_t *day);

// Native daysInMonth, iOS Simulator 24A434, 0x1c0373524,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18082f754, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; the sixteen bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationScalar.md.
static const uint8_t daysInMonth[16] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0, 0, 0};

// Native daysBeforeMonth, iOS Simulator 24A434, 0x1c0373534,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for the sixteen entries
// __CFAbsoluteFromYMD can index (its month bound is 15). The next symbol is 200 bytes later; the bytes
// after the sixteenth entry are not read by this function.
// iOS Device 24A446: 0x18082f764, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the
// thirty-two bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
static const uint16_t daysBeforeMonth[16] = {
    0xffff, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365, 0xffff, 0xffff
};

// Native CFGregorianDateIsValid, iOS Simulator 24A434, 0x1c018dca8,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1807427bc, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; reachable scalar behavior agrees.
// iOS Device divergence: CFGregorianDateIsValid at 0x1807427bc adds pointer authentication
// and outlines unreachable logging differently; confidence: verified. No raw-value behavior differs.
// Reading: Documentation/Reference/Research/CoreFoundationScalar.md.
uint8_t OCFGregorianDateIsValid(OCFGregorianDate gdate, OCFOptionFlags unitFlags) {
    if ((unitFlags & 1) != 0 && gdate.year < 1) {
        return 0;
    }
    if ((unitFlags & 2) != 0 && (uint8_t)(gdate.month - 1) > 11) {
        return 0;
    }
    if ((unitFlags & 4) != 0 && (uint8_t)(gdate.day - 1) > 30) {
        return 0;
    }
    if ((unitFlags & 8) != 0 && (uint8_t)gdate.hour > 23) {
        return 0;
    }
    if ((unitFlags & 16) != 0 && (uint8_t)gdate.minute > 59) {
        return 0;
    }
    if ((unitFlags & 32) != 0) {
        if (gdate.second < 0.0) {
            return 0;
        }
        if (gdate.second >= 60.0) {
            return 0;
        }
    }
    if ((unitFlags & 7) != 7) {
        return 1;
    }

    // All three low flags imply the already checked year, month and day bounds.
    // The native month logger and bounds trap are therefore unreachable here.
    int64_t year = (int64_t)(gdate.year - 2001) + 1;
    int64_t remainder = year % 400;
    if (remainder < 0) {
        remainder = -remainder;
    }
    uint8_t leap = 0;
    if ((remainder & 3) == 0 && remainder != 100 && remainder != 200 && remainder != 300) {
        leap = 1;
    }
    uint8_t month = (uint8_t)gdate.month;
    uint8_t lastDay = daysInMonth[month & 15];
    if (month != 2) {
        leap = 0;
    }
    lastDay = (uint8_t)(lastDay + leap);
    return lastDay >= gdate.day;
}

// Native CFGregorianDateGetAbsoluteTime, iOS Simulator 24A434, 0x1c018de3c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External. iOS Device 24A446: unread.
// __CFAbsoluteFromYMD receives year - 2001 (32-bit subtraction, sign-extended) and the 8-bit month and day.
// The time is days * 86400.0 + (fma(hour, 3600.0, minute * 60.0) + second), in this order (FMADD for the hour
// term). With a time zone, the offset at that time is subtracted and the offset at the corrected time is
// subtracted from the original time (two CFTimeZoneGetSecondsFromGMT calls).
// Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json.
OCFAbsoluteTime OCFGregorianDateGetAbsoluteTime(OCFGregorianDate gdate, OCFTimeZoneRef tz) {
    double days =
        __CFAbsoluteFromYMD((int64_t)(int32_t)((uint32_t)gdate.year - 2001), gdate.month, gdate.day);
    double at = days * 86400.0;
    double clock = fma((double)gdate.hour, 3600.0, (double)gdate.minute * 60.0);
    at = clock + gdate.second + at;
    if (tz != NULL) {
        OCFTimeInterval offset = OCFTimeZoneGetSecondsFromGMT(tz, at);
        at = at - OCFTimeZoneGetSecondsFromGMT(tz, at - offset);
    }
    return at;
}

// Native __CFAbsoluteFromYMD, iOS Simulator 24A434, 0x1c018dee0,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external; its callers are
// CFGregorianDateGetAbsoluteTime (year - 2001, 8-bit month and day) and CFAbsoluteTimeGetWeekOfYear
// (month 1, day 1).
// iOS Device 24A446: 0x1806d56e4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// computes the month leap flag with the tests in another order; the operations and results agree.
// year is relative to 2001. The day count starts as fma(year / 400, 146097.0, +0.0) (a fused FMADD with
// a zero addend; year / 400 truncates). For a positive remainder r = year % 400, 365.0 or 366.0 is
// added for each of the years 1..r, and for a negative remainder subtracted for each of the years
// -r - 1 down to 0 (wrapping unsigned arithmetic, reduced modulo 400); a year y is a leap year when
// y % 400 is a multiple of 4 other than 100, 200 and 300. When month, as unsigned 32 bits, is at most 15,
// daysBeforeMonth[month] plus the leap flag of (|(r + 1) % 400|), applied only after February, is
// added unless its low 16 bits are 0xffff (month 0 never adds; month 14 or 15 adds 0.0 in a leap year).
// Last, day - 1 is added. Additions are in this order, in double precision.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
double __CFAbsoluteFromYMD(int64_t year, int8_t month, int8_t day) {
    int64_t cycles = year / 400;
    double absolute = __builtin_fma((double)cycles, 146097.0, 0.0);
    int64_t remainder = year - cycles * 400;
    if (remainder < 0) {
        uint64_t cursor = (uint64_t)cycles * 400 + ~(uint64_t)year;
        for (int64_t count = remainder; count != 0; ++count) {
            uint64_t position = cursor % 400;
            double days = 365.0;
            if ((position & 3) == 0 && position != 100 && position != 300 && position != 200) {
                days = 366.0;
            }
            absolute = absolute - days;
            --cursor;
        }
    } else if (remainder != 0) {
        uint64_t cursor = 1;
        for (int64_t count = remainder; count != 0; --count) {
            uint64_t position = cursor % 400;
            double days = 365.0;
            if ((position & 3) == 0 && position != 100 && position != 300 && position != 200) {
                days = 366.0;
            }
            absolute = absolute + days;
            ++cursor;
        }
    }
    int64_t following = (remainder + 1) % 400;
    if (following < 0) {
        following = -following;
    }
    uint16_t leap = 1;
    if (following == 300 || following == 200 || following == 100 || (following & 3) != 0) {
        leap = 0;
    }
    if ((uint32_t)(int32_t)month <= 15) {
        if ((uint32_t)(int32_t)month <= 2) {
            leap = 0;
        }
        uint16_t days = (uint16_t)(daysBeforeMonth[month] + leap);
        if (days != 0xffff) {
            absolute = absolute + (double)days;
        }
    }
    return absolute + (double)(day - 1);
}

// Native CFAbsoluteTimeGetGregorianDate, iOS Simulator 24A434, 0x1c018e0a8,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External. iOS Device 24A446: unread.
// The time plus the zone's offset (0.0 without a zone) gives the day floor(fixed / 86400.0) (FCVTMS) for
// __CFYMDFromAbsolute; the year is min(year, 0x7FFFF82E) + 2001 in 32 bits. The second is
// fma(-60.0, floor(fixed / 60.0), fixed), plus 60.0 when negative, and +0.0 when zero. The minute and hour
// lanes take m = floor(fixed / 60.0) and h = floor(fixed / 3600.0), reduce them as fma(-d, floor(v / d), v)
// with d = 60.0 and 24.0, floor the result, narrow it to float and truncate it to 32 bits; a negative value
// gains 60 or 24, and the low byte is stored (vector lanes natively, the same operations per lane).
// Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json.
OCFGregorianDate OCFAbsoluteTimeGetGregorianDate(OCFAbsoluteTime at, OCFTimeZoneRef tz) {
    OCFTimeInterval offset = 0.0;
    if (tz != NULL) {
        offset = OCFTimeZoneGetSecondsFromGMT(tz, at);
    }
    int64_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    double fixed = at + offset;
    __CFYMDFromAbsolute((int64_t)floor(fixed / 86400.0), &year, &month, &day);
    int64_t clamped = year < 0x7FFFF82E ? year : 0x7FFFF82E;
    double minutes = floor(fixed / 60.0);
    double hours = floor(fixed / 3600.0);
    double second = fma(-60.0, minutes, fixed);
    if (second < 0.0) {
        second = second + 60.0;
    }
    if (second == 0.0) {
        second = 0.0;
    }
    int32_t minute = (int32_t)(float)floor(fma(-60.0, floor(minutes / 60.0), minutes));
    int32_t hour = (int32_t)(float)floor(fma(-24.0, floor(hours / 24.0), hours));
    if (minute < 0) {
        minute = minute + 60;
    }
    if (hour < 0) {
        hour = hour + 24;
    }
    OCFGregorianDate result;
    result.year = (int32_t)((uint32_t)clamped + 2001);
    result.month = (int8_t)month;
    result.day = (int8_t)day;
    result.hour = (int8_t)(uint8_t)hour;
    result.minute = (int8_t)(uint8_t)minute;
    result.second = second;
    return result;
}

// Native __CFYMDFromAbsolute, iOS Simulator 24A434, 0x1c018e208,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external; its one caller in this
// restoration is CFAbsoluteTimeGetGregorianDate. iOS Device 24A446: unread.
// absolute is a day count relative to 2001-01-01. The year starts at 400 * (absolute / 146097) (truncating);
// a negative remainder gains the length of the current year (whose leap test uses the year before the
// decrement) and the year decreases until it is not negative. The year advances while the remainder (compared
// unsigned the first time, signed afterwards) reaches the length of year + 1; a year y is a leap year when
// |y % 400| is a multiple of 4 other than 100, 200 and 300. The month search starts at remainder / 33 + 1
// (unsigned); while that month (8 bits) is at most 11 and the remainder reaches daysBeforeMonth[month + 1]
// (plus the leap day after February), the month advances; a search that passes month 11 stores 12. A first
// month above 15 subtracts 0xFFFF for the day; otherwise the day is the remainder minus
// daysBeforeMonth[month] (plus the leap day after February), plus one, stored as 8 bits. Reading: not yet
// written; the evidence is the assembly in Local/Evidence/cf-core/iphone-simulator-core-corpus.json.
static void __CFYMDFromAbsolute(int64_t absolute, int64_t *year, uint8_t *month, uint8_t *day) {
    int64_t cycles = absolute / 146097;
    int64_t y = cycles * 400;
    int64_t remainder = absolute - cycles * 146097;
    while (remainder < 0) {
        int64_t position = y % 400;
        if (position < 0) {
            position = -position;
        }
        int64_t length = 365;
        if ((position & 3) == 0 && position != 100 && position != 300 && position != 200) {
            length = 366;
        }
        remainder = remainder + length;
        y = y - 1;
    }
    int64_t following = (y + 1) % 400;
    if (following < 0) {
        following = -following;
    }
    int64_t length = 365;
    if ((following & 3) == 0 && following != 100 && following != 300 && following != 200) {
        length = 366;
    }
    if ((uint64_t)remainder >= (uint64_t)length) {
        do {
            following = (y + 2) % 400;
            if (following < 0) {
                following = -following;
            }
            int64_t next = 365;
            if ((following & 3) == 0 && following != 100 && following != 300 && following != 200) {
                next = 366;
            }
            y = y + 1;
            remainder = remainder - length;
            length = next;
        } while (remainder >= length);
    }
    *year = y;
    uint16_t leap = 1;
    if (following == 300 || following == 200 || following == 100 || (following & 3) != 0) {
        leap = 0;
    }
    uint32_t candidate = (uint32_t)((uint64_t)remainder / 33) + 1;
    uint32_t first = candidate & 0xff;
    uint32_t found = first > 12 ? first : 12;
    if (first <= 11) {
        uint32_t current = candidate;
        for (;;) {
            uint32_t next = (current + 1) & 0xff;
            uint16_t limit = (uint16_t)(daysBeforeMonth[next] + (next <= 2 ? 0 : leap));
            if (remainder < (int64_t)limit) {
                *month = (uint8_t)current;
                found = current;
                goto compute;
            }
            if ((current & 0xff) > 10) {
                break;
            }
            current = current + 1;
        }
    }
    *month = (uint8_t)found;
    if (first > 15) {
        *day = (uint8_t)(remainder - 0xffff + 1);
        return;
    }
compute:;
    uint32_t index = found & 0xff;
    uint16_t before = (uint16_t)(daysBeforeMonth[index] + (index <= 2 ? 0 : leap));
    *day = (uint8_t)(remainder - before + 1);
}

// Native kCFAbsoluteTimeIntervalSince1970, iOS Simulator 24A434, 0x1c046c310 (__TEXT,__const, 978307200.0),
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// iOS Device 24A446: unread.
const OCFTimeInterval kOCFAbsoluteTimeIntervalSince1970 = 978307200.0;

// The tagged NSDate class (__NSTaggedDate, an Objective-C class of this image, ONS spelling).
extern char taggedDateClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSTaggedDate));

// libobjc interfaces (re-exported libobjc.A.dylib); supplied. objc_debug_taggedpointer_mask is read through
// the GOT slot 0x222409418, bound by use as in Number/OCFNumber.c (confidence: medium).
extern uintptr_t objc_debug_taggedpointer_mask;
extern void _objc_registerTaggedPointerClass(uint16_t tag, Class cls);
extern Class objc_opt_class(id object);

// Native __NSDateTaggedPointersDisabled, 0x22e6d3730 (__DATA,__bss). Non-external; hidden here because the
// NSDate classes that read it are restored elsewhere.
uint8_t __NSDateTaggedPointersDisabled;

// Native ___33+[__NSPlaceholderDate initialize]_block_invoke, 0x1c02801d4, with its .cold.1 (0x1c030f128, a
// branch to OUTLINED_FUNCTION_0_38, 0x1c0281934: dispatch_once(&CFDateGetTypeID.initOnce,
// ___block_literal_global.100)). Non-external; the global block whose literal belongs to
// +[__NSPlaceholderDate initialize], which is restored elsewhere (the literal's address is not in the
// evidence; the name here is a local descriptive spelling). The block first makes sure of CFDateGetTypeID's
// once (the inlined dispatch_once of the TSR rates); executables linked on or after system version 7 with
// tagged pointers enabled (objc_debug_taggedpointer_mask nonzero) register objc_opt_class(__NSTaggedDate) for
// tag 6 (a tail call), otherwise tagged dates are disabled.
PlaceholderDateInitializeBlock const placeholderDateInitializeBlock = ^{
  dispatch_once(&dateTypeIDInitOnce, dateTypeIDInitBlock);
  if (_OCFExecutableLinkedOnOrAfter(7) && objc_debug_taggedpointer_mask != 0) {
      _objc_registerTaggedPointerClass(6, objc_opt_class((id)taggedDateClass));
      return;
  }
  __NSDateTaggedPointersDisabled = 1;
};

// Native CFAbsoluteTimeGetCurrent, iOS Simulator 24A434, 0x1c0280450, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External. iOS Device 24A446: unread.
// clock_gettime(CLOCK_REALTIME) with the timespec zeroed first; the result is one fused multiply-add,
// nanoseconds * 1e-9 + (seconds + -978307200.0) (FMADD; the constants at 0x1c0373650 and 0x1c046c300).
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
OCFAbsoluteTime OCFAbsoluteTimeGetCurrent(void) {
    struct timespec now = {0, 0};
    clock_gettime(CLOCK_REALTIME, &now);
    return fma((double)now.tv_nsec, 1e-9, (double)now.tv_sec + -978307200.0);
}

// Native NSSystemClockDidChangeNotification, 0x2227a82f0 (__DATA_CONST,__const). External. A constant string
// of its own name (0x2227b8540), the name __systemClockChangedNotification posts. Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3).
static const OCFConstantString systemClockDidChangeString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSSystemClockDidChangeNotification),
    sizeof(OF_NAME_STRING(ONSSystemClockDidChangeNotification)) - 1
};
const OCFStringRef ONSSystemClockDidChangeNotification = (OCFStringRef)&systemClockDidChangeString;

// Native __systemClockChangedNotification, 0x1c0280564, iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external; the callback of the system
// clock dependent notification below (the body reads no argument, so the parameters are declared void). iOS
// Device 24A446: unread. Posts "NSSystemClockDidChangeNotification" (a constant string of this image) to the
// local center with no object or user info, delivered immediately (a tail call). Reading: not yet written;
// the evidence is the assembly in Local/Evidence/cf-core/iphone-simulator-core-corpus.json.
void __systemClockChangedNotification(void) {
    OCFNotificationCenterPostNotification(
        OCFNotificationCenterGetLocalCenter(), ONSSystemClockDidChangeNotification, NULL, NULL, true
    );
}

// Native __CFTimeIntervalToTSR, 0x1c028166c, with .cold.1 (0x1c030f180, the dispatch_once of
// CFDateGetTypeID's TSR rates through OUTLINED_FUNCTION_0_38). Hidden (was a private external). iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device 24A446:
// unread. ti * __CFTSRRate, converted unsigned (FCVTZU, saturating; negative values give 0), or
// 0x3FFFFFFFFFFFFFFF when the product is above 2^62.
// Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json.
uint64_t __CFTimeIntervalToTSR(OCFTimeInterval ti) {
    dispatch_once(&dateTypeIDInitOnce, dateTypeIDInitBlock);
    double tsr = ti * __CFTSRRate;
    uint64_t converted = (uint64_t)tsr;
    return tsr > 4611686018427387904.0 ? 0x3FFFFFFFFFFFFFFF : converted;
}

// Native __CFTSRToTimeInterval, 0x1c02816c4, with .cold.1 (0x1c030f184, the same dispatch_once). Hidden (was
// a private external). __CF1_TSRRate * tsr (unsigned conversion).
OCFTimeInterval __CFTSRToTimeInterval(uint64_t tsr) {
    dispatch_once(&dateTypeIDInitOnce, dateTypeIDInitBlock);
    return __CF1_TSRRate * (double)tsr;
}

// Native __CFTimeIntervalUntilTSR, 0x1c0281708, with .cold.1-3 (0x1c030f188-0x1c030f190, the same
// dispatch_once before mach_absolute_time and again on each branch). Hidden (was a private external). A
// deadline at or after now gives __CF1_TSRRate * (tsr - now); an earlier one -((now - tsr) * __CF1_TSRRate)
// (FNMUL).
// Folds the compiler-outlined piece ___CFTimeIntervalUntilTSR.cold.2 (0x1c030f18c).
OCFTimeInterval __CFTimeIntervalUntilTSR(uint64_t tsr) {
    dispatch_once(&dateTypeIDInitOnce, dateTypeIDInitBlock);
    uint64_t now = mach_absolute_time();
    if (tsr < now) {
        dispatch_once(&dateTypeIDInitOnce, dateTypeIDInitBlock);
        return -((double)(now - tsr) * __CF1_TSRRate);
    }
    dispatch_once(&dateTypeIDInitOnce, dateTypeIDInitBlock);
    return __CF1_TSRRate * (double)(tsr - now);
}

// Native __CFTSRToNanoseconds, 0x1c0281794. Hidden (was a private external); no once check.
// floor(__CF1_TSRRate
// * tsr * 1e9) as an unsigned integer (FCVTMU).
uint64_t __CFTSRToNanoseconds(uint64_t tsr) {
    return (uint64_t)floor(__CF1_TSRRate * (double)tsr * 1000000000.0);
}

// Native __CFTSRToDispatchTime, 0x1c02817b8. Hidden (was a private external); no once check. The nanoseconds
// as in __CFTSRToNanoseconds, limited below 0x7FFFFFFFFFFFFFFE (unsigned), are the delta of dispatch_time
// with the base time 1 (MOV W0, #1), a tail call.
uint64_t __CFTSRToDispatchTime(uint64_t tsr) {
    uint64_t ns = (uint64_t)floor(__CF1_TSRRate * (double)tsr * 1000000000.0);
    return dispatch_time(1, (int64_t)(ns < 0x7FFFFFFFFFFFFFFE ? ns : 0x7FFFFFFFFFFFFFFE));
}

// Inferred organization: the CFDate object functions (0x1c0281828-0x1c0281923) are contiguous in the iOS
// Simulator __text section, after the TSR conversions (0x1c028166c-0x1c02817f8); the native source path is
// not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread. Reading: not yet
// written; the evidence is the assembly in Local/Evidence/cf-core/iphone-simulator-core-corpus.json.

// libobjc's objc_alloc (no SDK declaration); supplied.
extern id objc_alloc(Class cls);

// The class CFDateCreate allocates (NSDate, an Objective-C class of this image).
extern char bridgedDateClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDate));

// Native __CFDateClass, 0x2227a8330 (__DATA_CONST,__const). Hidden (was private external); version 0, name
// "CFDate", the debug description only.
const OCFRuntimeClass __CFDateClass = {
    0,
    "CFDate",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    __CFDateCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CFTSRRate and __CF1_TSRRate, 0x22e6d3760 and 0x22e6d3768 (__DATA,__bss). Non-external natively;
// hidden here for the TSR conversions.
double __CFTSRRate;
double __CF1_TSRRate;

// Native CFDateGetTypeID.initOnce, 0x22e6d3770 (__DATA,__bss), and ___block_literal_global.100, 0x2227a8390
// (__DATA_CONST,__const; flags 0x50000000), the global block whose invoke is ___CFDateGetTypeID_block_invoke
// (0x1c0281870). Non-external. The block reads mach_timebase_info into a zeroed record and stores
// 1e9 / numer * denom (in that order) and its reciprocal.
intptr_t dateTypeIDInitOnce;
void (^const dateTypeIDInitBlock)(void) = ^{
  mach_timebase_info_data_t info = {0, 0};
  mach_timebase_info(&info);
  double rate = 1000000000.0 / (double)info.numer * (double)info.denom;
  __CFTSRRate = rate;
  __CF1_TSRRate = 1.0 / rate;
};

// Native __CFDateCopyDescription, 0x1c0281828. Hidden here (non-external natively; the runtime describes
// tagged dates with it). -description inside an autorelease pool, retained when not NULL.
OCFStringRef __CFDateCopyDescription(OCFTypeRef cf) {
    void *pool = _OCFAutoreleasePoolPush();
    OCFStringRef description = ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)cf, sel_getUid("description"));
    if (description != NULL) {
        OCFRetain(description);
    }
    _OCFAutoreleasePoolPop(pool);
    return description;
}

// Native CFDateCreate, 0x1c02818f4. External. An NSDate (objc_alloc, then
// -initWithTimeIntervalSinceReferenceDate:); the allocator is not used.
OCFDateRef OCFDateCreate(OCFAllocatorRef allocator, OCFAbsoluteTime at) {
    (void)allocator;
    id date = objc_alloc((Class)bridgedDateClass);
    return ((OCFDateRef (*)(id, SEL, double))objc_msgSend)(
        date, sel_getUid("initWithTimeIntervalSinceReferenceDate:"), at
    );
}

// Native CFDateGetAbsoluteTime, 0x1c0281920. External; a tail call of -timeIntervalSinceReferenceDate.
OCFAbsoluteTime OCFDateGetAbsoluteTime(OCFDateRef theDate) {
    return ((double (*)(id, SEL))objc_msgSend)((id)theDate, sel_getUid("timeIntervalSinceReferenceDate"));
}

// The notify(3) name 0x2227b85c0 (coalesced natively with the calendar record's).
static const OCFConstantString systemClockSetString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "com.apple.system.clock_set",
    sizeof("com.apple.system.clock_set") - 1
};

// Native __CFSystemClockDidChangeDependentNotification, 0x2227a82f8 (__DATA_CONST,__const). Its
// sub-notifications, 0x2227a83b0, and its once token, 0x22e6d3738
// (__DATA,__bss). Non-external (the record was a private external).
static dispatch_once_t __CFSystemClockDidChangeInitOnce;
static const DependentSubnotification __CFSystemClockDidChangeSubnotes[1] = {
    {(OCFStringRef)&systemClockSetString, 3, {0}},
};
const DependentNotification __CFSystemClockDidChangeDependentNotification = {
    &ONSSystemClockDidChangeNotification,
    &__CFSystemClockDidChangeInitOnce,
    NULL,
    (OCFNotificationCallback)__systemClockChangedNotification,
    0,
    1,
    {0},
    __CFSystemClockDidChangeSubnotes
};
