//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFStringEncodingNames.h"

#include "CoreFoundation/StringEncoding/OCFStringEncodingDatabase.h"

#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"

#include <os/lock.h>
#include <strings.h>

// Inferred organization: CFStringConvertIANACharSetNameToEncoding (0x1c0233f00) sits between
// CFStringGetNameOfEncoding and CFStringConvertEncodingToIANACharSetName in the iOS Simulator __text section,
// in the native encoding-name unit whose CFStringConvertEncodingToNSStringEncoding (0x1c02340fc) is restored
// in StringEncoding/OCFStringEncodings.c; the coordinator may fold this file into that unit. The native
// source path is not in the evidence. iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified; iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationString.md.

// libsystem_platform SPI. The assembly passes the data synchronization option (0x10000).
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

// Native CFStringIsEncodingAvailable, 0x1c0233de8. External. iOS Device divergence: unread.
Boolean OCFStringIsEncodingAvailable(OCFStringEncoding encoding) {
    return CFStringEncodingIsValidEncoding(encoding);
}

// Native CFStringGetNameOfEncoding.mappingTable and .mappingTableLock (__DATA,__bss 0x22e6d3228 and
// 0x22e6d3230): the cache of names by encoding (NULL key callbacks, CFType values) and its lock.
static OCFMutableDictionaryRef nameOfEncodingTable;
static os_unfair_lock nameOfEncodingTableLock = OS_UNFAIR_LOCK_INIT;

// Native CFStringGetNameOfEncoding, 0x1c0233e00. External. The cached name, else the converter's name
// (__CFStringEncodingCopyNameAsCF), cached by the first thread to store it; NULL for an unnamed encoding.
OCFStringRef OCFStringGetNameOfEncoding(OCFStringEncoding encoding) {
    os_unfair_lock_lock(&nameOfEncodingTableLock);
    if (nameOfEncodingTable != NULL) {
        OCFStringRef cached = OCFDictionaryGetValue(nameOfEncodingTable, (const void *)(uintptr_t)encoding);
        os_unfair_lock_unlock(&nameOfEncodingTableLock);
        if (cached != NULL) {
            return cached;
        }
    } else {
        os_unfair_lock_unlock(&nameOfEncodingTableLock);
    }
    OCFStringRef name = __CFStringEncodingCopyNameAsCF(encoding);
    if (name == NULL) {
        return NULL;
    }
    os_unfair_lock_lock(&nameOfEncodingTableLock);
    OCFStringRef result;
    if (nameOfEncodingTable != NULL &&
        (result = OCFDictionaryGetValue(nameOfEncodingTable, (const void *)(uintptr_t)encoding)) != NULL) {
        os_unfair_lock_unlock(&nameOfEncodingTableLock);
        OCFRelease(name);
        return result;
    }
    if (nameOfEncodingTable == NULL) {
        nameOfEncodingTable = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, NULL, &kOCFTypeDictionaryValueCallBacks
        );
    }
    OCFDictionaryAddValue(nameOfEncodingTable, (const void *)(uintptr_t)encoding, name);
    os_unfair_lock_unlock(&nameOfEncodingTableLock);
    OCFRelease(name);
    return name;
}

// Native CFStringConvertIANACharSetNameToEncoding, 0x1c0233f00. External. The name is read through
// CFStringGetCStringPtr in the default eight-bit encoding or copied with CFStringGetCString into a 100-byte
// stack buffer (a failed copy answers 0xffffffff); it is looked up as a canonical name, then as an ICU name;
// a name starting with "ms950" (case-insensitive, five characters) answers the canonical lookup of "cp950".
OCFStringEncoding OCFStringConvertIANACharSetNameToEncoding(OCFStringRef theString) {
    char buffer[100];
    OCFStringEncoding eightBitEncoding = __OCFDefaultEightBitStringEncoding;
    if (eightBitEncoding == 0xffffffff) {
        eightBitEncoding = __OCFStringComputeEightBitStringEncoding();
    }
    const char *name = OCFStringGetCStringPtr(theString, eightBitEncoding);
    if (name == NULL) {
        OCFStringEncoding copyEncoding = __OCFDefaultEightBitStringEncoding;
        if (copyEncoding == 0xffffffff) {
            copyEncoding = __OCFStringComputeEightBitStringEncoding();
        }
        name = buffer;
        if (!OCFStringGetCString(theString, buffer, 100, copyEncoding)) {
            return 0xffffffff;
        }
    }
    OCFStringEncoding encoding = __CFStringEncodingGetFromCanonicalName(name);
    if (encoding != 0xffffffff) {
        return encoding;
    }
    encoding = __CFStringEncodingGetFromICUName(name);
    if (encoding != 0xffffffff) {
        return encoding;
    }
    if (strncasecmp(name, "ms950", 5) == 0) {
        return __CFStringEncodingGetFromCanonicalName("cp950");
    }
    return 0xffffffff;
}

// Native CFStringConvertEncodingToIANACharSetName, 0x1c0233ffc. External. iOS Device divergence: unread.
OCFStringRef OCFStringConvertEncodingToIANACharSetName(OCFStringEncoding encoding) {
    static os_unfair_lock lock = OS_UNFAIR_LOCK_INIT;
    static OCFMutableDictionaryRef mappingTable;
    const void *key = (const void *)(uintptr_t)encoding;
    char name[100];

    os_unfair_lock_lock_with_options(&lock, 0x10000);
    OCFStringRef value = mappingTable == NULL ? NULL : OCFDictionaryGetValue(mappingTable, key);
    if (value == NULL && __CFStringEncodingGetCanonicalName(encoding, name, sizeof(name))) {
        value = OCFStringCreateWithCString(NULL, name, kOCFStringEncodingASCII);
        if (value != NULL) {
            if (mappingTable == NULL) {
                mappingTable = OCFDictionaryCreateMutable(NULL, 0, NULL, &kOCFTypeDictionaryValueCallBacks);
            }
            OCFDictionaryAddValue(mappingTable, key, value);
            OCFRelease(value);
        }
    }
    os_unfair_lock_unlock(&lock);
    return value;
}
