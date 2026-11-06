//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN uint32_t __CFToISOLatin5(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromISOLatin5(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToISOLatin5Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToMacTurkish(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromMacTurkish(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToMacTurkishPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToDOSTurkish(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromDOSTurkish(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToDOSTurkishPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToWindowsLatin5(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromWindowsLatin5(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToWindowsLatin5Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN const StringEncodingConverterRecord __CFConverterISOLatin5;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacTurkish;

OF_EXTERN const StringEncodingConverterRecord __CFConverterDOSTurkish;

OF_EXTERN const StringEncodingConverterRecord __CFConverterWindowsLatin5;

// Native __CFIsValidCombiningCharacterForLatin5, 0x1c01dc2f8 (non-external), defined in this unit.
OF_EXTERN bool __CFIsValidCombiningCharacterForLatin5(uint16_t character);
