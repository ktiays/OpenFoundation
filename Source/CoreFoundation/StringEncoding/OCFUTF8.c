//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFUTF8.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"

#include <stddef.h>

// Inferred organization: __CFToUTF8 (0x1c0168fe4), __CFFromUTF8 (0x1c01691a0), __CFToUTF8Len (0x1c0169564)
// and
// __CFFromUTF8Len (0x1c0169604) are contiguous in the iOS Simulator __text section, with their tables in
// __TEXT,__const (0x1c0373034-0x1c0373154) and the converter record in __DATA_CONST,__const. The native
// source path is not in the evidence.

// Native firstByteMark, iOS Simulator 24A434, 0x1c0373034,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18082f290, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; all seven bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint8_t firstByteMark[7] = {0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc};

// Native trailingBytesForUTF8, 0x1c037303b (__TEXT,__const, 256 bytes). Non-external; read with LDRSB.
static const int8_t trailingBytesForUTF8[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5,
};

// Native offsetsFromUTF8, 0x1c037313c (__TEXT,__const, six words). Non-external.
static const uint32_t offsetsFromUTF8[6] = {
    0x00000000, 0x00003080, 0x000e2080, 0x03c82080, 0xfa082080, 0x82082080
};

// Native __CFToUTF8, iOS Simulator 24A434, 0x1c0168fe4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x180650f64, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; the reachable scalar and output-write behavior agrees.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFToUTF8(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex count,
    uint8_t *bytes,
    OCFIndex capacity,
    OCFIndex *usedBytes
) {
    uintptr_t inputStart = (uintptr_t)characters;
    uintptr_t current = inputStart;
    uintptr_t inputEnd = inputStart + ((uintptr_t)count << 1);
    uintptr_t outputStart = (uintptr_t)bytes;
    uintptr_t output = outputStart;
    uintptr_t outputEnd = outputStart + (uintptr_t)capacity;
    if (count >= 1) {
        for (;;) {
            if (capacity != 0 && output >= outputEnd) {
                break;
            }
            uint16_t unit;
            __builtin_memcpy(&unit, (const void *)current, sizeof(unit));
            uint32_t character = unit;
            uintptr_t next = current + 2;
            if (character <= 0x7f) {
                if (capacity != 0) {
                    *(uint8_t *)output = (uint8_t)character;
                }
                ++output;
            } else {
                if ((character >> 11) >= 0x1b) {
                    if ((character >> 10) <= 0x36) {
                        uint16_t low = 0;
                        if (next < inputEnd) {
                            __builtin_memcpy(&low, (const void *)next, sizeof(low));
                        }
                        if (next < inputEnd && (low & 0xfc00) == 0xdc00) {
                            next = current + 4;
                            character = low + (character << 10) + UINT32_C(0xfca02400);
                        } else if ((flags & 0x80) == 0) {
                            break;
                        }
                    } else if ((flags & 0x80) == 0 && character < 0xe000) {
                        break;
                    }
                }
                uint16_t length = character < 0x10000 ? 3 : 4;
                if (character < 0x800) {
                    length = 2;
                }
                if (character < 0x80) {
                    length = 1;
                }
                if (capacity != 0) {
                    uint32_t remaining = (uint32_t)outputEnd - (uint32_t)output;
                    if (remaining < length) {
                        break;
                    }
                    uint8_t *destination = (uint8_t *)output;
                    if (length > 2) {
                        if (length != 3) {
                            destination[3] = (uint8_t)((character & 0x3f) | 0x80);
                            character >>= 6;
                        }
                        destination[2] = (uint8_t)((character & 0x3f) | 0x80);
                        character >>= 6;
                    }
                    if (length != 1) {
                        destination[1] = (uint8_t)((character & 0x3f) | 0x80);
                        character >>= 6;
                    }
                    destination[0] = (uint8_t)(firstByteMark[length] | character);
                }
                output += length;
            }
            current = next;
            if (current >= inputEnd) {
                break;
            }
        }
    }
    if (usedBytes != 0) {
        *usedBytes = (OCFIndex)(output - outputStart);
    }
    return (OCFIndex)(current - inputStart) >> 1;
}

// Native __CFFromUTF8, 0x1c01691a0. Non-external. iOS Simulator 24A434; confidence: verified; iOS Device
// 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md. Returns the
// bytes consumed. A sequence needing more bytes than remain ends the conversion. Lead bytes 0xf8-0xff and,
// unless flags bit 11 skips validation, malformed sequences are replaced by U+FFFD only with bit 0 or when
// the lead byte is 0xa9; otherwise conversion stops before the byte. Surrogate code points (and values above
// U+10FFFF) end the conversion before the sequence unless bit 7 is set; above U+10FFFF they then become
// U+FFFD. With flags & 0xc0, members of character set 0x65 are decomposed (count-only: a BMP decomposition
// counts its value count, a supplementary one counts 1 or 2 per value). A supplementary pair that does not
// fit ends the conversion after its bytes were consumed, without storing it or counting it, as in the native
// code.
OCFIndex __CFFromUTF8(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
) {
    OCFIndex theUsedCharLen = 0;
    uint16_t *destination = characters;
    const uint8_t *source = bytes;
    if (numBytes != 0) {
        uint32_t isHFS = flags & 0xc0;
        uint32_t decomposed[10];
        for (;;) {
            if (maxCharLen != 0 && theUsedCharLen >= maxCharLen) {
                break;
            }
            uint8_t byte = *source;
            int8_t extraBytes = trailingBytesForUTF8[byte];
            if (numBytes <= (OCFIndex)(uint16_t)extraBytes) {
                break;
            }
            --numBytes;
            OCFIndex extra = (uint16_t)extraBytes;
            OCFIndex count;
            if ((~byte & 0xf8) == 0) {
                goto invalid;
            }
            if ((flags & 0x800) == 0) {
                if (byte >= 0xc0) {
                    const uint8_t *trail = source + extra;
                    do {
                        if ((int8_t)*trail >= -0x40) {
                            goto invalid;
                        }
                        --trail;
                    } while (trail > source);
                }
                if ((int8_t)byte < -0x3e || byte > 0xf4) {
                    goto invalid;
                }
                if (byte == 0xe0 && source[1] < 0xa0) {
                    goto invalid;
                }
                if (byte == 0xed && source[1] > 0x9f) {
                    goto invalid;
                }
                if (byte == 0xf0 && source[1] < 0x90) {
                    goto invalid;
                }
                if (byte == 0xf4 && source[1] >= 0x90) {
                    goto invalid;
                }
            }
            uint32_t character = 0;
            switch ((uint8_t)extraBytes) {
                default:
                    character += *source++;
                    character <<= 6;
                    // fallthrough
                case 2:
                    character += *source++;
                    character <<= 6;
                    // fallthrough
                case 1:
                    character += *source++;
                    character <<= 6;
                    // fallthrough
                case 0:
                    character += *source++;
                    break;
            }
            numBytes -= extra;
            character -= offsetsFromUTF8[extra];
            if (character < 0x10000) {
                if ((flags & 0x80) == 0 && (character & 0xf800) == 0xd800) {
                    goto backUp;
                }
                if (isHFS != 0 && character >= 0x80 && OCFUniCharIsMemberOf(character, 0x65) != 0) {
                    count = OCFUniCharDecomposeCharacter(character, decomposed, 10);
                    if (maxCharLen != 0) {
                        goto fill;
                    }
                    theUsedCharLen += count;
                } else {
                    if (maxCharLen != 0) {
                        *destination++ = (uint16_t)character;
                    }
                    ++theUsedCharLen;
                }
            } else if (character < 0x110000) {
                if (isHFS != 0 && (OCFUniCharIsMemberOf(character, 0x65) & 1) != 0) {
                    count = OCFUniCharDecomposeCharacter(character, decomposed, 10);
                    if (maxCharLen != 0) {
                        goto fill;
                    }
                    for (OCFIndex index = count; index >= 1; --index) {
                        theUsedCharLen += decomposed[index - 1] >= 0x10000 ? 2 : 1;
                    }
                } else if (maxCharLen == 0) {
                    theUsedCharLen += 2;
                } else {
                    if (theUsedCharLen + 2 > maxCharLen) {
                        break;
                    }
                    destination[0] =
                        (uint16_t)(UINT32_C(0xffffd800) + ((character + UINT32_C(0x3ff0000)) >> 10));
                    destination[1] = (uint16_t)(0xdc00 | (character & 0x3ff));
                    destination += 2;
                    theUsedCharLen += 2;
                }
            } else {
                if ((flags & 0x80) == 0) {
                    goto backUp;
                }
                if (maxCharLen != 0) {
                    *destination++ = 0xfffd;
                }
                ++theUsedCharLen;
            }
            goto next;

        fill:
            if ((OCFUniCharFillDestinationBuffer(
                     decomposed, count, (void **)&destination, maxCharLen, &theUsedCharLen, 0
                 ) &
                 1) == 0) {
                break;
            }
            goto next;

        invalid:
            if ((flags & 1) == 0 && byte != 0xa9) {
                break;
            }
            if (maxCharLen != 0) {
                *destination++ = 0xfffd;
            }
            ++source;
            ++theUsedCharLen;

        next:
            if (numBytes == 0) {
                break;
            }
            continue;

        backUp:
            source -= extra + 1;
            break;
        }
    }
    if (usedCharLen != NULL) {
        *usedCharLen = theUsedCharLen;
    }
    return source - bytes;
}

// Native __CFToUTF8Len, iOS Simulator 24A434, 0x1c0169564,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18072755c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; both use a wrapping 32-bit accumulator and ignore flags.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFToUTF8Len(uint32_t flags, const uint16_t *characters, OCFIndex count) {
    (void)flags;
    uint32_t length = 0;
    const uint16_t *current = characters;
    while (count != 0) {
        uint16_t unit;
        __builtin_memcpy(&unit, current, sizeof(unit));
        ++current;
        uint32_t character = unit;
        OCFIndex remaining = count - 1;
        if ((character & 0xfc00) != 0xd800 || count == 1) {
            --count;
        } else {
            uint16_t low;
            __builtin_memcpy(&low, current, sizeof(low));
            uint32_t combined = low + (character << 10) + UINT32_C(0xfca02400);
            count -= 2;
            if ((low & 0xfc00) == 0xdc00) {
                ++current;
                character = combined;
            } else {
                count = remaining;
            }
        }
        uint32_t bytes = character < 0x10000 ? 3 : 4;
        if (character < 0x800) {
            bytes = 2;
        }
        if (character < 0x80) {
            bytes = 1;
        }
        length += bytes;
    }
    return length;
}

// Native __CFFromUTF8Len, 0x1c0169604. Non-external. iOS Simulator 24A434; confidence: verified; iOS Device
// 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md. The validation
// of __CFFromUTF8 with a unit count: values above U+10FFFF count 1 without testing bit 7, and a supplementary
// value outside the decomposition set counts 2.
OCFIndex __CFFromUTF8Len(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes) {
    OCFIndex length = 0;
    if (numBytes == 0) {
        return 0;
    }
    uint32_t isHFS = flags & 0xc0;
    uint32_t decomposed[10];
    for (;;) {
        uint8_t byte = *bytes;
        int8_t extraBytes = trailingBytesForUTF8[byte];
        if (numBytes <= (OCFIndex)(uint16_t)extraBytes) {
            break;
        }
        --numBytes;
        OCFIndex extra = (uint16_t)extraBytes;
        if ((~byte & 0xf8) == 0) {
            goto invalid;
        }
        if ((flags & 0x800) == 0) {
            if (byte >= 0xc0) {
                const uint8_t *trail = bytes + extra;
                do {
                    if ((int8_t)*trail >= -0x40) {
                        goto invalid;
                    }
                    --trail;
                } while (trail > bytes);
            }
            if ((int8_t)byte < -0x3e || byte > 0xf4) {
                goto invalid;
            }
            if (byte == 0xe0 && bytes[1] < 0xa0) {
                goto invalid;
            }
            if (byte == 0xed && bytes[1] > 0x9f) {
                goto invalid;
            }
            if (byte == 0xf0 && bytes[1] < 0x90) {
                goto invalid;
            }
            if (byte == 0xf4 && bytes[1] >= 0x90) {
                goto invalid;
            }
        }
        uint32_t character = 0;
        switch ((uint8_t)extraBytes) {
            default:
                character += *bytes++;
                character <<= 6;
                // fallthrough
            case 2:
                character += *bytes++;
                character <<= 6;
                // fallthrough
            case 1:
                character += *bytes++;
                character <<= 6;
                // fallthrough
            case 0:
                character += *bytes++;
                break;
        }
        numBytes -= extra;
        character -= offsetsFromUTF8[extra];
        if (character < 0x10000) {
            if ((flags & 0x80) == 0 && (character & 0xf800) == 0xd800) {
                break;
            }
            if (isHFS != 0 && character >= 0x80 && OCFUniCharIsMemberOf(character, 0x65) != 0) {
                length += OCFUniCharDecomposeCharacter(character, decomposed, 10);
            } else {
                ++length;
            }
        } else if (character >= 0x110000) {
            ++length;
        } else if (isHFS != 0 && OCFUniCharIsMemberOf(character, 0x65) != 0) {
            OCFIndex count = OCFUniCharDecomposeCharacter(character, decomposed, 10);
            for (OCFIndex index = count; index >= 1; --index) {
                length += decomposed[index - 1] >= 0x10000 ? 2 : 1;
            }
        } else {
            length += 2;
        }
        goto next;

    invalid:
        if ((flags & 1) == 0 && byte != 0xa9) {
            break;
        }
        ++bytes;
        ++length;

    next:
        if (numBytes == 0) {
            break;
        }
    }
    return length;
}

// Native __CFConverterUTF8, 0x22279cca0 (__DATA_CONST,__const, 72 bytes). Hidden (was private external).
// Standard class (0), three bytes per character, decomposition length 2, both length callbacks.
const StringEncodingConverterRecord __CFConverterUTF8 = {
    .toBytes.standard = __CFToUTF8,
    .toUnicode.standard = __CFFromUTF8,
    .maxBytesPerChar = 3,
    .maxDecomposedCharLen = 2,
    .encodingClass = 0,
    .toBytesLen = __CFToUTF8Len,
    .toUnicodeLen = __CFFromUTF8Len,
};
