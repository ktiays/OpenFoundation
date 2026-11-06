//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/URL/OCFURLFileUtilities.h"

#include "CoreFoundation/Base/OCFFileUtilities.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Number/OCFNumber.h"

#include <dispatch/dispatch.h>
#include <sys/stat.h>

// Inferred organization: _CFURLExists (0x1c019ce6c) lies in the file-utility code of the iOS Simulator __text
// section, between _CFGetFileProperties (0x1c019cc60) and _CFGetSlash (0x1c019cefc, Base/OCFFileUtilities.c),
// apart from the CFURL unit; it is kept in its own translation unit until the coordinator merges it with
// that unit. iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS
// Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationURL.md (pending; the
// coordinator writes the reading).

// Native _CFURLExists, 0x1c019ce6c. Hidden (was private external). NULL answers false; otherwise
// _CFGetFileProperties with the system default allocator asks only for existence: a nonzero error answers
// false, otherwise whether the file exists.
Boolean _CFURLExists(OCFURLRef url) {
    Boolean exists = false;
    if (url == NULL) {
        return false;
    }
    if (_CFGetFileProperties(kOCFAllocatorSystemDefault, url, &exists, NULL, NULL, NULL, NULL, NULL) != 0) {
        return false;
    }
    return exists != 0;
}

typedef Boolean
CFNetworkURLWriteFunction(OCFURLRef url, OCFDataRef data, OCFDictionaryRef properties, SInt32 *errorCode);

// Native __CFNetwork__CFURLWriteDataAndPropertiesToResource.dyfunc, __DATA,__data 0x22e6cfbe0.
// The resolver overwrites the all-ones sentinel before the first call reads the slot.
static CFNetworkURLWriteFunction *__CFNetwork__CFURLWriteDataAndPropertiesToResource_dyfunc =
    (CFNetworkURLWriteFunction *)(uintptr_t)UINTPTR_MAX;

// Native CFURLWriteDataAndPropertiesToResource, 0x1c0250b40. External. Folds .cold.1, 0x1c030d990.
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC, confidence: verified.
// iOS Device 24A446: unread. Reading: Local/Evidence/cf-core/iphone-simulator-corpus-delta13.json.
// With file properties present, their result overwrites the data operation's error code (kept).
// CoreFoundation defect: the typed allocation at 0x1c0250d7c is unchecked before 0x1c0250d94 writes
// through it (verified). Return the file-operation error on allocation failure.
Boolean OCFURLWriteDataAndPropertiesToResource(
    OCFURLRef url, OCFDataRef data, OCFDictionaryRef properties, SInt32 *errorCode
) {
    OCFStringRef scheme = OCFURLCopyScheme(url);
    if (scheme == NULL) {
        if (errorCode != NULL) {
            *errorCode = -15;
        }
        return false;
    }
    OCFComparisonResult comparison = OCFStringCompare(scheme, OF_STR("file"), 0);
    OCFRelease(scheme);
    if (comparison != kOCFCompareEqualTo) {
        static dispatch_once_t once;
        // Native _____CFNetwork__CFURLWriteDataAndPropertiesToResource_block_invoke, 0x1c025127c.
        // Non-external. The lookup uses CFNetwork's native symbol name in both naming modes.
        dispatch_once(&once, ^{
          __CFNetwork__CFURLWriteDataAndPropertiesToResource_dyfunc =
              __CFLookupCFNetworkFunction("_CFURLWriteDataAndPropertiesToResource");
        });
        Boolean result = false;
        if (__CFNetwork__CFURLWriteDataAndPropertiesToResource_dyfunc != NULL) {
            result =
                __CFNetwork__CFURLWriteDataAndPropertiesToResource_dyfunc(url, data, properties, errorCode);
        }
        if (errorCode != NULL && !result) {
            *errorCode = -11;
        }
        return result;
    }
    if (errorCode != NULL) {
        *errorCode = 0;
    }
    UInt8 path[1026];
    bool wrote = true;
    if (data != NULL) {
        if (OCFURLHasDirectoryPath(url)) {
            if (!OCFURLGetFileSystemRepresentation(url, true, path, sizeof(path))) {
                wrote = false;
                if (errorCode != NULL) {
                    *errorCode = -15;
                }
            } else {
                wrote = _CFCreateDirectory((const char *)path);
                if (!wrote && errorCode != NULL) {
                    *errorCode = -10;
                }
            }
        } else {
            int32_t length = (int32_t)OCFDataGetLength(data);
            const UInt8 *bytes = length != 0 ? OCFDataGetBytePtr(data) : (const UInt8 *)"";
            wrote = _CFWriteBytesToFile(url, bytes, (size_t)(int64_t)length);
            if (!wrote && errorCode != NULL) {
                *errorCode = -10;
            }
        }
    }
    if (properties == NULL) {
        return wrote;
    }
    if (!OCFURLGetFileSystemRepresentation(url, true, path, sizeof(path))) {
        if (errorCode != NULL) {
            *errorCode = -15;
        }
        return false;
    }
    const void *localKeys[8] = {0};
    const void *localValues[8] = {0};
    const void **keys = localKeys;
    const void **values = localValues;
    int32_t count = (int32_t)OCFDictionaryGetCount(properties);
    if (count >= 8) {
        OCFAllocatorRef allocator = OCFGetAllocator(url);
        keys = OCFAllocatorAllocateTyped(allocator, 16 * (OCFIndex)count, 0x80040B8603338, 0);
        if (keys == NULL) {
            if (errorCode != NULL) {
                *errorCode = -10;
            }
            return false;
        }
        values = keys + count;
    }
    OCFDictionaryGetKeysAndValues(properties, keys, values);
    bool setProperties = true;
    OCFStringRef posixMode = OF_STR("kCFURLFilePOSIXMode");
    for (int32_t index = 0; index < count; ++index) {
        if (keys[index] != posixMode && !OCFEqual(posixMode, keys[index])) {
            setProperties = false;
            continue;
        }
        int32_t mode = 0;
        OCFTypeID numberType = OCFNumberGetTypeID();
        if (numberType == OCFGetTypeID(values[index])) {
            OCFNumberGetValue(values[index], kOCFNumberSInt32Type, &mode);
        } else {
            mode = *(const uint16_t *)OCFDataGetBytePtr(values[index]);
        }
        if (chmod((const char *)path, (mode_t)mode) != 0) {
            setProperties = false;
        }
    }
    if (keys != localKeys) {
        OCFAllocatorDeallocate(OCFGetAllocator(url), keys);
    }
    if (errorCode != NULL) {
        *errorCode = setProperties ? 0 : -10;
    }
    return wrote && setProperties;
}
