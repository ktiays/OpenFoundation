//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdint.h>

// The 8-byte result of _CFBuildVersionForCFSystemVersion, returned in X0: the platform in the low 32
// bits and the packed version in the high 32 bits (ORR X0, X8, X0, LSL #32), the layout of dyld's
// build-version pair. OCFBuildVersion and its field names are local descriptive spellings; iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
typedef struct {
    uint32_t platform;
    uint32_t version;
} OCFBuildVersion;

// systemVersion is compared as an unsigned 64-bit value; platform is a 32-bit dyld platform number.
OF_EXPORT OCFBuildVersion _OCFBuildVersionForCFSystemVersion(uint64_t systemVersion, uint32_t platform);
