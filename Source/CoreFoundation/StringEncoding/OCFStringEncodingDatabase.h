//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stdint.h>

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN uint32_t __CFStringEncodingGetWindowsCodePage(uint32_t encoding);

OF_EXTERN bool __CFStringEncodingGetCanonicalName(uint32_t encoding, char *buffer, OCFIndex bufferSize);

OF_EXTERN const char *__CFStringEncodingGetICUName(uint32_t encoding);

OF_EXTERN uint32_t __CFStringEncodingGetFromICUName(const char *icuName);

OF_EXTERN uint32_t __CFStringEncodingGetFromWindowsCodePage(uint32_t codePage);

OF_EXTERN uint32_t __CFStringEncodingGetFromCanonicalName(const char *canonicalName);

// Native __CFCanonicalNameCompare (0x1c02aeeb0) and __CFCanonicalNameHash (0x1c02aeed4), non-external; the
// canonical-name dictionary's key callbacks, defined in this unit.
OF_EXTERN Boolean __CFCanonicalNameCompare(const void *value1, const void *value2);

OF_EXTERN OCFHashCode __CFCanonicalNameHash(const void *value);

// Native __CFStringEncodingGetName (0x1c02aef34) and __CFStringEncodingCopyNameAsCF (0x1c02af0cc), hidden
// (were private externals): the display name of an encoding the converters add, as a C string and as a
// string.
OF_EXTERN const char *__CFStringEncodingGetName(uint32_t encoding);
OF_EXTERN OCFStringRef __CFStringEncodingCopyNameAsCF(uint32_t encoding);
