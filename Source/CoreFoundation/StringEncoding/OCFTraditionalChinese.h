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
OF_EXTERN uint32_t __CFToBig5(uint32_t flags, uint16_t character, uint8_t *bytes);

OF_EXTERN uint32_t __CFFromBig5(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes, uint16_t *character);

OF_EXTERN OCFIndex __CFToBig5Len(uint32_t flags, const uint16_t *characters, OCFIndex numChars);

OF_EXTERN OCFIndex __CFFromBig5Len(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN OCFIndex __CFToBig5Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFToMacTraditionalChinese(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromMacTraditionalChinese(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFFromMacTraditionalChineseLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN const StringEncodingConverterRecord __CFConverterBig5;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacTraditionalChinese;

// Native __CFIsValidCombiningCharTraditionalChinese, 0x1c0253798 (non-external, defined in this unit).
OF_EXTERN bool __CFIsValidCombiningCharTraditionalChinese(uint16_t character);
