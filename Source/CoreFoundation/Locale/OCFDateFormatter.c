//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/OFPortHooks.h"
#include "CoreFoundation/Locale/OCFDateFormatter.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Date/OCFTimeZoneEntryPoints.h"
#include "CoreFoundation/Locale/OCFCalendarEntryPoints.h"
#include "CoreFoundation/Locale/OCFCalendarICU.h"
#include "CoreFoundation/Locale/OCFLocaleComponents.h"
#include "CoreFoundation/Locale/OCFLocaleEntryPoints.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <Block.h>
#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <math.h>
#include <os/lock.h>
#include <os/log.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Inferred organization: the CFDateFormatter functions restored here lie in one iOS Simulator __text region
// (useTemplatePatternGenerator 0x1c018ee3c through __CFDateFormatterSetSymbolsArray 0x1c0194710), with the
// unit's other functions (outside the scope) between them and the .cold pieces at 0x1c0307ef0 and
// 0x1c0307f04; the property keys are in the locale-keys table (Locale/OCFLocaleKeys.c). The native source
// path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; static tables read with
// Local/Scratch/cf-core/macho.py; the pseudocode was a guide only); iOS Device 24A446: unread. Reading: not
// yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json and, for the two global block invokes
// (___useTemplatePatternGenerator_block_invoke and _____CFDateFormatterCreateForcedString_block_invoke) and
// their literals, Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.

// ICU (libicucore, linked by the native image); supplied, never reimplemented. Declared here because the SDK
// installs no udat.h, ucal.h or udatpg.h. Enumerations are 32-bit; UBool is 8-bit; UDateFormat, UCalendar and
// UDateTimePatternGenerator are opaque (UDateTimePatternGenerator * is spelled void **, the block signature
// "v16@?0^^v8" of the native work blocks). UDate is a double.
extern void *udat_open(
    int32_t timeStyle,
    int32_t dateStyle,
    const char *locale,
    const UniChar *tzID,
    int32_t tzIDLength,
    const UniChar *pattern,
    int32_t patternLength,
    int32_t *status
);
extern void udat_close(void *format);
extern void udat_setContext(void *format, int32_t value, int32_t *status);
extern int32_t udat_getContext(const void *format, int32_t type, int32_t *status);
extern void udat_setLenient(void *format, int8_t isLenient);
extern int8_t udat_isLenient(const void *format);
extern void udat_applyPattern(void *format, int8_t localized, const UniChar *pattern, int32_t patternLength);
extern void udat_applyPatternRelative(
    void *format,
    const UniChar *datePattern,
    int32_t datePatternLength,
    const UniChar *timePattern,
    int32_t timePatternLength,
    int32_t *status
);
extern int32_t
udat_toPattern(const void *format, int8_t localized, UniChar *result, int32_t resultLength, int32_t *status);
extern int32_t
udat_toPatternRelativeDate(const void *format, UniChar *result, int32_t resultLength, int32_t *status);
extern int32_t
udat_toPatternRelativeTime(const void *format, UniChar *result, int32_t resultLength, int32_t *status);
extern void udat_set2DigitYearStart(void *format, double date, int32_t *status);
extern const void *udat_getCalendar(const void *format);
extern void udat_setCalendar(void *format, const void *calendar);
extern int32_t udat_format(
    const void *format,
    double dateToFormat,
    UniChar *result,
    int32_t resultLength,
    void *position,
    int32_t *status
);
extern int32_t udat_countSymbols(const void *format, int32_t type);
extern int32_t udat_getSymbols(
    const void *format,
    int32_t type,
    int32_t symbolIndex,
    UniChar *result,
    int32_t resultLength,
    int32_t *status
);
extern void udat_setSymbols(
    void *format,
    int32_t type,
    int32_t symbolIndex,
    const UniChar *value,
    int32_t valueLength,
    int32_t *status
);
extern void *ucal_clone(const void *calendar, int32_t *status);
extern void ucal_close(void *calendar);
extern void ucal_setAttribute(void *calendar, int32_t attribute, int32_t newValue);
extern void ucal_setTimeZone(void *calendar, const UniChar *zoneID, int32_t length, int32_t *status);
extern double ucal_getGregorianChange(const void *calendar, int32_t *status);
extern void ucal_setGregorianChange(void *calendar, double date, int32_t *status);
extern void **udatpg_open(const char *locale, int32_t *status);
extern void udatpg_close(void **generator);
extern int32_t uadatpg_remapPatternWithOptions(
    void **generator,
    const UniChar *pattern,
    int32_t patternLength,
    uint32_t options,
    UniChar *result,
    int32_t resultCapacity,
    int32_t *status
);

// libobjc's autorelease pool entry points (the native code calls them directly); supplied, never
// reimplemented.
extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *context);

// The image's Mach-O header (dword_1C014A000), the dso argument of the native os_log call.
extern struct mach_header __dso_handle;

// The CFDateFormatter instance, 528 bytes (_CFRuntimeCreateInstance extra bytes 0x200), read from the
// restored functions: the ICU date format at 0x10, the locale at 0x18, the time and date styles at 0x20 and
// 0x28, the format at 0x30 and the default format at 0x38; the property values from 0x40 (lenient, relative
// formatting, the has-custom-format flag, the time zone, calendar, calendar identifier, two-digit start date,
// default date and Gregorian start date); the eighteen symbol arrays (0x88-0x110, in the order of
// symbolTypes); the AM and PM symbols (0x118, 0x120); the ambiguous-year strategy, character-direction
// and formatting-context values (0x128-0x138); the eighteen symbol arrays of the AppleICUDateTimeSymbols
// preference (0x140-0x1c8, same order); the date and time format strings of the AppleICUDateFormatStrings and
// AppleICUTimeFormatStrings preferences (0x1d0, 0x1d8); the force-24-hour and force-12-hour preferences
// (0x1e0, 0x1e8); the preference AM and PM symbols (0x1f0, 0x1f8); and the AppleFirstWeekday and
// AppleMinDaysInFirstWeek dictionaries (0x200, 0x208). Field names are local descriptive spellings;
// confidence: verified for every offset (each is read or written by the restored functions).
struct __OCFDateFormatter {
    OCFRuntimeBase base;
    void *icuFormatter;
    OCFLocaleRef locale;
    OCFDateFormatterStyle timeStyle;
    OCFDateFormatterStyle dateStyle;
    OCFStringRef format;
    OCFStringRef defaultFormat;
    OCFBooleanRef isLenient;
    OCFBooleanRef doesRelativeDateFormatting;
    OCFBooleanRef hasCustomFormat;
    OCFTimeZoneRef timeZone;
    OCFCalendarRef calendar;
    OCFStringRef calendarName;
    OCFDateRef twoDigitStartDate;
    OCFDateRef defaultDate;
    OCFDateRef gregorianStartDate;
    OCFArrayRef symbols[18];
    OCFStringRef amSymbol;
    OCFStringRef pmSymbol;
    OCFNumberRef ambiguousYearStrategy;
    OCFBooleanRef usesCharacterDirection;
    OCFNumberRef formattingContext;
    OCFArrayRef customSymbols[18];
    OCFStringRef customDateFormat;
    OCFStringRef customTimeFormat;
    OCFBooleanRef force24HourTime;
    OCFBooleanRef force12HourTime;
    OCFStringRef customAMSymbol;
    OCFStringRef customPMSymbol;
    OCFDictionaryRef customFirstWeekday;
    OCFDictionaryRef customMinDaysInFirstWeek;
};

OF_ABI_ASSERT(sizeof(struct __OCFDateFormatter) == 0x210, "The native date formatter is 528 bytes.");
OF_ABI_ASSERT(offsetof(struct __OCFDateFormatter, symbols) == 0x88, "Symbol arrays at 0x88.");
OF_ABI_ASSERT(offsetof(struct __OCFDateFormatter, formattingContext) == 0x138, "Context at 0x138.");
OF_ABI_ASSERT(offsetof(struct __OCFDateFormatter, customSymbols) == 0x140, "Preference symbols at 0x140.");
OF_ABI_ASSERT(offsetof(struct __OCFDateFormatter, customFirstWeekday) == 0x200, "First weekday at 0x200.");

// Native __CFDateFormatterDeallocate, 0x1c018f718. Non-external; the class's finalizer. The ICU formatter is
// closed, then every retained field is released in the native order: the locale, the formats, the flags but
// hasCustomFormat, the time zone through the first custom symbol array, then the custom symbol arrays 2, 3,
// 8, 9, 1, 4, 10, 5, 6, 7, 11-15 and 17 (16 is not released), the custom formats, the forced hour cycles, the
// custom AM and PM symbols and the two custom calendar dictionaries. Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device divergence: unread).
static void __CFDateFormatterDeallocate(OCFTypeRef cf) {
    struct __OCFDateFormatter *formatter = (struct __OCFDateFormatter *)cf;
    if (formatter->icuFormatter != NULL) {
        udat_close(formatter->icuFormatter);
    }
    if (formatter->locale != NULL) {
        OCFRelease(formatter->locale);
    }
    if (formatter->format != NULL) {
        OCFRelease(formatter->format);
    }
    if (formatter->defaultFormat != NULL) {
        OCFRelease(formatter->defaultFormat);
    }
    if (formatter->isLenient != NULL) {
        OCFRelease(formatter->isLenient);
    }
    if (formatter->doesRelativeDateFormatting != NULL) {
        OCFRelease(formatter->doesRelativeDateFormatting);
    }
    if (formatter->timeZone != NULL) {
        OCFRelease(formatter->timeZone);
    }
    if (formatter->calendar != NULL) {
        OCFRelease(formatter->calendar);
    }
    if (formatter->calendarName != NULL) {
        OCFRelease(formatter->calendarName);
    }
    if (formatter->twoDigitStartDate != NULL) {
        OCFRelease(formatter->twoDigitStartDate);
    }
    if (formatter->defaultDate != NULL) {
        OCFRelease(formatter->defaultDate);
    }
    if (formatter->gregorianStartDate != NULL) {
        OCFRelease(formatter->gregorianStartDate);
    }
    if (formatter->symbols[0] != NULL) {
        OCFRelease(formatter->symbols[0]);
    }
    if (formatter->symbols[1] != NULL) {
        OCFRelease(formatter->symbols[1]);
    }
    if (formatter->symbols[2] != NULL) {
        OCFRelease(formatter->symbols[2]);
    }
    if (formatter->symbols[3] != NULL) {
        OCFRelease(formatter->symbols[3]);
    }
    if (formatter->symbols[4] != NULL) {
        OCFRelease(formatter->symbols[4]);
    }
    if (formatter->symbols[5] != NULL) {
        OCFRelease(formatter->symbols[5]);
    }
    if (formatter->symbols[6] != NULL) {
        OCFRelease(formatter->symbols[6]);
    }
    if (formatter->symbols[7] != NULL) {
        OCFRelease(formatter->symbols[7]);
    }
    if (formatter->symbols[8] != NULL) {
        OCFRelease(formatter->symbols[8]);
    }
    if (formatter->symbols[9] != NULL) {
        OCFRelease(formatter->symbols[9]);
    }
    if (formatter->symbols[10] != NULL) {
        OCFRelease(formatter->symbols[10]);
    }
    if (formatter->symbols[11] != NULL) {
        OCFRelease(formatter->symbols[11]);
    }
    if (formatter->symbols[12] != NULL) {
        OCFRelease(formatter->symbols[12]);
    }
    if (formatter->symbols[13] != NULL) {
        OCFRelease(formatter->symbols[13]);
    }
    if (formatter->symbols[14] != NULL) {
        OCFRelease(formatter->symbols[14]);
    }
    if (formatter->symbols[15] != NULL) {
        OCFRelease(formatter->symbols[15]);
    }
    if (formatter->symbols[16] != NULL) {
        OCFRelease(formatter->symbols[16]);
    }
    if (formatter->symbols[17] != NULL) {
        OCFRelease(formatter->symbols[17]);
    }
    if (formatter->amSymbol != NULL) {
        OCFRelease(formatter->amSymbol);
    }
    if (formatter->pmSymbol != NULL) {
        OCFRelease(formatter->pmSymbol);
    }
    if (formatter->ambiguousYearStrategy != NULL) {
        OCFRelease(formatter->ambiguousYearStrategy);
    }
    if (formatter->usesCharacterDirection != NULL) {
        OCFRelease(formatter->usesCharacterDirection);
    }
    if (formatter->formattingContext != NULL) {
        OCFRelease(formatter->formattingContext);
    }
    if (formatter->customSymbols[0] != NULL) {
        OCFRelease(formatter->customSymbols[0]);
    }
    if (formatter->customSymbols[2] != NULL) {
        OCFRelease(formatter->customSymbols[2]);
    }
    if (formatter->customSymbols[3] != NULL) {
        OCFRelease(formatter->customSymbols[3]);
    }
    if (formatter->customSymbols[8] != NULL) {
        OCFRelease(formatter->customSymbols[8]);
    }
    if (formatter->customSymbols[9] != NULL) {
        OCFRelease(formatter->customSymbols[9]);
    }
    if (formatter->customSymbols[1] != NULL) {
        OCFRelease(formatter->customSymbols[1]);
    }
    if (formatter->customSymbols[4] != NULL) {
        OCFRelease(formatter->customSymbols[4]);
    }
    if (formatter->customSymbols[10] != NULL) {
        OCFRelease(formatter->customSymbols[10]);
    }
    if (formatter->customSymbols[5] != NULL) {
        OCFRelease(formatter->customSymbols[5]);
    }
    if (formatter->customSymbols[6] != NULL) {
        OCFRelease(formatter->customSymbols[6]);
    }
    if (formatter->customSymbols[7] != NULL) {
        OCFRelease(formatter->customSymbols[7]);
    }
    if (formatter->customSymbols[11] != NULL) {
        OCFRelease(formatter->customSymbols[11]);
    }
    if (formatter->customSymbols[12] != NULL) {
        OCFRelease(formatter->customSymbols[12]);
    }
    if (formatter->customSymbols[13] != NULL) {
        OCFRelease(formatter->customSymbols[13]);
    }
    if (formatter->customSymbols[14] != NULL) {
        OCFRelease(formatter->customSymbols[14]);
    }
    if (formatter->customSymbols[15] != NULL) {
        OCFRelease(formatter->customSymbols[15]);
    }
    if (formatter->customSymbols[17] != NULL) {
        OCFRelease(formatter->customSymbols[17]);
    }
    if (formatter->customDateFormat != NULL) {
        OCFRelease(formatter->customDateFormat);
    }
    if (formatter->customTimeFormat != NULL) {
        OCFRelease(formatter->customTimeFormat);
    }
    if (formatter->force24HourTime != NULL) {
        OCFRelease(formatter->force24HourTime);
    }
    if (formatter->force12HourTime != NULL) {
        OCFRelease(formatter->force12HourTime);
    }
    if (formatter->customAMSymbol != NULL) {
        OCFRelease(formatter->customAMSymbol);
    }
    if (formatter->customPMSymbol != NULL) {
        OCFRelease(formatter->customPMSymbol);
    }
    if (formatter->customFirstWeekday != NULL) {
        OCFRelease(formatter->customFirstWeekday);
    }
    if (formatter->customMinDaysInFirstWeek != NULL) {
        OCFRelease(formatter->customMinDaysInFirstWeek);
    }
}

// Native __CFDateFormatterCopyDescription, 0x1c018fa0c. Non-external; the class's debug description:
// "<CFDateFormatter %p [%p]>" with the formatter and its allocator, created with that allocator.
static OCFStringRef __CFDateFormatterCopyDescription(OCFTypeRef cf) {
    OCFAllocatorRef allocator = OCFGetAllocator(cf);
    return OCFStringCreateWithFormat(
        allocator, NULL, OF_STR("<CFDateFormatter %p [%p]>"), cf, OCFGetAllocator(cf)
    );
}

// Native __CFDateFormatterClass, 0x22279d718 (__DATA_CONST,__const). Hidden (was private external); version
// 0, name "CFDateFormatter" (0x1c048062e), the finalizer and the debug description only. Builtin slot 56.
const OCFRuntimeClass __CFDateFormatterClass = {
    0,
    "CFDateFormatter",
    NULL,
    NULL,
    __CFDateFormatterDeallocate,
    NULL,
    NULL,
    NULL,
    __CFDateFormatterCopyDescription,
    NULL,
    NULL,
    0,
};

// The CFDateFormatter type ID, the literal 56 (0x38) at every native use.
#define kDateFormatterTypeID 56

// U_BUFFER_OVERFLOW_ERROR, the literal 15 at the native uses.
#define kICUBufferOverflowError 15

// UDAT_AM_PMS, the symbol type 5 at the native uses.
#define kICUAMPMSymbols 5

// The malloc type ID of the native UniChar buffers (0x1000040BDFB0063 at each call site).
#define kUniCharBufferTypeID 0x1000040BDFB0063ULL

static Boolean
useTemplatePatternGenerator(OCFLocaleRef locale, void (^__attribute__((noescape)) work)(void **));
static OCFDateFormatterRef __CreateCFDateFormatter(
    OCFAllocatorRef allocator,
    OCFLocaleRef locale,
    OCFDateFormatterStyle dateStyle,
    OCFDateFormatterStyle timeStyle,
    OCFBooleanRef calculateISO8601
);
static OCFStringRef __CFDateFormatterCreateForcedString(OCFDateFormatterRef formatter, OCFStringRef inString);
static void __ResetUDateFormat(OCFDateFormatterRef formatter, Boolean goingToHaveCustomFormat);
static void __CFDateFormatterSetProperty(
    OCFDateFormatterRef formatter, OCFStringRef key, OCFTypeRef value, Boolean directToICU
);
static OCFArrayRef __CFDateFormatterCopySymbolsArray(void *icuFormatter, int32_t icuCode, int32_t indexBase);
static void __substituteFormatStringFromPrefsDF(OCFDateFormatterRef formatter, Boolean doTime);
static void __ApplyUDateFormatSymbol(OCFDateFormatterRef formatter);
static void __CFDateFormatterStoreSymbolPrefs(const void *key, const void *value, void *context);
static void
__CFDateFormatterSetSymbolsArray(void *icuFormatter, int32_t icuCode, int32_t indexBase, OCFArrayRef array);

// Native _useTemplatePatternGenerator.ptg, .ptgLock, .ptgLocaleName and .pressureSrc (__DATA,__bss
// 0x22e6d27b8, 0x22e6d27c0, 0x22e6d27c8, 0x22e6d27d0). Non-external; the generator and its locale name are
// hidden (see Locale/OCFDateFormatter.h).
void **templatePatternGenerator;
static os_unfair_lock templatePatternGeneratorLock = OS_UNFAIR_LOCK_INIT;
char *templatePatternGeneratorLocaleName;
static dispatch_source_t templatePatternGeneratorPressureSource;

// Native useTemplatePatternGenerator, 0x1c018ee3c, with its memory-pressure handler block
// ___useTemplatePatternGenerator_block_invoke_2 (0x1c0193cc4). Non-external.
// The locale identifier (the empty constant string for a NULL locale) is used as a UTF-8 C string pointer or
// copied into a 768-byte buffer (NULL when the copy fails). Under the lock: a cached generator for another
// locale (strcmp) is closed and its name freed; with no cached generator one is opened (udatpg_open), and on
// success its locale name is strdup'ed and, the first time, a memory-pressure source (mask 6, warn and
// critical) is created on the global queue of qos_class_main() with flags 2 and resumed. The work block runs
// with the generator, still under the lock, when there is one and the status is not a failure; that is also
// the answer. The handler cancels and releases the source, clears it and calls the captured global block
// (useTemplatePatternGeneratorResetBlock, whose invoke is unread), under the lock. The native stack blocks
// carry no copy or dispose helpers (flags 0xc0000000); the work parameter is declared noescape to match,
// while the compiler's helpers for the escaping handler block, which captures a block, are its own
// (confidence: medium for that layout detail; behavior is the same, the captured block is global).
static Boolean
useTemplatePatternGenerator(OCFLocaleRef locale, void (^__attribute__((noescape)) work)(void **)) {
    OCFStringRef identifier = locale != NULL ? OCFLocaleGetIdentifier(locale) : OF_STR("");
    char buffer[768];
    const char *localeName = OCFStringGetCStringPtr(identifier, kOCFStringEncodingUTF8);
    if (localeName == NULL) {
        localeName = buffer;
        if (!OCFStringGetCString(identifier, buffer, 768, kOCFStringEncodingUTF8)) {
            localeName = NULL;
        }
    }
    os_unfair_lock_lock(&templatePatternGeneratorLock);
    void **generator;
    int32_t status;
    if (templatePatternGeneratorLocaleName != NULL &&
        strcmp(templatePatternGeneratorLocaleName, localeName) != 0) {
        udatpg_close(templatePatternGenerator);
        templatePatternGenerator = NULL;
        free(templatePatternGeneratorLocaleName);
        templatePatternGeneratorLocaleName = NULL;
        status = 0;
    } else {
        generator = templatePatternGenerator;
        status = 0;
        if (generator != NULL) {
            goto done;
        }
    }
    generator = udatpg_open(localeName, &status);
    templatePatternGenerator = generator;
    if (generator != NULL && status <= 0) {
        templatePatternGeneratorLocaleName = strdup(localeName);
        if (templatePatternGeneratorPressureSource == NULL) {
            dispatch_queue_t queue = dispatch_get_global_queue(qos_class_main(), 2);
            templatePatternGeneratorPressureSource = OF_PORT_ANSWER(
                memoryPressureSource,
                dispatch_source_create(DISPATCH_SOURCE_TYPE_MEMORYPRESSURE, 0, 6, queue),
                queue
            );
            void (^resetBlock)(void) = (void (^)(void))(void *)useTemplatePatternGeneratorResetBlock;
            dispatch_source_set_event_handler(templatePatternGeneratorPressureSource, ^{
              os_unfair_lock_lock(&templatePatternGeneratorLock);
              dispatch_source_cancel(templatePatternGeneratorPressureSource);
              dispatch_release(templatePatternGeneratorPressureSource);
              templatePatternGeneratorPressureSource = NULL;
              resetBlock();
              os_unfair_lock_unlock(&templatePatternGeneratorLock);
            });
            dispatch_resume(templatePatternGeneratorPressureSource);
            generator = templatePatternGenerator;
        }
    }
done:;
    Boolean result = generator != NULL && status < 1;
    if (result) {
        work(generator);
    }
    os_unfair_lock_unlock(&templatePatternGeneratorLock);
    return result;
}

// Native __CreateCFDateFormatter, 0x1c018fd98. Non-external.
// A NULL allocator is the thread default (TSD slot 1) or the system default. A new type-56 instance (0x200
// extra bytes) stores the time and date styles, each replaced by 2 (medium) when 5 or above (unsigned). A
// NULL locale is CFLocaleGetSystem(); the locale is retained. When calculateISO8601 is kCFBooleanTrue
// (pointer comparison) the calendar is a new Gregorian calendar with first weekday 2 and four minimum days.
// The time zone is CFTimeZoneCopyDefault(). A locale whose calendar identifier equals kCFGregorianCalendar
// gets the two-digit start date -1609459200.0 (qword_1C0373558, 1950-01-01). __ResetUDateFormat follows;
// without an ICU format the instance is released and NULL answered.
static OCFDateFormatterRef __CreateCFDateFormatter(
    OCFAllocatorRef allocator,
    OCFLocaleRef locale,
    OCFDateFormatterStyle dateStyle,
    OCFDateFormatterStyle timeStyle,
    OCFBooleanRef calculateISO8601
) {
    if (allocator == NULL) {
        allocator = _OCFGetTSD(1);
        if (allocator == NULL) {
            allocator = kOCFAllocatorSystemDefault;
        }
    }
    struct __OCFDateFormatter *formatter =
        (struct __OCFDateFormatter *)_OCFRuntimeCreateInstance(allocator, kDateFormatterTypeID, 0x200, NULL);
    if (formatter == NULL) {
        return NULL;
    }
    formatter->timeStyle = timeStyle;
    formatter->dateStyle = dateStyle;
    if ((uint64_t)dateStyle >= 5) {
        formatter->dateStyle = kOCFDateFormatterMediumStyle;
    }
    if ((uint64_t)timeStyle >= 5) {
        formatter->timeStyle = kOCFDateFormatterMediumStyle;
    }
    if (locale == NULL) {
        locale = OCFLocaleGetSystem();
    }
    formatter->locale = OCFRetain(locale);
    if (kOCFBooleanTrue == calculateISO8601) {
        OCFCalendarRef calendar =
            OCFCalendarCreateWithIdentifier(kOCFAllocatorSystemDefault, kOCFGregorianCalendar);
        OCFCalendarSetFirstWeekday(calendar, 2);
        OCFCalendarSetMinimumDaysInFirstWeek(calendar, 4);
        formatter->calendar = calendar;
    }
    formatter->timeZone = OCFTimeZoneCopyDefault();
    OCFTypeRef calendarIdentifier = OCFLocaleGetValue(formatter->locale, kOCFLocaleCalendarIdentifierKey);
    if (calendarIdentifier != NULL && OCFEqual(calendarIdentifier, kOCFGregorianCalendar)) {
        formatter->twoDigitStartDate = OCFDateCreate(kOCFAllocatorSystemDefault, -1609459200.0);
    }
    __ResetUDateFormat(formatter, false);
    if (formatter->icuFormatter == NULL) {
        OCFRelease(formatter);
        return NULL;
    }
    return formatter;
}

// Native CFDateFormatterSetFormat, 0x1c018ff04. External. Type 56 is asserted (_CFAssertMismatchedTypeID).
// The forced string (__CFDateFormatterCreateForcedString) is applied unless it is the current format (pointer
// comparison) or longer than 1024 characters. Before it, a formatter without a custom format
// (hasCustomFormat not kCFBooleanTrue) that does relative formatting (kCFBooleanTrue) is reset for a custom
// format. The characters come from CFStringGetCharactersPtr or a buffer (malloc'ed above 256 bytes, else a
// zeroed stack buffer, NULL for an empty string); the format becomes a copy (the formatter's allocator) and
// hasCustomFormat stores kCFBooleanTrue unretained. The forced string is released when not NULL.
void OCFDateFormatterSetFormat(OCFDateFormatterRef formatter, OCFStringRef formatString) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != kDateFormatterTypeID) {
        _CFAssertMismatchedTypeID(kDateFormatterTypeID, typeID);
    }
    OCFStringRef forced = __CFDateFormatterCreateForcedString(formatter, formatString);
    OCFIndex length = OCFStringGetLength(forced);
    if (formatter->format != forced && length <= 0x400) {
        OCFBooleanRef trueValue = kOCFBooleanTrue;
        if (formatter->hasCustomFormat != trueValue && formatter->doesRelativeDateFormatting == trueValue) {
            __ResetUDateFormat(formatter, true);
        }
        size_t bytes = (size_t)length << 1;
        UniChar *buffer;
        if (bytes >= 0x101) {
            buffer = malloc_type_malloc(bytes, kUniCharBufferTypeID);
        } else if (length >= 1) {
            buffer = __builtin_alloca(bytes);
            bzero(buffer, bytes);
        } else {
            buffer = NULL;
        }
        const UniChar *characters = OCFStringGetCharactersPtr(forced);
        if (characters == NULL) {
            OCFStringGetCharacters(forced, (OCFRange){0, length}, buffer);
            characters = buffer;
        }
        udat_applyPattern(formatter->icuFormatter, false, characters, (int32_t)length);
        if (formatter->format != NULL) {
            OCFRelease(formatter->format);
        }
        formatter->format = OCFStringCreateCopy(OCFGetAllocator(formatter), forced);
        formatter->hasCustomFormat = trueValue;
        if (bytes >= 0x101) {
            free(buffer);
        }
    }
    if (forced != NULL) {
        OCFRelease(forced);
    }
}

// Native CFDateFormatterCreate, 0x1c01900b0. External. A tail call of __CreateCFDateFormatter with
// kCFBooleanFalse.
OCFDateFormatterRef OCFDateFormatterCreate(
    OCFAllocatorRef allocator,
    OCFLocaleRef locale,
    OCFDateFormatterStyle dateStyle,
    OCFDateFormatterStyle timeStyle
) {
    return __CreateCFDateFormatter(allocator, locale, dateStyle, timeStyle, kOCFBooleanFalse);
}

// Native ___CFDateFormatterCreateForcedString.hourCharacters and .onceToken (__DATA,__bss 0x22e6d27d8,
// 0x22e6d27e0). Non-external; the character set is hidden (see Locale/OCFDateFormatter.h).
const struct __OCFCharacterSet *forcedStringHourCharacters;
static dispatch_once_t forcedStringOnceToken;

// Native __CFDateFormatterCreateForcedString, 0x1c01901a0, with .cold.1 (0x1c0307ef0, the dispatch_once of
// ___block_literal_global.31) and its work block _____CFDateFormatterCreateForcedString_block_invoke_2
// (0x1c0193d50). Non-external.
// A NULL string answers NULL. The options are 0x40000000 (UADATPG_FORCE_24_HOUR_CYCLE) when the force-24-hour
// preference is true, else 0x20000000 (UADATPG_FORCE_12_HOUR_CYCLE) when the force-12-hour preference is
// true; both are read when present (the 12-hour one first). Without options, or when the string has no
// character of forcedStringHourCharacters (searched from a range initialized to {kCFNotFound, 0},
// xmmword_1C0373580), the string is retained and answered. Otherwise the template pattern generator of the
// formatter's locale remaps it: the work block remaps into a 256-character stack buffer, retrying with a
// calloc'ed buffer of the reported length plus one on U_BUFFER_OVERFLOW_ERROR, and creates the result string
// (system default allocator) only when the status is not a failure; the remapped length is stored even on
// failure. A result with a positive remapped length is answered; any other result is released and the string
// retained instead. The __block variables are disposed of (_Block_object_dispose, flag 8) in reverse order of
// declaration.
static OCFStringRef
__CFDateFormatterCreateForcedString(OCFDateFormatterRef formatter, OCFStringRef inString) {
    if (inString == NULL) {
        return NULL;
    }
    uint32_t options;
    if (formatter->force12HourTime != NULL) {
        options = OCFBooleanGetValue(formatter->force12HourTime) ? 0x20000000 : 0;
    } else {
        options = 0;
    }
    if (formatter->force24HourTime != NULL && OCFBooleanGetValue(formatter->force24HourTime)) {
        options = 0x40000000;
    } else if (options == 0) {
        return OCFRetain(inString);
    }
    dispatch_once(&forcedStringOnceToken, (dispatch_block_t)(void *)forcedStringHourCharactersOnceBlock);
    OCFRange result = {kOCFNotFound, 0};
    OCFCharacterSetRef hourCharacters = forcedStringHourCharacters;
    if (!OCFStringFindCharacterFromSet(
            inString, hourCharacters, (OCFRange){0, OCFStringGetLength(inString)}, 0, &result
        ) ||
        result.location == kOCFNotFound) {
        return OCFRetain(inString);
    }
    __block OCFStringRef outString = NULL;
    __block int32_t outLength = 0;
    Boolean used = useTemplatePatternGenerator(formatter->locale, ^(void **generator) {
      OCFIndex length = OCFStringGetLength(inString);
      size_t bytes = (size_t)length << 1;
      UniChar *buffer;
      if (bytes >= 0x101) {
          buffer = malloc_type_malloc(bytes, kUniCharBufferTypeID);
      } else if (length >= 1) {
          buffer = __builtin_alloca(bytes);
          bzero(buffer, bytes);
      } else {
          buffer = NULL;
      }
      const UniChar *pattern = OCFStringGetCharactersPtr(inString);
      if (pattern == NULL) {
          OCFStringGetCharacters(inString, (OCFRange){0, length}, buffer);
          pattern = buffer;
      }
      int32_t status = 0;
      UniChar remapped[256];
      outLength = uadatpg_remapPatternWithOptions(
          generator, pattern, (int32_t)length, options, remapped, 0x100, &status
      );
      if (status <= 0) {
          outString =
              OCFStringCreateWithCharacters(kOCFAllocatorSystemDefault, remapped, (OCFIndex)outLength);
      } else if (status == kICUBufferOverflowError) {
          status = 0;
          UniChar *large = malloc_type_calloc((size_t)((int64_t)outLength + 1), 2, kUniCharBufferTypeID);
          outLength = uadatpg_remapPatternWithOptions(
              generator, pattern, (int32_t)length, options, large, outLength + 1, &status
          );
          if (status <= 0) {
              outString =
                  OCFStringCreateWithCharacters(kOCFAllocatorSystemDefault, large, (OCFIndex)outLength);
          }
          free(large);
      }
      if (bytes >= 0x101) {
          free(buffer);
      }
    });
    OCFStringRef answer = outString;
    if (used) {
        if (answer != NULL) {
            if (outLength > 0) {
                return answer;
            }
            OCFRelease(answer);
        }
    } else if (answer != NULL) {
        OCFRelease(answer);
    }
    return OCFRetain(inString);
}

// unk_1C0373590: the ICU date styles of CFDateFormatterStyle 0-3 (UDAT_NONE, UDAT_SHORT, UDAT_MEDIUM,
// UDAT_LONG); styles above 3 use 0 (UDAT_FULL).
static const int32_t icuDateStyles[4] = {-1, 3, 2, 1};

// off_22279D830: the AppleICUDateFormatStrings and AppleICUTimeFormatStrings keys of styles 1-4, the
// constant strings "1"-"4" (0x2227ae540-0x2227ae5a0); other styles use "0".
static const OCFConstantString styleKeyString1 = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "1", 1
};
static const OCFConstantString styleKeyString2 = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "2", 1
};
static const OCFConstantString styleKeyString3 = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "3", 1
};
static const OCFConstantString styleKeyString4 = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "4", 1
};
static const OCFStringRef formatStringsStyleKeys[4] = {
    (OCFStringRef)&styleKeyString1,
    (OCFStringRef)&styleKeyString2,
    (OCFStringRef)&styleKeyString3,
    (OCFStringRef)&styleKeyString4,
};

// The os_log format literal of __ResetUDateFormat.cold.1, placed in __TEXT,__oslogstring like the
// compiler-generated native one.
__attribute__((
    section("__TEXT,__oslogstring,cstring_literals")
)) static const char emptyTimeZoneNameFormat[] = "Error: CFDateFormatter time zone has an empty name: %@";

// Native __ResetUDateFormat, 0x1c01903d8, with .cold.1 (0x1c0307f04, the os_log_error of a time zone without
// a name). Non-external.
// Inside an autorelease pool: the ICU format is closed and cleared; the locale identifier (the empty string
// for a NULL locale) is copied as UTF-8 into 768 bytes and the time zone name (GMT without a time zone) into
// at most 768 characters; a time zone whose name is NULL logs the error (type 0x10, the time zone pointer as
// %@) when enabled and stops. The ICU time style is -1 for style 0, 3, 2 and 1 for styles 1-3 and 0
// otherwise; the date style comes from icuDateStyles, with UDAT_RELATIVE (0x80) added when relative
// formatting is kCFBooleanTrue, there is no custom format (hasCustomFormat kCFBooleanTrue or
// goingToHaveCustomFormat) and the date style is not 0. A failed udat_open stops. Relative formatting sets
// the capitalization context 0x103; lenience is whether the lenient property is kCFBooleanTrue; two zero
// styles apply an empty pattern; a non-relative custom format is applied. A Gregorian locale calendar sets
// the two-digit start -631152000000.0 (qword_1C0373560). The format is stored, then the preferences are read
// into the instance: the AppleICUDateTimeSymbols dictionary (applied through
// __CFDateFormatterStoreSymbolPrefs), the AppleFirstWeekday and AppleMinDaysInFirstWeek dictionaries, the
// force-24-hour and force-12-hour Booleans and the date and time format strings of the style keys. A calendar
// for the ICU format is created from the locale identifier when there is a calendar or calendar identifier
// (else, or when that fails, a clone of the format's); it receives lenience, the time zone, the Gregorian
// change (the given start date, or for a Gregorian calendar identifier the format's own, 1582-10-15 when that
// read fails), the first weekday and minimum days (the calendar's, else the preference dictionaries' CFIndex
// values for the calendar identifier), and replaces the format's calendar. Non-relative or custom formats
// substitute the preference formats, apply the symbols and force the hour cycle of the pattern (at most 1024
// characters); relative formats apply the preference or relative date and time patterns, the symbols, and
// force the hour cycle of the time pattern. Last, the default format retains the format and every property
// set before is applied again directly to ICU. The stack status, buffer and pool slots the native frame
// shares are separate variables here.
static void __ResetUDateFormat(OCFDateFormatterRef formatter, Boolean goingToHaveCustomFormat) {
    void *pool = objc_autoreleasePoolPush();
    if (formatter->icuFormatter != NULL) {
        udat_close(formatter->icuFormatter);
    }
    formatter->icuFormatter = NULL;
    char localeName[768];
    localeName[0] = 0;
    OCFStringGetCString(
        formatter->locale != NULL ? OCFLocaleGetIdentifier(formatter->locale) : OF_STR(""),
        localeName,
        0x300,
        kOCFStringEncodingUTF8
    );
    UniChar timeZoneID[768];
    timeZoneID[0] = 0;
    OCFStringRef timeZoneName;
    if (formatter->timeZone != NULL) {
        timeZoneName = OCFTimeZoneGetName(formatter->timeZone);
        if (timeZoneName == NULL) {
            os_log_t log = _CFOSLog();
            if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
                uint8_t logBuffer[12] = {0x02, 0x01, 0x40, 0x08};
                OCFTimeZoneRef timeZone = formatter->timeZone;
                memcpy(&logBuffer[4], &timeZone, sizeof(timeZone));
                _os_log_error_impl(
                    &__dso_handle,
                    log,
                    OS_LOG_TYPE_ERROR,
                    emptyTimeZoneNameFormat,
                    logBuffer,
                    sizeof(logBuffer)
                );
            }
            objc_autoreleasePoolPop(pool);
            return;
        }
    } else {
        timeZoneName = OF_STR("GMT");
    }
    OCFIndex timeZoneLength;
    if (OCFStringGetLength(timeZoneName) > 0x300) {
        timeZoneLength = 0x300;
    } else {
        timeZoneLength = OCFStringGetLength(timeZoneName);
    }
    OCFStringGetCharacters(timeZoneName, (OCFRange){0, timeZoneLength}, timeZoneID);
    OCFDateFormatterStyle dateStyle = formatter->dateStyle;
    int32_t udateStyle = (uint64_t)dateStyle <= 3 ? icuDateStyles[dateStyle] : 0;
    Boolean timeStyleIsNone = false;
    int32_t utimeStyle;
    OCFDateFormatterStyle timeStyle = formatter->timeStyle;
    if (timeStyle > 1) {
        if (timeStyle == 2) {
            utimeStyle = 2;
        } else if (timeStyle == 3) {
            utimeStyle = 1;
        } else {
            utimeStyle = 0;
        }
    } else if (timeStyle == 0) {
        utimeStyle = -1;
        timeStyleIsNone = true;
    } else if (timeStyle == 1) {
        utimeStyle = 3;
    } else {
        utimeStyle = 0;
    }
    OCFBooleanRef trueValue = kOCFBooleanTrue;
    Boolean wantRelative =
        formatter->doesRelativeDateFormatting != NULL && formatter->doesRelativeDateFormatting == trueValue;
    Boolean hasFormat = (formatter->hasCustomFormat != NULL && formatter->hasCustomFormat == trueValue) ||
                        goingToHaveCustomFormat;
    Boolean plainPatterns = hasFormat || !wantRelative;
    if (!plainPatterns && dateStyle != 0) {
        udateStyle |= 0x80;
    }
    int32_t status = 0;
    void *icuFormatter =
        udat_open(utimeStyle, udateStyle, localeName, timeZoneID, (int32_t)timeZoneLength, NULL, 0, &status);
    if (icuFormatter == NULL || status > 0) {
        objc_autoreleasePoolPop(pool);
        return;
    }
    if (wantRelative) {
        udat_setContext(icuFormatter, 0x103, &status);
    }
    udat_setLenient(icuFormatter, formatter->isLenient != NULL && trueValue == formatter->isLenient);
    if (formatter->dateStyle == 0 && formatter->timeStyle == 0) {
        udat_applyPattern(icuFormatter, false, NULL, 0);
    }
    if (!wantRelative && formatter->hasCustomFormat == trueValue) {
        OCFIndex length = OCFStringGetLength(formatter->format);
        size_t bytes = (size_t)length << 1;
        UniChar *buffer;
        if (bytes >= 0x101) {
            buffer = malloc_type_malloc(bytes, kUniCharBufferTypeID);
        } else if (length >= 1) {
            buffer = __builtin_alloca(bytes);
            bzero(buffer, bytes);
        } else {
            buffer = NULL;
        }
        const UniChar *characters = OCFStringGetCharactersPtr(formatter->format);
        if (characters == NULL) {
            OCFStringGetCharacters(formatter->format, (OCFRange){0, length}, buffer);
            characters = buffer;
        }
        udat_applyPattern(icuFormatter, false, characters, (int32_t)length);
        if (bytes >= 0x101) {
            free(buffer);
        }
    }
    OCFStringRef calendarIdentifierKey = kOCFLocaleCalendarIdentifierKey;
    OCFTypeRef localeCalendar = OCFLocaleGetValue(formatter->locale, calendarIdentifierKey);
    if (localeCalendar != NULL && OCFEqual(localeCalendar, kOCFGregorianCalendar)) {
        status = 0;
        udat_set2DigitYearStart(icuFormatter, -631152000000.0, &status);
    }
    formatter->icuFormatter = icuFormatter;
    OCFTypeRef preference = _CFLocaleGetAppleICUDateTimeSymbolsPref(formatter->locale);
    if (preference != NULL && OCFGetTypeID(preference) == OCFDictionaryGetTypeID()) {
        OCFDictionaryApplyFunction(preference, __CFDateFormatterStoreSymbolPrefs, formatter);
    }
    preference = _CFLocaleGetAppleFirstWeekdayPref(formatter->locale);
    if (preference != NULL && OCFGetTypeID(preference) == OCFDictionaryGetTypeID()) {
        if (formatter->customFirstWeekday != NULL) {
            OCFRelease(formatter->customFirstWeekday);
        }
        formatter->customFirstWeekday = OCFRetain(preference);
    }
    preference = _CFLocaleGetAppleMinDaysInFirstWeekPref(formatter->locale);
    if (preference != NULL && OCFGetTypeID(preference) == OCFDictionaryGetTypeID()) {
        if (formatter->customMinDaysInFirstWeek != NULL) {
            OCFRelease(formatter->customMinDaysInFirstWeek);
        }
        formatter->customMinDaysInFirstWeek = OCFRetain(preference);
    }
    preference = _CFLocaleGetAppleICUForce24HourTimePref(formatter->locale);
    if (preference != NULL && OCFGetTypeID(preference) == OCFBooleanGetTypeID()) {
        if (formatter->force24HourTime != NULL) {
            OCFRelease(formatter->force24HourTime);
        }
        formatter->force24HourTime = OCFRetain(preference);
    }
    preference = _CFLocaleGetAppleICUForce12HourTimePref(formatter->locale);
    if (preference != NULL && OCFGetTypeID(preference) == OCFBooleanGetTypeID()) {
        if (formatter->force12HourTime != NULL) {
            OCFRelease(formatter->force12HourTime);
        }
        formatter->force12HourTime = OCFRetain(preference);
    }
    preference = _CFLocaleGetAppleICUDateFormatStringsPref(formatter->locale);
    if (preference != NULL && OCFGetTypeID(preference) == OCFDictionaryGetTypeID()) {
        uint64_t index = (uint64_t)formatter->dateStyle - 1;
        OCFStringRef styleKey = index <= 3 ? formatStringsStyleKeys[index] : OF_STR("0");
        OCFTypeRef value = OCFDictionaryGetValue(preference, styleKey);
        if (value != NULL && OCFGetTypeID(value) == OCFStringGetTypeID()) {
            if (formatter->customDateFormat != NULL) {
                OCFRelease(formatter->customDateFormat);
            }
            formatter->customDateFormat = OCFRetain(value);
        }
    }
    preference = _CFLocaleGetAppleICUTimeFormatStringsPref(formatter->locale);
    if (preference != NULL && OCFGetTypeID(preference) == OCFDictionaryGetTypeID()) {
        uint64_t index = (uint64_t)formatter->timeStyle - 1;
        OCFStringRef styleKey = index <= 3 ? formatStringsStyleKeys[index] : OF_STR("0");
        OCFTypeRef value = OCFDictionaryGetValue(preference, styleKey);
        if (value != NULL && OCFGetTypeID(value) == OCFStringGetTypeID()) {
            if (formatter->customTimeFormat != NULL) {
                OCFRelease(formatter->customTimeFormat);
            }
            formatter->customTimeFormat = OCFRetain(value);
        }
    }
    OCFTypeRef calendarIdentifier = formatter->calendarName;
    if (calendarIdentifier == NULL) {
        calendarIdentifier = OCFLocaleGetValue(formatter->locale, calendarIdentifierKey);
    }
    int32_t calendarStatus = 0;
    const void *formatCalendar = udat_getCalendar(formatter->icuFormatter);
    void *calendar = NULL;
    if (formatter->calendar != NULL || formatter->calendarName != NULL) {
        calendar =
            __CFCalendarCreateUCalendar(NULL, OCFLocaleGetIdentifier(formatter->locale), formatter->timeZone);
    }
    if (calendar == NULL) {
        calendar = ucal_clone(formatCalendar, &calendarStatus);
    }
    if (formatter->isLenient != NULL) {
        calendarStatus = 0;
        ucal_setAttribute(calendar, 0, trueValue == formatter->isLenient);
    }
    UniChar patternBuffer[768];
    if (formatter->timeZone != NULL) {
        calendarStatus = 0;
        OCFStringRef name = OCFTimeZoneGetName(formatter->timeZone);
        OCFIndex length = OCFStringGetLength(name);
        OCFIndex count = length < 0x300 ? length : 0x300;
        OCFStringGetCharacters(name, (OCFRange){0, count}, patternBuffer);
        ucal_setTimeZone(calendar, patternBuffer, (int32_t)count, &calendarStatus);
    }
    if (formatter->gregorianStartDate != NULL) {
        calendarStatus = 0;
        OCFAbsoluteTime at = OCFDateGetAbsoluteTime(formatter->gregorianStartDate);
        ucal_setGregorianChange(calendar, (at + kOCFAbsoluteTimeIntervalSince1970) * 1000.0, &calendarStatus);
    } else if (calendarIdentifier != NULL && OCFEqual(calendarIdentifier, kOCFGregorianCalendar)) {
        calendarStatus = 0;
        double change = ucal_getGregorianChange(formatCalendar, &calendarStatus);
        double since1970 = kOCFAbsoluteTimeIntervalSince1970;
        double date;
        if (calendarStatus > 0) {
            // 1582-10-15 as an absolute time (qword_1C0373568); FADD D0, D1, D0.
            date = since1970 + -13197600000.0;
        } else {
            OCFAbsoluteTime at = change / 1000.0 - since1970;
            date = since1970 + at;
        }
        calendarStatus = 0;
        ucal_setGregorianChange(calendar, date * 1000.0, &calendarStatus);
    }
    if (formatter->calendar != NULL) {
        ucal_setAttribute(calendar, 1, (int32_t)OCFCalendarGetFirstWeekday(formatter->calendar));
    } else if (formatter->customFirstWeekday != NULL) {
        OCFTypeRef value = OCFDictionaryGetValue(formatter->customFirstWeekday, calendarIdentifier);
        if (value != NULL && OCFGetTypeID(value) == OCFNumberGetTypeID()) {
            OCFIndex weekday = 0;
            if (OCFNumberGetValue(value, kOCFNumberCFIndexType, &weekday)) {
                calendarStatus = 0;
                ucal_setAttribute(calendar, 1, (int32_t)weekday);
            }
        }
    }
    if (formatter->calendar != NULL) {
        ucal_setAttribute(calendar, 2, (int32_t)OCFCalendarGetMinimumDaysInFirstWeek(formatter->calendar));
    } else if (formatter->customMinDaysInFirstWeek != NULL) {
        OCFTypeRef value = OCFDictionaryGetValue(formatter->customMinDaysInFirstWeek, calendarIdentifier);
        if (value != NULL && OCFGetTypeID(value) == OCFNumberGetTypeID()) {
            OCFIndex minimumDays = 0;
            if (OCFNumberGetValue(value, kOCFNumberCFIndexType, &minimumDays)) {
                calendarStatus = 0;
                ucal_setAttribute(calendar, 2, (int32_t)minimumDays);
            }
        }
    }
    udat_setCalendar(formatter->icuFormatter, calendar);
    ucal_close(calendar);
    UniChar timeBuffer[768];
    UniChar patternResult[768];
    if (plainPatterns) {
        __substituteFormatStringFromPrefsDF(formatter, false);
        __substituteFormatStringFromPrefsDF(formatter, true);
        __ApplyUDateFormatSymbol(formatter);
    } else {
        if (formatter->dateStyle == 0) {
            __substituteFormatStringFromPrefsDF(formatter, false);
            __substituteFormatStringFromPrefsDF(formatter, true);
        } else {
            OCFIndex dateLength = -1;
            Boolean haveDate = false;
            if (formatter->customDateFormat != NULL) {
                if (OCFStringGetLength(formatter->customDateFormat) > 0x2ff) {
                    dateLength = 0x300;
                } else {
                    dateLength = OCFStringGetLength(formatter->customDateFormat);
                }
                OCFStringGetCharacters(formatter->customDateFormat, (OCFRange){0, dateLength}, patternBuffer);
                haveDate = dateLength != -1;
            }
            if (!haveDate) {
                calendarStatus = 0;
                int32_t length = udat_toPatternRelativeDate(
                    formatter->icuFormatter, patternBuffer, 0x300, &calendarStatus
                );
                dateLength = (OCFIndex)(calendarStatus > 0 ? -1 : length);
            }
            OCFIndex timeLength = -1;
            Boolean haveTime = false;
            if (formatter->timeStyle != 0 && formatter->customTimeFormat != NULL) {
                if (OCFStringGetLength(formatter->customTimeFormat) > 0x2ff) {
                    timeLength = 0x300;
                } else {
                    timeLength = OCFStringGetLength(formatter->customTimeFormat);
                }
                OCFStringGetCharacters(formatter->customTimeFormat, (OCFRange){0, timeLength}, timeBuffer);
                haveTime = timeLength != -1;
            }
            int32_t relativeStatus;
            if (!haveTime) {
                relativeStatus = 0;
                int32_t length =
                    udat_toPatternRelativeTime(formatter->icuFormatter, timeBuffer, 0x300, &relativeStatus);
                timeLength = (OCFIndex)(relativeStatus > 0 ? -1 : length);
            }
            relativeStatus = 0;
            udat_applyPatternRelative(
                formatter->icuFormatter,
                dateLength < 0 ? NULL : patternBuffer,
                dateLength < 0 ? 0 : (int32_t)dateLength,
                timeLength < 0 ? NULL : timeBuffer,
                timeLength < 0 ? 0 : (int32_t)timeLength,
                &relativeStatus
            );
        }
        __ApplyUDateFormatSymbol(formatter);
        if (formatter->dateStyle != 0) {
            status = 0;
            int32_t dateLength = udat_toPatternRelativeDate(icuFormatter, patternBuffer, 0x300, &status);
            OCFIndex timeLength;
            if (timeStyleIsNone) {
                timeLength = 0;
            } else {
                timeLength = (OCFIndex)udat_toPatternRelativeTime(icuFormatter, timeBuffer, 0x300, &status);
            }
            if (status > 0 || dateLength > 0x300 || timeLength > 0x300) {
                goto properties;
            }
            OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
            OCFStringRef timeString = OCFStringCreateWithCharacters(systemDefault, timeBuffer, timeLength);
            OCFStringRef forced = __CFDateFormatterCreateForcedString(formatter, timeString);
            OCFIndex forcedLength = OCFStringGetLength(forced);
            if (forcedLength <= 0x300) {
                OCFStringGetCharacters(forced, (OCFRange){0, forcedLength}, timeBuffer);
                status = 0;
                udat_applyPatternRelative(
                    icuFormatter, patternBuffer, dateLength, timeBuffer, (int32_t)forcedLength, &status
                );
                status = 0;
                int32_t length = udat_toPattern(icuFormatter, false, patternResult, 0x300, &status);
                if (status <= 0 && length <= 0x300) {
                    if (formatter->format != NULL) {
                        OCFRelease(formatter->format);
                    }
                    formatter->format =
                        OCFStringCreateWithCharacters(systemDefault, patternResult, (OCFIndex)length);
                }
            }
            OCFRelease(forced);
            OCFRelease(timeString);
            goto properties;
        }
    }
    status = 0;
    int32_t length = udat_toPattern(icuFormatter, false, patternBuffer, 0x300, &status);
    if (status <= 0 && length <= 0x300) {
        OCFStringRef pattern =
            OCFStringCreateWithCharacters(kOCFAllocatorSystemDefault, patternBuffer, (OCFIndex)length);
        OCFStringRef forced = __CFDateFormatterCreateForcedString(formatter, pattern);
        OCFIndex forcedLength = OCFStringGetLength(forced);
        if (forcedLength <= 0x400) {
            size_t bytes = (size_t)forcedLength << 1;
            UniChar *buffer;
            if (bytes >= 0x101) {
                buffer = malloc_type_malloc(bytes, kUniCharBufferTypeID);
            } else if (forcedLength >= 1) {
                buffer = __builtin_alloca(bytes);
                bzero(buffer, bytes);
            } else {
                buffer = NULL;
            }
            const UniChar *characters = OCFStringGetCharactersPtr(forced);
            if (characters == NULL) {
                OCFStringGetCharacters(forced, (OCFRange){0, forcedLength}, buffer);
                characters = buffer;
            }
            udat_applyPattern(formatter->icuFormatter, false, characters, (int32_t)forcedLength);
            if (formatter->format != NULL) {
                OCFRelease(formatter->format);
            }
            formatter->format = OCFStringCreateCopy(OCFGetAllocator(formatter), forced);
            if (bytes >= 0x101) {
                free(buffer);
            }
        }
        OCFRelease(forced);
        OCFRelease(pattern);
    }
properties:
    if (formatter->defaultFormat != NULL) {
        OCFRelease(formatter->defaultFormat);
    }
    formatter->defaultFormat = formatter->format != NULL ? OCFRetain(formatter->format) : NULL;
    if (formatter->isLenient != NULL) {
        __CFDateFormatterSetProperty(formatter, kOCFDateFormatterIsLenientKey, formatter->isLenient, true);
    }
    if (formatter->doesRelativeDateFormatting != NULL) {
        __CFDateFormatterSetProperty(
            formatter,
            kOCFDateFormatterDoesRelativeDateFormattingKey,
            formatter->doesRelativeDateFormatting,
            true
        );
    }
    if (formatter->calendar != NULL) {
        __CFDateFormatterSetProperty(formatter, kOCFDateFormatterCalendarKey, formatter->calendar, true);
    }
    if (formatter->calendarName != NULL) {
        __CFDateFormatterSetProperty(formatter, kOCFDateFormatterCalendarName, formatter->calendarName, true);
    }
    if (formatter->timeZone != NULL) {
        __CFDateFormatterSetProperty(formatter, kOCFDateFormatterTimeZoneKey, formatter->timeZone, true);
    }
    if (formatter->twoDigitStartDate != NULL) {
        __CFDateFormatterSetProperty(
            formatter, kOCFDateFormatterTwoDigitStartDateKey, formatter->twoDigitStartDate, true
        );
    }
    if (formatter->defaultDate != NULL) {
        __CFDateFormatterSetProperty(
            formatter, kOCFDateFormatterDefaultDateKey, formatter->defaultDate, true
        );
    }
    if (formatter->gregorianStartDate != NULL) {
        __CFDateFormatterSetProperty(
            formatter, kOCFDateFormatterGregorianStartDateKey, formatter->gregorianStartDate, true
        );
    }
    if (formatter->ambiguousYearStrategy != NULL) {
        __CFDateFormatterSetProperty(
            formatter, kOCFDateFormatterAmbiguousYearStrategyKey, formatter->ambiguousYearStrategy, true
        );
    }
    if (formatter->usesCharacterDirection != NULL) {
        __CFDateFormatterSetProperty(
            formatter, kOCFDateFormatterUsesCharacterDirectionKey, formatter->usesCharacterDirection, true
        );
    }
    if (formatter->formattingContext != NULL) {
        __CFDateFormatterSetProperty(
            formatter, OF_STR("kCFDateFormatterFormattingContextKey"), formatter->formattingContext, true
        );
    }
    objc_autoreleasePoolPop(pool);
}

// Native CFDateFormatterCreateStringWithDate, 0x1c01911c4. External. A NULL allocator is the thread default
// (TSD slot 1) or the system default; type 56 is asserted before the date is read.
OCFStringRef OCFDateFormatterCreateStringWithDate(
    OCFAllocatorRef allocator, OCFDateFormatterRef formatter, OCFDateRef date
) {
    if (allocator == NULL) {
        allocator = _OCFGetTSD(1);
        if (allocator == NULL) {
            allocator = kOCFAllocatorSystemDefault;
        }
    }
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != kDateFormatterTypeID) {
        _CFAssertMismatchedTypeID(kDateFormatterTypeID, typeID);
    }
    return OCFDateFormatterCreateStringWithAbsoluteTime(allocator, formatter, OCFDateGetAbsoluteTime(date));
}

// Native CFDateFormatterCreateStringWithAbsoluteTime, 0x1c019123c. External.
// A NULL allocator is the thread default (TSD slot 1) or the system default; type 56 is asserted. The UDate
// is fma(at + kCFAbsoluteTimeIntervalSince1970, 1000.0, 0.5) (FMADD). udat_format writes from the second slot
// of a 769-character stack buffer (capacity 768); on U_BUFFER_OVERFLOW_ERROR or a length above 768 a buffer
// of length + 2 characters (CFAllocatorAllocateTyped, system default) is formatted again from its second slot
// with capacity length + 2, as natively (ICU writes length characters and a terminator, which fit). A failure
// answers NULL. When the character-direction property is kCFBooleanTrue and the locale's language is
// right-to-left, U+200F is stored in the first slot and counted.
OCFStringRef OCFDateFormatterCreateStringWithAbsoluteTime(
    OCFAllocatorRef allocator, OCFDateFormatterRef formatter, OCFAbsoluteTime at
) {
    if (allocator == NULL) {
        allocator = _OCFGetTSD(1);
        if (allocator == NULL) {
            allocator = kOCFAllocatorSystemDefault;
        }
    }
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != kDateFormatterTypeID) {
        _CFAssertMismatchedTypeID(kDateFormatterTypeID, typeID);
    }
    int32_t status = 0;
    double udate = fma(at + kOCFAbsoluteTimeIntervalSince1970, 1000.0, 0.5);
    UniChar buffer[1 + 768];
    OCFIndex length = udat_format(formatter->icuFormatter, udate, buffer + 1, 0x300, NULL, &status);
    UniChar *large = NULL;
    if (status == kICUBufferOverflowError || length >= 0x301) {
        length += 2;
        large = OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, length << 1, kUniCharBufferTypeID, 0);
        status = 0;
        length = udat_format(formatter->icuFormatter, udate, large + 1, (int32_t)length, NULL, &status);
    }
    OCFStringRef string;
    if (status > 0) {
        string = NULL;
    } else {
        UniChar *base = large != NULL ? large : buffer;
        const UniChar *characters;
        if (formatter->usesCharacterDirection == kOCFBooleanTrue &&
            OCFLocaleGetLanguageCharacterDirection(OCFLocaleGetIdentifier(formatter->locale)) ==
                kOCFLocaleLanguageDirectionRightToLeft) {
            base[0] = 0x200F;
            length++;
            characters = base;
        } else {
            characters = large != NULL ? large + 1 : buffer + 1;
        }
        string = OCFStringCreateWithCharacters(allocator, characters, length);
    }
    if (large != NULL) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, large);
    }
    return string;
}

// Native CFDateFormatterCopyProperty, 0x1c01920a4. External. Type 56 is asserted.
// The keys are compared by pointer in native order, the formatting-context key last with CFEqual against the
// literal "kCFDateFormatterFormattingContextKey". A stored property is retained and answered. Without one:
// lenience asks ICU (kCFBooleanTrue/kCFBooleanFalse); relative formatting and character direction answer
// kCFBooleanFalse; the calendar and calendar identifier answer the locale's kCFLocaleCalendarKey and
// kCFLocaleCalendarIdentifierKey values (retained when not NULL); the Gregorian start date is ICU's change
// converted with FDIV by 1000.0 and FSUB of kCFAbsoluteTimeIntervalSince1970 (NULL on failure); the symbol
// arrays are copied from ICU; the AM and PM symbols are ICU's symbols 0 and 1 of type 5 when there are at
// least two (the native code compares the symbol count, not the length, with 768: kept); the formatting
// context is ICU's capitalization context (type 1) as a kCFNumberIntType number. The status variable is
// initialized once and shared. Other keys answer NULL.
OCFTypeRef OCFDateFormatterCopyProperty(OCFDateFormatterRef formatter, OCFDateFormatterKey key) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != kDateFormatterTypeID) {
        _CFAssertMismatchedTypeID(kDateFormatterTypeID, typeID);
    }
    int32_t status = 0;
    OCFTypeRef value;
    int32_t symbolType;
    int32_t symbolIndex;
    UniChar buffer[768];
    if (kOCFDateFormatterIsLenientKey == key) {
        if (formatter->isLenient != NULL) {
            return OCFRetain(formatter->isLenient);
        }
        return OCFRetain(udat_isLenient(formatter->icuFormatter) == 0 ? kOCFBooleanFalse : kOCFBooleanTrue);
    }
    if (kOCFDateFormatterDoesRelativeDateFormattingKey == key) {
        if (formatter->doesRelativeDateFormatting != NULL) {
            return OCFRetain(formatter->doesRelativeDateFormatting);
        }
        return OCFRetain(kOCFBooleanFalse);
    }
    if (kOCFDateFormatterCalendarKey == key) {
        if (formatter->calendar != NULL) {
            return OCFRetain(formatter->calendar);
        }
        value = OCFLocaleGetValue(formatter->locale, kOCFLocaleCalendarKey);
        return value != NULL ? OCFRetain(value) : NULL;
    }
    if (kOCFDateFormatterCalendarName == key) {
        if (formatter->calendarName != NULL) {
            return OCFRetain(formatter->calendarName);
        }
        value = OCFLocaleGetValue(formatter->locale, kOCFLocaleCalendarIdentifierKey);
        return value != NULL ? OCFRetain(value) : NULL;
    }
    if (kOCFDateFormatterTimeZoneKey == key) {
        return formatter->timeZone != NULL ? OCFRetain(formatter->timeZone) : NULL;
    }
    if (kOCFDateFormatterDefaultFormatKey == key) {
        return formatter->defaultFormat != NULL ? OCFRetain(formatter->defaultFormat) : NULL;
    }
    if (kOCFDateFormatterTwoDigitStartDateKey == key) {
        return formatter->twoDigitStartDate != NULL ? OCFRetain(formatter->twoDigitStartDate) : NULL;
    }
    if (kOCFDateFormatterDefaultDateKey == key) {
        return formatter->defaultDate != NULL ? OCFRetain(formatter->defaultDate) : NULL;
    }
    if (kOCFDateFormatterGregorianStartDateKey == key) {
        if (formatter->gregorianStartDate != NULL) {
            return OCFRetain(formatter->gregorianStartDate);
        }
        double change = ucal_getGregorianChange(udat_getCalendar(formatter->icuFormatter), &status);
        if (status > 0) {
            return NULL;
        }
        OCFAbsoluteTime at = change / 1000.0 - kOCFAbsoluteTimeIntervalSince1970;
        return OCFDateCreate(OCFGetAllocator(formatter), at);
    }
    if (kOCFDateFormatterEraSymbolsKey == key) {
        value = formatter->symbols[0];
        symbolType = 0;
        symbolIndex = 0;
    } else if (kOCFDateFormatterLongEraSymbolsKey == key) {
        value = formatter->symbols[1];
        symbolType = 7;
        symbolIndex = 0;
    } else if (kOCFDateFormatterMonthSymbolsKey == key) {
        value = formatter->symbols[2];
        symbolType = 1;
        symbolIndex = 0;
    } else if (kOCFDateFormatterShortMonthSymbolsKey == key) {
        value = formatter->symbols[3];
        symbolType = 2;
        symbolIndex = 0;
    } else if (kOCFDateFormatterVeryShortMonthSymbolsKey == key) {
        value = formatter->symbols[4];
        symbolType = 8;
        symbolIndex = 0;
    } else if (kOCFDateFormatterStandaloneMonthSymbolsKey == key) {
        value = formatter->symbols[5];
        symbolType = 10;
        symbolIndex = 0;
    } else if (kOCFDateFormatterShortStandaloneMonthSymbolsKey == key) {
        value = formatter->symbols[6];
        symbolType = 11;
        symbolIndex = 0;
    } else if (kOCFDateFormatterVeryShortStandaloneMonthSymbolsKey == key) {
        value = formatter->symbols[7];
        symbolType = 12;
        symbolIndex = 0;
    } else if (kOCFDateFormatterWeekdaySymbolsKey == key) {
        value = formatter->symbols[8];
        symbolType = 3;
        symbolIndex = 1;
    } else if (kOCFDateFormatterShortWeekdaySymbolsKey == key) {
        value = formatter->symbols[9];
        symbolType = 4;
        symbolIndex = 1;
    } else if (kOCFDateFormatterVeryShortWeekdaySymbolsKey == key) {
        value = formatter->symbols[10];
        symbolType = 9;
        symbolIndex = 1;
    } else if (kOCFDateFormatterStandaloneWeekdaySymbolsKey == key) {
        value = formatter->symbols[11];
        symbolType = 13;
        symbolIndex = 1;
    } else if (kOCFDateFormatterShortStandaloneWeekdaySymbolsKey == key) {
        value = formatter->symbols[12];
        symbolType = 14;
        symbolIndex = 1;
    } else if (kOCFDateFormatterVeryShortStandaloneWeekdaySymbolsKey == key) {
        value = formatter->symbols[13];
        symbolType = 15;
        symbolIndex = 1;
    } else if (kOCFDateFormatterQuarterSymbolsKey == key) {
        value = formatter->symbols[14];
        symbolType = 16;
        symbolIndex = 0;
    } else if (kOCFDateFormatterShortQuarterSymbolsKey == key) {
        value = formatter->symbols[15];
        symbolType = 17;
        symbolIndex = 0;
    } else if (kOCFDateFormatterStandaloneQuarterSymbolsKey == key) {
        value = formatter->symbols[16];
        symbolType = 18;
        symbolIndex = 0;
    } else if (kOCFDateFormatterShortStandaloneQuarterSymbolsKey == key) {
        value = formatter->symbols[17];
        symbolType = 19;
        symbolIndex = 0;
    } else if (kOCFDateFormatterAMSymbolKey == key || kOCFDateFormatterPMSymbolKey == key) {
        Boolean isAM = kOCFDateFormatterAMSymbolKey == key;
        value = isAM ? formatter->amSymbol : formatter->pmSymbol;
        if (value != NULL) {
            return OCFRetain(value);
        }
        int32_t count = udat_countSymbols(formatter->icuFormatter, kICUAMPMSymbols);
        if (count < 2) {
            return NULL;
        }
        int32_t length =
            udat_getSymbols(formatter->icuFormatter, kICUAMPMSymbols, isAM ? 0 : 1, buffer, 0x300, &status);
        if ((uint32_t)count > 0x300 || status > 0) {
            return NULL;
        }
        return OCFStringCreateWithCharacters(OCFGetAllocator(formatter), buffer, (OCFIndex)length);
    } else if (kOCFDateFormatterAmbiguousYearStrategyKey == key) {
        return formatter->ambiguousYearStrategy != NULL ? OCFRetain(formatter->ambiguousYearStrategy) : NULL;
    } else if (kOCFDateFormatterUsesCharacterDirectionKey == key) {
        if (formatter->usesCharacterDirection != NULL) {
            return OCFRetain(formatter->usesCharacterDirection);
        }
        return OCFRetain(kOCFBooleanFalse);
    } else if (OCFEqual(key, OF_STR("kCFDateFormatterFormattingContextKey"))) {
        if (formatter->formattingContext != NULL) {
            return OCFRetain(formatter->formattingContext);
        }
        int32_t context = udat_getContext(formatter->icuFormatter, 1, &status);
        return OCFNumberCreate(OCFGetAllocator(formatter), kOCFNumberIntType, &context);
    } else {
        return NULL;
    }
    if (value != NULL) {
        return OCFRetain(value);
    }
    return __CFDateFormatterCopySymbolsArray(formatter->icuFormatter, symbolType, symbolIndex);
}

// Native CFDateFormatterSetProperty, 0x1c0192934. External. Type 56 is asserted; a tail call of
// __CFDateFormatterSetProperty with directToICU false.
void OCFDateFormatterSetProperty(OCFDateFormatterRef formatter, OCFStringRef key, OCFTypeRef value) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(formatter);
    if (typeID != kDateFormatterTypeID) {
        _CFAssertMismatchedTypeID(kDateFormatterTypeID, typeID);
    }
    __CFDateFormatterSetProperty(formatter, key, value, false);
}

// The ICU symbol types and index bases of the eighteen symbol arrays in field order (the literals at each
// case of CFDateFormatterCopyProperty and __CFDateFormatterSetProperty; the types are also the table
// unk_1C03735B4 that __ApplyUDateFormatSymbol reads).
static const int32_t symbolTypes[18] = {0, 7, 1, 2, 8, 10, 11, 12, 3, 4, 9, 13, 14, 15, 16, 17, 18, 19};
static const int32_t symbolBases[18] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};

// Native __CFDateFormatterSetProperty, 0x1c0192988. Non-external.
// The keys are compared by pointer in native order; the formatting-context key last with CFEqual. Unless a
// case says otherwise, a stored-property case returns at once when directToICU is set, else replaces the
// stored value (the old one is cleared, then released after the case) and resets the ICU format:
// - lenience: the value retained (NULL allowed), reset;
// - relative formatting: kCFBooleanTrue when the value is kCFBooleanTrue, else kCFBooleanFalse, reset;
// - calendar: the locale becomes _CFLocaleCreateCopyWithNewCalendarIdentifier (the locale's allocator, the
//   calendar's identifier), also when directToICU (the stored calendar is then untouched); else the stored
//   calendar is CFDateFormatterCopyProperty's, reset;
// - calendar identifier: the locale is rebuilt from its components with kCFLocaleCalendarIdentifierKey set to
//   the value (system default allocator; CFLocaleCreate with the locale's allocator), also when directToICU;
//   else the stored identifier is CFDateFormatterCopyProperty's, reset;
// - time zone: nothing when the value is the stored one; the stored value becomes the value retained or
//   CFTimeZoneCopyDefault(); when directToICU the old one is released at once and nothing else happens; else
//   the stored value becomes CFDateFormatterCopyProperty's, the format is reset and that intermediate value
//   released, then the old one;
// - default format: nothing;
// - two-digit start and default date: the value retained, no reset; Gregorian start date: the same, reset;
// - symbol arrays: when directToICU only ICU receives them (a tail call of __CFDateFormatterSetSymbolsArray);
//   else ICU receives them and the stored array is copied back from ICU, no reset;
// - AM and PM: ICU symbol 0 or 1 of type 5 receives at most 768 characters (or the whole string from
//   CFStringGetCharactersPtr), also when directToICU; else the stored value is a copy (NULL allocator), no
//   reset;
// - ambiguous-year strategy: replaced even when directToICU, the value retained without a NULL check;
//   character direction: the same without clearing first; neither resets;
// - formatting context: the value's kCFNumberIntType value is the ICU capitalization context, also when
//   directToICU; else the value is retained and stored; no reset.
// Every other key does nothing. The ICU status variable is initialized once and shared.
static void __CFDateFormatterSetProperty(
    OCFDateFormatterRef formatter, OCFStringRef key, OCFTypeRef value, Boolean directToICU
) {
    int32_t status = 0;
    OCFTypeRef old = NULL;
    Boolean reset = false;
    if (kOCFDateFormatterIsLenientKey == key) {
        if (directToICU) {
            return;
        }
        old = formatter->isLenient;
        formatter->isLenient = NULL;
        formatter->isLenient = value != NULL ? OCFRetain(value) : NULL;
        reset = true;
    } else if (kOCFDateFormatterDoesRelativeDateFormattingKey == key) {
        if (directToICU) {
            return;
        }
        old = formatter->doesRelativeDateFormatting;
        formatter->doesRelativeDateFormatting = NULL;
        OCFTypeRef relative = kOCFBooleanTrue == value ? value : kOCFBooleanFalse;
        formatter->doesRelativeDateFormatting = relative != NULL ? OCFRetain(relative) : NULL;
        reset = true;
    } else if (kOCFDateFormatterCalendarKey == key) {
        OCFStringRef calendarKey = kOCFDateFormatterCalendarKey;
        if (!directToICU) {
            old = formatter->calendar;
            formatter->calendar = NULL;
        }
        OCFAllocatorRef allocator = OCFGetAllocator(formatter->locale);
        OCFLocaleRef locale = _CFLocaleCreateCopyWithNewCalendarIdentifier(
            allocator, formatter->locale, OCFCalendarGetIdentifier((OCFCalendarRef)value)
        );
        if (formatter->locale != NULL) {
            OCFRelease(formatter->locale);
        }
        formatter->locale = locale;
        if (!directToICU) {
            formatter->calendar = (OCFCalendarRef)OCFDateFormatterCopyProperty(formatter, calendarKey);
            reset = true;
        }
    } else if (kOCFDateFormatterCalendarName == key) {
        OCFStringRef calendarNameKey = kOCFDateFormatterCalendarName;
        if (!directToICU) {
            old = formatter->calendarName;
            formatter->calendarName = NULL;
        }
        OCFStringRef identifier = OCFLocaleGetIdentifier(formatter->locale);
        OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
        OCFDictionaryRef components =
            OCFLocaleCreateComponentsFromLocaleIdentifier(systemDefault, identifier);
        OCFMutableDictionaryRef mutableComponents =
            OCFDictionaryCreateMutableCopy(systemDefault, 0, components);
        OCFDictionarySetValue(mutableComponents, kOCFLocaleCalendarIdentifierKey, value);
        OCFStringRef newIdentifier =
            OCFLocaleCreateLocaleIdentifierFromComponents(systemDefault, mutableComponents);
        OCFRelease(mutableComponents);
        OCFRelease(components);
        OCFLocaleRef locale = OCFLocaleCreate(OCFGetAllocator(formatter->locale), newIdentifier);
        OCFRelease(newIdentifier);
        OCFRelease(formatter->locale);
        formatter->locale = locale;
        if (!directToICU) {
            formatter->calendarName = OCFDateFormatterCopyProperty(formatter, calendarNameKey);
            reset = true;
        }
    } else if (kOCFDateFormatterTimeZoneKey == key) {
        OCFStringRef timeZoneKey = kOCFDateFormatterTimeZoneKey;
        OCFTimeZoneRef previous = formatter->timeZone;
        if (previous == value) {
            return;
        }
        OCFTimeZoneRef releaseNow;
        if (directToICU) {
            releaseNow = previous;
        } else {
            old = previous;
            releaseNow = NULL;
            formatter->timeZone = NULL;
        }
        formatter->timeZone = value != NULL ? OCFRetain(value) : OCFTimeZoneCopyDefault();
        if (releaseNow != NULL) {
            OCFRelease(releaseNow);
        }
        if (!directToICU) {
            OCFTypeRef intermediate = formatter->timeZone;
            formatter->timeZone = OCFDateFormatterCopyProperty(formatter, timeZoneKey);
            __ResetUDateFormat(formatter, false);
            if (intermediate != NULL) {
                OCFRelease(intermediate);
            }
        }
    } else if (kOCFDateFormatterDefaultFormatKey == key) {
        return;
    } else if (kOCFDateFormatterTwoDigitStartDateKey == key) {
        if (directToICU) {
            return;
        }
        old = formatter->twoDigitStartDate;
        formatter->twoDigitStartDate = NULL;
        formatter->twoDigitStartDate = value != NULL ? OCFRetain(value) : NULL;
    } else if (kOCFDateFormatterDefaultDateKey == key) {
        if (directToICU) {
            return;
        }
        old = formatter->defaultDate;
        formatter->defaultDate = NULL;
        formatter->defaultDate = value != NULL ? OCFRetain(value) : NULL;
    } else if (kOCFDateFormatterGregorianStartDateKey == key) {
        if (directToICU) {
            return;
        }
        old = formatter->gregorianStartDate;
        formatter->gregorianStartDate = NULL;
        formatter->gregorianStartDate = value != NULL ? OCFRetain(value) : NULL;
        reset = true;
    } else {
        const OCFStringRef symbolKeys[18] = {
            kOCFDateFormatterEraSymbolsKey,
            kOCFDateFormatterLongEraSymbolsKey,
            kOCFDateFormatterMonthSymbolsKey,
            kOCFDateFormatterShortMonthSymbolsKey,
            kOCFDateFormatterVeryShortMonthSymbolsKey,
            kOCFDateFormatterStandaloneMonthSymbolsKey,
            kOCFDateFormatterShortStandaloneMonthSymbolsKey,
            kOCFDateFormatterVeryShortStandaloneMonthSymbolsKey,
            kOCFDateFormatterWeekdaySymbolsKey,
            kOCFDateFormatterShortWeekdaySymbolsKey,
            kOCFDateFormatterVeryShortWeekdaySymbolsKey,
            kOCFDateFormatterStandaloneWeekdaySymbolsKey,
            kOCFDateFormatterShortStandaloneWeekdaySymbolsKey,
            kOCFDateFormatterVeryShortStandaloneWeekdaySymbolsKey,
            kOCFDateFormatterQuarterSymbolsKey,
            kOCFDateFormatterShortQuarterSymbolsKey,
            kOCFDateFormatterStandaloneQuarterSymbolsKey,
            kOCFDateFormatterShortStandaloneQuarterSymbolsKey,
        };
        int symbol = 0;
        while (symbol < 18 && symbolKeys[symbol] != key) {
            symbol++;
        }
        if (symbol < 18) {
            int32_t type = symbolTypes[symbol];
            int32_t base = symbolBases[symbol];
            if (directToICU) {
                __CFDateFormatterSetSymbolsArray(formatter->icuFormatter, type, base, value);
                return;
            }
            old = formatter->symbols[symbol];
            formatter->symbols[symbol] = NULL;
            __CFDateFormatterSetSymbolsArray(formatter->icuFormatter, type, base, value);
            formatter->symbols[symbol] =
                __CFDateFormatterCopySymbolsArray(formatter->icuFormatter, type, base);
        } else if (kOCFDateFormatterAMSymbolKey == key || kOCFDateFormatterPMSymbolKey == key) {
            Boolean isAM = kOCFDateFormatterAMSymbolKey == key;
            if (!directToICU) {
                old = isAM ? formatter->amSymbol : formatter->pmSymbol;
                if (isAM) {
                    formatter->amSymbol = NULL;
                } else {
                    formatter->pmSymbol = NULL;
                }
            }
            OCFIndex length = OCFStringGetLength(value);
            OCFIndex capacity = length < 0x300 ? length : 0x300;
            UniChar buffer[capacity];
            const UniChar *characters = OCFStringGetCharactersPtr(value);
            if (characters == NULL) {
                OCFStringGetCharacters(value, (OCFRange){0, capacity}, buffer);
                characters = buffer;
                length = capacity;
            }
            udat_setSymbols(
                formatter->icuFormatter, kICUAMPMSymbols, isAM ? 0 : 1, characters, (int32_t)length, &status
            );
            if (!directToICU) {
                OCFStringRef copy = value != NULL ? OCFStringCreateCopy(NULL, value) : NULL;
                if (isAM) {
                    formatter->amSymbol = copy;
                } else {
                    formatter->pmSymbol = copy;
                }
            }
        } else if (kOCFDateFormatterAmbiguousYearStrategyKey == key) {
            old = formatter->ambiguousYearStrategy;
            formatter->ambiguousYearStrategy = NULL;
            formatter->ambiguousYearStrategy = OCFRetain(value);
        } else if (kOCFDateFormatterUsesCharacterDirectionKey == key) {
            old = formatter->usesCharacterDirection;
            formatter->usesCharacterDirection = OCFRetain(value);
        } else if (OCFEqual(key, OF_STR("kCFDateFormatterFormattingContextKey"))) {
            if (!directToICU) {
                old = formatter->formattingContext;
                formatter->formattingContext = NULL;
            }
            int32_t context = 0;
            OCFNumberGetValue(value, kOCFNumberIntType, &context);
            udat_setContext(formatter->icuFormatter, context, &status);
            if (!directToICU) {
                formatter->formattingContext = OCFRetain(value);
            }
        } else {
            return;
        }
    }
    if (reset) {
        __ResetUDateFormat(formatter, false);
    }
    if (old != NULL) {
        OCFRelease(old);
    }
}

// Native __CFDateFormatterCopySymbolsArray, 0x1c0193784. Non-external.
// With no symbols from indexBase on (count <= indexBase, signed) an empty array (system default allocator,
// kCFTypeArrayCallBacks) is answered. Otherwise each symbol from indexBase to the count is read into a
// 768-character buffer and becomes a string (system default allocator), or the retained constant "<error>"
// when the ICU status is a failure, the string creation fails or the number of symbols (not the length,
// compared unsigned with 768, as natively) is too large. The value buffer's first min(8 * count, 512) bytes
// are zeroed first. The array is created and the strings released from the last to the first.
static OCFArrayRef __CFDateFormatterCopySymbolsArray(void *icuFormatter, int32_t icuCode, int32_t indexBase) {
    int32_t count = udat_countSymbols(icuFormatter, icuCode);
    if (count <= indexBase) {
        return OCFArrayCreate(kOCFAllocatorSystemDefault, NULL, 0, &kOCFTypeArrayCallBacks);
    }
    uint64_t symbolCount = (uint64_t)(uint32_t)count - (uint64_t)(uint32_t)indexBase;
    uint64_t bytes = symbolCount << 3;
    OCFStringRef *strings = __builtin_alloca((bytes + 15) & 0xFFFFFFFF0ULL);
    bzero(strings, bytes < 0x200 ? bytes : 0x200);
    OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
    OCFStringRef *slot = strings;
    for (uint64_t index = (uint32_t)indexBase; index != (uint32_t)count; index++) {
        int32_t status = 0;
        UniChar buffer[768];
        int32_t length = udat_getSymbols(icuFormatter, icuCode, (int32_t)index, buffer, 0x300, &status);
        OCFStringRef string = NULL;
        if (symbolCount <= 0x300 && status <= 0) {
            string = OCFStringCreateWithCharacters(systemDefault, buffer, (OCFIndex)length);
        }
        if (string == NULL) {
            string = OCFRetain(OF_STR("<error>"));
        }
        *slot++ = string;
    }
    OCFArrayRef array =
        OCFArrayCreate(systemDefault, (const void **)strings, (OCFIndex)symbolCount, &kOCFTypeArrayCallBacks);
    for (uint64_t remaining = symbolCount; remaining != 0; remaining--) {
        OCFRelease(strings[remaining - 1]);
    }
    return array;
}

// The layout of the unit's global block literals and of their shared descriptor (Block ABI; the type names
// and field names are local descriptive spellings): the isa (_NSConcreteGlobalBlock, the libsystem_blocks
// class at 0x266b28168 outside the image), the flags 0x50000000 (BLOCK_IS_GLOBAL | BLOCK_HAS_SIGNATURE) and a
// zero reserved word, the invoke function and the descriptor.
typedef struct {
    uintptr_t reserved;
    uintptr_t size;
    const char *signature;
    const char *layout;
} GlobalBlockDescriptor;

typedef struct {
    void *isa;
    int32_t flags;
    int32_t reserved;
    void (*invoke)(void *block);
    const GlobalBlockDescriptor *descriptor;
} GlobalBlockLiteral;

OF_ABI_ASSERT(sizeof(GlobalBlockLiteral) == 32, "A global block literal is 32 bytes.");

// Native ___block_descriptor_32_e5_v8?0l, 0x22279ce68 (__DATA_CONST,__const, was a private external; read
// with Local/Scratch/cf-core/macho.py): the descriptor both literals share, {0x50000000, 32, "v8@?0"
// (0x1c047d8bb), no layout}. Hidden natively; static here, since only this unit's literals use it.
static const GlobalBlockDescriptor globalBlockDescriptor = {0x50000000, 32, "v8@?0", NULL};

// Native ___useTemplatePatternGenerator_block_invoke, 0x1c0193c8c. Non-external. The work block the
// memory-pressure handler of useTemplatePatternGenerator calls under the lock: the cached generator is
// closed (udatpg_close) and cleared, then its locale name freed and cleared.
static void useTemplatePatternGeneratorResetInvoke(void *block) {
    (void)block;
    udatpg_close(templatePatternGenerator);
    templatePatternGenerator = NULL;
    free(templatePatternGeneratorLocaleName);
    templatePatternGeneratorLocaleName = NULL;
}

// Native ___block_literal_global, 0x22279d7a0 (__DATA_CONST,__const, non-external): the global block whose
// invoke is useTemplatePatternGeneratorResetInvoke. The object is defined under the hidden symbol that
// Locale/OCFDateFormatter.h declares as useTemplatePatternGeneratorResetBlock (an assembler label, so that
// the header's byte-array declaration names this record).
__attribute__((visibility("hidden"))) const GlobalBlockLiteral useTemplatePatternGeneratorResetLiteral __asm(
    OF_SYMBOL("useTemplatePatternGeneratorResetBlock")
) = {_NSConcreteGlobalBlock, 0x50000000, 0, useTemplatePatternGeneratorResetInvoke, &globalBlockDescriptor};

// Native _____CFDateFormatterCreateForcedString_block_invoke, 0x1c0193d20. Non-external. The dispatch_once
// block of __CFDateFormatterCreateForcedString: forcedStringHourCharacters becomes
// CFCharacterSetCreateWithCharactersInString(kCFAllocatorSystemDefault, "hHkK") (the constant string
// 0x2227ae500).
static void forcedStringHourCharactersOnceInvoke(void *block) {
    (void)block;
    forcedStringHourCharacters =
        OCFCharacterSetCreateWithCharactersInString(kOCFAllocatorSystemDefault, OF_STR("hHkK"));
}

// Native ___block_literal_global.31, 0x22279d7e8 (__DATA_CONST,__const, non-external): the global block whose
// invoke is forcedStringHourCharactersOnceInvoke, defined under the hidden symbol that
// Locale/OCFDateFormatter.h declares as forcedStringHourCharactersOnceBlock.
__attribute__((visibility("hidden"))) const GlobalBlockLiteral forcedStringHourCharactersOnceLiteral __asm(
    OF_SYMBOL("forcedStringHourCharactersOnceBlock")
) = {_NSConcreteGlobalBlock, 0x50000000, 0, forcedStringHourCharactersOnceInvoke, &globalBlockDescriptor};

// Native __substituteFormatStringFromPrefsDF, 0x1c0193f84. Non-external.
// For the time (doTime) or date style and its preference format (0x1d8 or 0x1d0): nothing when the style is 0
// or there is no preference format. A temporary ICU format of that style alone (4 - style for styles below 5,
// else -1; the other style -1) for the locale identifier (UTF-8 pointer, or a 768-byte copy, NULL on failure)
// gives the default pattern; when the formatter's pattern contains it (CFStringFindWithOptions over the
// pattern), it is replaced by the preference format and the result applied (a stack buffer of the length's
// low 32 bits when CFStringGetCharactersPtr fails). A pattern above 768 characters or a failed status stops
// early; the temporary format is closed.
static void __substituteFormatStringFromPrefsDF(OCFDateFormatterRef formatter, Boolean doTime) {
    OCFDateFormatterStyle style = doTime ? formatter->timeStyle : formatter->dateStyle;
    OCFStringRef preferenceFormat = doTime ? formatter->customTimeFormat : formatter->customDateFormat;
    if (style == 0 || preferenceFormat == NULL) {
        return;
    }
    int32_t icuStyle = (uint64_t)style < 5 ? (int32_t)(4 - (uint32_t)style) : -1;
    OCFStringRef identifier = OCFLocaleGetIdentifier(formatter->locale);
    char buffer[768];
    const char *localeName = OCFStringGetCStringPtr(identifier, kOCFStringEncodingUTF8);
    if (localeName == NULL) {
        localeName = OCFStringGetCString(identifier, buffer, 0x300, kOCFStringEncodingUTF8) ? buffer : NULL;
    }
    int32_t status = 0;
    void *icuFormatter =
        udat_open(doTime ? icuStyle : -1, doTime ? -1 : icuStyle, localeName, NULL, 0, NULL, 0, &status);
    if (icuFormatter == NULL) {
        return;
    }
    status = 0;
    UniChar pattern[768];
    int32_t length = udat_toPattern(icuFormatter, false, pattern, 0x300, &status);
    if (status <= 0 && length <= 0x300) {
        OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
        OCFStringRef defaultPattern = OCFStringCreateWithCharacters(systemDefault, pattern, (OCFIndex)length);
        status = 0;
        length = udat_toPattern(formatter->icuFormatter, false, pattern, 0x300, &status);
        if (status <= 0 && length <= 0x300) {
            OCFMutableStringRef current = OCFStringCreateMutable(systemDefault, 0);
            OCFStringAppendCharacters(current, pattern, (OCFIndex)length);
            OCFRange found = {0, 0};
            if (OCFStringFindWithOptions(
                    current, defaultPattern, (OCFRange){0, (OCFIndex)length}, 0, &found
                )) {
                OCFStringReplace(current, found, preferenceFormat);
                OCFIndex newLength = OCFStringGetLength(current);
                UniChar replaced[(uint32_t)newLength];
                const UniChar *characters = OCFStringGetCharactersPtr(current);
                if (characters == NULL) {
                    OCFStringGetCharacters(current, (OCFRange){0, (OCFIndex)(int32_t)newLength}, replaced);
                    characters = replaced;
                }
                status = 0;
                udat_applyPattern(formatter->icuFormatter, false, characters, (int32_t)newLength);
            }
            OCFRelease(current);
        }
        OCFRelease(defaultPattern);
    }
    udat_close(icuFormatter);
}

// Native __ApplyUDateFormatSymbol, 0x1c0194200. Non-external.
// For each of the eighteen symbol arrays, the property value or else the preference value (both read before
// the loop) is applied (__CFDateFormatterSetSymbolsArray with the type from symbolTypes (unk_1C03735B4) and
// the index base built on the stack: 1 for the weekday arrays 8-13, else 0). Then the AM and PM symbols (the
// property, else the preference) are set as ICU symbols 0 and 1 of type 5, at most 768 characters each, the
// status cleared before each call.
static void __ApplyUDateFormatSymbol(OCFDateFormatterRef formatter) {
    OCFArrayRef preferenceSymbols[18];
    OCFArrayRef propertySymbols[18];
    int32_t indexBases[18] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};
    memcpy(propertySymbols, formatter->symbols, sizeof(propertySymbols));
    memcpy(preferenceSymbols, formatter->customSymbols, sizeof(preferenceSymbols));
    for (int index = 0; index < 18; index++) {
        OCFArrayRef array = propertySymbols[index];
        if (array == NULL) {
            array = preferenceSymbols[index];
        }
        if (array != NULL) {
            __CFDateFormatterSetSymbolsArray(
                formatter->icuFormatter, symbolTypes[index], indexBases[index], array
            );
        }
    }
    OCFStringRef amSymbol = formatter->amSymbol;
    if (amSymbol == NULL) {
        amSymbol = formatter->customAMSymbol;
    }
    OCFStringRef pmSymbol = formatter->pmSymbol;
    if (pmSymbol == NULL) {
        pmSymbol = formatter->customPMSymbol;
    }
    OCFStringRef symbol = amSymbol;
    for (int32_t index = 0; index < 2; index++) {
        if (symbol != NULL) {
            OCFIndex length = OCFStringGetLength(symbol);
            OCFIndex capacity = length < 0x300 ? length : 0x300;
            UniChar buffer[capacity];
            const UniChar *characters = OCFStringGetCharactersPtr(symbol);
            if (characters == NULL) {
                OCFStringGetCharacters(symbol, (OCFRange){0, capacity}, buffer);
                length = capacity;
                characters = buffer;
            }
            int32_t status = 0;
            udat_setSymbols(
                formatter->icuFormatter, kICUAMPMSymbols, index, characters, (int32_t)length, &status
            );
        }
        symbol = pmSymbol;
    }
}

// Native __CFDateFormatterStoreSymbolPrefs, 0x1c0194400 (the CFDictionaryApplyFunction applier of
// __ResetUDateFormat). Non-external.
// A string key with an array value is read with CFStringGetIntValue (CFArrayGetCount is called before the
// range check). Keys 0-19 except 5 and 6 replace the preference symbol array of their ICU type (the jump
// table at 0x1c03735a0, verified): the old value released, the array retained. Key 5 stores the array's first
// and second elements, when they are strings, as the preference AM and PM symbols. Key 6 and other values do
// nothing.
static void __CFDateFormatterStoreSymbolPrefs(const void *key, const void *value, void *context) {
    struct __OCFDateFormatter *formatter = context;
    if (OCFGetTypeID(key) != OCFStringGetTypeID()) {
        return;
    }
    if (OCFGetTypeID(value) != OCFArrayGetTypeID()) {
        return;
    }
    int32_t symbolKey = OCFStringGetIntValue(key);
    OCFIndex count = OCFArrayGetCount(value);
    // The preference array index of each ICU symbol type (the inverse of symbolTypes); -1 marks keys 5 and 6.
    static const int8_t preferenceSlots[20] = {0, 2, 3, 8,  9,  -1, -1, 1,  4,  10,
                                               5, 6, 7, 11, 12, 13, 14, 15, 16, 17};
    if ((uint32_t)symbolKey > 19) {
        return;
    }
    if (symbolKey == 5) {
        for (OCFIndex index = 0; index < count; index++) {
            OCFTypeRef element = OCFArrayGetValueAtIndex(value, index);
            if (OCFGetTypeID(element) != OCFStringGetTypeID()) {
                continue;
            }
            if (index == 1) {
                if (formatter->customPMSymbol != NULL) {
                    OCFRelease(formatter->customPMSymbol);
                }
                formatter->customPMSymbol = OCFRetain(element);
            } else if (index == 0) {
                if (formatter->customAMSymbol != NULL) {
                    OCFRelease(formatter->customAMSymbol);
                }
                formatter->customAMSymbol = OCFRetain(element);
            }
        }
        return;
    }
    int8_t slot = preferenceSlots[symbolKey];
    if (slot < 0) {
        return;
    }
    if (formatter->customSymbols[slot] != NULL) {
        OCFRelease(formatter->customSymbols[slot]);
    }
    formatter->customSymbols[slot] = OCFRetain(value);
}

// Native __CFDateFormatterSetSymbolsArray, 0x1c0194710. Non-external.
// Each element of the array (count read once; nothing when below 1) is set as ICU symbol indexBase + index
// (32-bit addition) of the type: at most 768 characters copied into a stack buffer when
// CFStringGetCharactersPtr fails, else the whole string; the status cleared before each call.
static void
__CFDateFormatterSetSymbolsArray(void *icuFormatter, int32_t icuCode, int32_t indexBase, OCFArrayRef array) {
    int32_t status = 0;
    OCFIndex count = OCFArrayGetCount(array);
    if (count < 1) {
        return;
    }
    for (OCFIndex index = 0; index != count; index++) {
        OCFStringRef item = OCFArrayGetValueAtIndex(array, index);
        OCFIndex length = OCFStringGetLength(item);
        OCFIndex capacity = length < 0x300 ? length : 0x300;
        UniChar buffer[capacity];
        const UniChar *characters = OCFStringGetCharactersPtr(item);
        if (characters == NULL) {
            OCFStringGetCharacters(item, (OCFRange){0, capacity}, buffer);
            characters = buffer;
            length = capacity;
        }
        status = 0;
        udat_setSymbols(
            icuFormatter, icuCode, indexBase + (int32_t)index, characters, (int32_t)length, &status
        );
    }
}
