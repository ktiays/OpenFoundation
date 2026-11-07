//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/OCFCalendar.h"

#include "CoreFoundation/Locale/OCFLocale.h"
#include "CoreFoundation/Notification/OCFNotificationCenter.h"
#include "CoreFoundation/String/OCFString.h"

#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <pthread/qos.h>
#include <sys/time.h>
#include <time.h>

// Inferred organization: _CFCalendarResetCurrent (0x1c02a8118) and __postAndResetMidnight (0x1c02a812c) lie
// in one iOS Simulator __text cluster with _CFCalendarGetMidnightNoteCount (0x1c02a8124, outside the scope)
// and the day-changed notification functions (__initDayChangedNotification 0x1c02a82a4 through
// __dayChangedNotification 0x1c02a833c, __initDayChangedNotification.cold.1 at 0x1c0310390). The native
// source path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json and, for _CFCalendarResetCurrent,
// Local/Evidence/cf-core/iphone-simulator-corpus-delta.json (the two agree).

// The class both functions message: the class reference 0x2227ce338, NSLocale (not NSCalendar), as natively.
extern char calendarResetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSLocale));

// Native __CFLastLocalMidnightDay, 0x22e6d38d8, and __CFMidnightTimer, 0x22e6d38e0 (__DATA,__bss).
// Non-external.
static int64_t __CFLastLocalMidnightDay;
static dispatch_source_t __CFMidnightTimer;

// Native _CFCalendarResetCurrent, 0x1c02a8118. External; a tail call of +[NSLocale _resetCurrent] (the class
// reference is NSLocale's).
void _OCFCalendarResetCurrent(void) {
    ((void (*)(id, SEL))objc_msgSend)((id)calendarResetClass, sel_getUid("_resetCurrent"));
}

// Native __postAndResetMidnight, 0x1c02a812c. Hidden (was private external).
// The local day number is (tv_sec + tm_gmtoff) / 86400 (signed, truncating) from gettimeofday and localtime_r
// into a zeroed timeval and tm. When postIfEarlier is set and the day is before __CFLastLocalMidnightDay, or
// postIfLater is set and it is after, the current locale is reset (+[NSLocale _resetCurrent], after fetching
// the local center) and NSCalendarDayChangedNotification is posted to the local center with no object or user
// info, delivered immediately. The time is then read again (into the same records), the day stored, and the
// midnight timer set to the wall time day * 86400 - tm_gmtoff + 86400 (nanoseconds 0), no repeat
// (DISPATCH_TIME_FOREVER) and a leeway of 3,000,000,000 ns.
void __postAndResetMidnight(Boolean postIfEarlier, Boolean postIfLater) {
    struct timeval now = {0, 0};
    struct tm local = {0};
    gettimeofday(&now, NULL);
    localtime_r(&now.tv_sec, &local);
    int64_t day = (local.tm_gmtoff + now.tv_sec) / 86400;
    if ((postIfEarlier && day < __CFLastLocalMidnightDay) ||
        (postIfLater && day > __CFLastLocalMidnightDay)) {
        OCFNotificationCenterRef center = OCFNotificationCenterGetLocalCenter();
        ((void (*)(id, SEL))objc_msgSend)((id)calendarResetClass, sel_getUid("_resetCurrent"));
        OCFNotificationCenterPostNotification(center, ONSCalendarDayChangedNotification, NULL, NULL, 1);
    }
    gettimeofday(&now, NULL);
    localtime_r(&now.tv_sec, &local);
    int64_t today = (now.tv_sec + local.tm_gmtoff) / 86400;
    __CFLastLocalMidnightDay = today;
    struct timespec midnight = {today * 86400 - local.tm_gmtoff + 86400, 0};
    dispatch_source_t timer = __CFMidnightTimer;
    dispatch_source_set_timer(timer, dispatch_walltime(&midnight, 0), DISPATCH_TIME_FOREVER, 3000000000ULL);
}

// Native __initDayChangedNotification, 0x1c02a82a4, with __initDayChangedNotification.cold.1 (0x1c0310390:
// dispatch_once of the onceToken, 0x22e6d38e8, and ___block_literal_global_36). Hidden (was a private
// external). The once block (_____initDayChangedNotification_block_invoke, 0x1c02a82cc, non-external) creates
// __CFMidnightTimer, a timer source (the GOT slot 0x2224171E8 outside the image, as in the restore timer of
// Notification/OCFNotificationCenter.c: DISPATCH_SOURCE_TYPE_TIMER; confidence: medium) on the global queue
// of the main thread's QoS (flags 2), whose handler (___block_literal_global.2, invoke
// _____initDayChangedNotification_block_invoke_2, 0x1c02a8330, non-external) is __postAndResetMidnight(1, 1);
// it then calls __postAndResetMidnight(0, 0) and resumes the timer.
void __initDayChangedNotification(void) {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      dispatch_queue_t queue = dispatch_get_global_queue(qos_class_main(), 2);
      __CFMidnightTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
      dispatch_source_set_event_handler(__CFMidnightTimer, ^{
        __postAndResetMidnight(true, true);
      });
      __postAndResetMidnight(false, false);
      dispatch_resume(__CFMidnightTimer);
    });
}

// Native __dayChangedNotification, 0x1c02a833c. Non-external; the callback of the calendar day-changed
// dependent notification below. __postAndResetMidnight with 1 and whether the name is CFEqual to
// "com.apple.system.timezone" (the CFString 0x2227b9420; the corpus names the operand cfstr_ComAppleSystem_1,
// resolved from the ADRP/ADD encoding at 0x1c02a8348; confidence: verified). The other arguments are not
// used.
static void _dayChangedNotification(
    OCFNotificationCenterRef center,
    void *observer,
    OCFStringRef name,
    const void *object,
    OCFDictionaryRef userInfo
) {
    (void)center;
    (void)observer;
    (void)object;
    (void)userInfo;
    __postAndResetMidnight(true, OCFEqual(OF_STR("com.apple.system.timezone"), name));
}

// Native NSCalendarDayChangedNotification, 0x2227a8718 (__DATA_CONST,__const). External. A constant string of
// its own name (0x2227b93a0). Reading: Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json
// (round 3).
static const OCFConstantString calendarDayChangedString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSCalendarDayChangedNotification),
    sizeof(OF_NAME_STRING(ONSCalendarDayChangedNotification)) - 1
};
const OCFStringRef ONSCalendarDayChangedNotification = (OCFStringRef)&calendarDayChangedString;

// The sub-notification names 0x2227b85c0 and 0x2227b9420, shared natively with the system clock and time zone
// records (one coalesced literal each).
static const OCFConstantString calendarClockSetString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "com.apple.system.clock_set",
    sizeof("com.apple.system.clock_set") - 1
};
static const OCFConstantString calendarTimeZoneString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "com.apple.system.timezone",
    sizeof("com.apple.system.timezone") - 1
};

// Native __CFCalendarDayChangedDependentNotification, 0x2227a8760 (__DATA_CONST,__const). Its
// sub-notifications, 0x2227a87c0, and its once token, 0x22e6d38f0
// (__DATA,__bss). Non-external (the record was a private external).
static dispatch_once_t __CFCalendarDayChangedInitOnce;
static const DependentSubnotification __CFCalendarDidChangeSubnotes[2] = {
    {(OCFStringRef)&calendarClockSetString, 2, {0}},
    {(OCFStringRef)&calendarTimeZoneString, 2, {0}},
};
const DependentNotification __CFCalendarDayChangedDependentNotification = {
    &ONSCalendarDayChangedNotification,
    &__CFCalendarDayChangedInitOnce,
    __initDayChangedNotification,
    _dayChangedNotification,
    0xe,
    2,
    {0},
    __CFCalendarDidChangeSubnotes
};
