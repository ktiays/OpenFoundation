//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Unicode/OCFUniChar.h"
#include "CoreFoundation/Unicode/OCFUniCharData.h"

#include <string.h>

// Native unnamed local table of CFUniCharGetBitmapForPlane, iOS Simulator 24A434, 0x1c03a46a8
// (seven 16-bit newline characters and two bytes of padding before __CFUniCharCaseMappingTableCounts),
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1808a0718, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified
// for the seven values the loop reads. kNewlineCharacters is a local descriptive name.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
static const uint16_t kNewlineCharacters[7] = {10, 11, 12, 13, 133, 8232, 8233};

// Native CFUniCharIsMemberOf, iOS Simulator 24A434, 0x1c02512c4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1806d523c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified for code; branch scheduling differs.
// iOS Device divergence: CFUniCharIsMemberOf at 0x1806d523c has unread authenticated
// descriptor target bindings; independently captured bitmap payloads agree.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
uint8_t OCFUniCharIsMemberOf(uint32_t character, uint32_t characterSet) {
    uint32_t mappedSet = characterSet;
    if (characterSet - 16 < 84) {
        mappedSet = characterSet + 85;
    }
    if (characterSet == 1) {
        mappedSet = 108;
    }
    if (mappedSet == 15) {
        return character - 10 < 4 || character - 8232 < 2 || character == 133;
    }
    if (mappedSet == 2 || mappedSet == 3) {
        if (character == 9 || character == 32 || character == 160 || character == 5760) {
            return 1;
        }
        if (character - 0x2000 < 12) {
            return 1;
        }
        if (mappedSet == 2) {
            return character == 8239 || character == 8287 || character == 12288;
        }
        if (character - 10 < 4 || character == 133) {
            return 1;
        }
        if (character - 8232 <= 55 && (UINT64_C(0x80000000000083) >> (character - 8232) & 1) != 0) {
            return 1;
        }
        return character == 12288;
    }
    uint32_t tableSet = mappedSet;
    if (mappedSet > 99) {
        tableSet -= 85;
    }
    uint32_t index = tableSet - 4;
    if (index > 21) {
        return 0;
    }
    const UniCharBitmapData *entry = &__CFUniCharBitmapDataArray[index];
    uint32_t plane = (character >> 16) & 255;
    uint32_t lowByte = character & 255;
    if (mappedSet == 108 && plane == 14) {
        return lowByte == 1 || lowByte - 32 < 96;
    }
    if (mappedSet == 12) {
        if (plane == 14) {
            return !(lowByte == 1 || lowByte - 128 >= UINT32_C(0xffffffa0));
        }
        if (plane - 15 < 2) {
            return 0;
        }
        if (plane < entry->planeCount) {
            const uint8_t *bitmap = entry->planes[plane];
            if (bitmap != 0) {
                return ((bitmap[(uint16_t)character >> 3] >> (character & 7)) & 1) == 0;
            }
        }
        return 1;
    }
    if (plane < entry->planeCount) {
        const uint8_t *bitmap = entry->planes[plane];
        if (bitmap != 0) {
            return (bitmap[(uint16_t)character >> 3] >> (character & 7)) & 1;
        }
    }
    return 0;
}

// Native CFUniCharGetBitmapPtrForPlane, iOS Simulator 24A434, 0x1c0251524,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1806400ec, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified for code.
// iOS Device divergence: CFUniCharGetBitmapPtrForPlane at 0x1806400ec has unread
// authenticated descriptor target bindings; independently captured bitmap payloads agree.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
const uint8_t *OCFUniCharGetBitmapPtrForPlane(uint32_t characterSet, uint32_t plane) {
    uint32_t mappedSet = characterSet;
    if (characterSet - 16 < 84) {
        mappedSet = characterSet + 85;
    }
    if (characterSet == 1) {
        mappedSet = 108;
    }
    if (mappedSet <= 15 && (UINT32_C(0x900f) >> mappedSet & 1) != 0) {
        return 0;
    }
    if (mappedSet > 99) {
        mappedSet -= 85;
    }
    uint32_t index = mappedSet - 4;
    if (index <= 21 && plane < __CFUniCharBitmapDataArray[index].planeCount) {
        return __CFUniCharBitmapDataArray[index].planes[plane];
    }
    return 0;
}

// Native CFUniCharGetBitmapForPlane, iOS Simulator 24A434, 0x1c02515a0,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden (private external).
// iOS Device 24A446: 0x180715aa0, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: CFUniCharGetBitmapForPlane at 0x180715aa0 inlines
// CFUniCharGetBitmapPtrForPlane instead of calling it; the observable results agree.
// Native callers pass isInverted as 0 or 1 (CSET, UBFX, EOR of flags); the body tests bit 0 and the
// whole word, which agree only on those values.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
uint8_t CFUniCharGetBitmapForPlane(uint32_t characterSet, uint32_t plane, void *bitmap, bool isInverted) {
    uint8_t *destination = bitmap;
    const uint8_t *source = OCFUniCharGetBitmapPtrForPlane(characterSet, plane);
    if (source != 0) {
        if (isInverted) {
            for (uint32_t count = 8193; count > 1; --count) {
                *destination++ = ~*source++;
            }
        } else {
            for (uint32_t count = 8193; count > 1; --count) {
                *destination++ = *source++;
            }
        }
        return 0;
    }
    int32_t result;
    if (characterSet == 12) {
        if (plane > 14) {
            if (plane - 15 <= 1) {
                uint32_t *words = bitmap;
                for (uint32_t count = 2049; count > 1; --count) {
                    *words++ = isInverted ? UINT32_MAX : 0;
                }
                ((uint8_t *)words)[-5] = isInverted ? 0x3f : 0xc0;
                return 0;
            }
        } else {
            const uint8_t *legal = __CFUniCharLegalCharacterSetBitmap[plane];
            if (legal != 0) {
                if (isInverted) {
                    for (uint32_t count = 8193; count > 1; --count) {
                        *destination++ = *legal++;
                    }
                } else {
                    for (uint32_t count = 8193; count > 1; --count) {
                        *destination++ = ~*legal++;
                    }
                }
                return 0;
            }
            if (plane == 14) {
                *destination = 2;
                uint32_t counter = UINT32_C(0xfffffff1);
                for (uint32_t count = 0x1fff; count != 0; --count) {
                    bool leading = counter < UINT32_C(0xfffffff4);
                    ++counter;
                    *++destination = isInverted != leading ? 0xff : 0;
                }
                return 0;
            }
        }
        result = -1;
    } else {
        if ((characterSet < 4 || characterSet == 15) && plane == 0) {
            memset(destination, isInverted ? 0xff : 0, 0x2000);
            if (characterSet == 15 || characterSet == 3) {
                for (size_t index = 0; index != 7; ++index) {
                    uint16_t character = kNewlineCharacters[index];
                    uint8_t mask = (uint8_t)(1 << (character & 7));
                    if (isInverted) {
                        destination[character >> 3] &= (uint8_t)~mask;
                    } else {
                        destination[character >> 3] |= mask;
                    }
                }
                if (characterSet == 15) {
                    return 0;
                }
            }
            if (isInverted) {
                destination[1] &= 0xfd;
                destination[4] &= 0xfe;
                destination[20] &= 0xfe;
                destination[720] &= 0xfe;
                destination[1029] &= 0x7f;
                destination[1035] &= 0x7f;
                destination[1536] &= 0xfe;
            } else {
                destination[1] |= 0x02;
                destination[4] |= 0x01;
                destination[20] |= 0x01;
                destination[720] |= 0x01;
                destination[1029] |= 0x80;
                destination[1035] |= 0x80;
                destination[1536] |= 0x01;
            }
            for (size_t character = 0x2000; character != 0x200c; ++character) {
                uint8_t mask = (uint8_t)(1 << (character & 7));
                if (isInverted) {
                    destination[character >> 3] &= (uint8_t)~mask;
                } else {
                    destination[character >> 3] |= mask;
                }
            }
            return 0;
        }
        result = 1;
    }
    if (!isInverted) {
        result = -result;
    }
    return (uint8_t)result;
}

// Native CFUniCharGetNumberOfPlanes, iOS Simulator 24A434, 0x1c0251870,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden (private external).
// iOS Device 24A446: 0x180715de4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions.
// The record index is not checked: sets outside the 22 read records read past the descriptor, as
// in the native code. Callers in this restoration pass read set identifiers only.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
uint32_t CFUniCharGetNumberOfPlanes(uint32_t characterSet) {
    if (characterSet == 1 || characterSet == 108) {
        return 15;
    }
    if (characterSet < 4) {
        return 1;
    }
    if (characterSet == 12) {
        return 17;
    }
    uint32_t mappedSet = characterSet;
    if (characterSet - 16 < 84) {
        mappedSet = characterSet + 85;
    }
    if (mappedSet > 99) {
        mappedSet -= 85;
    }
    return __CFUniCharBitmapDataArray[mappedSet - 4].planeCount;
}

// Native CFUniCharMapCaseTo, iOS Simulator 24A434, 0x1c02518e0,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Exported.
// iOS Device 24A446: 0x1806cb400, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: CFUniCharMapCaseTo at 0x1806cb400 outlines the Lithuanian replacement switch into
// CFUniCharMapCaseTo.cold.1 (0x1807c7c18; confidence: verified), which takes (flags & 4) == 0, returns 1 to
// fall through to the bitmap test and otherwise stores the unit count through a stack slot; it tests the same
// characters in a different order with the same stores. The single-unit surrogate-pair store is outlined
// into CFUniCharMapCaseTo.cold.2 (0x1807c7cdc; confidence: verified) with the same formula. Every branch,
// constant, table read and store agrees.
// The language code is read as one 16-bit load, as in the native code. The context indexes the case tables
// and the membership remapping without a check; contexts 0..3 (lower, upper, title, fold) are the read
// ones. The language-specific, self-mapping and decomposed-accent stores do not check maxLength, as in the
// native code. A UTF-32 extra-table run (bit 31 with a count other than one) that fits completely falls
// through to the self mapping; no pinned table entry has that form.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
OCFIndex OCFUniCharMapCaseTo(
    uint32_t character,
    uint16_t *converted,
    OCFIndex maxLength,
    uint32_t context,
    uint32_t flags,
    const uint8_t *langCode
) {
    uint32_t decomposed[10];
    const uint32_t *decomposedEnd = decomposed;
    uint32_t finalSigma = (uint32_t)(character == 0x3a3) & flags;
    uint32_t originalContext = context;
    uint32_t foldedCharacter = character & UINT32_C(0xffffffdf);
    bool keepsDotAbove = !((flags & 2) != 0 && character == 0x307);
    uint32_t plane = (character >> 16) & 0xff;
    uint32_t mask = UINT32_C(1) << (character & 7);
    uint32_t byteIndex = (character >> 3) & 0x1fff;
    for (;;) {
        if (finalSigma != 0) {
            *converted = originalContext == 0 ? 0x3c2 : 0x3a3;
            return 1;
        }
        if (langCode != 0) {
            if ((flags & 0x10) != 0) {
                if (character == 0x301) {
                    return 0;
                }
                if (character == 0x344) {
                    *converted = 0x308;
                    return 1;
                }
                if (OCFUniCharIsMemberOf(character, 9) != 0) {
                    OCFIndex count = OCFUniCharDecomposeCharacter(character, decomposed, 10);
                    if (count >= 2) {
                        decomposedEnd = decomposed + count;
                        const uint32_t *cursor = decomposed + 1;
                        do {
                            if (*cursor++ == 0x301) {
                                goto mapDecomposed;
                            }
                        } while (cursor < decomposedEnd);
                    }
                }
            }
            uint16_t language;
            memcpy(&language, langCode, sizeof(language));
            bool turkic = false;
            if (language > 0x746b) {
                if (language == 0x7a61) {
                    turkic = true;
                } else if (language == 0x746c) {
                    if (!keepsDotAbove) {
                        return 0;
                    }
                    if (context == 0) {
                        // The native code stores i and U+0307 as one 32-bit word; the unit order agrees.
                        uint16_t last = 0;
                        if ((flags & 4) != 0) {
                            if ((int32_t)character > 0xcc) {
                                if (character == 0xcd) {
                                    last = 0x301;
                                } else if (character == 0x128) {
                                    last = 0x303;
                                } else if (character == 0x12e) {
                                    converted[0] = 0x12f;
                                    converted[1] = 0x307;
                                    return 2;
                                }
                            } else if (character == 0x49) {
                                converted[0] = 0x69;
                                converted[1] = 0x307;
                                return 2;
                            } else if (character == 0x4a) {
                                converted[0] = 0x6a;
                                converted[1] = 0x307;
                                return 2;
                            } else if (character == 0xcc) {
                                last = 0x300;
                            }
                        } else if (character == 0x128) {
                            last = 0x303;
                        } else if (character == 0xcd) {
                            last = 0x301;
                        } else if (character == 0xcc) {
                            last = 0x300;
                        }
                        if (last != 0) {
                            converted[0] = 0x69;
                            converted[1] = 0x307;
                            converted[2] = last;
                            return 3;
                        }
                    }
                }
            } else if (language == 0x6c6e) {
                if (foldedCharacter == 0x4a) {
                    uint16_t letter = (flags & 8) == 0 ? 0x6a : 0x4a;
                    *converted = context - 1 < 2 ? 0x4a : letter;
                    return 1;
                }
            } else if (language == 0x7274) {
                turkic = true;
            }
            if (turkic) {
                bool dotlessCandidate = false;
                if ((int32_t)character <= 0x130) {
                    if (character == 0x49) {
                        dotlessCandidate = true;
                    } else if (character == 0x69 || character == 0x130) {
                        *converted = context == 3 || context == 0 ? 0x69 : 0x130;
                        return 1;
                    }
                } else if (character == 0x307) {
                    if ((flags & 2) != 0) {
                        if (context == 0) {
                            return 0;
                        }
                        *converted = 0x307;
                        return 1;
                    }
                } else if (character == 0x131) {
                    dotlessCandidate = true;
                }
                if (dotlessCandidate) {
                    if (context == 3 || context == 0) {
                        *converted = (flags & 4) == 0 ? 0x131 : 0x69;
                    } else {
                        *converted = 0x49;
                    }
                    return 1;
                }
            }
        }
        uint32_t set = context + 103;
        uint32_t mappedSet = context + 87 < 84 ? context + 188 : set;
        if (set == 1) {
            mappedSet = 108;
        }
        if (mappedSet > 99) {
            mappedSet -= 85;
        }
        const UniCharBitmapData *record = &__CFUniCharBitmapDataArray[(uint32_t)(mappedSet - 4)];
        if (plane < record->planeCount) {
            const uint8_t *bitmap = record->planes[plane];
            if (bitmap != 0 && (mask & bitmap[byteIndex]) != 0) {
                break;
            }
        }
        bool repeat = context == 3;
        context = 0;
        if (!repeat) {
            goto selfMapping;
        }
    }
    const uint32_t *table = __CFUniCharCaseMappingTable[context];
    uint32_t value = 0;
    if (character >= table[0]) {
        const uint32_t *high = table + 2 * (size_t)(uint32_t)(__CFUniCharCaseMappingTableCounts[context] - 1);
        if (character <= high[0]) {
            const uint32_t *low = table;
            do {
                const uint32_t *middle = low + 2 * ((high - low) / 4);
                if (character < middle[0]) {
                    high = middle - 2;
                } else if (character <= middle[0]) {
                    value = middle[1];
                    break;
                } else {
                    low = middle + 2;
                }
            } while (low <= high);
        }
    }
    if (context == 2 && value == 0) {
        if (character < 0x61) {
            *converted = (uint16_t)character;
            return 1;
        }
        if (character > 0x1e943) {
            goto selfMapping;
        }
        // The title-case fallback searches the uppercase table directly, through its pair at byte 0x3158.
        const uint32_t *low = __CFUniCharToUppercaseMappingTable;
        const uint32_t *high = __CFUniCharToUppercaseMappingTable + 3158;
        for (;;) {
            const uint32_t *middle = low + 2 * ((high - low) / 4);
            if (character < middle[0]) {
                high = middle - 2;
            } else if (character <= middle[0]) {
                value = middle[1];
                context = value == 0 ? 2 : 1;
                break;
            } else {
                low = middle + 2;
            }
            if (low > high) {
                goto selfMapping;
            }
        }
    }
    if (value == 0) {
        goto selfMapping;
    }
    OCFIndex length = (value >> 24) & 0x1f;
    if (length == 1) {
        if ((value & UINT32_C(0x80000000)) != 0) {
            if (maxLength < 2) {
                goto selfMapping;
            }
            converted[0] = (uint16_t)((((value & UINT32_C(0xfffc00)) + UINT32_C(0x3ff0000)) >> 10) +
                                      UINT32_C(0xffffd800));
            converted[1] = (uint16_t)((value & 0x3ff) | 0xdc00);
            return 2;
        }
        *converted = (uint16_t)value;
        return 1;
    }
    if (maxLength <= length) {
        goto selfMapping;
    }
    const uint32_t *extra = __CFUniCharCaseMappingExtraTable[context] + (value & UINT32_C(0xffffff));
    if ((value & UINT32_C(0x80000000)) != 0) {
        if (length == 0) {
            goto selfMapping;
        }
        uint64_t remaining = (uint64_t)length + 1;
        OCFIndex written = 0;
        do {
            uint32_t scalar = *extra++;
            OCFIndex next;
            if (scalar >= 0x10000) {
                next = written + 2;
                if (next >= maxLength) {
                    if (remaining == 2) {
                        return written;
                    }
                    goto selfMapping;
                }
                converted[written] =
                    (uint16_t)((((scalar & UINT32_C(0xfffc00)) + UINT32_C(0x3ff0000)) >> 10) +
                               UINT32_C(0xffffd800));
                converted[written + 1] = (uint16_t)((scalar & 0x3ff) | 0xdc00);
            } else {
                next = written + 1;
                if (next >= maxLength) {
                    if (remaining == 2) {
                        return written;
                    }
                    goto selfMapping;
                }
                converted[written] = (uint16_t)scalar;
            }
            written = next;
            --remaining;
        } while (remaining > 1);
        goto selfMapping;
    }
    if (length == 0) {
        return 0;
    }
    for (OCFIndex index = 0; index != length; ++index) {
        converted[index] = (uint16_t)extra[index];
    }
    return length;

selfMapping:
    if (character >= 0x10000) {
        converted[0] = (uint16_t)((((character & UINT32_C(0xfffc00)) + UINT32_C(0x3ff0000)) >> 10) +
                                  UINT32_C(0xffffd800));
        converted[1] = (uint16_t)((character & 0x3ff) | 0xdc00);
        return 2;
    }
    *converted = (uint16_t)character;
    return 1;

mapDecomposed:;
    OCFIndex result = OCFUniCharMapCaseTo(decomposed[0], converted, maxLength, context, 0, langCode);
    if (result == 0) {
        *converted = (uint16_t)decomposed[0];
        result = 1;
    }
    const uint32_t *cursor = decomposed + 1;
    do {
        uint32_t scalar = *cursor;
        if (scalar != 0x301) {
            if ((scalar >> 16) != 0) {
                converted[result] = (uint16_t)(((scalar + UINT32_C(0x3ff0000)) >> 10) + UINT32_C(0xffffd800));
                converted[result + 1] = (uint16_t)((scalar & 0x3ff) | 0xdc00);
                result += 2;
            } else {
                converted[result++] = (uint16_t)scalar;
            }
        }
        ++cursor;
    } while (cursor < decomposedEnd);
    return result;
}

// Native CFUniCharGetConditionalCaseMappingFlags, iOS Simulator 24A434, 0x1c0252064,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden (private external).
// iOS Device 24A446: 0x180774138, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// register allocation and scheduling differ, every branch, read and result agrees.
// Native pointer walks are expressed as buffer indexes; the comparisons are the same.
// CoreFoundation defect: the Lithuanian forward scans (CFUniCharGetConditionalCaseMappingFlags at
// 0x1c0252344 and 0x1c0252454, iOS Device 0x180774428 and 0x18077453c; confidence: verified) test the unit
// two places after a high surrogate for the low-surrogate range and then combine with the unit one place
// after it. With exactly two units left, the tested unit is buffer[length], one unit past the caller's
// range. The restoration does not read it and treats that unit as not a low surrogate; every in-range
// result, including the unusual unit choice, is preserved.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
uint32_t CFUniCharGetConditionalCaseMappingFlags(
    uint32_t character,
    const uint16_t *buffer,
    OCFIndex currentIndex,
    OCFIndex length,
    uint32_t type,
    const uint8_t *langCode,
    uint32_t lastFlags
) {
    uint32_t decomposed[10];
    uint32_t scalar;
    if (character == 0x3a3) {
        if (currentIndex < 1 || type != 0) {
            return 0;
        }
        OCFIndex cursor = currentIndex - 1;
        while (cursor >= 0) {
            OCFIndex previous = cursor - 1;
            scalar = buffer[cursor];
            if ((scalar & 0xfc00) == 0xdc00 && previous >= 0) {
                uint32_t high = buffer[previous];
                uint32_t combined = scalar + UINT32_C(0xfca02400) + (high << 10);
                if ((high & 0xfc00) == 0xd800) {
                    scalar = combined;
                    previous = cursor - 2;
                }
            }
            cursor = previous;
            if ((OCFUniCharIsMemberOf(scalar, 0x6d) & 1) == 0) {
                if ((OCFUniCharIsMemberOf(scalar, 7) & 1) == 0 && OCFUniCharIsMemberOf(scalar, 6) == 0) {
                    return 0;
                }
                break;
            }
        }
        OCFIndex index = currentIndex + 1;
        while (index < length) {
            OCFIndex next = index + 1;
            scalar = buffer[index];
            if ((scalar & 0xfc00) == 0xd800 && next < length) {
                uint32_t low = buffer[next];
                uint32_t combined = UINT32_C(0xfca02400) + (scalar << 10) + low;
                if ((low & 0xfc00) == 0xdc00) {
                    scalar = combined;
                    next = index + 2;
                }
            }
            index = next;
            if ((OCFUniCharIsMemberOf(scalar, 0x6d) & 1) == 0) {
                if ((OCFUniCharIsMemberOf(scalar, 7) & 1) != 0) {
                    return 0;
                }
                if ((OCFUniCharIsMemberOf(scalar, 6) & 1) != 0) {
                    return 0;
                }
                return 1;
            }
        }
        return 1;
    }
    if (langCode == 0) {
        return 0;
    }
    uint16_t language;
    memcpy(&language, langCode, sizeof(language));
    bool turkic = false;
    if (language > 0x746b) {
        if (language == 0x7a61) {
            turkic = true;
        } else if (language == 0x746c) {
            if (character == 0x307 && (lastFlags & 6) == 6) {
                if (currentIndex < 1) {
                    return 0;
                }
                OCFIndex position = currentIndex;
                OCFIndex remaining = currentIndex - 1;
                OCFIndex index = currentIndex;
                OCFIndex previous;
                if (remaining == 0) {
                    scalar = buffer[position - 1];
                    goto decompose;
                }
                for (;;) {
                    previous = position - 1;
                    scalar = buffer[previous];
                    if ((scalar & 0xfc00) == 0xdc00) {
                        index -= 2;
                        if (index == 0) {
                            goto combineSurrogates;
                        }
                        position -= 2;
                        uint32_t high = buffer[position];
                        if ((high & 0xfc00) != 0xd800) {
                            goto combineSurrogates;
                        }
                        scalar = scalar + UINT32_C(0xfca02400) + (high << 10);
                    } else {
                        position = previous;
                        index = remaining;
                    }
                    if (OCFUniCharIsMemberOf(scalar, 8) == 0) {
                        remaining = index;
                        break;
                    }
                    if (scalar >> 17 == 0) {
                        const uint8_t *table = __CFUniCharCombiningPriorityTable[scalar >> 16];
                        if (table != 0) {
                            uint8_t page = table[(scalar >> 8) & 0xff];
                            if (page != 0 &&
                                table[((uint32_t)page << 8) - 256 + (scalar & 0xff) + 256] == 230) {
                                return 0;
                            }
                        }
                    }
                    remaining = index - 1;
                    if (index <= 1) {
                        break;
                    }
                }
                previous = position;
                if (remaining == 0) {
                    scalar = buffer[position - 1];
                    goto decompose;
                }
            combineSurrogates:
                if ((scalar & 0xfc00) == 0xdc00) {
                    uint32_t high = buffer[previous - 1];
                    uint32_t combined = (scalar & 0xdfff) + UINT32_C(0xfca02400) + (high << 10);
                    if ((high & 0xfc00) == 0xd800) {
                        scalar = combined;
                    }
                }
            decompose:;
                OCFIndex count = OCFUniCharDecomposeCharacter(scalar, decomposed, 10);
                if (count < 2) {
                    return 2;
                }
                const uint32_t *cursor = decomposed + 1;
                for (OCFIndex left = count - 1; left != 0; --left) {
                    uint32_t mark = *cursor++;
                    uint32_t plane = (mark >> 16) & 0xff;
                    if (plane <= 1) {
                        const uint8_t *table = __CFUniCharCombiningPriorityTable[plane];
                        if (table != 0) {
                            uint8_t page = table[(mark >> 8) & 0xff];
                            if (page != 0 &&
                                table[((uint32_t)page << 8) - 256 + (mark & 0xff) + 256] == 230) {
                                return 0;
                            }
                        }
                    }
                }
                return 2;
            }
            uint32_t found;
            if (type == 0) {
                if (character - 0x49 >= 2 && character != 0x12e) {
                    goto generic;
                }
                found = 4;
            } else {
                if (character - 0x69 > 1) {
                    goto generic;
                }
                found = 6;
            }
            // The native code repeats this scan for each result value; only the returned constant differs.
            OCFIndex position = currentIndex + 1;
            OCFIndex remaining = length - position;
            if (remaining < 1) {
                return 0;
            }
            do {
                OCFIndex next = position + 1;
                scalar = buffer[position];
                OCFIndex left = remaining - 1;
                if (left != 0 && (scalar & 0xfc00) == 0xd800 && left != 1 &&
                    (buffer[position + 2] & 0xfc00) == 0xdc00) {
                    scalar = (scalar << 10) + UINT32_C(0xfca02400) + buffer[next];
                    remaining -= 2;
                    position += 2;
                } else {
                    position = next;
                    remaining = left;
                }
                if (OCFUniCharIsMemberOf(scalar, 8) == 0) {
                    return 0;
                }
                uint32_t plane = (scalar >> 16) & 0xff;
                if (plane <= 1) {
                    const uint8_t *table = __CFUniCharCombiningPriorityTable[plane];
                    if (table != 0) {
                        uint8_t page = table[(scalar >> 8) & 0xff];
                        if (page != 0 && table[((uint32_t)page << 8) - 256 + (scalar & 0xff) + 256] == 230) {
                            return found;
                        }
                    }
                }
            } while (remaining > 0);
            return 0;
        }
    } else if (language == 0x6c6e) {
        if ((lastFlags & 8) != 0) {
            return (character & UINT32_C(0xffffffdf)) == 0x4a ? 8 : 0;
        }
        if ((character & UINT32_C(0xffffffdf)) == 0x49 && type == 2) {
            OCFIndex next = currentIndex + 1;
            if (next >= length) {
                return 0;
            }
            uint32_t unit = buffer[next];
            uint32_t result = unit == 0x4a ? 8 : 0;
            return unit == 0x6a ? 8 : result;
        }
    } else if (language == 0x7274) {
        turkic = true;
    }
    if (turkic && type == 0) {
        if (character == 0x49) {
            OCFIndex next = currentIndex + 1;
            if (next >= length) {
                return 0;
            }
            return buffer[next] == 0x307 ? 4 : 0;
        }
        if (character == 0x307) {
            return (lastFlags >> 1) & 2;
        }
    }
generic:
    if ((lastFlags & 0x10) != 0 && (OCFUniCharIsMemberOf(character, 8) & 1) != 0) {
        return 0x10;
    }
    if (character - 0x370 >= 0x90 && (character >> 8) != 0x1f) {
        return 0;
    }
    if (type != 1) {
        return 0;
    }
    if ((OCFUniCharIsMemberOf(character, 5) & 1) == 0) {
        return 0;
    }
    return 0x10;
}

// Native __CFUniCharRecursivelyDecomposeCharacter, iOS Simulator 24A434, 0x1c02529c8,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1806fc864, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; recursive lookup/capacity/write behavior agrees.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
static OCFIndex
__CFUniCharRecursivelyDecomposeCharacter(uint32_t character, uint32_t *converted, OCFIndex capacity) {
    uint32_t mapping = 0;
    if (character - 195102 >= UINT32_C(0xfffd06a2)) {
        const uint32_t *low = __CFUniCharDecompositionTable;
        const uint32_t *high = __CFUniCharDecompositionTable + 4156;
        while (low <= high) {
            const uint32_t *middle = low + 2 * ((high - low) / 4);
            if (character < middle[0]) {
                high = middle - 2;
            } else if (character > middle[0]) {
                low = middle + 2;
            } else {
                mapping = middle[1];
                break;
            }
        }
    }
    OCFIndex length = (mapping >> 24) & 31;
    uint32_t single = mapping & UINT32_C(0xffffff);
    const uint32_t *multiple = (const uint32_t *)((uintptr_t)__CFUniCharMultipleDecompositionTable +
                                                  (uintptr_t)single * sizeof(uint32_t));
    const uint32_t *characters = &single;
    if ((mapping & UINT32_C(0x1e000000)) != 0) {
        characters = multiple;
    }
    OCFIndex available = (OCFIndex)((uint64_t)capacity - (uint64_t)length);
    if (capacity < length) {
        return 0;
    }
    OCFIndex recursiveLength = 0;
    if ((mapping & UINT32_C(0x40000000)) != 0) {
        recursiveLength = __CFUniCharRecursivelyDecomposeCharacter(characters[0], converted, available);
        if (recursiveLength == 0 || recursiveLength + length - 1 > capacity) {
            return 0;
        }
        characters = (length > 1 ? multiple : &single) + 1;
        converted += recursiveLength;
        --length;
    }
    OCFIndex result = recursiveLength + length;
    while (length != 0) {
        *converted++ = *characters++;
        --length;
    }
    return result;
}

// Native CFUniCharDecomposeCharacter, iOS Simulator 24A434, 0x1c0252910,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device divergence: CFUniCharDecomposeCharacter at 0x1806fc7b4 outlines the Hangul
// body into 0x1807c7d04; UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// The inclusive upper comparison and capacity behavior agree.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
OCFIndex OCFUniCharDecomposeCharacter(uint32_t character, uint32_t *converted, OCFIndex capacity) {
    uint32_t offset = character - 44032;
    if (offset > 0x2ba4) {
        return __CFUniCharRecursivelyDecomposeCharacter(character, converted, capacity);
    }
    uint32_t trailing = (uint16_t)offset % 28;
    OCFIndex length = trailing == 0 ? 2 : 3;
    if (capacity < length) {
        return 0;
    }
    converted[0] = 0x1100 + offset / 588;
    converted[1] = 0x1161 + (offset % 588) / 28;
    if (trailing != 0) {
        converted[2] = 4519 + trailing;
    }
    return length;
}

// Native CFUniCharFillDestinationBuffer, iOS Simulator 24A434, 0x1c0252754,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18077483c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; all format/capacity branches agree.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
uint8_t OCFUniCharFillDestinationBuffer(
    const uint32_t *characters,
    OCFIndex length,
    void **destination,
    OCFIndex capacity,
    OCFIndex *filledLength,
    uint32_t format
) {
    // Native function-local firstByteMark: 0x1c03a46c8 / iOS Device 0x1808a0728,
    // same image UUIDs as this function; confidence: verified; all seven bytes agree.
    static const uint8_t firstByteMark[7] = {0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc};
    OCFIndex filled = *filledLength;
    uintptr_t cursor = (uintptr_t)*destination;
    for (OCFIndex index = 0; index < length; ++index) {
        uint32_t character = characters[index];
        if (format == 0) {
            OCFIndex next = (OCFIndex)((uint64_t)filled + (character < 0x10000 ? 1 : 2));
            if (capacity != 0) {
                if ((character < 0x10000 && filled >= capacity) ||
                    (character >= 0x10000 && next > capacity)) {
                    return 0;
                }
                uint16_t first = (uint16_t)character;
                if (character >= 0x10000) {
                    first = (uint16_t)(((character + UINT32_C(0x03ff0000)) >> 10) - 10240);
                }
                *(uint8_t *)cursor = (uint8_t)first;
                *(uint8_t *)(cursor + 1) = (uint8_t)(first >> 8);
                cursor += 2;
                if (character >= 0x10000) {
                    uint16_t last = (character & 1023) | 0xdc00;
                    *(uint8_t *)cursor = (uint8_t)last;
                    *(uint8_t *)(cursor + 1) = (uint8_t)(last >> 8);
                    cursor += 2;
                }
            }
            filled = next;
        } else if (format != 2) {
            if (capacity != 0) {
                if (filled >= capacity) {
                    return 0;
                }
                for (uint32_t byte = 0; byte < 4; ++byte) {
                    *(uint8_t *)(cursor + byte) = (uint8_t)(character >> (byte * 8));
                }
                cursor += 4;
            }
            filled = (OCFIndex)((uint64_t)filled + 1);
        } else {
            uint32_t scalar = character >= 0x200000 ? 65533 : character;
            uint32_t width = character >= 0x200000 ? 2 : 4;
            if (character < 0x10000) {
                width = 3;
            }
            if (character < 0x800) {
                scalar = character;
                width = 2;
            }
            if (character < 0x80) {
                scalar = character;
                width = 1;
            }
            filled = (OCFIndex)((uint64_t)filled + width);
            if (capacity != 0) {
                if (filled > capacity) {
                    return 0;
                }
                uintptr_t end = cursor + width;
                for (uint32_t byte = width; byte > 1; --byte) {
                    *(uint8_t *)--end = (scalar & 63) | 0x80;
                    scalar >>= 6;
                }
                *(uint8_t *)(end - 1) = firstByteMark[width] | scalar;
                cursor = end - 1 + width;
            }
        }
    }
    *destination = (void *)cursor;
    *filledLength = filled;
    return 1;
}

// Native CFUniCharGetUnicodePropertyDataForPlane, iOS Simulator 24A434, 0x1c025268c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18065673c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions.
// The property type indexes __CFUniCharUnicodePropertyTable without a check, as in the native code;
// types 0 (combining priority) and 1 (bidi) are the two read records.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
const uint8_t *OCFUniCharGetUnicodePropertyDataForPlane(uint32_t propertyType, uint32_t plane) {
    const UniCharBitmapData *entry = &__CFUniCharUnicodePropertyTable[propertyType];
    if (plane < entry->planeCount) {
        return entry->planes[plane];
    }
    return 0;
}

// Native __CFUniCharRecursivelyCompatibilityDecomposeCharacter, iOS Simulator 24A434, 0x1c0253188,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x180700d14, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// block order differs, the range test, search, capacities, calls and writes agree.
// The range test accepts U+00A0..U+1FBF9 by one wrapping subtraction. The output buffer is not bounded:
// compatibility expansions write without a capacity, canonical expansions receive 33 - count plus the
// element index, as in the native code. A canonical expansion that returns zero drops its element.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
static OCFIndex
__CFUniCharRecursivelyCompatibilityDecomposeCharacter(uint32_t character, uint32_t *converted) {
    if (character - 0x1fbfa < UINT32_C(0xfffe04a6)) {
        return 0;
    }
    const uint32_t *low = __CFUniCharCompatibilityDecompositionTable;
    const uint32_t *high = __CFUniCharCompatibilityDecompositionTable + 7664;
    uint32_t value;
    for (;;) {
        const uint32_t *middle = low + 2 * ((high - low) / 4);
        if (character < middle[0]) {
            high = middle - 2;
        } else if (character <= middle[0]) {
            value = middle[1];
            break;
        } else {
            low = middle + 2;
        }
        if (low > high) {
            return 0;
        }
    }
    OCFIndex length = (value >> 24) & 0x1f;
    uint32_t single = value & UINT32_C(0xffffff);
    if (length == 0) {
        return 0;
    }
    const uint32_t *characters =
        length == 1 ? &single : __CFUniCharCompatibilityMultipleDecompositionTable + single;
    uint64_t remaining = (uint64_t)length + 1;
    OCFIndex capacity = 33 - length;
    do {
        uint32_t scalar = *characters++;
        const uint8_t *bitmap = __CFUniCharDecompositionCanonicalDecomposableCharacterSetBitmapPlane0;
        OCFIndex filled;
        if (scalar >= 0x10000) {
            bitmap = OCFUniCharGetBitmapPtrForPlane(9, (scalar >> 16) & 0xff);
        }
        if (bitmap != 0 && ((bitmap[(scalar >> 3) & 0x1fff] >> (scalar & 7)) & 1) != 0) {
            filled = __CFUniCharRecursivelyDecomposeCharacter(scalar, converted, capacity);
        } else if (OCFUniCharIsMemberOf(scalar, 100) == 0) {
            *converted++ = scalar;
            goto next;
        } else {
            filled = __CFUniCharRecursivelyCompatibilityDecomposeCharacter(scalar, converted);
        }
        converted += filled;
        length = length + filled - 1;
    next:
        --remaining;
        ++capacity;
    } while (remaining > 1);
    return length;
}

// Native CFUniCharCompatibilityDecompose, iOS Simulator 24A434, 0x1c0253068,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Exported.
// iOS Device 24A446: 0x180700fac, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// the tail move counts down a byte distance instead of comparing pointers; the moved range, the capacity
// test and the copies agree.
// The scan limit stays at the original end: characters moved past it by an expansion are not examined,
// and a later expansion moves only the elements before that limit, as in the native code. An expansion of
// length zero would leave the cursor in place; every member of compatibility set 100 in the pinned tables
// has a nonzero entry. The 32-element stack buffer is not bounded by the helper, as in the native code.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
OCFIndex
OCFUniCharCompatibilityDecompose(uint32_t *convertedChars, OCFIndex length, OCFIndex maxBufferLength) {
    uint32_t buffer[32];
    if (length < 1) {
        return length;
    }
    uint32_t *cursor = convertedChars;
    uint32_t *limit = convertedChars + length;
    do {
        uint32_t character = *cursor;
        if (OCFUniCharIsMemberOf(character, 100) == 0) {
            ++cursor;
            continue;
        }
        OCFIndex filled = __CFUniCharRecursivelyCompatibilityDecomposeCharacter(character, buffer);
        length = length + filled - 1;
        if (length > maxBufferLength) {
            return 0;
        }
        if (filled >= 2) {
            if (limit > cursor + 1) {
                uint32_t *source = limit;
                do {
                    --source;
                    source[filled - 1] = *source;
                } while (source > cursor + 1);
            }
        } else if (filled != 1) {
            continue;
        }
        const uint32_t *expansion = buffer;
        for (uint64_t count = (uint64_t)filled + 1; count > 1; --count) {
            *cursor++ = *expansion++;
        }
    } while (cursor < limit);
    return length;
}

// Native __CFUniCharPrioritySort, iOS Simulator 24A434, 0x1c0253324,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1806fcf40, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// register allocation differs, the comparisons, swaps and table reads agree.
// It skips characters whose combining priority is zero, then bubble-sorts the remainder by priority.
// The previous priority always becomes the current element's, also after a swap, as in the native
// code. No native caller reads the returned register, so the restoration returns void.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
static void __CFUniCharPrioritySort(uint32_t *characters, OCFIndex length) {
    uint32_t *end = characters + length;
    if (length >= 1) {
        do {
            uint32_t character = *characters;
            if (character >> 17 == 0) {
                const uint8_t *table = __CFUniCharDecompositionCombiningPriorityTable[character >> 16];
                uint8_t page = table[(character >> 8) & 0xff];
                if (page != 0 && table[((uint32_t)page << 8) - 256 + (character & 0xff) + 256] != 0) {
                    break;
                }
            }
            ++characters;
        } while (characters < end);
    }
    if ((intptr_t)((uintptr_t)end - (uintptr_t)characters) < 5) {
        return;
    }
    bool swapped;
    do {
        uint32_t character = *characters;
        uint32_t previousPriority = 0;
        if (character >> 17 == 0) {
            const uint8_t *table = __CFUniCharDecompositionCombiningPriorityTable[character >> 16];
            uint8_t page = table[(character >> 8) & 0xff];
            if (page != 0) {
                previousPriority = table[((uint32_t)page << 8) - 256 + (character & 0xff) + 256];
            }
        }
        if (characters + 1 >= end) {
            return;
        }
        swapped = false;
        for (uint32_t *cursor = characters + 1; cursor < end; ++cursor) {
            uint32_t current = *cursor;
            uint32_t priority = 0;
            if (current >> 17 == 0) {
                const uint8_t *table = __CFUniCharDecompositionCombiningPriorityTable[current >> 16];
                uint8_t page = table[(current >> 8) & 0xff];
                if (page != 0) {
                    priority = table[((uint32_t)page << 8) - 256 + (current & 0xff) + 256];
                }
            }
            if (previousPriority > priority) {
                uint32_t moved = cursor[-1];
                cursor[-1] = current;
                *cursor = moved;
                swapped = true;
            }
            previousPriority = priority;
        }
    } while (swapped);
}

// Native CFUniCharPrioritySort, iOS Simulator 24A434, 0x1c0253320,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. A tail call of __CFUniCharPrioritySort.
// iOS Device 24A446: 0x1806fcc28, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
void OCFUniCharPrioritySort(uint32_t *characters, OCFIndex length) {
    __CFUniCharPrioritySort(characters, length);
}
