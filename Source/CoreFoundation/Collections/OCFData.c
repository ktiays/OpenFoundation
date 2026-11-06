//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Collections/OCFData.h"

#include "CoreFoundation/Base/OCFFileUtilities.h"
#include "CoreFoundation/Base/OCFHashBytes.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/PropertyList/OCFBinaryPList.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <_assert.h>
#include <mach/vm_page_size.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/log.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Inferred organization: the CFData functions 0x1c01893d0-0x1c018ae4c are contiguous in the iOS Simulator
// __text section, __CFDataCreateFromURL (0x1c02562d0) lies with the file utilities, and the cold fragments
// (0x1c0307a5c-0x1c0307d4c) and outlined assertion fragments (0x1c018afb8-0x1c018b074) belong to them. The
// native source file is CFData.c (the __assert_rtn file name of __CFDataValidateRange, which the native
// CFDataReplaceBytes inlines); the directory follows the native Collections grouping. Unless a definition
// says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS
// Device 24A446: unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json.

// libdyld (no SDK declaration); supplied.
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);

// The build versions CFDataCreateCopy and CFDataReplaceBytes ask dyld about (MOV X0, #0x7E10901FFFFFFFF and
// #0x7E60901FFFFFFFF): any platform, versions 0x07e10901 and 0x07e60901.
static const OCFBuildVersion kCopyRetainsVersion = {0xffffffff, 0x07e10901};
static const OCFBuildVersion kValidateRangeAssertsVersion = {0xffffffff, 0x07e60901};

// The malloc type ID of the byte stores grown or copied by __CFDataGrow and CFDataReplaceBytes.
static const uint64_t kDataStoreTypeID = UINT64_C(0x100004077774924);

// The CFData object (48 bytes): the length at 16, the capacity at 24, the bytes deallocator at 32 and the
// bytes at 40; inline bytes start at the first 16-byte boundary after the object, (cf + 63) & ~15. Info bits:
// 0 and 1 the variety (0 immutable, 1 fixed-capacity mutable, 3 growable mutable), 2 inline bytes, 3 bytes
// owned through the allocator, 4 bytes not deallocated, 6 bytes to be zeroed when the length grows. The field
// names are local descriptive spellings.
struct __OCFData {
    OCFRuntimeBase base;
    OCFIndex length;
    OCFIndex capacity;
    OCFAllocatorRef bytesDeallocator;
    uint8_t *bytes;
};

_Static_assert(sizeof(struct __OCFData) == 48, "The native CFData object is 48 bytes.");

enum {
    kDataVarietyMask = 0x3,
    kDataFixedMutable = 0x1,
    kDataGrowable = 0x2,
    kDataGrowableMutable = 0x3,
    kDataInlineBytes = 0x4,
    kDataUsesAllocator = 0x8,
    kDataDontDeallocate = 0x10,
    kDataNeedsToZero = 0x40,
};

static void __CFDataDeallocate(OCFTypeRef cf);
static Boolean __CFDataEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFDataHash(OCFTypeRef cf);
static OCFStringRef __CFDataCopyDescription(OCFTypeRef cf);
static OCFMutableDataRef __CFDataInit(
    OCFAllocatorRef allocator,
    uint32_t flags,
    OCFIndex capacity,
    const uint8_t *bytes,
    OCFIndex length,
    OCFAllocatorRef bytesDeallocator
);
static bool __CFDataShouldUseAllocator(OCFAllocatorRef allocator);
static void __CFDataGrow(OCFMutableDataRef data, OCFIndex numNewValues, bool clear);
static void *__CFDataAllocate(OCFTypeRef data, size_t size, bool clear);
__attribute__((noreturn)) static void __CFDataHandleOutOfMemory(OCFIndex numBytes);

// Native __CFDataClass, 0x22279d5d0 (__DATA_CONST,__const). Hidden (was private external); version 1, name
// "CFData", finalize, equal, hash and the debug description.
const OCFRuntimeClass __CFDataClass = {
    1,
    "CFData",
    NULL,
    NULL,
    __CFDataDeallocate,
    __CFDataEqual,
    __CFDataHash,
    NULL,
    __CFDataCopyDescription,
    NULL,
    NULL,
    0,
};

static bool isTagged(OCFTypeRef cf) {
    return ((uintptr_t)cf >> 63) != 0;
}

// The native dispatch test of every entry point: tagged pointers and Objective-C objects receive a message.
static bool isObjCData(OCFTypeRef cf) {
    return isTagged(cf) || __CF_IS_OBJC(20, cf) != 0;
}

static uint64_t infoOf(OCFTypeRef cf) {
    return __atomic_load_n(&((const OCFRuntimeBase *)cf)->info, __ATOMIC_ACQUIRE);
}

// One native CASAL loop: the bits in clear are replaced by set.
static void updateInfo(OCFTypeRef cf, uint64_t clear, uint64_t set) {
    uint64_t *info = &((OCFRuntimeBase *)cf)->info;
    uint64_t current = __atomic_load_n(info, __ATOMIC_ACQUIRE);
    uint64_t updated;
    do {
        updated = (current & ~clear) | set;
    } while (
        !__atomic_compare_exchange_n(info, &current, updated, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
}

static uint8_t *inlineBytes(OCFTypeRef cf) {
    return (uint8_t *)(((uintptr_t)cf + 63) & ~(uintptr_t)15);
}

// The inlined byte-pointer read: the inline store when info bit 2 is set, else the bytes field.
static uint8_t *bytesOf(OCFTypeRef cf) {
    if ((infoOf(cf) & kDataInlineBytes) != 0) {
        return inlineBytes(cf);
    }
    return ((const struct __OCFData *)cf)->bytes;
}

// The inlined allocator read of an untagged object: the system default (info bit 7) or the allocator stored
// 16 bytes before the object.
static OCFAllocatorRef storedAllocator(OCFTypeRef cf) {
    if ((infoOf(cf) & 0x80) != 0) {
        return kOCFAllocatorSystemDefault;
    }
    return *(const OCFAllocatorRef *)((const uint8_t *)cf - 16);
}

// Native __CFDataDeallocate, 0x1c01893d0. Non-external; the class finalize. Inline and not-deallocated stores
// are left alone. A bytes deallocator frees the bytes and is released (its field keeps the stale pointer, as
// natively); allocator-owned bytes go back to the object's allocator (the system default for a tagged
// pointer); other bytes are freed. The bytes field is cleared last.
static void __CFDataDeallocate(OCFTypeRef cf) {
    if ((infoOf(cf) & kDataInlineBytes) != 0) {
        return;
    }
    if ((infoOf(cf) & kDataDontDeallocate) != 0) {
        return;
    }
    struct __OCFData *data = (struct __OCFData *)cf;
    OCFAllocatorRef deallocator = data->bytesDeallocator;
    if (deallocator != NULL) {
        OCFAllocatorDeallocate(deallocator, data->bytes);
        OCFRelease(deallocator);
    } else if ((infoOf(cf) & kDataUsesAllocator) != 0) {
        OCFAllocatorRef allocator = isTagged(cf) ? kOCFAllocatorSystemDefault : storedAllocator(cf);
        OCFAllocatorDeallocate(allocator, data->bytes);
    } else if (data->bytes != NULL) {
        free(data->bytes);
    }
    data->bytes = NULL;
}

// Native __CFDataEqual, 0x1c0189478. Non-external. Equal lengths and the same byte pointer, or equal bytes.
static Boolean __CFDataEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    OCFIndex length = ((const struct __OCFData *)cf1)->length;
    if (length != ((const struct __OCFData *)cf2)->length) {
        return false;
    }
    const uint8_t *bytes1 = bytesOf(cf1);
    const uint8_t *bytes2 = bytesOf(cf2);
    if (bytes1 == bytes2) {
        return true;
    }
    return memcmp(bytes1, bytes2, (size_t)length) == 0;
}

// Native __CFDataHash, 0x1c01894f4. Non-external. CFHashBytes over at most the first 80 bytes.
static OCFHashCode __CFDataHash(OCFTypeRef cf) {
    const uint8_t *bytes = bytesOf(cf);
    OCFIndex length = ((const struct __OCFData *)cf)->length;
    return OCFHashBytes(bytes, length < 80 ? length : 80);
}

// Native __CFDataCopyDescription, 0x1c0189528. Non-external. Up to 24 bytes are written in full ("%02x"
// each); longer data writes its first 16 bytes and, after " ... ", the bytes from length - 8 on, four per
// "%02x%02x%02x%02x".
static OCFStringRef __CFDataCopyDescription(OCFTypeRef cf) {
    const struct __OCFData *data = (const struct __OCFData *)cf;
    OCFIndex length = data->length;
    const uint8_t *bytes = bytesOf(cf);
    OCFMutableStringRef result = OCFStringCreateMutable(OCFGetAllocator(cf), 0);
    OCFStringAppendFormat(
        result,
        NULL,
        OF_STR("<CFData %p [%p]>{length = %lu, capacity = %lu, bytes = 0x"),
        cf,
        OCFGetAllocator(cf),
        length,
        data->capacity
    );
    if (length >= 25) {
        for (OCFIndex index = 0;; index += 4) {
            OCFStringAppendFormat(
                result,
                NULL,
                OF_STR("%02x%02x%02x%02x"),
                bytes[index],
                bytes[index + 1],
                bytes[index + 2],
                bytes[index + 3]
            );
            if ((uint64_t)index >= 12) {
                break;
            }
        }
        OCFStringAppend(result, OF_STR(" ... "));
        for (OCFIndex index = length - 8; index < length; index += 4) {
            OCFStringAppendFormat(
                result,
                NULL,
                OF_STR("%02x%02x%02x%02x"),
                bytes[index],
                bytes[index + 1],
                bytes[index + 2],
                bytes[index + 3]
            );
        }
    } else if (length >= 1) {
        for (OCFIndex remaining = length; remaining != 0; --remaining) {
            OCFStringAppendFormat(result, NULL, OF_STR("%02x"), *bytes++);
        }
    }
    OCFStringAppend(result, OF_STR("}"));
    return result;
}

// Native CFDataGetTypeID, 0x1c01896a0. External.
OCFTypeID OCFDataGetTypeID(void) {
    return 20;
}

// Native __CFDataMtbl, 0x1c01896a8. External. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json. Info bit 0 (the mutable bit) of an acquire load
// of the info word.
Boolean __OCFDataMtbl(OCFDataRef data) {
    return __atomic_load_n(&((const OCFRuntimeBase *)data)->info, __ATOMIC_ACQUIRE) & 1;
}

// Native __CFDataCreateDispatchData, 0x1c01896b8, with ____CFDataCreateDispatchData_block_invoke
// (0x1c018998c, the destructor block, descriptor ___block_descriptor_40_e5_v8_?0l, flags 0xc0000000). Hidden
// (was private external). Objective-C data answer -_createDispatchData; CF data are copied with their own
// allocator and the dispatch data wrap the copy's bytes (no queue), releasing the copy when destroyed.
dispatch_data_t _CFDataCreateDispatchData(OCFDataRef data) {
    if (isObjCData(data)) {
        return ((dispatch_data_t (*)(id, SEL))objc_msgSend)((id)data, sel_getUid("_createDispatchData"));
    }
    OCFDataRef copy = OCFDataCreateCopy(OCFGetAllocator(data), data);
    const UInt8 *bytes = OCFDataGetBytePtr(copy);
    size_t length;
    if (isObjCData(copy)) {
        length = ((size_t (*)(id, SEL))objc_msgSend)((id)copy, sel_getUid("length"));
    } else {
        length = (size_t)copy->length;
    }
    return dispatch_data_create(bytes, length, NULL, ^{
      OCFRelease(copy);
    });
}

// Native CFDataCreateCopy, 0x1c01897cc. External. From the 0x07e10901 SDKs on, Objective-C data answer -copy,
// and immutable CF data whose bytes need no other owner (inline, or no bytes deallocator) and whose byte
// allocator matches the requested one (NULL for the allocators __CFDataShouldUseAllocator rejects) are
// retained. Otherwise the bytes are copied into new immutable data.
OCFDataRef OCFDataCreateCopy(OCFAllocatorRef allocator, OCFDataRef theData) {
    if (dyld_program_sdk_at_least(kCopyRetainsVersion)) {
        if (isObjCData(theData)) {
            return ((OCFDataRef (*)(id, SEL))objc_msgSend)((id)theData, sel_getUid("copy"));
        }
        if ((infoOf(theData) & kDataFixedMutable) == 0) {
            OCFAllocatorRef requested = __CFDataShouldUseAllocator(allocator) ? allocator : NULL;
            OCFAllocatorRef current = NULL;
            if ((infoOf(theData) & kDataUsesAllocator) != 0) {
                current = storedAllocator(theData);
            }
            if (requested == current &&
                ((infoOf(theData) & kDataInlineBytes) != 0 || theData->bytesDeallocator == NULL)) {
                return OCFRetain(theData);
            }
        }
    }
    OCFIndex length;
    if (isObjCData(theData)) {
        length = ((OCFIndex (*)(id, SEL))objc_msgSend)((id)theData, sel_getUid("length"));
    } else {
        length = theData->length;
    }
    return __CFDataInit(allocator, 0, length, OCFDataGetBytePtr(theData), length, NULL);
}

// Native CFDataGetBytePtr, 0x1c01898ec. External.
const UInt8 *OCFDataGetBytePtr(OCFDataRef theData) {
    if (isObjCData(theData)) {
        return ((const UInt8 *(*)(id, SEL))objc_msgSend)((id)theData, sel_getUid("bytes"));
    }
    return bytesOf(theData);
}

// Native CFDataGetLength, 0x1c0189948. External.
OCFIndex OCFDataGetLength(OCFDataRef theData) {
    if (isObjCData(theData)) {
        return ((OCFIndex (*)(id, SEL))objc_msgSend)((id)theData, sel_getUid("length"));
    }
    return theData->length;
}

// Native CFDataCreate, 0x1c0189994. External.
OCFDataRef OCFDataCreate(OCFAllocatorRef allocator, const UInt8 *bytes, OCFIndex length) {
    return __CFDataInit(allocator, 0, length, bytes, length, NULL);
}

// Native __CFDataInit, 0x1c01899a8, with its function-local statics __CFDataInit.didCheck (0x22e6d27b2) and
// __CFDataInit.setToNull (0x22e6d27b1, __DATA,__bss). Non-external. flags: bit 0 mutable, bit 1 growable.
// Immutable and fixed-capacity data without a bytes deallocator keep their bytes inline when the capacity is
// below 4 * vm_page_size - 63 (unsigned; vm_page_size is the libsystem_kernel variable, bound by the GOT slot
// 0x22241a5b0 that __CFAllocatorCustomValloc also reads; confidence: medium for that binding). The instance
// has capacity + 47 extra bytes when inline, else 32. A bytes deallocator adopts the bytes; otherwise the
// store is allocated (cleared when the executable predates system version 6 for fixed-capacity data) and the
// bytes are copied in with CFDataReplaceBytes. Empty immutable inline data get a NULL byte pointer and lose
// the inline flag, except in /Remote ("com.apple.Remote" before 2.1, checked once), which keeps the inline
// store. The object is mutable (variety 1 or 3) while its bytes are copied in and immutable data clear the
// variety last.
static OCFMutableDataRef __CFDataInit(
    OCFAllocatorRef allocator,
    uint32_t flags,
    OCFIndex capacity,
    const uint8_t *bytes,
    OCFIndex length,
    OCFAllocatorRef bytesDeallocator
) {
    static bool didCheck;
    static bool setToNull;
    bool useInline = false;
    if (flags <= 1 && bytesDeallocator == NULL) {
        useInline = (uint64_t)capacity < (uint64_t)(4 * vm_page_size - 63);
    }
    if (allocator == NULL) {
        OCFAllocatorRef current = _OCFGetTSD(1);
        allocator = current != NULL ? current : kOCFAllocatorSystemDefault;
    }
    bool useAllocator = __CFDataShouldUseAllocator(allocator);
    OCFIndex extraBytes = useInline ? capacity + 47 : 32;
    struct __OCFData *data = (struct __OCFData *)_OCFRuntimeCreateInstance(allocator, 20, extraBytes, NULL);
    if (data == NULL) {
        return NULL;
    }
    data->length = 0;
    updateInfo(data, kDataInlineBytes, useInline ? kDataInlineBytes : 0);
    updateInfo(data, kDataUsesAllocator, (uint64_t)useAllocator << 3);
    if (flags >= 2 && (flags & 1) != 0) {
        data->capacity = 16;
        updateInfo(data, 0, kDataGrowableMutable);
    } else {
        data->capacity = capacity;
        updateInfo(data, kDataVarietyMask, kDataFixedMutable);
    }
    if (bytesDeallocator != NULL) {
        data->bytes = (uint8_t *)bytes;
        data->bytesDeallocator = OCFRetain(bytesDeallocator);
        data->length = length;
        if ((flags & 1) == 0) {
            updateInfo(data, kDataVarietyMask, 0);
        }
        return data;
    }
    bool cleared;
    if (flags == 1) {
        cleared = !_OCFExecutableLinkedOnOrAfter(6);
    } else {
        cleared = false;
    }
    if (!useInline) {
        data->bytes = __CFDataAllocate(data, (size_t)data->capacity, cleared);
        if (__OCFOASafe == 1) {
            __OCFSetLastAllocationEventName(data->bytes, "CFData (store)");
        }
        if (data->bytes == NULL) {
            OCFRelease(data);
            return NULL;
        }
    } else {
        cleared = true;
        if (length == 0 && (flags & 1) == 0) {
            data->bytes = NULL;
            updateInfo(data, kDataInlineBytes, 0);
            if (!didCheck) {
                const char *path = _OCFProcessPath();
                const char *name = path != NULL ? strrchr(path, '/') : "";
                if (path == NULL || name != NULL) {
                    if (strcmp(name, "/Remote") == 0 &&
                        _OCFAppVersionCheckLessThan(OF_STR("com.apple.Remote"), 0xffffffff, 2.1)) {
                        setToNull = true;
                    }
                }
                didCheck = true;
            }
            if (setToNull == 1) {
                data->bytes = inlineBytes(data);
                updateInfo(data, 0, kDataInlineBytes);
            }
            cleared = true;
        }
    }
    updateInfo(data, kDataNeedsToZero, ((uint64_t)cleared << 6) ^ kDataNeedsToZero);
    data->bytesDeallocator = NULL;
    OCFDataReplaceBytes(data, __OCFRangeMake(0, 0), bytes, length);
    if ((flags & 1) == 0) {
        updateInfo(data, kDataVarietyMask, 0);
    }
    return data;
}

// Native CFDataCreateWithBytesNoCopy, 0x1c0189d04. External. A NULL bytes deallocator means the thread's
// default allocator (or the system default).
OCFDataRef OCFDataCreateWithBytesNoCopy(
    OCFAllocatorRef allocator, const UInt8 *bytes, OCFIndex length, OCFAllocatorRef bytesDeallocator
) {
    if (bytesDeallocator == NULL) {
        OCFAllocatorRef current = _OCFGetTSD(1);
        bytesDeallocator = current != NULL ? current : kOCFAllocatorSystemDefault;
    }
    return __CFDataInit(allocator, 0, length, bytes, length, bytesDeallocator);
}

// Native __CFDataShouldUseAllocator, 0x1c0189d68. Non-external. kCFAllocatorDefault resolves to the thread's
// default (or the system default); the system default, malloc and malloc-zone allocators answer false.
static bool __CFDataShouldUseAllocator(OCFAllocatorRef allocator) {
    if (allocator == kOCFAllocatorDefault) {
        OCFAllocatorRef current = _OCFGetTSD(1);
        allocator = current != NULL ? current : kOCFAllocatorSystemDefault;
    }
    if (allocator == kOCFAllocatorSystemDefault || allocator == kOCFAllocatorMalloc) {
        return false;
    }
    return allocator != kOCFAllocatorMallocZone;
}

// Native _CFDataCreateFixedMutableWithBuffer, 0x1c0189dec. Non-external (was a private external).
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC, confidence: verified.
// iOS Device 24A446: unread. Reading: Local/Evidence/cf-core/iphone-simulator-corpus-delta13.json.
OCFMutableDataRef _CFDataCreateFixedMutableWithBuffer(
    OCFAllocatorRef allocator, OCFIndex capacity, const UInt8 *buffer, OCFAllocatorRef deallocator
) {
    return __CFDataInit(allocator, 1, capacity, buffer, 0, deallocator);
}

// Native CFDataCreateMutable, 0x1c0189e04. External. A nonzero capacity is fixed (flags 1), zero growable
// (3).
OCFMutableDataRef OCFDataCreateMutable(OCFAllocatorRef allocator, OCFIndex capacity) {
    return __CFDataInit(allocator, capacity != 0 ? 1 : 3, capacity, NULL, 0, NULL);
}

// Native CFDataCreateMutableCopy, 0x1c0189e24. External. The byte pointer is read before the length.
OCFMutableDataRef OCFDataCreateMutableCopy(OCFAllocatorRef allocator, OCFIndex capacity, OCFDataRef theData) {
    const UInt8 *bytes = OCFDataGetBytePtr(theData);
    OCFIndex length;
    if (isObjCData(theData)) {
        length = ((OCFIndex (*)(id, SEL))objc_msgSend)((id)theData, sel_getUid("length"));
    } else {
        length = theData->length;
    }
    return __CFDataInit(allocator, capacity != 0 ? 1 : 3, capacity, bytes, length, NULL);
}

// Native _CFDataGetBytePtrNonObjC, 0x1c0189ea0. Hidden (was a private external). The byte pointer without
// dispatch (delta capture 4).
uint8_t *_CFDataGetBytePtrNonObjC(OCFDataRef data) {
    return bytesOf(data);
}

// Native CFDataGetMutableBytePtr, 0x1c0189ec0. External.
UInt8 *OCFDataGetMutableBytePtr(OCFMutableDataRef theData) {
    if (isObjCData(theData)) {
        return ((UInt8 * (*)(id, SEL)) objc_msgSend)((id)theData, sel_getUid("mutableBytes"));
    }
    return bytesOf(theData);
}

// Native CFDataSetLength, 0x1c018a03c, with CFDataSetLength.cold.1 (0x1c0307bd4). External. Only mutable data
// change; immutable data store the length unchecked. A negative length traps (BRK #1) for fixed-capacity data
// and raises the out-of-memory exception for growable data. A length beyond the capacity grows growable data
// (cleared) and crashes for fixed-capacity data; a longer length zeroes the new bytes when info bit 6 is set;
// a shorter length sets it.
void OCFDataSetLength(OCFMutableDataRef theData, OCFIndex length) {
    if (isObjCData(theData)) {
        ((void (*)(id, SEL, OCFIndex))objc_msgSend)((id)theData, sel_getUid("setLength:"), length);
        return;
    }
    OCFIndex oldLength = theData->length;
    OCFIndex capacity = theData->capacity;
    uint64_t variety = infoOf(theData) & 0xff;
    if ((infoOf(theData) & kDataFixedMutable) != 0) {
        if (length < 0) {
            if ((variety & kDataGrowable) == 0) {
                __builtin_trap();
            }
            __CFDataHandleOutOfMemory(length);
        }
        if (capacity < length) {
            if ((variety & kDataGrowable) == 0) {
                crashWithMessage("fixed-capacity CFMutableData is full");
            }
            __CFDataGrow(theData, length - oldLength, true);
        } else if (length - oldLength > 0 && (infoOf(theData) & kDataNeedsToZero) != 0) {
            bzero(bytesOf(theData) + oldLength, (size_t)(length - oldLength));
        } else if (length < oldLength) {
            updateInfo(theData, 0, kDataNeedsToZero);
        }
    }
    theData->length = length;
}

// Native __CFDataGrow, 0x1c018a138. Non-external. The new capacity for length + numNewValues (out of memory
// at 2^42 or more): 16 below 16; below 2^20, 2^(f + (f & 1)) for f = flsl(length); below 2^32,
// 2^flsl(length); else 2^29 + (length >> 29) * 2^29 (the shift is flsl(2^29) - 1, computed by a call), at
// most 2^42 - 1. A clearing grow beyond 2^17 bytes of malloc-owned bytes that at least quintuples a nonempty
// length (or starts from empty) moves the bytes into a fresh cleared store and deallocates the old one;
// otherwise the store is reallocated (through the object's allocator when info bit 3 is set) and the new
// bytes zeroed when clearing. Info bit 6 ends set except after the fresh-store path. The reallocation through
// the allocator passes the malloc type ID in X3 and 0 in X4, the hint and type-ID parameters of
// __CFSafelyReallocateWithAllocatorTyped as the allocator unit restores it; the native argument order is
// kept.
static void __CFDataGrow(OCFMutableDataRef data, OCFIndex numNewValues, bool clear) {
    OCFIndex oldLength = data->length;
    uint64_t desiredLength = (uint64_t)oldLength + (uint64_t)numNewValues;
    if ((desiredLength >> 42) != 0) {
        __CFDataHandleOutOfMemory((OCFIndex)desiredLength);
    }
    uint64_t capacity;
    if (desiredLength < 16) {
        capacity = 16;
    } else if ((desiredLength >> 20) == 0) {
        int last = flsl((long)desiredLength);
        capacity = (uint64_t)1 << ((uint64_t)(last & 1) + (uint64_t)(int64_t)last);
    } else if ((desiredLength >> 32) == 0) {
        capacity = (uint64_t)1 << flsl((long)desiredLength);
    } else {
        int last = flsl(0x20000000);
        uint64_t rounded = 0x20000000 + ((desiredLength >> ((int64_t)last - 1)) << 29);
        capacity = rounded < UINT64_C(0x3FFFFFFFFFF) ? rounded : UINT64_C(0x3FFFFFFFFFF);
    }
    OCFAllocatorRef allocator = OCFGetAllocator(data);
    uint8_t *bytes = data->bytes;
    uint8_t *newBytes;
    uint64_t needsToZero;
    if (clear) {
        uint64_t info = infoOf(data);
        if ((int64_t)capacity > 0x20000 && (info & kDataUsesAllocator) == 0 &&
            (infoOf(data) & kDataUsesAllocator) == 0 &&
            (oldLength == 0 || (OCFIndex)desiredLength / oldLength >= 5)) {
            newBytes = __CFDataAllocate(data, (size_t)capacity, true);
            if (newBytes != NULL) {
                memmove(newBytes, bytes, (size_t)oldLength);
                __CFDataDeallocate(data);
                needsToZero = 0;
                goto store;
            }
        }
    }
    if ((infoOf(data) & kDataUsesAllocator) != 0) {
        newBytes = __OCFSafelyReallocateWithAllocatorTyped(
            allocator, bytes, (OCFIndex)capacity, kDataStoreTypeID, 0, NULL
        );
    } else {
        newBytes = __OCFSafelyReallocateTyped(bytes, (size_t)capacity, kDataStoreTypeID, NULL);
    }
    if (newBytes == NULL) {
        __CFDataHandleOutOfMemory((OCFIndex)capacity);
    }
    needsToZero = kDataNeedsToZero;
    if (numNewValues >= 1 && clear) {
        bzero(newBytes + oldLength, (size_t)numNewValues);
        needsToZero = kDataNeedsToZero;
    }
store:
    data->capacity = (OCFIndex)capacity;
    updateInfo(data, kDataNeedsToZero, needsToZero);
    data->bytes = newBytes;
    if (__OCFOASafe == 1) {
        __OCFSetLastAllocationEventName(newBytes, "CFData (store)");
    }
}

// Native CFDataAppendBytes, 0x1c018a3c4. External. A replacement of the empty range at the end.
void OCFDataAppendBytes(OCFMutableDataRef theData, const UInt8 *bytes, OCFIndex length) {
    if (isObjCData(theData)) {
        ((void (*)(id, SEL, const UInt8 *, OCFIndex))objc_msgSend)(
            (id)theData, sel_getUid("appendBytes:length:"), bytes, length
        );
        return;
    }
    OCFDataReplaceBytes(theData, __OCFRangeMake(theData->length, 0), bytes, length);
}

// Native CFDataReplaceBytes, 0x1c018a434, with the inlined __CFDataValidateRange,
// CFDataReplaceBytes.cold.1-13 (0x1c0307be8-0x1c0307d4c) and _OUTLINED_FUNCTION_0_5, 1_4, 2_3, 3_1 (the
// os_log fault of _CFOSLog, type 0x11, a two-byte empty buffer) and 4_1-7_1 (the __assert_rtn calls of
// __CFDataValidateRange, "CFData.c", lines 218 to 221). External. The range is validated in the order
// location < 0, location > length (signed), length < 0, location + length > length (unsigned): from the
// 0x07e60901 SDKs on the first failure asserts, earlier SDKs log a fault and continue. A negative range
// length, a negative new length, a negative resulting length and NULL bytes with a nonzero length then crash
// with their messages. Fixed-capacity data crash when the result exceeds the capacity; growable data grow
// (not cleared), first copying new bytes that overlap the old store. The tail moves when the lengths differ,
// then the new bytes are copied in.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_1_4 (0x1c018afcc), _OUTLINED_FUNCTION_2_3
// (0x1c018afd8), _OUTLINED_FUNCTION_3_1 (0x1c018afe4).
// Folds the compiler-outlined pieces _CFDataReplaceBytes.cold.2 (0x1c0307c1c), _CFDataReplaceBytes.cold.3
// (0x1c0307c28), _CFDataReplaceBytes.cold.4 (0x1c0307c5c), _CFDataReplaceBytes.cold.5 (0x1c0307c68),
// _CFDataReplaceBytes.cold.6 (0x1c0307c9c), _CFDataReplaceBytes.cold.7 (0x1c0307ca8),
// _CFDataReplaceBytes.cold.8 (0x1c0307cdc), _CFDataReplaceBytes.cold.9 (0x1c0307cf0),
// _CFDataReplaceBytes.cold.10 (0x1c0307d04), _CFDataReplaceBytes.cold.11 (0x1c0307d18),
// _CFDataReplaceBytes.cold.12 (0x1c0307d2c), _CFDataReplaceBytes.cold.13 (0x1c0307d40).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_4_1 (0x1c018aff4).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_5_1 (0x1c018b014).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_6_1 (0x1c018b034).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_7_1 (0x1c018b054).
void OCFDataReplaceBytes(
    OCFMutableDataRef theData, OCFRange range, const UInt8 *newBytes, OCFIndex newLength
) {
    if (isObjCData(theData)) {
        ((void (*)(id, SEL, OCFRange, const UInt8 *, OCFIndex))objc_msgSend)(
            (id)theData, sel_getUid("replaceBytesInRange:withBytes:length:"), range, newBytes, newLength
        );
        return;
    }
    OCFIndex length = theData->length;
    OCFIndex capacity = theData->capacity;
    if (range.location < 0) {
        if (dyld_program_sdk_at_least(kValidateRangeAssertsVersion)) {
            __assert_rtn("__CFDataValidateRange", "CFData.c", 218, "range.location >= 0");
        }
        os_log_fault(_CFOSLog(), "range.location out of range (<0)");
    } else if (range.location > length) {
        if (dyld_program_sdk_at_least(kValidateRangeAssertsVersion)) {
            __assert_rtn("__CFDataValidateRange", "CFData.c", 219, "range.location <= dataLength");
        }
        os_log_fault(_CFOSLog(), "range.location out of range (>len)");
    } else if (range.length < 0) {
        if (dyld_program_sdk_at_least(kValidateRangeAssertsVersion)) {
            __assert_rtn("__CFDataValidateRange", "CFData.c", 220, "range.length >= 0");
        }
        os_log_fault(_CFOSLog(), "length cannot be less than zero");
    } else if ((uint64_t)range.location + (uint64_t)range.length > (uint64_t)length) {
        if (dyld_program_sdk_at_least(kValidateRangeAssertsVersion)) {
            __assert_rtn(
                "__CFDataValidateRange", "CFData.c", 221, "range.location + range.length <= dataLength"
            );
        }
        os_log_fault(_CFOSLog(), "ending index out of bounds");
    }
    if (range.length < 0) {
        crashWithMessage("Negative range.length passed to CFDataReplaceBytes");
    }
    if (newLength < 0) {
        crashWithMessage("Negative buffer length passed to CFDataReplaceBytes");
    }
    OCFIndex newCount = (OCFIndex)((uint64_t)(length - range.length) + (uint64_t)newLength);
    if (newCount < 0) {
        crashWithMessage("Invalid range passed to CFDataReplaceBytes");
    }
    if (newBytes == NULL && newLength != 0) {
        crashWithMessage("Invalid length passed to CFDataReplaceBytes when newBytes == NULL");
    }
    uint8_t *bytes = bytesOf(theData);
    const uint8_t *source = newBytes;
    uint64_t variety = infoOf(theData) & kDataVarietyMask;
    if (variety == kDataFixedMutable) {
        if (newCount > capacity) {
            crashWithMessage("fixed-capacity CFMutableData is full");
        }
    } else if (variety == kDataGrowableMutable && capacity < newCount) {
        if (newBytes != NULL && bytes != NULL && (uintptr_t)newBytes < (uintptr_t)(bytes + capacity) &&
            (uintptr_t)bytes < (uintptr_t)(newBytes + newLength)) {
            void *copy = malloc_type_malloc((size_t)newLength, kDataStoreTypeID);
            if (copy == NULL) {
                __CFDataHandleOutOfMemory(newLength);
            }
            memmove(copy, newBytes, (size_t)newLength);
            source = copy;
        }
        __CFDataGrow(theData, newCount - length, false);
        bytes = bytesOf(theData);
    }
    bool copyBytes = newLength != 0 && source != NULL;
    if (newLength != range.length) {
        OCFIndex tail = length - (range.location + range.length);
        if (tail >= 1) {
            memmove(bytes + range.location + newLength, bytes + range.location + range.length, (size_t)tail);
        }
    }
    if (copyBytes) {
        memmove(bytes + range.location, source, (size_t)newLength);
    }
    if (source != newBytes) {
        free((void *)source);
    }
    theData->length = newCount;
}

// Native __CFDataGetBinaryPlistTopLevelInfo, 0x1c018acd4. Hidden (was private external). Objective-C data
// answer -_getBPlistMarker:offset:trailer:; the length of CF data is read after a second __CF_IS_OBJC test.
bool __CFDataGetBinaryPlistTopLevelInfo(
    OCFDataRef data, uint8_t *marker, uint64_t *offset, BinaryPlistTrailer *trailer
) {
    if (isObjCData(data)) {
        return ((bool (*)(id, SEL, uint8_t *, uint64_t *, BinaryPlistTrailer *))objc_msgSend)(
            (id)data, sel_getUid("_getBPlistMarker:offset:trailer:"), marker, offset, trailer
        );
    }
    const UInt8 *bytes = OCFDataGetBytePtr(data);
    uint64_t length;
    if (__CF_IS_OBJC(20, data) != 0) {
        length = ((uint64_t (*)(id, SEL))objc_msgSend)((id)data, sel_getUid("length"));
    } else {
        length = (uint64_t)data->length;
    }
    return __OCFBinaryPlistGetTopLevelInfo(bytes, length, marker, offset, trailer);
}

// Native __CFDataAllocate, 0x1c018ad7c. Non-external. Allocator-owned stores come from the object's allocator
// (malloc type 0x5890d4f0, hint 0) and are zeroed when clear is set; other stores are calloc'ed (0xe0159386)
// or malloc'ed (0x4e71426a).
// CoreFoundation defect (kept): an allocator that returns NULL with clear set makes bzero write through NULL;
// confidence: verified (disassembly, no test of the result before bzero). Reached only on allocation failure
// of a custom allocator; the native behavior is kept and never exercised by tests.
static void *__CFDataAllocate(OCFTypeRef data, size_t size, bool clear) {
    if ((infoOf(data) & kDataUsesAllocator) == 0) {
        if (clear) {
            return malloc_type_calloc(1, size, 0xE0159386);
        }
        return malloc_type_malloc(size, 0x4E71426A);
    }
    OCFAllocatorRef allocator = isTagged(data) ? kOCFAllocatorSystemDefault : storedAllocator(data);
    void *bytes = OCFAllocatorAllocateTyped(allocator, (OCFIndex)size, 0x5890D4F0, 0);
    if (clear) {
        bzero(bytes, size);
    }
    return bytes;
}

// Native __CFDataCreateFromURL, 0x1c02562d0. Hidden (was private external). The file is read with the system
// default allocator (no size limit, open flags 4) into data that adopt the bytes; a failure stores an
// NSOSStatusErrorDomain error with code -10 when error is not NULL.
OCFDataRef _CFDataCreateFromURL(OCFURLRef url, OCFErrorRef *error) {
    void *bytes = NULL;
    OCFIndex length = 0;
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    if (_CFReadBytesFromFile(allocator, url, &bytes, &length, 0, 4)) {
        return OCFDataCreateWithBytesNoCopy(allocator, bytes, length, allocator);
    }
    if (error != NULL) {
        *error = OCFErrorCreate(allocator, kOCFErrorDomainOSStatus, -10, NULL);
    }
    return NULL;
}

// Native __CFDataHandleOutOfMemory, 0x1c0307a5c. Non-external. Raises NSMallocException through
// _CFRaiseMemoryException; sizes outside 1 ... 2^42 - 1 also name the maximum size.
__attribute__((noreturn)) static void __CFDataHandleOutOfMemory(OCFIndex numBytes) {
    OCFStringRef reason;
    if ((uint64_t)numBytes - 1 > UINT64_C(0x3FFFFFFFFFE)) {
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("Attempt to allocate %ld bytes for NS/CFData failed. Maximum size: %lld"),
            numBytes,
            (long long)0x3FFFFFFFFFF
        );
    } else {
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("Attempt to allocate %ld bytes for NS/CFData failed"),
            numBytes
        );
    }
    _CFRaiseMemoryException(reason);
}
