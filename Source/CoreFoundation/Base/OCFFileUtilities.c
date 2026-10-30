//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Base/OCFFileUtilities.h"

// Native _CFGetSlash, iOS Simulator 24A434, 0x1c019cefc, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external (was a private external).
// iOS Device 24A446: 0x18066f0b8, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions (MOV W0, #0x2F; RET).
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
uint16_t _CFGetSlash(void) {
    return '/';
}

// Native _CFStartOfLastPathComponent, iOS Simulator 24A434, 0x1c019d374,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external (was a private
// external).
// iOS Device divergence: _CFStartOfLastPathComponent at 0x180698360 (UUID
// 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified) moves the no-slash drive test into
// _CFStartOfLastPathComponent.cold.1 (0x1807bd49c, 44 bytes; confidence: verified): it sets W8 to 1 when
// unichars[1] is not ':', otherwise to 0 for (unichars[0] & ~0x20) - 'A' <= 25 and to all ones (CSETM)
// for any other unit, then branches to the shared _OUTLINED_FUNCTION_0_19 (0x18065c694: AND W0, W8, #1;
// RET; confidence: verified); the caller
// returns 0 when bit 0 of W0 is set and 2 otherwise. The selected units equal the simulator's.
// Lengths below 2 give 0. The scan from index length - 2 down to 0 returns one past the last '/'
// (a trailing '/' at length - 1 is ignored). Without one, a length above 2 with "X:" in front gives 2;
// otherwise 0. X is an ASCII letter: the native test masks bit 5 and compares with 'A'..'Z', which
// selects the same units as the two ranges below.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
OCFIndex _CFStartOfLastPathComponent(const uint16_t *unichars, OCFIndex length) {
    if (length < 2) {
        return 0;
    }
    for (OCFIndex index = length - 2; index >= 0; --index) {
        if (unichars[index] == '/') {
            return index + 1;
        }
    }
    if (length != 2 && unichars[1] == ':' &&
        ((unichars[0] >= 'A' && unichars[0] <= 'Z') || (unichars[0] >= 'a' && unichars[0] <= 'z'))) {
        return 2;
    }
    return 0;
}

// Native _CFLengthAfterDeletingLastPathComponent, iOS Simulator 24A434, 0x1c019d458,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external (was a private
// external).
// iOS Device divergence: _CFLengthAfterDeletingLastPathComponent at 0x18069ab0c (UUID
// 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified) moves the no-slash drive test into
// _CFLengthAfterDeletingLastPathComponent.cold.1 (0x1807bd4c8, 44 bytes; confidence: verified), the same
// instructions as _CFStartOfLastPathComponent.cold.1 ending in a branch to _OUTLINED_FUNCTION_0_19
// (AND W0, W8, #1; confidence: verified); the caller returns 0 when bit 0 of W0 is set and 2 otherwise. The
// scan, the slash-at-2 drive test and their results agree. Lengths below 2 give 0. The last '/' before index
// length - 1 at index i gives i, except that i = 0 gives 1 (the root) and i = 2 after "X:" gives 3. Without
// one, a length above 2 with "X:" in front gives 2; otherwise 0. The drive test after a slash at index 2
// compares the two letter ranges separately; the no-slash test masks bit 5 as in _CFStartOfLastPathComponent.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
OCFIndex _CFLengthAfterDeletingLastPathComponent(const uint16_t *unichars, OCFIndex length) {
    if (length < 2) {
        return 0;
    }
    for (OCFIndex index = length - 2; index >= 0; --index) {
        if (unichars[index] == '/') {
            if (index == 0 || (index == 2 && unichars[1] == ':' &&
                               ((unichars[0] >= 'A' && unichars[0] <= 'Z') ||
                                (unichars[0] >= 'a' && unichars[0] <= 'z')))) {
                return index + 1;
            }
            return index;
        }
    }
    if (length != 2 && unichars[1] == ':' &&
        ((unichars[0] >= 'A' && unichars[0] <= 'Z') || (unichars[0] >= 'a' && unichars[0] <= 'z'))) {
        return 2;
    }
    return 0;
}
