//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "Foundation/Decimal/ONSDecimal.h"

OF_EXTERN __attribute__((visibility("hidden"))) void _ONSDecimalCompact(ONSDecimal *number);

OF_EXTERN __attribute__((visibility("hidden"))) ONSComparisonResult
_ONSDecimalCompare(const ONSDecimal *leftOperand, const ONSDecimal *rightOperand);

OF_EXTERN __attribute__((visibility("hidden"))) void _ONSDecimalRound(
    ONSDecimal *result, const ONSDecimal *number, ONSInteger scale, ONSRoundingMode roundingMode
);

OF_EXTERN __attribute__((visibility("hidden"))) ONSCalculationError
_ONSDecimalNormalize(ONSDecimal *number1, ONSDecimal *number2, ONSRoundingMode roundingMode);

OF_EXTERN __attribute__((visibility("hidden"))) ONSCalculationError _ONSDecimalAdd(
    ONSDecimal *result,
    const ONSDecimal *leftOperand,
    const ONSDecimal *rightOperand,
    ONSRoundingMode roundingMode
);

OF_EXTERN __attribute__((visibility("hidden"))) ONSCalculationError _ONSDecimalSubtract(
    ONSDecimal *result,
    const ONSDecimal *leftOperand,
    const ONSDecimal *rightOperand,
    ONSRoundingMode roundingMode
);

OF_EXTERN __attribute__((visibility("hidden"))) ONSCalculationError _ONSDecimalMultiply(
    ONSDecimal *result,
    const ONSDecimal *leftOperand,
    const ONSDecimal *rightOperand,
    ONSRoundingMode roundingMode
);

OF_EXTERN __attribute__((visibility("hidden"))) ONSCalculationError _ONSDecimalDivide(
    ONSDecimal *result,
    const ONSDecimal *leftOperand,
    const ONSDecimal *rightOperand,
    ONSRoundingMode roundingMode
);

OF_EXTERN __attribute__((visibility("hidden"))) ONSCalculationError _ONSDecimalPower(
    ONSDecimal *result, const ONSDecimal *number, ONSUInteger power, ONSRoundingMode roundingMode
);

OF_EXTERN __attribute__((visibility("hidden"))) ONSCalculationError _ONSDecimalMultiplyByPowerOf10(
    ONSDecimal *result, const ONSDecimal *number, short power, ONSRoundingMode roundingMode
);
