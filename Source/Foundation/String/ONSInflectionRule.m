//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSInflectionRule.h"
#import "Foundation/String/ONSAttributedStringInflection.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSString.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#import "CoreFoundation/Objective-C/OCFNSMutableArray.h"
#include "CoreFoundation/String/OCFString.h"
#include <dispatch/dispatch.h>
#include <objc/runtime.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Scratch/cf-core-r2-fg/Reading.md. Ownership follows the captured MRC calls.

// Attribute-key bytes are shared with serialized attributed strings in both naming modes.
static const OCFConstantString morphologyAttribute = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NS"
    "Morphology",
    sizeof("NS"
           "Morphology") -
        1
};
static const OCFConstantString inflectAttribute = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSInflect", sizeof("NSInflect") - 1
};
static const OCFConstantString referentConceptAttribute = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSInflectionReferentConcept",
    sizeof("NSInflectionReferentConcept") - 1
};
static const OCFConstantString agreementConceptAttribute = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSInflectionAgreementConcept",
    sizeof("NSInflectionAgreementConcept") - 1
};
static const OCFConstantString agreementArgumentAttribute = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSInflectionAgreementArgument",
    sizeof("NSInflectionAgreementArgument") - 1
};
static const OCFConstantString localizedNumberFormatAttribute = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSLocalizedNumberFormat",
    sizeof("NSLocalizedNumberFormat") - 1
};
static const OCFConstantString grammarAttribute = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSGrammar", sizeof("NSGrammar") - 1
};
static const OCFConstantString inflectionAlternativeAttribute = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSInflectionAlternative",
    sizeof("NSInflectionAlternative") - 1
};
static const OCFConstantString assumedFallbackInflectionAttribute = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSAssumedFallbackInflection",
    sizeof("NSAssumedFallbackInflection") - 1
};
static const OCFConstantString languageAttribute = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSLanguage", sizeof("NSLanguage") - 1
};

// Native _NSLanguageAttributeName, 0x2227d7b78. External.
id const _ONSLanguageAttributeName = (id)&languageAttribute;
// Native NSMorphologyAttributeName, 0x2227d8160. External.
id const ONSMorphologyAttributeName = (id)&morphologyAttribute;
// Native NSInflectionRuleAttributeName, 0x2227d8168. External.
id const ONSInflectionRuleAttributeName = (id)&inflectAttribute;
// Native NSInflectionReferentConceptAttributeName, 0x2227d8170. External.
id const ONSInflectionReferentConceptAttributeName = (id)&referentConceptAttribute;
// Native NSInflectionAgreementConceptAttributeName, 0x2227d8178. External.
id const ONSInflectionAgreementConceptAttributeName = (id)&agreementConceptAttribute;
// Native NSInflectionAgreementArgumentAttributeName, 0x2227d8180. External.
id const ONSInflectionAgreementArgumentAttributeName = (id)&agreementArgumentAttribute;
// Native NSLocalizedNumberFormatAttributeName, 0x2227d8190. External.
id const ONSLocalizedNumberFormatAttributeName = (id)&localizedNumberFormatAttribute;
// Native _NSGrammarAttributeName, 0x2227d8198. External.
id const _ONSGrammarAttributeName = (id)&grammarAttribute;
// Native _NSInflectableAttributeName, 0x2227d81a0. External. Shares the rule attribute's object.
id const _ONSInflectableAttributeName = (id)&inflectAttribute;
// Native _NSInflectionAlternativeAttributeName, 0x2227d81a8. External.
id const _ONSInflectionAlternativeAttributeName = (id)&inflectionAlternativeAttribute;
// Native _NSAssumedFallbackInflectionAttributeName, 0x2227d81b0. External.
id const _ONSAssumedFallbackInflectionAttributeName = (id)&assumedFallbackInflectionAttribute;

// Native _NSInflectionObserver, 0x22e6fcb60. Non-external zero-fill storage. Setter remains unread.
id inflectionObserver = nil;

extern char ruleMutableArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char ruleMorphologyClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMorphology));
extern char
    inflectionRuleExplicitPublicClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSInflectionRuleExplicit));

@implementation ONSInflectionRule

// Native -[NSInflectionRule _init], 0x1c0624834. Non-external.
- (instancetype)_init {
    return [super init];
}

// Native +[NSInflectionRule automaticRule], 0x1c0624894. Non-external.
+ (id)automaticRule {
    static dispatch_once_t once;
    static _ONSInflectionRuleAutomatic *rule;
    // Native ___33+[NSInflectionRule automaticRule]_block_invoke, 0x1c06248d4. Non-external.
    dispatch_once(&once, ^{
      rule = [[_ONSInflectionRuleAutomatic alloc] _init];
    });
    return rule;
}

// Native -[NSInflectionRule copyWithZone:], 0x1c0fe7ac4. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    ONSRequestConcreteImplementation(self, _cmd, objc_opt_class(self));
    return nil;
}

// Native -[NSInflectionRule encodeWithCoder:], 0x1c0fe7afc. Non-external.
- (void)encodeWithCoder:(id)coder {
    ONSRequestConcreteImplementation(self, _cmd, objc_opt_class(self));
}

// Native -[NSInflectionRule initWithCoder:], 0x1c0fe7b2c. Non-external.
- (instancetype)initWithCoder:(id)coder {
    ONSRequestConcreteImplementation(self, _cmd, objc_opt_class(self));
    objc_release(self);
    return nil;
}

// Native +[NSInflectionRule supportsSecureCoding], 0x1c0fe7b68. Non-external.
+ (BOOL)supportsSecureCoding {
    ONSRequestConcreteImplementation(self, _cmd, objc_opt_class(self));
    return NO;
}

@end

@implementation _ONSInflectionRuleAutomatic

// Native -[_NSInflectionRuleAutomatic copyWithZone:], 0x1c06248fc. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

// Native -[_NSInflectionRuleAutomatic encodeWithCoder:], 0x1c0624908. Non-external. The body is RET.
- (void)encodeWithCoder:(id)coder {
}

// Native -[_NSInflectionRuleAutomatic initWithCoder:], 0x1c062490c. Non-external.
- (instancetype)initWithCoder:(id)coder {
    objc_release(self);
    return objc_retain([ONSInflectionRule automaticRule]);
}

// Native -[_NSInflectionRuleAutomatic description], 0x1c062492c. Non-external.
- (id)description {
    return [ONSString stringWithFormat:(id)OF_STR("%@ (automatic inflection rule)"), [super description]];
}

@end

@implementation ONSInflectionRuleExplicit

// Native -[NSInflectionRuleExplicit initWithMorphology:], 0x1c06249b0. Non-external.
- (instancetype)initWithMorphology:(ONSMorphology *)morphology {
    self = [super _init];
    if (self) {
        _morphology = [morphology copy];
    }
    return self;
}

// Native -[NSInflectionRuleExplicit dealloc], 0x1c0624a34. Non-external.
- (void)dealloc {
    objc_release(_morphology);
    [super dealloc];
}

// Native -[NSInflectionRuleExplicit encodeWithCoder:], 0x1c0624ab0. Non-external.
- (void)encodeWithCoder:(id)coder {
    [coder encodeObject:[self morphology] forKey:(id)OF_STR("NS.Morphology")];
}

// Native -[NSInflectionRuleExplicit initWithCoder:], 0x1c0624ae0. Non-external.
- (instancetype)initWithCoder:(id)coder {
    self = [super _init];
    if (self) {
        Class cls = objc_opt_class((id)ruleMorphologyClass);
        _morphology = objc_retain([coder decodeObjectOfClass:cls forKey:(id)OF_STR("NS.Morphology")]);
    }
    return self;
}

// Native -[NSInflectionRuleExplicit hash], 0x1c0624b80. Non-external.
- (NSUInteger)hash {
    return [[self morphology] hash];
}

// Native -[NSInflectionRuleExplicit isEqual:], 0x1c0624b94. Non-external.
- (BOOL)isEqual:(id)object {
    if (!objc_opt_isKindOfClass(object, objc_opt_class(self))) {
        return NO;
    }
    ONSMorphology *morphology = [self morphology];
    return [morphology isEqual:[object morphology]];
}

// Native -[NSInflectionRuleExplicit description], 0x1c0624bf4. Non-external.
- (id)description {
    id description = [super description];
    return [ONSString stringWithFormat:(id)OF_STR("%@ { morphology = %@ }"), description, [self morphology]];
}

// Native -[NSInflectionRuleExplicit morphology], 0x1c0624c90. Non-external.
- (ONSMorphology *)morphology {
    return objc_getProperty(self, _cmd, 8, YES);
}

@end

@implementation _ONSInflectionRuleExplicit

// Native -[_NSInflectionRuleExplicit initWithAttributesDictionary:], 0x1c0624d1c. Non-external.
- (instancetype)initWithAttributesDictionary:(id<GrammarCollectionMessages>)attributes {
    NSInteger argument = 0;
    if (_ONSIsNSNumber([attributes objectForKeyedSubscript:ONSInflectionAgreementArgumentAttributeName])) {
        argument =
            [[attributes objectForKeyedSubscript:ONSInflectionAgreementArgumentAttributeName] integerValue];
    }
    NSInteger concept = 0;
    if (_ONSIsNSNumber([attributes objectForKeyedSubscript:ONSInflectionAgreementConceptAttributeName])) {
        concept =
            [[attributes objectForKeyedSubscript:ONSInflectionAgreementConceptAttributeName] integerValue];
    }
    NSInteger referent = 0;
    if (_ONSIsNSNumber([attributes objectForKeyedSubscript:ONSInflectionReferentConceptAttributeName])) {
        referent =
            [[attributes objectForKeyedSubscript:ONSInflectionReferentConceptAttributeName] integerValue];
    }
    id rule = [attributes objectForKeyedSubscript:ONSInflectionRuleAttributeName];
    ONSMorphology *morphology = nil;
    if (rule && objc_opt_isKindOfClass(rule, objc_opt_class((id)inflectionRuleExplicitPublicClass))) {
        morphology = [rule morphology];
    }
    if (!argument && !concept && !referent && !morphology) {
        objc_release(self);
        return nil;
    }
    self = [super _init];
    if (self) {
        _morphology = [morphology copy];
        _agreeWithConcept = concept;
        _agreeWithArgument = argument;
        _referentConcept = referent;
    }
    return self;
}

// Native -[_NSInflectionRuleExplicit dealloc], 0x1c0624f98. Non-external.
- (void)dealloc {
    objc_release(_morphology);
    [super dealloc];
}

// Native -[_NSInflectionRuleExplicit description], 0x1c062500c. Non-external.
- (id)description {
    id<GrammarCollectionMessages> parts =
        [(id<GrammarCollectionMessages>)objc_alloc((Class)ruleMutableArrayClass) initWithCapacity:3];
    if ([self morphology]) {
        [parts addObject:[ONSString stringWithFormat:(id)OF_STR("morphology = %@"), [self morphology]]];
    }
    if ([self agreeWithConcept]) {
        [parts addObject:[ONSString
                             stringWithFormat:(id)OF_STR("agreeWithConcept = %ld"), [self agreeWithConcept]]];
    }
    if ([self agreeWithArgument]) {
        [parts addObject:[ONSString stringWithFormat:(id)OF_STR("agreeWithArgument = %ld"),
                                                     [self agreeWithArgument]]];
    }
    if ([self referentConcept]) {
        [parts addObject:[ONSString
                             stringWithFormat:(id)OF_STR("referentConcept = %ld"), [self referentConcept]]];
    }
    id description;
    if ([parts count]) {
        id baseDescription = [super description];
        id joined = [(id)parts componentsJoinedByString:(id)OF_STR(", ")];
        description = [ONSString stringWithFormat:(id)OF_STR("%@ { %@ }"), baseDescription, joined];
    } else {
        description =
            [ONSString stringWithFormat:(id)OF_STR("%@ (automatic inflection rule)"), [super description]];
    }
    objc_release(parts);
    return description;
}

// Native -[_NSInflectionRuleExplicit hash], 0x1c06251e8. Non-external.
- (NSUInteger)hash {
    NSUInteger hash = [[self morphology] hash];
    hash ^= [self referentConcept];
    hash ^= [self agreeWithConcept];
    hash ^= [self agreeWithArgument];
    return hash;
}

// Native -[_NSInflectionRuleExplicit isEqual:], 0x1c0625240. Non-external.
// The native test ignores the other morphology when this rule's morphology is nil (kept).
- (BOOL)isEqual:(id)object {
    if (!objc_opt_isKindOfClass(object, objc_opt_class(self))) {
        return NO;
    }
    if ([self morphology]) {
        ONSMorphology *morphology = [self morphology];
        if (![morphology isEqual:[object morphology]]) {
            return NO;
        }
    }
    NSInteger referent = [self referentConcept];
    if (referent != [object referentConcept]) {
        return NO;
    }
    NSInteger concept = [self agreeWithConcept];
    if (concept != [object agreeWithConcept]) {
        return NO;
    }
    NSInteger argument = [self agreeWithArgument];
    return argument == [object agreeWithArgument];
}

// Native -[_NSInflectionRuleExplicit morphology], 0x1c0625350. Non-external.
- (ONSMorphology *)morphology {
    return objc_getProperty(self, _cmd, 8, YES);
}

// Native -[_NSInflectionRuleExplicit agreeWithConcept], 0x1c062535c. Non-external.
- (NSInteger)agreeWithConcept {
    return _agreeWithConcept;
}

// Native -[_NSInflectionRuleExplicit agreeWithArgument], 0x1c0625364. Non-external.
- (NSInteger)agreeWithArgument {
    return _agreeWithArgument;
}

// Native -[_NSInflectionRuleExplicit referentConcept], 0x1c062536c. Non-external.
- (NSInteger)referentConcept {
    return _referentConcept;
}

@end
