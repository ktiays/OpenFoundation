//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringGrammarInflection.h"
#import "Foundation/String/ONSAttributedStringInflection.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "CoreFoundation/Objective-C/OCFNSData.h"
#import "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/String/OCFString.h"

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified except shared-cache GOT bindings (medium). iOS Device divergence: iOS Device 24A446
// remains unread. Reading: Local/Scratch/cf-core-r2-fg/Reading.md. The native retains/releases use MRC.

extern char grammarDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDictionary));
extern char grammarDataClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSData));
extern char grammarClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                     OF_NAME_STRING(_ONSAttributedStringGrammarInflection));

@implementation _ONSAttributedStringGrammarInflection

// Native -[_NSAttributedStringGrammarInflection init], 0x1c062b4d4. Non-external.
- (instancetype)init {
    return [super init];
}

// Native -[_NSAttributedStringGrammarInflection initWithCoder:], 0x1c062b534. Non-external.
- (instancetype)initWithCoder:(id)coder {
    id error = nil;
    Class cls = objc_opt_class((id)grammarDataClass);
    id data = [coder decodeObjectOfClass:cls forKey:(id)OF_STR("externalRepresentation")];
    self = [self initWithExternalRepresentation:data error:&error];
    if (!self) {
        [coder failWithError:error];
    }
    return self;
}

// Native -[_NSAttributedStringGrammarInflection encodeWithCoder:], 0x1c062b5d4. Non-external.
- (void)encodeWithCoder:(id)coder {
    [coder encodeObject:[self externalRepresentation] forKey:(id)OF_STR("externalRepresentation")];
}

// Native +[_NSAttributedStringGrammarInflection supportsSecureCoding], 0x1c062b604. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[_NSAttributedStringGrammarInflection dealloc], 0x1c062b60c. Non-external.
- (void)dealloc {
    objc_release(_englishCustomPronoun);
    [super dealloc];
}

// Native -[_NSAttributedStringGrammarInflection copyWithZone:], 0x1c062b680. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    _ONSAttributedStringGrammarInflection *copy = [[_ONSAttributedStringGrammarInflection alloc] init];
    copy->_grammaticalCase = [self grammaticalCase];
    copy->_gender = [self gender];
    copy->_number = [self number];
    copy->_person = [self person];
    copy->_context = [self context];
    copy->_determinationType = [self determinationType];
    copy->_englishCustomPronoun = [[self englishCustomPronoun] copy];
    return copy;
}

// Native -[_NSAttributedStringGrammarInflection mutableCopyWithZone:], 0x1c062b70c. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return [self copyWithZone:zone];
}

// Native -[_NSAttributedStringGrammarInflection hash], 0x1c062b710. Non-external.
- (NSUInteger)hash {
    NSUInteger hash = [self context];
    hash ^= [self gender];
    hash ^= [self grammaticalCase];
    hash ^= [self number];
    hash ^= [self person];
    hash ^= [self determinationType];
    if ([self englishCustomPronoun]) {
        hash ^= [[self englishCustomPronoun] hash];
    }
    return hash;
}

// Native -[_NSAttributedStringGrammarInflection isEqual:], 0x1c062b7a0. Non-external.
- (BOOL)isEqual:(id)object {
    if (!objc_opt_isKindOfClass(object, objc_opt_class((id)grammarClass))) {
        return NO;
    }
    _ONSAttributedStringGrammarInflection *other = object;
    NSInteger context = [self context];
    if (context != [other context]) {
        return NO;
    }
    NSInteger gender = [self gender];
    if (gender != [other gender]) {
        return NO;
    }
    NSInteger grammaticalCase = [self grammaticalCase];
    if (grammaticalCase != [other grammaticalCase]) {
        return NO;
    }
    NSInteger number = [self number];
    if (number != [other number]) {
        return NO;
    }
    NSInteger person = [self person];
    if (person != [other person]) {
        return NO;
    }
    NSInteger determination = [self determinationType];
    if (determination != [other determinationType]) {
        return NO;
    }
    id pronoun = [self englishCustomPronoun];
    if (pronoun == [other englishCustomPronoun]) {
        return YES;
    }
    pronoun = [self englishCustomPronoun];
    return [pronoun isEqual:[other englishCustomPronoun]];
}

// Native -[_NSAttributedStringGrammarInflection englishCustomPronoun], 0x1c062b958. Non-external.
- (ONSMorphologyCustomPronoun *)englishCustomPronoun {
    return objc_autorelease([_englishCustomPronoun copy]);
}

// Native -[_NSAttributedStringGrammarInflection initWithMorphology:], 0x1c062b970. Non-external.
// Mapping tables at 0x1c13dfa78, 0x1c13dfa98 and 0x1c13dfad0.
- (instancetype)initWithMorphology:(id)morphology {
    self = [super init];
    if (!self) {
        return nil;
    }

    static const NSInteger genders[] = {0, 2, 1, 3};
    NSUInteger gender = [morphology grammaticalGender];
    if (gender <= 3) {
        [self setGender:genders[gender]];
    }

    static const NSInteger numbers[] = {0, 1, 5, 6, 2, 3, 4};
    NSUInteger number = [morphology number];
    if (number <= 6) {
        [self setNumber:numbers[number]];
    }

    static const NSInteger cases[] = {5, 1, 2, 3, 8, 0, 0, 0, 0, 0, 0, 0, 4};
    NSUInteger grammaticalCase = (NSUInteger)[morphology grammaticalCase] - 1;
    [self setGrammaticalCase:grammaticalCase <= 12 ? cases[grammaticalCase] : 0];

    NSInteger definiteness = [morphology definiteness];
    [self setDeterminationType:definiteness == 2 ? 2 : definiteness == 1 ? 3 : 0];

    id<GrammarCollectionMessages> customPronouns = [morphology _customPronouns];
    id<GrammarCollectionMessages> pronouns = [customPronouns allValues];
    if ([pronouns count] == 1) {
        _englishCustomPronoun = [[pronouns objectAtIndexedSubscript:0] copy];
    }
    return self;
}

// Native -[_NSAttributedStringGrammarInflection description], 0x1c062bb44. Non-external.
- (id)description {
    ONSMutableString *description =
        [ONSMutableString stringWithFormat:(id)OF_STR("<%@ %p { "), objc_opt_class(self), self];
    if ([self context]) {
        [description appendFormat:(id)OF_STR(" context = %d;"), (int)[self context]];
    }
    if ([self gender]) {
        [description appendFormat:(id)OF_STR(" gender = %d;"), (int)[self gender]];
    }
    if ([self grammaticalCase]) {
        [description appendFormat:(id)OF_STR(" grammaticalCase = %d;"), (int)[self grammaticalCase]];
    }
    if ([self number]) {
        [description appendFormat:(id)OF_STR(" number = %d;"), (int)[self number]];
    }
    if ([self person]) {
        [description appendFormat:(id)OF_STR(" person = %d;"), (int)[self person]];
    }
    if ([self englishCustomPronoun]) {
        [description appendFormat:(id)OF_STR(" English pronoun = %@;"), [self englishCustomPronoun]];
    }
    [description appendString:(id)OF_STR("}>")];
    return objc_autorelease([description copy]);
}

// Native -[_NSAttributedStringGrammarInflection context], 0x1c062bc94. Non-external.
- (NSInteger)context {
    return _context;
}

// Native -[_NSAttributedStringGrammarInflection setContext:], 0x1c062bc9c. Non-external.
- (void)setContext:(NSInteger)value {
    _context = value;
}

// Native -[_NSAttributedStringGrammarInflection gender], 0x1c062bca4. Non-external.
- (NSInteger)gender {
    return _gender;
}

// Native -[_NSAttributedStringGrammarInflection setGender:], 0x1c062bcac. Non-external.
- (void)setGender:(NSInteger)value {
    _gender = value;
}

// Native -[_NSAttributedStringGrammarInflection grammaticalCase], 0x1c062bcb4. Non-external.
- (NSInteger)grammaticalCase {
    return _grammaticalCase;
}

// Native -[_NSAttributedStringGrammarInflection setGrammaticalCase:], 0x1c062bcbc. Non-external.
- (void)setGrammaticalCase:(NSInteger)value {
    _grammaticalCase = value;
}

// Native -[_NSAttributedStringGrammarInflection determinationType], 0x1c062bcc4. Non-external.
- (NSInteger)determinationType {
    return _determinationType;
}

// Native -[_NSAttributedStringGrammarInflection setDeterminationType:], 0x1c062bccc. Non-external.
- (void)setDeterminationType:(NSInteger)value {
    _determinationType = value;
}

// Native -[_NSAttributedStringGrammarInflection number], 0x1c062bcd4. Non-external.
- (NSInteger)number {
    return _number;
}

// Native -[_NSAttributedStringGrammarInflection setNumber:], 0x1c062bcdc. Non-external.
- (void)setNumber:(NSInteger)value {
    _number = value;
}

// Native -[_NSAttributedStringGrammarInflection person], 0x1c062bce4. Non-external.
- (NSInteger)person {
    return _person;
}

// Native -[_NSAttributedStringGrammarInflection setPerson:], 0x1c062bcec. Non-external.
- (void)setPerson:(NSInteger)value {
    _person = value;
}

@end

@implementation _ONSAttributedStringGrammarInflection (NSExternalRepresentation)

// Native -[_NSAttributedStringGrammarInflection(NSExternalRepresentation)
// initWithExternalRepresentation:error:], 0x1c062c5cc. Non-external.
- (instancetype)initWithExternalRepresentation:(id)data error:(id *)error {
    id object = [ONSJSONSerialization JSONObjectWithData:data options:0 error:error];
    if (object) {
        if (_ONSIsNSDictionary(object)) {
            return [self initWithExternalRepresentationDictionary:object error:error];
        }
        if (error) {
            *error = [ONSError errorWithDomain:ONSCocoaErrorDomain code:3840 userInfo:nil];
        }
    }
    objc_release(self);
    return nil;
}

// Native -[_NSAttributedStringGrammarInflection(NSExternalRepresentation)
// initWithExternalRepresentationDictionary:error:], 0x1c062c664. Non-external.
- (instancetype)initWithExternalRepresentationDictionary:(id<GrammarCollectionMessages>)dictionary
                                                   error:(id *)error {
    self = [super init];
    if (!self) {
        return nil;
    }

    id context = [dictionary objectForKeyedSubscript:(id)OF_STR("context")];
    NSInteger contextValue = 0;
    if (context && ![context isEqual:(id)OF_STR("none")]) {
        if ([context isEqual:(id)OF_STR("technicalTerm")]) {
            contextValue = 1;
        } else {
            goto invalid;
        }
    }
    [self setContext:contextValue];

    id grammaticalGender = [dictionary objectForKeyedSubscript:(id)OF_STR("grammaticalGender")];
    NSInteger grammaticalGenderValue = 0;
    if (grammaticalGender && ![grammaticalGender isEqual:(id)OF_STR("none")]) {
        if ([grammaticalGender isEqual:(id)OF_STR("masculine")]) {
            grammaticalGenderValue = 1;
        } else if ([grammaticalGender isEqual:(id)OF_STR("feminine")]) {
            grammaticalGenderValue = 2;
        } else if ([grammaticalGender isEqual:(id)OF_STR("neuter")]) {
            grammaticalGenderValue = 3;
        } else if ([grammaticalGender isEqual:(id)OF_STR("common")]) {
            grammaticalGenderValue = 4;
        } else {
            goto invalid;
        }
    }
    [self setGender:grammaticalGenderValue];

    id grammaticalCase = [dictionary objectForKeyedSubscript:(id)OF_STR("grammaticalCase")];
    NSInteger grammaticalCaseValue = 0;
    if (grammaticalCase && ![grammaticalCase isEqual:(id)OF_STR("none")]) {
        if ([grammaticalCase isEqual:(id)OF_STR("accusative")]) {
            grammaticalCaseValue = 1;
        } else if ([grammaticalCase isEqual:(id)OF_STR("dative")]) {
            grammaticalCaseValue = 2;
        } else if ([grammaticalCase isEqual:(id)OF_STR("genitive")]) {
            grammaticalCaseValue = 3;
        } else if ([grammaticalCase isEqual:(id)OF_STR("locative")]) {
            grammaticalCaseValue = 4;
        } else if ([grammaticalCase isEqual:(id)OF_STR("nominative")]) {
            grammaticalCaseValue = 5;
        } else if ([grammaticalCase isEqual:(id)OF_STR("partitive")]) {
            grammaticalCaseValue = 6;
        } else if ([grammaticalCase isEqual:(id)OF_STR("instrumental")]) {
            grammaticalCaseValue = 7;
        } else if ([grammaticalCase isEqual:(id)OF_STR("prepositional")]) {
            grammaticalCaseValue = 8;
        } else if ([grammaticalCase isEqual:(id)OF_STR("object")]) {
            grammaticalCaseValue = 9;
        } else if ([grammaticalCase isEqual:(id)OF_STR("directObject")]) {
            grammaticalCaseValue = 10;
        } else if ([grammaticalCase isEqual:(id)OF_STR("indirectObject")]) {
            grammaticalCaseValue = 11;
        } else {
            goto invalid;
        }
    }
    [self setGrammaticalCase:grammaticalCaseValue];

    id number = [dictionary objectForKeyedSubscript:(id)OF_STR("number")];
    NSInteger numberValue = 0;
    if (number && ![number isEqual:(id)OF_STR("none")]) {
        if ([number isEqual:(id)OF_STR("one")]) {
            numberValue = 1;
        } else if ([number isEqual:(id)OF_STR("two")]) {
            numberValue = 2;
        } else if ([number isEqual:(id)OF_STR("few")]) {
            numberValue = 3;
        } else if ([number isEqual:(id)OF_STR("many")]) {
            numberValue = 4;
        } else if ([number isEqual:(id)OF_STR("zero")]) {
            numberValue = 5;
        } else if ([number isEqual:(id)OF_STR("other")]) {
            numberValue = 6;
        } else {
            goto invalid;
        }
    }
    [self setNumber:numberValue];

    id person = [dictionary objectForKeyedSubscript:(id)OF_STR("person")];
    NSInteger personValue = 0;
    if (person && ![person isEqual:(id)OF_STR("none")]) {
        if ([person isEqual:(id)OF_STR("first")]) {
            personValue = 1;
        } else if ([person isEqual:(id)OF_STR("second")]) {
            personValue = 2;
        } else if ([person isEqual:(id)OF_STR("third")]) {
            personValue = 3;
        } else {
            goto invalid;
        }
    }
    [self setPerson:personValue];

    ONSMorphologyCustomPronoun *pronoun = nil;
    id<GrammarCollectionMessages> pronouns =
        [dictionary objectForKeyedSubscript:(id)OF_STR("customPronouns")];
    if (_ONSIsNSDictionary(pronouns) && [pronouns count]) {
        id<GrammarCollectionMessages> english = [pronouns objectForKeyedSubscript:(id)OF_STR("en")];
        if (!english || !_ONSIsNSDictionary(english)) {
            goto invalid;
        }
        id subject = [english objectForKeyedSubscript:(id)OF_STR("subjectForm")];
        id object = [english objectForKeyedSubscript:(id)OF_STR("objectForm")];
        id possessive = [english objectForKeyedSubscript:(id)OF_STR("possessiveForm")];
        id possessiveAdjective = [english objectForKeyedSubscript:(id)OF_STR("possessiveAdjectiveForm")];
        id reflexive = [english objectForKeyedSubscript:(id)OF_STR("reflexiveForm")];
        if (!_ONSIsNSString(subject) || !_ONSIsNSString(object) || !_ONSIsNSString(possessive) ||
            !_ONSIsNSString(possessiveAdjective) || !_ONSIsNSString(reflexive)) {
            goto invalid;
        }
        pronoun = objc_autorelease([[ONSMorphologyCustomPronoun alloc] init]);
        [pronoun setSubjectForm:subject];
        [pronoun setObjectForm:object];
        [pronoun setPossessiveForm:possessive];
        [pronoun setPossessiveAdjectiveForm:possessiveAdjective];
        [pronoun setReflexiveForm:reflexive];
        if (!pronoun) {
            goto invalid;
        }
    }
    _englishCustomPronoun = objc_retain(pronoun);
    return self;

invalid:
    if (error) {
        *error = [ONSError errorWithDomain:ONSCocoaErrorDomain code:3840 userInfo:nil];
    }
    objc_release(self);
    return nil;
}

// Native -[_NSAttributedStringGrammarInflection(NSExternalRepresentation) externalRepresentationDictionary],
// 0x1c062cc34. Non-external.
- (id)externalRepresentationDictionary {
    id<GrammarCollectionMessages> dictionary = objc_autorelease([[ONSMutableDictionary alloc] init]);
    id context = [self context] == 1 ? (id)OF_STR("technicalTerm") : nil;
    [dictionary setObject:context forKeyedSubscript:(id)OF_STR("context")];

    id genders[] = {
        (id)OF_STR("masculine"), (id)OF_STR("feminine"), (id)OF_STR("neuter"), (id)OF_STR("common")
    };
    NSUInteger gender = (NSUInteger)[self gender] - 1;
    [dictionary setObject:gender < 4 ? genders[gender] : nil
        forKeyedSubscript:(id)OF_STR("grammaticalGender")];

    id cases[] = {
        (id)OF_STR("accusative"),
        (id)OF_STR("dative"),
        (id)OF_STR("genitive"),
        (id)OF_STR("locative"),
        (id)OF_STR("nominative"),
        (id)OF_STR("partitive"),
        (id)OF_STR("instrumental"),
        (id)OF_STR("prepositional"),
        (id)OF_STR("object"),
        (id)OF_STR("directObject"),
        (id)OF_STR("indirectObject")
    };
    NSUInteger grammaticalCase = (NSUInteger)[self grammaticalCase] - 1;
    [dictionary setObject:grammaticalCase < 11 ? cases[grammaticalCase] : nil
        forKeyedSubscript:(id)OF_STR("grammaticalCase")];

    id numbers[] = {
        (id)OF_STR("one"),
        (id)OF_STR("two"),
        (id)OF_STR("few"),
        (id)OF_STR("many"),
        (id)OF_STR("zero"),
        (id)OF_STR("other")
    };
    NSUInteger number = (NSUInteger)[self number] - 1;
    [dictionary setObject:number < 6 ? numbers[number] : nil forKeyedSubscript:(id)OF_STR("number")];

    id people[] = {(id)OF_STR("first"), (id)OF_STR("second"), (id)OF_STR("third")};
    NSUInteger person = (NSUInteger)[self person] - 1;
    [dictionary setObject:person < 3 ? people[person] : nil forKeyedSubscript:(id)OF_STR("person")];

    ONSMorphologyCustomPronoun *pronoun = [self englishCustomPronoun];
    if ([pronoun subjectForm] && [pronoun objectForm] && [pronoun possessiveForm] &&
        [pronoun possessiveAdjectiveForm] && [pronoun reflexiveForm]) {
        id keys[] = {
            (id)OF_STR("subjectForm"),
            (id)OF_STR("objectForm"),
            (id)OF_STR("possessiveForm"),
            (id)OF_STR("possessiveAdjectiveForm"),
            (id)OF_STR("reflexiveForm")
        };
        id values[5];
        values[0] = [pronoun subjectForm];
        values[1] = [pronoun objectForm];
        values[2] = [pronoun possessiveForm];
        values[3] = [pronoun possessiveAdjectiveForm];
        values[4] = [pronoun reflexiveForm];
        id english = [(id<GrammarCollectionMessages>)grammarDictionaryClass dictionaryWithObjects:values
                                                                                          forKeys:keys
                                                                                            count:5];
        id language = (id)OF_STR("en");
        id pronouns = [(id<GrammarCollectionMessages>)grammarDictionaryClass dictionaryWithObjects:&english
                                                                                           forKeys:&language
                                                                                             count:1];
        if (pronouns) {
            [dictionary setObject:pronouns forKeyedSubscript:(id)OF_STR("customPronouns")];
        }
    }
    return dictionary;
}

// Native -[_NSAttributedStringGrammarInflection(NSExternalRepresentation) externalRepresentation],
// 0x1c062ceb4. Non-external.
- (id)externalRepresentation {
    return [ONSJSONSerialization dataWithJSONObject:[self externalRepresentationDictionary]
                                            options:2
                                              error:NULL];
}

@end
