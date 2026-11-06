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
OF_EXTERN uint32_t
__CFToMacDingbats(uint32_t flags, const uint16_t *characters, OCFIndex numChars, uint8_t *byte);

OF_EXTERN uint32_t __CFFromMacDingbats(uint32_t flags, uint8_t byte, uint16_t *characters);

OF_EXTERN OCFIndex __CFToMacDingbatsLen(uint32_t flags, const uint16_t *characters, OCFIndex numChars);

OF_EXTERN OCFIndex __CFFromMacDingbatsLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN uint32_t
__CFToMacSymbol(uint32_t flags, const uint16_t *characters, OCFIndex numChars, uint8_t *byte);

OF_EXTERN uint32_t __CFFromMacSymbol(uint32_t flags, uint8_t byte, uint16_t *characters);

OF_EXTERN OCFIndex __CFToMacSymbolLen(uint32_t flags, const uint16_t *characters, OCFIndex numChars);

OF_EXTERN OCFIndex __CFFromMacSymbolLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacDingbats;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacSymbol;

// Native __CFPrecomposeCharMacSymbol, 0x1c022f984, and __CFIsValidCombiningCharMacSymbol, 0x1c022f9f0
// (non-external; both defined in this unit).
OF_EXTERN OCFIndex __CFPrecomposeCharMacSymbol(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN bool __CFIsValidCombiningCharMacSymbol(uint16_t character);
