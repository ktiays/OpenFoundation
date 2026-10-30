//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PropertyList/OCFOldStylePList.h"

// Native advanceToNonSpace, iOS Simulator 24A434, 0x1c02d4e94, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; callers test bit 0 or the whole of W0.
// iOS Device 24A446: 0x180702bac, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// reorders the comment blocks; the comparisons, bounds and stores agree.
// Skips U+0009..U+000D, U+2028 and U+2029 (SUB #9 / CMP #5, ADD #-0x2028 / CCMP #2), the space U+0020,
// "//" comments up to (not past) LF, CR, U+2028 or U+2029, and "/*" comments
// past the closing "*/". Returns true with curr at the first other unit, including a final lone '/'.
// Returns false with curr at the stop position when the end is reached; when curr is already at or
// beyond end it returns false without a store. An unterminated "/*" stops at end.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool advanceToNonSpace(OldStylePlistParseContext *context) {
    const uint16_t *p = context->curr;
    const uint16_t *end = context->end;
    if (p >= end) {
        return false;
    }
    do {
        uint16_t character = *p;
        if ((unsigned)(character - 9) < 5 || (unsigned)(character - 0x2028) < 2 || character == ' ') {
            ++p;
            continue;
        }
        if (character != '/' || p + 1 >= end) {
            context->curr = p;
            return true;
        }
        if (p[1] == '*') {
            const uint16_t *scan = p + 2;
            p = scan;
            if (scan >= end) {
                continue;
            }
            const uint16_t *next = scan;
            do {
                uint16_t commentCharacter = *next++;
                if (commentCharacter == '*' && next < end && *next == '/') {
                    p = scan + 2;
                    break;
                }
                scan = next;
                p = next;
            } while (next < end);
        } else if (p[1] == '/') {
            p += 2;
            while (p < end && !((unsigned)(*p - 0x2028) < 2 || *p == '\n' || *p == '\r')) {
                ++p;
            }
        } else {
            context->curr = p;
            return true;
        }
    } while (p < end);
    context->curr = p;
    return false;
}

// Native lineNumberStrings, iOS Simulator 24A434, 0x1c02d54e0, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; callers pass the context to error-message formats with %d.
// iOS Device 24A446: 0x18070d188, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// merges the two CR-LF bound tests into one CCMP; the results agree.
// The UTF-16 form of lineNumber: counts lines from begin up to (not including) curr within end; LF and
// CR each start a line and CR followed by LF inside both bounds counts once. The count starts at 1 and is
// computed in 32 bits.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
uint32_t lineNumberStrings(const OldStylePlistParseContext *context) {
    const uint16_t *p = context->begin;
    const uint16_t *end = context->end;
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
