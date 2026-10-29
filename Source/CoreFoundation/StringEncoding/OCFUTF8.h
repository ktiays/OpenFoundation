//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdint.h>

// Original hidden kernel names are preserved; no process-shared symbol is exported.
OF_EXTERN OCFIndex __CFToUTF8(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex count,
    uint8_t *bytes,
    OCFIndex capacity,
    OCFIndex *usedBytes
);

OF_EXTERN OCFIndex __CFToUTF8Len(uint32_t flags, const uint16_t *characters, OCFIndex count);
