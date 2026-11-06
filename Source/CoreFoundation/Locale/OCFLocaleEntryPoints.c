//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/OCFLocaleEntryPoints.h"

#include "CoreFoundation/String/OCFString.h"

#include <objc/message.h>
#include <objc/runtime.h>

// Inferred organization: _CFLocaleCreateCopyWithNewCalendarIdentifier (0x1c01dbd30, between CFLocaleCreate
// and CFLocaleGetIdentifier) and the seven preference getters (0x1c01dbea4-0x1c01dbeec, around the two number
// formatter getters) belong to the CFLocale unit, whose other functions Locale/OCFLocale.c restores; that
// file belongs to another family, so these have their own translation unit, in native address order. The
// native source path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.

// libobjc's objc_retain (no SDK declaration in MRC); supplied.
extern id objc_retain(id value);

// Native _CFLocaleCreateCopyWithNewCalendarIdentifier, 0x1c01dbd30. Hidden (was a private external).
// -_localeWithNewCalendarIdentifier: sent to the locale with the identifier (the allocator is not read),
// retained (objc_retain, a tail call).
OCFLocaleRef _CFLocaleCreateCopyWithNewCalendarIdentifier(
    OCFAllocatorRef allocator, OCFLocaleRef locale, OCFStringRef calendarIdentifier
) {
    (void)allocator;
    id copy = ((id (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)locale, sel_getUid("_localeWithNewCalendarIdentifier:"), calendarIdentifier
    );
    return (OCFLocaleRef)objc_retain(copy);
}

// Native _CFLocaleGetAppleFirstWeekdayPref, 0x1c01dbea4. Hidden (was a private external); a tail call of
// -_prefForKey: with "AppleFirstWeekday".
OCFTypeRef _CFLocaleGetAppleFirstWeekdayPref(OCFLocaleRef locale) {
    return ((OCFTypeRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)locale, sel_getUid("_prefForKey:"), OF_STR("AppleFirstWeekday")
    );
}

// Native _CFLocaleGetAppleICUDateTimeSymbolsPref, 0x1c01dbeb0. Hidden (was a private external); a tail call
// of -_prefForKey: with "AppleICUDateTimeSymbols".
OCFTypeRef _CFLocaleGetAppleICUDateTimeSymbolsPref(OCFLocaleRef locale) {
    return ((OCFTypeRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)locale, sel_getUid("_prefForKey:"), OF_STR("AppleICUDateTimeSymbols")
    );
}

// Native _CFLocaleGetAppleMinDaysInFirstWeekPref, 0x1c01dbebc. Hidden (was a private external); a tail call
// of -_prefForKey: with "AppleMinDaysInFirstWeek".
OCFTypeRef _CFLocaleGetAppleMinDaysInFirstWeekPref(OCFLocaleRef locale) {
    return ((OCFTypeRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)locale, sel_getUid("_prefForKey:"), OF_STR("AppleMinDaysInFirstWeek")
    );
}

// Native _CFLocaleGetAppleICUForce24HourTimePref, 0x1c01dbec8. Hidden (was a private external); a tail call
// of -_prefForKey: with "AppleICUForce24HourTime".
OCFTypeRef _CFLocaleGetAppleICUForce24HourTimePref(OCFLocaleRef locale) {
    return ((OCFTypeRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)locale, sel_getUid("_prefForKey:"), OF_STR("AppleICUForce24HourTime")
    );
}

// Native _CFLocaleGetAppleICUForce12HourTimePref, 0x1c01dbed4. Hidden (was a private external); a tail call
// of -_prefForKey: with "AppleICUForce12HourTime".
OCFTypeRef _CFLocaleGetAppleICUForce12HourTimePref(OCFLocaleRef locale) {
    return ((OCFTypeRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)locale, sel_getUid("_prefForKey:"), OF_STR("AppleICUForce12HourTime")
    );
}

// Native _CFLocaleGetAppleICUDateFormatStringsPref, 0x1c01dbee0. Hidden (was a private external); a tail
// call of -_prefForKey: with "AppleICUDateFormatStrings".
OCFTypeRef _CFLocaleGetAppleICUDateFormatStringsPref(OCFLocaleRef locale) {
    return ((OCFTypeRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)locale, sel_getUid("_prefForKey:"), OF_STR("AppleICUDateFormatStrings")
    );
}

// Native _CFLocaleGetAppleICUTimeFormatStringsPref, 0x1c01dbeec. Hidden (was a private external); a tail
// call of -_prefForKey: with "AppleICUTimeFormatStrings".
OCFTypeRef _CFLocaleGetAppleICUTimeFormatStringsPref(OCFLocaleRef locale) {
    return ((OCFTypeRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)locale, sel_getUid("_prefForKey:"), OF_STR("AppleICUTimeFormatStrings")
    );
}
