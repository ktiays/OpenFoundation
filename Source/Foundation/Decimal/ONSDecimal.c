//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "Foundation/Decimal/ONSDecimal.h"
#include "Foundation/Decimal/ONSDecimalInternal.h"

#include <stddef.h>

_Static_assert(sizeof(ONSDecimal) == 20, "The native decimal occupies 20 bytes.");
_Static_assert(_Alignof(ONSDecimal) == 4, "The native decimal has 4-byte alignment.");
_Static_assert(offsetof(ONSDecimal, _mantissa) == 4, "The native decimal mantissa starts at byte 4.");

// Native NSDecimalCopy, iOS Simulator 24A434, 0x1c04f90d0,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
// iOS Device NSDecimalCopy, 24A446, 0x180c9dab8,
// Foundation UUID C041F1D0-5EF1-30A3-BF96-830976173B56. Confidence: verified.
// iOS Device divergence: NSDecimalCopy at 0x180c9dab8 uses indexed rather than post-indexed limb copies;
// confidence verified, with the same field-write order and transferred bytes.
// Foundation defect: both native bodies trust the 4-bit length and copy 9-15 words past the 8-word
// mantissa for a malformed decimal. Preserve the header but cap the transfer at 8 words. Native
// destructive malformed-length cases are excluded from differential tests. Reading:
// Documentation/Reference/Research/FoundationDecimal.md.
void ONSDecimalCopy(ONSDecimal *destination, const ONSDecimal *source) {
    destination->_reserved = 0;
    destination->_isNegative = source->_isNegative;
    unsigned int length = source->_length;
    destination->_length = length;
    destination->_exponent = source->_exponent;
    destination->_isCompact = source->_isCompact;
    for (unsigned int index = 0; index < length && index < 8; ++index) {
        destination->_mantissa[index] = source->_mantissa[index];
    }
}

// Native NSDecimalCompact, iOS Simulator 24A434, 0x1c04f9214, Foundation
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified tail branch.
// iOS Device 24A446, 0x180eff4a0, Foundation UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified same C-to-Swift bridge boundary. Reading:
// Documentation/Reference/Research/FoundationDecimal.md.
void ONSDecimalCompact(ONSDecimal *number) {
    _ONSDecimalCompact(number);
}

// Native NSDecimalCompare, iOS Simulator 24A434, 0x1c04f9208, Foundation
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified tail branch.
// iOS Device 24A446, 0x180eff494, Foundation UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified same C-to-Swift bridge boundary. Reading:
// Documentation/Reference/Research/FoundationDecimal.md.
ONSComparisonResult ONSDecimalCompare(const ONSDecimal *leftOperand, const ONSDecimal *rightOperand) {
    return _ONSDecimalCompare(leftOperand, rightOperand);
}

// Native NSDecimalRound, iOS Simulator 24A434, 0x1c04f920c, Foundation
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified tail branch.
// iOS Device 24A446, 0x180eff498, Foundation UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified same C-to-Swift bridge boundary. Reading:
// Documentation/Reference/Research/FoundationDecimal.md.
void ONSDecimalRound(
    ONSDecimal *result, const ONSDecimal *number, ONSInteger scale, ONSRoundingMode roundingMode
) {
    _ONSDecimalRound(result, number, scale, roundingMode);
}

// Native NSDecimalNormalize, iOS Simulator 24A434, 0x1c04f9210, Foundation
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified tail branch.
// iOS Device 24A446, 0x180eff49c, Foundation UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified same C-to-Swift bridge boundary. Reading:
// Documentation/Reference/Research/FoundationDecimal.md.
ONSCalculationError
ONSDecimalNormalize(ONSDecimal *number1, ONSDecimal *number2, ONSRoundingMode roundingMode) {
    return _ONSDecimalNormalize(number1, number2, roundingMode);
}

// Native NSDecimalAdd, iOS Simulator 24A434, 0x1c04f91f0, Foundation
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified tail branch.
// iOS Device 24A446, 0x180eff47c, Foundation UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified same C-to-Swift bridge boundary. Reading:
// Documentation/Reference/Research/FoundationDecimal.md.
ONSCalculationError ONSDecimalAdd(
    ONSDecimal *result,
    const ONSDecimal *leftOperand,
    const ONSDecimal *rightOperand,
    ONSRoundingMode roundingMode
) {
    return _ONSDecimalAdd(result, leftOperand, rightOperand, roundingMode);
}

// Native NSDecimalSubtract, iOS Simulator 24A434, 0x1c04f91f4, Foundation
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified tail branch.
// iOS Device 24A446, 0x180eff480, Foundation UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified same C-to-Swift bridge boundary. Reading:
// Documentation/Reference/Research/FoundationDecimal.md.
ONSCalculationError ONSDecimalSubtract(
    ONSDecimal *result,
    const ONSDecimal *leftOperand,
    const ONSDecimal *rightOperand,
    ONSRoundingMode roundingMode
) {
    return _ONSDecimalSubtract(result, leftOperand, rightOperand, roundingMode);
}

// Native NSDecimalMultiply, iOS Simulator 24A434, 0x1c04f91f8, Foundation
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified tail branch.
// iOS Device 24A446, 0x180eff484, Foundation UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified same C-to-Swift bridge boundary. Reading:
// Documentation/Reference/Research/FoundationDecimal.md.
ONSCalculationError ONSDecimalMultiply(
    ONSDecimal *result,
    const ONSDecimal *leftOperand,
    const ONSDecimal *rightOperand,
    ONSRoundingMode roundingMode
) {
    return _ONSDecimalMultiply(result, leftOperand, rightOperand, roundingMode);
}

// Native NSDecimalDivide, iOS Simulator 24A434, 0x1c04f91fc, Foundation
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified tail branch.
// iOS Device 24A446, 0x180eff488, Foundation UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified same C-to-Swift bridge boundary. Reading:
// Documentation/Reference/Research/FoundationDecimal.md.
ONSCalculationError ONSDecimalDivide(
    ONSDecimal *result,
    const ONSDecimal *leftOperand,
    const ONSDecimal *rightOperand,
    ONSRoundingMode roundingMode
) {
    return _ONSDecimalDivide(result, leftOperand, rightOperand, roundingMode);
}

// Native NSDecimalPower, iOS Simulator 24A434, 0x1c04f9200, Foundation
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified tail branch.
// iOS Device 24A446, 0x180eff48c, Foundation UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified same C-to-Swift bridge boundary. Reading:
// Documentation/Reference/Research/FoundationDecimal.md.
ONSCalculationError ONSDecimalPower(
    ONSDecimal *result, const ONSDecimal *number, ONSUInteger power, ONSRoundingMode roundingMode
) {
    return _ONSDecimalPower(result, number, power, roundingMode);
}

// Native NSDecimalMultiplyByPowerOf10, iOS Simulator 24A434, 0x1c04f9204, Foundation
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified tail branch.
// iOS Device 24A446, 0x180eff490, Foundation UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified same C-to-Swift bridge boundary. Reading:
// Documentation/Reference/Research/FoundationDecimal.md.
ONSCalculationError ONSDecimalMultiplyByPowerOf10(
    ONSDecimal *result, const ONSDecimal *number, short power, ONSRoundingMode roundingMode
) {
    return _ONSDecimalMultiplyByPowerOf10(result, number, power, roundingMode);
}
