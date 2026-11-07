//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>
#include <OpenCoreFoundation/Date/OCFDate.h>

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFBag.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <dispatch/dispatch.h>
#include <mach/mach.h>
#include <os/lock.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

// Public CFRunLoop interface, iOS Simulator SDK CFRunLoop.h, spelled with the OCF prefix.
typedef OCFStringRef OCFRunLoopMode;
typedef struct __OCFRunLoop *OCFRunLoopRef;
typedef struct __OCFRunLoopSource *OCFRunLoopSourceRef;
typedef struct __OCFRunLoopObserver *OCFRunLoopObserverRef;
typedef struct __OCFRunLoopTimer *OCFRunLoopTimerRef;

typedef enum : SInt32 {
    kOCFRunLoopRunFinished = 1,
    kOCFRunLoopRunStopped = 2,
    kOCFRunLoopRunTimedOut = 3,
    kOCFRunLoopRunHandledSource = 4,
} OCFRunLoopRunResult;

typedef enum : OCFOptionFlags {
    kOCFRunLoopEntry = (1UL << 0),
    kOCFRunLoopBeforeTimers = (1UL << 1),
    kOCFRunLoopBeforeSources = (1UL << 2),
    kOCFRunLoopBeforeWaiting = (1UL << 5),
    kOCFRunLoopAfterWaiting = (1UL << 6),
    kOCFRunLoopExit = (1UL << 7),
    kOCFRunLoopAllActivities = 0x0FFFFFFFU,
} OCFRunLoopActivity;

typedef struct {
    OCFIndex version;
    void *info;
    const void *(*retain)(const void *info);
    void (*release)(const void *info);
    OCFStringRef (*copyDescription)(const void *info);
    Boolean (*equal)(const void *info1, const void *info2);
    OCFHashCode (*hash)(const void *info);
    void (*schedule)(void *info, OCFRunLoopRef rl, OCFRunLoopMode mode);
    void (*cancel)(void *info, OCFRunLoopRef rl, OCFRunLoopMode mode);
    void (*perform)(void *info);
} OCFRunLoopSourceContext;

typedef struct {
    OCFIndex version;
    void *info;
    const void *(*retain)(const void *info);
    void (*release)(const void *info);
    OCFStringRef (*copyDescription)(const void *info);
    Boolean (*equal)(const void *info1, const void *info2);
    OCFHashCode (*hash)(const void *info);
    mach_port_t (*getPort)(void *info);
    void *(*perform)(void *msg, OCFIndex size, OCFAllocatorRef allocator, void *info);
} OCFRunLoopSourceContext1;

typedef void (*OCFRunLoopObserverCallBack)(
    OCFRunLoopObserverRef observer, OCFRunLoopActivity activity, void *info
);
typedef void (*OCFRunLoopTimerCallBack)(OCFRunLoopTimerRef timer, void *info);

// The run loop objects, iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified for every offset named here (the accesses of the functions in RunLoop/OCFRunLoop.c), unread for
// the bytes marked unread. The type and field names are local descriptive spellings.
//
// CFRunLoop (type 43, 2,720 bytes). The nesting index at 84 selects the current entry of the per-run arrays:
// the stop flags (85, entry 0 is outside every run), the ignore-wake-ups flags (339) and the run options
// stored by each run (600 + 8 * index, written at indices 1 to 254; the store at index 254 lands on the
// thread field at 2632, see __CFRunLoopRunSpecificWithOptions). The block list items are 24-byte malloc
// blocks: next, mode (a string or a set of strings), block.
typedef struct RunLoopBlockItem {
    struct RunLoopBlockItem *next;
    OCFTypeRef mode;
    void (^block)(void);
} RunLoopBlockItem;

struct __OCFRunLoop {
    OCFRuntimeBase base;
    pthread_mutex_t lock;
    mach_port_t wakeUpPort;
    uint8_t nestingIndex;
    uint8_t stopped[254];
    uint8_t ignoreWakeUps[254];
    uint8_t unread0[7];
    uint64_t runOptions[254];
    pthread_t pthread;
    OCFMutableSetRef commonModes;
    OCFMutableSetRef commonModeItems;
    struct RunLoopMode *currentMode;
    OCFMutableSetRef modes;
    RunLoopBlockItem *blocksHead;
    RunLoopBlockItem *blocksTail;
    uint8_t unread1[8];
    double sleepTime;
    OCFTypeRef counterpart;
    uint8_t fromThreadDestructor;
    uint8_t perCalloutARP;
    uint8_t didDispatchMainQueue;
    uint8_t unread2;
    os_unfair_lock timerTSRLock;
};

OF_ABI_ASSERT(sizeof(struct __OCFRunLoop) == 2720, "The native CFRunLoop object is 2,720 bytes.");
OF_ABI_ASSERT(offsetof(struct __OCFRunLoop, ignoreWakeUps) == 339, "The ignore-wake-ups flags are at 339.");
OF_ABI_ASSERT(offsetof(struct __OCFRunLoop, pthread) == 2632, "The thread is at 2632.");
OF_ABI_ASSERT(offsetof(struct __OCFRunLoop, fromThreadDestructor) == 2712, "The destructor flag is at 2712.");
OF_ABI_ASSERT(offsetof(struct __OCFRunLoop, timerTSRLock) == 2716, "The timer lock is at 2716.");

// CFRunLoopMode (type 44, 200 bytes).
typedef struct RunLoopMode {
    OCFRuntimeBase base;
    pthread_mutex_t lock;
    OCFStringRef name;
    uint8_t stopped;
    uint8_t unread0[7];
    OCFMutableSetRef sources0;
    OCFMutableSetRef sources1;
    OCFMutableArrayRef observers;
    OCFMutableArrayRef timers;
    OCFMutableDictionaryRef portToV1SourceMap;
    mach_port_t portSet;
    uint32_t unread1;
    OCFOptionFlags observerMask;
    dispatch_source_t timerSource;
    dispatch_queue_t queue;
    uint8_t timerFired;
    uint8_t dispatchTimerArmed;
    uint8_t unread2[2];
    mach_port_t timerPort;
    uint8_t mkTimerArmed;
    uint8_t unread3[7];
    uint64_t timerSoftDeadline;
    uint64_t timerHardDeadline;
} RunLoopMode;

OF_ABI_ASSERT(sizeof(RunLoopMode) == 200, "The native CFRunLoopMode object is 200 bytes.");
OF_ABI_ASSERT(offsetof(RunLoopMode, observerMask) == 144, "The observer mask is at 144.");
OF_ABI_ASSERT(offsetof(RunLoopMode, timerPort) == 172, "The timer port is at 172.");

// CFRunLoopSource (type 46, 184 bytes). Info bit 3 marks a valid source. The context is a version 0 or a
// version 1 context; the perform callback of version 0 is at 176, of version 1 at 168.
struct __OCFRunLoopSource {
    OCFRuntimeBase base;
    pthread_mutex_t lock;
    OCFIndex order;
    uint64_t signaledTime;
    OCFMutableBagRef runLoops;

    union {
        OCFRunLoopSourceContext version0;
        OCFRunLoopSourceContext1 version1;
    } context;
};

OF_ABI_ASSERT(sizeof(struct __OCFRunLoopSource) == 184, "The native CFRunLoopSource object is 184 bytes.");

// CFRunLoopObserver (type 45). Info bit 0 marks a firing observer, bit 1 a repeating one, bit 2 one that may
// fire while it is firing, bit 3 a valid one. Only the offsets read by this unit are named; the object's size
// is unread.
typedef struct {
    OCFIndex version;
    void *info;
    const void *(*retain)(const void *info);
    void (*release)(const void *info);
    OCFStringRef (*copyDescription)(const void *info);
} RunLoopObserverContext;

struct __OCFRunLoopObserver {
    OCFRuntimeBase base;
    pthread_mutex_t lock;
    OCFRunLoopRef runLoop;
    OCFIndex runLoopCount;
    OCFOptionFlags activities;
    OCFIndex order;
    OCFRunLoopObserverCallBack callout;
    RunLoopObserverContext context;
};

// CFRunLoopTimer (type 47). The 16-bit word at 16 holds bit 0 (firing) and bit 2 (the run loop does not
// retain the timer while invalidating it); info bit 3 marks a valid timer. Only the offsets read by this unit
// are named; the object's size is unread.
typedef struct {
    OCFIndex version;
    void *info;
    const void *(*retain)(const void *info);
    void (*release)(const void *info);
    OCFStringRef (*copyDescription)(const void *info);
} RunLoopTimerContext;

struct __OCFRunLoopTimer {
    OCFRuntimeBase base;
    uint16_t bits;
    uint8_t unread0[6];
    pthread_mutex_t lock;
    OCFRunLoopRef runLoop;
    OCFMutableSetRef rlModes;
    OCFAbsoluteTime nextFireDate;
    OCFTimeInterval interval;
    OCFTimeInterval tolerance;
    uint64_t fireTSR;
    OCFIndex order;
    OCFRunLoopTimerCallBack callout;
    RunLoopTimerContext context;
};

_Static_assert(offsetof(struct __OCFRunLoopTimer, lock) == 24, "The timer lock is at 24.");
OF_ABI_ASSERT(offsetof(struct __OCFRunLoopTimer, fireTSR) == 128, "The fire time is at 128.");
OF_ABI_ASSERT(offsetof(struct __OCFRunLoopTimer, context) == 152, "The timer context is at 152.");

OF_EXPORT const OCFRunLoopMode kOCFRunLoopDefaultMode;
OF_EXPORT const OCFRunLoopMode kOCFRunLoopCommonModes;

OF_EXPORT OCFRunLoopRef OCFRunLoopGetCurrent(void);
OF_EXPORT OCFRunLoopRef OCFRunLoopGetMain(void);
OF_EXPORT OCFRunLoopMode OCFRunLoopCopyCurrentMode(OCFRunLoopRef rl);
OF_EXPORT OCFArrayRef OCFRunLoopCopyAllModes(OCFRunLoopRef rl);
OF_EXPORT OCFRunLoopRunResult
OCFRunLoopRunInMode(OCFRunLoopMode mode, OCFTimeInterval seconds, Boolean returnAfterSourceHandled);
OF_EXPORT Boolean OCFRunLoopIsWaiting(OCFRunLoopRef rl);
OF_EXPORT void OCFRunLoopWakeUp(OCFRunLoopRef rl);
OF_EXPORT Boolean OCFRunLoopContainsSource(OCFRunLoopRef rl, OCFRunLoopSourceRef source, OCFRunLoopMode mode);
OF_EXPORT void OCFRunLoopAddSource(OCFRunLoopRef rl, OCFRunLoopSourceRef source, OCFRunLoopMode mode);
OF_EXPORT void OCFRunLoopRemoveSource(OCFRunLoopRef rl, OCFRunLoopSourceRef source, OCFRunLoopMode mode);
OF_EXPORT void OCFRunLoopAddObserver(OCFRunLoopRef rl, OCFRunLoopObserverRef observer, OCFRunLoopMode mode);
OF_EXPORT void
OCFRunLoopRemoveObserver(OCFRunLoopRef rl, OCFRunLoopObserverRef observer, OCFRunLoopMode mode);
OF_EXPORT void OCFRunLoopAddTimer(OCFRunLoopRef rl, OCFRunLoopTimerRef timer, OCFRunLoopMode mode);
OF_EXPORT void OCFRunLoopRemoveTimer(OCFRunLoopRef rl, OCFRunLoopTimerRef timer, OCFRunLoopMode mode);
OF_EXPORT OCFRunLoopSourceRef
OCFRunLoopSourceCreate(OCFAllocatorRef allocator, OCFIndex order, OCFRunLoopSourceContext *context);
OF_EXPORT void OCFRunLoopSourceInvalidate(OCFRunLoopSourceRef source);
OF_EXPORT void OCFRunLoopSourceGetContext(OCFRunLoopSourceRef source, OCFRunLoopSourceContext *context);
OF_EXPORT void OCFRunLoopSourceSignal(OCFRunLoopSourceRef source);
OF_EXPORT Boolean OCFRunLoopSourceIsValid(OCFRunLoopSourceRef source);
OF_EXPORT void OCFRunLoopObserverInvalidate(OCFRunLoopObserverRef observer);
OF_EXPORT void OCFRunLoopTimerInvalidate(OCFRunLoopTimerRef timer);
OF_EXPORT OCFRunLoopTimerRef OCFRunLoopTimerCreate(
    OCFAllocatorRef allocator,
    OCFAbsoluteTime fireDate,
    OCFTimeInterval interval,
    OCFOptionFlags flags,
    OCFIndex order,
    OCFRunLoopTimerCallBack callout,
    RunLoopTimerContext *context
);
OF_EXPORT void OCFRunLoopTimerSetNextFireDate(OCFRunLoopTimerRef timer, OCFAbsoluteTime fireDate);
OF_EXPORT Boolean OCFRunLoopTimerIsValid(OCFRunLoopTimerRef timer);
OF_EXPORT void OCFRunLoopTimerGetContext(OCFRunLoopTimerRef timer, RunLoopTimerContext *context);
OF_EXPORT OCFAbsoluteTime OCFRunLoopTimerGetNextFireDate(OCFRunLoopTimerRef timer);
OF_EXPORT OCFTimeInterval OCFRunLoopTimerGetInterval(OCFRunLoopTimerRef timer);
OF_EXPORT OCFTimeInterval OCFRunLoopTimerGetTolerance(OCFRunLoopTimerRef timer);
OF_EXPORT void OCFRunLoopTimerSetTolerance(OCFRunLoopTimerRef timer, OCFTimeInterval tolerance);

// Natively exported SPI (no SDK declaration).
OF_EXPORT OCFRunLoopRef _OCFRunLoopGet0(pthread_t t);
OF_EXPORT OCFRunLoopRef _OCFRunLoopGet0b(pthread_t t);
OF_EXPORT bool _OCFRunLoopCurrentIsMain(void);
// Native _CFRunLoopGet2, 0x1c01dd570: the run loop's Foundation counterpart (an NSRunLoop).
OF_EXPORT OCFTypeRef _OCFRunLoopGet2(OCFRunLoopRef rl);

// The Foundation class _CFRunLoopGet2 looks up by name.
#ifdef __OBJC__
@class ONSRunLoop;
#endif

// Hidden (were private externals).
OF_EXTERN OCFRunLoopRef _CFRunLoopCacheLookup(pthread_t t, bool createMainIfAbsent);
OF_EXTERN OCFRunLoopRef _CFRunLoopGetButDontCreateCurrent(void);
OF_EXTERN void __CFFinalizeRunLoop(void *data);
OF_EXTERN void _CFRunLoopSourceWakeUpRunLoops(OCFRunLoopSourceRef rls);
OF_EXTERN void _CFRunLoopError_RunCalledWithInvalidMode(void);
OF_EXTERN void __CFRunLoopError_AddingSourceLackingReceiveRight(void);

// Non-external natively; hidden.
OF_EXTERN void __CFArmNextTimerInMode(RunLoopMode *rlm, OCFRunLoopRef rl);
OF_EXTERN OCFIndex __CFRunLoopInsertionIndexInTimerArray(OCFArrayRef array, OCFRunLoopTimerRef rlt);

// The builtin class records of this unit (types 43, 44, 45, 46 and 47).
OF_EXTERN const OCFRuntimeClass __CFRunLoopClass;
OF_EXTERN const OCFRuntimeClass __CFRunLoopModeClass;
OF_EXTERN const OCFRuntimeClass __CFRunLoopObserverClass;
OF_EXTERN const OCFRuntimeClass __CFRunLoopSourceClass;
OF_EXTERN const OCFRuntimeClass __CFRunLoopTimerClass;

// Native CFRunLoopPerformBlock, 0x1c01de728 (RunLoop/OCFRunLoop.c). External.
OF_EXPORT void OCFRunLoopPerformBlock(OCFRunLoopRef rl, OCFTypeRef mode, void (^block)(void));
