//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/OCFLocaleKeys.h"

#include "CoreFoundation/Locale/OCFCalendarEntryPoints.h"
#include "CoreFoundation/Locale/OCFDateFormatter.h"
#include "CoreFoundation/Locale/OCFLocaleEntryPoints.h"
#include "CoreFoundation/String/OCFString.h"

// Inferred organization: the CFLocale keys (from 0x2227a88d8, which the NSLocale names share), the
// CFDateFormatter keys and the CFNumberFormatter keys (0x2227a8ac0-0x2227a8bf8) are one run of eight-byte
// pointers in the iOS Simulator __DATA_CONST,__const section, before the NSCalendar identifiers; the run is
// the locale-keys table, kept in its own translation unit. kCFLocaleLanguageCodeKey, kCFLocaleCountryCodeKey,
// kCFLocaleScriptCodeKey, kCFLocaleVariantCodeKey, kCFLocaleCalendarIdentifierKey,
// kCFLocaleDecimalSeparatorKey, kCFLocaleCalendarKey, the 31 CFDateFormatter keys the SDK and the restored
// units name (0x2227a89a8-0x2227a8a98) and the CFNumberFormatter keys the restored units compare are defined
// here, in native address order, followed by kCFGregorianCalendar and kCFChineseCalendar; the other keys of
// the table are not restored on this branch. Locale/OCFLocaleKeys.h declares them all. The native source
// path is not in the evidence. iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (each pointer and its constant string, flags 0x7c8, read with Local/Scratch/cf-core/macho.py;
// every string equals its constant's name, except kCFLocaleCalendarIdentifierKey's "calendar" and the
// calendar identifiers). iOS Device 24A446: unread. Reading: not yet written; the evidence is the static data
// at the addresses above.
//
// The constant strings (0x2227b9580-0x2227ba280, __DATA_CONST,__cfstring) whose contents are key names are
// spelled as split literals: their contents are the native names, in both naming modes.

// Native kCFLocaleLanguageCodeKey, 0x2227a88e0 (its constant string at 0x2227b96e0). External; natively the
// same address as NSLocaleLanguageCode, an alias this branch does not define. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json ("data").
static const OCFConstantString localeLanguageCodeKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFLocale"
    "LanguageCodeKey",
    24
};
const OCFStringRef kOCFLocaleLanguageCodeKey = (OCFStringRef)&localeLanguageCodeKeyString;

// Native kCFLocaleCountryCodeKey, 0x2227a88e8 (its constant string at 0x2227b9600). External; natively the
// same address as NSLocaleCountryCode and NSLocaleCountryCodeKey, aliases this branch does not define.
static const OCFConstantString localeCountryCodeKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFLocale"
    "CountryCodeKey",
    23
};
const OCFStringRef kOCFLocaleCountryCodeKey = (OCFStringRef)&localeCountryCodeKeyString;

// Native kCFLocaleScriptCodeKey, 0x2227a88f0 (its constant string at 0x2227b97c0). External; natively the
// same address as NSLocaleScriptCode and NSLocaleScriptCodeKey, aliases this branch does not define.
static const OCFConstantString localeScriptCodeKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFLocale"
    "ScriptCodeKey",
    22
};
const OCFStringRef kOCFLocaleScriptCodeKey = (OCFStringRef)&localeScriptCodeKeyString;

// Native kCFLocaleVariantCodeKey, 0x2227a88f8 (its constant string at 0x2227b9800). External; natively the
// same address as NSLocaleVariantCode and NSLocaleVariantCodeKey, aliases this branch does not define.
static const OCFConstantString localeVariantCodeKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFLocale"
    "VariantCodeKey",
    23
};
const OCFStringRef kOCFLocaleVariantCodeKey = (OCFStringRef)&localeVariantCodeKeyString;

// Native kCFLocaleCalendarIdentifierKey, 0x2227a8900 (its constant string at 0x2227b9580). External; natively
// the same address as NSLocaleCalendarIdentifier and NSLocaleCalendarIdentifierKey, aliases this branch does
// not define.
static const OCFConstantString localeCalendarIdentifierKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "calendar", 8
};
const OCFStringRef kOCFLocaleCalendarIdentifierKey = (OCFStringRef)&localeCalendarIdentifierKeyString;

// Native kCFLocaleDecimalSeparatorKey, 0x2227a8910 (its constant string at 0x2227b9660). External; natively
// the same address as NSLocaleDecimalSeparator, an alias this branch does not define. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json ("data").
static const OCFConstantString localeDecimalSeparatorKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFLocale"
    "DecimalSeparatorKey",
    28
};
const OCFStringRef kOCFLocaleDecimalSeparatorKey = (OCFStringRef)&localeDecimalSeparatorKeyString;

// Native kCFLocaleCalendarKey, 0x2227a8968 (its constant string at 0x2227b95a0). External; natively the same
// address as NSLocaleCalendar and NSLocaleCalendarKey, aliases this branch does not define.
static const OCFConstantString localeCalendarKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFLocale"
    "CalendarKey",
    20
};
const OCFStringRef kOCFLocaleCalendarKey = (OCFStringRef)&localeCalendarKeyString;

// Native kCFDateFormatterAMSymbolKey, 0x2227a89a8 (its constant string at 0x2227b9820). External; natively
// the same address as kCFDateFormatterAMSymbol, aliases this branch does not define.
static const OCFConstantString dateFormatterAMSymbolKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "AMSymbolKey",
    27
};
const OCFStringRef kOCFDateFormatterAMSymbolKey = (OCFStringRef)&dateFormatterAMSymbolKeyString;

// Native kCFDateFormatterCalendarKey, 0x2227a89b0 (its constant string at 0x2227b9840). External; natively
// the same address as kCFDateFormatterCalendar, aliases this branch does not define.
static const OCFConstantString dateFormatterCalendarKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "CalendarKey",
    27
};
const OCFStringRef kOCFDateFormatterCalendarKey = (OCFStringRef)&dateFormatterCalendarKeyString;

// Native kCFDateFormatterCalendarName, 0x2227a89b8 (its constant string at 0x2227b9860). External; natively
// the same address as kCFDateFormatterCalendarIdentifier and kCFDateFormatterCalendarIdentifierKey, aliases
// this branch does not define.
static const OCFConstantString dateFormatterCalendarNameString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "CalendarIdentifierKey",
    37
};
const OCFStringRef kOCFDateFormatterCalendarName = (OCFStringRef)&dateFormatterCalendarNameString;

// Native kCFDateFormatterDefaultDateKey, 0x2227a89c0 (its constant string at 0x2227b9880). External; natively
// the same address as kCFDateFormatterDefaultDate, aliases this branch does not define.
static const OCFConstantString dateFormatterDefaultDateKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "DefaultDateKey",
    30
};
const OCFStringRef kOCFDateFormatterDefaultDateKey = (OCFStringRef)&dateFormatterDefaultDateKeyString;

// Native kCFDateFormatterDefaultFormatKey, 0x2227a89c8 (its constant string at 0x2227b98a0). External;
// natively the same address as kCFDateFormatterDefaultFormat, aliases this branch does not define.
static const OCFConstantString dateFormatterDefaultFormatKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "DefaultFormatKey",
    32
};
const OCFStringRef kOCFDateFormatterDefaultFormatKey = (OCFStringRef)&dateFormatterDefaultFormatKeyString;

// Native kCFDateFormatterDoesRelativeDateFormattingKey, 0x2227a89d0 (its constant string at 0x2227b98c0).
// External.
static const OCFConstantString dateFormatterDoesRelativeDateFormattingKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "DoesRelativeDateFormattingKey",
    45
};
const OCFStringRef kOCFDateFormatterDoesRelativeDateFormattingKey =
    (OCFStringRef)&dateFormatterDoesRelativeDateFormattingKeyString;

// Native kCFDateFormatterEraSymbolsKey, 0x2227a89d8 (its constant string at 0x2227b98e0). External; natively
// the same address as kCFDateFormatterEraSymbols, aliases this branch does not define.
static const OCFConstantString dateFormatterEraSymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "EraSymbolsKey",
    29
};
const OCFStringRef kOCFDateFormatterEraSymbolsKey = (OCFStringRef)&dateFormatterEraSymbolsKeyString;

// Native kCFDateFormatterGregorianStartDateKey, 0x2227a89e0 (its constant string at 0x2227b9900). External;
// natively the same address as kCFDateFormatterGregorianStartDate, aliases this branch does not define.
static const OCFConstantString dateFormatterGregorianStartDateKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "GregorianStartDateKey",
    37
};
const OCFStringRef kOCFDateFormatterGregorianStartDateKey =
    (OCFStringRef)&dateFormatterGregorianStartDateKeyString;

// Native kCFDateFormatterIsLenientKey, 0x2227a89e8 (its constant string at 0x2227b9920). External; natively
// the same address as kCFDateFormatterIsLenient, aliases this branch does not define.
static const OCFConstantString dateFormatterIsLenientKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "IsLenientKey",
    28
};
const OCFStringRef kOCFDateFormatterIsLenientKey = (OCFStringRef)&dateFormatterIsLenientKeyString;

// Native kCFDateFormatterLongEraSymbolsKey, 0x2227a89f0 (its constant string at 0x2227b9940). External;
// natively the same address as kCFDateFormatterLongEraSymbols, aliases this branch does not define.
static const OCFConstantString dateFormatterLongEraSymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "LongEraSymbolsKey",
    33
};
const OCFStringRef kOCFDateFormatterLongEraSymbolsKey = (OCFStringRef)&dateFormatterLongEraSymbolsKeyString;

// Native kCFDateFormatterMonthSymbolsKey, 0x2227a89f8 (its constant string at 0x2227b9960). External;
// natively the same address as kCFDateFormatterMonthSymbols, aliases this branch does not define.
static const OCFConstantString dateFormatterMonthSymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "MonthSymbolsKey",
    31
};
const OCFStringRef kOCFDateFormatterMonthSymbolsKey = (OCFStringRef)&dateFormatterMonthSymbolsKeyString;

// Native kCFDateFormatterPMSymbolKey, 0x2227a8a00 (its constant string at 0x2227b9980). External; natively
// the same address as kCFDateFormatterPMSymbol, aliases this branch does not define.
static const OCFConstantString dateFormatterPMSymbolKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "PMSymbolKey",
    27
};
const OCFStringRef kOCFDateFormatterPMSymbolKey = (OCFStringRef)&dateFormatterPMSymbolKeyString;

// Native kCFDateFormatterAmbiguousYearStrategyKey, 0x2227a8a08 (its constant string at 0x2227b99a0).
// External.
static const OCFConstantString dateFormatterAmbiguousYearStrategyKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "AmbiguousYearStrategyKey",
    40
};
const OCFStringRef kOCFDateFormatterAmbiguousYearStrategyKey =
    (OCFStringRef)&dateFormatterAmbiguousYearStrategyKeyString;

// Native kCFDateFormatterQuarterSymbolsKey, 0x2227a8a10 (its constant string at 0x2227b99c0). External;
// natively the same address as kCFDateFormatterQuarterSymbols, aliases this branch does not define.
static const OCFConstantString dateFormatterQuarterSymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "QuarterSymbolsKey",
    33
};
const OCFStringRef kOCFDateFormatterQuarterSymbolsKey = (OCFStringRef)&dateFormatterQuarterSymbolsKeyString;

// Native kCFDateFormatterShortMonthSymbolsKey, 0x2227a8a18 (its constant string at 0x2227b99e0). External;
// natively the same address as kCFDateFormatterShortMonthSymbols, aliases this branch does not define.
static const OCFConstantString dateFormatterShortMonthSymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "ShortMonthSymbolsKey",
    36
};
const OCFStringRef kOCFDateFormatterShortMonthSymbolsKey =
    (OCFStringRef)&dateFormatterShortMonthSymbolsKeyString;

// Native kCFDateFormatterShortQuarterSymbolsKey, 0x2227a8a20 (its constant string at 0x2227b9a00). External;
// natively the same address as kCFDateFormatterShortQuarterSymbols, aliases this branch does not define.
static const OCFConstantString dateFormatterShortQuarterSymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "ShortQuarterSymbolsKey",
    38
};
const OCFStringRef kOCFDateFormatterShortQuarterSymbolsKey =
    (OCFStringRef)&dateFormatterShortQuarterSymbolsKeyString;

// Native kCFDateFormatterShortStandaloneMonthSymbolsKey, 0x2227a8a28 (its constant string at 0x2227b9a20).
// External; natively the same address as kCFDateFormatterShortStandaloneMonthSymbols, aliases this branch
// does not define.
static const OCFConstantString dateFormatterShortStandaloneMonthSymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "ShortStandaloneMonthSymbolsKey",
    46
};
const OCFStringRef kOCFDateFormatterShortStandaloneMonthSymbolsKey =
    (OCFStringRef)&dateFormatterShortStandaloneMonthSymbolsKeyString;

// Native kCFDateFormatterShortStandaloneQuarterSymbolsKey, 0x2227a8a30 (its constant string at 0x2227b9a40).
// External; natively the same address as kCFDateFormatterShortStandaloneQuarterSymbols, aliases this branch
// does not define.
static const OCFConstantString dateFormatterShortStandaloneQuarterSymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "ShortStandaloneQuarterSymbolsKey",
    48
};
const OCFStringRef kOCFDateFormatterShortStandaloneQuarterSymbolsKey =
    (OCFStringRef)&dateFormatterShortStandaloneQuarterSymbolsKeyString;

// Native kCFDateFormatterShortStandaloneWeekdaySymbolsKey, 0x2227a8a38 (its constant string at 0x2227b9a60).
// External; natively the same address as kCFDateFormatterShortStandaloneWeekdaySymbols, aliases this branch
// does not define.
static const OCFConstantString dateFormatterShortStandaloneWeekdaySymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "ShortStandaloneWeekdaySymbolsKey",
    48
};
const OCFStringRef kOCFDateFormatterShortStandaloneWeekdaySymbolsKey =
    (OCFStringRef)&dateFormatterShortStandaloneWeekdaySymbolsKeyString;

// Native kCFDateFormatterShortWeekdaySymbolsKey, 0x2227a8a40 (its constant string at 0x2227b9a80). External;
// natively the same address as kCFDateFormatterShortWeekdaySymbols, aliases this branch does not define.
static const OCFConstantString dateFormatterShortWeekdaySymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "ShortWeekdaySymbolsKey",
    38
};
const OCFStringRef kOCFDateFormatterShortWeekdaySymbolsKey =
    (OCFStringRef)&dateFormatterShortWeekdaySymbolsKeyString;

// Native kCFDateFormatterStandaloneMonthSymbolsKey, 0x2227a8a48 (its constant string at 0x2227b9aa0).
// External; natively the same address as kCFDateFormatterStandaloneMonthSymbols, aliases this branch does not
// define.
static const OCFConstantString dateFormatterStandaloneMonthSymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "StandaloneMonthSymbolsKey",
    41
};
const OCFStringRef kOCFDateFormatterStandaloneMonthSymbolsKey =
    (OCFStringRef)&dateFormatterStandaloneMonthSymbolsKeyString;

// Native kCFDateFormatterStandaloneQuarterSymbolsKey, 0x2227a8a50 (its constant string at 0x2227b9ac0).
// External; natively the same address as kCFDateFormatterStandaloneQuarterSymbols, aliases this branch does
// not define.
static const OCFConstantString dateFormatterStandaloneQuarterSymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "StandaloneQuarterSymbolsKey",
    43
};
const OCFStringRef kOCFDateFormatterStandaloneQuarterSymbolsKey =
    (OCFStringRef)&dateFormatterStandaloneQuarterSymbolsKeyString;

// Native kCFDateFormatterStandaloneWeekdaySymbolsKey, 0x2227a8a58 (its constant string at 0x2227b9ae0).
// External; natively the same address as kCFDateFormatterStandaloneWeekdaySymbols, aliases this branch does
// not define.
static const OCFConstantString dateFormatterStandaloneWeekdaySymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "StandaloneWeekdaySymbolsKey",
    43
};
const OCFStringRef kOCFDateFormatterStandaloneWeekdaySymbolsKey =
    (OCFStringRef)&dateFormatterStandaloneWeekdaySymbolsKeyString;

// Native kCFDateFormatterTimeZoneKey, 0x2227a8a60 (its constant string at 0x2227b9b00). External; natively
// the same address as kCFDateFormatterTimeZone, aliases this branch does not define.
static const OCFConstantString dateFormatterTimeZoneKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "TimeZoneKey",
    27
};
const OCFStringRef kOCFDateFormatterTimeZoneKey = (OCFStringRef)&dateFormatterTimeZoneKeyString;

// Native kCFDateFormatterTwoDigitStartDateKey, 0x2227a8a68 (its constant string at 0x2227b9b20). External;
// natively the same address as kCFDateFormatterTwoDigitStartDate, aliases this branch does not define.
static const OCFConstantString dateFormatterTwoDigitStartDateKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "TwoDigitStartDateKey",
    36
};
const OCFStringRef kOCFDateFormatterTwoDigitStartDateKey =
    (OCFStringRef)&dateFormatterTwoDigitStartDateKeyString;

// Native kCFDateFormatterVeryShortMonthSymbolsKey, 0x2227a8a70 (its constant string at 0x2227b9b40).
// External; natively the same address as kCFDateFormatterVeryShortMonthSymbols, aliases this branch does not
// define.
static const OCFConstantString dateFormatterVeryShortMonthSymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "VeryShortMonthSymbolsKey",
    40
};
const OCFStringRef kOCFDateFormatterVeryShortMonthSymbolsKey =
    (OCFStringRef)&dateFormatterVeryShortMonthSymbolsKeyString;

// Native kCFDateFormatterVeryShortStandaloneMonthSymbolsKey, 0x2227a8a78 (its constant string at
// 0x2227b9b60). External; natively the same address as kCFDateFormatterVeryShortStandaloneMonthSymbols,
// aliases this branch does not define.
static const OCFConstantString dateFormatterVeryShortStandaloneMonthSymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "VeryShortStandaloneMonthSymbolsKey",
    50
};
const OCFStringRef kOCFDateFormatterVeryShortStandaloneMonthSymbolsKey =
    (OCFStringRef)&dateFormatterVeryShortStandaloneMonthSymbolsKeyString;

// Native kCFDateFormatterVeryShortStandaloneWeekdaySymbolsKey, 0x2227a8a80 (its constant string at
// 0x2227b9b80). External; natively the same address as kCFDateFormatterVeryShortStandaloneWeekdaySymbols,
// aliases this branch does not define.
static const OCFConstantString dateFormatterVeryShortStandaloneWeekdaySymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "VeryShortStandaloneWeekdaySymbolsKey",
    52
};
const OCFStringRef kOCFDateFormatterVeryShortStandaloneWeekdaySymbolsKey =
    (OCFStringRef)&dateFormatterVeryShortStandaloneWeekdaySymbolsKeyString;

// Native kCFDateFormatterVeryShortWeekdaySymbolsKey, 0x2227a8a88 (its constant string at 0x2227b9ba0).
// External; natively the same address as kCFDateFormatterVeryShortWeekdaySymbols, aliases this branch does
// not define.
static const OCFConstantString dateFormatterVeryShortWeekdaySymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "VeryShortWeekdaySymbolsKey",
    42
};
const OCFStringRef kOCFDateFormatterVeryShortWeekdaySymbolsKey =
    (OCFStringRef)&dateFormatterVeryShortWeekdaySymbolsKeyString;

// Native kCFDateFormatterWeekdaySymbolsKey, 0x2227a8a90 (its constant string at 0x2227b9bc0). External;
// natively the same address as kCFDateFormatterWeekdaySymbols, aliases this branch does not define.
static const OCFConstantString dateFormatterWeekdaySymbolsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "WeekdaySymbolsKey",
    33
};
const OCFStringRef kOCFDateFormatterWeekdaySymbolsKey = (OCFStringRef)&dateFormatterWeekdaySymbolsKeyString;

// Native kCFDateFormatterUsesCharacterDirectionKey, 0x2227a8a98 (its constant string at 0x2227b9be0).
// External; natively the same address as kCFDateFormatterUsesCharacterDirection, aliases this branch does not
// define.
static const OCFConstantString dateFormatterUsesCharacterDirectionKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFDateFormatter"
    "UsesCharacterDirectionKey",
    41
};
const OCFStringRef kOCFDateFormatterUsesCharacterDirectionKey =
    (OCFStringRef)&dateFormatterUsesCharacterDirectionKeyString;

// Native kCFNumberFormatterAlwaysShowDecimalSeparatorKey, 0x2227a8ac0 (its constant string at 0x2227b9c80).
// External.
static const OCFConstantString numberFormatterAlwaysShowDecimalSeparatorKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "AlwaysShowDecimalSeparatorKey",
    47
};
const OCFStringRef kOCFNumberFormatterAlwaysShowDecimalSeparatorKey =
    (OCFStringRef)&numberFormatterAlwaysShowDecimalSeparatorKeyString;

// Native kCFNumberFormatterCurrencyCodeKey, 0x2227a8ac8 (its constant string at 0x2227b9ca0). External.
static const OCFConstantString numberFormatterCurrencyCodeKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "CurrencyCodeKey",
    33
};
const OCFStringRef kOCFNumberFormatterCurrencyCodeKey = (OCFStringRef)&numberFormatterCurrencyCodeKeyString;

// Native kCFNumberFormatterCurrencyDecimalSeparatorKey, 0x2227a8ad0 (its constant string at 0x2227b9cc0).
// External.
static const OCFConstantString numberFormatterCurrencyDecimalSeparatorKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "CurrencyDecimalSeparatorKey",
    45
};
const OCFStringRef kOCFNumberFormatterCurrencyDecimalSeparatorKey =
    (OCFStringRef)&numberFormatterCurrencyDecimalSeparatorKeyString;

// Native kCFNumberFormatterCurrencyGroupingSeparatorKey, 0x2227a8ad8 (its constant string at 0x2227b9ce0).
// External.
static const OCFConstantString numberFormatterCurrencyGroupingSeparatorKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "CurrencyGroupingSeparatorKey",
    46
};
const OCFStringRef kOCFNumberFormatterCurrencyGroupingSeparatorKey =
    (OCFStringRef)&numberFormatterCurrencyGroupingSeparatorKeyString;

// Native kCFNumberFormatterCurrencySymbolKey, 0x2227a8ae0 (its constant string at 0x2227b9d00). External.
static const OCFConstantString numberFormatterCurrencySymbolKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "CurrencySymbolKey",
    35
};
const OCFStringRef kOCFNumberFormatterCurrencySymbolKey =
    (OCFStringRef)&numberFormatterCurrencySymbolKeyString;

// Native kCFNumberFormatterDecimalSeparatorKey, 0x2227a8ae8 (its constant string at 0x2227b9d20). External.
static const OCFConstantString numberFormatterDecimalSeparatorKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "DecimalSeparatorKey",
    37
};
const OCFStringRef kOCFNumberFormatterDecimalSeparatorKey =
    (OCFStringRef)&numberFormatterDecimalSeparatorKeyString;

// Native kCFNumberFormatterDefaultFormatKey, 0x2227a8af0 (its constant string at 0x2227b9d40). External.
static const OCFConstantString numberFormatterDefaultFormatKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "DefaultFormatKey",
    34
};
const OCFStringRef kOCFNumberFormatterDefaultFormatKey = (OCFStringRef)&numberFormatterDefaultFormatKeyString;

// Native kCFNumberFormatterExponentSymbolKey, 0x2227a8af8 (its constant string at 0x2227b9d60). External.
static const OCFConstantString numberFormatterExponentSymbolKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "ExponentSymbolKey",
    35
};
const OCFStringRef kOCFNumberFormatterExponentSymbolKey =
    (OCFStringRef)&numberFormatterExponentSymbolKeyString;

// Native kCFNumberFormatterFormatWidthKey, 0x2227a8b00 (its constant string at 0x2227b9d80). External.
static const OCFConstantString numberFormatterFormatWidthKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "FormatWidthKey",
    32
};
const OCFStringRef kOCFNumberFormatterFormatWidthKey = (OCFStringRef)&numberFormatterFormatWidthKeyString;

// Native kCFNumberFormatterGroupingSeparatorKey, 0x2227a8b08 (its constant string at 0x2227b9da0). External.
static const OCFConstantString numberFormatterGroupingSeparatorKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "GroupingSeparatorKey",
    38
};
const OCFStringRef kOCFNumberFormatterGroupingSeparatorKey =
    (OCFStringRef)&numberFormatterGroupingSeparatorKeyString;

// Native kCFNumberFormatterGroupingSizeKey, 0x2227a8b10 (its constant string at 0x2227b9dc0). External.
static const OCFConstantString numberFormatterGroupingSizeKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "GroupingSizeKey",
    33
};
const OCFStringRef kOCFNumberFormatterGroupingSizeKey = (OCFStringRef)&numberFormatterGroupingSizeKeyString;

// Native kCFNumberFormatterInfinitySymbolKey, 0x2227a8b18 (its constant string at 0x2227b9de0). External.
static const OCFConstantString numberFormatterInfinitySymbolKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "InfinitySymbolKey",
    35
};
const OCFStringRef kOCFNumberFormatterInfinitySymbolKey =
    (OCFStringRef)&numberFormatterInfinitySymbolKeyString;

// Native kCFNumberFormatterInternationalCurrencySymbolKey, 0x2227a8b20 (its constant string at 0x2227b9e00).
// External.
static const OCFConstantString numberFormatterInternationalCurrencySymbolKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "InternationalCurrencySymbolKey",
    48
};
const OCFStringRef kOCFNumberFormatterInternationalCurrencySymbolKey =
    (OCFStringRef)&numberFormatterInternationalCurrencySymbolKeyString;

// Native kCFNumberFormatterIsLenientKey, 0x2227a8b28 (its constant string at 0x2227b9e20). External.
static const OCFConstantString numberFormatterIsLenientKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "IsLenientKey",
    30
};
const OCFStringRef kOCFNumberFormatterIsLenientKey = (OCFStringRef)&numberFormatterIsLenientKeyString;

// Native kCFNumberFormatterMaxFractionDigitsKey, 0x2227a8b30 (its constant string at 0x2227b9e40). External.
static const OCFConstantString numberFormatterMaxFractionDigitsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "MaxFractionDigitsKey",
    38
};
const OCFStringRef kOCFNumberFormatterMaxFractionDigitsKey =
    (OCFStringRef)&numberFormatterMaxFractionDigitsKeyString;

// Native kCFNumberFormatterMaxIntegerDigitsKey, 0x2227a8b38 (its constant string at 0x2227b9e60). External.
static const OCFConstantString numberFormatterMaxIntegerDigitsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "MaxIntegerDigitsKey",
    37
};
const OCFStringRef kOCFNumberFormatterMaxIntegerDigitsKey =
    (OCFStringRef)&numberFormatterMaxIntegerDigitsKeyString;

// Native kCFNumberFormatterMaxSignificantDigitsKey, 0x2227a8b40 (its constant string at 0x2227b9e80).
// External.
static const OCFConstantString numberFormatterMaxSignificantDigitsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "MaxSignificantDigitsKey",
    41
};
const OCFStringRef kOCFNumberFormatterMaxSignificantDigitsKey =
    (OCFStringRef)&numberFormatterMaxSignificantDigitsKeyString;

// Native kCFNumberFormatterMinFractionDigitsKey, 0x2227a8b48 (its constant string at 0x2227b9ea0). External.
static const OCFConstantString numberFormatterMinFractionDigitsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "MinFractionDigitsKey",
    38
};
const OCFStringRef kOCFNumberFormatterMinFractionDigitsKey =
    (OCFStringRef)&numberFormatterMinFractionDigitsKeyString;

// Native kCFNumberFormatterMinIntegerDigitsKey, 0x2227a8b50 (its constant string at 0x2227b9ec0). External.
static const OCFConstantString numberFormatterMinIntegerDigitsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "MinIntegerDigitsKey",
    37
};
const OCFStringRef kOCFNumberFormatterMinIntegerDigitsKey =
    (OCFStringRef)&numberFormatterMinIntegerDigitsKeyString;

// Native kCFNumberFormatterMinSignificantDigitsKey, 0x2227a8b58 (its constant string at 0x2227b9ee0).
// External.
static const OCFConstantString numberFormatterMinSignificantDigitsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "MinSignificantDigitsKey",
    41
};
const OCFStringRef kOCFNumberFormatterMinSignificantDigitsKey =
    (OCFStringRef)&numberFormatterMinSignificantDigitsKeyString;

// Native kCFNumberFormatterMinusSignKey, 0x2227a8b60 (its constant string at 0x2227b9f00). External.
static const OCFConstantString numberFormatterMinusSignKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "MinusSignKey",
    30
};
const OCFStringRef kOCFNumberFormatterMinusSignKey = (OCFStringRef)&numberFormatterMinusSignKeyString;

// Native kCFNumberFormatterMultiplierKey, 0x2227a8b68 (its constant string at 0x2227b9f20). External.
static const OCFConstantString numberFormatterMultiplierKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "MultiplierKey",
    31
};
const OCFStringRef kOCFNumberFormatterMultiplierKey = (OCFStringRef)&numberFormatterMultiplierKeyString;

// Native kCFNumberFormatterNaNSymbolKey, 0x2227a8b70 (its constant string at 0x2227b9f40). External.
static const OCFConstantString numberFormatterNaNSymbolKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "NaNSymbolKey",
    30
};
const OCFStringRef kOCFNumberFormatterNaNSymbolKey = (OCFStringRef)&numberFormatterNaNSymbolKeyString;

// Native kCFNumberFormatterNegativePrefixKey, 0x2227a8b78 (its constant string at 0x2227b9f60). External.
static const OCFConstantString numberFormatterNegativePrefixKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "NegativePrefixKey",
    35
};
const OCFStringRef kOCFNumberFormatterNegativePrefixKey =
    (OCFStringRef)&numberFormatterNegativePrefixKeyString;

// Native kCFNumberFormatterNegativeSuffixKey, 0x2227a8b80 (its constant string at 0x2227b9f80). External.
static const OCFConstantString numberFormatterNegativeSuffixKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "NegativeSuffixKey",
    35
};
const OCFStringRef kOCFNumberFormatterNegativeSuffixKey =
    (OCFStringRef)&numberFormatterNegativeSuffixKeyString;

// Native kCFNumberFormatterPaddingCharacterKey, 0x2227a8b88 (its constant string at 0x2227b9fa0). External.
static const OCFConstantString numberFormatterPaddingCharacterKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "PaddingCharacterKey",
    37
};
const OCFStringRef kOCFNumberFormatterPaddingCharacterKey =
    (OCFStringRef)&numberFormatterPaddingCharacterKeyString;

// Native kCFNumberFormatterPaddingPositionKey, 0x2227a8b90 (its constant string at 0x2227b9fc0). External.
static const OCFConstantString numberFormatterPaddingPositionKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "PaddingPositionKey",
    36
};
const OCFStringRef kOCFNumberFormatterPaddingPositionKey =
    (OCFStringRef)&numberFormatterPaddingPositionKeyString;

// Native kCFNumberFormatterPerMillSymbolKey, 0x2227a8b98 (its constant string at 0x2227b9fe0). External.
static const OCFConstantString numberFormatterPerMillSymbolKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "PerMillSymbolKey",
    34
};
const OCFStringRef kOCFNumberFormatterPerMillSymbolKey = (OCFStringRef)&numberFormatterPerMillSymbolKeyString;

// Native kCFNumberFormatterPercentSymbolKey, 0x2227a8ba0 (its constant string at 0x2227ba000). External.
static const OCFConstantString numberFormatterPercentSymbolKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "PercentSymbolKey",
    34
};
const OCFStringRef kOCFNumberFormatterPercentSymbolKey = (OCFStringRef)&numberFormatterPercentSymbolKeyString;

// Native kCFNumberFormatterPlusSignKey, 0x2227a8ba8 (its constant string at 0x2227ba020). External.
static const OCFConstantString numberFormatterPlusSignKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "PlusSignKey",
    29
};
const OCFStringRef kOCFNumberFormatterPlusSignKey = (OCFStringRef)&numberFormatterPlusSignKeyString;

// Native kCFNumberFormatterPositivePrefixKey, 0x2227a8bb0 (its constant string at 0x2227ba040). External.
static const OCFConstantString numberFormatterPositivePrefixKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "PositivePrefixKey",
    35
};
const OCFStringRef kOCFNumberFormatterPositivePrefixKey =
    (OCFStringRef)&numberFormatterPositivePrefixKeyString;

// Native kCFNumberFormatterPositiveSuffixKey, 0x2227a8bb8 (its constant string at 0x2227ba060). External.
static const OCFConstantString numberFormatterPositiveSuffixKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "PositiveSuffixKey",
    35
};
const OCFStringRef kOCFNumberFormatterPositiveSuffixKey =
    (OCFStringRef)&numberFormatterPositiveSuffixKeyString;

// Native kCFNumberFormatterRoundingIncrementKey, 0x2227a8bc0 (its constant string at 0x2227ba080). External.
static const OCFConstantString numberFormatterRoundingIncrementKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "RoundingIncrementKey",
    38
};
const OCFStringRef kOCFNumberFormatterRoundingIncrementKey =
    (OCFStringRef)&numberFormatterRoundingIncrementKeyString;

// Native kCFNumberFormatterRoundingModeKey, 0x2227a8bc8 (its constant string at 0x2227ba0a0). External.
static const OCFConstantString numberFormatterRoundingModeKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "RoundingModeKey",
    33
};
const OCFStringRef kOCFNumberFormatterRoundingModeKey = (OCFStringRef)&numberFormatterRoundingModeKeyString;

// Native kCFNumberFormatterSecondaryGroupingSizeKey, 0x2227a8bd0 (its constant string at 0x2227ba0c0).
// External.
static const OCFConstantString numberFormatterSecondaryGroupingSizeKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "SecondaryGroupingSizeKey",
    42
};
const OCFStringRef kOCFNumberFormatterSecondaryGroupingSizeKey =
    (OCFStringRef)&numberFormatterSecondaryGroupingSizeKeyString;

// Native kCFNumberFormatterUseGroupingSeparatorKey, 0x2227a8bd8 (its constant string at 0x2227ba0e0).
// External.
static const OCFConstantString numberFormatterUseGroupingSeparatorKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "UseGroupingSeparatorKey",
    41
};
const OCFStringRef kOCFNumberFormatterUseGroupingSeparatorKey =
    (OCFStringRef)&numberFormatterUseGroupingSeparatorKeyString;

// Native kCFNumberFormatterUseSignificantDigitsKey, 0x2227a8be0 (its constant string at 0x2227ba100).
// External.
static const OCFConstantString numberFormatterUseSignificantDigitsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "UseSignificantDigitsKey",
    41
};
const OCFStringRef kOCFNumberFormatterUseSignificantDigitsKey =
    (OCFStringRef)&numberFormatterUseSignificantDigitsKeyString;

// Native kCFNumberFormatterUsesCharacterDirectionKey, 0x2227a8be8 (its constant string at 0x2227ba120).
// External.
static const OCFConstantString numberFormatterUsesCharacterDirectionKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "UsesCharacterDirectionKey",
    43
};
const OCFStringRef kOCFNumberFormatterUsesCharacterDirectionKey =
    (OCFStringRef)&numberFormatterUsesCharacterDirectionKeyString;

// Native kCFNumberFormatterZeroSymbolKey, 0x2227a8bf0 (its constant string at 0x2227ba140). External.
static const OCFConstantString numberFormatterZeroSymbolKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "ZeroSymbolKey",
    31
};
const OCFStringRef kOCFNumberFormatterZeroSymbolKey = (OCFStringRef)&numberFormatterZeroSymbolKeyString;

// Native kCFNumberFormatterMinGroupingDigitsKey, 0x2227a8bf8 (its constant string at 0x2227ba160). External.
static const OCFConstantString numberFormatterMinGroupingDigitsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFNumberFormatter"
    "MinGroupingDigitsKey",
    38
};
const OCFStringRef kOCFNumberFormatterMinGroupingDigitsKey =
    (OCFStringRef)&numberFormatterMinGroupingDigitsKeyString;

// Native kCFGregorianCalendar, 0x2227a8c00 (its constant string "gregorian" at 0x2227ba180), and
// kCFChineseCalendar, 0x2227a8c40 (its constant string "chinese" at 0x2227ba280), __DATA_CONST,__const.
// External. They lie in the run of NSCalendar identifiers that follows this table (0x2227a8c00-0x2227a8cd0;
// natively the same addresses as NSCalendarIdentifierGregorian and NSGregorianCalendar, and
// NSCalendarIdentifierChinese and NSChineseCalendar, aliases this branch does not define); the run is kept
// with the locale keys (confidence: medium for that placement). kCFChineseCalendar is defined because
// -[NSCalendar compareDate:toDate:toUnitGranularity:] loads it.
static const OCFConstantString gregorianCalendarString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "gregorian", 9
};
const OCFCalendarIdentifier kOCFGregorianCalendar = (OCFCalendarIdentifier)&gregorianCalendarString;

static const OCFConstantString chineseCalendarString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "chinese", 7
};
const OCFCalendarIdentifier kOCFChineseCalendar = (OCFCalendarIdentifier)&chineseCalendarString;
