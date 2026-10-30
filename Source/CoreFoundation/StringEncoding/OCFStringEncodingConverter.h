//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

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

// The converter definition record the wrappers receive in X0. The wrappers read only two slots:
// offset 0 (the to-bytes callback) and offset 8 (the to-unicode callback). Native fields after offset 16
// are not read by this group and are not described here. StringEncodingConverterRecord is a
// descriptive name; the native type name is not in the evidence.
typedef struct {
    union {
        StringEncodingCheapEightBitToBytesProc cheapEightBit;
        StringEncodingStandardEightBitToBytesProc standardEightBit;
        StringEncodingCheapMultiByteToBytesProc cheapMultiByte;
    } toBytes;

    union {
        StringEncodingCheapEightBitToUnicodeProc cheapEightBit;
        StringEncodingStandardEightBitToUnicodeProc standardEightBit;
        StringEncodingCheapMultiByteToUnicodeProc cheapMultiByte;
    } toUnicode;
} StringEncodingConverterRecord;

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
