//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/OCFBuild.h"
#include <objc/objc.h>

OF_EXPORT id const ONSInflectionAgreementArgumentAttributeName;
OF_EXPORT id const ONSInflectionAgreementConceptAttributeName;
OF_EXPORT id const ONSInflectionReferentConceptAttributeName;
OF_EXPORT id const ONSInflectionRuleAttributeName;
OF_EXPORT id const ONSLocalizedNumberFormatAttributeName;
OF_EXPORT id const ONSMorphologyAttributeName;
OF_EXPORT id const _ONSAssumedFallbackInflectionAttributeName;
OF_EXPORT id const _ONSGrammarAttributeName;
OF_EXPORT id const _ONSInflectableAttributeName;
OF_EXPORT id const _ONSInflectionAlternativeAttributeName;
OF_EXPORT id const _ONSLanguageAttributeName;

#ifdef __OBJC__
#import <objc/NSObject.h>

__attribute__((visibility("default")))
@interface ONSInflectionRule : NSObject
- (instancetype)_init;
+ (id)automaticRule;
- (id)copyWithZone:(struct _NSZone *)zone;
- (void)encodeWithCoder:(id)coder;
- (instancetype)initWithCoder:(id)coder;
+ (BOOL)supportsSecureCoding;
@end

@class ONSMorphology;

// Native class_ro_t 0x2228673c8, instance start 8, size 16.
__attribute__((visibility("default")))
@interface ONSInflectionRuleExplicit : ONSInflectionRule {
    ONSMorphology *_morphology;
}
- (instancetype)initWithMorphology:(ONSMorphology *)morphology;
- (ONSMorphology *)morphology;
@end

// Native class_ro_t 0x222867528, instance start 8, size 40.
@interface _ONSInflectionRuleExplicit : ONSInflectionRule {
    ONSMorphology *_morphology;
    NSInteger _agreeWithConcept;
    NSInteger _agreeWithArgument;
    NSInteger _referentConcept;
}
- (instancetype)initWithAttributesDictionary:(id)attributes;
- (ONSMorphology *)morphology;
- (NSInteger)referentConcept;
- (NSInteger)agreeWithArgument;
- (NSInteger)agreeWithConcept;
@end

// Native class 0x22e6dfb20, class_ro_t 0x2228672f8, size 8.
@interface _ONSInflectionRuleAutomatic : ONSInflectionRule
@end
#endif
