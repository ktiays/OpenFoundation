//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"

#include <string.h>

// Inferred organization: the nine wrappers (0x1c022b484-0x1c022bf68), __CFDefaultToBytesFallbackProc
// (0x1c022bf68) and __CFDefaultToUnicodeFallbackProc (0x1c022c174) are contiguous in the iOS Simulator
// __text section and are kept in one translation unit. The native source path is not in the evidence.
// __CFStrConvertBytesToUnicode (0x1c022c2a4), which follows them, is an open group: its character table is
// rewritten at run time by the function at 0x1c022c1a4, which calls the unrestored
// CFStringEncodingGetConverter.

// Native __CFToBytesCheapEightBitWrapper, iOS Simulator 24A434, 0x1c022b484,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1806bf188, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: __CFToBytesCheapEightBitWrapper at 0x1806bf188 calls the record's callback
// with BLRAAZ (arm64e pointer authentication, zero discriminator); the iOS Simulator build uses BLR.
// The scratch byte is zeroed once before the loop and keeps a callback's earlier value when a later
// callback succeeds without writing it.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFToBytesCheapEightBitWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    OCFIndex processCharLen = numChars;
    if (maxByteLen != 0) {
        processCharLen = maxByteLen < numChars ? maxByteLen : numChars;
    }
    OCFIndex length = 0;
    if (processCharLen >= 1) {
        uint8_t byte = 0;
        for (; length != processCharLen; ++length) {
            if (converter->toBytes.cheapEightBit(flags, characters[length], &byte) == 0) {
                break;
            }
            if (maxByteLen != 0) {
                bytes[length] = byte;
            }
        }
    }
    *usedByteLen = length;
    return length;
}

// Native __CFToUnicodeCheapEightBitWrapper, iOS Simulator 24A434, 0x1c022b564,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x180769534, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: __CFToUnicodeCheapEightBitWrapper at 0x180769534 calls the record's callback
// with BLRAAZ (arm64e pointer authentication, zero discriminator); the iOS Simulator build uses BLR.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFToUnicodeCheapEightBitWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
) {
    OCFIndex processByteLen = numBytes;
    if (maxCharLen != 0) {
        processByteLen = maxCharLen < numBytes ? maxCharLen : numBytes;
    }
    OCFIndex length = 0;
    if (processByteLen >= 1) {
        uint16_t character = 0;
        for (; length != processByteLen; ++length) {
            if (converter->toUnicode.cheapEightBit(flags, bytes[length], &character) == 0) {
                break;
            }
            if (maxCharLen != 0) {
                characters[length] = character;
            }
        }
    }
    *usedCharLen = length;
    return length;
}

// Native __CFToCanonicalUnicodeCheapEightBitWrapper, iOS Simulator 24A434, 0x1c022b644,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x180769618, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the
// decomposition buffer is indexed instead of walked by pointer. iOS Device divergence:
// __CFToCanonicalUnicodeCheapEightBitWrapper at 0x180769618 calls the record's callback with BLRAAZ (arm64e
// pointer authentication, zero discriminator); the iOS Simulator build uses BLR. Character set 0x65 is passed
// to CFUniCharIsMemberOf. When a decomposition does not fit, the wrapper returns at once: *usedCharLen keeps
// the count stored before the decomposition, and a capacity of zero ends the conversion at the first
// decomposable character, as in the native code. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFToCanonicalUnicodeCheapEightBitWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
) {
    OCFIndex usedLength = 0;
    OCFIndex processedByteLen = 0;
    if (numBytes >= 1) {
        uint16_t character = 0;
        uint32_t decomposed[10];
        for (;;) {
            if (maxCharLen != 0 && usedLength >= maxCharLen) {
                break;
            }
            if (converter->toUnicode.cheapEightBit(flags, bytes[processedByteLen], &character) == 0) {
                break;
            }
            if (character >= 0x80 && OCFUniCharIsMemberOf(character, 0x65) != 0) {
                OCFIndex count = OCFUniCharDecomposeCharacter(character, decomposed, 10);
                *usedCharLen = usedLength;
                if (count >= 1) {
                    uint32_t *cursor = decomposed;
                    do {
                        uint32_t value = *cursor;
                        if (value >= 0x10000) {
                            usedLength += 2;
                            if (usedLength > maxCharLen) {
                                return processedByteLen;
                            }
                            if (maxCharLen != 0) {
                                *cursor = value - 0x10000;
                                characters[0] = (uint16_t)(((value - 0x10000) >> 10) + 0xd800);
                                characters[1] = (uint16_t)(0xdc00 | (value & 0x3ff));
                                characters += 2;
                            }
                        } else {
                            if (usedLength >= maxCharLen) {
                                return processedByteLen;
                            }
                            ++usedLength;
                            *characters++ = (uint16_t)value;
                        }
                        ++cursor;
                    } while (--count != 0);
                }
            } else {
                if (maxCharLen != 0) {
                    *characters++ = character;
                }
                ++usedLength;
            }
            if (++processedByteLen == numBytes) {
                processedByteLen = numBytes;
                break;
            }
        }
    }
    *usedCharLen = usedLength;
    return processedByteLen;
}

// Native __CFToBytesStandardEightBitWrapper, iOS Simulator 24A434, 0x1c022b7d0,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1807697ac, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: __CFToBytesStandardEightBitWrapper at 0x1807697ac calls the record's callback
// with BLRAAZ (arm64e pointer authentication, zero discriminator); the iOS Simulator build uses BLR.
// Only a zero numChars returns early (CBZ); the loop advances by each callback's count until the
// remaining count reaches zero.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFToBytesStandardEightBitWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    *usedByteLen = 0;
    if (numChars == 0) {
        return 0;
    }
    OCFIndex processedCharLen = 0;
    uint8_t byte = 0;
    do {
        uint32_t count;
        if (maxByteLen != 0) {
            if (*usedByteLen >= maxByteLen) {
                break;
            }
            count = converter->toBytes.standardEightBit(flags, characters, numChars, &byte);
            if (count == 0) {
                break;
            }
            bytes[*usedByteLen] = byte;
        } else {
            count = converter->toBytes.standardEightBit(flags, characters, numChars, &byte);
            if (count == 0) {
                break;
            }
        }
        *usedByteLen = *usedByteLen + 1;
        characters += count;
        processedCharLen += count;
        numChars -= count;
    } while (numChars != 0);
    return processedCharLen;
}

// Native __CFToUnicodeStandardEightBitWrapper, iOS Simulator 24A434, 0x1c022b8e8,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1807698cc, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: __CFToUnicodeStandardEightBitWrapper at 0x1807698cc calls the record's
// callback with BLRAAZ (arm64e pointer authentication, zero discriminator); the iOS Simulator build
// uses BLR.
// The callback writes into a 40-byte stack buffer (twenty UTF-16 units; frame offsets 0x0-0x28).
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFToUnicodeStandardEightBitWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
) {
    *usedCharLen = 0;
    if (numBytes < 1) {
        return 0;
    }
    OCFIndex usedLength = 0;
    OCFIndex processedByteLen = 0;
    uint16_t buffer[20];
    for (;;) {
        if (maxCharLen != 0 && usedLength >= maxCharLen) {
            break;
        }
        uint32_t count = converter->toUnicode.standardEightBit(flags, bytes[processedByteLen], buffer);
        if (count == 0) {
            break;
        }
        OCFIndex previous = *usedCharLen;
        usedLength = previous + (OCFIndex)count;
        if (maxCharLen != 0) {
            if (usedLength > maxCharLen) {
                break;
            }
            memcpy(characters + previous, buffer, (size_t)count << 1);
        }
        *usedCharLen = usedLength;
        if (++processedByteLen == numBytes) {
            processedByteLen = numBytes;
            break;
        }
    }
    return processedByteLen;
}

// Native __CFToCanonicalUnicodeStandardEightBitWrapper, iOS Simulator 24A434, 0x1c022b9f0,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1807699d8, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the
// decomposition buffer is indexed instead of walked by pointer. iOS Device divergence:
// __CFToCanonicalUnicodeStandardEightBitWrapper at 0x1807699d8 calls the record's callback with BLRAAZ
// (arm64e pointer authentication, zero discriminator); the iOS Simulator build uses BLR. Both images write
// the callback's source unit, not the decomposed values: a decomposable unit is stored once per decomposed
// value (as 0xd800 | unit >> 10, 0xdc00 | unit & 0x3ff for a value at or above 0x10000). Units that are not
// decomposed are stored without a capacity check, so a callback returning several units can write past
// maxCharLen. Both are kept as native behavior; whether a native converter reaches the overrun is outside
// this group's evidence. Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFToCanonicalUnicodeStandardEightBitWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
) {
    OCFIndex usedLength = 0;
    OCFIndex processedByteLen = 0;
    if (numBytes >= 1) {
        uint16_t buffer[20];
        uint32_t decomposed[10];
        for (;;) {
            if (maxCharLen != 0 && usedLength >= maxCharLen) {
                break;
            }
            uint32_t count = converter->toUnicode.standardEightBit(flags, bytes[processedByteLen], buffer);
            if (count == 0) {
                break;
            }
            for (size_t index = 0; index != count; ++index) {
                if (buffer[index] >= 0x80 && OCFUniCharIsMemberOf(buffer[index], 0x65) != 0) {
                    OCFIndex decomposedCount = OCFUniCharDecomposeCharacter(buffer[index], decomposed, 10);
                    *usedCharLen = usedLength;
                    if (decomposedCount >= 1) {
                        const uint32_t *cursor = decomposed;
                        do {
                            if (*cursor++ >= 0x10000) {
                                usedLength += 2;
                                if (usedLength > maxCharLen) {
                                    return processedByteLen;
                                }
                                if (maxCharLen != 0) {
                                    characters[0] = (uint16_t)(0xd800 | (buffer[index] >> 10));
                                    characters[1] = (uint16_t)(0xdc00 | (buffer[index] & 0x3ff));
                                    characters += 2;
                                }
                            } else {
                                if (usedLength >= maxCharLen) {
                                    return processedByteLen;
                                }
                                ++usedLength;
                                *characters++ = buffer[index];
                            }
                        } while (--decomposedCount != 0);
                    }
                } else {
                    if (maxCharLen != 0) {
                        *characters++ = buffer[index];
                    }
                    ++usedLength;
                }
            }
            if (++processedByteLen == numBytes) {
                processedByteLen = numBytes;
                break;
            }
        }
    }
    *usedCharLen = usedLength;
    return processedByteLen;
}

// Native __CFToBytesCheapMultiByteWrapper, iOS Simulator 24A434, 0x1c022bba4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x180769b98, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the copy
// loop indexes the scratch buffer instead of walking it. iOS Device divergence:
// __CFToBytesCheapMultiByteWrapper at 0x180769b98 calls the record's callback with BLRAAZ (arm64e pointer
// authentication, zero discriminator); the iOS Simulator build uses BLR. The callback writes into a 20-byte
// uninitialized stack buffer (frame offsets 0x4-0x18). The copy reloads *usedByteLen for every byte, as in
// the native code. Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFToBytesCheapMultiByteWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    *usedByteLen = 0;
    if (numChars < 1) {
        return 0;
    }
    OCFIndex usedLength = 0;
    OCFIndex processedCharLen = 0;
    uint8_t buffer[20];
    for (;;) {
        if (maxByteLen != 0 && usedLength >= maxByteLen) {
            break;
        }
        uint32_t count = converter->toBytes.cheapMultiByte(flags, characters[processedCharLen], buffer);
        if (count == 0) {
            break;
        }
        if (maxByteLen != 0) {
            if (*usedByteLen + (OCFIndex)count > maxByteLen) {
                break;
            }
            const uint8_t *source = buffer;
            uint8_t *destination = bytes;
            for (size_t remaining = count; remaining != 0; --remaining) {
                destination[*usedByteLen] = *source++;
                ++destination;
            }
        }
        usedLength = *usedByteLen + (OCFIndex)count;
        *usedByteLen = usedLength;
        if (++processedCharLen == numChars) {
            processedCharLen = numChars;
            break;
        }
    }
    return processedCharLen;
}

// Native __CFToUnicodeCheapMultiByteWrapper, iOS Simulator 24A434, 0x1c022bcc0,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x180769cc4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: __CFToUnicodeCheapMultiByteWrapper at 0x180769cc4 calls the record's callback
// with BLRAAZ (arm64e pointer authentication, zero discriminator); the iOS Simulator build uses BLR.
// Only a zero numBytes returns early (CBZ); the loop advances by each callback's count until the
// remaining count reaches zero.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFToUnicodeCheapMultiByteWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
) {
    *usedCharLen = 0;
    if (numBytes == 0) {
        return 0;
    }
    OCFIndex processedByteLen = 0;
    uint16_t character = 0;
    do {
        uint32_t count;
        if (maxCharLen != 0) {
            if (*usedCharLen >= maxCharLen) {
                break;
            }
            count = converter->toUnicode.cheapMultiByte(flags, bytes, numBytes, &character);
            if (count == 0) {
                break;
            }
            *characters++ = character;
        } else {
            count = converter->toUnicode.cheapMultiByte(flags, bytes, numBytes, &character);
            if (count == 0) {
                break;
            }
        }
        *usedCharLen = *usedCharLen + 1;
        processedByteLen += count;
        bytes += count;
        numBytes -= count;
    } while (numBytes != 0);
    return processedByteLen;
}

// Native __CFToCanonicalUnicodeCheapMultiByteWrapper, iOS Simulator 24A434, 0x1c022bdd4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x180769de0, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// usedCharLen is spilled to the stack and the decomposition buffer is indexed. iOS Device divergence:
// __CFToCanonicalUnicodeCheapMultiByteWrapper at 0x180769de0 calls the record's callback with BLRAAZ (arm64e
// pointer authentication, zero discriminator); the iOS Simulator build uses BLR. Decomposition and early
// returns follow __CFToCanonicalUnicodeCheapEightBitWrapper. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFToCanonicalUnicodeCheapMultiByteWrapper(
    const StringEncodingConverterRecord *converter,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
) {
    OCFIndex usedLength = 0;
    OCFIndex processedByteLen = 0;
    if (numBytes != 0) {
        uint16_t character = 0;
        uint32_t decomposed[10];
        do {
            if (maxCharLen != 0 && usedLength >= maxCharLen) {
                break;
            }
            uint32_t count = converter->toUnicode.cheapMultiByte(flags, bytes, numBytes, &character);
            if (count == 0) {
                break;
            }
            if (character >= 0x80 && OCFUniCharIsMemberOf(character, 0x65) != 0) {
                OCFIndex decomposedCount = OCFUniCharDecomposeCharacter(character, decomposed, 10);
                *usedCharLen = usedLength;
                if (decomposedCount >= 1) {
                    uint32_t *cursor = decomposed;
                    do {
                        uint32_t value = *cursor;
                        if (value >= 0x10000) {
                            usedLength += 2;
                            if (usedLength > maxCharLen) {
                                return processedByteLen;
                            }
                            if (maxCharLen != 0) {
                                *cursor = value - 0x10000;
                                characters[0] = (uint16_t)(((value - 0x10000) >> 10) + 0xd800);
                                characters[1] = (uint16_t)(0xdc00 | (value & 0x3ff));
                                characters += 2;
                            }
                        } else {
                            if (usedLength >= maxCharLen) {
                                return processedByteLen;
                            }
                            ++usedLength;
                            *characters++ = (uint16_t)value;
                        }
                        ++cursor;
                    } while (--decomposedCount != 0);
                }
            } else {
                if (maxCharLen != 0) {
                    *characters++ = character;
                }
                ++usedLength;
            }
            processedByteLen += count;
            bytes += count;
            numBytes -= count;
        } while (numBytes != 0);
    }
    *usedCharLen = usedLength;
    return processedByteLen;
}

// Native _toLossyASCIITable, iOS Simulator 24A434, 0x1c03a2f54 (__TEXT,__const, 96 four-byte entries for
// U+00A0-U+00FF, NUL-padded), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external;
// referenced only by __CFDefaultToBytesFallbackProc.
// iOS Device 24A446: 0x18089edc4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the 384
// bytes agree. Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const char _toLossyASCIITable[96][4] = {
    " ",  "!",  "c", "L", "$", "Y", "|",  "",  "",  "(C)", "a", "<<", "",    "-",   "(R)", "",
    "",   "+-", "2", "3", "",  "",  "",   "",  "",  "1",   "o", ">>", "1/4", "1/2", "3/4", "?",
    "A",  "A",  "A", "A", "A", "A", "AE", "C", "E", "E",   "E", "E",  "I",   "I",   "I",   "I",
    "TH", "N",  "O", "O", "O", "O", "O",  "X", "O", "U",   "U", "U",  "U",   "Y",   "th",  "s",
    "a",  "a",  "a", "a", "a", "a", "ae", "c", "e", "e",   "e", "e",  "i",   "i",   "i",   "i",
    "TH", "n",  "o", "o", "o", "o", "o",  "/", "o", "u",   "u", "u",  "u",   "y",   "th",  "y",
};

// Native __CFDefaultToBytesFallbackProc, iOS Simulator 24A434, 0x1c022bf68,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external; its address is taken by
// __CFGetConverter (0x1c022a8ac, at 0x1c022aa20), and it calls itself once for a non-ASCII decomposition.
// iOS Device 24A446: 0x180769f88, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: __CFDefaultToBytesFallbackProc at 0x180769f88 signs its return address
// (PACIBSP/RETAB), forms the table address before the length limit, scales the unit with LSL instead of
// UBFIZ (equal for units up to 0xff) and orders the two surrogate CSELs differently; behavior is identical.
// Units below U+00A0 map to unit ^ 0x80, so ASCII input yields bytes 0x80-0xff. A unit of U+DC00 is treated
// as a leading surrogate. A negative maxByteLen stores one byte on the single-byte paths, as the native code
// tests only for zero.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFDefaultToBytesFallbackProc(
    const uint16_t *characters, OCFIndex numChars, uint8_t *bytes, OCFIndex maxByteLen, OCFIndex *usedByteLen
) {
    uint16_t character = characters[0];
    uint8_t byte;
    OCFIndex result;
    if (character <= 0x9f) {
        byte = (uint8_t)(character ^ 0x80);
        result = 1;
    } else if (character <= 0xff) {
        OCFIndex limit = maxByteLen < 4 ? maxByteLen : 4;
        if (maxByteLen == 0) {
            limit = 4;
        }
        OCFIndex length = 0;
        if (limit >= 1) {
            const char *entry = _toLossyASCIITable[character - 0xa0];
            for (; length != limit; ++length) {
                if (entry[length] == 0) {
                    break;
                }
                if (maxByteLen != 0) {
                    bytes[length] = (uint8_t)entry[length];
                }
            }
        }
        *usedByteLen = length;
        return 1;
    } else if ((character >> 11) == 0x1b) {
        byte = '?';
        result = 1;
        if (numChars >= 2 && character <= 0xdc00) {
            uint16_t next = characters[1];
            if ((next >> 10) >= 0x37) {
                result = next < 0xe000 ? 2 : 1;
            }
        }
    } else if ((OCFUniCharIsMemberOf(character, 2) & 1) != 0) {
        byte = ' ';
        result = 1;
    } else if ((OCFUniCharIsMemberOf(characters[0], 3) & 1) != 0) {
        byte = '\n';
        result = 1;
    } else if (characters[0] == 0x2026) {
        if (maxByteLen == 0) {
            *usedByteLen = 3;
            return 1;
        }
        if (maxByteLen >= 3) {
            bytes[2] = '.';
            bytes[0] = '.';
            bytes[1] = '.';
            *usedByteLen = 3;
            return 1;
        }
        byte = '?';
        result = 1;
    } else if (OCFUniCharIsMemberOf(characters[0], 9) == 0) {
        byte = '?';
        result = 1;
    } else {
        uint32_t decomposed[10];
        OCFUniCharDecomposeCharacter(characters[0], decomposed, 10);
        if (decomposed[0] > 0x7f) {
            uint16_t unit = (uint16_t)decomposed[0];
            return __CFDefaultToBytesFallbackProc(&unit, 1, bytes, maxByteLen, usedByteLen);
        }
        byte = (uint8_t)decomposed[0];
        result = 1;
    }
    if (maxByteLen != 0) {
        *bytes = byte;
    }
    *usedByteLen = 1;
    return result;
}

// Native __CFDefaultToUnicodeFallbackProc, iOS Simulator 24A434, 0x1c022c174,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external; used through a pointer only.
// iOS Device 24A446: 0x18076a198, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; identical
// instructions. The five parameters are read from the registers the body uses (X3, X2, X4); X0 and X1 are
// unused. Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFDefaultToUnicodeFallbackProc(
    const uint8_t *bytes, OCFIndex numBytes, uint16_t *characters, OCFIndex maxCharLen, OCFIndex *usedCharLen
) {
    (void)bytes;
    (void)numBytes;
    if (maxCharLen != 0) {
        *characters = '?';
    }
    *usedCharLen = 1;
    return 1;
}
