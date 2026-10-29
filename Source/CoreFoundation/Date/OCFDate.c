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
