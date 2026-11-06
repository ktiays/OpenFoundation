//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSMutableAttributedString.h"

#include <stdint.h>

@class ONSString;
@class ONSMutableString;
@class ONSRLEArray;
@class ONSMutableRLEArray;

// Native NSConcreteAttributedString, 0x22e6d6160, external. class_ro_t 0x2228480d0 gives
// superclass NSAttributedString, instance start 8 and size 24.
__attribute__((visibility("default")))
@interface ONSConcreteAttributedString : ONSAttributedString {
    @private
    ONSString *string;
    ONSRLEArray *attributes;
}
- (id)initWithString:(id)value attributes:(id)dictionary;
- (ONSRLEArray *)_runArrayHoldingAttributes;
@end

// Native NSConcreteMutableAttributedString, 0x22e6d6188, external. class_ro_t 0x222848258
// gives superclass NSMutableAttributedString, instance start 8 and size 28.
__attribute__((visibility("default")))
@interface ONSConcreteMutableAttributedString : ONSMutableAttributedString {
    @private
    ONSMutableString *mutableString;
    ONSMutableRLEArray *mutableAttributes;

    struct {
        uint32_t attributeFixingDisabled : 8;
        uint32_t mayNeedIntentResolution : 8;
        uint32_t : 16;
    } fields;
}
+ (Class)_mutableStringClass;
- (id)initWithString:(id)value attributes:(id)dictionary;
- (ONSRLEArray *)_runArrayHoldingAttributes;
- (void)edited:(NSUInteger)mask range:(ONSRange)range changeInLength:(NSInteger)delta;
@end
