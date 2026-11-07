//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/OCFLocale.h"

#include <OpenCoreFoundation/Date/OCFDate.h>

#include "CoreFoundation/Locale/OCFCalendar.h"
#include "CoreFoundation/Notification/OCFNotificationCenter.h"
#include "CoreFoundation/PlugIn/OCFBundleLocalization.h"
#include "CoreFoundation/Preferences/OCFPreferences.h"
#include "CoreFoundation/String/OCFString.h"

#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <pthread/qos.h>

// Inferred organization: the CFLocale functions restored here lie in one iOS Simulator __text region
// (CFLocaleGetTypeID 0x1c01dbc0c through ____CFLocalePrefsChanged_block_invoke 0x1c01dc0f4), with the unit's
// other functions (outside the scope) between them. In this image CFLocale is a thin layer over NSLocale (an
// Objective-C class of the image, its isa loaded from the class reference 0x2227ce338). The native source
// path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json and, for ___CFLocaleResetCurrent and
// _CFLocaleResetCurrent, Local/Evidence/cf-core/iphone-simulator-corpus-delta.json.
//
// The main queue of the reset paths is loaded from the GOT slot 0x222417150, outside the image; the image
// imports one queue object, _dispatch_main_q, which dispatch_get_main_queue() names (confidence: medium for
// that binding).

// libobjc's objc_retain (no SDK declaration in MRC); supplied.
extern id objc_retain(id value);

// The class the CFLocale entry points message (NSLocale, an Objective-C class of this image).
extern char localeClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSLocale));

// Native __kCFLocaleCollatorID, 0x2227a3b08 (__DATA_CONST,__const, before __CFLocaleClass at 0x2227a3b10).
// Hidden (was a private external). Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json
// ("data"); its constant string (0x2227b0d20, flags 0x7c8, 18 bytes) read with
// Local/Scratch/cf-core/macho.py.
static const OCFConstantString localeCollatorIDString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "locale:collator id", 18
};
const OCFStringRef __kCFLocaleCollatorID = (OCFStringRef)&localeCollatorIDString;

// Native __lastNote, 0x22e6d2eb0 (__DATA,__bss). Non-external: the time of the latest preferences change,
// compared by the delayed block.
static double __lastNote;

// Native ___block_literal_global_14 (__DATA_CONST,__const), the global block whose invoke is
// ____CFLocaleResetCurrent_block_invoke (0x1c01dbf78, non-external); shared by __CFLocaleResetCurrent,
// _CFLocaleResetCurrent and the preferences block, which dispatch it to the main queue. It resets the current
// locale again (+[NSLocale _resetCurrent]) and posts the three current-locale notifications, in this order,
// to the local center with no object or user info, delivered immediately.
static void (^const localeResetCurrentBlock)(void) = ^{
  ((void (*)(id, SEL))objc_msgSend)((id)localeClass, sel_getUid("_resetCurrent"));
  OCFNotificationCenterRef center = OCFNotificationCenterGetLocalCenter();
  OCFNotificationCenterPostNotification(
      center, OF_STR("kCFLocaleCurrentLocaleDidChangeNotification-4"), NULL, NULL, 1
  );
  OCFNotificationCenterPostNotification(
      center, OF_STR("kCFLocaleCurrentLocaleDidChangeNotification-2"), NULL, NULL, 1
  );
  OCFNotificationCenterPostNotification(
      center,
      OF_STR(
          "kCFLocaleCurrentLocale"
          "DidChangeNotification"
      ),
      NULL,
      NULL,
      1
  );
};

// Native CFLocaleGetTypeID, 0x1c01dbc0c. External; the constant 55.
OCFTypeID OCFLocaleGetTypeID(void) {
    return 55;
}

// Native CFLocaleGetSystem, 0x1c01dbc14. External; a tail call of +[NSLocale _system].
OCFLocaleRef OCFLocaleGetSystem(void) {
    return ((OCFLocaleRef (*)(id, SEL))objc_msgSend)((id)localeClass, sel_getUid("_system"));
}

// Native CFLocaleCopyCurrent, 0x1c01dbcf4. External; +[NSLocale _current], retained (objc_retain).
OCFLocaleRef OCFLocaleCopyCurrent(void) {
    id current = ((id (*)(id, SEL))objc_msgSend)((id)localeClass, sel_getUid("_current"));
    return (OCFLocaleRef)objc_retain(current);
}

// Native CFLocaleCreate, 0x1c01dbd10. External. NULL for a NULL identifier; otherwise a tail call of
// +[NSLocale _newLocaleWithIdentifier:]. The allocator is not used.
OCFLocaleRef OCFLocaleCreate(OCFAllocatorRef allocator, OCFLocaleIdentifier localeIdentifier) {
    (void)allocator;
    if (localeIdentifier == NULL) {
        return NULL;
    }
    return ((OCFLocaleRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)localeClass, sel_getUid("_newLocaleWithIdentifier:"), localeIdentifier
    );
}

// Native CFLocaleGetIdentifier, 0x1c01dbd48. External; a tail call of -localeIdentifier.
OCFLocaleIdentifier OCFLocaleGetIdentifier(OCFLocaleRef locale) {
    return ((OCFLocaleIdentifier (*)(id, SEL))objc_msgSend)((id)locale, sel_getUid("localeIdentifier"));
}

// Native CFLocaleGetValue, 0x1c01dbd4c. External; a tail call of -objectForKey:.
OCFTypeRef OCFLocaleGetValue(OCFLocaleRef locale, OCFLocaleKey key) {
    return ((OCFTypeRef (*)(id, SEL, OCFLocaleKey))objc_msgSend)(
        (id)locale, sel_getUid("objectForKey:"), key
    );
}

// Native CFLocaleCopyDisplayNameForPropertyValue, 0x1c01dbd54 (corpus delta capture 7). External;
// -displayNameForKey:value: (an NSLocale method, Foundation's), retained (objc_retain, a tail call).
OCFStringRef
OCFLocaleCopyDisplayNameForPropertyValue(OCFLocaleRef displayLocale, OCFStringRef key, OCFStringRef value) {
    id name = ((id (*)(id, SEL, OCFStringRef, OCFStringRef))objc_msgSend)(
        (id)displayLocale, sel_getUid("displayNameForKey:value:"), key, value
    );
    return (OCFStringRef)objc_retain(name);
}

// Native CFLocaleGetLanguageCharacterDirection, 0x1c01dbe2c. External; a tail call of
// +[NSLocale characterDirectionForLanguage:].
OCFLocaleLanguageDirection OCFLocaleGetLanguageCharacterDirection(OCFStringRef isoLangCode) {
    return ((OCFLocaleLanguageDirection (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)localeClass, sel_getUid("characterDirectionForLanguage:"), isoLangCode
    );
}

// Native _CFLocaleGetAppleICUNumberFormatStringsPref, 0x1c01dbe8c. Hidden (was private external); a tail call
// of -_prefForKey: with "AppleICUNumberFormatStrings".
OCFTypeRef _CFLocaleGetAppleICUNumberFormatStringsPref(OCFLocaleRef locale) {
    return ((OCFTypeRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)locale, sel_getUid("_prefForKey:"), OF_STR("AppleICUNumberFormatStrings")
    );
}

// Native _CFLocaleGetAppleICUNumberSymbolsPref, 0x1c01dbe98. Hidden (was private external); a tail call of
// -_prefForKey: with "AppleICUNumberSymbols".
OCFTypeRef _CFLocaleGetAppleICUNumberSymbolsPref(OCFLocaleRef locale) {
    return ((OCFTypeRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)locale, sel_getUid("_prefForKey:"), OF_STR("AppleICUNumberSymbols")
    );
}

// Native __CFLocaleGetDoesNotRequireSpecialCaseHandling, 0x1c01dbef8. Hidden (was private external);
// -_doesNotRequireSpecialCaseHandling (a framed call, not a tail call).
Boolean __CFLocaleGetDoesNotRequireSpecialCaseHandling(OCFLocaleRef locale) {
    return ((Boolean (*)(id, SEL))objc_msgSend)((id)locale, sel_getUid("_doesNotRequireSpecialCaseHandling"));
}

// Native _CFLocaleCopyLocaleIdentifierByAddingLikelySubtags, 0x1c01dbf0c. Hidden (was private external);
// +[NSLocale _localeIdentifierByAddingLikelySubtags:], retained (objc_retain).
OCFStringRef _CFLocaleCopyLocaleIdentifierByAddingLikelySubtags(OCFStringRef localeIdentifier) {
    id identifier = ((id (*)(id, SEL, OCFStringRef))objc_msgSend)(
        (id)localeClass, sel_getUid("_localeIdentifierByAddingLikelySubtags:"), localeIdentifier
    );
    return (OCFStringRef)objc_retain(identifier);
}

// Native __CFLocaleResetCurrent, 0x1c01dbf2c. Hidden (was private external).
// Synchronizes the current application's preferences, resets the current locale when the argument (tested as
// 32 bits) is not zero, and dispatches localeResetCurrentBlock to the main queue.
void __CFLocaleResetCurrent(Boolean resetLocale) {
    OCFPreferencesAppSynchronize(kOCFPreferencesCurrentApplication);
    if (resetLocale) {
        ((void (*)(id, SEL))objc_msgSend)((id)localeClass, sel_getUid("_resetCurrent"));
    }
    dispatch_async(dispatch_get_main_queue(), localeResetCurrentBlock);
}

// Native _CFLocaleResetCurrent, 0x1c01dbff0. External. As __CFLocaleResetCurrent with the reset always made.
void _OCFLocaleResetCurrent(void) {
    OCFPreferencesAppSynchronize(kOCFPreferencesCurrentApplication);
    ((void (*)(id, SEL))objc_msgSend)((id)localeClass, sel_getUid("_resetCurrent"));
    dispatch_async(dispatch_get_main_queue(), localeResetCurrentBlock);
}

// Native __CFLocalePrefsChanged, 0x1c01dc02c. Hidden (was private external).
// Records the current absolute time in __lastNote and schedules, 10 ms after now, on the global queue of the
// main thread's QoS class (flags 2, as natively), a block capturing that time
// (____CFLocalePrefsChanged_block_invoke, 0x1c01dc0f4; descriptor ___block_descriptor_40_e5_v8_?0l, flags
// 0xc0000000): when no later change replaced __lastNote (an exact comparison), it resets the current
// calendar, synchronizes the current application's preferences and dispatches localeResetCurrentBlock to the
// main queue.
void __CFLocalePrefsChanged(void) {
    OCFAbsoluteTime now = OCFAbsoluteTimeGetCurrent();
    __lastNote = now;
    dispatch_time_t when = dispatch_time(DISPATCH_TIME_NOW, 10000000);
    dispatch_queue_t queue = dispatch_get_global_queue(qos_class_main(), 2);
    dispatch_after(when, queue, ^{
      if (now == __lastNote) {
          _OCFCalendarResetCurrent();
          OCFPreferencesAppSynchronize(kOCFPreferencesCurrentApplication);
          dispatch_async(dispatch_get_main_queue(), localeResetCurrentBlock);
      }
    });
}

// Native kCFLocaleCurrentLocaleDidChangeNotification, 0x2227a3b90 (__DATA_CONST,__const). External. A
// constant string of its own name (0x2227b0ea0). Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json.
static const OCFConstantString localeDidChangeString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(kOCFLocaleCurrentLocaleDidChangeNotification),
    sizeof(OF_NAME_STRING(kOCFLocaleCurrentLocaleDidChangeNotification)) - 1
};
const OCFStringRef kOCFLocaleCurrentLocaleDidChangeNotification = (OCFStringRef)&localeDidChangeString;

// The seven preferences notifications (0x2227b0f00, 0x2227b0f20, 0x2227b0f40, 0x2227b0f60, 0x2227b0f80,
// 0x2227b0ee0 and 0x2227b0ec0), in the record's order. The callback reads no argument (OCFLocale.h declares
// it void); the native record stores it as a notification callback.
static const OCFConstantString localeNumberString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "AppleNumberPreferencesChangedNotification",
    sizeof("AppleNumberPreferencesChangedNotification") - 1
};
static const OCFConstantString localeTimeString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "AppleTimePreferencesChangedNotification",
    sizeof("AppleTimePreferencesChangedNotification") - 1
};
static const OCFConstantString localeDateString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "AppleDatePreferencesChangedNotification",
    sizeof("AppleDatePreferencesChangedNotification") - 1
};
static const OCFConstantString localeLanguageString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "AppleLanguagePreferencesChangedNotification",
    sizeof("AppleLanguagePreferencesChangedNotification") - 1
};
static const OCFConstantString localeTextBehaviorString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "AppleTextBehaviorPreferencesChangedNotification",
    sizeof("AppleTextBehaviorPreferencesChangedNotification") - 1
};
static const OCFConstantString localeTemperatureString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "AppleTemperatureUnitPreferencesChangedNotification",
    sizeof("AppleTemperatureUnitPreferencesChangedNotification") - 1
};
static const OCFConstantString localeMeasurementString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "AppleMeasurementSystemPreferencesChangedNotification",
    sizeof("AppleMeasurementSystemPreferencesChangedNotification") - 1
};

// Native __CFLocaleDidChangeDependentNotification, 0x2227a3b98 (__DATA_CONST,__const). Its sub-notifications,
// 0x2227a3be0, and its once token, 0x22e6d2eb8
// (__DATA,__bss). Non-external (the record was a private external).
static dispatch_once_t __CFLocaleDidChangeInitOnce;
static const DependentSubnotification __CFLocaleDidChangeSubnotes[7] = {
    {(OCFStringRef)&localeNumberString, 2, {0}},
    {(OCFStringRef)&localeTimeString, 2, {0}},
    {(OCFStringRef)&localeDateString, 2, {0}},
    {(OCFStringRef)&localeLanguageString, 2, {0}},
    {(OCFStringRef)&localeTextBehaviorString, 2, {0}},
    {(OCFStringRef)&localeTemperatureString, 2, {0}},
    {(OCFStringRef)&localeMeasurementString, 2, {0}},
};
const DependentNotification __CFLocaleDidChangeDependentNotification = {
    &kOCFLocaleCurrentLocaleDidChangeNotification,
    &__CFLocaleDidChangeInitOnce,
    NULL,
    (OCFNotificationCallback)__CFLocalePrefsChanged,
    0xe,
    7,
    {0},
    __CFLocaleDidChangeSubnotes
};
