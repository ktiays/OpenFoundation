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
OF_EXTERN uint32_t __CFToISOLatinGreek(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromISOLatinGreek(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToISOLatinGreekPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToMacGreek(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromMacGreek(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToMacGreekPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToDOSGreek(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromDOSGreek(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToDOSGreekPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToWindowsGreek(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromWindowsGreek(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToWindowsGreekPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN const StringEncodingConverterRecord __CFConverterISOLatinGreek;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacGreek;

OF_EXTERN const StringEncodingConverterRecord __CFConverterDOSGreek;

OF_EXTERN const StringEncodingConverterRecord __CFConverterWindowsGreek;

// Native __CFIsValidCombiningCharacterForGreek, 0x1c0305950, and __CFPrecomposeGreekCharacter, 0x1c0305e1c
// (non-external; both defined in this unit). The precompose helper stores 2 in
// *usedChars before it reads the characters and returns the precomposed unit or 0xfffd.
OF_EXTERN bool __CFIsValidCombiningCharacterForGreek(uint16_t character);

OF_EXTERN uint16_t
__CFPrecomposeGreekCharacter(const uint16_t *character, OCFIndex numChars, OCFIndex *usedChars);
