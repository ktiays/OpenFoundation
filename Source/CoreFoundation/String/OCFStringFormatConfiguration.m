//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/String/OCFStringFormatConfiguration.h"
#import "CoreFoundation/PlugIn/OCFBundleLocalization.h"
#import "CoreFoundation/Number/OCFNumber.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#import "CoreFoundation/Runtime/OCFRuntime.h"
#import "CoreFoundation/String/OCFStringFormat.h"
#import "CoreFoundation/String/OCFStringLocalizedFormatting.h"
#import "CoreFoundation/Unicode/OCFUniChar.h"
#import "CoreFoundation/PlugIn/OCFBundleMain.h"

#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: __CFStringGetFormatSpecifierConfiguration (0x1c02caef8) through
// _____NSGetCFLocalizedAttributedStringClass_block_invoke (0x1c02cbf84) are contiguous in the iOS Simulator
// __text section after the NSOrderedSet coder methods, with the kCFStringFormat* keys and the block literals
// (0x2227aa1a0-0x2227aa2a8) contiguous in __DATA_CONST,__const; the restored functions of that span are kept
// in one translation unit. The unit's __CFStringCopyWithFomatStringConfiguration (0x1c02caf38),
// __CFStringCreateWithWidthContexts (0x1c02caf88) and __CFStringCreateWithMarkdownAndConfiguration
// (0x1c02cbea0) are outside this family and not restored. The unit is Objective-C (message sends,
// objc_opt_isKindOfClass, objc_exception_throw from the new_id_array pattern). The native source path is not
// in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationStringFormat.md.

extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

// libobjc entry points the native code calls directly (re-exported libobjc.A.dylib); supplied, never
// reimplemented. The SDK's public headers do not declare them.
extern BOOL objc_opt_respondsToSelector(id object, SEL selector);
extern BOOL objc_opt_isKindOfClass(id object, Class otherClass);
extern id objc_alloc_init(Class cls);

// ICU (libicucore, linked by the native image); supplied, never reimplemented. The SDK installs no
// upluralrules.h, so the three entry points are declared here.
extern void *uplrules_open(const char *locale, int32_t *status);
extern int32_t
uplrules_select(const void *rules, double number, UniChar *keyword, int32_t capacity, int32_t *status);
extern void uplrules_close(void *rules);

// Native kCFStringFormatSpecTypeKey 0x2227aa1a0, kCFStringPluralRuleType 0x2227aa1b0,
// kCFStringFormatFamilyInfoKey 0x2227aa1c8, kCFStringFormatDefaultValueKey 0x2227aa1d8 ("default"),
// kCFStringFormatRuleInfoKey 0x2227aa210 and kCFStringFormatValueTypeKey 0x2227aa278 (__DATA_CONST,__const).
// External.
static const OCFConstantString formatSpecTypeKey = ConstantASCIIString("NSStringFormatSpecTypeKey");
static const OCFConstantString pluralRuleType = ConstantASCIIString("NSStringPluralRuleType");
static const OCFConstantString formatFamilyInfoKey = ConstantASCIIString("NSStringFormatFamilyInfoKey");
static const OCFConstantString formatDefaultValueKey = ConstantASCIIString("default");
static const OCFConstantString formatRuleInfoKey = ConstantASCIIString("NSStringFormatRuleInfoKey");
static const OCFConstantString formatValueTypeKey = ConstantASCIIString("NSStringFormatValueTypeKey");
const OCFStringRef kOCFStringFormatSpecTypeKey = (OCFStringRef)&formatSpecTypeKey;
const OCFStringRef kOCFStringPluralRuleType = (OCFStringRef)&pluralRuleType;
const OCFStringRef kOCFStringFormatFamilyInfoKey = (OCFStringRef)&formatFamilyInfoKey;
const OCFStringRef kOCFStringFormatDefaultValueKey = (OCFStringRef)&formatDefaultValueKey;
const OCFStringRef kOCFStringFormatRuleInfoKey = (OCFStringRef)&formatRuleInfoKey;
const OCFStringRef kOCFStringFormatValueTypeKey = (OCFStringRef)&formatValueTypeKey;

// Native __CFStringGetFormatSpecifierConfiguration, 0x1c02caef8. External.
// Any object that responds to -formatConfiguration (objc_opt_respondsToSelector) answers that message's
// result; others answer NULL.
OCFDictionaryRef _OCFStringGetFormatSpecifierConfiguration(OCFStringRef format) {
    SEL selector = sel_getUid("formatConfiguration");
    if (!objc_opt_respondsToSelector((id)format, selector)) {
        return NULL;
    }
    return ((OCFDictionaryRef (*)(OCFStringRef, SEL))objc_msgSend)(format, selector);
}

// Native __CFStringFindAffixRuleFormat, 0x1c02cafd8. Hidden (was a private external).
// Classifies the last character of a string of at least two characters for an affix rule: Finnish ("fi")
// answers vowel, consonant or other; Russian ("ru") answers other outside А..я (U+0410-U+044F) and for Ь/ь,
// neuter for о/е, feminine for а/я/у/ю and masculine otherwise; Korean ("ko") decomposes the character (up to
// ten code points) and classifies the last one as a vowel (U+1161-U+11A7) or a consonant (U+1100-U+115E,
// U+11A8-U+11FF), else other. Other languages and short strings answer NULL. The character sets are created
// with the NULL allocator and released before returning.
OCFStringRef _CFStringFindAffixRuleFormat(OCFStringRef languageCode, OCFStringRef string) {
    if (OCFStringGetLength(string) < 2) {
        return NULL;
    }
    UniChar character = OCFStringGetCharacterAtIndex(string, OCFStringGetLength(string) - 1);
    OCFCharacterSetRef first;
    OCFCharacterSetRef second;
    OCFStringRef result;
    if (OCFEqual(languageCode, OF_STR("fi"))) {
        first = OCFCharacterSetCreateWithCharactersInString(NULL, OF_USTR(u"aeiouyåäö"));
        second = OCFCharacterSetCreateWithCharactersInString(NULL, OF_USTR(u"bcdfghjklmnpqrstvwxzšž"));
        if (OCFCharacterSetIsCharacterMember(first, character)) {
            result = OF_STR("vowel");
        } else {
            result =
                OCFCharacterSetIsCharacterMember(second, character) ? OF_STR("consonant") : OF_STR("other");
        }
    } else if (OCFEqual(languageCode, OF_STR("ru"))) {
        first = OCFCharacterSetCreateWithCharactersInString(NULL, OF_USTR(u"ое"));
        second = OCFCharacterSetCreateWithCharactersInString(NULL, OF_USTR(u"аяую"));
        if (((uint32_t)(character - 0x450) >> 6 & 0x3ff) < 0x3ff) {
            result = OF_STR("other");
        } else if (character == 0x42c || character == 0x44c) {
            result = OF_STR("other");
        } else if (OCFCharacterSetIsCharacterMember(first, character)) {
            result = OF_STR("neuter");
        } else {
            result = OCFCharacterSetIsCharacterMember(second, character) ? OF_STR("feminine")
                                                                         : OF_STR("masculine");
        }
    } else if (OCFEqual(languageCode, OF_STR("ko"))) {
        uint32_t decomposed[10];
        OCFIndex count = OCFUniCharDecomposeCharacter(character, decomposed, 10);
        OCFStringRef kind = NULL;
        if (count >= 1) {
            uint16_t last = (uint16_t)decomposed[count - 1];
            if ((uint32_t)(last - 0x11a8) < 0x58 || (uint32_t)(last - 0x1100) < 0x5f) {
                kind = OF_STR("consonant");
            }
            if ((uint32_t)(last - 0x1161) < 0x47) {
                kind = OF_STR("vowel");
            }
        }
        return kind != NULL ? kind : OF_STR("other");
    } else {
        return NULL;
    }
    OCFRelease(first);
    OCFRelease(second);
    return result;
}

// Native __CFCopyResolvedFormatStringWithConfiguration.mainBundleLocale, .onceToken, .onceToken.44 and
// .formatter, and ___NSPersonNameComponentsClass and ___NSPersonNameComponentsFormatterClass (__DATA,__bss).
// Non-external.
static _Atomic(OCFLocaleRef) mainBundleLocale;
static dispatch_once_t personNameClassesOnceToken;
static dispatch_once_t formatterOnceToken;
static id formatter;
static Class __NSPersonNameComponentsClass;
static Class __NSPersonNameComponentsFormatterClass;

// Native __CFCopyResolvedFormatStringWithConfiguration, 0x1c02cb238, with .cold.1 (0x1c031229c) and .cold.2
// (0x1c03122b0), the dispatch_once calls of ____CFCopyResolvedFormatStringWithConfiguration_block_invoke_2
// (0x1c02cbe44) and _block_invoke_3 (0x1c02cbe7c), and the stack block _block_invoke (0x1c02cbd0c). Hidden
// (was a private external). Resolves one configured argument against a stringsdict-style configuration and
// answers the retained format string, or NULL. The locale is the configuration's NSStringFormatLocaleKey,
// else the argument locale, else a process-wide locale for the main bundle's first preferred localization
// ("en" when it has none) published with a compare-and-swap (the loser releases its copy). The rule type
// (NSStringFormatSpecTypeKey, compared by pointer and then CFEqual) selects:
// - plural: a CFNumber argument; exactly 0 (not floating) selects "zero" and INT64_MAX (or a failed CFIndex
//   conversion reads as -1) "other" before ICU's plural rules for the locale identifier (UTF-8, 157 bytes,
//   "en" on failure) choose a keyword; 1 (not floating) falls back to "one", and "other" is the last resort.
// - gender: a non-negative int CFNumber, printed with %d, selects its key or "0".
// - entity formatting: the locale summary of NSStringFormatLocaleInfoKey selects start/end rules (by the
// first
//   and last characters' formatting family), conditional rules or, failing those for a string argument, the
//   family rule whose character set contains every character of the argument.
// - affix: the affix class of a string argument, else "other".
// - vocative name: the vocative of a string, or of an NSPersonNameComponents' given name formatted again with
// a
//   shared NSPersonNameComponentsFormatter.
OCFTypeRef _CFCopyResolvedFormatStringWithConfiguration(
    OCFTypeRef argument, OCFDictionaryRef configuration, OCFTypeRef locale
) {
    OCFStringRef ruleType = OCFDictionaryGetValue(configuration, OF_STR("NSStringFormatSpecTypeKey"));
    if (ruleType == NULL) {
        return NULL;
    }
    OCFTypeRef resolvedLocale = OCFDictionaryGetValue(configuration, OF_STR("NSStringFormatLocaleKey"));
    if (resolvedLocale == NULL) {
        resolvedLocale = locale;
        if (locale == NULL) {
            resolvedLocale = atomic_load_explicit(&mainBundleLocale, memory_order_relaxed);
            if (resolvedLocale == NULL) {
                OCFLocaleRef created = NULL;
                OCFArrayRef localizations = OCFBundleCopyBundleLocalizations(OCFBundleGetMainBundle());
                if (localizations != NULL) {
                    OCFArrayRef preferred = OCFBundleCopyPreferredLocalizationsFromArray(localizations);
                    OCFRelease(localizations);
                    if (preferred != NULL) {
                        if (OCFArrayGetCount(preferred) > 0) {
                            created = OCFLocaleCreate(NULL, OCFArrayGetValueAtIndex(preferred, 0));
                            OCFRelease(preferred);
                        } else {
                            OCFRelease(preferred);
                        }
                    }
                }
                if (created == NULL) {
                    created = OCFLocaleCreate(NULL, OF_STR("en"));
                }
                OCFLocaleRef expected = NULL;
                if (atomic_compare_exchange_strong(&mainBundleLocale, &expected, created)) {
                    resolvedLocale = created;
                } else {
                    OCFRelease(created);
                    resolvedLocale = atomic_load_explicit(&mainBundleLocale, memory_order_relaxed);
                }
            }
        }
    }
    if (ruleType == OF_STR("NSStringPluralRuleType") ||
        OCFEqual(ruleType, OF_STR("NSStringPluralRuleType"))) {
        if (OCFGetTypeID(argument) != OCFNumberGetTypeID()) {
            return NULL;
        }
        OCFIndex integer = 0;
        const void *selected;
        if (OCFNumberGetValue(argument, 14, &integer)) {
            selected = NULL;
            if (integer == 0 && !OCFNumberIsFloatType(argument)) {
                selected = OCFDictionaryGetValue(configuration, OF_STR("zero"));
            }
            if (integer == LONG_MAX) {
                selected = OCFDictionaryGetValue(configuration, OF_STR("other"));
            }
            if (selected != NULL) {
                return OCFRetain(selected);
            }
        } else {
            integer = -1;
        }
        int32_t status = 0;
        OCFStringRef identifier = OCFLocaleGetIdentifier(resolvedLocale);
        if (identifier == NULL) {
            identifier = OF_STR("en");
        }
        char localeName[157];
        if (!OCFStringGetCString(identifier, localeName, 157, 0x8000100)) {
            strcpy(localeName, "en");
        }
        UniChar keyword[160];
        void *rules = uplrules_open(localeName, &status);
        if (status > 0) {
            selected = NULL;
        } else {
            status = 0;
            double number = 0.0;
            int32_t keywordLength;
            if (OCFNumberGetValue(argument, 13, &number)) {
                keywordLength = uplrules_select(rules, number, keyword, 157, &status);
            } else {
                keywordLength = 0;
            }
            uplrules_close(rules);
            selected = NULL;
            if (status <= 0 && keywordLength >= 1) {
                OCFStringRef key = OCFStringCreateWithCharactersNoCopy(
                    NULL, keyword, (uint32_t)keywordLength, kOCFAllocatorNull
                );
                selected = OCFDictionaryGetValue(configuration, key);
                OCFRelease(key);
            }
        }
        if (selected == NULL && integer == 1 && !OCFNumberIsFloatType(argument)) {
            selected = OCFDictionaryGetValue(configuration, OF_STR("one"));
        }
        if (selected == NULL) {
            selected = OCFDictionaryGetValue(configuration, OF_STR("other"));
            if (selected == NULL) {
                return NULL;
            }
        }
        return OCFRetain(selected);
    }
    if (ruleType == OF_STR("NSStringGenderRuleType") ||
        OCFEqual(ruleType, OF_STR("NSStringGenderRuleType"))) {
        const void *value = NULL;
        if (OCFGetTypeID(argument) != OCFNumberGetTypeID()) {
            return NULL;
        }
        int32_t gender = 0;
        if (!OCFNumberGetValue(argument, 9, &gender)) {
            return NULL;
        }
        char digits[20] = {0};
        if (gender < 0) {
            return NULL;
        }
        if (snprintf(digits, 20, "%d", gender) < 1) {
            return NULL;
        }
        if (digits[0] == 0) {
            return NULL;
        }
        OCFStringRef key = OCFStringCreateWithCStringNoCopy(NULL, digits, 0x600, kOCFAllocatorNull);
        if (key == NULL) {
            return NULL;
        }
        if (!OCFDictionaryGetValueIfPresent(configuration, key, &value)) {
            value = OCFDictionaryGetValue(configuration, OF_STR("0"));
        }
        OCFRelease(key);
        if (value == NULL) {
            return NULL;
        }
        return OCFRetain(value);
    }
    if (ruleType == OF_STR("NSStringEntityFormattingRuleType") ||
        OCFEqual(ruleType, OF_STR("NSStringEntityFormattingRuleType"))) {
        OCFDictionaryRef familyInfo =
            OCFDictionaryGetValue(configuration, OF_STR("NSStringFormatFamilyInfoKey"));
        OCFDictionaryRef localeSummary =
            OCFDictionaryGetValue(configuration, OF_STR("NSStringFormatLocaleInfoKey"));
        OCFDictionaryRef localeInfo =
            OCFRetain(_CFStringGetRelevantLocaleInfoFromLocaleSummary(localeSummary, resolvedLocale));
        const void *rules = NULL;
        OCFTypeRef format = NULL;
        if (OCFDictionaryGetValueIfPresent(localeInfo, OF_STR("NSStringFormatRuleStartEndInfoKey"), &rules)) {
            if (OCFGetTypeID(argument) == OCFStringGetTypeID()) {
                const void * (^ruleForCharacter)(OCFDictionaryRef, UniChar) =
                    ^const void *(OCFDictionaryRef families, UniChar character) {
                    OCFIndex count = OCFDictionaryGetCount(families);
                    const void *keys[count];
                    size_t clearedSize = (size_t)count * 8 < 0x200 ? (size_t)count * 8 : 0x200;
                    memset(keys, 0, clearedSize);
                    const void *values[count];
                    memset(values, 0, clearedSize);
                    OCFDictionaryGetKeysAndValues(families, keys, values);
                    for (OCFIndex index = 0; index < count; ++index) {
                        if (OCFStringCompare(keys[index], OF_STR("default"), 0) != kOCFCompareEqualTo &&
                            OCFCharacterSetIsCharacterMember(
                                OCFDictionaryGetValue(familyInfo, keys[index]), character
                            )) {
                            return values[index];
                        }
                    }
                    return OCFDictionaryGetValue(families, OF_STR("default"));
                };
                if (OCFStringGetLength(argument) >= 1) {
                    const void *start = ruleForCharacter(rules, OCFStringGetCharacterAtIndex(argument, 0));
                    if (start != NULL && OCFGetTypeID(start) == OCFDictionaryGetTypeID()) {
                        const void *end = ruleForCharacter(
                            start, OCFStringGetCharacterAtIndex(argument, OCFStringGetLength(argument) - 1)
                        );
                        if (end != NULL) {
                            format = OCFGetTypeID(end) == OCFStringGetTypeID() ? end : NULL;
                        }
                    }
                } else {
                    OCFTypeRef language = OCFLocaleGetValue(resolvedLocale, kOCFLocaleLanguageCodeKey);
                    const void *start = OCFDictionaryGetValue(rules, language);
                    if (start != NULL && OCFGetTypeID(start) == OCFDictionaryGetTypeID()) {
                        const void *end = OCFDictionaryGetValue(start, language);
                        if (end != NULL) {
                            format = OCFGetTypeID(end) == OCFStringGetTypeID() ? end : NULL;
                        }
                    }
                }
            } else {
                const void *start = OCFDictionaryGetValue(rules, OF_STR("default"));
                if (start != NULL && OCFGetTypeID(start) == OCFDictionaryGetTypeID()) {
                    const void *end = OCFDictionaryGetValue(start, OF_STR("default"));
                    if (end != NULL) {
                        format = OCFGetTypeID(end) == OCFStringGetTypeID() ? end : NULL;
                    }
                }
            }
        } else if (
            OCFDictionaryGetValueIfPresent(localeInfo, OF_STR("NSStringConditionalFormatRuleInfoKey"), &rules)
        ) {
            OCFDictionaryRef conditional = OCFDictionaryGetValue(rules, OF_STR("default"));
            if (OCFGetTypeID(argument) == OCFStringGetTypeID() &&
                _CFStringConditionalFormattingMeetsCondition(argument, resolvedLocale)) {
                format = OCFDictionaryGetValue(conditional, OF_STR("NSStringConditionalFormatKey"));
            } else {
                format = OCFDictionaryGetValue(conditional, OF_STR("default"));
            }
        }
        if (OCFGetTypeID(argument) == OCFStringGetTypeID() && format == NULL) {
            OCFDictionaryRef classInfo =
                OCFDictionaryGetValue(configuration, OF_STR("NSStringFormatClassInfoKey"));
            const void *classType = NULL;
            OCFDictionaryRef familyRules;
            if (OCFDictionaryGetValueIfPresent(
                    localeInfo, OF_STR("NSStringFormatClassTypeKey"), &classType
                )) {
                familyRules = OCFDictionaryGetValue(classInfo, classType);
            } else {
                familyRules = OCFDictionaryGetValue(localeInfo, OF_STR("NSStringFormatRuleInfoKey"));
            }
            OCFIndex count = OCFDictionaryGetCount(familyRules);
            if (OF_WORD_SHIFT((size_t)count, 60) != 0) {
                OCFStringRef reason = OCFStringCreateWithFormat(
                    NULL,
                    NULL,
                    OF_STR(
                        "*** attempt to create a temporary id buffer which is too large or with a negative "
                        "count (%lu) -- possibly data is corrupt"
                    ),
                    (size_t)count
                );
                id exception = ((id (*)(id, SEL, id, id, id))objc_msgSend)(
                    (id)exceptionClass,
                    sel_getUid("exceptionWithName:reason:userInfo:"),
                    (id)ONSGenericException,
                    (id)reason,
                    nil
                );
                OCFRelease(reason);
                objc_exception_throw(exception);
            }
            size_t used = (size_t)count <= 0x100 ? ((size_t)count > 1 ? (size_t)count : 1) : 1;
            const void *stackBuffer[used];
            size_t allocatedSize = 0;
            const void **keys = stackBuffer;
            void *allocated = NULL;
            if ((size_t)count > 0x100) {
                keys = _OCFCreateArrayStorage((size_t)count > 1 ? (size_t)count : 1, false, &allocatedSize);
                allocated = keys;
            }
            OCFDictionaryGetKeysAndValues(familyRules, keys, NULL);
            OCFStringRef family = NULL;
            for (OCFIndex remaining = count; remaining != 0; --remaining) {
                OCFStringRef key = *keys++;
                if (OCFStringCompare(key, OF_STR("default"), 0) != kOCFCompareEqualTo &&
                    _CFStringContentsInCharacterSet(argument, OCFDictionaryGetValue(familyInfo, key))) {
                    family = key;
                    break;
                }
            }
            format = OCFDictionaryGetValue(familyRules, family != NULL ? family : OF_STR("default"));
            free(allocated);
        }
        if (localeInfo != NULL) {
            OCFRelease(localeInfo);
        }
        if (format == NULL) {
            return NULL;
        }
        return OCFRetain(format);
    }
    if (ruleType == OF_STR("NSStringAffixRuleType") || OCFEqual(ruleType, OF_STR("NSStringAffixRuleType"))) {
        OCFTypeRef language = OCFLocaleGetValue(resolvedLocale, kOCFLocaleLanguageCodeKey);
        if (OCFGetTypeID(argument) == OCFStringGetTypeID()) {
            const void *format = OCFDictionaryGetValue(
                configuration,
                _CFStringFindAffixRuleFormat(language != NULL ? language : OF_STR("en"), argument)
            );
            if (format != NULL) {
                return OCFRetain(format);
            }
        }
        const void *format = OCFDictionaryGetValue(configuration, OF_STR("other"));
        if (format == NULL) {
            return NULL;
        }
        return OCFRetain(format);
    }
    if (ruleType == OF_STR("NSStringVocativeNameFormattingRuleType") ||
        OCFEqual(ruleType, OF_STR("NSStringVocativeNameFormattingRuleType"))) {
        // 7 is CFString's builtin type ID, compared as a literal natively.
        OCFTypeID argumentType = OCFGetTypeID(argument);
        OCFTypeRef givenName = argument;
        if (argumentType != 7) {
            dispatch_once(&personNameClassesOnceToken, ^{
              __NSPersonNameComponentsClass = objc_lookUpClass(OF_NAME_STRING(ONSPersonNameComponents));
              __NSPersonNameComponentsFormatterClass =
                  objc_lookUpClass(OF_NAME_STRING(ONSPersonNameComponentsFormatter));
            });
            if (__NSPersonNameComponentsClass == Nil) {
                return NULL;
            }
            if (!objc_opt_isKindOfClass((id)argument, __NSPersonNameComponentsClass)) {
                return NULL;
            }
            givenName = ((OCFTypeRef (*)(OCFTypeRef, SEL))objc_msgSend)(argument, sel_getUid("givenName"));
            if (OCFGetTypeID(givenName) != 7) {
                return NULL;
            }
        }
        if (givenName == NULL) {
            return NULL;
        }
        OCFStringRef vocative = _OCFStringCopyVocativeCaseOfGivenName(givenName, resolvedLocale);
        if (argumentType == 7 || vocative == NULL) {
            return vocative;
        }
        id components = ((id (*)(OCFTypeRef, SEL))objc_msgSend)(argument, sel_getUid("copy"));
        ((void (*)(id, SEL, OCFStringRef))objc_msgSend)(components, sel_getUid("setGivenName:"), vocative);
        dispatch_once(&formatterOnceToken, ^{
          formatter = objc_alloc_init(__NSPersonNameComponentsFormatterClass);
        });
        OCFRelease(vocative);
        id formatted = ((id (*)(id, SEL, id))objc_msgSend)(
            formatter, sel_getUid("stringFromPersonNameComponents:"), components
        );
        OCFStringRef result = ((OCFStringRef (*)(id, SEL))objc_msgSend)(formatted, sel_getUid("copy"));
        OCFRelease(components);
        return result;
    }
    return NULL;
}

// Native ___NSCFLocalizedAttributedStringClass and ___NSGetCFLocalizedAttributedStringClass.onceToken
// (__DATA,__bss). Non-external.
static Class __NSCFLocalizedAttributedStringClass;
static dispatch_once_t __NSGetCFLocalizedAttributedStringClassOnceToken;

// Native __CFStringObjCFormatRequiresInflection, 0x1c02cbf28, with .cold.1 (0x1c03122c8, through
// _OUTLINED_FUNCTION_0_56, 0x1c02cbfa8) and the global block
// _____NSGetCFLocalizedAttributedStringClass_block_invoke (0x1c02cbf84; that block's body is listed for the
// notify-error-misc family and is part of this function's dispatch_once here). Hidden (was a private
// external). The format's class (object_getClass, read before the lookup) must be exactly
// __NSCFLocalizedAttributedString, looked up once with objc_lookUpClass.
bool _CFStringObjCFormatRequiresInflection(OCFStringRef format) {
    Class formatClass = object_getClass((id)format);
    dispatch_once(&__NSGetCFLocalizedAttributedStringClassOnceToken, ^{
      __NSCFLocalizedAttributedStringClass =
          objc_lookUpClass(OF_NAME_STRING(__ONSCFLocalizedAttributedString));
    });
    return formatClass == __NSCFLocalizedAttributedStringClass;
}

// Native __CFStringCreateFormatWithInflectionAndArguments, 0x1c02cbf70. Hidden (was a private external).
// Tail-sends +createStringRequiringInflectionWithFormat:formatOptions:arguments: to the class that
// __CFStringObjCFormatRequiresInflection looked up (its only caller); the allocator is ignored.
OCFStringRef _CFStringCreateFormatWithInflectionAndArguments(
    OCFAllocatorRef alloc, OCFDictionaryRef formatOptions, OCFStringRef format, va_list arguments
) {
    (void)alloc;
    return ((OCFStringRef (*)(Class, SEL, OCFStringRef, OCFDictionaryRef, va_list))objc_msgSend)(
        __NSCFLocalizedAttributedStringClass,
        sel_getUid("createStringRequiringInflectionWithFormat:formatOptions:arguments:"),
        format,
        formatOptions,
        arguments
    );
}
