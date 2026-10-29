//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Number/OCFBigNumber.h"

#include <stdio.h>
#include <string.h>

// Native _CFBigNumInitWithInt8, iOS Simulator 24A434, 0x1c02e4428,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// iOS Device 24A446: 0x1807ac9a4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions.
// The only native caller sign-extends the argument (SXTB) and ignores the result register.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
void _OCFBigNumInitWithInt8(BigNum *value, int8_t number) {
    memset(value, 0, sizeof(BigNum));
    if (number < 0) {
        value->sign = -1;
        number = (int8_t)-number;
    }
    value->digits[0] = (uint8_t)number;
}

// Native _CFBigNumInitWithInt16, iOS Simulator 24A434, 0x1c02e4450,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// iOS Device 24A446: 0x1807ac9cc, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions.
// The only native caller sign-extends the argument (SXTH) and ignores the result register.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
void _OCFBigNumInitWithInt16(BigNum *value, int16_t number) {
    memset(value, 0, sizeof(BigNum));
    if (number < 0) {
        value->sign = -1;
        number = (int16_t)-number;
    }
    value->digits[0] = (uint16_t)number;
}

// Native _CFBigNumInitWithInt32, iOS Simulator 24A434, 0x1c02e4478,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// iOS Device 24A446: 0x180723988, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions.
// The magnitude is the 32-bit two's-complement negation, so INT32_MIN becomes 2^31.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
void _OCFBigNumInitWithInt32(BigNum *value, int32_t number) {
    memset(value, 0, sizeof(BigNum));
    uint32_t magnitude = (uint32_t)number;
    if (number < 0) {
        value->sign = -1;
        magnitude = -magnitude;
    }
    value->digits[0] = magnitude % 1000000000u;
    value->digits[1] = magnitude / 1000000000u;
}

// Native _CFBigNumInitWithInt64, iOS Simulator 24A434, 0x1c02e44bc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// iOS Device 24A446: 0x1806f4ca4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: _CFBigNumInitWithInt64 at 0x1806f4ca4 places the negative-sign block after
// the return and branches back to the division; the stores and arithmetic agree.
// The magnitude is the unsigned 64-bit negation, so INT64_MIN becomes 2^63.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
void _OCFBigNumInitWithInt64(BigNum *value, int64_t number) {
    memset(value, 0, sizeof(BigNum));
    uint64_t magnitude = (uint64_t)number;
    if (number < 0) {
        value->sign = -1;
        magnitude = -magnitude;
    }
    value->digits[0] = (uint32_t)(magnitude % UINT64_C(1000000000000000000) % UINT64_C(1000000000));
    value->digits[1] = (uint32_t)(magnitude % UINT64_C(1000000000000000000) / UINT64_C(1000000000));
    value->digits[2] = (uint32_t)(magnitude / UINT64_C(1000000000000000000));
}

// Native _CFBigNumInitWithInt128, iOS Simulator 24A434, 0x1c02e4534,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// iOS Device 24A446: 0x1806f2440, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions apart from PACIBSP/RETAB.
// The argument arrives in X1 (low) and X2 (high), the Darwin arm64 __int128 assignment; both native
// callers pass it so (one splits a high/low pair from memory). The magnitude is the unsigned 128-bit
// negation, so the minimum becomes 2^127. Four __udivti3 calls divide by 10^36, 10^27, 10^18 and 10^9.
// The first remainder uses the whole 128-bit quotient; the next two use only the low 32 bits of
// their quotients, and the last digit is the 32-bit difference, as in the native code.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
void _OCFBigNumInitWithInt128(BigNum *value, __int128 number) {
    const unsigned __int128 tenTo9 = 1000000000u;
    const unsigned __int128 tenTo18 = UINT64_C(1000000000000000000);
    const unsigned __int128 tenTo27 = tenTo18 * tenTo9;
    const unsigned __int128 tenTo36 = tenTo18 * tenTo18;
    memset(value, 0, sizeof(BigNum));
    unsigned __int128 magnitude = (unsigned __int128)number;
    if (number < 0) {
        value->sign = -1;
        magnitude = -magnitude;
    }
    unsigned __int128 quotient = magnitude / tenTo36;
    uint32_t digit4 = (uint32_t)quotient;
    magnitude -= quotient * tenTo36;
    uint32_t digit3 = (uint32_t)(magnitude / tenTo27);
    magnitude -= (unsigned __int128)digit3 * tenTo27;
    uint32_t digit2 = (uint32_t)(magnitude / tenTo18);
    magnitude -= (unsigned __int128)digit2 * tenTo18;
    uint32_t digit1 = (uint32_t)(magnitude / tenTo9);
    value->digits[0] = (uint32_t)magnitude - digit1 * 1000000000u;
    value->digits[1] = digit1;
    value->digits[2] = digit2;
    value->digits[3] = digit3;
    value->digits[4] = digit4;
}

// Native _CFBigNumToCString, iOS Simulator 24A434, 0x1c02e51f4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// iOS Device 24A446: 0x1806f2c64, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: _CFBigNumToCString at 0x1806f2c64 moves the '-' and leading-zero blocks
// after the return; the calls, their arguments and order agree.
// Format literal "%09u%09u%09u%09u%09u" (21 bytes with NUL): iOS Simulator 0x1c049bc9f, iOS Device
// 0x180acdf83; bytes agree. Bit 7 of the sign byte selects '-'; a sign or '+' consumes one byte of
// bufferSize without a check, so a zero size wraps. With leadingZeros the whole buffer is filled with
// '0' and the last min(bufferSize, 46) bytes of the 46-byte scratch string, its NUL included, are
// moved to the end. Otherwise leading '0' characters are skipped, keeping one for zero, and the rest
// is copied with strlcpy. The scratch string truncates digits wider than nine characters at 45.
// The only native caller ignores the result register; the restoration returns void. Release LTO lowers
// the memmove from the private scratch string to memcpy; the bytes written are the same.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
void _OCFBigNumToCString(
    const BigNum *value, uint8_t leadingZeros, uint8_t forcePlus, char *buffer, size_t bufferSize
) {
    if (value->sign < 0) {
        *buffer++ = '-';
        --bufferSize;
    } else if (forcePlus) {
        *buffer++ = '+';
        --bufferSize;
    }
    char digits[46];
    snprintf(
        digits,
        sizeof(digits),
        "%09u%09u%09u%09u%09u",
        value->digits[4],
        value->digits[3],
        value->digits[2],
        value->digits[1],
        value->digits[0]
    );
    if (leadingZeros) {
        memset(buffer, '0', bufferSize);
        size_t length = bufferSize < sizeof(digits) ? bufferSize : sizeof(digits);
        memmove(buffer + bufferSize - length, digits + sizeof(digits) - length, length);
    } else {
        const char *start = digits;
        while (*start == '0') {
            ++start;
        }
        if (*start == 0) {
            --start;
        }
        strlcpy(buffer, start, bufferSize);
    }
}
