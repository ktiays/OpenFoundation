//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <stdint.h>

typedef struct {
    uint32_t planeCount;
    const uint8_t *const *planes;
} UniCharBitmapData;

extern const uint8_t __CFUniCharDecimalDigitCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharDecimalDigitCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharLetterCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharLetterCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharLetterCharacterSetBitmapPlane2[8192];

extern const uint8_t __CFUniCharLetterCharacterSetBitmapPlane3[8192];

extern const uint8_t __CFUniCharLetterCharacterSetBitmapPlane14[8192];

extern const uint8_t __CFUniCharLowercaseLetterCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharLowercaseLetterCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharUppercaseLetterCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharUppercaseLetterCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharNonBaseCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharNonBaseCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharNonBaseCharacterSetBitmapPlane14[8192];

extern const uint8_t __CFUniCharCanonicalDecomposableCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharCanonicalDecomposableCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharCanonicalDecomposableCharacterSetBitmapPlane2[8192];

extern const uint8_t __CFUniCharAlphanumericCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharAlphanumericCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharAlphanumericCharacterSetBitmapPlane2[8192];

extern const uint8_t __CFUniCharAlphanumericCharacterSetBitmapPlane3[8192];

extern const uint8_t __CFUniCharAlphanumericCharacterSetBitmapPlane14[8192];

extern const uint8_t __CFUniCharPunctuationCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharPunctuationCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharLegalCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharLegalCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharLegalCharacterSetBitmapPlane2[8192];

extern const uint8_t __CFUniCharLegalCharacterSetBitmapPlane3[8192];

extern const uint8_t __CFUniCharLegalCharacterSetBitmapPlane14[8192];

extern const uint8_t __CFUniCharTitlecaseLetterCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharSymbolAndOperatorCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharSymbolAndOperatorCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharCompatibilityDecomposableCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharCompatibilityDecomposableCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharHfsPlusDecomposableCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharHfsPlusDecomposableCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharStrongRightToLeftCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharStrongRightToLeftCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharHasNonSelfLowercaseMappingBitmapPlane0[8192];

extern const uint8_t __CFUniCharHasNonSelfLowercaseMappingBitmapPlane1[8192];

extern const uint8_t __CFUniCharHasNonSelfUppercaseMappingBitmapPlane0[8192];

extern const uint8_t __CFUniCharHasNonSelfUppercaseMappingBitmapPlane1[8192];

extern const uint8_t __CFUniCharHasNonSelfTitlecaseMappingBitmapPlane0[8192];

extern const uint8_t __CFUniCharHasNonSelfTitlecaseMappingBitmapPlane1[8192];

extern const uint8_t __CFUniCharHasNonSelfCaseFoldingMappingBitmapPlane0[8192];

extern const uint8_t __CFUniCharHasMirroredMappingBitmapPlane0[8192];

extern const uint8_t __CFUniCharHasMirroredMappingBitmapPlane1[8192];

extern const uint8_t __CFUniCharControlAndFormatterBitmapPlane0[8192];

extern const uint8_t __CFUniCharControlAndFormatterBitmapPlane1[8192];

extern const uint8_t __CFUniCharControlAndFormatterBitmapPlane14[8192];

extern const uint8_t __CFUniCharCaseIgnorableCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharCaseIgnorableCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharCaseIgnorableCharacterSetBitmapPlane14[8192];

extern const uint8_t __CFUniCharGraphemeExtendCharacterSetBitmapPlane0[8192];

extern const uint8_t __CFUniCharGraphemeExtendCharacterSetBitmapPlane1[8192];

extern const uint8_t __CFUniCharGraphemeExtendCharacterSetBitmapPlane14[8192];

extern const uint8_t *const __CFUniCharDecimalDigitCharacterSetBitmap[2];

extern const uint8_t *const __CFUniCharLetterCharacterSetBitmap[15];

extern const uint8_t *const __CFUniCharLowercaseLetterCharacterSetBitmap[2];

extern const uint8_t *const __CFUniCharUppercaseLetterCharacterSetBitmap[2];

extern const uint8_t *const __CFUniCharNonBaseCharacterSetBitmap[15];

extern const uint8_t *const __CFUniCharCanonicalDecomposableCharacterSetBitmap[3];

extern const uint8_t *const __CFUniCharAlphanumericCharacterSetBitmap[15];

extern const uint8_t *const __CFUniCharPunctuationCharacterSetBitmap[2];

extern const uint8_t *const __CFUniCharLegalCharacterSetBitmap[15];

extern const uint8_t *const __CFUniCharTitlecaseLetterCharacterSetBitmap[1];

extern const uint8_t *const __CFUniCharSymbolAndOperatorCharacterSetBitmap[2];

extern const uint8_t *const __CFUniCharCompatibilityDecomposableCharacterSetBitmap[2];

extern const uint8_t *const __CFUniCharHfsPlusDecomposableCharacterSetBitmap[2];

extern const uint8_t *const __CFUniCharStrongRightToLeftCharacterSetBitmap[2];

extern const uint8_t *const __CFUniCharHasNonSelfLowercaseMappingBitmap[2];

extern const uint8_t *const __CFUniCharHasNonSelfUppercaseMappingBitmap[2];

extern const uint8_t *const __CFUniCharHasNonSelfTitlecaseMappingBitmap[2];

extern const uint8_t *const __CFUniCharHasNonSelfCaseFoldingMappingBitmap[1];

extern const uint8_t *const __CFUniCharHasMirroredMappingBitmap[2];

extern const uint8_t *const __CFUniCharControlAndFormatterBitmap[15];

extern const uint8_t *const __CFUniCharCaseIgnorableCharacterSetBitmap[15];

extern const uint8_t *const __CFUniCharGraphemeExtendCharacterSetBitmap[15];

extern const UniCharBitmapData __CFUniCharBitmapDataArray[22];

extern const uint32_t __CFUniCharDecompositionTable[4158];

extern const uint32_t __CFUniCharMultipleDecompositionTable[2088];
