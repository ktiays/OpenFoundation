//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Runtime/OCFRuntime.h"

// The library-loading unit (CFLibraryLoading.c, the file its assertions name), iOS Simulator
// 0x1c02ace1c-0x1c02acf20, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. Parameter names are local descriptive
// spellings.

// The natively exported _CFGetHandleForInsertedOrInterposingLibrary is declared in Runtime/OCFRuntime.h.

// Hidden (were private externals; original names are preserved).
OF_EXTERN Boolean _CFIsMatchingLibrary(const char *imagePath, const char *namePrefix);
OF_EXTERN void *_CFGetHandleForLoadedLibrary(const char *namePrefix);
