//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/URL/OCFURL.h"
#include "CoreFoundation/Base/OCFFileUtilities.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"
#include "CoreFoundation/URL/OCFURLKeys.h"

#include <assert.h>
#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/log.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Inferred organization: the CFURL functions from __foundation_swift_nsurl_feature_enabled (0x1c023e6a0)
// through the outlined fragments (0x1c024f810) are contiguous in the iOS Simulator __text section, and their
// assertion strings name CFURL.c; they are kept in one translation unit. The .cold fragments
// (0x1c030d45c-0x1c030d8e4) and OUTLINED_FUNCTION fragments belong to their parent functions. Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified; iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationURL.md
// (pending; the coordinator writes the reading).

// libobjc (supplied, never reimplemented).
extern id objc_retain(id value);
extern void objc_release(id value);

// libsystem's feature-flag SPI (os/feature_private.h).
extern bool _os_feature_enabled_impl(const char *domain, const char *feature);

// CoreServicesInternal (an unclassified cut, Configuration/Scope.json): resolves a file reference path from
// its volume and file numbers; FileReferenceURLPathToFullURLPath passes NULL as the fourth argument, whose
// meaning is unread.
extern OCFStringRef
_FSCreatePathForFileID(OCFAllocatorRef allocator, long long volumeID, long long fileID, void *unread);

// CoreServicesInternal (an unsupported cut, owner decision): writes the file system representation of the
// file with the given volume and file numbers into the buffer; CFURLGetFileSystemRepresentation passes NULL
// as the sixth argument, whose meaning is unread. The result is tested as W0 != 0 (CBZ W25). Declared with
// its native name and left undefined; signature from the register use (confidence: medium).
extern Boolean _FSGetFileSystemRepresentationForFileID(
    OCFAllocatorRef allocator,
    long long volumeID,
    long long fileID,
    UInt8 *buffer,
    OCFIndex length,
    void *unread
);

// CoreServicesInternal's resource-property and security-scope entry points, imported directly by the native
// image (no soft link, no NULL check): unsupported cuts. They are declared here with their native names and
// left undefined; Configuration/LinkDependencies.json links them unbound (weak, to null), so a call faults at
// the null address. Signatures from the register use of the
// CFURL call sites (confidence: medium): resourceInfo is the URL's __CFURLResourceInfoPtr; the Boolean
// results are tested as W0 != 0 (CBNZ W0); setFlag is 1 from CFURLSetResourcePropertyForKey and 0 from
// CFURLSetTemporaryResourcePropertyForKey (its meaning inside CoreServicesInternal is unread).
extern Boolean _FSURLCopyResourcePropertyForKey(
    OCFURLRef url, OCFStringRef key, void *propertyValueTypeRefPtr, void *resourceInfo, OCFErrorRef *error
);
extern OCFDictionaryRef
_FSURLCopyResourcePropertiesForKeys(OCFURLRef url, OCFArrayRef keys, void *resourceInfo, OCFErrorRef *error);
extern Boolean _FSURLSetResourcePropertyForKey(
    OCFURLRef url,
    OCFStringRef key,
    OCFTypeRef propertyValue,
    Boolean setFlag,
    void *resourceInfo,
    OCFErrorRef *error
);
extern Boolean _FSURLSetResourcePropertiesForKeys(
    OCFURLRef url, OCFDictionaryRef keyedPropertyValues, void *resourceInfo, OCFErrorRef *error
);
extern void _FSURLBeginResourcePropertyCacheAccess(OCFURLRef url, void *resourceInfo);
extern void _FSURLEndResourcePropertyCacheAccess(OCFURLRef url, void *resourceInfo);
extern void _FSURLClearResourcePropertyCacheForKey(OCFURLRef url, OCFStringRef key, void *resourceInfo);
extern int _CFURLStartAccessingSecurityScopedResource(OCFURLRef url);
extern void _CFURLStopAccessingSecurityScopedResource(OCFURLRef url);

// Non-external functions of this unit defined below in native order.
static OCFStringRef _CFURLCopyFileReferenceURLPath(OCFURLRef url, OCFErrorRef *error);
static OCFStringRef createFileReferencePath(
    OCFAllocatorRef allocator,
    long long volumeID,
    long long parentID,
    long long fileID,
    OCFStringRef name,
    Boolean isDirectory
);

// The NSURL class object the Swift-parsing paths message (the class reference at 0x2227ce3e0).
extern char urlClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSURL));

// The CFStringInlineBuffer interface of the SDK's CFString.h, which the native code inlines: 64 buffered
// characters, the string, its direct character and C-string pointers (ASCII), the buffered range and the
// buffered window. The getters below follow the native instruction sequences (the window starts four
// characters before the requested index, clamped to zero, and spans 64 characters, clamped to the range).
// StringInlineBuffer and its helpers are local descriptive spellings.
typedef struct {
    UniChar buffer[64];
    OCFStringRef theString;
    const UniChar *directUniCharBuffer;
    const char *directCStringBuffer;
    OCFRange rangeToBuffer;
    OCFIndex bufferedRangeStart;
    OCFIndex bufferedRangeEnd;
} StringInlineBuffer;

OF_ABI_ASSERT(sizeof(StringInlineBuffer) == 0xb8, "The SDK inline buffer is 184 bytes.");

static inline void initInlineBuffer(OCFStringRef string, StringInlineBuffer *buf, OCFRange range) {
    buf->theString = string;
    buf->rangeToBuffer = range;
    buf->directUniCharBuffer = OCFStringGetCharactersPtr(string);
    buf->directCStringBuffer =
        buf->directUniCharBuffer != NULL ? NULL : OCFStringGetCStringPtr(string, kOCFStringEncodingASCII);
    buf->bufferedRangeStart = 0;
    buf->bufferedRangeEnd = 0;
}

static inline UniChar inlineBufferCharacter(StringInlineBuffer *buf, OCFIndex idx) {
    if (buf->directUniCharBuffer != NULL) {
        return buf->directUniCharBuffer[idx + buf->rangeToBuffer.location];
    }
    if (buf->directCStringBuffer != NULL) {
        return (UniChar)buf->directCStringBuffer[idx + buf->rangeToBuffer.location];
    }
    if (idx >= buf->bufferedRangeEnd || idx < buf->bufferedRangeStart) {
        if ((buf->bufferedRangeStart = idx - 4) < 0) {
            buf->bufferedRangeStart = 0;
        }
        buf->bufferedRangeEnd = buf->bufferedRangeStart + 64;
        if (buf->bufferedRangeEnd > buf->rangeToBuffer.length) {
            buf->bufferedRangeEnd = buf->rangeToBuffer.length;
        }
        OCFStringGetCharacters(
            buf->theString,
            (OCFRange){
                buf->rangeToBuffer.location + buf->bufferedRangeStart,
                buf->bufferedRangeEnd - buf->bufferedRangeStart
            },
            buf->buffer
        );
    }
    return buf->buffer[idx - buf->bufferedRangeStart];
}

static inline const UniChar *inlineBufferCharactersPtr(StringInlineBuffer *buf, OCFRange desiredRange) {
    if (desiredRange.location < 0 ||
        desiredRange.location + desiredRange.length > buf->rangeToBuffer.length) {
        return NULL;
    }
    if (buf->directUniCharBuffer != NULL) {
        return &buf->directUniCharBuffer[buf->rangeToBuffer.location + desiredRange.location];
    }
    if (desiredRange.length > 64) {
        return NULL;
    }
    if (desiredRange.location + desiredRange.length > buf->bufferedRangeEnd ||
        desiredRange.location < buf->bufferedRangeStart) {
        buf->bufferedRangeStart = desiredRange.location;
        buf->bufferedRangeEnd = buf->bufferedRangeStart + 64;
        if (buf->bufferedRangeEnd > buf->rangeToBuffer.length) {
            buf->bufferedRangeEnd = buf->rangeToBuffer.length;
        }
        OCFIndex location = buf->rangeToBuffer.location + buf->bufferedRangeStart;
        OCFIndex length = buf->bufferedRangeEnd - buf->bufferedRangeStart;
        if (buf->directCStringBuffer != NULL) {
            UniChar *bufPtr = buf->buffer;
            while (length--) {
                *bufPtr++ = (UniChar)buf->directCStringBuffer[location++];
            }
        } else {
            OCFStringGetCharacters(buf->theString, (OCFRange){location, length}, buf->buffer);
        }
    }
    return &buf->buffer[desiredRange.location - buf->bufferedRangeStart];
}

// A mutable-string append buffer the native code inlines (no out-of-line copy in the image): 1,024 buffered
// characters at offset 0, the count at 0x800 and the string at 0x808. Initialization zeroes the first 0x200
// bytes of the characters and the count, then creates the string with capacity 0; a string longer than
// 1,024 characters flushes the buffer and is appended directly; a character is buffered after a flush of a
// full buffer. AppendBuffer and its helpers are local descriptive spellings.
typedef struct {
    UniChar characters[1024];
    OCFIndex count;
    OCFMutableStringRef string;
} AppendBuffer;

OF_ABI_ASSERT(sizeof(AppendBuffer) == 0x810, "The native append buffer is 0x810 bytes.");

static inline void appendBufferInit(AppendBuffer *buffer, OCFAllocatorRef allocator) {
    memset(buffer->characters, 0, 0x200);
    buffer->count = 0;
    buffer->string = OCFStringCreateMutable(allocator, 0);
}

static inline void appendBufferAppendString(AppendBuffer *buffer, OCFStringRef string) {
    OCFIndex length = OCFStringGetLength(string);
    if (length > 0x400) {
        if (buffer->count != 0) {
            OCFStringAppendCharacters(buffer->string, buffer->characters, buffer->count);
            buffer->count = 0;
        }
        OCFStringAppend(buffer->string, string);
    } else {
        if (buffer->count + length > 0x400) {
            OCFStringAppendCharacters(buffer->string, buffer->characters, buffer->count);
            buffer->count = 0;
        }
        OCFStringGetCharacters(string, (OCFRange){0, length}, &buffer->characters[buffer->count]);
        buffer->count += length;
    }
}

static inline void appendBufferAppendCharacter(AppendBuffer *buffer, UniChar character) {
    if (buffer->count >= 0x400) {
        OCFStringAppendCharacters(buffer->string, buffer->characters, buffer->count);
        buffer->count = 0;
    }
    buffer->characters[buffer->count] = character;
    buffer->count += 1;
}

static inline void
appendBufferAppendCharacters(AppendBuffer *buffer, const UniChar *characters, OCFIndex length) {
    if (length > 0x400) {
        if (buffer->count != 0) {
            OCFStringAppendCharacters(buffer->string, buffer->characters, buffer->count);
            buffer->count = 0;
        }
        OCFStringAppendCharacters(buffer->string, characters, length);
    } else {
        if (buffer->count + length > 0x400) {
            OCFStringAppendCharacters(buffer->string, buffer->characters, buffer->count);
            buffer->count = 0;
        }
        memcpy(&buffer->characters[buffer->count], characters, (size_t)length * sizeof(UniChar));
        buffer->count += length;
    }
}

static inline OCFMutableStringRef appendBufferCopyString(AppendBuffer *buffer) {
    if (buffer->count != 0) {
        OCFStringAppendCharacters(buffer->string, buffer->characters, buffer->count);
        buffer->count = 0;
    }
    OCFMutableStringRef string = buffer->string;
    buffer->string = NULL;
    return string;
}

// One instance per constant string the image deduplicates: callers compare CFURLCopyScheme's result with the
// "file" constant by address (cfstr_File, 0x2227ae180).
static inline OCFStringRef fileString(void) {
    return OF_STR("file");
}

static inline OCFStringRef httpString(void) {
    return OF_STR("http");
}

static inline OCFStringRef httpsString(void) {
    return OF_STR("https");
}

static inline OCFStringRef ftpString(void) {
    return OF_STR("ftp");
}

static inline OCFStringRef fileURLPrefixString(void) {
    return OF_STR("file://");
}

static inline OCFStringRef dataString(void) {
    return OF_STR("data");
}

static inline OCFStringRef percentString(void) {
    return OF_STR("%");
}

static inline OCFStringRef emptyString(void) {
    return OF_STR("");
}

static inline OCFStringRef backslashString(void) {
    return OF_STR("\\");
}

static inline OCFStringRef slashString(void) {
    return OF_STR("/");
}

static inline bool isObjCURL(OCFURLRef url) {
    return (intptr_t)url < 0 || __CF_IS_OBJC(29, url);
}

// Fields of the Swift layout read at fixed offsets (16-, 32- and 64-bit), as the native code does; copied out
// of the object's bytes.
static inline uint16_t swiftField16(OCFURLRef url, size_t offset) {
    uint16_t value;
    memcpy(&value, (const uint8_t *)url + offset, sizeof(value));
    return value;
}

static inline uint32_t swiftField32(OCFURLRef url, size_t offset) {
    uint32_t value;
    memcpy(&value, (const uint8_t *)url + offset, sizeof(value));
    return value;
}

static inline uint64_t swiftField64(OCFURLRef url, size_t offset) {
    uint64_t value;
    memcpy(&value, (const uint8_t *)url + offset, sizeof(value));
    return value;
}

// The index of a component's range: the number of set flags below its bit, counted one bit at a time from bit
// 0 (0 for bit 0), as the native loops do.
static inline uint32_t componentRangeIndex(UInt32 flags, UInt32 bit) {
    if ((bit & 1) != 0) {
        return 0;
    }
    uint32_t index = 0;
    UInt32 previous;
    do {
        previous = bit;
        bit >>= 1;
        index += flags & 1;
        flags >>= 1;
    } while ((previous & 2) == 0);
    return index;
}

// Native UnescapeAllWithUTF8.hexvalues, 0x1c03a42c0 (__TEXT,__const, 256 bytes). Non-external; a static of
// UnescapeAllWithUTF8, which CFURLCreateStringByReplacingPercentEscapes inlines: the value of each
// hexadecimal digit, 0x80 for every other byte.
static const uint8_t UnescapeAllWithUTF8HexValues[256] = {
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
};

// Native sURLValidCharacters, 0x1c03a43c0 (__TEXT,__const, 128 bytes). Non-external. Character classes of
// the 7-bit characters: bit 0 legal URL character, bit 1 scheme start (alphabetic), bit 3 scheme character,
// bit 4 hexadecimal digit; bits 2 and 5 are set for characters the restored functions do not test.
static const uint8_t sURLValidCharacters[128] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x00, 0x00, 0x05, 0x00, 0x05, 0x05, 0x05, 0x05, 0x05, 0x0d, 0x05, 0x0d, 0x0d, 0x05,
    0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x01, 0x01, 0x00, 0x05, 0x00, 0x01,
    0x01, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x05, 0x00,
};

// Native CanonicalFileURLStringToFileSystemRepresentation.hexvalues, 0x1c03a4440 (__TEXT,__const, 256 bytes).
// Non-external; a static of that function: the value of each hexadecimal digit, 0 for every other byte.
static const uint8_t CanonicalFileURLStringToFileSystemRepresentationHexValues[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// Native fileURLPrefix, 0x1c03a4540 (__TEXT,__const, 8 bytes). Non-external: "file://" and its terminator.
static const char fileURLPrefix[8] = "file://";

// Native sURLValidBytes, 0x1c03a4548 (__TEXT,__const, 256 bytes). Non-external. Bit 0 marks the bytes
// CreateStringFromFileSystemRepresentationByAddingPercentEscapes copies unescaped.
static const uint8_t sURLValidBytes[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// Native CreateStringFromFileSystemRepresentationByAddingPercentEscapes.hexchars, 0x1c048f09c
// (__TEXT,__cstring). Non-external; a static of that function, shared with the escaping loops here.
static const char CreateStringFromFileSystemRepresentationByAddingPercentEscapesHexChars[17] =
    "0123456789ABCDEF";

static OCFStringRef resolveAbsoluteURLStringBuffer(
    OCFAllocatorRef allocator,
    OCFStringRef relativeString,
    UInt32 relativeFlags,
    const OCFRange *relativeRanges,
    OCFStringRef baseString,
    UInt32 baseFlags,
    const OCFRange *baseRanges,
    UniChar *buf
);
static OCFStringRef __CFURLCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions);
static OCFStringRef __CFURLCopyDescription(OCFTypeRef cf);
static void __CFURLDeallocate(OCFTypeRef cf);
static Boolean __CFURLEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFURLHash(OCFTypeRef cf);
static OCFURLRef __CFURLCreateCopyAppendingPathComponent(
    OCFAllocatorRef allocator,
    OCFURLRef url,
    OCFStringRef pathComponent,
    Boolean isDirectory,
    Boolean checkAppend
);

// Native __CFURLClass, 0x2227a5f80 (__DATA_CONST,__const). Hidden (was private external); version 0, name
// "CFURL", finalize, equal, hash and both description callbacks.
const OCFRuntimeClass __CFURLClass = {
    0,
    "CFURL",
    NULL,
    NULL,
    __CFURLDeallocate,
    __CFURLEqual,
    __CFURLHash,
    __CFURLCopyFormattingDescription,
    __CFURLCopyDescription,
    NULL,
    NULL,
    0,
};

// The value of a hexadecimal digit, or -1, as the replacing loops decode both digits of an escape: the first
// digit is tested against '0'..'9', 'a'..'f' and then 'A'..'F' (unsigned compares), the second against
// '0'..'9', 'a'..'f' and 'A'..'F' the same way; the byte keeps the low eight bits of first * 16 + second.
static inline int hexDigitValue(UniChar character) {
    if ((uint32_t)(character - '0') < 10) {
        return character - '0';
    }
    if ((uint32_t)(character - 'a') < 6) {
        return character - 'a' + 10;
    }
    if ((uint32_t)(character - 'A') <= 5) {
        return character - 'A' + 10;
    }
    return -1;
}

// Native __foundation_swift_nsurl_feature_enabled.onceToken and .featureEnabled, 0x22e6d3338 and 0x22e6d3340
// (__DATA,__bss). Non-external.
static dispatch_once_t foundationSwiftNSURLOnceToken;
static uint8_t foundationSwiftNSURLFeatureEnabled;

// Native __foundation_swift_nsurl_feature_enabled, 0x1c023e6a0. External, but outside the CF/NS prefix rule;
// hidden until the owner records a naming decision. Defined here because its block is this unit's: while
// the token is not -1 the .cold.1 fragment (0x1c030d458) runs dispatch_once through OUTLINED_FUNCTION_0_31
// (0x1c024f6f8: the token and ___block_literal_global_24); the flag byte is answered. Callers that inline
// the check compare the byte with 1; the stored value is 0 or 1. The function and .cold.1: IDA (corpus
// delta capture), reconciled instruction by instruction (the fast path tests the token against -1 before
// the byte load, as the inline dispatch_once does); confidence: verified.
// Native ____foundation_swift_nsurl_feature_enabled_block_invoke, 0x1c023e6d0. Non-external. The flag is
// set when the Foundation/NSSwiftURL feature is enabled or the NSSwiftURL environment variable is present,
// and then the Foundation/NSURLUseSwift feature is enabled or the NSURLUseSwift environment variable is
// present; otherwise it is cleared.
bool _foundation_swift_nsurl_feature_enabled(void) {
    dispatch_once(&foundationSwiftNSURLOnceToken, ^{
      uint8_t enabled;
      if (_os_feature_enabled_impl("Foundation", "NSSwiftURL") || getenv("NSSwiftURL") != NULL) {
          if (_os_feature_enabled_impl("Foundation", "NSURLUseSwift")) {
              enabled = 1;
          } else {
              enabled = getenv("NSURLUseSwift") != NULL;
          }
      } else {
          enabled = 0;
      }
      foundationSwiftNSURLFeatureEnabled = enabled;
    });
    return foundationSwiftNSURLFeatureEnabled;
}

// Native CFURLCreateStringByReplacingPercentEscapes, 0x1c023e74c. External.
// Without an original string the result is NULL; without characters to leave escaped, or for an empty
// string, a copy. An empty set of characters to leave escaped selects the inlined UnescapeAllWithUTF8: the
// UTF-8 bytes of the whole string (CFStringGetMaximumSizeForEncoding of the length, at least 1, unsigned)
// are fetched into a 4,192-byte stack buffer when the maximum is below 2,097, otherwise into a
// malloc_type_malloc block of twice the maximum (type 0x100004077774924; NULL when it fails); the unescaped
// bytes are written after the fetched ones, a '%' needs two more bytes before the used length and two
// hexadecimal digits (UnescapeAllWithUTF8HexValues), and any failure answers NULL. Otherwise each "%XX"
// escape found by CFStringFindWithOptions is decoded; a byte with bit 7 set starts a UTF-8 sequence of 2, 3
// or 4 bytes (bits 5 and 4) whose continuation escapes must follow; the decoded string (a cached
// one-character external-characters string for an ASCII byte, or the UTF-8 string, with a lone BOM from
// EF BB BF mapped to U+FEFF) is kept escaped when charactersToLeaveEscaped contains it and otherwise replaces
// the escape, the characters since the last replacement being appended by a stack block
// (__CFURLCreateStringByReplacingPercentEscapes_block_invoke, 0x1c023ee20). A malformed escape answers NULL.
OCFStringRef OCFURLCreateStringByReplacingPercentEscapes(
    OCFAllocatorRef allocator, OCFStringRef originalString, OCFStringRef charactersToLeaveEscaped
) {
    bool unescapeAll;
    if (charactersToLeaveEscaped != NULL) {
        unescapeAll = OCFStringGetLength(charactersToLeaveEscaped) == 0;
    } else {
        unescapeAll = false;
    }
    if (originalString == NULL) {
        return NULL;
    }
    OCFIndex length = OCFStringGetLength(originalString);
    if (charactersToLeaveEscaped == NULL || length == 0) {
        return OCFStringCreateCopy(allocator, originalString);
    }
    OCFRange percentRange = {0, 0};
    UniChar character = 0;
    UInt8 bytes[4192];
    if (unescapeAll) {
        OCFIndex stringLength = OCFStringGetLength(originalString);
        OCFIndex maximum = OCFStringGetMaximumSizeForEncoding(
            (uint64_t)stringLength > 1 ? stringLength : 1, kOCFStringEncodingUTF8
        );
        UInt8 *buffer;
        if (maximum >= 0x831) {
            buffer = malloc_type_malloc((size_t)(maximum * 2), (malloc_type_id_t)UINT64_C(0x100004077774924));
            if (buffer == NULL) {
                return NULL;
            }
        } else {
            buffer = bytes;
        }
        OCFStringRef result = NULL;
        OCFIndex usedBufLen = 0;
        if (OCFStringGetBytes(
                originalString,
                (OCFRange){0, stringLength},
                kOCFStringEncodingUTF8,
                0,
                false,
                buffer,
                maximum,
                &usedBufLen
            ) != 0) {
            UInt8 *output = buffer + maximum;
            UInt8 *cursor = output;
            const UInt8 *input = buffer;
            bool failed = false;
            for (OCFIndex index = 0; index < usedBufLen; ++index) {
                UInt8 byte = *input;
                if (byte == '%') {
                    index += 2;
                    if (index >= usedBufLen) {
                        failed = true;
                        break;
                    }
                    uint8_t high = UnescapeAllWithUTF8HexValues[input[1]];
                    uint8_t low = UnescapeAllWithUTF8HexValues[input[2]];
                    if (((low | high) & 0x80) != 0) {
                        failed = true;
                        break;
                    }
                    input += 3;
                    byte = (UInt8)(low + (high << 4));
                } else {
                    input += 1;
                }
                *cursor++ = byte;
            }
            if (!failed) {
                result = OCFStringCreateWithBytes(
                    allocator, output, cursor - output, kOCFStringEncodingUTF8, false
                );
            }
        }
        if (buffer != bytes) {
            free(buffer);
        }
        return result;
    }
    if (!OCFStringFindWithOptions(originalString, percentString(), (OCFRange){0, length}, 0, &percentRange)) {
        return OCFStringCreateCopy(allocator, originalString);
    }
    OCFIndex mark = 0;
    OCFMutableStringRef escapedCharacter = NULL;
    OCFMutableStringRef newString = NULL;
    bool succeeded = false;
    for (;;) {
        if (length - percentRange.location < 3) {
            break;
        }
        UniChar first = OCFStringGetCharacterAtIndex(originalString, percentRange.location + 1);
        UniChar second = OCFStringGetCharacterAtIndex(originalString, percentRange.location + 2);
        int high = hexDigitValue(first);
        if (high < 0) {
            break;
        }
        int low = hexDigitValue(second);
        if (low < 0) {
            break;
        }
        bytes[0] = (UInt8)(high * 16 + low);
        OCFStringRef decoded;
        uint32_t byteCount;
        if ((bytes[0] & 0x80) == 0) {
            character = bytes[0];
            if (escapedCharacter == NULL) {
                escapedCharacter = OCFStringCreateMutableWithExternalCharactersNoCopy(
                    allocator, &character, 1, 1, kOCFAllocatorNull
                );
            }
            decoded = OCFRetain(escapedCharacter);
            byteCount = 1;
        } else {
            byteCount = (bytes[0] & 0x20) == 0 ? 2 : ((bytes[0] & 0x10) != 0 ? 4 : 3);
            if (length < percentRange.location + (OCFIndex)(uint32_t)(byteCount * 3)) {
                break;
            }
            uint32_t leadBits = bytes[0] & 0x30;
            OCFIndex count = byteCount & 0xff;
            OCFIndex offset = 5;
            OCFIndex index = 1;
            bool incomplete = false;
            for (;;) {
                if (OCFStringGetCharacterAtIndex(originalString, percentRange.location + offset - 2) != '%') {
                    incomplete = true;
                    break;
                }
                UniChar escapeFirst =
                    OCFStringGetCharacterAtIndex(originalString, percentRange.location + offset - 1);
                UniChar escapeSecond =
                    OCFStringGetCharacterAtIndex(originalString, percentRange.location + offset);
                bytes[index] = 0;
                int escapeHigh = hexDigitValue(escapeFirst);
                if (escapeHigh < 0) {
                    incomplete = true;
                    break;
                }
                bytes[index] = (UInt8)(escapeHigh << 4);
                int escapeLow = hexDigitValue(escapeSecond);
                if (escapeLow < 0) {
                    incomplete = true;
                    break;
                }
                bytes[index] = (UInt8)(bytes[index] + escapeLow);
                index += 1;
                offset += 3;
                if (count == index) {
                    break;
                }
            }
            decoded = OCFStringCreateWithBytes(allocator, bytes, count, kOCFStringEncodingUTF8, false);
            if (decoded == NULL) {
                break;
            }
            OCFIndex decodedLength = OCFStringGetLength(decoded);
            if (leadBits == 0x20 && decodedLength == 0 && bytes[0] == 0xef && bytes[1] == 0xbb &&
                bytes[2] == 0xbf) {
                character = 0xfeff;
                OCFMutableStringRef bom = escapedCharacter;
                if (bom == NULL) {
                    bom = OCFStringCreateMutableWithExternalCharactersNoCopy(
                        allocator, &character, 1, 1, kOCFAllocatorNull
                    );
                }
                OCFRelease(decoded);
                escapedCharacter = bom;
                decoded = OCFRetain(bom);
            }
            if (incomplete) {
                if (decoded != NULL) {
                    OCFRelease(decoded);
                }
                break;
            }
        }
        OCFIndex next = percentRange.location + (OCFIndex)(uint32_t)(byteCount * 3);
        if (OCFStringFind(charactersToLeaveEscaped, decoded, 0).location == kOCFNotFound) {
            if (newString == NULL) {
                newString = OCFStringCreateMutable(allocator, length);
            }
            if (percentRange.location - mark >= 1) {
                OCFRange range = {mark, percentRange.location - mark};
                OCFMutableStringRef appendTarget = newString;
                // Native __CFURLCreateStringByReplacingPercentEscapes_block_invoke, 0x1c023ee20.
                // Non-external; a stack block (flags 0xc0000000, descriptor ___block_descriptor_64_e5_v8_?0l)
                // built and invoked in place. Twice the range length as an unsigned size selects the buffer:
                // from 0x401 bytes a malloc_type_malloc block (type 0x1000040BDFB0063), otherwise a zeroed
                // stack area for a positive length and NULL for an empty one.
                ^{
                  OCFIndex count = range.length;
                  size_t size = (size_t)count * 2;
                  UniChar *characters;
                  if (size >= 0x401) {
                      characters = malloc_type_malloc(size, (malloc_type_id_t)UINT64_C(0x1000040BDFB0063));
                  } else if (count >= 1) {
                      characters = __builtin_alloca(size);
                      bzero(characters, size);
                  } else {
                      characters = NULL;
                  }
                  OCFStringGetCharacters(originalString, range, characters);
                  OCFStringAppendCharacters(appendTarget, characters, range.length);
                  if (size >= 0x401) {
                      free(characters);
                  }
                }();
            }
            OCFStringAppend(newString, decoded);
            mark = next;
        }
        if (decoded != NULL) {
            OCFRelease(decoded);
        }
        if (!OCFStringFindWithOptions(
                originalString, percentString(), (OCFRange){next, length - next}, 0, &percentRange
            )) {
            succeeded = true;
            break;
        }
    }
    if (escapedCharacter != NULL) {
        OCFRelease(escapedCharacter);
    }
    if (!succeeded) {
        if (newString != NULL) {
            OCFRelease(newString);
        }
        return NULL;
    }
    if (newString == NULL) {
        return OCFStringCreateCopy(allocator, originalString);
    }
    if (length - mark > 0) {
        OCFStringRef substring =
            OCFStringCreateWithSubstring(allocator, originalString, (OCFRange){mark, length - mark});
        OCFStringAppend(newString, substring);
        OCFRelease(substring);
    }
    return newString;
}

// Native CFURLCreateStringByReplacingPercentEscapesUsingEncoding, 0x1c023ef10. External.
// UTF-8 tail-calls CFURLCreateStringByReplacingPercentEscapes. Otherwise each run of consecutive "%XX"
// escapes is decoded into a byte buffer (eight stack bytes, then a malloc_type_malloc block of 16 bytes,
// doubled as a 32-bit capacity, type 0x100004077774924) and converted with the encoding; with a nonempty
// charsToLeaveEscaped each converted character found in it is re-escaped through
// _appendPercentEscapesForCharacter (not a surrogate pair), others are appended as they are. A malformed
// escape or a failed conversion answers NULL.
OCFStringRef OCFURLCreateStringByReplacingPercentEscapesUsingEncoding(
    OCFAllocatorRef allocator,
    OCFStringRef origString,
    OCFStringRef charsToLeaveEscaped,
    OCFStringEncoding encoding
) {
    if (encoding == kOCFStringEncodingUTF8) {
        return OCFURLCreateStringByReplacingPercentEscapes(allocator, origString, charsToLeaveEscaped);
    }
    OCFRange percentRange = {0, 0};
    if (charsToLeaveEscaped == NULL) {
        if (origString == NULL) {
            return NULL;
        }
        return OCFStringCreateCopy(allocator, origString);
    }
    OCFIndex leaveEscapedLength = OCFStringGetLength(charsToLeaveEscaped);
    if (origString == NULL) {
        return NULL;
    }
    OCFIndex length = OCFStringGetLength(origString);
    if (!OCFStringFindWithOptions(origString, percentString(), (OCFRange){0, length}, 0, &percentRange)) {
        return OCFStringCreateCopy(allocator, origString);
    }
    OCFMutableStringRef newString = NULL;
    OCFMutableStringRef escapedCharacter = NULL;
    OCFIndex position = 0;
    int32_t capacity = 8;
    UInt8 stackBytes[8];
    UInt8 *bytes = stackBytes;
    OCFIndex mark;
    UniChar character;
    bool succeeded = false;
    for (;;) {
        mark = position;
        character = 0;
        OCFIndex location = percentRange.location;
        if (length - location < 3) {
            break;
        }
        OCFIndex numBytes = 0;
        position = location + 3;
        OCFIndex consumed = -location;
        OCFIndex remaining;
        bool failed = false;
        for (;;) {
            if (numBytes == (OCFIndex)(uint32_t)capacity) {
                if (bytes != stackBytes) {
                    int32_t newCapacity = capacity * 2;
                    UInt8 *newBytes = malloc_type_malloc(
                        (size_t)newCapacity, (malloc_type_id_t)UINT64_C(0x100004077774924)
                    );
                    memmove(newBytes, bytes, (size_t)capacity);
                    free(bytes);
                    capacity = newCapacity;
                    bytes = newBytes;
                } else {
                    UInt8 *newBytes = malloc_type_malloc(16, (malloc_type_id_t)UINT64_C(0x100004077774924));
                    memmove(newBytes, stackBytes, (size_t)capacity);
                    capacity = 16;
                    bytes = newBytes;
                }
            }
            character = OCFStringGetCharacterAtIndex(origString, position - 2);
            UniChar second = OCFStringGetCharacterAtIndex(origString, position - 1);
            bytes[numBytes] = 0;
            int high = hexDigitValue(character);
            if (high < 0) {
                failed = true;
                break;
            }
            bytes[numBytes] = (UInt8)(high << 4);
            int low = hexDigitValue(second);
            if (low < 0) {
                failed = true;
                break;
            }
            bytes[numBytes] = (UInt8)(bytes[numBytes] + low);
            remaining = length - position;
            if (remaining > 0 && OCFStringGetCharacterAtIndex(origString, position) == '%') {
                position += 3;
                consumed -= 3;
                numBytes += 1;
                if (length + consumed < 3) {
                    failed = true;
                    break;
                }
                continue;
            }
            break;
        }
        if (failed) {
            break;
        }
        OCFStringRef converted =
            OCFStringCreateWithBytes(allocator, bytes, (OCFIndex)(uint32_t)(numBytes + 1), encoding, false);
        if (converted == NULL) {
            break;
        }
        if (newString == NULL) {
            newString = OCFStringCreateMutable(allocator, length);
        }
        OCFIndex skipped = percentRange.location - mark;
        if (skipped >= 1) {
            OCFStringRef substring =
                OCFStringCreateWithSubstring(allocator, origString, (OCFRange){mark, skipped});
            OCFStringAppend(newString, substring);
            OCFRelease(substring);
        }
        if (leaveEscapedLength != 0) {
            OCFIndex convertedLength = OCFStringGetLength(converted);
            if (escapedCharacter == NULL) {
                escapedCharacter = OCFStringCreateMutableWithExternalCharactersNoCopy(
                    allocator, &character, 1, 1, kOCFAllocatorNull
                );
            }
            for (OCFIndex index = 0; index < convertedLength; ++index) {
                character = OCFStringGetCharacterAtIndex(converted, index);
                if (OCFStringFind(charsToLeaveEscaped, escapedCharacter, 0).location != kOCFNotFound) {
                    _appendPercentEscapesForCharacter(&character, false, encoding, newString);
                } else {
                    OCFStringAppendCharacters(newString, &character, 1);
                }
            }
        } else {
            OCFStringAppend(newString, converted);
        }
        OCFRelease(converted);
        if (!OCFStringFindWithOptions(
                origString, percentString(), (OCFRange){position, remaining}, 0, &percentRange
            )) {
            succeeded = true;
            mark = position;
            break;
        }
    }
    if (escapedCharacter != NULL) {
        OCFRelease(escapedCharacter);
    }
    if (bytes != stackBytes) {
        free(bytes);
    }
    if (!succeeded) {
        if (newString != NULL) {
            OCFRelease(newString);
        }
        return NULL;
    }
    if (newString == NULL) {
        return OCFStringCreateCopy(allocator, origString);
    }
    if (length - mark > 0) {
        OCFStringRef substring =
            OCFStringCreateWithSubstring(allocator, origString, (OCFRange){mark, length - mark});
        OCFStringAppend(newString, substring);
        OCFRelease(substring);
    }
    return newString;
}

// Native _appendPercentEscapesForCharacter, 0x1c023f41c. Non-external. The character (two characters for a
// surrogate pair) is converted by CFStringEncodingUnicodeToBytes(encoding, 0, ...) into an 8-byte buffer
// without a used-character count; a nonzero conversion result answers false. Each converted byte becomes
// '%' and its two uppercase digits (CreateStringFromFileSystemRepresentationByAddingPercentEscapesHexChars)
// in a 24-character buffer, which is appended even when no byte was converted.
Boolean _appendPercentEscapesForCharacter(
    const UniChar *characters, Boolean isSurrogatePair, OCFStringEncoding encoding, OCFMutableStringRef string
) {
    UInt8 bytes[8];
    UniChar escaped[24];
    OCFIndex usedByteLength = 0;
    if (OCFStringEncodingUnicodeToBytes(
            encoding, 0, characters, isSurrogatePair ? 2 : 1, NULL, bytes, 8, &usedByteLength
        ) != 0) {
        return false;
    }
    const char *hexChars = CreateStringFromFileSystemRepresentationByAddingPercentEscapesHexChars;
    OCFIndex count = 0;
    for (OCFIndex index = 0; index < usedByteLength; ++index) {
        escaped[count] = '%';
        escaped[count + 1] = (UInt8)hexChars[bytes[index] >> 4];
        escaped[count + 2] = (UInt8)hexChars[bytes[index] & 0xf];
        count += 3;
    }
    OCFStringAppendCharacters(string, escaped, count);
    return true;
}

// Native CFURLCreateStringByAddingPercentEscapes, 0x1c023f524. External.
// Characters are read through an inlined CFStringInlineBuffer. A legal 7-bit URL character (bit 0 of
// sURLValidCharacters) is escaped only when legalURLCharactersToBeEscaped contains it; any other character
// unless charactersToLeaveUnescaped contains it. The first escape creates a mutable copy (with the original
// string's allocator) truncated at that character; escapes and kept characters collect in a 1,024-character
// stack buffer, flushed before an escape once it holds 1,000 characters and after a kept character fills
// it. A character the encoding cannot convert ends the scan unless the encoding is UTF-8 and it starts a
// surrogate pair whose trail follows; an unfinished scan releases the copy and answers NULL. Without any
// escape the result is a copy made with the original string's allocator; an empty string is copied with the
// given allocator.
OCFStringRef OCFURLCreateStringByAddingPercentEscapes(
    OCFAllocatorRef allocator,
    OCFStringRef originalString,
    OCFStringRef charactersToLeaveUnescaped,
    OCFStringRef legalURLCharactersToBeEscaped,
    OCFStringEncoding encoding
) {
    if (originalString == NULL) {
        return NULL;
    }
    OCFIndex length = OCFStringGetLength(originalString);
    if (length == 0) {
        return OCFStringCreateCopy(allocator, originalString);
    }
    StringInlineBuffer buf;
    initInlineBuffer(originalString, &buf, (OCFRange){0, length});
    if (length < 1) {
        return OCFStringCreateCopy(OCFGetAllocator(originalString), originalString);
    }
    OCFMutableStringRef newString = NULL;
    OCFIndex index = 0;
    OCFIndex count = 0;
    UniChar characters[1024];
    const char *hexChars = CreateStringFromFileSystemRepresentationByAddingPercentEscapesHexChars;
    do {
        UniChar character = inlineBufferCharacter(&buf, index);
        bool escape;
        if (character <= 0x7f && (sURLValidCharacters[character] & 1) != 0) {
            escape = legalURLCharactersToBeEscaped != NULL &&
                     _stringContainsCharacter(legalURLCharactersToBeEscaped, character);
        } else {
            escape = charactersToLeaveUnescaped == NULL ||
                     !_stringContainsCharacter(charactersToLeaveUnescaped, character);
        }
        if (escape) {
            OCFIndex usedByteLen = 0;
            uint8_t bytes[8];
            if (newString == NULL) {
                newString = OCFStringCreateMutableCopy(OCFGetAllocator(originalString), 0, originalString);
                OCFStringDelete(newString, (OCFRange){index, length - index});
            }
            if (count >= 1000) {
                OCFStringAppendCharacters(newString, characters, count);
                count = 0;
            }
            if (OCFStringEncodingUnicodeToBytes(encoding, 0, &character, 1, NULL, bytes, 8, &usedByteLen) ==
                0) {
                for (uint8_t *byte = bytes; byte < bytes + usedByteLen; ++byte) {
                    characters[count] = '%';
                    characters[count + 1] = (UniChar)hexChars[*byte >> 4];
                    characters[count + 2] = (UniChar)hexChars[*byte & 0xf];
                    count += 3;
                }
            } else {
                if (encoding != kOCFStringEncodingUTF8 || (character & 0xfc00) != 0xd800) {
                    break;
                }
                OCFIndex trailIndex = index + 1;
                if (trailIndex >= length) {
                    break;
                }
                if ((inlineBufferCharacter(&buf, trailIndex) >> 10) != 0x37) {
                    break;
                }
                UniChar pair[2];
                pair[0] = character;
                pair[1] = inlineBufferCharacter(&buf, trailIndex);
                if (OCFStringEncodingUnicodeToBytes(
                        kOCFStringEncodingUTF8, 0, pair, 2, NULL, bytes, 6, &usedByteLen
                    ) != 0) {
                    break;
                }
                for (uint8_t *byte = bytes; byte < bytes + usedByteLen; ++byte) {
                    characters[count] = '%';
                    characters[count + 1] = (UniChar)hexChars[*byte >> 4];
                    characters[count + 2] = (UniChar)hexChars[*byte & 0xf];
                    count += 3;
                }
                index = trailIndex;
            }
        } else if (newString != NULL) {
            characters[count] = character;
            if (count + 1 == 0x400) {
                OCFStringAppendCharacters(newString, characters, 0x400);
                count = 0;
            } else {
                count += 1;
            }
        }
        index += 1;
    } while (index < length);
    if (index < length) {
        if (newString != NULL) {
            OCFRelease(newString);
        }
        return NULL;
    }
    if (newString == NULL) {
        return OCFStringCreateCopy(OCFGetAllocator(originalString), originalString);
    }
    if (count != 0) {
        OCFStringAppendCharacters(newString, characters, count);
    }
    return newString;
}

// Native _stringContainsCharacter, 0x1c023faac. Non-external. Whether any character of the string, read
// through the inline buffer over the whole string, equals the character.
Boolean _stringContainsCharacter(OCFStringRef string, UniChar character) {
    OCFIndex length = OCFStringGetLength(string);
    StringInlineBuffer buffer;
    initInlineBuffer(string, &buffer, (OCFRange){0, length});
    for (OCFIndex index = 0; index < length; ++index) {
        if (inlineBufferCharacter(&buffer, index) == character) {
            return true;
        }
    }
    return false;
}

// Native __CFURLDeallocate, 0x1c023fc48. Non-external; the class's finalize callback. Released slots are
// overwritten with 0xdeadbeef. Swift layout: the string, the base URL and the object at 0x30 (plain
// accesses), then the slot at 0x28 (acquire load, release store). CF layout: the string (two acquire loads,
// release store), the base URL, the additional data's sanitized string and the block itself (freed with the
// URL's allocator, the pointer reloaded after the release), then the object at 0x30 (acquire load, release
// store).
static void __CFURLDeallocate(OCFTypeRef cf) {
    struct __OCFURL *url = (struct __OCFURL *)cf;
    if (_foundation_swift_nsurl_feature_enabled()) {
        if (url->string != NULL) {
            OCFRelease(url->string);
        }
        url->string = (OCFStringRef)(uintptr_t)0xdeadbeef;
        if (url->baseURL != NULL) {
            OCFRelease(url->baseURL);
        }
        if (url->resourceInfo != NULL) {
            OCFRelease(url->resourceInfo);
        }
        OCFTypeRef object = __atomic_load_n(&url->extra, __ATOMIC_SEQ_CST);
        if (object != NULL) {
            OCFRelease(object);
        }
        __atomic_store_n(&url->extra, (OCFURLAdditionalData *)(uintptr_t)0xdeadbeef, __ATOMIC_SEQ_CST);
        return;
    }
    OCFAllocatorRef allocator = OCFGetAllocator(url);
    if (__atomic_load_n(&url->string, __ATOMIC_SEQ_CST) != NULL) {
        OCFRelease(__atomic_load_n(&url->string, __ATOMIC_SEQ_CST));
    }
    __atomic_store_n(&url->string, (OCFStringRef)(uintptr_t)0xdeadbeef, __ATOMIC_SEQ_CST);
    if (url->baseURL != NULL) {
        OCFRelease(url->baseURL);
    }
    if (url->extra != NULL) {
        if (url->extra->sanitizedString != NULL) {
            OCFRelease(url->extra->sanitizedString);
        }
        if (url->extra != NULL) {
            OCFAllocatorDeallocate(allocator, url->extra);
        }
    }
    OCFTypeRef object = __atomic_load_n(&url->resourceInfo, __ATOMIC_SEQ_CST);
    if (object != NULL) {
        OCFRelease(object);
    }
    __atomic_store_n(&url->resourceInfo, (OCFTypeRef)(uintptr_t)0xdeadbeef, __ATOMIC_SEQ_CST);
}

// Native __CFURLEqual, 0x1c023fd34. Non-external; the class's equal callback. After the identity check the
// flags must agree in bits 0x7ffc (Swift layout), or, unless they differ in bit 17, in bits 0xe0006dff (CF
// layout); then the base URLs (both absent or CFEqual) and the strings (CFURLGetString) are compared.
// Folds the compiler-outlined piece ___CFURLEqual.cold.1 (0x1c030d460).
static Boolean __CFURLEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    OCFURLRef url1 = cf1;
    OCFURLRef url2 = cf2;
    if (_foundation_swift_nsurl_feature_enabled()) {
        if (url1 == url2) {
            return true;
        }
        if (((url1->flags ^ url2->flags) & 0x7ffc) != 0) {
            return false;
        }
    } else {
        if (url1 == url2) {
            return true;
        }
        UInt32 difference = url1->flags ^ url2->flags;
        if ((difference & 0x20000) == 0 && (difference & 0xe0006dff) != 0) {
            return false;
        }
    }
    OCFURLRef base1 = url1->baseURL;
    OCFURLRef base2 = url2->baseURL;
    if (base1 == NULL) {
        if (base2 != NULL) {
            return false;
        }
    } else {
        if (base2 == NULL) {
            return false;
        }
        if (!OCFEqual(base1, base2)) {
            return false;
        }
    }
    return OCFEqual(OCFURLGetString(url1), OCFURLGetString(url2));
}

// Native __CFURLHash, 0x1c023fe18. Non-external; the class's hash callback: 0 for NULL, otherwise the hash of
// CFURLGetString.
static OCFHashCode __CFURLHash(OCFTypeRef cf) {
    if (cf == NULL) {
        return 0;
    }
    return OCFHash(OCFURLGetString(cf));
}

// Native __CFURLCopyFormattingDescription, 0x1c023fe34. Non-external; the class's formatting description
// callback. The scheme is copied first; a "data" URL (case-insensitive) is shown through
// CreateTruncatedURLString. With a base URL the result is "%@ -- %@" of the string and the base URL (for a
// "data" URL, of the truncated strings, each released when present); without one a copy of the string (or
// the truncated string itself). The Swift layout reads the string with plain loads, the CF layout with
// acquire loads.
// Folds the compiler-outlined piece ___CFURLCopyFormattingDescription.cold.1 (0x1c030d464).
static OCFStringRef __CFURLCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions) {
    OCFURLRef url = cf;
    (void)formatOptions;
    bool swift = _foundation_swift_nsurl_feature_enabled();
    OCFAllocatorRef allocator = OCFGetAllocator(url);
    OCFStringRef scheme = OCFURLCopyScheme(url);
    OCFStringRef string;
    OCFURLRef base;
    if (swift) {
        if (scheme != NULL) {
            OCFComparisonResult comparison =
                OCFStringCompare(scheme, dataString(), kOCFCompareCaseInsensitive);
            OCFRelease(scheme);
            if (comparison == kOCFCompareEqualTo) {
                string = url->string;
                base = url->baseURL;
                goto truncated;
            }
        }
        string = url->string;
        if (url->baseURL == NULL) {
            return OCFStringCreateCopy(allocator, string);
        }
        return OCFStringCreateWithFormat(allocator, NULL, OF_STR("%@ -- %@"), string, url->baseURL);
    }
    if (scheme != NULL) {
        OCFComparisonResult comparison = OCFStringCompare(scheme, dataString(), kOCFCompareCaseInsensitive);
        OCFRelease(scheme);
        if (comparison == kOCFCompareEqualTo) {
            base = url->baseURL;
            string = __atomic_load_n(&url->string, __ATOMIC_SEQ_CST);
            goto truncated;
        }
    }
    base = url->baseURL;
    string = __atomic_load_n(&url->string, __ATOMIC_SEQ_CST);
    if (base == NULL) {
        return OCFStringCreateCopy(allocator, string);
    }
    return OCFStringCreateWithFormat(allocator, NULL, OF_STR("%@ -- %@"), string, url->baseURL);
truncated:;
    OCFStringRef truncatedString = CreateTruncatedURLString(allocator, string);
    if (base == NULL) {
        return truncatedString;
    }
    OCFStringRef truncatedBase = CreateTruncatedURLString(allocator, OCFURLGetString(url->baseURL));
    OCFStringRef result =
        OCFStringCreateWithFormat(allocator, NULL, OF_STR("%@ -- %@"), truncatedString, truncatedBase);
    if (truncatedString != NULL) {
        OCFRelease(truncatedString);
    }
    if (truncatedBase != NULL) {
        OCFRelease(truncatedBase);
    }
    return result;
}

// Native __CFURLCopyDescription, 0x1c023ffc0. Non-external; the class's debug description callback:
// "<CFURL %p [%p]>{string = %@, encoding = %u\n\tbase = %@}" with the base URL's CFCopyDescription (released
// after formatting), or "...encoding = %u, base = (null)}". A "data" URL (case-insensitive scheme) shows the
// CreateTruncatedURLString of its string, released after formatting. The Swift layout reports the stored
// encoding only when flag bit 16 is set (UTF-8 otherwise) and reads the string with plain loads; the CF
// layout reports the stored encoding and reads the string with acquire loads.
// Folds the compiler-outlined piece ___CFURLCopyDescription.cold.1 (0x1c030d468).
static OCFStringRef __CFURLCopyDescription(OCFTypeRef cf) {
    OCFURLRef url = cf;
    bool swift = _foundation_swift_nsurl_feature_enabled();
    OCFAllocatorRef allocator = OCFGetAllocator(url);
    OCFStringRef scheme = OCFURLCopyScheme(url);
    OCFStringRef withBase = OF_STR("<CFURL %p [%p]>{string = %@, encoding = %u\n\tbase = %@}");
    OCFStringRef withoutBase = OF_STR("<CFURL %p [%p]>{string = %@, encoding = %u, base = (null)}");
    if (swift) {
        bool notData;
        if (scheme != NULL) {
            notData =
                OCFStringCompare(scheme, dataString(), kOCFCompareCaseInsensitive) != kOCFCompareEqualTo;
            OCFRelease(scheme);
        } else {
            notData = true;
        }
        OCFStringEncoding encoding = (url->flags & 0x10000) != 0 ? url->encoding : kOCFStringEncodingUTF8;
        if (notData) {
            if (url->baseURL != NULL) {
                OCFStringRef baseDescription = OCFCopyDescription(url->baseURL);
                OCFStringRef result = OCFStringCreateWithFormat(
                    allocator, NULL, withBase, url, allocator, url->string, encoding, baseDescription
                );
                OCFRelease(baseDescription);
                return result;
            }
            return OCFStringCreateWithFormat(
                allocator, NULL, withoutBase, url, allocator, url->string, encoding
            );
        }
        OCFStringRef truncated = CreateTruncatedURLString(allocator, url->string);
        OCFStringRef result;
        if (url->baseURL != NULL) {
            OCFStringRef baseDescription = OCFCopyDescription(url->baseURL);
            result = OCFStringCreateWithFormat(
                allocator, NULL, withBase, url, allocator, truncated, encoding, baseDescription
            );
            OCFRelease(baseDescription);
        } else {
            result =
                OCFStringCreateWithFormat(allocator, NULL, withoutBase, url, allocator, truncated, encoding);
        }
        OCFRelease(truncated);
        return result;
    }
    if (scheme != NULL) {
        OCFComparisonResult comparison = OCFStringCompare(scheme, dataString(), kOCFCompareCaseInsensitive);
        OCFRelease(scheme);
        if (comparison == kOCFCompareEqualTo) {
            OCFStringRef truncated =
                CreateTruncatedURLString(allocator, __atomic_load_n(&url->string, __ATOMIC_SEQ_CST));
            OCFStringRef result;
            if (url->baseURL != NULL) {
                OCFStringRef baseDescription = OCFCopyDescription(url->baseURL);
                result = OCFStringCreateWithFormat(
                    allocator, NULL, withBase, url, allocator, truncated, url->encoding, baseDescription
                );
                OCFRelease(baseDescription);
            } else {
                result = OCFStringCreateWithFormat(
                    allocator, NULL, withoutBase, url, allocator, truncated, url->encoding
                );
            }
            OCFRelease(truncated);
            return result;
        }
    }
    if (url->baseURL != NULL) {
        OCFStringRef baseDescription = OCFCopyDescription(url->baseURL);
        OCFStringRef string = __atomic_load_n(&url->string, __ATOMIC_SEQ_CST);
        OCFStringRef result = OCFStringCreateWithFormat(
            allocator, NULL, withBase, url, allocator, string, url->encoding, baseDescription
        );
        OCFRelease(baseDescription);
        return result;
    }
    OCFStringRef string = __atomic_load_n(&url->string, __ATOMIC_SEQ_CST);
    return OCFStringCreateWithFormat(allocator, NULL, withoutBase, url, allocator, string, url->encoding);
}

// Native CFURLGetTypeID, 0x1c02401f0. External; the builtin type 29.
OCFTypeID OCFURLGetTypeID(void) {
    return 29;
}

static inline OCFURLRef sendURL(OCFURLRef url, const char *selector) {
    return ((OCFURLRef (*)(OCFURLRef, SEL))objc_msgSend)(url, sel_getUid(selector));
}

static inline OCFStringRef sendString(OCFTypeRef object, const char *selector) {
    return ((OCFStringRef (*)(OCFTypeRef, SEL))objc_msgSend)(object, sel_getUid(selector));
}

// Native _CFURLHasFileURLScheme, 0x1c0240850. Non-external. hasScheme may be NULL.
// Swift layout: an Objective-C URL walks its base URLs ([url baseURL]) and asks the last one's scheme (no
// release; hasScheme receives whether there is one); a CF URL answers flag bit 12 at once (hasScheme bit 2)
// without visiting its base. CF layout: CFURLGetBaseURL is followed to the end; a well-known scheme number
// (flags >> 29, nonzero) answers whether it is 3 (hasScheme: flag bit 0); otherwise the copied scheme is
// compared with "file", by address and then case-insensitively.
// Folds the compiler-outlined piece __CFURLHasFileURLScheme.cold.1 (0x1c030d470).
Boolean _CFURLHasFileURLScheme(OCFURLRef url, Boolean *hasScheme) {
    for (;;) {
        if (_foundation_swift_nsurl_feature_enabled()) {
            if (!isObjCURL(url)) {
                UInt32 flags = url->flags;
                if (hasScheme != NULL) {
                    *hasScheme = (flags >> 2) & 1;
                }
                return (flags >> 12) & 1;
            }
            OCFURLRef base = sendURL(url, "baseURL");
            if (base != NULL) {
                url = base;
                continue;
            }
            OCFStringRef scheme = sendString(url, "scheme");
            if (hasScheme != NULL) {
                *hasScheme = scheme != NULL;
            }
            if (scheme == NULL) {
                return false;
            }
            return OCFStringCompare(scheme, fileString(), kOCFCompareCaseInsensitive) == kOCFCompareEqualTo;
        }
        OCFURLRef base = OCFURLGetBaseURL(url);
        if (base != NULL) {
            url = base;
            continue;
        }
        break;
    }
    if (!isObjCURL(url)) {
        UInt32 flags = url->flags;
        if ((flags >> 29) != 0) {
            if (hasScheme != NULL) {
                *hasScheme = flags & 1;
                flags = url->flags;
            }
            return (flags >> 29) == 3;
        }
    }
    OCFStringRef scheme = OCFURLCopyScheme(url);
    if (scheme == NULL) {
        if (hasScheme != NULL) {
            *hasScheme = false;
        }
        return false;
    }
    Boolean result;
    if (scheme == fileString()) {
        result = true;
    } else {
        result = OCFStringCompare(scheme, fileString(), kOCFCompareCaseInsensitive) == kOCFCompareEqualTo;
    }
    if (hasScheme != NULL) {
        *hasScheme = true;
    }
    OCFRelease(scheme);
    return result;
}

// Native _CFURLCreateWithURLString, 0x1c0240dac. Non-external. checkForLegalCharacters is 1 from
// CFURLCreateWithString and 0 from _CFURLCreateWithArbitraryString.
// Swift layout: NULL without a string; an instance of type 29 with 0x48 extra bytes (0xa8 from 0x10000
// characters) is filled by +[NSURL __swiftParseSmall:into:allowEncoding:] (or __swiftParseBig:...) at
// offset 0x10, allowEncoding being !checkForLegalCharacters; a failed parse releases it. Unless the parse
// found a scheme (bit 2), a base URL is made absolute and stored, flag bit 12 marking a file-scheme base and
// bit 13 a file-reference base.
// CF layout: with checkForLegalCharacters the string (NULL answers NULL) is checked in 64-character chunks
// of an inlined CFStringInlineBuffer: a '%' needs two following hexadecimal digits (bit 4) and a character
// two places after it, at most one '#' is allowed, '[' and ']' pass, every other character must be legal
// (bit 0); any violation answers NULL. A string starting with a scheme-start character (bit 1) that reaches
// ':' through scheme characters (bit 3) ignores the base URL. _parseComponents fills the flags and up to nine
// ranges; the instance carries 0x28 + 16 * ranges extra bytes, the flags, UTF-8, the ranges, a copy of the
// string (release store) and the absolute base URL. The string is then sanitized through scanCharacters
// (per component for flag bit 14, otherwise from the scheme length plus one to the end); an escaped string
// becomes the additional data's sanitized string (the scan's additional flags stored first), otherwise flag
// bit 16 is set when the scan succeeded or found nothing to escape.
// Folds the compiler-outlined piece __CFURLCreateWithURLString.cold.1 (0x1c030d478).
OCFURLRef _CFURLCreateWithURLString(
    OCFAllocatorRef allocator, OCFStringRef string, Boolean checkForLegalCharacters, OCFURLRef baseURL
) {
    if (_foundation_swift_nsurl_feature_enabled()) {
        if (string == NULL) {
            return NULL;
        }
        struct __OCFURL *url;
        BOOL parsed;
        if (OCFStringGetLength(string) >= 0x10000) {
            url = (struct __OCFURL *)_OCFRuntimeCreateInstance(allocator, 29, 0xa8, NULL);
            if (url == NULL) {
                return NULL;
            }
            parsed = ((BOOL (*)(void *, SEL, OCFStringRef, void *, BOOL))objc_msgSend)(
                urlClass,
                sel_getUid("__swiftParseBig:into:allowEncoding:"),
                string,
                &url->flags,
                checkForLegalCharacters == 0
            );
        } else {
            url = (struct __OCFURL *)_OCFRuntimeCreateInstance(allocator, 29, 0x48, NULL);
            if (url == NULL) {
                return NULL;
            }
            parsed = ((BOOL (*)(void *, SEL, OCFStringRef, void *, BOOL))objc_msgSend)(
                urlClass,
                sel_getUid("__swiftParseSmall:into:allowEncoding:"),
                string,
                &url->flags,
                checkForLegalCharacters == 0
            );
        }
        if ((parsed & 1) == 0) {
            OCFRelease(url);
            return NULL;
        }
        if (baseURL != NULL && (url->flags & 4) == 0) {
            OCFURLRef absoluteBase = OCFURLCopyAbsoluteURL(baseURL);
            if (absoluteBase != NULL && _CFURLHasFileURLScheme(absoluteBase, NULL)) {
                url->flags |= 0x1000;
                if (OCFURLIsFileReferenceURL(absoluteBase)) {
                    url->flags |= 0x2000;
                }
            }
            url->baseURL = absoluteBase;
        }
        return url;
    }
    StringInlineBuffer buf;
    if (checkForLegalCharacters) {
        if (string == NULL) {
            return NULL;
        }
        memset(buf.buffer, 0, sizeof(buf.buffer));
        OCFIndex length = OCFStringGetLength(string);
        initInlineBuffer(string, &buf, (OCFRange){0, length});
        if (length >= 1) {
            bool sawHash = false;
            OCFIndex expectedHexDigits = 0;
            OCFIndex start = 0;
            do {
                OCFIndex chunk = start + 64 > length ? length - start : 64;
                const UniChar *characters = inlineBufferCharactersPtr(&buf, (OCFRange){start, chunk});
                for (OCFIndex offset = 0; offset < chunk; ++offset) {
                    UniChar character = characters[offset];
                    if (expectedHexDigits != 0) {
                        if (character > 0x7f || (sURLValidCharacters[character] & 0x10) == 0) {
                            return NULL;
                        }
                        expectedHexDigits -= 1;
                        continue;
                    }
                    if (character <= 'Z') {
                        if (character == '#') {
                            if (sawHash) {
                                return NULL;
                            }
                            expectedHexDigits = 0;
                            sawHash = true;
                            continue;
                        }
                        if (character == '%') {
                            if (start + offset + 2 >= length) {
                                return NULL;
                            }
                            expectedHexDigits = 2;
                            continue;
                        }
                    } else {
                        expectedHexDigits = 0;
                        if (character == '[' || character == ']') {
                            continue;
                        }
                    }
                    if (character > 0x7f || (sURLValidCharacters[character] & 1) == 0) {
                        return NULL;
                    }
                    expectedHexDigits = 0;
                }
                start += chunk;
            } while (start < length);
        }
    }
    memset(&buf, 0, sizeof(buf));
    OCFIndex length = OCFStringGetLength(string);
    initInlineBuffer(string, &buf, (OCFRange){0, length});
    if (length >= 1) {
        UniChar first = inlineBufferCharacter(&buf, 0);
        if (first <= 0x7f && (sURLValidCharacters[first] & 2) != 0) {
            OCFIndex start = 0;
            bool done = false;
            do {
                OCFIndex chunk = start + 64 > length ? length - start : 64;
                const UniChar *characters = inlineBufferCharactersPtr(&buf, (OCFRange){start, chunk});
                for (OCFIndex offset = 0; offset < chunk; ++offset) {
                    UniChar character = characters[offset];
                    if (character == ':') {
                        baseURL = NULL;
                        done = true;
                        break;
                    }
                    if (character > 0x7f || (sURLValidCharacters[character] & 8) == 0) {
                        done = true;
                        break;
                    }
                }
                start += chunk;
            } while (!done && start < length);
        }
    }
    uint8_t numberOfRanges = 0;
    UInt32 flags = 0;
    OCFRange ranges[9];
    _parseComponents(string, baseURL, &flags, ranges, &numberOfRanges);
    size_t rangesSize = (size_t)numberOfRanges * 16;
    struct __OCFURL *url =
        (struct __OCFURL *)_OCFRuntimeCreateInstance(allocator, 29, rangesSize + 0x28, NULL);
    if (url == NULL) {
        return NULL;
    }
    url->flags = flags;
    url->encoding = kOCFStringEncodingUTF8;
    memcpy(url->ranges, ranges, rangesSize);
    __atomic_store_n(&url->string, OCFStringCreateCopy(allocator, string), __ATOMIC_SEQ_CST);
    url->baseURL = baseURL != NULL ? OCFURLCopyAbsoluteURL(baseURL) : NULL;
    OCFAllocatorRef urlAllocator = OCFGetAllocator(url);
    OCFIndex stringLength = OCFStringGetLength(__atomic_load_n(&url->string, __ATOMIC_SEQ_CST));
    Boolean freeCharacters = false;
    Boolean useCString = false;
    const char *cString = NULL;
    OCFIndex mark = 0;
    const UniChar *characters = NULL;
    OCFMutableStringRef escapedString = NULL;
    UInt32 additionalFlags = 0;
    UInt8 buffer[1024];
    constructBuffers(
        __atomic_load_n(&url->string, __ATOMIC_SEQ_CST),
        buffer,
        &cString,
        &characters,
        &useCString,
        &freeCharacters
    );
    if ((url->flags & 0x4000) != 0) {
        mark = 0;
        for (UInt32 bit = 2; bit < 0x200; bit <<= 1) {
            UInt32 componentFlags = url->flags;
            OCFRange range;
            if ((componentFlags & bit) != 0) {
                uint32_t index = 0;
                UInt32 remaining = bit;
                UInt32 previous;
                do {
                    previous = remaining;
                    remaining >>= 1;
                    index += componentFlags & 1;
                    componentFlags >>= 1;
                } while ((previous & 2) == 0);
                range = url->ranges[index];
            } else {
                range = (OCFRange){kOCFNotFound, 0};
            }
            if (range.location != kOCFNotFound) {
                scanCharacters(
                    urlAllocator,
                    &escapedString,
                    &additionalFlags,
                    cString,
                    characters,
                    useCString,
                    range.location,
                    range.location + range.length,
                    &mark,
                    bit,
                    url->encoding
                );
            }
        }
        if (escapedString == NULL) {
            url->flags |= 0x10000;
            goto done;
        }
    } else {
        OCFIndex start = ((url->flags & 1) != 0 ? url->ranges[0].length : 0) + 1;
        mark = 0;
        if (!scanCharacters(
                urlAllocator,
                &escapedString,
                &additionalFlags,
                cString,
                characters,
                useCString,
                start,
                stringLength,
                &mark,
                0,
                url->encoding
            )) {
            url->flags |= 0x10000;
        }
        if (escapedString == NULL) {
            goto done;
        }
    }
    UInt32 scannedFlags = additionalFlags;
    if (scannedFlags != 0 && url->extra == NULL) {
        _CFURLAllocateExtraDataspace(url);
    }
    if (url->extra != NULL) {
        url->extra->additionalFlags = scannedFlags;
    }
    if (stringLength - mark > 0) {
        if (useCString) {
            __OCFStringAppendBytes(
                escapedString, cString + mark, stringLength - mark, kOCFStringEncodingISOLatin1
            );
        } else {
            OCFStringAppendCharacters(escapedString, characters + mark, stringLength - mark);
        }
    }
    if (url->extra == NULL) {
        _CFURLAllocateExtraDataspace(url);
    }
    if (url->extra != NULL) {
        if (url->extra->sanitizedString != NULL) {
            OCFRelease(url->extra->sanitizedString);
        }
        url->extra->sanitizedString = OCFStringCreateCopy(OCFGetAllocator(url), escapedString);
    }
    OCFRelease(escapedString);
done:
    if (freeCharacters) {
        free(useCString ? (void *)cString : (void *)characters);
    }
    return url;
}

// Native CFURLGetString, 0x1c0241670. External.
// An Objective-C URL answers -relativeString. Swift layout: the string at 0x30 when present, otherwise the
// one at 0x18. CF layout: the string itself (acquire load) when flag bit 16 is set, otherwise the additional
// data's sanitized string, or NULL without additional data.
// Folds the compiler-outlined piece _CFURLGetString.cold.1 (0x1c030d47c).
OCFStringRef OCFURLGetString(OCFURLRef anURL) {
    if (_foundation_swift_nsurl_feature_enabled()) {
        if (isObjCURL(anURL)) {
            return sendString(anURL, "relativeString");
        }
        OCFStringRef string = (OCFStringRef)anURL->resourceInfo;
        if (string != NULL) {
            return string;
        }
        return anURL->string;
    }
    if (isObjCURL(anURL)) {
        return sendString(anURL, "relativeString");
    }
    if ((anURL->flags & 0x10000) != 0) {
        return __atomic_load_n(&anURL->string, __ATOMIC_SEQ_CST);
    }
    if (anURL->extra != NULL) {
        return anURL->extra->sanitizedString;
    }
    return NULL;
}

// Native CFURLCreateWithString, 0x1c0241724. External. The feature state is computed first (no branch on
// it); NULL without a string, otherwise _CFURLCreateWithURLString checking for legal characters.
// Folds the compiler-outlined piece _CFURLCreateWithString.cold.1 (0x1c030d480).
OCFURLRef OCFURLCreateWithString(OCFAllocatorRef allocator, OCFStringRef URLString, OCFURLRef baseURL) {
    _foundation_swift_nsurl_feature_enabled();
    if (URLString == NULL) {
        return NULL;
    }
    return _CFURLCreateWithURLString(allocator, URLString, true, baseURL);
}

// Native CFURLCopyAbsoluteURL, 0x1c0241f20. External.
// An Objective-C URL answers -absoluteURL (objc_retain in the Swift layout, CFRetain when present in the CF
// layout); a URL without a base is retained. A file-reference base without a directory path is first turned
// into a file path URL (CFURLCreateFilePathURL; NULL answers NULL).
// Swift layout: an Objective-C base is re-created from its -relativeString (unchecked, no base); an empty
// string answers that base (or retains a CF base); otherwise +[NSURL
// __copySwiftAbsoluteString:baseHeader:useRFC1808:] (with YES) builds the absolute string, which becomes an
// unchecked URL carrying flag bit 16 and the encoding when the URL had bit 16.
// CF layout: the base's string, flags and ranges (parsed with _parseComponents for an Objective-C base) and
// the URL's are combined by resolveAbsoluteURLString into an arbitrary-string URL that receives the URL's
// encoding; a created file path base is released.
// Folds the compiler-outlined piece _CFURLCopyAbsoluteURL.cold.1 (0x1c030d488).
OCFURLRef OCFURLCopyAbsoluteURL(OCFURLRef relativeURL) {
    if (_foundation_swift_nsurl_feature_enabled()) {
        if (isObjCURL(relativeURL)) {
            return (OCFURLRef)objc_retain((id)sendURL(relativeURL, "absoluteURL"));
        }
        OCFURLRef base = relativeURL->baseURL;
        if (base == NULL) {
            return OCFRetain(relativeURL);
        }
        OCFAllocatorRef allocator = OCFGetAllocator(relativeURL);
        OCFURLRef headerBase;
        if (OCFURLIsFileReferenceURL(base) && (OCFURLHasDirectoryPath(base) & 1) == 0) {
            headerBase = OCFURLCreateFilePathURL(allocator, base, NULL);
        } else if (isObjCURL(base)) {
            headerBase =
                _CFURLCreateWithURLString(allocator, sendString(base, "relativeString"), false, NULL);
        } else {
            headerBase = base;
        }
        OCFStringRef absoluteString;
        if (headerBase == base) {
            if (OCFStringGetLength(relativeURL->string) == 0) {
                return OCFRetain(base);
            }
            absoluteString = ((OCFStringRef (*)(void *, SEL, const void *, const void *, BOOL))objc_msgSend)(
                urlClass,
                sel_getUid("__copySwiftAbsoluteString:baseHeader:useRFC1808:"),
                &relativeURL->flags,
                &base->flags,
                YES
            );
        } else {
            if (headerBase == NULL) {
                return NULL;
            }
            if (OCFStringGetLength(relativeURL->string) == 0) {
                return headerBase;
            }
            absoluteString = ((OCFStringRef (*)(void *, SEL, const void *, const void *, BOOL))objc_msgSend)(
                urlClass,
                sel_getUid("__copySwiftAbsoluteString:baseHeader:useRFC1808:"),
                &relativeURL->flags,
                &headerBase->flags,
                YES
            );
            OCFRelease(headerBase);
        }
        if (absoluteString == NULL) {
            return NULL;
        }
        struct __OCFURL *result =
            (struct __OCFURL *)_CFURLCreateWithURLString(allocator, absoluteString, false, NULL);
        OCFRelease(absoluteString);
        if (result != NULL && (relativeURL->flags & 0x10000) != 0) {
            result->flags |= 0x10000;
            result->encoding = relativeURL->encoding;
        }
        return result;
    }
    OCFAllocatorRef allocator = OCFGetAllocator(relativeURL);
    UInt32 baseFlags = 0;
    uint8_t numberOfRanges = 0;
    if (isObjCURL(relativeURL)) {
        OCFURLRef absolute = sendURL(relativeURL, "absoluteURL");
        if (absolute != NULL) {
            OCFRetain(absolute);
        }
        return absolute;
    }
    OCFURLRef base = relativeURL->baseURL;
    if (base == NULL) {
        return OCFRetain(relativeURL);
    }
    bool baseIsBorrowed = true;
    if (OCFURLIsFileReferenceURL(base) && (OCFURLHasDirectoryPath(base) & 1) == 0) {
        base = OCFURLCreateFilePathURL(allocator, base, NULL);
        if (base == NULL) {
            return NULL;
        }
        baseIsBorrowed = false;
    }
    OCFStringRef baseString;
    const OCFRange *baseRanges;
    OCFRange parsedRanges[9];
    if (isObjCURL(base)) {
        baseString = OCFURLGetString(base);
        baseFlags = 0;
        _parseComponents(baseString, NULL, &baseFlags, parsedRanges, &numberOfRanges);
        baseRanges = parsedRanges;
    } else {
        baseString = __atomic_load_n(&base->string, __ATOMIC_SEQ_CST);
        baseFlags = base->flags;
        baseRanges = base->ranges;
    }
    OCFStringRef absoluteString = resolveAbsoluteURLString(
        allocator,
        __atomic_load_n(&relativeURL->string, __ATOMIC_SEQ_CST),
        relativeURL->flags,
        relativeURL->ranges,
        baseString,
        baseFlags,
        baseRanges
    );
    struct __OCFURL *result;
    if (absoluteString != NULL) {
        result = (struct __OCFURL *)_CFURLCreateWithArbitraryString(allocator, absoluteString, NULL);
        OCFRelease(absoluteString);
        result->encoding = relativeURL->encoding;
    } else {
        result = NULL;
    }
    if (!baseIsBorrowed) {
        OCFRelease(base);
    }
    return result;
}

// Native _CFURLCreateWithArbitraryString, 0x1c02422c0. Non-external. The feature state is computed first (no
// branch on it); NULL without a string, otherwise _CFURLCreateWithURLString without the legal-character
// check.
// Folds the compiler-outlined piece __CFURLCreateWithArbitraryString.cold.1 (0x1c030d48c).
OCFURLRef
_CFURLCreateWithArbitraryString(OCFAllocatorRef allocator, OCFStringRef URLString, OCFURLRef baseURL) {
    _foundation_swift_nsurl_feature_enabled();
    if (URLString == NULL) {
        return NULL;
    }
    return _CFURLCreateWithURLString(allocator, URLString, false, baseURL);
}

// Native CFURLCopyScheme, 0x1c024232c. External.
// An Objective-C URL answers -scheme (objc_retain in the Swift layout, CFRetain when present in the CF
// layout). A URL without its own scheme answers its base URL's (NULL at the end of the chain), the feature
// state being asked again for each base.
// Swift layout: flag bit 2 marks a scheme; range forms 2 and 3 answer "file"; form 0 packs the range in 32
// bits at 0x38 and form 1 stores it as two words; a four-character scheme reads as ASCII bytes "http",
// "file" (one 32-bit compare) or "data" answers that constant (retained), any other a substring.
// CF layout: the well-known scheme numbers answer "http", "https", "file", "data" and "ftp" (retained);
// otherwise _retainedComponentString(url, 1, true, false).
// Folds the compiler-outlined piece _CFURLCopyScheme.cold.1 (0x1c030d490).
OCFStringRef OCFURLCopyScheme(OCFURLRef anURL) {
    for (;;) {
        if (_foundation_swift_nsurl_feature_enabled()) {
            if (isObjCURL(anURL)) {
                return (OCFStringRef)objc_retain((id)sendString(anURL, "scheme"));
            }
            UInt32 flags = anURL->flags;
            if ((flags & 4) == 0) {
                anURL = anURL->baseURL;
                if (anURL == NULL) {
                    return NULL;
                }
                continue;
            }
            OCFRange range;
            switch (flags & 3) {
                case 0: {
                    uint32_t packed = swiftField32(anURL, 0x38);
                    range = (OCFRange){packed & 0xffff, packed >> 16};
                    break;
                }
                case 1:
                    range = anURL->ranges[0];
                    break;
                default:
                    return OCFRetain(fileString());
            }
            if (range.length == 4) {
                uint8_t buffer[4] = {0, 0, 0, 0};
                OCFStringGetBytes(anURL->string, range, kOCFStringEncodingASCII, 0, false, buffer, 4, NULL);
                uint32_t word;
                memcpy(&word, buffer, 4);
                if (buffer[0] == 'h' && buffer[1] == 't' && buffer[2] == 't' && buffer[3] == 'p') {
                    return OCFRetain(httpString());
                }
                if (word == 0x656c6966) {
                    return OCFRetain(fileString());
                }
                if (buffer[0] == 'd' && buffer[1] == 'a' && buffer[2] == 't' && buffer[3] == 'a') {
                    return OCFRetain(dataString());
                }
            }
            return OCFStringCreateWithSubstring(OCFGetAllocator(anURL), anURL->string, range);
        }
        if (isObjCURL(anURL)) {
            OCFStringRef scheme = sendString(anURL, "scheme");
            if (scheme != NULL) {
                OCFRetain(scheme);
            }
            return scheme;
        }
        switch (anURL->flags >> 29) {
            case 1:
                return OCFRetain(httpString());
            case 2:
                return OCFRetain(httpsString());
            case 3:
                return OCFRetain(fileString());
            case 4:
                return OCFRetain(dataString());
            case 5:
                return OCFRetain(ftpString());
            default:
                break;
        }
        OCFStringRef scheme = _retainedComponentString(anURL, 1, true, false);
        if (scheme != NULL) {
            return scheme;
        }
        anURL = anURL->baseURL;
        if (anURL == NULL) {
            return NULL;
        }
    }
}

// The character at an index of the buffers constructBuffers selects, zero-extended from a byte in the
// C-string case. _parseComponents compares characters only with 7-bit values and tests them against 0x7f, so
// the native code's sign-extended first-byte load and zero-extended later loads give the same results.
static inline UniChar
parsedCharacter(const char *cString, const UniChar *characters, Boolean useCString, OCFIndex index) {
    return useCString ? (UniChar)(UInt8)cString[index] : characters[index];
}

// Native _parseComponents, 0x1c0242670. Non-external. One body is expanded twice natively (Latin-1 C string
// and UTF-16 buffers from constructBuffers into a 1,024-byte stack buffer); the expansions agree and are
// restored as one loop over parsedCharacter. The flags start from *theFlags; nine local ranges start at
// {0, 0}. Order: the fragment after the first '#' (bit 8, range 8; the parse then stops at the '#'); the
// scheme when the first character is ':' or an alphabetic scheme character above '@', scanning scheme
// characters (sURLValidCharacters bit 3, 7-bit) up to ':' (bit 0, range 0 = {0, length}); an exact
// "ftp", "data", "file", "http" or "https" stores scheme type 5, 4, 3, 1 or 2 in bits 29..31. With bit 0
// set (from the scheme or the incoming flags) and no '/' after the scheme, the flags without bit 8 are
// stored with only range 0 = {0, scheme length} and one range. Otherwise bit 14 is set; "//" starts a net
// location up to the next '/' or '?' (none when empty): user (bit 1) and password (bit 2) before the first
// '@', the host (bit 3) after it, bracketed spans setting bit 10 and the first ':' outside them starting
// the port (bit 4). The ranges of bit 1 and bit 3 take their full lengths only while bits 2 and 4 are clear
// in the flags. The query follows the first '?' (bit 7). The path (bit 5) is the span before it; an empty
// path is kept only with a net-location bit and then takes bit 11 from the base URL's directory flag. A
// nonempty path sets bit 13 for a "/.file/" prefix, else bit 15 when it contains no '%', and bit 11 when it
// ends in "/", "/.", "/.." or is "." or "..". The set ranges are then packed in bit order (bits 0..8, the
// stored flags reread each step) and the range count stored. The converted buffer is freed last.
void _parseComponents(
    OCFStringRef string, OCFURLRef baseURL, UInt32 *theFlags, OCFRange *packedRanges, uint8_t *numberOfRanges
) {
    OCFIndex length = OCFStringGetLength(string);
    __attribute__((aligned(16))) UInt8 buffer[1024];
    const char *cString;
    const UniChar *characters;
    Boolean useCString;
    Boolean freeCharacters;
    constructBuffers(string, buffer, &cString, &characters, &useCString, &freeCharacters);
    UInt32 flags = *theFlags;
    OCFRange ranges[9];
    memset(ranges, 0, sizeof(ranges));
    uint8_t count = 0;
    for (OCFIndex index = 0; index < length; ++index) {
        if (parsedCharacter(cString, characters, useCString, index) == '#') {
            flags |= 0x100;
            ranges[8] = (OCFRange){index + 1, length - (index + 1)};
            count = 1;
            length = index;
            break;
        }
    }
    OCFIndex schemeLength = 0;
    OCFIndex base = 0;
    UniChar first = length >= 1 ? parsedCharacter(cString, characters, useCString, 0) : 0;
    if (length >= 1 &&
        (first == ':' || (first > '@' && first <= 0x7f && (sURLValidCharacters[first] & 8) != 0))) {
        OCFIndex index = 0;
        for (;;) {
            UniChar character = parsedCharacter(cString, characters, useCString, index);
            if (character == ':') {
                flags |= 1;
                ranges[0] = (OCFRange){0, index};
                count += 1;
                schemeLength = index;
                base = index + 1;
                UniChar c0 = parsedCharacter(cString, characters, useCString, 0);
                if (index == 3) {
                    if (c0 == 'f' && parsedCharacter(cString, characters, useCString, 1) == 't' &&
                        parsedCharacter(cString, characters, useCString, 2) == 'p') {
                        flags = (flags & 0x1fffffff) | 0xa0000000;
                    }
                } else if (index == 4) {
                    UniChar c1 = parsedCharacter(cString, characters, useCString, 1);
                    UniChar c2 = parsedCharacter(cString, characters, useCString, 2);
                    if (c0 == 'd') {
                        if (c1 == 'a' && c2 == 't' &&
                            parsedCharacter(cString, characters, useCString, 3) == 'a') {
                            flags = (flags & 0x1fffffff) | 0x80000000;
                        }
                    } else if (c0 == 'f') {
                        if (c1 == 'i' && c2 == 'l' &&
                            parsedCharacter(cString, characters, useCString, 3) == 'e') {
                            flags = (flags & 0x1fffffff) | 0x60000000;
                        }
                    } else if (c0 == 'h') {
                        if (c1 == 't' && c2 == 't' &&
                            parsedCharacter(cString, characters, useCString, 3) == 'p') {
                            flags = (flags & 0x1fffffff) | 0x20000000;
                        }
                    }
                } else if (index == 5) {
                    if (c0 == 'h' && parsedCharacter(cString, characters, useCString, 1) == 't' &&
                        parsedCharacter(cString, characters, useCString, 2) == 't' &&
                        parsedCharacter(cString, characters, useCString, 3) == 'p' &&
                        parsedCharacter(cString, characters, useCString, 4) == 's') {
                        flags = (flags & 0x1fffffff) | 0x40000000;
                    }
                }
                break;
            }
            if (character > 0x7f || (sURLValidCharacters[character] & 8) == 0) {
                break;
            }
            if (++index >= length) {
                break;
            }
        }
    }
    if ((flags & 1) != 0 &&
        (length == base || parsedCharacter(cString, characters, useCString, base) != '/')) {
        *theFlags = flags & ~UINT32_C(0x100);
        packedRanges[0] = (OCFRange){0, schemeLength};
        *numberOfRanges = 1;
    } else {
        flags |= 0x4000;
        OCFIndex pathStart = base;
        if (length - base >= 2 && parsedCharacter(cString, characters, useCString, base) == '/' &&
            parsedCharacter(cString, characters, useCString, base + 1) == '/') {
            OCFIndex hostStart = base + 2;
            pathStart = hostStart;
            if (hostStart < length) {
                OCFIndex extent;
                for (extent = hostStart; extent < length; ++extent) {
                    if ((parsedCharacter(cString, characters, useCString, extent) | 0x10) == '?') {
                        break;
                    }
                }
                if (extent != hostStart) {
                    uint8_t hostCount = count;
                    OCFIndex at;
                    for (at = hostStart; at < extent; ++at) {
                        if (parsedCharacter(cString, characters, useCString, at) == '@') {
                            break;
                        }
                    }
                    if (at < extent) {
                        flags |= 2;
                        hostCount = count + 1;
                        ranges[1].location = hostStart;
                        for (OCFIndex colon = hostStart; colon < at; ++colon) {
                            if (parsedCharacter(cString, characters, useCString, colon) == ':') {
                                flags |= 4;
                                hostCount = count + 2;
                                ranges[2] = (OCFRange){colon + 1, at - colon - 1};
                                ranges[1].length = colon - hostStart;
                                break;
                            }
                        }
                        if ((flags & 4) == 0) {
                            ranges[1].length = at - hostStart;
                        }
                        hostStart = at + 1;
                    }
                    flags |= 8;
                    count = hostCount + 1;
                    ranges[3].location = hostStart;
                    for (OCFIndex index = hostStart; index < extent; ++index) {
                        UniChar character = parsedCharacter(cString, characters, useCString, index);
                        if (character == '[') {
                            do {
                                if (parsedCharacter(cString, characters, useCString, index) == ']') {
                                    flags |= 0x400;
                                    break;
                                }
                            } while (++index < extent);
                        } else if (character == ':') {
                            flags |= 0x10;
                            count = hostCount + 2;
                            ranges[4] = (OCFRange){index + 1, extent - (index + 1)};
                            ranges[3].length = index - hostStart;
                            break;
                        }
                    }
                    if ((flags & 0x10) == 0) {
                        ranges[3].length = extent - hostStart;
                    }
                    pathStart = extent;
                }
            }
        }
        OCFIndex pathEnd;
        for (pathEnd = pathStart; pathEnd < length; ++pathEnd) {
            if (parsedCharacter(cString, characters, useCString, pathEnd) == '?') {
                break;
            }
        }
        if (pathEnd < length) {
            flags |= 0x80;
            count += 1;
            ranges[7] = (OCFRange){pathEnd + 1, length - (pathEnd + 1)};
        } else {
            pathEnd = length;
        }
        OCFIndex pathLength = pathEnd - pathStart;
        if (pathLength == 0) {
            if ((flags & 0x1e) != 0) {
                flags |= 0x20;
                count += 1;
                ranges[5] = (OCFRange){pathStart, 0};
                if (baseURL != NULL && OCFURLHasDirectoryPath(baseURL)) {
                    flags |= 0x800;
                }
            }
        } else {
            count += 1;
            ranges[5] = (OCFRange){pathStart, pathLength};
            if (pathLength < 1) {
                flags |= 0x20;
                if (baseURL != NULL && OCFURLHasDirectoryPath(baseURL)) {
                    flags |= 0x800;
                }
            } else {
                OCFIndex scan = pathStart;
                UniChar character;
                do {
                    character = parsedCharacter(cString, characters, useCString, scan++);
                } while (character != '%' && scan < pathEnd);
                if ((uint64_t)pathLength >= 7 &&
                    parsedCharacter(cString, characters, useCString, pathStart) == '/' &&
                    parsedCharacter(cString, characters, useCString, pathStart + 1) == '.' &&
                    parsedCharacter(cString, characters, useCString, pathStart + 2) == 'f' &&
                    parsedCharacter(cString, characters, useCString, pathStart + 3) == 'i' &&
                    parsedCharacter(cString, characters, useCString, pathStart + 4) == 'l' &&
                    parsedCharacter(cString, characters, useCString, pathStart + 5) == 'e' &&
                    parsedCharacter(cString, characters, useCString, pathStart + 6) == '/') {
                    flags |= 0x2020;
                } else if (character == '%') {
                    flags |= 0x20;
                } else {
                    flags |= 0x8020;
                }
                UniChar last = parsedCharacter(cString, characters, useCString, pathEnd - 1);
                if (last == '/') {
                    flags |= 0x800;
                } else if (last == '.') {
                    UniChar previous;
                    if (pathLength == 1 ||
                        (previous = parsedCharacter(cString, characters, useCString, pathEnd - 2)) == '/') {
                        flags |= 0x800;
                    } else if (
                        previous == '.' &&
                        (pathLength == 2 ||
                         parsedCharacter(cString, characters, useCString, pathEnd - 3) == '/')
                    ) {
                        flags |= 0x800;
                    }
                }
            }
        }
        *theFlags = flags;
        *numberOfRanges = count;
        uint8_t packed = 0;
        UInt32 bit = 1;
        for (uint32_t component = 0; component < 9; ++component) {
            if ((*theFlags & bit) != 0) {
                packedRanges[packed] = ranges[component];
                packed += 1;
            }
            bit <<= 1;
        }
    }
    if (freeCharacters) {
        free(useCString ? (void *)cString : (void *)characters);
    }
}

// Native resolveAbsoluteURLString, 0x1c0243420. Non-external. The work buffer holds the sum of both string
// lengths (the base string's length is read first): up to 1,024 characters on the stack (at least one
// character), otherwise a malloc_type_malloc block (type 0x1000040BDFB0063; NULL when it fails), freed
// after resolveAbsoluteURLStringBuffer.
OCFStringRef resolveAbsoluteURLString(
    OCFAllocatorRef allocator,
    OCFStringRef relativeString,
    UInt32 relativeFlags,
    const OCFRange *relativeRanges,
    OCFStringRef baseString,
    UInt32 baseFlags,
    const OCFRange *baseRanges
) {
    OCFIndex baseLength = OCFStringGetLength(baseString);
    OCFIndex length = OCFStringGetLength(relativeString) + baseLength;
    if (length <= 0x400) {
        UniChar buffer[(uint64_t)length > 1 ? length : 1];
        return resolveAbsoluteURLStringBuffer(
            allocator,
            relativeString,
            relativeFlags,
            relativeRanges,
            baseString,
            baseFlags,
            baseRanges,
            buffer
        );
    }
    UniChar *buffer =
        malloc_type_malloc((size_t)length * sizeof(UniChar), (malloc_type_id_t)UINT64_C(0x1000040BDFB0063));
    if (buffer == NULL) {
        return NULL;
    }
    OCFStringRef result = resolveAbsoluteURLStringBuffer(
        allocator, relativeString, relativeFlags, relativeRanges, baseString, baseFlags, baseRanges, buffer
    );
    free(buffer);
    return result;
}

// Native _resolvedPath, 0x1c024357c. Non-external. Compacts "." and ".." components of the
// NUL-terminated characters [pathStr, end) in place and answers a no-copy string that the allocator frees.
// A final "." is cut (unless it is the whole path); "./" is removed unless it is the whole path; "../" or a
// final ".." removes the component before it when at least two characters precede it and that component
// is not "..", the whole path becoming "./" when it was the only component; a leading ".." is removed only
// with stripLeadingDotDots and when more than two characters follow it. The moves keep the terminator; the
// ".." moves count in 32-bit characters. A trailing delimiter is dropped on request when it is not the only
// character. The character count is 0 when end precedes pathStr.
OCFStringRef _resolvedPath(
    UniChar *pathStr,
    UniChar *end,
    UniChar pathDelimiter,
    Boolean stripLeadingDotDots,
    Boolean stripTrailingDelimiter,
    OCFAllocatorRef allocator
) {
    UniChar *idx = pathStr;
    while (idx < end) {
        if (*idx == '.') {
            if (idx + 1 == end) {
                if (idx != pathStr) {
                    *idx = '\0';
                    end = idx;
                }
                break;
            } else if (idx[1] == pathDelimiter) {
                if (idx == pathStr && idx + 2 == end) {
                    break;
                }
                memmove(idx, idx + 2, (size_t)(end - (idx + 2) + 1) * sizeof(UniChar));
                end -= 2;
                continue;
            } else if (
                end - idx >= 2 && idx[1] == '.' &&
                (idx + 2 == end || (end - idx != 2 && idx[2] == pathDelimiter))
            ) {
                if (idx - pathStr >= 2) {
                    UniChar *lastDelim = idx - 2;
                    while (lastDelim >= pathStr && *lastDelim != pathDelimiter) {
                        lastDelim--;
                    }
                    lastDelim++;
                    if (lastDelim != idx &&
                        (idx - lastDelim != 3 || lastDelim[0] != '.' || lastDelim[1] != '.')) {
                        if (idx + 2 != end) {
                            unsigned numCharsToMove = (unsigned)(end - (idx + 3) + 1);
                            memmove(lastDelim, idx + 3, numCharsToMove * sizeof(UniChar));
                            end -= (idx + 3 - lastDelim);
                            idx = lastDelim;
                            continue;
                        } else if (lastDelim != pathStr) {
                            *lastDelim = '\0';
                            end = lastDelim;
                            break;
                        } else {
                            pathStr[0] = '.';
                            pathStr[1] = '/';
                            pathStr[2] = '\0';
                            end = &pathStr[3];
                            break;
                        }
                    }
                } else if (stripLeadingDotDots) {
                    if (idx + 3 >= end) {
                        break;
                    }
                    unsigned numCharsToMove = (unsigned)(end - (idx + 3) + 1);
                    memmove(idx, idx + 3, numCharsToMove * sizeof(UniChar));
                    end -= 3;
                    continue;
                }
            }
        }
        while (idx < end && *idx != pathDelimiter) {
            idx++;
        }
        idx++;
    }
    if (stripTrailingDelimiter && end > pathStr && end - 1 != pathStr && end[-1] == pathDelimiter) {
        end--;
    }
    return OCFStringCreateWithCharactersNoCopy(
        allocator, pathStr, end < pathStr ? 0 : end - pathStr, allocator
    );
}

// Native CFURLIsFileReferenceURL, 0x1c02437d0. External.
// The base URLs are followed to the end (the feature state asked for each). An Objective-C URL answers
// -isFileReferenceURL; otherwise flag bit 13 (Swift layout) or flags & 0xe0002000 == 0x60002000 (a file
// scheme with the file-reference bit, CF layout).
// Folds the compiler-outlined piece _CFURLIsFileReferenceURL.cold.1 (0x1c030d494).
Boolean OCFURLIsFileReferenceURL(OCFURLRef url) {
    bool swift;
    for (;;) {
        swift = _foundation_swift_nsurl_feature_enabled();
        OCFURLRef base = OCFURLGetBaseURL(url);
        if (base == NULL) {
            break;
        }
        url = base;
    }
    if (isObjCURL(url)) {
        return ((Boolean (*)(OCFURLRef, SEL))objc_msgSend)(url, sel_getUid("isFileReferenceURL"));
    }
    if (swift) {
        return (url->flags >> 13) & 1;
    }
    return (url->flags & 0xe0002000) == 0x60002000;
}

// Native CFURLHasDirectoryPath, 0x1c0243890. External.
// An Objective-C URL is replaced by its -_cfurl. A URL with a base defers to the base while its own flags
// show no path (Swift layout: bit 5 clear; CF layout: bits 1..5 clear); the answer is bit 14 (Swift) or bit
// 11 (CF) of the last URL visited that has no base or a path.
// Folds the compiler-outlined piece _CFURLHasDirectoryPath.cold.1 (0x1c030d498).
Boolean OCFURLHasDirectoryPath(OCFURLRef anURL) {
    for (;;) {
        if (_foundation_swift_nsurl_feature_enabled()) {
            if (isObjCURL(anURL)) {
                anURL = sendURL(anURL, "_cfurl");
            }
            OCFURLRef base = anURL->baseURL;
            UInt32 flags = anURL->flags;
            if (base == NULL || (flags & 0x20) != 0) {
                return (flags >> 14) & 1;
            }
            anURL = base;
        } else {
            if (isObjCURL(anURL)) {
                anURL = sendURL(anURL, "_cfurl");
            }
            OCFURLRef base = anURL->baseURL;
            UInt32 flags = anURL->flags;
            if (base == NULL || (flags & 0x3e) != 0) {
                return (flags >> 11) & 1;
            }
            anURL = base;
        }
    }
}

// Native CFURLCreateFilePathURL, 0x1c0243960. External.
// A URL without a file scheme fails with kCFErrorDomainCocoa 262 (logging "CFURLCreateFilePathURL failed
// because it was passed a URL which has no scheme" at level 4 when it has no scheme at all); a URL whose
// absolute URL or POSIX path cannot be made fails with code 4. The new string is "file://", the net
// location, the percent-escaped UTF-8 path (CreateStringFromFileSystemRepresentationByAddingPercentEscapes;
// the C string in a 1,024-byte stack buffer below 1,024 maximum bytes, otherwise in a malloc_type_malloc
// block, type 0x100004077774924), a '/' for a directory URL whose path is not "/", and the resource
// specifier, built in an inlined append buffer. Nonzero query resolve flags are reinserted with
// -[NSString _urlStringByInsertingPathResolveFlags:]. Errors are created with kCFAllocatorDefault.
OCFURLRef OCFURLCreateFilePathURL(OCFAllocatorRef allocator, OCFURLRef url, OCFErrorRef *error) {
    Boolean hasScheme = false;
    if ((_CFURLHasFileURLScheme(url, &hasScheme) & 1) == 0) {
        if (!hasScheme) {
            OCFLog(
                4, OF_STR("CFURLCreateFilePathURL failed because it was passed a URL which has no scheme")
            );
        }
        if (error != NULL) {
            *error = OCFErrorCreate(kOCFAllocatorDefault, kOCFErrorDomainCocoa, 0x106, NULL);
        }
        return NULL;
    }
    OCFStringRef netLocation;
    OCFStringRef resourceSpecifier;
    OCFURLRef absolute = OCFURLCopyAbsoluteURL(url);
    OCFURLRef result;
    if (absolute == NULL) {
        netLocation = NULL;
        resourceSpecifier = NULL;
        goto failed;
    }
    OCFStringRef path = OCFURLCreateStringWithFileSystemPath(
        OCFGetAllocator(absolute), absolute, kOCFURLPOSIXPathStyle, false
    );
    netLocation = OCFURLCopyNetLocation(absolute);
    resourceSpecifier = OCFURLCopyResourceSpecifier(absolute);
    uint32_t resolveFlags = _CFURLGetQueryResolveFlags(absolute);
    OCFRelease(absolute);
    if (path == NULL) {
        goto failed;
    }
    OCFIndex pathLength = OCFStringGetLength(path);
    OCFIndex maximum =
        OCFStringGetMaximumSizeForEncoding((uint64_t)pathLength > 1 ? pathLength : 1, kOCFStringEncodingUTF8);

    union {
        char bytes[1024];
        AppendBuffer append;
    } stack;

    char *buffer;
    if (maximum >= 0x400) {
        buffer = malloc_type_malloc((size_t)(maximum + 1), (malloc_type_id_t)UINT64_C(0x100004077774924));
    } else {
        buffer = stack.bytes;
    }
    OCFStringRef escapedPath;
    if (OCFStringGetCString(path, buffer, maximum + 1, kOCFStringEncodingUTF8)) {
        escapedPath = CreateStringFromFileSystemRepresentationByAddingPercentEscapes(
            kOCFAllocatorDefault, (const UInt8 *)buffer, (OCFIndex)strlen(buffer), false, false, false, NULL
        );
    } else {
        escapedPath = NULL;
    }
    if (buffer != stack.bytes) {
        free(buffer);
    }
    AppendBuffer *append = &stack.append;
    appendBufferInit(append, allocator);
    appendBufferAppendString(append, OF_STR("file://"));
    if (netLocation != NULL) {
        appendBufferAppendString(append, netLocation);
    }
    appendBufferAppendString(append, escapedPath);
    if (OCFURLHasDirectoryPath(url) &&
        OCFStringCompare(escapedPath, slashString(), 0) != kOCFCompareEqualTo) {
        appendBufferAppendCharacter(append, '/');
    }
    if (resourceSpecifier != NULL) {
        appendBufferAppendString(append, resourceSpecifier);
    }
    OCFMutableStringRef newString = appendBufferCopyString(append);
    OCFStringRef URLString = newString;
    if (resolveFlags != 0) {
        URLString = ((OCFStringRef (*)(OCFStringRef, SEL, uint32_t))objc_msgSend)(
            newString, sel_getUid("_urlStringByInsertingPathResolveFlags:"), resolveFlags
        );
    }
    result = OCFURLCreateWithString(allocator, URLString, NULL);
    if (newString != NULL) {
        OCFRelease(newString);
    }
    OCFRelease(escapedPath);
    OCFRelease(path);
    goto done;
failed:
    if (error != NULL) {
        *error = OCFErrorCreate(kOCFAllocatorDefault, kOCFErrorDomainCocoa, 4, NULL);
    }
    result = NULL;
done:
    if (netLocation != NULL) {
        OCFRelease(netLocation);
    }
    if (resourceSpecifier != NULL) {
        OCFRelease(resourceSpecifier);
    }
    return result;
}

// Native CFURLGetBaseURL, 0x1c0244224. External. The feature state is computed (both layouts share the
// path): an Objective-C URL answers -baseURL, otherwise the base URL at 0x20.
// Folds the compiler-outlined piece _CFURLGetBaseURL.cold.1 (0x1c030d4ac).
OCFURLRef OCFURLGetBaseURL(OCFURLRef anURL) {
    _foundation_swift_nsurl_feature_enabled();
    if (isObjCURL(anURL)) {
        return sendURL(anURL, "baseURL");
    }
    return anURL->baseURL;
}

// Native __copyOriginalComponent, 0x1c024428c. Non-external. header is a URL's flags word (the string is the
// pointer 8 bytes after it, read without ordering). The substring of the range; with a component flag and a
// substring, +[NSURL __copySwiftDecodedString:encoding:] with the encoding word after the flags when flag
// bit 16 is set, otherwise +[NSURL __copySwiftDecodedString:], replaces it (the substring is released).
OCFStringRef __copyOriginalComponent(
    OCFAllocatorRef allocator, const UInt32 *header, OCFRange range, UInt32 componentFlag
) {
    OCFStringRef string;
    memcpy(&string, (const uint8_t *)header + 8, sizeof(string));
    OCFStringRef substring = OCFStringCreateWithSubstring(allocator, string, range);
    if (componentFlag != 0 && substring != NULL) {
        OCFStringRef decoded;
        if ((header[0] & 0x10000) != 0) {
            decoded = ((OCFStringRef (*)(void *, SEL, OCFStringRef, OCFStringEncoding))objc_msgSend)(
                urlClass, sel_getUid("__copySwiftDecodedString:encoding:"), substring, header[1]
            );
        } else {
            decoded = ((OCFStringRef (*)(void *, SEL, OCFStringRef))objc_msgSend)(
                urlClass, sel_getUid("__copySwiftDecodedString:"), substring
            );
        }
        OCFRelease(substring);
        substring = decoded;
    }
    return substring;
}

// Native _retainedComponentString, 0x1c0244304. Non-external. The component's range (NULL when the flag is
// clear); the scheme of a well-known scheme type (bits 29..31: 1 http, 2 https, 3 file, 4 data, 5 ftp) is
// the retained constant, otherwise the substring of the string (sequentially consistent load). Without
// either option, for a string that needed sanitizing (flag bit 16 clear) whose additional flags (0 without
// additional data) mark the component, the component is rescanned (constructBuffers, scanCharacters with
// the component's allocator and no flags word, the URL's encoding) and the escaped copy, completed from the
// mark (Latin-1 bytes or characters), or the retained component answers; the component is released. With
// removePercentEscapes the result is unescaped with "" left escaped
// (CFURLCreateStringByReplacingPercentEscapes for UTF-8, otherwise the encoding variant) and released.
OCFStringRef _retainedComponentString(
    OCFURLRef url, UInt32 component, Boolean fromOriginalString, Boolean removePercentEscapes
) {
    OCFAllocatorRef allocator = OCFGetAllocator(url);
    UInt32 flags = url->flags;
    OCFRange range;
    if ((flags & component) != 0) {
        range = url->ranges[componentRangeIndex(flags, component)];
    } else {
        range = (OCFRange){kOCFNotFound, 0};
    }
    if (range.location == kOCFNotFound) {
        return NULL;
    }
    OCFStringRef string = NULL;
    if ((component & 1) != 0) {
        switch (flags >> 29) {
            case 1:
                string = OCFRetain(httpString());
                break;
            case 2:
                string = OCFRetain(httpsString());
                break;
            case 3:
                string = OCFRetain(fileString());
                break;
            case 4:
                string = OCFRetain(dataString());
                break;
            case 5:
                string = OCFRetain(ftpString());
                break;
            default:
                break;
        }
    }
    if (string == NULL) {
        string =
            OCFStringCreateWithSubstring(allocator, __atomic_load_n(&url->string, __ATOMIC_SEQ_CST), range);
    }
    if (!removePercentEscapes && !fromOriginalString && string != NULL && (url->flags & 0x10000) == 0) {
        UInt32 additionalFlags = url->extra != NULL ? url->extra->additionalFlags : 0;
        if ((additionalFlags & component) != 0) {
            OCFStringEncoding encoding = url->encoding;
            OCFAllocatorRef stringAllocator = OCFGetAllocator(string);
            OCFIndex length = OCFStringGetLength(string);
            OCFMutableStringRef escapedString = NULL;
            OCFIndex mark = 0;
            __attribute__((aligned(16))) UInt8 buffer[1024];
            const char *cString;
            const UniChar *characters;
            Boolean useCString;
            Boolean freeCharacters;
            constructBuffers(string, buffer, &cString, &characters, &useCString, &freeCharacters);
            scanCharacters(
                stringAllocator,
                &escapedString,
                NULL,
                cString,
                characters,
                useCString,
                0,
                length,
                &mark,
                component,
                encoding
            );
            OCFStringRef result;
            if (escapedString == NULL) {
                OCFRetain(string);
                result = string;
            } else {
                if (length - mark > 0) {
                    if (useCString) {
                        __OCFStringAppendBytes(
                            escapedString, cString + mark, length - mark, kOCFStringEncodingISOLatin1
                        );
                    } else {
                        OCFStringAppendCharacters(escapedString, characters + mark, length - mark);
                    }
                }
                result = escapedString;
            }
            if (freeCharacters) {
                free(useCString ? (void *)cString : (void *)characters);
            }
            OCFRelease(string);
            return result;
        }
    }
    if (removePercentEscapes && string != NULL) {
        OCFStringEncoding encoding = url->encoding;
        OCFStringRef unescaped;
        if (encoding == kOCFStringEncodingUTF8) {
            unescaped = OCFURLCreateStringByReplacingPercentEscapes(allocator, string, emptyString());
        } else {
            unescaped = OCFURLCreateStringByReplacingPercentEscapesUsingEncoding(
                allocator, string, emptyString(), encoding
            );
        }
        OCFRelease(string);
        string = unescaped;
    }
    return string;
}

// Native CFURLCopyNetLocation, 0x1c02445e4. External.
// An Objective-C URL is replaced by its -_cfurl; a URL without a net location defers to its base URL (NULL
// at the end of the chain).
// Swift layout (flag bit 11): the location starts after "//" (2), or after the scheme and "://" when flag
// bit 2 marks a scheme (scheme length 4 for range form 2, the 64-bit length at 0x40 for form 1, the 16-bit
// length at 0x3a otherwise); its end is 0 or 7 for form 2 (by bit 2; the string length is computed and
// unused), the 64-bit value at 0x68 for form 1 and the 16-bit value at 0x44 otherwise. Without a replacement
// string at 0x30 the substring of the string at 0x18 is answered; with one, its substring is extended until
// '#', '/' or '?' or its end.
// CF layout (any of flag bits 1..4): _netLocationRange; an unsanitized string (flag bit 16 clear) with
// additional data whose flags show an escaped authority (bits 1..4) takes the substring of the sanitized
// string up to its first '/' after the location.
// Folds the compiler-outlined piece _CFURLCopyNetLocation.cold.1 (0x1c030d4b0).
OCFStringRef OCFURLCopyNetLocation(OCFURLRef anURL) {
    for (;;) {
        if (_foundation_swift_nsurl_feature_enabled()) {
            if (isObjCURL(anURL)) {
                anURL = sendURL(anURL, "_cfurl");
            }
            UInt32 flags = anURL->flags;
            if ((flags & 0x800) != 0) {
                uint64_t end;
                switch (flags & 3) {
                    case 2:
                        OCFStringGetLength(anURL->string);
                        flags = anURL->flags;
                        end = (flags & 4) != 0 ? 7 : 0;
                        break;
                    case 1:
                        end = swiftField64(anURL, 0x68);
                        break;
                    default:
                        end = swiftField16(anURL, 0x44);
                        break;
                }
                OCFIndex location;
                OCFIndex length;
                if ((flags & 4) == 0) {
                    length = (OCFIndex)end - 2;
                    location = 2;
                } else {
                    uint64_t schemeLength;
                    switch (flags & 3) {
                        case 2:
                            schemeLength = 4;
                            break;
                        case 1:
                            schemeLength = swiftField64(anURL, 0x40);
                            break;
                        default:
                            schemeLength = swiftField16(anURL, 0x3a);
                            break;
                    }
                    location = (OCFIndex)schemeLength + 3;
                    length = (OCFIndex)end - location;
                }
                OCFAllocatorRef allocator = OCFGetAllocator(anURL);
                OCFStringRef replacement = (OCFStringRef)anURL->resourceInfo;
                if (replacement == NULL) {
                    return OCFStringCreateWithSubstring(
                        allocator, anURL->string, (OCFRange){location, length}
                    );
                }
                OCFIndex replacementLength = OCFStringGetLength(replacement);
                for (OCFIndex index = location + length; index < replacementLength; ++index, ++length) {
                    UniChar character = OCFStringGetCharacterAtIndex(replacement, index);
                    if (character <= 0x3f &&
                        ((UINT64_C(1) << character) & UINT64_C(0x8000800800000000)) != 0) {
                        return OCFStringCreateWithSubstring(
                            allocator, replacement, (OCFRange){location, length}
                        );
                    }
                }
                return OCFStringCreateWithSubstring(
                    allocator, replacement, (OCFRange){location, replacementLength - location}
                );
            }
        } else {
            if (isObjCURL(anURL)) {
                anURL = sendURL(anURL, "_cfurl");
            }
            UInt32 flags = anURL->flags;
            if ((flags & 0x1e) != 0) {
                OCFRange range = _netLocationRange(flags, anURL->ranges);
                OCFURLAdditionalData *extra = anURL->extra;
                if ((flags & 0x10000) == 0 && extra != NULL &&
                    ((uint8_t)extra->additionalFlags & 0x1e) != 0) {
                    OCFRange slash = {0, 0};
                    OCFStringRef sanitized = extra->sanitizedString;
                    OCFIndex length = OCFStringGetLength(sanitized) - range.location;
                    if (OCFStringFindWithOptions(
                            sanitized, slashString(), (OCFRange){range.location, length}, 0, &slash
                        )) {
                        length = slash.location - range.location;
                    }
                    return OCFStringCreateWithSubstring(
                        OCFGetAllocator(anURL), sanitized, (OCFRange){range.location, length}
                    );
                }
                return OCFStringCreateWithSubstring(
                    OCFGetAllocator(anURL), __atomic_load_n(&anURL->string, __ATOMIC_SEQ_CST), range
                );
            }
        }
        anURL = anURL->baseURL;
        if (anURL == NULL) {
            return NULL;
        }
    }
}

// Native _netLocationRange, iOS Simulator 24A434, 0x1c0244938, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external. Callers pass the range array of a URL (X19 + 0x38 in
// CFURLCopyNetLocation) or a local array (resolveAbsoluteURLStringBuffer); the function reads only
// that array.
// iOS Device 24A446: 0x180712ef4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// indexes the four collected ranges by a counter and authenticates its return; the results agree.
// Without any of flag bits 1..4 the result is {-1, 0}. Otherwise the ranges of bits 1..4 are collected
// in order, each at the index equal to the number of lower set bits ({-1, 0} for a clear bit; bit 4
// counts bits 0..3 in a loop). The first collected range whose location is not -1 starts the result;
// each later one sets length = location - start + its length, with 64-bit wraparound.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
OCFRange _netLocationRange(uint32_t flags, const OCFRange *ranges) {
    if ((flags & 0x1e) == 0) {
        return (OCFRange){-1, 0};
    }
    OCFRange components[4];
    for (uint32_t component = 0; component < 4; ++component) {
        uint32_t bit = UINT32_C(2) << component;
        if ((flags & bit) == 0) {
            components[component] = (OCFRange){-1, 0};
            continue;
        }
        uint32_t index = 0;
        for (uint32_t lower = 1; lower < bit; lower <<= 1) {
            if (flags & lower) {
                ++index;
            }
        }
        components[component] = ranges[index];
    }
    OCFRange result = {-1, 0};
    for (uint32_t component = 0; component < 4; ++component) {
        if (components[component].location == -1) {
            continue;
        }
        if (result.location == -1) {
            result = components[component];
        } else {
            result.length = (OCFIndex)((uint64_t)components[component].location - (uint64_t)result.location +
                                       (uint64_t)components[component].length);
        }
    }
    return result;
}

// Native CFURLCopyPath, 0x1c0244a84. External. Swift layout: _CFURLCopyPath without removing escapes. CF
// layout: an Objective-C URL is replaced by its -_cfurl, then _retainedComponentString(url, 0x20, false,
// false).
// Folds the compiler-outlined piece _CFURLCopyPath.cold.1 (0x1c030d4b4).
OCFStringRef OCFURLCopyPath(OCFURLRef anURL) {
    if (_foundation_swift_nsurl_feature_enabled()) {
        return _OCFURLCopyPath(anURL, false);
    }
    if (isObjCURL(anURL)) {
        anURL = sendURL(anURL, "_cfurl");
    }
    return _retainedComponentString(anURL, 0x20, false, false);
}

// Native _CFURLCopyPath, 0x1c0244b0c. External.
// An Objective-C URL is replaced by its -_cfurl. Swift layout: NULL without a path (flag bit 5); the path
// range is 7 (or 0 without a scheme, bit 2) to the string's end for range form 2, the two 64-bit words at
// 0x68 for form 1, or the 32-bit pair at 0x44 (location in the low half); removePercentEscapes selects
// __copyOriginalComponent (flag 1), otherwise __copyEncodedComponent (flag 0x200000), both given the
// object's header at 0x10. CF layout: _retainedComponentString(url, 0x20, false, removePercentEscapes).
// Folds the compiler-outlined piece __CFURLCopyPath.cold.1 (0x1c030d4b8).
OCFStringRef _OCFURLCopyPath(OCFURLRef anURL, Boolean removePercentEscapes) {
    if (_foundation_swift_nsurl_feature_enabled()) {
        if (isObjCURL(anURL)) {
            anURL = sendURL(anURL, "_cfurl");
        }
        UInt32 flags = anURL->flags;
        if ((flags & 0x20) == 0) {
            return NULL;
        }
        OCFRange range;
        switch (flags & 3) {
            case 2: {
                OCFIndex length = OCFStringGetLength(anURL->string);
                if ((anURL->flags & 4) != 0) {
                    range = (OCFRange){7, length - 7};
                } else {
                    range = (OCFRange){0, length};
                }
                break;
            }
            case 1:
                range = (OCFRange){(OCFIndex)swiftField64(anURL, 0x68), (OCFIndex)swiftField64(anURL, 0x70)};
                break;
            default: {
                uint32_t packed = swiftField32(anURL, 0x44);
                range = (OCFRange){packed & 0xffff, packed >> 16};
                break;
            }
        }
        OCFAllocatorRef allocator = OCFGetAllocator(anURL);
        if (removePercentEscapes) {
            return __copyOriginalComponent(allocator, &anURL->flags, range, 1);
        }
        return __copyEncodedComponent(allocator, &anURL->flags, range, 0x200000);
    }
    if (isObjCURL(anURL)) {
        anURL = sendURL(anURL, "_cfurl");
    }
    return _retainedComponentString(anURL, 0x20, false, removePercentEscapes);
}

// Native __copyEncodedComponent, 0x1c0244c6c. Non-external. header is a Swift-layout URL's flags word (the
// string is the pointer 8 bytes after it, read without ordering). The substring of the range answers when the
// flags lack the component flag, when the scheme range form (bits 0..1) is 2, or when it is NULL. Otherwise
// the encoding is the word after the flags when flag bit 16 is set, else UTF-8, and the component flag
// selects +[NSURL __copySwiftEncodedUser:encoding:] (0x40000), Password (0x80000), Host (0x100000), Path
// (0x200000), Query (0x400000) or Fragment (0x800000); any other flag answers NULL. The substring is
// released.
OCFStringRef __copyEncodedComponent(
    OCFAllocatorRef allocator, const UInt32 *header, OCFRange range, UInt32 componentFlag
) {
    OCFStringRef string;
    memcpy(&string, (const uint8_t *)header + 8, sizeof(string));
    OCFStringRef substring = OCFStringCreateWithSubstring(allocator, string, range);
    UInt32 flags = header[0];
    if ((flags & componentFlag) == 0 || (flags & 3) == 2 || substring == NULL) {
        return substring;
    }
    OCFStringEncoding encoding = (flags & 0x10000) != 0 ? header[1] : kOCFStringEncodingUTF8;
    const char *selector;
    switch (componentFlag) {
        case 0x40000:
            selector = "__copySwiftEncodedUser:encoding:";
            break;
        case 0x80000:
            selector = "__copySwiftEncodedPassword:encoding:";
            break;
        case 0x100000:
            selector = "__copySwiftEncodedHost:encoding:";
            break;
        case 0x200000:
            selector = "__copySwiftEncodedPath:encoding:";
            break;
        case 0x400000:
            selector = "__copySwiftEncodedQuery:encoding:";
            break;
        case 0x800000:
            selector = "__copySwiftEncodedFragment:encoding:";
            break;
        default:
            selector = NULL;
            break;
    }
    OCFStringRef result = NULL;
    if (selector != NULL) {
        result = ((OCFStringRef (*)(void *, SEL, OCFStringRef, OCFStringEncoding))objc_msgSend)(
            urlClass, sel_getUid(selector), substring, encoding
        );
    }
    OCFRelease(substring);
    return result;
}

// Native CFURLCopyResourceSpecifier, 0x1c0244e34. External.
// An Objective-C URL is replaced by its -_cfurl.
// Swift layout: a decomposable URL (flag bit 10) answers NULL without a parameter, query or fragment (bits 6
// and 7); otherwise the rest of the string from the end of the path (see _CFURLCopyPath), or, with a
// replacement string at 0x30 when bits 22..23 are set, its substring from the first '?' or '#' at or after
// the path end (NULL without one). A URL that cannot be decomposed answers everything after the scheme and
// its
// ':' (of the replacement string when present); one without a scheme fails the assertion of
// __getSchemeRangeUnchecked (_NSSwiftURLShims.h:211, the .cold.2 fragment).
// CF layout: a decomposable URL (flag bit 14) answers, from the delimiter before its first parameter, query
// or fragment component (the lowest of bits 6..8; NULL without one), the rest of the string; an unsanitized
// string whose escaped components start at or after it uses the sanitized string, re-parsed with the base
// URL when an earlier component was escaped too. Otherwise everything after the scheme and its ':' (the
// sanitized string when present).
// Folds the compiler-outlined pieces _CFURLCopyResourceSpecifier.cold.1 (0x1c030d4bc),
// _CFURLCopyResourceSpecifier.cold.2 (0x1c030d4c0).
OCFStringRef OCFURLCopyResourceSpecifier(OCFURLRef anURL) {
    if (_foundation_swift_nsurl_feature_enabled()) {
        if (isObjCURL(anURL)) {
            anURL = sendURL(anURL, "_cfurl");
        }
        UInt32 flags = anURL->flags;
        OCFAllocatorRef allocator = OCFGetAllocator(anURL);
        if ((flags & 0x400) != 0) {
            if ((flags & 0xc0) == 0) {
                return NULL;
            }
            OCFIndex location;
            OCFIndex length;
            switch (anURL->flags & 3) {
                case 2: {
                    OCFIndex stringLength = OCFStringGetLength(anURL->string);
                    if ((anURL->flags & 4) != 0) {
                        location = 7;
                        length = stringLength - 7;
                    } else {
                        location = 0;
                        length = stringLength;
                    }
                    break;
                }
                case 1:
                    location = (OCFIndex)swiftField64(anURL, 0x68);
                    length = (OCFIndex)swiftField64(anURL, 0x70);
                    break;
                default: {
                    uint32_t packed = swiftField32(anURL, 0x44);
                    length = packed >> 16;
                    location = packed & 0xffff;
                    break;
                }
            }
            OCFIndex end = length + location;
            if ((flags & 0xc00000) == 0) {
                OCFIndex stringLength = OCFStringGetLength(anURL->string);
                return OCFStringCreateWithSubstring(
                    allocator, anURL->string, (OCFRange){end, stringLength - end}
                );
            }
            OCFStringRef replacement = (OCFStringRef)anURL->resourceInfo;
            if (replacement == NULL) {
                return NULL;
            }
            OCFIndex replacementLength = OCFStringGetLength(replacement);
            OCFIndex remaining = replacementLength - end;
            if (remaining <= 0) {
                return NULL;
            }
            do {
                UniChar character = OCFStringGetCharacterAtIndex(replacement, end);
                if (character == '?' || character == '#') {
                    return OCFStringCreateWithSubstring(allocator, replacement, (OCFRange){end, remaining});
                }
                end += 1;
                remaining -= 1;
            } while (end < replacementLength);
            return NULL;
        }
        flags = anURL->flags;
        if ((flags & 4) == 0) {
            __assert_rtn(
                "__getSchemeRangeUnchecked", "_NSSwiftURLShims.h", 0xd3, "header->_flags & __CFURLHasScheme"
            );
        }
        OCFIndex schemeLength;
        switch (flags & 3) {
            case 2:
                schemeLength = 4;
                break;
            case 1:
                schemeLength = (OCFIndex)swiftField64(anURL, 0x40);
                break;
            default:
                schemeLength = swiftField16(anURL, 0x3a);
                break;
        }
        OCFStringRef string = (OCFStringRef)anURL->resourceInfo;
        if (string == NULL) {
            string = anURL->string;
        }
        OCFIndex stringLength = OCFStringGetLength(string);
        return OCFStringCreateWithSubstring(
            allocator, string, (OCFRange){schemeLength + 1, stringLength - (schemeLength + 1)}
        );
    }
    if (isObjCURL(anURL)) {
        anURL = sendURL(anURL, "_cfurl");
    }
    UInt32 flags = anURL->flags;
    if ((flags & 0x4000) != 0) {
        UInt32 firstBit = 0;
        for (UInt32 bit = 0x100; bit != 0x20; bit >>= 1) {
            if ((bit & flags) != 0) {
                firstBit = bit;
            }
        }
        if (firstBit == 0) {
            return NULL;
        }
        OCFAllocatorRef allocator = OCFGetAllocator(anURL);
        UInt32 additionalFlags;
        OCFStringRef sanitized;
        if (anURL->extra != NULL) {
            additionalFlags = anURL->extra->additionalFlags;
            sanitized = anURL->extra->sanitizedString;
        } else {
            additionalFlags = 0;
            sanitized = NULL;
        }
        flags = anURL->flags;
        bool escapedFromHere = false;
        if ((flags & 0x10000) == 0) {
            for (UInt32 bit = firstBit; bit != 0x200; bit <<= 1) {
                if ((bit & additionalFlags) != 0) {
                    escapedFromHere = true;
                    break;
                }
            }
        }
        if (!escapedFromHere) {
            OCFIndex start;
            if ((flags & firstBit) == 0) {
                start = -2;
            } else {
                start = anURL->ranges[componentRangeIndex(flags, firstBit)].location - 1;
            }
            OCFIndex length = OCFStringGetLength(__atomic_load_n(&anURL->string, __ATOMIC_SEQ_CST));
            return OCFStringCreateWithSubstring(
                allocator,
                __atomic_load_n(&anURL->string, __ATOMIC_SEQ_CST),
                (OCFRange){start, length - start}
            );
        }
        bool escapedBefore = false;
        for (UInt32 bit = firstBit; bit >= 2;) {
            UInt32 lower = bit >> 1;
            bit = lower;
            if ((additionalFlags & lower) != 0) {
                escapedBefore = true;
                break;
            }
        }
        if (escapedBefore) {
            UInt32 sanitizedFlags = 0;
            OCFRange sanitizedRanges[9];
            uint8_t numberOfRanges;
            _parseComponents(sanitized, anURL->baseURL, &sanitizedFlags, sanitizedRanges, &numberOfRanges);
            OCFIndex start;
            if ((sanitizedFlags & firstBit) == 0) {
                start = -2;
            } else {
                start = sanitizedRanges[componentRangeIndex(sanitizedFlags, firstBit)].location - 1;
            }
            OCFIndex length = OCFStringGetLength(sanitized);
            return OCFStringCreateWithSubstring(
                OCFGetAllocator(anURL), sanitized, (OCFRange){start, length - start}
            );
        }
        OCFIndex start;
        if ((flags & firstBit) == 0) {
            start = -2;
        } else {
            start = anURL->ranges[componentRangeIndex(flags, firstBit)].location - 1;
        }
        OCFIndex length = OCFStringGetLength(sanitized);
        return OCFStringCreateWithSubstring(allocator, sanitized, (OCFRange){start, length - start});
    }
    // Without a scheme the not-found range {-1, 0} gives the start 0.
    OCFIndex start;
    if ((flags & 1) != 0) {
        start = anURL->ranges[0].length + anURL->ranges[0].location + 1;
    } else {
        start = 0;
    }
    if (anURL->extra != NULL && anURL->extra->sanitizedString != NULL) {
        OCFStringRef sanitized = anURL->extra->sanitizedString;
        OCFAllocatorRef allocator = OCFGetAllocator(anURL);
        OCFIndex length = OCFStringGetLength(sanitized);
        return OCFStringCreateWithSubstring(allocator, sanitized, (OCFRange){start, length - start});
    }
    OCFAllocatorRef allocator = OCFGetAllocator(anURL);
    OCFStringRef string = __atomic_load_n(&anURL->string, __ATOMIC_SEQ_CST);
    OCFIndex length = OCFStringGetLength(__atomic_load_n(&anURL->string, __ATOMIC_SEQ_CST));
    return OCFStringCreateWithSubstring(allocator, string, (OCFRange){start, length - start});
}

// Native CFURLCopyQueryString, 0x1c0245cc0. External.
// Swift layout: an Objective-C URL answers its retained -query when charactersToLeaveEscaped or the query is
// NULL; otherwise it is replaced by its -_cfurl. A decomposable URL (flag bit 10) with a query (bit 6) copies
// the encoded query (__copyEncodedComponent, flag 0x400000; the range is the 32-bit pair at 0x48 for form 0,
// the two words at 0x78 for forms 1 and 3, and form 2 fails the assertion of __getQueryRangeUnchecked,
// _NSSwiftURLShims.h:284, the .cold.2 fragment through OUTLINED_FUNCTION_3_10); without
// charactersToLeaveEscaped it is answered as it is, otherwise unescaped (with the stored encoding when flag
// bit 16 is set) and released. A URL with a path but no query (bit 5) answers NULL; one without either
// defers to its base URL. CF layout: _unescapedQueryString unescaped with the URL's encoding (the UTF-8
// function for UTF-8), then released.
// CoreFoundation defect: 0x1c0245d28 drops the retained -query of an Objective-C URL when
// charactersToLeaveEscaped and the query are both present (the register is overwritten before any release).
// The restoration releases it (objc_release) before continuing with -_cfurl; confidence: verified.
// Folds the compiler-outlined pieces _CFURLCopyQueryString.cold.1 (0x1c030d640), _CFURLCopyQueryString.cold.2
// (0x1c030d644).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_3_10 (0x1c024f74c).
OCFStringRef OCFURLCopyQueryString(OCFURLRef anURL, OCFStringRef charactersToLeaveEscaped) {
    for (;;) {
        if (_foundation_swift_nsurl_feature_enabled()) {
            if (isObjCURL(anURL)) {
                OCFStringRef query = (OCFStringRef)objc_retain((id)sendString(anURL, "query"));
                if (charactersToLeaveEscaped == NULL || query == NULL) {
                    return query;
                }
                objc_release((id)query);
                if (isObjCURL(anURL)) {
                    anURL = sendURL(anURL, "_cfurl");
                }
            }
            UInt32 flags = anURL->flags;
            if ((flags & 0x400) == 0) {
                return NULL;
            }
            if ((flags & 0x40) != 0) {
                OCFRange range;
                switch (flags & 3) {
                    case 0: {
                        uint32_t packed = swiftField32(anURL, 0x48);
                        range = (OCFRange){packed & 0xffff, packed >> 16};
                        break;
                    }
                    case 2:
                        __assert_rtn(
                            "__getQueryRangeUnchecked",
                            "_NSSwiftURLShims.h",
                            0x11c,
                            "!__isFileURLImpl(header)"
                        );
                    default:
                        range = (OCFRange){
                            (OCFIndex)swiftField64(anURL, 0x78), (OCFIndex)swiftField64(anURL, 0x80)
                        };
                        break;
                }
                OCFStringRef query =
                    __copyEncodedComponent(OCFGetAllocator(anURL), &anURL->flags, range, 0x400000);
                if (charactersToLeaveEscaped == NULL || query == NULL) {
                    return query;
                }
                OCFStringRef result;
                if ((flags & 0x10000) != 0) {
                    result = __createStringByReplacingPercentEscapesUsingEncoding(
                        query, charactersToLeaveEscaped, anURL->encoding
                    );
                } else {
                    result = __createStringByReplacingPercentEscapes(query, charactersToLeaveEscaped);
                }
                OCFRelease(query);
                return result;
            }
            if ((flags & 0x20) != 0) {
                return NULL;
            }
            anURL = anURL->baseURL;
            if (anURL == NULL) {
                return NULL;
            }
            continue;
        }
        OCFStringRef query = _unescapedQueryString(anURL);
        if (query == NULL) {
            return NULL;
        }
        OCFStringEncoding encoding = anURL->encoding;
        OCFAllocatorRef allocator = OCFGetAllocator(anURL);
        OCFStringRef result;
        if (encoding == kOCFStringEncodingUTF8) {
            result = OCFURLCreateStringByReplacingPercentEscapes(allocator, query, charactersToLeaveEscaped);
        } else {
            result = OCFURLCreateStringByReplacingPercentEscapesUsingEncoding(
                allocator, query, charactersToLeaveEscaped, anURL->encoding
            );
        }
        OCFRelease(query);
        return result;
    }
}

// Native __createStringByReplacingPercentEscapesUsingEncoding, 0x1c0245e60. Non-external. Tail calls
// +[NSURL __copySwiftDecodedString:encoding:] when charactersToLeaveEscaped is empty, otherwise
// +[NSURL __copySwiftDecodedString:encoding:charactersToLeaveEscaped:].
OCFStringRef __createStringByReplacingPercentEscapesUsingEncoding(
    OCFStringRef string, OCFStringRef charactersToLeaveEscaped, OCFStringEncoding encoding
) {
    if (OCFStringGetLength(charactersToLeaveEscaped) == 0) {
        return ((OCFStringRef (*)(void *, SEL, OCFStringRef, OCFStringEncoding))objc_msgSend)(
            urlClass, sel_getUid("__copySwiftDecodedString:encoding:"), string, encoding
        );
    }
    return ((OCFStringRef (*)(void *, SEL, OCFStringRef, OCFStringEncoding, OCFStringRef))objc_msgSend)(
        urlClass,
        sel_getUid("__copySwiftDecodedString:encoding:charactersToLeaveEscaped:"),
        string,
        encoding,
        charactersToLeaveEscaped
    );
}

// Native __createStringByReplacingPercentEscapes, 0x1c0245ec0. Non-external. Tail calls
// +[NSURL __copySwiftDecodedString:] when charactersToLeaveEscaped is empty, otherwise
// +[NSURL __copySwiftDecodedString:charactersToLeaveEscaped:].
OCFStringRef
__createStringByReplacingPercentEscapes(OCFStringRef string, OCFStringRef charactersToLeaveEscaped) {
    if (OCFStringGetLength(charactersToLeaveEscaped) == 0) {
        return ((OCFStringRef (*)(void *, SEL, OCFStringRef))objc_msgSend)(
            urlClass, sel_getUid("__copySwiftDecodedString:"), string
        );
    }
    return ((OCFStringRef (*)(void *, SEL, OCFStringRef, OCFStringRef))objc_msgSend)(
        urlClass,
        sel_getUid("__copySwiftDecodedString:charactersToLeaveEscaped:"),
        string,
        charactersToLeaveEscaped
    );
}

// Native _unescapedQueryString, 0x1c0245f0c. Non-external. A NULL URL fails the assertion "anURL"
// (CFURL.c:4689, the .cold.1 fragment). A tagged or Objective-C URL answers its retained -query. Otherwise
// the query component without unescaping; when absent, a URL whose flags have bit 14 set and none of bits
// 0..5 defers to its base URL (NULL without one); any other URL answers NULL.
// Folds the compiler-outlined piece __unescapedQueryString.cold.1 (0x1c030d650).
OCFStringRef _unescapedQueryString(OCFURLRef url) {
    for (;;) {
        if (url == NULL) {
            __assert_rtn("_unescapedQueryString", "CFURL.c", 0x1251, "anURL");
        }
        if (isObjCURL(url)) {
            OCFStringRef query = sendString(url, "query");
            if (query != NULL) {
                OCFRetain(query);
            }
            return query;
        }
        OCFStringRef query = _retainedComponentString(url, 0x80, false, false);
        if (query != NULL) {
            return query;
        }
        UInt32 flags = url->flags;
        if ((flags & 0x4000) == 0 || (flags & 0x3f) != 0) {
            return NULL;
        }
        url = url->baseURL;
        if (url == NULL) {
            return NULL;
        }
    }
}

// Native __CFURLResourceInfoPtr, 0x1c0247bbc, with .cold.1 (0x1c030d7ec, the dispatch_once of the NSSwiftURL
// feature through OUTLINED_FUNCTION_0_31) (corpus delta capture 7). External. NULL answers NULL at once. A
// tagged pointer or an Objective-C object of type 29 is replaced by its -_cfurl. With the feature (bit 0)
// the word at 0x28 is answered (acquire load, no NULL test); otherwise NULL for a NULL URL, else the resource
// info at 0x30 (acquire load).
void *__OCFURLResourceInfoPtr(OCFURLRef url) {
    if (url == NULL) {
        return NULL;
    }
    if ((OF_WORD_SHIFT((uintptr_t)url, 63)) != 0 || __CF_IS_OBJC(29, url) != 0) {
        url = ((OCFURLRef (*)(id, SEL))objc_msgSend)((id)url, sel_getUid("_cfurl"));
    }
    uintptr_t offset;
    if ((_foundation_swift_nsurl_feature_enabled() & 1) != 0) {
        offset = 0x28;
    } else {
        if (url == NULL) {
            return NULL;
        }
        offset = 0x30;
    }
    return __atomic_load_n((void *const *)((uintptr_t)url + offset), __ATOMIC_ACQUIRE);
}

// Native _CFURLCreateCurrentDirectoryURL, 0x1c0247d04. External. The current directory in a 1,026-byte stack
// buffer (_CFGetCurrentDirectory) becomes a directory URL without a base; NULL when it cannot be read.
OCFURLRef _OCFURLCreateCurrentDirectoryURL(OCFAllocatorRef allocator) {
    char path[0x402];
    if (!_CFGetCurrentDirectory(path, 0x402)) {
        return NULL;
    }
    return _CFURLCreateWithFileSystemRepresentation(
        allocator, (const UInt8 *)path, (OCFIndex)strlen(path), true, NULL
    );
}

// Native CFURLCreateFromFileSystemRepresentation, 0x1c0247d8c. External; no base URL.
OCFURLRef OCFURLCreateFromFileSystemRepresentation(
    OCFAllocatorRef allocator, const UInt8 *buffer, OCFIndex bufLen, Boolean isDirectory
) {
    return _CFURLCreateWithFileSystemRepresentation(allocator, buffer, bufLen, isDirectory, NULL);
}

// Native CFURLCreateWithFileSystemPath, 0x1c0247d94. External; no base URL, final flag false.
OCFURLRef OCFURLCreateWithFileSystemPath(
    OCFAllocatorRef allocator, OCFStringRef filePath, OCFURLPathStyle pathStyle, Boolean isDirectory
) {
    return _CFURLCreateWithFileSystemPath(allocator, filePath, pathStyle, isDirectory, NULL, false);
}

// The first 40 bytes of a Swift-layout URL after the object header, as +[NSURL
// __swiftParseFilePath:into:pathStyle:isDirectory:] fills them on the stack before they are copied to the
// instance (offset 0x10): the flags, the string, the base URL, an unread word and the replacement string.
// SwiftURLHeader is a local descriptive spelling.
typedef struct {
    UInt32 flags;
    OCFStringEncoding encoding;
    OCFStringRef string;
    OCFURLRef baseURL;
    void *unread0;
    OCFTypeRef replacementString;
} SwiftURLHeader;

OF_ABI_ASSERT(sizeof(SwiftURLHeader) == 0x28, "The copied Swift header is 40 bytes.");

// Native _CFURLCreateWithFileSystemPath, 0x1c0247da0. Non-external. allowFileIDPath is false from the public
// entry points.
// Swift layout: an empty path answers the retained base URL (or NULL). +[NSURL
// __swiftParseFilePath:into:pathStyle:isDirectory:] fills a zeroed stack header (NULL when it fails); an
// instance with 0x28 extra bytes receives it. A "/.file/id=" path (flag bit 13) without allowFileIDPath logs
// the fault "CFURL initialized with \"/.file/id=\" path" (_CFOSLog; the .cold.3 fragment) and releases the
// header's string and replacement string. Unless the parse found a scheme, a base URL is made absolute
// (flag bits 12 and 13 as in _CFURLCreateWithURLString), or flag bit 12 is set and the current directory
// URL becomes the base.
// CF layout: an empty path answers the retained base URL (or NULL). POSIX paths (a "/.file/id=" path
// without allowFileIDPath logs the same fault, .cold.2, and answers NULL), HFS paths (runs of "::" collapse
// parent components in a malloc_type_malloc copy, type 0x1000040BDFB0063; ':' separates components, '/'
// inside a component becomes ':', each component is percent-escaped for ";?/" and the components are joined
// with '/', absolute ones under "file://") and Windows paths (a drive letter with '|' or ':' or a leading
// '\\' is absolute, a leading '/' is a POSIX path) become the URL string. A relative path takes the base
// URL, or the current directory URL (released afterwards). The instance (0x38 extra bytes for a relative
// path, 0x48 for an absolute one) gets UTF-8, a copy of the string (release store) and the absolute base
// URL. A relative path has flags 0x14020 (0x14820 for a directory) and the path range {0, length}; an
// absolute path 0x14021 (0x14821), the file scheme number (0x60000000), bit 12 for a POSIX or slash-led
// Windows path, bit 15 when POSIXPathToURLPath added escapes, bit 13 for a file-ID path, the scheme range
// {0, 4} and the path range {7, length - 7}. A non-directory path of one character is a directory when the
// URL path is "/".
// CoreFoundation defect: 0x1c02484d8 leaks the new instance when the Swift-layout fault path is taken (only
// the header's objects are released). The restoration releases the instance there; confidence: verified.
// Folds the compiler-outlined pieces __CFURLCreateWithFileSystemPath.cold.1 (0x1c030d7f4),
// __CFURLCreateWithFileSystemPath.cold.2 (0x1c030d7f8), __CFURLCreateWithFileSystemPath.cold.3 (0x1c030d81c).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_4_8 (0x1c024f76c), _OUTLINED_FUNCTION_5_8
// (0x1c024f77c).
OCFURLRef _CFURLCreateWithFileSystemPath(
    OCFAllocatorRef allocator,
    OCFStringRef filePath,
    OCFURLPathStyle pathStyle,
    Boolean isDirectory,
    OCFURLRef baseURL,
    Boolean allowFileIDPath
) {
    if (_foundation_swift_nsurl_feature_enabled()) {
        if (OCFStringGetLength(filePath) == 0) {
            if (baseURL != NULL) {
                return OCFRetain(baseURL);
            }
            return NULL;
        }
        SwiftURLHeader header;
        memset(&header, 0, sizeof(header));
        BOOL parsed =
            ((BOOL (*)(void *, SEL, OCFStringRef, SwiftURLHeader *, OCFURLPathStyle, BOOL))objc_msgSend)(
                urlClass,
                sel_getUid("__swiftParseFilePath:into:pathStyle:isDirectory:"),
                filePath,
                &header,
                pathStyle,
                isDirectory != 0
            );
        if (!parsed) {
            return NULL;
        }
        struct __OCFURL *url = (struct __OCFURL *)_OCFRuntimeCreateInstance(allocator, 29, 0x28, NULL);
        if (url != NULL) {
            if (allowFileIDPath || (header.flags & 0x2000) == 0) {
                if (baseURL == NULL) {
                    if ((header.flags & 4) == 0) {
                        header.flags |= 0x1000;
                        header.baseURL = _OCFURLCreateCurrentDirectoryURL(allocator);
                    }
                } else if ((header.flags & 4) == 0) {
                    OCFURLRef absoluteBase = OCFURLCopyAbsoluteURL(baseURL);
                    if (absoluteBase != NULL && _CFURLHasFileURLScheme(absoluteBase, NULL)) {
                        header.flags |= 0x1000;
                        if (OCFURLIsFileReferenceURL(absoluteBase)) {
                            header.flags |= 0x2000;
                        }
                    }
                    header.baseURL = absoluteBase;
                }
                memcpy(&url->flags, &header, sizeof(header));
                return url;
            }
            os_log_fault(_CFOSLog(), "CFURL initialized with \"/.file/id=\" path");
        }
        if (header.string != NULL) {
            OCFRelease(header.string);
        }
        if (header.replacementString != NULL) {
            OCFRelease(header.replacementString);
        }
        if (url != NULL) {
            OCFRelease(url);
        }
        return NULL;
    }
    Boolean addedPercentEncoding = false;
    OCFIndex length = OCFStringGetLength(filePath);
    if (length < 1) {
        if (baseURL == NULL) {
            return NULL;
        }
        return OCFRetain(baseURL);
    }
    OCFStringRef urlPath;
    bool isRelative;
    bool notFileIDPath;
    uint32_t pathKind;
    OCFIndex pathLength;
    switch (pathStyle) {
        case kOCFURLWindowsPathStyle: {
            UniChar first = OCFStringGetCharacterAtIndex(filePath, 0);
            UniChar second = length == 1 ? 0 : OCFStringGetCharacterAtIndex(filePath, 1);
            if ((first <= 0x7f && (sURLValidCharacters[first] & 2) != 0 &&
                 (second == '|' || second == ':')) ||
                first == '\\') {
                pathLength = length;
                notFileIDPath = true;
                urlPath = WindowsPathToURLPath(filePath, allocator, isDirectory, true);
                isRelative = false;
                pathKind = 2;
            } else if (first == '/') {
                pathLength = length;
                notFileIDPath = true;
                urlPath = POSIXPathToURLPath(filePath, isDirectory, true, &addedPercentEncoding);
                isRelative = false;
                pathKind = 0;
            } else {
                urlPath = WindowsPathToURLPath(filePath, allocator, isDirectory, false);
                if (urlPath == NULL) {
                    return NULL;
                }
                pathLength = length;
                notFileIDPath = true;
                pathKind = 2;
                isRelative = true;
            }
            break;
        }
        case kOCFURLHFSPathStyle: {
            UniChar originalFirst = OCFStringGetCharacterAtIndex(filePath, 0);
            Boolean collapsed =
                OCFStringFindWithOptions(filePath, OF_STR("::"), (OCFRange){0, length}, 0, NULL);
            if (collapsed) {
                UniChar *characters =
                    malloc_type_malloc((size_t)length * 2, (malloc_type_id_t)UINT64_C(0x1000040BDFB0063));
                OCFStringGetCharacters(filePath, (OCFRange){0, length}, characters);
                OCFIndex read = 0;
                OCFIndex count = 0;
                OCFIndex firstColon = -1;
                do {
                    UniChar character = characters[read];
                    if (character == ':') {
                        OCFIndex next = read + 1;
                        if (next < length && characters[next] == ':') {
                            if (count >= 1 && firstColon >= 0) {
                                OCFIndex floor = (uint64_t)firstColon < (uint64_t)count ? firstColon : count;
                                if (count < 2) {
                                    count = 0;
                                } else {
                                    OCFIndex stop = floor - 1;
                                    for (;;) {
                                        if (count <= firstColon) {
                                            count = stop;
                                            break;
                                        }
                                        OCFIndex previous = count - 1;
                                        if (characters[count - 1] == ':') {
                                            count = previous;
                                            break;
                                        }
                                        count = previous;
                                        if (previous + 1 < 3) {
                                            count = 0;
                                            break;
                                        }
                                    }
                                }
                            }
                        } else {
                            next = read;
                        }
                        if (firstColon == -1) {
                            firstColon = count;
                        }
                        character = characters[next];
                        read = next;
                    }
                    characters[count++] = character;
                    read += 1;
                } while (read < length);
                filePath = OCFStringCreateWithCharacters(allocator, characters, count);
                pathLength = OCFStringGetLength(filePath);
                free(characters);
            } else {
                pathLength = length;
            }
            OCFArrayRef separated = OCFStringCreateArrayBySeparatingStrings(allocator, filePath, OF_STR(":"));
            OCFMutableArrayRef components = OCFArrayCreateMutableCopy(allocator, 0, separated);
            UniChar first = OCFStringGetCharacterAtIndex(filePath, 0);
            OCFRelease(separated);
            OCFStringRef collapsedPath = filePath;
            if (first == ':') {
                OCFArrayRemoveValueAtIndex(components, 0);
            } else {
                OCFStringRef component = OCFArrayGetValueAtIndex(components, 0);
                if (OCFStringGetLength(component) == 1 && OCFStringGetCharacterAtIndex(component, 0) == '/') {
                    OCFArrayRemoveValueAtIndex(components, 0);
                }
                OCFArrayInsertValueAtIndex(components, 0, OF_STR(""));
            }
            OCFIndex componentCount = OCFArrayGetCount(components);
            uint32_t componentCount32 = (uint32_t)componentCount;
            if (componentCount32 != 0) {
                for (uint64_t index = 0; index != componentCount32; ++index) {
                    OCFStringRef component = OCFArrayGetValueAtIndex(components, (OCFIndex)index);
                    OCFRange slash = {0, 0};
                    OCFIndex remaining = OCFStringGetLength(component);
                    if (OCFStringFindWithOptions(
                            component, slashString(), (OCFRange){0, remaining}, 0, &slash
                        )) {
                        OCFIndex start = 0;
                        OCFMutableStringRef replaced = NULL;
                        do {
                            OCFIndex total = start + remaining;
                            if (replaced == NULL) {
                                replaced = OCFStringCreateMutableCopy(allocator, total, component);
                            }
                            OCFStringReplace(replaced, slash, OF_STR(":"));
                            remaining = total + ~slash.location;
                            start = slash.location + 1;
                        } while (OCFStringFindWithOptions(
                            component, slashString(), (OCFRange){start, remaining}, 0, &slash
                        ));
                        if (replaced != NULL) {
                            OCFArraySetValueAtIndex(components, (OCFIndex)index, replaced);
                            OCFRelease(replaced);
                        }
                    }
                }
                if (isDirectory) {
                    OCFStringRef last = OCFArrayGetValueAtIndex(
                        components, (OCFIndex)(uint32_t)((uint32_t)componentCount - 1)
                    );
                    if (OCFStringGetLength(last) != 0) {
                        OCFArrayAppendValue(components, OF_STR(""));
                    }
                }
            }
            urlPath = NULL;
            if (components != NULL) {
                OCFAllocatorRef arrayAllocator = OCFGetAllocator(components);
                OCFIndex count = OCFArrayGetCount(components);
                OCFMutableArrayRef escaped = NULL;
                bool complete = count == 0;
                for (OCFIndex index = 0; index < count;) {
                    OCFStringRef component = OCFArrayGetValueAtIndex(components, index);
                    OCFStringRef escapedComponent = OCFURLCreateStringByAddingPercentEscapes(
                        arrayAllocator, component, NULL, OF_STR(";?/"), kOCFStringEncodingUTF8
                    );
                    if (escapedComponent == NULL) {
                        break;
                    }
                    if (escapedComponent != component) {
                        if (escaped == NULL) {
                            escaped = OCFArrayCreateMutableCopy(arrayAllocator, count, components);
                        }
                        OCFArraySetValueAtIndex(escaped, index, escapedComponent);
                    }
                    OCFRelease(escapedComponent);
                    index += 1;
                    if (count == index) {
                        complete = true;
                    }
                }
                if (complete) {
                    OCFArrayRef joinedComponents = escaped;
                    if (escaped == NULL) {
                        OCFRetain(components);
                        joinedComponents = components;
                    }
                    OCFRelease(components);
                    OCFStringRef joined;
                    if (OCFArrayGetCount(joinedComponents) == 1 &&
                        OCFStringGetLength(OCFArrayGetValueAtIndex(joinedComponents, 0)) == 0) {
                        joined = OCFRetain(slashString());
                    } else {
                        joined =
                            OCFStringCreateByCombiningStrings(allocator, joinedComponents, slashString());
                    }
                    if (originalFirst != ':' && joined != NULL) {
                        urlPath = OCFStringCreateWithFormat(allocator, NULL, OF_STR("file://%@"), joined);
                        OCFRelease(joined);
                    } else {
                        urlPath = joined;
                    }
                    OCFRelease(joinedComponents);
                } else {
                    if (escaped != NULL) {
                        OCFRelease(escaped);
                    }
                    OCFRelease(components);
                }
            }
            isRelative = originalFirst == ':';
            if (collapsed) {
                OCFRelease(collapsedPath);
            }
            notFileIDPath = true;
            pathKind = 1;
            if (urlPath == NULL) {
                return NULL;
            }
            break;
        }
        case kOCFURLPOSIXPathStyle: {
            pathLength = length;
            UniChar first = OCFStringGetCharacterAtIndex(filePath, 0);
            if (filePath != NULL) {
                Boolean hasPrefix = OCFStringHasPrefix(filePath, OF_STR("/.file/id="));
                notFileIDPath = !hasPrefix;
                if (!allowFileIDPath && hasPrefix) {
                    os_log_fault(_CFOSLog(), "CFURL initialized with \"/.file/id=\" path");
                    return NULL;
                }
            } else {
                notFileIDPath = true;
            }
            isRelative = first != '/';
            urlPath = POSIXPathToURLPath(filePath, isDirectory, first == '/', &addedPercentEncoding);
            pathKind = 0;
            break;
        }
        default:
            return NULL;
    }
    if (urlPath == NULL) {
        return NULL;
    }
    bool releaseBase;
    if (!isRelative) {
        releaseBase = false;
        baseURL = NULL;
    } else if (baseURL == NULL) {
        baseURL = _OCFURLCreateCurrentDirectoryURL(allocator);
        releaseBase = true;
    } else {
        releaseBase = false;
    }
    if (!isDirectory && pathLength == 1) {
        isDirectory = OCFStringGetCharacterAtIndex(urlPath, 0) == '/';
    }
    struct __OCFURL *url =
        (struct __OCFURL *)_OCFRuntimeCreateInstance(allocator, 29, isRelative ? 0x38 : 0x48, NULL);
    if (url != NULL) {
        url->encoding = kOCFStringEncodingUTF8;
        __atomic_store_n(&url->string, OCFStringCreateCopy(allocator, urlPath), __ATOMIC_SEQ_CST);
        url->baseURL = baseURL != NULL ? OCFURLCopyAbsoluteURL(baseURL) : NULL;
        if (isRelative) {
            url->flags = isDirectory ? 0x14820 : 0x14020;
            OCFIndex stringLength = OCFStringGetLength(__atomic_load_n(&url->string, __ATOMIC_SEQ_CST));
            url->ranges[0] = (OCFRange){0, stringLength};
        } else {
            UInt32 flags = isDirectory ? 0x14821 : 0x14021;
            if (notFileIDPath) {
                flags |= (pathKind == 0 ? 0x1000 : 0) | (addedPercentEncoding ? 0x8000 : 0);
            } else {
                flags |= 0x2000;
            }
            url->flags = flags | 0x60000000;
            url->ranges[0] = (OCFRange){0, 4};
            url->ranges[1] = (OCFRange){7, OCFStringGetLength(urlPath) - 7};
        }
    }
    if (baseURL != NULL && releaseBase) {
        OCFRelease(baseURL);
    }
    OCFRelease(urlPath);
    return url;
}

// Native CFURLCopyResourcePropertyForKey, 0x1c0248b70 (corpus delta capture 7). External. A URL without a
// file scheme stores NULL and answers true, logging "... has no scheme" (level 4) when it has no scheme at
// all. Otherwise CoreServicesInternal's _FSURLCopyResourcePropertyForKey (unsupported cut) is asked with the
// resource info: success answers true; a failure without an error stores NULL and answers true; a failure
// with one hands it to the caller (or releases it) and answers false.
Boolean OCFURLCopyResourcePropertyForKey(
    OCFURLRef url, OCFStringRef key, void *propertyValueTypeRefPtr, OCFErrorRef *error
) {
    Boolean hasScheme = false;
    if (!_CFURLHasFileURLScheme(url, &hasScheme)) {
        if (!hasScheme) {
            OCFLog(
                4,
                OF_STR(
                    "CFURLCopyResourcePropertyForKey failed because it was passed a URL which has no scheme"
                )
            );
        }
        *(OCFTypeRef *)propertyValueTypeRefPtr = NULL;
        return true;
    }
    OCFErrorRef underlyingError = NULL;
    if (_FSURLCopyResourcePropertyForKey(
            url, key, propertyValueTypeRefPtr, __OCFURLResourceInfoPtr(url), &underlyingError
        )) {
        return true;
    }
    if (underlyingError == NULL) {
        *(OCFTypeRef *)propertyValueTypeRefPtr = NULL;
        return true;
    }
    if (error != NULL) {
        *error = underlyingError;
    } else {
        OCFRelease(underlyingError);
    }
    return false;
}

// Native CFURLCreateWithFileSystemPathRelativeToBase, 0x1c0248c54. External; final flag false.
OCFURLRef OCFURLCreateWithFileSystemPathRelativeToBase(
    OCFAllocatorRef allocator,
    OCFStringRef filePath,
    OCFURLPathStyle pathStyle,
    Boolean isDirectory,
    OCFURLRef baseURL
) {
    return _CFURLCreateWithFileSystemPath(allocator, filePath, pathStyle, isDirectory, baseURL, false);
}

// Native __copyPOSIXPathForValidFileURLString, inlined by CFURLCopyFileSystemPath and
// CFURLCreateStringWithFileSystemPath (no out-of-line copy in the image; the name comes from its assertion,
// CFURL.c:6741, the callers' .cold.2 fragments through OUTLINED_FUNCTION_7_6). A Swift-layout file URL string
// with flag bit 15 is decoded by +[NSURL __copySwiftPOSIXPathForFileURLString:allocator:]; otherwise the
// string must start with "file://". A string shorter than 9 characters or without a trailing '/' answers
// the substring after "file://"; one not continuing with "/." drops the trailing '/'; the rest is decoded by
// the same NSURL method.
static inline OCFStringRef __copyPOSIXPathForValidFileURLString(OCFAllocatorRef allocator, OCFURLRef url) {
    if ((url->flags & 0x8000) != 0) {
        return ((OCFStringRef (*)(void *, SEL, OCFStringRef, OCFAllocatorRef))objc_msgSend)(
            urlClass, sel_getUid("__copySwiftPOSIXPathForFileURLString:allocator:"), url->string, allocator
        );
    }
    OCFStringRef string = url->string;
    if (!OCFStringHasPrefix(string, OF_STR("file://"))) {
        __assert_rtn(
            "__copyPOSIXPathForValidFileURLString",
            "CFURL.c",
            0x1a55,
            "CFStringHasPrefix(string, CFSTR(\"file://\"))"
        );
    }
    OCFIndex length = OCFStringGetLength(string);
    if (length < 9 || OCFStringGetCharacterAtIndex(string, length - 1) != '/') {
        return OCFStringCreateWithSubstring(allocator, string, (OCFRange){7, length - 7});
    }
    if (OCFStringGetCharacterAtIndex(string, 7) != '/' || OCFStringGetCharacterAtIndex(string, 8) != '.') {
        return OCFStringCreateWithSubstring(allocator, string, (OCFRange){7, length - 8});
    }
    return ((OCFStringRef (*)(void *, SEL, OCFStringRef, OCFAllocatorRef))objc_msgSend)(
        urlClass, sel_getUid("__copySwiftPOSIXPathForFileURLString:allocator:"), string, allocator
    );
}

// Native CFURLCopyFileSystemPath, 0x1c0248c5c. External.
// Swift layout: a CF file URL with a scheme, the file range form and no file-reference bit ((flags & 0x2007)
// == 6) and the POSIX style takes the inlined __copyPOSIXPathForValidFileURLString. CF layout: a POSIX
// request for a CF URL without a base whose flag bit 12 marks a canonical file URL string converts it with
// CanonicalFileURLStringToFileSystemRepresentation (a 1,024-byte stack buffer below 0x401 maximum UTF-8
// bytes, otherwise malloc_type_malloc, type 0x100004077774924). Everything else, and a failed conversion,
// answers CFURLCreateStringWithFileSystemPath without resolving against the base.
// Folds the compiler-outlined pieces _CFURLCopyFileSystemPath.cold.1 (0x1c030d868),
// _CFURLCopyFileSystemPath.cold.2 (0x1c030d86c).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_7_6 (0x1c024f7b0).
OCFStringRef OCFURLCopyFileSystemPath(OCFURLRef anURL, OCFURLPathStyle pathStyle) {
    bool swift = _foundation_swift_nsurl_feature_enabled();
    OCFAllocatorRef allocator = OCFGetAllocator(anURL);
    if (swift) {
        if ((intptr_t)anURL < 0 || pathStyle != kOCFURLPOSIXPathStyle || __CF_IS_OBJC(29, anURL) ||
            (anURL->flags & 0x2007) != 6) {
            return OCFURLCreateStringWithFileSystemPath(allocator, anURL, pathStyle, false);
        }
        return __copyPOSIXPathForValidFileURLString(allocator, anURL);
    }
    if (pathStyle == kOCFURLPOSIXPathStyle) {
        OCFURLRef base = OCFURLGetBaseURL(anURL);
        if ((intptr_t)anURL >= 0 && base == NULL && !__CF_IS_OBJC(29, anURL) &&
            (anURL->flags & 0x1000) != 0) {
            OCFIndex length = OCFStringGetLength(OCFURLGetString(anURL));
            OCFIndex maximum =
                OCFStringGetMaximumSizeForEncoding((uint64_t)length > 1 ? length : 1, kOCFStringEncodingUTF8);
            UInt8 stackBuffer[1024];
            UInt8 *buffer;
            if (maximum >= 0x401) {
                buffer = malloc_type_malloc((size_t)maximum, (malloc_type_id_t)UINT64_C(0x100004077774924));
            } else {
                buffer = stackBuffer;
            }
            OCFStringRef result;
            if (CanonicalFileURLStringToFileSystemRepresentation(
                    __atomic_load_n(&anURL->string, __ATOMIC_SEQ_CST), buffer, maximum
                )) {
                result = OCFStringCreateWithBytes(
                    allocator, buffer, (OCFIndex)strlen((const char *)buffer), kOCFStringEncodingUTF8, false
                );
            } else {
                result = NULL;
            }
            if (buffer != stackBuffer) {
                free(buffer);
            }
            if (result != NULL) {
                return result;
            }
        }
    }
    return OCFURLCreateStringWithFileSystemPath(allocator, anURL, pathStyle, false);
}

// Native CFURLCreateStringWithFileSystemPath, 0x1c0248f94. External.
// Swift layout: an Objective-C URL is replaced by its -_cfurl; a POSIX request for a valid file URL ((flags &
// 0x2007) == 6) takes the inlined __copyPOSIXPathForValidFileURLString. Otherwise the path (CFURLCopyPath; a
// URL without one defers once to its base URL unless resolveAgainstBase is false, else NULL) is resolved
// against the base URL's path when resolving and the path is not absolute: file-reference paths are first
// expanded by +[NSURL __copySwiftFilePathForFileReferencePath:resolveFlags:] (the base's with the base's
// query resolve flags; NULL answers NULL), then +[NSURL __copySwiftResolvedPath:basePath:] combines them; the
// result is converted by +[NSURL __copySwiftFileSystemPathForURLPath:encoding:pathStyle:isFileURL:] (the
// stored encoding when flag bit 16 is set, UTF-8 otherwise; isFileURL is flag bit 12). CF layout: when
// resolving, the base URL's file system path is computed first (without resolving). A POSIX request for a URL
// whose flag bit 15 marks an unescaped path uses _retainedComponentString(url, 0x20, true, true). Otherwise
// the path of the URL (or its -_cfurl) is converted: a file-reference URL without a base has its path
// expanded by FileReferenceURLPathToFullURLPath and its query resolve flags reinserted
// (-[NSString _pathByInsertingResolveFlags:]); POSIX paths through URLPathToPOSIXPath, Windows paths by
// splitting on '/', dropping an empty last component, turning a leading "X|" or "X:" drive component into
// "X:" and joining with '\\' after unescaping (leaving '\\' escaped); HFS and other styles answer the base
// path. A directory path longer than its root loses its trailing '/'. A relative result (not starting with
// '/', not starting with ':' for HFS, not a "\\\\" or drive path for Windows, or empty) is appended to the
// base path (after a slash for a directory base, otherwise after the base path's last slash; for HFS
// replacing it) in a CFAllocatorAllocateTyped buffer (type 0x1000040BDFB0063) that _resolvedPath takes over
// with the separator from _CFGetSlash. When the URL has no path the base path is answered.
// Folds the compiler-outlined pieces _CFURLCreateStringWithFileSystemPath.cold.1 (0x1c030d878),
// _CFURLCreateStringWithFileSystemPath.cold.2 (0x1c030d87c).
OCFStringRef OCFURLCreateStringWithFileSystemPath(
    OCFAllocatorRef allocator, OCFURLRef anURL, OCFURLPathStyle fsType, Boolean resolveAgainstBase
) {
    bool baseFollowed = resolveAgainstBase == 0;
    for (;;) {
        if (!_foundation_swift_nsurl_feature_enabled()) {
            break;
        }
        if (isObjCURL(anURL)) {
            anURL = sendURL(anURL, "_cfurl");
        }
        UInt32 flags = anURL->flags;
        if (fsType == kOCFURLPOSIXPathStyle && (flags & 0x2007) == 6) {
            return __copyPOSIXPathForValidFileURLString(allocator, anURL);
        }
        OCFStringRef path = OCFURLCopyPath(anURL);
        if (path == NULL) {
            if (baseFollowed) {
                return NULL;
            }
            anURL = anURL->baseURL;
            baseFollowed = true;
            if (anURL == NULL) {
                return NULL;
            }
            continue;
        }
        OCFStringRef basePath;
        if (baseFollowed || anURL->baseURL == NULL ||
            (OCFStringGetLength(path) != 0 && OCFStringGetCharacterAtIndex(path, 0) == '/')) {
            basePath = NULL;
        } else {
            basePath = OCFURLCopyPath(anURL->baseURL);
        }
        if ((flags & 0x2000) != 0) {
            if (basePath == NULL) {
                OCFStringRef expanded = ((OCFStringRef (*)(void *, SEL, OCFStringRef, uint32_t))objc_msgSend)(
                    urlClass,
                    sel_getUid("__copySwiftFilePathForFileReferencePath:resolveFlags:"),
                    path,
                    _CFURLGetQueryResolveFlags(anURL)
                );
                OCFRelease(path);
                path = expanded;
                if (path == NULL) {
                    return NULL;
                }
                goto convert;
            }
            uint32_t baseResolveFlags = _CFURLGetQueryResolveFlags(anURL->baseURL);
            OCFStringRef expandedBase = ((OCFStringRef (*)(void *, SEL, OCFStringRef, uint32_t))objc_msgSend)(
                urlClass,
                sel_getUid("__copySwiftFilePathForFileReferencePath:resolveFlags:"),
                basePath,
                baseResolveFlags
            );
            OCFRelease(basePath);
            basePath = expandedBase;
            if (basePath == NULL) {
                OCFRelease(path);
                return NULL;
            }
        } else if (basePath == NULL) {
            goto convert;
        }
        {
            OCFStringRef resolved = ((OCFStringRef (*)(void *, SEL, OCFStringRef, OCFStringRef))objc_msgSend)(
                urlClass, sel_getUid("__copySwiftResolvedPath:basePath:"), path, basePath
            );
            OCFRelease(path);
            OCFRelease(basePath);
            path = resolved;
        }
    convert:;
        OCFStringEncoding encoding = (anURL->flags & 0x10000) != 0 ? anURL->encoding : kOCFStringEncodingUTF8;
        OCFStringRef result = ((
            OCFStringRef (*)(void *, SEL, OCFStringRef, OCFStringEncoding, OCFURLPathStyle, BOOL)
        )objc_msgSend)(
            urlClass,
            sel_getUid("__copySwiftFileSystemPathForURLPath:encoding:pathStyle:isFileURL:"),
            path,
            encoding,
            fsType,
            (flags >> 12) & 1
        );
        OCFRelease(path);
        return result;
    }
    OCFURLRef base;
    OCFStringRef basePath;
    bool noBase;
    if (!baseFollowed && (base = OCFURLGetBaseURL(anURL)) != NULL) {
        basePath = OCFURLCreateStringWithFileSystemPath(allocator, base, fsType, false);
        noBase = false;
    } else {
        base = NULL;
        basePath = NULL;
        noBase = true;
    }
    OCFStringRef converted;
    if ((intptr_t)anURL >= 0) {
        uint8_t objc = __CF_IS_OBJC(29, anURL);
        if (fsType == kOCFURLPOSIXPathStyle && !objc && (anURL->flags & 0x8000) != 0) {
            converted = _retainedComponentString(anURL, 0x20, true, true);
            if (converted != NULL) {
                goto haveConverted;
            }
        }
        if (!__CF_IS_OBJC(29, anURL)) {
            goto haveCFURL;
        }
    }
    anURL = sendURL(anURL, "_cfurl");
haveCFURL:;
    OCFStringRef path = OCFURLCopyPath(anURL);
    OCFStringEncoding encoding = anURL->encoding;
    if (noBase && OCFURLIsFileReferenceURL(anURL)) {
        OCFStringRef fullPath = FileReferenceURLPathToFullURLPath(path, allocator);
        OCFRelease(path);
        uint32_t resolveFlags = _CFURLGetQueryResolveFlags(anURL);
        if (fullPath != NULL && resolveFlags != 0) {
            path = ((OCFStringRef (*)(OCFStringRef, SEL, uint32_t))objc_msgSend)(
                fullPath, sel_getUid("_pathByInsertingResolveFlags:"), resolveFlags
            );
            OCFRetain(path);
            OCFRelease(fullPath);
        } else {
            path = fullPath;
        }
    }
    if (path == NULL) {
        return basePath;
    }
    if (fsType == kOCFURLPOSIXPathStyle) {
        converted = URLPathToPOSIXPath(path, allocator, encoding, _CFURLHasFileURLScheme(anURL, NULL));
    } else if (fsType == kOCFURLWindowsPathStyle) {
        OCFArrayRef separated = OCFStringCreateArrayBySeparatingStrings(allocator, path, slashString());
        OCFIndex count = OCFArrayGetCount(separated);
        OCFMutableArrayRef components =
            OCFArrayCreateMutableCopy(allocator, (OCFIndex)(int32_t)count, separated);
        OCFRelease(separated);
        int32_t last = (int32_t)count - 1;
        if (OCFStringGetLength(OCFArrayGetValueAtIndex(components, last)) == 0) {
            OCFArrayRemoveValueAtIndex(components, last);
            count = (OCFIndex)(uint32_t)last;
        }
        if ((int32_t)count >= 2 && OCFStringGetLength(OCFArrayGetValueAtIndex(components, 0)) == 0) {
            OCFStringRef drive = OCFURLCreateStringByReplacingPercentEscapesUsingEncoding(
                allocator, OCFArrayGetValueAtIndex(components, 1), backslashString(), encoding
            );
            if (drive != NULL) {
                if (OCFStringGetLength(drive) == 2) {
                    UniChar separator = OCFStringGetCharacterAtIndex(drive, 1);
                    if (separator == '|' || separator == ':') {
                        OCFArrayRemoveValueAtIndex(components, 0);
                        if (separator == '|') {
                            OCFStringRef replacement = OCFStringCreateWithFormat(
                                allocator, NULL, OF_STR("%c:"), OCFStringGetCharacterAtIndex(drive, 0)
                            );
                            OCFArraySetValueAtIndex(components, 0, replacement);
                            OCFRelease(replacement);
                        }
                    }
                }
                OCFRelease(drive);
            }
        }
        OCFStringRef joined = OCFStringCreateByCombiningStrings(allocator, components, backslashString());
        OCFRelease(components);
        converted = OCFURLCreateStringByReplacingPercentEscapesUsingEncoding(
            allocator, joined, backslashString(), encoding
        );
        OCFRelease(joined);
    } else {
        OCFRelease(path);
        return basePath;
    }
    OCFRelease(path);
    if (converted == NULL) {
        return basePath;
    }
haveConverted:
    if (OCFURLHasDirectoryPath(anURL)) {
        OCFIndex length = OCFStringGetLength(converted);
        if (length > __rootLength(converted) &&
            OCFStringGetCharacterAtIndex(converted, OCFStringGetLength(converted) - 1) == '/') {
            OCFStringRef trimmed = OCFStringCreateWithSubstring(
                allocator, converted, (OCFRange){0, OCFStringGetLength(converted) - 1}
            );
            OCFRelease(converted);
            converted = trimmed;
            if (converted == NULL) {
                return basePath;
            }
        }
    }
    OCFIndex length = OCFStringGetLength(converted);
    bool isRelative;
    if (fsType == kOCFURLWindowsPathStyle) {
        if (length >= 3) {
            if (OCFStringGetCharacterAtIndex(converted, 0) == '\\' &&
                OCFStringGetCharacterAtIndex(converted, 1) == '\\') {
                isRelative = false;
            } else {
                isRelative = true;
                if (OCFStringGetCharacterAtIndex(converted, 1) == ':') {
                    isRelative = false;
                }
            }
        } else if (length == 2) {
            isRelative = true;
            if (OCFStringGetCharacterAtIndex(converted, 1) == ':') {
                isRelative = false;
            }
        } else {
            isRelative = true;
        }
    } else if (fsType == kOCFURLHFSPathStyle) {
        isRelative = length < 1 || OCFStringGetCharacterAtIndex(converted, 0) == ':';
    } else {
        isRelative = length < 1 || OCFStringGetCharacterAtIndex(converted, 0) != '/';
    }
    if (basePath == NULL || !isRelative) {
        if (basePath != NULL) {
            OCFRelease(basePath);
        }
        return converted;
    }
    Boolean baseIsDirectory = OCFURLHasDirectoryPath(base);
    OCFIndex baseLength = OCFStringGetLength(basePath);
    OCFIndex convertedLength = OCFStringGetLength(converted);
    UniChar slash = _CFGetSlash();
    UniChar *buffer = OCFAllocatorAllocateTyped(
        allocator, (baseLength + convertedLength) * 2 + 4, (OCFAllocatorTypeID)UINT64_C(0x1000040BDFB0063), 0
    );
    OCFStringGetCharacters(basePath, (OCFRange){0, baseLength}, buffer);
    if (baseIsDirectory & 1) {
        if (buffer[baseLength - 1] != slash) {
            buffer[baseLength] = slash;
            baseLength += 1;
        }
    } else {
        OCFIndex offset = baseLength * 2 - 2;
        while ((uint8_t *)buffer + offset > (uint8_t *)buffer && buffer[offset / 2] != slash) {
            offset -= 2;
        }
        baseLength = (offset >> 1) + 1;
    }
    UniChar *destination = buffer + baseLength + (fsType == kOCFURLHFSPathStyle ? -1 : 0);
    OCFStringGetCharacters(converted, (OCFRange){0, convertedLength}, destination);
    destination[convertedLength] = 0;
    OCFStringRef result = _resolvedPath(buffer, destination + convertedLength, slash, false, true, allocator);
    OCFRelease(basePath);
    OCFRelease(converted);
    return result;
}

// Native CanonicalFileURLStringToFileSystemRepresentation, 0x1c02497cc. Non-external. false without a buffer
// or with a zero length. The string after its 7-character "file://" prefix (an empty remainder stores an
// empty C string and answers true) is converted to UTF-8 (CFStringGetBytes, at most three bytes per
// character) in a 3,072-byte stack buffer, or for more than 1,024 characters a malloc_type_malloc block
// (type 0x100004077774924; false when it fails); a failed conversion answers false. The bytes are copied
// with each "%XY" decoded through CanonicalFileURLStringToFileSystemRepresentationHexValues; a '%' without
// two following bytes ends the copy without writing. At most bufferLength bytes are written; input left over
// at that point stores an empty C string and answers false. A trailing '/' is dropped when more than the root
// length was written (11 for "/.nofollow/", the length up to the '/' after "/.resolve/" and at least one
// digit, otherwise 1), and the terminator is stored when it fits; true.
// CoreFoundation defect: kept. The copy loop allows bufferLength bytes without reserving the terminator, so
// output that fills the buffer is answered true without a terminator, and a truncated final escape leaves one
// byte unwritten before the terminator. The restored caller sizes the buffer at three bytes per character of
// the whole string, which no decoded output reaches; the native behavior is preserved.
Boolean
CanonicalFileURLStringToFileSystemRepresentation(OCFStringRef string, UInt8 *buffer, OCFIndex bufferLength) {
    if (buffer == NULL || bufferLength == 0) {
        return false;
    }
    OCFIndex length = OCFStringGetLength(string) - 7;
    if (length == 0) {
        buffer[0] = '\0';
        return true;
    }
    OCFIndex maxBufferLength = length * 3;
    OCFIndex usedBufferLength = 0;
    UInt8 stackBuffer[3072];
    UInt8 *bytes;
    if (length >= 0x401) {
        bytes = malloc_type_malloc((size_t)maxBufferLength, (malloc_type_id_t)UINT64_C(0x100004077774924));
        if (bytes == NULL) {
            return false;
        }
    } else {
        bytes = stackBuffer;
    }
    Boolean result;
    if (OCFStringGetBytes(
            string,
            (OCFRange){7, length},
            kOCFStringEncodingUTF8,
            0,
            false,
            bytes,
            maxBufferLength,
            &usedBufferLength
        ) == 0) {
        result = false;
    } else {
        UInt8 *bufferPtr = buffer;
        Boolean trailingSlash = false;
        Boolean overflowed = false;
        if (usedBufferLength >= 1) {
            const UInt8 *bytePtr = bytes;
            OCFIndex remaining = bufferLength;
            OCFIndex index = 0;
            for (;;) {
                if (*bytePtr == '%') {
                    index += 2;
                    if (index < usedBufferLength) {
                        UInt8 high = CanonicalFileURLStringToFileSystemRepresentationHexValues[bytePtr[1]];
                        UInt8 low = CanonicalFileURLStringToFileSystemRepresentationHexValues[bytePtr[2]];
                        bytePtr += 3;
                        *bufferPtr = (UInt8)(low + (high << 4));
                        trailingSlash = *bufferPtr == '/';
                    }
                } else {
                    *bufferPtr = *bytePtr++;
                    trailingSlash = *bufferPtr == '/';
                }
                ++index;
                ++bufferPtr;
                if (index >= usedBufferLength) {
                    break;
                }
                if (--remaining == 0) {
                    overflowed = true;
                    break;
                }
            }
        }
        if (overflowed) {
            buffer[0] = '\0';
            result = false;
        } else {
            OCFIndex written = bufferPtr - buffer;
            OCFIndex rootLength = 1;
            if (written >= 11) {
                if (memcmp(buffer, "/.nofollow/", 11) == 0) {
                    rootLength = 11;
                } else if (written != 11 && memcmp(buffer, "/.resolve/", 10) == 0) {
                    for (OCFIndex digit = 0;; ++digit) {
                        UInt8 character = buffer[10 + digit];
                        if (character == '/') {
                            rootLength = digit != 0 ? digit + 11 : 1;
                            break;
                        }
                        if ((UInt8)(character - '0') > 9 || written - 11 == digit) {
                            break;
                        }
                    }
                }
            }
            if (written > rootLength && trailingSlash) {
                --bufferPtr;
            }
            if (bufferPtr < buffer + bufferLength) {
                *bufferPtr = '\0';
            }
            result = true;
        }
    }
    if (bytes != stackBuffer) {
        free(bytes);
    }
    return result;
}

// Native _CFURLGetQueryResolveFlags, 0x1c0249a84. Non-external. The -_queryResolveFlags of the URL's query
// string (CFURLCopyQueryString leaving nothing escaped; the message goes to NULL without a query), which is
// released afterwards. The callers test the 32-bit result.
uint32_t _CFURLGetQueryResolveFlags(OCFURLRef url) {
    OCFStringRef query = OCFURLCopyQueryString(url, NULL);
    uint32_t flags = ((uint32_t (*)(OCFStringRef, SEL))objc_msgSend)(query, sel_getUid("_queryResolveFlags"));
    if (query != NULL) {
        OCFRelease(query);
    }
    return flags;
}

// Native FileReferenceURLPathToFullURLPath, 0x1c0249ac0. Non-external. The path's UTF-8 form (a 2,048-byte
// buffer) must start with "/.file/id=", a decimal volume number (strtoll) and '.', then a decimal file
// number; one '/' after it is skipped. CoreServicesInternal's _FSCreatePathForFileID(allocator, volume, file,
// NULL) resolves the reference (NULL answers NULL). Its UTF-8 form (CFStringGetMaximumSizeForEncoding of the
// length, at least 1, plus one byte: a 1,024-byte stack buffer below 1,024 bytes, otherwise an unchecked
// malloc_type_malloc block, type 0x100004077774924) is escaped with
// CreateStringFromFileSystemRepresentationByAddingPercentEscapes(kCFAllocatorDefault, ..., false, false,
// false, NULL), or NULL when the conversion fails; the resolved path is released. A nonempty remainder is
// appended as "%@/%@" (NULL when it is not UTF-8), and the escaped path is released. CoreFoundation defect:
// kept. With a remainder and a failed conversion the escaped path is NULL and is passed to CFRelease. The
// conversion buffer is sized from the string's maximum UTF-8 size, so the conversion does not fail in
// ordinary use. Cut: _FSCreatePathForFileID is CoreServicesInternal's (unclassified in
// Configuration/Scope.json); it is declared here and left undefined.
OCFStringRef FileReferenceURLPathToFullURLPath(OCFStringRef path, OCFAllocatorRef allocator) {
    char pathBuffer[2048];
    if (!OCFStringGetCString(path, pathBuffer, sizeof(pathBuffer), kOCFStringEncodingUTF8)) {
        return NULL;
    }
    if (memcmp(pathBuffer, "/.file/id=", 10) != 0) {
        return NULL;
    }
    char *endPointer = NULL;
    long long volumeID = strtoll(pathBuffer + 10, &endPointer, 10);
    if (*endPointer != '.') {
        return NULL;
    }
    long long fileID = strtoll(endPointer + 1, &endPointer, 10);
    char *remainingPath = *endPointer == '/' ? endPointer + 1 : endPointer;
    OCFStringRef fullPath = _FSCreatePathForFileID(allocator, volumeID, fileID, NULL);
    if (fullPath == NULL) {
        return NULL;
    }
    OCFIndex length = OCFStringGetLength(fullPath);
    OCFIndex maximum =
        OCFStringGetMaximumSizeForEncoding((uint64_t)length > 1 ? length : 1, kOCFStringEncodingUTF8);
    char stackBuffer[1024];
    char *buffer;
    if (maximum >= 0x400) {
        buffer = malloc_type_malloc((size_t)maximum + 1, (malloc_type_id_t)UINT64_C(0x100004077774924));
    } else {
        buffer = stackBuffer;
    }
    OCFStringRef escapedPath;
    if (OCFStringGetCString(fullPath, buffer, maximum + 1, kOCFStringEncodingUTF8)) {
        escapedPath = CreateStringFromFileSystemRepresentationByAddingPercentEscapes(
            kOCFAllocatorDefault, (const UInt8 *)buffer, (OCFIndex)strlen(buffer), false, false, false, NULL
        );
    } else {
        escapedPath = NULL;
    }
    if (buffer != stackBuffer) {
        free(buffer);
    }
    OCFRelease(fullPath);
    if (*remainingPath == '\0') {
        return escapedPath;
    }
    OCFStringRef result;
    OCFStringRef remainder = OCFStringCreateWithCString(allocator, remainingPath, kOCFStringEncodingUTF8);
    if (remainder != NULL) {
        result = OCFStringCreateWithFormat(allocator, NULL, OF_STR("%@/%@"), escapedPath, remainder);
        OCFRelease(remainder);
    } else {
        result = NULL;
    }
    OCFRelease(escapedPath);
    return result;
}

// Native URLPathToPOSIXPath, 0x1c0249cf4. Non-external. The path unescaped in the encoding, leaving "/" and
// U+0000 escaped for a file URL (the UTF-16 constant "/\0", 0x2227b7620) and nothing otherwise; a final '/'
// beyond the original path's root length (__rootLength) is removed through a substring.
OCFStringRef URLPathToPOSIXPath(
    OCFStringRef path, OCFAllocatorRef allocator, OCFStringEncoding encoding, Boolean isFileURL
) {
    OCFStringRef result = OCFURLCreateStringByReplacingPercentEscapesUsingEncoding(
        allocator, path, isFileURL ? OF_STR("/\0") : emptyString(), encoding
    );
    if (result == NULL) {
        return NULL;
    }
    OCFIndex length = OCFStringGetLength(result);
    if (length > __rootLength(path) && OCFStringGetCharacterAtIndex(result, length - 1) == '/') {
        OCFStringRef truncated = OCFStringCreateWithSubstring(allocator, result, (OCFRange){0, length - 1});
        OCFRelease(result);
        return truncated;
    }
    return result;
}

// Native _CFURLIsFileURL, 0x1c0249db0. External. _CFURLHasFileURLScheme without the scheme answer (a tail
// call).
Boolean _OCFURLIsFileURL(OCFURLRef url) {
    return _CFURLHasFileURLScheme(url, NULL);
}

// Native __rootLength, 0x1c0249db8. Non-external. 11 for a path of at least 11 characters starting with
// "/.nofollow/"; for a longer path starting with "/.resolve/", the length up to and including the '/' that
// follows at least one digit; otherwise 1.
OCFIndex __rootLength(OCFStringRef path) {
    OCFIndex length = OCFStringGetLength(path);
    if (length < 11) {
        return 1;
    }
    if (OCFStringHasPrefix(path, OF_STR("/.nofollow/"))) {
        return 11;
    }
    if (length == 11 || !OCFStringHasPrefix(path, OF_STR("/.resolve/"))) {
        return 1;
    }
    for (OCFIndex index = 11;; ++index) {
        UniChar character = OCFStringGetCharacterAtIndex(path, index - 1);
        if (character == '/') {
            return index != 11 ? index : 1;
        }
        if ((uint32_t)(character - '0') > 9 || length == index) {
            return 1;
        }
    }
}

// Native CFURLGetFileSystemRepresentation, 0x1c0249e6c, with its .cold.1 fragment (the dispatch_once of the
// inlined __foundation_swift_nsurl_feature_enabled). External. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta12.json (family P-collections-url).
// Swift layout: a NULL URL or a zero maxBufLen answers false. The allocator is read; a CF URL (not tagged,
// not Objective-C) whose flags & 0x2007 are 6 is decoded by +[NSURL
// __swiftDecodeFileURLString:intoFSR:length:removePercentEscapes:] from its string (plain load at 0x18),
// with flag bit 15 as removePercentEscapes, and that answer is returned. Any other URL takes
// CFURLCreateStringWithFileSystemPath (POSIX style, resolveAgainstBase) and
// CFStringGetFileSystemRepresentation of it (the string released; NULL answers false). CF layout: the
// allocator is read before the NULL test (a NULL URL answers false). A CF URL with flag bit 12 (byte 0x11,
// bit 4) that either is not resolved against its base or has no base is converted directly from its string
// (acquire load) by CanonicalFileURLStringToFileSystemRepresentation (a tail call). Otherwise a URL without a
// base that is a file reference URL whose path (CFURLCopyPath) converts to UTF-8 in a 2048-byte stack buffer
// and starts with "/.file/id=" (one 8-byte and one 2-byte compare) followed by a decimal volume number, '.',
// a decimal file number (strtoll, base 10), an optional '/' and the end, is resolved by
// _FSGetFileSystemRepresentationForFileID(allocator, volume, file, buffer, maxBufLen, NULL), after "/"
// -_pathByInsertingResolveFlags: (when the URL's query resolve flags are nonzero) is copied in with
// CFStringGetBytes (UTF-8, no loss byte) and, when every character fit, the file path is written over its
// last byte; the path is released and a nonzero answer returns true. Every other case (and a zero answer)
// takes CFURLCreateStringWithFileSystemPath and CFStringGetFileSystemRepresentation as in the Swift layout.
// The CoreServicesInternal edge _FSGetFileSystemRepresentationForFileID is an unsupported cut (owner
// decision): it is declared above and left undefined, linked unbound (Configuration/LinkDependencies.json).
// Folds the compiler-outlined piece _CFURLGetFileSystemRepresentation.cold.1 (0x1c030d888).
Boolean OCFURLGetFileSystemRepresentation(
    OCFURLRef url, Boolean resolveAgainstBase, UInt8 *buffer, OCFIndex maxBufLen
) {
    if (_foundation_swift_nsurl_feature_enabled()) {
        if (url == NULL || maxBufLen == 0) {
            return false;
        }
        OCFAllocatorRef allocator = OCFGetAllocator(url);
        if (!isObjCURL(url) && (url->flags & 0x2007) == 6) {
            return ((BOOL (*)(void *, SEL, OCFStringRef, UInt8 *, OCFIndex, BOOL))objc_msgSend)(
                urlClass,
                sel_getUid("__swiftDecodeFileURLString:intoFSR:length:removePercentEscapes:"),
                url->string,
                buffer,
                maxBufLen,
                (url->flags >> 15) & 1
            );
        }
        OCFStringRef path =
            OCFURLCreateStringWithFileSystemPath(allocator, url, kOCFURLPOSIXPathStyle, resolveAgainstBase);
        if (path == NULL) {
            return false;
        }
        Boolean result = OCFStringGetFileSystemRepresentation(path, (char *)buffer, maxBufLen);
        OCFRelease(path);
        return result;
    }
    OCFAllocatorRef allocator = OCFGetAllocator(url);
    if (url == NULL) {
        return false;
    }
    bool direct;
    if (resolveAgainstBase) {
        OCFURLRef base = OCFURLGetBaseURL(url);
        direct = (intptr_t)url >= 0 && base == NULL;
    } else {
        direct = (intptr_t)url >= 0;
    }
    if (direct && !__CF_IS_OBJC(29, url) && ((url->flags >> 12) & 1) != 0) {
        return CanonicalFileURLStringToFileSystemRepresentation(
            __atomic_load_n(&((struct __OCFURL *)url)->string, __ATOMIC_ACQUIRE), buffer, maxBufLen
        );
    }
    if (OCFURLGetBaseURL(url) == NULL && OCFURLIsFileReferenceURL(url)) {
        OCFStringRef path = OCFURLCopyPath(url);
        if (path != NULL) {
            char pathBuffer[2048];
            if (OCFStringGetCString(path, pathBuffer, 0x800, kOCFStringEncodingUTF8)) {
                uint64_t prefix;
                uint16_t prefixEnd;
                memcpy(&prefix, pathBuffer, sizeof(prefix));
                memcpy(&prefixEnd, pathBuffer + 8, sizeof(prefixEnd));
                if (prefix == 0x692F656C69662E2F && prefixEnd == 0x3D64) {
                    char *end = NULL;
                    long long volumeID = strtoll(pathBuffer + 10, &end, 10);
                    if (*end == '.') {
                        long long fileID = strtoll(end + 1, &end, 10);
                        const char *tail = *end == '/' ? end + 1 : end;
                        if (*tail == '\0') {
                            uint32_t resolveFlags = _CFURLGetQueryResolveFlags(url);
                            OCFIndex usedBufLen = 0;
                            UInt8 *target = buffer;
                            OCFIndex targetLength = maxBufLen;
                            if (resolveFlags != 0) {
                                OCFStringRef resolvePrefix =
                                    ((OCFStringRef (*)(OCFStringRef, SEL, uint32_t))objc_msgSend)(
                                        slashString(),
                                        sel_getUid("_pathByInsertingResolveFlags:"),
                                        resolveFlags
                                    );
                                OCFRange range = {0, OCFStringGetLength(resolvePrefix)};
                                OCFStringGetBytes(
                                    resolvePrefix,
                                    range,
                                    kOCFStringEncodingUTF8,
                                    0,
                                    false,
                                    buffer,
                                    maxBufLen,
                                    &usedBufLen
                                );
                                if (usedBufLen == OCFStringGetLength(resolvePrefix)) {
                                    target = buffer + (usedBufLen - 1);
                                    targetLength = maxBufLen - (usedBufLen - 1);
                                }
                            }
                            Boolean found = _FSGetFileSystemRepresentationForFileID(
                                allocator, volumeID, fileID, target, targetLength, NULL
                            );
                            OCFRelease(path);
                            if (found) {
                                return true;
                            }
                            goto createString;
                        }
                    }
                }
            }
            OCFRelease(path);
        }
    }
createString:;
    OCFStringRef fileSystemPath =
        OCFURLCreateStringWithFileSystemPath(allocator, url, kOCFURLPOSIXPathStyle, resolveAgainstBase);
    if (fileSystemPath == NULL) {
        return false;
    }
    Boolean result = OCFStringGetFileSystemRepresentation(fileSystemPath, (char *)buffer, maxBufLen);
    OCFRelease(fileSystemPath);
    return result;
}

// Native _CFURLCreateWithFileSystemRepresentation, 0x1c024a224. Non-external.
// Swift layout: an empty buffer answers the retained base URL (or NULL). +[NSURL
// __swiftParseFileSystemRepresentation:length:into:isDirectory:] fills a zeroed stack header (NULL when it
// fails); an instance with 0x28 extra bytes receives it. A "/.file/id=" representation (flag bit 13) logs the
// fault "CFURL initialized with \"/.file/id=\" file system representation" (_CFOSLog; the .cold.3 fragment)
// and releases the header's string and replacement string. A relative representation without a base takes
// flag bit 12 and the current directory URL; a base is made absolute unless the parse found a scheme.
// CF layout: an empty buffer answers the retained base URL (or NULL). A "/.file/id=" representation (at
// least ten bytes; strncmp) logs the same fault (.cold.2) and answers NULL. A relative representation takes
// the base URL or the current directory URL (released afterwards). The string comes from
// CreateStringFromFileSystemRepresentationByAddingPercentEscapes; the instance (0x48 extra bytes for an
// absolute representation, 0x38 otherwise) gets UTF-8, a copy of the string (release store), the absolute
// base URL and the flags: 0x60014021 (0x60014821 for a directory) with bit 12, and bit 15 when the
// escaping reported false, plus the ranges {0, 4} and {7, length - 7} for an absolute representation;
// 0x14020 (0x14820) with bit 15 when the escaping reported false and the range {0, length} otherwise.
// CoreFoundation defect: 0x1c024a458 leaks the new instance when the Swift-layout fault path is taken (only
// the header's objects are released). The restoration releases the instance there; confidence: verified.
// Folds the compiler-outlined pieces __CFURLCreateWithFileSystemRepresentation.cold.1 (0x1c030d88c),
// __CFURLCreateWithFileSystemRepresentation.cold.2 (0x1c030d890),
// __CFURLCreateWithFileSystemRepresentation.cold.3 (0x1c030d8b4).
OCFURLRef _CFURLCreateWithFileSystemRepresentation(
    OCFAllocatorRef allocator, const UInt8 *buffer, OCFIndex bufLen, Boolean isDirectory, OCFURLRef baseURL
) {
    if (_foundation_swift_nsurl_feature_enabled()) {
        if (bufLen <= 0) {
            if (baseURL == NULL) {
                return NULL;
            }
            return OCFRetain(baseURL);
        }
        SwiftURLHeader header;
        memset(&header, 0, sizeof(header));
        BOOL parsed = ((BOOL (*)(void *, SEL, const UInt8 *, OCFIndex, SwiftURLHeader *, BOOL))objc_msgSend)(
            urlClass,
            sel_getUid("__swiftParseFileSystemRepresentation:length:into:isDirectory:"),
            buffer,
            bufLen,
            &header,
            isDirectory != 0
        );
        if (!parsed) {
            return NULL;
        }
        struct __OCFURL *url = (struct __OCFURL *)_OCFRuntimeCreateInstance(allocator, 29, 0x28, NULL);
        if (url != NULL) {
            if ((header.flags & 0x2000) == 0) {
                if (baseURL == NULL) {
                    if (buffer[0] != '/') {
                        header.flags |= 0x1000;
                        header.baseURL = _OCFURLCreateCurrentDirectoryURL(allocator);
                    }
                } else if ((header.flags & 4) == 0) {
                    OCFURLRef absoluteBase = OCFURLCopyAbsoluteURL(baseURL);
                    if (absoluteBase != NULL && _CFURLHasFileURLScheme(absoluteBase, NULL)) {
                        header.flags |= 0x1000;
                        if (OCFURLIsFileReferenceURL(absoluteBase)) {
                            header.flags |= 0x2000;
                        }
                    }
                    header.baseURL = absoluteBase;
                }
                memcpy(&url->flags, &header, sizeof(header));
                return url;
            }
            os_log_fault(_CFOSLog(), "CFURL initialized with \"/.file/id=\" file system representation");
        }
        if (header.string != NULL) {
            OCFRelease(header.string);
        }
        if (header.replacementString != NULL) {
            OCFRelease(header.replacementString);
        }
        if (url != NULL) {
            OCFRelease(url);
        }
        return NULL;
    }
    if (bufLen < 1) {
        if (baseURL == NULL) {
            return NULL;
        }
        return OCFRetain(baseURL);
    }
    UInt8 first = buffer[0];
    Boolean escapingResult = false;
    bool isAbsolute = first == '/';
    bool releaseBase;
    if (isAbsolute) {
        if ((uint64_t)bufLen >= 10 && strncmp((const char *)buffer, "/.file/id=", 10) == 0) {
            os_log_fault(_CFOSLog(), "CFURL initialized with \"/.file/id=\" file system representation");
            return NULL;
        }
        releaseBase = false;
        baseURL = NULL;
    } else if (baseURL != NULL) {
        releaseBase = false;
    } else {
        baseURL = _OCFURLCreateCurrentDirectoryURL(allocator);
        releaseBase = true;
    }
    struct __OCFURL *url = NULL;
    OCFStringRef path = CreateStringFromFileSystemRepresentationByAddingPercentEscapes(
        allocator, buffer, bufLen, isDirectory, isAbsolute, false, &escapingResult
    );
    if (path != NULL) {
        url = (struct __OCFURL *)_OCFRuntimeCreateInstance(allocator, 29, first == '/' ? 0x48 : 0x38, NULL);
        if (url != NULL) {
            url->encoding = kOCFStringEncodingUTF8;
            __atomic_store_n(&url->string, OCFStringCreateCopy(allocator, path), __ATOMIC_SEQ_CST);
            url->baseURL = baseURL != NULL ? OCFURLCopyAbsoluteURL(baseURL) : NULL;
            if (first == '/') {
                UInt32 escaped = escapingResult == 0 ? 0x9000 : 0x1000;
                url->flags = (isDirectory == 0 ? 0x60014021 : 0x60014821) | escaped;
                url->ranges[0] = (OCFRange){0, 4};
                url->ranges[1] = (OCFRange){7, OCFStringGetLength(path) - 7};
            } else {
                UInt32 escaped = escapingResult == 0 ? 0x8000 : 0;
                url->flags = (isDirectory == 0 ? 0x14020 : 0x14820) | escaped;
                OCFIndex length = OCFStringGetLength(__atomic_load_n(&url->string, __ATOMIC_SEQ_CST));
                url->ranges[0] = (OCFRange){0, length};
            }
        }
        OCFRelease(path);
    }
    if (baseURL != NULL && releaseBase) {
        OCFRelease(baseURL);
    }
    return url;
}

// Native CFURLCreateFromFileSystemRepresentationRelativeToBase, 0x1c024a604. External; a tail call.
OCFURLRef OCFURLCreateFromFileSystemRepresentationRelativeToBase(
    OCFAllocatorRef allocator, const UInt8 *buffer, OCFIndex bufLen, Boolean isDirectory, OCFURLRef baseURL
) {
    return _CFURLCreateWithFileSystemRepresentation(allocator, buffer, bufLen, isDirectory, baseURL);
}

// Native CFURLCopyLastPathComponent, 0x1c024a608. External.
// An Objective-C URL answers the last component of its POSIX path (CFURLCreateStringWithFileSystemPath
// without resolving; NULL without one): Swift layout through __getLastPathComponentRangeFromString (an empty
// range answers ""); CF layout by searching backwards for '/' before a trailing '/' (an empty path, or a path
// that is entirely its last component, answers the path itself). A file-reference URL is first turned into a
// file path URL (CFURLCreateFilePathURL; NULL answers NULL), released at the end. Swift layout: "" without a
// path (flag bit 5); otherwise the last component of the path range (see _CFURLCopyPath) through
// __getLastPathComponentRangeFromString, decoded when flag bit 15 is set by +[NSURL
// __copySwiftDecodedString:] (__copySwiftDecodedFilePath: for flag bit 12; with flag bit 16
// __copySwiftDecodedString:encoding: or, for bit 12, ...:encoding:charactersToLeaveEscaped: "/", with the
// stored encoding). CF layout: _rangeOfLastPathComponent ("" without one, "/" for a lone slash); unless flag
// bit 15 marks an unescaped path the substring is unescaped with the URL's encoding, leaving "/" escaped for
// a file URL and nothing otherwise.
// Folds the compiler-outlined piece _CFURLCopyLastPathComponent.cold.1 (0x1c030d8d8).
OCFStringRef OCFURLCopyLastPathComponent(OCFURLRef url) {
    if (_foundation_swift_nsurl_feature_enabled()) {
        OCFAllocatorRef allocator = OCFGetAllocator(url);
        if (isObjCURL(url)) {
            OCFStringRef path =
                OCFURLCreateStringWithFileSystemPath(allocator, url, kOCFURLPOSIXPathStyle, false);
            if (path == NULL) {
                return NULL;
            }
            OCFRange range = __getLastPathComponentRangeFromString(path, 0, OCFStringGetLength(path));
            OCFStringRef result;
            if (range.length == 0) {
                result = OCFRetain(emptyString());
            } else {
                result = OCFStringCreateWithSubstring(allocator, path, range);
            }
            OCFRelease(path);
            return result;
        }
        Boolean isFileReference = OCFURLIsFileReferenceURL(url);
        if (isFileReference) {
            url = OCFURLCreateFilePathURL(allocator, url, NULL);
            if (url == NULL) {
                return NULL;
            }
        }
        UInt32 flags = url->flags;
        if ((flags & 0x20) == 0) {
            if (isFileReference) {
                OCFRelease(url);
            }
            return OCFRetain(emptyString());
        }
        OCFRange pathRange;
        switch (flags & 3) {
            case 2: {
                OCFIndex length = OCFStringGetLength(url->string);
                if ((url->flags & 4) != 0) {
                    pathRange = (OCFRange){7, length - 7};
                } else {
                    pathRange = (OCFRange){0, length};
                }
                break;
            }
            case 1:
                pathRange = (OCFRange){(OCFIndex)swiftField64(url, 0x68), (OCFIndex)swiftField64(url, 0x70)};
                break;
            default: {
                uint32_t packed = swiftField32(url, 0x44);
                pathRange = (OCFRange){packed & 0xffff, packed >> 16};
                break;
            }
        }
        OCFRange range =
            __getLastPathComponentRangeFromString(url->string, pathRange.location, pathRange.length);
        OCFStringRef component;
        if (range.length == 0) {
            component = OCFRetain(emptyString());
        } else {
            component = OCFStringCreateWithSubstring(allocator, url->string, range);
        }
        if ((flags & 0x8000) != 0 && component != NULL) {
            OCFStringRef decoded;
            if ((flags & 0x10000) != 0) {
                OCFStringEncoding encoding =
                    (url->flags & 0x10000) != 0 ? url->encoding : kOCFStringEncodingUTF8;
                if ((flags & 0x1000) != 0) {
                    decoded = ((OCFStringRef (*)(void *, SEL, OCFStringRef, OCFStringEncoding, OCFStringRef))
                                   objc_msgSend)(
                        urlClass,
                        sel_getUid("__copySwiftDecodedString:encoding:charactersToLeaveEscaped:"),
                        component,
                        encoding,
                        slashString()
                    );
                } else {
                    decoded = ((OCFStringRef (*)(void *, SEL, OCFStringRef, OCFStringEncoding))objc_msgSend)(
                        urlClass, sel_getUid("__copySwiftDecodedString:encoding:"), component, encoding
                    );
                }
            } else if ((flags & 0x1000) != 0) {
                decoded = ((OCFStringRef (*)(void *, SEL, OCFStringRef))objc_msgSend)(
                    urlClass, sel_getUid("__copySwiftDecodedFilePath:"), component
                );
            } else {
                decoded = ((OCFStringRef (*)(void *, SEL, OCFStringRef))objc_msgSend)(
                    urlClass, sel_getUid("__copySwiftDecodedString:"), component
                );
            }
            OCFRelease(component);
            if (isFileReference) {
                OCFRelease(url);
            }
            return decoded;
        }
        if (isFileReference) {
            OCFRelease(url);
        }
        return component;
    }
    if (isObjCURL(url)) {
        OCFStringRef path =
            OCFURLCreateStringWithFileSystemPath(OCFGetAllocator(url), url, kOCFURLPOSIXPathStyle, false);
        OCFRange slash = {0, 0};
        if (path == NULL) {
            return path;
        }
        OCFIndex length = OCFStringGetLength(path);
        if (length == 0) {
            return path;
        }
        OCFIndex end = length - 1;
        if (OCFStringGetCharacterAtIndex(path, end) != '/') {
            end = length;
        }
        if (end == 0) {
            return path;
        }
        OCFIndex start =
            OCFStringFindWithOptions(path, slashString(), (OCFRange){0, end}, kOCFCompareBackwards, &slash)
                ? slash.location + 1
                : 0;
        OCFIndex componentLength = end - start;
        if (start == 0 && componentLength == length) {
            return path;
        }
        OCFStringRef component =
            OCFStringCreateWithSubstring(OCFGetAllocator(url), path, (OCFRange){start, componentLength});
        OCFRelease(path);
        return component;
    }
    Boolean isFileReference = OCFURLIsFileReferenceURL(url);
    if (isFileReference) {
        url = OCFURLCreateFilePathURL(OCFGetAllocator(url), url, NULL);
        if (url == NULL) {
            return NULL;
        }
    }
    OCFRange range = _rangeOfLastPathComponent(url);
    if (range.location == kOCFNotFound || range.length == 0) {
        if (isFileReference) {
            OCFRelease(url);
        }
        return OCFRetain(emptyString());
    }
    if (range.length == 1 &&
        OCFStringGetCharacterAtIndex(__atomic_load_n(&url->string, __ATOMIC_SEQ_CST), range.location) ==
            '/') {
        if (isFileReference) {
            OCFRelease(url);
        }
        return OCFRetain(slashString());
    }
    OCFStringRef component = OCFStringCreateWithSubstring(
        OCFGetAllocator(url), __atomic_load_n(&url->string, __ATOMIC_SEQ_CST), range
    );
    if ((url->flags & 0x8000) == 0) {
        OCFStringEncoding encoding = url->encoding;
        OCFStringRef leaveEscaped = _CFURLHasFileURLScheme(url, NULL) ? slashString() : emptyString();
        OCFAllocatorRef allocator = OCFGetAllocator(url);
        OCFStringRef unescaped;
        if (encoding == kOCFStringEncodingUTF8) {
            unescaped = OCFURLCreateStringByReplacingPercentEscapes(allocator, component, leaveEscaped);
        } else {
            unescaped = OCFURLCreateStringByReplacingPercentEscapesUsingEncoding(
                allocator, component, leaveEscaped, url->encoding
            );
        }
        OCFRelease(component);
        component = unescaped;
    }
    if (isFileReference) {
        OCFRelease(url);
    }
    return component;
}

// Native _rangeOfLastPathComponent, 0x1c024aabc. Non-external. The path range ({-1, 0} without a path)
// answers when it is absent or empty, and also when it is the single character '/'. Otherwise a final '/' is
// excluded, and a backwards search for "/" (cfstr 0x2227adb00) in what remains starts the component after the
// last slash; the component runs to the end of the searched range (the string is loaded sequentially
// consistent for each read).
OCFRange _rangeOfLastPathComponent(OCFURLRef url) {
    UInt32 flags = url->flags;
    OCFRange range;
    if ((flags & 0x20) != 0) {
        range = url->ranges[componentRangeIndex(flags, 0x20)];
    } else {
        range = (OCFRange){kOCFNotFound, 0};
    }
    if (range.location == kOCFNotFound || range.length == 0) {
        return range;
    }
    OCFRange result = {0, 0};
    OCFIndex length = range.length;
    if (OCFStringGetCharacterAtIndex(
            __atomic_load_n(&url->string, __ATOMIC_SEQ_CST), range.location + range.length - 1
        ) == '/') {
        length = range.length - 1;
        if (length == 0) {
            return range;
        }
    }
    if (OCFStringFindWithOptions(
            __atomic_load_n(&url->string, __ATOMIC_SEQ_CST),
            slashString(),
            (OCFRange){range.location, length},
            kOCFCompareBackwards,
            &result
        )) {
        length = range.location + length - result.location - 1;
        range.location = result.location + 1;
    }
    range.length = length;
    return range;
}

// Native CFURLCreateCopyAppendingPathComponent, 0x1c024ad70. External; slashes in the component are not
// escaped.
OCFURLRef OCFURLCreateCopyAppendingPathComponent(
    OCFAllocatorRef allocator, OCFURLRef url, OCFStringRef pathComponent, Boolean isDirectory
) {
    return __CFURLCreateCopyAppendingPathComponent(allocator, url, pathComponent, isDirectory, false);
}

// Native __CFURLCreateCopyAppendingPathComponent, 0x1c024ad78. Non-external. encodeSlashes selects ";?/"
// instead of ";?" as the characters to escape.
// Swift layout: an Objective-C URL is replaced by its -_cfurl; a file-reference URL (flag bit 13) is first
// turned into a file path URL, which must not be a file reference (assertion, CFURL.c:7510, the .cold.2
// fragment), and the call recurses on it. Without a path (flag bit 5) the result is NULL. The component is
// encoded by +[NSURL __copySwiftEncodedFSRPath:] (flag bit 12) or
// __copySwiftEncodedPathComponent:encodeSlashes: (with :encoding: and the stored encoding for flag bit 16),
// and inserted at the end of the path range (see _CFURLCopyPath) of a mutable copy of the string, after a '/'
// when neither the path ends with one nor the component starts with one and followed by '/' for a directory
// (built in a separate mutable string unless neither is needed). CF layout: an Objective-C URL is replaced by
// its -_cfurl and a file-reference URL by a file path URL (released afterwards); a URL without a path answers
// NULL; the component is encoded by POSIXPathToURLPath for a file URL, otherwise percent-escaped with the
// URL's encoding, and inserted after the path the same way, directly into the mutable copy. The result is an
// arbitrary-string URL with the URL's base.
// Folds the compiler-outlined piece ___CFURLCreateCopyAppendingPathComponent.cold.1 (0x1c030d8e0).
static OCFURLRef __CFURLCreateCopyAppendingPathComponent(
    OCFAllocatorRef allocator,
    OCFURLRef url,
    OCFStringRef pathComponent,
    Boolean isDirectory,
    Boolean encodeSlashes
) {
    if (_foundation_swift_nsurl_feature_enabled()) {
        if (isObjCURL(url)) {
            url = sendURL(url, "_cfurl");
        }
        UInt32 flags = url->flags;
        if ((flags & 0x2000) != 0) {
            OCFURLRef filePathURL = OCFURLCreateFilePathURL(allocator, url, NULL);
            if (filePathURL != NULL) {
                if (OCFURLIsFileReferenceURL(filePathURL)) {
                    __assert_rtn(
                        "__CFURLCreateCopyAppendingPathComponent",
                        "CFURL.c",
                        0x1d56,
                        "!CFURLIsFileReferenceURL(filePathURL)"
                    );
                }
                OCFURLRef result = __CFURLCreateCopyAppendingPathComponent(
                    allocator, filePathURL, pathComponent, isDirectory, encodeSlashes
                );
                OCFRelease(filePathURL);
                return result;
            }
        }
        if ((flags & 0x20) == 0) {
            return NULL;
        }
        OCFStringRef encoded;
        if ((flags & 0x1000) != 0) {
            encoded = ((OCFStringRef (*)(void *, SEL, OCFStringRef))objc_msgSend)(
                urlClass, sel_getUid("__copySwiftEncodedFSRPath:"), pathComponent
            );
        } else if ((flags & 0x10000) != 0) {
            OCFStringEncoding encoding = (url->flags & 0x10000) != 0 ? url->encoding : kOCFStringEncodingUTF8;
            encoded = ((OCFStringRef (*)(void *, SEL, OCFStringRef, BOOL, OCFStringEncoding))objc_msgSend)(
                urlClass,
                sel_getUid("__copySwiftEncodedPathComponent:encodeSlashes:encoding:"),
                pathComponent,
                encodeSlashes != 0,
                encoding
            );
        } else {
            encoded = ((OCFStringRef (*)(void *, SEL, OCFStringRef, BOOL))objc_msgSend)(
                urlClass,
                sel_getUid("__copySwiftEncodedPathComponent:encodeSlashes:"),
                pathComponent,
                encodeSlashes != 0
            );
        }
        if (encoded == NULL) {
            return NULL;
        }
        OCFIndex pathLocation;
        OCFIndex pathLength;
        switch (url->flags & 3) {
            case 2: {
                OCFIndex length = OCFStringGetLength(url->string);
                if ((url->flags & 4) != 0) {
                    pathLocation = 7;
                    pathLength = length - 7;
                } else {
                    pathLocation = 0;
                    pathLength = length;
                }
                break;
            }
            case 1:
                pathLocation = (OCFIndex)swiftField64(url, 0x68);
                pathLength = (OCFIndex)swiftField64(url, 0x70);
                break;
            default: {
                uint32_t packed = swiftField32(url, 0x44);
                pathLength = packed >> 16;
                pathLocation = packed & 0xffff;
                break;
            }
        }
        OCFIndex encodedLength = OCFStringGetLength(encoded);
        bool needsSlash;
        if (pathLength != 0 &&
            OCFStringGetCharacterAtIndex(url->string, pathLength + pathLocation - 1) == '/') {
            needsSlash = false;
        } else if (encodedLength != 0) {
            needsSlash = OCFStringGetCharacterAtIndex(encoded, 0) != '/';
        } else {
            needsSlash = true;
        }
        OCFIndex stringLength = OCFStringGetLength(url->string);
        OCFIndex insertCapacity = encodedLength + 2;
        OCFMutableStringRef newString =
            OCFStringCreateMutableCopy(allocator, insertCapacity + stringLength, url->string);
        if (newString == NULL) {
            OCFRelease(encoded);
            return NULL;
        }
        OCFStringRef inserted;
        if (isDirectory == 0 && !needsSlash) {
            inserted = encoded;
        } else {
            OCFMutableStringRef insertion = OCFStringCreateMutable(allocator, insertCapacity);
            if (insertion == NULL) {
                OCFRelease(encoded);
                OCFRelease(newString);
                return NULL;
            }
            if (needsSlash) {
                OCFStringAppend(insertion, slashString());
            }
            OCFStringAppend(insertion, encoded);
            if (isDirectory) {
                OCFStringAppend(insertion, slashString());
            }
            OCFRelease(encoded);
            inserted = insertion;
        }
        OCFStringInsert(newString, pathLength + pathLocation, inserted);
        OCFRelease(inserted);
        OCFURLRef result = _CFURLCreateWithArbitraryString(allocator, newString, url->baseURL);
        OCFRelease(newString);
        return result;
    }
    OCFURLRef target = isObjCURL(url) ? sendURL(url, "_cfurl") : url;
    bool borrowed;
    if (OCFURLIsFileReferenceURL(target)) {
        OCFURLRef filePathURL = OCFURLCreateFilePathURL(allocator, target, NULL);
        borrowed = filePathURL == NULL;
        target = filePathURL == NULL ? target : filePathURL;
    } else {
        borrowed = true;
    }
    OCFURLRef result = NULL;
    if ((target->flags & 0x20) != 0) {
        OCFMutableStringRef newString =
            OCFStringCreateMutableCopy(allocator, 0, __atomic_load_n(&target->string, __ATOMIC_SEQ_CST));
        if (newString != NULL) {
            OCFStringRef encoded;
            if (_CFURLHasFileURLScheme(target, NULL)) {
                encoded = POSIXPathToURLPath(pathComponent, false, false, NULL);
            } else {
                encoded = OCFURLCreateStringByAddingPercentEscapes(
                    allocator,
                    pathComponent,
                    NULL,
                    encodeSlashes == 0 ? OF_STR(";?") : OF_STR(";?/"),
                    target->encoding
                );
            }
            if (encoded != NULL) {
                UInt32 flags = target->flags;
                OCFIndex pathLocation;
                OCFIndex pathLength;
                bool needsSlash = true;
                if ((flags & 0x20) != 0) {
                    OCFRange range = target->ranges[componentRangeIndex(flags, 0x20)];
                    pathLocation = range.location;
                    pathLength = range.length;
                    if (pathLength != 0 &&
                        OCFStringGetCharacterAtIndex(
                            __atomic_load_n(&target->string, __ATOMIC_SEQ_CST), pathLocation + pathLength - 1
                        ) == '/') {
                        needsSlash = false;
                    }
                } else {
                    pathLength = 0;
                    pathLocation = kOCFNotFound;
                }
                if (needsSlash) {
                    if (OCFStringGetLength(encoded) == 0 || OCFStringGetCharacterAtIndex(encoded, 0) != '/') {
                        OCFStringInsert(newString, pathLocation + pathLength, slashString());
                        pathLength += 1;
                    }
                }
                OCFIndex insertionIndex = pathLength + pathLocation;
                OCFStringInsert(newString, insertionIndex, encoded);
                if (isDirectory) {
                    OCFStringInsert(newString, OCFStringGetLength(encoded) + insertionIndex, slashString());
                }
                OCFRelease(encoded);
                result = _CFURLCreateWithArbitraryString(allocator, newString, target->baseURL);
            }
            OCFRelease(newString);
        }
    }
    if (!borrowed) {
        OCFRelease(target);
    }
    return result;
}

// Native POSIXPathToURLPath, 0x1c024bc60. Non-external. The file system representation of the path
// (CFStringGetMaximumSizeOfFileSystemRepresentation bytes, 1 for an empty path: a 1,024-byte stack buffer
// up to 1,024 bytes, otherwise an unchecked malloc_type_malloc block, type 0x100004077774924) is escaped with
// CreateStringFromFileSystemRepresentationByAddingPercentEscapes(kCFAllocatorDefault, ..., isDirectory,
// isAbsolute, false, &escaped); NULL when the representation fails. posixResult (optional) receives whether
// nothing was escaped (true when the escaping was not reached).
OCFStringRef
POSIXPathToURLPath(OCFStringRef path, Boolean isDirectory, Boolean isAbsolute, Boolean *posixResult) {
    Boolean escaped = false;
    UInt8 stackBuffer[1024];
    UInt8 *buffer;
    OCFIndex maximum;
    if (OCFStringGetLength(path) == 0) {
        buffer = stackBuffer;
        maximum = 1;
    } else {
        maximum = OCFStringGetMaximumSizeOfFileSystemRepresentation(path);
        if (maximum >= 0x401) {
            buffer = malloc_type_malloc((size_t)maximum, (malloc_type_id_t)UINT64_C(0x100004077774924));
        } else {
            buffer = stackBuffer;
        }
    }
    OCFStringRef result;
    if (OCFStringGetFileSystemRepresentation(path, (char *)buffer, maximum)) {
        result = CreateStringFromFileSystemRepresentationByAddingPercentEscapes(
            kOCFAllocatorDefault,
            buffer,
            (OCFIndex)strlen((const char *)buffer),
            isDirectory,
            isAbsolute,
            false,
            &escaped
        );
    } else {
        result = NULL;
    }
    if (buffer != stackBuffer) {
        free(buffer);
    }
    if (posixResult != NULL) {
        *posixResult = escaped == 0;
    }
    return result;
}

// Native CFURLCreateFileReferenceURL, 0x1c024c4f8 (corpus delta capture 7). External. NULL for a NULL URL or
// a NULL absolute URL. A URL without a file scheme answers NULL: without any scheme it logs "... has no
// scheme" (level 4); with an error pointer it stores CFErrorCreate(kCFAllocatorDefault, kCFErrorDomainCocoa,
// 262, NULL). Otherwise the path, net location and resource specifier are copied. A path that already is a
// file reference ("/.file/id=" followed by no '/' or only a trailing one) is kept with the URL's query
// resolve flags; otherwise the path's -_pathResolveFlags (sent to a NULL path too, answering 0; an NSString
// category of Foundation) is read, the path released and the reference path made by
// _CFURLCopyFileReferenceURLPath (NULL answers NULL). A path ending in '/' marks a directory. Without
// resolve flags, resource specifier or net location other than "localhost" (CFEqual), the POSIX form of the
// path (URLPathToPOSIXPath with the URL's encoding, NULL answers NULL) becomes a POSIX file-system path URL
// (allowing file-ID paths); otherwise "file://%@%@%@" of the net location (or ""), the path and the resource
// specifier (or "") becomes the URL (CFURLCreateWithString, no base), after
// -_urlStringByInsertingQueryResolveFlags: (Foundation's) when there are resolve flags. Every copy is
// released, the absolute URL last.
OCFURLRef OCFURLCreateFileReferenceURL(OCFAllocatorRef allocator, OCFURLRef url, OCFErrorRef *error) {
    if (url == NULL) {
        return NULL;
    }
    OCFURLRef absolute = OCFURLCopyAbsoluteURL(url);
    if (absolute == NULL) {
        return NULL;
    }
    OCFURLRef result = NULL;
    Boolean hasScheme = false;
    if (!_CFURLHasFileURLScheme(absolute, &hasScheme)) {
        if (!hasScheme) {
            OCFLog(
                4,
                OF_STR("CFURLCreateFileReferenceURL failed because it was passed a URL which has no scheme")
            );
        }
        if (error != NULL) {
            *error = OCFErrorCreate(kOCFAllocatorDefault, kOCFErrorDomainCocoa, 262, NULL);
        }
        OCFRelease(absolute);
        return NULL;
    }
    OCFStringRef path = OCFURLCopyPath(absolute);
    OCFStringRef netLocation = OCFURLCopyNetLocation(absolute);
    OCFStringRef resourceSpecifier = OCFURLCopyResourceSpecifier(absolute);
    uint32_t resolveFlags;
    OCFStringRef referencePath;
    bool isReference = false;
    if (path != NULL && OCFStringHasPrefix(path, OF_STR("/.file/id="))) {
        OCFRange found = {0, 0};
        OCFRange searchRange = {10, OCFStringGetLength(path) - 10};
        if (!OCFStringFindWithOptions(path, OF_STR("/"), searchRange, 0, &found)) {
            isReference = true;
        } else {
            OCFIndex location = found.location;
            isReference = location == OCFStringGetLength(path) - 1;
        }
    }
    if (isReference) {
        resolveFlags = _CFURLGetQueryResolveFlags(absolute);
        referencePath = path;
    } else {
        resolveFlags = ((uint32_t (*)(id, SEL))objc_msgSend)((id)path, sel_getUid("_pathResolveFlags"));
        if (path != NULL) {
            OCFRelease(path);
        }
        referencePath = _CFURLCopyFileReferenceURLPath(absolute, error);
    }
    if (referencePath != NULL) {
        Boolean isDirectory =
            OCFStringGetCharacterAtIndex(referencePath, OCFStringGetLength(referencePath) - 1) == '/';
        if (resolveFlags == 0 && resourceSpecifier == NULL &&
            (netLocation == NULL || OCFEqual(netLocation, OF_STR("localhost")))) {
            OCFStringRef posixPath = URLPathToPOSIXPath(referencePath, allocator, absolute->encoding, true);
            if (posixPath != NULL) {
                result = _CFURLCreateWithFileSystemPath(
                    allocator, posixPath, kOCFURLPOSIXPathStyle, isDirectory, NULL, true
                );
                OCFRelease(posixPath);
            }
        } else {
            OCFStringRef created = OCFStringCreateWithFormat(
                allocator,
                NULL,
                OF_STR("file://%@%@%@"),
                netLocation != NULL ? netLocation : OF_STR(""),
                referencePath,
                resourceSpecifier != NULL ? resourceSpecifier : OF_STR("")
            );
            OCFStringRef urlString = created;
            if (resolveFlags != 0) {
                urlString = ((OCFStringRef (*)(id, SEL, uint32_t))objc_msgSend)(
                    (id)created, sel_getUid("_urlStringByInsertingQueryResolveFlags:"), resolveFlags
                );
            }
            result = OCFURLCreateWithString(allocator, urlString, NULL);
            OCFRelease(created);
        }
        OCFRelease(referencePath);
    }
    if (netLocation != NULL) {
        OCFRelease(netLocation);
    }
    if (resourceSpecifier != NULL) {
        OCFRelease(resourceSpecifier);
    }
    OCFRelease(absolute);
    return result;
}

// Native _CFURLCopyFileReferenceURLPath, 0x1c024c7d4, with ____CFURLCopyFileReferenceURLPath_block_invoke
// (0x1c024f4d8, the once block) (corpus delta capture 7). Non-external. The URL's allocator is
// read first; the once block (dispatch_once on the function's token, the block capturing that allocator)
// makes the property keys array (kCFURLIsDirectoryKey, _kCFURLVolumeIsHFSStandardKey, _kCFURLVolumeIDKey,
// _kCFURLFileIDKey, _kCFURLParentDirectoryIDKey, kCFURLNameKey; kCFTypeArrayCallBacks). The URL's values for
// those keys (CFURLCopyResourcePropertiesForKeys with the error pointer; NULL answers NULL) give: for a
// directory, the volume ID and file ID; otherwise for an HFS standard volume the volume ID, the parent
// directory ID and the name; else the volume ID and file ID. With a volume ID, an ID and (for the HFS case)
// a name, both IDs are read as long long (a failure stores NULL in the error and answers NULL) and
// createFileReferencePath forms "/.file/id=volume.parent/name" or "/.file/id=volume.file" (with a trailing
// '/' for a directory). Otherwise the parent directory URL (kCFURLParentDirectoryURLKey) is asked for (a
// failure answers NULL; none stores NULL in the error) and its reference path made recursively (NULL answers
// NULL); an empty parent path stores NULL in the error; otherwise the URL's name (kCFURLNameKey; a failure
// answers NULL) is appended: "%@%@" after a trailing '/', else "%@/%@", each with a '/' added for a
// directory. A NULL result stores NULL in the error. The copies are released, the properties last; finally
// a NULL result with an error pointer whose error is still NULL receives CFErrorCreate(allocator,
// kCFErrorDomainCocoa, 256, NULL).
static OCFStringRef _CFURLCopyFileReferenceURLPath(OCFURLRef url, OCFErrorRef *error) {
    static dispatch_once_t onceToken;
    static OCFArrayRef sPropertyKeys;
    long long volumeID = 0;
    long long fileID = 0;
    OCFAllocatorRef allocator = OCFGetAllocator(url);
    dispatch_once(&onceToken, ^{
      const void *values[6] = {
          kOCFURLIsDirectoryKey,
          _kOCFURLVolumeIsHFSStandardKey,
          _kOCFURLVolumeIDKey,
          _kOCFURLFileIDKey,
          _kOCFURLParentDirectoryIDKey,
          kOCFURLNameKey,
      };
      sPropertyKeys = OCFArrayCreate(allocator, values, 6, &kOCFTypeArrayCallBacks);
    });
    OCFDictionaryRef properties = OCFURLCopyResourcePropertiesForKeys(url, sPropertyKeys, error);
    OCFStringRef result = NULL;
    if (properties == NULL) {
        if (error != NULL && *error == NULL) {
            *error = OCFErrorCreate(allocator, kOCFErrorDomainCocoa, 256, NULL);
        }
        return NULL;
    }
    OCFTypeRef isDirectory = OCFDictionaryGetValue(properties, kOCFURLIsDirectoryKey);
    OCFNumberRef volumeNumber;
    OCFNumberRef idNumber;
    OCFStringRef name = NULL;
    bool useParentID = false;
    bool haveName = true;
    bool directory = false;
    if (isDirectory == kOCFBooleanTrue) {
        volumeNumber = OCFDictionaryGetValue(properties, _kOCFURLVolumeIDKey);
        directory = true;
        idNumber = OCFDictionaryGetValue(properties, _kOCFURLFileIDKey);
    } else {
        OCFTypeRef isHFSStandard = OCFDictionaryGetValue(properties, _kOCFURLVolumeIsHFSStandardKey);
        volumeNumber = OCFDictionaryGetValue(properties, _kOCFURLVolumeIDKey);
        if (isHFSStandard == kOCFBooleanTrue) {
            idNumber = OCFDictionaryGetValue(properties, _kOCFURLParentDirectoryIDKey);
            name = OCFDictionaryGetValue(properties, kOCFURLNameKey);
            haveName = false;
            useParentID = true;
        } else {
            idNumber = OCFDictionaryGetValue(properties, _kOCFURLFileIDKey);
        }
    }
    if (name != NULL) {
        haveName = true;
    }
    if (volumeNumber != NULL && idNumber != NULL && haveName) {
        if (!OCFNumberGetValue(volumeNumber, kOCFNumberLongLongType, &volumeID) ||
            !OCFNumberGetValue(idNumber, kOCFNumberLongLongType, &fileID)) {
            if (error != NULL) {
                *error = NULL;
            }
        } else {
            if (useParentID) {
                result = createFileReferencePath(allocator, volumeID, fileID, 0, name, false);
            } else {
                result = createFileReferencePath(allocator, volumeID, 0, fileID, NULL, directory);
            }
            if (error != NULL && result == NULL) {
                *error = NULL;
            }
        }
    } else {
        OCFURLRef parent = NULL;
        if (OCFURLCopyResourcePropertyForKey(url, kOCFURLParentDirectoryURLKey, &parent, error)) {
            if (parent == NULL) {
                if (error != NULL) {
                    *error = NULL;
                }
            } else {
                OCFStringRef parentPath = _CFURLCopyFileReferenceURLPath(parent, error);
                if (parentPath != NULL) {
                    OCFStringRef fileName = NULL;
                    OCFIndex length = OCFStringGetLength(parentPath);
                    if (length < 1) {
                        if (error != NULL) {
                            *error = NULL;
                        }
                    } else if (OCFURLCopyResourcePropertyForKey(url, kOCFURLNameKey, &fileName, error)) {
                        UniChar last = OCFStringGetCharacterAtIndex(parentPath, length - 1);
                        OCFStringRef format;
                        if (last == '/') {
                            format = isDirectory == kOCFBooleanTrue ? OF_STR("%@%@/") : OF_STR("%@%@");
                        } else {
                            format = isDirectory == kOCFBooleanTrue ? OF_STR("%@/%@/") : OF_STR("%@/%@");
                        }
                        result = OCFStringCreateWithFormat(allocator, NULL, format, parentPath, fileName);
                        if (error != NULL && result == NULL) {
                            *error = NULL;
                        }
                        if (fileName != NULL) {
                            OCFRelease(fileName);
                        }
                    }
                    OCFRelease(parentPath);
                }
                OCFRelease(parent);
            }
        }
    }
    OCFRelease(properties);
    if (error != NULL && result == NULL && *error == NULL) {
        *error = OCFErrorCreate(allocator, kOCFErrorDomainCocoa, 256, NULL);
    }
    return result;
}

// Native CFURLCopyResourcePropertiesForKeys, 0x1c024cbb4 (corpus delta capture 7). External. A URL without a
// file scheme (logging "... has no scheme", level 4, when it has none) answers an empty dictionary on its
// allocator (CFType callbacks). Otherwise CoreServicesInternal's _FSURLCopyResourcePropertiesForKeys
// (unsupported cut) answers; a NULL answer without an error is the empty dictionary, with one it is handed
// to the caller (or released) and NULL answered.
OCFDictionaryRef OCFURLCopyResourcePropertiesForKeys(OCFURLRef url, OCFArrayRef keys, OCFErrorRef *error) {
    Boolean hasScheme = false;
    if (_CFURLHasFileURLScheme(url, &hasScheme)) {
        OCFErrorRef underlyingError = NULL;
        OCFDictionaryRef result =
            _FSURLCopyResourcePropertiesForKeys(url, keys, __OCFURLResourceInfoPtr(url), &underlyingError);
        if (result != NULL) {
            return result;
        }
        if (underlyingError != NULL) {
            if (error != NULL) {
                *error = underlyingError;
            } else {
                OCFRelease(underlyingError);
            }
            return NULL;
        }
    } else if (!hasScheme) {
        OCFLog(
            4,
            OF_STR(
                "CFURLCopyResourcePropertiesForKeys failed because it was passed a URL which has no scheme"
            )
        );
    }
    return OCFDictionaryCreate(
        OCFGetAllocator(url),
        NULL,
        NULL,
        0,
        &kOCFTypeDictionaryKeyCallBacks,
        &kOCFTypeDictionaryValueCallBacks
    );
}

// Native CFURLSetResourcePropertyForKey, 0x1c024ccb0 (corpus delta capture 7). External. A URL without a file
// scheme answers true (logging "... has no scheme", level 4, when it has none). Otherwise
// CoreServicesInternal's _FSURLSetResourcePropertyForKey (unsupported cut) is asked with the flag 1 and the
// resource info; a failure with an error hands it to the caller (or releases it) and answers false, anything
// else true.
Boolean OCFURLSetResourcePropertyForKey(
    OCFURLRef url, OCFStringRef key, OCFTypeRef propertyValue, OCFErrorRef *error
) {
    Boolean hasScheme = false;
    if (!_CFURLHasFileURLScheme(url, &hasScheme)) {
        if (!hasScheme) {
            OCFLog(
                4,
                OF_STR(
                    "CFURLSetResourcePropertyForKey failed because it was passed a URL which has no scheme"
                )
            );
        }
        return true;
    }
    OCFErrorRef underlyingError = NULL;
    if (_FSURLSetResourcePropertyForKey(
            url, key, propertyValue, true, __OCFURLResourceInfoPtr(url), &underlyingError
        ) ||
        underlyingError == NULL) {
        return true;
    }
    if (error != NULL) {
        *error = underlyingError;
    } else {
        OCFRelease(underlyingError);
    }
    return false;
}

// Native CFURLSetResourcePropertiesForKeys, 0x1c024cda0 (corpus delta capture 7). External. As
// CFURLSetResourcePropertyForKey with CoreServicesInternal's _FSURLSetResourcePropertiesForKeys (unsupported
// cut).
Boolean
OCFURLSetResourcePropertiesForKeys(OCFURLRef url, OCFDictionaryRef keyedPropertyValues, OCFErrorRef *error) {
    Boolean hasScheme = false;
    if (!_CFURLHasFileURLScheme(url, &hasScheme)) {
        if (!hasScheme) {
            OCFLog(
                4,
                OF_STR(
                    "CFURLSetResourcePropertiesForKeys failed because it was passed a URL which has no scheme"
                )
            );
        }
        return true;
    }
    OCFErrorRef underlyingError = NULL;
    if (_FSURLSetResourcePropertiesForKeys(
            url, keyedPropertyValues, __OCFURLResourceInfoPtr(url), &underlyingError
        ) ||
        underlyingError == NULL) {
        return true;
    }
    if (error != NULL) {
        *error = underlyingError;
    } else {
        OCFRelease(underlyingError);
    }
    return false;
}

// Native __CFURLBeginResourcePropertyCacheAccess, 0x1c024ce78 (corpus delta capture 7). External; a tail call
// of CoreServicesInternal's _FSURLBeginResourcePropertyCacheAccess (unsupported cut) with the resource info.
void __OCFURLBeginResourcePropertyCacheAccess(OCFURLRef url) {
    _FSURLBeginResourcePropertyCacheAccess(url, __OCFURLResourceInfoPtr(url));
}

// Native __CFURLEndResourcePropertyCacheAccess, 0x1c024cea0 (corpus delta capture 7). External; a tail call
// of CoreServicesInternal's _FSURLEndResourcePropertyCacheAccess (unsupported cut) with the resource info.
void __OCFURLEndResourcePropertyCacheAccess(OCFURLRef url) {
    _FSURLEndResourcePropertyCacheAccess(url, __OCFURLResourceInfoPtr(url));
}

// Native CFURLClearResourcePropertyCacheForKey, 0x1c024cec8 (corpus delta capture 7). External; a tail call
// of CoreServicesInternal's _FSURLClearResourcePropertyCacheForKey (unsupported cut) with the resource info.
void OCFURLClearResourcePropertyCacheForKey(OCFURLRef url, OCFStringRef key) {
    _FSURLClearResourcePropertyCacheForKey(url, key, __OCFURLResourceInfoPtr(url));
}

// Native CFURLClearResourcePropertyCache, 0x1c024cef8 (corpus delta capture 7). External; as
// CFURLClearResourcePropertyCacheForKey with a NULL key.
void OCFURLClearResourcePropertyCache(OCFURLRef url) {
    _FSURLClearResourcePropertyCacheForKey(url, NULL, __OCFURLResourceInfoPtr(url));
}

// Native CFURLSetTemporaryResourcePropertyForKey, 0x1c024cf24 (corpus delta capture 7). External. A URL
// without a file scheme logs "... has no scheme" (level 4) when it has none; otherwise CoreServicesInternal's
// _FSURLSetResourcePropertyForKey (unsupported cut) is asked with the flag 0, the resource info and no error
// pointer, its result ignored.
void OCFURLSetTemporaryResourcePropertyForKey(OCFURLRef url, OCFStringRef key, OCFTypeRef propertyValue) {
    Boolean hasScheme = false;
    if (_CFURLHasFileURLScheme(url, &hasScheme)) {
        _FSURLSetResourcePropertyForKey(url, key, propertyValue, false, __OCFURLResourceInfoPtr(url), NULL);
    } else if (!hasScheme) {
        OCFLog(
            4,
            OF_STR(
                "CFURLSetTemporaryResourcePropertyForKey failed because it was passed a URL which has no "
                "scheme"
            )
        );
    }
}

// Native CFURLStartAccessingSecurityScopedResource, 0x1c024d7ac (corpus delta capture 7). External.
// CoreServicesInternal's _CFURLStartAccessingSecurityScopedResource (unsupported cut), its 32-bit result
// compared with 0.
Boolean OCFURLStartAccessingSecurityScopedResource(OCFURLRef url) {
    return _CFURLStartAccessingSecurityScopedResource(url) != 0;
}

// Native CFURLStopAccessingSecurityScopedResource, 0x1c024d7c8 (corpus delta capture 7). External; a tail
// call of CoreServicesInternal's _CFURLStopAccessingSecurityScopedResource (unsupported cut).
void OCFURLStopAccessingSecurityScopedResource(OCFURLRef url) {
    _CFURLStopAccessingSecurityScopedResource(url);
}

// Native CreateTruncatedURLString, 0x1c024d880. Non-external. A copy for up to 128 characters; otherwise the
// first 120 and the last 8 characters joined as "%@ ... %@" (both substrings released when present).
OCFStringRef CreateTruncatedURLString(OCFAllocatorRef allocator, OCFStringRef string) {
    OCFIndex length = OCFStringGetLength(string);
    if (length <= 0x80) {
        return OCFStringCreateCopy(allocator, string);
    }
    OCFStringRef head = OCFStringCreateWithSubstring(allocator, string, (OCFRange){0, 0x78});
    OCFStringRef tail = OCFStringCreateWithSubstring(allocator, string, (OCFRange){length - 8, 8});
    OCFStringRef result = OCFStringCreateWithFormat(allocator, NULL, OF_STR("%@ ... %@"), head, tail);
    if (head != NULL) {
        OCFRelease(head);
    }
    if (tail != NULL) {
        OCFRelease(tail);
    }
    return result;
}

// Native _CFURLAllocateExtraDataspace, 0x1c024d948. Non-external. Without additional data a 24-byte block is
// allocated with the URL's allocator (CFAllocatorAllocateTyped, type 0x10E0040191BE475, hint 0); the block's
// first 20 bytes are copied from the current additional data when present (re-read after the allocation)
// and zeroed otherwise.
void _CFURLAllocateExtraDataspace(struct __OCFURL *url) {
    if (url->extra != NULL) {
        return;
    }
    OCFURLAdditionalData *extra = OCFAllocatorAllocateTyped(
        OCFGetAllocator(url), 0x18, (OCFAllocatorTypeID)UINT64_C(0x10E0040191BE475), 0
    );
    OCFURLAdditionalData *current = url->extra;
    if (current != NULL) {
        extra->additionalFlags = current->additionalFlags;
        extra->unread0 = current->unread0;
        extra->sanitizedString = current->sanitizedString;
    } else {
        extra->additionalFlags = 0;
        extra->unread0 = NULL;
        extra->sanitizedString = NULL;
    }
    url->extra = extra;
}

// Native constructBuffers, 0x1c024d9b8. Non-external. The string's direct Latin-1 C string, else its direct
// characters; otherwise the string is measured in Latin-1 (CFStringGetBytes with no buffer and a 0x7fffffff
// limit). A lossless measurement fills a Latin-1 copy (the caller's 1,024-byte buffer up to 1,024 bytes,
// otherwise an unchecked malloc_type_malloc block, type 0x100004077774924); else the characters are copied
// (the buffer below 1,025 bytes, otherwise a block of type 0x1000040BDFB0063). freeCharacters records a
// block; *cString is always stored, *characters only when the direct C string is absent.
void constructBuffers(
    OCFStringRef string,
    UInt8 *buffer,
    const char **cString,
    const UniChar **characters,
    Boolean *useCString,
    Boolean *freeCharacters
) {
    *cString = OCFStringGetCStringPtr(string, kOCFStringEncodingISOLatin1);
    if (*cString != NULL) {
        *characters = NULL;
        *useCString = true;
        *freeCharacters = false;
        return;
    }
    *characters = OCFStringGetCharactersPtr(string);
    if (*characters != NULL) {
        *useCString = false;
        *freeCharacters = false;
        return;
    }
    OCFIndex usedBufferLength = 0;
    OCFIndex length = OCFStringGetLength(string);
    OCFStringGetBytes(
        string,
        (OCFRange){0, length},
        kOCFStringEncodingISOLatin1,
        0,
        false,
        NULL,
        0x7fffffff,
        &usedBufferLength
    );
    if (usedBufferLength == length) {
        UInt8 *bytes = buffer;
        if (length >= 0x401) {
            bytes = malloc_type_malloc((size_t)length, (malloc_type_id_t)UINT64_C(0x100004077774924));
            *freeCharacters = true;
        } else {
            *freeCharacters = false;
        }
        OCFStringGetBytes(
            string, (OCFRange){0, length}, kOCFStringEncodingISOLatin1, 0, false, bytes, length, NULL
        );
        *cString = (const char *)bytes;
        *useCString = true;
    } else {
        UniChar *uniBuffer = (UniChar *)(void *)buffer;
        size_t size = (size_t)length * sizeof(UniChar);
        if (size >= 0x401) {
            uniBuffer = malloc_type_malloc(size, (malloc_type_id_t)UINT64_C(0x1000040BDFB0063));
            *freeCharacters = true;
        } else {
            *freeCharacters = false;
        }
        OCFStringGetCharacters(string, (OCFRange){0, length}, uniBuffer);
        *characters = uniBuffer;
        *useCString = false;
    }
}

// Native scanCharacters, 0x1c024db74. Non-external. One body expanded twice natively (Latin-1 bytes and
// UTF-16 characters); restored as one loop. A C-string byte is sign-extended to 16 bits, as the native byte
// loads do, so bytes above 0x7f are illegal and escape as the characters U+FF80..U+FFFF. Over [start, end)
// (false for an empty range): legal characters (sURLValidCharacters bit 0) pass except '/', '?' and '@' in
// the user (2) and password (4) components; "%" followed by two hexadecimal digits (bit 4) passes; in the
// host (8) a leading '[' and a trailing ']' pass. Every other character sets the component flag in
// *additionalFlags (when both are nonzero), creates the escaped string on first use (capacity 0), appends the
// unescaped run since *mark (a Latin-1 substring that is appended and released, or the characters), and
// appends the character's escapes in the encoding, advancing *mark past it; when that conversion fails, UTF-8
// retries a high surrogate with the following low surrogate as a pair (advancing past both); otherwise *mark
// stays. The answer is whether any character needed escaping.
Boolean scanCharacters(
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
) {
    Boolean sawIllegalCharacter = false;
    if (start >= end) {
        return false;
    }
    Boolean setAdditionalFlag = additionalFlags != NULL && componentFlag != 0;
    for (OCFIndex idx = start; idx < end; idx++) {
        UniChar character = useCString ? (UniChar)(int8_t)cString[idx] : characters[idx];
        if (character <= 0x7f && (sURLValidCharacters[character] & 1) != 0) {
            if ((componentFlag != 4 && componentFlag != 2) ||
                (character != '/' && character != '?' && character != '@')) {
                continue;
            }
        } else {
            if (character == '%' && idx + 2 < end) {
                UniChar first = useCString ? (UniChar)(int8_t)cString[idx + 1] : characters[idx + 1];
                UniChar second = useCString ? (UniChar)(int8_t)cString[idx + 2] : characters[idx + 2];
                if (first <= 0x7f && (sURLValidCharacters[first] & 0x10) != 0 && second <= 0x7f &&
                    (sURLValidCharacters[second] & 0x10) != 0) {
                    continue;
                }
            }
            if (componentFlag == 8 &&
                ((idx == start && character == '[') || (idx == end - 1 && character == ']'))) {
                continue;
            }
        }
        if (setAdditionalFlag) {
            *additionalFlags |= componentFlag;
        }
        if (*escapedString == NULL) {
            *escapedString = OCFStringCreateMutable(allocator, 0);
        }
        if (useCString) {
            OCFStringRef run = OCFStringCreateWithBytes(
                allocator, (const UInt8 *)cString + *mark, idx - *mark, kOCFStringEncodingISOLatin1, false
            );
            OCFStringAppend(*escapedString, run);
            OCFRelease(run);
        } else {
            OCFStringAppendCharacters(*escapedString, characters + *mark, idx - *mark);
        }
        if (_appendPercentEscapesForCharacter(&character, false, encoding, *escapedString)) {
            *mark = idx + 1;
        } else if (encoding == kOCFStringEncodingUTF8 && idx + 1 < end && (character & 0xfc00) == 0xd800) {
            UniChar next = useCString ? (UniChar)(int8_t)cString[idx + 1] : characters[idx + 1];
            if ((next & 0xfc00) == 0xdc00) {
                UniChar pair[2] = {character, next};
                if (_appendPercentEscapesForCharacter(pair, true, kOCFStringEncodingUTF8, *escapedString)) {
                    *mark = idx + 2;
                    idx++;
                }
            }
        }
        sawIllegalCharacter = true;
    }
    return sawIllegalCharacter;
}

// Native resolveAbsoluteURLStringBuffer, 0x1c024e024. Non-external. RFC 1808 resolution into the inlined
// append buffer, buf being the caller's work buffer: the base scheme and ':'; a relative string with a net
// location is appended whole. Otherwise "//" and the base net location (_netLocationRange) follow. A
// relative path is resolved: the relative path when the base has none, or when it starts with '/' or the
// base path is empty; the base path when the relative path is empty; otherwise the base path up to its last
// '/' and the relative path, in a CFAllocatorAllocateTyped block (2 * (base + relative) + 2 bytes, type
// 0x1000040BDFB0063, hint 0; when it fails the string is released and NULL answers) compacted by
// _resolvedPath. A '/' precedes it when the base path is absent or empty and it does not start with '/'; the
// relative string after the path follows. Without a relative path the base path follows; then with no
// parameter, query or fragment in the relative flags, the base string from the separator before its first
// resource specifier (the lowest of bits 6..8); with relative parameters, the relative string from their
// ';'; otherwise ';' and the base parameters, '?' and the relative query (else the base query), and '#' and
// the relative fragment.
static OCFStringRef resolveAbsoluteURLStringBuffer(
    OCFAllocatorRef allocator,
    OCFStringRef relativeString,
    UInt32 relativeFlags,
    const OCFRange *relativeRanges,
    OCFStringRef baseString,
    UInt32 baseFlags,
    const OCFRange *baseRanges,
    UniChar *buf
) {
    AppendBuffer appendBuffer;
    appendBufferInit(&appendBuffer, allocator);
    OCFRange range = (baseFlags & 1) != 0 ? baseRanges[0] : (OCFRange){kOCFNotFound, 0};
    if (range.location != kOCFNotFound) {
        OCFStringGetCharacters(baseString, range, buf);
        appendBufferAppendCharacters(&appendBuffer, buf, range.length);
        appendBufferAppendCharacter(&appendBuffer, ':');
    }
    if ((relativeFlags & 0x1e) != 0) {
        appendBufferAppendString(&appendBuffer, relativeString);
        return appendBufferCopyString(&appendBuffer);
    }
    const UniChar slashes[2] = {'/', '/'};
    appendBufferAppendCharacters(&appendBuffer, slashes, 2);
    range = _netLocationRange(baseFlags, baseRanges);
    if (range.location != kOCFNotFound) {
        OCFStringGetCharacters(baseString, range, buf);
        appendBufferAppendCharacters(&appendBuffer, buf, range.length);
    }
    if ((relativeFlags & 0x20) != 0) {
        OCFRange relativePath = relativeRanges[componentRangeIndex(relativeFlags, 0x20)];
        OCFRange basePath = (baseFlags & 0x20) != 0 ? baseRanges[componentRangeIndex(baseFlags, 0x20)]
                                                    : (OCFRange){kOCFNotFound, 0};
        OCFStringRef newPath;
        if (basePath.location == kOCFNotFound) {
            newPath = OCFStringCreateWithSubstring(allocator, relativeString, relativePath);
        } else if (relativePath.length == 0) {
            newPath = OCFStringCreateWithSubstring(allocator, baseString, basePath);
        } else if (
            OCFStringGetCharacterAtIndex(relativeString, relativePath.location) == '/' || basePath.length == 0
        ) {
            newPath = OCFStringCreateWithSubstring(allocator, relativeString, relativePath);
        } else {
            UniChar *pathBuffer = OCFAllocatorAllocateTyped(
                allocator,
                2 * (basePath.length + relativePath.length) + 2,
                (OCFAllocatorTypeID)UINT64_C(0x1000040BDFB0063),
                0
            );
            if (pathBuffer == NULL) {
                if (appendBuffer.string != NULL) {
                    OCFRelease(appendBuffer.string);
                }
                return NULL;
            }
            OCFStringGetCharacters(baseString, basePath, pathBuffer);
            // The native loop tests from the end down to the second character, then the first; the last '/'
            // found is kept (the first character only when it is '/').
            UniChar *idx = pathBuffer + basePath.length - 1;
            while (idx != pathBuffer && *idx != '/') {
                idx--;
            }
            if (*idx == '/') {
                idx++;
            }
            OCFStringGetCharacters(relativeString, relativePath, idx);
            UniChar *end = idx + relativePath.length;
            *end = 0;
            newPath = _resolvedPath(pathBuffer, end, '/', false, false, allocator);
        }
        if ((basePath.location == kOCFNotFound || basePath.length == 0) &&
            OCFStringGetCharacterAtIndex(newPath, 0) != '/') {
            appendBufferAppendCharacter(&appendBuffer, '/');
        }
        appendBufferAppendString(&appendBuffer, newPath);
        OCFRelease(newPath);
        range.location = relativePath.location + relativePath.length;
        range.length = OCFStringGetLength(relativeString) - range.location;
        if (range.length > 0) {
            OCFStringGetCharacters(relativeString, range, buf);
            appendBufferAppendCharacters(&appendBuffer, buf, range.length);
        }
        return appendBufferCopyString(&appendBuffer);
    }
    range = (baseFlags & 0x20) != 0 ? baseRanges[componentRangeIndex(baseFlags, 0x20)]
                                    : (OCFRange){kOCFNotFound, 0};
    if (range.location != kOCFNotFound) {
        OCFStringGetCharacters(baseString, range, buf);
        appendBufferAppendCharacters(&appendBuffer, buf, range.length);
    }
    if ((relativeFlags & 0x1c0) == 0) {
        UInt32 resourceFlag = 0;
        for (UInt32 bit = 0x100; bit != 0x20; bit >>= 1) {
            if ((bit & baseFlags) != 0) {
                resourceFlag = bit;
            }
        }
        if (resourceFlag != 0) {
            OCFIndex location = (resourceFlag & baseFlags) != 0
                                    ? baseRanges[componentRangeIndex(baseFlags, resourceFlag)].location
                                    : kOCFNotFound;
            OCFIndex remaining = OCFStringGetLength(baseString) - location;
            range = (OCFRange){location - 1, remaining + 1};
            OCFStringGetCharacters(baseString, range, buf);
            appendBufferAppendCharacters(&appendBuffer, buf, range.length);
        }
    } else if ((relativeFlags & 0x40) != 0) {
        range.location = relativeRanges[componentRangeIndex(relativeFlags, 0x40)].location - 1;
        range.length = OCFStringGetLength(relativeString) - range.location;
        OCFStringGetCharacters(relativeString, range, buf);
        appendBufferAppendCharacters(&appendBuffer, buf, range.length);
    } else {
        range = (baseFlags & 0x40) != 0 ? baseRanges[componentRangeIndex(baseFlags, 0x40)]
                                        : (OCFRange){kOCFNotFound, 0};
        if (range.location != kOCFNotFound) {
            appendBufferAppendCharacter(&appendBuffer, ';');
            OCFStringGetCharacters(baseString, range, buf);
            appendBufferAppendCharacters(&appendBuffer, buf, range.length);
        }
        range = (relativeFlags & 0x80) != 0 ? relativeRanges[componentRangeIndex(relativeFlags, 0x80)]
                                            : (OCFRange){kOCFNotFound, 0};
        if (range.location != kOCFNotFound) {
            appendBufferAppendCharacter(&appendBuffer, '?');
            OCFStringGetCharacters(relativeString, range, buf);
            appendBufferAppendCharacters(&appendBuffer, buf, range.length);
        } else {
            range = (baseFlags & 0x80) != 0 ? baseRanges[componentRangeIndex(baseFlags, 0x80)]
                                            : (OCFRange){kOCFNotFound, 0};
            if (range.location != kOCFNotFound) {
                appendBufferAppendCharacter(&appendBuffer, '?');
                OCFStringGetCharacters(baseString, range, buf);
                appendBufferAppendCharacters(&appendBuffer, buf, range.length);
            }
        }
        range = (relativeFlags & 0x100) != 0 ? relativeRanges[componentRangeIndex(relativeFlags, 0x100)]
                                             : (OCFRange){kOCFNotFound, 0};
        if (range.location != kOCFNotFound) {
            appendBufferAppendCharacter(&appendBuffer, '#');
            OCFStringGetCharacters(relativeString, range, buf);
            appendBufferAppendCharacters(&appendBuffer, buf, range.length);
        }
    }
    return appendBufferCopyString(&appendBuffer);
}

// Native WindowsPathToURLPath, 0x1c024ebdc. Non-external. An empty path answers an empty ASCII C-string
// copy. Otherwise every "/" becomes "\\" in a mutable copy, which is split at "\\" into a mutable array; a
// final empty component is dropped for a file. Each component (after a leading "X:" drive of an absolute
// path; the count is read once) is escaped as a Windows path
// (CreateStringFromFileSystemRepresentationByAddingPercentEscapes(kCFAllocatorDefault, UTF-8 form, false,
// false, true, NULL); a 1,024-byte stack buffer below 1,024 bytes of maximum UTF-8 size, otherwise an
// unchecked malloc_type_malloc block of the size plus one, type 0x100004077774924) and replaces the component
// when it is a different object; a failure releases the array and answers an empty ASCII C-string copy. A
// directory gains an empty final component unless the last one is empty, an absolute path gains "file://"
// first, and the components are joined with "/".
OCFStringRef
WindowsPathToURLPath(OCFStringRef path, OCFAllocatorRef allocator, Boolean isDirectory, Boolean isAbsolute) {
    if (OCFStringGetLength(path) == 0) {
        return OCFStringCreateWithCString(allocator, "", kOCFStringEncodingASCII);
    }
    OCFMutableStringRef mutablePath = OCFStringCreateMutableCopy(allocator, 0, path);
    OCFStringFindAndReplace(
        mutablePath, slashString(), backslashString(), (OCFRange){0, OCFStringGetLength(mutablePath)}, 0
    );
    OCFArrayRef separated =
        OCFStringCreateArrayBySeparatingStrings(allocator, mutablePath, backslashString());
    OCFRelease(mutablePath);
    OCFMutableArrayRef components = OCFArrayCreateMutableCopy(allocator, 0, separated);
    OCFRelease(separated);
    if (!isDirectory) {
        OCFIndex last = OCFArrayGetCount(components) - 1;
        if (OCFEqual(OCFArrayGetValueAtIndex(components, last), emptyString())) {
            OCFArrayRemoveValueAtIndex(components, last);
        }
    }
    OCFStringRef firstComponent = OCFArrayGetValueAtIndex(components, 0);
    OCFIndex index = 0;
    if (isAbsolute && OCFStringGetLength(firstComponent) == 2 &&
        OCFStringGetCharacterAtIndex(firstComponent, 1) == ':') {
        index = 1;
    }
    OCFIndex count = OCFArrayGetCount(components);
    for (; index < count; ++index) {
        OCFStringRef component = OCFArrayGetValueAtIndex(components, index);
        OCFIndex length = OCFStringGetLength(component);
        OCFIndex maximum =
            OCFStringGetMaximumSizeForEncoding((uint64_t)length > 1 ? length : 1, kOCFStringEncodingUTF8);
        char stackBuffer[1024];
        char *buffer;
        if (maximum >= 0x400) {
            buffer = malloc_type_malloc((size_t)maximum + 1, (malloc_type_id_t)UINT64_C(0x100004077774924));
        } else {
            buffer = stackBuffer;
        }
        OCFStringRef escaped;
        if (OCFStringGetCString(component, buffer, maximum + 1, kOCFStringEncodingUTF8)) {
            escaped = CreateStringFromFileSystemRepresentationByAddingPercentEscapes(
                kOCFAllocatorDefault,
                (const UInt8 *)buffer,
                (OCFIndex)strlen(buffer),
                false,
                false,
                true,
                NULL
            );
        } else {
            escaped = NULL;
        }
        if (buffer != stackBuffer) {
            free(buffer);
        }
        if (escaped == NULL) {
            OCFRelease(components);
            return OCFStringCreateWithCString(allocator, "", kOCFStringEncodingASCII);
        }
        if (escaped != component) {
            OCFArraySetValueAtIndex(components, index, escaped);
        }
        OCFRelease(escaped);
    }
    if (isDirectory &&
        OCFStringGetLength(OCFArrayGetValueAtIndex(components, OCFArrayGetCount(components) - 1)) != 0) {
        OCFArrayAppendValue(components, emptyString());
    }
    if (isAbsolute) {
        OCFArrayInsertValueAtIndex(components, 0, fileURLPrefixString());
    }
    if (components == NULL) {
        return OCFStringCreateWithCString(allocator, "", kOCFStringEncodingASCII);
    }
    OCFStringRef result = OCFStringCreateByCombiningStrings(allocator, components, slashString());
    OCFRelease(components);
    return result;
}

// Native CreateStringFromFileSystemRepresentationByAddingPercentEscapes, 0x1c024f090. Non-external. The
// bytes are copied after an optional "file://" (fileURLPrefix, strlcpy) into a stack buffer of 3,072 bytes
// plus 8 for the prefix and 1 for a directory, or for more than 1,024 bytes a malloc_type_malloc block of
// three bytes per byte plus the same extras (type 0x100004077774924; NULL answers NULL with no escaping
// recorded). Bytes outside sURLValidBytes bit 0, and '/' in a Windows path, become "%XY"; a NUL ends the copy
// and answers NULL unless only NULs follow. A directory then ends in "%2F" (Windows, when at least four bytes
// were written and they do not already) or '/' (POSIX, when the input does not end in '/'); a file has its
// trailing "%2F" (Windows, while more than one input byte remains and at least four bytes are written) or
// '/' runs (POSIX, keeping the first input byte) removed. The UTF-8 string of the result; escapingResult
// (optional) receives whether any byte was escaped.
OCFStringRef CreateStringFromFileSystemRepresentationByAddingPercentEscapes(
    OCFAllocatorRef allocator,
    const UInt8 *bytes,
    OCFIndex numBytes,
    Boolean isDirectory,
    Boolean isAbsolute,
    Boolean windowsPath,
    Boolean *escapingResult
) {
    const char *hexChars = CreateStringFromFileSystemRepresentationByAddingPercentEscapesHexChars;
    OCFIndex prefixLength = isAbsolute ? (OCFIndex)sizeof(fileURLPrefix) : 0;
    OCFIndex directoryLength = isDirectory ? 1 : 0;
    size_t bufferSize = (size_t)(0xc00 + prefixLength + directoryLength);
    UInt8 stackBuffer[bufferSize];
    UInt8 *buffer;
    Boolean escaped = false;
    OCFStringRef result = NULL;
    if (numBytes >= 0x401) {
        bufferSize = (size_t)(numBytes * 3 + prefixLength + directoryLength);
        buffer = malloc_type_malloc(bufferSize, (malloc_type_id_t)UINT64_C(0x100004077774924));
        if (buffer == NULL) {
            if (escapingResult != NULL) {
                *escapingResult = false;
            }
            return NULL;
        }
    } else {
        buffer = stackBuffer;
    }
    UInt8 *bufferPtr = buffer;
    if (isAbsolute) {
        strlcpy((char *)buffer, fileURLPrefix, bufferSize);
        bufferPtr = buffer + 7;
    }
    OCFIndex idx;
    for (idx = 0; idx < numBytes; ++idx) {
        UInt8 byte = bytes[idx];
        if (byte == 0) {
            break;
        }
        Boolean copy =
            windowsPath ? (byte != '/' && (sURLValidBytes[byte] & 1) != 0) : (sURLValidBytes[byte] & 1) != 0;
        if (copy) {
            *bufferPtr++ = byte;
        } else {
            bufferPtr[0] = '%';
            bufferPtr[1] = (UInt8)hexChars[byte >> 4];
            bufferPtr[2] = (UInt8)hexChars[byte & 0xf];
            bufferPtr += 3;
            escaped = true;
        }
    }
    Boolean valid = true;
    if (idx < numBytes || numBytes < 0) {
        for (; idx < numBytes; ++idx) {
            if (bytes[idx] != 0) {
                break;
            }
        }
        valid = idx == numBytes;
    }
    if (valid) {
        if (isDirectory) {
            if (windowsPath) {
                if (bufferPtr - buffer >= 4 &&
                    !(bufferPtr[-3] == '%' && bufferPtr[-2] == '2' && bufferPtr[-1] == 'F')) {
                    bufferPtr[0] = '%';
                    bufferPtr[1] = '2';
                    bufferPtr[2] = 'F';
                    bufferPtr += 3;
                }
            } else if (numBytes >= 1 && bytes[numBytes - 1] != '/') {
                *bufferPtr++ = '/';
            }
        } else if (windowsPath) {
            if (numBytes >= 2 && bufferPtr - buffer >= 4) {
                OCFIndex length = bufferPtr - buffer - 3;
                while (bufferPtr[-3] == '%' && bufferPtr[-2] == '2' && bufferPtr[-1] == 'F') {
                    bufferPtr -= 3;
                    if (numBytes < 3) {
                        break;
                    }
                    numBytes--;
                    if (length < 4) {
                        break;
                    }
                    length -= 3;
                }
            }
        } else {
            while (numBytes > 1 && bytes[numBytes - 1] == '/') {
                bufferPtr--;
                numBytes--;
            }
        }
        result =
            OCFStringCreateWithBytes(allocator, buffer, bufferPtr - buffer, kOCFStringEncodingUTF8, false);
    }
    if (buffer != stackBuffer) {
        free(buffer);
    }
    if (escapingResult != NULL) {
        *escapingResult = escaped;
    }
    return result;
}

// Native __getLastPathComponentRangeFromString, 0x1c024f440. Non-external. Ranges shorter than two characters
// answer themselves. Otherwise a final '/' is excluded and the component starts after the last '/' before it
// (at the range's start when there is none).
OCFRange __getLastPathComponentRangeFromString(OCFStringRef string, OCFIndex location, OCFIndex length) {
    if (length < 2) {
        return (OCFRange){location, length};
    }
    OCFIndex end = location + length;
    if (OCFStringGetCharacterAtIndex(string, end - 1) == '/') {
        end = end - 1;
    }
    OCFIndex start = location;
    for (OCFIndex index = end; index > location; --index) {
        if (OCFStringGetCharacterAtIndex(string, index - 1) == '/') {
            start = index;
            break;
        }
    }
    return (OCFRange){start, end - start};
}

// Native createFileReferencePath, 0x1c024f590 (corpus delta capture 7). Non-external. A zero volume ID
// answers NULL. With a name: its UTF-8 form (a 1,024-byte buffer; a failure answers NULL) makes
// "/.file/id=volume.parentID/name" (with a trailing '/' for a directory); without one,
// "/.file/id=volume.fileID" (likewise). A result of snprintf above 1,023 (unsigned) answers NULL; otherwise
// the 1,024-byte buffer becomes a UTF-8 string on the allocator.
static OCFStringRef createFileReferencePath(
    OCFAllocatorRef allocator,
    long long volumeID,
    long long parentID,
    long long fileID,
    OCFStringRef name,
    Boolean isDirectory
) {
    char path[1024];
    if (volumeID == 0) {
        return NULL;
    }
    if (name != NULL) {
        char nameBuffer[1024];
        if (!OCFStringGetCString(name, nameBuffer, 1024, kOCFStringEncodingUTF8)) {
            return NULL;
        }
        int written = snprintf(
            path, 1024, "/.file/id=%lld.%lld/%s%s", volumeID, parentID, nameBuffer, isDirectory ? "/" : ""
        );
        if ((unsigned)written > 0x3FF) {
            return NULL;
        }
        return OCFStringCreateWithCString(allocator, path, kOCFStringEncodingUTF8);
    }
    int written = snprintf(path, 1024, "/.file/id=%lld.%lld%s", volumeID, fileID, isDirectory ? "/" : "");
    if ((unsigned)written > 0x3FF) {
        return NULL;
    }
    return OCFStringCreateWithCString(allocator, path, kOCFStringEncodingUTF8);
}
