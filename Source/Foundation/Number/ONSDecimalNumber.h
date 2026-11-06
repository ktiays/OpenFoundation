//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenFoundation/Decimal/ONSDecimal.h>
#import "Foundation/Number/ONSNumber.h"

// Native NSDecimalNumber, 0x22e6d7358. External. class_ro_t 0x22284bf30: NSNumber
// superclass, instance start 8, size 12 before variable-length mantissa storage at 12.
__attribute__((visibility("default")))
@interface ONSDecimalNumber : ONSNumber {
    @protected
    signed int _exponent : 8;
    unsigned int _length : 4;
    unsigned int _isNegative : 1;
    unsigned int _isCompact : 1;
    unsigned int _reserved : 1;
    unsigned int _hasExternalRefCount : 1;
    unsigned int _refs : 16;
    unsigned short _mantissa[0];
}

+ (id)decimalNumberWithDecimal:(ONSDecimal)decimal;
+ (id)minimumDecimalNumber;
+ (id)maximumDecimalNumber;
+ (id)notANumber;
+ (id)defaultBehavior;

- (id)initWithMantissa:(unsigned long long)mantissa exponent:(short)exponent isNegative:(BOOL)negative;
- (id)initWithDecimal:(ONSDecimal)decimal;
- (id)initWithString:(id)string;
- (id)initWithString:(id)string locale:(id)locale;
- (ONSDecimal)decimalValue;
- (id)decimalNumberByMultiplyingBy:(id)number;
- (id)decimalNumberByMultiplyingBy:(id)number withBehavior:(id)behavior;

@end

@interface ONSDecimalNumberPlaceholder : ONSDecimalNumber
@end

@interface _ONSStatic_NSDecimalNumber : ONSDecimalNumber
@end
