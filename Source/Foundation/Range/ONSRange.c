//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "Foundation/Range/ONSRange.h"

#include <stddef.h>

_Static_assert(sizeof(ONSUInteger) == 8, "The restored unsigned range field must be 64 bits.");
_Static_assert(sizeof(ONSRange) == 16, "The restored range must occupy two 64-bit fields.");
_Static_assert(_Alignof(ONSRange) == 8, "The restored range must have 64-bit field alignment.");
_Static_assert(offsetof(ONSRange, location) == 0, "The restored range location must precede length.");
_Static_assert(offsetof(ONSRange, length) == 8, "The restored range length must be the second field.");

// Native: NSUnionRange, iOS Simulator 27.0 (24A434), arm64, 0x1c05bd644.
// Foundation UUID: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
// Reading: Documentation/Reference/Research/FoundationRange.md.
// iOS Device: NSUnionRange, iOS 27.0.1 (24A446), arm64e, 0x180f67520.
// Foundation UUID: C041F1D0-5EF1-30A3-BF96-830976173B56. Confidence: verified; body bytes identical.
ONSRange ONSUnionRange(ONSRange range1, ONSRange range2) {
    ONSUInteger range1End = range1.location + range1.length;
    ONSUInteger range2End = range2.location + range2.length;
    ONSUInteger end = range1End;
    if (range1End <= range2End) {
        end = range2End;
    }
    if (range1.location >= range2.location) {
        range1.location = range2.location;
    }
    range1.length = end - range1.location;
    return range1;
}

// Native: NSIntersectionRange, iOS Simulator 27.0 (24A434), arm64, 0x1c05bd664.
// Foundation UUID: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
// Reading: Documentation/Reference/Research/FoundationRange.md.
// iOS Device: NSIntersectionRange, iOS 27.0.1 (24A446), arm64e, 0x180e02318.
// Foundation UUID: C041F1D0-5EF1-30A3-BF96-830976173B56. Confidence: verified; body bytes identical.
ONSRange ONSIntersectionRange(ONSRange range1, ONSRange range2) {
    ONSUInteger range1End = range1.location + range1.length;
    ONSUInteger range2End = range2.location + range2.length;
    ONSUInteger end = range1End;
    if (range1End >= range2End) {
        end = range2End;
    }
    if (range2.location > range1.location || range1.location >= range2End) {
        ONSUInteger length = end - range2.location;
        ONSUInteger location = range2.location;
        if (range2.location >= range1End) {
            location = 0;
            length = 0;
        }
        if (range1.location > range2.location) {
            range1.location = 0;
            range1.length = 0;
        } else {
            range1.location = location;
            range1.length = length;
        }
    } else {
        range1.length = end - range1.location;
    }
    return range1;
}
