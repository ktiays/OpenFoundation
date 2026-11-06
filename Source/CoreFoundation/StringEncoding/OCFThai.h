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
OF_EXTERN uint32_t __CFToDOSThai(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromDOSThai(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToThaiPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToISOLatinThai(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromISOLatinThai(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN uint32_t
__CFToMacThai(uint32_t flags, const uint16_t *characters, OCFIndex numChars, uint8_t *byte);

OF_EXTERN uint32_t __CFFromMacThai(uint32_t flags, uint8_t byte, uint16_t *characters);

OF_EXTERN OCFIndex __CFToMacThaiLen(uint32_t flags, const uint16_t *characters, OCFIndex numChars);

OF_EXTERN OCFIndex __CFFromMacThaiLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN const StringEncodingConverterRecord __CFConverterDOSThai;

OF_EXTERN const StringEncodingConverterRecord __CFConverterISOLatinThai;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacThai;

// Native __CFIsValidCombiningCharacterForThai, 0x1c02f41d0 (non-external, defined in this unit).
OF_EXTERN bool __CFIsValidCombiningCharacterForThai(uint16_t character);
