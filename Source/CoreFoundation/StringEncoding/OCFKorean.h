//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"
#include "CoreFoundation/StringEncoding/OCFPlatformConverter.h"

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdint.h>

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN OCFIndex __CFToDOSKorean(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromDOSKorean(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToDOSKoreanLen(uint32_t flags, const uint16_t *characters, OCFIndex numChars);

OF_EXTERN OCFIndex __CFFromDOSKoreanLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN OCFIndex __CFFromDOSKoreanFallback(
    const uint8_t *bytes, OCFIndex numBytes, uint16_t *characters, OCFIndex maxCharLen, OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToEUC_KR(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromEUC_KR(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToISO_2022_KR(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromISO_2022_KR(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToISO_2022_KRLen(uint32_t flags, const uint16_t *characters, OCFIndex numChars);

OF_EXTERN OCFIndex __CFFromISO_2022_KRLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN OCFIndex __CFToMacKorean(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromMacKorean(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFFromMacKoreanLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN uint16_t
__CFToKSX1001(const uint16_t *characters, OCFIndex numChars, OCFIndex *usedCharLen, uint16_t *precomposed);

OF_EXTERN uint16_t __CFFromKSX1001(uint16_t code);

OF_EXTERN const StringEncodingConverterRecord __CFConverterDOSKorean;

OF_EXTERN const StringEncodingConverterRecord __CFConverterEUC_KR;

OF_EXTERN const StringEncodingConverterRecord __CFConverterISO_2022_KR;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacKorean;

// Native __CFPrecomposeCharKorean, 0x1c0229d6c (non-external, defined in this unit). Reads the unit at
// character[0] and the combining unit at character[1]; returns the precomposed unit or 0 (W0, read as 16
// bits).
OF_EXTERN uint16_t __CFPrecomposeCharKorean(const uint16_t *character);
