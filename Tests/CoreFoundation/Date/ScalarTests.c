//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "ScalarTests.h"

#include "CoreFoundation/Date/GregorianCases.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// Raw native-value fixture, independently constructed after the native ABI reading.
// CFGregorianDateIsValid: iOS Simulator 24A434, 0x1c018dca8,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1807427bc, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; no on-device execution is claimed.
typedef struct {
    int32_t year;
    int8_t month;
    int8_t day;
    int8_t hour;
    int8_t minute;
    double second;
} ReferenceGregorianDate;

typedef uint8_t (*NativeValidation)(ReferenceGregorianDate date, unsigned long flags);

typedef uint8_t (*ReplacementValidation)(OCFGregorianDate date, OCFOptionFlags flags);

_Static_assert(sizeof(ReferenceGregorianDate) == 16, "Native Gregorian ABI size must be sixteen bytes.");
_Static_assert(
    offsetof(ReferenceGregorianDate, second) == 8, "Native Gregorian second ABI offset must be eight."
);

static int compareGregorian(
    NativeValidation native,
    ReplacementValidation replacement,
    OCFGregorianDate input,
    OCFOptionFlags flags,
    int existingExceptions,
    size_t *count
) {
    uint64_t secondBits;
    memcpy(&secondBits, &input.second, sizeof(secondBits));
    ReferenceGregorianDate nativeDate = {input.year, input.month, input.day, input.hour, input.minute, 0.0};
    memcpy(&nativeDate.second, &secondBits, sizeof(secondBits));
    OCFGregorianDate replacementDate = {input.year, input.month, input.day, input.hour, input.minute, 0.0};
    memcpy(&replacementDate.second, &secondBits, sizeof(secondBits));
    int initialRounding = fegetround();
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        (existingExceptions != 0 && feraiseexcept(existingExceptions) != 0)) {
        return 77;
    }
    uint8_t expected = native(nativeDate, flags);
    int nativeExceptions = fetestexcept(FE_ALL_EXCEPT);
    int nativeRounding = fegetround();
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        (existingExceptions != 0 && feraiseexcept(existingExceptions) != 0)) {
        return 77;
    }
    uint8_t actual = replacement(replacementDate, flags);
    int replacementExceptions = fetestexcept(FE_ALL_EXCEPT);
    int replacementRounding = fegetround();
    if (expected != actual || nativeExceptions != replacementExceptions ||
        nativeRounding != initialRounding || replacementRounding != initialRounding) {
        fprintf(
            stderr,
            "Gregorian differential mismatch at case %zu: year %d, fields %d/%d/%d/%d, "
            "second 0x%016" PRIx64 ", flags 0x%016" PRIx64 ", result %u/%u, "
            "exceptions %x/%x, rounding %x/%x/%x.\n",
            *count,
            input.year,
            input.month,
            input.day,
            input.hour,
            input.minute,
            secondBits,
            (uint64_t)flags,
            expected,
            actual,
            nativeExceptions,
            replacementExceptions,
            initialRounding,
            nativeRounding,
            replacementRounding
        );
        return 1;
    }
    ++*count;
    return 0;
}

static uint64_t nextScalarBits(uint64_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

static int compareGregorianCorpus(NativeValidation native, ReplacementValidation replacement, size_t *count) {
    for (size_t index = 0; index < sizeof(kGregorianCases) / sizeof(kGregorianCases[0]); ++index) {
        const GregorianCase *test = &kGregorianCases[index];
        OCFGregorianDate date = test->date;
        memcpy(&date.second, &test->secondBits, sizeof(date.second));
        int status = compareGregorian(native, replacement, date, test->flags, 0, count);
        if (status != 0) {
            return status;
        }
    }
    for (unsigned long flags = 0; flags < 64; ++flags) {
        for (unsigned field = 0; field < 4; ++field) {
            for (unsigned byte = 0; byte < 256; ++byte) {
                OCFGregorianDate date = {2000, 2, 29, 12, 30, 59.0};
                uint8_t *fields[] = {
                    (uint8_t *)&date.month,
                    (uint8_t *)&date.day,
                    (uint8_t *)&date.hour,
                    (uint8_t *)&date.minute
                };
                *fields[field] = (uint8_t)byte;
                for (unsigned high = 0; high < 2; ++high) {
                    OCFOptionFlags selected = flags;
                    if (high != 0) {
                        selected |= UINT64_C(0x8000000100000000);
                    }
                    int status = compareGregorian(native, replacement, date, selected, 0, count);
                    if (status != 0) {
                        return status;
                    }
                }
            }
        }
    }
    for (int year = 1600; year < 2000; ++year) {
        for (int month = 1; month <= 12; ++month) {
            for (int day = 28; day <= 31; ++day) {
                OCFGregorianDate date = {year, (int8_t)month, (int8_t)day, 0, 0, 0.0};
                int status = compareGregorian(native, replacement, date, 63, 0, count);
                if (status != 0) {
                    return status;
                }
            }
        }
    }
    const int32_t boundaryYears[] = {
        INT32_MIN, -1, 0, 1, 4, 100, 400, 1600, 1900, 2000, 2100, 2400, INT32_MAX
    };
    for (size_t year = 0; year < sizeof(boundaryYears) / sizeof(boundaryYears[0]); ++year) {
        for (int month = 1; month <= 12; ++month) {
            for (int day = 28; day <= 31; ++day) {
                OCFGregorianDate date = {boundaryYears[year], (int8_t)month, (int8_t)day, 0, 0, 0.0};
                int status = compareGregorian(native, replacement, date, 63, 0, count);
                if (status != 0) {
                    return status;
                }
            }
        }
    }
    uint64_t state = UINT64_C(0x618941d56392af07);
    for (unsigned index = 0; index < 4096; ++index) {
        uint64_t words[] = {nextScalarBits(&state), nextScalarBits(&state)};
        OCFGregorianDate date;
        memcpy(&date, words, sizeof(date));
        OCFOptionFlags flags = nextScalarBits(&state);
        int status = compareGregorian(native, replacement, date, flags, 0, count);
        if (status != 0) {
            return status;
        }
    }
    const int roundingModes[] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    const uint64_t seconds[] = {
        UINT64_C(0),
        UINT64_C(0x8000000000000000),
        UINT64_C(1),
        UINT64_C(0x8000000000000001),
        UINT64_C(0x404dffffffffffff),
        UINT64_C(0x404e000000000000),
        UINT64_C(0x7ff0000000000000),
        UINT64_C(0xfff0000000000000),
        UINT64_C(0x7ff8000000000001),
        UINT64_C(0xfff8000000000001),
        UINT64_C(0x7ff0000000000001),
        UINT64_C(0xfff0000000000001)
    };
    for (size_t mode = 0; mode < sizeof(roundingModes) / sizeof(roundingModes[0]); ++mode) {
        if (fesetround(roundingModes[mode]) != 0) {
            return 77;
        }
        for (size_t second = 0; second < sizeof(seconds) / sizeof(seconds[0]); ++second) {
            for (unsigned existing = 0; existing < 2; ++existing) {
                OCFGregorianDate date = {2000, 2, 29, 0, 0, 0.0};
                memcpy(&date.second, &seconds[second], sizeof(date.second));
                int status =
                    compareGregorian(native, replacement, date, 63, existing != 0 ? FE_DIVBYZERO : 0, count);
                if (status != 0) {
                    return status;
                }
            }
        }
    }
    return 0;
}

int runCoreFoundationScalarDifferentialTests(void) {
    void *nativeAddress = NULL;
    void *replacementAddress = NULL;
    int status = resolveReferenceFunction(
        "CoreFoundation", OF_REFERENCE_CF_UUID, "CFGregorianDateIsValid", &nativeAddress
    );
    if (status != 0) {
        return status;
    }
    status = resolveReplacementFunction(
        "OpenCoreFoundation", OF_NAME_STRING(OCFGregorianDateIsValid), &replacementAddress
    );
    if (status != 0) {
        return status;
    }
    if (nativeAddress == replacementAddress) {
        fprintf(stderr, "Setup failure: Gregorian oracle and replacement are the same function.\n");
        return 77;
    }
    fenv_t environment;
    if (fegetenv(&environment) != 0 || fesetenv(FE_DFL_ENV) != 0) {
        fprintf(stderr, "Setup failure: cannot establish the scalar floating environment.\n");
        return 77;
    }
    size_t count = 0;
    status = compareGregorianCorpus(
        (NativeValidation)nativeAddress, (ReplacementValidation)replacementAddress, &count
    );
    if (fesetenv(&environment) != 0) {
        return 77;
    }
    if (status == 0) {
        printf("CoreFoundation scalar differential cases passed: %zu.\n", count);
    }
    return status;
}
