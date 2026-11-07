//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFStringByteStream.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/StringEncoding/OCFSingleByte.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"

#include <dispatch/dispatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xlocale.h>

// Inferred organization: __CFSetCharToUniCharFunc (0x1c022c1a4) and __CFStrConvertBytesToUnicode
// (0x1c022c2a4) through _CFStringGetUserDefaultEncoding
// (0x1c022de0c) are contiguous in the iOS Simulator __text section (with __CFStringDecodeByteStream2,
// _CFStringGetInstallationEncodingAndRegion, _CFStringSaveUserDefaultEncoding after
// them, outside this sub-track, and this unit's copy of _objc_getTaggedPointerTag, 0x1c022e2ec, last),
// between the string-encoding converters and the string scanner, and form the native byte-stream unit
// (CFStringEncodings); the native source path is not in the evidence. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationString.md.

// The UTF-8 converter callbacks as the two byte-stream functions call them: to-unicode (flags, bytes, byte
// count, characters, capacity, used-character count; returns the bytes consumed) and to-bytes (__CFToUTF8).
typedef OCFIndex (*UTF8ToUnicodeProc)(
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    UniChar *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
);

typedef OCFIndex (*UTF8FromUnicodeProc)(
    uint32_t flags,
    const UniChar *characters,
    OCFIndex numChars,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
);

// Native _objc_getTaggedPointerTag, 0x1c022e2ec (listed as _objc_getTaggedPointerTag_0 in the evidence): this
// unit's static copy of libobjc's inline tag decoder, defined last in native order. The local spelling keeps
// it apart from the CFString unit's hidden copy declared in String/OCFString.h.
static uint32_t encodingsGetTaggedPointerTag(const void *ptr);

// Native __CFWantsToUseASCIICompatibleConversion, 0x22e6d3108 (__DATA,__bss, one byte). Non-external. Both
// byte-stream functions pass it as converter flags; no function of the closure writes it (a writer outside
// the declared scope is unread), so it keeps its zero initial value here.
static uint8_t __CFWantsToUseASCIICompatibleConversion;

// The converter record's encoding-class byte (offset 0x14 of the CFStringEncodingGetConverter result); 1 is
// the cheap eight-bit class. Read as a raw byte; the record type in StringEncoding/ describes only offsets 0
// and 8.
static uint8_t converterEncodingClass(const void *converter) {
    return *((const uint8_t *)converter + offsetof(StringEncodingConverterRecord, encodingClass));
}

// The ASCII-superset test both byte-stream functions inline: on the encoding's second byte (bits 8-15),
// 0: not for 1, 4, 5, 0x21, 0x22 (mask 0x600000032) or 0x98; 1: only UTF-8 (0x8000100); 2: all but 0x206;
// 6: only 0x600; 8 and 12: never; 10: not for 0xa01 + {0, 4, 15} (mask 0x8011 over 0xa01-0xa10); 11: not for
// 0xbf3 and 0xbf4; any other byte: when (encoding & 0xff00) < 0xc01. Comparisons are 32-bit as natively.
static bool encodingIsSupersetOfASCII(OCFStringEncoding encoding) {
    switch ((encoding >> 8) & 0xff) {
        case 0:
            if (encoding <= 0x22 && ((UINT64_C(1) << encoding) & UINT64_C(0x600000032)) != 0) {
                return false;
            }
            return encoding != 0x98;
        case 1:
            return encoding == 0x8000100;
        case 2:
            return encoding != 0x206;
        case 6:
            return encoding == 0x600;
        case 8:
            return false;
        case 10:
            if (encoding - 0xa01 > 0xf) {
                return true;
            }
            return ((UINT32_C(1) << (encoding - 0xa01)) & 0x8011) == 0;
        case 11:
            return encoding - 0xbf3 >= 2;
        case 12:
            return false;
        default:
            return (encoding & 0xff00) < 0xc01;
    }
}

// Native __CFCharToUniCharFunc, 0x22e6d4258 (__DATA,__common). External. The cheap eight-bit converter whose
// table __CFCharToUniCharTable holds; NULL until __CFSetCharToUniCharFunc installs one.
CharToUniCharFunction __OCFCharToUniCharFunc;

// Native __CFSetCharToUniCharFunc, 0x1c022c1a4. Hidden (was private external). Nothing happens when function
// is already installed. NULL selects __CFIdempotentCharToUniCharTable and leaves __CFCharToUniCharFunc as it
// is. Otherwise, when the MacRoman converter (CFStringEncodingGetConverter(0)) exists and its to-Unicode
// callback (offset 8) is function, __CFMacRomanCharToUnicharTable is selected; else
// __CFMutableCharToUniCharTable is selected first and then its entries 0x80-0xff are rewritten one by one
// with function(0, byte, &unit), each unit zeroed before the call and replaced by 0xfffd when the call
// answers 0 (the whole W0 is tested). The function is then recorded in __CFCharToUniCharFunc.
void __CFSetCharToUniCharFunc(CharToUniCharFunction function) {
    if (__OCFCharToUniCharFunc == function) {
        return;
    }
    if (function == NULL) {
        __CFCharToUniCharTable = __OCFIdempotentCharToUniCharTable;
        return;
    }
    const StringEncodingConverterRecord *converter = CFStringEncodingGetConverter(0);
    if (converter != NULL && (const void *)converter->toUnicode.cheapEightBit == (const void *)function) {
        __CFCharToUniCharTable = __OCFMacRomanCharToUnicharTable;
    } else {
        __CFCharToUniCharTable = __CFMutableCharToUniCharTable;
        for (uintptr_t index = 0x80; index != 0x100; ++index) {
            uint16_t unit = 0;
            uint32_t result = function(0, (uint8_t)index, &unit);
            __CFMutableCharToUniCharTable[index] = result != 0 ? unit : 0xfffd;
        }
    }
    __OCFCharToUniCharFunc = function;
}

// Native __CFStrConvertBytesToUnicode, 0x1c022c2a4. External. Maps each byte through the table
// __CFCharToUniCharTable points to (the pointer is loaded once).
void __OCFStrConvertBytesToUnicode(const UInt8 *bytes, UniChar *buffer, OCFIndex numChars) {
    if (numChars < 1) {
        return;
    }
    const UniChar *table = __CFCharToUniCharTable;
    do {
        *buffer++ = table[*bytes++];
        --numChars;
    } while (numChars != 0);
}

// Native __CFStringDecodeByteStream3, 0x1c022c2d8. External. Decodes bytes into buffer as eight-bit (isASCII)
// or UTF-16 characters. The local buffer holds 0x3f0 bytes or 0x1f8 characters; larger contents are allocated
// from the buffer's allocator (malloc type IDs 0x100004077774924 for bytes, 0x1000040BDFB0063 for
// characters), and shouldFreeChars is set as each path decides (some paths also set it when the caller
// supplied the characters). Failures after setup release allocated characters and reset the buffer; an
// unknown converter and an out-of-range UTF-32 value while counting return false without the reset. .cold.1
// (0x1c030cab0) is the dispatch_once slow path; its block (_____CFStringDecodeByteStream3_block_invoke,
// 0x1c022cf18) stores the UTF-8 converter's to-unicode callback (CFStringEncodingGetConverter(0x8000100) +
// 8). The function-scope statics are native __CFStringDecodeByteStream3.onceToken (0x22e6d3110),
// .__CFFromUTF8 (0x22e6d3118) and .lossyFlag (0x22e6d3120, set and never read).
Boolean __OCFStringDecodeByteStream3(
    const UInt8 *bytes,
    OCFIndex len,
    OCFStringEncoding encoding,
    Boolean alwaysUnicode,
    VarWidthCharBuffer *buffer,
    Boolean *useClientsMemoryPtr,
    UInt32 converterFlags
) {
    static dispatch_once_t onceToken;
    static UTF8ToUnicodeProc __CFFromUTF8;
    static uint8_t lossyFlag;

    if (useClientsMemoryPtr != NULL) {
        *useClientsMemoryPtr = false;
    }
    buffer->numChars = 0;
    buffer->isASCII = alwaysUnicode == 0;
    buffer->shouldFreeChars = false;
    if (len == 0) {
        return true;
    }
    OCFAllocatorRef allocator = buffer->allocator;
    if (allocator == NULL) {
        allocator = _OCFGetTSD(1);
        if (allocator == NULL) {
            allocator = kOCFAllocatorSystemDefault;
        }
    }
    buffer->allocator = allocator;
    const UInt8 *end = bytes + len;

    switch (encoding) {
        case 0x100:
        case 0x10000100:
        case 0x14000100: {
            OCFIndex byteLength = len & ~(OCFIndex)1;
            if (byteLength == 0) {
                goto fail;
            }
            const UInt8 *limit = bytes + byteLength;
            bool swap;
            OCFIndex numChars;
            if (encoding == 0x10000100) {
                swap = true;
            } else if (encoding == 0x100) {
                UniChar bom = *(const UniChar *)bytes;
                if (bom == 0xfffe || bom == 0xfeff) {
                    bytes += 2;
                    swap = bom != 0xfeff;
                } else {
                    swap = true;
                }
            } else {
                // UTF-16LE: the byte-swapped flag stays clear; the caller's bytes are used when allowed.
                numChars = len >> 1;
                buffer->numChars = numChars;
                if (useClientsMemoryPtr != NULL) {
                    *useClientsMemoryPtr = true;
                    buffer->chars.ascii = (uint8_t *)bytes;
                    buffer->isASCII = false;
                    return true;
                }
                swap = false;
                goto classifyUTF16;
            }
            byteLength = limit - bytes;
            numChars = byteLength >> 1;
            buffer->numChars = numChars;
        classifyUTF16:
            if (buffer->isASCII) {
                // A character is ASCII when its high byte and bit 7 are clear (0x80ff on byte-swapped input).
                UniChar mask = swap ? 0x80ff : 0xff80;
                const UInt8 *cursor = bytes;
                for (;;) {
                    if (cursor >= limit) {
                        uint8_t *characters = buffer->chars.ascii;
                        if (characters == NULL) {
                            if ((uint64_t)numChars < 0x3f1) {
                                characters = buffer->localBuffer;
                                buffer->chars.ascii = characters;
                            } else {
                                characters = OCFAllocatorAllocateTyped(
                                    buffer->allocator, numChars, 0x100004077774924, 0
                                );
                                buffer->chars.ascii = characters;
                                if (characters == NULL) {
                                    goto fail;
                                }
                                buffer->shouldFreeChars = true;
                            }
                        }
                        if (swap) {
                            while (bytes < limit) {
                                *characters++ = bytes[1];
                                bytes += 2;
                            }
                        } else {
                            while (bytes < limit) {
                                *characters++ = bytes[0];
                                bytes += 2;
                            }
                        }
                        return true;
                    }
                    UniChar character = *(const UniChar *)cursor;
                    cursor += 2;
                    if ((mask & character) != 0) {
                        break;
                    }
                }
                buffer->isASCII = false;
            }
            UniChar *characters = buffer->chars.unicode;
            if (characters == NULL) {
                if ((uint64_t)numChars < 0x1f9) {
                    characters = (UniChar *)buffer->localBuffer;
                    buffer->chars.unicode = characters;
                } else {
                    characters =
                        OCFAllocatorAllocateTyped(buffer->allocator, byteLength, 0x1000040BDFB0063, 0);
                    buffer->chars.unicode = characters;
                    if (characters == NULL) {
                        goto fail;
                    }
                    buffer->shouldFreeChars = true;
                }
            }
            if (swap) {
                while (bytes < limit) {
                    UniChar character = *(const UniChar *)bytes;
                    bytes += 2;
                    *characters++ = __builtin_bswap16(character);
                }
                return true;
            }
            memmove(characters, bytes, buffer->numChars * 2);
            return true;
        }
        case 0xc000100:
        case 0x18000100:
        case 0x1c000100: {
            OCFIndex byteLength = len & ~(OCFIndex)3;
            if (byteLength == 0) {
                goto fail;
            }
            const UInt8 *limit = bytes + byteLength;
            bool swap;
            if (encoding == 0x18000100) {
                swap = true;
            } else if (encoding == 0xc000100) {
                uint32_t bom = *(const uint32_t *)bytes;
                if (bom == 0xfeff || bom == 0xfffe0000) {
                    bytes += 4;
                    swap = bom != 0xfeff;
                } else {
                    swap = true;
                }
            } else {
                swap = false;
            }
            // Non-ASCII words (asciiMask) clear isASCII; words outside the BMP (highMask) count a second
            // character and must not exceed plane 16 after the optional swap.
            uint32_t highMask = swap ? 0xffff : 0xffff0000;
            uint32_t asciiMask = swap ? 0x80ffffff : 0xffffff80;
            OCFIndex byteSpan = limit - bytes;
            OCFIndex numChars = byteSpan >> 2;
            buffer->numChars = numChars;
            if (limit > bytes) {
                const UInt8 *cursor = bytes;
                do {
                    uint32_t word = *(const uint32_t *)cursor;
                    if ((word & asciiMask) != 0) {
                        buffer->isASCII = false;
                        if ((word & highMask) != 0) {
                            uint32_t value = swap ? __builtin_bswap32(word) : word;
                            if ((value >> 16) > 0x10) {
                                return false;
                            }
                            ++numChars;
                            buffer->numChars = numChars;
                        }
                    }
                    cursor += 4;
                } while (cursor < limit);
            }
            if (buffer->isASCII) {
                uint8_t *characters = buffer->chars.ascii;
                if (characters == NULL) {
                    if ((uint64_t)numChars < 0x3f1) {
                        characters = buffer->localBuffer;
                        buffer->chars.ascii = characters;
                    } else {
                        characters =
                            OCFAllocatorAllocateTyped(buffer->allocator, numChars, 0x100004077774924, 0);
                        buffer->chars.ascii = characters;
                        if (characters == NULL) {
                            goto fail;
                        }
                        buffer->shouldFreeChars = true;
                    }
                }
                if (swap) {
                    while (bytes < limit) {
                        *characters++ = bytes[3];
                        bytes += 4;
                    }
                } else {
                    while (bytes < limit) {
                        *characters++ = (uint8_t)*(const uint32_t *)bytes;
                        bytes += 4;
                    }
                }
                return true;
            }
            UniChar *characters = buffer->chars.unicode;
            if (characters == NULL) {
                if ((uint64_t)numChars < 0x1f9) {
                    characters = (UniChar *)buffer->localBuffer;
                    buffer->chars.unicode = characters;
                } else {
                    characters =
                        OCFAllocatorAllocateTyped(buffer->allocator, numChars * 2, 0x1000040BDFB0063, 0);
                    buffer->chars.unicode = characters;
                    if (characters == NULL) {
                        goto fail;
                    }
                    buffer->shouldFreeChars = true;
                }
            }
            if (byteSpan >= 1) {
                do {
                    uint32_t word = *(const uint32_t *)bytes;
                    bytes += 4;
                    uint32_t value = swap ? __builtin_bswap32(word) : word;
                    if ((value >> 16) == 0) {
                        // A lone surrogate code point (0xd800-0xdfff) fails.
                        if (((value & 0xf800) | 0x400) == 0xdc00) {
                            goto fail;
                        }
                    } else {
                        if ((value >> 16) > 0x10) {
                            goto fail;
                        }
                        *characters++ = (UniChar)(((value + 0x3ff0000) >> 10) + 0xffffd800);
                        value = 0xdc00 | (value & 0x3ff);
                    }
                    *characters++ = (UniChar)value;
                } while (bytes < limit);
            }
            return true;
        }
        case 0x8000100: {
            if (len >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb && bytes[2] == 0xbf) {
                len -= 3;
                if (len == 0) {
                    return true;
                }
                bytes += 3;
            }
            if (buffer->isASCII) {
                bool allASCII = true;
                for (OCFIndex index = 0; index < len; ++index) {
                    if ((int8_t)bytes[index] < 0) {
                        allASCII = false;
                        break;
                    }
                }
                if (allASCII) {
                    buffer->numChars = len;
                    uint8_t *characters = buffer->chars.ascii;
                    buffer->shouldFreeChars = (uint64_t)len > 0x3f0 || characters != NULL;
                    if (characters == NULL) {
                        if ((uint64_t)len <= 0x3f0) {
                            characters = buffer->localBuffer;
                            buffer->chars.ascii = characters;
                        } else {
                            characters =
                                OCFAllocatorAllocateTyped(buffer->allocator, len, 0x100004077774924, 0);
                            buffer->chars.ascii = characters;
                            if (characters == NULL) {
                                goto fail;
                            }
                        }
                    }
                    memmove(characters, bytes, len);
                    return true;
                }
                buffer->isASCII = false;
            }
            dispatch_once(&onceToken, ^{
              const StringEncodingConverterRecord *converter = CFStringEncodingGetConverter(0x8000100);
              __CFFromUTF8 = (UTF8ToUnicodeProc)(void *)converter->toUnicode.cheapMultiByte;
            });
            UniChar *characters = buffer->chars.unicode;
            buffer->shouldFreeChars = (uint64_t)len > 0x1f8 || characters != NULL;
            if (characters == NULL) {
                if ((uint64_t)len <= 0x1f8) {
                    characters = (UniChar *)buffer->localBuffer;
                    buffer->chars.unicode = characters;
                } else {
                    characters = OCFAllocatorAllocateTyped(buffer->allocator, len * 2, 0x1000040BDFB0063, 0);
                    buffer->chars.unicode = characters;
                    if (characters == NULL) {
                        goto fail;
                    }
                }
            }
            buffer->numChars = 0;
            if (bytes < end) {
                OCFIndex numChars = 0;
                do {
                    OCFIndex usedChars = 0;
                    OCFIndex consumed = __CFFromUTF8(
                        converterFlags,
                        bytes,
                        end - bytes,
                        buffer->chars.unicode + numChars,
                        len - numChars,
                        &usedChars
                    );
                    if (usedChars == 0) {
                        goto fail;
                    }
                    numChars = buffer->numChars + usedChars;
                    buffer->numChars = numChars;
                    bytes += consumed;
                } while (bytes < end);
            }
            return true;
        }
        case 0xbff: {
            // Non-lossy ASCII: '\' followed by three octal digits, "u" or "U" and four hex digits, or '\'.
            buffer->isASCII = false;
            UniChar *characters = buffer->chars.unicode;
            buffer->shouldFreeChars = (uint64_t)len > 0x1f8 || characters != NULL;
            if (characters == NULL) {
                if ((uint64_t)len <= 0x1f8) {
                    characters = (UniChar *)buffer->localBuffer;
                    buffer->chars.unicode = characters;
                } else {
                    characters = OCFAllocatorAllocateTyped(buffer->allocator, len * 2, 0x1000040BDFB0063, 0);
                    buffer->chars.unicode = characters;
                    if (characters == NULL) {
                        goto fail;
                    }
                }
            }
            buffer->numChars = 0;
            if (len < 1) {
                return true;
            }
            OCFIndex numChars = 0;
            uint8_t state = 0;
            uint32_t value = 0;
            do {
                uint8_t byte = *bytes++;
                bool emit = false;
                if (state == 1) {
                    if ((byte & 0xffffffdf) == 'U') {
                        value = 0;
                        state = 2;
                    } else if ((uint32_t)(byte - '0') <= 9) {
                        value = (uint32_t)(byte - '0');
                        state = 7;
                    } else if (byte == '\\') {
                        value = '\\';
                        emit = true;
                    } else {
                        goto fail;
                    }
                } else if (state == 0) {
                    if (byte == '\\') {
                        state = 1;
                    } else {
                        value = byte;
                        if ((byte & 0x80) != 0) {
                            goto fail;
                        }
                        emit = true;
                    }
                } else {
                    uint32_t digit = (uint32_t)byte - '0';
                    if ((int8_t)state > 5) {
                        if ((uint8_t)digit > 9) {
                            goto fail;
                        }
                        value = digit | (value << 3);
                        ++state;
                        if (state == 9) {
                            emit = true;
                        }
                    } else {
                        if ((uint8_t)digit <= 9) {
                            value = byte + (value << 4) - '0';
                        } else {
                            uint32_t letter = byte > 0x60 ? (uint32_t)byte - 0x20 : byte;
                            if ((uint8_t)(letter - 'A') > 5) {
                                goto fail;
                            }
                            value = ((letter & 0xff) - 0x37) | (value << 4);
                        }
                        ++state;
                        if (state == 6) {
                            emit = true;
                        }
                    }
                    if (!emit) {
                        if (state == 0) {
                            emit = true;
                        } else if (state == 0xff) {
                            goto fail;
                        }
                    }
                }
                if (emit) {
                    state = 0;
                    buffer->chars.unicode[numChars] = (UniChar)value;
                    ++numChars;
                    buffer->numChars = numChars;
                }
            } while (bytes < end);
            if (state != 0) {
                goto fail;
            }
            return true;
        }
        default:
            break;
    }

    const StringEncodingConverterRecord *converter = CFStringEncodingGetConverter(encoding);
    if (converter == NULL) {
        return false;
    }
    // needsConversion: some byte must go through the converter; asciiPassThrough: bytes below 0x80 map to
    // themselves (the encoding is an ASCII superset).
    bool needsConversion;
    bool asciiPassThrough;
    if (encodingIsSupersetOfASCII(encoding)) {
        asciiPassThrough = true;
        if (buffer->isASCII) {
            needsConversion = false;
            for (OCFIndex index = 0; index < len; ++index) {
                if ((int8_t)bytes[index] < 0) {
                    buffer->isASCII = false;
                    needsConversion = true;
                    break;
                }
            }
            goto convert;
        }
    } else {
        asciiPassThrough = false;
        buffer->isASCII = false;
    }
    needsConversion = true;
    if (encoding == 1 && alwaysUnicode == 0) {
        // Mac Japanese keeps eight-bit contents unless a byte is non-ASCII or a backslash.
        buffer->isASCII = true;
        needsConversion = false;
        for (OCFIndex index = 0; index < len; ++index) {
            int8_t byte = (int8_t)bytes[index];
            if (byte < 0 || byte == '\\') {
                buffer->isASCII = false;
                needsConversion = true;
                break;
            }
        }
    }
convert:
    if (converterEncodingClass(converter) == 1) {
        if (needsConversion) {
            UniChar *characters = buffer->chars.unicode;
            buffer->shouldFreeChars = (uint64_t)len > 0x1f8 || characters != NULL;
            if (characters == NULL) {
                if ((uint64_t)len <= 0x1f8) {
                    characters = (UniChar *)buffer->localBuffer;
                    buffer->chars.unicode = characters;
                } else {
                    characters = OCFAllocatorAllocateTyped(buffer->allocator, len * 2, 0x1000040BDFB0063, 0);
                    buffer->chars.unicode = characters;
                    if (characters == NULL) {
                        goto fail;
                    }
                }
            }
            buffer->numChars = len;
            if (encoding == 0x201 || encoding == 0x600) {
                for (OCFIndex index = 0; index < len; ++index) {
                    buffer->chars.unicode[index] = bytes[index];
                }
            } else {
                for (OCFIndex index = 0; len != 0; ++index, --len) {
                    int8_t byte = (int8_t)*bytes++;
                    UniChar *destination = buffer->chars.unicode;
                    if (asciiPassThrough && byte >= 0) {
                        destination[index] = (UniChar)byte;
                    } else if (
                        converter->toUnicode.cheapEightBit(0, (uint8_t)byte, &destination[index]) == 0
                    ) {
                        goto fail;
                    }
                }
            }
            return true;
        }
    } else if (needsConversion) {
        OCFIndex numChars = CFStringEncodingCharLengthForBytes(encoding, 0, bytes, len);
        UniChar *characters = buffer->chars.unicode;
        buffer->shouldFreeChars = characters != NULL || (uint64_t)numChars > 0x1f8;
        if (characters == NULL) {
            if ((uint64_t)numChars <= 0x1f8) {
                characters = (UniChar *)buffer->localBuffer;
                buffer->chars.unicode = characters;
            } else {
                characters = OCFAllocatorAllocateTyped(buffer->allocator, numChars * 2, 0x1000040BDFB0063, 0);
                buffer->chars.unicode = characters;
                if (characters == NULL) {
                    goto fail;
                }
            }
        }
        if ((lossyFlag & 1) == 0) {
            lossyFlag = 1;
        }
        if (OCFStringEncodingBytesToUnicode(
                encoding,
                __CFWantsToUseASCIICompatibleConversion,
                bytes,
                len,
                NULL,
                characters,
                (uint64_t)numChars > 0x1f8 ? numChars : 0x1f8,
                &buffer->numChars
            ) == 0) {
            return true;
        }
        goto fail;
    }
    // Eight-bit contents copied as they are.
    buffer->numChars = len;
    uint8_t *characters = buffer->chars.ascii;
    buffer->shouldFreeChars = (uint64_t)len > 0x3f0 || characters != NULL;
    if (characters == NULL) {
        if ((uint64_t)len <= 0x3f0) {
            characters = buffer->localBuffer;
            buffer->chars.ascii = characters;
        } else {
            characters = OCFAllocatorAllocateTyped(buffer->allocator, len, 0x100004077774924, 0);
            buffer->chars.ascii = characters;
            if (characters == NULL) {
                goto fail;
            }
        }
    }
    memmove(characters, bytes, len);
    return true;

fail:
    if (buffer->shouldFreeChars && buffer->chars.ascii != NULL) {
        OCFAllocatorDeallocate(buffer->allocator, buffer->chars.ascii);
    }
    buffer->isASCII = alwaysUnicode == 0;
    buffer->shouldFreeChars = false;
    buffer->chars.ascii = NULL;
    buffer->numChars = 0;
    return false;
}

// Native __CFStringEncodeByteStream, 0x1c022cf40. External. Converts characters [rangeLoc, rangeLoc +
// rangeLen) of string to encoding, writing at most max bytes to buffer (or only measuring when buffer is
// NULL); returns the number of characters converted and stores the byte count through usedBufLen. lossByte is
// compared and shifted as an unsigned byte (CMP W27, #0xff; LSL #24), so it is declared UInt8. Paths, in
// native order: the whole-string UTF-8 pointer; UTF-8 through the UTF-8 converter's to-bytes slot; non-lossy
// ASCII escapes; UTF-16 and UTF-32 (with optional BOM); inline tagged strings through
// CFStringGetBytes(ASCII); the eight-bit contents (C string or Pascal string pointer), copied directly when
// they are already in the target encoding and otherwise as an ASCII prefix; then
// CFStringEncodingUnicodeToBytes over the characters pointer or in 512-character chunks, retrying around
// composed character sequences that a chunk boundary splits.
OCFIndex __OCFStringEncodeByteStream(
    OCFStringRef string,
    OCFIndex rangeLoc,
    OCFIndex rangeLen,
    Boolean generatingExternalFile,
    OCFStringEncoding encoding,
    UInt8 lossByte,
    UInt8 *buffer,
    OCFIndex max,
    OCFIndex *usedBufLen
) {
    OCFIndex numCharsProcessed = 0;
    OCFIndex totalBytesWritten = 0;
    OCFIndex utf8Length = 0;
    OCFIndex consumedChars;
    bool useASCIICompatibility;
    bool tryCharactersPtr;
    const char *source;
    const char *cursor;

    if (rangeLoc == 0 && encoding == 0x8000100) {
        if (rangeLen == OCFStringGetLength(string)) {
            const char *utf8 = _CFStringGetUTF8PtrWithOptions(string, &utf8Length, false, true);
            if (utf8 != NULL && (buffer == NULL || utf8Length <= max)) {
                if (buffer != NULL && utf8Length >= 1) {
                    memmove(buffer, utf8, utf8Length);
                }
                if (usedBufLen != NULL) {
                    *usedBufLen = utf8Length;
                }
                return rangeLen;
            }
        }
    } else if (encoding != 0x8000100) {
        goto otherEncodings;
    }

    {
        const UniChar *characters = OCFStringGetCharactersPtr(string);
        if (characters != NULL) {
            UTF8FromUnicodeProc toUTF8 =
                (UTF8FromUnicodeProc)(void *)__CFConverterUTF8.toBytes.cheapMultiByte;
            numCharsProcessed = toUTF8(
                generatingExternalFile == 0 ? 0 : 0x100,
                characters + rangeLoc,
                rangeLen,
                buffer,
                buffer == NULL ? 0 : max,
                &totalBytesWritten
            );
            goto storeUsed;
        }
        consumedChars = 0;
        goto eightBit;
    }

otherEncodings:
    if (encoding == 0xbff) {
        OCFStringInlineBuffer inlineBuffer;
        // The native code clears the 128-byte character array before initializing the buffer.
        memset(inlineBuffer.buffer, 0, sizeof(inlineBuffer.buffer));
        stringInitInlineBuffer(string, &inlineBuffer, (OCFRange){rangeLoc, rangeLen});
        if (rangeLen < 1) {
            goto storeUsed;
        }
        OCFIndex index = 0;
        do {
            UniChar character = stringGetCharacterFromInlineBuffer(&inlineBuffer, index);
            uint8_t escape[8];
            size_t count;
            if (((UniChar)(character - 0x20) < 0x5f && character != '\\') ||
                (character <= 0xd && ((UINT32_C(1) << character) & 0x2600) != 0)) {
                escape[0] = (uint8_t)character;
                count = 1;
            } else if (character == '\\') {
                escape[1] = '\\';
                escape[0] = '\\';
                count = 2;
            } else if (character <= 0xff) {
                escape[1] = (uint8_t)((character >> 6) | 0x30);
                escape[2] = (uint8_t)(0x30 | ((character >> 3) & 7));
                escape[3] = (uint8_t)(0x30 | (character & 7));
                escape[0] = '\\';
                count = 4;
            } else {
                static const char hexDigits[] = "0123456789abcdef";
                escape[1] = 'u';
                escape[2] = (uint8_t)hexDigits[character >> 12];
                escape[3] = (uint8_t)hexDigits[(character >> 8) & 0xf];
                escape[4] = (uint8_t)hexDigits[(character >> 4) & 0xf];
                escape[5] = (uint8_t)hexDigits[character & 0xf];
                escape[0] = '\\';
                count = 6;
            }
            OCFIndex next = totalBytesWritten + (OCFIndex)count;
            if (buffer != NULL) {
                if (next > max) {
                    break;
                }
                memcpy(buffer + totalBytesWritten, escape, count);
            }
            ++index;
            numCharsProcessed = index;
            totalBytesWritten = next;
        } while (index < rangeLen);
        goto storeUsed;
    }

    if (encoding == 0x100 || encoding == 0x14000100 || encoding == 0x10000100) {
        bool bom = encoding == 0x100 && generatingExternalFile != 0;
        OCFIndex bomLength = bom ? 2 : 0;
        numCharsProcessed = rangeLen;
        OCFIndex size = bomLength + rangeLen * 2;
        if (buffer == NULL) {
            totalBytesWritten = size;
            goto storeUsed;
        }
        OCFIndex byteLength;
        if (size <= max) {
            byteLength = rangeLen * 2;
        } else {
            OCFIndex room = max - bomLength;
            rangeLen = room > 0 ? (OCFIndex)((uint64_t)room >> 1) : 0;
            byteLength = rangeLen * 2;
            numCharsProcessed = rangeLen;
            size = bomLength + rangeLen * 2;
        }
        totalBytesWritten = size;
        if (bom) {
            *(UniChar *)buffer = 0xfeff;
            buffer += 2;
            OCFStringGetCharacters(string, (OCFRange){rangeLoc, rangeLen}, (UniChar *)buffer);
            goto storeUsed;
        }
        OCFStringGetCharacters(string, (OCFRange){rangeLoc, rangeLen}, (UniChar *)buffer);
        if (encoding == 0x10000100 && rangeLen >= 1) {
            UniChar *cursor = (UniChar *)buffer;
            UniChar *limit = (UniChar *)(buffer + byteLength);
            do {
                *cursor = __builtin_bswap16(*cursor);
                ++cursor;
            } while (cursor < limit);
        }
        goto storeUsed;
    }

    if (encoding == 0xc000100 || encoding == 0x1c000100 || encoding == 0x18000100) {
        OCFStringInlineBuffer inlineBuffer;
        memset(inlineBuffer.buffer, 0, sizeof(inlineBuffer.buffer));
        if (generatingExternalFile != 0 && encoding == 0xc000100) {
            totalBytesWritten = 4;
            if (buffer != NULL) {
                if (max > 3) {
                    *(uint32_t *)buffer = 0xfeff;
                    buffer += 4;
                } else {
                    totalBytesWritten = 0;
                }
            }
        }
        stringInitInlineBuffer(string, &inlineBuffer, (OCFRange){rangeLoc, rangeLen});
        if (rangeLen < 1) {
            goto storeUsed;
        }
        OCFIndex index = 0;
        do {
            UniChar character = stringGetCharacterFromInlineBuffer(&inlineBuffer, index);
            UTF32Char value;
            if ((character & 0xfc00) == 0xdc00) {
                value = lossByte;
                if (lossByte == 0) {
                    break;
                }
            } else {
                value = character;
                if ((character & 0xfc00) == 0xd800) {
                    bool paired = false;
                    if (index + 1 < rangeLen) {
                        UniChar low = stringGetCharacterFromInlineBuffer(&inlineBuffer, index + 1);
                        if ((low >> 10) == 0x37) {
                            value = (value << 10) + low + 0xfca02400;
                            paired = true;
                        }
                    }
                    if (!paired) {
                        value = lossByte;
                        if (lossByte == 0) {
                            break;
                        }
                    }
                }
            }
            OCFIndex previous = totalBytesWritten;
            totalBytesWritten = previous + 4;
            if (buffer != NULL) {
                if (totalBytesWritten > max) {
                    totalBytesWritten = previous;
                    break;
                }
                *(uint32_t *)buffer = encoding == 0x18000100 ? __builtin_bswap32(value) : value;
                buffer += 4;
            }
            index += (value >> 16) != 0 ? 2 : 1;
            numCharsProcessed = index;
        } while (index < rangeLen);
        goto storeUsed;
    }

    consumedChars = 0;
    if (encodingIsSupersetOfASCII(encoding)) {
        goto eightBit;
    }
    goto checkValid;

eightBit:
    if (stringIsTagged(string) && encodingsGetTaggedPointerTag(string) == 2) {
        return OCFStringGetBytes(
            string, (OCFRange){rangeLoc, rangeLen}, 0x600, 0, false, buffer, max, usedBufLen
        );
    }
    {
        OCFStringEncoding eightBitEncoding = __OCFDefaultEightBitStringEncoding;
        if (eightBitEncoding == 0xffffffff) {
            eightBitEncoding = __OCFStringComputeEightBitStringEncoding();
        }
        const char *cString = _OCFStringGetCStringPtrWithOptions(string, eightBitEncoding, false, true);
        if (cString != NULL) {
            source = cString + rangeLoc;
            OCFStringEncoding current = __OCFDefaultEightBitStringEncoding;
            if (current == 0xffffffff) {
                current = __OCFStringComputeEightBitStringEncoding();
            }
            if (current == encoding) {
                goto directCopy;
            }
            current = __OCFDefaultEightBitStringEncoding;
            if (current == 0xffffffff) {
                current = __OCFStringComputeEightBitStringEncoding();
            }
            if (current == 0x600) {
                goto directCopy;
            }
            // The ASCII prefix stops at the first byte with bit 7 set or when only the part beyond the buffer
            // is left (limit is rangeLen - min(max, rangeLen) when there is a buffer).
            OCFIndex limit = buffer != NULL ? rangeLen - (max < rangeLen ? max : rangeLen) : 0;
            cursor = source;
            if ((int8_t)*cursor >= 0 && rangeLen > limit) {
                do {
                    ++cursor;
                    --rangeLen;
                } while ((int8_t)*cursor >= 0 && rangeLen > limit);
            }
            goto copyPrefix;
        }
        OCFStringEncoding pascalEncoding = __OCFDefaultEightBitStringEncoding;
        if (pascalEncoding == 0xffffffff) {
            pascalEncoding = __OCFStringComputeEightBitStringEncoding();
        }
        const unsigned char *pascal = OCFStringGetPascalStringPtr(string, pascalEncoding);
        if (pascal == NULL) {
            useASCIICompatibility = false;
            tryCharactersPtr = true;
            if (buffer == NULL) {
                goto convert;
            }
            goto checkMax;
        }
        source = (const char *)pascal + rangeLoc + 1;
        OCFStringEncoding current = __OCFDefaultEightBitStringEncoding;
        if (current == 0xffffffff) {
            current = __OCFStringComputeEightBitStringEncoding();
        }
        if (current == encoding) {
            goto directCopy;
        }
        cursor = source;
        if ((int8_t)*cursor >= 0 && rangeLen >= 1) {
            do {
                ++cursor;
                --rangeLen;
            } while ((int8_t)*cursor >= 0 && rangeLen != 0);
        }
        {
            OCFIndex copied;
        copyPrefix:
            copied = cursor - source;
            numCharsProcessed = copied;
            if (buffer != NULL) {
                copied = copied < max ? copied : max;
                numCharsProcessed = copied;
                memmove(buffer, source, copied);
                buffer += copied;
                max -= copied;
            }
            if (rangeLen == 0 || (buffer != NULL && max == 0)) {
                if (usedBufLen != NULL) {
                    *usedBufLen = copied;
                }
                return copied;
            }
            useASCIICompatibility = false;
            tryCharactersPtr = false;
            rangeLoc += copied;
            totalBytesWritten += copied;
            if (buffer == NULL) {
                goto convert;
            }
            goto checkMax;
        }
    }

directCopy:
    {
        OCFIndex count = rangeLen < max ? rangeLen : max;
        OCFIndex result = buffer != NULL ? count : rangeLen;
        if (buffer != NULL) {
            memmove(buffer, source, count);
        }
        if (usedBufLen != NULL) {
            *usedBufLen = result;
        }
        return result;
    }

checkValid:
    if (!CFStringEncodingIsValidEncoding(encoding)) {
        goto empty;
    }
    useASCIICompatibility = true;
    tryCharactersPtr = true;
    if (buffer == NULL) {
        goto convert;
    }
checkMax:
    if (max != 0) {
        goto convert;
    }
empty:
    if (usedBufLen != NULL) {
        *usedBufLen = 0;
    }
    return 0;

convert:
    {
        if (buffer == NULL) {
            max = 0;
        }
        // Converter flags: the loss byte in bits 24-31 plus 1 (just 1 for 0xff in ASCII; 0 without a loss
        // byte), 0x100 for an external representation (first call only), and the ASCII-compatibility byte.
        UInt32 flags;
        if (lossByte == 0) {
            flags = 0;
        } else if (lossByte == 0xff && encoding == 0x600) {
            flags = 1;
        } else {
            flags = ((UInt32)lossByte << 24) + 1;
        }
        flags |= generatingExternalFile == 0 ? 0 : 0x100;
        flags |= useASCIICompatibility ? __CFWantsToUseASCIICompatibleConversion : 0;
        if (tryCharactersPtr) {
            const UniChar *characters = OCFStringGetCharactersPtr(string);
            if (characters != NULL) {
                OCFStringEncodingUnicodeToBytes(
                    encoding,
                    flags,
                    characters + rangeLoc,
                    rangeLen,
                    &numCharsProcessed,
                    buffer,
                    max,
                    &totalBytesWritten
                );
                goto storeUsed;
            }
        }
        OCFIndex chunkBytes = 0;
        if (rangeLen < 1) {
            goto storeUsed;
        }
        UniChar characters[512];
        UInt32 state = 0;
        OCFIndex previousChars = 0;
        OCFIndex previousBytes = 0;
        UInt32 laterFlags = flags & 0xfffffeff;
        for (;;) {
            OCFIndex count = (uint64_t)rangeLen < 0x200 ? rangeLen : 0x200;
            OCFStringGetCharacters(string, (OCFRange){rangeLoc, count}, characters);
            // A chunk does not end on a high surrogate (the 512th character is tested even when fewer remain
            // only if more than 512 do).
            if ((uint64_t)rangeLen >= 0x201) {
                count -= (characters[511] & 0xfc00) == 0xd800;
            }
            UInt32 chunkFlags = ((uint64_t)rangeLen > (uint64_t)count ? 0x1000 : 0) | (state << 16);
            UInt32 status = OCFStringEncodingUnicodeToBytes(
                encoding, chunkFlags | flags, characters, count, &consumedChars, buffer, max, &chunkBytes
            );
            state = (status >> 16) & 0xff;
            status &= 0xff00ffff;
            if (status != 0) {
                OCFIndex savedPreviousBytes = previousBytes;
                if (status != 1) {
                    goto finish;
                }
                OCFIndex consumed = consumedChars;
                if ((uint64_t)rangeLen >= 0x201 && count - consumed <= 5) {
                    OCFRange composed =
                        OCFStringGetRangeOfComposedCharactersAtIndex(string, count + rangeLoc);
                    status = 1;
                    consumed = consumedChars;
                    if (composed.length <= 6 && composed.location < consumed + rangeLoc) {
                        status = OCFStringEncodingUnicodeToBytes(
                            encoding,
                            chunkFlags | flags,
                            characters,
                            composed.location - rangeLoc,
                            &consumedChars,
                            buffer,
                            max,
                            &chunkBytes
                        );
                        state = (status >> 16) & 0xff;
                        status &= 0xff00ffff;
                        consumed = consumedChars;
                    }
                }
                if (status != 0) {
                    // Re-convert the previous chunk's tail together with a composed sequence that starts
                    // there.
                    if (previousChars < 1 || consumed > 5) {
                        goto finish;
                    }
                    OCFRange back = OCFStringGetRangeOfComposedCharactersAtIndex(string, rangeLoc);
                    if (back.length > 6 || back.location >= rangeLoc) {
                        goto finish;
                    }
                    OCFStringGetCharacters(string, back, characters);
                    if (OCFStringEncodingUnicodeToBytes(
                            encoding, flags, characters, back.length, &consumedChars, NULL, 0, &chunkBytes
                        ) != 0) {
                        goto finish;
                    }
                    rangeLoc -= previousChars;
                    OCFIndex redoLength = back.location - rangeLoc;
                    OCFStringGetCharacters(string, (OCFRange){rangeLoc, redoLength}, characters);
                    UInt8 *redoBuffer = (UInt8 *)((uintptr_t)buffer - (uintptr_t)savedPreviousBytes);
                    OCFIndex redoMax = max == 0 ? 0 : max + savedPreviousBytes;
                    status = OCFStringEncodingUnicodeToBytes(
                        encoding,
                        chunkFlags | flags,
                        characters,
                        redoLength,
                        &consumedChars,
                        max == 0 ? NULL : redoBuffer,
                        redoMax,
                        &chunkBytes
                    );
                    if ((status & 0xff00ffff) != 0) {
                        goto finish;
                    }
                    state = status >> 16;
                    totalBytesWritten -= savedPreviousBytes;
                    numCharsProcessed -= previousChars;
                    rangeLen += previousChars;
                    buffer = max == 0 ? buffer : redoBuffer;
                    max = redoMax;
                }
            }
            previousBytes = chunkBytes;
            previousChars = consumedChars;
            totalBytesWritten += previousBytes;
            numCharsProcessed += previousChars;
            if (max != 0) {
                max -= previousBytes;
                if (max < 1) {
                    goto storeUsed;
                }
                buffer += previousBytes;
            }
            rangeLoc += previousChars;
            flags = laterFlags;
            rangeLen -= previousChars;
            if (rangeLen <= 0) {
                goto storeUsed;
            }
        }
    finish:
        totalBytesWritten += chunkBytes;
        numCharsProcessed += consumedChars;
    }

storeUsed:
    if (usedBufLen != NULL) {
        *usedBufLen = totalBytesWritten;
    }
    return numCharsProcessed;
}

// Native CFStringCreateWithFileSystemRepresentation, 0x1c022dd6c. External.
OCFStringRef OCFStringCreateWithFileSystemRepresentation(OCFAllocatorRef alloc, const char *buffer) {
    return OCFStringCreateWithCString(alloc, buffer, OCFStringFileSystemEncoding());
}

// Native CFStringGetMaximumSizeOfFileSystemRepresentation, 0x1c022dd9c. External. Three bytes per character
// for ASCII and Mac Roman fastest encodings, otherwise nine, plus one; -1 when the product would overflow
// (length above 0x2aaaaaaaaaaaaaaa or 0x0e38e38e38e38e38, signed).
OCFIndex OCFStringGetMaximumSizeOfFileSystemRepresentation(OCFStringRef string) {
    OCFIndex length = OCFStringGetLength(string);
    OCFStringEncoding fastest = OCFStringGetFastestEncoding(string);
    if (fastest == 0x600 || fastest == 0) {
        return length > (LONG_MAX / 3) ? -1 : length * 3 + 1;
    }
    return length > (LONG_MAX / 9) ? -1 : length * 9 + 1;
}

// Native _CFStringGetUserDefaultEncoding, 0x1c022de0c. External. Parses __CF_USER_TEXT_ENCODING as
// "<uid>:<script>:<region>" (strtol_l, base 0, NULL locale) when its uid equals the effective user ID;
// otherwise a positive uid rewrites the variable to "0x%X:0:0" and the values are zero. A script whose value
// with bit 0 cleared is 4 becomes 0, and Unicode encodings (ror32(script - 0x100, 26) is 0 or 3-7) store 0. A
// script not followed by ':' stores 0 for both values. The region is parsed only when regionValue is not
// NULL.
void _OCFStringGetUserDefaultEncoding(uint32_t *scriptValue, uint32_t *regionValue) {
    uint32_t uid = _OCFGetEUID();
    char *cursor = (char *)__CFgetenv("__CF_USER_TEXT_ENCODING");
    if (cursor != NULL) {
        long value = strtol_l(cursor, &cursor, 0, NULL);
        if (value == (long)(int32_t)uid && *cursor == ':') {
            ++cursor;
            goto parse;
        }
        cursor = NULL;
    }
    if ((int32_t)uid < 1) {
        goto zero;
    }
    char string[32];
    snprintf(string, sizeof(string), "0x%X:0:0", uid);
    setenv("__CF_USER_TEXT_ENCODING", string, 1);
    if (cursor == NULL) {
        goto zero;
    }
parse:
    {
        int32_t script = (int32_t)strtol_l(cursor, &cursor, 0, NULL);
        if ((script & 0xfffffffe) == 4) {
            script = 0;
        }
        uint32_t offset = (uint32_t)script - 0x100;
        uint32_t rotated = (offset >> 26) | (offset << 6);
        *scriptValue = (uint32_t)script;
        if (rotated - 3 < 5 || rotated == 0) {
            *scriptValue = 0;
        }
        if (*cursor != ':') {
            goto zero;
        }
        if (regionValue == NULL) {
            return;
        }
        ++cursor;
        *regionValue = (uint32_t)strtol_l(cursor, NULL, 0, NULL);
        return;
    }
zero:
    *scriptValue = 0;
    if (regionValue != NULL) {
        *regionValue = 0;
    }
}

// Native _objc_getTaggedPointerTag, 0x1c022e2ec. Non-external (static in this unit). The obfuscator is
// skipped for pointers with bits 63, 62 and 0-2 set; the low three bits of the decoded value are searched in
// the first seven entries of objc_debug_tag60_permutations; not found (index 7) answers the extended tag 8 +
// bits 55-62 of the decoded value.
static uint32_t encodingsGetTaggedPointerTag(const void *ptr) {
    uintptr_t pointer = (uintptr_t)ptr;
    uintptr_t decoded =
        ((~pointer & 0xC000000000000007) == 0 ? 0 : objc_debug_taggedpointer_obfuscator) ^ pointer;
    uintptr_t index = 0;
    do {
        if ((decoded & 7) == objc_debug_tag60_permutations[index]) {
            break;
        }
        ++index;
    } while (index != 7);
    if ((index & 7) == 7) {
        return (uint32_t)((OF_WORD_SHIFT((index | decoded), 55)) & 0xFF) + 8;
    }
    return (uint32_t)(index & 7);
}
