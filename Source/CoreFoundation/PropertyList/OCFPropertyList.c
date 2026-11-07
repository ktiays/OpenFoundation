//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PropertyList/OCFPropertyList.h"
#include "CoreFoundation/Stream/OCFConcreteStreams.h"

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/PropertyList/OCFBinaryPList.h"
#include "CoreFoundation/PropertyList/OCFKeyedArchiverUID.h"
#include "CoreFoundation/PropertyList/OCFOldStylePList.h"

#include <ctype.h>
#include <malloc/malloc.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Inferred organization: the XML property-list functions lie contiguously in the iOS Simulator __text
// section, from __CFPropertyListCreateError (0x1c01d37e0) through lineNumber (0x1c01d84dc), with their
// out-of-line .cold entries at 0x1c030b800-0x1c030b8ac; they are kept in one translation unit in native
// address order. The native source path is not in the evidence. Functions of the range outside the restored
// lists (the CFPropertyListCreateXMLData wrappers, __CFPropertyListCreateSingleValue, the stream and legacy
// entry points and __CFStringContainsNullCharacter) are not defined here; the XML parser, from
// parseXMLElement (0x1c01d8684) to _createUniqueStringWithUTF8Bytes (0x1c01dafd0), follows lineNumber. Unless
// a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (disassembly); iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationLeafHelpers.md
// (__CFPropertyListKeyBufferMatchesFilterBuffer, lineNumber); the other functions:
// Local/Scratch/cf-core/tasks/plist.txt (sub-track reading; no versioned reading yet).

static bool __CFPropertyListIsValidAux(OCFPropertyListRef plist, PlistValidationContext *context);
static bool _CFPropertyListCreateFromUTF8Data(
    OCFAllocatorRef allocator,
    OCFDataRef xmlData,
    OCFIndex skipBytes,
    OCFStringRef originalString,
    OCFStringEncoding guessedEncoding,
    OCFOptionFlags option,
    OCFErrorRef *outError,
    Boolean allowNewTypes,
    OCFPropertyListFormat *format,
    OCFSetRef keyPaths,
    OCFTypeRef *out,
    bool attemptXML,
    bool allowOpenStep
);
static bool _CFPropertyListCreateWithData(
    OCFAllocatorRef allocator,
    OCFDataRef data,
    OCFOptionFlags option,
    OCFErrorRef *outError,
    Boolean allowNewTypes,
    OCFPropertyListFormat *format,
    OCFSetRef keyPaths,
    OCFTypeRef *out
);
static void _CFAppendXML0(OCFTypeRef object, uint32_t indentation, OCFMutableDataRef xmlString);
static void __CFPropertyListIsArrayPlistAux(const void *value, void *context);
static void __CFPropertyListIsDictPlistAux(const void *key, const void *value, void *context);

// The encodings the parser names by value (iOS Simulator SDK CFString.h and CFStringEncodingExt.h).
enum {
    kEncodingMacRoman = 0,
    kEncodingUnicode = 0x100,
    kEncodingISOLatin1 = 0x201,
    kEncodingASCII = 0x600,
    kEncodingUTF8 = 0x08000100,
    kEncodingUTF16BE = 0x10000100,
    kEncodingUTF16LE = 0x14000100,
    kEncodingUTF32BE = 0x18000100,
    kEncodingUTF32LE = 0x1c000100,
};

// The malloc type ID every temporary key or value buffer of this unit passes.
static const uint64_t kTemporaryStorageTypeID = 0xc0040b8aa526dULL;

// Native CFXMLPlistTagsUnicode, 0x1c0375a9c (__TEXT,__const, 260 bytes). Non-external.
const UniChar CFXMLPlistTagsUnicode[13][10] = {
    {'p', 'l', 'i', 's', 't'},
    {'a', 'r', 'r', 'a', 'y'},
    {'d', 'i', 'c', 't'},
    {'k', 'e', 'y'},
    {'s', 't', 'r', 'i', 'n', 'g'},
    {'d', 'a', 't', 'a'},
    {'d', 'a', 't', 'e'},
    {'r', 'e', 'a', 'l'},
    {'i', 'n', 't', 'e', 'g', 'e', 'r'},
    {'t', 'r', 'u', 'e'},
    {'f', 'a', 'l', 's', 'e'},
    {'D', 'O', 'C', 'T', 'Y', 'P', 'E'},
    {'<', '!', '[', 'C', 'D', 'A', 'T', 'A', '['},
};

// The tag indexes of CFXMLPlistTagsUnicode.
enum {
    kPlistTag = 0,
    kArrayTag = 1,
    kDictTag = 2,
    kKeyTag = 3,
    kStringTag = 4,
    kDataTag = 5,
    kDateTag = 6,
    kRealTag = 7,
    kIntegerTag = 8,
    kTrueTag = 9,
    kFalseTag = 10,
};

// Native _appendIndents.tabs, 0x1c0375ba0 (__TEXT,__const): four UTF-16 tabs. Non-external; a local static of
// _appendIndents, which the native code inlines into _CFAppendXML0.
static const UniChar _appendIndentsTabs[4] = {'\t', '\t', '\t', '\t'};

// Native _XMLPlistAppendDataUsingBase64.__CFPLDataEncodeTable, 0x1c04862dd (__TEXT,__cstring). Non-external;
// a local static of _XMLPlistAppendDataUsingBase64, which the native code inlines into _CFAppendXML0.
static const char __CFPLDataEncodeTable[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// The temporary-storage crash shared by the .cold entries of this unit: each stores the message through
// _OUTLINED_FUNCTION_0 (0x1c01db0f8) and executes BRK #1.
__attribute__((noreturn)) static void temporaryStorageCrash(void) {
    crashWithMessage("CFPropertyList ran out of memory while attempting to allocate temporary storage.");
}

// Native __CFPropertyListCreateError, 0x1c01d37e0. Non-external (was a private external).
// With a format: the debug description is formatted from the variadic arguments in the system default
// allocator, put under kCFErrorDebugDescriptionKey in a mutable dictionary (copy-string keys, CFType values)
// and the Cocoa-domain error is created with it; the string and the dictionary are released afterwards.
// Without a format the error is created with no user info (a tail call).
OCFErrorRef __CFPropertyListCreateError(OCFIndex code, OCFStringRef format, ...) {
    if (format == NULL) {
        return OCFErrorCreate(kOCFAllocatorSystemDefault, kOCFErrorDomainCocoa, code, NULL);
    }
    va_list arguments;
    va_start(arguments, format);
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef debugDescription = OCFStringCreateWithFormatAndArguments(allocator, NULL, format, arguments);
    va_end(arguments);
    OCFMutableDictionaryRef userInfo = OCFDictionaryCreateMutable(
        allocator, 0, &kOCFCopyStringDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    OCFDictionarySetValue(userInfo, kOCFErrorDebugDescriptionKey, debugDescription);
    OCFErrorRef error = OCFErrorCreate(allocator, kOCFErrorDomainCocoa, code, userInfo);
    OCFRelease(debugDescription);
    OCFRelease(userInfo);
    return error;
}

// Native _CFPropertyListCreateXMLData, 0x1c01d391c. External.
// With checkValidPlist the list is first validated for the XML format (no error slot) and NULL is returned
// when it fails. The XML declaration and DOCTYPE are appended as bytes (49, 89 and 16 bytes) around the
// UTF-16 "plist" tag, then the tree at indentation 0 and the closing tag.
OCFDataRef _OCFPropertyListCreateXMLData(
    OCFAllocatorRef allocator, OCFPropertyListRef propertyList, Boolean checkValidPlist
) {
    if (checkValidPlist) {
        PlistValidationContext context = {true, kOCFPropertyListXMLFormat_v1_0, NULL, NULL, NULL};
        if (!__CFPropertyListIsValidAux(propertyList, &context)) {
            return NULL;
        }
    }
    OCFMutableDataRef xml = OCFDataCreateMutable(allocator, 0);
    OCFDataAppendBytes(xml, (const UInt8 *)"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE ", 49);
    _plistAppendCharacters(xml, CFXMLPlistTagsUnicode[kPlistTag], 5);
    OCFDataAppendBytes(
        xml,
        (
            const UInt8 *
        )" PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n<",
        89
    );
    _plistAppendCharacters(xml, CFXMLPlistTagsUnicode[kPlistTag], 5);
    OCFDataAppendBytes(xml, (const UInt8 *)" version=\"1.0\">\n", 16);
    _CFAppendXML0(propertyList, 0, xml);
    OCFDataAppendBytes(xml, (const UInt8 *)"</", 2);
    _plistAppendCharacters(xml, CFXMLPlistTagsUnicode[kPlistTag], 5);
    OCFDataAppendBytes(xml, (const UInt8 *)">\n", 2);
    return xml;
}

// Native CFPropertyListIsValid, 0x1c01d3a68. External.
// Validates with the given format and no error slot.
Boolean OCFPropertyListIsValid(OCFPropertyListRef plist, OCFPropertyListFormat format) {
    PlistValidationContext context = {true, format, NULL, NULL, NULL};
    return __CFPropertyListIsValidAux(plist, &context);
}

// Native _CFPropertyListCreateXMLDataWithExtras, 0x1c01d3ad4. External. iOS Device divergence: unread.
OCFDataRef
_OCFPropertyListCreateXMLDataWithExtras(OCFAllocatorRef allocator, OCFPropertyListRef propertyList) {
    return _OCFPropertyListCreateXMLData(allocator, propertyList, false);
}

// Native __CFPropertyListIsValidAux, 0x1c01d3adc. Non-external.
// NULL fails. Type 20 (data) is valid; a string (7) is valid unless the format is XML (100) and it contains a
// NUL character. Except for the OpenStep format (1), types 21, 22 and 42 (mask 0x40000600000, tested only for
// type IDs up to 42) and the keyed-archiver UID type are valid. Everything else must be a dictionary (18) or
// an array (19) not already on the current path; the path set is created on first use (system default
// allocator, no callbacks) and released by the call that created it. Except for the binary format (200), a
// bag (the kCFTypeBagCallBacks retain, release and description, no equality or hash, kCFAllocatorDefault)
// counts the non-empty containers entered; a container already counted more than 384 times fails. Failures
// store a retained constant or a new string through the error slot when there is one; the slot is overwritten
// without release. CoreFoundation defect (kept): the "Too many nested" failure returns without releasing a
// path set or bag this call created.
static bool __CFPropertyListIsValidAux(OCFPropertyListRef plist, PlistValidationContext *context) {
    OCFStringRef message;
    if (plist == NULL) {
        if (context->error == NULL) {
            return false;
        }
        message = OF_STR("property lists cannot contain NULL");
        goto storeConstantError;
    }
    OCFTypeID typeID = OCFGetTypeID(plist);
    if (typeID == 20) {
        return true;
    }
    if (typeID == 7) {
        if (context->format != kOCFPropertyListXMLFormat_v1_0 || !__CFStringContainsNullCharacter(plist)) {
            return true;
        }
        if (context->error == NULL) {
            return false;
        }
        message = OF_STR(
            "property list strings cannot contain embedded null characters for XML or OpenStep format"
        );
        goto storeConstantError;
    }
    if (context->format != kOCFPropertyListOpenStepFormat) {
        if (typeID <= 42 && ((UINT64_C(1) << typeID) & UINT64_C(0x40000600000)) != 0) {
            return true;
        }
        if (_OCFKeyedArchiverUIDGetTypeID() == typeID) {
            return true;
        }
    }
    OCFMutableSetRef set = context->set;
    if (set != NULL) {
        if (OCFSetContainsValue(set, plist)) {
            if (context->error == NULL) {
                return false;
            }
            message = OF_STR("property lists cannot contain recursive container references");
            goto storeConstantError;
        }
    } else {
        context->set = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, NULL);
    }
    OCFPropertyListFormat format = context->format;
    bool createdBag = false;
    if (format != kOCFPropertyListBinaryFormat_v1_0) {
        if (context->bag == NULL) {
            OCFBagCallBacks callBacks = {
                kOCFTypeBagCallBacks.version,
                kOCFTypeBagCallBacks.retain,
                kOCFTypeBagCallBacks.release,
                kOCFTypeBagCallBacks.copyDescription,
                NULL,
                NULL,
            };
            context->bag = OCFBagCreateMutable(kOCFAllocatorDefault, 0, &callBacks);
            createdBag = true;
        } else if (OCFBagGetCountOfValue(context->bag, plist) > 384) {
            if (context->error == NULL) {
                return false;
            }
            message = OF_STR(
                "Too many nested arrays or dictionaries please use kCFPropertyListBinaryFormat_v1_0 instead "
                "which "
                "supports references"
            );
            goto storeConstantError;
        }
    }
    bool result;
    if (typeID == 18) {
        OCFIndex count = OCFDictionaryGetCount(plist);
        if (format != kOCFPropertyListBinaryFormat_v1_0 && count >= 1) {
            OCFBagAddValue(context->bag, plist);
        }
        OCFSetAddValue(context->set, plist);
        OCFDictionaryApplyFunction(plist, __CFPropertyListIsDictPlistAux, context);
    } else if (typeID == 19) {
        OCFIndex count = OCFArrayGetCount(plist);
        if (format != kOCFPropertyListBinaryFormat_v1_0 && count >= 1) {
            OCFBagAddValue(context->bag, plist);
        }
        OCFSetAddValue(context->set, plist);
        OCFArrayApplyFunction(plist, __OCFRangeMake(0, count), __CFPropertyListIsArrayPlistAux, context);
    } else {
        if (context->error != NULL) {
            OCFStringRef description = OCFCopyTypeIDDescription(typeID);
            *context->error = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("property lists cannot contain objects of type '%@'"),
                description
            );
            OCFRelease(description);
        }
        result = false;
        goto releaseCreated;
    }
    OCFSetRemoveValue(context->set, plist);
    result = context->isValid;
releaseCreated:
    if (set == NULL) {
        OCFRelease(context->set);
        context->set = NULL;
    }
    if (createdBag) {
        OCFRelease(context->bag);
        context->bag = NULL;
    }
    return result;
storeConstantError:
    *context->error = OCFRetain(message);
    return false;
}

// Native __CFPropertyListCreateTopLevelKeyPaths, 0x1c01d3de0 (with .cold.1 0x1c030b800 and .cold.2
// 0x1c030b810). Non-external (was a private external).
// NULL key paths give NULL. A count with any of bits 60-63 set crashes (.cold.2). The values are read into a
// zeroed stack buffer of max(count, 1) words for up to 256 values (the frame always reserves at least one
// word and zeroes at most 512 bytes of it first), otherwise into a heap buffer from the system default
// allocator (type 0xc0040b8aa526d; failure crashes, .cold.1). Each array is added to the result as is; any
// other value is split at ':' (kCFTypeSetCallBacks, capacity count, the caller's allocator).
OCFSetRef __CFPropertyListCreateTopLevelKeyPaths(OCFAllocatorRef allocator, OCFSetRef keyPaths) {
    if (keyPaths == NULL) {
        return NULL;
    }
    OCFIndex count = OCFSetGetCount(keyPaths);
    if (((uint64_t)count >> 60) != 0) {
        temporaryStorageCrash();
    }
    uint64_t atLeastOne = (uint64_t)count > 1 ? (uint64_t)count : 1;
    uint64_t slots = (uint64_t)count < 0x101 ? atLeastOne : 1;
    const void *stackBuffer[slots];
    bzero(stackBuffer, slots * sizeof(void *) < 0x200 ? slots * sizeof(void *) : 0x200);
    const void **values = stackBuffer;
    if ((uint64_t)count <= 0x100) {
        bzero(values, atLeastOne * sizeof(void *));
    } else {
        values = OCFAllocatorAllocateTyped(
            kOCFAllocatorSystemDefault, (OCFIndex)(atLeastOne * sizeof(void *)), kTemporaryStorageTypeID, 0
        );
        if (values == NULL) {
            temporaryStorageCrash();
        }
    }
    OCFSetGetValues(keyPaths, values);
    OCFMutableSetRef result = OCFSetCreateMutable(allocator, count, &kOCFTypeSetCallBacks);
    if (count != 0) {
        for (OCFIndex index = 0; index != count; ++index) {
            if (OCFGetTypeID(values[index]) == 19) {
                OCFSetAddValue(result, values[index]);
            } else {
                OCFArrayRef components =
                    OCFStringCreateArrayBySeparatingStrings(allocator, values[index], OF_STR(":"));
                OCFSetAddValue(result, components);
                if (components != NULL) {
                    OCFRelease(components);
                }
            }
        }
        if ((uint64_t)count >= 0x101) {
            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, values);
        }
    }
    return result;
}

// Native __CFPropertyListCreateSplitKeypaths, 0x1c01d3fa0 (with .cold.1 0x1c030b820, .cold.2 0x1c030b830,
// .cold.3 0x1c030b840 and .cold.4 0x1c030b850). Non-external (was a private external).
// NULL current keys store NULL in both outputs. Each key path is an array: its first element joins theseKeys
// and, when it has more, an array of the remaining elements (kCFTypeArrayCallBacks, the caller's allocator)
// joins nextKeys; both sets are created on first need (kCFTypeSetCallBacks). The key-path buffer and each
// remainder buffer follow the stack-or-heap rule of __CFPropertyListCreateTopLevelKeyPaths (remainders on the
// heap for arrays of more than 257 elements); a count with any of bits 60-63 set crashes (.cold.4 for the key
// paths, .cold.3 for a remainder), as does a failed heap buffer (.cold.1, .cold.2). The remainder is read and
// created with CFArrayGetCount - 1, re-read for each call.
void __CFPropertyListCreateSplitKeypaths(
    OCFAllocatorRef allocator, OCFSetRef currentKeys, OCFSetRef *theseKeys, OCFSetRef *nextKeys
) {
    if (currentKeys == NULL) {
        *theseKeys = NULL;
        *nextKeys = NULL;
        return;
    }
    OCFIndex count = OCFSetGetCount(currentKeys);
    if (((uint64_t)count >> 60) != 0) {
        temporaryStorageCrash();
    }
    uint64_t atLeastOne = (uint64_t)count > 1 ? (uint64_t)count : 1;
    uint64_t slots = (uint64_t)count < 0x101 ? atLeastOne : 1;
    const void *stackBuffer[slots];
    bzero(stackBuffer, slots * sizeof(void *) < 0x200 ? slots * sizeof(void *) : 0x200);
    const void **keys = stackBuffer;
    OCFMutableSetRef these = NULL;
    OCFMutableSetRef next = NULL;
    if ((uint64_t)count >= 0x101) {
        keys = OCFAllocatorAllocateTyped(
            kOCFAllocatorSystemDefault, (OCFIndex)(atLeastOne * sizeof(void *)), kTemporaryStorageTypeID, 0
        );
        if (keys == NULL) {
            temporaryStorageCrash();
        }
        OCFSetGetValues(currentKeys, keys);
    } else {
        bzero(keys, atLeastOne * sizeof(void *));
        OCFSetGetValues(currentKeys, keys);
        if (count == 0) {
            goto store;
        }
    }
    for (OCFIndex index = 0; index != count; ++index) {
        OCFArrayRef keyPath = keys[index];
        OCFIndex length = OCFArrayGetCount(keyPath);
        if (length < 1) {
            continue;
        }
        if (these == NULL) {
            these = OCFSetCreateMutable(allocator, 0, &kOCFTypeSetCallBacks);
        }
        OCFSetAddValue(these, OCFArrayGetValueAtIndex(keyPath, 0));
        uint64_t remaining = (uint64_t)length - 1;
        if (remaining == 0) {
            continue;
        }
        if (next == NULL) {
            next = OCFSetCreateMutable(allocator, 0, &kOCFTypeSetCallBacks);
        }
        if ((remaining >> 60) != 0) {
            temporaryStorageCrash();
        }
        uint64_t remainderSlots = (uint64_t)length < 0x102 ? remaining : 1;
        const void *remainderStackBuffer[remainderSlots];
        bzero(
            remainderStackBuffer,
            remainderSlots * sizeof(void *) < 0x200 ? remainderSlots * sizeof(void *) : 0x200
        );
        const void **remainder = remainderStackBuffer;
        if ((uint64_t)length <= 0x101) {
            bzero(remainder, remaining * sizeof(void *));
        } else {
            remainder = OCFAllocatorAllocateTyped(
                kOCFAllocatorSystemDefault, (OCFIndex)(remaining * sizeof(void *)), kTemporaryStorageTypeID, 0
            );
            if (remainder == NULL) {
                temporaryStorageCrash();
            }
        }
        OCFArrayGetValues(keyPath, __OCFRangeMake(1, OCFArrayGetCount(keyPath) - 1), remainder);
        OCFArrayRef rest =
            OCFArrayCreate(allocator, remainder, OCFArrayGetCount(keyPath) - 1, &kOCFTypeArrayCallBacks);
        OCFSetAddValue(next, rest);
        if (rest != NULL) {
            OCFRelease(rest);
        }
        if ((uint64_t)length >= 0x102) {
            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, remainder);
        }
    }
    if ((uint64_t)count >= 0x101) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, keys);
    }
store:
    *theseKeys = these;
    *nextKeys = next;
}

// Native __CFPropertyListKeyPassesFilterSet, 0x1c01d42a4, with its block
// ____CFPropertyListKeyPassesFilterSet_block_invoke, 0x1c01d4378. Non-external (was a private external).
// A __block result (false) and a stack block capturing it and the key are applied to the filter set with
// CFSetApply; the byref is disposed (flag 8) before the result is returned. The block matches a filter of one
// unit "*" always; another one-unit filter only a one-unit key by CFEqual; a longer filter ending in '*' a
// key with the preceding prefix (a substring in the system default allocator, released after
// CFStringHasPrefix); any other filter (also an empty one) by CFEqual. A match sets the result and stops the
// enumeration.
bool __CFPropertyListKeyPassesFilterSet(OCFStringRef key, OCFSetRef filterSet) {
    __block bool passes = false;
    CFSetApply(filterSet, ^(const void *value, Boolean *stop) {
      OCFStringRef filter = value;
      OCFIndex length = OCFStringGetLength(filter);
      OCFIndex last = length - 1;
      if (last == 0) {
          if (OCFStringGetCharacterAtIndex(filter, 0) == '*') {
              goto matched;
          }
          if (OCFStringGetLength(key) != 1) {
              return;
          }
      } else if (length >= 2 && OCFStringGetCharacterAtIndex(filter, last) == '*') {
          OCFStringRef prefix =
              OCFStringCreateWithSubstring(kOCFAllocatorSystemDefault, filter, __OCFRangeMake(0, last));
          Boolean hasPrefix = OCFStringHasPrefix(key, prefix);
          OCFRelease(prefix);
          if (hasPrefix) {
              goto matched;
          }
          return;
      }
      if (!OCFEqual(key, filter)) {
          return;
      }
  matched:
      passes = true;
      *stop = true;
    });
    return passes;
}

// Native __CFPropertyListKeyBufferMatchesFilterBuffer, iOS Simulator 24A434, 0x1c01d446c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external (was a private
// external); its only caller is ____CFPropertyListTryFilteredDictionaryASCIIFastPath_block_invoke.
// iOS Device 24A446: 0x18068f8c4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// reorders the blocks and authenticates its return; the comparisons and memcmp calls agree.
// A one-byte filter "*" matches every key; any other one-byte filter matches only the equal one-byte
// key. A longer filter ending in '*' matches a key of at least filterLength - 1 bytes (signed) with
// that prefix. Otherwise the lengths must be equal and memcmp decides; a zero or negative filter
// length reaches memcmp only when the key length is equal, and a negative one is passed to memcmp as a
// huge size (native behavior; no native caller passes one).
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool __CFPropertyListKeyBufferMatchesFilterBuffer(
    const char *key, OCFIndex keyLength, const char *filter, OCFIndex filterLength
) {
    if (filterLength == 1) {
        if (filter[0] == '*') {
            return true;
        }
        return keyLength == 1 && (uint8_t)filter[0] == (uint8_t)key[0];
    }
    if (filterLength >= 2 && filter[filterLength - 1] == '*') {
        if (keyLength >= filterLength - 1) {
            return memcmp(key, filter, (size_t)(filterLength - 1)) == 0;
        }
        return false;
    }
    if (keyLength == filterLength) {
        return memcmp(key, filter, (size_t)filterLength) == 0;
    }
    return false;
}

// Native __CFPropertyListFilterKeyHasWildcard, 0x1c01d44f8. Non-external (was a private external).
// True when the last unit of a non-empty key is '*'.
bool __CFPropertyListFilterKeyHasWildcard(OCFStringRef key) {
    OCFIndex last = OCFStringGetLength(key) - 1;
    if (last < 0) {
        return false;
    }
    return OCFStringGetCharacterAtIndex(key, last) == '*';
}

// Native _CFPropertyListCreateFromXMLStringError, 0x1c01d4538. External.
// The string is converted to UTF-8 data, parsed with XML and the old-style fallback both enabled, no skip, no
// key paths and no guessed encoding, and the data released; the parse result itself is not tested.
OCFPropertyListRef _OCFPropertyListCreateFromXMLStringError(
    OCFAllocatorRef allocator,
    OCFStringRef xmlString,
    OCFOptionFlags option,
    OCFErrorRef *error,
    Boolean allowNewTypes,
    OCFPropertyListFormat *format
) {
    OCFDataRef data = _createUTF8DataFromString(allocator, xmlString);
    OCFTypeRef result = NULL;
    _CFPropertyListCreateFromUTF8Data(
        allocator, data, 0, xmlString, 0, option, error, allowNewTypes, format, NULL, &result, true, true
    );
    if (data != NULL) {
        OCFRelease(data);
    }
    return result;
}

// Native _createUTF8DataFromString, 0x1c01d4608. Non-external. The UTF-8 length of the whole string is
// measured with CFStringGetBytes (no loss byte, no buffer), that many bytes are allocated from allocator
// (type 0x100004077774924, no hint) and filled by a second CFStringGetBytes, and the bytes become a CFData
// owned by allocator (CFDataCreateWithBytesNoCopy with allocator as the deallocator). The allocation is not
// checked.
OCFDataRef _createUTF8DataFromString(OCFAllocatorRef allocator, OCFStringRef string) {
    OCFIndex length = 0;
    OCFStringGetBytes(
        string, __OCFRangeMake(0, OCFStringGetLength(string)), 0x8000100, 0, false, NULL, 0, &length
    );
    UInt8 *bytes = OCFAllocatorAllocateTyped(allocator, length, 0x100004077774924, 0);
    OCFStringGetBytes(
        string, __OCFRangeMake(0, OCFStringGetLength(string)), 0x8000100, 0, false, bytes, length, NULL
    );
    return OCFDataCreateWithBytesNoCopy(allocator, bytes, length, allocator);
}

// Native _CFPropertyListCreateFromUTF8Data, 0x1c01d470c. Non-external.
// Empty data, or a start address (bytes + skipBytes) whose signed sum with the length overflows, fails with
// error 3840 when there is an error slot. Otherwise the parser context spans the data after skipBytes. With
// attemptXML the data is retained, a burst trie, a string cache array (kCFTypeArrayCallBacks) and the
// top-level key paths are created, and the prolog is scanned: whitespace (tab, LF, CR, space), "<?"
// processing instructions, "<!--" comments and a "<!DOCTYPE" declaration (whitespace, then up to '[' or '>',
// then whitespace up to '>'), stopping at the first element, which parseXMLElement parses. The prolog errors
// ("Encountered unexpected EOF", "No XML content found", "Unexpected character %c at line %u") stop the scan;
// the DTD errors and the errors the skip functions leave stop it only when the error was created. A parsed
// element with a value stores kCFPropertyListXMLFormat_v1_0 in *format. The trie, the cache, the key paths
// and the data are released in that order before the result is returned. Without an XML result and with
// allowOpenStep, the old-style parser runs (its error is collected only when there is an error slot); a
// pending XML error and an old-style error are merged into a copy of the XML error's user info under
// "kCFPropertyListOldStyleParsingError" (capacity count + 1, system default allocator, the XML error's domain
// and code). Without allowOpenStep a pending XML error is passed on (only after an XML attempt), otherwise
// "Unsupported property list".
static bool _CFPropertyListCreateFromUTF8Data(
    OCFAllocatorRef allocator,
    OCFDataRef xmlData,
    OCFIndex skipBytes,
    OCFStringRef originalString,
    OCFStringEncoding guessedEncoding,
    OCFOptionFlags option,
    OCFErrorRef *outError,
    Boolean allowNewTypes,
    OCFPropertyListFormat *format,
    OCFSetRef keyPaths,
    OCFTypeRef *out,
    bool attemptXML,
    bool allowOpenStep
) {
    OCFStringRef message;
    OCFIndex length = OCFDataGetLength(xmlData);
    if (length == 0) {
        if (outError == NULL) {
            return false;
        }
        message = OF_STR("Conversion of string failed. The string is empty.");
        goto storeNewError;
    }
    OCFTypeRef value = NULL;
    XMLPlistParseContext context = {0};
    const uint8_t *begin = OCFDataGetBytePtr(xmlData) + skipBytes;
    intptr_t sum = 0;
    if (__builtin_add_overflow((intptr_t)begin, (intptr_t)length, &sum)) {
        if (outError == NULL) {
            return false;
        }
        message = OF_STR("Unable to address entirety of CFPropertyList");
        goto storeNewError;
    }
    context.begin = begin;
    context.curr = begin;
    context.end = begin + length - skipBytes;
    context.error = NULL;
    context.allocator = allocator;
    context.mutabilityOption = (uint32_t)(option & 0xff);
    context.allowNewTypes = allowNewTypes;
    context.skip = false;
    if (attemptXML) {
        bool success;
        OCFRetain(xmlData);
        context.stringTrie = OCFBurstTrieCreate();
        context.stringCache = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
        context.keyPaths = __CFPropertyListCreateTopLevelKeyPaths(allocator, keyPaths);
        const uint64_t whitespace = UINT64_C(0x100002600);
        for (;;) {
            const uint8_t *p = context.curr;
            const uint8_t *end = context.end;
            if (p >= end) {
                message = OF_STR("Encountered unexpected EOF");
                goto prologError;
            }
            while (*p <= 0x20 && ((UINT64_C(1) << *p) & whitespace) != 0) {
                ++p;
                if (p == end) {
                    p = end;
                    break;
                }
            }
            context.curr = p;
            if (p + 1 >= end) {
                message = OF_STR("No XML content found");
                goto prologError;
            }
            int8_t character = (int8_t)*p;
            if (character != '<') {
                uint32_t line = lineNumber(&context);
                value = NULL;
                success = false;
                context.error = __CFPropertyListCreateError(
                    3840, OF_STR("Unexpected character %c at line %u"), (int)character, line
                );
                goto cleanup;
            }
            context.curr = p + 1;
            uint8_t next = p[1];
            if (next == '?') {
                context.curr = p + 2;
                skipXMLProcessingInstruction(&context);
                goto checkError;
            }
            if (next != '!') {
                break;
            }
            const uint8_t *declaration = p + 2;
            context.curr = declaration;
            if (p + 3 < end && declaration[0] == '-' && p[3] == '-') {
                context.curr = p + 5;
                skipXMLComment(&context);
                goto checkError;
            }
            uint32_t doct = 0;
            uint32_t type = 0;
            if (end - declaration >= 7) {
                __builtin_memcpy(&doct, declaration, 4);
                __builtin_memcpy(&type, p + 5, 4);
            }
            if (end - declaration < 7 || doct != 0x54434f44 || type != 0x45505954) {
                uint32_t line = lineNumber(&context);
                context.error = __CFPropertyListCreateError(3840, OF_STR("Malformed DTD on line %u"), line);
                goto checkError;
            }
            const uint8_t *q = p + 9;
            context.curr = q;
            if (q < end) {
                while (*q <= 0x20 && ((UINT64_C(1) << *q) & whitespace) != 0) {
                    ++q;
                    context.curr = q;
                    if (q == end) {
                        q = end;
                        break;
                    }
                }
            }
            if (q < end) {
                for (;;) {
                    uint8_t c = *q;
                    if (c == '[') {
                        break;
                    }
                    if (c == '>') {
                        context.curr = q + 1;
                        goto checkError;
                    }
                    ++q;
                    context.curr = q;
                    if (q == end) {
                        q = end;
                        break;
                    }
                }
            }
            if (q == end || q >= end) {
                context.error =
                    __CFPropertyListCreateError(3840, OF_STR("Encountered unexpected EOF while parsing DTD"));
                goto checkError;
            }
            for (;;) {
                uint8_t c = *q;
                if (c > '>') {
                    break;
                }
                if (((UINT64_C(1) << c) & whitespace) == 0) {
                    if (c == '>') {
                        context.curr = q + 1;
                        goto checkError;
                    }
                    break;
                }
                ++q;
                context.curr = q;
                if (q == end) {
                    context.error = __CFPropertyListCreateError(
                        3840, OF_STR("Encountered unexpected EOF while parsing DTD")
                    );
                    goto checkError;
                }
            }
            {
                int character = (int8_t)*q;
                uint32_t line = lineNumber(&context);
                context.error = __CFPropertyListCreateError(
                    3840,
                    OF_STR("Encountered unexpected character %c on line %u while parsing DTD"),
                    character,
                    line
                );
            }
        checkError:
            if (context.error != NULL) {
                value = NULL;
                success = false;
                goto cleanup;
            }
        }
        success = parseXMLElement(&context, NULL, &value, 0);
        if (format != NULL && success && value != NULL) {
            *format = kOCFPropertyListXMLFormat_v1_0;
            success = true;
        }
        goto cleanup;
    prologError:
        value = NULL;
        success = false;
        context.error = __CFPropertyListCreateError(3840, message);
    cleanup:
        OCFBurstTrieRelease(context.stringTrie);
        OCFRelease(context.stringCache);
        if (context.keyPaths != NULL) {
            OCFRelease(context.keyPaths);
        }
        OCFRelease(xmlData);
        if (success) {
            *out = value;
            return true;
        }
    }
    if (allowOpenStep) {
        OCFErrorRef oldStyleError = NULL;
        OCFTypeRef result = __CFCreateOldStylePropertyListOrStringsFile(
            allocator,
            xmlData,
            originalString,
            guessedEncoding,
            option,
            outError == NULL ? NULL : &oldStyleError,
            format
        );
        OCFErrorRef xmlError = context.error;
        if (result != NULL) {
            if (xmlError != NULL) {
                OCFRelease(xmlError);
            }
            *out = result;
            return true;
        }
        if (outError == NULL) {
            if (xmlError != NULL) {
                OCFRelease(xmlError);
            }
            return false;
        }
        if (xmlError != NULL && oldStyleError != NULL) {
            OCFDictionaryRef userInfo = OCFErrorCopyUserInfo(xmlError);
            OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
            OCFMutableDictionaryRef merged =
                OCFDictionaryCreateMutableCopy(systemDefault, OCFDictionaryGetCount(userInfo) + 1, userInfo);
            OCFDictionaryAddValue(merged, OF_STR("kCFPropertyListOldStyleParsingError"), oldStyleError);
            OCFErrorDomain domain = OCFErrorGetDomain(xmlError);
            OCFErrorRef error = OCFErrorCreate(systemDefault, domain, OCFErrorGetCode(xmlError), merged);
            OCFRelease(userInfo);
            OCFRelease(merged);
            OCFRelease(oldStyleError);
            OCFRelease(xmlError);
            *outError = error;
            return false;
        }
        if (xmlError != NULL) {
            *outError = xmlError;
            return false;
        }
        if (oldStyleError != NULL) {
            *outError = oldStyleError;
            return false;
        }
        message = OF_STR("Encountered unknown error during parse");
        goto storeNewError;
    }
    if (outError == NULL) {
        if (context.error != NULL) {
            OCFRelease(context.error);
        }
        return false;
    }
    if (attemptXML && context.error != NULL) {
        *outError = context.error;
        return false;
    }
    message = OF_STR("Unsupported property list");
storeNewError:
    *outError = __CFPropertyListCreateError(3840, message);
    return false;
}

// Native _CFPropertyListCreateFromXMLString, 0x1c01d4cb4. External.
// Clears *errorString first. A parse error becomes its debug description (retained) or, when the user info
// carries "kCFPropertyListOldStyleParsingError", a combined message. The error is released.
// CoreFoundation defect (kept): the combined message's second argument is read from the XML error's user
// info, not from the copied old-style user info (which is only copied and released), so it repeats the XML
// debug description.
OCFPropertyListRef _OCFPropertyListCreateFromXMLString(
    OCFAllocatorRef allocator,
    OCFStringRef xmlString,
    OCFOptionFlags option,
    OCFStringRef *errorString,
    Boolean allowNewTypes,
    OCFPropertyListFormat *format
) {
    if (errorString != NULL) {
        *errorString = NULL;
    }
    OCFErrorRef error = NULL;
    OCFPropertyListRef result =
        _OCFPropertyListCreateFromXMLStringError(allocator, xmlString, option, &error, allowNewTypes, format);
    if (errorString == NULL || error == NULL) {
        if (error != NULL) {
            OCFRelease(error);
        }
        return result;
    }
    OCFDictionaryRef userInfo = OCFErrorCopyUserInfo(error);
    OCFErrorRef oldStyleError = NULL;
    Boolean hasOldStyleError = OCFDictionaryGetValueIfPresent(
        userInfo, OF_STR("kCFPropertyListOldStyleParsingError"), (const void **)&oldStyleError
    );
    OCFStringRef debugKey = kOCFErrorDebugDescriptionKey;
    OCFStringRef description = OCFDictionaryGetValue(userInfo, debugKey);
    if (hasOldStyleError) {
        OCFDictionaryRef oldStyleUserInfo = OCFErrorCopyUserInfo(oldStyleError);
        OCFStringRef oldStyleDescription = OCFDictionaryGetValue(userInfo, debugKey);
        description = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("XML parser error:\n\t%@\nOld-style plist parser error:\n\t%@\n"),
            description,
            oldStyleDescription
        );
        OCFRelease(oldStyleUserInfo);
    } else if (description != NULL) {
        OCFRetain(description);
    }
    OCFRelease(userInfo);
    *errorString = description;
    OCFRelease(error);
    return result;
}

// Native _CFPropertyListCreateFiltered, 0x1c01d4dfc. External.
// NULL data or key paths fail without a store. Binary data of at least 8 bytes whose top-level information
// reads is parsed with the top-level key paths and an object table (no key callbacks, CFType values); option
// bit 16 passes the data itself as the backing data. Other data goes through _CFPropertyListCreateWithData
// with new types allowed. The value is stored only on success with a value slot; otherwise a created value is
// released.
bool _OCFPropertyListCreateFiltered(
    OCFAllocatorRef allocator,
    OCFDataRef data,
    OCFOptionFlags option,
    OCFSetRef keyPaths,
    OCFPropertyListRef *value,
    OCFErrorRef *error
) {
    if (data == NULL || keyPaths == NULL) {
        return false;
    }
    uint8_t marker = 0;
    BinaryPlistTrailer trailer = {0};
    uint64_t offset = 0;
    const uint8_t *bytes = OCFDataGetBytePtr(data);
    OCFIndex length = OCFDataGetLength(data);
    OCFPropertyListRef out = NULL;
    bool success;
    if ((uint64_t)length >= 8 && __CFDataGetBinaryPlistTopLevelInfo(data, &marker, &offset, &trailer)) {
        OCFSetRef splitKeyPaths = __CFPropertyListCreateTopLevelKeyPaths(allocator, keyPaths);
        OCFMutableDictionaryRef objects =
            OCFDictionaryCreateMutable(allocator, 0, NULL, &kOCFTypeDictionaryValueCallBacks);
        success = __CFBinaryPlistCreateObjectFiltered(
            bytes,
            (uint64_t)length,
            offset,
            &trailer,
            allocator,
            option,
            (option & 0x10000) != 0 ? data : NULL,
            objects,
            NULL,
            0,
            splitKeyPaths,
            &out,
            NULL
        );
        OCFRelease(splitKeyPaths);
        OCFRelease(objects);
    } else {
        success = _CFPropertyListCreateWithData(allocator, data, option, error, true, NULL, keyPaths, &out);
    }
    if (value != NULL && success) {
        *value = out;
    } else if (out != NULL) {
        OCFRelease(out);
    }
    return success;
}

// Native _CFPropertyListCreateWithData, 0x1c01d4fa0. Non-external.
// Option bits 8, 9 and 10 select the OpenStep, binary and XML readers; when none is set all three are tried.
// NULL or empty data fails with error 3840. The binary reader gets the low option byte and stores
// kCFPropertyListBinaryFormat_v1_0 on success. For the text readers the encoding is sniffed with 32-bit
// length tests: a UTF-32 byte-order mark (skip 4), a UTF-16 byte-order mark (skip 2), a zero first or second
// byte (kCFStringEncodingUnicode, also skip 2), a UTF-8 byte-order mark (skip 3, parsed in place), or an
// "<?xml" declaration whose encoding attribute (before the first '?' or '>') names the encoding: "utf-8" in
// any case of "utf" parses in place, any other name goes through CFStringConvertIANACharSetNameToEncoding
// (the name as an ISO Latin-1 string). UTF-8 data is parsed in place with kCFStringEncodingUTF8 as the guess;
// other encodings are converted to a string and re-encoded as UTF-8 for the parser, with the string as the
// original. An unknown encoding name, a missing name before the end of the buffer, kCFStringEncodingMacRoman
// (0) as the mapped encoding and a failed conversion fail with error 3840 when there is an error slot.
static bool _CFPropertyListCreateWithData(
    OCFAllocatorRef allocator,
    OCFDataRef data,
    OCFOptionFlags option,
    OCFErrorRef *outError,
    Boolean allowNewTypes,
    OCFPropertyListFormat *format,
    OCFSetRef keyPaths,
    OCFTypeRef *out
) {
    uint32_t formatBits = ((uint32_t)option >> 8 | (uint32_t)option >> 9 | (uint32_t)option >> 10) & 0xff;
    bool anyFormat = (formatBits & 1) != 0;
    bool tryXML = anyFormat ? (((uint32_t)option >> 10) & 1) != 0 : true;
    bool tryOpenStep = anyFormat ? (((uint32_t)option >> 8) & 1) != 0 : true;
    OCFStringRef message;
    if (data == NULL || OCFDataGetLength(data) == 0) {
        if (outError == NULL) {
            return false;
        }
        message = OF_STR("Cannot parse a NULL or zero-length data");
        goto storeNewError;
    }
    if ((((uint32_t)option >> 9) & 1) != 0 || !anyFormat) {
        if (__CFTryParseBinaryPlist(allocator, data, option & 0xff, out, NULL)) {
            if (format != NULL) {
                *format = kOCFPropertyListBinaryFormat_v1_0;
            }
            return true;
        }
    }
    if (!tryXML && !tryOpenStep) {
        if (outError == NULL) {
            return false;
        }
        message = OF_STR("Unsupported property list");
        goto storeNewError;
    }
    const uint8_t *bytes = OCFDataGetBytePtr(data);
    uint32_t length = (uint32_t)OCFDataGetLength(data);
    OCFIndex skip = 0;
    OCFStringEncoding encoding = kEncodingUnicode;
    OCFErrorRef encodingError = NULL;
    uint8_t first;
    uint8_t second;
    if (length >= 5) {
        first = bytes[0];
        if (first == 0xff) {
            if (bytes[1] != 0xfe) {
                second = bytes[1];
                goto checkSecondByte;
            }
            if (bytes[2] != 0 || bytes[3] != 0) {
                encoding = kEncodingUTF16LE;
                skip = 2;
                goto convert;
            }
            encoding = kEncodingUTF32LE;
            skip = 4;
            goto convert;
        }
        if (first != 0) {
            goto checkUTF8Mark;
        }
        if (bytes[1] != 0 || bytes[2] != 0xfe || bytes[3] != 0xff) {
            encoding = kEncodingUnicode;
            skip = 2;
            goto convert;
        }
        encoding = kEncodingUTF32BE;
        skip = 4;
        goto convert;
    }
    if (length == 4) {
        first = bytes[0];
        goto checkUTF8Mark;
    }
    if (length < 3) {
        skip = 0;
        goto parseUTF8;
    }
    first = bytes[0];
    goto checkLeadByte;
checkUTF8Mark:
    if (first != 0xef) {
        goto checkLeadByte;
    }
    if (bytes[1] != 0xbb) {
        second = bytes[1];
        goto checkSecondByte;
    }
    if (bytes[2] != 0xbf) {
        goto checkDeclaration;
    }
    skip = 3;
    goto parseUTF8;
checkLeadByte:
    if (first == 0) {
        encoding = kEncodingUnicode;
        skip = 2;
        goto convert;
    }
    if (first == 0xfe) {
        if (bytes[1] == 0xff) {
            encoding = kEncodingUTF16BE;
            skip = 2;
            goto convert;
        }
        second = bytes[1];
        goto checkSecondByte;
    }
    if (first == 0xff) {
        if (bytes[1] == 0xfe) {
            encoding = kEncodingUTF16LE;
            skip = 2;
            goto convert;
        }
        second = bytes[1];
        goto checkSecondByte;
    }
    second = bytes[1];
checkSecondByte:
    if (second == 0) {
        encoding = kEncodingUnicode;
        skip = 2;
        goto convert;
    }
checkDeclaration:
    if (length < 5 || strncmp((const char *)bytes, "<?xml", 5) != 0) {
        skip = 0;
        goto parseUTF8;
    }
    {
        const uint8_t *idx = bytes + 5;
        const uint8_t *end = bytes + length;
        if (length >= 6) {
            uint8_t current = *idx;
            if ((current & 0xfe) == '>') {
                skip = 0;
                goto parseUTF8;
            }
            if (length < 15) {
                goto endOfBuffer;
            }
            for (;;) {
                uint8_t next = idx[1];
                if (current == 'e' && next == 'n' && idx[2] == 'c' && idx[3] == 'o' && idx[4] == 'd' &&
                    idx[5] == 'i' && idx[6] == 'n' && idx[7] == 'g' && idx[8] == '=') {
                    idx += 9;
                    break;
                }
                if ((next & 0xfe) == '>') {
                    skip = 0;
                    goto parseUTF8;
                }
                ++idx;
                current = next;
                if (idx + 9 >= end) {
                    goto endOfBuffer;
                }
            }
        }
        if (idx >= end) {
            skip = 0;
            goto parseUTF8;
        }
        uint8_t quote = *idx;
        if (quote != '\'' && quote != '"') {
            skip = 0;
            goto parseUTF8;
        }
        const uint8_t *base = idx + 1;
        if (base >= end) {
            skip = 0;
            goto parseUTF8;
        }
        uint64_t nameLength = 0;
        uint64_t available = (uint64_t)(end - base);
        while (base[nameLength] != quote) {
            ++nameLength;
            if (nameLength == available) {
                skip = 0;
                goto parseUTF8;
            }
        }
        nameLength = (uint32_t)nameLength;
        if (nameLength == 5 && (base[0] | 0x20) == 'u' && (idx[2] | 0x20) == 't' && (idx[3] | 0x20) == 'f' &&
            idx[4] == '-' && idx[5] == '8') {
            skip = 0;
            goto parseUTF8;
        }
        OCFStringRef name = OCFStringCreateWithBytes(
            kOCFAllocatorSystemDefault, base, (OCFIndex)nameLength, kEncodingISOLatin1, false
        );
        OCFStringEncoding mapped = OCFStringConvertIANACharSetNameToEncoding(name);
        if (mapped == 0xffffffffU) {
            encodingError =
                __CFPropertyListCreateError(3840, OF_STR("Encountered unknown encoding (%@)"), name);
            if (name != NULL) {
                OCFRelease(name);
            }
            goto encodingFailed;
        }
        if (name != NULL) {
            OCFRelease(name);
        }
        if (mapped == kEncodingUTF8) {
            skip = 0;
            goto parseUTF8;
        }
        encodingError = NULL;
        if (mapped != kEncodingMacRoman) {
            encoding = mapped;
            skip = 0;
            goto convert;
        }
        goto encodingFailed;
    }
endOfBuffer:
    encodingError =
        __CFPropertyListCreateError(3840, OF_STR("End of buffer while looking for encoding name"));
encodingFailed:
    if (outError != NULL && encodingError == NULL) {
        message = OF_STR("Could not determine the encoding of the XML data");
        goto storeNewError;
    }
    if (outError != NULL && encodingError != NULL) {
        *outError = encodingError;
        return false;
    }
    if (outError == NULL && encodingError != NULL) {
        OCFRelease(encodingError);
    }
    return false;
convert:
    {
        const uint8_t *converted = OCFDataGetBytePtr(data);
        OCFIndex convertedLength = OCFDataGetLength(data);
        OCFStringRef string =
            OCFStringCreateWithBytes(allocator, converted + skip, convertedLength - skip, encoding, false);
        if (string == NULL) {
            if (outError == NULL) {
                return false;
            }
            message = OF_STR("Could not determine the encoding of the XML data (string creation failed)");
            goto storeNewError;
        }
        OCFDataRef utf8 = _createUTF8DataFromString(allocator, string);
        bool result = _CFPropertyListCreateFromUTF8Data(
            allocator,
            utf8,
            0,
            string,
            0,
            option,
            outError,
            allowNewTypes,
            format,
            keyPaths,
            out,
            tryXML,
            tryOpenStep
        );
        OCFRelease(string);
        if (utf8 != NULL) {
            OCFRelease(utf8);
        }
        return result;
    }
parseUTF8:
    return _CFPropertyListCreateFromUTF8Data(
        allocator,
        data,
        skip,
        NULL,
        kEncodingUTF8,
        option,
        outError,
        allowNewTypes,
        format,
        keyPaths,
        out,
        tryXML,
        tryOpenStep
    );
storeNewError:
    *outError = __CFPropertyListCreateError(3840, message);
    return false;
}

// Native _CFPropertyListGetValueWithKeyPath, 0x1c01d5548. External.
// NULL arguments give NULL. An array key path is retained; anything else is split at ':' in the system
// default allocator. Each component indexes an array (CFStringGetIntValue, sign-extended) or keys a
// dictionary; any other object, or a NULL result before the last component, ends the walk with NULL. An empty
// key path returns the list itself. The components are released.
OCFTypeRef _OCFPropertyListGetValueWithKeyPath(OCFPropertyListRef plist, OCFStringRef keyPath) {
    if (plist == NULL || keyPath == NULL) {
        return NULL;
    }
    OCFArrayRef components;
    if (OCFGetTypeID(keyPath) == 19) {
        components = OCFRetain(keyPath);
    } else {
        components =
            OCFStringCreateArrayBySeparatingStrings(kOCFAllocatorSystemDefault, keyPath, OF_STR(":"));
    }
    OCFIndex count = OCFArrayGetCount(components);
    OCFTypeRef current = plist;
    if (count >= 1) {
        for (OCFIndex index = 1;; ++index) {
            OCFStringRef component = OCFArrayGetValueAtIndex(components, index - 1);
            OCFTypeID typeID = OCFGetTypeID(current);
            if (typeID == 19) {
                current = OCFArrayGetValueAtIndex(current, (OCFIndex)OCFStringGetIntValue(component));
            } else if (typeID == 18) {
                current = OCFDictionaryGetValue(current, component);
            } else {
                current = NULL;
                break;
            }
            if (index >= count || current == NULL) {
                break;
            }
        }
    }
    OCFRelease(components);
    return current;
}

// Native CFPropertyListCreateWithData, 0x1c01d5a40. External.
// Parses with new types allowed and no key paths and returns the value slot (NULL on failure).
OCFPropertyListRef OCFPropertyListCreateWithData(
    OCFAllocatorRef allocator,
    OCFDataRef data,
    OCFOptionFlags options,
    OCFPropertyListFormat *format,
    OCFErrorRef *error
) {
    OCFTypeRef out = NULL;
    _CFPropertyListCreateWithData(allocator, data, options, error, true, format, NULL, &out);
    return out;
}

// Native _CFPropertyListCopyTopLevelKeys, 0x1c01d5aa4. External.
// Binary data of at least 8 bytes with readable top-level information uses the binary reader. Otherwise the
// list is parsed (new types allowed) and a dictionary root's keys are read into a heap buffer
// (malloc_type_malloc, type 0xc0040b8aa526d) and made into a set (kCFTypeSetCallBacks, the caller's
// allocator). Every NULL result with an error slot ends with "Unable to extract top-level keys".
// CoreFoundation defect (kept): a non-dictionary root or a failed key buffer first stores its own error,
// which the final store then overwrites without release.
OCFSetRef _OCFPropertyListCopyTopLevelKeys(
    OCFAllocatorRef allocator, OCFDataRef data, OCFOptionFlags option, OCFErrorRef *outError
) {
    OCFSetRef result = NULL;
    if (data == NULL) {
        goto finish;
    }
    const uint8_t *bytes = OCFDataGetBytePtr(data);
    OCFIndex length = OCFDataGetLength(data);
    if ((uint64_t)length >= 8) {
        uint8_t marker = 0;
        BinaryPlistTrailer trailer = {0};
        uint64_t offset = 0;
        if (__CFDataGetBinaryPlistTopLevelInfo(data, &marker, &offset, &trailer)) {
            result = __CFBinaryPlistCopyTopLevelKeys(allocator, bytes, (uint64_t)length, offset, &trailer);
            goto finish;
        }
    }
    OCFTypeRef plist = NULL;
    _CFPropertyListCreateWithData(allocator, data, option, outError, true, NULL, NULL, &plist);
    if (plist == NULL) {
        goto finish;
    }
    if (OCFGetTypeID(plist) != 18) {
        if (outError != NULL) {
            *outError = __CFPropertyListCreateError(
                3840, OF_STR("Cannot copy top-level keys for plist with non-dictionary root object")
            );
        }
    } else {
        OCFIndex count = OCFDictionaryGetCount(plist);
        const void **keys = malloc_type_malloc((size_t)count * sizeof(void *), kTemporaryStorageTypeID);
        if (keys == NULL) {
            if (outError != NULL) {
                *outError =
                    __CFPropertyListCreateError(3840, OF_STR("Unable to convert string to correct encoding"));
            }
        } else {
            OCFDictionaryGetKeysAndValues(plist, keys, NULL);
            result = OCFSetCreate(allocator, keys, count, &kOCFTypeSetCallBacks);
            free(keys);
        }
    }
    OCFRelease(plist);
finish:
    if (outError != NULL && result == NULL) {
        *outError = __CFPropertyListCreateError(3840, OF_STR("Unable to extract top-level keys"));
    }
    return result;
}

// Native CFPropertyListCreateData, 0x1c01d5e40. External.
// The binary format writes through a stream with allocated buffers (the stream in the system default
// allocator, the buffers in the caller's) and copies kCFStreamPropertyDataWritten when at least one byte was
// written. The XML format validates first (error 3851 with the validation message, which is released) and
// writes without a second validation. The OpenStep format and unknown formats log at level 3 and give NULL.
OCFDataRef OCFPropertyListCreateData(
    OCFAllocatorRef allocator,
    OCFPropertyListRef propertyList,
    OCFPropertyListFormat format,
    OCFOptionFlags options,
    OCFErrorRef *error
) {
    if (format == kOCFPropertyListBinaryFormat_v1_0) {
        OCFWriteStreamRef stream =
            OCFWriteStreamCreateWithAllocatedBuffers(kOCFAllocatorSystemDefault, allocator);
        OCFWriteStreamOpen(stream);
        OCFDataRef data = NULL;
        if (OCFPropertyListWrite(propertyList, stream, kOCFPropertyListBinaryFormat_v1_0, options, error) >=
            1) {
            data = OCFWriteStreamCopyProperty(stream, kOCFStreamPropertyDataWritten);
        }
        OCFWriteStreamClose(stream);
        OCFRelease(stream);
        return data;
    }
    if (format == kOCFPropertyListXMLFormat_v1_0) {
        OCFStringRef validationError = NULL;
        PlistValidationContext context = {true, kOCFPropertyListXMLFormat_v1_0, NULL, &validationError, NULL};
        if (__CFPropertyListIsValidAux(propertyList, &context)) {
            return _OCFPropertyListCreateXMLData(allocator, propertyList, false);
        }
        if (error != NULL) {
            *error = __CFPropertyListCreateError(
                3851,
                OF_STR("Property list invalid for format: %ld (%@)"),
                (long)kOCFPropertyListXMLFormat_v1_0,
                validationError
            );
        }
        if (validationError != NULL) {
            OCFRelease(validationError);
        }
        return NULL;
    }
    if (format == kOCFPropertyListOpenStepFormat) {
        OCFLog(3, OF_STR("Property list format kCFPropertyListOpenStepFormat not supported for writing"));
    } else {
        OCFLog(3, OF_STR("Unknown format option"));
    }
    return NULL;
}

// Native CFPropertyListWrite, 0x1c01d5fd0. External.
// Validates for the requested format first (error 3851 with the validation message, which is released). The
// binary format delegates with a zero estimate. The XML format builds the data (validating again, in the
// system default allocator) and writes it in a loop: a zero write stores "stream is full", a negative write
// wraps the stream's error (when it has one) as the underlying error of a Cocoa error 3851 whose dictionary
// uses the system default allocator; both return 0 after releasing the data. A full write returns the data
// length. The OpenStep format, unknown formats and a failed XML build log at level 3 and return 0.
OCFIndex OCFPropertyListWrite(
    OCFPropertyListRef propertyList,
    OCFWriteStreamRef stream,
    OCFPropertyListFormat format,
    OCFOptionFlags options,
    OCFErrorRef *error
) {
    OCFStringRef validationError = NULL;
    PlistValidationContext context = {true, format, NULL, &validationError, NULL};
    if (!__CFPropertyListIsValidAux(propertyList, &context)) {
        if (error != NULL) {
            *error = __CFPropertyListCreateError(
                3851, OF_STR("Property list invalid for format: %ld (%@)"), (long)format, validationError
            );
        }
        if (validationError != NULL) {
            OCFRelease(validationError);
        }
        return 0;
    }
    if (format == kOCFPropertyListBinaryFormat_v1_0) {
        return __OCFBinaryPlistWrite(propertyList, stream, 0, options, error);
    }
    if (format == kOCFPropertyListXMLFormat_v1_0) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFDataRef data = _OCFPropertyListCreateXMLData(allocator, propertyList, true);
        if (data == NULL) {
            OCFLog(
                3,
                OF_STR(
                    "Property list format kCFPropertyListXMLFormat_v1_0 specified but was not a valid "
                    "property list "
                    "type"
                )
            );
            return 0;
        }
        OCFIndex remaining = OCFDataGetLength(data);
        const UInt8 *bytes = OCFDataGetBytePtr(data);
        while (remaining >= 1) {
            OCFIndex written = OCFWriteStreamWrite(stream, bytes, remaining);
            if (written == 0) {
                if (error != NULL) {
                    *error = __CFPropertyListCreateError(
                        3851, OF_STR("Property list writing could not be completed because stream is full.")
                    );
                }
                OCFRelease(data);
                return 0;
            }
            if (written < 0) {
                OCFErrorRef streamError = OCFWriteStreamCopyError(stream);
                if (streamError != NULL) {
                    if (error != NULL) {
                        OCFMutableDictionaryRef userInfo = OCFDictionaryCreateMutable(
                            allocator,
                            0,
                            &kOCFCopyStringDictionaryKeyCallBacks,
                            &kOCFTypeDictionaryValueCallBacks
                        );
                        OCFDictionarySetValue(
                            userInfo,
                            kOCFErrorDebugDescriptionKey,
                            OF_STR(
                                "Property list writing could not be completed because the stream had an "
                                "unknown "
                                "error."
                            )
                        );
                        OCFDictionarySetValue(userInfo, kOCFErrorUnderlyingErrorKey, streamError);
                        *error = OCFErrorCreate(allocator, kOCFErrorDomainCocoa, 3851, userInfo);
                        OCFRelease(userInfo);
                    }
                    OCFRelease(streamError);
                }
                OCFRelease(data);
                return 0;
            }
            bytes += written;
            remaining -= written;
        }
        OCFIndex length = OCFDataGetLength(data);
        OCFRelease(data);
        return length;
    }
    if (format == kOCFPropertyListOpenStepFormat) {
        OCFLog(3, OF_STR("Property list format kCFPropertyListOpenStepFormat not supported for writing"));
    } else {
        OCFLog(3, OF_STR("Unknown format option"));
    }
    return 0;
}

// Native _CFPropertyListValidateData, 0x1c01d6674. Hidden (was a private external).
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC, confidence: verified.
// iOS Device 24A446: unread. Reading: Local/Evidence/cf-core/iphone-simulator-corpus-delta13.json.
// The binary path passes a NULL plist output and a type ID output at SP+0x20, unlike the pseudocode.
bool _CFPropertyListValidateData(OCFDataRef data, OCFTypeID *outType) {
    uint8_t marker = 0;
    BinaryPlistTrailer trailer = {0};
    uint64_t offset = 0;
    const uint8_t *bytes = OCFDataGetBytePtr(data);
    uint64_t length = (uint64_t)OCFDataGetLength(data);
    if (length >= 8 && __CFDataGetBinaryPlistTopLevelInfo(data, &marker, &offset, &trailer)) {
        OCFTypeID type = 0;
        bool valid = __CFBinaryPlistCreateObjectFiltered(
            bytes,
            length,
            offset,
            &trailer,
            kOCFAllocatorSystemDefault,
            0,
            NULL,
            NULL,
            NULL,
            0,
            NULL,
            NULL,
            &type
        );
        if (outType != NULL) {
            *outType = type;
        }
        return valid;
    }
    OCFPropertyListRef plist = NULL;
    _CFPropertyListCreateWithData(kOCFAllocatorSystemDefault, data, 1, NULL, true, NULL, NULL, &plist);
    if (plist == NULL) {
        return false;
    }
    if (outType != NULL) {
        *outType = OCFGetTypeID(plist);
    }
    OCFRelease(plist);
    return true;
}

// Native CFPropertyListCreateDeepCopy, 0x1c01d67c0 (with .cold.1 0x1c030b860 and .cold.2 0x1c030b870).
// External.
// The list is validated for the binary format (no error slot); an invalid list gives NULL. A tagged pointer
// is returned as is, without a retain, unless mutabilityOption is kCFPropertyListMutableContainersAndLeaves
// (2). Strings and data are copied (mutable copies for option 2), booleans retained, numbers re-created from
// their _CFNumberGetType2 value (the native code reads the value into the validation context's storage),
// dates re-created from their absolute time. Arrays use _arrayDeepImmutableCopy for option 0, otherwise a
// mutable array (kCFTypeArrayCallBacks) filled with deep copies; a failed element releases the array and
// gives NULL. Dictionaries with no entries become empty dictionaries (mutable unless option 0; CFType key and
// value callbacks). Otherwise keys and values are read into one buffer of 2 * count words (on the stack for
// up to 128 entries, otherwise from the system default allocator with type 0xc0040b8aa526d; a failure,
// .cold.1, or a count with any of bits 59-62 set, .cold.2, crashes): every key is copied with
// CFStringCreateCopy and every value deep-copied; a failure releases the copies made so far and gives NULL.
// The dictionary is created from the copies (option 0) or filled with CFDictionarySetValue, and every copy is
// released. Any other type (including the keyed-archiver UID, which validation admits) gives NULL.
OCFPropertyListRef OCFPropertyListCreateDeepCopy(
    OCFAllocatorRef allocator, OCFPropertyListRef propertyList, OCFOptionFlags mutabilityOption
) {
    PlistValidationContext context = {true, kOCFPropertyListBinaryFormat_v1_0, NULL, NULL, NULL};
    if (!__CFPropertyListIsValidAux(propertyList, &context)) {
        return NULL;
    }
    if ((OF_WORD_SHIFT((uintptr_t)propertyList, 63)) != 0 &&
        mutabilityOption != kOCFPropertyListMutableContainersAndLeaves) {
        return propertyList;
    }
    OCFTypeID typeID = OCFGetTypeID(propertyList);
    switch (typeID) {
        case 7:
            if (mutabilityOption == kOCFPropertyListMutableContainersAndLeaves) {
                return OCFStringCreateMutableCopy(allocator, 0, propertyList);
            }
            return OCFStringCreateCopy(allocator, propertyList);
        case 18:
            break;
        case 19: {
            if (mutabilityOption == kOCFPropertyListImmutable) {
                return _arrayDeepImmutableCopy(allocator, propertyList);
            }
            OCFIndex count = OCFArrayGetCount(propertyList);
            OCFMutableArrayRef array = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
            if (array == NULL) {
                return NULL;
            }
            OCFIndex index = 0;
            if (count >= 1) {
                for (; index != count; ++index) {
                    OCFPropertyListRef element = OCFPropertyListCreateDeepCopy(
                        allocator, OCFArrayGetValueAtIndex(propertyList, index), mutabilityOption
                    );
                    if (element == NULL) {
                        break;
                    }
                    OCFArrayAppendValue(array, element);
                    OCFRelease(element);
                }
            }
            if (index == count) {
                return array;
            }
            OCFRelease(array);
            return NULL;
        }
        case 20:
            if (mutabilityOption == kOCFPropertyListMutableContainersAndLeaves) {
                return OCFDataCreateMutableCopy(allocator, 0, propertyList);
            }
            return OCFDataCreateCopy(allocator, propertyList);
        case 21:
            OCFRetain(propertyList);
            return propertyList;
        case 22: {
            OCFNumberType numberType = _OCFNumberGetType2(propertyList);
            uint64_t numberValue[5];
            OCFNumberGetValue(propertyList, numberType, numberValue);
            return OCFNumberCreate(allocator, numberType, numberValue);
        }
        case 42:
            return OCFDateCreate(allocator, OCFDateGetAbsoluteTime(propertyList));
        default:
            return NULL;
    }
    OCFIndex count = OCFDictionaryGetCount(propertyList);
    if (count == 0) {
        if (mutabilityOption != kOCFPropertyListImmutable) {
            return OCFDictionaryCreateMutable(
                allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
            );
        }
        return OCFDictionaryCreate(
            allocator, NULL, NULL, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    if ((((uint64_t)count >> 59) & 0xf) != 0) {
        temporaryStorageCrash();
    }
    uint64_t total = (uint64_t)count * 2;
    uint64_t slots = total < 0x101 ? total : 1;
    const void *stackBuffer[slots];
    bzero(stackBuffer, slots * sizeof(void *) < 0x200 ? slots * sizeof(void *) : 0x200);
    const void **keys = stackBuffer;
    if (total <= 0x100) {
        bzero(keys, (uint64_t)count * 2 * sizeof(void *));
    } else {
        keys = OCFAllocatorAllocateTyped(
            kOCFAllocatorSystemDefault,
            (OCFIndex)((uint64_t)count * 2 * sizeof(void *)),
            kTemporaryStorageTypeID,
            0
        );
        if (keys == NULL) {
            temporaryStorageCrash();
        }
    }
    const void **values = keys + count;
    OCFDictionaryGetKeysAndValues(propertyList, keys, values);
    OCFTypeRef result = NULL;
    if (count >= 1) {
        OCFIndex index = 0;
        for (; index != count; ++index) {
            OCFStringRef key = OCFStringCreateCopy(allocator, keys[index]);
            if (key == NULL) {
                break;
            }
            keys[index] = key;
            OCFPropertyListRef value =
                OCFPropertyListCreateDeepCopy(allocator, values[index], mutabilityOption);
            if (value == NULL) {
                OCFRelease(keys[index]);
                break;
            }
            values[index] = value;
        }
        if (index == count) {
            OCFDictionaryRef dictionary;
            if (mutabilityOption != kOCFPropertyListImmutable) {
                dictionary = OCFDictionaryCreateMutable(
                    allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
                );
            } else {
                dictionary = OCFDictionaryCreate(
                    allocator,
                    keys,
                    values,
                    count,
                    &kOCFTypeDictionaryKeyCallBacks,
                    &kOCFTypeDictionaryValueCallBacks
                );
            }
            for (OCFIndex entry = 0; entry != count; ++entry) {
                if (mutabilityOption != kOCFPropertyListImmutable) {
                    OCFDictionarySetValue((OCFMutableDictionaryRef)dictionary, keys[entry], values[entry]);
                }
                OCFRelease(keys[entry]);
                OCFRelease(values[entry]);
            }
            result = dictionary;
        } else {
            for (OCFIndex entry = 0; entry != index; ++entry) {
                OCFRelease(keys[entry]);
                OCFRelease(values[entry]);
            }
        }
    }
    if (total >= 0x101) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, keys);
    }
    return result;
}

// Native _arrayDeepImmutableCopy, 0x1c01d6c44 (with .cold.1 0x1c030b880 and .cold.2 0x1c030b890).
// Non-external. An empty array becomes CFArrayCreate(allocator, NULL, 0, &kCFTypeArrayCallBacks) (a tail
// call). Otherwise the values are read into a buffer (on the stack for up to 256 values, else from the system
// default allocator with type 0xc0040b8aa526d; a failure, .cold.1, or a count with any of bits 60-63 set,
// .cold.2, crashes) and replaced one by one with immutable deep copies; the first failure stops. When every
// copy succeeded the array is created from them with kCFTypeArrayCallBacks, otherwise the result is NULL;
// the copies made are released either way.
OCFArrayRef _arrayDeepImmutableCopy(OCFAllocatorRef allocator, OCFArrayRef array) {
    OCFIndex count = OCFArrayGetCount(array);
    if (count == 0) {
        return OCFArrayCreate(allocator, NULL, 0, &kOCFTypeArrayCallBacks);
    }
    if (((uint64_t)count >> 60) != 0) {
        temporaryStorageCrash();
    }
    uint64_t slots = (uint64_t)count < 0x101 ? (uint64_t)count : 1;
    const void *stackBuffer[slots];
    bzero(stackBuffer, slots * sizeof(void *) < 0x200 ? slots * sizeof(void *) : 0x200);
    const void **values = stackBuffer;
    if ((uint64_t)count <= 0x100) {
        bzero(values, (uint64_t)count * sizeof(void *));
    } else {
        values = OCFAllocatorAllocateTyped(
            kOCFAllocatorSystemDefault,
            (OCFIndex)((uint64_t)count * sizeof(void *)),
            kTemporaryStorageTypeID,
            0
        );
        if (values == NULL) {
            temporaryStorageCrash();
        }
    }
    OCFArrayGetValues(array, __OCFRangeMake(0, count), values);
    OCFIndex copied = 0;
    for (; copied != count; ++copied) {
        OCFPropertyListRef copy = OCFPropertyListCreateDeepCopy(allocator, values[copied], 0);
        if (copy == NULL) {
            break;
        }
        values[copied] = copy;
    }
    OCFArrayRef result = NULL;
    if (copied == count) {
        result = OCFArrayCreate(allocator, values, count, &kOCFTypeArrayCallBacks);
    }
    for (OCFIndex index = 0; index != copied; ++index) {
        OCFRelease(values[index]);
    }
    if ((uint64_t)count >= 0x101) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, values);
    }
    return result;
}

// Native _plistAppendCharacters, 0x1c01d6e38. Non-external.
// ASCII units are appended through a 400-byte stack buffer, up to 400 at a time; at the first non-ASCII unit
// the rest is wrapped without copying (system default allocator, kCFAllocatorNull contents deallocator),
// converted to UTF-8 (loss byte 0) and appended; a failed wrap or conversion appends nothing more. A length
// below 1 appends nothing. CoreFoundation defect: each pass of the native ASCII loop restarts its unit index
// at 0 instead of at the current location while bounding it by location + 399, so a second pass (a run of
// more than 400 ASCII units) writes up to 800 bytes into the 400-byte buffer, past the stack guard, and
// appends the first units again. The restored loop copies from the current location; for every input the
// native loop handles within its buffer (a first ASCII run of at most 400 units) the output is identical. The
// native callers pass tag names, tabs, _appendEscapedString's 64-unit chunks and the strings of
// _plistAppendString.
void _plistAppendCharacters(OCFMutableDataRef mData, const UniChar *chars, OCFIndex length) {
    OCFIndex location = 0;
    do {
        UInt8 buffer[400];
        OCFIndex count = location;
        while (count < length && count - location < 400 && chars[count] < 0x80) {
            buffer[count - location] = (UInt8)chars[count];
            ++count;
        }
        if (count > location) {
            OCFDataAppendBytes(mData, buffer, count - location);
            location = count;
        }
    } while (length - location > 0 && chars[location] < 0x80);
    if (length - location <= 0) {
        return;
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef string = OCFStringCreateWithCharactersNoCopy(
        allocator, chars + location, length - location, kOCFAllocatorNull
    );
    if (string == NULL) {
        return;
    }
    OCFDataRef data = OCFStringCreateExternalRepresentation(allocator, string, kEncodingUTF8, 0);
    if (data != NULL) {
        const UInt8 *bytes = OCFDataGetBytePtr(data);
        OCFDataAppendBytes(mData, bytes, OCFDataGetLength(data));
        OCFRelease(data);
    }
    OCFRelease(string);
}

// The helpers below reproduce code the native _CFAppendXML0 inlines at each use (no native symbols):
// _appendIndents appends the tabs four at a time for a nonzero count; the tag helpers append "<", the UTF-16
// tag name and the given byte suffix, or "</", the name and ">\n".
static void appendIndents(OCFMutableDataRef xmlString, uint64_t count) {
    if (count == 0) {
        return;
    }
    for (;;) {
        _plistAppendCharacters(xmlString, _appendIndentsTabs, (OCFIndex)(count < 4 ? count : 4));
        if (count <= 4) {
            break;
        }
        count -= 4;
    }
}

static void appendOpenTag(OCFMutableDataRef xmlString, int tag, OCFIndex nameLength) {
    OCFDataAppendBytes(xmlString, (const UInt8 *)"<", 1);
    _plistAppendCharacters(xmlString, CFXMLPlistTagsUnicode[tag], nameLength);
}

static void appendCloseTag(OCFMutableDataRef xmlString, int tag, OCFIndex nameLength) {
    OCFDataAppendBytes(xmlString, (const UInt8 *)"</", 2);
    _plistAppendCharacters(xmlString, CFXMLPlistTagsUnicode[tag], nameLength);
    OCFDataAppendBytes(xmlString, (const UInt8 *)">\n", 2);
}

// Native _CFAppendXML0, 0x1c01d6fd0 (with .cold.1 0x1c030b8a0). Non-external.
// The indentation is appended first for every object, then the element; type IDs are compared as 32-bit
// values. Strings and dictionary keys go through _appendEscapedString. A keyed-archiver UID becomes a
// dictionary with the key "CF$UID" (at indentation + 1, 32-bit) and its value as a 64-bit CFNumber formatted
// with "%@". Integers are formatted with "%@", reals with __CFNumberCopyFormattingDescriptionAsFloat64, dates
// as
// "%04d-%02d-%02dT%02d:%02d:%02dZ" from the Gregorian date in no time zone (the seconds truncated by FCVTZS
// before any output). Empty arrays and dictionaries and the booleans are self-closing. Dictionary keys are
// sorted with CFStringCompare through a mutable array (system default allocator, CFType callbacks), read into
// a buffer of the 32-bit count (on the stack up to 256 keys, otherwise from the system default allocator with
// type 0xc0040b8aa526d; failure crashes, .cold.1). Data is base64-encoded into 88-byte line buffers flushed
// at 76 - 8 * min(indentation, 8) characters, each line preceded by min(indentation, 8) tabs when the
// indentation is nonzero. Other types append only the indentation.
static void _CFAppendXML0(OCFTypeRef object, uint32_t indentation, OCFMutableDataRef xmlString) {
    OCFTypeID typeID = OCFGetTypeID(object);
    uint64_t indents = indentation;
    appendIndents(xmlString, indents);
    uint32_t type = (uint32_t)typeID;
    if (type == 7) {
        appendOpenTag(xmlString, kStringTag, 6);
        OCFDataAppendBytes(xmlString, (const UInt8 *)">", 1);
        _appendEscapedString(object, xmlString);
        appendCloseTag(xmlString, kStringTag, 6);
        return;
    }
    if (_OCFKeyedArchiverUIDGetTypeID() == type) {
        appendOpenTag(xmlString, kDictTag, 4);
        OCFDataAppendBytes(xmlString, (const UInt8 *)">\n", 2);
        uint32_t childIndentation = indentation + 1;
        appendIndents(xmlString, childIndentation);
        appendOpenTag(xmlString, kKeyTag, 3);
        OCFDataAppendBytes(xmlString, (const UInt8 *)">", 1);
        _appendEscapedString(OF_STR("CF$UID"), xmlString);
        appendCloseTag(xmlString, kKeyTag, 3);
        appendIndents(xmlString, childIndentation);
        appendOpenTag(xmlString, kIntegerTag, 7);
        OCFDataAppendBytes(xmlString, (const UInt8 *)">", 1);
        uint64_t value = _OCFKeyedArchiverUIDGetValue(object);
        OCFNumberRef number = OCFNumberCreate(kOCFAllocatorSystemDefault, 4, &value);
        _plistAppendFormat(xmlString, OF_STR("%@"), number);
        OCFRelease(number);
        appendCloseTag(xmlString, kIntegerTag, 7);
        appendIndents(xmlString, indents);
        appendCloseTag(xmlString, kDictTag, 4);
        return;
    }
    switch (type) {
        case 18: {
            uint64_t count = (uint32_t)OCFDictionaryGetCount(object);
            appendOpenTag(xmlString, kDictTag, 4);
            if (count == 0) {
                OCFDataAppendBytes(xmlString, (const UInt8 *)"/>\n", 3);
                return;
            }
            OCFDataAppendBytes(xmlString, (const UInt8 *)">\n", 2);
            uint64_t slots = count < 0x101 ? count : 1;
            const void *stackBuffer[slots];
            bzero(stackBuffer, slots * sizeof(void *) < 0x200 ? slots * sizeof(void *) : 0x200);
            const void **keys = stackBuffer;
            OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
            if (count <= 0x100) {
                bzero(keys, count * sizeof(void *));
            } else {
                keys = OCFAllocatorAllocateTyped(
                    allocator, (OCFIndex)(count * sizeof(void *)), kTemporaryStorageTypeID, 0
                );
                if (keys == NULL) {
                    temporaryStorageCrash();
                }
            }
            OCFDictionaryGetKeysAndValues(object, keys, NULL);
            // The native code passes CFStringCompare itself as the comparator (its flags receive the NULL
            // context).
            OCFMutableArrayRef sorted =
                OCFArrayCreateMutable(allocator, (OCFIndex)count, &kOCFTypeArrayCallBacks);
            OCFArrayReplaceValues(sorted, __OCFRangeMake(0, 0), keys, (OCFIndex)count);
            OCFArraySortValues(
                sorted,
                __OCFRangeMake(0, (OCFIndex)count),
                (OCFComparatorFunction)(void (*)(void))OCFStringCompare,
                NULL
            );
            OCFArrayGetValues(sorted, __OCFRangeMake(0, (OCFIndex)count), keys);
            OCFRelease(sorted);
            uint32_t childIndentation = indentation + 1;
            for (uint64_t index = 0; index != count; ++index) {
                OCFStringRef key = keys[index];
                appendIndents(xmlString, childIndentation);
                appendOpenTag(xmlString, kKeyTag, 3);
                OCFDataAppendBytes(xmlString, (const UInt8 *)">", 1);
                _appendEscapedString(key, xmlString);
                appendCloseTag(xmlString, kKeyTag, 3);
                _CFAppendXML0(OCFDictionaryGetValue(object, key), childIndentation, xmlString);
            }
            if (count >= 0x101) {
                OCFAllocatorDeallocate(allocator, keys);
            }
            appendIndents(xmlString, indents);
            appendCloseTag(xmlString, kDictTag, 4);
            return;
        }
        case 19: {
            uint64_t count = (uint32_t)OCFArrayGetCount(object);
            appendOpenTag(xmlString, kArrayTag, 5);
            if (count == 0) {
                OCFDataAppendBytes(xmlString, (const UInt8 *)"/>\n", 3);
                return;
            }
            OCFDataAppendBytes(xmlString, (const UInt8 *)">\n", 2);
            for (uint64_t index = 0; index != count; ++index) {
                _CFAppendXML0(OCFArrayGetValueAtIndex(object, (OCFIndex)index), indentation + 1, xmlString);
            }
            appendIndents(xmlString, indents);
            appendCloseTag(xmlString, kArrayTag, 5);
            return;
        }
        case 20: {
            appendOpenTag(xmlString, kDataTag, 4);
            OCFDataAppendBytes(xmlString, (const UInt8 *)">\n", 2);
            const uint8_t *bytes = OCFDataGetBytePtr(object);
            OCFIndex length = OCFDataGetLength(object);
            uint64_t lineIndents = indents < 8 ? indents : 8;
            if (length >= 1) {
                char buffer[88];
                int64_t position = 0;
                int64_t maxLineLength = 76 - (int64_t)(lineIndents << 3);
                const uint8_t *p = bytes;
                for (uint64_t index = 0;; ++index) {
                    p = bytes + index;
                    switch (index % 3) {
                        case 2:
                            buffer[position] = __CFPLDataEncodeTable[((p[-1] << 8 | p[0]) >> 6) & 0x3f];
                            buffer[position + 1] = __CFPLDataEncodeTable[p[0] & 0x3f];
                            position += 2;
                            break;
                        case 1:
                            buffer[position] = __CFPLDataEncodeTable[((p[-1] << 8 | p[0]) >> 4) & 0x3f];
                            position += 1;
                            break;
                        default:
                            buffer[position] = __CFPLDataEncodeTable[p[0] >> 2];
                            position += 1;
                            break;
                    }
                    if (position >= maxLineLength) {
                        buffer[position] = '\n';
                        buffer[position + 1] = '\0';
                        if (indentation != 0) {
                            appendIndents(xmlString, lineIndents);
                        }
                        OCFDataAppendBytes(xmlString, (const UInt8 *)buffer, (OCFIndex)strlen(buffer));
                        position = 0;
                    }
                    if (index + 1 == (uint64_t)length) {
                        break;
                    }
                }
                switch ((uint64_t)length % 3) {
                    case 2:
                        buffer[position] = __CFPLDataEncodeTable[(p[0] & 0xf) << 2];
                        buffer[position + 1] = '=';
                        position += 2;
                        break;
                    case 1:
                        buffer[position] = __CFPLDataEncodeTable[(p[0] & 0x3) << 4];
                        buffer[position + 1] = '=';
                        buffer[position + 2] = '=';
                        position += 3;
                        break;
                    default:
                        break;
                }
                if (position >= 1) {
                    buffer[position] = '\n';
                    buffer[position + 1] = '\0';
                    if (indentation != 0) {
                        appendIndents(xmlString, lineIndents);
                    }
                    OCFDataAppendBytes(xmlString, (const UInt8 *)buffer, (OCFIndex)strlen(buffer));
                }
            }
            appendIndents(xmlString, indents);
            appendCloseTag(xmlString, kDataTag, 4);
            return;
        }
        case 21: {
            Boolean value = OCFBooleanGetValue(object);
            OCFDataAppendBytes(xmlString, (const UInt8 *)"<", 1);
            if (value) {
                _plistAppendCharacters(xmlString, CFXMLPlistTagsUnicode[kTrueTag], 4);
            } else {
                _plistAppendCharacters(xmlString, CFXMLPlistTagsUnicode[kFalseTag], 5);
            }
            OCFDataAppendBytes(xmlString, (const UInt8 *)"/>\n", 3);
            return;
        }
        case 22: {
            Boolean isFloat = OCFNumberIsFloatType(object);
            if (isFloat) {
                appendOpenTag(xmlString, kRealTag, 4);
                OCFDataAppendBytes(xmlString, (const UInt8 *)">", 1);
                OCFStringRef description = __CFNumberCopyFormattingDescriptionAsFloat64(object);
                _plistAppendString(xmlString, description);
                OCFRelease(description);
                appendCloseTag(xmlString, kRealTag, 4);
            } else {
                appendOpenTag(xmlString, kIntegerTag, 7);
                OCFDataAppendBytes(xmlString, (const UInt8 *)">", 1);
                _plistAppendFormat(xmlString, OF_STR("%@"), object);
                appendCloseTag(xmlString, kIntegerTag, 7);
            }
            return;
        }
        case 42: {
            OCFGregorianDate date = OCFAbsoluteTimeGetGregorianDate(OCFDateGetAbsoluteTime(object), NULL);
            int32_t second = (int32_t)date.second;
            appendOpenTag(xmlString, kDateTag, 4);
            OCFDataAppendBytes(xmlString, (const UInt8 *)">", 1);
            _plistAppendFormat(
                xmlString,
                OF_STR("%04d-%02d-%02dT%02d:%02d:%02dZ"),
                date.year,
                (int)date.month,
                (int)date.day,
                (int)date.hour,
                (int)date.minute,
                second
            );
            appendCloseTag(xmlString, kDateTag, 4);
            return;
        }
        default:
            return;
    }
}

// Native _appendEscapedString, 0x1c01d7cf4. Non-external. The string is read through an inline buffer
// (CFStringGetCharactersPtr, else the ASCII C-string pointer with sign-extended bytes, else 64-unit refills
// from index - min(index, 4)); units are collected in a 64-unit buffer that is appended with
// _plistAppendCharacters when full, before an escape and at the end. A high surrogate arriving with at least
// 62 units collected flushes them first, so a pair is not split. '&', '<' and '>' are appended as the bytes
// "&amp;", "&lt;" and "&gt;" (CFDataAppendBytes).
void _appendEscapedString(OCFStringRef string, OCFMutableDataRef mData) {
    OCFIndex length = OCFStringGetLength(string);
    OCFStringInlineBuffer inlineBuffer;
    inlineBuffer.theString = string;
    inlineBuffer.rangeToBuffer = __OCFRangeMake(0, length);
    inlineBuffer.directUniCharBuffer = OCFStringGetCharactersPtr(string);
    inlineBuffer.directCStringBuffer =
        inlineBuffer.directUniCharBuffer != NULL ? NULL : OCFStringGetCStringPtr(string, 0x600);
    inlineBuffer.bufferedRangeStart = 0;
    inlineBuffer.bufferedRangeEnd = 0;
    if (length < 1) {
        return;
    }
    memset(inlineBuffer.buffer, 0, sizeof(inlineBuffer.buffer));
    UniChar output[64];
    OCFIndex count = 0;
    for (OCFIndex index = 0; index != length; ++index) {
        uint16_t character;
        if (inlineBuffer.directUniCharBuffer != NULL) {
            character = inlineBuffer.directUniCharBuffer[inlineBuffer.rangeToBuffer.location + index];
        } else if (inlineBuffer.directCStringBuffer != NULL) {
            character = (uint16_t)(int8_t)
                            inlineBuffer.directCStringBuffer[inlineBuffer.rangeToBuffer.location + index];
        } else {
            if (index >= inlineBuffer.bufferedRangeEnd || index < inlineBuffer.bufferedRangeStart) {
                OCFIndex back = (uint64_t)index < 4 ? index : 4;
                OCFIndex start = index - back;
                OCFIndex bufferLength = inlineBuffer.rangeToBuffer.length;
                OCFIndex end = start + 64 < bufferLength ? start + 64 : bufferLength;
                inlineBuffer.bufferedRangeStart = start;
                inlineBuffer.bufferedRangeEnd = end;
                OCFStringGetCharacters(
                    inlineBuffer.theString,
                    __OCFRangeMake(inlineBuffer.rangeToBuffer.location + start, end - start),
                    inlineBuffer.buffer
                );
            }
            character = inlineBuffer.buffer[index - inlineBuffer.bufferedRangeStart];
        }
        if (count >= 0x3E && (character & 0xFC00) == 0xD800) {
            _plistAppendCharacters(mData, output, count);
            count = 0;
        }
        if (character == '&') {
            if (count != 0) {
                _plistAppendCharacters(mData, output, count);
            }
            OCFDataAppendBytes(mData, (const UInt8 *)"&amp;", 5);
            count = 0;
        } else if (character == '>') {
            if (count != 0) {
                _plistAppendCharacters(mData, output, count);
            }
            OCFDataAppendBytes(mData, (const UInt8 *)"&gt;", 4);
            count = 0;
        } else if (character == '<') {
            if (count != 0) {
                _plistAppendCharacters(mData, output, count);
            }
            OCFDataAppendBytes(mData, (const UInt8 *)"&lt;", 4);
            count = 0;
        } else {
            output[count++] = character;
            if (count == 64) {
                _plistAppendCharacters(mData, output, 64);
                count = 0;
            }
        }
    }
    if (count != 0) {
        _plistAppendCharacters(mData, output, count);
    }
}

// Native __plistAppendFormat, iOS Simulator 24A434, 0x1c01d7f84, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; hidden because String/OCFStringPropertyListSupport.c also declares the
// plist appenders. iOS Device 24A446: unread. Formats with the system default allocator and no options,
// appends the result with __plistAppendString (the XML writer's appender; plist family) and releases it.
void _plistAppendFormat(OCFMutableDataRef data, OCFStringRef format, ...) {
    va_list arguments;
    va_start(arguments, format);
    OCFStringRef string =
        OCFStringCreateWithFormatAndArguments(kOCFAllocatorSystemDefault, NULL, format, arguments);
    va_end(arguments);
    _plistAppendString(data, string);
    OCFRelease(string);
}

// Native _plistAppendString, 0x1c01d8010. Non-external.
// Direct UTF-16 contents go to _plistAppendCharacters (a tail call; the length is read after the pointer),
// direct ASCII or else UTF-8 C-string contents are appended with their strlen; otherwise the UTF-8 external
// representation (system default allocator, loss byte 0) is appended when it can be made.
void _plistAppendString(OCFMutableDataRef mData, OCFStringRef str) {
    const UniChar *characters = OCFStringGetCharactersPtr(str);
    if (characters != NULL) {
        _plistAppendCharacters(mData, characters, OCFStringGetLength(str));
        return;
    }
    const char *cString = OCFStringGetCStringPtr(str, kEncodingASCII);
    if (cString == NULL) {
        cString = OCFStringGetCStringPtr(str, kEncodingUTF8);
    }
    if (cString != NULL) {
        OCFDataAppendBytes(mData, (const UInt8 *)cString, (OCFIndex)strlen(cString));
        return;
    }
    OCFDataRef data =
        OCFStringCreateExternalRepresentation(kOCFAllocatorSystemDefault, str, kEncodingUTF8, 0);
    if (data == NULL) {
        return;
    }
    const UInt8 *bytes = OCFDataGetBytePtr(data);
    OCFDataAppendBytes(mData, bytes, OCFDataGetLength(data));
    OCFRelease(data);
}

// Native __CFPropertyListIsArrayPlistAux, 0x1c01d832c. Non-external; the CFArrayApplyFunction applier of
// __CFPropertyListIsValidAux.
// Does nothing once the context failed. A NULL element fails, storing a retained message only into an empty
// error slot; any other element is validated. The result byte is stored.
static void __CFPropertyListIsArrayPlistAux(const void *value, void *context) {
    PlistValidationContext *validation = context;
    if (validation->isValid != true) {
        return;
    }
    bool result;
    if (value != NULL) {
        result = __CFPropertyListIsValidAux(value, validation);
    } else if (validation->error == NULL || *validation->error != NULL) {
        result = false;
    } else {
        *validation->error = OCFRetain(OF_STR("property list arrays cannot contain NULL"));
        result = false;
    }
    validation->isValid = result;
}

// Native __CFPropertyListIsDictPlistAux, 0x1c01d839c. Non-external; the CFDictionaryApplyFunction applier of
// __CFPropertyListIsValidAux.
// Does nothing once the context failed. NULL keys, NULL values, keys that are not strings and, for the XML
// format, keys with a NUL character fail, storing a message only into an empty error slot (the type
// description of a non-string key is released after formatting); otherwise the value is validated. The result
// byte is stored.
static void __CFPropertyListIsDictPlistAux(const void *key, const void *value, void *context) {
    PlistValidationContext *validation = context;
    if (validation->isValid != true) {
        return;
    }
    OCFStringRef message;
    bool result;
    if (key == NULL) {
        message = OF_STR("property list dictionaries cannot contain NULL keys");
        goto storeConstantError;
    }
    if (value == NULL) {
        message = OF_STR("property list dictionaries cannot contain NULL values");
        goto storeConstantError;
    }
    if (OCFGetTypeID(key) != 7) {
        if (validation->error != NULL && *validation->error == NULL) {
            OCFStringRef description = OCFCopyTypeIDDescription(OCFGetTypeID(key));
            *validation->error = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("property list dictionaries may only have keys which are CFStrings, not '%@'"),
                description
            );
            OCFRelease(description);
        }
        result = false;
        goto store;
    }
    if (validation->format == kOCFPropertyListXMLFormat_v1_0 && __CFStringContainsNullCharacter(key)) {
        message =
            OF_STR("property list dictionary keys cannot contain embedded null characters for XML format");
        goto storeConstantError;
    }
    result = __CFPropertyListIsValidAux(value, validation);
    goto store;
storeConstantError:
    result = false;
    if (validation->error != NULL && *validation->error == NULL) {
        *validation->error = OCFRetain(message);
    }
store:
    validation->isValid = result;
}

// Native lineNumber, iOS Simulator 24A434, 0x1c01d84dc, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; callers pass the context to error-message formats with %u.
// iOS Device 24A446: 0x1806f6734, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions.
// Counts lines from begin up to (not including) curr, never reading at or beyond end: LF and CR each
// start a line and a CR followed by LF inside both bounds counts once. The count starts at 1 and is
// computed in 32 bits.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
uint32_t lineNumber(const XMLPlistParseContext *context) {
    const uint8_t *p = context->begin;
    const uint8_t *end = context->end;
    uint32_t count = 1;
    while (p < end && p < context->curr) {
        if (*p == '\n') {
            ++count;
        } else if (*p == '\r') {
            ++count;
            if (p + 1 < end && p + 1 < context->curr && p[1] == '\n') {
                ++p;
            }
        }
        ++p;
    }
    return count;
}

// Native skipXMLComment, 0x1c01d8554. Non-external. Looks for "-->" from the current position; the cursor
// moves past it. Without one (also when fewer than four bytes remain) the context's error becomes
// kCFPropertyListReadCorruptError "Unterminated comment started on line %u" and the cursor stays.
void skipXMLComment(XMLPlistParseContext *context) {
    const uint8_t *cursor = context->curr;
    const uint8_t *end = context->end;
    if (cursor < end - 3) {
        uintptr_t remaining = (uintptr_t)(end - cursor) - 3;
        const uint8_t *position = cursor + 3;
        do {
            if (position[-3] == '-' && position[-2] == '-' && position[-1] == '>') {
                context->curr = position;
                return;
            }
            ++position;
        } while (--remaining != 0);
    }
    context->error = __CFPropertyListCreateError(
        0xF00, OF_STR("Unterminated comment started on line %u"), lineNumber(context)
    );
}

// Native skipXMLProcessingInstruction, 0x1c01d85ec. Non-external. Looks for "?>"; the cursor follows each
// byte examined and finally moves past the terminator. Without one the cursor returns to where it started
// and the context's error becomes kCFPropertyListReadCorruptError "Encountered unexpected EOF while parsing
// the processing instruction begun on line %u".
void skipXMLProcessingInstruction(XMLPlistParseContext *context) {
    const uint8_t *begin = context->curr;
    const uint8_t *end = context->end;
    if (begin < end - 2) {
        uintptr_t remaining = (uintptr_t)(end - begin) - 2;
        const uint8_t *position = begin + 1;
        do {
            if (position[-1] == '?' && position[0] == '>') {
                context->curr = position + 1;
                return;
            }
            context->curr = position;
            ++position;
        } while (--remaining != 0);
    }
    context->curr = begin;
    context->error = __CFPropertyListCreateError(
        0xF00,
        OF_STR("Encountered unexpected EOF while parsing the processing instruction begun on line %u"),
        lineNumber(context)
    );
}

// Native CFXMLPlistTags, 0x1c0375ba8 (__TEXT,__const, 13 tags of 10 bytes, zero padded). Non-external.
static const char CFXMLPlistTags[13][10] = {
    "plist",
    "array",
    "dict",
    "key",
    "string",
    "data",
    "date",
    "real",
    "integer",
    "true",
    "false",
    "DOCTYPE",
    "<![CDATA[",
};

// The parser's tag indexes into CFXMLPlistTags (local descriptive names).
enum {
    kXMLPlistTag = 0,
    kXMLArrayTag = 1,
    kXMLDictTag = 2,
    kXMLKeyTag = 3,
    kXMLStringTag = 4,
    kXMLDataTag = 5,
    kXMLDateTag = 6,
    kXMLRealTag = 7,
    kXMLIntegerTag = 8,
    kXMLTrueTag = 9,
    kXMLFalseTag = 10,
};

// The whitespace set the parser tests with the mask 0x100002600: tab, line feed, carriage return and space.
static inline bool isXMLWhitespace(uint8_t character) {
    return character <= 0x20 && ((UINT64_C(1) << character) & UINT64_C(0x100002600)) != 0;
}

static bool parsePListTag(XMLPlistParseContext *context, OCFTypeRef *out, OCFIndex depth);
static bool parseArrayTag(XMLPlistParseContext *context, OCFTypeRef *out, OCFIndex depth);
static bool parseDictTag(XMLPlistParseContext *context, OCFTypeRef *out, OCFIndex depth);
static bool parseStringTag(XMLPlistParseContext *context, OCFTypeRef *out);
static bool checkForCloseTag(XMLPlistParseContext *context, const char *tag, OCFIndex tagLength);
static bool parseDataTag(XMLPlistParseContext *context, OCFTypeRef *out);
static bool parseDateTag(XMLPlistParseContext *context, OCFTypeRef *out);
static bool parseRealTag(XMLPlistParseContext *context, OCFTypeRef *out);
static bool parseIntegerTag(XMLPlistParseContext *context, OCFTypeRef *out);
static bool getContentObject(XMLPlistParseContext *context, bool *isKey, OCFTypeRef *out, OCFIndex depth);
static OCFStringRef
_createUniqueStringWithUTF8Bytes(XMLPlistParseContext *context, const uint8_t *bytes, OCFIndex length);

// Native parseXMLElement, 0x1c01d8684. Non-external. Entered after '<'. *isKey is cleared first. The tag name
// ends at the first whitespace (tab, LF, CR, space) or at '>', a '/' before '>' excluded; the cursor moves
// past
// '>' (reaching the end without '>' answers false without an error). An empty name rewinds to the name and
// reports "Malformed tag on line %u". The name selects plist, array, dict, key (which sets *isKey and closes
// with three bytes), string, data, date, real, integer, true or false; without the context's allow-new-types
// byte anything but plist, array, dict, key, string and data reports "Encountered new tag when expecting only
// old-style property list objects". Empty tags (a '/' before '>'): plist and data, date, real and integer
// report errors; array, dict and the strings create empty values (mutable for a nonzero mutability option,
// strings only for option 2), or NULL while skipping; true and false accept an empty tag or their close tag
// ("Encountered non-empty <true>/<false> on line %u"). Other tags parse their content (depth + 1 for the
// containers); a string or key is released when its close tag fails. An unknown name reports "Encountered
// unknown tag %@ on line %u" with the name (UTF-8) or "<unknown>", after rewinding to the name. Errors are
// stored in the context (an earlier error is not released).
bool parseXMLElement(XMLPlistParseContext *context, bool *isKey, OCFTypeRef *out, OCFIndex depth) {
    const uint8_t *marker = context->curr;
    if (isKey != NULL) {
        *isKey = false;
    }
    const uint8_t *cursor = context->curr;
    const uint8_t *end = context->end;
    int32_t markerLength = -1;
    if (cursor < end) {
        int32_t offset = (int32_t)(cursor - marker);
        do {
            uint8_t character = *cursor;
            if (character <= '>') {
                if (((UINT64_C(1) << character) & UINT64_C(0x100002600)) != 0) {
                    if (markerLength == -1) {
                        markerLength = offset;
                    }
                } else if (character == '>') {
                    break;
                }
            }
            context->curr = ++cursor;
            ++offset;
        } while (cursor != end);
    }
    if (cursor >= end) {
        return false;
    }
    uint8_t previous = cursor[-1];
    if (markerLength == -1) {
        markerLength = (int32_t)(cursor - marker) - (previous == '/' ? 1 : 0);
    }
    context->curr = cursor + 1;
    if (markerLength == 0) {
        context->curr = marker;
        context->error =
            __CFPropertyListCreateError(0xF00, OF_STR("Malformed tag on line %u"), lineNumber(context));
        return false;
    }
    int32_t tag = -1;
    OCFIndex closeLength = 6;
    switch (marker[0]) {
        case 'a':
            if (markerLength == 5 && memcmp(marker, "array", 5) == 0) {
                tag = kXMLArrayTag;
            }
            break;
        case 'd':
            if (markerLength == 4) {
                if (memcmp(marker, "dict", 4) == 0) {
                    tag = kXMLDictTag;
                } else if (memcmp(marker, "data", 4) == 0) {
                    tag = kXMLDataTag;
                } else if (memcmp(marker, "date", 4) == 0) {
                    tag = kXMLDateTag;
                }
            }
            break;
        case 'f':
            if (markerLength == 5 && memcmp(marker, "false", 5) == 0) {
                tag = kXMLFalseTag;
            }
            break;
        case 'i':
            if (markerLength == 7 && memcmp(marker, "integer", 7) == 0) {
                tag = kXMLIntegerTag;
            }
            break;
        case 'k':
            if (markerLength == 3 && memcmp(marker, "key", 3) == 0) {
                tag = kXMLKeyTag;
                closeLength = 3;
                if (isKey != NULL) {
                    *isKey = true;
                }
            }
            break;
        case 'p':
            if (markerLength == 5 && memcmp(marker, "plist", 5) == 0) {
                tag = kXMLPlistTag;
            }
            break;
        case 'r':
            if (markerLength == 4 && memcmp(marker, "real", 4) == 0) {
                tag = kXMLRealTag;
            }
            break;
        case 's':
            if (markerLength == 6 && memcmp(marker, "string", 6) == 0) {
                tag = kXMLStringTag;
            }
            break;
        case 't':
            if (markerLength == 4 && memcmp(marker, "true", 4) == 0) {
                tag = kXMLTrueTag;
            }
            break;
        default:
            break;
    }
    bool oldStyle = tag >= kXMLPlistTag && tag <= kXMLDataTag;
    if (!oldStyle && !context->allowNewTypes) {
        context->error = __CFPropertyListCreateError(
            0xF00, OF_STR("Encountered new tag when expecting only old-style property list objects")
        );
        return false;
    }
    bool empty = previous == '/';
    switch (tag) {
        case kXMLPlistTag:
            if (empty) {
                context->error = __CFPropertyListCreateError(0xF00, OF_STR("Encountered empty plist tag"));
                return false;
            }
            return parsePListTag(context, out, depth + 1);
        case kXMLArrayTag:
            if (!empty) {
                return parseArrayTag(context, out, depth + 1);
            }
            if (context->skip) {
                *out = NULL;
                return true;
            }
            if (context->mutabilityOption != 0) {
                *out = OCFArrayCreateMutable(context->allocator, 0, &kOCFTypeArrayCallBacks);
            } else {
                *out = OCFArrayCreate(context->allocator, NULL, 0, &kOCFTypeArrayCallBacks);
            }
            return true;
        case kXMLDictTag:
            if (!empty) {
                return parseDictTag(context, out, depth + 1);
            }
            if (context->skip) {
                *out = NULL;
                return true;
            }
            if (context->mutabilityOption != 0) {
                *out = OCFDictionaryCreateMutable(
                    context->allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
                );
            } else {
                *out = OCFDictionaryCreate(
                    context->allocator,
                    NULL,
                    NULL,
                    0,
                    &kOCFTypeDictionaryKeyCallBacks,
                    &kOCFTypeDictionaryValueCallBacks
                );
            }
            return true;
        case kXMLKeyTag:
        case kXMLStringTag:
            if (empty) {
                if (context->skip) {
                    *out = NULL;
                    return true;
                }
                if (context->mutabilityOption == 2) {
                    *out = OCFStringCreateMutable(context->allocator, 0);
                } else {
                    *out = OCFStringCreateWithCharacters(context->allocator, NULL, 0);
                }
                return true;
            }
            if (!parseStringTag(context, out)) {
                return false;
            }
            if (checkForCloseTag(context, CFXMLPlistTags[tag], closeLength)) {
                return true;
            }
            if (*out != NULL) {
                OCFRelease(*out);
            }
            return false;
        case kXMLDataTag:
            if (empty) {
                context->error = __CFPropertyListCreateError(
                    0xF00, OF_STR("Encountered empty <data> on line %u"), lineNumber(context)
                );
                return false;
            }
            return parseDataTag(context, out);
        case kXMLDateTag:
            if (empty) {
                context->error = __CFPropertyListCreateError(
                    0xF00, OF_STR("Encountered empty <date> on line %u"), lineNumber(context)
                );
                return false;
            }
            return parseDateTag(context, out);
        case kXMLRealTag:
            if (empty) {
                context->error = __CFPropertyListCreateError(
                    0xF00, OF_STR("Encountered empty <real> on line %u"), lineNumber(context)
                );
                return false;
            }
            return parseRealTag(context, out);
        case kXMLIntegerTag:
            if (empty) {
                context->error = __CFPropertyListCreateError(
                    0xF00, OF_STR("Encountered empty <integer> on line %u"), lineNumber(context)
                );
                return false;
            }
            return parseIntegerTag(context, out);
        case kXMLTrueTag:
            if (!empty && !checkForCloseTag(context, "true", 4)) {
                context->error = __CFPropertyListCreateError(
                    0xF00, OF_STR("Encountered non-empty <true> on line %u"), lineNumber(context)
                );
                return false;
            }
            *out = context->skip ? NULL : OCFRetain(kOCFBooleanTrue);
            return true;
        case kXMLFalseTag:
            if (!empty && !checkForCloseTag(context, "false", 5)) {
                context->error = __CFPropertyListCreateError(
                    0xF00, OF_STR("Encountered non-empty <false> on line %u"), lineNumber(context)
                );
                return false;
            }
            *out = context->skip ? NULL : OCFRetain(kOCFBooleanFalse);
            return true;
        default: {
            OCFStringRef name = OCFStringCreateWithBytes(
                kOCFAllocatorSystemDefault, marker, (OCFIndex)markerLength, 0x8000100, false
            );
            OCFStringRef shown = name != NULL ? name : OF_STR("<unknown>");
            context->curr = marker;
            context->error = __CFPropertyListCreateError(
                0xF00, OF_STR("Encountered unknown tag %@ on line %u"), shown, lineNumber(context)
            );
            if (name != NULL) {
                OCFRelease(name);
            }
            return false;
        }
    }
}

// Native parsePListTag, 0x1c01d8ef8. Non-external. The content object is read; failing without an error
// reports "Encountered empty plist tag". A second object rewinds to it, releases both and reports
// "Encountered unexpected element at line %u (plist can only include one object)". Otherwise an error or a
// missing "</plist>" releases the object; else it is the result.
static bool parsePListTag(XMLPlistParseContext *context, OCFTypeRef *out, OCFIndex depth) {
    OCFTypeRef result = NULL;
    if (!getContentObject(context, NULL, &result, depth)) {
        if (context->error != NULL) {
            return false;
        }
        context->error = __CFPropertyListCreateError(0xF00, OF_STR("Encountered empty plist tag"));
        return false;
    }
    const uint8_t *save = context->curr;
    OCFTypeRef extra = NULL;
    if (getContentObject(context, NULL, &extra, depth)) {
        if (extra != NULL) {
            OCFRelease(extra);
        }
        if (result != NULL) {
            OCFRelease(result);
        }
        context->curr = save;
        context->error = __CFPropertyListCreateError(
            0xF00,
            OF_STR("Encountered unexpected element at line %u (plist can only include one object)"),
            lineNumber(context)
        );
        return false;
    }
    if (context->error == NULL && checkForCloseTag(context, CFXMLPlistTags[kXMLPlistTag], 5)) {
        *out = result;
        return true;
    }
    if (result != NULL) {
        OCFRelease(result);
    }
    return false;
}

// Native allowImmutableCollections, 0x22e6cf758 (__DATA,__data, 32-bit, initially -1). Non-external. Set on
// first use to whether the CFPropertyListAllowImmutableCollections environment variable exists; 1 makes
// immutable parses copy their mutable containers into immutable ones.
static int32_t allowImmutableCollections = -1;

static inline bool allowsImmutableCollections(void) {
    if (allowImmutableCollections == -1) {
        allowImmutableCollections = getenv("CFPropertyListAllowImmutableCollections") != NULL;
    }
    return allowImmutableCollections == 1;
}

// Native parseArrayTag, 0x1c01d9030. Non-external. While skipping, the values are read and released and
// "</array>" answers NULL. Otherwise a mutable array (CFType callbacks) collects the values; the key paths
// are split (__CFPropertyListCreateSplitKeypaths): with a filter set, each value whose index ("%ld", from 0)
// is neither in the set nor matched by "*" is parsed with the skip byte, the remaining key paths installed
// for it and the previous ones restored after it. NULL values (skipped ones) are not appended. An error or a
// missing "</array>" releases the array. With allowImmutableCollections and mutability option 0 the array
// becomes an immutable copy.
static bool parseArrayTag(XMLPlistParseContext *context, OCFTypeRef *out, OCFIndex depth) {
    OCFTypeRef value = NULL;
    if (context->skip) {
        while (getContentObject(context, NULL, &value, depth)) {
            if (value != NULL) {
                OCFRelease(value);
            }
        }
        if (context->error != NULL) {
            return false;
        }
        if (!checkForCloseTag(context, "array", 5)) {
            return false;
        }
        *out = NULL;
        return true;
    }
    OCFMutableArrayRef array = OCFArrayCreateMutable(context->allocator, 0, &kOCFTypeArrayCallBacks);
    OCFSetRef oldKeyPaths = context->keyPaths;
    OCFSetRef theseKeys = NULL;
    OCFSetRef nextKeys = NULL;
    __CFPropertyListCreateSplitKeypaths(context->allocator, oldKeyPaths, &theseKeys, &nextKeys);
    OCFIndex count;
    bool more;
    if (theseKeys != NULL) {
        OCFStringRef index = OCFStringCreateWithFormat(context->allocator, NULL, OF_STR("%ld"), (OCFIndex)0);
        if (!OCFSetContainsValue(theseKeys, index) && !OCFSetContainsValue(theseKeys, OF_STR("*"))) {
            context->skip = true;
        }
        if (index != NULL) {
            OCFRelease(index);
        }
        context->keyPaths = nextKeys;
        more = getContentObject(context, NULL, &value, depth);
        count = 1;
        context->keyPaths = oldKeyPaths;
        context->skip = false;
    } else {
        more = getContentObject(context, NULL, &value, depth);
        count = 0;
    }
    while (more) {
        if (value != NULL) {
            OCFArrayAppendValue(array, value);
            if (value != NULL) {
                OCFRelease(value);
            }
        }
        if (theseKeys != NULL) {
            OCFStringRef index = OCFStringCreateWithFormat(context->allocator, NULL, OF_STR("%ld"), count);
            if (!OCFSetContainsValue(theseKeys, index) && !OCFSetContainsValue(theseKeys, OF_STR("*"))) {
                context->skip = true;
            }
            if (index != NULL) {
                OCFRelease(index);
            }
            ++count;
            context->keyPaths = nextKeys;
            more = getContentObject(context, NULL, &value, depth);
            context->keyPaths = oldKeyPaths;
            context->skip = false;
        } else {
            more = getContentObject(context, NULL, &value, depth);
        }
    }
    if (nextKeys != NULL) {
        OCFRelease(nextKeys);
    }
    if (theseKeys != NULL) {
        OCFRelease(theseKeys);
    }
    if (context->error != NULL || !checkForCloseTag(context, "array", 5)) {
        if (array != NULL) {
            OCFRelease(array);
        }
        return false;
    }
    OCFTypeRef result = array;
    if (allowsImmutableCollections() && context->mutabilityOption == 0) {
        result = OCFArrayCreateCopy(context->allocator, array);
        if (array != NULL) {
            OCFRelease(array);
        }
    }
    *out = result;
    return true;
}

// Native parseDictTag, 0x1c01d9388. Non-external. While skipping, key and value pairs are read and released
// ("Found non-key inside <dict> at line %u" for a non-key, "Value missing for key inside <dict> at line %u"
// for a missing value, neither replacing an earlier error) and "</dict>" answers NULL. Otherwise the key
// paths are split; with a filter set a value whose key does not pass __CFPropertyListKeyPassesFilterSet is
// parsed with the skip byte, the remaining key paths installed for it. The dictionary (CFType callbacks,
// capacity hint 10) is created at the first pair with both a key and a value. A NULL key ends the content.
// Errors release the pending key, the key paths and the dictionary. An empty dictionary is created at the end
// (mutable for a nonzero option). A single entry "CF$UID" with a number becomes a keyed-archiver UID of its
// SInt32 value; with allowImmutableCollections and option 0 the dictionary becomes an immutable copy.
static bool parseDictTag(XMLPlistParseContext *context, OCFTypeRef *out, OCFIndex depth) {
    bool isKey = false;
    OCFTypeRef key = NULL;
    OCFTypeRef value = NULL;
    if (context->skip) {
        while (getContentObject(context, &isKey, &key, depth)) {
            if (!isKey) {
                if (context->error != NULL) {
                    return false;
                }
                context->error = __CFPropertyListCreateError(
                    0xF00, OF_STR("Found non-key inside <dict> at line %u"), lineNumber(context)
                );
                return false;
            }
            if (!getContentObject(context, NULL, &value, depth)) {
                if (context->error != NULL) {
                    return false;
                }
                context->error = __CFPropertyListCreateError(
                    0xF00, OF_STR("Value missing for key inside <dict> at line %u"), lineNumber(context)
                );
                return false;
            }
            if (key != NULL) {
                OCFRelease(key);
            }
            key = NULL;
            if (value != NULL) {
                OCFRelease(value);
            }
            value = NULL;
        }
        if (!checkForCloseTag(context, "dict", 4)) {
            return false;
        }
        *out = NULL;
        return true;
    }
    OCFSetRef oldKeyPaths = context->keyPaths;
    OCFSetRef theseKeys = NULL;
    OCFSetRef nextKeys = NULL;
    __CFPropertyListCreateSplitKeypaths(context->allocator, oldKeyPaths, &theseKeys, &nextKeys);
    OCFMutableDictionaryRef dict = NULL;
    bool more = getContentObject(context, &isKey, &key, depth);
    while (more && key != NULL) {
        if (!isKey) {
            if (context->error == NULL) {
                context->error = __CFPropertyListCreateError(
                    0xF00, OF_STR("Found non-key inside <dict> at line %u"), lineNumber(context)
                );
            }
            if (key != NULL) {
                OCFRelease(key);
            }
            if (nextKeys != NULL) {
                OCFRelease(nextKeys);
            }
            if (theseKeys != NULL) {
                OCFRelease(theseKeys);
            }
            goto failure;
        }
        bool gotValue;
        if (theseKeys != NULL) {
            if (!__CFPropertyListKeyPassesFilterSet(key, theseKeys)) {
                context->skip = true;
            }
            context->keyPaths = nextKeys;
            gotValue = getContentObject(context, NULL, &value, depth);
            context->keyPaths = oldKeyPaths;
            context->skip = false;
        } else {
            gotValue = getContentObject(context, NULL, &value, depth);
        }
        if (!gotValue) {
            if (context->error == NULL) {
                context->error = __CFPropertyListCreateError(
                    0xF00, OF_STR("Value missing for key inside <dict> at line %u"), lineNumber(context)
                );
            }
            if (key != NULL) {
                OCFRelease(key);
            }
            if (nextKeys != NULL) {
                OCFRelease(nextKeys);
            }
            if (theseKeys != NULL) {
                OCFRelease(theseKeys);
            }
            goto failure;
        }
        if (key != NULL && value != NULL) {
            if (dict == NULL) {
                dict = OCFDictionaryCreateMutable(
                    context->allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
                );
                _OCFDictionarySetCapacity(dict, 10);
            }
            OCFDictionarySetValue(dict, key, value);
        }
        if (key != NULL) {
            OCFRelease(key);
        }
        key = NULL;
        if (value != NULL) {
            OCFRelease(value);
        }
        value = NULL;
        more = getContentObject(context, &isKey, &key, depth);
    }
    if (nextKeys != NULL) {
        OCFRelease(nextKeys);
    }
    if (theseKeys != NULL) {
        OCFRelease(theseKeys);
    }
    if (context->error != NULL || !checkForCloseTag(context, "dict", 4)) {
        goto failure;
    }
    if (dict == NULL) {
        if (context->mutabilityOption != 0) {
            *out = OCFDictionaryCreateMutable(
                context->allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
            );
        } else {
            *out = OCFDictionaryCreate(
                context->allocator,
                NULL,
                NULL,
                0,
                &kOCFTypeDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
        }
        return true;
    }
    if (OCFDictionaryGetCount(dict) == 1) {
        OCFTypeRef uid = OCFDictionaryGetValue(dict, OF_STR("CF$UID"));
        if (uid != NULL && OCFGetTypeID(uid) == 22) {
            int32_t number = 0;
            OCFNumberGetValue(uid, kOCFNumberSInt32Type, &number);
            OCFTypeRef result = _OCFKeyedArchiverUIDCreate(context->allocator, (uint32_t)number);
            OCFRelease(dict);
            *out = result;
            return true;
        }
    }
    OCFTypeRef result = dict;
    if (allowsImmutableCollections() && context->mutabilityOption == 0) {
        result = OCFDictionaryCreateCopy(context->allocator, dict);
        OCFRelease(dict);
    }
    *out = result;
    return true;
failure:
    if (dict != NULL) {
        OCFRelease(dict);
    }
    return false;
}

// Native parseStringTag, 0x1c01d9830. Non-external. An earlier error answers false. Characters up to a '<'
// that does not start "<!" (or the end) form the string. Entities ("&amp;", "&lt;", "&gt;", "&apos;",
// "&quot;", "&#...;" decimal or "&#x...;" hexadecimal of at most eight digits, encoded through UTF-32BE and
// converted to UTF-8) and "<![CDATA[...]]>" sections move the text gathered so far into a mutable data
// buffer. Errors: "Encountered unexpected EOF", "Encountered unknown ampersand-escape sequence at line %u",
// "Encountered improper CDATA opening at line %u", "Could not find end of CDATA started on line %u" (rewound
// to the section), "...too large of a value for a Unicode sequence", "Encountered unexpected character %c at
// line %u while parsing data" and "...input did not result in a real string"; each releases the buffer. While
// skipping the result is NULL. Option 2 makes a mutable copy of the UTF-8 string, other options the uniqued
// string; a failed conversion reports "Unable to convert string to correct encoding".
static bool parseStringTag(XMLPlistParseContext *context, OCFTypeRef *out) {
    if (context->error != NULL) {
        return false;
    }
    OCFMutableDataRef data = NULL;
    const uint8_t *cursor = context->curr;
    const uint8_t *mark = cursor;
    while (cursor < context->end) {
        uint8_t character = *cursor;
        const uint8_t *end = context->end;
        if (character == '&') {
            if (data == NULL) {
                data = OCFDataCreateMutable(context->allocator, 0);
                cursor = context->curr;
            }
            OCFDataAppendBytes(data, mark, cursor - mark);
            cursor = context->curr;
            end = context->end;
            const uint8_t *next = cursor + 1;
            context->curr = next;
            int32_t remaining = (int32_t)(end - next);
            if (remaining <= 0) {
                goto unexpectedEOF;
            }
            uint8_t entity = *next;
            uint8_t replacement;
            OCFIndex advance;
            if (entity == 'g' || entity == 'l') {
                if ((uint32_t)remaining < 3 || cursor[2] != 't' || cursor[3] != ';') {
                    goto unknownEntity;
                }
                replacement = entity == 'g' ? '>' : '<';
                advance = 4;
            } else if (entity == 'q') {
                if ((uint32_t)remaining < 5 || cursor[2] != 'u' || cursor[3] != 'o' || cursor[4] != 't' ||
                    cursor[5] != ';') {
                    goto unknownEntity;
                }
                replacement = '"';
                advance = 6;
            } else if (entity == 'a') {
                if ((uint32_t)remaining <= 3) {
                    goto unexpectedEOF;
                }
                if (cursor[2] == 'm') {
                    if (cursor[3] != 'p' || cursor[4] != ';') {
                        goto unknownEntity;
                    }
                    replacement = '&';
                    advance = 5;
                } else {
                    if (remaining == 4 || cursor[2] != 'p' || cursor[3] != 'o' || cursor[4] != 's' ||
                        cursor[5] != ';') {
                        goto unknownEntity;
                    }
                    replacement = '\'';
                    advance = 6;
                }
            } else if (entity == '#') {
                if ((uint32_t)remaining <= 3) {
                    goto unexpectedEOF;
                }
                const uint8_t *digit = cursor + 2;
                context->curr = digit;
                uint8_t format = cursor[2];
                if (format == 'x') {
                    digit = cursor + 3;
                    context->curr = digit;
                }
                if (digit >= end) {
                    goto unexpectedEOF;
                }
                uint32_t number = 0;
                int32_t digitsLeft = 8;
                for (;;) {
                    int32_t value = (int8_t)*digit++;
                    context->curr = digit;
                    if (value == ';') {
                        break;
                    }
                    if (digitsLeft == 0) {
                        context->error = __CFPropertyListCreateError(
                            0xF00,
                            OF_STR(
                                "Encountered unparseable unicode sequence at line %u while parsing data (too "
                                "large of a value for a Unicode sequence)"
                            ),
                            lineNumber(context)
                        );
                        goto next;
                    }
                    if ((uint32_t)(value - '0') <= 9) {
                        number = (format == 'x' ? number << 4 : number * 10) + (uint32_t)value - '0';
                    } else if (format != 'x') {
                        goto unexpectedCharacter;
                    } else if ((uint32_t)(value - 'a') <= 5) {
                        number = (number << 4) + (uint32_t)value - 0x57;
                    } else if ((uint32_t)(value - 'A') <= 5) {
                        number = (number << 4) + (uint32_t)value - 0x37;
                    } else {
                    unexpectedCharacter:
                        context->error = __CFPropertyListCreateError(
                            0xF00,
                            OF_STR("Encountered unexpected character %c at line %u while parsing data"),
                            value,
                            lineNumber(context)
                        );
                        goto next;
                    }
                    --digitsLeft;
                    if (digit == end) {
                        goto unexpectedEOF;
                    }
                }
                uint32_t bigEndian = __builtin_bswap32(number);
                OCFStringRef string = OCFStringCreateWithBytes(
                    context->allocator, (const UInt8 *)&bigEndian, 4, 0x18000100, false
                );
                if (string == NULL) {
                    context->error = __CFPropertyListCreateError(
                        0xF00,
                        OF_STR(
                            "Encountered unparseable Unicode sequence at line %u while parsing data (input "
                            "did "
                            "not result in a real string)"
                        ),
                        lineNumber(context)
                    );
                    goto next;
                }
                UInt8 utf8[6];
                OCFIndex used = 0;
                OCFStringGetBytes(
                    string, __OCFRangeMake(0, OCFStringGetLength(string)), 0x8000100, 0, false, utf8, 6, &used
                );
                OCFDataAppendBytes(data, utf8, used);
                OCFRelease(string);
                goto next;
            } else {
                goto unknownEntity;
            }
            context->curr = cursor + advance;
            OCFDataAppendBytes(data, &replacement, 1);
            goto next;
        unknownEntity:
            context->error = __CFPropertyListCreateError(
                0xF00, OF_STR("Encountered unknown ampersand-escape sequence at line %u"), lineNumber(context)
            );
            goto next;
        } else if (character == '<') {
            if (cursor + 1 >= end || cursor[1] != '!') {
                break;
            }
            if (data == NULL) {
                data = OCFDataCreateMutable(context->allocator, 0);
                cursor = context->curr;
            }
            OCFDataAppendBytes(data, mark, cursor - mark);
            cursor = context->curr;
            end = context->end;
            if (end - cursor <= 8) {
                goto unexpectedEOF;
            }
            if (memcmp(cursor, "<![CDATA[", 9) != 0) {
                context->error = __CFPropertyListCreateError(
                    0xF00, OF_STR("Encountered improper CDATA opening at line %u"), lineNumber(context)
                );
                goto next;
            }
            const uint8_t *start = cursor + 9;
            context->curr = start;
            for (const uint8_t *position = start; position < end - 2; ++position) {
                if (position[0] == ']' && position[1] == ']' && position[2] == '>') {
                    OCFDataAppendBytes(data, start, position - start);
                    context->curr += 3;
                    goto next;
                }
                context->curr = position + 1;
            }
            context->curr = start;
            context->error = __CFPropertyListCreateError(
                0xF00, OF_STR("Could not find end of CDATA started on line %u"), lineNumber(context)
            );
            goto next;
        } else {
            context->curr = ++cursor;
            if (context->error != NULL) {
                goto failure;
            }
            continue;
        }
    unexpectedEOF:
        context->error = __CFPropertyListCreateError(0xF00, OF_STR("Encountered unexpected EOF"));
    next:
        cursor = context->curr;
        mark = cursor;
        if (context->error != NULL) {
            goto failure;
        }
    }
    if (data != NULL) {
        if (context->skip) {
            *out = NULL;
            OCFRelease(data);
            return true;
        }
        OCFDataAppendBytes(data, mark, cursor - mark);
        OCFTypeRef result;
        if (context->mutabilityOption == 2) {
            OCFStringRef string = OCFStringCreateWithBytes(
                context->allocator, OCFDataGetBytePtr(data), OCFDataGetLength(data), 0x8000100, false
            );
            if (string == NULL) {
                OCFRelease(data);
                goto conversionFailure;
            }
            result = OCFStringCreateMutableCopy(context->allocator, 0, string);
            OCFRelease(string);
        } else {
            result =
                _createUniqueStringWithUTF8Bytes(context, OCFDataGetBytePtr(data), OCFDataGetLength(data));
            if (result == NULL) {
                OCFRelease(data);
                goto conversionFailure;
            }
        }
        *out = result;
        OCFRelease(data);
        return true;
    }
    if (context->skip) {
        *out = NULL;
        return true;
    }
    if (context->mutabilityOption == 2) {
        OCFStringRef string =
            OCFStringCreateWithBytes(context->allocator, mark, cursor - mark, 0x8000100, false);
        if (string == NULL) {
            goto conversionFailure;
        }
        *out = OCFStringCreateMutableCopy(context->allocator, 0, string);
        OCFRelease(string);
        return true;
    }
    OCFStringRef unique = _createUniqueStringWithUTF8Bytes(context, mark, cursor - mark);
    if (unique == NULL) {
        goto conversionFailure;
    }
    *out = unique;
    return true;
conversionFailure:
    context->error =
        __CFPropertyListCreateError(0xF00, OF_STR("Unable to convert string to correct encoding"));
    return false;
failure:
    if (data != NULL) {
        OCFRelease(data);
    }
    return false;
}

// Native checkForCloseTag, 0x1c01d9f00. Non-external. Needs tagLength + 3 bytes; then '<', '/', the tag bytes
// (memcmp), optional whitespace and '>', after which the cursor follows the '>'. Errors, created only while
// the context has none: "Encountered unexpected EOF", "Encountered unexpected character %c on line %u while
// looking for close tag" (the byte sign-extended) and "Close tag on line %u does not match open tag %@" (the
// expected tag as UTF-8; the string is released either way).
static bool checkForCloseTag(XMLPlistParseContext *context, const char *tag, OCFIndex tagLength) {
    const uint8_t *cursor = context->curr;
    const uint8_t *end = context->end;
    if (end - cursor < tagLength + 3) {
        goto unexpectedEOF;
    }
    int32_t character = cursor[0];
    if (character == '<') {
        context->curr = cursor + 1;
        character = cursor[1];
        if (character == '/') {
            const uint8_t *name = cursor + 2;
            context->curr = name;
            if (memcmp(name, tag, (size_t)tagLength) != 0) {
                OCFStringRef expected = OCFStringCreateWithBytes(
                    kOCFAllocatorSystemDefault, (const UInt8 *)tag, tagLength, 0x8000100, false
                );
                if (context->error == NULL) {
                    context->error = __CFPropertyListCreateError(
                        0xF00,
                        OF_STR("Close tag on line %u does not match open tag %@"),
                        lineNumber(context),
                        expected
                    );
                }
                OCFRelease(expected);
                return false;
            }
            const uint8_t *position = name + tagLength;
            context->curr = position;
            while (position < end && isXMLWhitespace(*position)) {
                context->curr = ++position;
            }
            if (position == end) {
                goto unexpectedEOF;
            }
            character = (int8_t)*position;
            if (character == '>') {
                context->curr = position + 1;
                return true;
            }
            if (context->error != NULL) {
                return false;
            }
            goto unexpectedCharacter;
        }
    }
    if (context->error != NULL) {
        return false;
    }
    character = (int8_t)character;
unexpectedCharacter:
    context->error = __CFPropertyListCreateError(
        0xF00,
        OF_STR("Encountered unexpected character %c on line %u while looking for close tag"),
        character,
        lineNumber(context)
    );
    return false;
unexpectedEOF:
    if (context->error == NULL) {
        context->error = __CFPropertyListCreateError(0xF00, OF_STR("Encountered unexpected EOF"));
    }
    return false;
}

// Native parseDataTag.dataDecodeTable, 0x1c0375c2a (__TEXT,__const; the 128 entries read for bytes below
// 0x80). Non-external. Base64 digit values, 0 for '=', -1 elsewhere.
static const int8_t dataDecodeTable[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, -1, -1, -1, 0,  -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
};

// Native parseDataTag, 0x1c01da0b0. Non-external. Base64 is decoded up to '<' or the end into a buffer of 256
// bytes from the context's allocator (type 0x100004077774924; none while skipping) grown with
// __CFSafelyReallocateWithAllocatorTyped (times 4 below 0x40000 bytes, then times 2 below 2^24, then plus
// 0x40000) when fewer than three bytes remain. '=' counts padding, any other non-space byte resets the count
// (isspace); bytes from 0x80 report "Could not interpret <data> on line %u (invalid character 0x%hhX)" and
// free the buffer. Every fourth digit emits three bytes, or two with one '=' and one with more. While
// skipping
// "</data>" answers NULL. Option 2 copies into a mutable CFData and frees the buffer; otherwise the buffer
// becomes CFData owned by the context's allocator. A failed CFData rewinds and reports "Could not interpret
// <data> at line %u (should be base64-encoded)"; a missing "</data>" releases it.
static bool parseDataTag(XMLPlistParseContext *context, OCFTypeRef *out) {
    const uint8_t *start = context->curr;
    uint8_t *bytes = NULL;
    if (!context->skip) {
        bytes = OCFAllocatorAllocateTyped(context->allocator, 0x100, 0x100004077774924, 0);
    }
    OCFIndex length = 0;
    const uint8_t *cursor = context->curr;
    if (cursor < context->end) {
        int32_t digits = 0;
        uint32_t accumulator = 0;
        int32_t padding = 0;
        int32_t used = 0;
        int32_t capacity = 0x100;
        do {
            uint8_t character = *cursor;
            if (character == '=') {
                ++padding;
            } else if (character == '<') {
                break;
            } else if ((character & 0x80) != 0) {
                (void)__maskrune(character, 0x4000);
                context->error = __CFPropertyListCreateError(
                    0xF00,
                    OF_STR("Could not interpret <data> on line %u (invalid character 0x%hhX)"),
                    lineNumber(context),
                    (int)character
                );
                if (bytes != NULL) {
                    OCFAllocatorDeallocate(context->allocator, bytes);
                }
                return false;
            } else if (!isspace(character)) {
                padding = 0;
            }
            int32_t value = dataDecodeTable[character];
            if (value >= 0) {
                ++digits;
                accumulator = (uint32_t)value + (accumulator << 6);
                if (!context->skip && (digits & 3) == 0) {
                    OCFIndex position = used;
                    if (capacity <= (int32_t)(position + 2)) {
                        int32_t grown = (capacity >> 24) != 0 ? capacity + 0x40000 : capacity << 1;
                        capacity = capacity >= 0x40000 ? grown : capacity << 2;
                        bytes = __OCFSafelyReallocateWithAllocatorTyped(
                            context->allocator, bytes, capacity, 0x679EC776, 0, NULL
                        );
                    }
                    bytes[position] = (uint8_t)(accumulator >> 16);
                    if (padding > 1) {
                        used = (int32_t)(position + 1);
                    } else {
                        bytes[position + 1] = (uint8_t)(accumulator >> 8);
                        if (padding == 1) {
                            used = (int32_t)(position + 2);
                        } else {
                            used = (int32_t)position + 3;
                            bytes[position + 2] = (uint8_t)accumulator;
                        }
                    }
                }
            }
            cursor = context->curr + 1;
            context->curr = cursor;
        } while (cursor < context->end);
        length = used;
    }
    OCFDataRef result;
    if (context->skip) {
        if (!checkForCloseTag(context, "data", 4)) {
            return false;
        }
        *out = NULL;
        return true;
    }
    if (context->mutabilityOption == 2) {
        OCFMutableDataRef mutableData = OCFDataCreateMutable(context->allocator, 0);
        OCFDataAppendBytes(mutableData, bytes, length);
        OCFAllocatorDeallocate(context->allocator, bytes);
        result = mutableData;
    } else {
        result = OCFDataCreateWithBytesNoCopy(context->allocator, bytes, length, context->allocator);
    }
    if (result == NULL) {
        context->curr = start;
        context->error = __CFPropertyListCreateError(
            0xF00,
            OF_STR("Could not interpret <data> at line %u (should be base64-encoded)"),
            lineNumber(context)
        );
        return false;
    }
    if (!checkForCloseTag(context, "data", 4)) {
        OCFRelease(result);
        return false;
    }
    *out = result;
    return true;
}

// Native parseDateTag, 0x1c01da38c. Non-external. "[-]YYYY...-MM-DDTHH:MM:SSZ" (any number of year digits,
// the cursor following each byte read) then "</date>"; any mismatch, including a failed close tag (whose own
// error is overwritten without a release, native), reports "Could not interpret <date> at line %u". The
// fields are converted with CFGregorianDateGetAbsoluteTime (no time zone; the seconds as an unsigned
// conversion to double) and the date is created unless skipping (NULL).
static bool parseDateTag(XMLPlistParseContext *context, OCFTypeRef *out) {
    const uint8_t *cursor = context->curr;
    const uint8_t *end = context->end;
    bool positive = true;
    if (cursor < end && *cursor == '-') {
        positive = false;
        context->curr = ++cursor;
    }
    if (cursor >= end) {
        goto failure;
    }
    int32_t year = 0;
    for (;;) {
        int32_t character = (int8_t)*cursor;
        if ((uint32_t)(character - '0') > 9) {
            if (character != '-') {
                goto failure;
            }
            break;
        }
        year = year * 10 + character - '0';
        context->curr = ++cursor;
        if (cursor == end) {
            goto failure;
        }
    }
    const uint8_t *p = cursor;
    int32_t fields[10];
    static const uint8_t separators[5] = {'-', 'T', ':', ':', 'Z'};
    context->curr = p + 1;
    for (int index = 0; index < 5; ++index) {
        const uint8_t *pair = p + 1 + 3 * index;
        if (pair + 2 >= end) {
            goto failure;
        }
        fields[2 * index] = (int8_t)pair[0];
        fields[2 * index + 1] = (int8_t)pair[1];
        context->curr = pair + 2;
        if ((uint32_t)(fields[2 * index] - '0') > 9 || (uint32_t)(fields[2 * index + 1] - '0') > 9) {
            goto failure;
        }
        if (pair[2] != separators[index]) {
            goto failure;
        }
        context->curr = pair + 3;
    }
    if (!checkForCloseTag(context, "date", 4)) {
        goto failure;
    }
    OCFGregorianDate date;
    date.year = positive ? year : -year;
    date.month = (int8_t)(fields[0] * 10 + fields[1] - 0x210);
    date.day = (int8_t)(fields[2] * 10 + fields[3] - 0x210);
    date.hour = (int8_t)(fields[4] * 10 + fields[5] - 0x210);
    date.minute = (int8_t)(fields[6] * 10 + fields[7] - 0x210);
    date.second = (double)(uint32_t)(fields[8] * 10 + fields[9] - 0x210);
    OCFAbsoluteTime at = OCFGregorianDateGetAbsoluteTime(date, NULL);
    *out = context->skip ? NULL : OCFDateCreate(context->allocator, at);
    return true;
failure:
    context->error = __CFPropertyListCreateError(
        0xF00, OF_STR("Could not interpret <date> at line %u"), lineNumber(context)
    );
    return false;
}

// Native parseRealTag, 0x1c01da680. Non-external. The content is parsed as a string; failing without an error
// reports "Encountered empty <real> on line %u". While skipping the result is NULL. "nan", "+infinity",
// "-infinity", "infinity", "-inf", "inf" and "+inf" (case-insensitive CFStringCompare, in this order) answer
// the retained NaN and infinity constants; anything else is scanned with __CFStringScanDouble through an
// inline buffer and must be consumed entirely (CFNumber of kCFNumberDoubleType), else the string is released
// and "Encountered misformatted real on line %u" reported. A missing "</real>" releases the number.
static bool parseRealTag(XMLPlistParseContext *context, OCFTypeRef *out) {
    OCFStringRef string = NULL;
    if (!parseStringTag(context, (OCFTypeRef *)&string)) {
        if (context->error != NULL) {
            return false;
        }
        context->error = __CFPropertyListCreateError(
            0xF00, OF_STR("Encountered empty <real> on line %u"), lineNumber(context)
        );
        return false;
    }
    OCFTypeRef result = NULL;
    if (!context->skip) {
        OCFNumberRef constant = NULL;
        bool named = true;
        if (OCFStringCompare(string, OF_STR("nan"), 1) == 0) {
            constant = kOCFNumberNaN;
        } else if (OCFStringCompare(string, OF_STR("+infinity"), 1) == 0) {
            constant = kOCFNumberPositiveInfinity;
        } else if (OCFStringCompare(string, OF_STR("-infinity"), 1) == 0) {
            constant = kOCFNumberNegativeInfinity;
        } else if (OCFStringCompare(string, OF_STR("infinity"), 1) == 0) {
            constant = kOCFNumberPositiveInfinity;
        } else if (OCFStringCompare(string, OF_STR("-inf"), 1) == 0) {
            constant = kOCFNumberNegativeInfinity;
        } else if (
            OCFStringCompare(string, OF_STR("inf"), 1) == 0 ||
            OCFStringCompare(string, OF_STR("+inf"), 1) == 0
        ) {
            constant = kOCFNumberPositiveInfinity;
        } else {
            named = false;
        }
        if (named && constant != NULL) {
            result = OCFRetain(constant);
        } else {
            OCFIndex length = OCFStringGetLength(string);
            OCFStringInlineBuffer buffer;
            memset(buffer.buffer, 0, sizeof(buffer.buffer));
            buffer.theString = string;
            buffer.rangeToBuffer = __OCFRangeMake(0, length);
            buffer.directUniCharBuffer = OCFStringGetCharactersPtr(string);
            buffer.directCStringBuffer =
                buffer.directUniCharBuffer != NULL ? NULL : OCFStringGetCStringPtr(string, 0x600);
            buffer.bufferedRangeStart = 0;
            buffer.bufferedRangeEnd = 0;
            SInt32 index = 0;
            double value = 0;
            if (!__CFStringScanDouble(&buffer, NULL, &index, &value) || length != (OCFIndex)index) {
                if (string != NULL) {
                    OCFRelease(string);
                }
                context->error = __CFPropertyListCreateError(
                    0xF00, OF_STR("Encountered misformatted real on line %u"), lineNumber(context)
                );
                return false;
            }
            result = OCFNumberCreate(context->allocator, kOCFNumberDoubleType, &value);
        }
    }
    if (string != NULL) {
        OCFRelease(string);
    }
    if (!checkForCloseTag(context, "real", 4)) {
        if (result != NULL) {
            OCFRelease(result);
        }
        return false;
    }
    *out = result;
    return true;
}

// The whitespace the integer parser skips (one byte at a time, also inside a UTF-8 sequence): bytes below
// 0x21, bytes 0x7f-0xa0, and with at least three bytes left the leading byte of U+2000-U+200B (0xe2 0x80
// 0x80-0x8b; the range test is 0x50-0x8b on the third byte), U+202F, U+205F and U+3000.
static inline bool integerSkips(const uint8_t *position, OCFIndex remaining) {
    uint8_t character = position[0];
    if (character < 0x21 || (uint32_t)(character - 0x7F) < 0x22) {
        return true;
    }
    if (remaining < 3 || (character & 0xFE) != 0xE2) {
        return false;
    }
    uint8_t second = position[1];
    uint8_t third = position[2];
    if (character == 0xE2 && second == 0x80) {
        return third == 0xAF || (uint32_t)(third - 0x50) <= 0x3B;
    }
    if (character == 0xE2 && second == 0x81) {
        return third == 0x9F;
    }
    return character == 0xE3 && second == 0x80 && third == 0x80;
}

// Native parseIntegerTag, 0x1c01da918. Non-external. Leading whitespace (integerSkips) is skipped, then an
// optional sign (more whitespace may follow it), an optional "0x" or "0X" prefix and leading zeros. Running
// out of input reports "Premature end of file after <integer> on line %u"; '<' at once "Encountered empty
// <integer> on line %u", '<' after zeros alone the value 0 (SInt32), '<' after only a sign or prefix
// "Incomplete <integer> on line %u". Digits accumulate unsigned in base 10 or 16 with overflow checks
// ("Integer overflow in <integer> on line %u"); a negative value beyond 2^63 reports "Integer underflow
// ...", a hexadecimal digit without the prefix "Hex digit in non-hex <integer> ...", anything else "Unknown
// character '%c' (0x%x) in <integer> on line %u". After "</integer>": NULL while skipping, a 128-bit number
// for positive values from 2^63, else SInt64 (negated for a minus sign).
static bool parseIntegerTag(XMLPlistParseContext *context, OCFTypeRef *out) {
    const uint8_t *cursor = context->curr;
    const uint8_t *end = context->end;
    while (cursor < end && integerSkips(cursor, end - cursor)) {
        context->curr = ++cursor;
    }
    if (cursor == end) {
        goto prematureEnd;
    }
    bool negative = false;
    if (*cursor == '<') {
        context->error = __CFPropertyListCreateError(
            0xF00, OF_STR("Encountered empty <integer> on line %u"), lineNumber(context)
        );
        return false;
    }
    if (*cursor == '-' || *cursor == '+') {
        negative = *cursor == '-';
        context->curr = ++cursor;
        while (cursor < end && integerSkips(cursor, end - cursor)) {
            context->curr = ++cursor;
        }
    }
    if (cursor == end) {
        goto prematureEnd;
    }
    bool isHex = false;
    bool sawZero = false;
    if (*cursor == '0') {
        if (cursor + 1 < end && (cursor[1] | 0x20) == 'x') {
            isHex = true;
            ++cursor;
        } else {
            sawZero = true;
        }
        context->curr = ++cursor;
    }
    if (cursor == end) {
        goto prematureEnd;
    }
    uint8_t character = *cursor;
    while (character == '0') {
        context->curr = ++cursor;
        if (cursor == end) {
            goto prematureEnd;
        }
        character = *cursor;
        sawZero = true;
    }
    if (sawZero && character == '<') {
        int32_t zero = 0;
        if (!checkForCloseTag(context, "integer", 7)) {
            return false;
        }
        *out = context->skip ? NULL : OCFNumberCreate(context->allocator, kOCFNumberSInt32Type, &zero);
        return true;
    }
    if (character == '<') {
        context->error = __CFPropertyListCreateError(
            0xF00, OF_STR("Incomplete <integer> on line %u"), lineNumber(context)
        );
        return false;
    }
    uint64_t value = 0;
    uint64_t limit = isHex ? UINT64_C(0x0FFFFFFFFFFFFFFF) : UINT64_C(0x1999999999999999);
    uint64_t base = isHex ? 16 : 10;
    const uint8_t *next = cursor + 1;
    for (;;) {
        int32_t signedCharacter = (int8_t)character;
        uint32_t digit;
        if (character > '@') {
            if ((uint32_t)(character - 'A') < 6 || (uint32_t)(character - 'a') < 6) {
                if (!isHex) {
                    context->error = __CFPropertyListCreateError(
                        0xF00, OF_STR("Hex digit in non-hex <integer> on line %u"), lineNumber(context)
                    );
                    return false;
                }
                digit = (uint32_t)(signedCharacter + ((uint32_t)(character - 'A') < 6 ? -0x37 : -0x57));
            } else {
                goto unknownCharacter;
            }
        } else if ((uint32_t)(character - '0') < 10) {
            digit = (uint32_t)(signedCharacter - '0');
        } else if (character == '<') {
            break;
        } else {
        unknownCharacter:
            context->error = __CFPropertyListCreateError(
                0xF00,
                OF_STR("Unknown character '%c' (0x%x) in <integer> on line %u"),
                signedCharacter,
                signedCharacter,
                lineNumber(context)
            );
            return false;
        }
        if (limit < value) {
            goto overflow;
        }
        uint64_t scaled = value * base;
        if (scaled + digit < scaled) {
            goto overflow;
        }
        value = scaled + digit;
        if (value >= UINT64_C(0x8000000000000001) && negative) {
            context->error = __CFPropertyListCreateError(
                0xF00, OF_STR("Integer underflow in <integer> on line %u"), lineNumber(context)
            );
            return false;
        }
        context->curr = next;
        if (next == end) {
            goto prematureEnd;
        }
        character = *next++;
    }
    if (!checkForCloseTag(context, "integer", 7)) {
        return false;
    }
    if (context->skip) {
        *out = NULL;
        return true;
    }
    if ((int64_t)value < 0 && !negative) {
        uint64_t wide[2] = {0, value};
        *out = OCFNumberCreate(context->allocator, (OCFNumberType)17, wide);
    } else {
        int64_t signedValue = negative ? -(int64_t)value : (int64_t)value;
        *out = OCFNumberCreate(context->allocator, kOCFNumberSInt64Type, &signedValue);
    }
    return true;
overflow:
    context->error = __CFPropertyListCreateError(
        0xF00, OF_STR("Integer overflow in <integer> on line %u"), lineNumber(context)
    );
    return false;
prematureEnd:
    context->error = __CFPropertyListCreateError(
        0xF00, OF_STR("Premature end of file after <integer> on line %u"), lineNumber(context)
    );
    return false;
}

// Native getContentObject, 0x1c01dae38. Non-external. A depth above 128 reports "Too many nested arrays or
// dictionaries, failing on line %u". *isKey is cleared; an earlier error answers false. Whitespace is
// skipped;
// "<?" processing instructions and "<!--" comments are skipped (their errors end the scan); "</" rewinds to
// the '<' and answers false (the end of a container); any other "<" starts parseXMLElement (a tail call).
// Running out after '<' or a "<!" without "--" reports "Encountered unexpected EOF"; a byte above '<' or a
// non-whitespace control byte "Encountered unexpected character %c on line %u while looking for open tag";
// the end of input before any '<' answers false without an error.
static bool getContentObject(XMLPlistParseContext *context, bool *isKey, OCFTypeRef *out, OCFIndex depth) {
    if ((uint64_t)depth >= 0x81) {
        context->error = __CFPropertyListCreateError(
            0xF00, OF_STR("Too many nested arrays or dictionaries, failing on line %u"), lineNumber(context)
        );
        return false;
    }
    if (isKey != NULL) {
        *isKey = false;
    }
    if (context->error != NULL) {
        return false;
    }
    for (;;) {
        const uint8_t *cursor = context->curr;
        const uint8_t *end = context->end;
        if (cursor >= end) {
            return false;
        }
        uint8_t character = *cursor;
        while (character <= '<' && ((UINT64_C(1) << character) & UINT64_C(0x100002600)) != 0) {
            context->curr = ++cursor;
            if (cursor == end) {
                goto unexpectedEOF;
            }
            character = *cursor;
        }
        if (character != '<') {
            context->error = __CFPropertyListCreateError(
                0xF00,
                OF_STR("Encountered unexpected character %c on line %u while looking for open tag"),
                (int32_t)(int8_t)character,
                lineNumber(context)
            );
            return false;
        }
        const uint8_t *next = cursor + 1;
        context->curr = next;
        if (next >= end) {
            goto unexpectedEOF;
        }
        uint8_t kind = *next;
        if (kind == '!') {
            if (cursor + 3 >= end || cursor[2] != '-' || cursor[3] != '-') {
                goto unexpectedEOF;
            }
            context->curr = cursor + 4;
            skipXMLComment(context);
        } else if (kind == '?') {
            skipXMLProcessingInstruction(context);
        } else if (kind == '/') {
            context->curr = cursor;
            return false;
        } else {
            return parseXMLElement(context, isKey, out, depth);
        }
        if (context->error != NULL) {
            return false;
        }
    }
unexpectedEOF:
    context->error = __CFPropertyListCreateError(0xF00, OF_STR("Encountered unexpected EOF"));
    return false;
}

// Native _createUniqueStringWithUTF8Bytes, 0x1c01dafd0. Non-external. An empty run answers the retained
// constant empty string. A run the context's burst trie holds with a nonzero payload answers the retained
// cache entry at payload - 1. Otherwise the string is created from UTF-8 with the context's allocator (NULL
// on failure) and, when the trie accepts the run with payload count + 1 (32-bit), appended to the cache; the
// new string is answered.
static OCFStringRef
_createUniqueStringWithUTF8Bytes(XMLPlistParseContext *context, const uint8_t *bytes, OCFIndex length) {
    if (length == 0) {
        return OCFRetain(kOCFEmptyString);
    }
    uint32_t payload = 0;
    if (OCFBurstTrieContainsUTF8String(context->stringTrie, bytes, length, &payload) && payload != 0) {
        OCFStringRef cached = OCFArrayGetValueAtIndex(context->stringCache, (OCFIndex)payload - 1);
        OCFRetain(cached);
        return cached;
    }
    OCFStringRef string = OCFStringCreateWithBytes(context->allocator, bytes, length, 0x8000100, false);
    if (string == NULL) {
        return NULL;
    }
    payload = (uint32_t)OCFArrayGetCount(context->stringCache) + 1;
    if (OCFBurstTrieAddUTF8String(context->stringTrie, bytes, length, payload)) {
        OCFArrayAppendValue(context->stringCache, string);
    }
    return string;
}
