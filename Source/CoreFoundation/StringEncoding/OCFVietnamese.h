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
OF_EXTERN uint32_t __CFToWindowsVietnamese(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromWindowsVietnamese(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN OCFIndex __CFToWindowsVietnamesePrecompose(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN const StringEncodingConverterRecord __CFConverterWindowsVietnamese;

// Native __CFIsValidCombiningCharacterForVietnamese, 0x1c0160fdc (non-external), defined in this unit.
OF_EXTERN bool __CFIsValidCombiningCharacterForVietnamese(uint16_t character);
