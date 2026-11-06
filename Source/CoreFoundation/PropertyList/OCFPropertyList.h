//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFBag.h"
#include "CoreFoundation/Collections/OCFBurstTrie.h"
#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include <OpenCoreFoundation/Date/OCFDate.h>
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/Stream/OCFStream.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/String/OCFStringPropertyListSupport.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Public CFPropertyList interface, iOS Simulator SDK CFPropertyList.h, spelled with the OCF prefix.
typedef OCFTypeRef OCFPropertyListRef;

typedef enum : OCFOptionFlags {
    kOCFPropertyListImmutable = 0,
    kOCFPropertyListMutableContainers = 1 << 0,
    kOCFPropertyListMutableContainersAndLeaves = 1 << 1,
} OCFPropertyListMutabilityOptions;

typedef enum : OCFIndex {
    kOCFPropertyListOpenStepFormat = 1,
    kOCFPropertyListXMLFormat_v1_0 = 100,
    kOCFPropertyListBinaryFormat_v1_0 = 200,
} OCFPropertyListFormat;

enum : OCFIndex {
    kOCFPropertyListReadCorruptError = 3840,
    kOCFPropertyListReadUnknownVersionError = 3841,
    kOCFPropertyListReadStreamError = 3842,
    kOCFPropertyListWriteStreamError = 3851,
};

// The XML property-list parser context, iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified for every offset and width: lineNumber reads begin (0), curr (8) and end (16), and
// _CFPropertyListCreateFromUTF8Data (0x1c01d470c) builds the whole 88-byte record on its stack (sp + 0x30):
// the error (24), the allocator (32), the mutability option as a 32-bit store of its low byte (40), the burst
// trie (48), the string cache array (56), the allow-new-types byte (64), the top-level key paths (72) and the
// skip byte (80): the trie maps a string's UTF-8 bytes to its 1-based index in the cache; the skip byte makes
// the element parsers consume values without creating them (key-path filtering). XMLPlistParseContext and its
// field names are local descriptive spellings.
typedef struct {
    const uint8_t *begin;
    const uint8_t *curr;
    const uint8_t *end;
    OCFErrorRef error;
    OCFAllocatorRef allocator;
    uint32_t mutabilityOption;
    OCFBurstTrieRef stringTrie;
    OCFMutableArrayRef stringCache;
    bool allowNewTypes;
    OCFSetRef keyPaths;
    bool skip;
} XMLPlistParseContext;

_Static_assert(sizeof(XMLPlistParseContext) == 88, "The native XML parser context is 88 bytes.");
_Static_assert(offsetof(XMLPlistParseContext, mutabilityOption) == 40, "Mutability option at 40.");
_Static_assert(offsetof(XMLPlistParseContext, allowNewTypes) == 64, "Allow-new-types byte at 64.");
_Static_assert(offsetof(XMLPlistParseContext, keyPaths) == 72, "Key paths at 72.");
_Static_assert(offsetof(XMLPlistParseContext, skip) == 80, "Skip byte at 80.");

// The validation context __CFPropertyListIsValidAux and its two applier callbacks share, iOS Simulator
// 24A434; confidence: verified: the result byte (0, cleared by the callbacks on failure), the format (8), the
// set of containers on the current path (16), the error-string slot (24) and the bag counting nested
// containers (32). Every native caller builds it on its stack with the result byte 1, bytes 1-7 zero and the
// other words set. PlistValidationContext and its field names are local descriptive spellings.
typedef struct {
    bool isValid;
    OCFPropertyListFormat format;
    OCFMutableSetRef set;
    OCFStringRef *error;
    OCFMutableBagRef bag;
} PlistValidationContext;

_Static_assert(sizeof(PlistValidationContext) == 40, "The native validation context is 40 bytes.");
_Static_assert(offsetof(PlistValidationContext, error) == 24, "Error slot at 24.");

// Property-list functions of the units in this directory that other units call. Signatures read from the
// native definitions and their call sites (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC).
OF_EXTERN bool _CFPropertyListValidateData(OCFDataRef data, OCFTypeID *outType);

OF_EXTERN OCFErrorRef __CFPropertyListCreateError(OCFIndex code, OCFStringRef format, ...);
OF_EXTERN OCFSetRef __CFPropertyListCreateTopLevelKeyPaths(OCFAllocatorRef allocator, OCFSetRef keyPaths);
OF_EXTERN void __CFPropertyListCreateSplitKeypaths(
    OCFAllocatorRef allocator, OCFSetRef currentKeys, OCFSetRef *theseKeys, OCFSetRef *nextKeys
);
OF_EXTERN bool __CFPropertyListKeyPassesFilterSet(OCFStringRef key, OCFSetRef filterSet);
OF_EXTERN bool __CFPropertyListFilterKeyHasWildcard(OCFStringRef key);

// Public CFPropertyList functions (iOS Simulator SDK CFPropertyList.h signatures).
OF_EXPORT Boolean OCFPropertyListIsValid(OCFPropertyListRef plist, OCFPropertyListFormat format);
OF_EXPORT OCFPropertyListRef OCFPropertyListCreateWithData(
    OCFAllocatorRef allocator,
    OCFDataRef data,
    OCFOptionFlags options,
    OCFPropertyListFormat *format,
    OCFErrorRef *error
);
OF_EXPORT OCFDataRef OCFPropertyListCreateData(
    OCFAllocatorRef allocator,
    OCFPropertyListRef propertyList,
    OCFPropertyListFormat format,
    OCFOptionFlags options,
    OCFErrorRef *error
);
OF_EXPORT OCFIndex OCFPropertyListWrite(
    OCFPropertyListRef propertyList,
    OCFWriteStreamRef stream,
    OCFPropertyListFormat format,
    OCFOptionFlags options,
    OCFErrorRef *error
);
OF_EXPORT OCFPropertyListRef OCFPropertyListCreateDeepCopy(
    OCFAllocatorRef allocator, OCFPropertyListRef propertyList, OCFOptionFlags mutabilityOption
);

// Private exported functions (no SDK declaration; signatures read from the native code and its call sites).
OF_EXPORT OCFDataRef _OCFPropertyListCreateXMLData(
    OCFAllocatorRef allocator, OCFPropertyListRef propertyList, Boolean checkValidPlist
);
OF_EXPORT OCFDataRef
_OCFPropertyListCreateXMLDataWithExtras(OCFAllocatorRef allocator, OCFPropertyListRef propertyList);
OF_EXPORT OCFPropertyListRef _OCFPropertyListCreateFromXMLStringError(
    OCFAllocatorRef allocator,
    OCFStringRef xmlString,
    OCFOptionFlags option,
    OCFErrorRef *error,
    Boolean allowNewTypes,
    OCFPropertyListFormat *format
);
OF_EXPORT OCFPropertyListRef _OCFPropertyListCreateFromXMLString(
    OCFAllocatorRef allocator,
    OCFStringRef xmlString,
    OCFOptionFlags option,
    OCFStringRef *errorString,
    Boolean allowNewTypes,
    OCFPropertyListFormat *format
);
OF_EXPORT bool _OCFPropertyListCreateFiltered(
    OCFAllocatorRef allocator,
    OCFDataRef data,
    OCFOptionFlags option,
    OCFSetRef keyPaths,
    OCFPropertyListRef *value,
    OCFErrorRef *error
);
OF_EXPORT OCFTypeRef _OCFPropertyListGetValueWithKeyPath(OCFPropertyListRef plist, OCFStringRef keyPath);
OF_EXPORT OCFSetRef _OCFPropertyListCopyTopLevelKeys(
    OCFAllocatorRef allocator, OCFDataRef data, OCFOptionFlags option, OCFErrorRef *outError
);

// The XML writer's character appenders (non-external natively; the escaped-string and format appenders of the
// same unit, restored by another sub-track, call them).
OF_EXTERN void _plistAppendCharacters(OCFMutableDataRef mData, const UniChar *chars, OCFIndex length);
OF_EXTERN void _plistAppendString(OCFMutableDataRef mData, OCFStringRef str);

// The 13 XML tag names as UTF-16 (plist, array, dict, key, string, data, date, real, integer, true, false,
// DOCTYPE, <![CDATA[), ten units each with zero padding: native CFXMLPlistTagsUnicode, 0x1c0375a9c
// (__TEXT,__const), non-external; the XML parser of the same unit reads it too.
OF_EXTERN const UniChar CFXMLPlistTagsUnicode[13][10];

// Functions of this directory's XML unit that the property-list functions call across their definitions
// (non-external natively).
OF_EXTERN OCFDataRef _createUTF8DataFromString(OCFAllocatorRef allocator, OCFStringRef string);
OF_EXTERN bool parseXMLElement(XMLPlistParseContext *context, bool *isKey, OCFTypeRef *out, OCFIndex depth);
OF_EXTERN void skipXMLComment(XMLPlistParseContext *context);
OF_EXTERN void skipXMLProcessingInstruction(XMLPlistParseContext *context);
OF_EXTERN void _appendEscapedString(OCFStringRef string, OCFMutableDataRef mData);
OF_EXTERN void _plistAppendFormat(OCFMutableDataRef mData, OCFStringRef format, ...);
OF_EXTERN OCFArrayRef _arrayDeepImmutableCopy(OCFAllocatorRef allocator, OCFArrayRef array);

// Original names are preserved. __CFPropertyListKeyBufferMatchesFilterBuffer was a private external
// and lineNumber is non-external; no process-shared symbol is exported.
OF_EXTERN bool __CFPropertyListKeyBufferMatchesFilterBuffer(
    const char *key, OCFIndex keyLength, const char *filter, OCFIndex filterLength
);

OF_EXTERN uint32_t lineNumber(const XMLPlistParseContext *context);
