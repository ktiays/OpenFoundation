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
OF_EXTERN uint32_t __CFToISOLatinHebrew(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromISOLatinHebrew(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN uint32_t __CFToWindowsHebrew(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint32_t __CFFromWindowsHebrew(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN const StringEncodingConverterRecord __CFConverterISOLatinHebrew;

OF_EXTERN const StringEncodingConverterRecord __CFConverterWindowsHebrew;
