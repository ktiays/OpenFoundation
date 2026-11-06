//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stdint.h>

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN uint32_t __CFToISOCyrillic(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromISOCyrillic(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToISOCyrillicPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToDOSRussian(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromDOSRussian(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToDOSRussianPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToWinCyrillic(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromWinCyrillic(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToWinCyrillicPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToMacCyrillic(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromMacCyrillic(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToMacCyrillicPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToKOI8R(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromKOI8R(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN const StringEncodingConverterRecord __CFConverterISOCyrillic;

OF_EXTERN const StringEncodingConverterRecord __CFConverterDOSRussian;

OF_EXTERN const StringEncodingConverterRecord __CFConverterWinCyrillic;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacCyrillic;

OF_EXTERN const StringEncodingConverterRecord __CFConverterKOI8R;

// Native __CFIsValidCombiningCharacterForCyrillic, 0x1c02c3eec, and __CFPrecomposeCyrillicCharacter,
// 0x1c02c43bc (non-external; both defined in this unit). The precompose helper
// reads character[0] and character[1] and returns the precomposed unit or 0xfffd.
OF_EXTERN bool __CFIsValidCombiningCharacterForCyrillic(uint16_t character);

OF_EXTERN uint16_t __CFPrecomposeCyrillicCharacter(const uint16_t *character);
