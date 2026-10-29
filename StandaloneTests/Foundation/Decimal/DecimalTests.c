//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "Foundation/Decimal/DecimalStandaloneTests.h"

#include <OpenFoundation/Decimal/ONSDecimal.h>

#include <limits.h>
#include <mach-o/dyld.h>
#include <malloc/malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Native raw NSDecimal group: iOS Simulator 24A434 public entries 0x1c04f90d0 / 0x1c04f91f0-0x1c04f9214,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; Device public Copy 0x180c9dab8 and
// entries 0x180eff47c-0x180eff4a0, UUID C041F1D0-5EF1-30A3-BF96-830976173B56. Confidence: verified.
// Reading: Documentation/Reference/Research/FoundationDecimal.md. Literal outcomes exercise the read
// integer/decimal dependency group without loading the system reference frameworks.
static ONSDecimal smallDecimal(unsigned short mantissa, int exponent, unsigned int negative) {
    ONSDecimal result = {0};
    result._exponent = exponent;
    result._length = mantissa != 0 ? 1 : 0;
    result._isNegative = negative;
    result._isCompact = mantissa != 0 ? 1 : 0;
    result._mantissa[0] = mantissa;
    return result;
}

static int expectDecimal(const char *name, const ONSDecimal *actual, ONSDecimal expected) {
    if (memcmp(actual, &expected, sizeof(*actual)) == 0) {
        return 0;
    }
    unsigned int header;
    memcpy(&header, actual, sizeof(header));
    fprintf(
        stderr,
        "Standalone decimal %s mismatch: header %08x, first limb %u.\n",
        name,
        header,
        actual->_mantissa[0]
    );
    return 1;
}

static int checkDecimalIsolation(void) {
    for (uint32_t image = 0; image < _dyld_image_count(); ++image) {
        const char *path = _dyld_get_image_name(image);
        if (strstr(path, "/System/Library/Frameworks/Foundation.framework/") ||
            strstr(path, "/System/Library/Frameworks/CoreFoundation.framework/")) {
            fprintf(stderr, "Standalone decimal isolation failure: loaded %s.\n", path);
            return 1;
        }
    }
    return 0;
}

int runFoundationDecimalStandaloneTests(void) {
    int status = checkDecimalIsolation();
    if (status != 0) {
        return status;
    }
    size_t assertions = 0;
    ONSDecimal twelve = smallDecimal(12, 0, 0);
    ONSDecimal three = smallDecimal(3, 0, 0);
    ONSDecimal zero = smallDecimal(0, 0, 0);
    ONSDecimal nan = smallDecimal(0, 0, 1);
    ONSDecimal output;
    memset(&output, 0xA5, sizeof(output));
    ONSDecimalCopy(&output, &twelve);
    static const unsigned char kCopiedBytes[20] = {0x00, 0x21, 0x00, 0x00, 0x0C, 0x00, 0xA5,
                                                   0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5,
                                                   0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5};
    if (memcmp(&output, kCopiedBytes, sizeof(output)) != 0) {
        fputs("Standalone decimal copy did not preserve inactive destination words.\n", stderr);
        return 1;
    }
    ++assertions;
    ONSDecimal compact = smallDecimal(1200, -2, 0);
    compact._isCompact = 0;
    ONSDecimalCompact(&compact);
    if (expectDecimal("compact", &compact, twelve) != 0) {
        return 1;
    }
    ++assertions;
    if (ONSDecimalCompare(&twelve, &three) != ONSOrderedDescending ||
        ONSDecimalCompare(&three, &twelve) != ONSOrderedAscending ||
        ONSDecimalCompare(&nan, &twelve) != ONSOrderedAscending ||
        ONSDecimalCompare(&nan, &nan) != ONSOrderedSame) {
        fputs("Standalone decimal compare mismatch.\n", stderr);
        return 1;
    }
    assertions += 4;
    ONSDecimal tie = smallDecimal(125, -2, 0);
    ONSDecimalRound(&output, &tie, 1, ONSRoundPlain);
    if (expectDecimal("plain round", &output, smallDecimal(13, -1, 0)) != 0) {
        return 1;
    }
    ++assertions;
    ONSDecimalRound(&tie, &tie, 1, ONSRoundBankers);
    if (expectDecimal("bankers in-place round", &tie, smallDecimal(12, -1, 0)) != 0) {
        return 1;
    }
    ++assertions;
    ONSDecimal first = twelve;
    ONSDecimal second = three;
    if (ONSDecimalNormalize(&first, &second, ONSRoundPlain) != ONSCalculationNoError ||
        expectDecimal("normalize first", &first, twelve) != 0 ||
        expectDecimal("normalize second", &second, three) != 0) {
        return 1;
    }
    assertions += 3;
    if (ONSDecimalAdd(&output, &twelve, &three, ONSRoundPlain) != ONSCalculationNoError ||
        expectDecimal("add", &output, smallDecimal(15, 0, 0)) != 0) {
        return 1;
    }
    assertions += 2;
    if (ONSDecimalSubtract(&output, &twelve, &three, ONSRoundPlain) != ONSCalculationNoError ||
        expectDecimal("subtract", &output, smallDecimal(9, 0, 0)) != 0) {
        return 1;
    }
    assertions += 2;
    if (ONSDecimalMultiply(&output, &twelve, &three, ONSRoundPlain) != ONSCalculationNoError ||
        expectDecimal("multiply", &output, smallDecimal(36, 0, 0)) != 0) {
        return 1;
    }
    assertions += 2;
    if (ONSDecimalDivide(&output, &twelve, &three, ONSRoundPlain) != ONSCalculationNoError ||
        expectDecimal("divide", &output, smallDecimal(4, 0, 0)) != 0) {
        return 1;
    }
    assertions += 2;
    if (ONSDecimalPower(&output, &three, 3, ONSRoundPlain) != ONSCalculationNoError ||
        expectDecimal("power", &output, smallDecimal(27, 0, 0)) != 0) {
        return 1;
    }
    assertions += 2;
    ONSDecimal two = smallDecimal(2, 0, 0);
    if (ONSDecimalPower(&output, &two, ULONG_MAX, ONSRoundPlain) != ONSCalculationNoError ||
        expectDecimal("signed power boundary", &output, smallDecimal(5, -1, 0)) != 0) {
        return 1;
    }
    assertions += 2;
    if (ONSDecimalMultiplyByPowerOf10(&output, &twelve, 2, ONSRoundPlain) != ONSCalculationNoError ||
        expectDecimal("power of ten", &output, smallDecimal(12, 2, 0)) != 0) {
        return 1;
    }
    assertions += 2;
    if (ONSDecimalDivide(&output, &twelve, &zero, ONSRoundPlain) != ONSCalculationDivideByZero ||
        expectDecimal("divide-by-zero result", &output, nan) != 0) {
        return 1;
    }
    assertions += 2;
    if (ONSDecimalMultiply(&output, &nan, &three, ONSRoundPlain) != ONSCalculationOverflow ||
        expectDecimal("NaN propagation", &output, nan) != 0) {
        return 1;
    }
    assertions += 2;
    ONSDecimal tiny = smallDecimal(1, -128, 0);
    if (ONSDecimalMultiplyByPowerOf10(&output, &tiny, -1, ONSRoundPlain) != ONSCalculationUnderflow ||
        expectDecimal("underflow", &output, nan) != 0) {
        return 1;
    }
    assertions += 2;
    if (ONSDecimalMultiply(&output, &tiny, &tiny, ONSRoundPlain) != ONSCalculationNoError ||
        expectDecimal("native multiply exponent wrapping", &output, smallDecimal(1, 0, 0)) != 0) {
        return 1;
    }
    assertions += 2;
    if (ONSDecimalPower(&output, &nan, 0, ONSRoundPlain) != ONSCalculationOverflow ||
        expectDecimal("NaN to zero power", &output, nan) != 0) {
        return 1;
    }
    assertions += 2;
    if (ONSDecimalPower(&output, &zero, ULONG_MAX, ONSRoundPlain) != ONSCalculationNoError ||
        expectDecimal("zero to negative power", &output, nan) != 0) {
        return 1;
    }
    assertions += 2;
    // Established native defect divergences are asserted only against the replacement.
    if (ONSDecimalPower(&output, &two, (ONSUInteger)1 << 63, ONSRoundPlain) != ONSCalculationOverflow ||
        expectDecimal("minimum signed power safety", &output, nan) != 0) {
        return 1;
    }
    assertions += 2;

    struct GuardedDecimal {
        ONSDecimal value;
        unsigned int guard[4];
    } guarded;

    memset(&guarded, 0x47, sizeof(guarded));
    ONSDecimal malformed = twelve;
    malformed._length = 15;
    ONSDecimalCopy(&guarded.value, &malformed);
    for (size_t index = 0; index < 4; ++index) {
        if (guarded.guard[index] != 0x47474747U) {
            fputs("Standalone decimal malformed-copy bounds failure.\n", stderr);
            return 1;
        }
    }
    if (guarded.value._length != 15) {
        fputs("Standalone decimal safety copy changed the native header.\n", stderr);
        return 1;
    }
    assertions += 5;
    for (size_t index = 0; index < 128; ++index) {
        (void)ONSDecimalDivide(&output, &twelve, &zero, ONSRoundPlain);
        (void)ONSDecimalMultiply(&output, &nan, &three, ONSRoundPlain);
        (void)ONSDecimalMultiplyByPowerOf10(&output, &tiny, -1, ONSRoundPlain);
    }
    // ASan quarantine retains freed error allocations in the malloc-zone statistics. The ordinary
    // isolated matrix checks exact allocation counts; ASan keeps its quarantine and checks accesses.
#if !__has_feature(address_sanitizer)
    malloc_statistics_t before;
    malloc_statistics_t after;
    malloc_zone_statistics(NULL, &before);
#endif
    for (size_t index = 0; index < 4096; ++index) {
        (void)ONSDecimalDivide(&output, &twelve, &zero, ONSRoundPlain);
        (void)ONSDecimalMultiply(&output, &nan, &three, ONSRoundPlain);
        (void)ONSDecimalMultiplyByPowerOf10(&output, &tiny, -1, ONSRoundPlain);
    }

#if !__has_feature(address_sanitizer)
    malloc_zone_statistics(NULL, &after);
    if (after.blocks_in_use != before.blocks_in_use || after.size_in_use != before.size_in_use) {
        fprintf(
            stderr,
            "Standalone decimal Swift error allocations grew: blocks %u/%u, bytes %zu/%zu.\n",
            before.blocks_in_use,
            after.blocks_in_use,
            before.size_in_use,
            after.size_in_use
        );
        return 1;
    }
    ++assertions;
#endif
    status = checkDecimalIsolation();
    if (status != 0) {
        return status;
    }
#if __has_feature(address_sanitizer)
    printf(
        "Foundation decimal standalone passed: %zu assertions across 11 APIs; 12288 error paths under "
        "ASan.\n",
        assertions
    );
#else
    printf(
        "Foundation decimal standalone passed: %zu assertions across 11 APIs; 12288 error paths without "
        "allocation growth.\n",
        assertions
    );
#endif
    return 0;
}
