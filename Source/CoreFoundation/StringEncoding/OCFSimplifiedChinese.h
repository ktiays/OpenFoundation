//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"
#include "CoreFoundation/StringEncoding/OCFPlatformConverter.h"
#include "CoreFoundation/StringEncoding/OCFSingleByte.h"

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stdint.h>

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN uint32_t __CFToEUC_CN(uint32_t flags, uint16_t character, uint8_t *bytes);

OF_EXTERN uint32_t
__CFFromEUC_CN(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes, uint16_t *character);

OF_EXTERN OCFIndex __CFToEUC_CNLen(uint32_t flags, const uint16_t *characters, OCFIndex numChars);

OF_EXTERN OCFIndex __CFFromEUC_CNLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN OCFIndex __CFToEUC_CNPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFToGB_HZ(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromGB_HZ(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToMacSimplifiedChinese(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromMacSimplifiedChinese(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFFromMacSimplifiedChineseLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN const StringEncodingConverterRecord __CFConverterEUC_CN;

OF_EXTERN const StringEncodingConverterRecord __CFConverterGB_HZ;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacSimplifiedChinese;

// Native __CFIsValidCombiningCharSimplifiedChinese, 0x1c0206f2c (non-external, defined in this unit).
OF_EXTERN bool __CFIsValidCombiningCharSimplifiedChinese(uint16_t character);
