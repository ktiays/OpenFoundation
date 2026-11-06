//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringFormatting.h"
#import "Foundation/String/ONSAttributedStringInternal.h"
#import "Foundation/String/ONSAttributedStringInflection.h"
#include "CoreFoundation/Collections/OCFNSCFDictionary.h"

// Inferred organization: NSAttributedStringFormattingSPI. Evidence:
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json, FC-sets-attributed. iOS Simulator 24A434,
// arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless noted.
// iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md. The assembly settles arguments omitted by
// Hex-Rays: the first enumeration passes {sourceCursor, span}, 0x100000 and the block. Shared-cache bindings
// (confidence: medium): NSDictionary 0x22227fad0, NSLocale 0x22227fb00, kCFAllocatorSystemDefault
// 0x222280b70; metadata keys at 0x222280890 AddedIsolates, 0x2222808a0 ArgumentObject, 0x2222808a8
// ReplacementIndex, 0x2222808b0/8b8 ReplacementRangeLength/Location, 0x2222808c0/8c8
// SpecifierRangeLength/Location. Symbol metadata confirms the imports, but these slots lie outside the
// extracted image. Blocks capture object references as __unsafe_unretained to preserve the native 0xc0000000
// stack blocks' absence of object copy/dispose helpers. The byref flag is a single byte.

extern char formattingNumberClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNumber));
extern char formattingLocaleClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSLocale));
extern char
    formattingReplacementClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(_ONSAttributedStringReplacement));

@protocol AttributedStringFormattingMessages
- (id)pluralizationNumber;
- (id)localeIdentifier;
+ (id)currentLocale;
- (id)__baseAttributedString;
- (NSInteger)integerValue;
- (BOOL)boolValue;
+ (id)_replacementsFromMetadataArray:(id)metadata;
@end

static OCFStringRef _NSAttributedStringDescriptionWithLocaleFunc(void *object, const void *locale);

@implementation ONSAttributedString (NSAttributedStringFormattingSPI)

// Native -[NSAttributedString(NSAttributedStringFormattingSPI) _initWithFormat:options:locale:arguments:],
// 0x1c0548b6c. Non-external.
- (id)_initWithFormat:(id)format options:(NSUInteger)options locale:(id)locale arguments:(va_list)arguments {
    return [self __initWithFormat:format
                 attributeOptions:options
                formattingOptions:nil
                           locale:locale
                        arguments:arguments];
}

// Native -[NSAttributedString(NSAttributedStringFormattingSPI)
// __initWithFormat:attributeOptions:formattingOptions:locale:arguments:], 0x1c0548b7c. Non-external.
- (id)__initWithFormat:(id)format
      attributeOptions:(NSUInteger)attributeOptions
     formattingOptions:(id)formattingOptions
                locale:(id)locale
             arguments:(va_list)arguments {
    return [self __initWithFormat:format
                 attributeOptions:attributeOptions
                formattingOptions:formattingOptions
                           locale:locale
                          context:(id)&__ONSDictionary0__struct
                        arguments:arguments];
}

// Native -[NSAttributedString(NSAttributedStringFormattingSPI)
// __initWithFormat:attributeOptions:formattingOptions:locale:context:arguments:], 0x1c0548b8c.
// Non-external. Reapply literal attributes around each replacement, then the argument's own attributes;
// optional replacement indices follow, then inflection and removal of the format-configuration attribute.
- (id)__initWithFormat:(id)format
      attributeOptions:(NSUInteger)attributeOptions
     formattingOptions:(id)formattingOptions
                locale:(id)locale
               context:(id)context
             arguments:(va_list)arguments {
    if ([format length] == 0) {
        return [self initWithString:(id)OF_STR("")];
    }
    ONSRange configurationRange = {0, 0};
    id metadata = nil;
    NSUInteger formatLength = [format length];
    id configuration = [format attribute:(id)OF_STR("NSFormatSpecifierConfiguration")
                                 atIndex:0
                   longestEffectiveRange:&configurationRange
                                 inRange:(ONSRange){0, formatLength}];
    formatLength = [format length];
    if (configurationRange.length != formatLength || configurationRange.location != 0) {
        configuration = nil;
    }
    if (_os_feature_enabled_impl("Foundation", "LanguageAwareStringStatisticalLocalizedStringWithFormat")) {
        if ([[locale localeIdentifier] length] == 0) {
            locale = [(id)formattingLocaleClass currentLocale];
        }
    }
    id pluralizationNumber = [formattingOptions pluralizationNumber];
    id formatString = [format string];
    OCFStringRef formatted = _OCFStringCreateWithFormatAndArgumentsReturningMetadata(
        kOCFAllocatorSystemDefault,
        _NSAttributedStringDescriptionWithLocaleFunc,
        _ONSDescriptionWithStringProxyFunc,
        (OCFDictionaryRef)locale,
        (OCFDictionaryRef)configuration,
        (OCFTypeRef)pluralizationNumber,
        (OCFStringRef)formatString,
        (OCFArrayRef *)&metadata,
        arguments
    );
    __block uint8_t hasInflection = 0;
    if (formatted == NULL) {
        objc_release(metadata);
        return [self initWithString:(id)OF_STR("")];
    }
    __unsafe_unretained id output =
        [objc_alloc((Class)attributedStringMutableBaseClass) initWithString:(id)formatted];
    formatLength = [format length];
    NSUInteger count = [metadata count];
    if ((NSInteger)count >= 0) {
        NSUInteger sourceCursor = 0;
        NSUInteger targetCursor = 0;
        id sourceLocationKey = (id)_kOCFStringFormatMetadataSpecifierRangeLocationInFormatStringKey;
        id sourceLengthKey = (id)_kOCFStringFormatMetadataSpecifierRangeLengthInFormatStringKey;
        id targetLocationKey = (id)_kOCFStringFormatMetadataReplacementRangeLocationKey;
        id targetLengthKey = (id)_kOCFStringFormatMetadataReplacementRangeLengthKey;
        id argumentKey = (id)_kOCFStringFormatMetadataArgumentObjectKey;
        id isolatesKey = (id)_kOCFStringFormatMetadataAddedIsolatesKey;
        for (NSUInteger index = 0; index != count + 1; index++) {
            id record = (NSInteger)index < (NSInteger)count ? [metadata objectAtIndexedSubscript:index] : nil;
            id locationNumber = [record objectForKeyedSubscript:sourceLocationKey];
            Class numberClass = objc_opt_class((id)formattingNumberClass);
            BOOL locationValid = objc_opt_isKindOfClass(locationNumber, (id)numberClass);
            id lengthNumber = [record objectForKeyedSubscript:sourceLengthKey];
            numberClass = objc_opt_class((id)formattingNumberClass);
            BOOL lengthValid = objc_opt_isKindOfClass(lengthNumber, (id)numberClass);
            NSUInteger sourceLocation = 0x7FFFFFFFFFFFFFFF;
            NSUInteger sourceLength = 0x7FFFFFFFFFFFFFFF;
            if ((locationValid & (locationNumber != nil) & lengthValid) == 1 && lengthNumber != nil) {
                sourceLocation = (NSUInteger)[locationNumber integerValue];
                sourceLength = (NSUInteger)[lengthNumber integerValue];
            }
            NSUInteger span = (record != nil ? sourceLocation : formatLength) - sourceCursor;
            // Native block invoke 0x1c05494e4. Non-external. Check keys in native short-circuit order.
            [format
                enumerateAttributesInRange:(ONSRange){sourceCursor, span}
                                   options:0x100000
                                usingBlock:^(id attributes, ONSRange range, BOOL *stop) {
                                  (void)stop;
                                  if ([attributes objectForKeyedSubscript:ONSInflectionRuleAttributeName] !=
                                          nil ||
                                      [attributes
                                          objectForKeyedSubscript:_ONSInflectionAlternativeAttributeName] !=
                                          nil ||
                                      [attributes objectForKeyedSubscript:
                                                      ONSInflectionReferentConceptAttributeName] != nil ||
                                      [attributes objectForKeyedSubscript:
                                                      ONSInflectionAgreementConceptAttributeName] != nil ||
                                      [attributes objectForKeyedSubscript:
                                                      ONSInflectionAgreementArgumentAttributeName] != nil ||
                                      [attributes
                                          objectForKeyedSubscript:ONSLocalizedNumberFormatAttributeName] !=
                                          nil) {
                                      hasInflection = 1;
                                  }
                                  [output addAttributes:attributes
                                                  range:(ONSRange){
                                                            targetCursor + range.location - sourceCursor,
                                                            range.length
                                                        }];
                                }];
            if (record == nil) {
                break;
            }
            locationNumber = [record objectForKeyedSubscript:targetLocationKey];
            numberClass = objc_opt_class((id)formattingNumberClass);
            locationValid = objc_opt_isKindOfClass(locationNumber, (id)numberClass);
            lengthNumber = [record objectForKeyedSubscript:targetLengthKey];
            numberClass = objc_opt_class((id)formattingNumberClass);
            lengthValid = objc_opt_isKindOfClass(lengthNumber, (id)numberClass);
            NSUInteger targetLocation = 0x7FFFFFFFFFFFFFFF;
            NSUInteger targetLength = 0x7FFFFFFFFFFFFFFF;
            if ((locationValid & (locationNumber != nil) & lengthValid) == 1 && lengthNumber != nil) {
                targetLocation = (NSUInteger)[locationNumber integerValue];
                targetLength = (NSUInteger)[lengthNumber integerValue];
            }
            id argument = [record objectForKeyedSubscript:argumentKey];
            Class argumentClass = objc_opt_class((id)attributedStringBaseClass);
            BOOL attributedArgument = objc_opt_isKindOfClass(argument, (id)argumentClass);
            if ((attributedArgument & (argument != nil)) == 0) {
                argumentClass = objc_opt_class((id)attributedStringStringClass);
                BOOL stringArgument = objc_opt_isKindOfClass(argument, (id)argumentClass);
                id string = (stringArgument & 1) ? argument : nil;
                argument = [string __baseAttributedString];
            }
            id isolatesNumber = [record objectForKeyedSubscript:isolatesKey];
            numberClass = objc_opt_class((id)formattingNumberClass);
            BOOL isolatesValid = objc_opt_isKindOfClass(isolatesNumber, (id)numberClass);
            id validIsolatesNumber = (isolatesValid & 1) ? isolatesNumber : nil;
            uint32_t isolates = (uint32_t)[validIsolatesNumber boolValue];
            id attributes = [format attributesAtIndex:sourceLocation effectiveRange:NULL];
            if (attributes != nil && ((attributeOptions & (argument != nil)) & 1) == 0) {
                [output addAttributes:attributes range:(ONSRange){targetLocation, targetLength}];
            }
            if (isolates != 0) {
                [output addAttributes:attributes range:(ONSRange){targetLocation - 1, 1}];
                [output addAttributes:attributes range:(ONSRange){targetLocation + targetLength, 1}];
            }
            if (argument != nil) {
                NSUInteger argumentLength = [argument length];
                // Native block invoke_2, 0x1c05495cc. Non-external. Its captured target length is unused.
                [argument
                    enumerateAttributesInRange:(ONSRange){0, argumentLength}
                                       options:0x100000
                                    usingBlock:^(id argumentAttributes, ONSRange range, BOOL *stop) {
                                      (void)stop;
                                      if ([argumentAttributes
                                              objectForKeyedSubscript:ONSInflectionRuleAttributeName] !=
                                              nil ||
                                          [argumentAttributes objectForKeyedSubscript:
                                                                  _ONSInflectionAlternativeAttributeName] !=
                                              nil ||
                                          [argumentAttributes
                                              objectForKeyedSubscript:
                                                  ONSInflectionReferentConceptAttributeName] != nil ||
                                          [argumentAttributes
                                              objectForKeyedSubscript:
                                                  ONSInflectionAgreementConceptAttributeName] != nil ||
                                          [argumentAttributes
                                              objectForKeyedSubscript:
                                                  ONSInflectionAgreementArgumentAttributeName] != nil ||
                                          [argumentAttributes objectForKeyedSubscript:
                                                                  ONSLocalizedNumberFormatAttributeName] !=
                                              nil) {
                                          hasInflection = 1;
                                      }
                                      [output addAttributes:argumentAttributes
                                                      range:(ONSRange){
                                                                targetLocation + range.location, range.length
                                                            }];
                                    }];
            }
            sourceCursor = sourceLocation + sourceLength;
            targetCursor = targetLocation + targetLength + isolates;
        }
    }
    if (attributeOptions & 2) {
        id replacementKey = (id)_kOCFStringFormatMetadataReplacementIndexKey;
        id locationKey = (id)_kOCFStringFormatMetadataReplacementRangeLocationKey;
        id lengthKey = (id)_kOCFStringFormatMetadataReplacementRangeLengthKey;
        for (id record in metadata) {
            id replacementNumber = [record objectForKeyedSubscript:replacementKey];
            Class numberClass = objc_opt_class((id)formattingNumberClass);
            BOOL replacementValid = objc_opt_isKindOfClass(replacementNumber, (id)numberClass);
            id replacement = (replacementValid & 1) ? replacementNumber : nil;
            id locationNumber = [record objectForKeyedSubscript:locationKey];
            numberClass = objc_opt_class((id)formattingNumberClass);
            BOOL locationValid = objc_opt_isKindOfClass(locationNumber, (id)numberClass);
            id lengthNumber = [record objectForKeyedSubscript:lengthKey];
            numberClass = objc_opt_class((id)formattingNumberClass);
            BOOL lengthValid = objc_opt_isKindOfClass(lengthNumber, (id)numberClass);
            NSUInteger location = 0x7FFFFFFFFFFFFFFF;
            NSUInteger length = 0x7FFFFFFFFFFFFFFF;
            if ((locationValid & lengthValid & (locationNumber != nil)) == 1 && lengthNumber != nil) {
                location = (NSUInteger)[locationNumber integerValue];
                length = (NSUInteger)[lengthNumber integerValue];
            }
            if (replacement != nil && location != 0x7FFFFFFFFFFFFFFF && length != 0x7FFFFFFFFFFFFFFF) {
                id key = (id)OF_STR("NSReplacementIndex");
                id attributes = [(id)attributedStringDictionaryClass dictionaryWithObjects:&replacement
                                                                                   forKeys:&key
                                                                                     count:1];
                [output addAttributes:attributes range:(ONSRange){location, length}];
            }
        }
    }
    objc_release((id)formatted);
    BOOL needsInflection = (hasInflection & 1) != 0;
    if (!needsInflection) {
        NSUInteger length = [output length];
        // Native block invoke_3, 0x1c05496b0. Non-external. Stop at the first inflection-bearing run.
        [output
            enumerateAttributesInRange:(ONSRange){0, length}
                               options:0x100000
                            usingBlock:^(id attributes, ONSRange range, BOOL *stop) {
                              (void)range;
                              if ([attributes objectForKeyedSubscript:ONSInflectionRuleAttributeName] !=
                                      nil ||
                                  [attributes
                                      objectForKeyedSubscript:_ONSInflectionAlternativeAttributeName] !=
                                      nil ||
                                  [attributes
                                      objectForKeyedSubscript:ONSInflectionReferentConceptAttributeName] !=
                                      nil ||
                                  [attributes
                                      objectForKeyedSubscript:ONSInflectionAgreementConceptAttributeName] !=
                                      nil ||
                                  [attributes
                                      objectForKeyedSubscript:ONSInflectionAgreementArgumentAttributeName] !=
                                      nil ||
                                  [attributes
                                      objectForKeyedSubscript:ONSLocalizedNumberFormatAttributeName] != nil) {
                                  hasInflection = 1;
                                  *stop = YES;
                              }
                            }];
        needsInflection = hasInflection == 1;
    }
    if (needsInflection) {
        id replacements = [(id)formattingReplacementClass _replacementsFromMetadataArray:metadata];
        id concepts = [context objectForKeyedSubscript:(id)OF_STR("NSContextInflectionConcepts")];
        [output _inflectWithLocale:locale replacements:replacements concepts:concepts preflight:NO];
    }
    NSUInteger length = [output length];
    [output removeAttribute:(id)OF_STR("NSFormatSpecifierConfiguration") range:(ONSRange){0, length}];
    id result = [self initWithAttributedString:output];
    objc_release(metadata);
    objc_release(output);
    return result;
}

@end

// Native __NSAttributedStringDescriptionWithLocaleFunc, 0x1c0549494. Non-external.
static OCFStringRef _NSAttributedStringDescriptionWithLocaleFunc(void *object, const void *locale) {
    Class cls = objc_opt_class((id)attributedStringBaseClass);
    if ((objc_opt_isKindOfClass((id)object, (id)cls) & 1) != 0) {
        object = [(id)object string];
    }
    return _ONSDescriptionWithLocaleFunc(object, locale);
}
