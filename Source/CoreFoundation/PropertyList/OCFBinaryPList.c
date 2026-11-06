//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PropertyList/OCFBinaryPList.h"

#include "CoreFoundation/Collections/OCFBasicHash.h"
#include "CoreFoundation/PropertyList/OCFKeyedArchiverUID.h"

#include <malloc/malloc.h>
#include <os/log.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Inferred organization: the native binary property-list writer and reader lie in one run of the iOS
// Simulator
// __text section, from __CFBinaryPlistWriteOrPresize (0x1c0162a50) through
// ____CFPropertyListTryFilteredDictionaryASCIIFastPath_block_invoke (0x1c0167ac8), with the cold fragment
// __CFBinaryPlistCreateObjectFiltered.cold.1 (0x1c03063ac) out of line; they are kept in one translation unit
// and the restored definitions follow native address order except __CFBinaryPlistGetTopLevelInfo, which an
// earlier unit placed last. The native source path is not in the evidence. Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// (disassembly); iOS Device 24A446: unread for the definitions added by the plist sub-track. Reading:
// Local/Scratch/cf-core/tasks/plist.txt (sub-track reading; no versioned reading yet).

// The binary-plist type IDs the native code stores as literals: CFString 7, CFNull 16, CFSet 17, CFDictionary
// 18, CFArray 19, CFData 20, CFBoolean 21, CFNumber 22, CFKeyedArchiverUID 41, CFDate 42.
enum {
    kBinaryPlistStringTypeID = 7,
    kBinaryPlistNullTypeID = 16,
    kBinaryPlistSetTypeID = 17,
    kBinaryPlistDictionaryTypeID = 18,
    kBinaryPlistArrayTypeID = 19,
    kBinaryPlistDataTypeID = 20,
    kBinaryPlistBooleanTypeID = 21,
    kBinaryPlistNumberTypeID = 22,
    kBinaryPlistUIDTypeID = 41,
    kBinaryPlistDateTypeID = 42,
};

// Allocation type descriptors the native call sites pass.
#define kBinaryPlistOffsetsTypeID UINT64_C(0x0100004000313F17)
#define kBinaryPlistWriteBufferTypeID UINT64_C(0x010E0040DB85EAC6)
#define kBinaryPlistObjectListTypeID UINT64_C(0x000C0040B8AA526D)
#define kBinaryPlistTopLevelKeysTypeID UINT64_C(0x0006004044C4A2DF)

static OCFMutableDictionaryRef __CFPropertyListCreateFilteredDictionary(
    OCFAllocatorRef allocator,
    OCFSetRef keyPaths,
    const uint8_t *databytes,
    uint64_t datalen,
    uint64_t startOffset,
    const BinaryPlistTrailer *trailer,
    bool skipValues,
    OCFOptionFlags mutability,
    OCFDataRef backingData,
    OCFMutableDictionaryRef objects,
    OCFMutableSetRef set,
    uint64_t curDepth
);

static void bufferWrite(BinaryPlistWriteBuffer *buf, const uint8_t *buffer, OCFIndex count, bool presize);
static void writeBytes(BinaryPlistWriteBuffer *buf, const uint8_t *bytes, OCFIndex length, bool presize);
static bool _appendObject(
    BinaryPlistWriteBuffer *buf,
    OCFPropertyListRef obj,
    OCFDictionaryRef objtable,
    uint8_t objRefSize,
    bool presize
);
static void _appendString(BinaryPlistWriteBuffer *buf, OCFStringRef string, bool presize);
static void _appendInt(BinaryPlistWriteBuffer *buf, uint64_t value, bool presize);
static void _flattenPlist(
    OCFPropertyListRef plist,
    OCFMutableArrayRef objlist,
    OCFMutableDictionaryRef objtable,
    OCFMutableSetRef uniquingSet
);

// The helpers below reproduce code the native functions inline at each use (no native symbols).
//
// The big-endian value of width bytes. The native code switches on the width (1, 2, 4 and 8 with LDR + REV
// loads, a byte loop otherwise, and no read for width 0); every case yields this byte loop's value.
static uint64_t readBigEndian(const uint8_t *bytes, uint64_t width) {
    uint64_t value = 0;
    for (uint64_t byte = 0; byte < width; ++byte) {
        value = (value << 8) | bytes[byte];
    }
    return value;
}

// The integer width the writer chooses for a value: the bytes that hold it, then the next of 1, 2 and 4, or 8
// for anything wider (the count loop and the normalization loop __CFBinaryPlistWriteOrPresize repeats for the
// object-reference and offset widths).
static uint8_t bytesForValue(uint64_t value) {
    uint8_t count = 0;
    if (value != 0) {
        uint64_t mask = UINT64_MAX;
        bool more = false;
        do {
            ++count;
            more = (value & (mask << 8)) != 0;
            mask <<= 8;
        } while (more);
        if (count > 7) {
            return count;
        }
    }
    for (;;) {
        uint8_t current = count;
        if (current <= 4 && ((1u << current) & 0x16) != 0) {
            break;
        }
        ++count;
        if (current >= 7) {
            count = 8;
            break;
        }
    }
    return count;
}

// Native __CFBinaryPlistWriteOrPresize, 0x1c0162a50. Non-external (was a private external).
// estimate (X2) and options (X3) are not read. The object table is a CFBasicHash created with flags 0xA001
// and ten null callbacks, then given type ID 18 (a CFDictionary); the object list and the uniquing set use
// the CFType retain and release callbacks only. The reference width is chosen from the object count and the
// offset width from the offset of the offset table (bytesForValue). The trailer is built big-endian in a
// local and written after the offset table; each offset is written as its low width bytes. On success the
// result is offsetTableOffset + count * offsetWidth + 32, or 0 when the buffer recorded an error (stored
// through error, or released when error is NULL). When _appendObject fails the error is handled the same way
// and the result is 0. The offsets array is freed before the trailer is written on success and after the
// buffer on failure.
OCFIndex __CFBinaryPlistWriteOrPresize(
    OCFPropertyListRef plist,
    OCFTypeRef stream,
    uint64_t estimate,
    OCFOptionFlags options,
    bool presize,
    OCFErrorRef *error
) {
    (void)estimate;
    (void)options;
    BinaryPlistTrailer trailer;
    BasicHashCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    OCFMutableDictionaryRef objtable =
        (OCFMutableDictionaryRef)CFBasicHashCreate(kOCFAllocatorSystemDefault, 0xA001, &callbacks);
    _OCFRuntimeSetInstanceTypeIDAndIsa(objtable, kBinaryPlistDictionaryTypeID);
    OCFArrayCallBacks arrayCallBacks = {0, __CFTypeCollectionRetain, __CFTypeCollectionRelease, NULL, NULL};
    OCFMutableArrayRef objlist = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &arrayCallBacks);
    OCFSetCallBacks setCallBacks = {0, __CFTypeCollectionRetain, __CFTypeCollectionRelease, NULL, NULL, NULL};
    OCFMutableSetRef uniquingSet = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &setCallBacks);
    _flattenPlist(plist, objlist, objtable, uniquingSet);
    OCFRelease(uniquingSet);
    OCFIndex count = OCFArrayGetCount(objlist);
    uint64_t *offsets = OCFAllocatorAllocateTyped(
        kOCFAllocatorSystemDefault, count * (OCFIndex)sizeof(uint64_t), kBinaryPlistOffsetsTypeID, 0
    );
    BinaryPlistWriteBuffer *buf = OCFAllocatorAllocateTyped(
        kOCFAllocatorSystemDefault, sizeof(BinaryPlistWriteBuffer), kBinaryPlistWriteBufferTypeID, 0
    );
    buf->stream = stream;
    buf->databytes = NULL;
    buf->datalen = 0;
    buf->error = NULL;
    if (stream != NULL) {
        buf->streamIsData = OCFGetTypeID(stream) == OCFDataGetTypeID();
    } else {
        buf->streamIsData = true;
    }
    buf->written = 0;
    buf->used = 0;
    bufferWrite(buf, (const uint8_t *)"bplist00", 8, presize);
    memset(&trailer, 0, sizeof(trailer));
    trailer.numObjects = __builtin_bswap64((uint64_t)count);
    uint8_t objRefSize = bytesForValue((uint64_t)count);
    trailer.objectRefSize = objRefSize;
    for (OCFIndex idx = 0; idx < count; ++idx) {
        offsets[idx] = buf->written + (int64_t)buf->used;
        OCFPropertyListRef obj = OCFArrayGetValueAtIndex(objlist, idx);
        if (!_appendObject(buf, obj, objtable, objRefSize, presize)) {
            OCFRelease(objtable);
            OCFRelease(objlist);
            OCFErrorRef recorded = buf->error;
            if (error != NULL) {
                if (recorded != NULL) {
                    *error = recorded;
                }
            } else if (recorded != NULL) {
                OCFRelease(recorded);
            }
            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, buf);
            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, offsets);
            return 0;
        }
    }
    OCFRelease(objtable);
    OCFRelease(objlist);
    uint64_t offsetTableOffset = buf->written + (int64_t)buf->used;
    trailer.offsetTableOffset = __builtin_bswap64(offsetTableOffset);
    uint8_t offsetIntSize = bytesForValue(offsetTableOffset);
    trailer.offsetIntSize = offsetIntSize;
    for (OCFIndex idx = 0; idx < count; ++idx) {
        uint64_t swapped = __builtin_bswap64(offsets[idx]);
        bufferWrite(buf, (const uint8_t *)&swapped + sizeof(swapped) - offsetIntSize, offsetIntSize, presize);
    }
    OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, offsets);
    bufferWrite(buf, (const uint8_t *)&trailer, sizeof(trailer), presize);
    writeBytes(buf, buf->buffer, buf->used, presize);
    buf->used = 0;
    OCFIndex result = 0;
    OCFErrorRef recorded = buf->error;
    if (recorded == NULL) {
        result = (OCFIndex)(offsetTableOffset + (uint64_t)count * offsetIntSize + 32);
    } else if (error != NULL) {
        *error = recorded;
    } else {
        OCFRelease(recorded);
    }
    OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, buf);
    return result;
}

// Native _flattenPlist, 0x1c0162e64. Non-external.
// Strings, data, numbers and dates (type IDs 7, 20, 22 and 42: the mask 0x40000500080 for IDs up to 42) are
// uniqued through uniquingSet: a value already present reuses the reference number of the equal object (the
// dictionary value truncated to 32 bits), or nothing happens when it is that object. Any other object gets
// the next reference number (the list count, truncated to 32 bits) and is appended, and arrays (19) and
// dictionaries (18) recurse into their values (dictionaries keys first). Up to 256 array values or 128
// dictionary entries are read into a stack buffer whose first 512 bytes at most are zeroed; larger
// collections use an unchecked typed allocation of the system default allocator.
static void _flattenPlist(
    OCFPropertyListRef plist,
    OCFMutableArrayRef objlist,
    OCFMutableDictionaryRef objtable,
    OCFMutableSetRef uniquingSet
) {
    OCFTypeID type = OCFGetTypeID(plist);
    if (type <= 42 && ((UINT64_C(1) << type) & UINT64_C(0x40000500080)) != 0) {
        OCFIndex before = OCFSetGetCount(uniquingSet);
        OCFSetAddValue(uniquingSet, plist);
        if (OCFSetGetCount(uniquingSet) == before) {
            const void *unique = OCFSetGetValue(uniquingSet, plist);
            if (unique != plist) {
                const void *refnum = OCFDictionaryGetValue(objtable, unique);
                OCFDictionaryAddValue(objtable, plist, (const void *)(uintptr_t)(uint32_t)(uintptr_t)refnum);
            }
            return;
        }
    }
    OCFIndex refnum = OCFArrayGetCount(objlist);
    OCFArrayAppendValue(objlist, plist);
    OCFDictionaryAddValue(objtable, plist, (const void *)(uintptr_t)(uint32_t)refnum);
    if (type == kBinaryPlistArrayTypeID) {
        OCFIndex count = OCFArrayGetCount((OCFArrayRef)plist);
        OCFIndex stackCount = (uint64_t)(count - 1) < 0x100 ? count : 1;
        const void *buffer[stackCount];
        size_t bufferBytes = (size_t)stackCount * sizeof(void *);
        bzero(buffer, bufferBytes < 0x200 ? bufferBytes : 0x200);
        const void **list = buffer;
        if (count >= 0x101) {
            list = OCFAllocatorAllocateTyped(
                kOCFAllocatorSystemDefault, count * (OCFIndex)sizeof(void *), kBinaryPlistObjectListTypeID, 0
            );
        }
        OCFArrayGetValues((OCFArrayRef)plist, (OCFRange){0, count}, list);
        for (OCFIndex idx = 0; idx < count; ++idx) {
            _flattenPlist(list[idx], objlist, objtable, uniquingSet);
        }
        if (list != buffer) {
            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, list);
        }
    } else if (type == kBinaryPlistDictionaryTypeID) {
        OCFIndex count = OCFDictionaryGetCount((OCFDictionaryRef)plist);
        OCFIndex stackCount = (uint64_t)(count - 1) < 0x80 ? 2 * count : 1;
        const void *buffer[stackCount];
        size_t bufferBytes = (size_t)stackCount * sizeof(void *);
        bzero(buffer, bufferBytes < 0x200 ? bufferBytes : 0x200);
        const void **list = buffer;
        if (count >= 0x81) {
            list = OCFAllocatorAllocateTyped(
                kOCFAllocatorSystemDefault,
                count * 2 * (OCFIndex)sizeof(void *),
                kBinaryPlistObjectListTypeID,
                0
            );
        }
        OCFDictionaryGetKeysAndValues((OCFDictionaryRef)plist, list, list + count);
        for (OCFIndex idx = 0; idx < 2 * count; ++idx) {
            _flattenPlist(list[idx], objlist, objtable, uniquingSet);
        }
        if (list != buffer) {
            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, list);
        }
    }
}

// Native bufferWrite, 0x1c01631b4. Non-external (a namesake of another native unit is at 0x1c02fa47c).
// Nothing is done for zero bytes. 8160 bytes or more flush the buffer and go to writeBytes directly.
// Otherwise up to the buffer's room is copied (one to four bytes one at a time, more with memmove) unless
// presizing or there is neither a stream nor a fixed output buffer; the used count grows by the bytes taken
// and a full buffer is flushed, the rest of the bytes moved to its start (under the same condition) and the
// used count set to that rest (32-bit).
static void bufferWrite(BinaryPlistWriteBuffer *buf, const uint8_t *buffer, OCFIndex count, bool presize) {
    if (count == 0) {
        return;
    }
    if (count >= 0x1FE0) {
        writeBytes(buf, buf->buffer, buf->used, presize);
        buf->used = 0;
        writeBytes(buf, buffer, count, presize);
        return;
    }
    OCFIndex room = 0x1FE0 - (OCFIndex)buf->used;
    OCFIndex copyLength = count < room ? count : room;
    bool copies = !presize && (buf->stream != NULL || buf->databytes != NULL);
    if (copies) {
        switch (copyLength) {
            case 4:
                buf->buffer[buf->used + 3] = buffer[3];
                [[fallthrough]];
            case 3:
                buf->buffer[buf->used + 2] = buffer[2];
                [[fallthrough]];
            case 2:
                buf->buffer[buf->used + 1] = buffer[1];
                [[fallthrough]];
            case 1:
                buf->buffer[buf->used] = buffer[0];
                break;
            default:
                memmove(buf->buffer + buf->used, buffer, (size_t)copyLength);
                break;
        }
    }
    buf->used = (int32_t)((uint32_t)buf->used + (uint32_t)copyLength);
    if (buf->used != 0x1FE0) {
        return;
    }
    writeBytes(buf, buf->buffer, 0x1FE0, presize);
    if (!presize && (buf->stream != NULL || buf->databytes != NULL)) {
        memmove(buf->buffer, buffer + copyLength, (size_t)(count - copyLength));
    }
    buf->used = (int32_t)((uint32_t)count - (uint32_t)copyLength);
}

// Native _appendObject, 0x1c0163338. Non-external. Writes one object's record. Strings go to _appendString;
// dictionaries (marker 0xd0) and arrays (0xa0) write their count (low nibble, or 0xf and an integer from 15
// up) and then, for every key and then every value (arrays: every value), the reference from objtable (the
// value's low 32 bits, big-endian, its low objRefSize bytes) or, without a table, the object itself (a
// failure frees a heap buffer and answers false). Up to 256 values (128 entries) use a 4096-byte stack buffer
// whose first 512 bytes are zeroed, more an unchecked typed allocation (0xc0040b8aa526d) of the system
// default allocator. Data (0x40) writes its length and bytes; booleans 0x08 or 0x09; floating numbers 0x22
// with the big-endian Float32 for byte sizes up to 4, else 0x23 with the Float64; 128-bit integers 0x14 with
// both halves byte-swapped in place; other numbers the SInt64 value through _appendInt; keyed-archiver UIDs
// 0x80 | (width - 1) with the 1, 2 or 4 low big-endian bytes of the value; dates 0x33 with the big-endian
// absolute time. Any other type answers false.
static bool _appendObject(
    BinaryPlistWriteBuffer *buf,
    OCFPropertyListRef obj,
    OCFDictionaryRef objtable,
    uint8_t objRefSize,
    bool presize
) {
    const void *stackValues[512];
    const void **list = stackValues;
    OCFIndex listCount;
    OCFTypeID type = OCFGetTypeID(obj);
    switch (type) {
        case kBinaryPlistStringTypeID:
            _appendString(buf, obj, presize);
            return true;
        case kBinaryPlistDictionaryTypeID: {
            OCFIndex count = OCFDictionaryGetCount(obj);
            uint8_t marker = (uint8_t)((count < 15 ? count : 15) | 0xD0);
            bufferWrite(buf, &marker, 1, presize);
            if (count >= 15) {
                _appendInt(buf, (uint64_t)count, presize);
            }
            memset(stackValues, 0, 0x200);
            if ((uint64_t)count >= 0x101) {
                list = OCFAllocatorAllocateTyped(
                    kOCFAllocatorSystemDefault, count << 4, kBinaryPlistObjectListTypeID, 0
                );
            }
            OCFDictionaryGetKeysAndValues(obj, list, list + count);
            if (count < 1) {
                return true;
            }
            listCount = 2 * count > 1 ? 2 * count : 1;
            break;
        }
        case kBinaryPlistArrayTypeID: {
            OCFIndex count = OCFArrayGetCount(obj);
            memset(stackValues, 0, 0x200);
            uint8_t marker = (uint8_t)((count < 15 ? count : 15) | 0xA0);
            bufferWrite(buf, &marker, 1, presize);
            if (count >= 15) {
                _appendInt(buf, (uint64_t)count, presize);
                if ((uint64_t)count >= 0x101) {
                    list = OCFAllocatorAllocateTyped(
                        kOCFAllocatorSystemDefault, count << 3, kBinaryPlistObjectListTypeID, 0
                    );
                }
            }
            OCFArrayGetValues(obj, __OCFRangeMake(0, count), list);
            if (count < 1) {
                return true;
            }
            listCount = count;
            break;
        }
        case kBinaryPlistDataTypeID: {
            OCFIndex length = OCFDataGetLength(obj);
            uint8_t marker = (uint8_t)((length < 15 ? length : 15) | 0x40);
            bufferWrite(buf, &marker, 1, presize);
            if (length >= 15) {
                _appendInt(buf, (uint64_t)length, presize);
            }
            bufferWrite(buf, OCFDataGetBytePtr(obj), length, presize);
            return true;
        }
        case kBinaryPlistBooleanTypeID: {
            uint8_t marker = OCFBooleanGetValue(obj) ? 9 : 8;
            bufferWrite(buf, &marker, 1, presize);
            return true;
        }
        case kBinaryPlistNumberTypeID: {
            if (OCFNumberIsFloatType(obj)) {
                uint8_t marker;
                const uint8_t *bytes;
                OCFIndex size;
                uint32_t swapped32 = 0;
                uint64_t swapped64 = 0;
                if (OCFNumberGetByteSize(obj) <= 4) {
                    uint32_t value = 0;
                    OCFNumberGetValue(obj, 5, &value);
                    swapped32 = __builtin_bswap32(value);
                    bytes = (const uint8_t *)&swapped32;
                    marker = 0x22;
                    size = 4;
                } else {
                    uint64_t value = 0;
                    OCFNumberGetValue(obj, 6, &value);
                    swapped64 = __builtin_bswap64(value);
                    bytes = (const uint8_t *)&swapped64;
                    marker = 0x23;
                    size = 8;
                }
                bufferWrite(buf, &marker, 1, presize);
                bufferWrite(buf, bytes, size, presize);
                return true;
            }
            if (_OCFNumberGetType2(obj) == 17) {
                uint64_t value[2] = {0, 0};
                OCFNumberGetValue(obj, 17, value);
                uint64_t swapped[2] = {__builtin_bswap64(value[0]), __builtin_bswap64(value[1])};
                uint8_t marker = 0x14;
                bufferWrite(buf, &marker, 1, presize);
                bufferWrite(buf, (const uint8_t *)swapped, 16, presize);
                return true;
            }
            int64_t value = 0;
            OCFNumberGetValue(obj, 4, &value);
            _appendInt(buf, (uint64_t)value, presize);
            return true;
        }
        case kBinaryPlistUIDTypeID: {
            uint32_t value = _OCFKeyedArchiverUIDGetValue((OCFKeyedArchiverUIDRef)obj);
            OCFIndex size = value >= 0x100 ? (value < 0x10000 ? 2 : 4) : 1;
            uint8_t marker = (uint8_t)((size - 1) | 0x80);
            uint64_t swapped = __builtin_bswap64((uint64_t)value);
            bufferWrite(buf, &marker, 1, presize);
            bufferWrite(buf, (const uint8_t *)&swapped + 8 - size, size, presize);
            return true;
        }
        case kBinaryPlistDateTypeID: {
            uint8_t marker = 0x33;
            bufferWrite(buf, &marker, 1, presize);
            double time = OCFDateGetAbsoluteTime(obj);
            uint64_t bits;
            memcpy(&bits, &time, sizeof(bits));
            uint64_t swapped = __builtin_bswap64(bits);
            bufferWrite(buf, (const uint8_t *)&swapped, 8, presize);
            return true;
        }
        default:
            return false;
    }
    for (OCFIndex idx = 0; idx != listCount; ++idx) {
        if (objtable != NULL) {
            uint32_t refnum = (uint32_t)(uintptr_t)OCFDictionaryGetValue(objtable, list[idx]);
            uint32_t swapped = __builtin_bswap32(refnum);
            bufferWrite(buf, (const uint8_t *)&swapped + 4 - objRefSize, objRefSize, presize);
        } else if (!_appendObject(buf, list[idx], NULL, objRefSize, presize)) {
            if (list != stackValues) {
                OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, list);
            }
            return false;
        }
    }
    if (list != stackValues) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, list);
    }
    return true;
}

// Native __CFBinaryPlistWrite, 0x1c0163a30. External; tail-calls __CFBinaryPlistWriteOrPresize with presize
// false.
OCFIndex __OCFBinaryPlistWrite(
    OCFPropertyListRef plist, OCFTypeRef stream, uint64_t estimate, OCFOptionFlags options, OCFErrorRef *error
) {
    return __CFBinaryPlistWriteOrPresize(plist, stream, estimate, options, false, error);
}

// Native __CFBinaryPlistWriteToStreamWithOptions, 0x1c0163a54. External. iOS Device divergence: unread.
OCFIndex __OCFBinaryPlistWriteToStreamWithOptions(
    OCFPropertyListRef plist, OCFTypeRef stream, uint64_t estimate, OCFOptionFlags options
) {
    return __CFBinaryPlistWriteOrPresize(plist, stream, estimate, options, false, NULL);
}

// Native __CFBinaryPlistCreateDataUsingExternalBufferAllocator, 0x1c0163a60. Hidden (was private external).
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC, confidence: verified.
// iOS Device 24A446: unread. Reading: Local/Evidence/cf-core/iphone-simulator-corpus-delta13.json.
// The write pass leaves X2/X3 unspecified, which the writer never reads. The assembly establishes the
// allocator block's ABI and typed allocation's zero hint, which the pseudocode omits.
// CoreFoundation defect: on data creation failure at 0x1c0163b4c, the native path frees the external buffer
// but leaks the allocator (+1 from 0x1c0163aa8). Verified. Release it after deallocating the buffer.
OCFDataRef __CFBinaryPlistCreateDataUsingExternalBufferAllocator(
    OCFPropertyListRef plist,
    uint64_t estimate,
    OCFOptionFlags options,
    OCFAllocatorRef (^makeAllocator)(size_t size),
    OCFErrorRef *error
) {
    OCFIndex length = __CFBinaryPlistWriteOrPresize(plist, NULL, estimate, options, true, error);
    if (length < 1) {
        return NULL;
    }
    OCFAllocatorRef allocator = makeAllocator((size_t)length);
    if (allocator == NULL) {
        if (error != NULL) {
            *error = __CFPropertyListCreateError(
                kOCFPropertyListWriteStreamError,
                OF_STR(
                    "Binary property list writing could not be completed because an allocator could not be "
                    "created."
                )
            );
        }
        return NULL;
    }
    UInt8 *bytes = OCFAllocatorAllocateTyped(allocator, length, 0xA6643E65, 0);
    if (bytes == NULL) {
        OCFRelease(allocator);
        if (error != NULL) {
            *error = __CFPropertyListCreateError(
                kOCFPropertyListWriteStreamError,
                OF_STR(
                    "Binary property list writing could not be completed because an external buffer could "
                    "not be allocated."
                )
            );
        }
        return NULL;
    }
    OCFMutableDataRef data =
        _CFDataCreateFixedMutableWithBuffer(kOCFAllocatorSystemDefault, length, bytes, allocator);
    if (data == NULL) {
        OCFAllocatorDeallocate(allocator, bytes);
        OCFRelease(allocator);
        if (error != NULL) {
            *error = __CFPropertyListCreateError(
                kOCFPropertyListWriteStreamError,
                OF_STR(
                    "Binary property list writing could not be completed because a CFMutableDataRef using "
                    "the external buffer could not be allocated."
                )
            );
        }
        return NULL;
    }
    OCFRelease(allocator);
    if (length != __CFBinaryPlistWriteOrPresize(plist, data, 0, 0, false, error)) {
        OCFRelease(data);
        return NULL;
    }
    return data;
}

// Native __CFBinaryPlistGetOffsetForValueFromArray2, iOS Simulator 24A434, 0x1c0163e48,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// iOS Device divergence: __CFBinaryPlistGetOffsetForValueFromArray2 at 0x180733140 (UUID
// 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified for the inline body and the cold entries)
// moves the 8-byte reference read (.cold.1, 0x1807b9414, from 0x18073347c), the 4-byte reference read
// (.cold.2, 0x1807b9418, from 0x180733460) and the 4-byte offset-table read (.cold.3, 0x1807b941c, from
// 0x1807333f0) out of line. Each cold entry is a single unconditional B (no trap) into
// _OUTLINED_FUNCTION_2_0 (.cold.1) or _OUTLINED_FUNCTION_1_2 (.cold.2, .cold.3); the caller passes the
// entry address in X0 and a stack slot in X1 and reloads the value from that slot.
// _OUTLINED_FUNCTION_2_0 (0x18061363c) is LDR X8, [X0]; REV X8; STR X8, [X1] and _OUTLINED_FUNCTION_1_2
// (0x180613ac4) is LDR W8, [X0]; REV W8; STR X8, [X1] (confidence: verified): the simulator's LDR + REV
// reads. Every inline check, its order and the inline reads agree.
// Neither length (X1) nor the seventh argument (X6, passed by one of the two callers) is read. The
// array marker at startOffset must have high nibble 0xA; a low nibble of 0xF takes the count from the
// following integer object, whose 2^n-byte value fails for n > 7 or, at 8 bytes and more, when bit 63
// is set. The reference table and the selected reference must lie inside the object area ending one
// byte before offsetTableOffset; the reference offset is compared signed against 8. The offset-table
// entry of a reference below numObjects is read without a bound: the native function relies on the
// trailer its callers validated. *offset is written only on success and only when offset is nonnull.
// The native function switches on the low byte of the integer width (1, 2, 4 and 8 with REV loads, a
// byte loop otherwise) and reads the reference and offset-table entries the same way (width 0 yields
// 0 without a read); every case yields the big-endian value of the byte loop below, and bit 63 can be
// set only at 8 bytes and more.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool __OCFBinaryPlistGetOffsetForValueFromArray2(
    const uint8_t *databytes,
    uint64_t length,
    uint64_t startOffset,
    const BinaryPlistTrailer *trailer,
    OCFIndex index,
    uint64_t *offset,
    const void *ignored
) {
    (void)length;
    (void)ignored;
    if (startOffset < 8) {
        return false;
    }
    uint64_t tableOffset = trailer->offsetTableOffset;
    uint64_t objectsRangeEnd = tableOffset - 1;
    if (objectsRangeEnd < startOffset) {
        return false;
    }
    uintptr_t base = (uintptr_t)databytes;
    uintptr_t markerAddress = base + startOffset;
    if (markerAddress == UINTPTR_MAX) {
        return false;
    }
    uint8_t marker = *(const uint8_t *)markerAddress;
    if ((marker & 0xf0) != 0xa0) {
        return false;
    }
    uint64_t referencesOffset = startOffset + 1;
    uint64_t count = marker & 0x0f;
    if (count == 0x0f) {
        if (objectsRangeEnd <= startOffset) {
            return false;
        }
        uint8_t integerMarker = *(const uint8_t *)(base + referencesOffset);
        if ((integerMarker & 0xf0) != 0x10) {
            return false;
        }
        referencesOffset = startOffset + 2;
        uint64_t exponent = integerMarker & 0x0f;
        uint64_t byteCount = UINT64_C(1) << exponent;
        const uint8_t *integerBytes = (const uint8_t *)(base + referencesOffset);
        if ((uintptr_t)integerBytes + byteCount < (uintptr_t)integerBytes) {
            return false;
        }
        referencesOffset += byteCount;
        if (base + objectsRangeEnd < base + referencesOffset - 1) {
            return false;
        }
        if (exponent > 7) {
            return false;
        }
        count = 0;
        for (uint64_t byte = 0; byte < byteCount; ++byte) {
            count = (count << 8) | integerBytes[byte];
        }
        if (count >> 63) {
            return false;
        }
    }
    if (count <= (uint64_t)index) {
        return false;
    }
    uint8_t referenceSize = trailer->objectRefSize;
    uint64_t referencesBytes = 0;
    if (__builtin_mul_overflow(count, (uint64_t)referenceSize, &referencesBytes)) {
        return false;
    }
    uintptr_t referencesAddress = base + referencesOffset;
    if (referencesAddress + referencesBytes < referencesAddress) {
        return false;
    }
    if (base + objectsRangeEnd < referencesAddress + referencesBytes - 1) {
        return false;
    }
    uint64_t referenceOffset = (uint64_t)index * referenceSize + referencesOffset;
    if ((int64_t)referenceOffset < 8) {
        return false;
    }
    uintptr_t tableAddress = base + tableOffset;
    const uint8_t *referenceBytes = (const uint8_t *)(base + referenceOffset);
    if (tableAddress - referenceSize < (uintptr_t)referenceBytes) {
        return false;
    }
    uint64_t objectReference = 0;
    for (uint8_t byte = 0; byte < referenceSize; ++byte) {
        objectReference = (objectReference << 8) | referenceBytes[byte];
    }
    if (trailer->numObjects <= objectReference) {
        return false;
    }
    if (offset != NULL) {
        uint8_t offsetSize = trailer->offsetIntSize;
        const uint8_t *entry = (const uint8_t *)(tableAddress + objectReference * offsetSize);
        uint64_t value = 0;
        for (uint8_t byte = 0; byte < offsetSize; ++byte) {
            value = (value << 8) | entry[byte];
        }
        *offset = value;
    }
    return true;
}

// Native __CFBinaryPlistCopyTopLevelKeys, 0x1c0164278. Non-external (was a private external).
// The object at startOffset must be a dictionary (marker 0xD_) inside the object area that ends one byte
// before offsetTableOffset. Its count is read as in __CFBinaryPlistParseASCIIString (a 0xF nibble takes the
// following integer object; an integer of more than 2^7 bytes yields 0, a value with bit 63 set fails), the 2
// * count references must fit (umulh test of 2 * count by the reference width; 2 * count itself is not
// tested) and end inside the object area. The keys are collected into malloc_type_malloc storage (type
// 0x0006004044C4A2DF) of count entries, or 512 when 2 * count is at least 0x402, grown by half with
// malloc_type_realloc. Each key reference is bounded like the array references of
// __CFBinaryPlistCreateObjectFiltered and parsed with no allocator options, objects, set or key paths; every
// key must parse as a string (type ID 7) or the whole function returns NULL. The result is a CFSet with the
// CFType callbacks; the parsed keys are released and the storage freed on every path after allocation.
OCFSetRef __CFBinaryPlistCopyTopLevelKeys(
    OCFAllocatorRef allocator,
    const uint8_t *databytes,
    uint64_t datalen,
    uint64_t offset,
    const BinaryPlistTrailer *trailer
) {
    if (offset < 8) {
        return NULL;
    }
    uint64_t objectsEnd = trailer->offsetTableOffset - 1;
    if (objectsEnd < offset) {
        return NULL;
    }
    const uint8_t *markerPtr = databytes + offset;
    if ((uintptr_t)markerPtr == UINTPTR_MAX) {
        return NULL;
    }
    uint8_t marker = *markerPtr;
    if ((marker & 0xf0) != 0xd0) {
        return NULL;
    }
    uint64_t refsOffset = offset + 1;
    uint64_t count = marker & 0x0f;
    if (count == 0x0f) {
        if (objectsEnd <= offset) {
            return NULL;
        }
        uint8_t integerMarker = databytes[offset + 1];
        if ((integerMarker & 0xf0) != 0x10) {
            return NULL;
        }
        uint64_t exponent = integerMarker & 0x0f;
        uint64_t byteCount = UINT64_C(1) << exponent;
        const uint8_t *integerBytes = databytes + offset + 2;
        if ((uintptr_t)integerBytes + byteCount < (uintptr_t)integerBytes) {
            return NULL;
        }
        refsOffset = offset + 2 + byteCount;
        if ((uintptr_t)databytes + objectsEnd < (uintptr_t)databytes + refsOffset - 1) {
            return NULL;
        }
        count = 0;
        if (exponent <= 7) {
            count = readBigEndian(integerBytes, byteCount);
            if (count >> 63) {
                return NULL;
            }
        }
    }
    uint64_t entryCount = 0;
    uint64_t referencesBytes = 0;
    if (count != 0) {
        entryCount = count << 1;
        if (__builtin_mul_overflow(entryCount, (uint64_t)trailer->objectRefSize, &referencesBytes)) {
            return NULL;
        }
    }
    uintptr_t references = (uintptr_t)databytes + refsOffset;
    if (references + referencesBytes < references) {
        return NULL;
    }
    if ((uintptr_t)databytes + objectsEnd < references + referencesBytes - 1) {
        return NULL;
    }
    uint64_t keyCount = entryCount >> 1;
    uint64_t capacity = entryCount < 0x402 ? keyCount : 512;
    const void **keys = malloc_type_malloc(capacity * sizeof(void *), kBinaryPlistTopLevelKeysTypeID);
    if (keys == NULL) {
        return NULL;
    }
    OCFSetRef result = NULL;
    uint64_t stored = 0;
    if (entryCount == 0) {
        result = OCFSetCreate(allocator, keys, 0, &kOCFTypeSetCallBacks);
    } else {
        for (;;) {
            if (stored >= capacity) {
                capacity = (capacity * 3) >> 1;
                const void **grown =
                    malloc_type_realloc(keys, capacity * sizeof(void *), kBinaryPlistTopLevelKeysTypeID);
                if (grown == NULL) {
                    goto fail;
                }
                keys = grown;
            }
            if ((int64_t)refsOffset < 8) {
                goto fail;
            }
            uintptr_t table = (uintptr_t)databytes + trailer->offsetTableOffset;
            uint8_t refSize = trailer->objectRefSize;
            if (table - refSize < (uintptr_t)databytes + refsOffset) {
                goto fail;
            }
            uint64_t ref = readBigEndian(databytes + refsOffset, refSize);
            if (trailer->numObjects <= ref) {
                goto fail;
            }
            uint8_t offsetSize = trailer->offsetIntSize;
            uint64_t keyOffset = readBigEndian((const uint8_t *)(table + ref * offsetSize), offsetSize);
            OCFTypeID keyType = 0;
            OCFPropertyListRef key = NULL;
            bool parsed = __CFBinaryPlistCreateObjectFiltered(
                databytes,
                datalen,
                keyOffset,
                trailer,
                allocator,
                0,
                NULL,
                NULL,
                NULL,
                0,
                NULL,
                &key,
                &keyType
            );
            if (!parsed || keyType != kBinaryPlistStringTypeID) {
                if (key != NULL) {
                    OCFRelease(key);
                }
                goto fail;
            }
            keys[stored] = key;
            ++stored;
            refsOffset += trailer->objectRefSize;
            if (stored == keyCount) {
                break;
            }
        }
        result = OCFSetCreate(allocator, keys, (OCFIndex)keyCount, &kOCFTypeSetCallBacks);
        stored = keyCount;
    }
    goto done;
fail:
    result = NULL;
done:
    for (uint64_t idx = 0; idx < stored; ++idx) {
        if (keys[idx] != NULL) {
            OCFRelease(keys[idx]);
        }
    }
    free(keys);
    return result;
}

// Native __CFBinaryPlistCreateObjectFiltered, 0x1c01646e8, with __CFBinaryPlistCreateObjectFiltered.cold.1
// (0x1c03063ac) and the two block invokes ____CFBinaryPlistCreateObjectFiltered_block_invoke (0x1c0166a18)
// and
// ____CFBinaryPlistCreateObjectFiltered_block_invoke_2 (0x1c0166b28). Non-external (was a private external).
// A depth of 0x81 or more logs "Too many nested arrays or dictionaries" as a fault through _CFOSLog (the
// os_log_type_enabled test inline, _os_log_fault_impl in .cold.1) and fails. The objects cache is keyed by
// ~startOffset: a cached object is retained and returned (with its CFGetTypeID). An offset already in set
// (the objects being parsed) fails, as does an offset below 8 or past the object area (offsetTableOffset -
// 1). The marker's high nibble selects the object; nibbles 7, 9, 0xB, 0xE and 0xF fail. Each kind checks its
// own bounds in the native order below (the marker address UINTPTR_MAX check is per kind; null and booleans
// do not make it). Type IDs are stored through outPlistTypeID before plist is tested; with plist NULL the
// function only validates. Numbers, dates, UIDs, data (when not mutable) and immutable strings created by
// copying are added to objects; containers are added only when mutability (the low byte of mutabilityOption)
// is 0 and the parse succeeded. Integers: 2^n bytes for n <= 4 (16 bytes keep the low 64 bits and become an
// SInt128 with a zero high word, CFNumber type 17). Reals: 4 or 8 bytes. Dates: marker 0x33 only. UIDs: 1-16
// bytes, values above 32 bits fail; the object is created inline as _CFKeyedArchiverUIDCreate does
// (_CFRuntimeCreateInstance type 41, 8 extra bytes, the 32-bit value at offset 16). Data and container counts
// take a following integer object for 0xF with the per-kind checks below. Strings are parsed by the block
// functions with stack blocks capturing (in layout order) outPlistTypeID, plist, mutability,
// mutabilityOption, allocator, backingData, startOffset, objects and the cache key: mutability 2 or bit 31
// and above of mutabilityOption create a no-copy string over the bytes (kCFAllocatorNull, made a mutable copy
// for mutability 2, not cached); otherwise the bytes are copied, a heap string is replaced by
// __NSCreateBPlistMappedString(startOffset, backingData) when backingData is set and that returns a string,
// and the result is cached. Unicode counts with bit 63 set fail, others are doubled. Arrays and sets: the
// reference count may take an integer of more than 2^7 bytes, which yields 0 and skips the reference range
// check. Values go into a stack buffer of up to 256 entries (only its first 512 bytes are zeroed) or a typed
// allocation for more; with plist NULL no buffer is used. Without a set and at depth 16 or more a set is made
// to detect cycles. The key paths are split (__CFPropertyListCreateSplitKeypaths); an array with key paths
// and no "*" in its first component is filtered: each component that is a non-negative integer
// ("0" by string compare, INT32_MAX rejected) selects one element, parsed with the remaining key paths, and
// the rest are skipped. Otherwise every reference is parsed (arrays and sets with the remaining key paths
// when filtered, none otherwise). Dictionaries with key paths are delegated to
// __CFPropertyListCreateFilteredDictionary; without, all keys must be strings.
// CoreFoundation defect: when an element of an unfiltered array or set fails, the split key-path sets are not
// released and the offset stays in set (native, kept). The native function has landing pads (0x1c0165f40
// through 0x1c0165fe4) that release the cycle set it made when an exception unwinds through it; this unit is
// compiled without -fexceptions, so they are not reproduced (unread: the LSDA call-site ranges).
bool __CFBinaryPlistCreateObjectFiltered(
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
) {
    if (curDepth >= 0x81) {
        os_log_fault(_CFOSLog(), "Too many nested arrays or dictionaries");
        return false;
    }
    const void *offsetKey = (const void *)(uintptr_t)~startOffset;
    if (objects != NULL && plist != NULL) {
        OCFPropertyListRef cached = OCFDictionaryGetValue(objects, offsetKey);
        *plist = cached;
        if (cached != NULL) {
            OCFRetain(cached);
            if (outPlistTypeID != NULL) {
                *outPlistTypeID = OCFGetTypeID(*plist);
                return true;
            }
            return true;
        }
    }
    if (set != NULL && OCFSetContainsValue(set, offsetKey)) {
        return false;
    }
    if (startOffset < 8) {
        return false;
    }
    uint64_t objectsEnd = trailer->offsetTableOffset - 1;
    if (objectsEnd < startOffset) {
        return false;
    }
    const uint8_t *markerPtr = databytes + startOffset;
    uint8_t marker = *markerPtr;
    uintptr_t dataEnd = (uintptr_t)databytes + objectsEnd;
    OCFOptionFlags mutability = mutabilityOption & 0xff;
    switch (marker >> 4) {
        case 0x0: {
            if (marker == 0x09) {
                if (plist != NULL) {
                    *plist = kOCFBooleanTrue;
                }
                if (outPlistTypeID != NULL) {
                    *outPlistTypeID = kBinaryPlistBooleanTypeID;
                }
                return true;
            }
            if (marker == 0x08) {
                if (plist != NULL) {
                    *plist = kOCFBooleanFalse;
                }
                if (outPlistTypeID != NULL) {
                    *outPlistTypeID = kBinaryPlistBooleanTypeID;
                }
                return true;
            }
            if (marker != 0) {
                return false;
            }
            if (plist != NULL) {
                *plist = kOCFNull;
            }
            if (outPlistTypeID != NULL) {
                *outPlistTypeID = kBinaryPlistNullTypeID;
            }
            return true;
        }
        case 0x1: {
            if ((uintptr_t)markerPtr == UINTPTR_MAX) {
                return false;
            }
            const uint8_t *bytes = markerPtr + 1;
            uint64_t exponent = marker & 0x0f;
            uint64_t byteCount = UINT64_C(1) << exponent;
            if ((uintptr_t)bytes + byteCount < (uintptr_t)bytes) {
                return false;
            }
            if (dataEnd < (uintptr_t)bytes + byteCount - 1 || exponent > 4) {
                return false;
            }
            uint64_t value = readBigEndian(bytes, byteCount);
            if (outPlistTypeID != NULL) {
                *outPlistTypeID = kBinaryPlistNumberTypeID;
            }
            if (plist == NULL) {
                return true;
            }
            OCFNumberRef number = NULL;
            if (exponent == 4) {
                struct {
                    int64_t high;
                    uint64_t low;
                } value128 = {0, value};

                number = OCFNumberCreate(allocator, 17, &value128);
            } else {
                int64_t value64 = (int64_t)value;
                number = OCFNumberCreate(allocator, 4, &value64);
            }
            bool result = number != NULL;
            if (objects != NULL && number != NULL) {
                OCFDictionarySetValue(objects, offsetKey, number);
            }
            *plist = number;
            return result;
        }
        case 0x2: {
            if ((marker & 0x0f) == 3) {
                if ((uintptr_t)markerPtr == UINTPTR_MAX) {
                    return false;
                }
                if ((uintptr_t)(markerPtr + 1) > UINTPTR_MAX - 8) {
                    return false;
                }
                if (dataEnd < (uintptr_t)markerPtr + 8) {
                    return false;
                }
                uint64_t bits = 0;
                memcpy(&bits, markerPtr + 1, sizeof(bits));
                bits = __builtin_bswap64(bits);
                if (outPlistTypeID != NULL) {
                    *outPlistTypeID = kBinaryPlistNumberTypeID;
                }
                if (plist == NULL) {
                    return true;
                }
                OCFNumberRef number = OCFNumberCreate(allocator, 6, &bits);
                bool result = number != NULL;
                if (number != NULL && objects != NULL) {
                    OCFDictionarySetValue(objects, offsetKey, number);
                }
                *plist = number;
                return result;
            }
            if ((marker & 0x0f) != 2) {
                return false;
            }
            if ((uintptr_t)markerPtr == UINTPTR_MAX) {
                return false;
            }
            if ((uintptr_t)(markerPtr + 1) > UINTPTR_MAX - 4) {
                return false;
            }
            if (dataEnd < (uintptr_t)markerPtr + 4) {
                return false;
            }
            uint32_t bits = 0;
            memcpy(&bits, markerPtr + 1, sizeof(bits));
            bits = __builtin_bswap32(bits);
            if (outPlistTypeID != NULL) {
                *outPlistTypeID = kBinaryPlistNumberTypeID;
            }
            if (plist == NULL) {
                return true;
            }
            OCFNumberRef number = OCFNumberCreate(allocator, 5, &bits);
            bool result = number != NULL;
            if (number != NULL && objects != NULL) {
                OCFDictionarySetValue(objects, offsetKey, number);
            }
            *plist = number;
            return result;
        }
        case 0x3: {
            if ((uintptr_t)markerPtr == UINTPTR_MAX) {
                return false;
            }
            if (marker != 0x33) {
                return false;
            }
            if ((uintptr_t)(markerPtr + 1) > UINTPTR_MAX - 8) {
                return false;
            }
            if (dataEnd < (uintptr_t)markerPtr + 8) {
                return false;
            }
            uint64_t bits = 0;
            memcpy(&bits, markerPtr + 1, sizeof(bits));
            if (outPlistTypeID != NULL) {
                *outPlistTypeID = kBinaryPlistDateTypeID;
            }
            if (plist == NULL) {
                return true;
            }
            bits = __builtin_bswap64(bits);
            double at = 0;
            memcpy(&at, &bits, sizeof(at));
            OCFDateRef date = OCFDateCreate(allocator, at);
            bool result = date != NULL;
            if (objects != NULL && date != NULL) {
                OCFDictionarySetValue(objects, offsetKey, date);
            }
            *plist = date;
            return result;
        }
        case 0x4: {
            if ((uintptr_t)markerPtr == UINTPTR_MAX) {
                return false;
            }
            const uint8_t *bytes = markerPtr + 1;
            uint64_t count = marker & 0x0f;
            if (count == 0x0f) {
                if (dataEnd < (uintptr_t)bytes) {
                    return false;
                }
                uint8_t integerMarker = markerPtr[1];
                if ((integerMarker & 0xf0) != 0x10) {
                    return false;
                }
                const uint8_t *integerBytes = markerPtr + 2;
                uint64_t exponent = integerMarker & 0x0f;
                uint64_t byteCount = UINT64_C(1) << exponent;
                if ((uintptr_t)integerBytes + byteCount < (uintptr_t)integerBytes) {
                    return false;
                }
                bytes = integerBytes + byteCount;
                if (dataEnd < (uintptr_t)bytes - 1) {
                    return false;
                }
                count = 0;
                if (exponent <= 7) {
                    count = readBigEndian(integerBytes, byteCount);
                    if (count >> 63) {
                        return false;
                    }
                }
            }
            if ((uintptr_t)bytes + count < (uintptr_t)bytes) {
                return false;
            }
            if (dataEnd < (uintptr_t)bytes + count - 1) {
                return false;
            }
            if (outPlistTypeID != NULL) {
                *outPlistTypeID = kBinaryPlistDataTypeID;
            }
            if (plist == NULL) {
                return true;
            }
            OCFDataRef data = NULL;
            bool result = false;
            if (mutability == 2) {
                OCFMutableDataRef mutableData = OCFDataCreateMutable(allocator, 0);
                if (mutableData != NULL) {
                    OCFDataAppendBytes(mutableData, bytes, (OCFIndex)count);
                }
                data = mutableData;
                result = data != NULL;
            } else {
                data = OCFDataCreate(allocator, bytes, (OCFIndex)count);
                result = data != NULL;
                if (objects != NULL && data != NULL) {
                    OCFDictionarySetValue(objects, offsetKey, data);
                    result = true;
                }
            }
            *plist = data;
            return result;
        }
        case 0x5: {
            return __OCFBinaryPlistParseASCIIString(
                databytes, objectsEnd, markerPtr, ^bool(const uint8_t *characters, uint64_t count) {
                  if (outPlistTypeID != NULL) {
                      *outPlistTypeID = kBinaryPlistStringTypeID;
                  }
                  if (plist == NULL) {
                      return true;
                  }
                  OCFStringRef string = NULL;
                  if (mutability == 2 || (mutabilityOption >> 31) != 0) {
                      string = _CFStringCreateWithBytesNoCopyDisallowROM(
                          allocator, characters, (OCFIndex)count, 0x0600, false, kOCFAllocatorNull
                      );
                      if (mutability == 2 && string != NULL) {
                          OCFStringRef copy = OCFStringCreateMutableCopy(allocator, 0, string);
                          OCFRelease(string);
                          string = copy;
                      }
                  } else {
                      string =
                          OCFStringCreateWithBytes(allocator, characters, (OCFIndex)count, 0x0600, false);
                      if (string != NULL) {
                          if (backingData != NULL && malloc_size(string) != 0) {
                              OCFStringRef mapped = __ONSCreateBPlistMappedString(startOffset, backingData);
                              if (mapped != NULL) {
                                  OCFRelease(string);
                                  string = mapped;
                              }
                          }
                          if (objects != NULL) {
                              OCFDictionarySetValue(objects, offsetKey, string);
                          }
                      }
                  }
                  *plist = string;
                  return string != NULL;
                }
            );
        }
        case 0x6: {
            return __OCFBinaryPlistParseUnicode16String(
                databytes, objectsEnd, markerPtr, ^bool(const uint8_t *characters, uint64_t count) {
                  if (outPlistTypeID != NULL) {
                      *outPlistTypeID = kBinaryPlistStringTypeID;
                  }
                  if (plist == NULL) {
                      return true;
                  }
                  if (count != 0) {
                      if ((int64_t)count < 0) {
                          return false;
                      }
                      count <<= 1;
                  }
                  OCFStringRef string = NULL;
                  if (mutability == 2 || (mutabilityOption >> 31) != 0) {
                      string = _CFStringCreateWithBytesNoCopyDisallowROM(
                          allocator, characters, (OCFIndex)count, 0x10000100, false, kOCFAllocatorNull
                      );
                      if (mutability == 2 && string != NULL) {
                          OCFStringRef copy = OCFStringCreateMutableCopy(allocator, 0, string);
                          OCFRelease(string);
                          string = copy;
                      }
                  } else {
                      string =
                          OCFStringCreateWithBytes(allocator, characters, (OCFIndex)count, 0x10000100, false);
                      if (string != NULL) {
                          if (backingData != NULL && malloc_size(string) != 0) {
                              OCFStringRef mapped = __ONSCreateBPlistMappedString(startOffset, backingData);
                              if (mapped != NULL) {
                                  OCFRelease(string);
                                  string = mapped;
                              }
                          }
                          if (objects != NULL) {
                              OCFDictionarySetValue(objects, offsetKey, string);
                          }
                      }
                  }
                  *plist = string;
                  return string != NULL;
                }
            );
        }
        case 0x8: {
            if ((uintptr_t)markerPtr == UINTPTR_MAX) {
                return false;
            }
            const uint8_t *bytes = markerPtr + 1;
            uint64_t byteCount = (uint64_t)(marker & 0x0f) + 1;
            if ((uintptr_t)bytes + byteCount < (uintptr_t)bytes) {
                return false;
            }
            if (dataEnd < (uintptr_t)bytes + byteCount - 1) {
                return false;
            }
            // Widths 1, 2 and 4 cannot exceed 32 bits; the native code tests the high word for the others.
            uint64_t value = readBigEndian(bytes, byteCount);
            if ((value >> 32) != 0) {
                return false;
            }
            if (outPlistTypeID != NULL) {
                *outPlistTypeID = kBinaryPlistUIDTypeID;
            }
            if (plist == NULL) {
                return true;
            }
            OCFTypeRef uid = _OCFRuntimeCreateInstance(allocator, kBinaryPlistUIDTypeID, 8, NULL);
            bool result = uid != NULL;
            if (uid != NULL) {
                *(uint32_t *)((uint8_t *)(uintptr_t)uid + 16) = (uint32_t)value;
                if (objects != NULL) {
                    OCFDictionarySetValue(objects, offsetKey, uid);
                }
            }
            *plist = uid;
            return result;
        }
        case 0xA:
        case 0xC: {
            if ((uintptr_t)markerPtr == UINTPTR_MAX) {
                return false;
            }
            uint64_t refsOffset = startOffset + 1;
            uint64_t count = marker & 0x0f;
            bool checkReferences = true;
            if (count == 0x0f) {
                if (objectsEnd <= startOffset) {
                    return false;
                }
                uint8_t integerMarker = databytes[startOffset + 1];
                if ((integerMarker & 0xf0) != 0x10) {
                    return false;
                }
                uint64_t exponent = integerMarker & 0x0f;
                uint64_t byteCount = UINT64_C(1) << exponent;
                const uint8_t *integerBytes = databytes + startOffset + 2;
                if ((uintptr_t)integerBytes + byteCount < (uintptr_t)integerBytes) {
                    return false;
                }
                refsOffset = startOffset + 2 + byteCount;
                if (dataEnd < (uintptr_t)databytes + refsOffset - 1) {
                    return false;
                }
                count = 0;
                if (exponent <= 7) {
                    count = readBigEndian(integerBytes, byteCount);
                    if (count >> 63) {
                        return false;
                    }
                } else {
                    checkReferences = false;
                }
            }
            uint64_t listBytes = 0;
            if (checkReferences) {
                uint64_t referencesBytes = 0;
                if (count != 0 &&
                    __builtin_mul_overflow(count, (uint64_t)trailer->objectRefSize, &referencesBytes)) {
                    return false;
                }
                uintptr_t references = (uintptr_t)databytes + refsOffset;
                if (references + referencesBytes < references) {
                    return false;
                }
                if (dataEnd < references + referencesBytes - 1) {
                    return false;
                }
                if (count != 0) {
                    if (count >> 61) {
                        return false;
                    }
                    listBytes = count * sizeof(void *);
                }
            }
            OCFIndex stackCount = count - 1 < 0x100 ? (OCFIndex)count : 1;
            const void *buffer[stackCount];
            size_t bufferBytes = (size_t)stackCount * sizeof(void *);
            bzero(buffer, bufferBytes < 0x200 ? bufferBytes : 0x200);
            const void **list = NULL;
            if (plist != NULL) {
                list = buffer;
                if (count >= 0x101) {
                    list = OCFAllocatorAllocateTyped(
                        kOCFAllocatorSystemDefault, (OCFIndex)listBytes, kBinaryPlistObjectListTypeID, 0
                    );
                    if (list == NULL) {
                        return false;
                    }
                }
            }
            OCFMutableSetRef madeSet = NULL;
            OCFMutableSetRef currentSet = set;
            if (currentSet == NULL && curDepth >= 0x10) {
                madeSet = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, NULL);
                currentSet = madeSet;
            }
            if (currentSet != NULL) {
                OCFSetAddValue(currentSet, offsetKey);
            }
            OCFSetRef theseKeys = NULL;
            OCFSetRef nextKeys = NULL;
            __CFPropertyListCreateSplitKeypaths(kOCFAllocatorSystemDefault, keyPaths, &theseKeys, &nextKeys);
            uint32_t objectType = marker & 0xf0;
            bool unfiltered = keyPaths == NULL || objectType != 0xa0;
            bool hasWildcard = theseKeys != NULL && OCFSetContainsValue(theseKeys, OF_STR("*"));
            bool result = false;
            OCFTypeID typeID = kBinaryPlistArrayTypeID;
            if (unfiltered || hasWildcard) {
                OCFPropertyListRef child = NULL;
                OCFPropertyListRef *childPlist = plist != NULL ? &child : NULL;
                uint64_t stored = 0;
                for (; stored < count; ++stored) {
                    if ((int64_t)refsOffset < 8) {
                        goto releaseValues;
                    }
                    uintptr_t table = (uintptr_t)databytes + trailer->offsetTableOffset;
                    uint8_t refSize = trailer->objectRefSize;
                    if (table - refSize < (uintptr_t)databytes + refsOffset) {
                        goto releaseValues;
                    }
                    uint64_t ref = readBigEndian(databytes + refsOffset, refSize);
                    if (trailer->numObjects <= ref) {
                        goto releaseValues;
                    }
                    uint8_t offsetSize = trailer->offsetIntSize;
                    uint64_t valueOffset =
                        readBigEndian((const uint8_t *)(table + ref * offsetSize), offsetSize);
                    child = NULL;
                    if (!__CFBinaryPlistCreateObjectFiltered(
                            databytes,
                            datalen,
                            valueOffset,
                            trailer,
                            allocator,
                            mutabilityOption,
                            backingData,
                            objects,
                            currentSet,
                            curDepth + 1,
                            unfiltered ? NULL : nextKeys,
                            childPlist,
                            NULL
                        )) {
                        goto releaseValues;
                    }
                    if (list != NULL) {
                        list[stored] = child;
                    }
                    refsOffset += trailer->objectRefSize;
                }
                if (objectType == 0xa0) {
                    typeID = kBinaryPlistArrayTypeID;
                    if (plist != NULL) {
                        if (mutability == 0) {
                            *plist = __CFArrayCreateTransfer(allocator, list, (OCFIndex)count);
                        } else {
                            OCFMutableArrayRef array =
                                OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
                            *plist = array;
                            OCFArrayReplaceValues(array, (OCFRange){0, 0}, list, (OCFIndex)count);
                            for (uint64_t idx = 0; idx < count; ++idx) {
                                OCFRelease(list[idx]);
                            }
                        }
                    }
                } else {
                    typeID = kBinaryPlistSetTypeID;
                    if (plist != NULL) {
                        if (mutability == 0) {
                            *plist = __CFSetCreateTransfer(allocator, list, (OCFIndex)count);
                        } else {
                            *plist = OCFSetCreateMutable(allocator, 0, &kOCFTypeSetCallBacks);
                            if (count != 0) {
                                for (uint64_t idx = 0; idx < count; ++idx) {
                                    OCFSetAddValue((OCFMutableSetRef)(uintptr_t)*plist, list[idx]);
                                }
                                for (uint64_t idx = 0; idx < count; ++idx) {
                                    OCFRelease(list[idx]);
                                }
                            }
                        }
                    }
                }
                result = true;
                goto finish;
            releaseValues:
                if (list != NULL) {
                    for (; stored != 0; --stored) {
                        OCFRelease(list[stored - 1]);
                    }
                    if (list != buffer) {
                        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, list);
                    }
                }
                if (madeSet != NULL) {
                    OCFRelease(madeSet);
                }
                return false;
            }
            {
                OCFMutableArrayRef array =
                    OCFArrayCreateMutable(allocator, OCFSetGetCount(theseKeys), &kOCFTypeArrayCallBacks);
                result = true;
                if (theseKeys != NULL) {
                    const void **keys = malloc_type_malloc(
                        OCFSetGetCount(theseKeys) * sizeof(void *), kBinaryPlistObjectListTypeID
                    );
                    OCFSetGetValues(theseKeys, keys);
                    OCFIndex keyCount = OCFSetGetCount(theseKeys);
                    OCFPropertyListRef element = NULL;
                    OCFPropertyListRef *elementPlist = plist != NULL ? &element : NULL;
                    for (OCFIndex idx = 0; idx < keyCount; ++idx) {
                        OCFStringRef key = keys[idx];
                        SInt32 index = OCFStringGetIntValue(key);
                        if (index == INT32_MAX) {
                            continue;
                        }
                        if (index == 0) {
                            if (OCFStringCompare(OF_STR("0"), key, 0) != kOCFCompareEqualTo) {
                                continue;
                            }
                        } else if (index < 0) {
                            continue;
                        }
                        // The native call leaves X1 and X6 holding unrelated values; the callee reads
                        // neither.
                        uint64_t elementOffset = 0;
                        if (!__OCFBinaryPlistGetOffsetForValueFromArray2(
                                databytes,
                                datalen,
                                startOffset,
                                trailer,
                                (OCFIndex)(uint32_t)index,
                                &elementOffset,
                                NULL
                            )) {
                            continue;
                        }
                        element = NULL;
                        if (!__CFBinaryPlistCreateObjectFiltered(
                                databytes,
                                datalen,
                                elementOffset,
                                trailer,
                                allocator,
                                mutabilityOption,
                                backingData,
                                objects,
                                currentSet,
                                curDepth + 1,
                                nextKeys,
                                elementPlist,
                                NULL
                            )) {
                            result = false;
                            break;
                        }
                        if (element != NULL) {
                            OCFArrayAppendValue(array, element);
                            OCFRelease(element);
                        }
                    }
                    free(keys);
                }
                if (plist != NULL && result) {
                    if (mutability - 1 < 2) {
                        *plist = array;
                    } else {
                        *plist = OCFArrayCreateCopy(allocator, array);
                        OCFRelease(array);
                    }
                    result = true;
                } else if (array != NULL) {
                    OCFRelease(array);
                }
                typeID = kBinaryPlistArrayTypeID;
            }
        finish:
            if (outPlistTypeID != NULL) {
                *outPlistTypeID = typeID;
            }
            if (currentSet != NULL) {
                OCFSetRemoveValue(currentSet, offsetKey);
            }
            if (plist != NULL && objects != NULL && (mutability != 0 ? false : result)) {
                OCFDictionarySetValue(objects, offsetKey, *plist);
            }
            if (list != NULL && list != buffer) {
                OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, list);
            }
            if (theseKeys != NULL) {
                OCFRelease(theseKeys);
            }
            if (nextKeys != NULL) {
                OCFRelease(nextKeys);
            }
            if (madeSet != NULL) {
                OCFRelease(madeSet);
            }
            return result;
        }
        case 0xD: {
            if ((uintptr_t)markerPtr == UINTPTR_MAX) {
                return false;
            }
            uint64_t refsOffset = startOffset + 1;
            uint64_t count = marker & 0x0f;
            if (count == 0x0f) {
                if (objectsEnd <= startOffset) {
                    return false;
                }
                uint8_t integerMarker = databytes[startOffset + 1];
                if ((integerMarker & 0xf0) != 0x10) {
                    return false;
                }
                uint64_t exponent = integerMarker & 0x0f;
                uint64_t byteCount = UINT64_C(1) << exponent;
                const uint8_t *integerBytes = databytes + startOffset + 2;
                if ((uintptr_t)integerBytes + byteCount < (uintptr_t)integerBytes) {
                    return false;
                }
                refsOffset = startOffset + 2 + byteCount;
                if (dataEnd < (uintptr_t)databytes + refsOffset - 1) {
                    return false;
                }
                count = 0;
                if (exponent <= 7) {
                    count = readBigEndian(integerBytes, byteCount);
                    if (count >> 63) {
                        return false;
                    }
                }
            }
            uint64_t entryCount = 0;
            uint64_t referencesBytes = 0;
            if (count != 0) {
                entryCount = count << 1;
                if (__builtin_mul_overflow(entryCount, (uint64_t)trailer->objectRefSize, &referencesBytes)) {
                    return false;
                }
            }
            uintptr_t references = (uintptr_t)databytes + refsOffset;
            if (references + referencesBytes < references) {
                return false;
            }
            if (dataEnd < references + referencesBytes - 1) {
                return false;
            }
            uint64_t listBytes = 0;
            if (count != 0) {
                if (entryCount >> 61) {
                    return false;
                }
                listBytes = entryCount * sizeof(void *);
            }
            OCFIndex stackCount = entryCount - 1 < 0x100 ? (OCFIndex)entryCount : 1;
            const void *buffer[stackCount];
            size_t bufferBytes = (size_t)stackCount * sizeof(void *);
            bzero(buffer, bufferBytes < 0x200 ? bufferBytes : 0x200);
            const void **list = NULL;
            if (plist != NULL) {
                list = buffer;
                if ((int64_t)entryCount >= 0x101) {
                    list = OCFAllocatorAllocateTyped(
                        kOCFAllocatorSystemDefault, (OCFIndex)listBytes, kBinaryPlistObjectListTypeID, 0
                    );
                    if (list == NULL) {
                        return false;
                    }
                }
            }
            OCFMutableSetRef madeSet = NULL;
            OCFMutableSetRef currentSet = set;
            if (currentSet == NULL && curDepth >= 0x10) {
                madeSet = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, NULL);
                currentSet = madeSet;
            }
            if (currentSet != NULL) {
                OCFSetAddValue(currentSet, offsetKey);
            }
            bool result = false;
            if (keyPaths != NULL) {
                OCFMutableDictionaryRef filtered = __CFPropertyListCreateFilteredDictionary(
                    allocator,
                    keyPaths,
                    databytes,
                    datalen,
                    startOffset,
                    trailer,
                    plist == NULL,
                    mutability,
                    backingData,
                    objects,
                    currentSet,
                    curDepth
                );
                if (plist != NULL && filtered != NULL) {
                    if (mutability - 1 < 2) {
                        *plist = filtered;
                    } else {
                        *plist = OCFDictionaryCreateCopy(allocator, filtered);
                        OCFRelease(filtered);
                    }
                    result = true;
                } else if (filtered != NULL) {
                    OCFRelease(filtered);
                    result = true;
                } else {
                    result = false;
                }
            } else {
                OCFIndex pairCount = (OCFIndex)entryCount >> 1;
                OCFPropertyListRef child = NULL;
                OCFTypeID childType = 0;
                OCFPropertyListRef *childPlist = plist != NULL ? &child : NULL;
                OCFIndex stored = 0;
                for (; stored < (OCFIndex)entryCount; ++stored) {
                    if ((int64_t)refsOffset < 8) {
                        goto releaseEntries;
                    }
                    uintptr_t table = (uintptr_t)databytes + trailer->offsetTableOffset;
                    uint8_t refSize = trailer->objectRefSize;
                    if (table - refSize < (uintptr_t)databytes + refsOffset) {
                        goto releaseEntries;
                    }
                    uint64_t ref = readBigEndian(databytes + refsOffset, refSize);
                    if (trailer->numObjects <= ref) {
                        goto releaseEntries;
                    }
                    uint8_t offsetSize = trailer->offsetIntSize;
                    uint64_t entryOffset =
                        readBigEndian((const uint8_t *)(table + ref * offsetSize), offsetSize);
                    child = NULL;
                    childType = 0;
                    if (!__CFBinaryPlistCreateObjectFiltered(
                            databytes,
                            datalen,
                            entryOffset,
                            trailer,
                            allocator,
                            mutabilityOption,
                            backingData,
                            objects,
                            currentSet,
                            curDepth + 1,
                            NULL,
                            childPlist,
                            &childType
                        ) ||
                        (stored < pairCount && childType != kBinaryPlistStringTypeID)) {
                        if (child != NULL) {
                            OCFRelease(child);
                        }
                        goto releaseEntries;
                    }
                    if (list != NULL) {
                        list[stored] = child;
                    }
                    refsOffset += trailer->objectRefSize;
                }
                if (plist != NULL) {
                    OCFDictionaryRef dictionary = NULL;
                    if (mutability == 0) {
                        dictionary =
                            __CFDictionaryCreateTransfer(allocator, list, list + pairCount, pairCount);
                    } else {
                        OCFMutableDictionaryRef mutableDictionary = OCFDictionaryCreateMutable(
                            allocator,
                            pairCount,
                            &kOCFTypeDictionaryKeyCallBacks,
                            &kOCFTypeDictionaryValueCallBacks
                        );
                        for (OCFIndex idx = 0; idx < pairCount; ++idx) {
                            OCFDictionaryAddValue(mutableDictionary, list[idx], list[idx + pairCount]);
                        }
                        for (OCFIndex idx = 0; idx < (OCFIndex)entryCount; ++idx) {
                            OCFRelease(list[idx]);
                        }
                        dictionary = mutableDictionary;
                    }
                    *plist = dictionary;
                }
                if (outPlistTypeID != NULL) {
                    *outPlistTypeID = kBinaryPlistDictionaryTypeID;
                }
                result = true;
                goto finishDictionary;
            releaseEntries:
                if (list != NULL) {
                    for (; stored != 0; --stored) {
                        OCFRelease(list[stored - 1]);
                    }
                    if (list != buffer) {
                        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, list);
                    }
                }
                if (madeSet != NULL) {
                    OCFRelease(madeSet);
                }
                return false;
            }
        finishDictionary:
            if (currentSet != NULL) {
                OCFSetRemoveValue(currentSet, offsetKey);
            }
            if (plist != NULL && objects != NULL && (mutability != 0 ? false : result)) {
                OCFDictionarySetValue(objects, offsetKey, *plist);
            }
            if (list != NULL && list != buffer) {
                OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, list);
            }
            if (madeSet != NULL) {
                OCFRelease(madeSet);
            }
            return result;
        }
        default:
            return false;
    }
}

// Native __CFBinaryPlistGetOffsetForValueFromDictionary3, 0x1c0165fe8. External.
// iOS Device divergence: unread. The final two caller-supplied arguments are unused in the assembly.
// CoreFoundation defect: the verified fast path at 0x1c0166460 loads the key marker before checking
// its offset, and 0x1c0166420 and 0x1c0166788 read table entries without checking the data length.
// Guard these reads for a malformed offset or trailer.
bool __OCFBinaryPlistGetOffsetForValueFromDictionary3(
    const uint8_t *bytes,
    uint64_t length,
    uint64_t offset,
    const BinaryPlistTrailer *trailer,
    OCFTypeRef key,
    uint64_t *keyOffset,
    uint64_t *valueOffset,
    const void *unused,
    OCFMutableDictionaryRef objects
) {
    (void)unused;
    (void)objects;
    if (key == NULL) {
        return false;
    }
    OCFTypeID typeID = OCFGetTypeID(key);
    if (typeID <= 19 && ((0xe0001u >> typeID) & 1) != 0) {
        return false;
    }
    uint64_t table = trailer->offsetTableOffset;
    if (offset < 8 || offset >= table || table > length) {
        return false;
    }
    uint8_t marker = bytes[offset];
    if ((marker & 0xf0) != 0xd0) {
        return false;
    }
    uint64_t count = marker & 0xf;
    uint64_t refs = offset + 1;
    if (count == 15) {
        if (refs == table || (bytes[refs] & 0xf0) != 0x10) {
            return false;
        }
        uint8_t exponent = bytes[refs++] & 0xf;
        uint64_t width = UINT64_C(1) << exponent;
        if (width > table - refs) {
            return false;
        }
        count = exponent <= 7 ? readBigEndian(bytes + refs, width) : 0;
        if ((count >> 63) != 0) {
            return false;
        }
        refs += width;
    }
    uint64_t refsLength;
    if (__builtin_mul_overflow(count * 2, (uint64_t)trailer->objectRefSize, &refsLength) ||
        refsLength > table - refs) {
        return false;
    }

    char buffer[16];
    const char *ascii = NULL;
    OCFIndex keyLength = -1;
    uint8_t refSize;
    if (typeID == 7) {
        keyLength = OCFStringGetLength(key);
        refSize = trailer->objectRefSize;
        if (keyLength != -1) {
            ascii = OCFStringGetCStringPtr(key, 0);
            if (keyLength <= 15 && ascii == NULL && OCFStringGetCString(key, buffer, sizeof(buffer), 0) &&
                strnlen(buffer, sizeof(buffer)) == (size_t)keyLength) {
                ascii = buffer;
            }
        }
    } else {
        refSize = trailer->objectRefSize;
    }
    uint64_t valuesDistance = count * refSize;
    refSize = trailer->objectRefSize;

    for (uint64_t index = 0; index < count; ++index, refs += refSize) {
        uint64_t tableOffset = trailer->offsetTableOffset;
        if (tableOffset > length || refs > tableOffset || refSize > tableOffset - refs) {
            return false;
        }
        uint64_t keyRef = readBigEndian(bytes + refs, refSize);
        if (keyRef >= trailer->numObjects) {
            return false;
        }
        uint64_t offsetSize = trailer->offsetIntSize;
        uint64_t entry;
        if (__builtin_mul_overflow(keyRef, offsetSize, &entry) || entry > length - tableOffset ||
            offsetSize > length - tableOffset - entry) {
            return false;
        }
        uint64_t candidate = readBigEndian(bytes + tableOffset + entry, offsetSize);
        bool equal;
        if (ascii != NULL && candidate >= length) {
            return false;
        }
        if (ascii != NULL && (bytes[candidate] & 0xf0) == 0x50) {
            uint64_t asciiLength = bytes[candidate] & 0xf;
            uint64_t asciiOffset = candidate + 1;
            if (keyLength >= 15 && asciiLength == 15) {
                if (asciiOffset >= table || (bytes[asciiOffset] & 0xf0) != 0x10) {
                    return false;
                }
                uint8_t exponent = bytes[asciiOffset++] & 0xf;
                uint64_t width = UINT64_C(1) << exponent;
                if (width > table - asciiOffset) {
                    return false;
                }
                asciiLength = exponent <= 7 ? readBigEndian(bytes + asciiOffset, width) : 0;
                if ((asciiLength >> 63) != 0) {
                    return false;
                }
                asciiOffset += width;
            }
            equal = false;
            if (asciiLength == (uint64_t)keyLength) {
                if (asciiOffset > table || asciiLength > table - asciiOffset) {
                    return false;
                }
                equal = memcmp(bytes + asciiOffset, ascii, (size_t)asciiLength) == 0;
            }
        } else {
            OCFPropertyListRef candidateObject = NULL;
            OCFTypeID candidateType = 0;
            bool parsed = __CFBinaryPlistCreateObjectFiltered(
                bytes,
                length,
                candidate,
                trailer,
                kOCFAllocatorSystemDefault,
                (OCFOptionFlags)(int64_t)INT32_MIN,
                NULL,
                NULL,
                NULL,
                0,
                NULL,
                &candidateObject,
                &candidateType
            );
            if (!parsed || (candidateType <= 19 && ((0xe0001u >> candidateType) & 1) != 0)) {
                if (candidateObject != NULL) {
                    OCFRelease(candidateObject);
                }
                return false;
            }
            equal = OCFEqual(key, candidateObject);
            OCFRelease(candidateObject);
            refSize = trailer->objectRefSize;
        }
        if (!equal) {
            continue;
        }

        uint64_t valueRefOffset = refs + valuesDistance;
        uint8_t valueRefSize = trailer->objectRefSize;
        tableOffset = trailer->offsetTableOffset;
        if ((int64_t)valueRefOffset < 8 || tableOffset > length || valueRefOffset > tableOffset ||
            valueRefSize > tableOffset - valueRefOffset) {
            return false;
        }
        uint64_t valueRef = readBigEndian(bytes + valueRefOffset, valueRefSize);
        if (valueRef >= trailer->numObjects) {
            return false;
        }
        if (valueOffset != NULL) {
            offsetSize = trailer->offsetIntSize;
            if (__builtin_mul_overflow(valueRef, offsetSize, &entry) || entry > length - tableOffset ||
                offsetSize > length - tableOffset - entry) {
                return false;
            }
            *valueOffset = readBigEndian(bytes + tableOffset + entry, offsetSize);
        }
        if (keyOffset != NULL) {
            *keyOffset = candidate;
        }
        return true;
    }
    return false;
}

// Native __CFBinaryPlistParseASCIIString, iOS Simulator 24A434, 0x1c01667dc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// iOS Device 24A446: 0x18061f684, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// reorders the integer-width cases and tail-calls the block with an authenticated BRAA; the checks and
// results agree.
// marker must not be the last address (marker + 1 overflow). The count is the marker's low nibble, or
// for 0xF the following integer object: its marker byte must lie at or before base + limit and have high
// nibble 1, its 2^n bytes must end at or before base + limit, n > 7 yields a count of zero (no failure)
// and a value with bit 63 set fails. The characters must end at or before base + limit; then the block is
// tail-called with the first character byte and the count (the Block ABI call through the invoke pointer
// at offset 16) and its result is returned. The marker's high nibble is not tested. The block is the
// caller's; both native callers pass stack blocks.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool __OCFBinaryPlistParseASCIIString(
    const uint8_t *base, uint64_t limit, const uint8_t *marker, BinaryPlistStringBlock block
) {
    if ((uintptr_t)marker == UINTPTR_MAX) {
        return false;
    }
    uintptr_t end = (uintptr_t)base + limit;
    const uint8_t *characters = marker + 1;
    uint64_t count = marker[0] & 0x0f;
    if (count == 0x0f) {
        if (end < (uintptr_t)characters) {
            return false;
        }
        uint8_t integerMarker = marker[1];
        if ((integerMarker & 0xf0) != 0x10) {
            return false;
        }
        const uint8_t *integerBytes = marker + 2;
        uint64_t exponent = integerMarker & 0x0f;
        uint64_t byteCount = UINT64_C(1) << exponent;
        if ((uintptr_t)integerBytes + byteCount < (uintptr_t)integerBytes) {
            return false;
        }
        characters = (const uint8_t *)((uintptr_t)integerBytes + byteCount);
        if (end < (uintptr_t)characters - 1) {
            return false;
        }
        count = 0;
        if (exponent <= 7) {
            for (uint64_t byte = 0; byte < byteCount; ++byte) {
                count = (count << 8) | integerBytes[byte];
            }
            if (count >> 63) {
                return false;
            }
        }
    }
    if ((uintptr_t)characters + count < (uintptr_t)characters) {
        return false;
    }
    if (end < (uintptr_t)characters + count - 1) {
        return false;
    }
    return block(characters, count);
}

// Native __CFBinaryPlistParseUnicode16String, iOS Simulator 24A434, 0x1c01668f4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// iOS Device divergence: __CFBinaryPlistParseUnicode16String at 0x18068035c (UUID
// 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified for the inline body) adds a stack guard,
// moves the 8-byte count read into .cold.1 (0x1807b955c, from 0x1806804d8), a single unconditional B
// (no trap) into _OUTLINED_FUNCTION_0_5 (0x18065c54c; confidence: verified), which assembles the eight
// count bytes at marker + 2 big-endian with LDRB/ORR and stores them through X2, the simulator's LDUR +
// REV value; the caller then tests bit 63 as the simulator does. It tail-calls the block with BRAA; the other
// checks and their order agree.
// The count is read as in __CFBinaryPlistParseASCIIString and is in UTF-16 units: the function then
// fails when (characters + count - 1) + count overflows, then when characters + count overflows, then
// when the 2 * count bytes end after base + limit. The block receives the first character byte and the
// unit count.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool __OCFBinaryPlistParseUnicode16String(
    const uint8_t *base, uint64_t limit, const uint8_t *marker, BinaryPlistStringBlock block
) {
    if ((uintptr_t)marker == UINTPTR_MAX) {
        return false;
    }
    uintptr_t end = (uintptr_t)base + limit;
    const uint8_t *characters = marker + 1;
    uint64_t count = marker[0] & 0x0f;
    if (count == 0x0f) {
        if (end < (uintptr_t)characters) {
            return false;
        }
        uint8_t integerMarker = marker[1];
        if ((integerMarker & 0xf0) != 0x10) {
            return false;
        }
        const uint8_t *integerBytes = marker + 2;
        uint64_t exponent = integerMarker & 0x0f;
        uint64_t byteCount = UINT64_C(1) << exponent;
        if ((uintptr_t)integerBytes + byteCount < (uintptr_t)integerBytes) {
            return false;
        }
        characters = (const uint8_t *)((uintptr_t)integerBytes + byteCount);
        if (end < (uintptr_t)characters - 1) {
            return false;
        }
        count = 0;
        if (exponent <= 7) {
            for (uint64_t byte = 0; byte < byteCount; ++byte) {
                count = (count << 8) | integerBytes[byte];
            }
            if (count >> 63) {
                return false;
            }
        }
    }
    uintptr_t last = (uintptr_t)characters + count - 1;
    if (last + count < last) {
        return false;
    }
    if ((uintptr_t)characters + count < (uintptr_t)characters) {
        return false;
    }
    if (end < last + count) {
        return false;
    }
    return block(characters, count);
}

// Native __CFPropertyListTryFilteredDictionaryASCIIFastPath has no symbol: it is inlined into
// __CFPropertyListCreateFilteredDictionary (0x1c0167140-0x1c016718c), and only its block,
// ____CFPropertyListTryFilteredDictionaryASCIIFastPath_block_invoke (0x1c0167ac8, non-external), is out of
// line. It is kept as an always-inline function so that the block keeps its native name. The 64-byte stack
// block captures filter, filterLength, outKey and allocator (offsets 32-56) and always returns true; when
// __CFPropertyListKeyBufferMatchesFilterBuffer accepts the ASCII key bytes it stores a new ASCII string
// (CFStringCreateWithBytes, not external representation) through outKey. The result is the parse result of
// __CFBinaryPlistParseASCIIString with limit offsetTableOffset - 1.
static inline __attribute__((always_inline)) bool __CFPropertyListTryFilteredDictionaryASCIIFastPath(
    const uint8_t *databytes,
    uint64_t limit,
    uint64_t keyOffset,
    const char *filter,
    OCFIndex filterLength,
    OCFAllocatorRef allocator,
    OCFStringRef *outKey
) {
    *outKey = NULL;
    return __OCFBinaryPlistParseASCIIString(
        databytes, limit, databytes + keyOffset, ^bool(const uint8_t *characters, uint64_t count) {
          if (__CFPropertyListKeyBufferMatchesFilterBuffer(
                  (const char *)characters, (OCFIndex)count, filter, filterLength
              )) {
              *outKey = OCFStringCreateWithBytes(allocator, characters, (OCFIndex)count, 0x0600, false);
          }
          return true;
        }
    );
}

// Native __CFPropertyListCreateFilteredDictionary, 0x1c0166c54, with
// ____CFPropertyListCreateFilteredDictionary_block_invoke (0x1c0167a84). Non-external.
// The key paths are split; without a first-level set the result is NULL (nextKeys is then not released). The
// dictionary header and its references are checked as in __CFBinaryPlistCopyTopLevelKeys; a failure releases
// both split sets and returns NULL. A __block count of the keys still wanted starts at the first-level set's
// count, or -1 when CFSetApply's block finds a wildcard component (__CFPropertyListFilterKeyHasWildcard), and
// the walk stops when it reaches 0. A single first-level key whose C string (CFStringGetCStringPtr, or
// CFStringGetCString into 16 bytes, kCFStringEncodingMacRoman) is nonempty enables the ASCII fast path for
// ASCII string keys; any other key is parsed with the system default allocator, mutability option
// 0xFFFFFFFF80000000 (no-copy strings), depth 0 and no key paths, must not be a set, dictionary, array or
// type 0 (otherwise the result fails and the parsed key is not released), and is kept when it is a string
// accepted by
// __CFPropertyListKeyPassesFilterSet (copied with allocator). Kept keys get their value parsed with
// mutability, depth + 1 and the second-level key paths (into no object when skipValues); each parsed value
// decrements a positive count, and non-NULL values are set in the result. A parse or reference failure
// releases the result and returns NULL. The split sets are released and the __block variable disposed on
// every path after it exists. The native CFSetApply takes a noescape block, so the native block has no copy
// or dispose helpers (descriptor 0x22279ca98); Collections/OCFSet.h does not declare the parameter noescape.
static OCFMutableDictionaryRef __CFPropertyListCreateFilteredDictionary(
    OCFAllocatorRef allocator,
    OCFSetRef keyPaths,
    const uint8_t *databytes,
    uint64_t datalen,
    uint64_t startOffset,
    const BinaryPlistTrailer *trailer,
    bool skipValues,
    OCFOptionFlags mutability,
    OCFDataRef backingData,
    OCFMutableDictionaryRef objects,
    OCFMutableSetRef set,
    uint64_t curDepth
) {
    OCFSetRef theseKeys = NULL;
    OCFSetRef nextKeys = NULL;
    __CFPropertyListCreateSplitKeypaths(kOCFAllocatorSystemDefault, keyPaths, &theseKeys, &nextKeys);
    if (theseKeys == NULL) {
        return NULL;
    }
    uint64_t objectsEnd = trailer->offsetTableOffset - 1;
    uint64_t refsOffset = startOffset + 1;
    uint64_t count = 0;
    bool headerValid = false;
    do {
        if (objectsEnd < startOffset) {
            break;
        }
        const uint8_t *markerPtr = databytes + startOffset;
        if ((uintptr_t)markerPtr == UINTPTR_MAX) {
            break;
        }
        uint8_t marker = *markerPtr;
        if ((marker & 0xf0) != 0xd0) {
            break;
        }
        count = marker & 0x0f;
        if (count == 0x0f) {
            if (objectsEnd <= startOffset) {
                break;
            }
            uint8_t integerMarker = databytes[startOffset + 1];
            if ((integerMarker & 0xf0) != 0x10) {
                break;
            }
            uint64_t exponent = integerMarker & 0x0f;
            uint64_t byteCount = UINT64_C(1) << exponent;
            const uint8_t *integerBytes = databytes + startOffset + 2;
            if ((uintptr_t)integerBytes + byteCount < (uintptr_t)integerBytes) {
                break;
            }
            refsOffset = startOffset + 2 + byteCount;
            if ((uintptr_t)databytes + objectsEnd < (uintptr_t)databytes + refsOffset - 1) {
                break;
            }
            count = 0;
            if (exponent <= 7) {
                count = readBigEndian(integerBytes, byteCount);
                if (count >> 63) {
                    break;
                }
            }
        }
        uint64_t referencesBytes = 0;
        if (count != 0 &&
            __builtin_mul_overflow(count << 1, (uint64_t)trailer->objectRefSize, &referencesBytes)) {
            break;
        }
        uintptr_t references = (uintptr_t)databytes + refsOffset;
        if (references + referencesBytes < references) {
            break;
        }
        if ((uintptr_t)databytes + objectsEnd < references + referencesBytes - 1) {
            break;
        }
        headerValid = true;
    } while (0);
    if (!headerValid) {
        OCFRelease(theseKeys);
        if (nextKeys != NULL) {
            OCFRelease(nextKeys);
        }
        return NULL;
    }
    uint64_t refSize = trailer->objectRefSize;
    __block OCFIndex remaining = OCFSetGetCount(theseKeys);
    CFSetApply(theseKeys, ^(const void *value, Boolean *stop) {
      if (__CFPropertyListFilterKeyHasWildcard(value)) {
          remaining = -1;
          *stop = true;
      }
    });
    char filterBuffer[16] = {0};
    const char *filter = NULL;
    OCFIndex filterLength = 0;
    if (OCFSetGetCount(theseKeys) == 1) {
        OCFStringRef onlyKey = NULL;
        OCFSetGetValues(theseKeys, (const void **)&onlyKey);
        filter = OCFStringGetCStringPtr(onlyKey, 0);
        if (filter == NULL) {
            if (OCFStringGetCString(onlyKey, filterBuffer, 16, 0)) {
                filter = filterBuffer;
            }
        }
        if (filter != NULL) {
            filterLength = (OCFIndex)strlen(filter);
        }
    }
    OCFMutableDictionaryRef result = OCFDictionaryCreateMutable(
        allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    if (count != 0) {
        uint64_t valuesDistance = count * refSize;
        bool noFastPath = filterLength == 0 || filter == NULL;
        OCFPropertyListRef value = NULL;
        OCFPropertyListRef *valuePlist = skipValues ? NULL : &value;
        for (uint64_t idx = 0; idx != count; ++idx) {
            if (remaining == 0) {
                break;
            }
            if ((int64_t)refsOffset < 8) {
                goto failResult;
            }
            uint64_t tableOffset = trailer->offsetTableOffset;
            uintptr_t table = (uintptr_t)databytes + tableOffset;
            uint8_t keyRefSize = trailer->objectRefSize;
            if (table - keyRefSize < (uintptr_t)databytes + refsOffset) {
                goto failResult;
            }
            uint64_t keyRef = readBigEndian(databytes + refsOffset, keyRefSize);
            if (trailer->numObjects <= keyRef) {
                goto failResult;
            }
            uint8_t offsetSize = trailer->offsetIntSize;
            uint64_t keyOffset = readBigEndian((const uint8_t *)(table + keyRef * offsetSize), offsetSize);
            OCFStringRef key = NULL;
            bool failed = false;
            if (!noFastPath && (databytes[keyOffset] & 0xf0) == 0x50) {
                failed = !__CFPropertyListTryFilteredDictionaryASCIIFastPath(
                    databytes, tableOffset - 1, keyOffset, filter, filterLength, allocator, &key
                );
            } else {
                OCFPropertyListRef keyObject = NULL;
                OCFTypeID keyType = 0;
                if (!__CFBinaryPlistCreateObjectFiltered(
                        databytes,
                        datalen,
                        keyOffset,
                        trailer,
                        kOCFAllocatorSystemDefault,
                        (OCFOptionFlags)(int64_t)INT32_MIN,
                        backingData,
                        objects,
                        set,
                        0,
                        NULL,
                        &keyObject,
                        &keyType
                    ) ||
                    keyType - kBinaryPlistSetTypeID < 3 || keyType == 0) {
                    failed = true;
                } else {
                    if (keyType == kBinaryPlistStringTypeID &&
                        __CFPropertyListKeyPassesFilterSet(keyObject, theseKeys)) {
                        key = OCFStringCreateCopy(allocator, keyObject);
                    }
                    if (keyObject != NULL) {
                        OCFRelease(keyObject);
                    }
                }
            }
            if (key == NULL) {
                if (failed) {
                    goto failResult;
                }
            } else {
                uint64_t valueRefOffset = refsOffset + valuesDistance;
                if ((int64_t)valueRefOffset < 8) {
                    OCFRelease(key);
                    goto failResult;
                }
                uintptr_t valueTable = (uintptr_t)databytes + trailer->offsetTableOffset;
                uint8_t valueRefSize = trailer->objectRefSize;
                if (valueTable - valueRefSize < (uintptr_t)databytes + valueRefOffset) {
                    OCFRelease(key);
                    goto failResult;
                }
                uint64_t valueRef = readBigEndian(databytes + valueRefOffset, valueRefSize);
                if (trailer->numObjects <= valueRef) {
                    OCFRelease(key);
                    goto failResult;
                }
                uint8_t valueOffsetSize = trailer->offsetIntSize;
                uint64_t valueOffset = readBigEndian(
                    (const uint8_t *)(valueTable + valueRef * valueOffsetSize), valueOffsetSize
                );
                value = NULL;
                if (!__CFBinaryPlistCreateObjectFiltered(
                        databytes,
                        datalen,
                        valueOffset,
                        trailer,
                        allocator,
                        mutability,
                        backingData,
                        objects,
                        set,
                        curDepth + 1,
                        nextKeys,
                        valuePlist,
                        NULL
                    )) {
                    OCFRelease(key);
                    goto failResult;
                }
                if (remaining - 1 >= 0) {
                    remaining = remaining - 1;
                }
                if (value != NULL) {
                    OCFDictionarySetValue(result, key, value);
                    OCFRelease(value);
                }
                OCFRelease(key);
            }
            refsOffset += trailer->objectRefSize;
        }
    }
    goto finish;
failResult:
    OCFRelease(result);
    result = NULL;
finish:
    if (theseKeys != NULL) {
        OCFRelease(theseKeys);
    }
    if (nextKeys != NULL) {
        OCFRelease(nextKeys);
    }
    return result;
}

// Native __CFBinaryPlistCreateObject, 0x1c01674ac. External. iOS Device divergence: unread.
bool __OCFBinaryPlistCreateObject(
    const uint8_t *bytes,
    uint64_t length,
    uint64_t offset,
    const BinaryPlistTrailer *trailer,
    OCFAllocatorRef allocator,
    OCFOptionFlags options,
    OCFMutableDictionaryRef objects,
    OCFPropertyListRef *object
) {
    return __CFBinaryPlistCreateObjectFiltered(
        bytes, length, offset, trailer, allocator, options, NULL, objects, NULL, 0, NULL, object, NULL
    );
}

// Native __CFTryParseBinaryPlist, 0x1c01674e0. Non-external (was a private external).
// Data shorter than 8 bytes or rejected by __CFDataGetBinaryPlistTopLevelInfo is not binary (false, nothing
// stored). Otherwise the top object is parsed with option as the mutability option, no backing data, set or
// key paths, and an objects cache (system default allocator, NULL key callbacks, CFType value callbacks)
// whose capacity is set to the object count. On success the object is stored through plist when plist is
// nonnull (and leaked otherwise); on failure NULL is stored through plist and the retained constant "binary
// data is corrupt" through errorString, each when nonnull. The cache is released on both paths.
bool __CFTryParseBinaryPlist(
    OCFAllocatorRef allocator,
    OCFDataRef data,
    OCFOptionFlags option,
    OCFPropertyListRef *plist,
    OCFStringRef *errorString
) {
    const uint8_t *databytes = OCFDataGetBytePtr(data);
    uint64_t datalen = (uint64_t)OCFDataGetLength(data);
    if (datalen < 8) {
        return false;
    }
    uint8_t marker = 0;
    uint64_t offset = 0;
    BinaryPlistTrailer trailer;
    memset(&trailer, 0, sizeof(trailer));
    if (!__CFDataGetBinaryPlistTopLevelInfo(data, &marker, &offset, &trailer)) {
        return false;
    }
    OCFMutableDictionaryRef objects =
        OCFDictionaryCreateMutable(kOCFAllocatorSystemDefault, 0, NULL, &kOCFTypeDictionaryValueCallBacks);
    _OCFDictionarySetCapacity(objects, (OCFIndex)trailer.numObjects);
    OCFPropertyListRef pl = NULL;
    bool result = __CFBinaryPlistCreateObjectFiltered(
        databytes, datalen, offset, &trailer, allocator, option, NULL, objects, NULL, 0, NULL, &pl, NULL
    );
    if (result) {
        if (plist != NULL) {
            *plist = pl;
        }
    } else {
        if (plist != NULL) {
            *plist = NULL;
        }
        if (errorString != NULL) {
            *errorString = (OCFStringRef)OCFRetain(OF_STR("binary data is corrupt"));
        }
    }
    OCFRelease(objects);
    return result;
}

// Native __CFBinaryPlistGetTopLevelInfo, iOS Simulator 24A434, 0x1c0163b94,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External; its one caller is
// __CFDataGetBinaryPlistTopLevelInfo (0x1c018ad74).
// iOS Device divergence: __CFBinaryPlistGetTopLevelInfo at 0x18061ce78 (UUID
// 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified) adds a stack guard and moves the 8-byte
// and 4-byte top-object offset reads out of line (.cold.1 0x1807b9410 into _OUTLINED_FUNCTION_2_0, .cold.2
// 0x18070d4d4 into _OUTLINED_FUNCTION_1_2; LDR + REV + STR through X1, the simulator's reads). The
// checks, their order and the stores agree.
// The header is the seven bytes "bplist0" (two overlapping 32-bit compares at 0 and 3); the version
// digit is not tested. Trailer fields are big-endian in the last 32 bytes. Each check fails with false
// and no store; the order below is the native order. Every offset-table entry must be at most
// offsetTableOffset - 1 and the top object's offset must lie in [8, offsetTableOffset). The native
// function switches on the entry width (1, 2, 4 and 8 with REV loads, a byte loop otherwise); every
// case yields the big-endian value of the byte loop below. On success the trailer is stored, then
// *offset, then *marker (the byte at the top object's offset), each only when nonnull.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool __OCFBinaryPlistGetTopLevelInfo(
    const uint8_t *databytes, uint64_t datalen, uint8_t *marker, uint64_t *offset, BinaryPlistTrailer *trailer
) {
    if (databytes == NULL || datalen < 0x29) {
        return false;
    }
    uint32_t head = 0;
    uint32_t tail = 0;
    __builtin_memcpy(&head, databytes, 4);
    __builtin_memcpy(&tail, databytes + 3, 4);
    if (head != 0x696c7062 || tail != 0x30747369) {
        return false;
    }
    const uint8_t *end = databytes + datalen;
    uint64_t numObjects = 0;
    __builtin_memcpy(&numObjects, end - 24, 8);
    if (numObjects == 0) {
        return false;
    }
    numObjects = __builtin_bswap64(numObjects);
    if (numObjects >> 63) {
        return false;
    }
    uint64_t offsetTableOffset = 0;
    __builtin_memcpy(&offsetTableOffset, end - 8, 8);
    offsetTableOffset = __builtin_bswap64(offsetTableOffset);
    if (offsetTableOffset >> 63) {
        return false;
    }
    uint64_t topObject = 0;
    __builtin_memcpy(&topObject, end - 16, 8);
    topObject = __builtin_bswap64(topObject);
    if (numObjects <= topObject) {
        return false;
    }
    if (offsetTableOffset < 9) {
        return false;
    }
    if (datalen - 32 <= offsetTableOffset) {
        return false;
    }
    uint8_t offsetIntSize = end[-26];
    if (offsetIntSize == 0) {
        return false;
    }
    uint8_t objectRefSize = end[-25];
    if (objectRefSize == 0) {
        return false;
    }
    uint64_t tableSize = 0;
    if (__builtin_mul_overflow(numObjects, (uint64_t)offsetIntSize, &tableSize)) {
        return false;
    }
    if (tableSize == 0) {
        return false;
    }
    if (offsetTableOffset + tableSize > UINT64_C(0xffffffffffffffdf)) {
        return false;
    }
    if (offsetTableOffset > ~tableSize) {
        return false;
    }
    if (datalen != offsetTableOffset + tableSize + 32) {
        return false;
    }
    if (objectRefSize <= 7 && (numObjects >> (8 * objectRefSize)) != 0) {
        return false;
    }
    uintptr_t base = (uintptr_t)databytes;
    if (base > UINT64_C(0xfffffffffffffff6)) {
        return false;
    }
    if (offsetIntSize <= 7 && (offsetTableOffset >> (8 * offsetIntSize)) != 0) {
        return false;
    }
    uintptr_t table = 0;
    if (__builtin_add_overflow(base, (uintptr_t)offsetTableOffset, &table)) {
        return false;
    }
    uintptr_t tableLast = 0;
    if (__builtin_add_overflow(table, (uintptr_t)(tableSize - 1), &tableLast)) {
        return false;
    }
    const uint8_t *entry = (const uint8_t *)table;
    for (uint64_t index = 0; index < numObjects; ++index) {
        uint64_t value = 0;
        for (uint8_t byte = 0; byte < offsetIntSize; ++byte) {
            value = (value << 8) | entry[byte];
        }
        if (offsetTableOffset - 1 < value) {
            return false;
        }
        entry += offsetIntSize;
    }
    const uint8_t *topEntry = (const uint8_t *)(table + topObject * offsetIntSize);
    uint64_t topOffset = 0;
    for (uint8_t byte = 0; byte < offsetIntSize; ++byte) {
        topOffset = (topOffset << 8) | topEntry[byte];
    }
    if (topOffset < 8 || offsetTableOffset <= topOffset) {
        return false;
    }
    if (trailer != NULL) {
        uint32_t leading = 0;
        uint16_t following = 0;
        __builtin_memcpy(&leading, end - 32, 4);
        __builtin_memcpy(&following, end - 28, 2);
        __builtin_memcpy(trailer->unused + 4, &following, 2);
        __builtin_memcpy(trailer->unused, &leading, 4);
        trailer->offsetIntSize = offsetIntSize;
        trailer->objectRefSize = objectRefSize;
        trailer->numObjects = numObjects;
        trailer->topObject = topObject;
        trailer->offsetTableOffset = offsetTableOffset;
    }
    if (offset != NULL) {
        *offset = topOffset;
    }
    if (marker != NULL) {
        *marker = databytes[topOffset];
    }
    return true;
}

// Native writeBytes, 0x1c0167648. Non-external (a namesake of another native unit is at 0x1c02fb290).
// Nothing is done for fewer than one byte or after an error. A fixed output buffer takes the bytes at the
// written offset unless they would pass its capacity (or overflow): kCFPropertyListWriteStreamError
// "...because databytes is full." Then a data stream appends them (unless presizing) and the written count
// grows; any other stream is written in a loop (presizing, or no stream, counts everything at once): a zero
// result records "...because stream is full.", a negative one the stream's error or "...because the stream
// had an unknown error.".
static void writeBytes(BinaryPlistWriteBuffer *buf, const uint8_t *bytes, OCFIndex length, bool presize) {
    if (length < 1 || buf->error != NULL) {
        return;
    }
    if (buf->databytes != NULL) {
        uint64_t written = buf->written;
        if ((uint64_t)length + written < written || buf->datalen < written + (uint64_t)length) {
            buf->error = __CFPropertyListCreateError(
                0xF0B,
                OF_STR("Binary property list writing could not be completed because databytes is full.")
            );
            return;
        }
        if (!presize) {
            memmove(buf->databytes + written, bytes, (size_t)length);
        }
    }
    if (buf->streamIsData) {
        if (!presize && buf->stream != NULL) {
            OCFDataAppendBytes((OCFMutableDataRef)buf->stream, bytes, length);
        }
        buf->written += (uint64_t)length;
        return;
    }
    do {
        OCFIndex result = length;
        if (!presize && buf->stream != NULL) {
            result = OCFWriteStreamWrite((OCFWriteStreamRef)buf->stream, bytes, length);
            if (result == 0) {
                buf->error = __CFPropertyListCreateError(
                    0xF0B,
                    OF_STR("Binary property list writing could not be completed because stream is full.")
                );
                return;
            }
            if (result < 0) {
                OCFErrorRef streamError =
                    buf->stream != NULL ? OCFWriteStreamCopyError((OCFWriteStreamRef)buf->stream) : NULL;
                if (streamError == NULL) {
                    streamError = __CFPropertyListCreateError(
                        0xF0B,
                        OF_STR(
                            "Binary property list writing could not be completed because the stream had an "
                            "unknown error."
                        )
                    );
                }
                buf->error = streamError;
                return;
            }
        }
        buf->written += (uint64_t)result;
        bytes += result;
        length -= result;
    } while (length > 0);
}

// Native _appendString, 0x1c0167774. Non-external. The string is first converted to ASCII (CFStringGetBytes
// into a 1024-byte stack buffer, or an unchecked allocation of its length from the system default allocator,
// type 0x100004077774924). When every character converts, marker 0x50 with the byte count (0xf and an integer
// from 15 up) precedes the bytes; otherwise marker 0x60 with the length precedes the big-endian UTF-16 units
// (an unchecked allocation of twice the length, type 0x1000040BDFB0063, freed after the write). A heap ASCII
// buffer is freed.
static void _appendString(BinaryPlistWriteBuffer *buf, OCFStringRef string, bool presize) {
    uint8_t stackBuffer[1024];
    OCFIndex length = OCFStringGetLength(string);
    uint8_t *bytes = stackBuffer;
    if (length >= 0x401) {
        bytes = OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, length, 0x100004077774924, 0);
    }
    OCFIndex used = 0;
    OCFIndex converted =
        OCFStringGetBytes(string, __OCFRangeMake(0, length), 0x600, 0, false, bytes, length, &used);
    if (converted == length) {
        uint8_t marker = (uint8_t)((used < 15 ? used : 15) | 0x50);
        bufferWrite(buf, &marker, 1, presize);
        if (used >= 15) {
            _appendInt(buf, (uint64_t)used, presize);
        }
        bufferWrite(buf, bytes, used, presize);
    } else {
        uint8_t marker = (uint8_t)((length < 15 ? length : 15) | 0x60);
        bufferWrite(buf, &marker, 1, presize);
        if (length >= 15) {
            _appendInt(buf, (uint64_t)length, presize);
        }
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFIndex size = length << 1;
        UniChar *characters = OCFAllocatorAllocateTyped(allocator, size, 0x1000040BDFB0063, 0);
        OCFStringGetCharacters(string, __OCFRangeMake(0, length), characters);
        for (OCFIndex idx = 0; idx < length; ++idx) {
            characters[idx] = __builtin_bswap16(characters[idx]);
        }
        bufferWrite(buf, (const uint8_t *)characters, size, presize);
        OCFAllocatorDeallocate(allocator, characters);
    }
    if (bytes != stackBuffer) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, bytes);
    }
}

// Native _appendInt, 0x1c01679ac. Non-external. Marker 0x10, 0x11, 0x12 or 0x13 for values below 0x100,
// 0x10000, 2^32 and the rest (unsigned), then the low 1, 2, 4 or 8 bytes of the big-endian value.
static void _appendInt(BinaryPlistWriteBuffer *buf, uint64_t value, bool presize) {
    uint8_t marker;
    OCFIndex size;
    if (value < 0x100) {
        marker = 0x10;
        size = 1;
    } else if (value < 0x10000) {
        marker = 0x11;
        size = 2;
    } else if ((value >> 32) == 0) {
        marker = 0x12;
        size = 4;
    } else {
        marker = 0x13;
        size = 8;
    }
    uint64_t swapped = __builtin_bswap64(value);
    bufferWrite(buf, &marker, 1, presize);
    bufferWrite(buf, (const uint8_t *)&swapped + 8 - size, size, presize);
}
