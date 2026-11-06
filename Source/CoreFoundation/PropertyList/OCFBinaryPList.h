//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/PropertyList/OCFPropertyList.h"

#include <os/log.h>
#include <stdbool.h>
#include <stdint.h>

// Binary property-list trailer, as __CFBinaryPlistGetTopLevelInfo (iOS Simulator 24A434, 0x1c0163b94,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified) fills it: the six leading bytes of
// the file trailer copied unchanged (a 4-byte store at 0, a 2-byte store at 4), the offset-table entry
// width (byte at 6), the object-reference width (byte at 7), the object count (64 bits at 8), the top
// object index (64 bits at 16) and the offset-table offset (64 bits at 24), all host-endian.
// __CFBinaryPlistGetOffsetForValueFromArray2 reads the widths, the count and the table offset. The 32
// bytes are the extent of the fields written; BinaryPlistTrailer and its field names are local
// descriptive spellings.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
typedef struct {
    uint8_t unused[6];
    uint8_t offsetIntSize;
    uint8_t objectRefSize;
    uint64_t numObjects;
    uint64_t topObject;
    uint64_t offsetTableOffset;
} BinaryPlistTrailer;

// The callers' string blocks receive the first character byte and the character count (bytes for
// ASCII, UTF-16 units for Unicode16) and return the parse result.
typedef bool (^BinaryPlistStringBlock)(const uint8_t *characters, uint64_t count);

OF_EXPORT bool __OCFBinaryPlistGetOffsetForValueFromArray2(
    const uint8_t *databytes,
    uint64_t length,
    uint64_t startOffset,
    const BinaryPlistTrailer *trailer,
    OCFIndex index,
    uint64_t *offset,
    const void *ignored
);

OF_EXPORT bool __OCFBinaryPlistParseASCIIString(
    const uint8_t *base, uint64_t limit, const uint8_t *marker, BinaryPlistStringBlock block
);

OF_EXPORT bool __OCFBinaryPlistParseUnicode16String(
    const uint8_t *base, uint64_t limit, const uint8_t *marker, BinaryPlistStringBlock block
);

OF_EXPORT bool __OCFBinaryPlistGetTopLevelInfo(
    const uint8_t *databytes, uint64_t datalen, uint8_t *marker, uint64_t *offset, BinaryPlistTrailer *trailer
);

// Binary property-list functions that other units call. Signatures read from the native definitions and their
// call sites (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified).
// estimate and options are not read by the native writer. curDepth is compared unsigned. backingData is the
// CFData whose bytes are parsed when the caller asks for mapped strings (option bit 16 at
// _CFPropertyListCreateFiltered), otherwise NULL; it is passed with the string's object offset to
// __NSCreateBPlistMappedString.
OF_EXTERN OCFIndex __CFBinaryPlistWriteOrPresize(
    OCFPropertyListRef plist,
    OCFTypeRef stream,
    uint64_t estimate,
    OCFOptionFlags options,
    bool presize,
    OCFErrorRef *error
);
OF_EXPORT OCFIndex __OCFBinaryPlistWrite(
    OCFPropertyListRef plist, OCFTypeRef stream, uint64_t estimate, OCFOptionFlags options, OCFErrorRef *error
);
OF_EXTERN OCFDataRef __CFBinaryPlistCreateDataUsingExternalBufferAllocator(
    OCFPropertyListRef plist,
    uint64_t estimate,
    OCFOptionFlags options,
    OCFAllocatorRef (^makeAllocator)(size_t size),
    OCFErrorRef *error
);

OF_EXPORT OCFIndex __OCFBinaryPlistWriteToStreamWithOptions(
    OCFPropertyListRef plist, OCFTypeRef stream, uint64_t estimate, OCFOptionFlags options
);
OF_EXTERN OCFSetRef __CFBinaryPlistCopyTopLevelKeys(
    OCFAllocatorRef allocator,
    const uint8_t *databytes,
    uint64_t datalen,
    uint64_t offset,
    const BinaryPlistTrailer *trailer
);
OF_EXTERN bool __CFBinaryPlistCreateObjectFiltered(
    const uint8_t *databytes,
    uint64_t datalen,
    uint64_t startOffset,
    const BinaryPlistTrailer *trailer,
    OCFAllocatorRef allocator,
    OCFOptionFlags mutabilityOption,
    OCFDataRef backingData,
    OCFMutableDictionaryRef objects,
    OCFMutableSetRef set,
    uint64_t curDepth,
    OCFSetRef keyPaths,
    OCFPropertyListRef *plist,
    OCFTypeID *outPlistTypeID
);
OF_EXPORT bool __OCFBinaryPlistGetOffsetForValueFromDictionary3(
    const uint8_t *bytes,
    uint64_t length,
    uint64_t offset,
    const BinaryPlistTrailer *trailer,
    OCFTypeRef key,
    uint64_t *keyOffset,
    uint64_t *valueOffset,
    const void *unused,
    OCFMutableDictionaryRef objects
);
OF_EXPORT bool __OCFBinaryPlistCreateObject(
    const uint8_t *bytes,
    uint64_t length,
    uint64_t offset,
    const BinaryPlistTrailer *trailer,
    OCFAllocatorRef allocator,
    OCFOptionFlags options,
    OCFMutableDictionaryRef objects,
    OCFPropertyListRef *object
);
OF_EXTERN bool __CFTryParseBinaryPlist(
    OCFAllocatorRef allocator,
    OCFDataRef data,
    OCFOptionFlags option,
    OCFPropertyListRef *plist,
    OCFStringRef *errorString
);

// The CFData unit's top-level reader (Collections/OCFData.c; non-external, was a private external). Declared
// here because its trailer type is this unit's: the CFData header cannot include this one without a cycle.
OF_EXTERN bool __CFDataGetBinaryPlistTopLevelInfo(
    OCFDataRef data, uint8_t *marker, uint64_t *offset, BinaryPlistTrailer *trailer
);

// The write buffer of the binary writer, as __CFBinaryPlistWriteOrPresize initializes it (an 8208-byte typed
// allocation; iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for the
// offsets the writer stores: stream 0, the zeroed words at 8 and 16, error 24, written 32, used 40 (32-bit,
// sign-extended at every read), streamIsData 44 and the 8160-byte buffer at 45). The words at 8 and 16 are a
// fixed output buffer and its capacity, as writeBytes reads them. BinaryPlistWriteBuffer and its field names
// are local descriptive spellings.
typedef struct {
    OCFTypeRef stream;
    uint8_t *databytes;
    uint64_t datalen;
    OCFErrorRef error;
    uint64_t written;
    int32_t used;
    bool streamIsData;
    uint8_t buffer[8160];
} BinaryPlistWriteBuffer;

_Static_assert(sizeof(BinaryPlistWriteBuffer) == 0x2010, "The native write buffer is 8208 bytes.");

// Foundation (external there, imported by the native image): a string mapped from backingData at the string
// object's offset. Parameter roles from the one call site (medium confidence).
OF_EXPORT OCFStringRef __ONSCreateBPlistMappedString(uint64_t offset, OCFDataRef backingData);
