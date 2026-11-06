//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

OF_EXPORT bool OCFStringEncodingIsValidCombiningCharacterForLatin1(uint16_t character);

OF_EXPORT uint16_t
OCFStringEncodingPrecomposeLatinCharacter(const uint16_t *character, OCFIndex numChars, OCFIndex *usedChars);

OF_EXPORT const uint16_t __OCFIdempotentCharToUniCharTable[256];

OF_EXPORT const uint16_t __OCFMacRomanCharToUnicharTable[256];

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN uint32_t __CFToASCII(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromASCII(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN uint32_t __CFToISOLatin1(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromISOLatin1(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToISOLatin1Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToMacRoman(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromMacRoman(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToMacRomanPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToWinLatin1(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromWinLatin1(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToWinLatin1Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToNextStepLatin(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromNextStepLatin(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToNextStepLatinPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToISOLatin6(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromISOLatin6(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToISOLatin6Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToISOLatin7(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromISOLatin7(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToISOLatin7Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToISOLatin8(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromISOLatin8(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToISOLatin8Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToISOLatin9(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromISOLatin9(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToISOLatin9Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToMacIcelandic(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromMacIcelandic(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToMacIcelandicPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToDOSIcelandic(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromDOSIcelandic(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToDOSIcelandicPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToDOSBalticRim(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromDOSBalticRim(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToDOSBalticRimPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToWindowsBalticRim(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromWindowsBalticRim(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToWindowsBalticRimPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToMacRomanLatin1(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromMacRomanLatin1(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToMacRomanLatin1Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToDOSLatinUS(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromDOSLatinUS(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToDOSLatinUSPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToDOSLatin1(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromDOSLatin1(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToDOSLatin1Precompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN const StringEncodingConverterRecord __CFConverterASCII;

OF_EXTERN const StringEncodingConverterRecord __CFConverterISOLatin1;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacRoman;

OF_EXTERN const StringEncodingConverterRecord __CFConverterWinLatin1;

OF_EXTERN const StringEncodingConverterRecord __CFConverterNextStepLatin;

OF_EXTERN const StringEncodingConverterRecord __CFConverterISOLatin6;

OF_EXTERN const StringEncodingConverterRecord __CFConverterISOLatin7;

OF_EXTERN const StringEncodingConverterRecord __CFConverterISOLatin8;

OF_EXTERN const StringEncodingConverterRecord __CFConverterISOLatin9;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacIcelandic;

OF_EXTERN const StringEncodingConverterRecord __CFConverterDOSIcelandic;

OF_EXTERN const StringEncodingConverterRecord __CFConverterDOSBalticRim;

OF_EXTERN const StringEncodingConverterRecord __CFConverterWindowsBalticRim;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacRomanLatin1;

OF_EXTERN const StringEncodingConverterRecord __CFConverterDOSLatinUS;

OF_EXTERN const StringEncodingConverterRecord __CFConverterDOSLatin1;

// Native __CFIsValidCombiningCharacterForLatinSupp, 0x1c019f0e0 (non-external), defined in this unit.
OF_EXTERN bool __CFIsValidCombiningCharacterForLatinSupp(uint16_t character);
