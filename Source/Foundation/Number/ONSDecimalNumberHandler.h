//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenFoundation/Decimal/ONSDecimal.h>

#import <objc/NSObject.h>

// Native NSDecimalNumberHandler, 0x22e6d7448. External. class_ro_t 0x22284c138:
// instance start 8, size 32. Superclass binds to NSObject (medium confidence).
__attribute__((visibility("default")))
@interface ONSDecimalNumberHandler : NSObject {
    @private
    signed int _scale : 16;
    unsigned int _roundingMode : 3;
    unsigned int _raiseOnExactness : 1;
    unsigned int _raiseOnOverflow : 1;
    unsigned int _raiseOnUnderflow : 1;
    unsigned int _raiseOnDivideByZero : 1;
    unsigned int _unused : 9;
    void *_reserved2;
    void *_reserved;
}

+ (id)defaultDecimalNumberHandler;

- (id)initWithRoundingMode:(ONSRoundingMode)mode
                     scale:(short)scale
          raiseOnExactness:(BOOL)exactness
           raiseOnOverflow:(BOOL)overflow
          raiseOnUnderflow:(BOOL)underflow
       raiseOnDivideByZero:(BOOL)divideByZero;
- (ONSRoundingMode)roundingMode;
- (short)scale;
- (void)encodeWithCoder:(id)coder;
- (id)initWithCoder:(id)coder;
- (id)exceptionDuringOperation:(SEL)operation
                         error:(ONSCalculationError)error
                   leftOperand:(id)leftOperand
                  rightOperand:(id)rightOperand;

@end
