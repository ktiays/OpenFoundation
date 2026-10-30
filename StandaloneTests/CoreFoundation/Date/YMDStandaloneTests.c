//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Date/YMDStandaloneTests.h"
#include "CoreFoundation/Date/OCFDate.h"

#include <stdio.h>

static int check(int64_t year, int8_t month, int8_t day, double expected, int line) {
    double actual = __CFAbsoluteFromYMD(year, month, day);
    if (actual != expected) {
        fprintf(stderr, "__CFAbsoluteFromYMD standalone mismatch at source line %d: %.17g.\n", line, actual);
        return 1;
    }
    return 0;
}

// Native __CFAbsoluteFromYMD 0x1c018dee0, iOS Simulator 24A434 UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Device comparison: 0x1806d56e4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified. Years are relative to 2001; results are day counts from 2001-01-01.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationYMDStandaloneTests(void) {
    struct {
        int64_t year;
        int8_t month;
        int8_t day;
        double expected;
        int line;
    } cases[] = {
        {0, 1, 1, 0.0, __LINE__},
        // 2000 is a leap year.
        {-1, 1, 1, -366.0, __LINE__},
        // 2001-03-01, not a leap year.
        {0, 3, 1, 59.0, __LINE__},
        // 2004-03-01: three years of 365 days plus 31 + 29.
        {3, 3, 1, 1155.0, __LINE__},
        // 2004-02-29.
        {3, 2, 29, 1154.0, __LINE__},
        // One 400-year cycle.
        {400, 1, 1, 146097.0, __LINE__},
        {-400, 1, 1, -146097.0, __LINE__},
        // 2100 is not a leap year: 2100-03-01 follows 2100-02-28.
        {99, 3, 1, 36218.0, __LINE__},
        // Month 0 adds nothing; month 13 adds a full year; month 14 adds 0xffff + 1 = 0 in a leap year.
        {0, 0, 1, 0.0, __LINE__},
        {0, 13, 1, 365.0, __LINE__},
        {3, 14, 1, 1095.0, __LINE__},
        // Months above 15 or negative add nothing; the day is added as day - 1.
        {0, 16, 10, 9.0, __LINE__},
        {0, -1, -128, -129.0, __LINE__},
    };

    size_t count = sizeof(cases) / sizeof(cases[0]);
    for (size_t index = 0; index < count; ++index) {
        if (check(
                cases[index].year,
                cases[index].month,
                cases[index].day,
                cases[index].expected,
                cases[index].line
            ) != 0) {
            return 1;
        }
    }
    printf("__CFAbsoluteFromYMD standalone cases passed: %zu.\n", count);
    return 0;
}
