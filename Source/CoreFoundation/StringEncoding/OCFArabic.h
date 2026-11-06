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
OF_EXTERN uint32_t __CFToISOArabic(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromISOArabic(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToISOArabicPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN uint32_t __CFToWinArabic(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromWinArabic(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToWinArabicPrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN const StringEncodingConverterRecord __CFConverterISOLatinArabic;

OF_EXTERN const StringEncodingConverterRecord __CFConverterWindowsArabic;

// Native __CFIsValidCombiningCharacterForArabic, 0x1c0156fbc (non-external), defined in this unit.
OF_EXTERN bool __CFIsValidCombiningCharacterForArabic(uint16_t character);
