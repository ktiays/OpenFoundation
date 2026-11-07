//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSLocale.h"

#import "CoreFoundation/Locale/OCFLocale.h"
#import "CoreFoundation/Objective-C/OCFNSObjectGlue.h"
#import "CoreFoundation/Preferences/OCFPreferences.h"
#import "CoreFoundation/String/OCFString.h"

// Inferred organization: the NSLocale methods of this image (0x1c02ad49c-0x1c02ad8fc, with the outlined
// fragments OUTLINED_FUNCTION_0_50 and _1_36 at 0x1c02ad974 and 0x1c02ad988, and -objectForKey: and
// -displayNameForKey:value: in the cold region at 0x1c031066c and 0x1c0310690) lie together in the iOS
// Simulator __text section; following the owner's decision, the class has its own translation unit.
// Objective-C compiled with manual reference counting (objc_retain, objc_autorelease). The native source path
// is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; the placeholder read
// with Local/Scratch/cf-core/macho.py); iOS Device 24A446: unread. Reading: not yet written; the evidence is
// the assembly in Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.
//
// The methods pass constant strings (ADRL of __cfstring literals), not the CFLocale key globals; the strings
// that equal mapped key names are spelled as split literals, so that their contents are the native names in
// both naming modes. The super send of +allocWithZone: passes the selector address 0x2387a9fd4.

// The NSLocale class reference (off_2227CE338) and the placeholder's class (_NSPlaceholderLocale, a class of
// this image not restored on this branch).
extern char localeClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSLocale));
extern char placeholderLocaleClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(_ONSPlaceholderLocale));

// Selectors this unit sends to the NSLocale class and to its objects that the class does not implement here,
// declared for the message sends only; no protocol object is emitted. The language directions and the
// Windows locale code are spelled as the SDK's NSLocaleLanguageDirection (NSUInteger) and uint32_t.
@protocol LocaleMessages

+ (id)_system;
+ (id)_current;
+ (id)_availableLocaleIdentifiers;
+ (id)_isoLanguageCodes;
+ (id)_isoCountryCodes;
+ (id)_isoCurrencyCodes;
+ (id)_commonISOCurrencyCodes;
+ (id)_preferredLanguages;
+ (id)_componentsFromLocaleIdentifier:(id)string;
+ (id)_localeIdentifierFromComponents:(id)dict;
+ (id)_canonicalLocaleIdentifierFromString:(id)string;
+ (id)_canonicalLanguageIdentifierFromString:(id)string;
+ (id)_localeIdentifierFromWindowsLocaleCode:(uint32_t)lcid;
+ (uint32_t)_windowsLocaleCodeFromLocaleIdentifier:(id)localeIdentifier;
+ (NSUInteger)_characterDirectionForLanguage:(id)isoLangCode;
+ (NSUInteger)_lineDirectionForLanguage:(id)isoLangCode;
- (BOOL)boolValue;

@end

// Native __placeholderLocale, 0x2227a88c8 (__DATA_CONST,__const, 16 bytes, before the locale keys).
// Non-external: an object whose isa is _NSPlaceholderLocale, then a zero word.
static const struct {
    const void *isa;
    uintptr_t unused;
} __placeholderLocale = {placeholderLocaleClass, 0};

@implementation ONSLocale

// Native +[NSLocale allocWithZone:], 0x1c02ad49c. Non-external. NSLocale itself (compared twice with its self
// class reference ___SCR_NSLocale, loaded twice; one comparison here) answers the placeholder; a subclass
// gets super's +allocWithZone:.
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == (Class)localeClass) {
        return (id)&__placeholderLocale;
    }
    return [super allocWithZone:zone];
}

// Native -[NSLocale init], 0x1c02ad524. Non-external; -initWithLocaleIdentifier: with nil (a tail call).
- (id)init {
    return [self initWithLocaleIdentifier:nil];
}

// Native -[NSLocale _cfTypeID], 0x1c02ad52c. Non-external; a tail call of CFLocaleGetTypeID.
- (OCFTypeID)_cfTypeID {
    return OCFLocaleGetTypeID();
}

// Native -[NSLocale hash], 0x1c02ad530. Non-external; the identifier's hash.
- (NSUInteger)hash {
    return [[self localeIdentifier] hash];
}

// Native -[NSLocale isEqual:], 0x1c02ad544. Non-external. The receiver itself is equal and nil is not; an
// object that is not a kind of NSLocale (objc_opt_self of the class reference) is not; otherwise the
// identifiers are compared with -isEqual: (a tail call).
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    if (![object isKindOfClass:[ONSLocale self]]) {
        return NO;
    }
    return [[self localeIdentifier] isEqual:[object localeIdentifier]];
}

// Native -[NSLocale copyWithZone:], 0x1c02ad5c0. Non-external; the receiver retained (objc_retain).
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native +[NSLocale supportsSecureCoding], 0x1c02ad5c4. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSLocale initWithCoder:], 0x1c02ad5cc. Non-external; nil (the receiver is not released).
- (id)initWithCoder:(id)coder {
    return nil;
}

// Native -[NSLocale encodeWithCoder:], 0x1c02ad5d4. Non-external; does nothing.
- (void)encodeWithCoder:(id)coder {
}

// Native -[NSLocale localeIdentifier], 0x1c02ad5d8. Non-external; -objectForKey: with
// "kCFLocaleIdentifierKey" (a tail call).
- (id)localeIdentifier {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "IdentifierKey"
                              )];
}

// Native -[NSLocale localizedStringForLocaleIdentifier:], 0x1c02ad5e4. Non-external;
// -displayNameForKey:value: with "kCFLocaleIdentifierKey" (a tail call).
- (id)localizedStringForLocaleIdentifier:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "IdentifierKey"
                                   )
                             value:value];
}

// Native -[NSLocale languageCode], 0x1c02ad5f4. Non-external; -objectForKey: with "kCFLocaleLanguageCodeKey"
// (a tail call).
- (id)languageCode {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "LanguageCodeKey"
                              )];
}

// Native -[NSLocale localizedStringForLanguageCode:], 0x1c02ad600. Non-external; -displayNameForKey:value:
// with "kCFLocaleLanguageCodeKey" (a tail call).
- (id)localizedStringForLanguageCode:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "LanguageCodeKey"
                                   )
                             value:value];
}

// Native -[NSLocale countryCode], 0x1c02ad610. Non-external; -objectForKey: with "kCFLocaleCountryCodeKey" (a
// tail call).
- (id)countryCode {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "CountryCodeKey"
                              )];
}

// Native -[NSLocale localizedStringForCountryCode:], 0x1c02ad61c. Non-external; -displayNameForKey:value:
// with "kCFLocaleCountryCodeKey" (a tail call).
- (id)localizedStringForCountryCode:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "CountryCodeKey"
                                   )
                             value:value];
}

// Native -[NSLocale scriptCode], 0x1c02ad62c. Non-external; -objectForKey: with "kCFLocaleScriptCodeKey" (a
// tail call).
- (id)scriptCode {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "ScriptCodeKey"
                              )];
}

// Native -[NSLocale localizedStringForScriptCode:], 0x1c02ad638. Non-external; -displayNameForKey:value: with
// "kCFLocaleScriptCodeKey" (a tail call).
- (id)localizedStringForScriptCode:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "ScriptCodeKey"
                                   )
                             value:value];
}

// Native -[NSLocale variantCode], 0x1c02ad648. Non-external; -objectForKey: with "kCFLocaleVariantCodeKey" (a
// tail call).
- (id)variantCode {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "VariantCodeKey"
                              )];
}

// Native -[NSLocale localizedStringForVariantCode:], 0x1c02ad654. Non-external; -displayNameForKey:value:
// with "kCFLocaleVariantCodeKey" (a tail call).
- (id)localizedStringForVariantCode:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "VariantCodeKey"
                                   )
                             value:value];
}

// Native -[NSLocale calendarIdentifier], 0x1c02ad664. Non-external; -objectForKey: with "calendar" (a tail
// call).
- (id)calendarIdentifier {
    return [self objectForKey:(id)OF_STR("calendar")];
}

// Native -[NSLocale localizedStringForCalendarIdentifier:], 0x1c02ad670. Non-external;
// -displayNameForKey:value: with "calendar" (a tail call).
- (id)localizedStringForCalendarIdentifier:(id)value {
    return [self displayNameForKey:(id)OF_STR("calendar") value:value];
}

// Native -[NSLocale collationIdentifier], 0x1c02ad680. Non-external; -objectForKey: with "collation" (a tail
// call).
- (id)collationIdentifier {
    return [self objectForKey:(id)OF_STR("collation")];
}

// Native -[NSLocale localizedStringForCollationIdentifier:], 0x1c02ad68c. Non-external;
// -displayNameForKey:value: with "collation" (a tail call).
- (id)localizedStringForCollationIdentifier:(id)value {
    return [self displayNameForKey:(id)OF_STR("collation") value:value];
}

// Native -[NSLocale decimalSeparator], 0x1c02ad69c. Non-external; -objectForKey: with
// "kCFLocaleDecimalSeparatorKey" (a tail call).
- (id)decimalSeparator {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "DecimalSeparatorKey"
                              )];
}

// Native -[NSLocale localizedStringForDecimalSeparator:], 0x1c02ad6a8. Non-external;
// -displayNameForKey:value: with "kCFLocaleDecimalSeparatorKey" (a tail call).
- (id)localizedStringForDecimalSeparator:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "DecimalSeparatorKey"
                                   )
                             value:value];
}

// Native -[NSLocale groupingSeparator], 0x1c02ad6b8. Non-external; -objectForKey: with
// "kCFLocaleGroupingSeparatorKey" (a tail call).
- (id)groupingSeparator {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "GroupingSeparatorKey"
                              )];
}

// Native -[NSLocale localizedStringForGroupingSeparator:], 0x1c02ad6c4. Non-external;
// -displayNameForKey:value: with "kCFLocaleGroupingSeparatorKey" (a tail call).
- (id)localizedStringForGroupingSeparator:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "GroupingSeparatorKey"
                                   )
                             value:value];
}

// Native -[NSLocale currencySymbol], 0x1c02ad6d4. Non-external; -objectForKey: with
// "kCFLocaleCurrencySymbolKey" (a tail call).
- (id)currencySymbol {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "CurrencySymbolKey"
                              )];
}

// Native -[NSLocale localizedStringForCurrencySymbol:], 0x1c02ad6e0. Non-external; -displayNameForKey:value:
// with "kCFLocaleCurrencySymbolKey" (a tail call).
- (id)localizedStringForCurrencySymbol:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "CurrencySymbolKey"
                                   )
                             value:value];
}

// Native -[NSLocale currencyCode], 0x1c02ad6f0. Non-external; -objectForKey: with "currency" (a tail call).
- (id)currencyCode {
    return [self objectForKey:(id)OF_STR("currency")];
}

// Native -[NSLocale localizedStringForCurrencyCode:], 0x1c02ad6fc. Non-external; -displayNameForKey:value:
// with "currency" (a tail call).
- (id)localizedStringForCurrencyCode:(id)value {
    return [self displayNameForKey:(id)OF_STR("currency") value:value];
}

// Native -[NSLocale collatorIdentifier], 0x1c02ad70c. Non-external; -objectForKey: with
// "kCFLocaleCollatorIdentifierKey" (a tail call).
- (id)collatorIdentifier {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "CollatorIdentifierKey"
                              )];
}

// Native -[NSLocale localizedStringForCollatorIdentifier:], 0x1c02ad718. Non-external;
// -displayNameForKey:value: with "kCFLocaleCollatorIdentifierKey" (a tail call).
- (id)localizedStringForCollatorIdentifier:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "CollatorIdentifierKey"
                                   )
                             value:value];
}

// Native -[NSLocale quotationBeginDelimiter], 0x1c02ad728. Non-external; -objectForKey: with
// "kCFLocaleQuotationBeginDelimiterKey" (a tail call).
- (id)quotationBeginDelimiter {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "QuotationBeginDelimiterKey"
                              )];
}

// Native -[NSLocale localizedStringForQuotationBeginDelimiter:], 0x1c02ad734. Non-external;
// -displayNameForKey:value: with "kCFLocaleQuotationBeginDelimiterKey" (a tail call).
- (id)localizedStringForQuotationBeginDelimiter:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "QuotationBeginDelimiterKey"
                                   )
                             value:value];
}

// Native -[NSLocale quotationEndDelimiter], 0x1c02ad744. Non-external; -objectForKey: with
// "kCFLocaleQuotationEndDelimiterKey" (a tail call).
- (id)quotationEndDelimiter {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "QuotationEndDelimiterKey"
                              )];
}

// Native -[NSLocale localizedStringForQuotationEndDelimiter:], 0x1c02ad750. Non-external;
// -displayNameForKey:value: with "kCFLocaleQuotationEndDelimiterKey" (a tail call).
- (id)localizedStringForQuotationEndDelimiter:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "QuotationEndDelimiterKey"
                                   )
                             value:value];
}

// Native -[NSLocale alternateQuotationBeginDelimiter], 0x1c02ad760. Non-external; -objectForKey: with
// "kCFLocaleAlternateQuotationBeginDelimiterKey" (a tail call).
- (id)alternateQuotationBeginDelimiter {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "AlternateQuotationBeginDelimiterKey"
                              )];
}

// Native -[NSLocale localizedStringForAlternateQuotationBeginDelimiter:], 0x1c02ad76c. Non-external;
// -displayNameForKey:value: with "kCFLocaleAlternateQuotationBeginDelimiterKey" (a tail call).
- (id)localizedStringForAlternateQuotationBeginDelimiter:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "AlternateQuotationBeginDelimiterKey"
                                   )
                             value:value];
}

// Native -[NSLocale alternateQuotationEndDelimiter], 0x1c02ad77c. Non-external; -objectForKey: with
// "kCFLocaleAlternateQuotationEndDelimiterKey" (a tail call).
- (id)alternateQuotationEndDelimiter {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "AlternateQuotationEndDelimiterKey"
                              )];
}

// Native -[NSLocale localizedStringForAlternateQuotationEndDelimiter:], 0x1c02ad788. Non-external;
// -displayNameForKey:value: with "kCFLocaleAlternateQuotationEndDelimiterKey" (a tail call).
- (id)localizedStringForAlternateQuotationEndDelimiter:(id)value {
    return [self displayNameForKey:(id)OF_STR(
                                       "kCFLocale"
                                       "AlternateQuotationEndDelimiterKey"
                                   )
                             value:value];
}

// Native -[NSLocale exemplarCharacterSet], 0x1c02ad798. Non-external; -objectForKey: with
// "kCFLocaleExemplarCharacterSetKey" (a tail call).
- (id)exemplarCharacterSet {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "ExemplarCharacterSetKey"
                              )];
}

// Native -[NSLocale usesMetricSystem], 0x1c02ad7a4. Non-external; -boolValue (a tail call) of -objectForKey:
// with "kCFLocaleUsesMetricSystemKey".
- (BOOL)usesMetricSystem {
    return [[self objectForKey:(id)OF_STR(
                                   "kCFLocale"
                                   "UsesMetricSystemKey"
                               )] boolValue];
}

// Native -[NSLocale regionCode], 0x1c02ad7c0. Non-external; -objectForKey: with "kCFLocaleCountryCodeKey" (a
// tail call).
- (id)regionCode {
    return [self objectForKey:(id)OF_STR(
                                  "kCFLocale"
                                  "CountryCodeKey"
                              )];
}

// Native -[NSLocale languageIdentifier], 0x1c02ad7cc. Non-external; -objectForKey: with
// "locale:languageIdentifier" (a tail call).
- (id)languageIdentifier {
    return [self objectForKey:(id)OF_STR("locale:languageIdentifier")];
}

// Native -[NSLocale identifier], 0x1c02ad7d8. Non-external; a tail call of -localeIdentifier.
- (id)identifier {
    return [self localeIdentifier];
}

// Native -[NSLocale initWithLocaleIdentifier:], 0x1c02ad7dc. Non-external; returns the receiver unchanged.
- (id)initWithLocaleIdentifier:(id)string {
    return self;
}

// Native +[NSLocale systemLocale], 0x1c02ad7e0. Non-external; +_system sent to the NSLocale class reference
// (a tail call).
+ (id)systemLocale {
    return [(id)localeClass _system];
}

// Native +[NSLocale currentLocale], 0x1c02ad7ec. Non-external; +_current sent to the NSLocale class reference
// (a tail call).
+ (id)currentLocale {
    return [(id)localeClass _current];
}

// Native +[NSLocale localeWithLocaleIdentifier:], 0x1c02ad7f8. Non-external.
+ (id)localeWithLocaleIdentifier:(id)ident {
    return [[[self alloc] initWithLocaleIdentifier:ident] autorelease];
}

// Native +[NSLocale availableLocaleIdentifiers], 0x1c02ad820. Non-external; +_availableLocaleIdentifiers sent
// to the NSLocale class reference (a tail call).
+ (id)availableLocaleIdentifiers {
    return [(id)localeClass _availableLocaleIdentifiers];
}

// Native +[NSLocale ISOLanguageCodes], 0x1c02ad82c. Non-external; +_isoLanguageCodes sent to the NSLocale
// class reference (a tail call).
+ (id)ISOLanguageCodes {
    return [(id)localeClass _isoLanguageCodes];
}

// Native +[NSLocale ISOCountryCodes], 0x1c02ad838. Non-external; +_isoCountryCodes sent to the NSLocale class
// reference (a tail call).
+ (id)ISOCountryCodes {
    return [(id)localeClass _isoCountryCodes];
}

// Native +[NSLocale ISOCurrencyCodes], 0x1c02ad844. Non-external; +_isoCurrencyCodes sent to the NSLocale
// class reference (a tail call).
+ (id)ISOCurrencyCodes {
    return [(id)localeClass _isoCurrencyCodes];
}

// Native +[NSLocale commonISOCurrencyCodes], 0x1c02ad850. Non-external; +_commonISOCurrencyCodes sent to the
// NSLocale class reference (a tail call).
+ (id)commonISOCurrencyCodes {
    return [(id)localeClass _commonISOCurrencyCodes];
}

// Native +[NSLocale preferredLanguages], 0x1c02ad85c. Non-external; +_preferredLanguages sent to the NSLocale
// class reference (a tail call).
+ (id)preferredLanguages {
    return [(id)localeClass _preferredLanguages];
}

// Native +[NSLocale componentsFromLocaleIdentifier:], 0x1c02ad868. Non-external;
// +_componentsFromLocaleIdentifier: sent to the NSLocale class reference (a tail call).
+ (id)componentsFromLocaleIdentifier:(id)string {
    return [(id)localeClass _componentsFromLocaleIdentifier:string];
}

// Native +[NSLocale localeIdentifierFromComponents:], 0x1c02ad874. Non-external;
// +_localeIdentifierFromComponents: sent to the NSLocale class reference (a tail call).
+ (id)localeIdentifierFromComponents:(id)dict {
    return [(id)localeClass _localeIdentifierFromComponents:dict];
}

// Native +[NSLocale canonicalLocaleIdentifierFromString:], 0x1c02ad880. Non-external. nil answers nil;
// otherwise +_canonicalLocaleIdentifierFromString: sent to the NSLocale class reference (a tail call).
+ (id)canonicalLocaleIdentifierFromString:(id)string {
    if (string == nil) {
        return nil;
    }
    return [(id)localeClass _canonicalLocaleIdentifierFromString:string];
}

// Native +[NSLocale canonicalLanguageIdentifierFromString:], 0x1c02ad898. Non-external. nil answers nil;
// otherwise +_canonicalLanguageIdentifierFromString: sent to the NSLocale class reference (a tail call).
+ (id)canonicalLanguageIdentifierFromString:(id)string {
    if (string == nil) {
        return nil;
    }
    return [(id)localeClass _canonicalLanguageIdentifierFromString:string];
}

// Native +[NSLocale localeIdentifierFromWindowsLocaleCode:], 0x1c02ad8b0. Non-external; a tail call.
+ (id)localeIdentifierFromWindowsLocaleCode:(uint32_t)lcid {
    return [(id)localeClass _localeIdentifierFromWindowsLocaleCode:lcid];
}

// Native +[NSLocale windowsLocaleCodeFromLocaleIdentifier:], 0x1c02ad8bc. Non-external; a tail call.
+ (uint32_t)windowsLocaleCodeFromLocaleIdentifier:(id)localeIdentifier {
    return [(id)localeClass _windowsLocaleCodeFromLocaleIdentifier:localeIdentifier];
}

// Native +[NSLocale characterDirectionForLanguage:], 0x1c02ad8c8. Non-external; a tail call.
+ (NSUInteger)characterDirectionForLanguage:(id)isoLangCode {
    return [(id)localeClass _characterDirectionForLanguage:isoLangCode];
}

// Native +[NSLocale lineDirectionForLanguage:], 0x1c02ad8d4. Non-external; a tail call.
+ (NSUInteger)lineDirectionForLanguage:(id)isoLangCode {
    return [(id)localeClass _lineDirectionForLanguage:isoLangCode];
}

// Native -[NSLocale _copyDisplayNameForKey:value:], 0x1c02ad8e0. Non-external; -displayNameForKey:value:,
// retained (objc_retain, a tail call).
- (id)_copyDisplayNameForKey:(id)key value:(id)value {
    return [[self displayNameForKey:key value:value] retain];
}

// Native -[NSLocale _prefForKey:], 0x1c02ad8f4. Non-external; nil.
- (id)_prefForKey:(id)key {
    return nil;
}

// Native +[NSLocale internetServicesRegion], 0x1c02ad8fc. Non-external.
// The application preference "AppleInternetServicesRegion" (CFPreferencesCopyAppValue with
// kCFPreferencesCurrentApplication) is answered autoreleased when it is a string, and released otherwise;
// without a string, +currentLocale (sent to the NSLocale class reference) -objectForKey: with
// "kCFLocaleCountryCodeKey" (a tail call).
+ (id)internetServicesRegion {
    OCFPropertyListRef value =
        OCFPreferencesCopyAppValue(OF_STR("AppleInternetServicesRegion"), kOCFPreferencesCurrentApplication);
    if (value != NULL) {
        if (OCFGetTypeID(value) == OCFStringGetTypeID()) {
            return [(id)value autorelease];
        }
        OCFRelease(value);
    }
    return [[(id)localeClass currentLocale] objectForKey:(id)OF_STR(
                                                             "kCFLocale"
                                                             "CountryCodeKey"
                                                         )];
}

// Native -[NSLocale objectForKey:], 0x1c031066c (cold region), with OUTLINED_FUNCTION_0_50 (0x1c02ad974:
// __CFLookUpClass("NSLocale")) and _1_36 (0x1c02ad988). Non-external. The abstract method:
// __CFRequireConcreteImplementation, then nil.
- (id)objectForKey:(id)key {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSLocale)), self, _cmd);
    return nil;
}

// Native -[NSLocale displayNameForKey:value:], 0x1c0310690 (cold region), with the same outlined fragments.
// Non-external. The abstract method: __CFRequireConcreteImplementation, then nil.
- (id)displayNameForKey:(id)key value:(id)value {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSLocale)), self, _cmd);
    return nil;
}

@end
