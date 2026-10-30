//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PropertyList/OCFBinaryPList.h"

#include <stddef.h>

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
