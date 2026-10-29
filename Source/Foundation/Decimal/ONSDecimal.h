//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenFoundation/Base/ONSObjCRuntime.h>

#pragma clang assume_nonnull begin

typedef enum __attribute__((enum_extensibility(open))) ONSRoundingMode : ONSUInteger ONSRoundingMode;

enum ONSRoundingMode : ONSUInteger {
    ONSRoundPlain = 0,
    ONSRoundDown = 1,
    ONSRoundUp = 2,
    ONSRoundBankers = 3,
};

typedef enum __attribute__((enum_extensibility(open))) ONSCalculationError : ONSUInteger ONSCalculationError;

enum ONSCalculationError : ONSUInteger {
    ONSCalculationNoError = 0,
    ONSCalculationLossOfPrecision = 1,
    ONSCalculationUnderflow = 2,
    ONSCalculationOverflow = 3,
    ONSCalculationDivideByZero = 4,
};

typedef struct __attribute__((swift_attr("@Sendable"))) {
    signed int _exponent : 8;
    unsigned int _length : 4;
    unsigned int _isNegative : 1;
    unsigned int _isCompact : 1;
    unsigned int _reserved : 18;
    unsigned short _mantissa[8];
} ONSDecimal;

OF_EXPORT void ONSDecimalCopy(ONSDecimal *destination, const ONSDecimal *source);

OF_EXPORT void ONSDecimalCompact(ONSDecimal *number);

OF_EXPORT ONSComparisonResult
ONSDecimalCompare(const ONSDecimal *leftOperand, const ONSDecimal *rightOperand);

OF_EXPORT void
ONSDecimalRound(ONSDecimal *result, const ONSDecimal *number, ONSInteger scale, ONSRoundingMode roundingMode);

OF_EXPORT ONSCalculationError
ONSDecimalNormalize(ONSDecimal *number1, ONSDecimal *number2, ONSRoundingMode roundingMode);

OF_EXPORT ONSCalculationError ONSDecimalAdd(
    ONSDecimal *result,
    const ONSDecimal *leftOperand,
    const ONSDecimal *rightOperand,
    ONSRoundingMode roundingMode
);

OF_EXPORT ONSCalculationError ONSDecimalSubtract(
    ONSDecimal *result,
    const ONSDecimal *leftOperand,
    const ONSDecimal *rightOperand,
    ONSRoundingMode roundingMode
);

OF_EXPORT ONSCalculationError ONSDecimalMultiply(
    ONSDecimal *result,
    const ONSDecimal *leftOperand,
    const ONSDecimal *rightOperand,
    ONSRoundingMode roundingMode
);

OF_EXPORT ONSCalculationError ONSDecimalDivide(
    ONSDecimal *result,
    const ONSDecimal *leftOperand,
    const ONSDecimal *rightOperand,
    ONSRoundingMode roundingMode
);

OF_EXPORT ONSCalculationError ONSDecimalPower(
    ONSDecimal *result, const ONSDecimal *number, ONSUInteger power, ONSRoundingMode roundingMode
);

OF_EXPORT ONSCalculationError ONSDecimalMultiplyByPowerOf10(
    ONSDecimal *result, const ONSDecimal *number, short power, ONSRoundingMode roundingMode
);

#pragma clang assume_nonnull end
