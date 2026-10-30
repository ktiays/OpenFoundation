//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stdint.h>

// Format-specification record as __CFStringFindTypeAndSizeForArgumentIndex and
// __CFStringValidateFormatSpecBuffers read it, iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for the offsets, widths and the
// 32-byte stride (ADD X8, X8, #0x20 per record); unread for bytes 4..23 and 27..31, which neither
// function touches. The size at offset 0 and the type at offset 2 are copied as 16-bit values (LDURH,
// STRH) and compared as their low bytes (LDURB). The three signed argument numbers at offsets 24..26
// take -1 (main) or -2 (width, precision) for "assign the next sequential argument"; the width,
// precision and main roles of offsets 26, 25 and 24 are inferred from that consumption order.
// FormatSpec and its field names are local descriptive spellings; the binary carries no type name.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
typedef struct {
    int16_t size;
    int16_t type;
    uint8_t unread0[20];
    int8_t mainArgNum;
    int8_t precArgNum;
    int8_t widthArgNum;
    uint8_t unread1[5];
} FormatSpec;

// Original non-external names are preserved; no process-shared symbol is exported.
OF_EXTERN bool __CFStringFindTypeAndSizeForArgumentIndex(
    const FormatSpec *specs, int32_t count, OCFIndex argumentIndex, int16_t *type, int16_t *size
);

OF_EXTERN bool __CFStringValidateFormatSpecBuffers(
    const FormatSpec *specs,
    int32_t count,
    const FormatSpec *expectedSpecs,
    int32_t expectedCount,
    OCFIndex nextArgumentIndex
);

OF_EXTERN bool __rangeEqual(const void *first, const void *second);
