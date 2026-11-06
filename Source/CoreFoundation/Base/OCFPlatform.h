//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <stdbool.h>
#include <stdint.h>

// Per-thread data table under the reserved pthread key 55: a 1,128-byte calloc block. The first word
// counts finalization passes, then 70 slot values and 70 destructors (one per slot). OCFTSDTable and
// its field names are local descriptive spellings; iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (__CFTSDGetTable, _CFSetTSD,
// __CFTSDFinalize offsets 0, 8 and 0x238).
enum {
    kOCFTSDKey = 55,
    kOCFTSDSlotCount = 70,
};

typedef void (*OCFTSDDestructor)(void *value);

typedef struct {
    uint32_t destructorCount;
    uint32_t padding;
    uintptr_t data[kOCFTSDSlotCount];
    OCFTSDDestructor destructors[kOCFTSDSlotCount];
} OCFTSDTable;

_Static_assert(sizeof(OCFTSDTable) == 0x468, "The native per-thread table is 1,128 bytes.");

// Set when the main thread's per-thread data is finalized.
OF_EXTERN uint8_t __CFMainThreadHasExited;

OF_EXPORT const char *_OCFProcessPath(void);

// Native _CFGetProcessPath, 0x1c01ce124. External.
OF_EXPORT const char **_OCFGetProcessPath(void);

// Returns the address of the cached program name, computing the process path first when it is unset.
OF_EXPORT const char **_OCFGetProgname(void);

OF_EXTERN OCFTSDTable *__CFTSDGetTable(bool create);

OF_EXTERN void __CFTSDFinalize(void *table);

OF_EXPORT void *_OCFGetTSDCreateIfNeeded(uint32_t slot, bool create);

OF_EXPORT void *_OCFGetTSD(uint32_t slot);

// Stores value and destructor in slot and returns the previous value.
OF_EXPORT void *_OCFSetTSD(uint32_t slot, void *value, OCFTSDDestructor destructor);

// Native _CFGetCurrentDirectory, 0x1c01cdf84 (was a private external), before _CFGetProgname in this unit's
// range. Writes the current directory into path (maxlen bytes) and answers whether it could.
OF_EXTERN bool _CFGetCurrentDirectory(char *path, int maxlen);

// The user and search-path interfaces of the platform units (SDK signatures for the public ones; the hidden
// ones from the native register use).
OF_EXPORT OCFStringRef OCFCopyUserName(void);
OF_EXPORT OCFURLRef OCFCopyHomeDirectoryURLForUser(OCFStringRef uName);
OF_EXTERN const char *_CFGetCachedUnsandboxedHomeDirectoryForCurrentUser(void);
// Returns a malloc block the caller owns.
OF_EXTERN const char *_CFUnsandboxedHomeDirectoryForCurrentUser(void);
OF_EXTERN OCFStringRef _CFProcessNameString(void);
OF_EXPORT OCFArrayRef
OCFCopySearchPathForDirectoriesInDomains(OCFIndex directory, OCFIndex domainMask, Boolean expandTilde);
