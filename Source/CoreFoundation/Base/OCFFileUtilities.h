//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <stdbool.h>
#include <stdint.h>

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/URL/OCFURL.h"
#include "CoreFoundation/String/OCFString.h"

// Original names are preserved. All three were private externals and are non-external in the pinned
// image; no process-shared symbol is exported. Paths are UTF-16 unit buffers.
OF_EXTERN uint16_t _CFGetSlash(void);

OF_EXTERN OCFIndex _CFStartOfLastPathComponent(const uint16_t *unichars, OCFIndex length);

OF_EXTERN OCFIndex _CFLengthAfterDeletingLastPathComponent(const uint16_t *unichars, OCFIndex length);

OF_EXTERN bool _CFCreateDirectory(const char *path);

// Native _CFReadBytesFromFile, 0x1c019c36c, and _CFGetFileProperties, 0x1c019cc60; both were private
// externals. extraOpenFlags are the open(2) flags.
OF_EXTERN bool _CFReadBytesFromFile(
    OCFAllocatorRef alloc,
    OCFURLRef url,
    void **bytes,
    OCFIndex *length,
    OCFIndex maxLength,
    int32_t extraOpenFlags
);

// Native _CFWriteBytesToFile, 0x1c019c5f4. Hidden (was a private external).
OF_EXTERN bool _CFWriteBytesToFile(OCFURLRef url, const void *bytes, size_t length);

OF_EXTERN SInt32 _CFGetFileProperties(
    OCFAllocatorRef allocator,
    OCFURLRef url,
    Boolean *exists,
    SInt32 *posixMode,
    SInt64 *size,
    OCFTypeRef *modificationTime,
    SInt32 *ownerID,
    OCFArrayRef *directoryContents
);

// matchingAbstractType selects names by extension; dirSpec (X2) is never read by the native function.
OF_EXTERN OCFMutableArrayRef _CFCreateContentsOfDirectory(
    OCFAllocatorRef alloc, char *dirPath, void *dirSpec, OCFURLRef dirURL, OCFStringRef matchingAbstractType
);

// Path and directory helpers of this unit's range (were private externals; hidden, native names).
OF_EXTERN OCFStringRef _CFGetSlashStr(void);
OF_EXTERN void _CFAppendTrailingPathSlash2(OCFMutableStringRef path);
OF_EXTERN void _CFAppendPathComponent2(OCFMutableStringRef path, OCFStringRef component);
OF_EXTERN Boolean _CFAppendPathExtension2(OCFMutableStringRef path, OCFStringRef extension);
OF_EXTERN OCFStringRef
_CFCreateLastPathComponent(OCFAllocatorRef allocator, OCFStringRef path, OCFIndex *slashIndex);
OF_EXTERN OCFIndex _CFStartOfLastPathComponent2(OCFStringRef path);
OF_EXTERN OCFIndex _CFStartOfPathExtension2(OCFStringRef path);
OF_EXTERN OCFIndex _CFStartOfPathExtension(const UniChar *unichars, OCFIndex length);
OF_EXTERN OCFIndex _CFLengthAfterDeletingPathExtension2(OCFStringRef path);
OF_EXTERN OCFIndex _CFLengthAfterDeletingPathExtension(const UniChar *unichars, OCFIndex length);

// The native function leaves an unrelated value in X0 that no caller reads; restored as void.
OF_EXTERN void _CFIterateDirectory(
    OCFStringRef directoryPath,
    Boolean appendSlashForDirectories,
    OCFArrayRef stuffToPrefix,
    Boolean (^fileHandler)(OCFStringRef fileName, OCFStringRef fileNameWithPrefix, uint8_t fileType)
);
