//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stdint.h>

// Binary property-list trailer, as __CFBinaryPlistGetTopLevelInfo (iOS Simulator 24A434, 0x1c0163b94,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified) fills it: the six leading bytes of
// the file trailer copied unchanged (a 4-byte store at 0, a 2-byte store at 4), the offset-table entry
// width (byte at 6), the object-reference width (byte at 7), the object count (64 bits at 8), the top
// object index (64 bits at 16) and the offset-table offset (64 bits at 24), all host-endian.
// __CFBinaryPlistGetOffsetForValueFromArray2 reads the widths, the count and the table offset. The 32
// bytes are the extent of the fields written; BinaryPlistTrailer and its field names are local
// descriptive spellings.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
typedef struct {
    uint8_t unused[6];
    uint8_t offsetIntSize;
    uint8_t objectRefSize;
    uint64_t numObjects;
    uint64_t topObject;
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

OF_EXPORT bool __OCFBinaryPlistGetTopLevelInfo(
    const uint8_t *databytes, uint64_t datalen, uint8_t *marker, uint64_t *offset, BinaryPlistTrailer *trailer
);
