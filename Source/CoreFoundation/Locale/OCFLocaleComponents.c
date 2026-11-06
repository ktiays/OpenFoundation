//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/OCFLocaleComponents.h"

#include "CoreFoundation/Locale/OCFCalendarEntryPoints.h"
#include "CoreFoundation/Locale/OCFLocaleEntryPoints.h"
#include "CoreFoundation/Locale/OCFLocaleKeys.h"
#include "CoreFoundation/String/OCFString.h"

#include <malloc/malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: CFLocaleCreateComponentsFromLocaleIdentifier (0x1c01a1040),
// CFLocaleCreateLocaleIdentifierFromComponents (0x1c01a13c8) and their helper __CStringFromString
// (0x1c01a1720) are contiguous in the iOS Simulator __text section, inside the locale-identifier region whose
// other helpers Locale/OCFLocaleIdentifier.c restores; that unit belongs to another family, so these three
// have their own translation unit. The native source path is not in the evidence. Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read in full; the pseudocode was a guide only); iOS Device 24A446: unread. Reading: not yet written; the
// evidence is the assembly in Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.

// ICU (libicucore, linked by the native image); supplied, never reimplemented. Declared here because the SDK
// installs no uloc.h or uenum.h. UErrorCode is a 32-bit enumeration; UEnumeration is opaque.
typedef struct UEnumeration UEnumeration;

extern int32_t uloc_getLanguage(const char *localeID, char *language, int32_t languageCapacity, int32_t *err);
extern int32_t uloc_getScript(const char *localeID, char *script, int32_t scriptCapacity, int32_t *err);
extern int32_t uloc_getCountry(const char *localeID, char *country, int32_t countryCapacity, int32_t *err);
extern int32_t uloc_getVariant(const char *localeID, char *variant, int32_t variantCapacity, int32_t *err);
extern UEnumeration *uloc_openKeywords(const char *localeID, int32_t *status);
extern const char *uenum_next(UEnumeration *en, int32_t *resultLength, int32_t *status);
extern void uenum_close(UEnumeration *en);
extern int32_t uloc_getKeywordValue(
    const char *localeID, const char *keywordName, char *buffer, int32_t bufferCapacity, int32_t *status
);
extern int32_t uloc_setKeywordValue(
    const char *keywordName, const char *keywordValue, char *buffer, int32_t bufferCapacity, int32_t *status
);

// Native CFLocaleCreateComponentsFromLocaleIdentifier, 0x1c01a1040. External.
// The identifier is converted with CFStringGetBytes (UTF-8, loss byte '?', external representation, at most
// 256 bytes into a 257-byte buffer, then terminated at the used length). The language, script, country and
// variant are each read into one 257-byte buffer (capacity 0x101) with the status reset to zero first, and
// added only when the status is not positive and the length is at least one. Each keyword whose value
// (100-byte buffer) has a length of at least one and a status that is not positive is added as a key/value
// pair of strings when both strings were created; the enumeration ends at a NULL keyword or a positive
// status. A NULL identifier skips all of it (the status is still cleared). The mutable dictionary (capacity
// 10, CFType callbacks) is copied with the caller's allocator and released.
OCFDictionaryRef
OCFLocaleCreateComponentsFromLocaleIdentifier(OCFAllocatorRef allocator, OCFLocaleIdentifier localeID) {
    char keywordValue[100];
    char component[257];
    char cLocaleID[257];
    OCFMutableDictionaryRef working = OCFDictionaryCreateMutable(
        allocator, 10, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    int32_t status = 0;
    if (localeID != NULL) {
        OCFIndex usedLength = 0;
        OCFStringGetBytes(
            localeID,
            (OCFRange){0, OCFStringGetLength(localeID)},
            kOCFStringEncodingUTF8,
            '?',
            true,
            (uint8_t *)cLocaleID,
            0x100,
            &usedLength
        );
        cLocaleID[usedLength] = '\0';
        int32_t length = uloc_getLanguage(cLocaleID, component, 0x101, &status);
        if (status <= 0 && length >= 1) {
            OCFStringRef string = OCFStringCreateWithBytes(
                allocator, (const uint8_t *)component, length, kOCFStringEncodingUTF8, true
            );
            OCFDictionaryAddValue(working, kOCFLocaleLanguageCodeKey, string);
            OCFRelease(string);
        }
        status = 0;
        length = uloc_getScript(cLocaleID, component, 0x101, &status);
        if (status <= 0 && length >= 1) {
            OCFStringRef string = OCFStringCreateWithBytes(
                allocator, (const uint8_t *)component, length, kOCFStringEncodingUTF8, true
            );
            OCFDictionaryAddValue(working, kOCFLocaleScriptCodeKey, string);
            OCFRelease(string);
        }
        status = 0;
        length = uloc_getCountry(cLocaleID, component, 0x101, &status);
        if (status <= 0 && length >= 1) {
            OCFStringRef string = OCFStringCreateWithBytes(
                allocator, (const uint8_t *)component, length, kOCFStringEncodingUTF8, true
            );
            OCFDictionaryAddValue(working, kOCFLocaleCountryCodeKey, string);
            OCFRelease(string);
        }
        status = 0;
        length = uloc_getVariant(cLocaleID, component, 0x101, &status);
        if (status <= 0 && length >= 1) {
            OCFStringRef string = OCFStringCreateWithBytes(
                allocator, (const uint8_t *)component, length, kOCFStringEncodingUTF8, true
            );
            OCFDictionaryAddValue(working, kOCFLocaleVariantCodeKey, string);
            OCFRelease(string);
        }
        status = 0;
        UEnumeration *keywords = uloc_openKeywords(cLocaleID, &status);
        int32_t keywordLength = 0;
        const char *keyword = uenum_next(keywords, &keywordLength, &status);
        if (keyword != NULL && status <= 0) {
            while (true) {
                int32_t valueLength = uloc_getKeywordValue(cLocaleID, keyword, keywordValue, 100, &status);
                if (valueLength >= 1 && status <= 0) {
                    OCFStringRef key = OCFStringCreateWithBytes(
                        allocator, (const uint8_t *)keyword, strlen(keyword), kOCFStringEncodingUTF8, true
                    );
                    OCFStringRef value = OCFStringCreateWithBytes(
                        allocator,
                        (const uint8_t *)keywordValue,
                        strlen(keywordValue),
                        kOCFStringEncodingUTF8,
                        true
                    );
                    if (key != NULL && value != NULL) {
                        OCFDictionaryAddValue(working, key, value);
                    }
                    if (key != NULL) {
                        OCFRelease(key);
                    }
                    if (value != NULL) {
                        OCFRelease(value);
                    }
                }
                keyword = uenum_next(keywords, &keywordLength, &status);
                if (keyword == NULL || status >= 1) {
                    break;
                }
            }
        }
        uenum_close(keywords);
    }
    OCFDictionaryRef result = OCFDictionaryCreateCopy(allocator, working);
    OCFRelease(working);
    return result;
}

static char *__CStringFromString(OCFStringRef string);

// Native CFLocaleCreateLocaleIdentifierFromComponents, 0x1c01a13c8. External.
// A NULL dictionary answers NULL. The values and then the keys are fetched into two stack allocations of
// (8 * count + 15) & ~15 bytes, of which only min(8 * count, 512) bytes are cleared first. The first pass
// takes the language, script, country and variant values (compared with CFEqual against the four keys in that
// order) as malloc'd UTF-8 C strings and clears the key slot of each one taken. The identifier is
// "%s%s%s%s%s%s%s" of language, "_" and script when there is a script, "_" when there is a country or a
// variant, the country, then "_" and the variant when there is a variant (missing parts are ""), copied into
// a 514-byte buffer (strlcpy). The second pass sets each remaining key as an ICU keyword (status cleared
// before each call): kCFLocaleCalendarKey becomes "calendar" with the calendar's identifier as its value;
// other values are converted as strings. The result is created with the caller's allocator from the buffer
// (UTF-8).
OCFLocaleIdentifier
OCFLocaleCreateLocaleIdentifierFromComponents(OCFAllocatorRef allocator, OCFDictionaryRef dictionary) {
    char buffer[514];
    if (dictionary == NULL) {
        return NULL;
    }
    OCFIndex count = OCFDictionaryGetCount(dictionary);
    size_t allocationSize = ((size_t)count * 8 + 15) & ~(size_t)15;
    size_t clearedSize = (size_t)count * 8 < 0x200 ? (size_t)count * 8 : 0x200;
    const void **values = (const void **)__builtin_alloca(allocationSize);
    bzero(values, clearedSize);
    const void **keys = (const void **)__builtin_alloca(allocationSize);
    bzero(keys, clearedSize);
    OCFDictionaryGetKeysAndValues(dictionary, keys, values);
    char *language = NULL;
    char *script = NULL;
    char *country = NULL;
    char *variant = NULL;
    if (count >= 1) {
        OCFStringRef languageKey = kOCFLocaleLanguageCodeKey;
        OCFStringRef scriptKey = kOCFLocaleScriptCodeKey;
        OCFStringRef countryKey = kOCFLocaleCountryCodeKey;
        OCFStringRef variantKey = kOCFLocaleVariantCodeKey;
        for (OCFIndex index = 0; index < count; index++) {
            if (OCFEqual(languageKey, keys[index])) {
                language = __CStringFromString(values[index]);
            } else if (OCFEqual(scriptKey, keys[index])) {
                script = __CStringFromString(values[index]);
            } else if (OCFEqual(countryKey, keys[index])) {
                country = __CStringFromString(values[index]);
            } else if (OCFEqual(variantKey, keys[index])) {
                variant = __CStringFromString(values[index]);
            } else {
                continue;
            }
            keys[index] = NULL;
        }
    }
    char *formatted = NULL;
    asprintf(
        &formatted,
        "%s%s%s%s%s%s%s",
        language != NULL ? language : "",
        script != NULL ? "_" : "",
        script != NULL ? script : "",
        (country != NULL || variant != NULL) ? "_" : "",
        country != NULL ? country : "",
        variant != NULL ? "_" : "",
        variant != NULL ? variant : ""
    );
    strlcpy(buffer, formatted, sizeof(buffer));
    free(language);
    free(script);
    free(country);
    free(variant);
    free(formatted);
    for (OCFIndex index = 0; index < count; index++) {
        if (keys[index] == NULL) {
            continue;
        }
        char *keyword = __CStringFromString(keys[index]);
        int comparison = strcmp(
            keyword,
            "kCFLocale"
            "CalendarKey"
        );
        char *value;
        if (comparison != 0) {
            value = __CStringFromString(values[index]);
        } else {
            value = __CStringFromString(OCFCalendarGetIdentifier((OCFCalendarRef)values[index]));
            char *calendarKeyword = strdup("calendar");
            free(keyword);
            keyword = calendarKeyword;
        }
        int32_t status = 0;
        uloc_setKeywordValue(keyword, value, buffer, 514, &status);
        free(keyword);
        free(value);
    }
    return OCFStringCreateWithCString(allocator, buffer, kOCFStringEncodingUTF8);
}

// Native __CStringFromString, 0x1c01a1720. Non-external.
// NULL answers NULL. The UTF-8 length (loss byte '?', no external representation) is measured with a first
// CFStringGetBytes, a buffer of that length plus one is allocated (malloc type 0x100004077774924), and the
// second CFStringGetBytes fills it with the measured length as its capacity before the terminator is stored
// at the used length.
static char *__CStringFromString(OCFStringRef string) {
    if (string == NULL) {
        return NULL;
    }
    OCFIndex length = OCFStringGetLength(string);
    OCFIndex usedLength = 0;
    OCFStringGetBytes(
        string, (OCFRange){0, length}, kOCFStringEncodingUTF8, '?', false, NULL, 0, &usedLength
    );
    char *buffer = (char *)malloc_type_malloc(usedLength + 1, 0x100004077774924);
    OCFStringGetBytes(
        string,
        (OCFRange){0, length},
        kOCFStringEncodingUTF8,
        '?',
        false,
        (uint8_t *)buffer,
        usedLength,
        &usedLength
    );
    buffer[usedLength] = '\0';
    return buffer;
}
