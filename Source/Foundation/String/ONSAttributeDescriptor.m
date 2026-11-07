//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributeDescriptor.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSCharacterSet.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/String/OCFString.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#import "Foundation/String/ONSAttributedStringInflection.h"
#import "Foundation/String/ONSInflectionRule.h"

#include <stdlib.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

extern char numericDescriptorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                               OF_NAME_STRING(_ONSNumericAttributeDescriptor));
extern char assumedFallbackInflectionDescriptorClass[] __asm(
    OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(_ONSAssumedFallbackInflectionAttributeDescriptor)
);
extern char
    numberFormattingDescriptorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                OF_NAME_STRING(_ONSNumberFormattingAttributeDescriptor));
extern char morphologyDescriptorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                  OF_NAME_STRING(_ONSMorphologyAttributeDescriptor));
extern char inflectionDescriptorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                  OF_NAME_STRING(_ONSInflectionAttributeDescriptor));
extern char markdownStringDescriptorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                      OF_NAME_STRING(_ONSMarkdownStringAttributeDescriptor));
extern char stringDescriptorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                              OF_NAME_STRING(_ONSStringAttributeDescriptor));
extern char uRLDescriptorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(_ONSURLAttributeDescriptor));

// CoreFoundation's NSURL class (GOT slot 0x22227FB88, bound by its selector; confidence: medium).
extern char attributeURLClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSURL));

@protocol AttributeDescriptorMessages
+ (id)URLWithString:(id)string;
@end

// Native _attributeKeyToDescriptor, 0x2227d5bb8, non-external. The descriptors following the
// table at 0x2227d5c98 hold their class pointer and a constant name (the Language entries share one).
typedef struct {
    const void *isa;
    const OCFConstantString *name;
} StaticAttributeDescriptor;

typedef struct {
    OCFStringRef key;
    const StaticAttributeDescriptor *descriptor;
} AttributeDescriptorEntry;

static const OCFConstantString kAgreeWithArgumentKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSAgreeWithArgumentAttributeName",
    sizeof("NSAgreeWithArgumentAttributeName") - 1
};
static const OCFConstantString kAgreeWithArgumentName = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSInflectionAgreementArgument",
    sizeof("NSInflectionAgreementArgument") - 1
};
static const StaticAttributeDescriptor kAgreeWithArgumentDescriptor = {
    numericDescriptorClass, &kAgreeWithArgumentName
};
static const OCFConstantString kAgreeWithConceptKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSAgreeWithConceptAttributeName",
    sizeof("NSAgreeWithConceptAttributeName") - 1
};
static const OCFConstantString kAgreeWithConceptName = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSInflectionAgreementConcept",
    sizeof("NSInflectionAgreementConcept") - 1
};
static const StaticAttributeDescriptor kAgreeWithConceptDescriptor = {
    numericDescriptorClass, &kAgreeWithConceptName
};
static const OCFConstantString kAssumedFallbackInflectionKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSAssumedFallbackInflectionAttributeName",
    sizeof("NSAssumedFallbackInflectionAttributeName") - 1
};
static const OCFConstantString kAssumedFallbackInflectionName = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSAssumedFallbackInflection",
    sizeof("NSAssumedFallbackInflection") - 1
};
static const StaticAttributeDescriptor kAssumedFallbackInflectionDescriptor = {
    assumedFallbackInflectionDescriptorClass, &kAssumedFallbackInflectionName
};
static const OCFConstantString kFormatNumberKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSFormatNumberAttributeName",
    sizeof("NSFormatNumberAttributeName") - 1
};
static const OCFConstantString kFormatNumberName = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSLocalizedNumberFormat",
    sizeof("NSLocalizedNumberFormat") - 1
};
static const StaticAttributeDescriptor kFormatNumberDescriptor = {
    numberFormattingDescriptorClass, &kFormatNumberName
};
static const OCFConstantString kGrammarKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSGrammarAttributeName",
    sizeof("NSGrammarAttributeName") - 1
};
static const OCFConstantString kGrammarName = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSGrammar", sizeof("NSGrammar") - 1
};
static const StaticAttributeDescriptor kGrammarDescriptor = {morphologyDescriptorClass, &kGrammarName};
static const OCFConstantString kInflectKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSInflectAttributeName",
    sizeof("NSInflectAttributeName") - 1
};
static const OCFConstantString kInflectName = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSInflect", sizeof("NSInflect") - 1
};
static const StaticAttributeDescriptor kInflectDescriptor = {inflectionDescriptorClass, &kInflectName};
static const OCFConstantString kInflectionAlternativeKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSInflectionAlternativeAttributeName",
    sizeof("NSInflectionAlternativeAttributeName") - 1
};
static const OCFConstantString kInflectionAlternativeName = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSInflectionAlternative",
    sizeof("NSInflectionAlternative") - 1
};
static const StaticAttributeDescriptor kInflectionAlternativeDescriptor = {
    markdownStringDescriptorClass, &kInflectionAlternativeName
};
static const OCFConstantString kLanguageKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSLanguageAttributeName",
    sizeof("NSLanguageAttributeName") - 1
};
static const OCFConstantString kLanguageName = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSLanguage", sizeof("NSLanguage") - 1
};
static const StaticAttributeDescriptor kLanguageDescriptor = {stringDescriptorClass, &kLanguageName};
static const OCFConstantString kLanguageIdentifierKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSLanguageIdentifierAttributeName",
    sizeof("NSLanguageIdentifierAttributeName") - 1
};
static const OCFConstantString kLinkKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSLinkAttributeName",
    sizeof("NSLinkAttributeName") - 1
};
static const OCFConstantString kLinkName = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSLink", sizeof("NSLink") - 1
};
static const StaticAttributeDescriptor kLinkDescriptor = {uRLDescriptorClass, &kLinkName};
static const OCFConstantString kMorphologyKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NS"
    "MorphologyAttributeName",
    sizeof("NS"
           "MorphologyAttributeName") -
        1
};
static const OCFConstantString kMorphologyName = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NS"
    "Morphology",
    sizeof("NS"
           "Morphology") -
        1
};
static const StaticAttributeDescriptor kMorphologyDescriptor = {morphologyDescriptorClass, &kMorphologyName};
static const OCFConstantString kReferentConceptKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSReferentConceptAttributeName",
    sizeof("NSReferentConceptAttributeName") - 1
};
static const OCFConstantString kReferentConceptName = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NSInflectionReferentConcept",
    sizeof("NSInflectionReferentConcept") - 1
};
static const StaticAttributeDescriptor kReferentConceptDescriptor = {
    numericDescriptorClass, &kReferentConceptName
};

static const AttributeDescriptorEntry attributeKeyToDescriptor[] = {
    {(OCFStringRef)&kAgreeWithArgumentKey, &kAgreeWithArgumentDescriptor},
    {(OCFStringRef)&kAgreeWithConceptKey, &kAgreeWithConceptDescriptor},
    {(OCFStringRef)&kAssumedFallbackInflectionKey, &kAssumedFallbackInflectionDescriptor},
    {(OCFStringRef)&kFormatNumberKey, &kFormatNumberDescriptor},
    {(OCFStringRef)&kGrammarKey, &kGrammarDescriptor},
    {(OCFStringRef)&kInflectKey, &kInflectDescriptor},
    {(OCFStringRef)&kInflectionAlternativeKey, &kInflectionAlternativeDescriptor},
    {(OCFStringRef)&kLanguageKey, &kLanguageDescriptor},
    {(OCFStringRef)&kLanguageIdentifierKey, &kLanguageDescriptor},
    {(OCFStringRef)&kLinkKey, &kLinkDescriptor},
    {(OCFStringRef)&kMorphologyKey, &kMorphologyDescriptor},
    {(OCFStringRef)&kReferentConceptKey, &kReferentConceptDescriptor},
};

@implementation _ONSAttributeDescriptor

// Native +[_NSAttributeDescriptor initialize], 0x1c0574498. Non-external. Realizes the subclasses of the
// static descriptors (objc_opt_class) before the table is used.
+ (void)initialize {
    if (self == [_ONSAttributeDescriptor self]) {
        [_ONSURLAttributeDescriptor class];
        [_ONSInflectionAttributeDescriptor class];
        [_ONSMorphologyAttributeDescriptor class];
    }
}

// Native -[_NSAttributeDescriptor dealloc], 0x1c0574514. Non-external.
- (void)dealloc {
    objc_release(_name);
    [super dealloc];
}

// Native -[_NSAttributeDescriptor name], 0x1c0574588. Non-external.
- (ONSString *)name {
    return _name;
}

// Native -[_NSAttributeDescriptor attributeValueFromValue:], 0x1c0fe3b7c. Non-external.
// This is the native abstract primitive. It raises via NSRequestConcreteImplementation.
- (id)attributeValueFromValue:(id)value {
    Class abstractClass = ONSClassFromString((id)OF_STR(OF_NAME_STRING(_ONSAttributeDescriptor)));
    ONSRequestConcreteImplementation(self, _cmd, abstractClass);
}

@end

// The static descriptors' retain and release do nothing and their retain count is NSUIntegerMax (-1); dealloc
// traps (brk #1, in the cold region).
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

@implementation _ONSURLAttributeDescriptor

// Native -[_NSURLAttributeDescriptor retain], 0x1c0574590. Non-external.
- (id)retain {
    return self;
}

// Native -[_NSURLAttributeDescriptor release], 0x1c0574594. Non-external.
- (oneway void)release {
}

// Native -[_NSURLAttributeDescriptor retainCount], 0x1c0574598. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

// Native -[_NSURLAttributeDescriptor attributeValueFromValue:], 0x1c05745a0. Non-external. A string becomes
// an NSURL (+URLWithString:); anything else nil.
- (id)attributeValueFromValue:(id)value {
    if (_ONSIsNSString(value)) {
        return [(id)attributeURLClass URLWithString:value];
    }
    return nil;
}

// Native -[_NSURLAttributeDescriptor dealloc], 0x1c0fe3bbc. Non-external.
- (void)dealloc {
    __builtin_trap();
}

@end

@implementation _ONSInflectionAttributeDescriptor

// Native -[_NSInflectionAttributeDescriptor retain], 0x1c05745e4. Non-external.
- (id)retain {
    return self;
}

// Native -[_NSInflectionAttributeDescriptor release], 0x1c05745e8. Non-external.
- (oneway void)release {
}

// Native -[_NSInflectionAttributeDescriptor retainCount], 0x1c05745ec. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

// Native -[_NSInflectionAttributeDescriptor attributeValueFromValue:], 0x1c05745f4. Non-external. A number is
// itself when true (else nil); a dictionary is an explicit inflection rule over the morphology it describes
// (nil when the morphology is); anything else nil.
- (id)attributeValueFromValue:(id)value {
    if (_ONSIsNSNumber(value)) {
        return [value boolValue] ? value : nil;
    }
    if (!_ONSIsNSDictionary(value)) {
        return nil;
    }
    id morphology = objc_autorelease([[ONSMorphology alloc] _initWithExternalRepresentationDictionary:value]);
    if (morphology == nil) {
        return nil;
    }
    return objc_autorelease([[ONSInflectionRuleExplicit alloc] initWithMorphology:morphology]);
}

// Native -[_NSInflectionAttributeDescriptor dealloc], 0x1c0fe3bc0. Non-external.
- (void)dealloc {
    __builtin_trap();
}

@end

@implementation _ONSMorphologyAttributeDescriptor

// Native -[_NSMorphologyAttributeDescriptor retain], 0x1c05747b0. Non-external.
- (id)retain {
    return self;
}

// Native -[_NSMorphologyAttributeDescriptor release], 0x1c05747b4. Non-external.
- (oneway void)release {
}

// Native -[_NSMorphologyAttributeDescriptor retainCount], 0x1c05747b8. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

// Native -[_NSMorphologyAttributeDescriptor attributeValueFromValue:], 0x1c05747c0. Non-external. A
// dictionary is the morphology it describes; a morphology is itself; anything else nil.
- (id)attributeValueFromValue:(id)value {
    if (_ONSIsNSDictionary(value)) {
        return objc_autorelease([[ONSMorphology alloc] _initWithExternalRepresentationDictionary:value]);
    }
    if ([value isKindOfClass:[ONSMorphology class]]) {
        return value;
    }
    return nil;
}

// Native -[_NSMorphologyAttributeDescriptor dealloc], 0x1c0fe3bd0. Non-external.
- (void)dealloc {
    __builtin_trap();
}

@end

#pragma clang diagnostic pop

@implementation _ONSAttributeDescriptor (NSAttributedDescriptorForAttributeKey)

// Native +[_NSAttributeDescriptor(NSAttributedDescriptorForAttributeKey) descriptorForAttributeKey:],
// 0x1c05748ec. Non-external.
+ (_ONSAttributeDescriptor *)descriptorForAttributeKey:(ONSString *)key {
    if (![key hasPrefix:(id)OF_STR("NS")]) {
        if ([key length] == 0) {
            key = nil;
        } else {
            ONSCharacterSet *lowercase = [ONSCharacterSet lowercaseLetterCharacterSet];
            if ([lowercase characterIsMember:[key characterAtIndex:0]]) {
                ONSMutableString *normalized = objc_autorelease([key mutableCopy]);
                ONSString *initial = [key substringWithRange:(ONSRange){0, 1}];
                [normalized replaceCharactersInRange:(ONSRange){0, 1} withString:[initial capitalizedString]];
                [normalized insertString:(id)OF_STR("NS") atIndex:0];
                [normalized appendString:(id)OF_STR("AttributeName")];
                key = normalized;
            } else {
                key = nil;
            }
        }
    }
    if ([key length] == 0) {
        return nil;
    }
    AttributeDescriptorEntry search = {(OCFStringRef)key, NULL};
    // Native ___91+[_NSAttributeDescriptor(NSAttributedDescriptorForAttributeKey)
    // descriptorForAttributeKey:]_block_invoke, 0x1c0574a24. Non-external.
    const AttributeDescriptorEntry *match = bsearch_b(
        &search,
        attributeKeyToDescriptor,
        sizeof(attributeKeyToDescriptor) / sizeof(*attributeKeyToDescriptor),
        sizeof(AttributeDescriptorEntry),
        ^int(const void *lhs, const void *rhs) {
          const AttributeDescriptorEntry *left = lhs;
          const AttributeDescriptorEntry *right = rhs;
          return (int)[(ONSString *)left->key compare:(id)right->key];
        }
    );
    return match != NULL ? (_ONSAttributeDescriptor *)match->descriptor : nil;
}

@end
