//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/StringEncoding/OCFSingleByte.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stdint.h>

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN uint32_t __CFToISOLatin2(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromISOLatin2(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToISOLatin2Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToDOSLatin2(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromDOSLatin2(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToDOSLatin2Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToWinLatin2(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromWinLatin2(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToWinLatin2Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToMacCentEuro(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromMacCentEuro(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToMacCentEuroPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToMacCroatian(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromMacCroatian(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToMacCroatianPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t
__CFToMacRomanian(uint32_t flags, const uint16_t *characters, OCFIndex numChars, uint8_t *byte);

OF_EXTERN uint32_t __CFFromMacRomanian(uint32_t flags, uint8_t byte, uint16_t *characters);

OF_EXTERN OCFIndex __CFToMacRomanianLen(uint32_t flags, const uint16_t *characters, OCFIndex numChars);

OF_EXTERN OCFIndex __CFFromMacRomanianLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN OCFIndex __CFToMacRomanianPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN const StringEncodingConverterRecord __CFConverterISOLatin2;

OF_EXTERN const StringEncodingConverterRecord __CFConverterDOSLatin2;

OF_EXTERN const StringEncodingConverterRecord __CFConverterWinLatin2;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacCentEuro;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacCroatian;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacRomanian;

// Native __CFIsValidCombiningCharacterForLatin2, 0x1c02f26d4 (non-external, defined in this unit).
OF_EXTERN bool __CFIsValidCombiningCharacterForLatin2(uint16_t character);
