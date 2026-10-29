//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "Foundation/Decimal/DecimalDifferentialTests.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <OpenFoundation/Decimal/ONSDecimal.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Native raw NSDecimal group: public Simulator entries 0x1c04f90d0 / 0x1c04f91f0-0x1c04f9214,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; Device public Copy 0x180c9dab8 and
// arithmetic entries 0x180eff47c-0x180eff4a0, UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
// Independent 20-byte values and destination canaries are compared byte for byte. Native known
// destructive cases (length > 8, signed Int.min power/scale) are never invoked.
typedef void (*DecimalCopyFunction)(ONSDecimal *, const ONSDecimal *);
typedef void (*DecimalCompactFunction)(ONSDecimal *);
typedef ONSComparisonResult (*DecimalCompareFunction)(const ONSDecimal *, const ONSDecimal *);
typedef void (*DecimalRoundFunction)(ONSDecimal *, const ONSDecimal *, ONSInteger, ONSRoundingMode);
typedef ONSCalculationError (*DecimalNormalizeFunction)(ONSDecimal *, ONSDecimal *, ONSRoundingMode);
typedef ONSCalculationError (*DecimalBinaryFunction)(
    ONSDecimal *, const ONSDecimal *, const ONSDecimal *, ONSRoundingMode
);
typedef ONSCalculationError (*DecimalPowerFunction)(
    ONSDecimal *, const ONSDecimal *, ONSUInteger, ONSRoundingMode
);
typedef ONSCalculationError (*DecimalPowerTenFunction)(
    ONSDecimal *, const ONSDecimal *, short, ONSRoundingMode
);

typedef struct DecimalFunctions {
    DecimalCopyFunction copy;
    DecimalCompactFunction compact;
    DecimalCompareFunction compare;
    DecimalRoundFunction round;
    DecimalNormalizeFunction normalize;
    DecimalBinaryFunction binary[4];
    DecimalPowerFunction power;
    DecimalPowerTenFunction powerTen;
} DecimalFunctions;

static ONSDecimal decimalFixture(size_t index) {
    static const int kExponents[] = {-128, -127, -32, -1, 0, 1, 32, 126, 127};
    ONSDecimal result = {0};
    result._exponent = kExponents[(index / 9) % 9];
    result._length = (unsigned int)(index % 9);
    result._isNegative = (unsigned int)((index / 3) & 1);
    result._isCompact = (unsigned int)((index / 2) & 1);
    result._reserved = (unsigned int)((index * 0x4125U) & 0x3FFFFU);
    uint64_t state = UINT64_C(0x69374b6d2e815c09) ^ index;
    for (unsigned int word = 0; word < result._length; ++word) {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        result._mantissa[word] = (unsigned short)(state | 1);
    }
    if (index == 1 || index == 10) {
        result._exponent = 0;
        result._isNegative = 0;
        result._isCompact = 1;
        result._reserved = 0;
        result._mantissa[0] = index == 1 ? 1 : 10;
    }
    if (index == 8) {
        for (unsigned int word = 0; word < 8; ++word) {
            result._mantissa[word] = USHRT_MAX;
        }
    }
    return result;
}

static int resolveDecimalFunctions(DecimalFunctions *native, DecimalFunctions *replacement) {
    static const char *const kNativeNames[] = {
        "NSDecimalCopy",
        "NSDecimalCompact",
        "NSDecimalCompare",
        "NSDecimalRound",
        "NSDecimalNormalize",
        "NSDecimalAdd",
        "NSDecimalSubtract",
        "NSDecimalMultiply",
        "NSDecimalDivide",
        "NSDecimalPower",
        "NSDecimalMultiplyByPowerOf10"
    };
    static const char *const kReplacementNames[] = {
        OF_NAME_STRING(ONSDecimalCopy),
        OF_NAME_STRING(ONSDecimalCompact),
        OF_NAME_STRING(ONSDecimalCompare),
        OF_NAME_STRING(ONSDecimalRound),
        OF_NAME_STRING(ONSDecimalNormalize),
        OF_NAME_STRING(ONSDecimalAdd),
        OF_NAME_STRING(ONSDecimalSubtract),
        OF_NAME_STRING(ONSDecimalMultiply),
        OF_NAME_STRING(ONSDecimalDivide),
        OF_NAME_STRING(ONSDecimalPower),
        OF_NAME_STRING(ONSDecimalMultiplyByPowerOf10)
    };
    void *addresses[2][11];
    for (size_t index = 0; index < 11; ++index) {
        int status = resolveReferenceFunction(
            "Foundation", OF_REFERENCE_NS_UUID, kNativeNames[index], &addresses[0][index]
        );
        if (status != 0) {
            return status;
        }
        status = resolveReplacementFunction("OpenFoundation", kReplacementNames[index], &addresses[1][index]);
        if (status != 0) {
            return status;
        }
        if (addresses[0][index] == addresses[1][index]) {
            fprintf(
                stderr, "Setup failure: decimal oracle and replacement alias for %s.\n", kNativeNames[index]
            );
            return 77;
        }
    }
    DecimalFunctions *sets[] = {native, replacement};
    for (size_t set = 0; set < 2; ++set) {
        sets[set]->copy = (DecimalCopyFunction)addresses[set][0];
        sets[set]->compact = (DecimalCompactFunction)addresses[set][1];
        sets[set]->compare = (DecimalCompareFunction)addresses[set][2];
        sets[set]->round = (DecimalRoundFunction)addresses[set][3];
        sets[set]->normalize = (DecimalNormalizeFunction)addresses[set][4];
        for (size_t index = 0; index < 4; ++index) {
            sets[set]->binary[index] = (DecimalBinaryFunction)addresses[set][5 + index];
        }
        sets[set]->power = (DecimalPowerFunction)addresses[set][9];
        sets[set]->powerTen = (DecimalPowerTenFunction)addresses[set][10];
    }
    return 0;
}

static int compareDecimalBytes(
    const char *operation, size_t fixture, const ONSDecimal *native, const ONSDecimal *replacement
) {
    if (memcmp(native, replacement, sizeof(*native)) == 0) {
        return 0;
    }
    fprintf(stderr, "Decimal %s byte mismatch at fixture %zu.\nNative:     ", operation, fixture);
    for (size_t byte = 0; byte < sizeof(*native); ++byte) {
        fprintf(stderr, "%02x", ((const unsigned char *)native)[byte]);
    }
    fputs("\nReplacement:", stderr);
    for (size_t byte = 0; byte < sizeof(*replacement); ++byte) {
        fprintf(stderr, "%02x", ((const unsigned char *)replacement)[byte]);
    }
    fputc('\n', stderr);
    return 1;
}

int runFoundationDecimalDifferentialTests(void) {
    DecimalFunctions native;
    DecimalFunctions replacement;
    int status = resolveDecimalFunctions(&native, &replacement);
    if (status != 0) {
        return status;
    }
    size_t comparisons = 0;
    static const ONSInteger kScales[] = {-129, -128, -17, -1, 0, 1, 17, 38, 127, 32767};
    static const short kPowersOfTen[] = {-32768, -129, -128, -1, 0, 1, 127, 128, 32767};
    static const ONSUInteger kPowers[] = {0, 1, 2, 3, 7, ULONG_MAX};
    for (size_t fixture = 0; fixture < 81; ++fixture) {
        ONSDecimal input = decimalFixture(fixture);
        ONSDecimal nativeInput = input;
        ONSDecimal replacementInput = input;
        ONSDecimal nativeResult;
        ONSDecimal replacementResult;
        memset(&nativeResult, 0xA5, sizeof(nativeResult));
        memset(&replacementResult, 0xA5, sizeof(replacementResult));
        native.copy(&nativeResult, &nativeInput);
        replacement.copy(&replacementResult, &replacementInput);
        status = compareDecimalBytes("copy", fixture, &nativeResult, &replacementResult);
        if (status != 0) {
            return status;
        }
        ++comparisons;
        native.copy(&nativeInput, &nativeInput);
        replacement.copy(&replacementInput, &replacementInput);
        status = compareDecimalBytes("self copy", fixture, &nativeInput, &replacementInput);
        if (status != 0) {
            return status;
        }
        ++comparisons;
        nativeInput = input;
        replacementInput = input;
        native.compact(&nativeInput);
        replacement.compact(&replacementInput);
        status = compareDecimalBytes("compact", fixture, &nativeInput, &replacementInput);
        if (status != 0) {
            return status;
        }
        ++comparisons;
        for (size_t other = 0; other < 81; ++other) {
            ONSDecimal second = decimalFixture(other);
            nativeInput = input;
            replacementInput = input;
            ONSDecimal nativeSecond = second;
            ONSDecimal replacementSecond = second;
            ONSComparisonResult nativeOrder = native.compare(&nativeInput, &nativeSecond);
            ONSComparisonResult replacementOrder = replacement.compare(&replacementInput, &replacementSecond);
            if (nativeOrder != replacementOrder) {
                fprintf(
                    stderr,
                    "Decimal compare mismatch at fixture %zu/%zu: %ld / %ld.\n",
                    fixture,
                    other,
                    (long)nativeOrder,
                    (long)replacementOrder
                );
                return 1;
            }
            ++comparisons;
        }
        for (unsigned int mode = 0; mode < 4; ++mode) {
            for (size_t scale = 0; scale < sizeof(kScales) / sizeof(kScales[0]); ++scale) {
                for (unsigned int alias = 0; alias < 2; ++alias) {
                    nativeInput = input;
                    replacementInput = input;
                    memset(&nativeResult, 0x5A, sizeof(nativeResult));
                    memset(&replacementResult, 0x5A, sizeof(replacementResult));
                    ONSDecimal *nativeOutput = alias ? &nativeInput : &nativeResult;
                    ONSDecimal *replacementOutput = alias ? &replacementInput : &replacementResult;
                    native.round(nativeOutput, &nativeInput, kScales[scale], (ONSRoundingMode)mode);
                    replacement.round(
                        replacementOutput, &replacementInput, kScales[scale], (ONSRoundingMode)mode
                    );
                    status = compareDecimalBytes(
                        "round",
                        fixture * 1000 + mode * 100 + scale * 2 + alias,
                        nativeOutput,
                        replacementOutput
                    );
                    if (status != 0) {
                        return status;
                    }
                    ++comparisons;
                }
            }
            ONSDecimal second = decimalFixture((fixture * 13 + 7) % 81);
            for (unsigned int alias = 0; alias < 2; ++alias) {
                nativeInput = input;
                replacementInput = input;
                ONSDecimal nativeSecond = second;
                ONSDecimal replacementSecond = second;
                ONSDecimal *nativeOther = alias ? &nativeInput : &nativeSecond;
                ONSDecimal *replacementOther = alias ? &replacementInput : &replacementSecond;
                ONSCalculationError nativeError =
                    native.normalize(&nativeInput, nativeOther, (ONSRoundingMode)mode);
                ONSCalculationError replacementError =
                    replacement.normalize(&replacementInput, replacementOther, (ONSRoundingMode)mode);
                if (nativeError != replacementError) {
                    fprintf(
                        stderr,
                        "Decimal normalize error mismatch at fixture %zu mode %u: %lu / %lu.\n",
                        fixture,
                        mode,
                        (unsigned long)nativeError,
                        (unsigned long)replacementError
                    );
                    return 1;
                }
                status = compareDecimalBytes("normalize first", fixture, &nativeInput, &replacementInput);
                if (status == 0) {
                    status = compareDecimalBytes("normalize second", fixture, nativeOther, replacementOther);
                }
                if (status != 0) {
                    return status;
                }
                comparisons += 3;
            }
            for (size_t operation = 0; operation < 4; ++operation) {
                for (unsigned int alias = 0; alias < 3; ++alias) {
                    nativeInput = input;
                    replacementInput = input;
                    ONSDecimal nativeSecond = second;
                    ONSDecimal replacementSecond = second;
                    memset(&nativeResult, 0x93, sizeof(nativeResult));
                    memset(&replacementResult, 0x93, sizeof(replacementResult));
                    ONSDecimal *nativeOutput = alias == 1   ? &nativeInput
                                               : alias == 2 ? &nativeSecond
                                                            : &nativeResult;
                    ONSDecimal *replacementOutput = alias == 1   ? &replacementInput
                                                    : alias == 2 ? &replacementSecond
                                                                 : &replacementResult;
                    ONSCalculationError nativeError = native.binary[operation](
                        nativeOutput, &nativeInput, &nativeSecond, (ONSRoundingMode)mode
                    );
                    ONSCalculationError replacementError = replacement.binary[operation](
                        replacementOutput, &replacementInput, &replacementSecond, (ONSRoundingMode)mode
                    );
                    if (nativeError != replacementError) {
                        fprintf(
                            stderr,
                            "Decimal binary %zu error mismatch at fixture %zu mode %u alias %u: %lu / %lu.\n",
                            operation,
                            fixture,
                            mode,
                            alias,
                            (unsigned long)nativeError,
                            (unsigned long)replacementError
                        );
                        return 1;
                    }
                    status = compareDecimalBytes(
                        "binary",
                        fixture * 1000 + operation * 100 + mode * 10 + alias,
                        nativeOutput,
                        replacementOutput
                    );
                    if (status != 0) {
                        return status;
                    }
                    comparisons += 2;
                }
            }
            for (size_t power = 0; power < sizeof(kPowers) / sizeof(kPowers[0]); ++power) {
                nativeInput = input;
                replacementInput = input;
                memset(&nativeResult, 0x16, sizeof(nativeResult));
                memset(&replacementResult, 0x16, sizeof(replacementResult));
                ONSCalculationError nativeError =
                    native.power(&nativeResult, &nativeInput, kPowers[power], (ONSRoundingMode)mode);
                ONSCalculationError replacementError = replacement.power(
                    &replacementResult, &replacementInput, kPowers[power], (ONSRoundingMode)mode
                );
                if (nativeError != replacementError) {
                    fprintf(
                        stderr,
                        "Decimal power error mismatch at fixture %zu mode %u power %zu.\n",
                        fixture,
                        mode,
                        power
                    );
                    return 1;
                }
                status = compareDecimalBytes(
                    "power", fixture * 1000 + mode * 100 + power, &nativeResult, &replacementResult
                );
                if (status != 0) {
                    return status;
                }
                comparisons += 2;
            }
            for (size_t power = 0; power < sizeof(kPowersOfTen) / sizeof(kPowersOfTen[0]); ++power) {
                nativeInput = input;
                replacementInput = input;
                memset(&nativeResult, 0x61, sizeof(nativeResult));
                memset(&replacementResult, 0x61, sizeof(replacementResult));
                ONSCalculationError nativeError =
                    native.powerTen(&nativeResult, &nativeInput, kPowersOfTen[power], (ONSRoundingMode)mode);
                ONSCalculationError replacementError = replacement.powerTen(
                    &replacementResult, &replacementInput, kPowersOfTen[power], (ONSRoundingMode)mode
                );
                if (nativeError != replacementError) {
                    fprintf(
                        stderr,
                        "Decimal power-of-ten error mismatch at fixture %zu mode %u power %zu.\n",
                        fixture,
                        mode,
                        power
                    );
                    return 1;
                }
                status = compareDecimalBytes(
                    "power of ten", fixture * 1000 + mode * 100 + power, &nativeResult, &replacementResult
                );
                if (status != 0) {
                    return status;
                }
                comparisons += 2;
            }
        }
    }
    printf(
        "Foundation decimal differential passed: %zu exact byte/error comparisons across 11 APIs.\n",
        comparisons
    );
    return 0;
}
