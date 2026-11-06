//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringInflection.h"

#import "CoreFoundation/Objective-C/OCFNSDictionary.h"
#import "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"
#import "CoreFoundation/Objective-C/OCFNSSet.h"
#import "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSMorphologyCustomPronoun.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured assembly. Manual reference counting.
// iOS Device divergence: see Local/Scratch/cf-core-r2-fi/Reading.md for comparison status.

@protocol MorphologyConversionMessages
- (NSInteger)gender;
- (NSInteger)number;
- (id)englishCustomPronoun;
- (id)valueForKey:(id)key;
@end

#define MORPHOLOGY_STRING(text) {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, text, sizeof(text) - 1}

// Native constant string table, 0x22281c398.
static const OCFConstantString kGenderNames[] = {
    MORPHOLOGY_STRING("NotSet"),
    MORPHOLOGY_STRING("Feminine"),
    MORPHOLOGY_STRING("Masculine"),
    MORPHOLOGY_STRING("Neuter"),
};

// Native constant string table, 0x22281c3b8.
static const OCFConstantString kNumberNames[] = {
    MORPHOLOGY_STRING("NotSet"),
    MORPHOLOGY_STRING("Singular"),
    MORPHOLOGY_STRING("Zero"),
    MORPHOLOGY_STRING("Plural"),
    MORPHOLOGY_STRING("PluralTwo"),
    MORPHOLOGY_STRING("PluralFew"),
    MORPHOLOGY_STRING("PluralMany"),
};

// Native constant string table, 0x22281c3f0.
static const OCFConstantString kPartOfSpeechNames[] = {
    MORPHOLOGY_STRING("NotSet"),
    MORPHOLOGY_STRING("Determiner"),
    MORPHOLOGY_STRING("Pronoun"),
    MORPHOLOGY_STRING("Letter"),
    MORPHOLOGY_STRING("Adverb"),
    MORPHOLOGY_STRING("Particle"),
    MORPHOLOGY_STRING("Adjective"),
    MORPHOLOGY_STRING("Adposition"),
    MORPHOLOGY_STRING("Verb"),
    MORPHOLOGY_STRING("Noun"),
    MORPHOLOGY_STRING("Conjunction"),
    MORPHOLOGY_STRING("Numeral"),
    MORPHOLOGY_STRING("Interjection"),
    MORPHOLOGY_STRING("Preposition"),
    MORPHOLOGY_STRING("Abbreviation"),
};

// Native constant string table, 0x22281c468.
static const OCFConstantString kCaseNames[] = {
    MORPHOLOGY_STRING("NotSet"),
    MORPHOLOGY_STRING("Nominative"),
    MORPHOLOGY_STRING("Accusative"),
    MORPHOLOGY_STRING("Dative"),
    MORPHOLOGY_STRING("Genitive"),
    MORPHOLOGY_STRING("Prepositional"),
    MORPHOLOGY_STRING("Ablative"),
    MORPHOLOGY_STRING("Adessive"),
    MORPHOLOGY_STRING("Allative"),
    MORPHOLOGY_STRING("Elative"),
    MORPHOLOGY_STRING("Illative"),
    MORPHOLOGY_STRING("Essive"),
    MORPHOLOGY_STRING("Inessive"),
    MORPHOLOGY_STRING("Locative"),
    MORPHOLOGY_STRING("Translative"),
};

// Native constant string table, 0x22281c4e0.
static const OCFConstantString kDefinitenessNames[] = {
    MORPHOLOGY_STRING("NotSet"),
    MORPHOLOGY_STRING("Indefinite"),
    MORPHOLOGY_STRING("Definite"),
};

// Native constant string table, 0x22281c4f8.
static const OCFConstantString kDeterminationNames[] = {
    MORPHOLOGY_STRING("NotSet"),
    MORPHOLOGY_STRING("Independent"),
    MORPHOLOGY_STRING("Dependent"),
};

// Native constant string table, 0x22281c510.
static const OCFConstantString kPersonNames[] = {
    MORPHOLOGY_STRING("NotSet"),
    MORPHOLOGY_STRING("First"),
    MORPHOLOGY_STRING("Second"),
    MORPHOLOGY_STRING("Third"),
};

// Native constant string table, 0x22281c530.
static const OCFConstantString kPronounTypeNames[] = {
    MORPHOLOGY_STRING("NotSet"),
    MORPHOLOGY_STRING("Personal"),
    MORPHOLOGY_STRING("Reflexive"),
    MORPHOLOGY_STRING("Possessive"),
};

#undef MORPHOLOGY_STRING

@implementation ONSMorphology

// Native -[NSMorphology dealloc], 0x1c04c7df4. Non-external.
- (void)dealloc {
    objc_release(_customPronouns);
    [super dealloc];
}

// Native -[NSMorphology _customPronouns], 0x1c04c7e68. Non-external.
- (id)_customPronouns {
    return objc_autorelease([_customPronouns copy]);
}

// Native -[NSMorphology copyWithZone:], 0x1c04c7ebc. Non-external.
// Foundation defect: the store at 0x1c04c7ee0 dereferences a nil allocation result. Return nil before
// copying.
- (id)copyWithZone:(struct _NSZone *)zone {
    (void)zone;
    ONSMorphology *copy = objc_alloc_init(objc_opt_class(self));
    if (copy == nil) {
        return nil;
    }
    copy->_grammaticalGender = [self grammaticalGender];
    copy->_grammaticalCase = [self grammaticalCase];
    copy->_partOfSpeech = [self partOfSpeech];
    copy->_definiteness = [self definiteness];
    copy->_number = [self number];
    copy->_determination = [self determination];
    copy->_grammaticalPerson = [self grammaticalPerson];
    copy->_pronounType = [self pronounType];
    copy->_adjectival = [self _adjectival];
    copy->_clusivity = [self _clusivity];
    copy->_customPronouns = [_customPronouns mutableCopy];
    return copy;
}

// Native -[NSMorphology hash], 0x1c04c7f70. Non-external.
- (NSUInteger)hash {
    NSUInteger hash = (NSUInteger)[self grammaticalGender];
    hash ^= (NSUInteger)[self grammaticalCase];
    hash ^= (NSUInteger)[self partOfSpeech];
    hash ^= (NSUInteger)[self definiteness];
    hash ^= (NSUInteger)[self number];
    hash ^= (NSUInteger)[self determination];
    hash ^= (NSUInteger)[self grammaticalPerson];
    hash ^= (NSUInteger)[self pronounType];
    hash ^= (NSUInteger)[self _adjectival];
    hash ^= (NSUInteger)[self _clusivity];
    return hash ^ [_customPronouns hash];
}

// Native -[NSMorphology isEqual:], 0x1c04c8020. Non-external.
- (BOOL)isEqual:(id)object {
    if (!objc_opt_isKindOfClass(object, [ONSMorphology class])) {
        return NO;
    }
    ONSMorphology *other = object;
    NSInteger grammaticalGenderValue = [self grammaticalGender];
    if (grammaticalGenderValue != [other grammaticalGender]) {
        return NO;
    }
    NSInteger grammaticalCaseValue = [self grammaticalCase];
    if (grammaticalCaseValue != [other grammaticalCase]) {
        return NO;
    }
    NSInteger partOfSpeechValue = [self partOfSpeech];
    if (partOfSpeechValue != [other partOfSpeech]) {
        return NO;
    }
    NSInteger definitenessValue = [self definiteness];
    if (definitenessValue != [other definiteness]) {
        return NO;
    }
    NSInteger numberValue = [self number];
    if (numberValue != [other number]) {
        return NO;
    }
    NSInteger determinationValue = [self determination];
    if (determinationValue != [other determination]) {
        return NO;
    }
    NSInteger grammaticalPersonValue = [self grammaticalPerson];
    if (grammaticalPersonValue != [other grammaticalPerson]) {
        return NO;
    }
    NSInteger pronounTypeValue = [self pronounType];
    if (pronounTypeValue != [other pronounType]) {
        return NO;
    }
    NSInteger _adjectivalValue = [self _adjectival];
    if (_adjectivalValue != [other _adjectival]) {
        return NO;
    }
    NSInteger _clusivityValue = [self _clusivity];
    if (_clusivityValue != [other _clusivity]) {
        return NO;
    }
    if ([(id)_customPronouns count] != 0 || [(id)other->_customPronouns count] != 0) {
        return [_customPronouns isEqual:other->_customPronouns];
    }
    return YES;
}

// Native +[NSMorphology supportsSecureCoding], 0x1c04c81b8. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSMorphology encodeWithCoder:], 0x1c04c81c0. Non-external.
- (void)encodeWithCoder:(ONSCoder *)coder {
    [coder encodeInteger:[self grammaticalGender] forKey:(id)OF_STR("grammaticalGender")];
    [coder encodeInteger:[self grammaticalCase] forKey:(id)OF_STR("grammaticalCase")];
    [coder encodeInteger:[self partOfSpeech] forKey:(id)OF_STR("partOfSpeech")];
    [coder encodeInteger:[self definiteness] forKey:(id)OF_STR("definiteness")];
    [coder encodeInteger:[self number] forKey:(id)OF_STR("number")];
    [coder encodeInteger:[self determination] forKey:(id)OF_STR("determination")];
    [coder encodeInteger:[self grammaticalPerson] forKey:(id)OF_STR("grammaticalPerson")];
    [coder encodeInteger:[self pronounType] forKey:(id)OF_STR("pronounType")];
    if (_customPronouns != nil && [(id)_customPronouns count] != 0) {
        [coder encodeObject:_customPronouns forKey:(id)OF_STR("customPronouns")];
    }
}

// Native -[NSMorphology initWithCoder:], 0x1c04c82e8. Non-external.
- (id)initWithCoder:(ONSCoder *)coder {
    self = [super init];
    if (self == nil) {
        return nil;
    }
    _grammaticalGender = [coder decodeIntegerForKey:(id)OF_STR("grammaticalGender")];
    _grammaticalCase = [coder decodeIntegerForKey:(id)OF_STR("grammaticalCase")];
    _partOfSpeech = [coder decodeIntegerForKey:(id)OF_STR("partOfSpeech")];
    _definiteness = [coder decodeIntegerForKey:(id)OF_STR("definiteness")];
    _number = [coder decodeIntegerForKey:(id)OF_STR("number")];
    _determination = [coder decodeIntegerForKey:(id)OF_STR("determination")];
    _grammaticalPerson = [coder decodeIntegerForKey:(id)OF_STR("grammaticalPerson")];
    _pronounType = [coder decodeIntegerForKey:(id)OF_STR("pronounType")];
    Class dictionaryClass = [ONSDictionary class];
    Class stringClass = [ONSString class];
    Class pronounClass = [ONSMorphologyCustomPronoun class];
    id classes = [ONSSet setWithObjects:dictionaryClass, stringClass, pronounClass, nil];
    id pronouns = [coder decodeObjectOfClasses:classes forKey:(id)OF_STR("customPronouns")];
    if (pronouns != nil) {
        if (!_ONSIsNSDictionary(pronouns)) {
            goto invalid;
        }
        for (id language in pronouns) {
            if (!_ONSIsNSString(language)) {
                goto invalid;
            }
            id pronoun = [pronouns objectForKeyedSubscript:language];
            if (!objc_opt_isKindOfClass(pronoun, [ONSMorphologyCustomPronoun class])) {
                goto invalid;
            }
        }
    }
    _customPronouns = [pronouns mutableCopy];
    return self;

invalid:
    [coder failWithError:[ONSError errorWithDomain:ONSCocoaErrorDomain code:4864 userInfo:nil]];
    objc_release(self);
    return nil;
}

// Native -[NSMorphology description], 0x1c04c8584. Non-external.
- (id)description {
    id superclassDescription = [super description];
    NSInteger grammaticalGenderValue = [self grammaticalGender];
    id grammaticalGenderName = (NSUInteger)grammaticalGenderValue < 4
                                   ? (id)&kGenderNames[grammaticalGenderValue]
                                   : [ONSString stringWithFormat:(id)OF_STR("(NSGrammaticalGender)(%lld)"),
                                                                 (long long)grammaticalGenderValue];
    NSInteger numberValue = [self number];
    id numberName =
        (NSUInteger)numberValue < 7
            ? (id)&kNumberNames[numberValue]
            : [ONSString stringWithFormat:(id)OF_STR("(NSGrammaticalNumber)(%lld)"), (long long)numberValue];
    NSInteger partOfSpeechValue = [self partOfSpeech];
    id partOfSpeechName = (NSUInteger)partOfSpeechValue < 15
                              ? (id)&kPartOfSpeechNames[partOfSpeechValue]
                              : [ONSString stringWithFormat:(id)OF_STR("(NSGrammaticalPartOfSpeech)(%lld)"),
                                                            (long long)partOfSpeechValue];
    NSInteger grammaticalCaseValue = [self grammaticalCase];
    id grammaticalCaseName = (NSUInteger)grammaticalCaseValue < 15
                                 ? (id)&kCaseNames[grammaticalCaseValue]
                                 : [ONSString stringWithFormat:(id)OF_STR("(NSGrammaticalCase)(%lld)"),
                                                               (long long)grammaticalCaseValue];
    NSInteger definitenessValue = [self definiteness];
    id definitenessName = (NSUInteger)definitenessValue < 3
                              ? (id)&kDefinitenessNames[definitenessValue]
                              : [ONSString stringWithFormat:(id)OF_STR("(NSGrammaticalDefiniteness)(%lld)"),
                                                            (long long)definitenessValue];
    NSInteger determinationValue = [self determination];
    id determinationName = (NSUInteger)determinationValue < 3
                               ? (id)&kDeterminationNames[determinationValue]
                               : [ONSString stringWithFormat:(id)OF_STR("(NSGrammaticalDetermination)(%lld)"),
                                                             (long long)determinationValue];
    NSInteger grammaticalPersonValue = [self grammaticalPerson];
    id grammaticalPersonName = (NSUInteger)grammaticalPersonValue < 4
                                   ? (id)&kPersonNames[grammaticalPersonValue]
                                   : [ONSString stringWithFormat:(id)OF_STR("(NSGrammaticalPerson)(%lld)"),
                                                                 (long long)grammaticalPersonValue];
    NSInteger pronounTypeValue = [self pronounType];
    id pronounTypeName = (NSUInteger)pronounTypeValue < 4
                             ? (id)&kPronounTypeNames[pronounTypeValue]
                             : [ONSString stringWithFormat:(id)OF_STR("(NSGrammaticalPronounType)(%lld)"),
                                                           (long long)pronounTypeValue];
    return [ONSString
        stringWithFormat:(id)OF_STR(
                             "%@ { grammaticalGender = %@, number = %@, partOfSpeech = %@, case = %@, "
                             "definiteness = %@, determination = %@, grammaticalPerson = %@, "
                             "pronounType = %@, customPronouns = %@ }"
                         ),
                         superclassDescription,
                         grammaticalGenderName,
                         numberName,
                         partOfSpeechName,
                         grammaticalCaseName,
                         definitenessName,
                         determinationName,
                         grammaticalPersonName,
                         pronounTypeName,
                         _customPronouns];
}

// Native -[NSMorphology grammaticalGender], 0x1c04c8824. Non-external.
- (NSInteger)grammaticalGender {
    return _grammaticalGender;
}

// Native -[NSMorphology setGrammaticalGender:], 0x1c04c882c. Non-external.
- (void)setGrammaticalGender:(NSInteger)value {
    _grammaticalGender = value;
}

// Native -[NSMorphology partOfSpeech], 0x1c04c8834. Non-external.
- (NSInteger)partOfSpeech {
    return _partOfSpeech;
}

// Native -[NSMorphology setPartOfSpeech:], 0x1c04c883c. Non-external.
- (void)setPartOfSpeech:(NSInteger)value {
    _partOfSpeech = value;
}

// Native -[NSMorphology number], 0x1c04c8844. Non-external.
- (NSInteger)number {
    return _number;
}

// Native -[NSMorphology setNumber:], 0x1c04c884c. Non-external.
- (void)setNumber:(NSInteger)value {
    _number = value;
}

// Native -[NSMorphology grammaticalCase], 0x1c04c8854. Non-external.
- (NSInteger)grammaticalCase {
    return _grammaticalCase;
}

// Native -[NSMorphology setGrammaticalCase:], 0x1c04c885c. Non-external.
- (void)setGrammaticalCase:(NSInteger)value {
    _grammaticalCase = value;
}

// Native -[NSMorphology determination], 0x1c04c8864. Non-external.
- (NSInteger)determination {
    return _determination;
}

// Native -[NSMorphology setDetermination:], 0x1c04c886c. Non-external.
- (void)setDetermination:(NSInteger)value {
    _determination = value;
}

// Native -[NSMorphology grammaticalPerson], 0x1c04c8874. Non-external.
- (NSInteger)grammaticalPerson {
    return _grammaticalPerson;
}

// Native -[NSMorphology setGrammaticalPerson:], 0x1c04c887c. Non-external.
- (void)setGrammaticalPerson:(NSInteger)value {
    _grammaticalPerson = value;
}

// Native -[NSMorphology pronounType], 0x1c04c8884. Non-external.
- (NSInteger)pronounType {
    return _pronounType;
}

// Native -[NSMorphology setPronounType:], 0x1c04c888c. Non-external.
- (void)setPronounType:(NSInteger)value {
    _pronounType = value;
}

// Native -[NSMorphology definiteness], 0x1c04c8894. Non-external.
- (NSInteger)definiteness {
    return _definiteness;
}

// Native -[NSMorphology setDefiniteness:], 0x1c04c889c. Non-external.
- (void)setDefiniteness:(NSInteger)value {
    _definiteness = value;
}

@end

@implementation ONSMorphology (NSCustomPronouns)

// Native -[NSMorphology(NSCustomPronouns) setCustomPronoun:forLanguage:error:], 0x1c04c88d8. Non-external.
// Hex-Rays omits the fourth explicit argument register X4, which carries the caller's error pointer.
- (BOOL)setCustomPronoun:(id)pronoun forLanguage:(id)language error:(id *)error {
    if (pronoun != nil) {
        __unsafe_unretained id<MorphologyConversionMessages> capturedPronoun = pronoun;
        // Native ___69-[NSMorphology(NSCustomPronouns) setCustomPronoun:forLanguage:error:]_block_invoke,
        // 0x1c04c89cc. Non-external.
        id (^accessor)(id) = ^id(id key) {
          return [capturedPronoun valueForKey:key];
        };
        if (![ONSMorphologyCustomPronoun _validateCustomPronounWithAccessor:accessor
                                                                forLanguage:language
                                                                      error:error]) {
            return NO;
        }
    }
    if (_customPronouns == nil) {
        _customPronouns = objc_alloc_init([ONSMutableDictionary class]);
    }
    id copy = [pronoun copy];
    id key = [language lowercaseString];
    [(id<MorphologyMutableDictionaryMessages>)_customPronouns setObject:copy forKeyedSubscript:key];
    objc_release(copy);
    return YES;
}

@end

@implementation ONSMorphology (NSExternalRepresentation)

// Native -[NSMorphology(NSExternalRepresentation) _initWithExternalRepresentationDictionary:],
// 0x1c04ca2b0. Non-external.
- (id)_initWithExternalRepresentationDictionary:(id)dictionary {
    self = [super init];
    if (self == nil) {
        return nil;
    }
    if (!_NSGrammaticalGenderGetFromExternalRepresentation(
            [dictionary objectForKeyedSubscript:(id)OF_STR("grammaticalGender")], &_grammaticalGender
        ) ||
        !_NSGrammaticalNumberGetFromExternalRepresentation(
            [dictionary objectForKeyedSubscript:(id)OF_STR("number")], &_number
        ) ||
        !_NSGrammaticalPartOfSpeechGetFromExternalRepresentation(
            [dictionary objectForKeyedSubscript:(id)OF_STR("partOfSpeech")], &_partOfSpeech
        ) ||
        !_NSGrammaticalCaseGetFromExternalRepresentation(
            [dictionary objectForKeyedSubscript:(id)OF_STR("grammaticalCase")], &_grammaticalCase
        ) ||
        !_NSGrammaticalPronounTypeGetFromExternalRepresentation(
            [dictionary objectForKeyedSubscript:(id)OF_STR("pronounType")], &_pronounType
        )) {
        goto invalid;
    }

    id determination = [dictionary objectForKeyedSubscript:(id)OF_STR("determination")];
    if (determination == nil) {
        _determination = 0;
    } else if ([determination isEqual:(id)OF_STR("independent")]) {
        _determination = 1;
    } else if ([determination isEqual:(id)OF_STR("dependent")]) {
        _determination = 2;
    } else {
        goto invalid;
    }

    id definiteness = [dictionary objectForKeyedSubscript:(id)OF_STR("definiteness")];
    if (definiteness == nil) {
        _definiteness = 0;
    } else if ([definiteness isEqual:(id)OF_STR("indefinite")]) {
        _definiteness = 1;
    } else if ([definiteness isEqual:(id)OF_STR("definite")]) {
        _definiteness = 2;
    } else {
        goto invalid;
    }

    id pronouns = [dictionary objectForKeyedSubscript:(id)OF_STR("customPronouns")];
    if (pronouns == nil) {
        return self;
    }
    if (!_ONSIsNSDictionary(pronouns)) {
        goto invalid;
    }
    for (id language in pronouns) {
        id forms = [pronouns objectForKey:language];
        if (!_ONSIsNSString(language) || !_ONSIsNSDictionary(forms)) {
            goto invalid;
        }
        if ([ONSMorphologyCustomPronoun isSupportedForLanguage:language]) {
            id pronoun = [objc_alloc([ONSMorphologyCustomPronoun class])
                _initWithExternalRepresentationDictionary:forms];
            if (pronoun == nil) {
                goto invalid;
            }
            if (_customPronouns == nil) {
                _customPronouns = objc_alloc_init([ONSMutableDictionary class]);
            }
            [(id<MorphologyMutableDictionaryMessages>)_customPronouns setObject:pronoun
                                                              forKeyedSubscript:language];
            objc_release(pronoun);
        }
    }
    return self;

invalid:
    objc_release(self);
    return nil;
}

@end

@implementation ONSMorphology (NSMorphologyConversionInternal)

// Native -[NSMorphology(NSMorphologyConversionInternal) initWithInflection:], 0x1c04cac34. Non-external.
- (id)initWithInflection:(id<MorphologyConversionMessages>)inflection {
    self = [self init];
    if (self != nil) {
        // Native tables 0x1c12a4f60 and 0x1c12a4f80.
        static const NSInteger genders[] = {0, 2, 1, 3};
        static const NSInteger numbers[] = {0, 1, 4, 5, 6, 2, 3};
        NSUInteger gender = (NSUInteger)[inflection gender];
        if (gender < sizeof(genders) / sizeof(genders[0])) {
            [self setGrammaticalGender:genders[gender]];
        }
        NSUInteger number = (NSUInteger)[inflection number];
        if (number < sizeof(numbers) / sizeof(numbers[0])) {
            [self setNumber:numbers[number]];
        }
        id pronoun = [inflection englishCustomPronoun];
        if (pronoun != nil) {
            [self setCustomPronoun:pronoun forLanguage:(id)OF_STR("en") error:NULL];
        }
    }
    return self;
}

@end

@implementation ONSMorphology (NSMorphologyInternalGrammemes)

// Native -[NSMorphology(NSMorphologyInternalGrammemes) _clusivity], 0x1c04caccc. Non-external.
- (NSInteger)_clusivity {
    return _clusivity;
}

// Native -[NSMorphology(NSMorphologyInternalGrammemes) set_clusivity:], 0x1c04cacd4. Non-external.
- (void)set_clusivity:(NSInteger)value {
    _clusivity = value;
}

// Native -[NSMorphology(NSMorphologyInternalGrammemes) _adjectival], 0x1c04cacdc. Non-external.
- (NSInteger)_adjectival {
    return _adjectival;
}

// Native -[NSMorphology(NSMorphologyInternalGrammemes) set_adjectival:], 0x1c04cace4. Non-external.
- (void)set_adjectival:(NSInteger)value {
    _adjectival = value;
}

@end

extern void *uplrules_open(const char *locale, int32_t *status);
extern int32_t
uplrules_select(const void *rules, double number, UniChar *keyword, int32_t capacity, int32_t *status);
extern void uplrules_close(void *rules);

@implementation ONSMorphology (Inflection)

// Native -[NSMorphology(Inflection) applyPluralityRulesForNumbers:inLanguages:], 0x1c05343f0. Non-external.
// Hex-Rays loses the doubleValue result in D0 at the uplrules_select call.
// Foundation defect: the native locale buffer can be filled to its advertised capacity without a terminator.
// Keep that capacity and append a terminator in an extra initialized byte.
- (void)applyPluralityRulesForNumbers:(id<MorphologyIndexedMessages>)numbers
                          inLanguages:(id<MorphologyIndexedMessages>)languages {
    if ([self number] == 3) {
        return;
    }

    NSInteger selected = 0;
    NSInteger count = (NSInteger)[numbers count];
    for (NSInteger index = 0; index < count; index++) {
        id number = [numbers objectAtIndexedSubscript:(NSUInteger)index];
        ONSString *language = [languages objectAtIndexedSubscript:(NSUInteger)index];
        OCFIndex integer = -1;
        if (!OCFNumberGetValue((OCFNumberRef)number, kOCFNumberCFIndexType, &integer)) {
            integer = -1;
        }
        if (selected != 0) {
            continue;
        }

        char locale[158] = {0};
        int32_t status = 0;
        NSUInteger length = [language length];
        if (![language getBytes:locale
                      maxLength:157
                     usedLength:NULL
                       encoding:4
                        options:0
                          range:(ONSRange){0, length}
                 remainingRange:NULL]) {
            locale[0] = 'e';
            locale[1] = 'n';
            locale[2] = '\0';
        }
        void *rules = uplrules_open(locale, &status);
        if (status > 0) {
            continue;
        }
        status = 0;
        UniChar keyword[157] = {0};
        double value = [number doubleValue];
        int32_t keywordLength = uplrules_select(rules, value, keyword, 157, &status);
        uplrules_close(rules);
        if (status <= 0 && keywordLength >= 1) {
            id category = [objc_alloc([ONSString class]) initWithCharactersNoCopy:keyword
                                                                           length:(NSUInteger)keywordLength
                                                                     freeWhenDone:NO];
            if ([category isEqual:(id)OF_STR("one")]) {
                selected = 1;
            } else if ([category isEqual:(id)OF_STR("two")]) {
                selected = 4;
            } else if ([category isEqual:(id)OF_STR("other")]) {
                selected = 3;
            }
            objc_release(category);
        }
    }

    if (selected == 0) {
        selected = 3;
    }
    if ([self number] == 0) {
        [self setNumber:selected];
    } else if ([self number] != selected) {
        [self setNumber:3];
    }
}

@end
