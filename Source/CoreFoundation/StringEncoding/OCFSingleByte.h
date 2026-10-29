//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>

#include <stdint.h>

OF_EXTERN uint8_t __CFToASCII(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint8_t __CFFromASCII(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN uint8_t __CFToISOLatin1(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint8_t __CFFromISOLatin1(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN uint8_t __CFToWinLatin1(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint8_t __CFFromWinLatin1(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN uint8_t __CFToNextStepLatin(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint8_t __CFFromNextStepLatin(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN uint8_t __CFToMacRomanLatin1(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint8_t __CFFromMacRomanLatin1(uint32_t flags, uint8_t byte, uint16_t *character);

OF_EXTERN uint8_t __CFToDOSLatin1(uint32_t flags, uint16_t character, uint8_t *byte);

OF_EXTERN uint8_t __CFFromDOSLatin1(uint32_t flags, uint8_t byte, uint16_t *character);
