//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFDate.h>
#include <CoreFoundation/CFString.h>

CF_EXTERN_C_BEGIN

typedef CFStringRef CFRunLoopMode CF_EXTENSIBLE_STRING_ENUM;

typedef struct CF_BRIDGED_MUTABLE_TYPE(id) __CFRunLoop *CFRunLoopRef;
typedef struct CF_BRIDGED_MUTABLE_TYPE(id) __CFRunLoopSource *CFRunLoopSourceRef;
typedef struct CF_BRIDGED_MUTABLE_TYPE(id) __CFRunLoopObserver *CFRunLoopObserverRef;
typedef struct CF_BRIDGED_MUTABLE_TYPE(NSTimer) __CFRunLoopTimer *CFRunLoopTimerRef;

typedef CF_ENUM(SInt32, CFRunLoopRunResult) {
    kCFRunLoopRunFinished = 1,
    kCFRunLoopRunStopped = 2,
    kCFRunLoopRunTimedOut = 3,
    kCFRunLoopRunHandledSource = 4
};

typedef CF_OPTIONS(CFOptionFlags, CFRunLoopActivity) {
    kCFRunLoopEntry = (1UL << 0),
    kCFRunLoopBeforeTimers = (1UL << 1),
    kCFRunLoopBeforeSources = (1UL << 2),
    kCFRunLoopBeforeWaiting = (1UL << 5),
    kCFRunLoopAfterWaiting = (1UL << 6),
    kCFRunLoopExit = (1UL << 7),
    kCFRunLoopAllActivities = 0x0FFFFFFFU
};

CF_EXPORT const CFRunLoopMode kCFRunLoopDefaultMode;
CF_EXPORT const CFRunLoopMode kCFRunLoopCommonModes;

CF_EXPORT CFRunLoopRef CFRunLoopGetCurrent(void);
CF_EXPORT CFRunLoopRef CFRunLoopGetMain(void);
CF_EXPORT CFRunLoopMode CFRunLoopCopyCurrentMode(CFRunLoopRef rl);
CF_EXPORT CFArrayRef CFRunLoopCopyAllModes(CFRunLoopRef rl);
CF_EXPORT void CFRunLoopRun(void);
CF_EXPORT CFRunLoopRunResult CFRunLoopRunInMode(
    CFRunLoopMode mode,
    CFTimeInterval seconds,
    Boolean returnAfterSourceHandled
);
CF_EXPORT Boolean CFRunLoopIsWaiting(CFRunLoopRef rl);
CF_EXPORT void CFRunLoopWakeUp(CFRunLoopRef rl);
CF_EXPORT void CFRunLoopStop(CFRunLoopRef rl);
CF_EXPORT void CFRunLoopPerformBlock(CFRunLoopRef rl, CFTypeRef mode, void (^block)(void));
CF_EXPORT Boolean CFRunLoopContainsSource(CFRunLoopRef rl, CFRunLoopSourceRef source, CFRunLoopMode mode);
CF_EXPORT void CFRunLoopAddSource(CFRunLoopRef rl, CFRunLoopSourceRef source, CFRunLoopMode mode);
CF_EXPORT void CFRunLoopRemoveSource(CFRunLoopRef rl, CFRunLoopSourceRef source, CFRunLoopMode mode);
CF_EXPORT void CFRunLoopAddObserver(CFRunLoopRef rl, CFRunLoopObserverRef observer, CFRunLoopMode mode);
CF_EXPORT void CFRunLoopRemoveObserver(CFRunLoopRef rl, CFRunLoopObserverRef observer, CFRunLoopMode mode);
CF_EXPORT void CFRunLoopAddTimer(CFRunLoopRef rl, CFRunLoopTimerRef timer, CFRunLoopMode mode);
CF_EXPORT void CFRunLoopRemoveTimer(CFRunLoopRef rl, CFRunLoopTimerRef timer, CFRunLoopMode mode);

typedef struct {
    CFIndex version;
    void *info;
    const void *(*retain)(const void *info);
    void (*release)(const void *info);
    CFStringRef (*copyDescription)(const void *info);
    Boolean (*equal)(const void *info1, const void *info2);
    CFHashCode (*hash)(const void *info);
    void (*schedule)(void *info, CFRunLoopRef rl, CFRunLoopMode mode);
    void (*cancel)(void *info, CFRunLoopRef rl, CFRunLoopMode mode);
    void (*perform)(void *info);
} CFRunLoopSourceContext;

CF_EXPORT CFRunLoopSourceRef CFRunLoopSourceCreate(
    CFAllocatorRef allocator,
    CFIndex order,
    CFRunLoopSourceContext *context
);
CF_EXPORT void CFRunLoopSourceInvalidate(CFRunLoopSourceRef source);
CF_EXPORT Boolean CFRunLoopSourceIsValid(CFRunLoopSourceRef source);
CF_EXPORT void CFRunLoopSourceGetContext(CFRunLoopSourceRef source, CFRunLoopSourceContext *context);
CF_EXPORT void CFRunLoopSourceSignal(CFRunLoopSourceRef source);

typedef struct {
    CFIndex version;
    void *info;
    const void *(*retain)(const void *info);
    void (*release)(const void *info);
    CFStringRef (*copyDescription)(const void *info);
} CFRunLoopObserverContext;

typedef void (*CFRunLoopObserverCallBack)(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info);

CF_EXPORT CFRunLoopObserverRef CFRunLoopObserverCreate(
    CFAllocatorRef allocator,
    CFOptionFlags activities,
    Boolean repeats,
    CFIndex order,
    CFRunLoopObserverCallBack callout,
    CFRunLoopObserverContext *context
);
CF_EXPORT void CFRunLoopObserverInvalidate(CFRunLoopObserverRef observer);

typedef struct {
    CFIndex version;
    void *info;
    const void *(*retain)(const void *info);
    void (*release)(const void *info);
    CFStringRef (*copyDescription)(const void *info);
} CFRunLoopTimerContext;

typedef void (*CFRunLoopTimerCallBack)(CFRunLoopTimerRef timer, void *info);

CF_EXPORT CFTypeID CFRunLoopTimerGetTypeID(void);
CF_EXPORT CFRunLoopTimerRef CFRunLoopTimerCreate(
    CFAllocatorRef allocator,
    CFAbsoluteTime fireDate,
    CFTimeInterval interval,
    CFOptionFlags flags,
    CFIndex order,
    CFRunLoopTimerCallBack callout,
    CFRunLoopTimerContext *context
);
CF_EXPORT CFAbsoluteTime CFRunLoopTimerGetNextFireDate(CFRunLoopTimerRef timer);
CF_EXPORT void CFRunLoopTimerSetNextFireDate(CFRunLoopTimerRef timer, CFAbsoluteTime fireDate);
CF_EXPORT CFTimeInterval CFRunLoopTimerGetInterval(CFRunLoopTimerRef timer);
CF_EXPORT void CFRunLoopTimerInvalidate(CFRunLoopTimerRef timer);
CF_EXPORT Boolean CFRunLoopTimerIsValid(CFRunLoopTimerRef timer);
CF_EXPORT void CFRunLoopTimerGetContext(CFRunLoopTimerRef timer, CFRunLoopTimerContext *context);
CF_EXPORT CFTimeInterval CFRunLoopTimerGetTolerance(CFRunLoopTimerRef timer);
CF_EXPORT void CFRunLoopTimerSetTolerance(CFRunLoopTimerRef timer, CFTimeInterval tolerance);

CF_EXTERN_C_END
