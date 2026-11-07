//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringMorphun.h"

#include "CoreFoundation/String/OCFString.h"
#import "Foundation/String/ONSAttributedStringInternal.h"
#import "Foundation/String/ONSAttributedStringInflection.h"
#import "Foundation/String/ONSCharacterSet.h"
#import "CoreFoundation/Objective-C/OCFNSEmptyCollections.h"
#import "CoreFoundation/Objective-C/OCFNSMutableArray.h"
#import "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"
#import "Foundation/String/ONSLexicon.h"
#import "Foundation/String/ONSLexiconMorphunDictionary.h"
#import "Foundation/String/ONSMorphologyMorphunConstraints.h"
#import "Foundation/String/ONSMorphologyPronoun.h"
#import "Foundation/String/ONSTermOfAddress.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

#include <Block.h>
#include <assert.h>
#include <dlfcn.h>
#include <OpenFoundation/Range/ONSRange.h>

#import <objc/NSObject.h>

// Inferred organization: Foundation's NSMorphology(_Morphun) unit spans 0x1c057ed5c-0x1c057ff5f of the iOS
// Simulator __text section: _NSRequiresMorphunInflectionForLanguageIdentifier (0x1c057ed5c),
// _NSLanguageIdentifierBeginsWithLanguageCode (0x1c057ee30), -[NSMorphology(_Morphun)
// initWithMorphunAttributeValues:] (0x1c057eef8), -[NSMorphology(_Morphun) addConstraintsTo:] (0x1c057f368)
// and _NSLexiconInflectedStringUsingMorphun (0x1c057f55c); the _NSLexiconMorphunDictionary methods follow.
// The two language-code functions and the captured inflection caller are restored here. The loader and
// the engine function-pointer state remain undefined (the initializer is absent from the capture).
// Objective-C compiled
// with manual reference counting. Evidence: Local/Evidence/cf-core/iphone-simulator-foundation-link.json
// (family FC-sets-attributed). Unless a definition says otherwise: iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Source/Foundation/Collections/FoundationSetsAttributedReading.md.

// The messages the functions send to strings, declared for the sends only; no protocol object is emitted.
@protocol MorphunLanguageMessages

- (NSUInteger)length;
- (NSInteger)compare:(id)string options:(NSUInteger)mask range:(ONSRange)range;
- (unsigned short)characterAtIndex:(NSUInteger)index;

@end

static BOOL _NSLanguageIdentifierBeginsWithLanguageCode(id identifier, id code);

// Native _NSRequiresMorphunInflectionForLanguageIdentifier, 0x1c057ed5c. Was a private external. Tests "en",
// "es", "it", "fr", "pt", "de", "ko", "hi" and "nl" in that order (constant strings).
BOOL _NSRequiresMorphunInflectionForLanguageIdentifier(id languageIdentifier) {
    if (_NSLanguageIdentifierBeginsWithLanguageCode(languageIdentifier, (id)OF_STR("en")) ||
        _NSLanguageIdentifierBeginsWithLanguageCode(languageIdentifier, (id)OF_STR("es")) ||
        _NSLanguageIdentifierBeginsWithLanguageCode(languageIdentifier, (id)OF_STR("it")) ||
        _NSLanguageIdentifierBeginsWithLanguageCode(languageIdentifier, (id)OF_STR("fr")) ||
        _NSLanguageIdentifierBeginsWithLanguageCode(languageIdentifier, (id)OF_STR("pt")) ||
        _NSLanguageIdentifierBeginsWithLanguageCode(languageIdentifier, (id)OF_STR("de")) ||
        _NSLanguageIdentifierBeginsWithLanguageCode(languageIdentifier, (id)OF_STR("ko")) ||
        _NSLanguageIdentifierBeginsWithLanguageCode(languageIdentifier, (id)OF_STR("hi"))) {
        return YES;
    }
    return _NSLanguageIdentifierBeginsWithLanguageCode(languageIdentifier, (id)OF_STR("nl"));
}

// Native _NSLanguageIdentifierBeginsWithLanguageCode, 0x1c057ee30. Non-external. A case-insensitive
// (options 1) prefix comparison, true when the identifier is exactly the code or continues with '-' or '_'.
static BOOL _NSLanguageIdentifierBeginsWithLanguageCode(id identifier, id code) {
    if ([identifier length] < [code length]) {
        return NO;
    }
    if ([identifier compare:code options:1 range:(ONSRange){0, [code length]}] != 0) {
        return NO;
    }
    if ([identifier length] == [code length]) {
        return YES;
    }
    if ([identifier characterAtIndex:[code length]] == '-') {
        return YES;
    }
    return [identifier characterAtIndex:[code length]] == '_';
}

// These message signatures preserve the 16-byte NSRange return in X0/X1 and the 32-bit constraints
// result. Classes and functions outside this family are declared in ONSUnrestoredInterfaces.h.
@protocol MorphunInflectionMessages
+ (id)localeWithLocaleIdentifier:(id)identifier;
- (NSUInteger)count;
- (ONSRange)rangeOfCharacterFromSet:(id)set;
- (ONSRange)rangeOfCharacterFromSet:(id)set options:(NSUInteger)options range:(ONSRange)range;
- (ONSRange)replacementRangeInResult;
- (NSInteger)replacementKind;
- (id)argument;
- (id)substringWithRange:(ONSRange)range;
- (id)substringFromIndex:(NSUInteger)index;
- (id)stringByAppendingString:(id)string;
- (BOOL)addConstraintsTo:(void *)inflectable;
@end

extern char morphunLocaleClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSLocale));

// Native ___block_literal_global (IDA ___block_literal_global_29), 0x22281cdd8. Static data: global
// block flags 0x50000000, invoke 0x1c0582224, descriptor 0x2227d03f8 {0x50000000, 32, "v8@?0", NULL}.
static const struct {
    uintptr_t reserved;
    uintptr_t size;
    const char *signature;
    const char *layout;
} kMorphunLoadDescriptor = {0x50000000, 32, "v8@?0", NULL};

static const struct {
    void *isa;
    int32_t flags;
    int32_t reserved;
    void (*invoke)(void *);
    const void *descriptor;
} kMorphunLoadBlock = {_NSConcreteGlobalBlock, 0x50000000, 0, morphunLoadInvoke, &kMorphunLoadDescriptor};

// The error-path sequence inlined after each engine call, not another restored native function. The
// packed 22-byte buffer exactly follows 0x1c057f988 etc.: the native descriptors for a C string then an
// object.
static inline void morphunLogError(const char *operation, OCFTypeRef error) {
    os_log_t log = _NSInflectionLog();
    if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
        struct __attribute__((packed)) {
            uint32_t header;
            const char *operation;
            uint16_t objectDescriptor;
            OCFTypeRef error;
        } buffer = {0x08200202, operation, 0x0840, error};

        _os_log_error_impl(
            &__dso_handle,
            log,
            OS_LOG_TYPE_ERROR,
            "Inflection engine reported an error for '%s' while attempting to inflect: %@",
            (uint8_t *)&buffer,
            22
        );
    }
}

// Morphun descriptor keys and values come from the constant dictionaries at 0x22289c010–0x22289c358.
// Each dictionary has options 1, two shared string keys, and two NSConstantIntegerNumber values.
extern char morphunConstantIntegerClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                    OF_NAME_STRING(ONSConstantIntegerNumber));
extern char morphunConstantDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                       OF_NAME_STRING(ONSConstantDictionary));

#define MORPHUN_STRING(text) {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, text, sizeof(text) - 1}

static const OCFConstantString descriptorFieldKey = MORPHUN_STRING("f");
static const OCFConstantString descriptorValueKey = MORPHUN_STRING("v");
static const id descriptorKeys[] = {(id)&descriptorFieldKey, (id)&descriptorValueKey};

typedef struct {
    const void *isa;
    const char *type;
    int64_t value;
} MorphunConstantInteger;

typedef struct {
    const void *isa;
    NSUInteger options;
    NSUInteger count;
    const id *keys;
    const id *objects;
} MorphunConstantDictionary;

// Native constant dictionary 0x22289c010.
static const MorphunConstantInteger abbreviationValues[] = {
    {morphunConstantIntegerClass, "i", 0}, {morphunConstantIntegerClass, "q", 14}
};
static const id abbreviationObjects[] = {(id)&abbreviationValues[0], (id)&abbreviationValues[1]};
static const MorphunConstantDictionary abbreviationDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, abbreviationObjects
};

// Native constant dictionary 0x22289c038.
static const MorphunConstantInteger adjectiveValues[] = {
    {morphunConstantIntegerClass, "i", 0}, {morphunConstantIntegerClass, "q", 6}
};
static const id adjectiveObjects[] = {(id)&adjectiveValues[0], (id)&adjectiveValues[1]};
static const MorphunConstantDictionary adjectiveDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, adjectiveObjects
};

// Native constant dictionary 0x22289c060.
static const MorphunConstantInteger adverbValues[] = {
    {morphunConstantIntegerClass, "i", 0}, {morphunConstantIntegerClass, "q", 4}
};
static const id adverbObjects[] = {(id)&adverbValues[0], (id)&adverbValues[1]};
static const MorphunConstantDictionary adverbDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, adverbObjects
};

// Native constant dictionary 0x22289c088.
static const MorphunConstantInteger conjunctionValues[] = {
    {morphunConstantIntegerClass, "i", 0}, {morphunConstantIntegerClass, "q", 10}
};
static const id conjunctionObjects[] = {(id)&conjunctionValues[0], (id)&conjunctionValues[1]};
static const MorphunConstantDictionary conjunctionDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, conjunctionObjects
};

// Native constant dictionary 0x22289c0b0.
static const MorphunConstantInteger interjectionValues[] = {
    {morphunConstantIntegerClass, "i", 0}, {morphunConstantIntegerClass, "q", 12}
};
static const id interjectionObjects[] = {(id)&interjectionValues[0], (id)&interjectionValues[1]};
static const MorphunConstantDictionary interjectionDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, interjectionObjects
};

// Native constant dictionary 0x22289c0d8.
static const MorphunConstantInteger letterValues[] = {
    {morphunConstantIntegerClass, "i", 0}, {morphunConstantIntegerClass, "q", 3}
};
static const id letterObjects[] = {(id)&letterValues[0], (id)&letterValues[1]};
static const MorphunConstantDictionary letterDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, letterObjects
};

// Native constant dictionary 0x22289c100.
static const MorphunConstantInteger nounValues[] = {
    {morphunConstantIntegerClass, "i", 0}, {morphunConstantIntegerClass, "q", 9}
};
static const id nounObjects[] = {(id)&nounValues[0], (id)&nounValues[1]};
static const MorphunConstantDictionary nounDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, nounObjects
};

// Native constant dictionary 0x22289c128.
static const MorphunConstantInteger numberValues[] = {
    {morphunConstantIntegerClass, "i", 0}, {morphunConstantIntegerClass, "q", 11}
};
static const id numberObjects[] = {(id)&numberValues[0], (id)&numberValues[1]};
static const MorphunConstantDictionary numberDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, numberObjects
};

// Native constant dictionary 0x22289c150.
static const MorphunConstantInteger particleValues[] = {
    {morphunConstantIntegerClass, "i", 0}, {morphunConstantIntegerClass, "q", 5}
};
static const id particleObjects[] = {(id)&particleValues[0], (id)&particleValues[1]};
static const MorphunConstantDictionary particleDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, particleObjects
};

// Native constant dictionary 0x22289c178.
static const MorphunConstantInteger pronounValues[] = {
    {morphunConstantIntegerClass, "i", 0}, {morphunConstantIntegerClass, "q", 2}
};
static const id pronounObjects[] = {(id)&pronounValues[0], (id)&pronounValues[1]};
static const MorphunConstantDictionary pronounDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, pronounObjects
};

// Native constant dictionary 0x22289c1a0.
static const MorphunConstantInteger verbValues[] = {
    {morphunConstantIntegerClass, "i", 0}, {morphunConstantIntegerClass, "q", 8}
};
static const id verbObjects[] = {(id)&verbValues[0], (id)&verbValues[1]};
static const MorphunConstantDictionary verbDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, verbObjects
};

// Native constant dictionary 0x22289c1c8.
static const MorphunConstantInteger feminineValues[] = {
    {morphunConstantIntegerClass, "i", 1}, {morphunConstantIntegerClass, "q", 1}
};
static const id feminineObjects[] = {(id)&feminineValues[0], (id)&feminineValues[1]};
static const MorphunConstantDictionary feminineDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, feminineObjects
};

// Native constant dictionary 0x22289c1f0.
static const MorphunConstantInteger masculineValues[] = {
    {morphunConstantIntegerClass, "i", 1}, {morphunConstantIntegerClass, "q", 2}
};
static const id masculineObjects[] = {(id)&masculineValues[0], (id)&masculineValues[1]};
static const MorphunConstantDictionary masculineDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, masculineObjects
};

// Native constant dictionary 0x22289c218.
static const MorphunConstantInteger neuterValues[] = {
    {morphunConstantIntegerClass, "i", 1}, {morphunConstantIntegerClass, "q", 3}
};
static const id neuterObjects[] = {(id)&neuterValues[0], (id)&neuterValues[1]};
static const MorphunConstantDictionary neuterDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, neuterObjects
};

// Native constant dictionary 0x22289c240.
static const MorphunConstantInteger singularValues[] = {
    {morphunConstantIntegerClass, "i", 2}, {morphunConstantIntegerClass, "q", 1}
};
static const id singularObjects[] = {(id)&singularValues[0], (id)&singularValues[1]};
static const MorphunConstantDictionary singularDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, singularObjects
};

// Native constant dictionary 0x22289c268.
static const MorphunConstantInteger pluralValues[] = {
    {morphunConstantIntegerClass, "i", 2}, {morphunConstantIntegerClass, "q", 3}
};
static const id pluralObjects[] = {(id)&pluralValues[0], (id)&pluralValues[1]};
static const MorphunConstantDictionary pluralDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, pluralObjects
};

// Native constant dictionary 0x22289c290.
static const MorphunConstantInteger accusativeValues[] = {
    {morphunConstantIntegerClass, "i", 4}, {morphunConstantIntegerClass, "q", 2}
};
static const id accusativeObjects[] = {(id)&accusativeValues[0], (id)&accusativeValues[1]};
static const MorphunConstantDictionary accusativeDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, accusativeObjects
};

// Native constant dictionary 0x22289c2b8.
static const MorphunConstantInteger dativeValues[] = {
    {morphunConstantIntegerClass, "i", 4}, {morphunConstantIntegerClass, "q", 3}
};
static const id dativeObjects[] = {(id)&dativeValues[0], (id)&dativeValues[1]};
static const MorphunConstantDictionary dativeDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, dativeObjects
};

// Native constant dictionary 0x22289c2e0.
static const MorphunConstantInteger genitiveValues[] = {
    {morphunConstantIntegerClass, "i", 4}, {morphunConstantIntegerClass, "q", 4}
};
static const id genitiveObjects[] = {(id)&genitiveValues[0], (id)&genitiveValues[1]};
static const MorphunConstantDictionary genitiveDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, genitiveObjects
};

// Native constant dictionary 0x22289c308.
static const MorphunConstantInteger locativeValues[] = {
    {morphunConstantIntegerClass, "i", 4}, {morphunConstantIntegerClass, "q", 13}
};
static const id locativeObjects[] = {(id)&locativeValues[0], (id)&locativeValues[1]};
static const MorphunConstantDictionary locativeDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, locativeObjects
};

// Native constant dictionary 0x22289c330.
static const MorphunConstantInteger nominativeValues[] = {
    {morphunConstantIntegerClass, "i", 4}, {morphunConstantIntegerClass, "q", 1}
};
static const id nominativeObjects[] = {(id)&nominativeValues[0], (id)&nominativeValues[1]};
static const MorphunConstantDictionary nominativeDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, nominativeObjects
};

// Native constant dictionary 0x22289c358.
static const MorphunConstantInteger prepositionalValues[] = {
    {morphunConstantIntegerClass, "i", 4}, {morphunConstantIntegerClass, "q", 5}
};
static const id prepositionalObjects[] = {(id)&prepositionalValues[0], (id)&prepositionalValues[1]};
static const MorphunConstantDictionary prepositionalDescriptor = {
    morphunConstantDictionaryClass, 1, 2, descriptorKeys, prepositionalObjects
};

static const MorphunConstantInteger properNounField = {morphunConstantIntegerClass, "i", 0};
static const MorphunConstantInteger commonField = {morphunConstantIntegerClass, "i", 1};

@implementation ONSMorphology (_Morphun)

// Native -[NSMorphology(_Morphun) initWithMorphunAttributeValues:], 0x1c057eef8. Non-external.
- (id)initWithMorphunAttributeValues:(id)values {
    self = [super init];
    if (self == nil) {
        return nil;
    }
    const id properNoun[] = {(id)&properNounField, [ONSNumber numberWithInteger:15]};
    id properNounDescriptor = [ONSDictionary dictionaryWithObjects:properNoun forKeys:descriptorKeys count:2];
    const id common[] = {(id)&commonField, [ONSNumber numberWithInteger:4]};
    id commonDescriptor = [ONSDictionary dictionaryWithObjects:common forKeys:descriptorKeys count:2];

    const id keys[] = {
        (id)OF_STR("abbreviation"), (id)OF_STR("adjective"),    (id)OF_STR("adverb"),
        (id)OF_STR("conjunction"),  (id)OF_STR("interjection"), (id)OF_STR("letter"),
        (id)OF_STR("noun"),         (id)OF_STR("proper-noun"),  (id)OF_STR("number"),
        (id)OF_STR("particle"),     (id)OF_STR("pronoun"),      (id)OF_STR("verb"),
        (id)OF_STR("feminine"),     (id)OF_STR("masculine"),    (id)OF_STR("neuter"),
        (id)OF_STR("common"),       (id)OF_STR("singular"),     (id)OF_STR("plural"),
        (id)OF_STR("accusative"),   (id)OF_STR("dative"),       (id)OF_STR("genitive"),
        (id)OF_STR("locative"),     (id)OF_STR("nominative"),   (id)OF_STR("prepositional"),
    };
    const id descriptors[] = {
        (id)&abbreviationDescriptor, (id)&adjectiveDescriptor,
        (id)&adverbDescriptor,       (id)&conjunctionDescriptor,
        (id)&interjectionDescriptor, (id)&letterDescriptor,
        (id)&nounDescriptor,         properNounDescriptor,
        (id)&numberDescriptor,       (id)&particleDescriptor,
        (id)&pronounDescriptor,      (id)&verbDescriptor,
        (id)&feminineDescriptor,     (id)&masculineDescriptor,
        (id)&neuterDescriptor,       commonDescriptor,
        (id)&singularDescriptor,     (id)&pluralDescriptor,
        (id)&accusativeDescriptor,   (id)&dativeDescriptor,
        (id)&genitiveDescriptor,     (id)&locativeDescriptor,
        (id)&nominativeDescriptor,   (id)&prepositionalDescriptor,
    };
    id mapping = [ONSDictionary dictionaryWithObjects:descriptors forKeys:keys count:24];
    for (id attribute in values) {
        id descriptor = [mapping objectForKeyedSubscript:attribute];
        if (descriptor == nil) {
            continue;
        }
        switch ([[descriptor objectForKeyedSubscript:(id)&descriptorFieldKey] unsignedIntegerValue]) {
            case 0:
                [self setPartOfSpeech:[[descriptor objectForKeyedSubscript:(id)&descriptorValueKey]
                                          unsignedIntegerValue]];
                break;
            case 1:
                [self setGrammaticalGender:[[descriptor objectForKeyedSubscript:(id)&descriptorValueKey]
                                               unsignedIntegerValue]];
                break;
            case 2:
                [self setNumber:[[descriptor objectForKeyedSubscript:(id)&descriptorValueKey]
                                    unsignedIntegerValue]];
                break;
            case 4:
                [self setGrammaticalCase:[[descriptor objectForKeyedSubscript:(id)&descriptorValueKey]
                                             unsignedIntegerValue]];
                break;
        }
    }
    return self;
}

// Native -[NSMorphology(_Morphun) addConstraintsTo:], 0x1c057f368. Non-external.
- (BOOL)addConstraintsTo:(void *)inflectable {
    id constraints = [self _morphunConstraints];
    BOOL hasNonPOSValue = NO;
    for (id key in constraints) {
        id value = [constraints objectForKeyedSubscript:key];
        OCFTypeRef error = NULL;
        morphunLexiconFunctions.minf_putConstraintByName(
            inflectable, (OCFStringRef)key, (OCFStringRef)value, &error
        );
        if (error != NULL) {
            os_log_info(
                _NSInflectionLog(),
                "Unable to set constraint %{public}@=%{public}@: %{public}@",
                key,
                value,
                (id)error
            );
            OCFRelease(error);
        }
        // The native instruction compares the value, even when the key is "pos" (kept).
        hasNonPOSValue |= ![value isEqualToString:(id)OF_STR("pos")];
    }
    return hasNonPOSValue;
}

@end

// Native __NSLexiconInflectedStringUsingMorphun, 0x1c057f55c. Was a private external. The captured length
// argument is unused. An absent provider function after the native soft-link attempt takes the
// nil-result path. The replacement-prefix scan
// keeps the original first-nonwhitespace offset across replacements, as the assembly does (kept).
id _NSLexiconInflectedStringUsingMorphun(
    id string,
    NSUInteger location,
    NSUInteger length,
    id languageIdentifier,
    id morphology,
    id featureName,
    id replacements
) {
    (void)length;
    dispatch_once(&morphunLoadOnceToken, (dispatch_block_t)(void *)&kMorphunLoadBlock);
    if (morphunFunctions.getDefaultProvider == NULL) {
        return nil;
    }
    id locale = [(id)morphunLocaleClass localeWithLocaleIdentifier:languageIdentifier];
    if (locale == nil) {
        return nil;
    }
    id prefix = nil;
    if ([replacements count] != 0) {
        id nonWhitespace = [[ONSCharacterSet whitespaceAndNewlineCharacterSet] invertedSet];
        ONSRange first = [string rangeOfCharacterFromSet:nonWhitespace];
        if (first.location == 0x7FFFFFFFFFFFFFFF) {
            return nil;
        }
        NSUInteger replacementStart = first.location + location;
        for (id replacement in replacements) {
            if ([replacement replacementKind] != 1 && !_ONSIsNSNumber([replacement argument])) {
                continue;
            }
            ONSRange range = [replacement replacementRangeInResult];
            if (range.location != replacementStart) {
                continue;
            }
            NSUInteger stringLength = [string length];
            if (range.length >= stringLength) {
                return nil;
            }
            NSUInteger boundary = first.location + range.length;
            stringLength = [string length];
            ONSRange next = [string rangeOfCharacterFromSet:nonWhitespace
                                                    options:0
                                                      range:(ONSRange){boundary, stringLength - boundary}];
            NSUInteger split = next.location > boundary ? next.location : boundary;
            if (next.location == 0x7FFFFFFFFFFFFFFF) {
                split = boundary;
            }
            prefix = [string substringWithRange:(ONSRange){0, split}];
            string = [string substringFromIndex:split];
        }
    }
    OCFTypeRef error = NULL;
    void *provider = morphunFunctions.getDefaultProvider(&error);
    if (error != NULL) {
        morphunLogError("mlccfp_getDefaultCommonConceptFactoryProvider(&error)", error);
    }
    if (provider == NULL || error != NULL) {
        if (error != NULL) {
            OCFRelease(error);
        }
        return nil;
    }
    error = NULL;
    void *factory = morphunFunctions.getCommonFactory(provider, (OCFTypeRef)locale, &error);
    if (error != NULL) {
        morphunLogError("mlccfp_getCommonConceptFactory(provider, (CFLocaleRef)locale, &error)", error);
    }
    if (factory == NULL || error != NULL) {
        if (error != NULL) {
            OCFRelease(error);
        }
        return nil;
    }
    error = NULL;
    void *model = morphunFunctions.getSemanticModel(factory, &error);
    if (error != NULL) {
        morphunLogError("mccf_getSemanticFeatureModel(factory, &error)", error);
    }
    if (model == NULL || error != NULL) {
        if (error != NULL) {
            OCFRelease(error);
        }
        return nil;
    }
    error = NULL;
    void *source = morphunFunctions.createSpeakable((OCFStringRef)string, &error);
    if (error != NULL) {
        morphunLogError("mss_create((CFStringRef)stringToInflect, &error)", error);
    }
    id result = nil;
    if (source != NULL && error == NULL) {
        error = NULL;
        void *inflectable = morphunFunctions.createInflectable(model, source, &error);
        if (error != NULL) {
            morphunLogError("minf_create(model, speakableSource, &error)", error);
        }
        if (inflectable != NULL && error == NULL) {
            BOOL constrained = [morphology addConstraintsTo:inflectable];
            BOOL produceResult = YES;
            if (constrained != 0) {
                error = NULL;
                uint8_t exists = morphunFunctions.inflectionExists(inflectable, &error);
                if (error != NULL) {
                    morphunLogError("minf_isExists(inflectable, &error)", error);
                }
                produceResult = error == NULL && (exists & 1) != 0;
            } else if (featureName == nil) {
                error = NULL;
                produceResult = NO;
            }
            if (produceResult) {
                error = NULL;
                void *speakable;
                if (featureName != nil) {
                    speakable = morphunFunctions.copyFeature(inflectable, (OCFStringRef)featureName, &error);
                    if (error != NULL) {
                        morphunLogError(
                            "minf_getFeatureValueByNameCopy(inflectable,(CFStringRef)featureName, &error)",
                            error
                        );
                    }
                } else {
                    speakable = morphunFunctions.toSpeakable(inflectable, &error);
                    if (error != NULL) {
                        morphunLogError("minf_toSpeakableString(inflectable, &error)", error);
                    }
                }
                if (speakable != NULL && error == NULL) {
                    error = NULL;
                    OCFStringRef copy = morphunFunctions.copyPrint(speakable, &error);
                    if (error != NULL) {
                        morphunLogError("mss_createPrintCopy(speakableResult, &error)", error);
                    }
                    // Native 0x1c057fcc4 still autoreleases and returns a nonnil copy with an error;
                    // the CF error is released only after the three engine cleanup calls (kept).
                    result = objc_autorelease((id)copy);
                    if (prefix != nil && result != nil) {
                        result = [prefix stringByAppendingString:result];
                    }
                }
                if (speakable != NULL) {
                    morphunFunctions.releaseSpeakable(speakable);
                }
            }
        }
        if (inflectable != NULL) {
            morphunFunctions.releaseInflectable(inflectable);
        }
    }
    if (source != NULL) {
        morphunFunctions.releaseSpeakable(source);
    }
    if (error != NULL) {
        OCFRelease(error);
    }
    return result;
}

// Native analyzer factory state 0x22e76c880 and its once predicate 0x22e76c888.
static void *morphunAnalyzerFactory;
static dispatch_once_t morphunAnalyzerFactoryOnce;

// Inlined log-and-release paths shared by the native dictionary initializer and word visitor.
#define MORPHUN_REPORT_ERROR(error, operation)                                                               \
    do {                                                                                                     \
        morphunLogError(operation, error);                                                                   \
        OCFRelease(error);                                                                                   \
    } while (0)

@implementation _ONSLexiconMorphunDictionary

// Native -[_NSLexiconMorphunDictionary initWithLanguage:], 0x1c057ff60. Non-external.
- (id)initWithLanguage:(id)language {
    self = [super init];
    if (self == nil) {
        return nil;
    }
    dispatch_once(&morphunLoadOnceToken, (dispatch_block_t)(void *)&kMorphunLoadBlock);
    if (morphunFunctions.getDefaultProvider == NULL ||
        !_NSRequiresMorphunInflectionForLanguageIdentifier(language)) {
        goto unavailable;
    }

    dispatch_once(&morphunLoadOnceToken, (dispatch_block_t)(void *)&kMorphunLoadBlock);
    if (morphunFunctions.getDefaultProvider == NULL) {
        __assert_rtn(
            "_NSLexiconGetMorphunAnalyzerFactory",
            "NSLexiconMorphun.m",
            565,
            "_NSLexiconLoadMorphunIfNeeded()"
        );
    }
    // Native ____NSLexiconGetMorphunAnalyzerFactory_block_invoke, 0x1c0582d18. Non-external.
    dispatch_once(&morphunAnalyzerFactoryOnce, ^{
      OCFTypeRef error = NULL;
      morphunAnalyzerFactory = morphunLexiconFunctions.mdaf_create(33, &error);
      if (morphunAnalyzerFactory == NULL && error != NULL) {
          os_log_error(_NSInflectionLog(), "Cannot initialize analyzer factory: %{public}@", (id)error);
          OCFRelease(error);
      }
    });
    if (morphunAnalyzerFactory == NULL) {
        goto unavailable;
    }

    id locale = [(id<MorphunInflectionMessages>)morphunLocaleClass localeWithLocaleIdentifier:language];
    void *analyzer = NULL;
    void *tokenizer = NULL;
    void *dictionary = NULL;
    if (locale != nil) {
        OCFTypeRef error = NULL;
        analyzer = morphunLexiconFunctions.mdaf_getAnalyzer(morphunAnalyzerFactory, locale, &error);
        if (error != NULL) {
            os_log_error(
                _NSInflectionLog(),
                "Cannot use inflection engine for language %{public}@ (Getting analyzer from factory): "
                "%{public}@",
                language,
                (id)error
            );
            OCFRelease(error);
        }
        if (analyzer != NULL) {
            error = NULL;
            tokenizer = morphunLexiconFunctions.mtok_createTokenizer(locale, &error);
            if (error != NULL) {
                os_log_error(
                    _NSInflectionLog(),
                    "Cannot use inflection engine for language %{public}@ (Creating a tokenizer): %{public}@",
                    language,
                    (id)error
                );
                OCFRelease(error);
            }
            error = NULL;
            morphunLexiconFunctions.mtok_setStyle(tokenizer, 2, &error);
            if (error != NULL) {
                os_log_error(
                    _NSInflectionLog(),
                    "Cannot use inflection engine for language %{public}@ (Setting the tokenizer to use the "
                    "unmodified style): %{public}@",
                    language,
                    (id)error
                );
                OCFRelease(error);
                morphunLexiconFunctions.mtok_destroy(tokenizer);
                tokenizer = NULL;
            } else if (tokenizer != NULL) {
                error = NULL;
                dictionary = morphunLexiconFunctions.mddmd_getDictionary(locale, &error);
                if (error != NULL) {
                    os_log_error(
                        _NSInflectionLog(),
                        "Cannot use inflection engine for language %{public}@ (Getting the dictionary for "
                        "this locale): %{public}@",
                        language,
                        (id)error
                    );
                    OCFRelease(error);
                }
            }
        }
    }

    _languageCode = [[language lowercaseString] copy];
    if (tokenizer != NULL && dictionary != NULL) {
        _analyzer = analyzer;
        _tokenizer = tokenizer;
        _dictionary = dictionary;
        _appropriateLocale = [locale copy];
        return self;
    }
    // Foundation defect: the failure path at 0x1c05801e8 releases self before the tokenizer is stored.
    if (tokenizer != NULL) {
        morphunLexiconFunctions.mtok_destroy(tokenizer);
    }

unavailable:
    objc_release(self);
    return nil;
}

// Native -[_NSLexiconMorphunDictionary dealloc], 0x1c058041c. Non-external.
- (void)dealloc {
    if (_tokenizer != NULL) {
        morphunLexiconFunctions.mtok_destroy(_tokenizer);
    }
    objc_release(_languageCode);
    objc_release(_appropriateLocale);
    [super dealloc];
}

// Native -[_NSLexiconMorphunDictionary allPossibleWordAttributesForWord:], 0x1c05804ac. Non-external.
// iOS Device divergence: 0x180f3fb58 has an additional byref cleanup tail at 0x180f3fc70 (verified).
- (id)allPossibleWordAttributesForWord:(id)word {
    // This byref variable emits ___Block_byref_object_copy__11 (0x1c0fde520)
    // and ___Block_byref_object_dispose__11 (0x1c0fde530), with flags 131.
    __block id result = nil;
    __unsafe_unretained _ONSLexiconMorphunDictionary *dictionary = self;
    // Native ___64-[_NSLexiconMorphunDictionary allPossibleWordAttributesForWord:]_block_invoke,
    // 0x1c05805b0. Non-external.
    _NSLexiconIterateFormsOfWord(
        word, _appropriateLocale, ^(id form, LexiconWordCase restoreCase, BOOL *stop) {
          (void)restoreCase;
          OCFTypeRef error = NULL;
          void *tokenChain = morphunLexiconFunctions.mtok_createTokenChain(
              dictionary->_tokenizer, (OCFStringRef)form, &error
          );
          if (error != NULL) {
              os_log_error(
                  _NSInflectionLog(),
                  "Cannot use inflection engine (Obtaining the token chain from a word): %{public}@",
                  (id)error
              );
              OCFRelease(error);
          }
          if (tokenChain == NULL) {
              return;
          }

          error = NULL;
          void *annotated =
              morphunLexiconFunctions.mca_createTokenChain(dictionary->_analyzer, tokenChain, &error);
          if (error != NULL) {
              os_log_error(
                  _NSInflectionLog(),
                  "Cannot use inflection engine (Annotating the token chain): %{public}@",
                  (id)error
              );
              OCFRelease(error);
          }
          if (annotated == NULL) {
              morphunLexiconFunctions.mtokc_destroy(tokenChain);
              return;
          }

          error = NULL;
          void *token = morphunLexiconFunctions.mtokc_getToken(annotated, 0, &error);
          if (error != NULL) {
              os_log_error(
                  _NSInflectionLog(),
                  "Cannot use inflection engine (Obtaining the first token): %{public}@",
                  (id)error
              );
              OCFRelease(error);
          }
          error = NULL;
          uint32_t correctType = morphunLexiconFunctions.mtok_isType(token, 6, &error);
          if (error != NULL) {
              os_log_error(
                  _NSInflectionLog(),
                  "Cannot use inflection engine (Checking that the token is the correct type): %{public}@",
                  (id)error
              );
              OCFRelease(error);
              goto destroyChains;
          }
          if (token == NULL || correctType != 1) {
              goto destroyChains;
          }

          error = NULL;
          void *wordToken = morphunLexiconFunctions.matmwt_getMorphologicalWordToken(token, &error);
          if (error != NULL) {
              os_log_error(
                  _NSInflectionLog(),
                  "Cannot use inflection engine (Casting a token to obtain a morphological word token): "
                  "%{public}@",
                  (id)error
              );
              OCFRelease(error);
          }
          if (wordToken == NULL) {
              goto destroyChains;
          }
          error = NULL;
          OCFArrayRef lemmas = morphunLexiconFunctions.matmwt_createLemmaMappings(wordToken, &error);
          if (error != NULL) {
              os_log_error(
                  _NSInflectionLog(),
                  "Cannot use inflection engine (Obtaining the lemma mappings for a morphological word "
                  "token): %{public}@",
                  (id)error
              );
              OCFRelease(error);
          }
          if (lemmas == NULL) {
              goto destroyChains;
          }

          id attributes = objc_alloc_init([ONSMutableArray class]);
          OCFIndex count = OCFArrayGetCount(lemmas);
          for (OCFIndex index = 0; index < count; index++) {
              const void *lemma = OCFArrayGetValueAtIndex(lemmas, index);
              error = NULL;
              uint64_t grammemes = morphunLexiconFunctions.matlm_getSurfaceFormGrammemes(lemma, &error);
              if (error != NULL) {
                  os_log_error(
                      _NSInflectionLog(),
                      "Cannot use inflection engine (Obtaining surface form grammemes): %{public}@",
                      (id)error
                  );
                  OCFRelease(error);
                  continue;
              }
              id propertyNames = morphunLexiconFunctions.mddmd_createPropertyNames(
                  dictionary->_dictionary, grammemes, &error
              );
              if (error != NULL) {
                  os_log_error(
                      _NSInflectionLog(),
                      "Cannot use inflection engine (Obtaining dictionary names for grammemes): %{public}@",
                      (id)error
                  );
                  OCFRelease(error);
              } else {
                  ONSMorphology *morphology =
                      [objc_alloc([ONSMorphology class]) initWithMorphunAttributeValues:propertyNames];
                  if ([morphology partOfSpeech] != 15) {
                      [attributes addObject:morphology];
                  }
                  objc_release(morphology);
              }
              objc_release(propertyNames);
          }
          if ([attributes count] != 0) {
              result = attributes;
              *stop = YES;
          } else {
              objc_release(attributes);
          }
          OCFRelease(lemmas);

      destroyChains:
          morphunLexiconFunctions.mtokc_destroy(annotated);
          morphunLexiconFunctions.mtokc_destroy(tokenChain);
        }
    );
    return result != nil ? objc_autorelease(result) : __ONSArray0__;
}

// Native __DisplayValueRelease, 0x1c0580c94. Non-external.
static void _DisplayValueRelease(OCFAllocatorRef allocator, const void *value) {
    (void)allocator;
    morphunLexiconFunctions.mdv_destroy(value);
}

// Native __DisplayValueArrayCallbacks at 0x2227d5d90: version 0, only release is set.
static const OCFArrayCallBacks kDisplayValueArrayCallbacks = {0, NULL, _DisplayValueRelease, NULL, NULL};

extern char morphunConstantArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSConstantArray));

static const OCFConstantString kCaseFeature = MORPHUN_STRING("case");
static const OCFConstantString kDeterminationFeature = MORPHUN_STRING("determination");
static const OCFConstantString kPronounTypeFeature = MORPHUN_STRING("pronounType");
static const id kPronounFeatures[] = {
    (id)&kCaseFeature, (id)&kDeterminationFeature, (id)&kPronounTypeFeature
};

// Native constant array at 0x222899dd0, backing values 0x22288c760.
static const struct {
    const void *isa;
    NSUInteger count;
    const id *objects;
} kPronounFeatureArray = {morphunConstantArrayClass, 3, kPronounFeatures};

// Native -[_NSLexiconMorphunDictionary preferredPronoun:forTermOfAddress:morphology:dependency:],
// 0x1c0580ca4. Non-external. The Swift constraints and membership selectors are deferred scope edges.
- (id)preferredPronoun:(id)original
      forTermOfAddress:(ONSTermOfAddress *)term
            morphology:(ONSMorphology *)morphology
            dependency:(id)dependency {
    dispatch_once(&morphunLoadOnceToken, (dispatch_block_t)(void *)&kMorphunLoadBlock);
    if (morphunFunctions.getDefaultProvider == NULL) {
        return nil;
    }
    id locale =
        [(id<MorphunInflectionMessages>)morphunLocaleClass localeWithLocaleIdentifier:[self languageCode]];
    if (locale == nil) {
        return nil;
    }

    id result = nil;
    OCFTypeRef error = NULL;
    void *provider = morphunFunctions.getDefaultProvider(&error);
    if (error != NULL) {
        MORPHUN_REPORT_ERROR(error, "mlccfp_getDefaultCommonConceptFactoryProvider(&error)");
        return objc_autorelease(nil);
    }
    if (provider == NULL) {
        return objc_autorelease(nil);
    }
    error = NULL;
    void *factory = morphunFunctions.getCommonFactory(provider, (OCFTypeRef)locale, &error);
    if (error != NULL) {
        MORPHUN_REPORT_ERROR(error, "mlccfp_getCommonConceptFactory(provider, (CFLocaleRef)locale, &error)");
        return objc_autorelease(nil);
    }
    if (factory == NULL) {
        return objc_autorelease(nil);
    }
    error = NULL;
    void *model = morphunFunctions.getSemanticModel(factory, &error);
    if (error != NULL) {
        MORPHUN_REPORT_ERROR(error, "mccf_getSemanticFeatureModel(factory, &error)");
        return objc_autorelease(nil);
    }
    if (model == NULL) {
        return objc_autorelease(nil);
    }
    error = NULL;
    void *pronoun =
        morphunLexiconFunctions.mpron_createFromInitialPronoun(model, (OCFStringRef)original, &error);
    if (error != NULL) {
        MORPHUN_REPORT_ERROR(error, "mpron_createFromInitialPronoun(model, (CFStringRef)original, &error)");
        return objc_autorelease(nil);
    }
    if (pronoun == NULL) {
        return objc_autorelease(nil);
    }

    OCFMutableArrayRef displayValues = NULL;
    void *localizedPronoun = NULL;
    id originalGrammemes = nil;
    error = NULL;
    void *concept = morphunLexiconFunctions.mpron_toSemanticFeatureConcept(pronoun, &error);
    if (error != NULL) {
        MORPHUN_REPORT_ERROR(error, "mpron_toSemanticFeatureConcept(pronoun, &error)");
        goto destroyPronoun;
    }
    if (concept == NULL) {
        goto destroyPronoun;
    }

    if ([term isEqual:[ONSTermOfAddress feminine]]) {
        error = NULL;
        morphunLexiconFunctions.msfc_putConstraintByName(
            concept, OF_STR("gender"), OF_STR("feminine"), &error
        );
        if (error != NULL) {
            MORPHUN_REPORT_ERROR(
                error,
                "msfc_putConstraintByName(pronounSemanticFeatureConcept, CFSTR(\"gender\"), "
                "CFSTR(\"feminine\"), &error)"
            );
            goto destroyPronoun;
        }
    } else if ([term isEqual:[ONSTermOfAddress masculine]]) {
        error = NULL;
        morphunLexiconFunctions.msfc_putConstraintByName(
            concept, OF_STR("gender"), OF_STR("masculine"), &error
        );
        if (error != NULL) {
            MORPHUN_REPORT_ERROR(
                error,
                "msfc_putConstraintByName(pronounSemanticFeatureConcept, CFSTR(\"gender\"), "
                "CFSTR(\"masculine\"), &error)"
            );
            goto destroyPronoun;
        }
    } else if ([term isEqual:[ONSTermOfAddress neutral]]) {
        error = NULL;
        morphunLexiconFunctions.msfc_clearConstraintByName(concept, OF_STR("gender"), &error);
        if (error != NULL) {
            MORPHUN_REPORT_ERROR(
                error, "msfc_clearConstraintByName(pronounSemanticFeatureConcept, CFSTR(\"gender\"), &error)"
            );
            goto destroyPronoun;
        }
    } else if ([[term pronouns] count] != 0) {
        displayValues =
            OCFArrayCreateMutable(NULL, (OCFIndex)[[term pronouns] count], &kDisplayValueArrayCallbacks);
        for (ONSMorphologyPronoun *candidate in [term pronouns]) {
            id constraints = [candidate _morphunConstraints];
            error = NULL;
            const void *display = morphunLexiconFunctions.mdv_create(
                model, (OCFStringRef)[candidate pronoun], constraints, &error
            );
            // Foundation defect: the native mdv_create failure at 0x1c05815a0 leaves its error unreleased.
            if (error != NULL) {
                if (display != NULL) {
                    os_log_error(
                        _NSInflectionLog(), "Inflection engine reported an error in mdv_create: %@", (id)error
                    );
                    morphunLexiconFunctions.mdv_destroy(display);
                }
                OCFRelease(error);
            } else if (display != NULL) {
                OCFArrayAppendValue(displayValues, display);
            }
        }

        error = NULL;
        localizedPronoun = morphunLexiconFunctions.mpron_createWithCustom(model, displayValues, NULL, &error);
        if (error != NULL) {
            MORPHUN_REPORT_ERROR(error, "mpron_createWithCustom(model, localizedPronouns, nil, &error)");
            localizedPronoun = NULL;
            goto destroyPronoun;
        }
        if (localizedPronoun == NULL) {
            goto destroyPronoun;
        }

        error = NULL;
        void *localizedConcept =
            morphunLexiconFunctions.mpron_toSemanticFeatureConcept(localizedPronoun, &error);
        if (error != NULL) {
            MORPHUN_REPORT_ERROR(error, "mpron_toSemanticFeatureConcept(localizedPronoun, &error)");
            goto destroyPronoun;
        }
        if (localizedConcept == NULL) {
            goto destroyPronoun;
        }

        originalGrammemes = objc_autorelease(objc_alloc_init([ONSMutableDictionary class]));
        for (id key in (ONSArray *)&kPronounFeatureArray) {
            error = NULL;
            void *value =
                morphunLexiconFunctions.msfc_createFeatureValueByNameCopy(concept, (OCFStringRef)key, &error);
            if (error != NULL) {
                MORPHUN_REPORT_ERROR(
                    error,
                    "msfc_createFeatureValueByNameCopy(pronounSemanticFeatureConcept, (CFStringRef)grammeme, "
                    "&error)"
                );
                continue;
            }
            if (value == NULL) {
                continue;
            }
            error = NULL;
            OCFStringRef printed = morphunFunctions.copyPrint(value, &error);
            if (error != NULL) {
                MORPHUN_REPORT_ERROR(error, "mss_createPrintCopy(grammemeValue, &error)");
            } else if (printed != NULL) {
                [(id<MorphologyMutableDictionaryMessages>)originalGrammemes setObject:(id)printed
                                                                    forKeyedSubscript:key];
                error = NULL;
                morphunLexiconFunctions.msfc_putConstraintByName(
                    localizedConcept, (OCFStringRef)key, printed, &error
                );
                if (error != NULL) {
                    MORPHUN_REPORT_ERROR(
                        error,
                        "msfc_putConstraintByName(localizedSemanticFeatureConcept, (__bridge "
                        "CFStringRef)grammeme, grammemeValueString, &error)"
                    );
                }
                OCFRelease(printed);
            }
            morphunFunctions.releaseSpeakable(value);
        }
        concept = localizedConcept;
    }

    if (morphology != nil) {
        id constraints = [morphology _morphunConstraints];
        for (id key in constraints) {
            id value = [[morphology _morphunConstraints] objectForKeyedSubscript:key];
            error = NULL;
            morphunLexiconFunctions.msfc_putConstraintByName(
                concept, (OCFStringRef)key, (OCFStringRef)value, &error
            );
            if (error != NULL) {
                MORPHUN_REPORT_ERROR(
                    error,
                    "msfc_putConstraintByName(pronounSemanticFeatureConcept, (__bridge CFStringRef)grammeme, "
                    "(__bridge CFStringRef)value, &error)"
                );
            }
        }
    }

    error = NULL;
    uint8_t exists = morphunLexiconFunctions.msfc_isExists(concept, &error);
    if (error != NULL) {
        MORPHUN_REPORT_ERROR(error, "msfc_isExists(pronounSemanticFeatureConcept, &error)");
        goto destroyPronoun;
    }
    if (!(exists & 1)) {
        goto destroyPronoun;
    }

    void *speakable = NULL;
    if (dependency != nil) {
        error = NULL;
        void *dependencySpeakable = morphunFunctions.createSpeakable((OCFStringRef)dependency, &error);
        if (error != NULL) {
            MORPHUN_REPORT_ERROR(error, "mss_create((CFStringRef)dependency, &error)");
            goto destroyPronoun;
        }
        if (dependencySpeakable == NULL) {
            goto destroyPronoun;
        }

        error = NULL;
        void *dependencyInflectable = morphunFunctions.createInflectable(model, dependencySpeakable, &error);
        if (error != NULL) {
            MORPHUN_REPORT_ERROR(error, "minf_create(model, dependencySpeakableString, &error)");
            morphunFunctions.releaseSpeakable(dependencySpeakable);
            goto destroyPronoun;
        }
        if (dependencyInflectable == NULL) {
            morphunFunctions.releaseSpeakable(dependencySpeakable);
            goto destroyPronoun;
        }

        error = NULL;
        void *dependencyConcept =
            morphunLexiconFunctions.minf_toSemanticFeatureConcept(dependencyInflectable, &error);
        if (error != NULL) {
            MORPHUN_REPORT_ERROR(
                error, "minf_toSemanticFeatureConcept(dependencyInflectableStringConcept, &error)"
            );
        } else if (dependencyConcept != NULL) {
            error = NULL;
            speakable = morphunLexiconFunctions.msfc_toSpeakableStringWithDependencyCopy(
                concept, dependencyConcept, &error
            );
            if (error != NULL) {
                MORPHUN_REPORT_ERROR(
                    error,
                    "msfc_toSpeakableStringWithDependencyCopy(pronounSemanticFeatureConcept, "
                    "dependencySemanticFeatureConcept, &error)"
                );
                speakable = NULL;
            }
        }
        morphunFunctions.releaseSpeakable(dependencySpeakable);
        morphunFunctions.releaseInflectable(dependencyInflectable);
    } else {
        error = NULL;
        speakable = morphunLexiconFunctions.msfc_toSpeakableStringCopy(concept, &error);
        if (error != NULL) {
            MORPHUN_REPORT_ERROR(error, "msfc_toSpeakableStringCopy(pronounSemanticFeatureConcept, &error)");
            goto destroyPronoun;
        }
    }
    if (speakable == NULL) {
        goto destroyPronoun;
    }

    error = NULL;
    id printed = (id)morphunFunctions.copyPrint(speakable, &error);
    if (error != NULL) {
        MORPHUN_REPORT_ERROR(error, "(NSString *)mss_createPrintCopy(speakableResult, &error)");
    } else if (printed != nil) {
        if (localizedPronoun == NULL || [term _containsPronoun:printed withGrammemes:originalGrammemes]) {
            id adjusted = printed;
            if ([original isEqualToString:[original lowercaseStringWithLocale:locale]]) {
                adjusted = [printed lowercaseStringWithLocale:locale];
            } else if ([original isEqualToString:[original uppercaseStringWithLocale:locale]]) {
                adjusted = [printed uppercaseStringWithLocale:locale];
            } else if ([original isEqualToString:[original capitalizedStringWithLocale:locale]]) {
                adjusted = [printed capitalizedStringWithLocale:locale];
            }
            result = [adjusted copy];
        }
        // Foundation defect: 0x1c0581f74 clears the result before releasing the rejected print string.
        objc_release(printed);
    }
    morphunFunctions.releaseSpeakable(speakable);

destroyPronoun:
    morphunLexiconFunctions.mpron_destroy(pronoun);
    if (localizedPronoun != NULL) {
        morphunLexiconFunctions.mpron_destroy(localizedPronoun);
    }
    if (displayValues != NULL) {
        OCFRelease(displayValues);
    }
    return objc_autorelease(result);
}

// Native -[_NSLexiconMorphunDictionary languageCode], 0x1c058221c. Non-external.
- (ONSString *)languageCode {
    return _languageCode;
}

@end

#undef MORPHUN_REPORT_ERROR
#undef MORPHUN_STRING

// Native pointer slots 0x22e76c708–0x22e76c878 and dispatch predicate 0x22e76c840. Non-external.
MorphunFunctions morphunFunctions;
MorphunLexiconFunctions morphunLexiconFunctions;
dispatch_once_t morphunLoadOnceToken;

// Native ____NSLexiconLoadMorphunIfNeeded_block_invoke, 0x1c0582224. Non-external.
// iOS Device 24A446: 0x180f41904, verified equivalent lookup order and failure paths with arm64e
// return-address authentication. The block leaves the table zeroed when dlopen fails and retains
// the library handle for the process lifetime. Each successful lookup is stored before the next.
// The caller uses the void block ABI; Hex-Rays shows the incidental X0 at the epilogue as a result.
// Reading: Local/Scratch/cf-core-r2-fi/Reading.md.
void morphunLoadInvoke(void *block) {
    (void)block;
    void *handle = dlopen("/usr/lib/libmorphun.dylib", RTLD_FIRST);
    if (handle == NULL) {
        return;
    }

#define LOAD_MORPHUN_SYMBOL(slot, name, line)                                                                \
    do {                                                                                                     \
        void *symbol = dlsym(handle, name);                                                                  \
        if (symbol == NULL) {                                                                                \
            __assert_rtn(                                                                                    \
                "_NSLexiconLoadMorphunIfNeeded_block_invoke", "NSLexiconMorphun.m", line, "symbol"           \
            );                                                                                               \
        }                                                                                                    \
        slot = (__typeof__(slot))symbol;                                                                     \
    } while (0)

    LOAD_MORPHUN_SYMBOL(
        morphunFunctions.getDefaultProvider, "mlccfp_getDefaultCommonConceptFactoryProvider", 191
    );
    LOAD_MORPHUN_SYMBOL(morphunFunctions.getCommonFactory, "mlccfp_getCommonConceptFactory", 192);
    LOAD_MORPHUN_SYMBOL(morphunFunctions.getSemanticModel, "mccf_getSemanticFeatureModel", 193);
    LOAD_MORPHUN_SYMBOL(morphunFunctions.createSpeakable, "mss_create", 194);
    LOAD_MORPHUN_SYMBOL(morphunFunctions.createInflectable, "minf_create", 195);
    LOAD_MORPHUN_SYMBOL(morphunFunctions.toSpeakable, "minf_toSpeakableString", 196);
    LOAD_MORPHUN_SYMBOL(morphunFunctions.copyFeature, "minf_getFeatureValueByNameCopy", 197);
    LOAD_MORPHUN_SYMBOL(
        morphunLexiconFunctions.minf_toSemanticFeatureConcept, "minf_toSemanticFeatureConcept", 198
    );
    LOAD_MORPHUN_SYMBOL(morphunFunctions.copyPrint, "mss_createPrintCopy", 199);
    LOAD_MORPHUN_SYMBOL(morphunFunctions.releaseSpeakable, "mss_destroy", 200);
    LOAD_MORPHUN_SYMBOL(morphunFunctions.releaseInflectable, "minf_destroy", 201);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.minf_putConstraintByName, "minf_putConstraintByName", 202);
    LOAD_MORPHUN_SYMBOL(
        morphunLexiconFunctions.matlm_getSurfaceFormGrammemes, "matlm_getSurfaceFormGrammemes", 203
    );
    LOAD_MORPHUN_SYMBOL(
        morphunLexiconFunctions.matmwt_createLemmaMappings, "matmwt_createLemmaMappings", 204
    );
    LOAD_MORPHUN_SYMBOL(
        morphunLexiconFunctions.matmwt_getMorphologicalWordToken, "matmwt_getMorphologicalWordToken", 205
    );
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mca_createTokenChain, "mca_createTokenChain", 206);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mdaf_create, "mdaf_create", 207);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mdaf_getAnalyzer, "mdaf_getAnalyzer", 208);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mddmd_createPropertyNames, "mddmd_createPropertyNames", 209);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mddmd_getDictionary, "mddmd_getDictionary", 210);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mtok_createTokenizer, "mtok_createTokenizer", 211);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mtok_createTokenChain, "mtok_createTokenChain", 212);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mtok_destroy, "mtok_destroy", 213);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mtok_setStyle, "mtok_setStyle", 214);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mtok_isType, "mtok_isType", 215);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mtokc_getToken, "mtokc_getToken", 216);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mtokc_destroy, "mtokc_destroy", 217);
    LOAD_MORPHUN_SYMBOL(morphunFunctions.inflectionExists, "minf_isExists", 218);
    LOAD_MORPHUN_SYMBOL(
        morphunLexiconFunctions.mpron_toSemanticFeatureConcept, "mpron_toSemanticFeatureConcept", 219
    );
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mpron_toPronounConcept, "mpron_toPronounConcept", 220);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mpron_createWithCustom, "mpron_createWithCustom", 221);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mpron_createWithDefaults, "mpron_createWithDefaults", 222);
    LOAD_MORPHUN_SYMBOL(
        morphunLexiconFunctions.mpron_createFromInitialPronoun, "mpron_createFromInitialPronoun", 223
    );
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mpron_destroy, "mpron_destroy", 224);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.msfc_isExists, "msfc_isExists", 225);
    LOAD_MORPHUN_SYMBOL(
        morphunLexiconFunctions.msfc_createConstraintByNameCopy, "msfc_createConstraintByNameCopy", 226
    );
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.msfc_putConstraintByName, "msfc_putConstraintByName", 227);
    LOAD_MORPHUN_SYMBOL(
        morphunLexiconFunctions.msfc_clearConstraintByName, "msfc_clearConstraintByName", 228
    );
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.msfc_reset, "msfc_reset", 229);
    LOAD_MORPHUN_SYMBOL(
        morphunLexiconFunctions.msfc_createFeatureValueByNameCopy, "msfc_createFeatureValueByNameCopy", 230
    );
    LOAD_MORPHUN_SYMBOL(
        morphunLexiconFunctions.msfc_toSpeakableStringCopy, "msfc_toSpeakableStringCopy", 231
    );
    LOAD_MORPHUN_SYMBOL(
        morphunLexiconFunctions.msfc_toSpeakableStringWithDependencyCopy,
        "msfc_toSpeakableStringWithDependencyCopy",
        232
    );
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.msfc_clone, "msfc_clone", 233);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.msfc_destroy, "msfc_destroy", 234);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mdv_create, "mdv_create", 235);
    LOAD_MORPHUN_SYMBOL(morphunLexiconFunctions.mdv_destroy, "mdv_destroy", 236);
#undef LOAD_MORPHUN_SYMBOL
}
