//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stdint.h>

// Binary property-list trailer fields read by __CFBinaryPlistGetOffsetForValueFromArray2,
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for the four
// fields read: the offset-table entry width (byte at 6), the object-reference width (byte at 7), the
// object count (64 bits at 8) and the offset-table offset (64 bits at 24), all host-endian. Bytes 0..5
// and 16..23 are unread here; the 32-byte size is the extent of the fields read, not a measurement of
// the producer. BinaryPlistTrailer and its field names are local descriptive spellings.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
typedef struct {
    uint8_t unread0[6];
    uint8_t offsetIntSize;
    uint8_t objectRefSize;
    uint64_t numObjects;
    uint64_t unread1;
    uint64_t offsetTableOffset;
} BinaryPlistTrailer;

// The callers' string blocks receive the first character byte and the character count (bytes for
// ASCII, UTF-16 units for Unicode16) and return the parse result.
typedef bool (^BinaryPlistStringBlock)(const uint8_t *characters, uint64_t count);

OF_EXPORT bool __OCFBinaryPlistGetOffsetForValueFromArray2(
    const uint8_t *databytes,
    uint64_t length,
    uint64_t startOffset,
    const BinaryPlistTrailer *trailer,
    OCFIndex index,
    uint64_t *offset,
    const void *ignored
);

OF_EXPORT bool __OCFBinaryPlistParseASCIIString(
    const uint8_t *base, uint64_t limit, const uint8_t *marker, BinaryPlistStringBlock block
);

OF_EXPORT bool __OCFBinaryPlistParseUnicode16String(
    const uint8_t *base, uint64_t limit, const uint8_t *marker, BinaryPlistStringBlock block
);
