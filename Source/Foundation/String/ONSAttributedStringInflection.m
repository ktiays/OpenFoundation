//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringInflection.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"

#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Objective-C/OCFNSEmptyCollections.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/String/OCFString.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#include "Foundation/String/ONSAttributedStringMorphun.h"
#import "Foundation/String/ONSAttributedStringTermOfAddress.h"

#include <assert.h>
#include <Block.h>
#include <os/log.h>

// Class objects referenced by direct libobjc class queries in the native capture.
extern char runtimeONSMorphologyClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMorphology));

// Inferred organization: the native source file is NSAttributedString_Inflection.m (the name its assertions
// pass). Its part in this family spans 0x1c052fb64-0x1c05343a3 of the iOS Simulator __text section: the
// NSAttributedString(NSInflection) methods with their blocks (0x1c052fb64-0x1c052ff57), _NSInflect
// (0x1c0530060) with its blocks ____NSInflect_block_invoke, _2, _3, .24, .27, .35, .39, .41 and .45
// (0x1c053032c-0x1c05332c3, _NSInflectionGrammarWithLensAndRange at 0x1c0532cec between .27 and .35), then
// the NSMutableAttributedString(NSInflection) methods with their blocks (0x1c05332c4-0x1c05343a3). The two
// NSMorphology direct methods (0x1c04bb12c, 0x1c04bb398) belong to NSMorphology's own unit; they are defined
// first here, in a category (see the header). The blocks are written as block literals at their native
// positions; the compiler emits their invoke functions and the __block helpers
// (___Block_byref_object_copy__8 0x1c0fdcc80 and ___Block_byref_object_dispose__8 0x1c0fdcc90, folded into
// the unit). Native block literals carry flags 0xC0000000 (signature and extended layout, no copy or dispose
// helpers) even where they capture objects. Clang emits additional copy/dispose helpers for the source
// block literals here. Their behavior if a callee copies these blocks has not been validated; synchronous
// invocation is the captured path. This remains an integration verification boundary. Objective-C compiled
// with manual reference counting (objc_retain, objc_release; __block object variables are unretained).
// Evidence: Local/Evidence/cf-core/iphone-simulator-foundation-link.json (family FC-sets-attributed). Unless
// a definition says otherwise: iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.
//
// Shared-cache GOT slots outside the extracted image, identified from their use (confidence: medium): the
// classes NSArray 0x22227fa90, NSLocale 0x22227fb00, NSMutableArray 0x22227fb18, NSMutableDictionary
// 0x22227fb28, NSDictionary 0x22227fad0 and NSNull 0x22227fb40 (CoreFoundation's), and the static objects
// __NSArray0__struct 0x2222807b0 (the empty array of an @[] literal), __kCFBooleanFalse 0x2222807e0 and
// __kCFBooleanTrue 0x2222807e8 (the @NO and @YES literals). The code loads each slot and uses the value
// directly; the restored code references the same symbols.

extern char inflectionMutableAttributedClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                         OF_NAME_STRING(ONSMutableAttributedString));

// Class symbols the unit references (the classes are declared in the header).
extern char arrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSArray));
extern char assertionHandlerClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSAssertionHandler));
extern char bundleClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSBundle));
extern char characterSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSCharacterSet));
extern char dictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDictionary));
extern char inflectionRuleClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSInflectionRule));
extern char localeClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSLocale));
extern char localizedNumberFormatRuleClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                       OF_NAME_STRING(ONSLocalizedNumberFormatRule));
extern char morphologyClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMorphology));
extern char mutableArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableArray));
extern char mutableDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableDictionary));
extern char nullClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSNull));
extern char numberClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSNumber));
extern char stringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char termOfAddressClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSTermOfAddress));
extern char threadClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSThread));
extern char valueClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSValue));
extern char grammarInflectionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                               OF_NAME_STRING(_ONSAttributedStringGrammarInflection));
extern char inflectionRuleExplicitClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                    OF_NAME_STRING(_ONSInflectionRuleExplicit));
extern char lexiconMorphunDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                      OF_NAME_STRING(_ONSLexiconMorphunDictionary));

// The messages the unit sends to objects of other classes (replacements, rules, morphologies, strings,
// collections, locales, the observer), declared for the sends only; no protocol object is emitted.
@protocol InflectionMessages

// Replacements (_NSAttributedStringReplacement).
- (ONSRange)replacementRangeInResult;
- (NSInteger)replacementKind;
- (NSInteger)index;
- (id)argument;

// Inflection rules and grammar objects.
+ (id)automaticRule;
- (id)morphology;
- (NSInteger)referentConcept;
- (NSInteger)agreeWithArgument;
- (NSInteger)agreeWithConcept;
- (id)initWithAttributesDictionary:(id)dictionary;
- (id)initWithExternalRepresentationDictionary:(id)dictionary error:(id *)error;
- (id)initWithMorphology:(id)morphology;
- (id)initWithInflection:(id)inflection;
+ (id)_heuristicMorphologyForLocale:(id)locale
                             prefix:(id)prefix
                             suffix:(id)suffix
                        featureName:(id *)featureName
                matchedPrefixLength:(NSUInteger *)matchedPrefixLength
                matchedSuffixLength:(NSUInteger *)matchedSuffixLength;
- (void)applyPluralityRulesForNumbers:(id)numbers inLanguages:(id)languages;
- (id)_formattedString:(id)string withStringLocale:(id)stringLocale andDisplayLocale:(id)displayLocale;

// Terms of address and the lexicon.
+ (id)currentUser;
- (id)_morphologyForLanguage:(id)language;
- (BOOL)_isSupportedInLanguage:(id)language;
- (id)_withExtrapolatedPronouns;
- (id)initWithLanguage:(id)language;
- (id)allPossibleWordAttributesForWord:(id)word;
- (id)preferredPronoun:(id)pronoun
      forTermOfAddress:(id)termOfAddress
            morphology:(id)morphology
            dependency:(id)dependency;

// The inflection observer.
- (void)didSkipInflectableRangeWithUnfilledFormatSpecifiers:(ONSRange)range
                                            sourceSubstring:(id)substring
                                             inflectionRule:(id)rule
                                                alternative:(id)alternative;
- (void)willProcessInflectableRange:(ONSRange)range
                    sourceSubstring:(id)substring
                     inflectionRule:(id)rule
                             target:(id)target
                        alternative:(id)alternative
          assumedInflectionFallback:(id)fallback
                    fallbackWasUsed:(BOOL)fallbackWasUsed;

// Collections, values and numbers.
- (NSUInteger)count;
- (id)firstObject;
- (id)lastObject;
- (void)addObject:(id)object;
- (void)removeAllObjects;
- (id)objectAtIndexedSubscript:(NSUInteger)index;
- (id)objectForKeyedSubscript:(id)key;
- (void)setObject:(id)object forKeyedSubscript:(id)key;
- (void)removeObjectForKey:(id)key;
- (id)allKeys;
+ (id)valueWithRange:(ONSRange)range;
- (ONSRange)rangeValue;
+ (id)numberWithUnsignedInteger:(NSUInteger)value;
- (NSUInteger)unsignedIntegerValue;
- (NSInteger)integerValue;
+ (id)null;

// Strings, character sets and attributed strings.
+ (id)stringWithUTF8String:(const char *)string;
- (NSUInteger)length;
- (id)string;
- (id)mutableString;
- (unsigned short)characterAtIndex:(NSUInteger)index;
- (id)substringWithRange:(ONSRange)range;
- (id)substringFromIndex:(NSUInteger)index;
- (id)stringByAppendingString:(id)string;
- (BOOL)isEqualToString:(id)string;
- (id)lowercaseStringWithLocale:(id)locale;
- (id)capitalizedStringWithLocale:(id)locale;
- (ONSRange)rangeOfString:(id)string;
- (ONSRange)rangeOfString:(id)string options:(NSUInteger)mask range:(ONSRange)range;
- (ONSRange)rangeOfCharacterFromSet:(id)set;
- (void)enumerateSubstringsInRange:(ONSRange)range
                           options:(NSUInteger)options
                        usingBlock:
                            (void (^)(
                                id substring, ONSRange substringRange, ONSRange enclosingRange, BOOL *stop
                            ))block;
- (void)replaceCharactersInRange:(ONSRange)range withString:(id)string;
- (void)replaceCharactersInRange:(ONSRange)range withAttributedString:(id)attributedString;
+ (id)characterSetWithCharactersInString:(id)string;
- (id)invertedSet;
- (id)initWithString:(id)string;
- (void)enumerateAttribute:(id)name
                   inRange:(ONSRange)range
                   options:(NSUInteger)options
                usingBlock:(void (^)(id value, ONSRange range, BOOL *stop))block;
- (void)enumerateAttributesInRange:(ONSRange)range
                           options:(NSUInteger)options
                        usingBlock:(void (^)(id attributes, ONSRange range, BOOL *stop))block;
- (void)addAttribute:(id)name value:(id)value range:(ONSRange)range;
- (void)removeAttribute:(id)name range:(ONSRange)range;

// Locales, bundles, threads and assertions.
+ (id)currentLocale;
+ (id)localeWithLocaleIdentifier:(id)identifier;
- (id)initWithLocaleIdentifier:(id)identifier;
- (id)localeIdentifier;
+ (id)mainBundle;
- (id)preferredLocalizations;
- (id)developmentLocalization;
+ (BOOL)isMainThread;
+ (id)currentHandler;
- (void)handleFailureInFunction:(id)functionName
                           file:(id)fileName
                     lineNumber:(NSInteger)line
                    description:(id)format, ...;

@end

// Native __NSInflectionLog.log, 0x22e776208, and .onceToken, 0x22e776210. Private externals;
// __common zero initialization is native.
__attribute__((visibility("hidden"))) os_log_t inflectionLogObject;
__attribute__((visibility("hidden"))) dispatch_once_t inflectionLogOnceToken;

// Native ____NSInflectionLog_block_invoke, 0x1c0fdcb88. Non-external (was a private external).
OF_EXTERN void inflectionLogInvoke(void *block) __asm(OF_SYMBOL("___NSInflectionLog_block_invoke"));

void inflectionLogInvoke(void *block) {
    (void)block;
    inflectionLogObject = os_log_create("com.apple.Foundation", "Inflection");
}

// Native global block 0x22281c978, descriptor 0x2227d03f8. Block ABI
// metadata is read from the pinned image: flags 0x50000000, size 32, signature v8@?0, no layout.
static const struct {
    uintptr_t reserved;
    uintptr_t size;
    const char *signature;
    const char *layout;
} kInflectionLogDescriptor = {0x50000000, 32, "v8@?0", NULL};

__attribute__((visibility("hidden"))) const struct {
    void *isa;
    int32_t flags;
    int32_t reserved;
    void (*invoke)(void *);
    const void *descriptor;
} inflectionLogBlockLiteral __asm(OF_SYMBOL("inflectionLogGlobalBlock")) = {
    _NSConcreteGlobalBlock, 0x50000000, 0, inflectionLogInvoke, &kInflectionLogDescriptor
};

// The fast-enumeration loops below are the native countByEnumeratingWithState:objects:count: loops with a
// 16-object buffer and objc_enumerationMutation checks (the for-in statement's code).

// Native _NSInflectionObserver, 0x22e6fcb60, non-external. Its setter is outside this family's capture.
// The undefined data dependency preserves observer callbacks and prevents Release from folding the
// observer to nil. The coordinating session supplies storage with the setter's complete group.
// The overlap test the unit inlines in three places (no symbol natively): whether the run [location, end)
// and the candidate range share at least one index, computed as the native code does (the smaller end against
// the later start). The candidate's start is tested first when it lies inside the run.
static inline BOOL inflectionRangesOverlap(ONSRange candidate, NSUInteger location, NSUInteger end) {
    NSUInteger candidateEnd = candidate.location + candidate.length;
    NSUInteger smallerEnd = candidateEnd < end ? candidateEnd : end;
    NSUInteger start;
    if (location <= candidate.location && candidate.location < end) {
        start = candidate.location;
    } else if (candidate.location <= location && location < candidateEnd) {
        start = location;
    } else {
        return NO;
    }
    return smallerEnd != start;
}

@implementation ONSMorphology (InflectionDirectMethods)

// Native -[NSMorphology mergeMorphology:override:], 0x1c04bb12c. A direct method. For each feature in the
// native order (grammatical gender, part of speech, number, grammatical case, determination, grammatical
// person, pronoun type, definiteness, _adjectival, _clusivity): when the other morphology sets it, copy it if
// bit 0 of override is set or the receiver leaves it unset.
- (void)mergeMorphology:(id)morphology override:(BOOL)override {
    if ([morphology grammaticalGender] != 0) {
        if (override || [self grammaticalGender] == 0) {
            [self setGrammaticalGender:[morphology grammaticalGender]];
        }
    }
    if ([morphology partOfSpeech] != 0) {
        if (override || [self partOfSpeech] == 0) {
            [self setPartOfSpeech:[morphology partOfSpeech]];
        }
    }
    if ([morphology number] != 0) {
        if (override || [self number] == 0) {
            [self setNumber:[morphology number]];
        }
    }
    if ([morphology grammaticalCase] != 0) {
        if (override || [self grammaticalCase] == 0) {
            [self setGrammaticalCase:[morphology grammaticalCase]];
        }
    }
    if ([morphology determination] != 0) {
        if (override || [self determination] == 0) {
            [self setDetermination:[morphology determination]];
        }
    }
    if ([morphology grammaticalPerson] != 0) {
        if (override || [self grammaticalPerson] == 0) {
            [self setGrammaticalPerson:[morphology grammaticalPerson]];
        }
    }
    if ([morphology pronounType] != 0) {
        if (override || [self pronounType] == 0) {
            [self setPronounType:[morphology pronounType]];
        }
    }
    if ([morphology definiteness] != 0) {
        if (override || [self definiteness] == 0) {
            [self setDefiniteness:[morphology definiteness]];
        }
    }
    if ([morphology _adjectival] != 0) {
        if (override || [self _adjectival] == 0) {
            [self set_adjectival:[morphology _adjectival]];
        }
    }
    if ([morphology _clusivity] != 0) {
        if (override || [self _clusivity] == 0) {
            [self set_clusivity:[morphology _clusivity]];
        }
    }
}

// Native -[NSMorphology isIdentity], 0x1c04bb398. A direct method. Tests grammatical gender, grammatical
// case, part of speech, definiteness, number, determination, grammatical person, pronoun type and _clusivity
// in that order, then _adjectival.
- (BOOL)isIdentity {
    if ([self grammaticalGender] != 0 || [self grammaticalCase] != 0 || [self partOfSpeech] != 0 ||
        [self definiteness] != 0 || [self number] != 0 || [self determination] != 0 ||
        [self grammaticalPerson] != 0 || [self pronounType] != 0 || [self _clusivity] != 0) {
        return NO;
    }
    return [self _adjectival] == 0;
}

@end

@implementation ONSAttributedString (NSInflection)

// Native -[NSAttributedString(NSInflection) _firstValueOfAttributeWithKey:inRange:], 0x1c052fb64, with its
// block (0x1c052fc50). Enumerates with options 0x100000 (longest effective range not required); the block
// retains the first non-nil value and stops. The value is returned autoreleased.
- (id)_firstValueOfAttributeWithKey:(id)key inRange:(ONSRange)range {
    __block id value = nil;
    [(id)self enumerateAttribute:key
                         inRange:range
                         options:0x100000
                      usingBlock:^(id attributeValue, ONSRange attributeRange, BOOL *stop) {
                        (void)attributeRange;
                        if (attributeValue != nil) {
                            value = objc_retain((id)(attributeValue));
                            *stop = YES;
                        }
                      }];
    return objc_autorelease((id)(value));
}

// Native -[NSAttributedString(NSInflection) _inflectedAttributedString], 0x1c052fc8c. A mutable copy
// inflected with the current locale, empty replacements and concepts (the @[] literal), preflight YES.
- (id)_inflectedAttributedString {
    id copy = [self mutableCopy];
    [copy _inflectWithLocale:[(id)localeClass currentLocale]
                replacements:(id)__ONSArray0__struct
                    concepts:(id)__ONSArray0__struct
                   preflight:YES];
    return objc_autorelease((id)(copy));
}

// Native -[NSAttributedString(NSInflection) _identicalAttributesInRange:], 0x1c052fcd8, with its block
// (0x1c052fdd4). The attributes every run of the range shares (options 0x100000): the first run's attributes
// copied, then each key whose value differs in a later run removed; enumeration stops when none remain. An
// empty range answers a new empty NSDictionary. The result is returned autoreleased.
- (id)_identicalAttributesInRange:(ONSRange)range {
    __block id identical = nil;
    [(id)self enumerateAttributesInRange:range
                                 options:0x100000
                              usingBlock:^(id attributes, ONSRange attributesRange, BOOL *stop) {
                                (void)attributesRange;
                                if (identical != nil) {
                                    for (id key in [identical allKeys]) {
                                        if (![[attributes objectForKeyedSubscript:key]
                                                isEqual:[identical objectForKeyedSubscript:key]]) {
                                            [identical removeObjectForKey:key];
                                        }
                                    }
                                    *stop = [identical count] == 0;
                                } else {
                                    identical = [attributes mutableCopy];
                                }
                              }];
    id result = identical;
    if (result == nil) {
        result = objc_alloc_init((Class)((id)dictionaryClass));
    }
    return objc_autorelease((id)(result));
}

@end

// Native _NSInflectionGrammarWithLensAndRange, 0x1c0532cec. Non-external; defined after _NSInflect, in native
// address order.
static id
_NSInflectionGrammarWithLensAndRange(InflectionFirstValueBlock firstValueOfAttribute, ONSRange range);

// Native _NSInflect, 0x1c0530060. Was a private external. The observer is inflectionObserver on the main
// thread and nil elsewhere. The dictionary cache (a __block NSMutableDictionary created on first use) and the
// array of processed ranges (an NSMutableArray) are released at the end. The three enumerations run in order:
// the inflectable blocks over (0, length()), the formattable blocks over (0, length()), and the
// _NSInflectionAlternative runs over (0, length()); length() is called again before each.
void _NSInflect(
    id locale,
    id replacements,
    id concepts,
    id termOfAddress,
    InflectionLengthBlock length,
    BOOL preflight,
    InflectionEnumerateAttributeBlock enumerateAttribute,
    InflectionEnumerateRangeBlock enumerateInflectableBlocks,
    InflectionEnumerateRangeBlock enumerateFormattableBlocks,
    InflectionRemoveAttributeBlock removeAttribute,
    InflectionRangePredicate hasFormatSpecifier,
    InflectionFirstValueBlock firstValueOfAttribute,
    InflectionSubstringBlock substringWithRange,
    InflectionEnumerateSubstringsBlock enumerateSubstrings,
    InflectionReplaceBlock replaceCharacters,
    InflectionAddAttributeBlock addAttribute,
    InflectionFormatAlternativeBlock formatInflectionAlternative
) {
    id observer = [(id)threadClass isMainThread] ? inflectionObserver : nil;
    __block id dictionaries = nil;

    // Native ____NSInflect_block_invoke, 0x1c053032c: the lexicon of a language, cached by language. A new
    // _NSLexiconMorphunDictionary is autoreleased; the cache dictionary is created on first insertion.
    id (^dictionaryForLanguage)(id) = ^id(id language) {
      id dictionary = [dictionaries objectForKeyedSubscript:language];
      if (dictionary == nil) {
          dictionary = objc_autorelease((id)([objc_alloc((Class)((id)lexiconMorphunDictionaryClass))
              initWithLanguage:language]));
          if (dictionary != nil) {
              if (dictionaries == nil) {
                  dictionaries = objc_alloc_init((Class)((id)mutableDictionaryClass));
              }
              [dictionaries setObject:dictionary forKeyedSubscript:language];
          }
      }
      return dictionary;
    };

    // Native ____NSInflect_block_invoke_2, 0x1c05303d4: the language of a range. The run's _NSLanguage
    // attribute when it is a string; otherwise the locale's identifier, the main bundle's first preferred
    // localization, or its development localization; asserts (line 221) when none exists.
    InflectionSubstringBlock languageForRange = ^id(ONSRange range) {
      id language = firstValueOfAttribute(_ONSLanguageAttributeName, range);
      BOOL isString = _ONSIsNSString(language);
      if (language != nil && isString) {
          return language;
      }
      language = [locale localeIdentifier];
      if (language == nil) {
          language = [[[(id)bundleClass mainBundle] preferredLocalizations] firstObject];
          if (language == nil) {
              language = [[(id)bundleClass mainBundle] developmentLocalization];
              if (language == nil) {
                  __assert_rtn(
                      "_NSInflect_block_invoke_2", "NSAttributedString_Inflection.m", 221, "language"
                  );
              }
          }
      }
      return language;
    };

    id processedRanges = objc_alloc_init((Class)((id)mutableArrayClass));

    // Native ____NSInflect_block_invoke_3, 0x1c053047c (8672 bytes): inflects one inflectable run. See the
    // comments inside for the native branch structure.
    NSUInteger stringLength = length();
    enumerateInflectableBlocks((ONSRange){0, stringLength}, ^(id rule, ONSRange range, BOOL *stop) {
      (void)stop;
      if (rule == nil) {
          return;
      }
      NSUInteger end = range.location + range.length;

      // The replacements whose result range overlaps the run.
      id relevant = objc_alloc_init((Class)((id)mutableArrayClass));
      for (id replacement in replacements) {
          if (inflectionRangesOverlap([replacement replacementRangeInResult], range.location, end)) {
              [relevant addObject:replacement];
          }
      }
      os_log_debug(
          _NSInflectionLog(),
          "%{public}lld format specifiers are relevant (out of %{public}lld) in range %{public}@",
          (long long)[replacements count],
          (long long)[relevant count],
          ONSStringFromRange(range)
      );

      // Preflight (byte 0x90 of the captures compared with 1): a run with a format specifier or an external
      // dependency is skipped and reported.
      if (preflight == 1) {
          BOOL dependency;
          BOOL skip = NO;
          BOOL decided = NO;
          if (objc_opt_isKindOfClass(
                  (id)(rule), (id)(objc_opt_class((id)((Class)inflectionRuleExplicitClass)))
              )) {
              dependency = [rule agreeWithConcept] != 0 || [rule agreeWithArgument] != 0 ||
                           [rule referentConcept] != 0;
              if ([relevant count] != 0) {
                  skip = dependency;
                  decided = YES;
              }
          } else {
              if ([relevant count] != 0) {
                  decided = YES;
              }
              dependency = NO;
          }
          if (!decided) {
              skip = ((hasFormatSpecifier(range) | dependency) & 1) != 0;
          }
          if (skip) {
              os_log_debug(
                  _NSInflectionLog(),
                  "Range %{public}@ of string with length %lld has a format specifier or external "
                  "dependency; "
                  "not inflecting during preflight.",
                  ONSStringFromRange(range),
                  (long long)stringLength
              );
              if (observer != nil) {
                  id substring = substringWithRange(range);
                  [observer
                      didSkipInflectableRangeWithUnfilledFormatSpecifiers:range
                                                          sourceSubstring:substring
                                                           inflectionRule:rule
                                                              alternative:
                                                                  firstValueOfAttribute(
                                                                      _ONSInflectionAlternativeAttributeName,
                                                                      range
                                                                  )];
              }
              [processedRanges addObject:[(id)valueClass valueWithRange:range]];
              objc_release((id)(relevant));
              return;
          }
      }

      // Bit 0: a replacement or word fixed the noun; bit 1: the rule supplied a morphology.
      __block NSUInteger hints = 0;
      ONSMorphology *target = objc_alloc_init((Class)morphologyClass);
      id featureName = nil;
      id termOfAddressConcept = nil;
      id agreementConcept = nil;
      id agreementArgument = nil;
      id heuristicTarget = nil;
      BOOL capitalize = NO;
      BOOL shouldInflect;
      NSUInteger inflectLocation = range.location;
      NSUInteger inflectLength = range.length;

      if ([rule isEqual:(id)&__kOCFBooleanFalse]) {
          os_log_debug(
              _NSInflectionLog(),
              "NSInflect attribute is NO, not inflecting in range %{public}@",
              ONSStringFromRange(range)
          );
          objc_release((id)(target));
          objc_release((id)(relevant));
          return;
      }
      if ([rule isEqual:(id)&__kOCFBooleanTrue] || rule == [(id)inflectionRuleClass automaticRule]) {
          // Automatic inflection.
          if ([relevant count] == 0) {
              id morphology = [termOfAddress _morphologyForLanguage:languageForRange(range)];
              [target mergeMorphology:morphology override:YES];
              if ([target grammaticalGender] == 3) {
                  [target setGrammaticalGender:4];
              }
          } else if ([relevant count] != 0) {
              id heuristicLocale = [(id)localeClass localeWithLocaleIdentifier:languageForRange(range)];
              ONSRange first = [[relevant firstObject] replacementRangeInResult];
              ONSRange last = [[relevant lastObject] replacementRangeInResult];
              NSUInteger firstEnd = first.location + first.length;
              NSUInteger lastEnd = last.location + last.length;
              NSUInteger maximumEnd = firstEnd > lastEnd ? firstEnd : lastEnd;
              NSUInteger minimumStart = first.location < last.location ? first.location : last.location;
              id prefix = substringWithRange((ONSRange){range.location, minimumStart - range.location});
              id suffix =
                  substringWithRange((ONSRange){maximumEnd, range.location + range.length - maximumEnd});
              NSUInteger matchedPrefixLength = 0;
              NSUInteger matchedSuffixLength = 0;
              heuristicTarget = [(id)morphologyClass _heuristicMorphologyForLocale:heuristicLocale
                                                                            prefix:prefix
                                                                            suffix:suffix
                                                                       featureName:&featureName
                                                               matchedPrefixLength:&matchedPrefixLength
                                                               matchedSuffixLength:&matchedSuffixLength];
              if (((uintptr_t)heuristicTarget | (uintptr_t)featureName) != 0) {
                  if ((matchedPrefixLength | matchedSuffixLength) == 0) {
                      __assert_rtn(
                          "_NSInflect_block_invoke",
                          "NSAttributedString_Inflection.m",
                          349,
                          "matchedPrefixLength || matchedSuffixLength"
                      );
                  }
                  // Native 0x1c0530fbc-0x1c0531000: the 22-byte record stores header
                  // 0x08400202, the morphology at 4, descriptor 0x0840 at 12, then the feature at 14.
                  // Keep the direct native call; an os_log macro emits a duplicate compiler helper.
                  os_log_t log = _NSInflectionLog();
                  if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
                      struct __attribute__((packed)) {
                          uint32_t header;
                          id morphology;
                          uint16_t featureDescriptor;
                          id feature;
                      } buffer = {0x08400202, heuristicTarget, 0x0840, featureName};
                      _Static_assert(sizeof(buffer) == 22, "The captured log record is 22 bytes.");
                      _os_log_debug_impl(
                          &__dso_handle,
                          log,
                          OS_LOG_TYPE_DEBUG,
                          "Found language-specific rule for prefix / suffix pair: %@, %@",
                          (uint8_t *)&buffer,
                          22
                      );
                  }
                  if (matchedPrefixLength != 0) {
                      capitalize =
                          ![prefix isEqualToString:[prefix lowercaseStringWithLocale:heuristicLocale]];
                  } else {
                      capitalize = NO;
                  }
                  inflectLocation = matchedPrefixLength + range.location;
                  inflectLength = range.length - (matchedPrefixLength + matchedSuffixLength);
                  if ([relevant count] == 1) {
                      if (inflectLength == maximumEnd - minimumStart) {
                          [relevant removeAllObjects];
                      }
                  }
              }
          }
          os_log_debug(
              _NSInflectionLog(),
              "NSInflect attribute is YES, using automatic inflection in range %{public}@, initial target: "
              "%@, "
              "language-specific rules: %@ (%@)",
              ONSStringFromRange(range),
              target,
              heuristicTarget,
              featureName
          );
          shouldInflect = YES;
      } else if (
          objc_opt_isKindOfClass((id)(rule), (id)(objc_opt_class((id)((Class)inflectionRuleExplicitClass))))
      ) {
          // An explicit inflection rule.
          [target mergeMorphology:[rule morphology] override:YES];
          hints |= 2;
          shouldInflect = YES;
          if ([rule referentConcept] != 0) {
              if ([rule referentConcept] < 1 || (NSUInteger)[rule referentConcept] > [concepts count]) {
                  os_log_error(
                      _NSInflectionLog(),
                      "NSInflectionReferentConcept attribute in range %{public}@ is set to %{public}ld which "
                      "is "
                      "out of bounds.",
                      ONSStringFromRange(range),
                      (long)[rule referentConcept]
                  );
                  shouldInflect = NO;
                  termOfAddressConcept = nil;
              } else {
                  termOfAddressConcept = [concepts objectAtIndexedSubscript:[rule referentConcept] - 1];
                  if (!objc_opt_isKindOfClass(
                          (id)(termOfAddressConcept), (id)(objc_opt_class((id)((Class)arrayClass)))
                      )) {
                      os_log_error(
                          _NSInflectionLog(),
                          "NSInflectionReferentConcept attribute in range %{public}@ is set to %{public}ld. "
                          "Expected a 'TermOfAddress' concept, but found %{private}@ instead.",
                          ONSStringFromRange(range),
                          (long)[rule referentConcept],
                          termOfAddressConcept
                      );
                      shouldInflect = NO;
                      termOfAddressConcept = nil;
                  }
              }
          }
          if ([rule agreeWithArgument] != 0) {
              id argument = nil;
              if ([rule agreeWithArgument] >= 1) {
                  for (id replacement in replacements) {
                      if ([replacement index] == [rule agreeWithArgument]) {
                          argument = replacement;
                          break;
                      }
                  }
              }
              if (argument != nil) {
                  agreementArgument = argument;
                  if ([rule agreeWithConcept] != 0) {
                      os_log_error(
                          _NSInflectionLog(),
                          "Both NSInflectionAgreementArgument and NSInflectionAgreementConcept attributes "
                          "specified in range %{public}@. The latter will be discarded.",
                          ONSStringFromRange(range)
                      );
                  }
                  agreementConcept = nil;
              } else {
                  os_log_error(
                      _NSInflectionLog(),
                      "NSInflectionAgreementArgument attribute in range %{public}@ is set to %{public}ld "
                      "which "
                      "is out of bounds.",
                      ONSStringFromRange(range),
                      (long)[rule agreeWithArgument]
                  );
                  agreementArgument = nil;
                  agreementConcept = nil;
                  shouldInflect = NO;
              }
          } else if ([rule agreeWithConcept] != 0) {
              if ([rule agreeWithConcept] >= 1 && (NSUInteger)[rule agreeWithConcept] <= [concepts count]) {
                  agreementConcept = [concepts objectAtIndexedSubscript:[rule agreeWithConcept] - 1];
              } else {
                  os_log_error(
                      _NSInflectionLog(),
                      "NSInflectionAgreementConcept attribute in range %{public}@ is set to %{public}ld "
                      "which "
                      "is out of bounds.",
                      ONSStringFromRange(range),
                      (long)[rule agreeWithConcept]
                  );
                  agreementConcept = nil;
                  shouldInflect = NO;
              }
              agreementArgument = nil;
          }
          os_log_debug(
              _NSInflectionLog(),
              "NSInflect attribute is an inflection rule (%@ / %@), using it in range %{public}@",
              rule,
              target,
              ONSStringFromRange(range)
          );
      } else {
          // An internal grammar inflection object, or another value (used as is).
          if (objc_opt_isKindOfClass((id)(rule), (id)(objc_opt_class((id)((Class)grammarInflectionClass))))) {
              [target mergeMorphology:objc_autorelease((id)([(id)objc_alloc((Class)morphologyClass)
                                          initWithInflection:rule]))
                             override:YES];
              hints |= 2;
              os_log_debug(
                  _NSInflectionLog(),
                  "NSInflect attribute is an internal inflection object (%@), using it in range %{public}@",
                  target,
                  ONSStringFromRange(range)
              );
          }
          shouldInflect = YES;
      }

      // An identity target with no language-specific rule falls back to the assumed inflection attribute.
      BOOL identity = [target isIdentity];
      BOOL fallbackUsed = NO;
      if ((heuristicTarget != nil ? NO : identity) == 1 && featureName == nil) {
          id fallback = firstValueOfAttribute(_ONSAssumedFallbackInflectionAttributeName, range);
          if (_ONSIsNSDictionary(fallback)) {
              fallback = objc_autorelease((id)([objc_alloc((Class)((id)grammarInflectionClass))
                  initWithExternalRepresentationDictionary:fallback
                                                     error:NULL]));
          }
          if (objc_opt_isKindOfClass(
                  (id)(fallback), (id)(objc_opt_class((id)((id)runtimeONSMorphologyClass)))
              )) {
              fallback = objc_autorelease((id)([objc_alloc((Class)((id)grammarInflectionClass))
                  initWithMorphology:fallback]));
          }
          fallbackUsed = objc_opt_isKindOfClass(
                             (id)(fallback), (id)(objc_opt_class((id)((Class)grammarInflectionClass)))
                         ) &
                         (fallback != nil);
          if (fallbackUsed == 1) {
              [target mergeMorphology:objc_autorelease((id)([(id)objc_alloc((Class)morphologyClass)
                                          initWithInflection:fallback]))
                             override:YES];
              os_log_debug(
                  _NSInflectionLog(),
                  "Inflection was identity; assumed inflection attribute value %{public}@ will be used "
                  "instead in "
                  "range %{public}@",
                  target,
                  ONSStringFromRange(range)
              );
              shouldInflect = YES;
          }
          removeAttribute(_ONSAssumedFallbackInflectionAttributeName, range);
      }
      os_log_debug(
          _NSInflectionLog(),
          "Range %{public}@ of string with length %lld will be inflected with requested inflection: %@, "
          "resolved "
          "inflection %@",
          ONSStringFromRange(range),
          (long long)stringLength,
          rule,
          target
      );
      removeAttribute(ONSInflectionRuleAttributeName, range);
      removeAttribute(ONSInflectionAgreementConceptAttributeName, range);
      removeAttribute(ONSInflectionAgreementArgumentAttributeName, range);
      removeAttribute(ONSInflectionReferentConceptAttributeName, range);

      // Inflections inferred from the replacements, the agreement argument or the agreement concept.
      NSUInteger relevantCount = [relevant count];
      if (agreementArgument != nil || relevantCount != 0 || agreementConcept != nil) {
          __block id numbers = nil;
          __block id numberLanguages = nil;
          __block ONSMorphology *nounMorphology = objc_alloc_init((Class)morphologyClass);
          __block ONSMorphology *nounOrAdjectiveMorphology = objc_alloc_init((Class)morphologyClass);
          __block ONSMorphology *adjectiveMorphology = objc_alloc_init((Class)morphologyClass);
          __block ONSMorphology *otherMorphology = objc_alloc_init((Class)morphologyClass);

          // Native ____NSInflect_block_invoke.24, 0x1c05327e0: sorts the possible attributes of one word
          // (skipping those whose part of speech equals skippedPartOfSpeech when it is not 0) into a noun
          // (part of speech 9), an adjective (6) and the first other one. Returns 5 for no attributes, 2 when
          // a noun and an adjective are found (the noun merged into the noun-or-adjective morphology, with
          // override and then without), 1 for a noun alone (merged into the noun morphology; hint bit 0
          // set), 3 for an adjective alone, 4 otherwise (the first other word merged into the other
          // morphology when present).
          NSInteger (^addWordAttributes)(id, NSInteger) = ^NSInteger(
              id attributes, NSInteger skippedPartOfSpeech
          ) {
            if ([attributes count] == 0) {
                return 5;
            }
            os_log_t log = _NSInflectionLog();
            if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
                os_log_debug(log, "%{public}s: %{public}@", "Adding attributes of a single word", attributes);
            }
            id noun = nil;
            id adjective = nil;
            id other = nil;
            for (id word in attributes) {
                NSInteger partOfSpeech = [word partOfSpeech];
                if (skippedPartOfSpeech != 0 && partOfSpeech == skippedPartOfSpeech) {
                    continue;
                }
                if (noun == nil && partOfSpeech == 9) {
                    noun = word;
                } else if (adjective == nil && partOfSpeech == 6) {
                    adjective = word;
                } else if (other == nil) {
                    other = word;
                }
                if (noun != nil && adjective != nil) {
                    [nounOrAdjectiveMorphology mergeMorphology:noun override:YES];
                    [nounOrAdjectiveMorphology mergeMorphology:noun override:NO];
                    return 2;
                }
            }
            if (noun != nil && adjective == nil) {
                [nounMorphology mergeMorphology:noun override:NO];
                hints |= 1;
                return 1;
            }
            if (noun != nil) {
                // Unreachable natively (the loop returns as soon as both are found); kept as compiled: the
                // merge without override only.
                [nounOrAdjectiveMorphology mergeMorphology:noun override:NO];
                return 2;
            }
            if (adjective != nil) {
                [adjectiveMorphology mergeMorphology:adjective override:NO];
                return 3;
            }
            if (other != nil) {
                [otherMorphology mergeMorphology:other override:NO];
            }
            return 4;
          };

          // Native ____NSInflect_block_invoke.27, 0x1c0532a9c: one replacement. A numeric replacement
          // (kind 1, asserted at line 529 to carry an NSNumber, or any NSNumber argument) adds its argument
          // to the numbers and the language of its result range to the number languages (both arrays
          // created on first use); a word replacement (a string, or an attributed string's string) adds
          // the possible attributes of the word in its language, skipping the part of speech of the range's
          // grammar. The range's grammar is then merged into the target without override.
          void (^addReplacement)(id) = ^(id replacement) {
            ONSRange resultRange = [replacement replacementRangeInResult];
            if ([replacement replacementKind] == 1 || _ONSIsNSNumber([replacement argument])) {
                if ([replacement replacementKind] == 1 && !_ONSIsNSNumber([replacement argument])) {
                    [[(id)assertionHandlerClass currentHandler]
                        handleFailureInFunction:
                            [(id)stringClass
                                stringWithUTF8String:
                                    "void _NSInflect(NSLocale * _Nullable, "
                                    "NSArray<_NSAttributedStringReplacement "
                                    "*> * _Nonnull, NSArray<NSObject *> * _Nullable, NSTermOfAddress * "
                                    "_Nullable, "
                                    "NSUInteger (^ _Nonnull)(void), BOOL, void (^ "
                                    "_Nonnull)(NSAttributedStringKey, "
                                    "NSRange, __attribute__((noescape)) void (^)(id _Nullable, NSRange, BOOL "
                                    "* "
                                    "_Nonnull)), void (^ _Nonnull)(NSRange, __attribute__((noescape)) void "
                                    "(^)(id "
                                    "_Nullable, NSRange, BOOL * _Nonnull)), void (^ _Nonnull)(NSRange, "
                                    "__attribute__((noescape)) void (^)(NSLocalizedNumberFormatRule *, "
                                    "NSRange, BOOL "
                                    "* _Nonnull)), void (^ _Nonnull)(NSAttributedStringKey, NSRange), BOOL "
                                    "(^ "
                                    "_Nonnull)(NSRange), id  _Nullable (^ _Nonnull)(NSAttributedStringKey, "
                                    "NSRange), NSString *(^ _Nonnull)(NSRange), void (^ _Nonnull)(NSRange, "
                                    "__attribute__((noescape)) void (^)(NSString * _Nullable, NSRange, BOOL "
                                    "* "
                                    "_Nonnull)), void (^ _Nonnull)(NSRange, id), void (^ _Nonnull)(NSRange, "
                                    "NSAttributedStringKey, id), id  _Nullable (^ "
                                    "_Nonnull)(id))_block_invoke"]
                                           file:(id)OF_STR("NSAttributedString_Inflection.m")
                                     lineNumber:529
                                    description:
                                        (id)OF_STR(
                                            "Attribute string replacement argument should be a NSNumber "
                                            "when the formatting specifier is numeric"
                                        )];
                }
                if (numbers == nil) {
                    numbers = objc_alloc_init((Class)((id)mutableArrayClass));
                }
                [numbers addObject:[replacement argument]];
                if (numberLanguages == nil) {
                    numberLanguages = objc_alloc_init((Class)((id)mutableArrayClass));
                }
                [numberLanguages addObject:languageForRange(resultRange)];
                return;
            }
            id word = [replacement argument];
            id grammar = _NSInflectionGrammarWithLensAndRange(
                firstValueOfAttribute, [replacement replacementRangeInResult]
            );
            BOOL isWord = YES;
            if (!_ONSIsNSString(word)) {
                if (_ONSIsNSAttributedString(word)) {
                    word = [word string];
                } else {
                    isWord = NO;
                }
            }
            if (isWord && word != nil) {
                NSInteger partOfSpeech = grammar != nil ? [grammar partOfSpeech] : 0;
                id language = languageForRange(resultRange);
                addWordAttributes(
                    [dictionaryForLanguage(language) allPossibleWordAttributesForWord:word], partOfSpeech
                );
            }
            if (grammar != nil) {
                [target mergeMorphology:grammar override:NO];
            }
          };

          if (agreementConcept != nil) {
              id language = languageForRange(range);
              if (_ONSIsNSString(agreementConcept)) {
                  addWordAttributes(
                      [dictionaryForLanguage(language) allPossibleWordAttributesForWord:agreementConcept], 0
                  );
              } else if (_ONSIsNSArray(agreementConcept)) {
                  id supported = nil;
                  for (id term in agreementConcept) {
                      if (objc_opt_isKindOfClass(
                              (id)(term), (id)(objc_opt_class((id)((Class)termOfAddressClass)))
                          ) &&
                          [term _isSupportedInLanguage:language]) {
                          supported = [term _withExtrapolatedPronouns];
                          break;
                      }
                  }
                  id morphology = [supported _morphologyForLanguage:language];
                  if (morphology != nil) {
                      [target mergeMorphology:morphology override:NO];
                  }
              }
          } else if (agreementArgument != nil) {
              addReplacement(agreementArgument);
          } else {
              for (id replacement in relevant) {
                  ONSRange resultRange = [replacement replacementRangeInResult];
                  if ([replacement replacementKind] != 2 && range.location <= resultRange.location &&
                      end >= resultRange.location + resultRange.length) {
                      addReplacement(replacement);
                  }
              }
          }

          [target mergeMorphology:nounMorphology override:NO];
          os_log_debug(
              _NSInflectionLog(),
              "Range %{public}@ of string with length %lld -- edited target inflection with attributes from "
              "replacements that are unambiguous nouns: %@.",
              ONSStringFromRange(range),
              (long long)stringLength,
              target
          );
          [target mergeMorphology:nounOrAdjectiveMorphology override:NO];
          os_log_debug(
              _NSInflectionLog(),
              "Range %{public}@ of string with length %lld -- edited target inflection with attributes from "
              "replacements that could be nouns or adjectives: %@.",
              ONSStringFromRange(range),
              (long long)stringLength,
              target
          );
          [target mergeMorphology:adjectiveMorphology override:NO];
          os_log_debug(
              _NSInflectionLog(),
              "Range %{public}@ of string with length %lld -- edited target inflection with attributes from "
              "replacements that are unambiguous adjectives: %@.",
              ONSStringFromRange(range),
              (long long)stringLength,
              target
          );
          [target mergeMorphology:otherMorphology override:NO];
          os_log_debug(
              _NSInflectionLog(),
              "Range %{public}@ of string with length %lld -- edited target inflection with attributes from "
              "replacements that are other words: %@.",
              ONSStringFromRange(range),
              (long long)stringLength,
              target
          );
          objc_release((id)(nounMorphology));
          objc_release((id)(nounOrAdjectiveMorphology));
          objc_release((id)(adjectiveMorphology));
          objc_release((id)(otherMorphology));
          if (numbers != nil && numberLanguages != nil) {
              [(id)target applyPluralityRulesForNumbers:numbers inLanguages:numberLanguages];
              os_log_debug(
                  _NSInflectionLog(),
                  "Range %{public}@ of string with length %lld -- edited target inflection with numbers "
                  "supplied "
                  "as replacements with result: %@.",
                  ONSStringFromRange(range),
                  (long long)stringLength,
                  target
              );
          }
          objc_release((id)(numbers));
          objc_release((id)(numberLanguages));
          os_log_debug(
              _NSInflectionLog(),
              "Target inflection after applying inflections inferred from replacements: %{public}@",
              target
          );
      }

      if (observer != nil) {
          id substring = substringWithRange(range);
          id alternative = firstValueOfAttribute(_ONSInflectionAlternativeAttributeName, range);
          [observer willProcessInflectableRange:range
                                sourceSubstring:substring
                                 inflectionRule:rule
                                         target:target
                                    alternative:alternative
                      assumedInflectionFallback:firstValueOfAttribute(
                                                    _ONSAssumedFallbackInflectionAttributeName, range
                                                )
                                fallbackWasUsed:fallbackUsed];
      }
      if ((hints & 1) == 0) {
          // Native ____NSInflect_block_invoke.35, 0x1c0532da0: a word whose possible attributes are all nouns
          // (part of speech 9) sets hint bit 0 and stops the enumeration.
          enumerateSubstrings(range, ^(id substring, ONSRange substringRange, BOOL *stopWords) {
            id dictionary = dictionaryForLanguage(languageForRange(substringRange));
            if (dictionary == nil) {
                return;
            }
            id attributes = [dictionary allPossibleWordAttributesForWord:substring];
            NSUInteger count = 0;
            for (id word in attributes) {
                count++;
                if ([word partOfSpeech] != 9) {
                    return;
                }
            }
            if (count == 0) {
                return;
            }
            hints |= 1;
            *stopWords = YES;
          });
      }
      os_log_debug(
          _NSInflectionLog(),
          "Range %{public}@ of string with length %lld -- determined final disambiguation hints: %lld.",
          ONSStringFromRange(range),
          (long long)stringLength,
          (long long)hints
      );

      id language = languageForRange(range);
      BOOL requiresMorphun = _NSRequiresMorphunInflectionForLanguageIdentifier(language);
      os_log_info(
          _NSInflectionLog(),
          "Inflection engine is enabled? %{public}s for language %{public}@",
          requiresMorphun ? "true" : "false",
          language
      );

      BOOL processed = NO;
      if ((shouldInflect & requiresMorphun) == 1) {
          id substring = substringWithRange((ONSRange){inflectLocation, inflectLength});
          id result = nil;
          BOOL haveResult = YES;
          if (termOfAddressConcept != nil) {
              id preferred = preferredTermOfAddressForLanguageFromList(language, termOfAddressConcept);
              id dictionary = nil;
              if (preferred != nil) {
                  dictionary = dictionaryForLanguage(language);
              }
              if (preferred == nil || dictionary == nil) {
                  os_log_debug(
                      _NSInflectionLog(),
                      "Could not find a term of address string in range %{public}@ that is compatible with "
                      "language %{public}@.",
                      ONSStringFromRange(range),
                      language
                  );
                  haveResult = NO;
              } else {
                  id morphology = firstValueOfAttribute(ONSMorphologyAttributeName, range);
                  id dependency;
                  if (agreementArgument != nil) {
                      dependency = [agreementArgument argument];
                      if (!_ONSIsNSString(dependency)) {
                          if (_ONSIsNSAttributedString(dependency)) {
                              dependency = [dependency string];
                          } else {
                              dependency = nil;
                          }
                      }
                  } else if (agreementConcept != nil) {
                      dependency = _ONSIsNSString(agreementConcept) ? agreementConcept : nil;
                  } else {
                      dependency = nil;
                  }
                  result = [dictionary
                      preferredPronoun:substring
                      forTermOfAddress:preferred
                            morphology:(objc_opt_isKindOfClass(
                                            (id)(morphology),
                                            (id)(objc_opt_class((id)((id)runtimeONSMorphologyClass)))
                                        )
                                            ? morphology
                                            : nil)dependency:dependency];
              }
          } else {
              id grammar = _NSInflectionGrammarWithLensAndRange(firstValueOfAttribute, range);
              if (grammar != nil) {
                  [target mergeMorphology:grammar override:NO];
                  [target setPartOfSpeech:[grammar partOfSpeech]];
              }
              result = _NSLexiconInflectedStringUsingMorphun(
                  substring, inflectLocation, inflectLength, language, target, nil, relevant
              );
              if (((uintptr_t)featureName | (uintptr_t)heuristicTarget) != 0) {
                  result = _NSLexiconInflectedStringUsingMorphun(
                      result == nil ? substring : result,
                      NSIntegerMax,
                      0,
                      language,
                      heuristicTarget,
                      featureName,
                      (id)__ONSArray0__struct
                  );
              }
              if (capitalize) {
                  id capitalizationLocale =
                      [objc_alloc((Class)((id)localeClass)) initWithLocaleIdentifier:language];
                  __block ONSRange capitalizedRange = {0, 0};
                  __block id capitalized = nil;
                  // Native ____NSInflect_block_invoke.39, 0x1c0532ee4: the first word (options 2, by words)
                  // capitalized in the language's locale and retained, with its range; always stops.
                  [result
                      enumerateSubstringsInRange:(ONSRange){0, [result length]}
                                         options:2
                                      usingBlock:^(
                                          id word, ONSRange wordRange, ONSRange enclosingRange, BOOL *stopWord
                                      ) {
                                        (void)enclosingRange;
                                        if (word != nil) {
                                            capitalized = objc_retain((id)([word
                                                capitalizedStringWithLocale:capitalizationLocale]));
                                            capitalizedRange = wordRange;
                                        }
                                        *stopWord = YES;
                                      }];
                  objc_release((id)(capitalizationLocale));
                  if (capitalized != nil) {
                      result = [capitalized
                          stringByAppendingString:[result substringFromIndex:capitalizedRange.location +
                                                                             capitalizedRange.length]];
                      objc_release((id)(capitalized));
                  }
              }
          }
          if (haveResult && result != nil) {
              if (range.length != inflectLength) {
                  substring = substringWithRange(range);
              }
              if ([result isEqual:substring]) {
                  os_log_info(
                      _NSInflectionLog(),
                      "Useful inflection but no replacement in range %{public}@",
                      ONSStringFromRange(range)
                  );
              } else {
                  replaceCharacters(range, result);
                  os_log_info(
                      _NSInflectionLog(),
                      "Useful inflection with new replacement in range %{public}@",
                      ONSStringFromRange(range)
                  );
              }
              range.length = [result length];
              processed = YES;
          }
      }
      objc_release((id)(target));
      objc_release((id)(relevant));
      if (processed) {
          [processedRanges addObject:[(id)valueClass valueWithRange:range]];
      }
    });

    // Native ____NSInflect_block_invoke.41, 0x1c0532f48: formats one localized-number run. Without a locale
    // it logs an error and stops. Otherwise the run's original number string (its
    // "NSLocalizedNumberOriginalNumber" attribute when a string, else its substring) is formatted with the
    // rule for the run's language and displayed in the locale; a changed result records the original in that
    // attribute and replaces the run. The run's range is added to the processed ranges when a result exists.
    NSUInteger formattableLength = length();
    enumerateFormattableBlocks((ONSRange){0, formattableLength}, ^(id rule, ONSRange range, BOOL *stop) {
      if (locale == nil) {
          os_log_error(
              _NSInflectionLog(),
              "A `formatNumber` attribute is present, but no locale has been specified for formatting. The "
              "attribute will have no effect."
          );
          *stop = YES;
          return;
      }
      id original = firstValueOfAttribute((id)OF_STR("NSLocalizedNumberOriginalNumber"), range);
      if (original == nil || !_ONSIsNSString(original)) {
          original = substringWithRange(range);
      }
      id stringLocale = [(id)localeClass localeWithLocaleIdentifier:languageForRange(range)];
      id formatted = [rule _formattedString:original withStringLocale:stringLocale andDisplayLocale:locale];
      if (formatted == nil) {
          return;
      }
      if (![formatted isEqualToString:original]) {
          addAttribute(range, (id)OF_STR("NSLocalizedNumberOriginalNumber"), original);
          replaceCharacters(range, formatted);
      }
      [processedRanges addObject:[(id)valueClass valueWithRange:range]];
    });

    // Native ____NSInflect_block_invoke.45, 0x1c0533100: one _NSInflectionAlternative run. A run overlapping
    // a processed range loses the attribute; otherwise it is replaced by the formatted alternative (when
    // there are replacements and the formatting gives neither nil nor NSNull) or by the alternative itself.
    NSUInteger alternativeLength = length();
    enumerateAttribute(
        _ONSInflectionAlternativeAttributeName,
        (ONSRange){0, alternativeLength},
        ^(id alternative, ONSRange range, BOOL *stop) {
          (void)stop;
          if (alternative == nil) {
              return;
          }
          NSUInteger end = range.location + range.length;
          for (id value in processedRanges) {
              ONSRange processedRange = [value rangeValue];
              NSUInteger processedEnd = processedRange.location + processedRange.length;
              NSUInteger smallerEnd = end < processedEnd ? end : processedEnd;
              NSUInteger start;
              if (processedRange.location <= range.location && range.location < processedEnd) {
                  start = range.location;
              } else if (range.location <= processedRange.location && processedRange.location < end) {
                  start = processedRange.location;
              } else {
                  continue;
              }
              if (smallerEnd != start) {
                  removeAttribute(_ONSInflectionAlternativeAttributeName, range);
                  return;
              }
          }
          if ([replacements count] != 0) {
              id formatted = formatInflectionAlternative(alternative);
              if (formatted != nil && formatted != [(id)nullClass null]) {
                  replaceCharacters(range, formatted);
                  return;
              }
          }
          replaceCharacters(range, alternative);
        }
    );

    objc_release((id)(processedRanges));
    objc_release((id)(dictionaries));
}

// Native _NSInflectionGrammarWithLensAndRange, 0x1c0532cec. Non-external. The range's NSMorphology attribute
// when it is an NSMorphology; otherwise its _NSGrammar attribute when that is one; otherwise nil.
static id
_NSInflectionGrammarWithLensAndRange(InflectionFirstValueBlock firstValueOfAttribute, ONSRange range) {
    id morphology = firstValueOfAttribute(ONSMorphologyAttributeName, range);
    if (objc_opt_isKindOfClass((id)(morphology), (id)(objc_opt_class((id)((id)runtimeONSMorphologyClass))))) {
        return morphology;
    }
    id grammar = firstValueOfAttribute(_ONSGrammarAttributeName, range);
    return objc_opt_isKindOfClass((id)(grammar), (id)(objc_opt_class((id)((id)runtimeONSMorphologyClass))))
               ? grammar
               : nil;
}

@implementation ONSMutableAttributedString (NSInflection)

// Native -[NSMutableAttributedString(NSInflection)
// _addAttributesIfNotPresentMergingInlinePresentationIntents: toRange:], 0x1c05332c4, with its block
// (0x1c053339c). For each run of the range (options 0x100000), every attribute the run lacks is added from
// the dictionary, and the run's inline presentation intents are ORed with the dictionary's; a nonzero union
// is set as an NSNumber. Nothing happens for an empty dictionary.
- (void)_addAttributesIfNotPresentMergingInlinePresentationIntents:(id)attributes toRange:(ONSRange)range {
    if ([attributes count] == 0) {
        return;
    }
    id intentsValue = [attributes objectForKeyedSubscript:ONSInlinePresentationIntentAttributeName];
    NSUInteger intents = intentsValue != nil ? [intentsValue unsignedIntegerValue] : 0;
    [(id)self enumerateAttributesInRange:range
                                 options:0x100000
                              usingBlock:^(id runAttributes, ONSRange runRange, BOOL *stop) {
                                (void)stop;
                                id runIntentsValue = [runAttributes
                                    objectForKeyedSubscript:ONSInlinePresentationIntentAttributeName];
                                NSUInteger runIntents =
                                    runIntentsValue != nil ? [runIntentsValue unsignedIntegerValue] : 0;
                                NSUInteger mergedIntents = runIntents | intents;
                                for (id key in attributes) {
                                    if ([runAttributes objectForKeyedSubscript:key] == nil) {
                                        [(id)self addAttribute:key
                                                         value:[attributes objectForKeyedSubscript:key]
                                                         range:runRange];
                                    }
                                }
                                if (mergedIntents != 0) {
                                    [(id)self
                                        addAttribute:ONSInlinePresentationIntentAttributeName
                                               value:[(id)numberClass numberWithUnsignedInteger:mergedIntents]
                                               range:runRange];
                                }
                              }];
}

// Native -[NSMutableAttributedString(NSInflection) _formatInflectionAlternative:withReplacements:],
// 0x1c0533528. A mutable attributed copy of the alternative (an NSString or an NSAttributedString; anything
// else answers nil) whose "{{" become "{", whose "{N}" (N a positive decimal) become the receiver's text at
// the result range of the replacement with index N (looked up at N - 1 first, then by enumeration), and whose
// "}}" become "}". A lone "{" or "}" at the end, an unterminated or non-numeric placeholder, an unknown index
// or a lone "}" make the result nil. The result is returned autoreleased.
- (id)_formatInflectionAlternative:(id)alternative withReplacements:(id)replacements {
    id nonDigits =
        [[(id)characterSetClass characterSetWithCharactersInString:(id)OF_STR("0123456789")] invertedSet];
    id result;
    if (_ONSIsNSString(alternative)) {
        result = [(id)objc_alloc((Class)inflectionMutableAttributedClass) initWithString:alternative];
    } else if (_ONSIsNSAttributedString(alternative)) {
        result = [alternative mutableCopy];
    } else {
        return objc_autorelease((id)((id)nil));
    }
    if (result == nil) {
        return objc_autorelease((id)(result));
    }
    [result length];
    NSUInteger brace = [[result string] rangeOfString:(id)OF_STR("{")].location;
    if (brace != (NSUInteger)NSIntegerMax) {
        id open = (id)OF_STR("{");
        while (brace < [result length] - 1) {
            NSUInteger next = brace + 1;
            if ([[result string] characterAtIndex:next] == '{') {
                [[result mutableString] replaceCharactersInRange:(ONSRange){brace, 2} withString:open];
            } else {
                id string = [result string];
                NSUInteger close = [string rangeOfString:(id)OF_STR("}")
                                                 options:0
                                                   range:(ONSRange){next, [result length] - next}]
                                       .location;
                if (close == (NSUInteger)NSIntegerMax) {
                    goto failure;
                }
                NSUInteger digitsLength = close + ~brace;
                id digits = [[result string] substringWithRange:(ONSRange){next, digitsLength}];
                if ([digits rangeOfCharacterFromSet:nonDigits].location != (NSUInteger)NSIntegerMax) {
                    goto failure;
                }
                NSInteger index = [digits integerValue];
                if (index - 1 < 0) {
                    goto failure;
                }
                id replacement = nil;
                if ((NSUInteger)(index - 1) < [replacements count] &&
                    [[replacements objectAtIndexedSubscript:index - 1] index] == index) {
                    replacement = [replacements objectAtIndexedSubscript:index - 1];
                }
                if (replacement == nil) {
                    for (id candidate in replacements) {
                        if ([candidate index] == index) {
                            replacement = candidate;
                            break;
                        }
                    }
                    if (replacement == nil) {
                        goto failure;
                    }
                }
                id text = [[(id)self string] substringWithRange:[replacement replacementRangeInResult]];
                [[result mutableString] replaceCharactersInRange:(ONSRange){brace, digitsLength + 2}
                                                      withString:text];
                NSUInteger textLength = [text length];
                if (textLength == (NSUInteger)NSIntegerMax) {
                    goto failure;
                }
                next = textLength + brace;
            }
            NSUInteger remaining = [result length] - next;
            brace = [[result string] rangeOfString:open options:0 range:(ONSRange){next, remaining}].location;
            if (brace == (NSUInteger)NSIntegerMax) {
                break;
            }
        }
        if (brace != (NSUInteger)NSIntegerMax) {
            goto failure;
        }
    }
    [result length];
    NSUInteger closing = [[result string] rangeOfString:(id)OF_STR("}")].location;
    if (closing == (NSUInteger)NSIntegerMax) {
        return objc_autorelease((id)(result));
    }
    id close = (id)OF_STR("}");
    while (closing < [result length] - 1) {
        NSUInteger next = closing + 1;
        if ([[result string] characterAtIndex:next] != '}') {
            break;
        }
        [[result mutableString] replaceCharactersInRange:(ONSRange){closing, 2} withString:close];
        NSUInteger remaining = [result length] - next;
        closing = [[result string] rangeOfString:close options:0 range:(ONSRange){next, remaining}].location;
        if (closing == (NSUInteger)NSIntegerMax) {
            return objc_autorelease((id)(result));
        }
    }
failure:
    objc_release((id)(result));
    result = nil;
    return objc_autorelease((id)(result));
}

// Native -[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:],
// 0x1c0533928, with its thirteen blocks (0x1c0533b64-0x1c0533ed7): calls _NSInflect with the current user's
// NSTermOfAddress and accessors over the receiver. Attribute enumeration runs in reverse (options 2);
// substrings are enumerated by words with localized semantics (options 3).
- (void)_inflectWithLocale:(id)locale
              replacements:(id)replacements
                  concepts:(id)concepts
                 preflight:(BOOL)preflight {
    id termOfAddress = [(id)termOfAddressClass currentUser];
    _NSInflect(
        locale,
        replacements,
        concepts,
        termOfAddress,
        // Native block_invoke, 0x1c0533b64.
        ^NSUInteger(void) {
          return [(id)self length];
        },
        preflight,
        // Native block_invoke_2, 0x1c0533b6c.
        ^(id key, ONSRange range, InflectionRangeVisitor visitor) {
          [(id)self enumerateAttribute:key inRange:range options:2 usingBlock:visitor];
        },
        // Native block_invoke_3, 0x1c0533b88.
        ^(ONSRange range, InflectionRangeVisitor visitor) {
          [self enumerateInflectableBlocksInRange:range usingBlock:visitor];
        },
        // Native block_invoke_4, 0x1c0533b9c.
        ^(ONSRange range, InflectionRangeVisitor visitor) {
          [self enumerateFormattableBlocksInRange:range usingBlock:visitor];
        },
        // Native block_invoke_5, 0x1c0533bb0.
        ^(id key, ONSRange range) {
          [(id)self removeAttribute:key range:range];
        },
        // Native block_invoke_6, 0x1c0533bc4: whether the range holds a "%" that does not begin "%%"; a "%"
        // in the range's last character does not count.
        ^BOOL(ONSRange range) {
          NSUInteger location = range.location;
          NSUInteger remaining = range.length;
          if (remaining == 0) {
              return NO;
          }
          while (YES) {
              ONSRange found = [[(id)self string] rangeOfString:(id)OF_STR("%")
                                                        options:0
                                                          range:(ONSRange){location, remaining}];
              if (found.location == (NSUInteger)NSIntegerMax) {
                  return NO;
              }
              NSUInteger end = location + remaining;
              if (found.location > end - 2) {
                  return NO;
              }
              if ([[(id)self string] characterAtIndex:found.location + 1] != '%') {
                  return YES;
              }
              location = found.location + found.length;
              remaining = end - location;
              if (remaining == 0) {
                  return NO;
              }
          }
        },
        // Native block_invoke_7, 0x1c0533c84.
        ^id(id key, ONSRange range) {
          return [self _firstValueOfAttributeWithKey:key inRange:range];
        },
        // Native block_invoke_8, 0x1c0533c98.
        ^id(ONSRange range) {
          return [[(id)self string] substringWithRange:range];
        },
        // Native block_invoke_9, 0x1c0533cc8, with block_invoke_10 (0x1c0533d74, which passes the substring,
        // its range and the stop pointer on).
        ^(ONSRange range, InflectionSubstringVisitor visitor) {
          [[(id)self mutableString]
              enumerateSubstringsInRange:range
                                 options:3
                              usingBlock:^(
                                  id substring, ONSRange substringRange, ONSRange enclosingRange, BOOL *stop
                              ) {
                                (void)enclosingRange;
                                visitor(substring, substringRange, stop);
                              }];
        },
        // Native block_invoke_11, 0x1c0533d84: a string replaces the characters; an attributed string
        // replaces them and gets back the attributes the range shared (except _NSInflectionAlternative) where
        // it lacks them; anything else is an assertion failure (line 1063).
        ^(ONSRange range, id replacement) {
          if (_ONSIsNSString(replacement)) {
              [[(id)self mutableString] replaceCharactersInRange:range withString:replacement];
          } else if (_ONSIsNSAttributedString(replacement)) {
              id attributes = [[self _identicalAttributesInRange:range] mutableCopy];
              [attributes removeObjectForKey:_ONSInflectionAlternativeAttributeName];
              [(id)self replaceCharactersInRange:range withAttributedString:replacement];
              [self _addAttributesIfNotPresentMergingInlinePresentationIntents:attributes
                                                                       toRange:(ONSRange){
                                                                                   range.location,
                                                                                   [replacement length]
                                                                               }];
              objc_release((id)(attributes));
          } else {
              [[(id)assertionHandlerClass currentHandler]
                  handleFailureInFunction:
                      [(id)stringClass stringWithUTF8String:"-[NSMutableAttributedString(NSInflection) "
                                                            "_inflectWithLocale:replacements:"
                                                            "concepts:preflight:]_block_invoke_11"]
                                     file:(id)OF_STR("NSAttributedString_Inflection.m")
                               lineNumber:1063
                              description:(id)OF_STR("Unknown kind of replacement: %@"), replacement];
          }
        },
        // Native block_invoke_12, 0x1c0533eb4.
        ^(ONSRange range, id key, id value) {
          [(id)self addAttribute:key value:value range:range];
        },
        // Native block_invoke_13, 0x1c0533ecc.
        ^id(id alternative) {
          return [self _formatInflectionAlternative:alternative withReplacements:replacements];
        }
    );
}

// Native -[NSMutableAttributedString(NSInflection) enumerateInflectableBlocksInRange:usingBlock:],
// 0x1c0533edc, with its block (0x1c053402c). Walks the attribute runs in reverse (options 2), turning each
// run's attributes into an inflection rule (an _NSInflectionRuleExplicit from the attributes, else a copy of
// the NSInflect attribute). Each change of rule reports the previous rule (when not nil) over the span from
// the changed run's end to the end of the span already covered; the last rule (when not nil) is reported over
// (0, end), location 0 whatever the range's location. The rules are owned (retained) until replaced.
- (void)enumerateInflectableBlocksInRange:(ONSRange)range usingBlock:(InflectionRangeVisitor)block {
    __block id current = nil;
    __block NSUInteger spanEnd = range.location + range.length;
    [(id)self
        enumerateAttributesInRange:range
                           options:2
                        usingBlock:^(id attributes, ONSRange runRange, BOOL *stop) {
                          id rule = [objc_alloc((Class)((id)inflectionRuleExplicitClass))
                              initWithAttributesDictionary:attributes];
                          if (rule == nil) {
                              rule = [[attributes objectForKeyedSubscript:_ONSInflectableAttributeName] copy];
                          }
                          if (current == rule || [current isEqual:rule]) {
                              objc_release((id)(rule));
                              return;
                          }
                          NSUInteger runEnd = runRange.location + runRange.length;
                          if (current != nil) {
                              block(current, (ONSRange){runEnd, spanEnd - runEnd}, stop);
                          }
                          objc_release((id)(current));
                          current = rule;
                          spanEnd = runEnd;
                        }];
    if (current != nil) {
        BOOL stop = NO;
        block(current, (ONSRange){0, spanEnd}, &stop);
        objc_release((id)(current));
    }
}

// Native -[NSMutableAttributedString(NSInflection) enumerateFormattableBlocksInRange:usingBlock:],
// 0x1c0534154, with its block (0x1c05342a4). As -enumerateInflectableBlocksInRange:usingBlock:, with the
// run's NSLocalizedNumberFormat attribute when it is an NSLocalizedNumberFormatRule (else nil) as the rule,
// copied when it changes.
- (void)enumerateFormattableBlocksInRange:(ONSRange)range usingBlock:(InflectionRangeVisitor)block {
    __block id current = nil;
    __block NSUInteger spanEnd = range.location + range.length;
    [(id)self enumerateAttributesInRange:range
                                 options:2
                              usingBlock:^(id attributes, ONSRange runRange, BOOL *stop) {
                                id rule = [attributes
                                    objectForKeyedSubscript:ONSLocalizedNumberFormatAttributeName];
                                if (!objc_opt_isKindOfClass(
                                        (id)(rule),
                                        (id)(objc_opt_class((id)((Class)localizedNumberFormatRuleClass)))
                                    )) {
                                    rule = nil;
                                }
                                if (current == rule || [current isEqual:rule]) {
                                    return;
                                }
                                NSUInteger runEnd = runRange.location + runRange.length;
                                if (current != nil) {
                                    block(current, (ONSRange){runEnd, spanEnd - runEnd}, stop);
                                }
                                objc_release((id)(current));
                                current = [rule copy];
                                spanEnd = runEnd;
                              }];
    if (current != nil) {
        BOOL stop = NO;
        block(current, (ONSRange){0, spanEnd}, &stop);
        objc_release((id)(current));
    }
}

@end
