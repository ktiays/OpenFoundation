//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Base/OCFHashBytes.h"

// Native CFHashBytes: 0x1c02561ec, iOS Simulator 27.0 (24A434), arm64.
// CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device divergence: CFHashBytes at 0x180665d1c, iOS 27.0.1 (24A446), arm64e,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B, computes block addresses from length
// and remainder rather than advancing a cursor. Byte order and scalar results agree;
// confidence: verified. Reading: Documentation/Reference/Research/CoreFoundationBase.md.
OCFHashCode OCFHashBytes(const uint8_t *bytes, OCFIndex length) {
    uint32_t hash = 0;
    uint64_t remaining = (uint64_t)length;
    uint32_t highBits;

    if ((int32_t)length >= 4) {
        remaining = (uint64_t)length & UINT64_C(0x7fffffff);
        const uint8_t *cursor =
            (const uint8_t *)((uintptr_t)bytes + ((uint64_t)length & UINT64_C(0xffffffff80000000)));
        do {
            hash = cursor[0] + (hash << 4);
            hash ^= (hash >> 28) << 4;
            hash = cursor[1] + (hash << 4);
            hash ^= (hash >> 28) << 4;
            hash = cursor[2] + (hash << 4);
            hash ^= (hash >> 28) << 4;
            hash = cursor[3] + (hash << 4);
            highBits = hash & UINT32_C(0xf0000000);
            hash ^= highBits >> 24;
            hash &= ~highBits;
            cursor += 4;
            remaining -= 4;
        } while (remaining > 3);
    }

    uintptr_t endAddress = (uintptr_t)bytes + (uint64_t)length;
    switch ((uint32_t)remaining) {
        case 3:
            hash = *(const uint8_t *)(endAddress - 3) + (hash << 4);
            highBits = hash & UINT32_C(0xf0000000);
            hash ^= highBits >> 24;
            hash &= ~highBits;
            __attribute__((fallthrough));
        case 2:
            hash = *(const uint8_t *)(endAddress - 2) + (hash << 4);
            highBits = hash & UINT32_C(0xf0000000);
            hash ^= highBits >> 24;
            hash &= ~highBits;
            __attribute__((fallthrough));
        case 1:
            hash = *(const uint8_t *)(endAddress - 1) + (hash << 4);
            highBits = hash & UINT32_C(0xf0000000);
            hash ^= highBits >> 24;
            hash &= ~highBits;
            break;
        default:
            break;
    }
    return hash;
}
