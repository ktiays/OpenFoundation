//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"
#include "CoreFoundation/StringEncoding/OCFICUConverters.h"
#include "CoreFoundation/StringEncoding/OCFPlatformConverter.h"
#include "CoreFoundation/StringEncoding/OCFSingleByte.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingDatabase.h"
#include "CoreFoundation/StringEncoding/OCFUTF8.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"

#include <string.h>

// Inferred organization: the converter registry (CFStringEncodingUnicodeToBytes 0x1c0229f60 through
// CFStringEncodingByteLengthForCharacters 0x1c022b0dc, with __CFGetConverter at 0x1c022a8ac), the nine
// wrappers (0x1c022b484-0x1c022bf68), __CFDefaultToBytesFallbackProc (0x1c022bf68) and
// __CFDefaultToUnicodeFallbackProc (0x1c022c174) are contiguous in the iOS Simulator __text section and are
// kept in one translation unit. The native source path is not in the evidence. __CFStrConvertBytesToUnicode
// (0x1c022c2a4), which follows them, is an open group: its character table is rewritten at run time by
// __CFSetCharToUniCharFunc (0x1c022c1a4, String/OCFStringByteStream.c), which calls
// CFStringEncodingGetConverter.
//
// The registry definitions below: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified; iOS Device 24A446: unread. Every caller of __CFGetConverter that later reads the
// fallback slots stores zeros into them before the call (STP XZR, XZR); the callee writes the whole 48-byte
// value on every path, so the stores have no observable effect and are not reproduced. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.

// Count-only conversions pass a NULL buffer with a zero capacity and still advance the buffer position;
// the native code forms these addresses with integer arithmetic, as here, never dereferencing them.
static inline uint8_t *bytesAtOffset(uint8_t *bytes, OCFIndex offset) {
    return (uint8_t *)((uintptr_t)bytes + (uintptr_t)offset);
}

static inline uint16_t *charactersAtOffset(uint16_t *characters, OCFIndex offset) {
    return (uint16_t *)((uintptr_t)characters + ((uintptr_t)offset << 1));
}

// Native CFStringEncodingUnicodeToBytes, 0x1c0229f60. External.
// UTF-8 (0x8000100) never reaches __CFGetConverter: flags & 0xc0 decomposes through CFUniCharDecompose
// (reorder, UTF-8 format 2, HFS from bit 7); otherwise the converter record's to-bytes callback, cached once
// in the function-local static (native _CFStringEncodingUnicodeToBytes.__CFToUTF8, __DATA,__bss), converts
// with flags 0. The result is then 0 when every unit was used, 1 for no capacity, more than nine bytes left,
// a lone low surrogate or a high surrogate without a following unit or followed by a non-low-surrogate, and 2
// otherwise. Other encodings: bit 3 drops precomposition (and, with bit 0, the combining test); otherwise bit
// 5 drops precomposition and its absence adds bit 0x10 to the flags passed on. Classes 4 and 5 forward the
// modified flags. A unit the converter stops at is handled in this order: an insufficient buffer (the
// capacity is exactly used, or the converter would accept the unit with no buffer) ends with 2; a combining
// unit after at least one converted unit is precomposed with the units before it when the record has a
// precompose callback; otherwise bit 0 selects the lossy byte (flags >> 24) or the record's fallback.
// Combining units without a precompose callback skip the bit 0 test. Results: 0, 1 (invalid input), 2
// (insufficient buffer), 3 (no converter).
uint32_t OCFStringEncodingUnicodeToBytes(
    uint32_t encoding,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    OCFIndex *usedCharLen,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    if (encoding == 0x8000100) {
        OCFIndex theUsedCharLen = 0;
        OCFIndex theUsedByteLen = 0;
        if ((flags & 0xc0) != 0) {
            OCFUniCharDecompose(
                characters,
                numChars,
                &theUsedCharLen,
                bytes,
                maxByteLen,
                &theUsedByteLen,
                true,
                2,
                ((flags >> 7) & 1) != 0
            );
        } else {
            static StringEncodingStandardToBytesProc toUTF8 = NULL;
            if (toUTF8 == NULL) {
                toUTF8 = __CFConverterUTF8.toBytes.standard;
            }
            theUsedCharLen = toUTF8(0, characters, numChars, bytes, maxByteLen, &theUsedByteLen);
        }
        if (usedCharLen != NULL) {
            *usedCharLen = theUsedCharLen;
        }
        if (usedByteLen != NULL) {
            *usedByteLen = theUsedByteLen;
        }
        OCFIndex remaining = numChars - theUsedCharLen;
        if (remaining == 0) {
            return 0;
        }
        if (maxByteLen < 1) {
            return 1;
        }
        if (maxByteLen - theUsedByteLen > 9) {
            return 1;
        }
        const uint16_t *character = characters + theUsedCharLen;
        uint32_t kind = character[0] & 0xfc00;
        if (kind == 0xdc00) {
            return 1;
        }
        if (kind != 0xd800) {
            return 2;
        }
        if (remaining == 1) {
            return 1;
        }
        if ((uint32_t)(character[1] - 0xe000) >= 0xfffffc00) {
            return 2;
        }
        return 1;
    }

    StringEncodingRuntimeConverter converter = __CFGetConverter(encoding);
    OCFIndex localUsed = 0;
    const StringEncodingConverterRecord *definition = converter.definition;
    if (definition == NULL) {
        return 3;
    }
    StringEncodingIsValidCombiningCharProc isValidCombiningChar;
    StringEncodingToBytesPrecomposeProc toBytesPrecompose;
    if ((flags & 8) != 0) {
        toBytesPrecompose = NULL;
        isValidCombiningChar = (flags & 1) != 0 ? NULL : definition->isValidCombiningChar;
    } else {
        isValidCombiningChar = definition->isValidCombiningChar;
        if ((flags & 0x20) != 0) {
            toBytesPrecompose = NULL;
        } else {
            toBytesPrecompose = definition->toBytesPrecompose;
            flags |= 0x10;
        }
    }
    if (definition->encodingClass == 4) {
        return __CFStringEncodingPlatformUnicodeToBytes(
            encoding, flags, characters, numChars, usedCharLen, bytes, maxByteLen, usedByteLen
        );
    }
    if (definition->encodingClass == 5) {
        return (uint32_t)__CFStringEncodingICUToBytes(
            converter.toBytes.icuName,
            flags,
            characters,
            numChars,
            usedCharLen,
            bytes,
            maxByteLen,
            usedByteLen
        );
    }

    OCFIndex theUsedByteLen = 0;
    OCFIndex theUsedCharLen = 0;
    uint32_t result = 0;
    if (numChars >= 1) {
        uint8_t lossyByte = (uint8_t)(flags >> 24);
        StringEncodingToBytesWrapperProc toBytes = converter.toBytes.wrapper;
        StringEncodingToBytesFallbackProc toBytesFallback = converter.toBytesFallback;
        for (;;) {
            if (maxByteLen != 0 && maxByteLen <= theUsedByteLen) {
                break;
            }
            OCFIndex capacity = maxByteLen != 0 ? maxByteLen - theUsedByteLen : 0;
            OCFIndex processed;
            if (toBytes != NULL) {
                processed = toBytes(
                    definition,
                    flags,
                    characters + theUsedCharLen,
                    numChars - theUsedCharLen,
                    bytesAtOffset(bytes, theUsedByteLen),
                    capacity,
                    &localUsed
                );
            } else {
                processed = definition->toBytes.standard(
                    flags,
                    characters + theUsedCharLen,
                    numChars - theUsedCharLen,
                    bytesAtOffset(bytes, theUsedByteLen),
                    capacity,
                    &localUsed
                );
            }
            OCFIndex index = processed + theUsedCharLen;
            OCFIndex remaining = numChars - index;
            if (remaining <= 0) {
                theUsedCharLen = index;
                goto next;
            }
            OCFIndex probeUsed = 0;
            const uint16_t *current = characters + index;
            if (isValidCombiningChar == NULL || index < 1 || !isValidCombiningChar(*current)) {
                if (maxByteLen != 0) {
                    if (maxByteLen == localUsed + theUsedByteLen) {
                        goto insufficient;
                    }
                    OCFIndex probe;
                    if (toBytes != NULL) {
                        probe = toBytes(definition, flags, current, remaining, NULL, 0, &probeUsed);
                    } else {
                        probe = definition->toBytes.standard(flags, current, remaining, NULL, 0, &probeUsed);
                    }
                    if (probe != 0) {
                        goto insufficient;
                    }
                }
                theUsedByteLen += localUsed;
                if ((flags & 1) == 0) {
                    result = 1;
                    theUsedCharLen = index;
                    goto check;
                }
                if (lossyByte != 0) {
                    goto lossy;
                }
                theUsedCharLen = toBytesFallback(
                                     current,
                                     remaining,
                                     bytesAtOffset(bytes, theUsedByteLen),
                                     maxByteLen != 0 ? maxByteLen - theUsedByteLen : 0,
                                     &localUsed
                                 ) +
                                 index;
                goto next;
            }
            if (toBytesPrecompose == NULL) {
                if (maxByteLen != 0) {
                    OCFIndex probe;
                    if (maxByteLen == localUsed + theUsedByteLen) {
                        probe = 1;
                    } else if (toBytes != NULL) {
                        probe = toBytes(definition, flags, current, remaining, NULL, 0, &probeUsed);
                    } else {
                        probe = definition->toBytes.standard(flags, current, remaining, NULL, 0, &probeUsed);
                    }
                    if (probe != 0) {
                        theUsedByteLen += localUsed;
                        result = 2;
                        theUsedCharLen = index;
                        goto check;
                    }
                }
                if ((flags & 0x20) != 0) {
                    theUsedCharLen = index + 1;
                    while (theUsedCharLen < numChars &&
                           (isValidCombiningChar(characters[theUsedCharLen]) & 1)) {
                        ++theUsedCharLen;
                    }
                    goto next;
                }
                theUsedByteLen += localUsed;
                if (lossyByte != 0) {
                    goto lossy;
                }
                theUsedCharLen = toBytesFallback(
                                     current,
                                     remaining,
                                     bytesAtOffset(bytes, theUsedByteLen),
                                     maxByteLen != 0 ? maxByteLen - theUsedByteLen : 0,
                                     &localUsed
                                 ) +
                                 index;
                goto next;
            }

            // Back up over the combining units before the stop, to the base unit (offset <= -1).
            OCFIndex offset = 0;
            OCFIndex before;
            bool valid;
            do {
                before = index + offset;
                valid = isValidCombiningChar(current[offset - 1]);
                --offset;
            } while ((uint64_t)before >= 2 && valid);
            const uint16_t *base = current + offset;
            theUsedByteLen += localUsed;
            if (definition->maxBytesPerChar < 2) {
                theUsedByteLen -= 1;
            } else {
                if (toBytes != NULL) {
                    toBytes(definition, flags, base, -offset, NULL, 0, &localUsed);
                } else {
                    definition->toBytes.standard(flags, base, -offset, NULL, 0, &localUsed);
                }
                theUsedByteLen -= localUsed;
            }
            OCFIndex rest = numChars - processed - theUsedCharLen;
            OCFIndex precomposeCapacity = maxByteLen != 0 ? maxByteLen - theUsedByteLen : 0;
            OCFIndex precomposed = toBytesPrecompose(
                flags,
                base,
                rest - offset,
                bytesAtOffset(bytes, theUsedByteLen),
                precomposeCapacity,
                &localUsed
            );
            if (precomposed >= 1) {
                OCFIndex after = precomposed + index + offset;
                if (after < numChars && isValidCombiningChar(characters[after])) {
                    theUsedByteLen += localUsed;
                    theUsedCharLen = processed + theUsedCharLen + precomposed + offset;
                    result = 1;
                    goto check;
                }
                theUsedCharLen = after;
                goto next;
            }
            if ((flags & 1) == 0) {
                theUsedCharLen = processed + theUsedCharLen + offset;
                result = 1;
                goto check;
            }
            if (lossyByte != 0) {
                theUsedCharLen = index + offset + 1;
                while (theUsedCharLen < numChars && (isValidCombiningChar(characters[theUsedCharLen]) & 1)) {
                    ++theUsedCharLen;
                }
                localUsed = 1;
                if (maxByteLen != 0) {
                    bytes[theUsedByteLen] = lossyByte;
                }
                goto next;
            }
            // The base unit is dropped: the fallback starts after it, and its bytes were subtracted above.
            theUsedCharLen = toBytesFallback(
                                 base + 1,
                                 rest + ~offset,
                                 bytesAtOffset(bytes, theUsedByteLen),
                                 precomposeCapacity,
                                 &localUsed
                             ) +
                             index + offset + 1;
            goto next;

        lossy:
            theUsedCharLen = index + 1;
            localUsed = 1;
            if (maxByteLen != 0) {
                bytes[theUsedByteLen] = lossyByte;
            }
            goto next;

        insufficient:
            theUsedByteLen += localUsed;
            if ((flags & 0xff000001) == 1) {
                localUsed = 0;
                do {
                    OCFIndex count =
                        toBytesFallback(characters + index, numChars - index, NULL, 0, &localUsed);
                    if (count == 0) {
                        break;
                    }
                    index += localUsed == 0 ? count : 0;
                } while (index < numChars && localUsed == 0);
            }
            result = index < numChars ? 2 : 0;
            theUsedCharLen = index;
            goto check;

        next:
            theUsedByteLen += localUsed;
            if (theUsedCharLen >= numChars) {
                break;
            }
        }
    }
    result = 0;

check:
    if (maxByteLen != 0 && theUsedCharLen < numChars && result == 0) {
        if ((flags & 0xff000001) == 1) {
            localUsed = 0;
            StringEncodingToBytesFallbackProc toBytesFallback = converter.toBytesFallback;
            do {
                OCFIndex count = toBytesFallback(
                    characters + theUsedCharLen, numChars - theUsedCharLen, NULL, 0, &localUsed
                );
                if (count == 0) {
                    break;
                }
                theUsedCharLen += localUsed == 0 ? count : 0;
            } while (theUsedCharLen < numChars && localUsed == 0);
        }
        result = theUsedCharLen < numChars ? 2 : 0;
    }
    if (usedByteLen != NULL) {
        *usedByteLen = theUsedByteLen;
    }
    if (usedCharLen != NULL) {
        *usedCharLen = theUsedCharLen;
    }
    return result;
}

// Native CFStringEncodingGetConverter, 0x1c022a7ec. Hidden (was private external).
// MacRoman, ISO Latin 1, Windows Latin 1, ASCII, NextStep Latin and UTF-8 are answered directly; an encoding
// with a nonzero upper halfword goes to __CFStringEncodingGetExternalConverter, and the rest try
// __CFStringEncodingConverterGetUncommonConverter first (tail call to the external lookup when it fails).
const StringEncodingConverterRecord *CFStringEncodingGetConverter(uint32_t encoding) {
    switch (encoding) {
        case 0:
            return &__CFConverterMacRoman;
        case 0x201:
            return &__CFConverterISOLatin1;
        case 0x500:
            return &__CFConverterWinLatin1;
        case 0x600:
            return &__CFConverterASCII;
        case 0xb01:
            return &__CFConverterNextStepLatin;
        case 0x8000100:
            return &__CFConverterUTF8;
        default:
            break;
    }
    if ((encoding >> 16) == 0) {
        const StringEncodingConverterRecord *converter =
            __CFStringEncodingConverterGetUncommonConverter(encoding);
        if (converter != NULL) {
            return converter;
        }
    }
    return __CFStringEncodingGetExternalConverter(encoding);
}

// Native __CFGetConverter, 0x1c022a8ac, with __CFGetConverter.cold.1 (0x1c030ca9c). Non-external; the
// 48-byte value is returned through X8. The record is chosen as CFStringEncodingGetConverter chooses it (the
// six common encodings, then the uncommon converter for encodings without an upper halfword, then the
// external lookup); no record yields 48 zero bytes. Otherwise the value starts as the record and zeros, and
// the record's class byte selects the wrappers: 1 cheap eight-bit, 2 standard eight-bit, 3 cheap multibyte
// (to bytes, to Unicode and to canonical Unicode at 8, 16 and 24), 5 the ICU name from
// __CFStringEncodingGetICUName at 8, 0 and 4 none; any other class crashes ("Cannot create encoding converter
// for unknown encoding class"). The fallbacks are the record's, or __CFDefaultToBytesFallbackProc and
// __CFDefaultToUnicodeFallbackProc when NULL.
StringEncodingRuntimeConverter __CFGetConverter(uint32_t encoding) {
    const StringEncodingConverterRecord *definition;
    switch (encoding) {
        case 0:
            definition = &__CFConverterMacRoman;
            break;
        case 0x201:
            definition = &__CFConverterISOLatin1;
            break;
        case 0x500:
            definition = &__CFConverterWinLatin1;
            break;
        case 0x600:
            definition = &__CFConverterASCII;
            break;
        case 0xb01:
            definition = &__CFConverterNextStepLatin;
            break;
        case 0x8000100:
            definition = &__CFConverterUTF8;
            break;
        default:
            definition = NULL;
            if ((encoding >> 16) == 0) {
                definition = __CFStringEncodingConverterGetUncommonConverter(encoding);
            }
            if (definition == NULL) {
                definition = __CFStringEncodingGetExternalConverter(encoding);
            }
            if (definition == NULL) {
                StringEncodingRuntimeConverter empty;
                memset(&empty, 0, sizeof(empty));
                return empty;
            }
            break;
    }
    StringEncodingRuntimeConverter converter;
    converter.definition = definition;
    converter.toBytes.wrapper = NULL;
    converter.toUnicode = NULL;
    converter.toCanonicalUnicode = NULL;
    switch (definition->encodingClass) {
        case 0:
        case 4:
            break;
        case 1:
            converter.toBytes.wrapper = __CFToBytesCheapEightBitWrapper;
            converter.toUnicode = __CFToUnicodeCheapEightBitWrapper;
            converter.toCanonicalUnicode = __CFToCanonicalUnicodeCheapEightBitWrapper;
            break;
        case 2:
            converter.toBytes.wrapper = __CFToBytesStandardEightBitWrapper;
            converter.toUnicode = __CFToUnicodeStandardEightBitWrapper;
            converter.toCanonicalUnicode = __CFToCanonicalUnicodeStandardEightBitWrapper;
            break;
        case 3:
            converter.toBytes.wrapper = __CFToBytesCheapMultiByteWrapper;
            converter.toUnicode = __CFToUnicodeCheapMultiByteWrapper;
            converter.toCanonicalUnicode = __CFToCanonicalUnicodeCheapMultiByteWrapper;
            break;
        case 5:
            converter.toBytes.icuName = __CFStringEncodingGetICUName(encoding);
            break;
        default:
            crashWithMessage("Cannot create encoding converter for unknown encoding class");
    }
    StringEncodingToBytesFallbackProc toBytesFallback = definition->toBytesFallback;
    StringEncodingToUnicodeFallbackProc toUnicodeFallback = definition->toUnicodeFallback;
    converter.toBytesFallback = toBytesFallback == NULL ? __CFDefaultToBytesFallbackProc : toBytesFallback;
    converter.toUnicodeFallback =
        toUnicodeFallback == NULL ? __CFDefaultToUnicodeFallbackProc : toUnicodeFallback;
    return converter;
}

// Native CFStringEncodingBytesToUnicode, 0x1c022aa6c. External.
// Classes 4 and 5 forward every argument. Each round converts with the class wrapper (the canonical one when
// flags & 0xc0) or, without a wrapper, the record's to-Unicode callback. When bytes remain: an exactly used
// capacity ends with 2; with a capacity, the canonical flags or exactly one unit of room left, the remaining
// bytes are probed with no buffer (accepted: 2; rejected without bit 0: 1); otherwise bit 0 clear ends with 1
// and bit 0 set converts one step through the fallback. A loop that stops on the capacity check, or a final
// result 0 with bytes left and a capacity, yields 2. Results: 0, 1, 2, 3 (no converter).
uint32_t OCFStringEncodingBytesToUnicode(
    uint32_t encoding,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    OCFIndex *usedByteLen,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
) {
    StringEncodingRuntimeConverter converter = __CFGetConverter(encoding);
    const StringEncodingConverterRecord *definition = converter.definition;
    if (definition == NULL) {
        return 3;
    }
    if (definition->encodingClass == 4) {
        return __CFStringEncodingPlatformBytesToUnicode(
            encoding, flags, bytes, numBytes, usedByteLen, characters, maxCharLen, usedCharLen
        );
    }
    if (definition->encodingClass == 5) {
        return (uint32_t)__CFStringEncodingICUToUnicode(
            converter.toBytes.icuName,
            flags,
            bytes,
            numBytes,
            usedByteLen,
            characters,
            maxCharLen,
            usedCharLen
        );
    }
    OCFIndex theUsedByteLen = 0;
    OCFIndex theUsedCharLen = 0;
    uint32_t result = 0;
    if (numBytes >= 1) {
        uint32_t isHFS = flags & 0xc0;
        OCFIndex localUsed = 0;
        StringEncodingToUnicodeWrapperProc toUnicode = converter.toUnicode;
        StringEncodingToUnicodeWrapperProc toCanonicalUnicode = converter.toCanonicalUnicode;
        StringEncodingToUnicodeFallbackProc toUnicodeFallback = converter.toUnicodeFallback;
        do {
            if (maxCharLen != 0 && maxCharLen <= theUsedCharLen) {
                break;
            }
            uint16_t *destination = charactersAtOffset(characters, theUsedCharLen);
            OCFIndex capacity = maxCharLen != 0 ? maxCharLen - theUsedCharLen : 0;
            OCFIndex processed;
            if (toUnicode == NULL) {
                processed = definition->toUnicode.standard(
                    flags,
                    bytes + theUsedByteLen,
                    numBytes - theUsedByteLen,
                    destination,
                    capacity,
                    &localUsed
                );
            } else if (isHFS == 0) {
                processed = toUnicode(
                    definition,
                    flags,
                    bytes + theUsedByteLen,
                    numBytes - theUsedByteLen,
                    destination,
                    capacity,
                    &localUsed
                );
            } else {
                processed = toCanonicalUnicode(
                    definition,
                    flags,
                    bytes + theUsedByteLen,
                    numBytes - theUsedByteLen,
                    destination,
                    capacity,
                    &localUsed
                );
            }
            theUsedByteLen += processed;
            OCFIndex remaining = numBytes - theUsedByteLen;
            if (remaining > 0) {
                OCFIndex probeUsed = 0;
                bool probe = false;
                if (maxCharLen != 0) {
                    OCFIndex used = localUsed + theUsedCharLen;
                    if (maxCharLen == used) {
                        result = 2;
                        theUsedCharLen += localUsed;
                        goto finish;
                    }
                    probe = isHFS != 0 || maxCharLen == used + 1;
                }
                if (probe) {
                    OCFIndex accepted;
                    if (toUnicode == NULL) {
                        accepted = definition->toUnicode.standard(
                            flags, bytes + theUsedByteLen, remaining, NULL, 0, &probeUsed
                        );
                    } else if (isHFS == 0) {
                        accepted = toUnicode(
                            definition, flags, bytes + theUsedByteLen, remaining, NULL, 0, &probeUsed
                        );
                    } else {
                        accepted = toCanonicalUnicode(
                            definition, flags, bytes + theUsedByteLen, remaining, NULL, 0, &probeUsed
                        );
                    }
                    if (accepted != 0 || (flags & 1) == 0) {
                        result = accepted != 0 ? 2 : 1;
                        theUsedCharLen += localUsed;
                        goto finish;
                    }
                } else if ((flags & 1) == 0) {
                    result = 1;
                    theUsedCharLen += localUsed;
                    goto finish;
                }
                theUsedCharLen += localUsed;
                theUsedByteLen += toUnicodeFallback(
                    bytes + theUsedByteLen,
                    remaining,
                    charactersAtOffset(characters, theUsedCharLen),
                    maxCharLen != 0 ? maxCharLen - theUsedCharLen : 0,
                    &localUsed
                );
            }
            theUsedCharLen += localUsed;
        } while (theUsedByteLen < numBytes);
    }
    result = 0;

finish:
    if (result == 0 && theUsedByteLen < numBytes && maxCharLen != 0) {
        result = 2;
    }
    if (usedCharLen != NULL) {
        *usedCharLen = theUsedCharLen;
    }
    if (usedByteLen != NULL) {
        *usedByteLen = theUsedByteLen;
    }
    return result;
}

// Native CFStringEncodingIsValidEncoding, 0x1c022ada4. Hidden (was private external).
// The built-in Unicode encodings (0x100, 0x8000100, 0xc000100, 0x10000100, 0x14000100, 0x18000100,
// 0x1c000100), MacRoman, Windows Latin 1, ASCII, NextStep Latin and 0xbff are valid without a converter
// lookup.
bool CFStringEncodingIsValidEncoding(uint32_t encoding) {
    switch (encoding) {
        case 0:
        case 0x100:
        case 0x500:
        case 0x600:
        case 0xb01:
        case 0xbff:
        case 0x8000100:
        case 0xc000100:
        case 0x10000100:
        case 0x14000100:
        case 0x18000100:
        case 0x1c000100:
            return true;
        default:
            return CFStringEncodingGetConverter(encoding) != NULL;
    }
}

// Native CFStringEncodingCharLengthForBytes, 0x1c022ae84. Hidden (was private external).
// No converter: 0. Classes 4 and 5 tail-call their length functions; one byte per character returns numBytes;
// a to-Unicode length callback is tail-called. Otherwise each round converts with no buffer through the class
// wrapper (canonical for flags & 0xc0) or the record's callback, and bit 0 lets the fallback consume what the
// converter rejects; a rejection without bit 0 returns 0 unless no bytes remain.
OCFIndex CFStringEncodingCharLengthForBytes(
    uint32_t encoding, uint32_t flags, const uint8_t *bytes, OCFIndex numBytes
) {
    StringEncodingRuntimeConverter converter = __CFGetConverter(encoding);
    const StringEncodingConverterRecord *definition = converter.definition;
    if (definition == NULL) {
        return 0;
    }
    if (definition->encodingClass == 4) {
        return __CFStringEncodingPlatformCharLengthForBytes(encoding, flags, bytes, numBytes);
    }
    if (definition->encodingClass == 5) {
        return __CFStringEncodingICUCharLength(converter.toBytes.icuName, flags, bytes, numBytes);
    }
    if (definition->maxBytesPerChar == 1) {
        return numBytes;
    }
    if (definition->toUnicodeLen != NULL) {
        return definition->toUnicodeLen(flags, bytes, numBytes);
    }
    OCFIndex usedCharLen = 0;
    if (numBytes < 1) {
        return 0;
    }
    OCFIndex totalLength = 0;
    uint32_t isHFS = flags & 0xc0;
    StringEncodingToUnicodeWrapperProc toUnicode = converter.toUnicode;
    StringEncodingToUnicodeWrapperProc toCanonicalUnicode = converter.toCanonicalUnicode;
    StringEncodingToUnicodeFallbackProc toUnicodeFallback = converter.toUnicodeFallback;
    for (;;) {
        OCFIndex processed;
        if (toUnicode == NULL) {
            processed =
                converter.definition->toUnicode.standard(flags, bytes, numBytes, NULL, 0, &usedCharLen);
        } else if (isHFS == 0) {
            processed = toUnicode(converter.definition, flags, bytes, numBytes, NULL, 0, &usedCharLen);
        } else {
            processed =
                toCanonicalUnicode(converter.definition, flags, bytes, numBytes, NULL, 0, &usedCharLen);
        }
        numBytes -= processed;
        totalLength = usedCharLen + totalLength;
        if (numBytes < 1 || (flags & 1) == 0) {
            return numBytes < 1 ? totalLength : 0;
        }
        bytes += processed;
        OCFIndex consumed = toUnicodeFallback(bytes, numBytes, NULL, 0, &usedCharLen);
        bytes += consumed;
        totalLength = usedCharLen + totalLength;
        numBytes -= consumed;
        if (numBytes <= 0) {
            return totalLength;
        }
    }
}

// Native CFStringEncodingByteLengthForCharacters, 0x1c022b0dc. Hidden (was private external).
// No converter: 0. Classes 4 and 5 tail-call their length functions; one byte per character returns numChars;
// a to-bytes length callback is tail-called; otherwise the byte count of a count-only
// CFStringEncodingUnicodeToBytes, or 0 when it does not succeed.
OCFIndex CFStringEncodingByteLengthForCharacters(
    uint32_t encoding, uint32_t flags, const uint16_t *characters, OCFIndex numChars
) {
    StringEncodingRuntimeConverter converter = __CFGetConverter(encoding);
    const StringEncodingConverterRecord *definition = converter.definition;
    if (definition == NULL) {
        return 0;
    }
    if (definition->encodingClass == 4) {
        return __CFStringEncodingPlatformByteLengthForCharacters(encoding, flags, characters, numChars);
    }
    if (definition->encodingClass == 5) {
        return __CFStringEncodingICUByteLength(converter.toBytes.icuName, flags, characters, numChars);
    }
    if (definition->maxBytesPerChar == 1) {
        return numChars;
    }
    if (definition->toBytesLen != NULL) {
        return definition->toBytesLen(flags, characters, numChars);
    }
    OCFIndex usedByteLen = 0;
    uint32_t result =
        OCFStringEncodingUnicodeToBytes(encoding, flags, characters, numChars, NULL, NULL, 0, &usedByteLen);
    return result == 0 ? usedByteLen : 0;
}

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
