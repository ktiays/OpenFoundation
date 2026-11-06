//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"

#include <stdbool.h>
#include <stdint.h>

// Callback shapes read from the nine wrappers' argument and return registers. Every callback returns
// its result in W0 and the wrappers test or zero-extend the whole 32-bit register (CBZ W0, MOV Wn, W0).
typedef uint32_t (*StringEncodingCheapEightBitToBytesProc)(uint32_t flags, uint16_t character, uint8_t *byte);

typedef uint32_t (*StringEncodingStandardEightBitToBytesProc)(
    uint32_t flags, const uint16_t *characters, OCFIndex numChars, uint8_t *byte
);

typedef uint32_t (*StringEncodingCheapMultiByteToBytesProc)(
    uint32_t flags, uint16_t character, uint8_t *bytes
);

typedef uint32_t (*StringEncodingCheapEightBitToUnicodeProc)(
    uint32_t flags, uint8_t byte, uint16_t *character
);

typedef uint32_t (*StringEncodingStandardEightBitToUnicodeProc)(
    uint32_t flags, uint8_t byte, uint16_t *characters
);

typedef uint32_t (*StringEncodingCheapMultiByteToUnicodeProc)(
    uint32_t flags, const uint8_t *bytes, OCFIndex numBytes, uint16_t *character
);

typedef OCFIndex (*StringEncodingStandardToBytesProc)(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

typedef OCFIndex (*StringEncodingStandardToUnicodeProc)(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

typedef OCFIndex (*StringEncodingToBytesLenProc)(
    uint32_t flags, const uint16_t *characters, OCFIndex numChars
);

typedef OCFIndex (*StringEncodingToUnicodeLenProc)(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes);

typedef OCFIndex (*StringEncodingToBytesFallbackProc)(
    const uint16_t *characters, OCFIndex numChars, uint8_t *bytes, OCFIndex maxByteLen, OCFIndex *usedByteLen
);

typedef OCFIndex (*StringEncodingToUnicodeFallbackProc)(
    const uint8_t *bytes, OCFIndex numBytes, uint16_t *characters, OCFIndex maxCharLen, OCFIndex *usedCharLen
);

typedef OCFIndex (*StringEncodingToBytesPrecomposeProc)(
    uint32_t flags,
    const uint16_t *character,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

typedef bool (*StringEncodingIsValidCombiningCharProc)(uint16_t character);

// The converter definition record, 72 bytes (__DATA_CONST,__const), read from the 64 native records and from
// their readers: the wrappers read offsets 0 and 8; __CFGetConverter reads the class byte at offset 20 (LDRB)
// and the fallbacks at 40 and 48 (LDP); CFStringEncodingUnicodeToBytes reads maxBytesPerChar as a halfword at
// offset 16 (LDRH) and the precompose and combining-character callbacks at 56 and 64. Offset 18 is the second
// halfword of every record (the maximum decomposed length); bytes 21-23 are zero in every record. The
// encoding classes are 0 (standard), 1 (cheap eight-bit), 2 (standard eight-bit), 3 (cheap multibyte), 4
// (platform) and 5 (ICU), from __CFGetConverter's dispatch. StringEncodingConverterRecord is a descriptive
// name; the native type name is not in the evidence.
typedef struct {
    union {
        StringEncodingCheapEightBitToBytesProc cheapEightBit;
        StringEncodingStandardEightBitToBytesProc standardEightBit;
        StringEncodingCheapMultiByteToBytesProc cheapMultiByte;
        StringEncodingStandardToBytesProc standard;
    } toBytes;

    union {
        StringEncodingCheapEightBitToUnicodeProc cheapEightBit;
        StringEncodingStandardEightBitToUnicodeProc standardEightBit;
        StringEncodingCheapMultiByteToUnicodeProc cheapMultiByte;
        StringEncodingStandardToUnicodeProc standard;
    } toUnicode;

    uint16_t maxBytesPerChar;
    uint16_t maxDecomposedCharLen;
    uint8_t encodingClass;
    StringEncodingToBytesLenProc toBytesLen;
    StringEncodingToUnicodeLenProc toUnicodeLen;
    StringEncodingToBytesFallbackProc toBytesFallback;
    StringEncodingToUnicodeFallbackProc toUnicodeFallback;
    StringEncodingToBytesPrecomposeProc toBytesPrecompose;
    StringEncodingIsValidCombiningCharProc isValidCombiningChar;
} StringEncodingConverterRecord;

typedef OCFIndex (*StringEncodingToBytesWrapperProc)(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

typedef OCFIndex (*StringEncodingToUnicodeWrapperProc)(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

// The 48-byte converter value __CFGetConverter (0x1c022a8ac) returns through X8: the definition record, the
// class's three wrappers (to bytes, to Unicode, to canonical Unicode) and the two fallbacks, defaulted to
// __CFDefaultToBytesFallbackProc and __CFDefaultToUnicodeFallbackProc. For the ICU class (5) the to-bytes
// slot holds the ICU converter name from __CFStringEncodingGetICUName and the other wrapper slots stay NULL;
// the standard (0) and platform (4) classes leave all three wrapper slots NULL. An unknown encoding yields 48
// zero bytes. StringEncodingRuntimeConverter is a descriptive name; the native type name is not in the
// evidence.
typedef struct {
    const StringEncodingConverterRecord *definition;

    union {
        StringEncodingToBytesWrapperProc wrapper;
        const char *icuName;
    } toBytes;

    StringEncodingToUnicodeWrapperProc toUnicode;
    StringEncodingToUnicodeWrapperProc toCanonicalUnicode;
    StringEncodingToBytesFallbackProc toBytesFallback;
    StringEncodingToUnicodeFallbackProc toUnicodeFallback;
} StringEncodingRuntimeConverter;

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN OCFIndex __CFToBytesCheapEightBitWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFToUnicodeCheapEightBitWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToCanonicalUnicodeCheapEightBitWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToBytesStandardEightBitWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFToUnicodeStandardEightBitWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToCanonicalUnicodeStandardEightBitWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToBytesCheapMultiByteWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFToUnicodeCheapMultiByteWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFToCanonicalUnicodeCheapMultiByteWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN OCFIndex __CFDefaultToBytesFallbackProc(
    const uint16_t *characters, OCFIndex numChars, uint8_t *bytes, OCFIndex maxByteLen, OCFIndex *usedByteLen
);

OF_EXTERN OCFIndex __CFDefaultToUnicodeFallbackProc(
    const uint8_t *bytes, OCFIndex numBytes, uint16_t *characters, OCFIndex maxCharLen, OCFIndex *usedCharLen
);

// The converter registry, 0x1c0229f60-0x1c022b268 (see OCFStringEncodingConverter.c).
// CFStringEncodingUnicodeToBytes and CFStringEncodingBytesToUnicode are external; the others were private
// externals and keep their native hidden names.
OF_EXPORT uint32_t OCFStringEncodingUnicodeToBytes(
    uint32_t encoding,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    OCFIndex *usedCharLen,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

OF_EXTERN const StringEncodingConverterRecord *CFStringEncodingGetConverter(uint32_t encoding);

OF_EXPORT uint32_t OCFStringEncodingBytesToUnicode(
    uint32_t encoding,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    OCFIndex *usedByteLen,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

OF_EXTERN bool CFStringEncodingIsValidEncoding(uint32_t encoding);

OF_EXTERN OCFIndex CFStringEncodingCharLengthForBytes(
    uint32_t encoding, uint32_t flags, const uint8_t *bytes, OCFIndex numBytes
);

OF_EXTERN OCFIndex CFStringEncodingByteLengthForCharacters(
    uint32_t encoding, uint32_t flags, const uint16_t *characters, OCFIndex numChars
);

// __CFGetConverter (0x1c022a8ac, non-external) returns its 48-byte value through X8.
OF_EXTERN StringEncodingRuntimeConverter __CFGetConverter(uint32_t encoding);
