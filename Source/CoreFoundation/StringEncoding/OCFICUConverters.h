//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdint.h>

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN OCFIndex __CFStringEncodingICUToBytes(
    const char *icuName,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    OCFIndex *usedCharLen,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFStringEncodingICUToUnicode(
    const char *icuName,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    OCFIndex *usedByteLen,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex
__CFStringEncodingICUCharLength(const char *icuName, uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN OCFIndex __CFStringEncodingICUByteLength(
    const char *icuName, uint32_t flags, const uint16_t *characters, OCFIndex numChars
);

// __CFICUThreadDataDestructor (0x1c02ae75c, non-external) is the slot-7 destructor.
OF_EXTERN void __CFICUThreadDataDestructor(void *context);
