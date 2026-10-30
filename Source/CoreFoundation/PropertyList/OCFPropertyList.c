//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PropertyList/OCFPropertyList.h"

#include <string.h>

// Native __CFPropertyListKeyBufferMatchesFilterBuffer, iOS Simulator 24A434, 0x1c01d446c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external (was a private
// external); its only caller is ____CFPropertyListTryFilteredDictionaryASCIIFastPath_block_invoke.
// iOS Device 24A446: 0x18068f8c4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// reorders the blocks and authenticates its return; the comparisons and memcmp calls agree.
// A one-byte filter "*" matches every key; any other one-byte filter matches only the equal one-byte
// key. A longer filter ending in '*' matches a key of at least filterLength - 1 bytes (signed) with
// that prefix. Otherwise the lengths must be equal and memcmp decides; a zero or negative filter
// length reaches memcmp only when the key length is equal, and a negative one is passed to memcmp as a
// huge size (native behavior; no native caller passes one).
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool __CFPropertyListKeyBufferMatchesFilterBuffer(
    const char *key, OCFIndex keyLength, const char *filter, OCFIndex filterLength
) {
    if (filterLength == 1) {
        if (filter[0] == '*') {
            return true;
        }
        return keyLength == 1 && (uint8_t)filter[0] == (uint8_t)key[0];
    }
    if (filterLength >= 2 && filter[filterLength - 1] == '*') {
        if (keyLength >= filterLength - 1) {
            return memcmp(key, filter, (size_t)(filterLength - 1)) == 0;
        }
        return false;
    }
    if (keyLength == filterLength) {
        return memcmp(key, filter, (size_t)filterLength) == 0;
    }
    return false;
}

// Native lineNumber, iOS Simulator 24A434, 0x1c01d84dc, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; callers pass the context to error-message formats with %u.
// iOS Device 24A446: 0x1806f6734, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions.
// Counts lines from begin up to (not including) curr, never reading at or beyond end: LF and CR each
// start a line and a CR followed by LF inside both bounds counts once. The count starts at 1 and is
// computed in 32 bits.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
uint32_t lineNumber(const XMLPlistParseContext *context) {
    const uint8_t *p = context->begin;
    const uint8_t *end = context->end;
    uint32_t count = 1;
    while (p < end && p < context->curr) {
        if (*p == '\n') {
            ++count;
        } else if (*p == '\r') {
            ++count;
            if (p + 1 < end && p + 1 < context->curr && p[1] == '\n') {
                ++p;
            }
        }
        ++p;
    }
    return count;
}
