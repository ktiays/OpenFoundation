//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"
#include "CoreFoundation/StringEncoding/OCFUTF8.h"

// The functions of this unit (__CFStrConvertBytesToUnicode, __CFStringDecodeByteStream3,
// __CFStringEncodeByteStream, CFStringCreateWithFileSystemRepresentation,
// CFStringGetMaximumSizeOfFileSystemRepresentation, _CFStringGetUserDefaultEncoding) are declared family-wide
// in String/OCFString.h.

// The character-to-Unicode function of eight-bit contents: native __CFSetCharToUniCharFunc, 0x1c022c1a4 (was
// a private external), before __CFStrConvertBytesToUnicode in this unit's range, and __CFCharToUniCharFunc
// (external data). The function is a cheap eight-bit converter callback; the native call in
// __CFSetCharToUniCharFunc tests the whole W0 (CMP W0, #0), so the result is 32-bit.
typedef uint32_t (*CharToUniCharFunction)(uint32_t flags, uint8_t ch, uint16_t *unicodeChar);
OF_EXTERN void __CFSetCharToUniCharFunc(CharToUniCharFunction function);
OF_EXPORT CharToUniCharFunction __OCFCharToUniCharFunc;
