//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdint.h>

// Original non-external name is preserved; no process-shared symbol is exported. ranges is the packed
// component-range array: one 16-byte range per set flag bit, in bit order (bit 0 first).
OF_EXTERN OCFRange _netLocationRange(uint32_t flags, const OCFRange *ranges);
