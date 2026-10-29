//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdint.h>

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN uint32_t __CFStringEncodingPlatformUnicodeToBytes(
    uint32_t encoding,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    OCFIndex *usedCharLen,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFStringEncodingPlatformBytesToUnicode(
    uint32_t encoding,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    OCFIndex *usedByteLen,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFStringEncodingPlatformCharLengthForBytes(
    uint32_t encoding, uint32_t flags, const uint8_t *bytes, OCFIndex numBytes
);
