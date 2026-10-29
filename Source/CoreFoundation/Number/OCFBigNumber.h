//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stddef.h>
#include <stdint.h>

// Value layout read from the initializers and _CFBigNumToCString, iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for offsets, size and stores.
// Twenty-four bytes, four-byte aligned: five 32-bit base-10^9 digits, least significant first
// (offsets 0, 4, 8, 12, 16), and a sign byte at offset 20 that is 0xff when negative and zero
// otherwise; bytes 21..23 are zero after initialization. The initializers set the sign with a 32-bit
// read-modify-write of the word at offset 20 (LDR, ORR #0xff, STR), which the two bit-fields below
// reproduce; a lone 8-bit field compiles to a byte store. BigNum, its field names and the bit-field
// representation are local descriptive spellings (inferred); the binary carries no type name. Reading:
// Documentation/Reference/Research/CoreFoundationBigNum.md.
typedef struct {
    uint32_t digits[5];
    int32_t sign : 8;
    int32_t reserved : 24;
} BigNum;

OF_EXPORT void _OCFBigNumInitWithInt8(BigNum *value, int8_t number);

OF_EXPORT void _OCFBigNumInitWithInt16(BigNum *value, int16_t number);

OF_EXPORT void _OCFBigNumInitWithInt32(BigNum *value, int32_t number);

OF_EXPORT void _OCFBigNumInitWithInt64(BigNum *value, int64_t number);

OF_EXPORT void _OCFBigNumInitWithInt128(BigNum *value, __int128 number);

OF_EXPORT void _OCFBigNumToCString(
    const BigNum *value, uint8_t leadingZeros, uint8_t forcePlus, char *buffer, size_t bufferSize
);
