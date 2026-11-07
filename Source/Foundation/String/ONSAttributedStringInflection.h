//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"

#include <OpenFoundation/Range/ONSRange.h>
#import "Foundation/String/ONSMutableAttributedString.h"

#include <OpenCoreFoundation/OCFBuild.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <os/log.h>
#include <dispatch/dispatch.h>
#include <stdint.h>

// Shared declarations of the inflection unit (String/ONSAttributedStringInflection.m): the NSInflection
// categories of NSAttributedString and NSMutableAttributedString, _NSInflect and the two NSMorphology direct
// methods it calls. iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless a declaration says otherwise. iOS Device 24A446: unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

// Block types of _NSInflect's parameters, spelled from the native __PRETTY_FUNCTION__ string that the
// assertion in ____NSInflect_block_invoke.27 (0x1c0532a9c) passes (NSRange spelled ONSRange, object types
// id). The names are local and descriptive.
typedef void (^InflectionRangeVisitor)(id value, ONSRange range, BOOL *stop);
typedef void (^InflectionSubstringVisitor)(id substring, ONSRange range, BOOL *stop);
typedef NSUInteger (^InflectionLengthBlock)(void);
typedef void (^InflectionEnumerateAttributeBlock)(
    id key, ONSRange range, __attribute__((noescape)) InflectionRangeVisitor visitor
);
typedef void (^InflectionEnumerateRangeBlock)(
    ONSRange range, __attribute__((noescape)) InflectionRangeVisitor visitor
);
typedef void (^InflectionRemoveAttributeBlock)(id key, ONSRange range);
typedef BOOL (^InflectionRangePredicate)(ONSRange range);
typedef id (^InflectionFirstValueBlock)(id key, ONSRange range);
typedef id (^InflectionSubstringBlock)(ONSRange range);
typedef void (^InflectionEnumerateSubstringsBlock)(
    ONSRange range, __attribute__((noescape)) InflectionSubstringVisitor visitor
);
typedef void (^InflectionReplaceBlock)(ONSRange range, id replacement);
typedef void (^InflectionAddAttributeBlock)(ONSRange range, id key, id value);
typedef id (^InflectionFormatAlternativeBlock)(id alternative);

// Native _NSInflect, 0x1c0530060 (was a private external): inflects the inflectable runs of a string through
// the accessors the caller supplies, then formats the localized-number runs and replaces the inflection
// alternatives. Signature from the native __PRETTY_FUNCTION__ string: locale, replacements
// (NSArray<_NSAttributedStringReplacement *>), concepts, term of address, then the length, preflight flag and
// eleven accessor blocks.
OF_EXTERN void _NSInflect(
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
);

// Interfaces of families not yet restored on this branch.

// Native inline _NSInflectionLog expansion. Captured call sites test the predicate at 0x22e776210,
// pass the global block at 0x22281c978 to dispatch_once, then load the log at 0x22e776208. These common
// variables are private externals.
extern __attribute__((visibility("hidden")))
dispatch_once_t inflectionLogOnceToken __asm(OF_SYMBOL("_NSInflectionLog.onceToken"));
extern __attribute__((visibility("hidden")))
os_log_t inflectionLogObject __asm(OF_SYMBOL("_NSInflectionLog.log"));
OF_EXTERN const uint8_t inflectionLogGlobalBlock[];

static inline os_log_t _NSInflectionLog(void) {
    if (inflectionLogOnceToken != -1) {
        dispatch_once(&inflectionLogOnceToken, (dispatch_block_t)(void *)inflectionLogGlobalBlock);
    }
    return inflectionLogObject;
}

// Attribute names of Foundation (__DATA_CONST,__const NSString pointers, external natively) that the unit
// loads through their symbols: NSInflectionRuleAttributeName 0x2227d8168, NSMorphologyAttributeName
// 0x2227d8160, NSInflectionReferentConceptAttributeName 0x2227d8170,
// NSInflectionAgreementConceptAttributeName 0x2227d8178, NSInflectionAgreementArgumentAttributeName
// 0x2227d8180, NSLocalizedNumberFormatAttributeName 0x2227d8190, NSInlinePresentationIntentAttributeName
// 0x2227d7b38, _NSInflectableAttributeName 0x2227d81a0, _NSInflectionAlternativeAttributeName 0x2227d81a8,
// _NSAssumedFallbackInflectionAttributeName 0x2227d81b0, _NSGrammarAttributeName 0x2227d8198 and
// _NSLanguageAttributeName 0x2227d7b78. Their definitions lie outside this family.

#ifdef __OBJC__
#import <objc/NSObject.h>

// Classes the inflection unit references by class symbol (runtime names mapped by the prefix tool). NSArray,
// NSDictionary, NSMutableArray, NSMutableDictionary, NSLocale and NSNull are CoreFoundation's; the others are
// Foundation's and outside this family.

#import "Foundation/String/ONSMorphology.h"

@interface ONSAttributedString (NSInflection)

- (id)_firstValueOfAttributeWithKey:(id)key inRange:(ONSRange)range;
- (id)_inflectedAttributedString;
- (id)_identicalAttributesInRange:(ONSRange)range;

@end

@interface ONSMutableAttributedString (NSInflection)

- (void)_addAttributesIfNotPresentMergingInlinePresentationIntents:(id)attributes toRange:(ONSRange)range;
- (id)_formatInflectionAlternative:(id)alternative withReplacements:(id)replacements;
- (void)_inflectWithLocale:(id)locale
              replacements:(id)replacements
                  concepts:(id)concepts
                 preflight:(BOOL)preflight;
- (void)enumerateInflectableBlocksInRange:(ONSRange)range usingBlock:(InflectionRangeVisitor)block;
- (void)enumerateFormattableBlocksInRange:(ONSRange)range usingBlock:(InflectionRangeVisitor)block;

@end
#endif

// Native _NSInflectionObserver, 0x22e6fcb60, non-external; unread setter outside the captured family.
extern __attribute__((visibility("hidden"))) id inflectionObserver __asm(OF_SYMBOL("_NSInflectionObserver"));
