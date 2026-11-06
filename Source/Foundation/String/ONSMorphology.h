//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSMorphology (__DATA,__objc_data 0x22e6d6408, external), assigned to family FM-misc's class
// list; only its two direct methods below are in this family's scope. class_ro_t 0x222848860: flags 0,
// instance start 8, size 96; ivars _customPronouns (NSMutableDictionary) at 8, then the NSInteger features
// _adjectival 16, _clusivity 24, _grammaticalGender 32, _partOfSpeech 40, _number 48, _grammaticalCase 56,
// _determination 64, _grammaticalPerson 72, _pronounType 80 and _definiteness 88 (offset variables
// 0x22e6d43a8-0x22e6d43d0; read with Local/Scratch/cf-core-fc/classes.py). Its superclass pointer lies
// outside the image: NSObject. The coordinator reconciles this declaration with FM-misc. Only the accessors
// the restored code sends are declared.
@class ONSCoder;
@class ONSMutableDictionary;

__attribute__((visibility("default")))
@interface ONSMorphology : NSObject {
    ONSMutableDictionary *_customPronouns;
    NSInteger _adjectival;
    NSInteger _clusivity;
    NSInteger _grammaticalGender;
    NSInteger _partOfSpeech;
    NSInteger _number;
    NSInteger _grammaticalCase;
    NSInteger _determination;
    NSInteger _grammaticalPerson;
    NSInteger _pronounType;
    NSInteger _definiteness;
}

+ (BOOL)supportsSecureCoding;
- (id)initWithCoder:(ONSCoder *)coder;
- (void)encodeWithCoder:(ONSCoder *)coder;
- (id)_customPronouns;

- (NSInteger)grammaticalGender;
- (void)setGrammaticalGender:(NSInteger)value;
- (NSInteger)partOfSpeech;
- (void)setPartOfSpeech:(NSInteger)value;
- (NSInteger)number;
- (void)setNumber:(NSInteger)value;
- (NSInteger)grammaticalCase;
- (void)setGrammaticalCase:(NSInteger)value;
- (NSInteger)determination;
- (void)setDetermination:(NSInteger)value;
- (NSInteger)grammaticalPerson;
- (void)setGrammaticalPerson:(NSInteger)value;
- (NSInteger)pronounType;
- (void)setPronounType:(NSInteger)value;
- (NSInteger)definiteness;
- (void)setDefiniteness:(NSInteger)value;
@end

@interface ONSMorphology (NSMorphologyInternalGrammemes)
- (NSInteger)_adjectival;
- (void)set_adjectival:(NSInteger)value;
- (NSInteger)_clusivity;
- (void)set_clusivity:(NSInteger)value;

@end

@interface ONSMorphology (NSCustomPronouns)
- (BOOL)setCustomPronoun:(id)pronoun forLanguage:(id)language error:(id *)error;
@end

@interface ONSMorphology (NSExternalRepresentation)
- (id)_initWithExternalRepresentationDictionary:(id)dictionary;
@end

@interface ONSMorphology (NSMorphologyConversionInternal)
- (id)initWithInflection:(id)inflection;
@end

@interface ONSMorphology (Inflection)
- (void)applyPluralityRulesForNumbers:(id)numbers inLanguages:(id)languages;
@end

@interface ONSMorphology (_Morphun)
- (id)initWithMorphunAttributeValues:(id)values;
- (BOOL)addConstraintsTo:(void *)inflectable;
@end

// The two NSMorphology direct methods of this family (natively in NSMorphology's own implementation; direct
// methods carry no runtime metadata, so declaring and defining them in a category is not observable).
@interface ONSMorphology (InflectionDirectMethods)

// Native -[NSMorphology mergeMorphology:override:], 0x1c04bb12c, a direct method (no selector argument; a nil
// receiver returns at once).
- (void)mergeMorphology:(id)morphology override:(BOOL)override __attribute__((objc_direct));

// Native -[NSMorphology isIdentity], 0x1c04bb398, a direct method (a nil receiver answers NO).
- (BOOL)isIdentity __attribute__((objc_direct));

@end
#endif
