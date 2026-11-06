//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/PlugIn/OCFBundleInfoPlist.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/PropertyList/OCFPropertyList.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

// File-type sniffing helpers of _CFBundleGrokFileType and _CFBundleGrokMachTypeForFatFile, iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. Parameter widths and signedness follow the native
// register use and comparisons; parameter names are local descriptive spellings. `length` is the byte
// count of `bytes`, which is read only when `fd` is negative or cannot be positioned (or, for the
// image-information words, cannot be read). Flags are tested as 32-bit registers (Darwin arm64 widens an
// 8-bit argument to 32 bits); every native caller passes 0 or 1.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
// Original non-external names are preserved; no process-shared symbol is exported.
OF_EXTERN const char *
_CFBundleGrokFileTypeForZipFile(int fd, const void *bytes, size_t length, off_t fileLength);

OF_EXTERN const char *_CFBundleGrokFileTypeForOLEFile(int fd, const void *bytes, size_t length, off_t offset);

OF_EXTERN uint8_t _CFBundleGrokX11FromFile(
    int fd, const void *bytes, size_t length, uint32_t offset, uint8_t swapped, uint8_t sixtyFour
);

OF_EXTERN void _CFBundleGrokObjcImageInfoFromFile(
    int fd,
    const void *bytes,
    size_t length,
    uint32_t offset,
    uint8_t swapped,
    uint8_t sixtyFour,
    uint8_t *segmentFound,
    uint32_t *firstInfoWord,
    uint32_t *secondInfoWord
);

// The CFBundle_Grok.c functions that read Info.plist data and classify executables, iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. Parameter widths follow the native register use; names are local
// descriptive spellings. _CFBundleGrokBinaryType is natively exported; the others keep their native
// non-external (or private-external) names. Reading:
// Documentation/Reference/Research/CoreFoundationBundleGrok.md.
OF_EXTERN OCFMutableDictionaryRef _CFBundleCreateInfoDictFromMainExecutable(void);

OF_EXTERN const UInt8 *_CFBundleGetSectData(const char *segname, const char *sectname, uint64_t *size);

OF_EXTERN OCFMutableDictionaryRef _CFBundleCreateInfoDictFromData(const UInt8 *bytes, uint32_t length);

OF_EXTERN Boolean _CFBundleGrokFileType(
    OCFURLRef url,
    OCFDataRef data,
    OCFStringRef *extension,
    UInt32 *machtype,
    OCFArrayRef *architectures,
    OCFDictionaryRef *infodict,
    uint8_t *hasObjc,
    uint32_t *objcVersion,
    uint32_t *objcFlags
);

OF_EXPORT UInt32 _OCFBundleGrokBinaryType(OCFURLRef executableURL);

OF_EXTERN OCFMutableDictionaryRef _CFBundleCreateInfoDictFromFile(
    int fd, const void *bytes, size_t length, uint32_t offset, uint8_t swapped, uint8_t sixtyFour
);

OF_EXTERN UInt32 _CFBundleGrokMachTypeForFatFile(
    int fd,
    const void *bytes,
    size_t length,
    uint8_t swapped,
    uint8_t *isX11,
    OCFArrayRef *architectures,
    OCFDictionaryRef *infodict,
    uint8_t *hasObjc,
    uint32_t *objcVersion,
    uint32_t *objcFlags
);

OF_EXTERN Boolean _CFBundleGrokObjCImageInfoFromMainExecutable(uint32_t *objcVersion, uint32_t *objcFlags);

OF_EXTERN OCFArrayRef _CFBundleCopyArchitecturesForExecutable(OCFURLRef url);

OF_EXTERN Boolean _CFBundleGetObjCImageInfo(OCFBundleRef bundle, uint32_t *objcVersion, uint32_t *objcFlags);
