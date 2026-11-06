//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Number/ONSDecimalNumberHandler.h"

#import "Foundation/Number/ONSUnrestoredInterfaces.h"

#import <objc/objc-exception.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified except the external class and exception bindings. iOS Device divergence:
// iOS Device 24A446 remains unread. Reading: Foundation/Number/Reading.md, round 2. Manual reference
// counting.

static _Atomic(id) defaultDecimalNumberHandler;

@implementation ONSDecimalNumberHandler

// Native +[NSDecimalNumberHandler defaultDecimalNumberHandler], 0x1c04fadb0. Non-external.
// Foundation defect: the native singleton load at 0x1c04fadc4 and store at 0x1c04fadf0 race,
// and concurrent first calls can leak a handler. Publish the initialized object atomically and
// release the losing allocation. Confidence: verified.
+ (id)defaultDecimalNumberHandler {
    id handler = atomic_load_explicit(&defaultDecimalNumberHandler, memory_order_acquire);
    if (handler != nil) {
        return handler;
    }

    id candidate = [objc_allocWithZone(self) initWithRoundingMode:0
                                                            scale:INT16_MAX
                                                 raiseOnExactness:NO
                                                  raiseOnOverflow:YES
                                                 raiseOnUnderflow:YES
                                              raiseOnDivideByZero:YES];
    if (atomic_compare_exchange_strong_explicit(
            &defaultDecimalNumberHandler, &handler, candidate, memory_order_acq_rel, memory_order_acquire
        )) {
        return candidate;
    }
    objc_release(candidate);
    return handler;
}

// Native -[NSDecimalNumberHandler initWithRoundingMode:scale:raiseOnExactness:raiseOnOverflow:
// raiseOnUnderflow:raiseOnDivideByZero:], 0x1c04fae00. Non-external.
- (id)initWithRoundingMode:(ONSRoundingMode)mode
                     scale:(short)scale
          raiseOnExactness:(BOOL)exactness
           raiseOnOverflow:(BOOL)overflow
          raiseOnUnderflow:(BOOL)underflow
       raiseOnDivideByZero:(BOOL)divideByZero {
    self = [super init];
    _scale = scale;
    _roundingMode = (unsigned int)mode & 7;
    _raiseOnExactness = exactness != NO;
    _raiseOnOverflow = overflow != NO;
    _raiseOnUnderflow = underflow != NO;
    _raiseOnDivideByZero = divideByZero != NO;
    return self;
}

// Native -[NSDecimalNumberHandler roundingMode], 0x1c04faf44. Non-external.
- (ONSRoundingMode)roundingMode {
    return _roundingMode;
}

// Native -[NSDecimalNumberHandler scale], 0x1c04faf50. Non-external.
- (short)scale {
    return (short)_scale;
}

// Native -[NSDecimalNumberHandler encodeWithCoder:], 0x1c04faf58. Non-external.
// At 0x1c04faf84 the assembly loads NS.roundingmode, absent from the pseudocode.
- (void)encodeWithCoder:(id)coder {
    if (![coder allowsKeyedCoding]) {
        return;
    }
    if (_roundingMode != 0) {
        [coder encodeInteger:_roundingMode forKey:(id)OF_STR("NS.roundingmode")];
    }
    if (_scale != INT16_MAX) {
        [coder encodeInteger:_scale forKey:(id)OF_STR("NS.scale")];
    }
    if (_raiseOnExactness) {
        [coder encodeBool:YES forKey:(id)OF_STR("NS.raise.exactness")];
    }
    if (_raiseOnOverflow) {
        [coder encodeBool:YES forKey:(id)OF_STR("NS.raise.overflow")];
    }
    if (_raiseOnUnderflow) {
        [coder encodeBool:YES forKey:(id)OF_STR("NS.raise.underflow")];
    }
    if (_raiseOnDivideByZero) {
        [coder encodeBool:YES forKey:(id)OF_STR("NS.raise.dividebyzero")];
    }
}

// Native -[NSDecimalNumberHandler initWithCoder:], 0x1c04fb044. Non-external.
- (id)initWithCoder:(id)coder {
    if (![coder allowsKeyedCoding]) {
        objc_release(self);
        return nil;
    }
    ONSRoundingMode mode = (ONSRoundingMode)[coder decodeIntegerForKey:(id)OF_STR("NS.roundingmode")];
    short scale = INT16_MAX;
    if ([coder containsValueForKey:(id)OF_STR("NS.scale")]) {
        scale = (short)[coder decodeIntegerForKey:(id)OF_STR("NS.scale")];
    }
    BOOL exactness = [coder decodeBoolForKey:(id)OF_STR("NS.raise.exactness")];
    BOOL overflow = [coder decodeBoolForKey:(id)OF_STR("NS.raise.overflow")];
    BOOL underflow = [coder decodeBoolForKey:(id)OF_STR("NS.raise.underflow")];
    BOOL divideByZero = [coder decodeBoolForKey:(id)OF_STR("NS.raise.dividebyzero")];
    return [self initWithRoundingMode:mode
                                scale:scale
                     raiseOnExactness:exactness
                      raiseOnOverflow:overflow
                     raiseOnUnderflow:underflow
                  raiseOnDivideByZero:divideByZero];
}

// Native -[NSDecimalNumberHandler exceptionDuringOperation:error:leftOperand:rightOperand:], 0x1c04fb154.
// Non-external. The assembly ends at objc_exception_throw, before the next function shown by Hex-Rays.
- (id)exceptionDuringOperation:(SEL)operation
                         error:(ONSCalculationError)error
                   leftOperand:(id)leftOperand
                  rightOperand:(id)rightOperand {
    id name;
    id reason;
    switch (error) {
        case 1:
            if (!_raiseOnExactness) {
                return nil;
            }
            name = (id)OF_STR("NSDecimalNumberExactnessException");
            reason = (id)OF_STR("NSDecimalNumber exactness exception");
            break;
        case 2:
            if (!_raiseOnUnderflow) {
                return nil;
            }
            name = (id)OF_STR("NSDecimalNumberUnderflowException");
            reason = (id)OF_STR("NSDecimalNumber underflow exception");
            break;
        case 3:
            if (!_raiseOnOverflow) {
                return nil;
            }
            name = (id)OF_STR("NSDecimalNumberOverflowException");
            reason = (id)OF_STR("NSDecimalNumber overflow exception");
            break;
        case 4:
            if (!_raiseOnDivideByZero) {
                return nil;
            }
            name = (id)OF_STR("NSDecimalNumberDivideByZeroException");
            reason = (id)OF_STR("NSDecimalNumber divide by zero exception");
            break;
        default:
            return nil;
    }
    objc_exception_throw([ONSException exceptionWithName:name reason:reason userInfo:nil]);
}

@end
