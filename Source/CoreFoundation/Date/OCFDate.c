//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Date/OCFDate.h"

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
