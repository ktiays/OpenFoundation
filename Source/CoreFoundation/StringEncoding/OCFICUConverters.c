//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFICUConverters.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"

#include <stdbool.h>
#include <string.h>
#include <strings.h>

// Inferred organization: __CFStringEncodingICUToBytes (0x1c02adc60),
// __CFStringEncodingConverterReleaseICUConverter (0x1c02ae02c), __CFStringEncodingICUToUnicode (0x1c02ae2d0),
// __CFStringEncodingICUCharLength (0x1c02ae5cc) and __CFStringEncodingICUByteLength (0x1c02ae634) are
// contiguous in the iOS Simulator __text section, between __CFStringEncodingGetFromICUName and
// __CFStringEncodingCreateICUEncodings; the name functions around them are in OCFStringEncodingDatabase.c.
// The native source path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.
//
// ICU (libicucore) is a library the native image links (/usr/lib/libicucore.A.dylib); it is supplied, never
// reimplemented. The SDK installs no ucnv.h, so the called entry points are declared here from their call
// sites: UErrorCode is a 32-bit slot, UBool is passed in W6 as 0 or 1, the substitution and invalid-unit
// lengths are signed bytes (int8_t). The two callback addresses come from GOT slots outside this image
// extraction (0x222407878 for the from-Unicode callback, 0x222407880 for the to-Unicode callback); the image
// imports exactly one callback of each direction from libicucore, UCNV_FROM_U_CALLBACK_STOP and
// UCNV_TO_U_CALLBACK_STOP, so the binding is by elimination (confidence: medium).
typedef struct UConverter UConverter;

typedef void (*UConverterFromUCallback)(
    const void *context,
    void *fromUArgs,
    const uint16_t *codeUnits,
    int32_t length,
    int32_t codePoint,
    int32_t reason,
    int32_t *pErrorCode
);

typedef void (*UConverterToUCallback)(
    const void *context,
    void *toUArgs,
    const char *codeUnits,
    int32_t length,
    int32_t reason,
    int32_t *pErrorCode
);

extern UConverter *ucnv_open(const char *converterName, int32_t *err);

extern void ucnv_close(UConverter *converter);

extern void ucnv_setSubstChars(UConverter *converter, const char *subChars, int8_t len, int32_t *err);

extern void ucnv_setFromUCallBack(
    UConverter *converter,
    UConverterFromUCallback newAction,
    const void *newContext,
    UConverterFromUCallback *oldAction,
    const void **oldContext,
    int32_t *err
);

extern void ucnv_setToUCallBack(
    UConverter *converter,
    UConverterToUCallback newAction,
    const void *newContext,
    UConverterToUCallback *oldAction,
    const void **oldContext,
    int32_t *err
);

extern void ucnv_fromUnicode(
    UConverter *converter,
    char **target,
    const char *targetLimit,
    const uint16_t **source,
    const uint16_t *sourceLimit,
    int32_t *offsets,
    bool flush,
    int32_t *err
);

extern void ucnv_toUnicode(
    UConverter *converter,
    uint16_t **target,
    const uint16_t *targetLimit,
    const char **source,
    const char *sourceLimit,
    int32_t *offsets,
    bool flush,
    int32_t *err
);

extern void ucnv_resetFromUnicode(UConverter *converter);

extern void
ucnv_getInvalidUChars(const UConverter *converter, uint16_t *errUChars, int8_t *len, int32_t *err);

extern void ucnv_getInvalidChars(const UConverter *converter, char *errBytes, int8_t *len, int32_t *err);

extern void UCNV_FROM_U_CALLBACK_STOP(
    const void *context,
    void *fromUArgs,
    const uint16_t *codeUnits,
    int32_t length,
    int32_t codePoint,
    int32_t reason,
    int32_t *pErrorCode
);

extern void UCNV_TO_U_CALLBACK_STOP(
    const void *context,
    void *toUArgs,
    const char *codeUnits,
    int32_t length,
    int32_t reason,
    int32_t *pErrorCode
);

// U_BUFFER_OVERFLOW_ERROR, the literal 15 at every native comparison.
#define kICUBufferOverflowError 15

// The 16-byte per-thread converter cache in TSD slot 7, read from its three users: the slot count (byte 0),
// the next free slot (byte 1) and the converter array (offset 8). ICUThreadData is a descriptive name; the
// native type name is not in the evidence.
typedef struct {
    uint8_t numSlots;
    uint8_t nextSlot;
    UConverter **converters;
} ICUThreadData;

// The thread-data lookup inlined at each of its three uses: TSD slot 7, created on first use as 16 zero bytes
// from the default allocator (malloc type 0x1080040ef48e34e) with __CFICUThreadDataDestructor. The allocator
// argument is the NULL returned by _CFGetTSD (the native code reuses X0).
static inline ICUThreadData *icuThreadData(void) {
    ICUThreadData *data = _OCFGetTSD(7);
    if (data == NULL) {
        data = OCFAllocatorAllocateTyped(NULL, 16, 0x1080040ef48e34eULL, 0);
        data->numSlots = 0;
        data->nextSlot = 0;
        data->converters = NULL;
        _OCFSetTSD(7, data, __CFICUThreadDataDestructor);
    }
    return data;
}

static OCFIndex
__CFStringEncodingConverterReleaseICUConverter(UConverter *converter, uint32_t flags, OCFIndex status);

// Native __CFStringEncodingICUToBytes, 0x1c02adc60. Hidden (was private external).
// The converter comes from the per-thread slot (flags >> 16) - 1 when flags & 0xff0000 and the slot is
// filled, otherwise from ucnv_open (failure: 3) with the substitution byte '?' for (flags & 0xff000001) == 1,
// flags >> 24 when nonzero, or the stop callback. Bit 12 clear flushes. With a capacity, an overflow backs
// the source limit up before the last unit (a pair as one value) while that unit is in character set 8, the
// non-base set, and converts again from the start; the overflow status is kept. The count-only path converts
// through a 1000-byte buffer. Status: 0 for U_ZERO_ERROR, 2 for an overflow, 1 for any other code; for 1 the
// consumed count excludes the invalid units ICU reports, or is 0 (with *usedByteLen 0) when that query fails.
// Returns the status OR'd with the streaming value of __CFStringEncodingConverterReleaseICUConverter.
// CoreFoundation defect: the native back-up loop reads the unit before the source position without a bounds
// check: at source == characters (the first unit did not fit) it reads characters[-1], and for a low
// surrogate at characters[0] it reads characters[-1] as the high unit. In both cases the native outcome is
// the reset to the start, because the backed-up limit is at or before characters; the restoration takes that
// outcome without the out-of-bounds reads (and without the GetBitmapPtrForPlane call the stray pair would
// make). CoreFoundation defect (kept): when the unit before the limit is a non-base character the native loop
// re-reads the same unchanged source position forever (0x1c02ade60 branches back to 0x1c02addf8, which
// reloads the source pointer that nothing has updated). The hang is reproduced.
OCFIndex __CFStringEncodingICUToBytes(
    const char *icuName,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    OCFIndex *usedCharLen,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    int32_t errorCode = 0;
    char *destination = (char *)bytes;
    const uint16_t *source = characters;
    bool flush = (flags & 0x1000) == 0;
    uint16_t buffer[500];
    UConverter *converter = NULL;
    if ((flags & 0xff0000) != 0) {
        ICUThreadData *data = icuThreadData();
        uint8_t slot = (uint8_t)((flags >> 16) - 1);
        if (data->numSlots > slot) {
            converter = data->converters[slot];
        }
    }
    if (converter == NULL) {
        int32_t openError = 0;
        converter = ucnv_open(icuName, &openError);
        if (converter == NULL) {
            return 3;
        }
        char lossyByte = (flags & 0xff000001) == 1 ? '?' : (char)(flags >> 24);
        if (lossyByte != 0) {
            ucnv_setSubstChars(converter, &lossyByte, 1, &openError);
        } else {
            ucnv_setFromUCallBack(converter, UCNV_FROM_U_CALLBACK_STOP, NULL, NULL, NULL, &openError);
        }
    }
    const uint16_t *sourceLimit = characters + numChars;
    if (maxByteLen != 0) {
        ucnv_fromUnicode(
            converter,
            &destination,
            (const char *)bytes + maxByteLen,
            &source,
            sourceLimit,
            NULL,
            flush,
            &errorCode
        );
        if (errorCode == kICUBufferOverflowError) {
            const uint8_t *bitmap = OCFUniCharGetBitmapPtrForPlane(8, 0);
            for (;;) {
                if (source <= characters) {
                    destination = (char *)bytes;
                    source = characters;
                    break;
                }
                const uint16_t *limit = source - 1;
                uint32_t character = *limit;
                const uint8_t *planeBitmap = bitmap;
                if ((character & 0xfc00) == 0xdc00) {
                    if (limit <= characters) {
                        destination = (char *)bytes;
                        source = characters;
                        break;
                    }
                    const uint16_t *high = source - 2;
                    uint32_t combined = character + ((uint32_t)*high << 10) + UINT32_C(0xfca02400);
                    planeBitmap = OCFUniCharGetBitmapPtrForPlane(8, (combined >> 16) & 0xf);
                    character = combined & 0xffff;
                    limit = high;
                }
                if (limit <= characters) {
                    destination = (char *)bytes;
                    source = characters;
                    break;
                }
                if (planeBitmap != NULL && ((planeBitmap[character >> 3] >> (character & 7)) & 1) != 0) {
                    continue;
                }
                destination = (char *)bytes;
                source = characters;
                errorCode = 0;
                ucnv_resetFromUnicode(converter);
                ucnv_fromUnicode(
                    converter,
                    &destination,
                    (const char *)bytes + maxByteLen,
                    &source,
                    limit,
                    NULL,
                    flush,
                    &errorCode
                );
                if (errorCode != kICUBufferOverflowError) {
                    break;
                }
            }
            errorCode = kICUBufferOverflowError;
        }
        if (usedByteLen != NULL) {
            *usedByteLen = destination - (char *)bytes;
        }
    } else {
        OCFIndex totalLength = 0;
        if (source < sourceLimit && errorCode == 0) {
            do {
                destination = (char *)buffer;
                ucnv_fromUnicode(
                    converter,
                    &destination,
                    (const char *)buffer + 1000,
                    &source,
                    sourceLimit,
                    NULL,
                    flush,
                    &errorCode
                );
                if (errorCode == kICUBufferOverflowError) {
                    errorCode = 0;
                }
                totalLength += destination - (char *)buffer;
            } while (source < sourceLimit && errorCode == 0);
        }
        if (usedByteLen != NULL) {
            *usedByteLen = totalLength;
        }
    }
    OCFIndex status = errorCode == 0 ? 0 : (errorCode == kICUBufferOverflowError ? 2 : 1);
    if (usedCharLen != NULL) {
        if (status == 1) {
            int8_t length = 32;
            errorCode = 0;
            ucnv_getInvalidUChars(converter, buffer, &length, &errorCode);
            if (errorCode != 0) {
                source = characters;
                if (usedByteLen != NULL) {
                    *usedByteLen = 0;
                }
            } else {
                source -= length;
            }
        }
        *usedCharLen = source - characters;
    }
    return __CFStringEncodingConverterReleaseICUConverter(converter, flags, status) | status;
}

// Native __CFStringEncodingConverterReleaseICUConverter, 0x1c02ae02c. Non-external.
// Keeps the converter for streaming when status is not 1 and either flags bit 12 is set or status is 2 with
// bit 13: an existing streaming value (flags & 0xff0000) is returned as is; otherwise the converter goes into
// the next free per-thread slot (the array starts with 10 zeroed slots, malloc type 0x2004093837f09, and
// grows by 10 with bzero, memcpy and CFAllocatorDeallocate; more than 245 slots logs at level 3, closes the
// converter and returns 0), the next free slot is recomputed, and the value is the one-based slot << 16.
// Otherwise a converter taken from its slot clears the slot (lowering the next free slot), and the converter
// is closed; result 0.
static OCFIndex
__CFStringEncodingConverterReleaseICUConverter(UConverter *converter, uint32_t flags, OCFIndex status) {
    if (status != 1 && ((flags & 0x1000) != 0 || (status == 2 && (flags & 0x2000) != 0))) {
        if ((flags & 0xff0000) != 0) {
            return (OCFIndex)((flags >> 16) & 0xff) << 16;
        }
        ICUThreadData *data = icuThreadData();
        UConverter **converters = data->converters;
        OCFIndex slot;
        OCFIndex count;
        if (converters == NULL) {
            converters = OCFAllocatorAllocateTyped(NULL, 0x50, 0x2004093837f09ULL, 0);
            slot = 0;
            data->converters = converters;
            memset(converters, 0, 0x50);
            data->numSlots = 10;
            converters = data->converters;
            count = 10;
        } else {
            slot = data->nextSlot;
            count = data->numSlots;
            if (slot >= count || converters[slot] != NULL) {
                bool found = false;
                if (count != 0) {
                    for (OCFIndex index = 0; index != count; ++index) {
                        if (converters[index] == NULL) {
                            slot = index;
                            found = true;
                            break;
                        }
                    }
                    if (!found && count > 0xf5) {
                        OCFLog(
                            3, OF_STR("Per-thread streaming ID for ICU converters exhausted. Ignoring...")
                        );
                        ucnv_close(converter);
                        return 0;
                    }
                }
                if (!found) {
                    OCFIndex newCount = count + 10;
                    size_t size = (size_t)newCount * sizeof(void *);
                    converters = OCFAllocatorAllocateTyped(NULL, (OCFIndex)size, 0x2004093837f09ULL, 0);
                    bzero(converters, size);
                    memcpy(converters, data->converters, (size_t)data->numSlots * sizeof(void *));
                    OCFAllocatorDeallocate(NULL, data->converters);
                    data->converters = converters;
                    slot = data->numSlots;
                    data->numSlots = (uint8_t)newCount;
                    count = newCount;
                }
            }
        }
        converters[(uint8_t)slot] = converter;
        slot += 1;
        data->nextSlot = (uint8_t)slot;
        if ((uint8_t)slot >= (uint8_t)count || converters[(uint8_t)slot] != NULL) {
            OCFIndex next = count;
            for (OCFIndex index = 0; index != (uint8_t)count; ++index) {
                if (converters[index] == NULL) {
                    next = index;
                    break;
                }
            }
            data->nextSlot = (uint8_t)next;
        }
        return (OCFIndex)(uint8_t)slot << 16;
    }
    if ((flags & 0xff0000) != 0) {
        ICUThreadData *data = icuThreadData();
        uint8_t slot = (uint8_t)((flags >> 16) - 1);
        if (data->numSlots > slot && data->converters[slot] == converter) {
            data->converters[slot] = NULL;
            if (data->nextSlot > slot) {
                data->nextSlot = slot;
            }
        }
    }
    ucnv_close(converter);
    return 0;
}

// Native __CFStringEncodingICUToUnicode, 0x1c02ae2d0. Hidden (was private external).
// The converter is acquired as in __CFStringEncodingICUToBytes, with the to-Unicode stop callback. With a
// capacity one ucnv_toUnicode call; count-only, rounds through a 1000-unit buffer while bytes remain and no
// error is pending (an overflow is cleared each round). Status as in __CFStringEncodingICUToBytes; for 1 the
// consumed count excludes the invalid bytes ICU reports, less one when the last reported byte is zero, or is
// 0 (with *usedCharLen 0) when that query fails.
OCFIndex __CFStringEncodingICUToUnicode(
    const char *icuName,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    OCFIndex *usedByteLen,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
) {
    int32_t errorCode = 0;
    uint16_t *destination = characters;
    const char *source = (const char *)bytes;
    bool flush = (flags & 0x1000) == 0;
    uint16_t buffer[1000];
    UConverter *converter = NULL;
    if ((flags & 0xff0000) != 0) {
        ICUThreadData *data = icuThreadData();
        uint8_t slot = (uint8_t)((flags >> 16) - 1);
        if (data->numSlots > slot) {
            converter = data->converters[slot];
        }
    }
    if (converter == NULL) {
        int32_t openError = 0;
        converter = ucnv_open(icuName, &openError);
        if (converter == NULL) {
            return 3;
        }
        char lossyByte = (flags & 0xff000001) == 1 ? '?' : (char)(flags >> 24);
        if (lossyByte != 0) {
            ucnv_setSubstChars(converter, &lossyByte, 1, &openError);
        } else {
            ucnv_setToUCallBack(converter, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &openError);
        }
    }
    const char *sourceLimit = (const char *)bytes + numBytes;
    if (maxCharLen != 0) {
        ucnv_toUnicode(
            converter, &destination, characters + maxCharLen, &source, sourceLimit, NULL, flush, &errorCode
        );
        if (usedCharLen != NULL) {
            *usedCharLen = destination - characters;
        }
    } else {
        OCFIndex totalLength = 0;
        if (numBytes >= 1) {
            do {
                destination = buffer;
                ucnv_toUnicode(
                    converter, &destination, buffer + 1000, &source, sourceLimit, NULL, flush, &errorCode
                );
                if (errorCode == kICUBufferOverflowError) {
                    errorCode = 0;
                }
                totalLength += destination - buffer;
            } while (source < sourceLimit && errorCode == 0);
        }
        if (usedCharLen != NULL) {
            *usedCharLen = totalLength;
        }
    }
    OCFIndex status = errorCode == 0 ? 0 : (errorCode == kICUBufferOverflowError ? 2 : 1);
    if (usedByteLen != NULL) {
        if (status == 1) {
            int8_t length = 32;
            errorCode = 0;
            ucnv_getInvalidChars(converter, (char *)buffer, &length, &errorCode);
            if (errorCode != 0) {
                source = (const char *)bytes;
                if (usedCharLen != NULL) {
                    *usedCharLen = 0;
                }
            } else {
                int32_t count = length;
                if (count >= 1) {
                    count -= ((const char *)buffer)[(uint32_t)(count - 1)] == 0 ? 1 : 0;
                }
                source -= (int8_t)count;
            }
        }
        *usedByteLen = source - (const char *)bytes;
    }
    return __CFStringEncodingConverterReleaseICUConverter(converter, flags, status) | status;
}

// Native __CFStringEncodingICUCharLength, 0x1c02ae5cc. Hidden (was private external).
OCFIndex __CFStringEncodingICUCharLength(
    const char *icuName, uint32_t flags, const uint8_t *bytes, OCFIndex numBytes
) {
    OCFIndex usedCharLen = 0;
    OCFIndex result =
        __CFStringEncodingICUToUnicode(icuName, flags, bytes, numBytes, NULL, NULL, 0, &usedCharLen);
    return result == 0 ? usedCharLen : 0;
}

// Native __CFStringEncodingICUByteLength, 0x1c02ae634. Hidden (was private external).
OCFIndex __CFStringEncodingICUByteLength(
    const char *icuName, uint32_t flags, const uint16_t *characters, OCFIndex numChars
) {
    OCFIndex usedByteLen = 0;
    OCFIndex result =
        __CFStringEncodingICUToBytes(icuName, flags, characters, numChars, NULL, NULL, 0, &usedByteLen);
    return result == 0 ? usedByteLen : 0;
}

// Native __CFICUThreadDataDestructor, 0x1c02ae75c. Non-external; the destructor of TSD slot 7. Every non-NULL
// converter among the first numSlots entries is closed (ucnv_close), the array is freed when present, and
// the record itself is freed, both with the default allocator (NULL).
void __CFICUThreadDataDestructor(void *context) {
    ICUThreadData *data = context;
    UConverter **converters = data->converters;
    if (converters != NULL) {
        if (data->numSlots != 0) {
            UConverter **end = converters + data->numSlots;
            do {
                if (*converters != NULL) {
                    ucnv_close(*converters);
                }
                ++converters;
            } while (converters < end);
            converters = data->converters;
        }
        OCFAllocatorDeallocate(NULL, converters);
    }
    OCFAllocatorDeallocate(NULL, data);
}
