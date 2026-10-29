//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Unicode/OCFUniChar.h"
#include "CoreFoundation/Unicode/OCFUniCharData.h"

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
