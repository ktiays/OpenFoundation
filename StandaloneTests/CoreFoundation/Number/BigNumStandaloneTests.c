//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Number/BigNumStandaloneTests.h"
#include "CoreFoundation/Number/OCFBigNumber.h"
#include "CoreFoundation/Number/OCFNumber.h"

#include <stdio.h>
#include <string.h>

static int checkDigits(const BigNum *value, const uint32_t digits[5], uint8_t sign, int line) {
    uint8_t expected[24] = {0};
    memcpy(expected, digits, 20);
    expected[20] = sign;
    if (sizeof(BigNum) != 24 || memcmp(value, expected, sizeof(expected)) != 0) {
        fprintf(stderr, "BigNum standalone mismatch at source line %d.\n", line);
        return 1;
    }
    return 0;
}

static int checkString(const char *actual, const char *expected, size_t length, int line) {
    if (memcmp(actual, expected, length) != 0) {
        fprintf(stderr, "BigNum standalone mismatch at source line %d.\n", line);
        return 1;
    }
    return 0;
}

// Native _CFBigNumInitWithInt8 0x1c02e4428, _CFBigNumInitWithInt16 0x1c02e4450,
// _CFBigNumInitWithInt32 0x1c02e4478, _CFBigNumInitWithInt64 0x1c02e44bc,
// _CFBigNumInitWithInt128 0x1c02e4534, _CFBigNumToCString 0x1c02e51f4, emit128 0x1c01ac500.
// iOS Simulator 24A434 UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// Device comparison: 0x1807ac9a4, 0x1807ac9cc, 0x180723988, 0x1806f4ca4, 0x1806f2440, 0x1806f2c64,
// 0x18069c78c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
int runCoreFoundationBigNumStandaloneTests(void) {
    size_t cases = 0;
    BigNum value;
    memset(&value, 0x5a, sizeof(value));
    _OCFBigNumInitWithInt8(&value, INT8_MIN);
    if (checkDigits(&value, (const uint32_t[5]){128, 0, 0, 0, 0}, 0xff, __LINE__) != 0) {
        return 1;
    }
    _OCFBigNumInitWithInt16(&value, INT16_MIN);
    if (checkDigits(&value, (const uint32_t[5]){32768, 0, 0, 0, 0}, 0xff, __LINE__) != 0) {
        return 1;
    }
    _OCFBigNumInitWithInt32(&value, INT32_MIN);
    if (checkDigits(&value, (const uint32_t[5]){147483648, 2, 0, 0, 0}, 0xff, __LINE__) != 0) {
        return 1;
    }
    _OCFBigNumInitWithInt64(&value, INT64_MAX);
    if (checkDigits(&value, (const uint32_t[5]){854775807, 223372036, 9, 0, 0}, 0, __LINE__) != 0) {
        return 1;
    }
    // 2^127 = 170141183460469231731687303715884105728.
    __int128 minimum = (__int128)((unsigned __int128)1 << 127);
    _OCFBigNumInitWithInt128(&value, minimum);
    if (checkDigits(
            &value, (const uint32_t[5]){884105728, 687303715, 469231731, 141183460, 170}, 0xff, __LINE__
        ) != 0) {
        return 1;
    }
    cases += 5;
    char text[64];
    memset(text, 0x5a, sizeof(text));
    _OCFBigNumToCString(&value, 0, 0, text, sizeof(text));
    if (checkString(text, "-170141183460469231731687303715884105728", 41, __LINE__) != 0) {
        return 1;
    }
    _OCFBigNumInitWithInt32(&value, 0);
    memset(text, 0x5a, sizeof(text));
    _OCFBigNumToCString(&value, 0, 1, text, 8);
    if (checkString(text, "+0\0\x5a", 4, __LINE__) != 0) {
        return 1;
    }
    _OCFBigNumInitWithInt64(&value, -42);
    memset(text, 0x5a, sizeof(text));
    _OCFBigNumToCString(&value, 1, 0, text, 6);
    if (checkString(text, "-0042\0\x5a", 7, __LINE__) != 0) {
        return 1;
    }
    _OCFBigNumInitWithInt64(&value, 123456);
    memset(text, 0x5a, sizeof(text));
    _OCFBigNumToCString(&value, 0, 0, text, 4);
    if (checkString(text, "123\0\x5a", 5, __LINE__) != 0) {
        return 1;
    }
    memset(text, 0x5a, sizeof(text));
    _OCFBigNumToCString(&value, 1, 1, text, 50);
    if (checkString(text, "+000000000000000000000000000000000000000000123456\0\x5a", 51, __LINE__) != 0) {
        return 1;
    }
    cases += 5;
    Int128Parts parts = {-1, 0};
    memset(text, 0x5a, sizeof(text));
    emit128(text, &parts, 0);
    if (checkString(text, "-18446744073709551616\0\x5a", 23, __LINE__) != 0) {
        return 1;
    }
    parts = (Int128Parts){0, 0};
    memset(text, 0x5a, sizeof(text));
    emit128(text, &parts, 1);
    if (checkString(text, "+0\0\x5a", 4, __LINE__) != 0) {
        return 1;
    }
    parts = (Int128Parts){INT64_MIN, 0};
    memset(text, 0x5a, sizeof(text));
    emit128(text, &parts, 0);
    if (checkString(text, "-0\0\x5a", 4, __LINE__) != 0) {
        return 1;
    }
    parts = (Int128Parts){INT64_MAX, UINT64_MAX};
    memset(text, 0x5a, sizeof(text));
    emit128(text, &parts, 0);
    if (checkString(text, "170141183460469231731687303715884105727\0\x5a", 41, __LINE__) != 0) {
        return 1;
    }
    cases += 4;
    printf("BigNum standalone cases passed: %zu (finite coverage).\n", cases);
    return 0;
}
