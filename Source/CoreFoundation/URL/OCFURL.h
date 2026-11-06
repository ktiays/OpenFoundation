//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/String/OCFString.h"

#include <stdbool.h>
#include <stdint.h>

typedef const struct __OCFURL *OCFURLRef;

// CFURLPathStyle (SDK CFURL.h).
typedef enum : OCFIndex {
    kOCFURLPOSIXPathStyle = 0,
    kOCFURLHFSPathStyle = 1,
    kOCFURLWindowsPathStyle = 2,
} OCFURLPathStyle;

// The CFURL object, iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for
// every offset named here (__CFURLCreateWithURLString, __CFURLDeallocate, CFURLGetString). Type and field
// names are local descriptive spellings. Two layouts share the object:
// - CF parsing (the NSSwiftURL feature disabled): flags at 0x10 (bits 0..8 mark the components whose ranges
//   follow, bit 14 asks for a per-component scan, bit 16 marks a string that needed no sanitizing,
//   bits 29..31 name a well-known scheme: 1 http, 2 https, 3 file, 4 data, 5 ftp), the encoding at 0x14, the
//   string at 0x18 (acquire loads and release stores where the native code uses them), the base URL at 0x20,
//   the additional data at 0x28, the resource info at 0x30 (read by __CFURLResourceInfoPtr, released by
//   deallocation), and one 16-byte range per
//   set component bit at 0x38, in bit order. Instances are created with 0x28 + 16 * ranges extra bytes.
// - Swift parsing (the feature enabled): +[NSURL __swiftParseSmall:into:allowEncoding:] (0x48 extra bytes) or
//   __swiftParseBig:into:allowEncoding: (0xa8) fills the object from 0x10; CF reads its flags at 0x10 (bit 2
//   a scheme, bits 0..1 the scheme range form, bit 12 a file scheme, bit 13 a file reference, bit 14 a
//   directory path), the string at 0x18, the base URL at 0x20 (stored by CF), an optional replacement string
//   at 0x30, and the scheme range at 0x38 (32 bits, location in the low half and length in the high half, or
//   two 64-bit words). The remaining Swift layout is unread.
typedef struct {
    void *unread0;
    OCFStringRef sanitizedString;
    UInt32 additionalFlags;
} OCFURLAdditionalData;

_Static_assert(sizeof(OCFURLAdditionalData) == 0x18, "The native additional data block is 24 bytes.");

struct __OCFURL {
    uintptr_t isa;
    uint64_t info;
    UInt32 flags;
    OCFStringEncoding encoding;
    OCFStringRef string;
    OCFURLRef baseURL;
    OCFURLAdditionalData *extra;
    OCFTypeRef resourceInfo;
    OCFRange ranges[];
};

_Static_assert(__builtin_offsetof(struct __OCFURL, ranges) == 0x38, "Component ranges start at 0x38.");

// SDK CFURL.h interfaces (external natively).
OF_EXPORT OCFTypeID OCFURLGetTypeID(void);
OF_EXPORT OCFStringRef OCFURLCreateStringByReplacingPercentEscapes(
    OCFAllocatorRef allocator, OCFStringRef originalString, OCFStringRef charactersToLeaveEscaped
);
OF_EXPORT OCFStringRef OCFURLCreateStringByReplacingPercentEscapesUsingEncoding(
    OCFAllocatorRef allocator,
    OCFStringRef origString,
    OCFStringRef charsToLeaveEscaped,
    OCFStringEncoding encoding
);
OF_EXPORT OCFStringRef OCFURLCreateStringByAddingPercentEscapes(
    OCFAllocatorRef allocator,
    OCFStringRef originalString,
    OCFStringRef charactersToLeaveUnescaped,
    OCFStringRef legalURLCharactersToBeEscaped,
    OCFStringEncoding encoding
);
OF_EXPORT OCFStringRef OCFURLGetString(OCFURLRef anURL);
OF_EXPORT OCFURLRef
OCFURLCreateWithString(OCFAllocatorRef allocator, OCFStringRef URLString, OCFURLRef baseURL);
OF_EXPORT OCFURLRef OCFURLCopyAbsoluteURL(OCFURLRef relativeURL);
OF_EXPORT OCFStringRef OCFURLCopyScheme(OCFURLRef anURL);
OF_EXPORT Boolean OCFURLIsFileReferenceURL(OCFURLRef url);
OF_EXPORT Boolean OCFURLHasDirectoryPath(OCFURLRef anURL);
OF_EXPORT OCFURLRef OCFURLCreateFilePathURL(OCFAllocatorRef allocator, OCFURLRef url, OCFErrorRef *error);
OF_EXPORT OCFURLRef
OCFURLCreateFileReferenceURL(OCFAllocatorRef allocator, OCFURLRef url, OCFErrorRef *error);
OF_EXPORT Boolean OCFURLCopyResourcePropertyForKey(
    OCFURLRef url, OCFStringRef key, void *propertyValueTypeRefPtr, OCFErrorRef *error
);
OF_EXPORT OCFDictionaryRef
OCFURLCopyResourcePropertiesForKeys(OCFURLRef url, OCFArrayRef keys, OCFErrorRef *error);
OF_EXPORT Boolean OCFURLSetResourcePropertyForKey(
    OCFURLRef url, OCFStringRef key, OCFTypeRef propertyValue, OCFErrorRef *error
);
OF_EXPORT Boolean
OCFURLSetResourcePropertiesForKeys(OCFURLRef url, OCFDictionaryRef keyedPropertyValues, OCFErrorRef *error);
OF_EXPORT void OCFURLClearResourcePropertyCacheForKey(OCFURLRef url, OCFStringRef key);
OF_EXPORT void OCFURLClearResourcePropertyCache(OCFURLRef url);
OF_EXPORT void
OCFURLSetTemporaryResourcePropertyForKey(OCFURLRef url, OCFStringRef key, OCFTypeRef propertyValue);
OF_EXPORT Boolean OCFURLStartAccessingSecurityScopedResource(OCFURLRef url);
OF_EXPORT void OCFURLStopAccessingSecurityScopedResource(OCFURLRef url);
OF_EXPORT OCFURLRef OCFURLGetBaseURL(OCFURLRef anURL);
OF_EXPORT OCFStringRef OCFURLCopyNetLocation(OCFURLRef anURL);
OF_EXPORT OCFStringRef OCFURLCopyPath(OCFURLRef anURL);
OF_EXPORT OCFStringRef OCFURLCopyResourceSpecifier(OCFURLRef anURL);
OF_EXPORT OCFStringRef OCFURLCopyQueryString(OCFURLRef anURL, OCFStringRef charactersToLeaveEscaped);
OF_EXPORT OCFURLRef OCFURLCreateFromFileSystemRepresentation(
    OCFAllocatorRef allocator, const UInt8 *buffer, OCFIndex bufLen, Boolean isDirectory
);
OF_EXPORT OCFURLRef OCFURLCreateWithFileSystemPath(
    OCFAllocatorRef allocator, OCFStringRef filePath, OCFURLPathStyle pathStyle, Boolean isDirectory
);
OF_EXPORT OCFURLRef OCFURLCreateWithFileSystemPathRelativeToBase(
    OCFAllocatorRef allocator,
    OCFStringRef filePath,
    OCFURLPathStyle pathStyle,
    Boolean isDirectory,
    OCFURLRef baseURL
);
OF_EXPORT OCFStringRef OCFURLCopyFileSystemPath(OCFURLRef anURL, OCFURLPathStyle pathStyle);
OF_EXPORT Boolean OCFURLGetFileSystemRepresentation(
    OCFURLRef url, Boolean resolveAgainstBase, UInt8 *buffer, OCFIndex maxBufLen
);
OF_EXPORT OCFURLRef OCFURLCreateFromFileSystemRepresentationRelativeToBase(
    OCFAllocatorRef allocator, const UInt8 *buffer, OCFIndex bufLen, Boolean isDirectory, OCFURLRef baseURL
);
OF_EXPORT OCFStringRef OCFURLCopyLastPathComponent(OCFURLRef url);
OF_EXPORT OCFURLRef OCFURLCreateCopyAppendingPathComponent(
    OCFAllocatorRef allocator, OCFURLRef url, OCFStringRef pathComponent, Boolean isDirectory
);

// External natively without an SDK declaration; signatures from the native code and call sites.
OF_EXPORT OCFStringRef _OCFURLCopyPath(OCFURLRef anURL, Boolean removePercentEscapes);
// The resource info word of the URL (__CFURLResourceInfoPtr, 0x1c0247bbc) and the resource-property cache
// brackets (__CFURLBeginResourcePropertyCacheAccess 0x1c024ce78, __CFURLEndResourcePropertyCacheAccess
// 0x1c024cea0); external natively, no SDK declaration (signatures from the register use and the NSURL call
// sites; confidence: medium for the two brackets' void results, which are tail calls).
OF_EXPORT void *__OCFURLResourceInfoPtr(OCFURLRef url);
OF_EXPORT void __OCFURLBeginResourcePropertyCacheAccess(OCFURLRef url);
OF_EXPORT void __OCFURLEndResourcePropertyCacheAccess(OCFURLRef url);
OF_EXPORT OCFURLRef _OCFURLCreateCurrentDirectoryURL(OCFAllocatorRef allocator);
OF_EXPORT OCFStringRef OCFURLCreateStringWithFileSystemPath(
    OCFAllocatorRef allocator, OCFURLRef anURL, OCFURLPathStyle fsType, Boolean resolveAgainstBase
);

// Original private-external names are preserved (hidden; no process-shared symbol is exported).
OF_EXTERN const struct __OCFRuntimeClass __CFURLClass;

// Original non-external names are preserved; no process-shared symbol is exported. ranges is the packed
// component-range array: one 16-byte range per set flag bit, in bit order (bit 0 first).
OF_EXTERN OCFRange _netLocationRange(uint32_t flags, const OCFRange *ranges);

OF_EXTERN Boolean _CFURLHasFileURLScheme(OCFURLRef url, Boolean *hasScheme);

// Native _CFURLIsFileURL, 0x1c0249db0, external (no SDK declaration).
OF_EXPORT Boolean _OCFURLIsFileURL(OCFURLRef url);
OF_EXTERN OCFURLRef _CFURLCreateWithURLString(
    OCFAllocatorRef allocator, OCFStringRef string, Boolean checkForLegalCharacters, OCFURLRef baseURL
);
OF_EXTERN OCFURLRef
_CFURLCreateWithArbitraryString(OCFAllocatorRef allocator, OCFStringRef URLString, OCFURLRef baseURL);
OF_EXTERN uint32_t _CFURLGetQueryResolveFlags(OCFURLRef url);
OF_EXTERN void _CFURLAllocateExtraDataspace(struct __OCFURL *url);
OF_EXTERN OCFURLRef _CFURLCreateWithFileSystemPath(
    OCFAllocatorRef allocator,
    OCFStringRef filePath,
    OCFURLPathStyle pathStyle,
    Boolean isDirectory,
    OCFURLRef baseURL,
    Boolean allowFileIDPath
);
OF_EXTERN OCFURLRef _CFURLCreateWithFileSystemRepresentation(
    OCFAllocatorRef allocator, const UInt8 *buffer, OCFIndex bufLen, Boolean isDirectory, OCFURLRef baseURL
);

// The unit's other non-external functions (parsing, escaping, path conversion and resolution), hidden with
// their native names; signatures from the native register use.
OF_EXTERN Boolean _appendPercentEscapesForCharacter(
    const UniChar *characters, Boolean isSurrogatePair, OCFStringEncoding encoding, OCFMutableStringRef string
);
OF_EXTERN Boolean _stringContainsCharacter(OCFStringRef string, UniChar character);
OF_EXTERN void _parseComponents(
    OCFStringRef string, OCFURLRef baseURL, UInt32 *flags, OCFRange *ranges, uint8_t *numberOfRanges
);
OF_EXTERN void constructBuffers(
    OCFStringRef string,
    UInt8 *buffer,
    const char **cString,
    const UniChar **characters,
    Boolean *useCString,
    Boolean *freeCharacters
);
OF_EXTERN Boolean scanCharacters(
    OCFAllocatorRef allocator,
    OCFMutableStringRef *escapedString,
    UInt32 *additionalFlags,
    const char *cString,
    const UniChar *characters,
    Boolean useCString,
    OCFIndex start,
    OCFIndex end,
    OCFIndex *mark,
    UInt32 componentFlag,
    OCFStringEncoding encoding
);
OF_EXTERN OCFStringRef resolveAbsoluteURLString(
    OCFAllocatorRef allocator,
    OCFStringRef relativeString,
    UInt32 relativeFlags,
    const OCFRange *relativeRanges,
    OCFStringRef baseString,
    UInt32 baseFlags,
    const OCFRange *baseRanges
);
OF_EXTERN OCFStringRef _retainedComponentString(
    OCFURLRef url, UInt32 component, Boolean fromOriginalString, Boolean removePercentEscapes
);
OF_EXTERN OCFStringRef
__copyEncodedComponent(OCFAllocatorRef allocator, const UInt32 *header, OCFRange range, UInt32 componentFlag);
OF_EXTERN OCFStringRef __copyOriginalComponent(
    OCFAllocatorRef allocator, const UInt32 *header, OCFRange range, UInt32 componentFlag
);
OF_EXTERN OCFStringRef
__createStringByReplacingPercentEscapes(OCFStringRef string, OCFStringRef charactersToLeaveEscaped);
OF_EXTERN OCFStringRef __createStringByReplacingPercentEscapesUsingEncoding(
    OCFStringRef string, OCFStringRef charactersToLeaveEscaped, OCFStringEncoding encoding
);
OF_EXTERN OCFStringRef _unescapedQueryString(OCFURLRef url);
// posixResult receives whether nothing was escaped;
// CreateStringFromFileSystemRepresentationByAddingPercentEscapes's escapingResult receives whether anything
// was.
OF_EXTERN OCFStringRef
POSIXPathToURLPath(OCFStringRef path, Boolean isDirectory, Boolean isAbsolute, Boolean *posixResult);
OF_EXTERN OCFStringRef
WindowsPathToURLPath(OCFStringRef path, OCFAllocatorRef allocator, Boolean isDirectory, Boolean isAbsolute);
OF_EXTERN OCFStringRef URLPathToPOSIXPath(
    OCFStringRef path, OCFAllocatorRef allocator, OCFStringEncoding encoding, Boolean isFileURL
);
OF_EXTERN OCFStringRef FileReferenceURLPathToFullURLPath(OCFStringRef path, OCFAllocatorRef allocator);
OF_EXTERN Boolean
CanonicalFileURLStringToFileSystemRepresentation(OCFStringRef string, UInt8 *buffer, OCFIndex bufferLength);
OF_EXTERN OCFIndex __rootLength(OCFStringRef path);
OF_EXTERN OCFStringRef _resolvedPath(
    UniChar *pathStr,
    UniChar *end,
    UniChar pathDelimiter,
    Boolean stripLeadingDotDots,
    Boolean stripTrailingDelimiter,
    OCFAllocatorRef allocator
);
OF_EXTERN OCFRange
__getLastPathComponentRangeFromString(OCFStringRef string, OCFIndex location, OCFIndex length);
OF_EXTERN OCFRange _rangeOfLastPathComponent(OCFURLRef url);
OF_EXTERN OCFStringRef CreateTruncatedURLString(OCFAllocatorRef allocator, OCFStringRef string);
OF_EXTERN OCFStringRef CreateStringFromFileSystemRepresentationByAddingPercentEscapes(
    OCFAllocatorRef allocator,
    const UInt8 *bytes,
    OCFIndex numBytes,
    Boolean isDirectory,
    Boolean isAbsolute,
    Boolean windowsPath,
    Boolean *escapingResult
);

// Native __foundation_swift_nsurl_feature_enabled, 0x1c023e6a0: external, but outside the CF/NS prefix rule;
// hidden until the owner records a naming decision. It answers the once-computed NSSwiftURL feature state.
// Every native caller inlines it (the once check on its onceToken, 0x22e6d3338, the dispatch_once in the
// caller's .cold.1 fragment through OUTLINED_FUNCTION_0_31, and the byte compare with 1 on its
// featureEnabled, 0x22e6d3340); the restored callers call it.
OF_EXTERN bool _foundation_swift_nsurl_feature_enabled(void);
