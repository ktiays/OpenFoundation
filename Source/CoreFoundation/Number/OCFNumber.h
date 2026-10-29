//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdint.h>

// Signed 128-bit value as emit128 and its tables read it, iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified: the signed high word at offset 0,
// the unsigned low word at offset 8. Int128Parts and its field names are local descriptive spellings.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
typedef struct {
    int64_t high;
    uint64_t low;
} Int128Parts;

// Original non-external name is preserved; no process-shared symbol is exported.
OF_EXTERN void emit128(char *buffer, const Int128Parts *value, uint8_t forcePlus);
