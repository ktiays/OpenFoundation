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
