//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/PlugIn/OCFBundle.h"

#include <stdint.h>

// The Info.plist unit (CFBundle_InfoPlist.c, the source path its functions log and assert with), iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC, with the Info.plist key validation
// (0x1c027fea4) and the file-name splitter (0x1c02f43f0). Prototypes follow the native register use;
// parameter names are local descriptive spellings.

// The Info.plist key validation, iOS Simulator 0x1c027fea4-0x1c027ff98. Hidden (was private external); the
// original name is preserved. The native parameter is compared with CFEqual only.
OF_EXTERN Boolean _CFBundleInfoKeyIsValidSpecialCase(OCFStringRef key);

// Splits a resource file name. noProductOrPlatform receives the name without its "-platform"/"~product"
// part (outcomes 2, 3 and 4); the function never reads its seventh argument (W6, which its callers set to 1
// or 2); outcome points to a 32-bit result.
OF_EXTERN void _CFBundleSplitFileName(
    OCFStringRef fileName,
    OCFStringRef *noProductOrPlatform,
    OCFStringRef *endType,
    OCFStringRef *startType,
    OCFStringRef expectedProduct,
    OCFStringRef expectedPlatform,
    uint8_t searchForProductAndPlatform,
    int32_t *outcome
);

// Natively exported.
OF_EXPORT OCFStringRef _OCFGetProductName(void);
OF_EXPORT UInt32 _OCFVersionNumberFromString(OCFStringRef versStr);

// Hidden (were private externals; original names are preserved).
OF_EXTERN OCFStringRef _CFBundleGetProductNameSuffix(void);
OF_EXTERN OCFStringRef _CFBundleGetPlatformNameSuffix(void);
OF_EXTERN Boolean _CFBundleSupportedPlatformName(OCFStringRef fileName, OCFRange searchRange);
OF_EXTERN Boolean _CFBundleSupportedProductName(OCFStringRef fileName, OCFRange searchRange);
// Reads only X0, the dictionary (one native caller also sets W1 to 0).
OF_EXTERN void _CFBundleInfoPlistProcessInfoDictionary(OCFMutableDictionaryRef dict);
// X0 the allocator, X1 the directory URL, X2 the address of the one-byte layout version it stores.
OF_EXTERN OCFDictionaryRef
_CFBundleCopyInfoDictionaryInDirectory(OCFAllocatorRef allocator, OCFURLRef url, uint8_t *version);
// Reads only X0, the data.
OF_EXTERN OCFMutableDictionaryRef _CFBundleCreateInfoPlistWithData(OCFDataRef data);
// Called with the lock at +104 held.
OF_EXTERN void _CFBundleRefreshInfoDictionaryAlreadyLocked(OCFBundleRef bundle);
