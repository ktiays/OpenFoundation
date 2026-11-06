//
//  Created by ktiays on 2026/11/05.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/String/OCFString.h"

#include <stdint.h>

// The CFString UText provider (String/OCFStringUText.c), iOS Simulator 24A434, arm64, CoreFoundation UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device 24A446: unread.

// ICU's UText (libicucore, supplied; the SDK installs no ICU headers), declared with the layout the native
// provider addresses: providerProperties at 8, chunkNativeLimit at 16, nativeIndexingLimit at 28,
// chunkNativeStart at 32, chunkOffset at 40, chunkLength at 44, chunkContents at 48, pFuncs at 56, context at
// 72, p at 80, q at 88 and c at 124. The member names are ICU's; the struct name is local.
typedef struct ProviderUText {
    uint32_t magic;
    int32_t flags;
    int32_t providerProperties;
    int32_t sizeOfStruct;
    int64_t chunkNativeLimit;
    int32_t extraSize;
    int32_t nativeIndexingLimit;
    int64_t chunkNativeStart;
    int32_t chunkOffset;
    int32_t chunkLength;
    const uint16_t *chunkContents;
    const void *pFuncs;
    void *pExtra;
    const void *context;
    const void *p;
    const void *q;
    const void *r;
    void *privP;
    int64_t a;
    int32_t b;
    int32_t c;
    int64_t privA;
    int32_t privB;
    int32_t privC;
} ProviderUText;

_Static_assert(__builtin_offsetof(ProviderUText, context) == 72, "UText context is at 72.");
_Static_assert(__builtin_offsetof(ProviderUText, c) == 124, "UText c is at 124.");

// Native CFStringOpenUText, 0x1c0232fd8. External (no SDK declaration). A UText over the string (NULL for a
// NULL string), set up in ut or allocated by ICU.
OF_EXPORT ProviderUText *OCFStringOpenUText(ProviderUText *ut, OCFStringRef string);

// Native __CFStringUTextInit, 0x1c0232fe4. Hidden (was a private external). Provider properties of 0x10 or
// more select the attributed-string functions.
OF_EXTERN ProviderUText *__CFStringUTextInit(ProviderUText *ut, uint32_t properties, const void *string);
