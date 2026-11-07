//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// CoreFoundation definitions the wasm port supplies in place of trimmed or unrestored native code
// (Port/Trim.tsv; owner decision, 2026-11-07: a trimmed part that turns out to be needed gets a simple
// implementation).
//
// - CFPreferences: the cfprefsd client is trimmed. Preferences live in the process: a dictionary per
// application
//   identifier, empty at launch, which CFPreferencesSetAppValue changes and the readers consult.
// - CoreServicesInternal's file-reference, resource-property and bookmark interfaces (an unsupported cut on
//   Apple builds too, LinkDependencies.json): every one fails, as a call into an absent image would.
// The preference interfaces are spelled with their native names: the port defines several that no restored
// declaration names, so the declaration-derived mapping does not cover them.
//
// - Constants of families that are not restored yet; their string values are their own names (values not
// read).
// - CFDataGetBytes, which consumers call (OpenCoreGraphics, OpenCoreAnimation) and no restored source defines: the
//   bytes of the range copied from CFDataGetBytePtr. Spelled with its native name, as no restored declaration
//   names it.
// - CFAttributedStringCreate, which no restored source defines (OpenCoreAnimation's OCATextLayer calls it): a
//   mutable attributed string given the string and the attributes over its whole length, returned as an immutable
//   copy, from the restored CFAttributedString functions. Native name, as above.

#include "CoreFoundation/Base/OCFBase.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/String/OCFAttributedString.h"
#include "CoreFoundation/Notification/OCFNotificationCenter.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <os/lock.h>
#include <stdlib.h>
#include <string.h>

// A constant string usable in a static initializer (OF_STR is a statement expression).
#define PORT_CONSTANT_STRING(literal) ((OCFStringRef)__builtin___CFStringMakeConstantString("" literal ""))

#pragma mark - Preferences

const OCFStringRef kCFPreferencesAnyApplication = PORT_CONSTANT_STRING("kCFPreferencesAnyApplication");
const OCFStringRef kCFPreferencesCurrentApplication =
    PORT_CONSTANT_STRING("kCFPreferencesCurrentApplication");
const OCFStringRef kCFPreferencesAnyHost = PORT_CONSTANT_STRING("kCFPreferencesAnyHost");
const OCFStringRef kCFPreferencesCurrentHost = PORT_CONSTANT_STRING("kCFPreferencesCurrentHost");
const OCFStringRef kCFPreferencesAnyUser = PORT_CONSTANT_STRING("kCFPreferencesAnyUser");
const OCFStringRef kCFPreferencesCurrentUser = PORT_CONSTANT_STRING("kCFPreferencesCurrentUser");

static os_unfair_lock gPreferencesLock = OS_UNFAIR_LOCK_INIT;
static OCFMutableDictionaryRef gPreferencesByApplication;

static OCFMutableDictionaryRef preferencesForApplication(OCFStringRef application, bool create) {
    if (gPreferencesByApplication == NULL) {
        if (!create) {
            return NULL;
        }
        gPreferencesByApplication = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    OCFMutableDictionaryRef preferences =
        (OCFMutableDictionaryRef)OCFDictionaryGetValue(gPreferencesByApplication, application);
    if (preferences == NULL && create) {
        preferences = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
        OCFDictionarySetValue(gPreferencesByApplication, application, preferences);
        OCFRelease(preferences);
    }
    return preferences;
}

OCFTypeRef CFPreferencesCopyAppValue(OCFStringRef key, OCFStringRef applicationID) {
    if (key == NULL || applicationID == NULL) {
        return NULL;
    }
    os_unfair_lock_lock(&gPreferencesLock);
    OCFMutableDictionaryRef preferences = preferencesForApplication(applicationID, false);
    OCFTypeRef value = preferences != NULL ? OCFDictionaryGetValue(preferences, key) : NULL;
    if (value != NULL) {
        OCFRetain(value);
    }
    os_unfair_lock_unlock(&gPreferencesLock);
    return value;
}

OCFTypeRef CFPreferencesCopyValue(
    OCFStringRef key, OCFStringRef applicationID, OCFStringRef userName, OCFStringRef hostName
) {
    (void)userName;
    (void)hostName;
    return CFPreferencesCopyAppValue(key, applicationID);
}

OCFDictionaryRef CFPreferencesCopyMultiple(
    OCFArrayRef keysToFetch, OCFStringRef applicationID, OCFStringRef userName, OCFStringRef hostName
) {
    (void)userName;
    (void)hostName;
    OCFMutableDictionaryRef result = OCFDictionaryCreateMutable(
        kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    os_unfair_lock_lock(&gPreferencesLock);
    OCFMutableDictionaryRef preferences = preferencesForApplication(applicationID, false);
    if (preferences != NULL) {
        if (keysToFetch == NULL) {
            OCFIndex count = OCFDictionaryGetCount(preferences);
            const void **keys = malloc((size_t)(count > 0 ? count : 1) * sizeof(void *));
            const void **values = malloc((size_t)(count > 0 ? count : 1) * sizeof(void *));
            OCFDictionaryGetKeysAndValues(preferences, keys, values);
            for (OCFIndex index = 0; index < count; index++) {
                OCFDictionarySetValue(result, keys[index], values[index]);
            }
            free(keys);
            free(values);
        } else {
            for (OCFIndex index = 0; index < OCFArrayGetCount(keysToFetch); index++) {
                const void *key = OCFArrayGetValueAtIndex(keysToFetch, index);
                const void *value = OCFDictionaryGetValue(preferences, key);
                if (value != NULL) {
                    OCFDictionarySetValue(result, key, value);
                }
            }
        }
    }
    os_unfair_lock_unlock(&gPreferencesLock);
    return result;
}

void CFPreferencesSetAppValue(OCFStringRef key, OCFTypeRef value, OCFStringRef applicationID) {
    if (key == NULL || applicationID == NULL) {
        return;
    }
    os_unfair_lock_lock(&gPreferencesLock);
    OCFMutableDictionaryRef preferences = preferencesForApplication(applicationID, true);
    if (value != NULL) {
        OCFDictionarySetValue(preferences, key, value);
    } else {
        OCFDictionaryRemoveValue(preferences, key);
    }
    os_unfair_lock_unlock(&gPreferencesLock);
}

Boolean CFPreferencesAppSynchronize(OCFStringRef applicationID) {
    (void)applicationID;
    return true;
}

Boolean CFPreferencesGetAppBooleanValue(
    OCFStringRef key, OCFStringRef applicationID, Boolean *keyExistsAndHasValidFormat
) {
    Boolean result = false;
    Boolean valid = false;
    OCFTypeRef value = CFPreferencesCopyAppValue(key, applicationID);
    if (value != NULL) {
        if (OCFGetTypeID(value) == OCFBooleanGetTypeID()) {
            result = OCFBooleanGetValue(value);
            valid = true;
        } else if (OCFGetTypeID(value) == OCFNumberGetTypeID()) {
            int number = 0;
            OCFNumberGetValue(value, kOCFNumberIntType, &number);
            result = number != 0;
            valid = true;
        } else if (OCFGetTypeID(value) == OCFStringGetTypeID()) {
            if (OCFStringCompare(value, OF_STR("YES"), kOCFCompareCaseInsensitive) == kOCFCompareEqualTo ||
                OCFStringCompare(value, OF_STR("true"), kOCFCompareCaseInsensitive) == kOCFCompareEqualTo) {
                result = true;
                valid = true;
            } else if (
                OCFStringCompare(value, OF_STR("NO"), kOCFCompareCaseInsensitive) == kOCFCompareEqualTo ||
                OCFStringCompare(value, OF_STR("false"), kOCFCompareCaseInsensitive) == kOCFCompareEqualTo
            ) {
                valid = true;
            }
        }
        OCFRelease(value);
    }
    if (keyExistsAndHasValidFormat != NULL) {
        *keyExistsAndHasValidFormat = valid;
    }
    return result;
}

OCFIndex CFPreferencesGetAppIntegerValue(
    OCFStringRef key, OCFStringRef applicationID, Boolean *keyExistsAndHasValidFormat
) {
    OCFIndex result = 0;
    Boolean valid = false;
    OCFTypeRef value = CFPreferencesCopyAppValue(key, applicationID);
    if (value != NULL) {
        if (OCFGetTypeID(value) == OCFNumberGetTypeID()) {
            valid = OCFNumberGetValue(value, kOCFNumberCFIndexType, &result);
        } else if (OCFGetTypeID(value) == OCFStringGetTypeID()) {
            result = OCFStringGetIntValue(value);
            valid = true;
        }
        OCFRelease(value);
    }
    if (keyExistsAndHasValidFormat != NULL) {
        *keyExistsAndHasValidFormat = valid;
    }
    return result;
}

#pragma mark - CoreServicesInternal (unsupported)

OCFStringRef
_FSCreatePathForFileID(OCFAllocatorRef allocator, long long volumeID, long long fileID, void *unread) {
    (void)allocator;
    (void)volumeID;
    (void)fileID;
    (void)unread;
    return NULL;
}

Boolean _FSGetFileSystemRepresentationForFileID(
    OCFAllocatorRef allocator,
    long long volumeID,
    long long fileID,
    UInt8 *buffer,
    OCFIndex length,
    void *unread
) {
    (void)allocator;
    (void)volumeID;
    (void)fileID;
    (void)buffer;
    (void)length;
    (void)unread;
    return false;
}

void _FSURLBeginResourcePropertyCacheAccess(OCFURLRef url, void *resourceInfo) {
    (void)url;
    (void)resourceInfo;
}

void _FSURLClearResourcePropertyCacheForKey(OCFURLRef url, OCFStringRef key, void *resourceInfo) {
    (void)url;
    (void)key;
    (void)resourceInfo;
}

OCFDictionaryRef
_FSURLCopyResourcePropertiesForKeys(OCFURLRef url, OCFArrayRef keys, void *resourceInfo, OCFErrorRef *error) {
    (void)url;
    (void)keys;
    (void)resourceInfo;
    if (error != NULL) {
        *error = NULL;
    }
    return NULL;
}

Boolean _FSURLCopyResourcePropertyForKey(
    OCFURLRef url, OCFStringRef key, void *propertyValueTypeRefPtr, void *resourceInfo, OCFErrorRef *error
) {
    (void)url;
    (void)key;
    (void)resourceInfo;
    if (propertyValueTypeRefPtr != NULL) {
        *(OCFTypeRef *)propertyValueTypeRefPtr = NULL;
    }
    if (error != NULL) {
        *error = NULL;
    }
    return false;
}

void _FSURLEndResourcePropertyCacheAccess(OCFURLRef url, void *resourceInfo) {
    (void)url;
    (void)resourceInfo;
}

Boolean _FSURLSetResourcePropertiesForKeys(
    OCFURLRef url, OCFDictionaryRef keyedPropertyValues, void *resourceInfo, OCFErrorRef *error
) {
    (void)url;
    (void)keyedPropertyValues;
    (void)resourceInfo;
    if (error != NULL) {
        *error = NULL;
    }
    return false;
}

Boolean _FSURLSetResourcePropertyForKey(
    OCFURLRef url,
    OCFStringRef key,
    OCFTypeRef propertyValue,
    Boolean setFlag,
    void *resourceInfo,
    OCFErrorRef *error
) {
    (void)url;
    (void)key;
    (void)propertyValue;
    (void)setFlag;
    (void)resourceInfo;
    if (error != NULL) {
        *error = NULL;
    }
    return false;
}

OCFDataRef _CFURLCreateBookmarkData(
    OCFAllocatorRef allocator,
    OCFURLRef url,
    OCFOptionFlags options,
    OCFArrayRef resourcePropertiesToInclude,
    OCFURLRef relativeToURL,
    OCFErrorRef *error
) {
    (void)allocator;
    (void)url;
    (void)options;
    (void)resourcePropertiesToInclude;
    (void)relativeToURL;
    if (error != NULL) {
        *error = NULL;
    }
    return NULL;
}

OCFDataRef
_CFURLCreateBookmarkDataFromFile(OCFAllocatorRef allocator, OCFURLRef fileURL, OCFErrorRef *errorRef) {
    (void)allocator;
    (void)fileURL;
    if (errorRef != NULL) {
        *errorRef = NULL;
    }
    return NULL;
}

OCFDictionaryRef _CFURLCreateResourcePropertiesForKeysFromBookmarkData(
    OCFAllocatorRef allocator, OCFArrayRef resourcePropertiesToReturn, OCFDataRef bookmark
) {
    (void)allocator;
    (void)resourcePropertiesToReturn;
    (void)bookmark;
    return NULL;
}

int _CFURLStartAccessingSecurityScopedResource(OCFURLRef url) {
    (void)url;
    return 0;
}

void _CFURLStopAccessingSecurityScopedResource(OCFURLRef url) {
    (void)url;
}

Boolean _CFURLWriteBookmarkDataToFile(
    OCFDataRef bookmarkRef, OCFURLRef fileURL, OCFOptionFlags options, OCFErrorRef *errorRef
) {
    (void)bookmarkRef;
    (void)fileURL;
    (void)options;
    if (errorRef != NULL) {
        *errorRef = NULL;
    }
    return false;
}

#pragma mark - CFData

void CFDataGetBytes(OCFDataRef theData, OCFRange range, UInt8 *buffer) {
    if (range.length <= 0) {
        return;
    }
    memmove(buffer, OCFDataGetBytePtr(theData) + range.location, (size_t)range.length);
}

#pragma mark - CFAttributedString

OCFAttributedStringRef CFAttributedStringCreate(OCFAllocatorRef alloc, OCFStringRef str, OCFDictionaryRef attributes) {
    OCFMutableAttributedStringRef mutableString = OCFAttributedStringCreateMutable(alloc, 0);
    OCFAttributedStringReplaceString(mutableString, (OCFRange){0, 0}, str);
    OCFIndex length = OCFAttributedStringGetLength(mutableString);
    if (attributes != NULL && length > 0) {
        OCFAttributedStringSetAttributes(mutableString, (OCFRange){0, length}, attributes, true);
    }
    OCFAttributedStringRef result = OCFAttributedStringCreateCopy(alloc, mutableString);
    OCFRelease(mutableString);
    return result;
}

#pragma mark - Constants of unrestored families

// kCFErrorDescriptionKey: the error description family is not restored (its value not read).
const OCFStringRef kCFErrorDescriptionKey = PORT_CONSTANT_STRING("kCFErrorDescriptionKey");

const OCFStringRef _kOCFStringFormatMetadataArgumentNumberKey =
    PORT_CONSTANT_STRING("_kCFStringFormatMetadataArgumentNumberKey");

// The custom local center configuration is not restored; a zeroed configuration describes no behavior.
const NotificationConfiguration _kOCFXNotificationConfigurationCustomLocalConfiguration;
