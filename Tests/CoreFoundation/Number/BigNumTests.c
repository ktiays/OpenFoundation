//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Number/BigNumTests.h"
#include "CoreFoundation/Number/OCFBigNumber.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <stdio.h>
#include <string.h>

typedef void (*InitInt8Function)(BigNum *, int8_t);

typedef void (*InitInt16Function)(BigNum *, int16_t);

typedef void (*InitInt32Function)(BigNum *, int32_t);

typedef void (*InitInt64Function)(BigNum *, int64_t);

typedef void (*InitInt128Function)(BigNum *, __int128);

typedef void (*ToCStringFunction)(const BigNum *, uint8_t, uint8_t, char *, size_t);

typedef void (*Emit128Function)(char *, const Int128Parts *, uint8_t);

typedef struct {
    InitInt8Function initInt8;
    InitInt16Function initInt16;
    InitInt32Function initInt32;
    InitInt64Function initInt64;
    InitInt128Function initInt128;
    ToCStringFunction toCString;
    Emit128Function emit128;
} BigNumFunctions;

enum {
    kFunctionCount = 7,
    kValueSlack = 8,
    kStringArea = 160,
    kStringOffset = 16,
};

static uint64_t nextRandom(uint64_t *state) {
    uint64_t value = (*state += UINT64_C(0x9e3779b97f4a7c15));
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

// A random 128-bit value whose magnitude has a random bit length, so every digit count occurs.
static __int128 randomInt128(uint64_t *state) {
    unsigned __int128 bits = ((unsigned __int128)nextRandom(state) << 64) | nextRandom(state);
    unsigned shift = (unsigned)(nextRandom(state) % 128);
    __int128 value = (__int128)(bits >> shift);
    return (nextRandom(state) & 1) ? -value : value;
}

// Value bytes are compared with a canary before and after the 24-byte value.
typedef struct {
    uint32_t words[kValueSlack + sizeof(BigNum) / sizeof(uint32_t) + kValueSlack];
} ValueArea;

static BigNum *valueIn(ValueArea *area) {
    memset(area, 0xa5, sizeof(*area));
    return (BigNum *)(void *)&area->words[kValueSlack];
}

static int compareValues(const ValueArea *expected, const ValueArea *actual, const char *name) {
    if (memcmp(expected, actual, sizeof(*expected)) != 0) {
        fprintf(stderr, "BigNum mismatch: %s.\n", name);
        return 1;
    }
    return 0;
}

static int compareStrings(
    const BigNumFunctions *native,
    const BigNumFunctions *replacement,
    const BigNum *value,
    uint8_t leadingZeros,
    uint8_t forcePlus,
    size_t bufferSize,
    size_t *cases
) {
    char expected[kStringArea];
    char actual[kStringArea];
    memset(expected, 0xcc, sizeof(expected));
    memset(actual, 0xcc, sizeof(actual));
    native->toCString(value, leadingZeros, forcePlus, expected + kStringOffset, bufferSize);
    replacement->toCString(value, leadingZeros, forcePlus, actual + kStringOffset, bufferSize);
    if (memcmp(expected, actual, sizeof(expected)) != 0) {
        const uint8_t *bytes = (const uint8_t *)value;
        fprintf(
            stderr,
            "BigNum C-string mismatch: size %zu, zeros %u, plus %u, value",
            bufferSize,
            leadingZeros,
            forcePlus
        );
        for (size_t index = 0; index < sizeof(BigNum); ++index) {
            fprintf(stderr, " %02x", bytes[index]);
        }
        fputs(".\n", stderr);
        return 1;
    }
    ++*cases;
    return 0;
}

// Buffer sizes cover zero, small and odd sizes, the 45-digit width with and without its NUL and
// prefix, and sizes beyond the 46-byte scratch string. Leading-zero mode with a prefix and size zero
// wraps the size and fills memory without bound in both implementations; it is never called.
static const size_t kBufferSizes[] = {0, 1, 2, 3, 5, 7, 9, 10, 11, 17, 19, 31, 44, 45, 46, 47, 48, 63, 96};

static int compareAllStrings(
    const BigNumFunctions *native, const BigNumFunctions *replacement, const BigNum *value, size_t *cases
) {
    for (size_t sizeIndex = 0; sizeIndex < sizeof(kBufferSizes) / sizeof(kBufferSizes[0]); ++sizeIndex) {
        for (uint8_t leadingZeros = 0; leadingZeros < 2; ++leadingZeros) {
            for (uint8_t forcePlus = 0; forcePlus < 2; ++forcePlus) {
                size_t size = kBufferSizes[sizeIndex];
                if (leadingZeros && size == 0 && (forcePlus || value->sign < 0)) {
                    continue;
                }
                int status = compareStrings(native, replacement, value, leadingZeros, forcePlus, size, cases);
                if (status != 0) {
                    return status;
                }
            }
        }
    }
    return 0;
}

static int compareRandomString(
    const BigNumFunctions *native,
    const BigNumFunctions *replacement,
    const BigNum *value,
    uint64_t *state,
    size_t *cases
) {
    uint64_t choice = nextRandom(state);
    uint8_t leadingZeros = choice & 1;
    uint8_t forcePlus = (choice >> 1) & 1;
    size_t size = kBufferSizes[(choice >> 8) % (sizeof(kBufferSizes) / sizeof(kBufferSizes[0]))];
    if (leadingZeros && size == 0 && (forcePlus || value->sign < 0)) {
        size = 1;
    }
    return compareStrings(native, replacement, value, leadingZeros, forcePlus, size, cases);
}

static int compareInt128(
    const BigNumFunctions *native,
    const BigNumFunctions *replacement,
    __int128 number,
    int allStrings,
    uint64_t *state,
    size_t *cases
) {
    ValueArea expected;
    ValueArea actual;
    native->initInt128(valueIn(&expected), number);
    replacement->initInt128(valueIn(&actual), number);
    if (compareValues(&expected, &actual, "Int128") != 0) {
        fprintf(
            stderr,
            "Int128 input high 0x%016llx low 0x%016llx.\n",
            (unsigned long long)((unsigned __int128)number >> 64),
            (unsigned long long)number
        );
        return 1;
    }
    ++*cases;
    const BigNum *value = (const BigNum *)(const void *)&actual.words[kValueSlack];
    int status = allStrings ? compareAllStrings(native, replacement, value, cases)
                            : compareRandomString(native, replacement, value, state, cases);
    if (status != 0) {
        return status;
    }
    Int128Parts parts = {(int64_t)(number >> 64), (uint64_t)number};
    for (uint8_t forcePlus = 0; forcePlus < 2; ++forcePlus) {
        char expectedText[64];
        char actualText[64];
        memset(expectedText, 0xcc, sizeof(expectedText));
        memset(actualText, 0xcc, sizeof(actualText));
        native->emit128(expectedText + 8, &parts, forcePlus);
        replacement->emit128(actualText + 8, &parts, forcePlus);
        if (memcmp(expectedText, actualText, sizeof(expectedText)) != 0) {
            fprintf(
                stderr,
                "emit128 mismatch: high 0x%016llx low 0x%016llx, plus %u.\n",
                (unsigned long long)parts.high,
                (unsigned long long)parts.low,
                forcePlus
            );
            return 1;
        }
        ++*cases;
    }
    return 0;
}

static int compareInt64(
    const BigNumFunctions *native,
    const BigNumFunctions *replacement,
    int64_t number,
    int allStrings,
    uint64_t *state,
    size_t *cases
) {
    ValueArea expected;
    ValueArea actual;
    native->initInt64(valueIn(&expected), number);
    replacement->initInt64(valueIn(&actual), number);
    if (compareValues(&expected, &actual, "Int64") != 0) {
        fprintf(stderr, "Int64 input %lld.\n", (long long)number);
        return 1;
    }
    ++*cases;
    const BigNum *value = (const BigNum *)(const void *)&actual.words[kValueSlack];
    if (allStrings) {
        return compareAllStrings(native, replacement, value, cases);
    }
    return compareRandomString(native, replacement, value, state, cases);
}

static int compareInt32(
    const BigNumFunctions *native, const BigNumFunctions *replacement, int32_t number, size_t *cases
) {
    ValueArea expected;
    ValueArea actual;
    native->initInt32(valueIn(&expected), number);
    replacement->initInt32(valueIn(&actual), number);
    if (compareValues(&expected, &actual, "Int32") != 0) {
        fprintf(stderr, "Int32 input %d.\n", number);
        return 1;
    }
    ++*cases;
    return 0;
}

static int resolveFunctions(BigNumFunctions *native, BigNumFunctions *replacement) {
    const char *nativeNames[kFunctionCount - 1] = {
        OF_NAME_STRING(_CFBigNumInitWithInt8),
        OF_NAME_STRING(_CFBigNumInitWithInt16),
        OF_NAME_STRING(_CFBigNumInitWithInt32),
        OF_NAME_STRING(_CFBigNumInitWithInt64),
        OF_NAME_STRING(_CFBigNumInitWithInt128),
        OF_NAME_STRING(_CFBigNumToCString),
    };
    const char *replacementNames[kFunctionCount - 1] = {
        OF_NAME_STRING(_OCFBigNumInitWithInt8),
        OF_NAME_STRING(_OCFBigNumInitWithInt16),
        OF_NAME_STRING(_OCFBigNumInitWithInt32),
        OF_NAME_STRING(_OCFBigNumInitWithInt64),
        OF_NAME_STRING(_OCFBigNumInitWithInt128),
        OF_NAME_STRING(_OCFBigNumToCString),
    };
    void *nativeFunctions[kFunctionCount];
    void *replacementFunctions[kFunctionCount];
    for (size_t index = 0; index < kFunctionCount - 1; ++index) {
        int status = resolveReferenceFunction(
            "CoreFoundation", OF_REFERENCE_CF_UUID, nativeNames[index], &nativeFunctions[index]
        );
        if (status == 0) {
            status = resolveReplacementFunction(
                "OpenCoreFoundation", replacementNames[index], &replacementFunctions[index]
            );
        }
        if (status != 0) {
            return status;
        }
    }
    int status = resolveReferenceAddress(
        "CoreFoundation",
        OF_REFERENCE_CF_UUID,
        UINT64_C(0x1c01ac500),
        UINT64_C(0x1c014a000),
        &nativeFunctions[kFunctionCount - 1]
    );
    if (status != 0) {
        return status;
    }
    replacementFunctions[kFunctionCount - 1] = (void *)emit128;
    status = checkInternalImplementation(replacementFunctions[kFunctionCount - 1]);
    if (status != 0) {
        return status;
    }
    for (size_t index = 0; index < kFunctionCount; ++index) {
        if (nativeFunctions[index] == replacementFunctions[index]) {
            fputs("Setup failure: BigNum reference resolved to the replacement.\n", stderr);
            return 77;
        }
    }
    void **targets[2] = {nativeFunctions, replacementFunctions};
    BigNumFunctions *sets[2] = {native, replacement};
    for (size_t set = 0; set < 2; ++set) {
        sets[set]->initInt8 = (InitInt8Function)targets[set][0];
        sets[set]->initInt16 = (InitInt16Function)targets[set][1];
        sets[set]->initInt32 = (InitInt32Function)targets[set][2];
        sets[set]->initInt64 = (InitInt64Function)targets[set][3];
        sets[set]->initInt128 = (InitInt128Function)targets[set][4];
        sets[set]->toCString = (ToCStringFunction)targets[set][5];
        sets[set]->emit128 = (Emit128Function)targets[set][6];
    }
    return 0;
}

// Native _CFBigNumInitWithInt8 0x1c02e4428, _CFBigNumInitWithInt16 0x1c02e4450,
// _CFBigNumInitWithInt32 0x1c02e4478, _CFBigNumInitWithInt64 0x1c02e44bc,
// _CFBigNumInitWithInt128 0x1c02e4534, _CFBigNumToCString 0x1c02e51f4 and emit128 0x1c01ac500
// (non-external; resolved by IDA offset). iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. iOS Device 24A446: 0x1807ac9a4, 0x1807ac9cc, 0x180723988, 0x1806f4ca4,
// 0x1806f2440, 0x1806f2c64, 0x18069c78c; UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Value bytes and C-string buffers are compared byte for byte, with canaries on both sides.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
int runCoreFoundationBigNumDifferentialTests(void) {
    BigNumFunctions native;
    BigNumFunctions replacement;
    int status = resolveFunctions(&native, &replacement);
    if (status != 0) {
        return status;
    }
    size_t cases = 0;
    uint64_t state = UINT64_C(0x5eed0f0b16a0001);
    for (int number = INT8_MIN; number <= INT8_MAX; ++number) {
        ValueArea expected;
        ValueArea actual;
        native.initInt8(valueIn(&expected), (int8_t)number);
        replacement.initInt8(valueIn(&actual), (int8_t)number);
        if (compareValues(&expected, &actual, "Int8") != 0) {
            fprintf(stderr, "Int8 input %d.\n", number);
            return 1;
        }
        ++cases;
        status = compareAllStrings(
            &native, &replacement, (const BigNum *)(const void *)&actual.words[kValueSlack], &cases
        );
        if (status != 0) {
            return status;
        }
    }
    for (int number = INT16_MIN; number <= INT16_MAX; ++number) {
        ValueArea expected;
        ValueArea actual;
        native.initInt16(valueIn(&expected), (int16_t)number);
        replacement.initInt16(valueIn(&actual), (int16_t)number);
        if (compareValues(&expected, &actual, "Int16") != 0) {
            fprintf(stderr, "Int16 input %d.\n", number);
            return 1;
        }
        ++cases;
    }
    // Boundaries: zero, plus and minus one, each power of ten and its neighbors, the type limits and
    // the narrower type limits.
    const __int128 tenTo38 = (__int128)UINT64_C(10000000000000000000) * UINT64_C(10000000000000000000);
    const __int128 int128Max = (__int128)(~(unsigned __int128)0 >> 1);
    const __int128 int128Min = -int128Max - 1;
    __int128 limits[] = {
        0,
        INT8_MIN,
        INT8_MAX,
        INT16_MIN,
        INT16_MAX,
        INT32_MIN,
        INT32_MAX,
        UINT32_MAX,
        INT64_MIN,
        INT64_MAX,
        UINT64_MAX,
        int128Min,
        int128Max,
        int128Min + 1,
        int128Max - 1,
        tenTo38 + tenTo38 / 2,
        int128Max - tenTo38,
        (__int128)1 << 64,
    };
    for (size_t index = 0; index < sizeof(limits) / sizeof(limits[0]); ++index) {
        for (int negate = 0; negate < 2; ++negate) {
            __int128 number = negate ? (__int128)(-(unsigned __int128)limits[index]) : limits[index];
            status = compareInt128(&native, &replacement, number, 1, &state, &cases);
            if (status == 0) {
                status = compareInt64(&native, &replacement, (int64_t)number, 1, &state, &cases);
            }
            if (status == 0) {
                status = compareInt32(&native, &replacement, (int32_t)number, &cases);
            }
            if (status != 0) {
                return status;
            }
        }
    }
    __int128 power = 1;
    for (int exponent = 0; exponent <= 38; ++exponent) {
        for (int offset = -1; offset <= 1; ++offset) {
            for (int negate = 0; negate < 2; ++negate) {
                __int128 number = power + offset;
                if (negate) {
                    number = -number;
                }
                status = compareInt128(&native, &replacement, number, 1, &state, &cases);
                if (status == 0 && exponent <= 18) {
                    status = compareInt64(&native, &replacement, (int64_t)number, 1, &state, &cases);
                }
                if (status == 0 && exponent <= 9) {
                    status = compareInt32(&native, &replacement, (int32_t)number, &cases);
                }
                if (status != 0) {
                    return status;
                }
            }
        }
        if (exponent < 38) {
            power *= 10;
        }
    }
    for (size_t index = 0; index < 2000000; ++index) {
        status = compareInt128(&native, &replacement, randomInt128(&state), 0, &state, &cases);
        if (status != 0) {
            return status;
        }
        uint64_t bits = nextRandom(&state) >> (nextRandom(&state) % 64);
        int64_t number = (nextRandom(&state) & 1) ? -(int64_t)bits : (int64_t)bits;
        status = compareInt64(&native, &replacement, number, 0, &state, &cases);
        if (status == 0) {
            status = compareInt32(&native, &replacement, (int32_t)nextRandom(&state), &cases);
        }
        if (status != 0) {
            return status;
        }
    }
    // Values the initializers never produce: digits above 10^9 - 1 widen the scratch string beyond 45
    // characters and truncate it; sign bytes other than 0 and 0xff test bit 7 only.
    for (size_t index = 0; index < 500000; ++index) {
        BigNum value;
        uint64_t sign = nextRandom(&state);
        for (size_t digit = 0; digit < 5; ++digit) {
            uint64_t bits = nextRandom(&state);
            value.digits[digit] =
                (bits & 3) == 0 ? (uint32_t)(bits >> 32) : (uint32_t)((bits >> 32) % 1000000000u);
        }
        memcpy((uint8_t *)&value + 20, &sign, 4);
        status = index < 2000 ? compareAllStrings(&native, &replacement, &value, &cases)
                              : compareRandomString(&native, &replacement, &value, &state, &cases);
        if (status != 0) {
            return status;
        }
    }
    printf("BigNum differential cases passed: %zu (seven functions; finite coverage).\n", cases);
    return 0;
}
