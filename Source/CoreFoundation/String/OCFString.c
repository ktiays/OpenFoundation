//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFString.h"

// Native __CFStringValidateFormatSpecBuffers, iOS Simulator 24A434, 0x1c02242fc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1806ba570, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// indexes the record by a counter instead of advancing a cursor and authenticates its return; the
// comparisons, calls and the 0x18800000000 type mask agree.
// For each record of specs, the width (offset 26) and precision (offset 25) argument numbers are
// clamped to -1 when negative, -2 takes the next sequential argument number, and the main argument
// number (offset 24) takes the next one when it is -1. nextArgumentIndex is the first sequential
// number; only its low byte reaches a comparison. A width or precision argument must be found in
// expectedSpecs with type 0x21 and a size at most 4. The main argument must be found; its type must
// equal the record's type or be one of the accepted pairs below, and the two sizes must agree after
// sizes up to 4 are raised to 4. Every comparison uses the low byte of the 16-bit size and type.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool __CFStringValidateFormatSpecBuffers(
    const FormatSpec *specs,
    int32_t count,
    const FormatSpec *expectedSpecs,
    int32_t expectedCount,
    OCFIndex nextArgumentIndex
) {
    for (int32_t index = 0; index < count; ++index) {
        const FormatSpec *spec = &specs[index];
        int8_t width = spec->widthArgNum > -1 ? spec->widthArgNum : -1;
        if (spec->widthArgNum == -2) {
            width = (int8_t)nextArgumentIndex;
            ++nextArgumentIndex;
        }
        int8_t precision = spec->precArgNum > -1 ? spec->precArgNum : -1;
        if (spec->precArgNum == -2) {
            precision = (int8_t)nextArgumentIndex;
            ++nextArgumentIndex;
        }
        int8_t argument = spec->mainArgNum;
        if (argument == -1) {
            argument = (int8_t)nextArgumentIndex;
            ++nextArgumentIndex;
        }
        // The native function clears one 32-bit stack word holding both results once per record.
        int16_t type = 0;
        int16_t size = 0;
        if (width != -1) {
            if (!__CFStringFindTypeAndSizeForArgumentIndex(
                    expectedSpecs, expectedCount, width, &type, &size
                )) {
                return false;
            }
            if ((uint8_t)type != 0x21 || (uint8_t)size > 4) {
                return false;
            }
        }
        if (precision != -1) {
            if (!__CFStringFindTypeAndSizeForArgumentIndex(
                    expectedSpecs, expectedCount, precision, &type, &size
                )) {
                return false;
            }
            if ((uint8_t)type != 0x21 || (uint8_t)size > 4) {
                return false;
            }
        }
        if (!__CFStringFindTypeAndSizeForArgumentIndex(
                expectedSpecs, expectedCount, argument, &type, &size
            )) {
            return false;
        }
        uint8_t specType = (uint8_t)spec->type;
        uint8_t foundType = (uint8_t)type;
        if (specType != foundType) {
            if (specType <= 0x26) {
                if (specType == 0x23) {
                    if ((unsigned)(foundType - 0x25) >= 4 && foundType != 0x21) {
                        return false;
                    }
                } else if (specType != 0x26 || foundType != 0x23) {
                    return false;
                }
            } else if (specType == 0x27 || specType == 0x28) {
                if (foundType > 0x28 || ((UINT64_C(1) << foundType) & UINT64_C(0x18800000000)) == 0) {
                    return false;
                }
            } else if (specType != 0x29 || foundType != 0x21) {
                return false;
            }
        }
        uint8_t specSize = (uint8_t)spec->size;
        uint8_t foundSize = (uint8_t)size;
        if (specSize <= 4) {
            specSize = 4;
        }
        if (foundSize <= 4) {
            foundSize = 4;
        }
        if (specSize != foundSize) {
            return false;
        }
    }
    return true;
}

// Native __CFStringFindTypeAndSizeForArgumentIndex, iOS Simulator 24A434, 0x1c022452c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1806d64a8, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// indexes records by a counter; the comparisons, sequential numbering and stores agree.
// The records are numbered as in __CFStringValidateFormatSpecBuffers from a 64-bit counter that starts
// at zero; each assigned number is the counter's low byte, sign-extended. argumentIndex is compared in
// 64 bits (CMP X2, Wn, SXTB). A width or precision match stores type 0x21 and size 3; a main-argument
// match copies the record's 16-bit type and then its size. The first matching record wins and the out
// parameters are untouched when none matches.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool __CFStringFindTypeAndSizeForArgumentIndex(
    const FormatSpec *specs, int32_t count, OCFIndex argumentIndex, int16_t *type, int16_t *size
) {
    OCFIndex nextArgumentIndex = 0;
    for (int32_t index = 0; index < count; ++index) {
        const FormatSpec *spec = &specs[index];
        int8_t width = spec->widthArgNum > -1 ? spec->widthArgNum : -1;
        if (spec->widthArgNum == -2) {
            width = (int8_t)nextArgumentIndex;
            ++nextArgumentIndex;
        }
        int8_t precision = spec->precArgNum > -1 ? spec->precArgNum : -1;
        if (spec->precArgNum == -2) {
            precision = (int8_t)nextArgumentIndex;
        }
        if (argumentIndex == width || argumentIndex == precision) {
            *type = 0x21;
            *size = 3;
            return true;
        }
        if (spec->precArgNum == -2) {
            ++nextArgumentIndex;
        }
        int8_t argument = spec->mainArgNum;
        if (argument == -1) {
            argument = (int8_t)nextArgumentIndex;
        }
        if (argumentIndex == argument) {
            *type = spec->type;
            *size = spec->size;
            return true;
        }
        if (spec->mainArgNum == -1) {
            ++nextArgumentIndex;
        }
    }
    return false;
}

// Native __rangeEqual, iOS Simulator 24A434, 0x1c0218d64, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; no direct caller (an address-taken callback beside __rangeRetain,
// __rangeRelease and __rangeCopyDescription).
// iOS Device 24A446: 0x180764320, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions.
// Compares two 16-byte ranges: both 64-bit words must be equal (LDP, CMP, CCMP, CSET).
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool __rangeEqual(const void *first, const void *second) {
    const OCFRange *firstRange = first;
    const OCFRange *secondRange = second;
    return firstRange->location == secondRange->location && firstRange->length == secondRange->length;
}
