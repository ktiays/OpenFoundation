//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "ScalarStandaloneTests.h"

#include "CoreFoundation/Date/GregorianCases.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(OCFGregorianDate) == 16, "Gregorian date ABI size must be sixteen bytes.");
_Static_assert(_Alignof(OCFGregorianDate) == 8, "Gregorian date ABI alignment must be eight bytes.");
_Static_assert(offsetof(OCFGregorianDate, year) == 0, "Gregorian year ABI offset must be zero.");
_Static_assert(offsetof(OCFGregorianDate, month) == 4, "Gregorian month ABI offset must be four.");
_Static_assert(offsetof(OCFGregorianDate, day) == 5, "Gregorian day ABI offset must be five.");
_Static_assert(offsetof(OCFGregorianDate, hour) == 6, "Gregorian hour ABI offset must be six.");
_Static_assert(offsetof(OCFGregorianDate, minute) == 7, "Gregorian minute ABI offset must be seven.");
_Static_assert(offsetof(OCFGregorianDate, second) == 8, "Gregorian second ABI offset must be eight.");
_Static_assert(sizeof(OCFOptionFlags) == 8, "Gregorian flags ABI width must be eight bytes.");

// Native CFGregorianDateIsValid and daysInMonth: iOS Simulator 24A434,
// 0x1c018dca8/0x1c0373524, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// iOS Device 24A446, 0x1807427bc/0x18082f754,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
int runCoreFoundationScalarStandaloneTests(void) {
    fenv_t environment;
    if (fegetenv(&environment) != 0 || fesetenv(FE_DFL_ENV) != 0) {
        fprintf(stderr, "Setup failure: cannot save the scalar test floating environment.\n");
        return 77;
    }
    size_t count = sizeof(kGregorianCases) / sizeof(kGregorianCases[0]);
    int status = 0;
    for (size_t index = 0; index < count; ++index) {
        const GregorianCase *test = &kGregorianCases[index];
        OCFGregorianDate date = test->date;
        memcpy(&date.second, &test->secondBits, sizeof(date.second));
        if (feclearexcept(FE_ALL_EXCEPT) != 0) {
            status = 77;
            break;
        }
        uint8_t actual = OCFGregorianDateIsValid(date, test->flags);
        int exceptions = fetestexcept(FE_ALL_EXCEPT);
        if (actual != test->expected || exceptions != test->expectedExceptions) {
            fprintf(
                stderr,
                "Gregorian standalone mismatch: %s, result %u/%u, exceptions %x/%x.\n",
                test->name,
                actual,
                test->expected,
                exceptions,
                test->expectedExceptions
            );
            status = 1;
            break;
        }
    }
    if (fesetenv(&environment) != 0) {
        return 77;
    }
    if (status == 0) {
        printf("CoreFoundation scalar standalone cases passed: %zu.\n", count);
    }
    return status;
}
