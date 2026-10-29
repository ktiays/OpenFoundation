//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFUTF8.h"

// Native firstByteMark, iOS Simulator 24A434, 0x1c0373034,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x18082f290, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; all seven bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const uint8_t firstByteMark[7] = {0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc};

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
