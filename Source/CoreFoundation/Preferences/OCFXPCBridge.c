//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Preferences/OCFXPCBridge.h"

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Date/OCFDate.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/URL/OCFURL.h"
#include "CoreFoundation/UUID/OCFUUID.h"

#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

// Inferred organization: _CFXPCCreateCFObjectFromXPCMessage (0x1c02cc128, outside this restoration) through
// ____CFXPCCreateCFObjectFromXPCObject_block_invoke_2 (0x1c02cd140) are contiguous in the iOS Simulator
// __text section; the native source path is not in the evidence. The unit uses blocks and no Objective-C, so
// it is kept as C. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationPreferences.md.

// Constants as the native call sites pass them: kCFStringEncodingUTF8, the builtin type IDs of CFString (7),
// CFDictionary (18), CFArray (19), CFNumber (22) and CFDate (42), and kCFNotFound.
#define kUTF8Encoding ((OCFStringEncoding)0x08000100)
#define kStringTypeID ((OCFTypeID)7)
#define kDictionaryTypeID ((OCFTypeID)18)
#define kArrayTypeID ((OCFTypeID)19)
#define kNumberTypeID ((OCFTypeID)22)
#define kDateTypeID ((OCFTypeID)42)
#define kNotFound ((OCFIndex) - 1)

static xpc_object_t _CFXPCCreateXPCObjectFromOutOfRangeCFTimeInterval(OCFTimeInterval interval);
static void __CFXPCDictionaryApplier(const void *key, const void *value, void *context);

// Native _CFXPCCreateXPCObjectFromCFObject, 0x1c02cc190. External. NULL answers NULL. By type: CFNull is
// xpc_null; a boolean is xpc_bool (true only for kCFBooleanTrue); a string is converted to UTF-8 in a heap
// buffer of CFStringGetMaximumSizeForEncoding(length + 1) bytes (NULL when that size is kCFNotFound, the
// allocation fails or the conversion fails); data go through _CFDataCreateDispatchData; a number is read as a
// double when it is a float type and as SInt64 otherwise (NULL when CFNumberGetValue fails); a UUID is
// xpc_uuid of its bytes; a dictionary is an XPC dictionary of its string-keyed entries and an array an XPC
// array of its convertible elements, either answered only when every entry converted (otherwise released,
// NULL); a URL is a three-entry dictionary (magic UUID C3853DCC-9776-4114-B6C1-FD9F51944A6D, string, base
// or CFNull) answered only when it holds three entries. A date's time since 1970 becomes nanoseconds in an
// xpc_date when its magnitude is at most 9223372036.0 seconds (a NaN is out of range), otherwise the
// out-of-range dictionary. Any other type answers NULL.
xpc_object_t _OCFXPCCreateXPCObjectFromCFObject(OCFTypeRef object) {
    if (object == NULL) {
        return NULL;
    }
    OCFTypeID typeID = OCFGetTypeID(object);
    if (OCFNullGetTypeID() == typeID) {
        return xpc_null_create();
    }
    if (OCFBooleanGetTypeID() == typeID) {
        return xpc_bool_create(object == kOCFBooleanTrue);
    }
    if (typeID == kStringTypeID) {
        OCFIndex size = OCFStringGetMaximumSizeForEncoding(OCFStringGetLength(object) + 1, kUTF8Encoding);
        if (size == kNotFound) {
            return NULL;
        }
        char *buffer = malloc_type_malloc(size, 0x100004077774924);
        if (buffer == NULL) {
            return NULL;
        }
        xpc_object_t result = NULL;
        if (OCFStringGetCString(object, buffer, size, kUTF8Encoding)) {
            result = xpc_string_create(buffer);
        }
        free(buffer);
        return result;
    }
    if (OCFDataGetTypeID() == typeID) {
        dispatch_data_t data = _CFDataCreateDispatchData(object);
        xpc_object_t result = xpc_data_create_with_dispatch_data(data);
        dispatch_release(data);
        return result;
    }
    if (typeID == kDateTypeID) {
        OCFAbsoluteTime since1970 = OCFDateGetAbsoluteTime(object) + kOCFAbsoluteTimeIntervalSince1970;
        if (!(fabs(since1970) <= 9223372036.0)) {
            return _CFXPCCreateXPCObjectFromOutOfRangeCFTimeInterval(since1970);
        }
        return xpc_date_create((int64_t)(since1970 * 1000000000.0));
    }
    if (typeID == kNumberTypeID) {
        if (OCFNumberIsFloatType(object)) {
            double value = 0;
            if (OCFNumberGetValue(object, kOCFNumberDoubleType, &value)) {
                return xpc_double_create(value);
            }
        } else {
            int64_t value = 0;
            if (OCFNumberGetValue(object, kOCFNumberSInt64Type, &value)) {
                return xpc_int64_create(value);
            }
        }
        return NULL;
    }
    if (OCFUUIDGetTypeID() == typeID) {
        OCFUUIDBytes bytes = OCFUUIDGetUUIDBytes(object);
        return xpc_uuid_create((const unsigned char *)&bytes);
    }
    xpc_object_t result;
    size_t expected;
    size_t converted;
    if (typeID == kDictionaryTypeID) {
        result = xpc_dictionary_create(NULL, NULL, 0);
        if (result == NULL) {
            return NULL;
        }
        expected = OCFDictionaryGetCount(object);
        OCFDictionaryApplyFunction(object, __CFXPCDictionaryApplier, &result);
        converted = xpc_dictionary_get_count(result);
    } else if (typeID == kArrayTypeID) {
        result = xpc_array_create(NULL, 0);
        if (result == NULL) {
            return NULL;
        }
        OCFIndex count = OCFArrayGetCount(object);
        expected = count;
        for (OCFIndex index = 0; index < count; ++index) {
            xpc_object_t element = _OCFXPCCreateXPCObjectFromCFObject(OCFArrayGetValueAtIndex(object, index));
            if (element != NULL) {
                xpc_array_append_value(result, element);
                xpc_release(element);
            }
        }
        converted = xpc_array_get_count(result);
    } else {
        if (OCFURLGetTypeID() != typeID) {
            return NULL;
        }
        OCFStringRef string = OCFURLGetString(object);
        if (string == NULL) {
            return NULL;
        }
        OCFTypeRef baseURL = OCFURLGetBaseURL(object);
        if (baseURL == NULL) {
            baseURL = kOCFNull;
        }
        xpc_object_t base = _OCFXPCCreateXPCObjectFromCFObject(baseURL);
        if (base == NULL) {
            return NULL;
        }
        xpc_object_t xpcString = _OCFXPCCreateXPCObjectFromCFObject(string);
        if (xpcString == NULL) {
            xpc_release(base);
            return NULL;
        }
        uuid_t magic = {0};
        xpc_object_t magicObject;
        if (uuid_parse("C3853DCC-9776-4114-B6C1-FD9F51944A6D", magic) != 0 ||
            (magicObject = xpc_uuid_create(magic)) == NULL) {
            xpc_release(xpcString);
            xpc_release(base);
            return NULL;
        }
        const char *keys[3] = {"com.apple.CFURL.magic", "com.apple.CFURL.string", "com.apple.CFURL.base"};
        xpc_object_t values[3] = {magicObject, xpcString, base};
        result = xpc_dictionary_create(keys, values, 3);
        xpc_release(xpcString);
        xpc_release(base);
        xpc_release(magicObject);
        if (result == NULL || xpc_dictionary_get_count(result) == 3) {
            return result;
        }
        xpc_release(result);
        return NULL;
    }
    if (converted == expected) {
        return result;
    }
    xpc_release(result);
    return NULL;
}

// Native _CFXPCCreateXPCObjectFromOutOfRangeCFTimeInterval, 0x1c02cc664. Non-external. The interval (seconds
// since 1970) as a two-entry dictionary: magic UUID C119A421-DF31-48EF-843D-21B4030D1621 and the double.
// NULL when the double, the UUID or a two-entry dictionary cannot be made.
static xpc_object_t _CFXPCCreateXPCObjectFromOutOfRangeCFTimeInterval(OCFTimeInterval interval) {
    xpc_object_t value = xpc_double_create(interval);
    if (value == NULL) {
        return NULL;
    }
    uuid_t magic = {0};
    xpc_object_t magicObject;
    if (uuid_parse("C119A421-DF31-48EF-843D-21B4030D1621", magic) != 0 ||
        (magicObject = xpc_uuid_create(magic)) == NULL) {
        xpc_release(value);
        return NULL;
    }
    const char *keys[2] = {"com.apple.CFDate.magic", "com.apple.CFDate.ti"};
    xpc_object_t values[2] = {magicObject, value};
    xpc_object_t result = xpc_dictionary_create(keys, values, 2);
    xpc_release(value);
    xpc_release(magicObject);
    if (result != NULL && xpc_dictionary_get_count(result) != 2) {
        xpc_release(result);
        return NULL;
    }
    return result;
}

// Native __CFXPCDictionaryApplier, 0x1c02cc754. Non-external. A string key is converted to UTF-8 in a heap
// buffer (as for strings above) and the converted value, when there is one, is stored under it in the XPC
// dictionary the context points to. Other keys, and keys that do not convert, are skipped.
static void __CFXPCDictionaryApplier(const void *key, const void *value, void *context) {
    if (OCFGetTypeID(key) != kStringTypeID) {
        return;
    }
    OCFIndex size = OCFStringGetMaximumSizeForEncoding(OCFStringGetLength(key) + 1, kUTF8Encoding);
    if (size == kNotFound) {
        return;
    }
    char *buffer = malloc_type_malloc(size, 0x100004077774924);
    if (buffer == NULL) {
        return;
    }
    if (OCFStringGetCString(key, buffer, size, kUTF8Encoding)) {
        xpc_object_t object = _OCFXPCCreateXPCObjectFromCFObject(value);
        if (object != NULL) {
            xpc_dictionary_set_value(*(xpc_object_t *)context, buffer, object);
            xpc_release(object);
        }
    }
    free(buffer);
}

// Native _CFXPCCreateCFObjectFromXPCObject, 0x1c02cc830, with its blocks
// ____CFXPCCreateCFObjectFromXPCObject_block_invoke (0x1c02cd0f8, descriptor 0x2227aa2d8: the array
// element) and _block_invoke_2 (0x1c02cd140, descriptor 0x2227aa300: the dictionary entry). External. NULL
// answers NULL. By XPC type: null, bool, string (UTF-8), data (a length with the sign bit set answers NULL),
// int64 (SInt64 number), double, date (nanoseconds since 1970 divided by 1e9, minus
// kCFAbsoluteTimeIntervalSince1970) and UUID map to their CF objects. An array (count below 2^59) and a
// dictionary (count below 2^58) convert their entries into a 2,048-byte stack buffer whose first 512 bytes
// are zeroed (as natively; the source form of that partial zeroing is unread, confidence medium), or a heap
// buffer above 256 elements or 128 entries (NULL when it cannot be allocated); the collection is created
// only when every entry converted, and the converted entries are released afterwards. A three-entry
// dictionary with the URL magic and a two-entry dictionary with the date magic become a URL and a date; an
// absent or mismatched magic falls back to the generic dictionary, a magic that is not a UUID answers NULL.
// Any other type answers NULL.
OCFTypeRef _OCFXPCCreateCFObjectFromXPCObject(xpc_object_t object) {
    if (object == NULL) {
        return NULL;
    }
    xpc_type_t type = xpc_get_type(object);
    if (type == XPC_TYPE_NULL) {
        return OCFRetain(kOCFNull);
    }
    if (type == XPC_TYPE_BOOL) {
        return OCFRetain(xpc_bool_get_value(object) ? kOCFBooleanTrue : kOCFBooleanFalse);
    }
    if (type == XPC_TYPE_STRING) {
        return OCFStringCreateWithCString(
            kOCFAllocatorSystemDefault, xpc_string_get_string_ptr(object), kUTF8Encoding
        );
    }
    if (type == XPC_TYPE_DATA) {
        size_t length = xpc_data_get_length(object);
        if ((OCFIndex)length < 0) {
            return NULL;
        }
        return OCFDataCreate(kOCFAllocatorSystemDefault, xpc_data_get_bytes_ptr(object), (OCFIndex)length);
    }
    if (type == XPC_TYPE_INT64) {
        int64_t value = xpc_int64_get_value(object);
        return OCFNumberCreate(kOCFAllocatorSystemDefault, kOCFNumberSInt64Type, &value);
    }
    if (type == XPC_TYPE_DOUBLE) {
        double value = xpc_double_get_value(object);
        return OCFNumberCreate(kOCFAllocatorSystemDefault, kOCFNumberDoubleType, &value);
    }
    if (type == XPC_TYPE_DATE) {
        int64_t value = xpc_date_get_value(object);
        return OCFDateCreate(
            kOCFAllocatorSystemDefault, (double)value / 1000000000.0 - kOCFAbsoluteTimeIntervalSince1970
        );
    }
    if (type == XPC_TYPE_UUID) {
        OCFUUIDBytes bytes = *(const OCFUUIDBytes *)xpc_uuid_get_bytes(object);
        return OCFUUIDCreateFromUUIDBytes(kOCFAllocatorSystemDefault, bytes);
    }
    const void *stackBuffer[256];
    if (type == XPC_TYPE_ARRAY) {
        size_t count = xpc_array_get_count(object);
        if (count >> 59 != 0) {
            return NULL;
        }
        memset(stackBuffer, 0, 512);
        const void **values = stackBuffer;
        if (count >= 0x101) {
            values = malloc_type_malloc(8 * count, 0xc0040b8aa526d);
            if (values == NULL) {
                return NULL;
            }
        }
        __block OCFIndex converted = 0;
        xpc_array_apply(object, ^bool(size_t index, xpc_object_t value) {
          (void)index;
          OCFTypeRef element = _OCFXPCCreateCFObjectFromXPCObject(value);
          if (element != NULL) {
              values[converted++] = element;
          }
          return element != NULL;
        });
        OCFArrayRef result = NULL;
        if ((OCFIndex)count == converted) {
            result =
                OCFArrayCreate(kOCFAllocatorSystemDefault, values, (OCFIndex)count, &kOCFTypeArrayCallBacks);
        }
        for (OCFIndex index = 0; index < converted; ++index) {
            OCFRelease(values[index]);
        }
        if (values != stackBuffer) {
            free(values);
        }
        return result;
    }
    if (type != XPC_TYPE_DICTIONARY) {
        return NULL;
    }
    size_t count = xpc_dictionary_get_count(object);
    if (count >> 58 != 0) {
        return NULL;
    }
    const void **keys = stackBuffer;
    if (count == 3) {
        xpc_object_t magicObject = xpc_dictionary_get_value(object, "com.apple.CFURL.magic");
        if (magicObject != NULL) {
            if (xpc_get_type(magicObject) != XPC_TYPE_UUID) {
                return NULL;
            }
            uuid_t found = {0};
            uuid_t magic = {0};
            memcpy(found, xpc_uuid_get_bytes(magicObject), sizeof(uuid_t));
            if (uuid_parse("C3853DCC-9776-4114-B6C1-FD9F51944A6D", magic) == 0 &&
                uuid_compare(found, magic) == 0) {
                xpc_object_t string = xpc_dictionary_get_value(object, "com.apple.CFURL.string");
                if (string == NULL) {
                    return NULL;
                }
                xpc_object_t base = xpc_dictionary_get_value(object, "com.apple.CFURL.base");
                if (base == NULL) {
                    return NULL;
                }
                if (xpc_get_type(base) != XPC_TYPE_DICTIONARY && xpc_get_type(base) != XPC_TYPE_NULL) {
                    return NULL;
                }
                OCFTypeRef baseURL = _OCFXPCCreateCFObjectFromXPCObject(base);
                if (baseURL == NULL) {
                    return NULL;
                }
                if (OCFURLGetTypeID() == OCFGetTypeID(baseURL) ||
                    OCFNullGetTypeID() == OCFGetTypeID(baseURL)) {
                    OCFTypeRef urlString = _OCFXPCCreateCFObjectFromXPCObject(string);
                    if (urlString != NULL) {
                        if (OCFGetTypeID(urlString) == kStringTypeID) {
                            OCFURLRef result = OCFURLCreateWithString(
                                kOCFAllocatorSystemDefault,
                                urlString,
                                baseURL == kOCFNull ? NULL : (OCFURLRef)baseURL
                            );
                            OCFRelease(baseURL);
                            OCFRelease(urlString);
                            return result;
                        }
                        OCFRelease(urlString);
                    }
                }
                OCFRelease(baseURL);
                return NULL;
            }
        }
        memset(stackBuffer, 0, 512);
    } else if (count == 2) {
        xpc_object_t magicObject = xpc_dictionary_get_value(object, "com.apple.CFDate.magic");
        if (magicObject != NULL) {
            if (xpc_get_type(magicObject) != XPC_TYPE_UUID) {
                return NULL;
            }
            uuid_t found;
            uuid_t magic = {0};
            memcpy(found, xpc_uuid_get_bytes(magicObject), sizeof(uuid_t));
            if (uuid_parse("C119A421-DF31-48EF-843D-21B4030D1621", magic) == 0 &&
                uuid_compare(found, magic) == 0) {
                double interval = xpc_dictionary_get_double(object, "com.apple.CFDate.ti");
                return OCFDateCreate(
                    kOCFAllocatorSystemDefault, interval - kOCFAbsoluteTimeIntervalSince1970
                );
            }
        }
        memset(stackBuffer, 0, 512);
    } else {
        memset(stackBuffer, 0, 512);
        if (count >= 0x81) {
            keys = malloc_type_malloc(16 * count, 0xc0040b8aa526d);
            if (keys == NULL) {
                return NULL;
            }
        }
    }
    __block OCFIndex converted = 0;
    xpc_dictionary_apply(object, ^bool(const char *key, xpc_object_t value) {
      OCFTypeRef entry = _OCFXPCCreateCFObjectFromXPCObject(value);
      if (entry == NULL) {
          return false;
      }
      OCFStringRef keyString = OCFStringCreateWithCString(kOCFAllocatorSystemDefault, key, kUTF8Encoding);
      if (keyString == NULL) {
          OCFRelease(entry);
          return false;
      }
      keys[converted] = keyString;
      OCFIndex index = converted++;
      keys[count + index] = entry;
      return true;
    });
    const void **values = &keys[count];
    OCFDictionaryRef result = NULL;
    if ((OCFIndex)count == converted) {
        result = OCFDictionaryCreate(
            kOCFAllocatorSystemDefault,
            keys,
            values,
            (OCFIndex)count,
            &kOCFTypeDictionaryKeyCallBacks,
            &kOCFTypeDictionaryValueCallBacks
        );
    }
    for (OCFIndex index = 0; index < converted; ++index) {
        OCFRelease(keys[index]);
        OCFRelease(values[index]);
    }
    if (keys != stackBuffer) {
        free(keys);
    }
    return result;
}
