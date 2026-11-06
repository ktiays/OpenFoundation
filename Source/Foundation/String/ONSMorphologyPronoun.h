//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

@class ONSCoder;
@class ONSMorphology;
@class ONSString;

// NSMorphologyPronoun class object 0x22e6d64a8, class_ro_t 0x222848b38. Instance start 8, size 32.
__attribute__((visibility("default")))
@interface ONSMorphologyPronoun : NSObject {
    ONSString *_pronoun;
    ONSMorphology *_morphology;
    ONSMorphology *_dependentMorphology;
}
+ (BOOL)supportsSecureCoding;
- (id)initWithCoder:(ONSCoder *)coder;
- (void)encodeWithCoder:(ONSCoder *)coder;
- (ONSString *)pronoun;
- (ONSMorphology *)morphology;
- (ONSMorphology *)dependentMorphology;
@end
