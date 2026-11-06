//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"
#include "CoreFoundation/StringEncoding/OCFPlatformConverter.h"
#include "CoreFoundation/StringEncoding/OCFSingleByte.h"

#include <stdbool.h>
#include <stdint.h>

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN uint32_t __CFToShiftJIS(uint32_t flags, uint16_t character, uint8_t *bytes);

OF_EXTERN uint32_t
__CFFromShiftJIS(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes, uint16_t *character);

OF_EXTERN OCFIndex __CFToShiftJISLen(uint32_t flags, const uint16_t *characters, OCFIndex numChars);

OF_EXTERN OCFIndex __CFFromShiftJISLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN OCFIndex __CFFromShiftJISFallback(
    const uint8_t *bytes, OCFIndex numBytes, uint16_t *characters, OCFIndex maxCharLen, OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToShiftJISPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToDOSJapanese(uint32_t flags, uint16_t character, uint8_t *bytes);

OF_EXTERN uint32_t
__CFFromDOSJapanese(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes, uint16_t *character);

OF_EXTERN uint32_t __CFToEUC_JP(uint32_t flags, uint16_t character, uint8_t *bytes);

OF_EXTERN uint32_t
__CFFromEUC_JP(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes, uint16_t *character);

OF_EXTERN OCFIndex __CFToEUC_JPLen(uint32_t flags, const uint16_t *characters, OCFIndex numChars);

OF_EXTERN OCFIndex __CFFromEUC_JPLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN OCFIndex __CFFromEUC_JPFallback(
    const uint8_t *bytes, OCFIndex numBytes, uint16_t *characters, OCFIndex maxCharLen, OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToEUC_JPPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFToISO_2022_JP(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromISO_2022_JP(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToMacJapanese(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromMacJapanese(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFFromMacJapaneseLen(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

OF_EXTERN OCFIndex __CFToNTTDocomoSJIS(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromNTTDocomoSJIS(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToISO_2022_JP_KDDI(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromISO_2022_JP_KDDI(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToKDDISJIS(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromKDDISJIS(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToSoftBankSJIS(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFFromSoftBankSJIS(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN uint16_t __CFToJIS(uint16_t character, bool restrictToJIS0208);

OF_EXTERN uint16_t __CFFromJIS0208(uint16_t code);

OF_EXTERN OCFIndex __CFToEmojiIndex(const uint16_t *characters, OCFIndex numChars, OCFIndex *usedChars);

OF_EXTERN OCFIndex __CFFromEmojiIndex(OCFIndex index, uint16_t *characters);

OF_EXTERN const StringEncodingConverterRecord __CFConverterShiftJIS;

OF_EXTERN const StringEncodingConverterRecord __CFConverterDOSJapanese;

OF_EXTERN const StringEncodingConverterRecord __CFConverterEUC_JP;

OF_EXTERN const StringEncodingConverterRecord __CFConverterISO_2022_JP;

OF_EXTERN const StringEncodingConverterRecord __CFConverterMacJapanese;

OF_EXTERN const StringEncodingConverterRecord __CFConverterShiftJIS_NTT_Docomo;

OF_EXTERN const StringEncodingConverterRecord __CFConverterISO_2022_JP_KDDI;

OF_EXTERN const StringEncodingConverterRecord __CFConverterShiftJIS_KDDI;

OF_EXTERN const StringEncodingConverterRecord __CFConverterShiftJIS_SoftBank;

// Native JIS0208ToShiftJIS, 0x1c02764d8, and ShiftJISToJIS0208, 0x1c02765bc (non-external; both defined in
// this unit): the code in W0 and the two-byte destination in X1; the two bytes at X0, result in W0.
OF_EXTERN void JIS0208ToShiftJIS(uint16_t code, uint8_t *bytes);

OF_EXTERN uint16_t ShiftJISToJIS0208(const uint8_t *bytes);

// Native __CFPrecomposeCharJapanese, 0x1c027660c (non-external, defined in this unit): the base unit and
// the combining unit at X0, a 32-bit selector in W1 (0 or 1 at every call site) that enables the extended
// katakana compositions; returns the precomposed unit or 0xfffd.
OF_EXTERN uint16_t __CFPrecomposeCharJapanese(const uint16_t *character, bool extendedKatakana);

// Native __CFIsValidCombiningCharJapanese, 0x1c0271b8c, and __CFIsValidCombiningCharJapaneseWithJIS212,
// 0x1c02724b8 (non-external; both defined in this unit).
OF_EXTERN bool __CFIsValidCombiningCharJapanese(uint16_t character);

OF_EXTERN bool __CFIsValidCombiningCharJapaneseWithJIS212(uint16_t character);
