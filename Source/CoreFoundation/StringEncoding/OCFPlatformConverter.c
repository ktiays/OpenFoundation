//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFPlatformConverter.h"

// Inferred organization: the platform converter functions (0x1c01cecc4-0x1c01cece4) follow
// __CFStringEncodingGetExternalConverter (0x1c01cebc8) in the iOS Simulator __text section. The
// native source path is not in the evidence. __CFStringEncodingPlatformByteLengthForCharacters
// (0x1c01cecdc) is outside this group and is not restored.
//
// None of the three bodies reads an argument. The parameter lists follow the argument registers
// their only callers set up (CFStringEncodingUnicodeToBytes passes X0-X7,
// CFStringEncodingBytesToUnicode passes X0-X7, CFStringEncodingCharLengthForBytes tail-calls with its
// own arguments); their names and types beyond the register count are descriptive, not read.

// Native __CFStringEncodingPlatformUnicodeToBytes, iOS Simulator 24A434, 0x1c01cecc4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (body); parameter types inferred. Hidden
// (was private external). iOS Device 24A446: 0x18074ef20, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; identical instructions. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFStringEncodingPlatformUnicodeToBytes(
    uint32_t encoding,
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex numChars,
    OCFIndex *usedCharLen,
    uint8_t *bytes,
    OCFIndex maxByteLen,
    OCFIndex *usedByteLen
) {
    (void)encoding;
    (void)flags;
    (void)characters;
    (void)numChars;
    (void)usedCharLen;
    (void)bytes;
    (void)maxByteLen;
    (void)usedByteLen;
    return 3;
}

// Native __CFStringEncodingPlatformBytesToUnicode, iOS Simulator 24A434, 0x1c01ceccc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (body); parameter types inferred. Hidden
// (was private external). iOS Device 24A446: 0x18074ef28, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; identical instructions. Reading:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.
uint32_t __CFStringEncodingPlatformBytesToUnicode(
    uint32_t encoding,
    uint32_t flags,
    const uint8_t *bytes,
    OCFIndex numBytes,
    OCFIndex *usedByteLen,
    uint16_t *characters,
    OCFIndex maxCharLen,
    OCFIndex *usedCharLen
) {
    (void)encoding;
    (void)flags;
    (void)bytes;
    (void)numBytes;
    (void)usedByteLen;
    (void)characters;
    (void)maxCharLen;
    (void)usedCharLen;
    return 3;
}

// Native __CFStringEncodingPlatformCharLengthForBytes, iOS Simulator 24A434, 0x1c01cecd4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (body); parameter types inferred. Hidden
// (was private external). iOS Device 24A446: 0x18074ef30, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; identical instructions. The body clears all of X0 (MOV X0, #0), so the result is a
// 64-bit length. Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
OCFIndex __CFStringEncodingPlatformCharLengthForBytes(
    uint32_t encoding, uint32_t flags, const uint8_t *bytes, OCFIndex numBytes
) {
    (void)encoding;
    (void)flags;
    (void)bytes;
    (void)numBytes;
    return 0;
}
