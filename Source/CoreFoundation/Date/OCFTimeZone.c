//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Date/OCFTimeZone.h"

#include "CoreFoundation/Locale/OCFLocale.h"
#include "CoreFoundation/Notification/OCFNotificationCenter.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/message.h>
#include <objc/runtime.h>

// Inferred organization: __systemTimeZoneChangedNotification (0x1c02ab440) and CFTimeZoneGetSecondsFromGMT
// (0x1c02ad220) lie among the CFTimeZone entry points of the iOS Simulator __text section
// (CFTimeZoneCreateWithName 0x1c02ad1a4 through CFTimeZoneIsDaylightSavingTime 0x1c02ad2e8; the handler
// precedes them in the same cluster), the only ones in the scope. In this image CFTimeZone is a layer over
// NSTimeZone. The native source path is not in the evidence. iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json.

// The classes the handler messages, through the class references 0x2227ce338 (NSLocale) and 0x2227ce3d8
// (NSTimeZone).
extern char timeZoneLocaleClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSLocale));
extern char timeZoneClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSTimeZone));

// Native kCFTimeZoneSystemTimeZoneDidChangeNotification, 0x2227a87e0 (__DATA_CONST,__const). External. A
// constant string of its own name (0x2227b94a0), the name __systemTimeZoneChangedNotification posts. Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3).
static const OCFConstantString timeZoneDidChangeString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(kOCFTimeZoneSystemTimeZoneDidChangeNotification),
    sizeof(OF_NAME_STRING(kOCFTimeZoneSystemTimeZoneDidChangeNotification)) - 1
};
const OCFStringRef kOCFTimeZoneSystemTimeZoneDidChangeNotification = (OCFStringRef)&timeZoneDidChangeString;

// Native __systemTimeZoneChangedNotification, 0x1c02ab440. Non-external; the callback of the time zone
// dependent notification below; it reads no arguments. +[NSLocale _resetCurrent], then +[NSTimeZone
// _resetSystemTimeZone], whose result is the object of "kCFTimeZoneSystemTimeZoneDidChangeNotification"
// posted to the local center with deliverImmediately (a tail call).
static void _systemTimeZoneChangedNotification(void) {
    ((void (*)(id, SEL))objc_msgSend)((id)timeZoneLocaleClass, sel_getUid("_resetCurrent"));
    id timeZone = ((id (*)(id, SEL))objc_msgSend)((id)timeZoneClass, sel_getUid("_resetSystemTimeZone"));
    OCFNotificationCenterPostNotification(
        OCFNotificationCenterGetLocalCenter(),
        kOCFTimeZoneSystemTimeZoneDidChangeNotification,
        timeZone,
        NULL,
        true
    );
}

// The class whose date the function builds (NSDate, an Objective-C class of this image; class reference
// 0x2227ce2d0).
extern char timeZoneDateClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDate));

// Native CFTimeZoneGetSecondsFromGMT, 0x1c02ad220. External.
// Inside an autorelease pool: +[NSDate dateWithTimeIntervalSinceReferenceDate:at], then
// -secondsFromGMTForDate: of the time zone; the 64-bit integer result is converted to a double (SCVTF) before
// the pool is popped.
OCFTimeInterval OCFTimeZoneGetSecondsFromGMT(OCFTimeZoneRef tz, OCFAbsoluteTime at) {
    void *pool = _OCFAutoreleasePoolPush();
    id date = ((id (*)(id, SEL, double))objc_msgSend)(
        (id)timeZoneDateClass, sel_getUid("dateWithTimeIntervalSinceReferenceDate:"), at
    );
    int64_t seconds =
        ((int64_t (*)(id, SEL, id))objc_msgSend)((id)tz, sel_getUid("secondsFromGMTForDate:"), date);
    OCFTimeInterval result = (double)seconds;
    _OCFAutoreleasePoolPop(pool);
    return result;
}

// The notify(3) name 0x2227b9420 (coalesced natively with the calendar record's).
static const OCFConstantString timeZoneSetString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "com.apple.system.timezone",
    sizeof("com.apple.system.timezone") - 1
};

// Native __CFTimeZoneDidChangeDependentNotification, 0x2227a87e8 (__DATA_CONST,__const). Its
// sub-notifications, 0x2227a8830, and its once token, 0x22e6d3930
// (__DATA,__bss). Non-external (the record was a private external).
static dispatch_once_t __CFTimeZoneDidChangeInitOnce;
static const DependentSubnotification __CFTimeZoneDidChangeSubnotes[1] = {
    {(OCFStringRef)&timeZoneSetString, 3, {0}},
};
const DependentNotification __CFTimeZoneDidChangeDependentNotification = {
    &kOCFTimeZoneSystemTimeZoneDidChangeNotification,
    &__CFTimeZoneDidChangeInitOnce,
    NULL,
    (OCFNotificationCallback)_systemTimeZoneChangedNotification,
    0,
    1,
    {0},
    __CFTimeZoneDidChangeSubnotes
};
