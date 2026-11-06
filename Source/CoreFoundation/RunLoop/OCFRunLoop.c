//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/RunLoop/OCFRunLoop.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/RunLoop/OCFMachPort.h"
#include "CoreFoundation/RunLoop/OCFRunLoopProbes.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/Date/OCFDatePrivate.h"
#include "CoreFoundation/Collections/OCFSet.h"

#include <Block.h>
#include <dlfcn.h>
#include <mach/mach_time.h>
#include <malloc/malloc.h>
#include <math.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/log.h>
#include <os/object.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/qos.h>

// Inferred organization: the native CFRunLoop functions lie in one cluster of the iOS Simulator __text
// section, CFRunLoopGetCurrent (0x1c01dc778) through __CFRunLoopInsertionIndexInTimerArray (0x1c01e4ea8) and
// the debug-information block at 0x1c01e4f94, with their .cold entries at 0x1c030b8e0-0x1c030bf2c. They are
// kept in one translation unit in native address order; the native source path is not in the evidence.
// Functions of the cluster outside this unit's scope (the observer creation and accessors, the timer
// accessors not restored here, and others) are not restored here. __CFFireTimer and __CFDescribeTimer
// (0x1c01b4bb8, 0x1c01b4c0c) lie outside the cluster and the Mach port shortage reporters at
// 0x1c030b8b0-0x1c030b8dc before its .cold entries; they join this unit by the track's assignment. Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (disassembly); iOS Device 24A446: unread. Reading: Local/Scratch/cf-core/tasks/runloop-streams.txt
// (sub-track reading; no versioned reading yet); CFRunLoopSourceIsValid, _CFRunLoopSourceWakeUpRunLoops and
// __CFRunLoopSourceWakeUpLoop: Local/Evidence/cf-core/iphone-simulator-corpus-delta5.json.

// libsystem and libdispatch interfaces the native image imports and the SDK does not declare (libdispatch
// queue_private.h and the _4CF SPI, xnu kdebug, mk_timer and voucher interfaces).
extern pthread_t pthread_main_thread_np(void);
extern int kdebug_trace(uint32_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);
extern dispatch_queue_t _dispatch_runloop_root_queue_create_4CF(const char *label, unsigned long flags);
extern mach_port_t _dispatch_runloop_root_queue_get_port_4CF(dispatch_queue_t queue);
extern bool _dispatch_runloop_root_queue_perform_4CF(dispatch_queue_t queue);
extern mach_port_t _dispatch_get_main_queue_port_4CF(void);
extern void _dispatch_main_queue_callback_4CF(void *msg);
extern mach_port_name_t mk_timer_create(void);
extern kern_return_t mk_timer_destroy(mach_port_name_t name);
extern kern_return_t mk_timer_arm(mach_port_name_t name, uint64_t expireTime);
extern kern_return_t mk_timer_cancel(mach_port_name_t name, uint64_t *resultTime);
extern void *voucher_copy(void);
extern void objc_release(id object);
extern BOOL objc_opt_respondsToSelector(id object, SEL selector);
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);
extern bool kdebug_is_enabled(uint32_t code);
extern uint64_t kdebug_trace_string(uint32_t debugid, uint64_t stringID, const char *string);
extern id objc_retain(id object);
extern IMP object_getMethodImplementation(id object, SEL name);

// libdispatch's kDispatchQueueOvercommit and libsystem_platform's kUnfairLockDataSynchronization (private).
enum {
    kDispatchQueueOvercommit = 0x2,
    kUnfairLockDataSynchronization = 0x10000,
};

// Callees outside this unit (names as native, hidden) and the hidden functions of
// this unit that other units reach natively. The native callers
// emit nothing after the calls declared noreturn.
OF_EXTERN void __THE_SYSTEM_HAS_NO_PORTS_AVAILABLE__(void) __attribute__((noreturn));
OF_EXTERN void __THE_SYSTEM_HAS_NO_PORT_SETS_AVAILABLE__(kern_return_t ret) __attribute__((noreturn));
OF_EXTERN void __CFDescribeTimer(
    OCFRunLoopTimerRef rlt,
    void *info,
    char *libraryName,
    size_t libraryNameSize,
    char *functionName,
    size_t functionNameSize,
    void **functionAddress
);
OF_EXTERN void __CFFireTimer(OCFRunLoopTimerRef rlt, void *info);

// Native run loop type IDs (the CF string type is 7).
enum {
    kStringTypeID = 7,
    kRunLoopTypeID = 43,
    kRunLoopModeTypeID = 44,
    kRunLoopObserverTypeID = 45,
    kRunLoopSourceTypeID = 46,
    kRunLoopTimerTypeID = 47,
};

// Info bits of sources, observers and timers (bit 3 valid; observers: bit 0 firing, bit 1 repeats, bit 2 may
// fire while firing) and of the run loop (bit 1 sleeping, bit 2 deallocating).
enum {
    kInfoFiring = 0x1,
    kInfoRepeats = 0x2,
    kInfoAllowsRecursion = 0x4,
    kInfoValid = 0x8,
    kRunLoopSleeping = 0x2,
    kRunLoopDeallocating = 0x4,
};

// The per-thread data slots: 4 is set while the main dispatch queue is serviced, 10 holds the thread's run
// loop, 11 its finalization counter, 13 the voucher of the message being handled.
enum {
    kTSDIsInDispatchMainQueue = 4,
    kTSDRunLoop = 10,
    kTSDRunLoopCounter = 11,
    kTSDMachMessageVoucher = 13,
};

static void __CFRunLoopModeDeallocate(OCFTypeRef cf);
static Boolean __CFRunLoopModeEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFRunLoopModeHash(OCFTypeRef cf);
static OCFStringRef __CFRunLoopModeCopyDescription(OCFTypeRef cf);
static void __CFRunLoopDeallocate(OCFTypeRef cf);
static OCFStringRef __CFRunLoopCopyDescription(OCFTypeRef cf);
static OCFRunLoopRef __CFRunLoopCreate(pthread_t t);
static void __CFRunLoopRemoveAllSources(OCFRunLoopRef rl, OCFStringRef modeName);
static void __CFRunLoopGetModeName(const void *value, void *context);
static RunLoopMode *__CFRunLoopCopyMode(OCFRunLoopRef rl, OCFStringRef modeName, Boolean create);
static bool __CFRunLoopModeIsEmpty(OCFRunLoopRef rl, RunLoopMode *rlm);
static void __CFRunLoopAddItemToCommonModes(const void *value, void *ctx);
static void __CFRunLoopRemoveItemFromCommonModes(const void *value, void *ctx);
static void __CFRunLoopObserverSchedule(OCFRunLoopObserverRef rlo, OCFRunLoopRef rl);
static void __CFRunLoopObserverCancel(OCFRunLoopObserverRef rlo);
static void __CFRunLoopSourceDeallocate(OCFTypeRef cf);
static Boolean __CFRunLoopSourceEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFRunLoopSourceHash(OCFTypeRef cf);
static OCFStringRef __CFRunLoopSourceCopyDescription(OCFTypeRef cf);
static void __CFRunLoopSourceRemoveFromRunLoop(const void *value, void *context);
static void __CFRunLoopSourceWakeUpLoop(const void *value, void *context);
static void
__CFRunLoopObserverWithHandlerPerform(OCFRunLoopObserverRef rlo, OCFRunLoopActivity activity, void *info);
static void __CFRunLoopCleanseSources(const void *value, void *context);
static void __CFRepositionTimerInMode(RunLoopMode *rlm, OCFRunLoopTimerRef rlt, bool isInArray);
static void __CFRunLoopObserverDeallocate(OCFTypeRef cf);
static OCFStringRef __CFRunLoopObserverCopyDescription(OCFTypeRef cf);
static void __CFRunLoopTimerDeallocate(OCFTypeRef cf);
static OCFStringRef __CFRunLoopTimerCopyDescription(OCFTypeRef cf);
static void _runLoopTimerWithBlockContext(OCFRunLoopTimerRef rlt, void *info);
static void __cftrace_runloop_trace_iteration_start(OCFRunLoopRef rl);
static void timerDebugInfo(
    OCFRunLoopTimerRef rlt, void *info, char *libraryName, char *functionName, void **functionAddress
);
static void __CFRunLoopDeallocateSources(const void *value, void *context);
static void __CFRunLoopDeallocateObservers(const void *value, void *context);
static void __CFRunLoopDeallocateTimers(const void *value, void *context);
static void __CFRunLoopKillOneTimer(const void *value, void *context);
static void _CFRunLoopRunCleanup(void *context);
static void __CFRunLoopDoObservers(OCFRunLoopRef rl, RunLoopMode *rlm, OCFRunLoopActivity activity);
static int32_t
__CFRunLoopRun(OCFRunLoopRef rl, RunLoopMode *rlm, OCFOptionFlags options, OCFTimeInterval seconds);
static void __CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__(
    OCFRunLoopObserverCallBack func, OCFRunLoopObserverRef observer, OCFRunLoopActivity activity, void *info
) __attribute__((noinline));
static void __CFRunLoopPerCalloutARPEnd(void *pool);
static void __CFRunLoopDoBlocks(OCFRunLoopRef rl, RunLoopMode *rlm, uint32_t kind);
static bool __CFRunLoopDoSources0(OCFRunLoopRef rl, RunLoopMode *rlm, uint32_t stopAfterHandle);
static bool __CFRunLoopServiceMachPort(
    mach_port_name_t port,
    mach_msg_header_t **buffer,
    mach_port_t *livePort,
    mach_msg_timeout_t timeout,
    voucher_mach_msg_state_t *voucherState,
    void **voucherCopy
);
static bool __CFRunLoopDoTimers(OCFRunLoopRef rl, RunLoopMode *rlm, uint64_t limitTSR);
static void __CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__(void *msg) __attribute__((noinline));
static bool __CFRunLoopDoSource1(
    OCFRunLoopRef rl,
    RunLoopMode *rlm,
    OCFRunLoopSourceRef rls,
    mach_msg_header_t *msg,
    OCFIndex size,
    mach_msg_header_t **reply
);
static void __CFRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__(void (^block)(void)) __attribute__((noinline));
static void __CFRunLoopCollectSources0(const void *value, void *context);
static bool __CFRunLoopDoSource0(OCFRunLoopRef rl, OCFRunLoopSourceRef rls);
static OCFComparisonResult __CFRunLoopSourceComparator(const void *val1, const void *val2, void *context);
static void __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__(void (*perform)(void *), void *info)
    __attribute__((noinline));
static Boolean __CFRunLoopDoTimer(OCFRunLoopRef rl, RunLoopMode *rlm, OCFRunLoopTimerRef rlt);
static void __CFRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__(
    OCFRunLoopTimerCallBack func, OCFRunLoopTimerRef timer, void *info
) __attribute__((noinline));
static void __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__(
    void *(*perform)(void *, OCFIndex, OCFAllocatorRef, void *),
    mach_msg_header_t *msg,
    OCFIndex size,
    mach_msg_header_t **reply,
    void *info
) __attribute__((noinline));
static void __CFRunLoopRemoveSourcesFromCommonMode(const void *value, void *ctx);
static void __CFRunLoopRemoveSourceFromMode(const void *value, void *ctx);

// The mode name constant strings. The native code compares mode names with the addresses of these two
// compiled constant strings (coalesced by the linker); each is one object here so that the comparisons hold.
// The values spell the mapped names (kCFRunLoopDefaultMode and kCFRunLoopCommonModes in the system-name
// build).
static const OCFConstantString defaultModeString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(kOCFRunLoopDefaultMode),
    sizeof(OF_NAME_STRING(kOCFRunLoopDefaultMode)) - 1
};
static const OCFConstantString commonModesString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(kOCFRunLoopCommonModes),
    sizeof(OF_NAME_STRING(kOCFRunLoopCommonModes)) - 1
};

#define kDefaultModeName ((OCFStringRef) & defaultModeString)
#define kCommonModesName ((OCFStringRef) & commonModesString)

// Native kCFRunLoopDefaultMode and kCFRunLoopCommonModes, 0x2227a3d70 and 0x2227a3d78 (__DATA_CONST,__const).
// External; the constant strings "kCFRunLoopDefaultMode" and "kCFRunLoopCommonModes" (0x2227b0fa0,
// 0x2227b0fc0).
const OCFRunLoopMode kOCFRunLoopDefaultMode = kDefaultModeName;
const OCFRunLoopMode kOCFRunLoopCommonModes = kCommonModesName;

// Native __CFRunLoopModeClass, 0x2227a3d80 (__DATA_CONST,__const). Hidden (was private external); version 0,
// name "CFRunLoopMode", finalize, equal, hash and the debug description.
const OCFRuntimeClass __CFRunLoopModeClass = {
    0,
    OF_NAME_STRING(OCFRunLoopMode),
    NULL,
    NULL,
    __CFRunLoopModeDeallocate,
    __CFRunLoopModeEqual,
    __CFRunLoopModeHash,
    NULL,
    __CFRunLoopModeCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CFRunLoopClass, 0x2227a3de0 (__DATA_CONST,__const). Hidden (was private external); version 0, name
// "CFRunLoop", finalize and the debug description.
const OCFRuntimeClass __CFRunLoopClass = {
    0,
    "CFRunLoop",
    NULL,
    NULL,
    __CFRunLoopDeallocate,
    NULL,
    NULL,
    NULL,
    __CFRunLoopCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CFRunLoopSourceClass, 0x2227a3e80 (__DATA_CONST,__const). Hidden (was private external); version
// 1, name "CFRunLoopSource", finalize, equal, hash and the debug description.
const OCFRuntimeClass __CFRunLoopSourceClass = {
    1,
    "CFRunLoopSource",
    NULL,
    NULL,
    __CFRunLoopSourceDeallocate,
    __CFRunLoopSourceEqual,
    __CFRunLoopSourceHash,
    NULL,
    __CFRunLoopSourceCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CFRunLoopObserverClass, 0x2227a3ee0 (__DATA_CONST,__const). Hidden (the builtin table's record for
// type 45); version 0, name "CFRunLoopObserver", finalize and the debug description.
const OCFRuntimeClass __CFRunLoopObserverClass = {
    0,
    "CFRunLoopObserver",
    NULL,
    NULL,
    __CFRunLoopObserverDeallocate,
    NULL,
    NULL,
    NULL,
    __CFRunLoopObserverCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CFRunLoopTimerClass, 0x2227a3f40 (__DATA_CONST,__const). Hidden (the builtin table's record for
// type 47); version 0, name "CFRunLoopTimer", finalize and the debug description.
const OCFRuntimeClass __CFRunLoopTimerClass = {
    0,
    "CFRunLoopTimer",
    NULL,
    NULL,
    __CFRunLoopTimerDeallocate,
    NULL,
    NULL,
    NULL,
    __CFRunLoopTimerCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CFRunLoops and loopsLock, 0x22e6d2ec8 and 0x22e6d2ec0 (__DATA,__bss). Non-external; the dictionary
// of run loops by thread, created once and published with a compare-and-swap, and its lock.
static OCFMutableDictionaryRef __CFRunLoops;
static os_unfair_lock loopsLock;

// Native __MergedGlobals_3, 0x22e6d2f10 (__DATA,__bss; the first word of a merged-globals block whose second
// word is the assertion once token 0x22e6d2f18). Non-external; the main thread's run loop, set by
// CFRunLoopGetMain's once block. The name is a local descriptive spelling.
static OCFRunLoopRef mainRunLoop;
static dispatch_once_t mainThreadExitedLogOnceToken;

// The helpers below reproduce code the native functions inline at each use (no native symbols): the fork
// check
// (__CF_FORK_STATE is read as a volatile byte), the type assertion, the main-run-loop assertion of the
// .cold.1 entries (its block is native ___CF_RUNLOOP_ASSERTIONS_block_invoke, 0x1c01e29ac, and its .cold.1,
// 0x1c030be30; the dispatch_once call is _OUTLINED_FUNCTION_0_23, 0x1c01e5030), the info-word atomics and the
// per-run arrays.
static inline void checkForFork(void) {
    if ((*(volatile uint8_t *)&__CF_FORK_STATE & 2) == 0) {
        __CF_USED();
    }
    if ((*(volatile uint8_t *)&__CF_FORK_STATE & 1) != 0) {
        __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__();
    }
}

static inline void assertTypeID(OCFTypeRef cf, OCFTypeID expected) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(cf);
    if (typeID != expected) {
        _CFAssertMismatchedTypeID(expected, typeID);
    }
}

static inline void runLoopAssertions(OCFRunLoopRef rl) {
    if ((__atomic_load_n(&__CFMainThreadHasExited, __ATOMIC_SEQ_CST) & 1) == 0) {
        return;
    }
    if (mainRunLoop != NULL && rl == mainRunLoop) {
        dispatch_once(&mainThreadExitedLogOnceToken, ^{
          os_log_error(
              _CFOSLog(),
              "Attempting to use the main runloop, but the main thread has exited. This message will only "
              "log "
              "once."
          );
        });
    }
}

static inline uint64_t loadInfo(OCFTypeRef cf) {
    return __atomic_load_n(&((OCFRuntimeBase *)cf)->info, __ATOMIC_SEQ_CST);
}

static inline void setInfoBits(OCFTypeRef cf, uint64_t bits) {
    uint64_t *word = &((OCFRuntimeBase *)cf)->info;
    uint64_t info = __atomic_load_n(word, __ATOMIC_SEQ_CST);
    while (
        !__atomic_compare_exchange_n(word, &info, info | bits, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
    }
}

static inline void clearInfoBits(OCFTypeRef cf, uint64_t bits) {
    uint64_t *word = &((OCFRuntimeBase *)cf)->info;
    uint64_t info = __atomic_load_n(word, __ATOMIC_SEQ_CST);
    while (
        !__atomic_compare_exchange_n(word, &info, info & ~bits, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
    }
}

// The per-run entries are addressed from the base of each array, as natively; index 254 (the deepest nesting
// __CFRunLoopRunSpecificWithOptions admits) reaches the first byte after each 254-byte array.
static inline uint8_t *stoppedFlag(OCFRunLoopRef rl, uint8_t idx) {
    return (uint8_t *)rl + offsetof(struct __OCFRunLoop, stopped) + idx;
}

static inline uint8_t *ignoreWakeUpsFlag(OCFRunLoopRef rl, uint8_t idx) {
    return (uint8_t *)rl + offsetof(struct __OCFRunLoop, ignoreWakeUps) + idx;
}

// Native __CFFireTimer, 0x1c01b4bb8. Hidden (was a private external). It lies among the bridged
// Objective-C classes' methods and calls objc_retain_x19/_x20 and objc_release_x20/_x19, the code ARC emits,
// so the native unit is Objective-C; restored here, with the timer callbacks it serves, as C with the same
// libobjc calls in the same order. The information is {?, target, selector}: inside an autorelease pool the
// timer and the target are retained, the target is sent the selector with the timer, then the target and the
// timer are released and the pool is popped (a tail call).
void __CFFireTimer(OCFRunLoopTimerRef rlt, void *info) {
    id target = ((id *)info)[1];
    SEL selector = ((SEL *)info)[2];
    void *pool = _OCFAutoreleasePoolPush();
    objc_retain((id)rlt);
    objc_retain(target);
    ((void (*)(id, SEL, OCFRunLoopTimerRef))objc_msgSend)(target, selector, rlt);
    objc_release(target);
    objc_release((id)rlt);
    _OCFAutoreleasePoolPop(pool);
}

// Native __CFDescribeTimer, 0x1c01b4c0c. Hidden (was a private external); beside __CFFireTimer (see there for
// the native language). The target's class name and the selector's name ("<unknown name>" and
// "<unknown selector>" when NULL) fill functionName as "(CFTimer) %s %s"; the method implementation is stored
// first when functionAddress is given. The library is the dladdr image of the target's class
// ("<unknown library>" without a path); a failed dladdr leaves libraryName untouched.
void __CFDescribeTimer(
    OCFRunLoopTimerRef rlt,
    void *info,
    char *libraryName,
    size_t libraryNameSize,
    char *functionName,
    size_t functionNameSize,
    void **functionAddress
) {
    (void)rlt;
    id target = ((id *)info)[1];
    SEL selector = ((SEL *)info)[2];
    const char *className = object_getClassName(target);
    const char *selectorName = sel_getName(selector);
    if (functionAddress != NULL) {
        *functionAddress = (void *)object_getMethodImplementation(target, selector);
    }
    snprintf(
        functionName,
        functionNameSize,
        "(CFTimer) %s %s",
        className != NULL ? className : "<unknown name>",
        selectorName != NULL ? selectorName : "<unknown selector>"
    );
    Class cls = object_getClass(target);
    Dl_info dlInfo;
    memset(&dlInfo, 0, sizeof(dlInfo));
    if (dladdr((const void *)cls, &dlInfo) != 0) {
        snprintf(
            libraryName,
            libraryNameSize,
            "%s",
            dlInfo.dli_fname != NULL ? dlInfo.dli_fname : "<unknown library>"
        );
    }
}

// Native CFRunLoopGetCurrent, 0x1c01dc778. External. The run loop in per-thread slot 10, else _CFRunLoopGet0.
OCFRunLoopRef OCFRunLoopGetCurrent(void) {
    checkForFork();
    OCFRunLoopRef rl = (OCFRunLoopRef)_OCFGetTSD(kTSDRunLoop);
    if (rl != NULL) {
        return rl;
    }
    return _OCFRunLoopGet0(pthread_self());
}

// Native __CFRunLoopModeDeallocate, 0x1c01dc7c8. Non-external. Releases the five collections and the name,
// drops the port-set right (logging a failure), cancels and releases the timer source, releases the queue,
// destroys the timer port, destroys the lock and fills bytes 16-199 with 0x7c.
static void __CFRunLoopModeDeallocate(OCFTypeRef cf) {
    RunLoopMode *rlm = (RunLoopMode *)cf;
    if (rlm->sources0 != NULL) {
        OCFRelease(rlm->sources0);
    }
    if (rlm->sources1 != NULL) {
        OCFRelease(rlm->sources1);
    }
    if (rlm->observers != NULL) {
        OCFRelease(rlm->observers);
    }
    if (rlm->timers != NULL) {
        OCFRelease(rlm->timers);
    }
    if (rlm->portToV1SourceMap != NULL) {
        OCFRelease(rlm->portToV1SourceMap);
    }
    OCFRelease(rlm->name);
    mach_port_t portSet = rlm->portSet;
    kern_return_t ret = mach_port_mod_refs(mach_task_self(), portSet, MACH_PORT_RIGHT_PORT_SET, -1);
    if (ret != KERN_SUCCESS) {
        OCFLog(
            3,
            OF_STR("error (%d - %s) while trying to free port set: %d"),
            ret,
            mach_error_string(ret),
            portSet
        );
    }
    if (rlm->timerSource != NULL) {
        dispatch_source_cancel(rlm->timerSource);
        dispatch_release(rlm->timerSource);
    }
    if (rlm->queue != NULL) {
        dispatch_release(rlm->queue);
    }
    if (rlm->timerPort != MACH_PORT_NULL) {
        mk_timer_destroy(rlm->timerPort);
    }
    pthread_mutex_destroy(&rlm->lock);
    memset((uint8_t *)rlm + 16, 0x7c, sizeof(RunLoopMode) - 16);
}

// Native __CFRunLoopModeEqual, 0x1c01dc8d4. Non-external; a tail call comparing the names.
static Boolean __CFRunLoopModeEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    return OCFEqual(((const RunLoopMode *)cf1)->name, ((const RunLoopMode *)cf2)->name);
}

// Native __CFRunLoopModeHash, 0x1c01dc8e0. Non-external; a tail call hashing the name.
static OCFHashCode __CFRunLoopModeHash(OCFTypeRef cf) {
    return OCFHash(((const RunLoopMode *)cf)->name);
}

// Native __CFRunLoopModeCopyDescription, 0x1c01dc8e8. Non-external. The deadline intervals are computed from
// three mach_absolute_time calls after the current absolute time, in this order.
static OCFStringRef __CFRunLoopModeCopyDescription(OCFTypeRef cf) {
    const RunLoopMode *rlm = cf;
    OCFMutableStringRef result = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    OCFStringAppendFormat(
        result, NULL, OF_STR("<CFRunLoopMode %p [%p]>{name = %@, "), rlm, OCFGetAllocator(rlm), rlm->name
    );
    OCFStringAppendFormat(result, NULL, OF_STR("port set = 0x%x, "), rlm->portSet);
    OCFStringAppendFormat(result, NULL, OF_STR("queue = %p, "), rlm->queue);
    OCFStringAppendFormat(
        result, NULL, OF_STR("source = %p (%s), "), rlm->timerSource, rlm->timerFired ? "fired" : "not fired"
    );
    OCFStringAppendFormat(result, NULL, OF_STR("timer port = 0x%x, "), rlm->timerPort);
    OCFMutableSetRef sources0 = rlm->sources0;
    OCFMutableSetRef sources1 = rlm->sources1;
    OCFMutableArrayRef observers = rlm->observers;
    OCFMutableArrayRef timers = rlm->timers;
    OCFAbsoluteTime now = OCFAbsoluteTimeGetCurrent();
    uint64_t nowTSR = mach_absolute_time();
    uint64_t softDeadline = rlm->timerSoftDeadline;
    OCFTimeInterval softInterval = __CFTSRToTimeInterval(softDeadline - mach_absolute_time());
    uint64_t hardDeadline = rlm->timerHardDeadline;
    OCFTimeInterval hardInterval = __CFTSRToTimeInterval(hardDeadline - mach_absolute_time());
    OCFStringAppendFormat(
        result,
        NULL,
        OF_STR(
            "\n\tsources0 = %@,\n\tsources1 = %@,\n\tobservers = %@,\n\ttimers = %@,\n\tcurrently %0.09g "
            "(%lld) / "
            "soft deadline in: %0.09g sec (@ %lld) / hard deadline in: %0.09g sec (@ %lld)\n},\n"
        ),
        sources0,
        sources1,
        observers,
        timers,
        now,
        nowTSR,
        softInterval,
        softDeadline,
        hardInterval,
        rlm->timerHardDeadline
    );
    return result;
}

// Native __CFRunLoopDeallocate, 0x1c01dca78, with __CFRunLoopDeallocate.cold.1 (0x1c030b8e0). Non-external. A
// run loop is deallocated only from its thread's destructor (the flag at 2712, cleared with a release store);
// the main run loop is never deallocated (BRK #1). The deallocating bit is set, the four mode appliers run,
// the block list is freed under the lock, the collections are released and the wake-up port is destructed
// with the run loop as its guard (a failure stores the formatted message and traps); bytes 16-2719 are filled
// with 0x8c.
static void __CFRunLoopDeallocate(OCFTypeRef cf) {
    OCFRunLoopRef rl = (OCFRunLoopRef)cf;
    if (__atomic_load_n(&rl->fromThreadDestructor, __ATOMIC_SEQ_CST) == 0) {
        crashWithMessage(
            "Attempting to deallocate CFRunLoop outside of thread destructor -- this is likely an "
            "over-release of "
            "the run loop"
        );
    }
    kdebug_trace(0x211200F2, (uintptr_t)rl, 0, 0, 0);
    __atomic_store_n(&rl->fromThreadDestructor, 0, __ATOMIC_SEQ_CST);
    if (_OCFRunLoopGet0b(pthread_main_thread_np()) == rl) {
        __builtin_trap();
    }
    setInfoBits(rl, kRunLoopDeallocating);
    if (rl->modes != NULL) {
        OCFSetApplyFunction(rl->modes, __CFRunLoopCleanseSources, rl);
        OCFSetApplyFunction(rl->modes, __CFRunLoopDeallocateSources, rl);
        OCFSetApplyFunction(rl->modes, __CFRunLoopDeallocateObservers, rl);
        OCFSetApplyFunction(rl->modes, __CFRunLoopDeallocateTimers, rl);
    }
    pthread_mutex_lock(&rl->lock);
    RunLoopBlockItem *item = rl->blocksHead;
    while (item != NULL) {
        RunLoopBlockItem *next = item->next;
        OCFRelease(item->mode);
        _Block_release(item->block);
        free(item);
        item = next;
    }
    if (rl->commonModeItems != NULL) {
        OCFRelease(rl->commonModeItems);
    }
    if (rl->commonModes != NULL) {
        OCFRelease(rl->commonModes);
    }
    if (rl->modes != NULL) {
        OCFRelease(rl->modes);
    }
    mach_port_t wakeUpPort = rl->wakeUpPort;
    kern_return_t ret = mach_port_destruct(mach_task_self(), wakeUpPort, -1, (mach_port_context_t)rl);
    if (ret != KERN_SUCCESS) {
        char msg[256];
        snprintf(msg, 256, "*** Unable to destruct port. (0x%x, %d, %p) ***", wakeUpPort, ret, rl);
        crashWithMessage(msg);
    }
    rl->wakeUpPort = MACH_PORT_NULL;
    pthread_mutex_unlock(&rl->lock);
    pthread_mutex_destroy(&rl->lock);
    memset((uint8_t *)rl + 16, 0x8c, sizeof(struct __OCFRunLoop) - 16);
}

// Native __CFRunLoopCopyDescription, 0x1c01dcc4c. Non-external. The stopped and ignore-wake-ups flags of the
// current nesting index.
static OCFStringRef __CFRunLoopCopyDescription(OCFTypeRef cf) {
    OCFRunLoopRef rl = (OCFRunLoopRef)cf;
    OCFMutableStringRef result = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    OCFAllocatorRef allocator = OCFGetAllocator(rl);
    uint8_t stopped = *stoppedFlag(rl, rl->nestingIndex);
    uint8_t ignoreWakeUps = *ignoreWakeUpsFlag(rl, rl->nestingIndex);
    OCFStringRef currentModeName = rl->currentMode != NULL ? rl->currentMode->name : OF_STR("(none)");
    OCFStringAppendFormat(
        result,
        NULL,
        OF_STR(
            "<CFRunLoop %p [%p]>{wakeup port = 0x%x, stopped = %s, ignoreWakeUps = %s, \ncurrent mode = %@,\n"
        ),
        rl,
        allocator,
        rl->wakeUpPort,
        stopped ? "true" : "false",
        ignoreWakeUps ? "true" : "false",
        currentModeName
    );
    OCFStringAppendFormat(
        result,
        NULL,
        OF_STR("common modes = %@,\ncommon mode items = %@,\nmodes = %@}\n"),
        rl->commonModes,
        rl->commonModeItems,
        rl->modes
    );
    return result;
}

// Native _CFRunLoopCacheLookup, 0x1c01dcd2c. Hidden (was private external). A NULL thread means the main
// thread. When createMainIfAbsent is set and no dictionary exists, the lock is dropped while the dictionary
// and the main thread's run loop are created and published, then retaken.
OCFRunLoopRef _CFRunLoopCacheLookup(pthread_t t, bool createMainIfAbsent) {
    if (pthread_equal(t, NULL)) {
        t = pthread_main_thread_np();
    }
    os_unfair_lock_lock(&loopsLock);
    OCFMutableDictionaryRef loops = __CFRunLoops;
    if (createMainIfAbsent && loops == NULL) {
        os_unfair_lock_unlock(&loopsLock);
        OCFMutableDictionaryRef dict = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, NULL, &kOCFTypeDictionaryValueCallBacks
        );
        OCFRunLoopRef mainLoop = __CFRunLoopCreate(pthread_main_thread_np());
        OCFDictionarySetValue(dict, pthread_main_thread_np(), mainLoop);
        OCFMutableDictionaryRef expected = NULL;
        if (!__atomic_compare_exchange_n(
                &__CFRunLoops, &expected, dict, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
            )) {
            OCFRelease(dict);
        }
        OCFRelease(mainLoop);
        os_unfair_lock_lock(&loopsLock);
        loops = __CFRunLoops;
    }
    OCFRunLoopRef result = NULL;
    if (loops != NULL) {
        result = (OCFRunLoopRef)OCFDictionaryGetValue(loops, t);
    }
    os_unfair_lock_unlock(&loopsLock);
    return result;
}

// Native __CFRunLoopCreate, 0x1c01dce34, with __CFRunLoopCreate.cold.1 (0x1c030b8f4). Non-external. A
// 2,704-byte extra instance on the system allocator: a recursive lock, a wake-up port constructed with the
// run loop as its guard context (options 0x33, queue limit 1; a failure formats its message on the stack and
// calls
// __THE_SYSTEM_HAS_NO_PORTS_AVAILABLE__; a zero name traps), the nesting index and first stop flag cleared
// and the first ignore-wake-ups flag set, the common modes holding the default mode, the modes, the thread,
// the timer lock, the per-callout pool flag set and the default mode created.
static OCFRunLoopRef __CFRunLoopCreate(pthread_t t) {
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFRunLoopRef loop = (OCFRunLoopRef)_OCFRuntimeCreateInstance(
        allocator, kRunLoopTypeID, sizeof(struct __OCFRunLoop) - 16, NULL
    );
    if (loop == NULL) {
        return NULL;
    }
    pthread_mutexattr_t attr = {0};
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&loop->lock, &attr);
    pthread_mutexattr_destroy(&attr);
    mach_port_name_t port = MACH_PORT_NULL;
    mach_port_options_t options = {0};
    options.flags = 0x33;
    options.mpl.mpl_qlimit = 1;
    kern_return_t ret = mach_port_construct(mach_task_self(), &options, (mach_port_context_t)loop, &port);
    if (ret != KERN_SUCCESS) {
        char msg[256];
        snprintf(
            msg,
            256,
            "*** The system has no mach ports available. You may be able to diagnose which application(s) "
            "are using "
            "ports by using 'top' or Activity Monitor. (%d) ***",
            ret
        );
        gCRAnnotations[1] = (uint64_t)(uintptr_t)msg;
        __THE_SYSTEM_HAS_NO_PORTS_AVAILABLE__();
    }
    loop->wakeUpPort = port;
    if (port == MACH_PORT_NULL) {
        __builtin_trap();
    }
    loop->nestingIndex = 0;
    loop->stopped[0] = 0;
    loop->ignoreWakeUps[0] = 1;
    loop->commonModes = OCFSetCreateMutable(allocator, 0, &kOCFTypeSetCallBacks);
    OCFSetAddValue(loop->commonModes, kDefaultModeName);
    loop->modes = OCFSetCreateMutable(allocator, 0, &kOCFTypeSetCallBacks);
    loop->pthread = t;
    loop->timerTSRLock = OS_UNFAIR_LOCK_INIT;
    loop->perCalloutARP = 1;
    loop->didDispatchMainQueue = 0;
    RunLoopMode *rlm = __CFRunLoopCopyMode(loop, kDefaultModeName, true);
    if (rlm != NULL) {
        OCFRelease(rlm);
    }
    return loop;
}

// Native _CFRunLoopGetButDontCreateCurrent, 0x1c01dcfc0. Hidden (was private external). Per-thread slot 10
// without creating the table, else the cached run loop of the current thread.
OCFRunLoopRef _CFRunLoopGetButDontCreateCurrent(void) {
    OCFRunLoopRef rl = (OCFRunLoopRef)_OCFGetTSDCreateIfNeeded(kTSDRunLoop, false);
    if (rl != NULL) {
        return rl;
    }
    return _CFRunLoopCacheLookup(pthread_self(), false);
}

// Native _CFRunLoopGet0, 0x1c01dd01c. External. A NULL thread means the main thread. Unlike
// _CFRunLoopCacheLookup the lock is held while the dictionary is created. A missing run loop is created and
// stored under the lock; for the current thread it is stored in slot 10 and the finalization counter (slot
// 11) starts at 3.
OCFRunLoopRef _OCFRunLoopGet0(pthread_t t) {
    if (pthread_equal(t, NULL)) {
        t = pthread_main_thread_np();
    }
    os_unfair_lock_lock(&loopsLock);
    OCFMutableDictionaryRef loops = __CFRunLoops;
    if (loops == NULL) {
        OCFMutableDictionaryRef dict = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, NULL, &kOCFTypeDictionaryValueCallBacks
        );
        OCFRunLoopRef mainLoop = __CFRunLoopCreate(pthread_main_thread_np());
        OCFDictionarySetValue(dict, pthread_main_thread_np(), mainLoop);
        OCFMutableDictionaryRef expected = NULL;
        if (!__atomic_compare_exchange_n(
                &__CFRunLoops, &expected, dict, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
            )) {
            OCFRelease(dict);
        }
        OCFRelease(mainLoop);
        loops = __CFRunLoops;
    }
    OCFRunLoopRef loop = (OCFRunLoopRef)OCFDictionaryGetValue(loops, t);
    if (loop != NULL) {
        os_unfair_lock_unlock(&loopsLock);
    } else {
        loop = __CFRunLoopCreate(t);
        kdebug_trace(0x211200F1, (uintptr_t)loop, 0, 0, 0);
        OCFDictionarySetValue(__CFRunLoops, t, loop);
        os_unfair_lock_unlock(&loopsLock);
        if (loop != NULL) {
            OCFRelease(loop);
        }
    }
    if (pthread_equal(t, pthread_self())) {
        _OCFSetTSD(kTSDRunLoop, loop, NULL);
        if (_OCFGetTSD(kTSDRunLoopCounter) == NULL) {
            _OCFSetTSD(kTSDRunLoopCounter, (void *)3, __CFFinalizeRunLoop);
        }
    }
    return loop;
}

// Native __CFFinalizeRunLoop, 0x1c01dd194. Hidden (was private external). The per-thread destructor of slot
// 11: counts down to 1, then removes the thread's run loop from the dictionary; a run loop other than the
// main one loses its counterpart and every source of every mode (last mode first, then the common modes). The
// destructor flag is set with a release store before the final release.
void __CFFinalizeRunLoop(void *value) {
    uintptr_t data = (uintptr_t)value;
    if (data > 1) {
        _OCFSetTSD(kTSDRunLoopCounter, (void *)(data - 1), __CFFinalizeRunLoop);
        return;
    }
    os_unfair_lock_lock(&loopsLock);
    OCFMutableDictionaryRef loops = __CFRunLoops;
    if (loops == NULL) {
        os_unfair_lock_unlock(&loopsLock);
        return;
    }
    OCFRunLoopRef rl = (OCFRunLoopRef)OCFDictionaryGetValue(loops, pthread_self());
    if (rl == NULL) {
        OCFDictionaryRemoveValue(__CFRunLoops, pthread_self());
        os_unfair_lock_unlock(&loopsLock);
        return;
    }
    OCFRetain(rl);
    OCFDictionaryRemoveValue(__CFRunLoops, pthread_self());
    os_unfair_lock_unlock(&loopsLock);
    if (_OCFRunLoopGet0b(pthread_main_thread_np()) != rl) {
        if (rl->counterpart != NULL) {
            OCFRelease(rl->counterpart);
            rl->counterpart = NULL;
        }
        OCFArrayRef array = OCFRunLoopCopyAllModes(rl);
        for (OCFIndex idx = OCFArrayGetCount(array); idx--;) {
            OCFStringRef modeName = OCFArrayGetValueAtIndex(array, idx);
            __CFRunLoopRemoveAllSources(rl, modeName);
        }
        __CFRunLoopRemoveAllSources(rl, kCommonModesName);
        OCFRelease(array);
    }
    __atomic_store_n(&rl->fromThreadDestructor, 1, __ATOMIC_SEQ_CST);
    OCFRelease(rl);
}

// Native _CFRunLoopGet0b, 0x1c01dd2dc. External. The registered run loop of a thread (NULL: the main thread),
// or NULL; never creates one.
OCFRunLoopRef _OCFRunLoopGet0b(pthread_t t) {
    if (pthread_equal(t, NULL)) {
        t = pthread_main_thread_np();
    }
    os_unfair_lock_lock(&loopsLock);
    OCFRunLoopRef loop = NULL;
    if (__CFRunLoops != NULL) {
        loop = (OCFRunLoopRef)OCFDictionaryGetValue(__CFRunLoops, t);
    }
    os_unfair_lock_unlock(&loopsLock);
    return loop;
}

// Native CFRunLoopCopyAllModes, 0x1c01dd348. External. The type assertion precedes the fork check.
OCFArrayRef OCFRunLoopCopyAllModes(OCFRunLoopRef rl) {
    assertTypeID(rl, kRunLoopTypeID);
    checkForFork();
    pthread_mutex_lock(&rl->lock);
    OCFMutableArrayRef array =
        OCFArrayCreateMutable(kOCFAllocatorSystemDefault, OCFSetGetCount(rl->modes), &kOCFTypeArrayCallBacks);
    OCFSetApplyFunction(rl->modes, __CFRunLoopGetModeName, array);
    pthread_mutex_unlock(&rl->lock);
    return array;
}

// Native __CFRunLoopRemoveAllSources, 0x1c01dd3ec. Non-external. For the common modes (compared by address),
// every common mode loses its sources when common items exist; otherwise the mode's version 0 and version 1
// sources are removed from copies of their sets.
static void __CFRunLoopRemoveAllSources(OCFRunLoopRef rl, OCFStringRef modeName) {
    checkForFork();
    pthread_mutex_lock(&rl->lock);
    if (modeName == kCommonModesName) {
        if (rl->commonModeItems != NULL && rl->commonModes != NULL) {
            OCFSetRef set = OCFSetCreateCopy(kOCFAllocatorSystemDefault, rl->commonModes);
            if (set != NULL) {
                OCFSetApplyFunction(set, __CFRunLoopRemoveSourcesFromCommonMode, rl);
                OCFRelease(set);
            }
        }
    } else {
        RunLoopMode *rlm = __CFRunLoopCopyMode(rl, modeName, false);
        if (rlm != NULL) {
            pthread_mutex_lock(&rlm->lock);
            if (rlm->sources0 != NULL) {
                OCFSetRef set = OCFSetCreateCopy(kOCFAllocatorSystemDefault, rlm->sources0);
                const void *context[2] = {rl, modeName};
                OCFSetApplyFunction(set, __CFRunLoopRemoveSourceFromMode, context);
                OCFRelease(set);
            }
            if (rlm->sources1 != NULL) {
                OCFSetRef set = OCFSetCreateCopy(kOCFAllocatorSystemDefault, rlm->sources1);
                const void *context[2] = {rl, modeName};
                OCFSetApplyFunction(set, __CFRunLoopRemoveSourceFromMode, context);
                OCFRelease(set);
            }
            pthread_mutex_unlock(&rlm->lock);
            OCFRelease(rlm);
        }
    }
    pthread_mutex_unlock(&rl->lock);
}

// Native _CFRunLoopGet2, 0x1c01dd570, with _CFRunLoopGet2.cold.1 (0x1c030b928) (corpus delta capture 7).
// External. The run loop's Foundation counterpart (0xa90), under loopsLock: an existing one is answered
// after the unlock. Otherwise NSRunLoop is looked up by name; when the class exists and answers
// +respondsToSelector: for the selector at 0x2387ac2ba (outside the image; read as _new:, the selector of the
// message that follows; confidence: medium), +_new: with the run loop is retained (CFRetain, NULL kept) and
// stored as the counterpart; without the class or the selector the stored counterpart (NULL) is reread. A
// NULL counterpart after the unlock is the .cold.1 crash ("Foundation-only CFRunLoop function called without
// Foundation present", stored as the crash message, then BRK #1). NSRunLoop and +_new: are Foundation's (an
// edge into Foundation).
OCFTypeRef _OCFRunLoopGet2(OCFRunLoopRef rl) {
    os_unfair_lock_lock(&loopsLock);
    OCFTypeRef counterpart = rl->counterpart;
    if (counterpart != NULL) {
        os_unfair_lock_unlock(&loopsLock);
        return counterpart;
    }
    Class runLoopClass = objc_lookUpClass(OF_NAME_STRING(ONSRunLoop));
    if (runLoopClass != Nil && objc_opt_respondsToSelector((id)runLoopClass, sel_getUid("_new:"))) {
        OCFTypeRef created =
            ((OCFTypeRef (*)(id, SEL, OCFRunLoopRef))objc_msgSend)((id)runLoopClass, sel_getUid("_new:"), rl);
        counterpart = created != NULL ? OCFRetain(created) : NULL;
        rl->counterpart = counterpart;
    } else {
        counterpart = rl->counterpart;
    }
    os_unfair_lock_unlock(&loopsLock);
    if (counterpart == NULL) {
        crashWithMessage("Foundation-only CFRunLoop function called without Foundation present");
    }
    return counterpart;
}

// Native CFRunLoopGetMain, 0x1c01dd620, with CFRunLoopGetMain.cold.1 (0x1c030b93c, the dispatch_once call)
// and
// __CFRunLoopGetMain_block_invoke (0x1c01dd674). External. The once block gets the main thread's run loop and
// makes it immortal.
OCFRunLoopRef OCFRunLoopGetMain(void) {
    checkForFork();
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      mainRunLoop = _OCFRunLoopGet0(pthread_main_thread_np());
      __CFRuntimeSetImmortal(mainRunLoop);
    });
    return mainRunLoop;
}

// Native _CFRunLoopCurrentIsMain, 0x1c01dd694. External. Compares the thread pointers directly.
bool _OCFRunLoopCurrentIsMain(void) {
    checkForFork();
    pthread_t mainThread = pthread_main_thread_np();
    return mainThread == pthread_self();
}

// Native CFRunLoopCopyCurrentMode, 0x1c01dd6dc. External. The type assertion precedes the fork check.
OCFRunLoopMode OCFRunLoopCopyCurrentMode(OCFRunLoopRef rl) {
    assertTypeID(rl, kRunLoopTypeID);
    checkForFork();
    pthread_mutex_lock(&rl->lock);
    OCFStringRef result = NULL;
    if (rl->currentMode != NULL) {
        result = OCFRetain(rl->currentMode->name);
    }
    pthread_mutex_unlock(&rl->lock);
    return result;
}

// Native __CFRunLoopGetModeName, 0x1c01dd75c. Non-external; a set applier appending each mode's name.
static void __CFRunLoopGetModeName(const void *value, void *context) {
    OCFArrayAppendValue((OCFMutableArrayRef)context, ((const RunLoopMode *)value)->name);
}

// Native __CFRunLoopCopyMode, 0x1c01dda20, with __CFRunLoopCopyMode.cold.1-.cold.5 (0x1c030b990-0x1c030b9e0)
// and
// ____CFRunLoopCopyMode_block_invoke (0x1c01e29e4). Non-external. The mode is looked up with a zeroed stack
// mode of type 44 carrying the name. A created mode (184 extra bytes on the system allocator) gets a
// recursive lock, a copy of the name, a port set (failure: __THE_SYSTEM_HAS_NO_PORT_SETS_AVAILABLE__), a run
// loop root queue whose port (zero: .cold.5) joins the port set (failure: .cold.1, whose message names the
// timer port), a dispatch timer on that queue whose event handler sets timerFired through a __block pointer
// (leeway 321), an mk timer port (zero: .cold.4; insertion failure: .cold.2) and the run loop's wake-up port
// (zero or insertion failure: .cold.3).
// Folds the compiler-outlined pieces ___CFRunLoopCopyMode.cold.2 (0x1c030b9a4), ___CFRunLoopCopyMode.cold.3
// (0x1c030b9b8), ___CFRunLoopCopyMode.cold.4 (0x1c030b9cc).
static RunLoopMode *__CFRunLoopCopyMode(OCFRunLoopRef rl, OCFStringRef modeName, Boolean create) {
    checkForFork();
    RunLoopMode srlm;
    memset(&srlm, 0, sizeof(srlm));
    _OCFRuntimeSetInstanceTypeIDAndIsa(&srlm, kRunLoopModeTypeID);
    srlm.name = modeName;
    RunLoopMode *rlm = (RunLoopMode *)OCFSetGetValue(rl->modes, &srlm);
    if (rlm != NULL) {
        OCFRetain(rlm);
        return rlm;
    }
    if (!create) {
        return NULL;
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    rlm = (RunLoopMode *)_OCFRuntimeCreateInstance(
        allocator, kRunLoopModeTypeID, sizeof(RunLoopMode) - 16, NULL
    );
    if (rlm == NULL) {
        return NULL;
    }
    pthread_mutexattr_t attr = {0};
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&rlm->lock, &attr);
    pthread_mutexattr_destroy(&attr);
    rlm->name = OCFStringCreateCopy(allocator, modeName);
    mach_port_name_t portSet = MACH_PORT_NULL;
    kern_return_t portSetResult = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_PORT_SET, &portSet);
    if (portSetResult != KERN_SUCCESS) {
        __THE_SYSTEM_HAS_NO_PORT_SETS_AVAILABLE__(portSetResult);
    }
    rlm->timerFired = 0;
    rlm->portSet = portSet;
    rlm->timerSoftDeadline = UINT64_MAX;
    rlm->timerHardDeadline = UINT64_MAX;
    rlm->queue = _dispatch_runloop_root_queue_create_4CF("Run Loop Mode Queue", 0);
    mach_port_t queuePort = _dispatch_runloop_root_queue_get_port_4CF(rlm->queue);
    if (queuePort == MACH_PORT_NULL) {
        crashWithMessage("*** Unable to create run loop mode queue port. ***");
    }
    rlm->timerSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, rlm->queue);
    __block uint8_t *timerFiredPointer = &rlm->timerFired;
    dispatch_source_set_event_handler(rlm->timerSource, ^{
      *timerFiredPointer = 1;
    });
    dispatch_source_set_timer(rlm->timerSource, DISPATCH_TIME_FOREVER, DISPATCH_TIME_FOREVER, 321);
    dispatch_resume(rlm->timerSource);
    if (mach_port_insert_member(mach_task_self(), queuePort, rlm->portSet) != KERN_SUCCESS) {
        crashWithMessage("*** Unable to insert timer port into port set. ***");
    }
    rlm->timerPort = mk_timer_create();
    if (rlm->timerPort == MACH_PORT_NULL) {
        crashWithMessage("*** Unable to create timer Port ***");
    }
    if (mach_port_insert_member(mach_task_self(), rlm->timerPort, rlm->portSet) != KERN_SUCCESS) {
        crashWithMessage("*** Unable to insert timer port into port set. ***");
    }
    mach_port_t wakeUpPort = rl->wakeUpPort;
    if (wakeUpPort == MACH_PORT_NULL ||
        mach_port_insert_member(mach_task_self(), wakeUpPort, rlm->portSet) != KERN_SUCCESS) {
        crashWithMessage("*** Unable to insert wake up port into port set. ***");
    }
    OCFSetAddValue(rl->modes, rlm);
    return rlm;
}

// Native __CFRunLoopModeIsEmpty, 0x1c01ddcfc. Non-external. A common mode of the main run loop on the main
// thread, outside the main queue's servicing, is never empty; otherwise version 0 or 1 sources, timers
// (observers are not counted) or a pending block for this mode, directly or through the common modes, make
// the mode non-empty.
static bool __CFRunLoopModeIsEmpty(OCFRunLoopRef rl, RunLoopMode *rlm) {
    checkForFork();
    if (pthread_main_np() == 1 && _OCFGetTSD(kTSDIsInDispatchMainQueue) == NULL &&
        OCFRunLoopGetMain() == rl && OCFSetContainsValue(rl->commonModes, rlm->name)) {
        return false;
    }
    if (rlm->sources0 != NULL && OCFSetGetCount(rlm->sources0) > 0) {
        return false;
    }
    if (rlm->sources1 != NULL && OCFSetGetCount(rlm->sources1) > 0) {
        return false;
    }
    if (rlm->timers != NULL && OCFArrayGetCount(rlm->timers) > 0) {
        return false;
    }
    RunLoopBlockItem *item = rl->blocksHead;
    while (item != NULL) {
        RunLoopBlockItem *curr = item;
        item = item->next;
        if (OCFGetTypeID(curr->mode) == kStringTypeID) {
            if (OCFEqual(curr->mode, rlm->name)) {
                return false;
            }
            if (OCFEqual(curr->mode, kCommonModesName) && OCFSetContainsValue(rl->commonModes, rlm->name)) {
                return false;
            }
        } else {
            if (OCFSetContainsValue((OCFSetRef)curr->mode, rlm->name)) {
                return false;
            }
            if (OCFSetContainsValue((OCFSetRef)curr->mode, kCommonModesName) &&
                OCFSetContainsValue(rl->commonModes, rlm->name)) {
                return false;
            }
        }
    }
    return true;
}

// Native _CFRunLoopError_RunCalledWithInvalidMode, 0x1c01dde40. Hidden (was private external); an empty
// function to break on.
void _CFRunLoopError_RunCalledWithInvalidMode(void) {
    __asm__ volatile("");
}

// Native _CFRunLoopRunSpecificWithOptions, 0x1c01dde50, with .cold.1 (0x1c030b9f4, the main-run-loop
// assertion), .cold.2 (0x1c030ba34) and ___CFRunLoopRunSpecificWithOptions_block_invoke (0x1c01e29fc).
// Non-external. A NULL or common mode name logs once ("invalid mode") and finishes. A deallocating run loop,
// a missing mode or an empty mode finishes. The nesting index is incremented (at 254 already: "Too many
// nested CFRunLoopRuns"), its stop and ignore-wake-ups flags cleared and the options stored; the entry and
// exit observers bracket __CFRunLoopRun, and _CFRunLoopRunCleanup restores the previous mode and both locks.
// CoreFoundation defect (kept): an empty mode is unlocked although it was never locked (__CFRunLoopCopyMode
// returns it unlocked); unlocking a recursive mutex the thread does not own fails with EPERM and changes
// nothing. At nesting index 254 the options store lands on the thread field (2632), the stop flag on the
// first ignore-wake-ups flag and that flag on byte 593, as natively.
static int32_t _CFRunLoopRunSpecificWithOptions(
    OCFRunLoopRef rl, OCFStringRef modeName, OCFOptionFlags options, OCFTimeInterval seconds
) {
    assertTypeID(rl, kRunLoopTypeID);
    runLoopAssertions(rl);
    checkForFork();
    if (modeName == NULL || modeName == kCommonModesName || OCFEqual(modeName, kCommonModesName)) {
        static dispatch_once_t onceToken;
        dispatch_once(&onceToken, ^{
          OCFLog(
              3,
              OF_STR(
                  "invalid mode '%@' provided to CFRunLoopRunSpecific - break on "
                  "_CFRunLoopError_RunCalledWithInvalidMode to debug. This message will only appear once per "
                  "execution."
              ),
              modeName
          );
          _CFRunLoopError_RunCalledWithInvalidMode();
        });
        return kOCFRunLoopRunFinished;
    }
    if ((loadInfo(rl) & kRunLoopDeallocating) != 0) {
        return kOCFRunLoopRunFinished;
    }
    pthread_mutex_lock(&rl->lock);
    RunLoopMode *currentMode = __CFRunLoopCopyMode(rl, modeName, false);
    if (currentMode == NULL) {
        pthread_mutex_unlock(&rl->lock);
        return kOCFRunLoopRunFinished;
    }
    if (__CFRunLoopModeIsEmpty(rl, currentMode)) {
        pthread_mutex_unlock(&currentMode->lock);
        OCFRelease(currentMode);
        pthread_mutex_unlock(&rl->lock);
        return kOCFRunLoopRunFinished;
    }
    pthread_mutex_lock(&currentMode->lock);
    uint8_t idx = rl->nestingIndex;
    if (idx >= 0xFE) {
        crashWithMessage("Too many nested CFRunLoopRuns");
    }
    idx = idx + 1;
    rl->nestingIndex = idx;
    *stoppedFlag(rl, idx) = 0;
    *ignoreWakeUpsFlag(rl, idx) = 0;
    *(uint64_t *)((uint8_t *)rl + offsetof(struct __OCFRunLoop, runOptions) + 8 * (uintptr_t)idx) = options;
    RunLoopMode *previousMode = rl->currentMode;
    rl->currentMode = currentMode;
    void *cleanup[3] = {rl, currentMode, previousMode};
    uint8_t traceIndex = idx;
    if ((currentMode->observerMask & kOCFRunLoopEntry) != 0) {
        __CFRunLoopDoObservers(rl, currentMode, kOCFRunLoopEntry);
        traceIndex = rl->nestingIndex;
    }

    union {
        OCFTimeInterval seconds;
        uint64_t bits;
    } secondsBits = {seconds};

    kdebug_trace(0x21110005, traceIndex, (uintptr_t)rl, secondsBits.bits, options);
    int32_t result = __CFRunLoopRun(rl, currentMode, options, seconds);
    kdebug_trace(0x21110006, rl->nestingIndex, (uint32_t)result, 0, 0);
    if ((currentMode->observerMask & kOCFRunLoopExit) != 0) {
        __CFRunLoopDoObservers(rl, currentMode, kOCFRunLoopExit);
    }
    _CFRunLoopRunCleanup(cleanup);
    return result;
}

// Native CFRunLoopRunInMode, 0x1c01de15c. External. The options word is returnAfterSourceHandled != 0.
OCFRunLoopRunResult
OCFRunLoopRunInMode(OCFRunLoopMode mode, OCFTimeInterval seconds, Boolean returnAfterSourceHandled) {
    checkForFork();
    return _CFRunLoopRunSpecificWithOptions(
        OCFRunLoopGetCurrent(), mode, returnAfterSourceHandled != 0, seconds
    );
}

// Native CFRunLoopTimerGetNextFireDate, 0x1c01de31c (corpus delta capture 7). External. The fork check
// precedes the dispatch: a tagged pointer or bridged object is sent -_cffireTime; otherwise, under the
// timer's lock, the next fire date (0x68) of a valid timer (info bit 3, acquire load), else 0.0.
OCFAbsoluteTime OCFRunLoopTimerGetNextFireDate(OCFRunLoopTimerRef timer) {
    checkForFork();
    if (((uintptr_t)timer >> 63) != 0 || __CF_IS_OBJC(kRunLoopTimerTypeID, timer) != 0) {
        return ((OCFAbsoluteTime (*)(id, SEL))objc_msgSend)((id)timer, sel_getUid("_cffireTime"));
    }
    pthread_mutex_lock(&timer->lock);
    OCFAbsoluteTime fireDate = 0.0;
    if ((loadInfo(timer) & 8) != 0) {
        fireDate = timer->nextFireDate;
    }
    pthread_mutex_unlock(&timer->lock);
    return fireDate;
}

// Native CFRunLoopIsWaiting, 0x1c01de3ac, with CFRunLoopIsWaiting.cold.1 (0x1c030ba88). External; the
// sleeping bit.
Boolean OCFRunLoopIsWaiting(OCFRunLoopRef rl) {
    assertTypeID(rl, kRunLoopTypeID);
    runLoopAssertions(rl);
    checkForFork();
    return (loadInfo(rl) & kRunLoopSleeping) != 0;
}

// Native CFRunLoopWakeUp, 0x1c01de424, with CFRunLoopWakeUp.cold.1 (0x1c030bac8) and .cold.2 (0x1c030bb08).
// External. The fork check precedes the type assertion. Unless the current run ignores wake-ups, a 24-byte
// message (bits 0x13 = MACH_MSG_TYPE_COPY_SEND, id 0) is sent to the wake-up port with MACH_SEND_TIMEOUT and
// a zero timeout; any result other than success or MACH_SEND_TIMED_OUT stores "*** Unable to send message to
// wake up port. ***" and traps.
void OCFRunLoopWakeUp(OCFRunLoopRef rl) {
    checkForFork();
    assertTypeID(rl, kRunLoopTypeID);
    runLoopAssertions(rl);
    pthread_mutex_lock(&rl->lock);
    if (*ignoreWakeUpsFlag(rl, rl->nestingIndex) != 0) {
        pthread_mutex_unlock(&rl->lock);
        return;
    }
    kdebug_trace(0x211200F9, (uintptr_t)rl, 0, 0, 0);
    mach_msg_header_t header;
    header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    header.msgh_size = sizeof(mach_msg_header_t);
    header.msgh_remote_port = rl->wakeUpPort;
    header.msgh_local_port = MACH_PORT_NULL;
    header.msgh_voucher_port = MACH_PORT_NULL;
    header.msgh_id = 0;
    kern_return_t ret =
        mach_msg(&header, MACH_SEND_MSG | MACH_SEND_TIMEOUT, header.msgh_size, 0, MACH_PORT_NULL, 0, 0);
    __OCFMachMessageCheckForAndDestroyUnsentMessage(ret, &header);
    if (ret != MACH_MSG_SUCCESS && ret != MACH_SEND_TIMED_OUT) {
        crashWithMessage("*** Unable to send message to wake up port. ***");
    }
    kdebug_trace(0x211200FA, (uintptr_t)rl, 0, 0, 0);
    pthread_mutex_unlock(&rl->lock);
}

// Native CFRunLoopPerformBlock, 0x1c01de728, with its .cold.1 (0x1c030bb5c: once the main thread has exited,
// using the main run loop logs once through OUTLINED_FUNCTION_0_23, as RunLoop/OCFRunLoop.c's
// runLoopAssertions does). External. After the fork check and the type-43 assertion, the mode is normalized:
// a string is copied, and every named mode created (under the run loop lock); an array or a set becomes a
// set of its modes (values gathered in a malloc'd buffer), each mode created; any other mode is NULL. The
// block is copied. With both, an item (next, mode, block) is appended to the run loop's block list under the
// lock; otherwise whichever exists is released.
void OCFRunLoopPerformBlock(OCFRunLoopRef rl, OCFTypeRef mode, void (^block)(void)) {
    if ((*(volatile uint8_t *)&__CF_FORK_STATE & 2) == 0) {
        __CF_USED();
    }
    if ((*(volatile uint8_t *)&__CF_FORK_STATE & 1) != 0) {
        __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__();
    }
    OCFTypeID typeID = _CFGetNonObjCTypeID(rl);
    if (typeID != kRunLoopTypeID) {
        _CFAssertMismatchedTypeID(kRunLoopTypeID, typeID);
    }
    if ((__atomic_load_n(&__CFMainThreadHasExited, __ATOMIC_SEQ_CST) & 1) != 0) {
        if (mainRunLoop != NULL && rl == mainRunLoop) {
            dispatch_once(&mainThreadExitedLogOnceToken, ^{
              os_log_error(
                  _CFOSLog(),
                  "Attempting to use the main runloop, but the main thread has exited. This message will "
                  "only "
                  "log once."
              );
            });
        }
    }
    OCFTypeRef modes;
    struct __OCFRunLoop *loop = (struct __OCFRunLoop *)rl;
    if (OCFGetTypeID(mode) == kStringTypeID) {
        modes = OCFStringCreateCopy(kOCFAllocatorSystemDefault, mode);
        pthread_mutex_lock(&loop->lock);
        struct RunLoopMode *rlm = __CFRunLoopCopyMode(rl, modes, true);
        if (rlm != NULL) {
            OCFRelease(rlm);
        }
        pthread_mutex_unlock(&loop->lock);
    } else if (OCFArrayGetTypeID() == OCFGetTypeID(mode)) {
        OCFIndex count = OCFArrayGetCount(mode);
        const void **values = malloc_type_malloc(8 * count, 0xc0040b8aa526d);
        OCFArrayGetValues(mode, (OCFRange){0, count}, values);
        modes = OCFSetCreate(kOCFAllocatorSystemDefault, values, count, &kOCFTypeSetCallBacks);
        pthread_mutex_lock(&loop->lock);
        const void **value = values;
        for (OCFIndex remaining = count; remaining >= 1; --remaining) {
            struct RunLoopMode *rlm = __CFRunLoopCopyMode(rl, *value++, true);
            if (rlm != NULL) {
                OCFRelease(rlm);
            }
        }
        pthread_mutex_unlock(&loop->lock);
        free(values);
    } else if (OCFSetGetTypeID() == OCFGetTypeID(mode)) {
        OCFIndex count = OCFSetGetCount(mode);
        const void **values = malloc_type_malloc(8 * count, 0xc0040b8aa526d);
        OCFSetGetValues(mode, values);
        modes = OCFSetCreate(kOCFAllocatorSystemDefault, values, count, &kOCFTypeSetCallBacks);
        pthread_mutex_lock(&loop->lock);
        const void **value = values;
        for (OCFIndex remaining = count; remaining >= 1; --remaining) {
            struct RunLoopMode *rlm = __CFRunLoopCopyMode(rl, *value++, true);
            if (rlm != NULL) {
                OCFRelease(rlm);
            }
        }
        pthread_mutex_unlock(&loop->lock);
        free(values);
    } else {
        modes = NULL;
    }
    void (^copy)(void) = _Block_copy(block);
    if (modes != NULL && copy != NULL) {
        pthread_mutex_lock(&loop->lock);
        RunLoopBlockItem *item = malloc_type_malloc(sizeof(RunLoopBlockItem), 0xe00400670d255);
        item->next = NULL;
        item->mode = modes;
        item->block = copy;
        if (loop->blocksTail != NULL) {
            loop->blocksTail->next = item;
        } else {
            loop->blocksHead = item;
        }
        loop->blocksTail = item;
        pthread_mutex_unlock(&loop->lock);
        return;
    }
    if (modes != NULL) {
        OCFRelease(modes);
    }
    if (copy != NULL) {
        _Block_release(copy);
    }
}

// Native CFRunLoopContainsSource, 0x1c01dea48, with CFRunLoopContainsSource.cold.1 (0x1c030bb9c). External.
Boolean OCFRunLoopContainsSource(OCFRunLoopRef rl, OCFRunLoopSourceRef source, OCFRunLoopMode mode) {
    assertTypeID(rl, kRunLoopTypeID);
    runLoopAssertions(rl);
    checkForFork();
    Boolean hasValue = false;
    pthread_mutex_lock(&rl->lock);
    if (mode == kCommonModesName) {
        if (rl->commonModeItems != NULL) {
            hasValue = OCFSetContainsValue(rl->commonModeItems, source);
        }
    } else {
        RunLoopMode *rlm = __CFRunLoopCopyMode(rl, mode, false);
        if (rlm != NULL) {
            pthread_mutex_lock(&rlm->lock);
            if (rlm->sources0 != NULL && OCFSetContainsValue(rlm->sources0, source)) {
                hasValue = true;
            } else {
                hasValue = rlm->sources1 != NULL && OCFSetContainsValue(rlm->sources1, source);
            }
            pthread_mutex_unlock(&rlm->lock);
            OCFRelease(rlm);
        }
    }
    pthread_mutex_unlock(&rl->lock);
    return hasValue;
}

// Native __CFRunLoopError_AddingSourceLackingReceiveRight, 0x1c01deb74. Hidden (was private external); an
// empty function to break on.
void __CFRunLoopError_AddingSourceLackingReceiveRight(void) {
    __asm__ volatile("");
}

// Native CFRunLoopAddSource, 0x1c01deb78, with CFRunLoopAddSource.cold.1 (0x1c030bbdc) and
// ___CFRunLoopAddSource_block_invoke (0x1c01def14). External. The fork check precedes the type assertion. A
// version 1 source whose port lacks a receive right (MACH_PORT_TYPE_RECEIVE, 0x20000) logs once and calls the
// break function, then is still added. Valid sources join the common mode items (and every common mode) or
// the mode's version 0 or version 1 set; a version 1 source's port joins the port map and the port set. The
// run loop joins the source's bag; a version 0 source's schedule callback runs after both locks are dropped.
void OCFRunLoopAddSource(OCFRunLoopRef rl, OCFRunLoopSourceRef source, OCFRunLoopMode mode) {
    checkForFork();
    assertTypeID(rl, kRunLoopTypeID);
    runLoopAssertions(rl);
    if ((loadInfo(rl) & kRunLoopDeallocating) != 0) {
        return;
    }
    if (source->context.version0.version == 1) {
        mach_port_t port = source->context.version1.getPort(source->context.version1.info);
        mach_port_type_t type = 0;
        kern_return_t ret = mach_port_type(mach_task_self(), port, &type);
        if (ret == KERN_SUCCESS && (type & MACH_PORT_TYPE_RECEIVE) == 0) {
            static dispatch_once_t onceToken;
            dispatch_once(&onceToken, ^{
              OCFLog(
                  3,
                  OF_STR(
                      "Attempting to add a runloop source %p to runloop %p in mode '%@', but the port "
                      "associated "
                      "with this source does not have a receive right. This means your source will never be "
                      "signalled and is likely an error. Break on "
                      "__CFRunLoopError_AddingSourceLackingReceiveRight "
                      "to debug. This message will only appear once per execution."
                  ),
                  source,
                  rl,
                  mode
              );
            });
            __CFRunLoopError_AddingSourceLackingReceiveRight();
        }
    }
    if ((loadInfo(source) & kInfoValid) == 0) {
        return;
    }
    pthread_mutex_lock(&rl->lock);
    if (mode == kCommonModesName) {
        OCFSetRef set =
            rl->commonModes != NULL ? OCFSetCreateCopy(kOCFAllocatorSystemDefault, rl->commonModes) : NULL;
        if (rl->commonModeItems == NULL) {
            rl->commonModeItems = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeSetCallBacks);
        }
        OCFSetAddValue(rl->commonModeItems, source);
        if (set != NULL) {
            const void *context[2] = {rl, source};
            OCFSetApplyFunction(set, __CFRunLoopAddItemToCommonModes, context);
            OCFRelease(set);
        }
        pthread_mutex_unlock(&rl->lock);
        return;
    }
    RunLoopMode *rlm = __CFRunLoopCopyMode(rl, mode, true);
    if (rlm == NULL) {
        pthread_mutex_unlock(&rl->lock);
        return;
    }
    pthread_mutex_lock(&rlm->lock);
    if (rlm->sources0 == NULL) {
        rlm->sources0 = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeSetCallBacks);
        rlm->sources1 = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeSetCallBacks);
        rlm->portToV1SourceMap = OCFDictionaryCreateMutable(kOCFAllocatorSystemDefault, 0, NULL, NULL);
    }
    if (OCFSetContainsValue(rlm->sources0, source) || OCFSetContainsValue(rlm->sources1, source)) {
        pthread_mutex_unlock(&rlm->lock);
        OCFRelease(rlm);
        pthread_mutex_unlock(&rl->lock);
        return;
    }
    if (source->context.version0.version == 1) {
        OCFSetAddValue(rlm->sources1, source);
        mach_port_t port = source->context.version1.getPort(source->context.version1.info);
        if (port != MACH_PORT_NULL) {
            OCFDictionarySetValue(rlm->portToV1SourceMap, (const void *)(uintptr_t)port, source);
            mach_port_insert_member(mach_task_self(), port, rlm->portSet);
        }
    } else if (source->context.version0.version == 0) {
        OCFSetAddValue(rlm->sources0, source);
    }
    pthread_mutex_lock(&source->lock);
    if (source->runLoops == NULL) {
        source->runLoops = OCFBagCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeBagCallBacks);
    }
    OCFBagAddValue(source->runLoops, rl);
    pthread_mutex_unlock(&source->lock);
    if (source->context.version0.version != 0) {
        pthread_mutex_unlock(&rlm->lock);
        OCFRelease(rlm);
        pthread_mutex_unlock(&rl->lock);
        return;
    }
    void (*schedule)(void *, OCFRunLoopRef, OCFRunLoopMode) = source->context.version0.schedule;
    pthread_mutex_unlock(&rlm->lock);
    OCFRelease(rlm);
    pthread_mutex_unlock(&rl->lock);
    if (schedule != NULL) {
        source->context.version0.schedule(source->context.version0.info, rl, mode);
    }
}

// Native __CFRunLoopAddItemToCommonModes, 0x1c01def4c. Non-external; a set applier over mode names adding the
// context's item by its type (observer 45, timer 47, source 46).
static void __CFRunLoopAddItemToCommonModes(const void *value, void *ctx) {
    OCFStringRef modeName = value;
    OCFRunLoopRef rl = (OCFRunLoopRef)((const void **)ctx)[0];
    OCFTypeRef item = ((const void **)ctx)[1];
    switch (OCFGetTypeID(item)) {
        case kRunLoopObserverTypeID:
            OCFRunLoopAddObserver(rl, (OCFRunLoopObserverRef)item, modeName);
            break;
        case kRunLoopTimerTypeID:
            OCFRunLoopAddTimer(rl, (OCFRunLoopTimerRef)item, modeName);
            break;
        case kRunLoopSourceTypeID:
            OCFRunLoopAddSource(rl, (OCFRunLoopSourceRef)item, modeName);
            break;
        default:
            break;
    }
}

// Native CFRunLoopRemoveSource, 0x1c01defe8, with CFRunLoopRemoveSource.cold.1 (0x1c030bc1c). External. A
// found source is retained, loses its port (version 1), leaves both sets and the run loop leaves its bag; a
// version 0 source's cancel callback runs after both locks are dropped, then the retain is balanced.
void OCFRunLoopRemoveSource(OCFRunLoopRef rl, OCFRunLoopSourceRef source, OCFRunLoopMode mode) {
    assertTypeID(rl, kRunLoopTypeID);
    runLoopAssertions(rl);
    checkForFork();
    pthread_mutex_lock(&rl->lock);
    if (mode == kCommonModesName) {
        if (rl->commonModeItems != NULL && OCFSetContainsValue(rl->commonModeItems, source)) {
            if (rl->commonModes != NULL) {
                OCFSetRef set = OCFSetCreateCopy(kOCFAllocatorSystemDefault, rl->commonModes);
                OCFSetRemoveValue(rl->commonModeItems, source);
                if (set != NULL) {
                    const void *context[2] = {rl, source};
                    OCFSetApplyFunction(set, __CFRunLoopRemoveItemFromCommonModes, context);
                    OCFRelease(set);
                }
            } else {
                OCFSetRemoveValue(rl->commonModeItems, source);
            }
        }
        pthread_mutex_unlock(&rl->lock);
        return;
    }
    RunLoopMode *rlm = __CFRunLoopCopyMode(rl, mode, false);
    if (rlm == NULL) {
        pthread_mutex_unlock(&rl->lock);
        return;
    }
    bool notFound = true;
    bool doCancel = false;
    pthread_mutex_lock(&rlm->lock);
    if ((rlm->sources0 != NULL && OCFSetContainsValue(rlm->sources0, source)) ||
        (rlm->sources1 != NULL && OCFSetContainsValue(rlm->sources1, source))) {
        OCFRetain(source);
        if (source->context.version0.version == 1) {
            mach_port_t port = source->context.version1.getPort(source->context.version1.info);
            if (port != MACH_PORT_NULL) {
                OCFDictionaryRemoveValue(rlm->portToV1SourceMap, (const void *)(uintptr_t)port);
                mach_port_extract_member(mach_task_self(), port, rlm->portSet);
            }
        }
        OCFSetRemoveValue(rlm->sources0, source);
        OCFSetRemoveValue(rlm->sources1, source);
        pthread_mutex_lock(&source->lock);
        if (source->runLoops != NULL) {
            OCFBagRemoveValue(source->runLoops, rl);
        }
        pthread_mutex_unlock(&source->lock);
        notFound = false;
        if (source->context.version0.version == 0) {
            doCancel = source->context.version0.cancel != NULL;
        }
    }
    pthread_mutex_unlock(&rlm->lock);
    OCFRelease(rlm);
    pthread_mutex_unlock(&rl->lock);
    if (doCancel) {
        source->context.version0.cancel(source->context.version0.info, rl, mode);
    }
    if (!notFound) {
        OCFRelease(source);
    }
}

// Native __CFRunLoopRemoveItemFromCommonModes, 0x1c01df290. Non-external; the removal counterpart of
// __CFRunLoopAddItemToCommonModes.
static void __CFRunLoopRemoveItemFromCommonModes(const void *value, void *ctx) {
    OCFStringRef modeName = value;
    OCFRunLoopRef rl = (OCFRunLoopRef)((const void **)ctx)[0];
    OCFTypeRef item = ((const void **)ctx)[1];
    switch (OCFGetTypeID(item)) {
        case kRunLoopObserverTypeID:
            OCFRunLoopRemoveObserver(rl, (OCFRunLoopObserverRef)item, modeName);
            break;
        case kRunLoopTimerTypeID:
            OCFRunLoopRemoveTimer(rl, (OCFRunLoopTimerRef)item, modeName);
            break;
        case kRunLoopSourceTypeID:
            OCFRunLoopRemoveSource(rl, (OCFRunLoopSourceRef)item, modeName);
            break;
        default:
            break;
    }
}

// Native CFRunLoopAddObserver, 0x1c01df44c, with CFRunLoopAddObserver.cold.1 (0x1c030bc9c). External. The
// fork check precedes the type assertion. A valid observer not bound to another run loop is inserted after
// the last observer whose order does not exceed its own; the mode's observer mask gains its activities.
void OCFRunLoopAddObserver(OCFRunLoopRef rl, OCFRunLoopObserverRef observer, OCFRunLoopMode mode) {
    checkForFork();
    assertTypeID(rl, kRunLoopTypeID);
    runLoopAssertions(rl);
    if ((loadInfo(rl) & kRunLoopDeallocating) != 0) {
        return;
    }
    if ((loadInfo(observer) & kInfoValid) == 0) {
        return;
    }
    if (observer->runLoop != NULL && observer->runLoop != rl) {
        return;
    }
    pthread_mutex_lock(&rl->lock);
    if (mode == kCommonModesName) {
        OCFSetRef set =
            rl->commonModes != NULL ? OCFSetCreateCopy(kOCFAllocatorSystemDefault, rl->commonModes) : NULL;
        if (rl->commonModeItems == NULL) {
            rl->commonModeItems = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeSetCallBacks);
        }
        OCFSetAddValue(rl->commonModeItems, observer);
        if (set != NULL) {
            const void *context[2] = {rl, observer};
            OCFSetApplyFunction(set, __CFRunLoopAddItemToCommonModes, context);
            OCFRelease(set);
        }
    } else {
        RunLoopMode *rlm = __CFRunLoopCopyMode(rl, mode, true);
        if (rlm == NULL) {
            pthread_mutex_unlock(&rl->lock);
            return;
        }
        pthread_mutex_lock(&rlm->lock);
        if (rlm->observers == NULL) {
            rlm->observers = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
        }
        OCFMutableArrayRef observers = rlm->observers;
        if (!OCFArrayContainsValue(observers, (OCFRange){0, OCFArrayGetCount(observers)}, observer)) {
            OCFIndex idx = OCFArrayGetCount(rlm->observers);
            while (idx != 0) {
                OCFRunLoopObserverRef other =
                    (OCFRunLoopObserverRef)OCFArrayGetValueAtIndex(rlm->observers, idx - 1);
                if (other->order <= observer->order) {
                    break;
                }
                idx--;
            }
            OCFArrayInsertValueAtIndex(rlm->observers, idx, observer);
            rlm->observerMask |= observer->activities;
            __CFRunLoopObserverSchedule(observer, rl);
        }
        pthread_mutex_unlock(&rlm->lock);
        OCFRelease(rlm);
    }
    pthread_mutex_unlock(&rl->lock);
}

// Native __CFRunLoopObserverSchedule, 0x1c01df6a4. Non-external. The first schedule binds the run loop.
static void __CFRunLoopObserverSchedule(OCFRunLoopObserverRef rlo, OCFRunLoopRef rl) {
    pthread_mutex_lock(&rlo->lock);
    OCFIndex count = rlo->runLoopCount;
    if (count == 0) {
        rlo->runLoop = rl;
    }
    rlo->runLoopCount = count + 1;
    pthread_mutex_unlock(&rlo->lock);
}

// Native CFRunLoopRemoveObserver, 0x1c01df6e4, with CFRunLoopRemoveObserver.cold.1 (0x1c030bcdc). External.
void OCFRunLoopRemoveObserver(OCFRunLoopRef rl, OCFRunLoopObserverRef observer, OCFRunLoopMode mode) {
    assertTypeID(rl, kRunLoopTypeID);
    runLoopAssertions(rl);
    checkForFork();
    pthread_mutex_lock(&rl->lock);
    if (mode == kCommonModesName) {
        if (rl->commonModeItems != NULL && OCFSetContainsValue(rl->commonModeItems, observer)) {
            if (rl->commonModes == NULL) {
                OCFSetRemoveValue(rl->commonModeItems, observer);
            } else {
                OCFSetRef set = OCFSetCreateCopy(kOCFAllocatorSystemDefault, rl->commonModes);
                OCFSetRemoveValue(rl->commonModeItems, observer);
                if (set != NULL) {
                    const void *context[2] = {rl, observer};
                    OCFSetApplyFunction(set, __CFRunLoopRemoveItemFromCommonModes, context);
                    OCFRelease(set);
                }
            }
        }
    } else {
        RunLoopMode *rlm = __CFRunLoopCopyMode(rl, mode, false);
        if (rlm != NULL) {
            pthread_mutex_lock(&rlm->lock);
            if (rlm->observers != NULL) {
                OCFRetain(observer);
                OCFMutableArrayRef observers = rlm->observers;
                OCFIndex idx = OCFArrayGetFirstIndexOfValue(
                    observers, (OCFRange){0, OCFArrayGetCount(observers)}, observer
                );
                if (idx != (OCFIndex)-1) {
                    OCFArrayRemoveValueAtIndex(rlm->observers, idx);
                    __CFRunLoopObserverCancel(observer);
                }
                OCFRelease(observer);
            }
            pthread_mutex_unlock(&rlm->lock);
            OCFRelease(rlm);
        }
    }
    pthread_mutex_unlock(&rl->lock);
}

// Native __CFRunLoopObserverCancel, 0x1c01df8a4. Non-external. The last cancel unbinds the run loop.
static void __CFRunLoopObserverCancel(OCFRunLoopObserverRef rlo) {
    pthread_mutex_lock(&rlo->lock);
    rlo->runLoopCount--;
    if (rlo->runLoopCount == 0) {
        rlo->runLoop = NULL;
    }
    pthread_mutex_unlock(&rlo->lock);
}

// Native CFRunLoopAddTimer, 0x1c01dfa10, with CFRunLoopAddTimer.cold.1 (0x1c030bd5c). External. The fork
// check precedes the type assertion. The mode's timer array uses the CFType callbacks without an equal
// callback. A timer not yet in the mode and not bound to another run loop is bound, records the mode name and
// is positioned under the timer lock; executables linked before version 7 also wake a run loop other than the
// current one.
void OCFRunLoopAddTimer(OCFRunLoopRef rl, OCFRunLoopTimerRef timer, OCFRunLoopMode mode) {
    checkForFork();
    assertTypeID(rl, kRunLoopTypeID);
    runLoopAssertions(rl);
    if ((loadInfo(rl) & kRunLoopDeallocating) != 0) {
        return;
    }
    if ((loadInfo(timer) & kInfoValid) == 0) {
        return;
    }
    if (timer->runLoop != NULL && timer->runLoop != rl) {
        return;
    }
    pthread_mutex_lock(&rl->lock);
    if (mode == kCommonModesName) {
        OCFSetRef set =
            rl->commonModes != NULL ? OCFSetCreateCopy(kOCFAllocatorSystemDefault, rl->commonModes) : NULL;
        if (rl->commonModeItems == NULL) {
            rl->commonModeItems = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeSetCallBacks);
        }
        OCFSetAddValue(rl->commonModeItems, timer);
        if (set != NULL) {
            const void *context[2] = {rl, timer};
            OCFSetApplyFunction(set, __CFRunLoopAddItemToCommonModes, context);
            OCFRelease(set);
        }
        pthread_mutex_unlock(&rl->lock);
        return;
    }
    RunLoopMode *rlm = __CFRunLoopCopyMode(rl, mode, true);
    if (rlm == NULL) {
        pthread_mutex_unlock(&rl->lock);
        return;
    }
    pthread_mutex_lock(&rlm->lock);
    if (rlm->timers == NULL) {
        OCFArrayCallBacks callBacks = kOCFTypeArrayCallBacks;
        callBacks.equal = NULL;
        rlm->timers = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &callBacks);
    }
    if (!OCFSetContainsValue(timer->rlModes, rlm->name)) {
        pthread_mutex_lock(&timer->lock);
        if (timer->runLoop == NULL || timer->runLoop == rl) {
            if (timer->runLoop == NULL) {
                timer->runLoop = rl;
            }
            OCFSetAddValue(timer->rlModes, rlm->name);
            pthread_mutex_unlock(&timer->lock);
            os_unfair_lock_lock(&rl->timerTSRLock);
            __CFRepositionTimerInMode(rlm, timer, false);
            os_unfair_lock_unlock(&rl->timerTSRLock);
            if (!_OCFExecutableLinkedOnOrAfter(7) && rl != _CFRunLoopGetButDontCreateCurrent()) {
                OCFRunLoopWakeUp(rl);
            }
        } else {
            pthread_mutex_unlock(&timer->lock);
        }
    }
    pthread_mutex_unlock(&rlm->lock);
    OCFRelease(rlm);
    pthread_mutex_unlock(&rl->lock);
}

// Native __CFRepositionTimerInMode, 0x1c01dfc78. Non-external. Nothing happens without a timer or a timer
// array. A timer in the array (found from index 0; absent: nothing) is retained, removed and reinserted at
// its insertion index, the mode's next timer is armed for the timer's run loop and the timer is released (a
// tail call); otherwise the timer is inserted and the next timer armed (a tail call).
static void __CFRepositionTimerInMode(RunLoopMode *rlm, OCFRunLoopTimerRef rlt, bool isInArray) {
    if (rlt == NULL) {
        return;
    }
    OCFMutableArrayRef timers = rlm->timers;
    if (timers == NULL) {
        return;
    }
    if (isInArray) {
        OCFIndex index = OCFArrayGetFirstIndexOfValue(timers, (OCFRange){0, OCFArrayGetCount(timers)}, rlt);
        if (index == kOCFNotFound) {
            return;
        }
        OCFRetain(rlt);
        OCFArrayRemoveValueAtIndex(timers, index);
        OCFArrayInsertValueAtIndex(timers, __CFRunLoopInsertionIndexInTimerArray(timers, rlt), rlt);
        __CFArmNextTimerInMode(rlm, rlt->runLoop);
        OCFRelease(rlt);
        return;
    }
    OCFArrayInsertValueAtIndex(timers, __CFRunLoopInsertionIndexInTimerArray(timers, rlt), rlt);
    __CFArmNextTimerInMode(rlm, rlt->runLoop);
}

// Native CFRunLoopRemoveTimer, 0x1c01dfd58, with CFRunLoopRemoveTimer.cold.1 (0x1c030bd9c). External. A timer
// leaving its last mode is unbound; the mode's next timer is rearmed.
void OCFRunLoopRemoveTimer(OCFRunLoopRef rl, OCFRunLoopTimerRef timer, OCFRunLoopMode mode) {
    assertTypeID(rl, kRunLoopTypeID);
    runLoopAssertions(rl);
    checkForFork();
    pthread_mutex_lock(&rl->lock);
    if (mode == kCommonModesName) {
        if (rl->commonModeItems != NULL && OCFSetContainsValue(rl->commonModeItems, timer)) {
            if (rl->commonModes == NULL) {
                OCFSetRemoveValue(rl->commonModeItems, timer);
            } else {
                OCFSetRef set = OCFSetCreateCopy(kOCFAllocatorSystemDefault, rl->commonModes);
                OCFSetRemoveValue(rl->commonModeItems, timer);
                if (set != NULL) {
                    const void *context[2] = {rl, timer};
                    OCFSetApplyFunction(set, __CFRunLoopRemoveItemFromCommonModes, context);
                    OCFRelease(set);
                }
            }
        }
    } else {
        RunLoopMode *rlm = __CFRunLoopCopyMode(rl, mode, false);
        if (rlm != NULL) {
            pthread_mutex_lock(&rlm->lock);
            OCFMutableArrayRef timers = rlm->timers;
            if (timers != NULL) {
                OCFIndex idx =
                    OCFArrayGetFirstIndexOfValue(timers, (OCFRange){0, OCFArrayGetCount(rlm->timers)}, timer);
                if (idx != (OCFIndex)-1) {
                    pthread_mutex_lock(&timer->lock);
                    OCFSetRemoveValue(timer->rlModes, rlm->name);
                    if (OCFSetGetCount(timer->rlModes) == 0) {
                        timer->runLoop = NULL;
                    }
                    pthread_mutex_unlock(&timer->lock);
                    OCFArrayRemoveValueAtIndex(timers, idx);
                    __CFArmNextTimerInMode(rlm, rl);
                }
            }
            pthread_mutex_unlock(&rlm->lock);
            OCFRelease(rlm);
        }
    }
    pthread_mutex_unlock(&rl->lock);
}

// Native __CFArmNextTimerInMode, 0x1c01dff40. Non-external natively; hidden here (reached from
// __CFRepositionTimerInMode). Scans the sorted timers that are not firing: the soft deadline is the earliest
// fire time, the hard deadline the earliest fire time plus tolerance (saturating), and the scan stops at a
// fire time past the hard deadline. When either deadline changes: the next_timer_armed probe, a kdebug event,
// then a nonzero leeway arms the dispatch timer (cancelling an armed mk timer) and a zero leeway disarms the
// dispatch timer (leeway 888) and arms the mk timer. No timer cancels the mk timer and disarms the dispatch
// timer (leeway 333).
void __CFArmNextTimerInMode(RunLoopMode *rlm, OCFRunLoopRef rl) {
    uint64_t hardDeadline = UINT64_MAX;
    uint64_t softDeadline = UINT64_MAX;
    if (rlm->timers == NULL) {
        rlm->timerSoftDeadline = softDeadline;
        rlm->timerHardDeadline = hardDeadline;
        return;
    }
    OCFIndex count = OCFArrayGetCount(rlm->timers);
    for (OCFIndex idx = 0; idx < count; idx++) {
        OCFRunLoopTimerRef t = (OCFRunLoopTimerRef)OCFArrayGetValueAtIndex(rlm->timers, idx);
        if ((t->bits & 1) != 0) {
            continue;
        }
        uint64_t fireTSR = t->fireTSR;
        uint64_t toleranceTSR = __CFTimeIntervalToTSR(t->tolerance);
        if (fireTSR > hardDeadline) {
            break;
        }
        uint64_t fireAndTolerance = fireTSR + toleranceTSR;
        if (fireAndTolerance < fireTSR) {
            fireAndTolerance = UINT64_MAX;
        }
        if (fireTSR < softDeadline) {
            softDeadline = fireTSR;
        }
        if (fireAndTolerance < hardDeadline) {
            hardDeadline = fireAndTolerance;
        }
    }
    if (softDeadline != UINT64_MAX) {
        if (hardDeadline != rlm->timerHardDeadline || softDeadline != rlm->timerSoftDeadline) {
            if (CFRUNLOOP_NEXT_TIMER_ARMED_ENABLED()) {
                CFRUNLOOP_NEXT_TIMER_ARMED((unsigned long)(softDeadline - mach_absolute_time()));
            }
            kdebug_trace(0x21120080, (uintptr_t)rl, (uintptr_t)rlm, softDeadline - mach_absolute_time(), 0);
            uint64_t leeway = __CFTSRToNanoseconds(hardDeadline - softDeadline);
            dispatch_time_t deadline = __CFTSRToDispatchTime(softDeadline);
            if (leeway != 0) {
                if (rlm->mkTimerArmed && rlm->timerPort != MACH_PORT_NULL) {
                    uint64_t result = 0;
                    mk_timer_cancel(rlm->timerPort, &result);
                    rlm->mkTimerArmed = 0;
                }
                dispatch_source_set_timer(rlm->timerSource, deadline, DISPATCH_TIME_FOREVER, leeway);
                rlm->dispatchTimerArmed = 1;
            } else {
                if (rlm->dispatchTimerArmed) {
                    dispatch_source_set_timer(
                        rlm->timerSource, DISPATCH_TIME_FOREVER, DISPATCH_TIME_FOREVER, 888
                    );
                    rlm->dispatchTimerArmed = 0;
                }
                if (rlm->timerPort != MACH_PORT_NULL) {
                    mk_timer_arm(rlm->timerPort, softDeadline);
                    rlm->mkTimerArmed = 1;
                }
            }
        }
    } else {
        if (rlm->mkTimerArmed && rlm->timerPort != MACH_PORT_NULL) {
            uint64_t result = 0;
            mk_timer_cancel(rlm->timerPort, &result);
            rlm->mkTimerArmed = 0;
        }
        if (rlm->dispatchTimerArmed) {
            dispatch_source_set_timer(rlm->timerSource, DISPATCH_TIME_FOREVER, DISPATCH_TIME_FOREVER, 333);
            rlm->dispatchTimerArmed = 0;
        }
    }
    rlm->timerSoftDeadline = softDeadline;
    rlm->timerHardDeadline = hardDeadline;
}

// Native __CFRunLoopSourceDeallocate, 0x1c01e0168. Non-external. Invalidates, releases the context
// information, destroys the lock and zeroes bytes 16-183.
static void __CFRunLoopSourceDeallocate(OCFTypeRef cf) {
    OCFRunLoopSourceRef rls = (OCFRunLoopSourceRef)cf;
    OCFRunLoopSourceInvalidate(rls);
    if (rls->context.version0.release != NULL) {
        rls->context.version0.release(rls->context.version0.info);
    }
    pthread_mutex_destroy(&rls->lock);
    memset((uint8_t *)rls + 16, 0, sizeof(struct __OCFRunLoopSource) - 16);
}

// Native __CFRunLoopSourceEqual, 0x1c01e01bc. Non-external. Same validity, order, version, hash and equal
// callbacks and perform callback (version 0 at 176, version 1 at 168), then the equal callback or information
// identity.
static Boolean __CFRunLoopSourceEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    OCFRunLoopSourceRef rls1 = (OCFRunLoopSourceRef)cf1;
    OCFRunLoopSourceRef rls2 = (OCFRunLoopSourceRef)cf2;
    if (rls1 == rls2) {
        return true;
    }
    if (((loadInfo(rls1) ^ loadInfo(rls2)) & kInfoValid) != 0) {
        return false;
    }
    if (rls1->order != rls2->order) {
        return false;
    }
    OCFIndex version = rls1->context.version0.version;
    if (version != rls2->context.version0.version) {
        return false;
    }
    if (rls1->context.version0.hash != rls2->context.version0.hash) {
        return false;
    }
    Boolean (*equal)(const void *, const void *) = rls1->context.version0.equal;
    if (equal != rls2->context.version0.equal) {
        return false;
    }
    if (version == 1) {
        if ((void *)rls1->context.version1.perform != (void *)rls2->context.version1.perform) {
            return false;
        }
    } else if (version == 0) {
        if (rls1->context.version0.perform != rls2->context.version0.perform) {
            return false;
        }
    }
    if (equal != NULL) {
        return equal(rls1->context.version0.info, rls2->context.version0.info);
    }
    return rls1->context.version0.info == rls2->context.version0.info;
}

// Native __CFRunLoopSourceHash, 0x1c01e0270. Non-external; the hash callback of the information, or the
// information itself.
static OCFHashCode __CFRunLoopSourceHash(OCFTypeRef cf) {
    OCFRunLoopSourceRef rls = (OCFRunLoopSourceRef)cf;
    if (rls->context.version0.hash != NULL) {
        return rls->context.version0.hash(rls->context.version0.info);
    }
    return (OCFHashCode)rls->context.version0.info;
}

// Native __CFRunLoopSourceCopyDescription, 0x1c01e0284. Non-external. The context description is the copy
// description callback's result, else the version, information and perform callback with its dladdr symbol
// name
// ("???" unless the symbol starts exactly at the callback). The signalled time is loaded with acquire
// ordering.
static OCFStringRef __CFRunLoopSourceCopyDescription(OCFTypeRef cf) {
    OCFRunLoopSourceRef rls = (OCFRunLoopSourceRef)cf;
    OCFStringRef contextDesc = NULL;
    if (rls->context.version0.copyDescription != NULL) {
        contextDesc = rls->context.version0.copyDescription(rls->context.version0.info);
    }
    if (contextDesc == NULL) {
        void *addr = NULL;
        if (rls->context.version0.version == 0) {
            addr = (void *)rls->context.version0.perform;
        } else if (rls->context.version0.version == 1) {
            addr = (void *)rls->context.version1.perform;
        }
        Dl_info info;
        memset(&info, 0, sizeof(info));
        const char *name = "???";
        if (dladdr(addr, &info) != 0 && info.dli_sname != NULL && info.dli_saddr == addr) {
            name = info.dli_sname;
        }
        contextDesc = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("<CFRunLoopSource context>{version = %ld, info = %p, callout = %s (%p)}"),
            rls->context.version0.version,
            rls->context.version0.info,
            name,
            addr
        );
    }
    OCFAllocatorRef allocator = OCFGetAllocator(rls);
    const char *signalled = __atomic_load_n(&rls->signaledTime, __ATOMIC_ACQUIRE) != 0 ? "Yes" : "No";
    const char *valid = (loadInfo(rls) & kInfoValid) != 0 ? "Yes" : "No";
    OCFStringRef result = OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault,
        NULL,
        OF_STR("<CFRunLoopSource %p [%p]>{signalled = %s, valid = %s, order = %ld, context = %@}"),
        rls,
        allocator,
        signalled,
        valid,
        rls->order,
        contextDesc
    );
    OCFRelease(contextDesc);
    return result;
}

// Native CFRunLoopSourceCreate, 0x1c01e0400, with CFRunLoopSourceCreate.cold.1 (0x1c030bddc). External. A
// NULL context stores "*** NULL context value passed to CFRunLoopSourceCreate(). ***" and traps. The new
// source is valid with no signal time and a recursive lock; a version 0 context is copied whole, a version 1
// context without a perform callback at 176, any other version not at all; the retain callback replaces the
// information.
OCFRunLoopSourceRef
OCFRunLoopSourceCreate(OCFAllocatorRef allocator, OCFIndex order, OCFRunLoopSourceContext *context) {
    checkForFork();
    if (context == NULL) {
        crashWithMessage("*** NULL context value passed to CFRunLoopSourceCreate(). ***");
    }
    OCFRunLoopSourceRef rls = (OCFRunLoopSourceRef)_OCFRuntimeCreateInstance(
        allocator, kRunLoopSourceTypeID, sizeof(struct __OCFRunLoopSource) - 16, NULL
    );
    if (rls == NULL) {
        return NULL;
    }
    setInfoBits(rls, kInfoValid);
    __atomic_store_n(&rls->signaledTime, 0, __ATOMIC_SEQ_CST);
    pthread_mutexattr_t attr = {0};
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&rls->lock, &attr);
    pthread_mutexattr_destroy(&attr);
    rls->order = order;
    if (context->version == 0) {
        rls->context.version0 = *context;
    } else if (context->version == 1) {
        rls->context.version0.version = 1;
        rls->context.version0.info = context->info;
        rls->context.version0.retain = context->retain;
        rls->context.version0.release = context->release;
        rls->context.version0.copyDescription = context->copyDescription;
        rls->context.version0.equal = context->equal;
        rls->context.version0.hash = context->hash;
        rls->context.version0.schedule = context->schedule;
        rls->context.version0.cancel = context->cancel;
    }
    if (rls->context.version0.retain != NULL) {
        rls->context.version0.info = (void *)rls->context.version0.retain(context->info);
    }
    return rls;
}

// Native CFRunLoopSourceInvalidate, 0x1c01e0614. External. A valid source becomes invalid with no signal
// time; with run loops in its bag, the bag is detached and every run loop removes the source with the source
// unlocked.
void OCFRunLoopSourceInvalidate(OCFRunLoopSourceRef source) {
    assertTypeID(source, kRunLoopSourceTypeID);
    checkForFork();
    pthread_mutex_lock(&source->lock);
    OCFRetain(source);
    if ((loadInfo(source) & kInfoValid) != 0) {
        OCFMutableBagRef runLoops = source->runLoops;
        clearInfoBits(source, kInfoValid);
        __atomic_store_n(&source->signaledTime, 0, __ATOMIC_SEQ_CST);
        if (runLoops != NULL) {
            source->runLoops = NULL;
            pthread_mutex_unlock(&source->lock);
            const void *context[2] = {source, NULL};
            OCFBagApplyFunction(runLoops, __CFRunLoopSourceRemoveFromRunLoop, context);
            OCFRelease(runLoops);
            pthread_mutex_lock(&source->lock);
        }
    }
    pthread_mutex_unlock(&source->lock);
    OCFRelease(source);
}

// Native __CFRunLoopSourceRemoveFromRunLoop, 0x1c01e0728. Non-external; a bag applier. A run loop repeated in
// the bag (the previous value) is skipped; otherwise the source leaves every mode, last first, then the
// common modes.
static void __CFRunLoopSourceRemoveFromRunLoop(const void *value, void *context) {
    OCFRunLoopRef rl = (OCFRunLoopRef)value;
    const void **params = context;
    if (rl == params[1]) {
        return;
    }
    OCFRunLoopSourceRef rls = (OCFRunLoopSourceRef)params[0];
    pthread_mutex_lock(&rl->lock);
    OCFArrayRef array = OCFRunLoopCopyAllModes(rl);
    for (OCFIndex idx = OCFArrayGetCount(array); idx--;) {
        OCFStringRef modeName = OCFArrayGetValueAtIndex(array, idx);
        OCFRunLoopRemoveSource(rl, rls, modeName);
    }
    OCFRunLoopRemoveSource(rl, rls, kCommonModesName);
    pthread_mutex_unlock(&rl->lock);
    OCFRelease(array);
    params[1] = rl;
}

// Native CFRunLoopSourceIsValid, 0x1c01e07d8. External. The type check precedes the fork check; the valid
// bit of the info word (acquire load).
Boolean OCFRunLoopSourceIsValid(OCFRunLoopSourceRef source) {
    assertTypeID(source, kRunLoopSourceTypeID);
    checkForFork();
    return (loadInfo(source) & kInfoValid) != 0;
}

// Native CFRunLoopSourceGetContext, 0x1c01e0834. External. iOS Device divergence: unread.
// Zero-fill the version 1 tail before choosing the copy size from the caller's original version.
void OCFRunLoopSourceGetContext(OCFRunLoopSourceRef source, OCFRunLoopSourceContext *context) {
    assertTypeID(source, kRunLoopSourceTypeID);
    checkForFork();
    OCFRunLoopSourceContext snapshot = {0};
    if (source->context.version0.version == 0) {
        snapshot = source->context.version0;
    } else {
        memcpy(&snapshot, &source->context.version1, sizeof(OCFRunLoopSourceContext1));
    }
    memmove(context, &snapshot, context->version == 0 ? sizeof(snapshot) : sizeof(OCFRunLoopSourceContext1));
}

// Native CFRunLoopSourceSignal, 0x1c01e0948. External. A valid source records the first signal time with a
// compare-and-swap from zero, between two kdebug events.
void OCFRunLoopSourceSignal(OCFRunLoopSourceRef source) {
    assertTypeID(source, kRunLoopSourceTypeID);
    checkForFork();
    pthread_mutex_lock(&source->lock);
    if ((loadInfo(source) & kInfoValid) != 0) {
        kdebug_trace(
            0x141300F5,
            (uintptr_t)source,
            (uintptr_t)source->context.version0.perform,
            (uintptr_t)source->context.version0.info,
            0
        );
        uint64_t now = mach_absolute_time();
        uint64_t expected = 0;
        __atomic_compare_exchange_n(
            &source->signaledTime, &expected, now, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
        );
        kdebug_trace(
            0x141300F6,
            (uintptr_t)source,
            (uintptr_t)source->context.version0.perform,
            (uintptr_t)source->context.version0.info,
            0
        );
    }
    pthread_mutex_unlock(&source->lock);
}

// Native _CFRunLoopSourceWakeUpRunLoops, 0x1c01e0a78. Hidden (was a private external). Under the
// source's lock, a valid source with a run loop bag copies the bag (kCFAllocatorSystemDefault); after the
// unlock, every run loop of the copy (when created) is woken and the copy is released (a tail call). No fork
// check.
void _CFRunLoopSourceWakeUpRunLoops(OCFRunLoopSourceRef rls) {
    assertTypeID(rls, kRunLoopSourceTypeID);
    pthread_mutex_lock(&rls->lock);
    if ((loadInfo(rls) & kInfoValid) == 0 || rls->runLoops == NULL) {
        pthread_mutex_unlock(&rls->lock);
        return;
    }
    OCFBagRef loops = OCFBagCreateCopy(kOCFAllocatorSystemDefault, rls->runLoops);
    pthread_mutex_unlock(&rls->lock);
    if (loops == NULL) {
        return;
    }
    OCFBagApplyFunction(loops, __CFRunLoopSourceWakeUpLoop, NULL);
    OCFRelease(loops);
}

// Native __CFRunLoopSourceWakeUpLoop, 0x1c01e0b1c. Non-external; the bag applier (a tail call).
static void __CFRunLoopSourceWakeUpLoop(const void *value, void *context) {
    (void)context;
    OCFRunLoopWakeUp((OCFRunLoopRef)value);
}

// Native __CFRunLoopObserverDeallocate, 0x1c01e0b20 (the delta capture). Non-external; the observer class's
// finalize: invalidates the observer, then destroys its lock (a tail call).
static void __CFRunLoopObserverDeallocate(OCFTypeRef cf) {
    OCFRunLoopObserverRef rlo = (OCFRunLoopObserverRef)cf;
    OCFRunLoopObserverInvalidate(rlo);
    pthread_mutex_destroy(&rlo->lock);
}

// Native __CFRunLoopObserverCopyDescription, 0x1c01e0b44 (the delta capture). Non-external; the observer
// class's debug description. The context description comes from the copyDescription callback (with the
// information, NULL included) or, when there is none or it answers NULL, "<CFRunLoopObserver context %p>"
// (kCFAllocatorSystemDefault). The callout is named by dladdr when the symbol starts exactly at it, else
// "???". Valid (info bit 3) and repeats (bit 1) are read with separate acquire loads. The context
// description is released unconditionally, as natively.
static OCFStringRef __CFRunLoopObserverCopyDescription(OCFTypeRef cf) {
    OCFRunLoopObserverRef rlo = (OCFRunLoopObserverRef)cf;
    OCFStringRef contextDesc = NULL;
    if (rlo->context.copyDescription != NULL) {
        contextDesc = rlo->context.copyDescription(rlo->context.info);
    }
    if (contextDesc == NULL) {
        contextDesc = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault, NULL, OF_STR("<CFRunLoopObserver context %p>"), rlo->context.info
        );
    }
    void *callout = (void *)rlo->callout;
    Dl_info dlInfo;
    memset(&dlInfo, 0, sizeof(dlInfo));
    const char *name = "???";
    if (dladdr(callout, &dlInfo) != 0 && dlInfo.dli_sname != NULL && dlInfo.dli_saddr == callout) {
        name = dlInfo.dli_sname;
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFAllocatorRef objectAllocator = OCFGetAllocator(rlo);
    const char *valid = (__atomic_load_n(&rlo->base.info, __ATOMIC_ACQUIRE) & kInfoValid) != 0 ? "Yes" : "No";
    const char *repeats =
        (__atomic_load_n(&rlo->base.info, __ATOMIC_ACQUIRE) & kInfoRepeats) != 0 ? "Yes" : "No";
    OCFStringRef result = OCFStringCreateWithFormat(
        allocator,
        NULL,
        OF_STR(
            "<CFRunLoopObserver %p [%p]>{valid = %s, activities = 0x%lx, repeats = %s, order = %ld, callout "
            "= "
            "%s (%p), context = %@}"
        ),
        rlo,
        objectAllocator,
        valid,
        rlo->activities,
        repeats,
        rlo->order,
        name,
        callout,
        contextDesc
    );
    OCFRelease(contextDesc);
    return result;
}

// Native __CFRunLoopObserverWithHandlerPerform, 0x1c01e0ea0. Non-external; calls the handler block (the
// context information) with the observer and activity.
static void
__CFRunLoopObserverWithHandlerPerform(OCFRunLoopObserverRef rlo, OCFRunLoopActivity activity, void *info) {
    void (^block)(OCFRunLoopObserverRef, OCFRunLoopActivity) =
        (void (^)(OCFRunLoopObserverRef, OCFRunLoopActivity))info;
    block(rlo, activity);
}

// Native CFRunLoopObserverInvalidate, 0x1c01e0fbc. External. A valid observer becomes invalid and gives up
// its information; a bound run loop is retained and the observer leaves every mode (last first) and the
// common modes with the observer unlocked; the release callback then runs on the detached information.
void OCFRunLoopObserverInvalidate(OCFRunLoopObserverRef observer) {
    assertTypeID(observer, kRunLoopObserverTypeID);
    checkForFork();
    pthread_mutex_lock(&observer->lock);
    OCFRetain(observer);
    if ((loadInfo(observer) & kInfoValid) != 0) {
        OCFRunLoopRef rl = observer->runLoop;
        void *info = observer->context.info;
        observer->context.info = NULL;
        clearInfoBits(observer, kInfoValid);
        if (rl != NULL) {
            OCFRetain(rl);
            pthread_mutex_unlock(&observer->lock);
            pthread_mutex_lock(&rl->lock);
            OCFArrayRef array = OCFRunLoopCopyAllModes(rl);
            for (OCFIndex idx = OCFArrayGetCount(array); idx--;) {
                OCFStringRef modeName = OCFArrayGetValueAtIndex(array, idx);
                OCFRunLoopRemoveObserver(rl, observer, modeName);
            }
            OCFRunLoopRemoveObserver(rl, observer, kCommonModesName);
            pthread_mutex_unlock(&rl->lock);
            OCFRelease(array);
            OCFRelease(rl);
            pthread_mutex_lock(&observer->lock);
        }
        if (observer->context.release != NULL) {
            observer->context.release(info);
        }
    }
    pthread_mutex_unlock(&observer->lock);
    OCFRelease(observer);
}

// Native __CFRunLoopTimerDeallocate, 0x1c01e1214 (the delta capture). Non-external; the timer class's
// finalize: sets bit 2 of the timer's bits (no retain during the invalidation), invalidates the timer,
// releases and clears its mode-name set, then destroys its lock (a tail call).
static void __CFRunLoopTimerDeallocate(OCFTypeRef cf) {
    OCFRunLoopTimerRef rlt = (OCFRunLoopTimerRef)cf;
    rlt->bits |= 4;
    OCFRunLoopTimerInvalidate(rlt);
    OCFRelease(rlt->rlModes);
    rlt->rlModes = NULL;
    pthread_mutex_destroy(&rlt->lock);
}

// Native __CFRunLoopTimerCopyDescription, 0x1c01e1250 (the delta capture), with
// __CFRunLoopTimerCopyDescription.cold.1 (0x1c030bdf0, a branch to _OUTLINED_FUNCTION_1_17, the
// dispatch_once of the timer debug information). Non-external; the timer class's debug description. The
// context description is as for observers ("<CFRunLoopTimer context %p>"). The callout is described into two
// 2,048-byte stack buffers that start as "<unknown>" (library and function) with a NULL address by the
// inlined
// __CFRunLoopDebugInfoForRunLoopTimer (timerDebugInfo here). Valid is info bit 3 (acquire load), firing bit 0
// of the bits; the next fire date is also given relative to CFAbsoluteTimeGetCurrent, read after the
// allocator. The context description is released unconditionally, as natively.
static OCFStringRef __CFRunLoopTimerCopyDescription(OCFTypeRef cf) {
    OCFRunLoopTimerRef rlt = (OCFRunLoopTimerRef)cf;
    OCFStringRef contextDesc = NULL;
    if (rlt->context.copyDescription != NULL) {
        contextDesc = rlt->context.copyDescription(rlt->context.info);
    }
    if (contextDesc == NULL) {
        contextDesc = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault, NULL, OF_STR("<CFRunLoopTimer context %p>"), rlt->context.info
        );
    }
    void *callout = (void *)rlt->callout;
    void *info = rlt->context.info;
    char libraryName[2048];
    char functionName[2048];
    strcpy(libraryName, "<unknown>");
    strcpy(functionName, "<unknown>");
    void *functionAddress = NULL;
    timerDebugInfo(rlt, info, libraryName, functionName, &functionAddress);
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFAllocatorRef objectAllocator = OCFGetAllocator(rlt);
    const char *valid = (__atomic_load_n(&rlt->base.info, __ATOMIC_ACQUIRE) & kInfoValid) != 0 ? "Yes" : "No";
    const char *firing = (rlt->bits & 1) != 0 ? "Yes" : "No";
    OCFTimeInterval interval = rlt->interval;
    OCFTimeInterval tolerance = rlt->tolerance;
    OCFAbsoluteTime nextFireDate = rlt->nextFireDate;
    OCFTimeInterval untilFire = nextFireDate - OCFAbsoluteTimeGetCurrent();
    OCFStringRef result = OCFStringCreateWithFormat(
        allocator,
        NULL,
        OF_STR(
            "<CFRunLoopTimer %p [%p]>{valid = %s, firing = %s, interval = %0.09g, tolerance = %0.09g, next "
            "fire date = %0.09g (%0.09g @ %lld), callout = %s (%p / %p) (%s), context = %@}"
        ),
        rlt,
        objectAllocator,
        valid,
        firing,
        interval,
        tolerance,
        nextFireDate,
        untilFire,
        rlt->fireTSR,
        functionName,
        callout,
        functionAddress,
        libraryName,
        contextDesc
    );
    OCFRelease(contextDesc);
    return result;
}

// Native CFRunLoopTimerGetTypeID, 0x1c01e154c. External.
OCFTypeID OCFRunLoopTimerGetTypeID(void) {
    return kRunLoopTimerTypeID;
}

// Native CFRunLoopTimerCreate, 0x1c01e1554, with CFRunLoopTimerCreate.cold.1 (0x1c030bdf4) and .cold.2
// (0x1c030be08, which calls _OUTLINED_FUNCTION_2_15, 0x1c01e5058: the dispatch_once of the timer debug
// information). External. After the fork check a NaN interval stores "NaN was used as an interval for a
// CFRunLoopTimer" and traps. The timer (type 47, 0xB0 extra bytes) is marked valid (info bit 3, a
// compare-and-swap loop) with firing bit 0 of its bits cleared, gets a recursive lock (an attribute zeroed,
// initialized, set to type 2 and destroyed after use), no run loop, an empty mode-name set (system default
// allocator, CFType callbacks) and the order. A negative interval becomes 0 and the tolerance 0; the fire
// date is clamped to 4039289856 and stored with the interval. The fire time is the mach_absolute_time read
// before CFAbsoluteTimeGetCurrent, plus, unless the fire date is below now, the TSR of the remaining time
// clamped to 504911232 s. The context's retain, release and copyDescription are copied, then its information,
// retained through its retain callback when there is one (the version word stays as created, zero). The
// timer_created probe (when enabled: the callout described into two 2,048-byte stack buffers that start as
// "<unknown>" with a NULL address) and the kdebug event 0x21120090 report the fire date and interval in
// milliseconds and the time until the fire date (milliseconds; signed for the probe, unsigned for the event).
OCFRunLoopTimerRef OCFRunLoopTimerCreate(
    OCFAllocatorRef allocator,
    OCFAbsoluteTime fireDate,
    OCFTimeInterval interval,
    OCFOptionFlags flags,
    OCFIndex order,
    OCFRunLoopTimerCallBack callout,
    RunLoopTimerContext *context
) {
    (void)flags;
    checkForFork();
    if (isnan(interval)) {
        crashWithMessage("NaN was used as an interval for a CFRunLoopTimer");
    }
    OCFRunLoopTimerRef rlt =
        (OCFRunLoopTimerRef)_OCFRuntimeCreateInstance(allocator, kRunLoopTimerTypeID, 0xB0, NULL);
    if (rlt == NULL) {
        return NULL;
    }
    setInfoBits(rlt, kInfoValid);
    rlt->bits &= 0xFFFE;
    pthread_mutexattr_t attr;
    memset(&attr, 0, sizeof(attr));
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&rlt->lock, &attr);
    pthread_mutexattr_destroy(&attr);
    rlt->runLoop = NULL;
    rlt->rlModes = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeSetCallBacks);
    rlt->order = order;
    if (interval < 0.0) {
        interval = 0.0;
    }
    rlt->tolerance = 0.0;
    if (fireDate > 4039289856.0) {
        fireDate = 4039289856.0;
    }
    rlt->nextFireDate = fireDate;
    rlt->interval = interval;
    uint64_t nowTSR = mach_absolute_time();
    OCFAbsoluteTime now = OCFAbsoluteTimeGetCurrent();
    if (fireDate < now) {
        rlt->fireTSR = nowTSR;
    } else {
        OCFTimeInterval remaining = fireDate - now;
        if (remaining > 504911232.0) {
            remaining = 504911232.0;
        }
        rlt->fireTSR = __CFTimeIntervalToTSR(remaining) + nowTSR;
    }
    rlt->callout = callout;
    if (context != NULL) {
        rlt->context.retain = context->retain;
        rlt->context.release = context->release;
        rlt->context.copyDescription = context->copyDescription;
        void *info = context->info;
        if (context->retain != NULL) {
            info = (void *)context->retain(info);
        }
        rlt->context.info = info;
    }
    if (CFRUNLOOP_TIMER_CREATED_ENABLED()) {
        void *info = rlt->context.info;
        char libraryName[2048];
        char functionName[2048];
        strcpy(libraryName, "<unknown>");
        strcpy(functionName, "<unknown>");
        void *functionAddress = NULL;
        timerDebugInfo(rlt, info, libraryName, functionName, &functionAddress);
        CFRUNLOOP_TIMER_CREATED(
            rlt,
            (unsigned long)(fireDate * 1000.0),
            (unsigned long)(interval * 1000.0),
            (long)((fireDate - now) * 1000.0),
            libraryName,
            functionName,
            functionAddress
        );
    }
    uint64_t fireDateMilliseconds = (uint64_t)(fireDate * 1000.0);
    uint64_t intervalMilliseconds = (uint64_t)(interval * 1000.0);
    OCFTimeInterval untilFireMilliseconds = (fireDate - now) * 1000.0;
    kdebug_trace(
        0x21120090,
        (uintptr_t)rlt,
        fireDateMilliseconds,
        intervalMilliseconds,
        (uint64_t)untilFireMilliseconds
    );
    return rlt;
}

// Native _runLoopTimerWithBlockContext, 0x1c01e19d4. Non-external; the callout of block timers (also compared
// by the timer debug information) calls the block (the context information) with the timer (a tail call
// through the block's invoke pointer).
static void _runLoopTimerWithBlockContext(OCFRunLoopTimerRef rlt, void *info) {
    void (^block)(OCFRunLoopTimerRef) = (void (^)(OCFRunLoopTimerRef))info;
    block(rlt);
}

// Native CFRunLoopTimerSetNextFireDate, 0x1c01e19e8, with CFRunLoopTimerSetNextFireDate.cold.1 (0x1c030be1c,
// which calls _OUTLINED_FUNCTION_2_15: the dispatch_once of the timer debug information). External. The type
// assertion precedes the fork check; an invalid timer is left alone. The fire date is clamped to 4039289856;
// the fire time is the mach_absolute_time read before CFAbsoluteTimeGetCurrent, plus, unless the fire date is
// below now, the TSR of the remaining time clamped to 504911232 s. Without a run loop both are stored under
// the timer lock. Otherwise the mode names (copied to a stack array whose first 512 bytes at most are zeroed)
// and the run loop are retained, the timer unlocked and the run loop locked; each name is replaced by its
// existing mode (locked by __CFRunLoopCopyMode, or NULL) and released, then under the run loop's timer lock
// the fire time and date are stored and the timer repositioned in each mode found; the modes are unlocked and
// released, the run loop unlocked, woken unless it is the current one, and released. The timer_rescheduled
// probe (when enabled, as in CFRunLoopTimerCreate) and the kdebug event 0x21120098 report the fire date and
// the time until it in milliseconds.
void OCFRunLoopTimerSetNextFireDate(OCFRunLoopTimerRef timer, OCFAbsoluteTime fireDate) {
    assertTypeID(timer, kRunLoopTimerTypeID);
    checkForFork();
    if ((loadInfo(timer) & kInfoValid) == 0) {
        return;
    }
    if (fireDate > 4039289856.0) {
        fireDate = 4039289856.0;
    }
    uint64_t nextFireTSR = mach_absolute_time();
    OCFAbsoluteTime now = OCFAbsoluteTimeGetCurrent();
    if (!(fireDate < now)) {
        OCFTimeInterval remaining = fireDate - now;
        if (remaining > 504911232.0) {
            remaining = 504911232.0;
        }
        nextFireTSR = __CFTimeIntervalToTSR(remaining) + nextFireTSR;
    }
    pthread_mutex_lock(&timer->lock);
    if (timer->runLoop == NULL) {
        timer->fireTSR = nextFireTSR;
        timer->nextFireDate = fireDate;
        pthread_mutex_unlock(&timer->lock);
    } else {
        OCFIndex cnt = OCFSetGetCount(timer->rlModes);
        const void *modes[cnt > 0 ? cnt : 1];
        bzero(modes, (size_t)(8 * cnt) < 512 ? (size_t)(8 * cnt) : 512);
        OCFSetGetValues(timer->rlModes, modes);
        for (OCFIndex idx = 0; idx < cnt; idx++) {
            OCFRetain(modes[idx]);
        }
        OCFRunLoopRef rl = (OCFRunLoopRef)OCFRetain(timer->runLoop);
        pthread_mutex_unlock(&timer->lock);
        pthread_mutex_lock(&rl->lock);
        if (cnt >= 1) {
            for (OCFIndex idx = 0; idx < cnt; idx++) {
                OCFStringRef name = (OCFStringRef)modes[idx];
                modes[idx] = __CFRunLoopCopyMode(rl, name, false);
                OCFRelease(name);
            }
            os_unfair_lock_lock(&rl->timerTSRLock);
            timer->fireTSR = nextFireTSR;
            timer->nextFireDate = fireDate;
            for (OCFIndex idx = 0; idx < cnt; idx++) {
                RunLoopMode *rlm = (RunLoopMode *)modes[idx];
                if (rlm != NULL) {
                    __CFRepositionTimerInMode(rlm, timer, true);
                }
            }
            os_unfair_lock_unlock(&rl->timerTSRLock);
            for (OCFIndex idx = 0; idx < cnt; idx++) {
                RunLoopMode *rlm = (RunLoopMode *)modes[idx];
                if (rlm != NULL) {
                    pthread_mutex_unlock(&rlm->lock);
                    OCFRelease(rlm);
                }
            }
        } else {
            os_unfair_lock_lock(&rl->timerTSRLock);
            timer->fireTSR = nextFireTSR;
            timer->nextFireDate = fireDate;
            os_unfair_lock_unlock(&rl->timerTSRLock);
        }
        pthread_mutex_unlock(&rl->lock);
        if (rl != _CFRunLoopGetButDontCreateCurrent()) {
            OCFRunLoopWakeUp(rl);
        }
        OCFRelease(rl);
    }
    if (CFRUNLOOP_TIMER_RESCHEDULED_ENABLED()) {
        void *info = timer->context.info;
        char libraryName[2048];
        char functionName[2048];
        strcpy(libraryName, "<unknown>");
        strcpy(functionName, "<unknown>");
        void *functionAddress = NULL;
        timerDebugInfo(timer, info, libraryName, functionName, &functionAddress);
        CFRUNLOOP_TIMER_RESCHEDULED(
            timer,
            (unsigned long)(fireDate * 1000.0),
            (long)((fireDate - now) * 1000.0),
            libraryName,
            functionName,
            functionAddress
        );
    }
    uint64_t fireDateMilliseconds = (uint64_t)(fireDate * 1000.0);
    OCFTimeInterval untilFireMilliseconds = (fireDate - now) * 1000.0;
    kdebug_trace(0x21120098, (uintptr_t)timer, fireDateMilliseconds, (uint64_t)untilFireMilliseconds, 0);
}

// Native CFRunLoopTimerGetInterval, 0x1c01e1e88 (corpus delta capture 7). External. The fork check precedes
// the dispatch: a tagged pointer or bridged object is sent -timeInterval; otherwise the interval (0x70),
// without the lock.
OCFTimeInterval OCFRunLoopTimerGetInterval(OCFRunLoopTimerRef timer) {
    checkForFork();
    if (((uintptr_t)timer >> 63) != 0 || __CF_IS_OBJC(kRunLoopTimerTypeID, timer) != 0) {
        return ((OCFTimeInterval (*)(id, SEL))objc_msgSend)((id)timer, sel_getUid("timeInterval"));
    }
    return timer->interval;
}

// Native CFRunLoopTimerInvalidate, 0x1c01e1f98. External. A tagged pointer or bridged object is sent
// -invalidate. Unless bit 2 of the timer's bits is set, the timer is retained for the call. A valid timer
// becomes invalid and gives up its information; a bound run loop and the mode names (copied to a stack array
// whose first 512 bytes at most are zeroed) are retained and the timer leaves each mode and the common modes
// with the timer unlocked; the release callback then runs on the detached information.
void OCFRunLoopTimerInvalidate(OCFRunLoopTimerRef timer) {
    checkForFork();
    if (((uintptr_t)timer >> 63) != 0 || __CF_IS_OBJC(kRunLoopTimerTypeID, timer) != 0) {
        ((void (*)(id, SEL))objc_msgSend)((id)timer, sel_getUid("invalidate"));
        return;
    }
    pthread_mutex_lock(&timer->lock);
    if ((timer->bits & 4) == 0) {
        OCFRetain(timer);
    }
    if ((loadInfo(timer) & kInfoValid) != 0) {
        OCFRunLoopRef rl = timer->runLoop;
        void *info = timer->context.info;
        timer->context.info = NULL;
        clearInfoBits(timer, kInfoValid);
        if (rl != NULL) {
            OCFIndex cnt = OCFSetGetCount(timer->rlModes);
            const void *modes[cnt > 0 ? cnt : 1];
            bzero(modes, (size_t)(8 * cnt) < 512 ? (size_t)(8 * cnt) : 512);
            OCFSetGetValues(timer->rlModes, modes);
            for (OCFIndex idx = 0; idx < cnt; idx++) {
                OCFRetain(modes[idx]);
            }
            OCFRetain(rl);
            pthread_mutex_unlock(&timer->lock);
            pthread_mutex_lock(&rl->lock);
            for (OCFIndex idx = 0; idx < cnt; idx++) {
                OCFRunLoopRemoveTimer(rl, timer, modes[idx]);
            }
            OCFRunLoopRemoveTimer(rl, timer, kCommonModesName);
            pthread_mutex_unlock(&rl->lock);
            for (OCFIndex idx = 0; idx < cnt; idx++) {
                OCFRelease(modes[idx]);
            }
            OCFRelease(rl);
            pthread_mutex_lock(&timer->lock);
        }
        if (timer->context.release != NULL) {
            timer->context.release(info);
        }
    }
    pthread_mutex_unlock(&timer->lock);
    if ((timer->bits & 4) == 0) {
        OCFRelease(timer);
    }
}

// Native CFRunLoopTimerIsValid, 0x1c01e21ec. External. The fork check precedes the dispatch: a tagged pointer
// or bridged object is sent -isValid; otherwise info bit 3 (acquire load).
Boolean OCFRunLoopTimerIsValid(OCFRunLoopTimerRef timer) {
    checkForFork();
    if (((uintptr_t)timer >> 63) != 0 || __CF_IS_OBJC(kRunLoopTimerTypeID, timer) != 0) {
        return ((Boolean (*)(id, SEL))objc_msgSend)((id)timer, sel_getUid("isValid"));
    }
    return (loadInfo(timer) >> 3) & 1;
}

// Native CFRunLoopTimerGetContext, 0x1c01e2250. External. The type assertion precedes the fork check; the
// five context words are copied (version, then two 16-byte pairs).
void OCFRunLoopTimerGetContext(OCFRunLoopTimerRef timer, RunLoopTimerContext *context) {
    assertTypeID(timer, kRunLoopTimerTypeID);
    checkForFork();
    *context = timer->context;
}

// Native CFRunLoopTimerGetTolerance, 0x1c01e22c4 (corpus delta capture 7). External. The fork check
// precedes the dispatch: a tagged pointer or bridged object is sent -tolerance; otherwise the tolerance
// (0x78), without the lock.
OCFTimeInterval OCFRunLoopTimerGetTolerance(OCFRunLoopTimerRef timer) {
    checkForFork();
    if (((uintptr_t)timer >> 63) != 0 || __CF_IS_OBJC(kRunLoopTimerTypeID, timer) != 0) {
        return ((OCFTimeInterval (*)(id, SEL))objc_msgSend)((id)timer, sel_getUid("tolerance"));
    }
    return timer->tolerance;
}

// Native CFRunLoopTimerSetTolerance, 0x1c01e2324 (corpus delta capture 7). External. The fork check precedes
// the dispatch: a tagged pointer or bridged object is sent -setTolerance:. Otherwise, without the lock: with
// an interval above 0 (FCMP, B.LE: a NaN interval takes the other branch) the tolerance is capped at half the
// interval (FCSEL MI: a NaN tolerance becomes the half); otherwise a negative tolerance becomes 0.0 (a NaN
// one is kept). The result is stored at 0x78.
void OCFRunLoopTimerSetTolerance(OCFRunLoopTimerRef timer, OCFTimeInterval tolerance) {
    checkForFork();
    if (((uintptr_t)timer >> 63) != 0 || __CF_IS_OBJC(kRunLoopTimerTypeID, timer) != 0) {
        ((void (*)(id, SEL, OCFTimeInterval))objc_msgSend)((id)timer, sel_getUid("setTolerance:"), tolerance);
        return;
    }
    OCFTimeInterval interval = timer->interval;
    OCFTimeInterval result;
    if (interval > 0.0) {
        OCFTimeInterval half = interval * 0.5;
        result = tolerance < half ? tolerance : half;
    } else {
        result = tolerance < 0.0 ? 0.0 : tolerance;
    }
    timer->tolerance = result;
}

// Native __CFRunLoopCleanseSources, 0x1c01e23c4. Non-external; a mode applier. The version 0 and 1 sources
// are gathered (in a 256-entry stack buffer whose first 512 bytes are zeroed, or an allocation for more than
// 256) and the run loop leaves each source's bag.
static void __CFRunLoopCleanseSources(const void *value, void *context) {
    RunLoopMode *rlm = (RunLoopMode *)value;
    OCFRunLoopRef rl = (OCFRunLoopRef)context;
    OCFIndex cnt0 = 0;
    if (rlm->sources0 != NULL) {
        cnt0 = OCFSetGetCount(rlm->sources0);
    } else if (rlm->sources1 == NULL) {
        return;
    }
    OCFIndex cnt1 = rlm->sources1 != NULL ? OCFSetGetCount(rlm->sources1) : 0;
    OCFIndex cnt = cnt1 + cnt0;
    const void *buffer[256];
    const void **list =
        cnt >= 257 ? OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, 8 * cnt, 0x80040B8603338, 0)
                   : buffer;
    memset(buffer, 0, 512);
    if (rlm->sources0 != NULL) {
        OCFSetGetValues(rlm->sources0, list);
    }
    if (rlm->sources1 != NULL) {
        OCFSetGetValues(rlm->sources1, list + (rlm->sources0 != NULL ? OCFSetGetCount(rlm->sources0) : 0));
    }
    for (OCFIndex idx = 0; idx < cnt; idx++) {
        OCFRunLoopSourceRef rls = (OCFRunLoopSourceRef)list[idx];
        pthread_mutex_lock(&rls->lock);
        if (rls->runLoops != NULL) {
            OCFBagRemoveValue(rls->runLoops, rl);
        }
        pthread_mutex_unlock(&rls->lock);
    }
    if (list != buffer) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, list);
    }
}

// Native __CFRunLoopDeallocateSources, 0x1c01e2568. Non-external; a mode applier. The sources are gathered as
// in
// __CFRunLoopCleanseSources and retained, both sets emptied, then each source loses the run loop from its bag
// and either its port leaves the port set (version 1) or its cancel callback runs with the mode name (version
// 0).
static void __CFRunLoopDeallocateSources(const void *value, void *context) {
    RunLoopMode *rlm = (RunLoopMode *)value;
    OCFRunLoopRef rl = (OCFRunLoopRef)context;
    OCFIndex cnt0 = 0;
    if (rlm->sources0 != NULL) {
        cnt0 = OCFSetGetCount(rlm->sources0);
    } else if (rlm->sources1 == NULL) {
        return;
    }
    OCFIndex cnt1 = rlm->sources1 != NULL ? OCFSetGetCount(rlm->sources1) : 0;
    OCFIndex cnt = cnt1 + cnt0;
    const void *buffer[256];
    const void **list =
        cnt >= 257 ? OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, 8 * cnt, 0x80040B8603338, 0)
                   : buffer;
    memset(buffer, 0, 512);
    if (rlm->sources0 != NULL) {
        OCFSetGetValues(rlm->sources0, list);
    }
    if (rlm->sources1 != NULL) {
        OCFSetGetValues(rlm->sources1, list + (rlm->sources0 != NULL ? OCFSetGetCount(rlm->sources0) : 0));
    }
    for (OCFIndex idx = 0; idx < cnt; idx++) {
        OCFRetain(list[idx]);
    }
    if (rlm->sources0 != NULL) {
        OCFSetRemoveAllValues(rlm->sources0);
    }
    if (rlm->sources1 != NULL) {
        OCFSetRemoveAllValues(rlm->sources1);
    }
    for (OCFIndex idx = 0; idx < cnt; idx++) {
        OCFRunLoopSourceRef rls = (OCFRunLoopSourceRef)list[idx];
        pthread_mutex_lock(&rls->lock);
        if (rls->runLoops != NULL) {
            OCFBagRemoveValue(rls->runLoops, rl);
        }
        pthread_mutex_unlock(&rls->lock);
        if (rls->context.version0.version == 1) {
            mach_port_t port = rls->context.version1.getPort(rls->context.version1.info);
            if (port != MACH_PORT_NULL) {
                mach_port_extract_member(mach_task_self(), port, rlm->portSet);
            }
        } else if (rls->context.version0.version == 0) {
            if (rls->context.version0.cancel != NULL) {
                rls->context.version0.cancel(rls->context.version0.info, rl, rlm->name);
            }
        }
        OCFRelease(rls);
    }
    if (list != buffer) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, list);
    }
}

// Native __CFRunLoopDeallocateObservers, 0x1c01e27a8. Non-external; a mode applier. The observers are copied
// (stack buffer as above) and retained, the array emptied, then each is cancelled and released.
static void __CFRunLoopDeallocateObservers(const void *value, void *context) {
    RunLoopMode *rlm = (RunLoopMode *)value;
    (void)context;
    if (rlm->observers == NULL) {
        return;
    }
    OCFIndex cnt = OCFArrayGetCount(rlm->observers);
    const void *buffer[256];
    const void **list =
        cnt >= 257 ? OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, 8 * cnt, 0x80040B8603338, 0)
                   : buffer;
    memset(buffer, 0, 512);
    OCFArrayGetValues(rlm->observers, (OCFRange){0, cnt}, list);
    for (OCFIndex idx = 0; idx < cnt; idx++) {
        OCFRetain(list[idx]);
    }
    OCFArrayRemoveAllValues(rlm->observers);
    for (OCFIndex idx = 0; idx < cnt; idx++) {
        __CFRunLoopObserverCancel((OCFRunLoopObserverRef)list[idx]);
        OCFRelease(list[idx]);
    }
    if (list != buffer) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, list);
    }
}

// Native __CFRunLoopDeallocateTimers, 0x1c01e291c. Non-external; a mode applier killing every timer, then
// emptying the array.
static void __CFRunLoopDeallocateTimers(const void *value, void *context) {
    RunLoopMode *rlm = (RunLoopMode *)value;
    if (rlm->timers == NULL) {
        return;
    }
    OCFIndex count = OCFArrayGetCount(rlm->timers);
    if (count != 0) {
        OCFArrayApplyFunction(rlm->timers, (OCFRange){0, count}, __CFRunLoopKillOneTimer, context);
        OCFArrayRemoveAllValues(rlm->timers);
    }
}

// Native __CFRunLoopKillOneTimer, 0x1c01e2978. Non-external; an array applier unbinding a timer from every
// mode.
static void __CFRunLoopKillOneTimer(const void *value, void *context) {
    OCFRunLoopTimerRef rlt = (OCFRunLoopTimerRef)value;
    (void)context;
    pthread_mutex_lock(&rlt->lock);
    OCFSetRemoveAllValues(rlt->rlModes);
    rlt->runLoop = NULL;
    pthread_mutex_unlock(&rlt->lock);
}

// Native _CFRunLoopRunCleanup, 0x1c01e2a2c, with _CFRunLoopRunCleanup.cold.1 (0x1c030be70). Non-external. The
// context is {run loop, mode, previous mode}: a zero nesting index stores "CFRunLoopRun underflow" and traps;
// the index is decremented, the mode unlocked and released, the previous mode restored and the run loop
// unlocked.
static void _CFRunLoopRunCleanup(void *context) {
    void **params = context;
    OCFRunLoopRef rl = params[0];
    if (rl->nestingIndex == 0) {
        crashWithMessage("CFRunLoopRun underflow");
    }
    rl->nestingIndex--;
    RunLoopMode *rlm = params[1];
    pthread_mutex_unlock(&rlm->lock);
    OCFRelease(rlm);
    rl = params[0];
    rl->currentMode = params[2];
    pthread_mutex_unlock(&rl->lock);
}

// Native __CFRunLoopDoObservers, 0x1c01e2a84. Non-external. The observers for the activity that are valid and
// either not firing or allowed to recurse are retained into a stack array (one entry beyond 1,024 observers,
// which are collected in a malloc_type_malloc block instead; the stack array's first 512 bytes at most are
// zeroed). With both locks dropped each still-valid observer is marked firing and called out (with a
// per-callout pool when the run loop is NULL or asks for one), between kdebug events whose second argument
// packs the count, the position and the low seven info bits; a non-repeating observer is invalidated before
// its firing bit is cleared.
static void __CFRunLoopDoObservers(OCFRunLoopRef rl, RunLoopMode *rlm, OCFRunLoopActivity activity) {
    checkForFork();
    if (rlm->observers == NULL) {
        return;
    }
    OCFIndex cnt = OCFArrayGetCount(rlm->observers);
    if (cnt < 1) {
        return;
    }
    size_t bufferSize = 8 * (size_t)((uint64_t)cnt <= 1024 ? cnt : 1);
    OCFRunLoopObserverRef buffer[bufferSize / 8];
    bzero(buffer, bufferSize < 512 ? bufferSize : 512);
    OCFRunLoopObserverRef *collectedObservers =
        (uint64_t)cnt <= 1024 ? buffer : malloc_type_malloc(8 * cnt, 0x2004093837F09);
    OCFIndex obsCnt = 0;
    for (OCFIndex idx = 0; idx < cnt; idx++) {
        OCFRunLoopObserverRef rlo = (OCFRunLoopObserverRef)OCFArrayGetValueAtIndex(rlm->observers, idx);
        if ((rlo->activities & activity) == 0 || (loadInfo(rlo) & kInfoValid) == 0) {
            continue;
        }
        if ((loadInfo(rlo) & kInfoFiring) == 0 || (loadInfo(rlo) & kInfoAllowsRecursion) != 0) {
            collectedObservers[obsCnt++] = (OCFRunLoopObserverRef)OCFRetain(rlo);
        }
    }
    if (obsCnt > 0) {
        uint8_t idx = rl->nestingIndex;
        pthread_mutex_unlock(&rlm->lock);
        pthread_mutex_unlock(&rl->lock);
        uint64_t countBits = (uint64_t)obsCnt & 0xFFFF;
        for (OCFIndex position = 0; position < obsCnt; position++) {
            OCFRunLoopObserverRef rlo = collectedObservers[position];
            pthread_mutex_lock(&rlo->lock);
            if ((loadInfo(rlo) & kInfoValid) == 0) {
                pthread_mutex_unlock(&rlo->lock);
                OCFRelease(rlo);
                continue;
            }
            uint64_t infoBefore = loadInfo(rlo);
            OCFRunLoopObserverCallBack callout = rlo->callout;
            void *info = rlo->context.info;
            void *tracedCallout = (void *)callout;
            if (callout == __CFRunLoopObserverWithHandlerPerform) {
                tracedCallout = *(void **)((uint8_t *)info + 16);
            }
            OCFIndex order = rlo->order;
            uint64_t traceBits = loadInfo(rlo);
            setInfoBits(rlo, kInfoFiring);
            pthread_mutex_unlock(&rlo->lock);
            kdebug_trace(0x211200E1, idx, (uint64_t)order, activity, (uintptr_t)tracedCallout);
            void *pool = NULL;
            if (rl == NULL || rl->perCalloutARP) {
                pool = _OCFAutoreleasePoolPush();
            }
            __CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__(callout, rlo, activity, info);
            __CFRunLoopPerCalloutARPEnd(pool);
            kdebug_trace(
                0x211200E2,
                idx,
                countBits | (((uint64_t)position & 0xFFFF) << 16) | ((traceBits & 0x7F) << 32),
                0,
                (uintptr_t)callout
            );
            if ((infoBefore & kInfoRepeats) == 0) {
                OCFRunLoopObserverInvalidate(rlo);
            }
            clearInfoBits(rlo, kInfoFiring);
            OCFRelease(rlo);
        }
        pthread_mutex_lock(&rl->lock);
        pthread_mutex_lock(&rlm->lock);
    }
    if (collectedObservers != buffer) {
        free(collectedObservers);
    }
}

// Native __CFRunLoopRun, 0x1c01e2d94, with _____CFRunLoopRun_block_invoke (0x1c01e38e0) and
// _____CFRunLoopRun_block_invoke_2 (0x1c01e3920). Non-external. A pending stop of the current run or of the
// mode returns kCFRunLoopRunStopped at once. The main queue port is serviced for a common mode of the main
// run loop on the main thread outside the queue's own servicing; the mode queue's port is fetched (zero
// stores "Unable to get port for run loop mode queue" and traps). A positive timeout up to 504911232 s arms a
// one-shot dispatch timer on a global queue (main thread: the main QoS, else QOS_CLASS_UTILITY; overcommit)
// that wakes the run loop and whose cancel handler releases it; zero or less polls, more never times out.
// Each iteration: before-timers and before-sources observers, blocks, version 0 sources (then blocks again),
// a zero-timeout look at the main queue port unless it was dispatched last time, then (unless polling) the
// before-waiting observers, the sleeping bit, the main queue port joins the port set, both locks drop and the
// thread waits on the port set (timeout 0xFFFFFFFF, or 0 when polling), draining the mode queue while its
// port wakes the loop until its timer fires. The wake-up reason (1 none, 2 wake-up port, 3 timer or mode
// queue, 4 main queue, 5 a source) is traced, the locks are retaken, the sleep time accumulates, the
// after-waiting observers run and the woken port is handled: timers, the main queue (pool, slot 4 set to 6)
// or a version 1 source, whose reply is sent and freed; an unknown port is logged. The result is
// kCFRunLoopRunHandledSource, kCFRunLoopRunTimedOut, kCFRunLoopRunStopped or the mode's emptiness (continue
// while not empty).
static int32_t
__CFRunLoopRun(OCFRunLoopRef rl, RunLoopMode *rlm, OCFOptionFlags options, OCFTimeInterval seconds) {
    uint64_t startTSR = mach_absolute_time();
    uint8_t *stopped = (uint8_t *)rl + offsetof(struct __OCFRunLoop, stopped);
    if (stopped[rl->nestingIndex] != 0) {
        stopped[rl->nestingIndex] = 0;
        return kOCFRunLoopRunStopped;
    }
    if (rlm->stopped != 0) {
        rlm->stopped = 0;
        return kOCFRunLoopRunStopped;
    }
    mach_port_name_t dispatchPort = MACH_PORT_NULL;
    if (pthread_main_np() == 1 && _OCFGetTSD(kTSDIsInDispatchMainQueue) == NULL &&
        OCFRunLoopGetMain() == rl && OCFSetContainsValue(rl->commonModes, rlm->name)) {
        dispatchPort = _dispatch_get_main_queue_port_4CF();
    }
    mach_port_name_t modeQueuePort = MACH_PORT_NULL;
    if (rlm->queue != NULL) {
        modeQueuePort = _dispatch_runloop_root_queue_get_port_4CF(rlm->queue);
        if (modeQueuePort == MACH_PORT_NULL) {
            crashWithMessage("Unable to get port for run loop mode queue");
        }
    }
    dispatch_source_t timeoutTimer = NULL;
    uint64_t termTSR;
    if (seconds <= 0.0) {
        termTSR = 0;
    } else if (seconds <= 504911232.0) {
        termTSR = __CFTimeIntervalToTSR(seconds) + startTSR;
        dispatch_qos_class_t qos = pthread_main_np() == 1 ? qos_class_main() : QOS_CLASS_UTILITY;
        dispatch_queue_t queue = dispatch_get_global_queue(qos, kDispatchQueueOvercommit);
        dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
        OCFRetain(rl);
        dispatch_source_set_event_handler(timer, ^{
          CFRUNLOOP_WAKEUP_FOR_TIMEOUT();
          kdebug_trace(0x211200AC, (uintptr_t)rl, 0, 0, 0);
          OCFRunLoopWakeUp(rl);
        });
        dispatch_source_set_cancel_handler(timer, ^{
          OCFRelease(rl);
        });
        uint64_t ns = (uint64_t)((seconds + __CFTSRToTimeInterval(startTSR)) * 1000000000.0);
        dispatch_source_set_timer(timer, dispatch_time(1, (int64_t)ns), DISPATCH_TIME_FOREVER, 1000);
        timeoutTimer = timer;
        dispatch_resume(timer);
    } else {
        termTSR = UINT64_MAX;
    }
    uint32_t stopAfterHandle = options & 1;
    uint8_t *ignoreWakeUps = (uint8_t *)rl + offsetof(struct __OCFRunLoop, ignoreWakeUps);
    bool timeoutIsFinite = termTSR != 0;
    int32_t retVal;
    bool keepGoing;
    do {
        __cftrace_runloop_trace_iteration_start(rl);
        void *voucherCopy = NULL;
        voucher_mach_msg_state_t voucherState = VOUCHER_MACH_MSG_STATE_UNCHANGED;
        uint8_t msgBuffer[3072];
        mach_msg_header_t *msg = NULL;
        mach_port_t livePort = MACH_PORT_NULL;
        mach_port_name_t waitSet = rlm->portSet;
        ignoreWakeUps[rl->nestingIndex] = 0;
        if ((rlm->observerMask & kOCFRunLoopBeforeTimers) != 0) {
            __CFRunLoopDoObservers(rl, rlm, kOCFRunLoopBeforeTimers);
        }
        if ((rlm->observerMask & kOCFRunLoopBeforeSources) != 0) {
            __CFRunLoopDoObservers(rl, rlm, kOCFRunLoopBeforeSources);
        }
        __CFRunLoopDoBlocks(rl, rlm, 0);
        bool sourceHandledThisLoop = __CFRunLoopDoSources0(rl, rlm, stopAfterHandle);
        bool sleeps = timeoutIsFinite;
        if (sourceHandledThisLoop) {
            __CFRunLoopDoBlocks(rl, rlm, 1);
            sleeps = false;
        }
        if (dispatchPort != MACH_PORT_NULL && rl->didDispatchMainQueue == 0) {
            msg = (mach_msg_header_t *)msgBuffer;
            if (__CFRunLoopServiceMachPort(dispatchPort, &msg, &livePort, 0, &voucherState, NULL)) {
                goto handle_msg;
            }
        }
        rl->didDispatchMainQueue = 0;
        OCFAbsoluteTime sleepStart;
        if (sleeps) {
            if ((rlm->observerMask & kOCFRunLoopBeforeWaiting) != 0) {
                __CFRunLoopDoObservers(rl, rlm, kOCFRunLoopBeforeWaiting);
            }
            setInfoBits(rl, kRunLoopSleeping);
            if (dispatchPort != MACH_PORT_NULL) {
                mach_port_insert_member(mach_task_self(), dispatchPort, waitSet);
            }
            pthread_mutex_unlock(&rlm->lock);
            pthread_mutex_unlock(&rl->lock);
            sleepStart = OCFAbsoluteTimeGetCurrent();
        } else {
            setInfoBits(rl, kRunLoopSleeping);
            if (dispatchPort != MACH_PORT_NULL) {
                mach_port_insert_member(mach_task_self(), dispatchPort, waitSet);
            }
            pthread_mutex_unlock(&rlm->lock);
            pthread_mutex_unlock(&rl->lock);
            sleepStart = 0.0;
        }
        uint32_t timeout = sleeps ? 0xFFFFFFFF : 0;
        kdebug_trace(0x2111000D, rl->nestingIndex, waitSet, timeout, 0);
        kdebug_trace(0x1414000D, rl->nestingIndex, waitSet, timeout, 0);
        msg = (mach_msg_header_t *)msgBuffer;
        __CFRunLoopServiceMachPort(waitSet, &msg, &livePort, timeout, &voucherState, &voucherCopy);
        bool isQueueOrWakeUp = livePort == modeQueuePort;
        if (modeQueuePort != MACH_PORT_NULL && livePort == modeQueuePort) {
            for (;;) {
                while (_dispatch_runloop_root_queue_perform_4CF(rlm->queue)) {
                }
                if (rlm->timerFired) {
                    rlm->timerFired = 0;
                    isQueueOrWakeUp = true;
                    livePort = modeQueuePort;
                    break;
                }
                if (msg != NULL && msg != (mach_msg_header_t *)msgBuffer) {
                    free(msg);
                }
                msg = (mach_msg_header_t *)msgBuffer;
                __CFRunLoopServiceMachPort(waitSet, &msg, &livePort, timeout, &voucherState, &voucherCopy);
                if (livePort != modeQueuePort) {
                    isQueueOrWakeUp = false;
                    break;
                }
            }
        }
        uint64_t reason;
        if (livePort == MACH_PORT_NULL) {
            reason = 1;
        } else {
            if (livePort == rl->wakeUpPort) {
                isQueueOrWakeUp = true;
            }
            reason = livePort == rl->wakeUpPort ? 2 : 3;
            if (!isQueueOrWakeUp) {
                reason = livePort == rlm->timerPort ? 3 : (livePort != dispatchPort ? 5 : 4);
            }
        }
        kdebug_trace(0x2111000E, rl->nestingIndex, livePort, reason, 0);
        kdebug_trace(0x1414000E, rl->nestingIndex, livePort, reason, 0);
        pthread_mutex_lock(&rl->lock);
        pthread_mutex_lock(&rlm->lock);
        if (sleeps) {
            rl->sleepTime = rl->sleepTime + (OCFAbsoluteTimeGetCurrent() - sleepStart);
            if (dispatchPort != MACH_PORT_NULL) {
                mach_port_extract_member(mach_task_self(), dispatchPort, waitSet);
            }
            ignoreWakeUps[rl->nestingIndex] = 1;
            clearInfoBits(rl, kRunLoopSleeping);
            if ((rlm->observerMask & kOCFRunLoopAfterWaiting) != 0) {
                __CFRunLoopDoObservers(rl, rlm, kOCFRunLoopAfterWaiting);
            }
        } else {
            rl->sleepTime = rl->sleepTime + 0.0;
            if (dispatchPort != MACH_PORT_NULL) {
                mach_port_extract_member(mach_task_self(), dispatchPort, waitSet);
            }
            ignoreWakeUps[rl->nestingIndex] = 1;
            clearInfoBits(rl, kRunLoopSleeping);
        }
    handle_msg:
        ignoreWakeUps[rl->nestingIndex] = 1;
        if (livePort == MACH_PORT_NULL) {
            CFRUNLOOP_WAKEUP_FOR_NOTHING();
        } else if (livePort == rl->wakeUpPort) {
            CFRUNLOOP_WAKEUP_FOR_WAKEUP();
        } else if (livePort == modeQueuePort) {
            CFRUNLOOP_WAKEUP_FOR_TIMER();
            if (!__CFRunLoopDoTimers(rl, rlm, mach_absolute_time())) {
                __CFArmNextTimerInMode(rlm, rl);
            }
        } else if (livePort == rlm->timerPort) {
            CFRUNLOOP_WAKEUP_FOR_TIMER();
            if (!__CFRunLoopDoTimers(rl, rlm, mach_absolute_time())) {
                rlm->timerSoftDeadline = UINT64_MAX;
                rlm->timerHardDeadline = UINT64_MAX;
                __CFArmNextTimerInMode(rlm, rl);
            }
        } else if (livePort == dispatchPort) {
            CFRUNLOOP_WAKEUP_FOR_DISPATCH();
            pthread_mutex_unlock(&rlm->lock);
            pthread_mutex_unlock(&rl->lock);
            _OCFSetTSD(kTSDIsInDispatchMainQueue, (void *)6, NULL);
            void *pool = _OCFAutoreleasePoolPush();
            kdebug_trace(0x211200DD, (uintptr_t)rl, (uintptr_t)rlm, (uintptr_t)msg, dispatchPort);
            __CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__(msg);
            kdebug_trace(0x211200DE, (uintptr_t)rl, (uintptr_t)rlm, (uintptr_t)msg, dispatchPort);
            __CFRunLoopPerCalloutARPEnd(pool);
            _OCFSetTSD(kTSDIsInDispatchMainQueue, NULL, NULL);
            pthread_mutex_lock(&rl->lock);
            pthread_mutex_lock(&rlm->lock);
            sourceHandledThisLoop = true;
            rl->didDispatchMainQueue = 1;
        } else {
            CFRUNLOOP_WAKEUP_FOR_SOURCE();
            kdebug_trace(0x211200D9, 0, livePort, 0, 0);
            void *previousVoucher =
                _OCFSetTSD(kTSDMachMessageVoucher, voucherCopy, (OCFTSDDestructor)os_release);
            checkForFork();
            OCFRunLoopSourceRef rls = NULL;
            if (rlm->portToV1SourceMap != NULL) {
                rls = (OCFRunLoopSourceRef)OCFDictionaryGetValue(
                    rlm->portToV1SourceMap, (const void *)(uintptr_t)livePort
                );
            }
            if (rls != NULL) {
                mach_msg_header_t *reply = NULL;
                bool handled = __CFRunLoopDoSource1(rl, rlm, rls, msg, msg->msgh_size, &reply);
                if (reply != NULL) {
                    mach_msg(reply, MACH_SEND_MSG, reply->msgh_size, 0, MACH_PORT_NULL, 0, MACH_PORT_NULL);
                    OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, reply);
                }
                sourceHandledThisLoop = handled || sourceHandledThisLoop;
            } else {
                os_log_error(
                    _CFOSLog(),
                    "__CFRunLoopModeFindSourceForMachPort returned NULL for mode '%@' livePort: %u",
                    rlm->name,
                    livePort
                );
            }
            _OCFSetTSD(kTSDMachMessageVoucher, previousVoucher, (OCFTSDDestructor)os_release);
            kdebug_trace(0x211200DA, 0, 0, 0, 0);
        }
        if (msg != NULL && msg != (mach_msg_header_t *)msgBuffer) {
            free(msg);
        }
        __CFRunLoopDoBlocks(rl, rlm, 2);
        if (stopAfterHandle != 0 && sourceHandledThisLoop) {
            keepGoing = false;
            retVal = kOCFRunLoopRunHandledSource;
        } else if (termTSR < mach_absolute_time()) {
            keepGoing = false;
            retVal = kOCFRunLoopRunTimedOut;
        } else if (stopped[rl->nestingIndex] != 0) {
            keepGoing = false;
            retVal = kOCFRunLoopRunStopped;
            stopped[rl->nestingIndex] = 0;
        } else if (rlm->stopped != 0) {
            keepGoing = false;
            retVal = kOCFRunLoopRunStopped;
            rlm->stopped = 0;
        } else {
            retVal = __CFRunLoopModeIsEmpty(rl, rlm);
            keepGoing = retVal == 0;
        }
        voucher_mach_msg_revert(voucherState);
        objc_release((id)voucherCopy);
        kdebug_trace(0x2111000A, rl->nestingIndex, 0, 0, 0);
    } while (keepGoing);
    if (timeoutTimer != NULL) {
        dispatch_source_cancel(timeoutTimer);
        dispatch_release(timeoutTimer);
    }
    return retVal;
}

// Native __CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__, 0x1c01e37c0. Non-external; a frame of
// its own for backtraces (the callout is not a tail call).
static void __CFRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__(
    OCFRunLoopObserverCallBack func, OCFRunLoopObserverRef observer, OCFRunLoopActivity activity, void *info
) {
    if (func != NULL) {
        func(observer, activity, info);
        __asm__ volatile("");
    }
}

// Native __CFRunLoopPerCalloutARPEnd, 0x1c01e37e8. Non-external; pops a per-callout pool.
static void __CFRunLoopPerCalloutARPEnd(void *pool) {
    if (pool != NULL) {
        _OCFAutoreleasePoolPop(pool);
    }
}

// Native __cftrace_runloop_trace_iteration_start, 0x1c01e3928. Non-external. With kdebug code 0x21110008
// enabled, the current mode is copied and identified: the default and common modes, NSModalPanelRunLoopMode,
// NSEventTrackingRunLoopMode, UITrackingRunLoopMode and "com.apple.run-loop-mode.view-bridge.blocks"
// (CFEqual, in that order) use the fixed IDs -2 to -7; any other name is published with kdebug_trace_string,
// from its UTF-8 C string pointer or else from a stack copy (the first 176 units when longer, as a substring
// with the NULL allocator that replaces the copied mode; the buffer is the maximum UTF-8 size rounded up past
// a multiple of 16; a failed conversion gives -1). A published ID of -7 or above (unsigned) is released at
// once and becomes -1. After the mode is released, event 0x21110009 records the nesting index, whether the
// current run loop is the main one and the ID; a published string is then released.
static void __cftrace_runloop_trace_iteration_start(OCFRunLoopRef rl) {
    const uint32_t code = 0x21110008;
    if (!kdebug_is_enabled(code)) {
        return;
    }
    OCFStringRef mode = OCFRunLoopCopyCurrentMode(rl);
    bool published;
    uint64_t stringID;
    if (OCFEqual(mode, kDefaultModeName)) {
        published = false;
        stringID = (uint64_t)-2;
    } else if (OCFEqual(mode, kCommonModesName)) {
        published = false;
        stringID = (uint64_t)-3;
    } else if (OCFEqual(mode, OF_STR("NSModalPanelRunLoopMode"))) {
        published = false;
        stringID = (uint64_t)-4;
    } else if (OCFEqual(mode, OF_STR("NSEventTrackingRunLoopMode"))) {
        published = false;
        stringID = (uint64_t)-5;
    } else if (OCFEqual(mode, OF_STR("UITrackingRunLoopMode"))) {
        published = false;
        stringID = (uint64_t)-6;
    } else if (OCFEqual(mode, OF_STR("com.apple.run-loop-mode.view-bridge.blocks"))) {
        published = false;
        stringID = (uint64_t)-7;
    } else {
        const char *pointer = OCFStringGetCStringPtr(mode, kOCFStringEncodingUTF8);
        if (pointer != NULL) {
            stringID = kdebug_trace_string(code, 0, pointer);
            published = true;
        } else {
            OCFIndex length = OCFStringGetLength(mode);
            if (length > 0xB0) {
                length = 0xB0;
                OCFStringRef prefix = OCFStringCreateWithSubstring(NULL, mode, (OCFRange){0, 0xB0});
                OCFRelease(mode);
                mode = prefix;
            }
            OCFIndex maximum = OCFStringGetMaximumSizeForEncoding(length, kOCFStringEncodingUTF8);
            char buffer[(maximum + 16) & ~(OCFIndex)15];
            Boolean converted = OCFStringGetCString(mode, buffer, maximum + 1, kOCFStringEncodingUTF8);
            published = converted;
            stringID = converted ? kdebug_trace_string(code, 0, buffer) : (uint64_t)-1;
        }
        if (stringID >= (uint64_t)-7) {
            kdebug_trace_string(code, stringID, NULL);
            published = false;
            stringID = (uint64_t)-1;
        }
    }
    OCFRelease(mode);
    uint8_t nestingIndex = rl->nestingIndex;
    bool isMain = _OCFRunLoopCurrentIsMain();
    kdebug_trace(code + 1, nestingIndex, isMain, stringID, 0);
    if (published) {
        kdebug_trace_string(code, stringID, NULL);
    }
}

// Native __CFRunLoopDoBlocks, 0x1c01e3bb0. Non-external. With a mode name, the block list is detached and the
// run loop and mode unlocked; each item for this mode (a name equal to it, or the common modes while this
// mode is common; for a set of names, membership likewise) is unlinked, its mode released and the item freed,
// then its block runs (per-callout pool when the run loop asks for one) and is released, between kdebug
// events. The remaining items are put back in front of any added meanwhile.
static void __CFRunLoopDoBlocks(OCFRunLoopRef rl, RunLoopMode *rlm, uint32_t kind) {
    RunLoopBlockItem *head = rl->blocksHead;
    if (head == NULL) {
        return;
    }
    OCFStringRef curMode = rlm->name;
    if (curMode == NULL) {
        return;
    }
    RunLoopBlockItem *tail = rl->blocksTail;
    rl->blocksHead = NULL;
    rl->blocksTail = NULL;
    OCFSetRef commonModes = rl->commonModes;
    pthread_mutex_unlock(&rlm->lock);
    pthread_mutex_unlock(&rl->lock);
    uint64_t count = 0;
    RunLoopBlockItem *prev = NULL;
    uint8_t idx = rl->nestingIndex;
    RunLoopBlockItem *item = head;
    do {
        RunLoopBlockItem *curr = item;
        item = item->next;
        bool doit;
        if (OCFGetTypeID(curr->mode) == kStringTypeID) {
            doit = OCFEqual(curr->mode, curMode) ||
                   (OCFEqual(curr->mode, kCommonModesName) && OCFSetContainsValue(commonModes, curMode));
        } else {
            doit = OCFSetContainsValue((OCFSetRef)curr->mode, curMode) ||
                   (OCFSetContainsValue((OCFSetRef)curr->mode, kCommonModesName) &&
                    OCFSetContainsValue(commonModes, curMode));
        }
        if (!doit) {
            prev = curr;
            continue;
        }
        if (prev != NULL) {
            prev->next = item;
        }
        if (curr == head) {
            head = item;
        }
        if (curr == tail) {
            tail = prev;
        }
        void (^block)(void) = curr->block;
        OCFRelease(curr->mode);
        free(curr);
        void *invoke = *(void **)((uint8_t *)block + 16);
        void *pool = rl->perCalloutARP ? _OCFAutoreleasePoolPush() : NULL;
        kdebug_trace(0x211200E9, idx, 0, kind, (uintptr_t)invoke);
        __CFRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__(block);
        __CFRunLoopPerCalloutARPEnd(pool);
        _Block_release(block);
        kdebug_trace(0x211200EA, idx, count++, 0, 0);
    } while (item != NULL);
    pthread_mutex_lock(&rl->lock);
    pthread_mutex_lock(&rlm->lock);
    if (head != NULL && tail != NULL) {
        tail->next = rl->blocksHead;
        rl->blocksHead = head;
        if (rl->blocksTail == NULL) {
            rl->blocksTail = tail;
        }
    }
}

// Native __CFRunLoopDoSources0, 0x1c01e3da8. Non-external. The signalled valid version 0 sources are
// collected (one source, or an array); with both locks dropped a single source is performed, or the array is
// sorted by order and signal time and performed in turn. The result is the last source's result, or true when
// stopping after a handled source.
static bool __CFRunLoopDoSources0(OCFRunLoopRef rl, RunLoopMode *rlm, uint32_t stopAfterHandle) {
    kdebug_trace(0x211200D5, (uintptr_t)rl, (uintptr_t)rlm, stopAfterHandle, 0);
    checkForFork();
    OCFTypeRef sources = NULL;
    bool sourceHandled = false;
    if (rlm->sources0 != NULL && OCFSetGetCount(rlm->sources0) >= 1) {
        OCFSetApplyFunction(rlm->sources0, __CFRunLoopCollectSources0, &sources);
        if (sources != NULL) {
            pthread_mutex_unlock(&rlm->lock);
            pthread_mutex_unlock(&rl->lock);
            if (OCFGetTypeID(sources) == kRunLoopSourceTypeID) {
                sourceHandled = __CFRunLoopDoSource0(rl, (OCFRunLoopSourceRef)sources);
            } else {
                OCFIndex cnt = OCFArrayGetCount(sources);
                OCFArraySortValues(
                    (OCFMutableArrayRef)sources, (OCFRange){0, cnt}, __CFRunLoopSourceComparator, NULL
                );
                for (OCFIndex idx = 0; idx < cnt; idx++) {
                    OCFRunLoopSourceRef rls = (OCFRunLoopSourceRef)OCFArrayGetValueAtIndex(sources, idx);
                    sourceHandled = __CFRunLoopDoSource0(rl, rls);
                    if (stopAfterHandle != 0 && sourceHandled) {
                        sourceHandled = true;
                        break;
                    }
                }
            }
            OCFRelease(sources);
            pthread_mutex_lock(&rl->lock);
            pthread_mutex_lock(&rlm->lock);
        }
    }
    kdebug_trace(0x211200D6, (uintptr_t)rl, (uintptr_t)rlm, stopAfterHandle, 0);
    return sourceHandled;
}

// Native __CFRunLoopServiceMachPort, 0x1c01e3f74, with __CFRunLoopServiceMachPort.cold.1 (0x1c030be84).
// Non-external. Receives on the port into the buffer (first 3,072 bytes; options 0x7000806, plus
// MACH_RCV_TIMEOUT unless the timeout is 0xFFFFFFFF; the poll or sleep probe), reverts the previous voucher
// state and adopts the message's, copying the voucher when asked (nothing when unchanged), then the wakeup
// probe. A too-large message reallocates the buffer (never the caller's: the first reallocation starts from
// NULL) to its size plus the 68-byte maximum trailer, rounded to 4, releases the voucher copy and retries; a
// timeout frees a reallocated buffer and reports no port; success reports the local port. Any other result
// traps: header or body errors name the resource shortage (no message for other special bits), anything else
// stores mach_error_string.
static bool __CFRunLoopServiceMachPort(
    mach_port_name_t port,
    mach_msg_header_t **buffer,
    mach_port_t *livePort,
    mach_msg_timeout_t timeout,
    voucher_mach_msg_state_t *voucherState,
    void **voucherCopy
) {
    bool originalBuffer = true;
    mach_msg_size_t size = 3072;
    for (;;) {
        mach_msg_header_t *msg = *buffer;
        msg->msgh_remote_port = MACH_PORT_NULL;
        msg->msgh_local_port = port;
        msg->msgh_bits = 0;
        msg->msgh_size = size;
        msg->msgh_id = 0;
        mach_msg_option_t option;
        if (timeout == 0xFFFFFFFF) {
            CFRUNLOOP_SLEEP();
            option = 0x7000806;
        } else {
            CFRUNLOOP_POLL();
            option = 0x7000806 | MACH_RCV_TIMEOUT;
        }
        kern_return_t ret = mach_msg(msg, option, 0, msg->msgh_size, port, timeout, MACH_PORT_NULL);
        voucher_mach_msg_revert(*voucherState);
        *voucherState = voucher_mach_msg_adopt(msg);
        if (voucherCopy != NULL) {
            *voucherCopy = *voucherState != VOUCHER_MACH_MSG_STATE_UNCHANGED ? voucher_copy() : NULL;
        }
        CFRUNLOOP_WAKEUP((unsigned long)(long)ret);
        if (ret == MACH_RCV_TOO_LARGE) {
            mach_msg_size_t messageSize = msg->msgh_size;
            void *old;
            if (originalBuffer) {
                old = NULL;
                *buffer = NULL;
            } else {
                old = *buffer;
            }
            size_t newSize = ((size_t)(uint32_t)(messageSize + 68) + 3) & 0x1FFFFFFFC;
            size = (mach_msg_size_t)newSize;
            originalBuffer = false;
            *buffer = __OCFSafelyReallocateTyped(old, newSize, 0x9B1394D8, NULL);
            if (voucherCopy != NULL && *voucherCopy != NULL) {
                objc_release((id)*voucherCopy);
            }
            continue;
        }
        if (ret == MACH_RCV_TIMED_OUT) {
            if (!originalBuffer) {
                free(msg);
            }
            *buffer = NULL;
            *livePort = MACH_PORT_NULL;
            return false;
        }
        if (ret == MACH_MSG_SUCCESS) {
            *livePort = msg->msgh_local_port;
            return true;
        }
        if ((MACH_RCV_HEADER_ERROR & ret) == MACH_RCV_HEADER_ERROR ||
            (MACH_RCV_BODY_ERROR & ret) == MACH_RCV_BODY_ERROR) {
            uint32_t specialBits = (uint32_t)ret & MACH_MSG_MASK;
            if (specialBits == MACH_MSG_IPC_SPACE) {
                crashWithMessage("Out of IPC space");
            } else if (specialBits == MACH_MSG_VM_SPACE) {
                crashWithMessage("Out of VM address space");
            } else if (specialBits == MACH_MSG_IPC_KERNEL) {
                crashWithMessage("Kernel resource shortage handling IPC");
            } else if (specialBits == MACH_MSG_VM_KERNEL) {
                crashWithMessage("Kernel resource shortage handling out-of-line memory");
            }
            __builtin_trap();
        }
        crashWithMessage(mach_error_string(ret));
    }
}

// Native __CFRunLoopDoTimers, 0x1c01e4110. Non-external. The valid, not firing timers due by the limit are
// copied into an array (created on the first one) and fired in order; the result is whether any fired.
static bool __CFRunLoopDoTimers(OCFRunLoopRef rl, RunLoopMode *rlm, uint64_t limitTSR) {
    kdebug_trace(0x211200E5, (uintptr_t)rl, (uintptr_t)rlm, limitTSR, 0);
    OCFMutableArrayRef timers = NULL;
    bool timerHandled = false;
    if (rlm->timers != NULL) {
        OCFIndex cnt = OCFArrayGetCount(rlm->timers);
        for (OCFIndex idx = 0; idx < cnt; idx++) {
            OCFRunLoopTimerRef rlt = (OCFRunLoopTimerRef)OCFArrayGetValueAtIndex(rlm->timers, idx);
            if ((loadInfo(rlt) & kInfoValid) != 0 && (rlt->bits & 1) == 0 && rlt->fireTSR <= limitTSR) {
                if (timers == NULL) {
                    timers = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
                }
                OCFArrayAppendValue(timers, rlt);
            }
        }
    }
    if (timers != NULL) {
        OCFIndex cnt = OCFArrayGetCount(timers);
        for (OCFIndex idx = 0; idx < cnt; idx++) {
            OCFRunLoopTimerRef rlt = (OCFRunLoopTimerRef)OCFArrayGetValueAtIndex(timers, idx);
            timerHandled = (__CFRunLoopDoTimer(rl, rlm, rlt) != 0) || timerHandled;
        }
    }
    kdebug_trace(0x211200E6, (uintptr_t)rl, (uintptr_t)rlm, limitTSR, 0);
    if (timers != NULL) {
        OCFRelease(timers);
    }
    return timerHandled;
}

// Native __CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__, 0x1c01e42d8. Non-external; a frame for
// backtraces.
static void __CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__(void *msg) {
    _dispatch_main_queue_callback_4CF(msg);
    __asm__ volatile("");
}

// The source description of the source_fired probe, inlined natively into __CFRunLoopDoSource1 (no symbol):
// the mach port describer for __CFMachPortPerform, else the perform callback's dladdr names, else "<unknown
// library>" and "<unknown function>".
static void sourceDebugInfo(OCFRunLoopSourceRef rls, char *libraryName, char *functionName) {
    void *perform = (void *)rls->context.version1.perform;
    if (perform == (void *)__CFMachPortPerform) {
        __CFDescribeCFMachPortPerformCallout(
            rls->context.version1.info, libraryName, 2048, functionName, 2048
        );
        return;
    }
    Dl_info info;
    memset(&info, 0, sizeof(info));
    if (dladdr(perform, &info) != 0) {
        snprintf(functionName, 2048, "%s", info.dli_sname);
        snprintf(libraryName, 2048, "%s", info.dli_fname);
    } else {
        strcpy(functionName, "<unknown function>");
        strcpy(libraryName, "<unknown library>");
    }
}

// Native __CFRunLoopDoSource1, 0x1c01e42ec. Non-external. The source is retained and both locks dropped; a
// valid source loses its signal time (release store) and its perform callback is called out with the system
// allocator (per-callout pool when the run loop asks for one; the source_fired probe first when enabled),
// between kdebug events. The locks are retaken in the caller's order.
static bool __CFRunLoopDoSource1(
    OCFRunLoopRef rl,
    RunLoopMode *rlm,
    OCFRunLoopSourceRef rls,
    mach_msg_header_t *msg,
    OCFIndex size,
    mach_msg_header_t **reply
) {
    checkForFork();
    bool sourceHandled = false;
    OCFRetain(rls);
    pthread_mutex_unlock(&rlm->lock);
    pthread_mutex_unlock(&rl->lock);
    pthread_mutex_lock(&rls->lock);
    if ((loadInfo(rls) & kInfoValid) != 0) {
        __atomic_store_n(&rls->signaledTime, 0, __ATOMIC_SEQ_CST);
        pthread_mutex_unlock(&rls->lock);
        if (CFRUNLOOP_SOURCE_FIRED_ENABLED()) {
            char libraryName[2048];
            char functionName[2048];
            sourceDebugInfo(rls, libraryName, functionName);
            CFRUNLOOP_SOURCE_FIRED(rls, libraryName, functionName);
        }
        void *(*perform)(void *, OCFIndex, OCFAllocatorRef, void *) = rls->context.version1.perform;
        void *info = rls->context.version1.info;
        void *pool = rl->perCalloutARP ? _OCFAutoreleasePoolPush() : NULL;
        kdebug_trace(0x141300BD, (uintptr_t)rl, (uintptr_t)rlm, (uintptr_t)perform, (uintptr_t)info);
        __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__(perform, msg, size, reply, info);
        kdebug_trace(0x141300BE, (uintptr_t)rl, (uintptr_t)rlm, (uintptr_t)perform, (uintptr_t)info);
        __CFRunLoopPerCalloutARPEnd(pool);
        checkForFork();
        sourceHandled = true;
    } else {
        pthread_mutex_unlock(&rls->lock);
    }
    OCFRelease(rls);
    pthread_mutex_lock(&rl->lock);
    pthread_mutex_lock(&rlm->lock);
    return sourceHandled;
}

// Native __CFRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__, 0x1c01e4564. Non-external; a frame for backtraces.
static void __CFRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__(void (^block)(void)) {
    if (block != NULL) {
        block();
        __asm__ volatile("");
    }
}

// Native __CFRunLoopCollectSources0, 0x1c01e4580. Non-external; a set applier. A signalled (acquire load)
// valid version 0 source is retained as the single result, or both are moved into a new array.
static void __CFRunLoopCollectSources0(const void *value, void *context) {
    OCFRunLoopSourceRef rls = (OCFRunLoopSourceRef)value;
    OCFTypeRef *sources = context;
    if (rls->context.version0.version != 0 || (loadInfo(rls) & kInfoValid) == 0 ||
        __atomic_load_n(&rls->signaledTime, __ATOMIC_ACQUIRE) == 0) {
        return;
    }
    if (*sources == NULL) {
        *sources = OCFRetain(rls);
    } else if (OCFGetTypeID(*sources) == kRunLoopSourceTypeID) {
        OCFTypeRef oldrls = *sources;
        *sources = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
        OCFArrayAppendValue((OCFMutableArrayRef)*sources, oldrls);
        OCFArrayAppendValue((OCFMutableArrayRef)*sources, rls);
        OCFRelease(oldrls);
    } else {
        OCFArrayAppendValue((OCFMutableArrayRef)*sources, rls);
    }
}

// Native __CFRunLoopDoSource0, 0x1c01e464c. Non-external. A signalled source (acquire load) loses its signal
// time (release store); if still valid its perform callback is called out (per-callout pool when the run loop
// is NULL or asks for one) between kdebug events.
static bool __CFRunLoopDoSource0(OCFRunLoopRef rl, OCFRunLoopSourceRef rls) {
    pthread_mutex_lock(&rls->lock);
    if (__atomic_load_n(&rls->signaledTime, __ATOMIC_ACQUIRE) == 0) {
        pthread_mutex_unlock(&rls->lock);
        return false;
    }
    __atomic_store_n(&rls->signaledTime, 0, __ATOMIC_SEQ_CST);
    uint64_t info = loadInfo(rls);
    pthread_mutex_unlock(&rls->lock);
    if ((info & kInfoValid) == 0) {
        return false;
    }
    void (*perform)(void *) = rls->context.version0.perform;
    void *performInfo = rls->context.version0.info;
    void *pool = NULL;
    if (rl == NULL || rl->perCalloutARP) {
        pool = _OCFAutoreleasePoolPush();
    }
    kdebug_trace(0x141300B9, (uintptr_t)perform, (uintptr_t)performInfo, 0, 0);
    __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__(perform, performInfo);
    kdebug_trace(0x141300BA, (uintptr_t)perform, (uintptr_t)performInfo, 0, 0);
    __CFRunLoopPerCalloutARPEnd(pool);
    checkForFork();
    return true;
}

// Native __CFRunLoopSourceComparator, 0x1c01e4744. Non-external. By order, then by the signal times (acquire
// loads) converted to double.
static OCFComparisonResult __CFRunLoopSourceComparator(const void *val1, const void *val2, void *context) {
    OCFRunLoopSourceRef o1 = (OCFRunLoopSourceRef)val1;
    OCFRunLoopSourceRef o2 = (OCFRunLoopSourceRef)val2;
    (void)context;
    if (o1->order < o2->order) {
        return kOCFCompareLessThan;
    }
    if (o2->order < o1->order) {
        return kOCFCompareGreaterThan;
    }
    double t1 = (double)__atomic_load_n(&o1->signaledTime, __ATOMIC_ACQUIRE);
    double t2 = (double)__atomic_load_n(&o2->signaledTime, __ATOMIC_ACQUIRE);
    if (t1 < t2) {
        return kOCFCompareLessThan;
    }
    return t1 > t2 ? kOCFCompareGreaterThan : kOCFCompareEqualTo;
}

// Native __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__, 0x1c01e4794. Non-external; a frame for
// backtraces.
static void __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__(void (*perform)(void *), void *info) {
    if (perform != NULL) {
        perform(info);
        __asm__ volatile("");
    }
}

// The timer description of the timer_fired probe, inlined natively into __CFRunLoopDoTimer as
// __CFRunLoopDebugInfoForRunLoopTimer (no symbol; its statics are 0x22e6d2ee8-0x22e6d2f08). Its once block
// (_____CFRunLoopDebugInfoForRunLoopTimer_block_invoke, 0x1c01e4f94; the dispatch_once call is
// DoTimer.cold.1, 0x1c030bf18, and _OUTLINED_FUNCTION_1_17, 0x1c01e5044) looks up Foundation's timer and
// delayed-perform callouts and describers. Block timers are described by their block's invoke function.
static void timerDebugInfo(
    OCFRunLoopTimerRef rlt, void *info, char *libraryName, char *functionName, void **functionAddress
) {
    static dispatch_once_t onceToken;
    static void *foundationTimerFunc;
    static void (*foundationDescribeTimerCalloutFunc)(
        OCFRunLoopTimerRef, void *, char *, size_t, char *, size_t, void **
    );
    static void *foundationDelayedPerformFunc;
    static void (*foundationDescribeDelayedPerformFunc)(
        OCFRunLoopTimerRef, void *, char *, size_t, char *, size_t, void **
    );
    dispatch_once(&onceToken, ^{
      void *handle = dlopen("/System/Library/Frameworks/Foundation.framework/Foundation", RTLD_NOW);
      if (handle != NULL) {
          foundationTimerFunc = dlsym(handle, "__NSFireTimer");
          foundationDescribeTimerCalloutFunc = dlsym(handle, "__NSDescribeTimerCallout");
          foundationDelayedPerformFunc = dlsym(handle, "__NSFireDelayedPerform");
          foundationDescribeDelayedPerformFunc = dlsym(handle, "__NSDescribeDelayedPerformCallout");
          dlclose(handle);
      }
    });
    void *callout = (void *)rlt->callout;
    if (callout == (void *)_runLoopTimerWithBlockContext) {
        callout = *(void **)((uint8_t *)info + 16);
    }
    if (callout == (void *)__CFFireTimer) {
        __CFDescribeTimer(rlt, info, libraryName, 2048, functionName, 2048, functionAddress);
        return;
    }
    if (callout == foundationTimerFunc && foundationTimerFunc != NULL &&
        foundationDescribeTimerCalloutFunc != NULL) {
        foundationDescribeTimerCalloutFunc(rlt, info, libraryName, 2048, functionName, 2048, functionAddress);
        return;
    }
    if (callout == foundationDelayedPerformFunc && foundationDelayedPerformFunc != NULL &&
        foundationDescribeDelayedPerformFunc != NULL) {
        foundationDescribeDelayedPerformFunc(
            rlt, info, libraryName, 2048, functionName, 2048, functionAddress
        );
        return;
    }
    Dl_info dlInfo;
    memset(&dlInfo, 0, sizeof(dlInfo));
    if (dladdr(callout, &dlInfo) != 0) {
        snprintf(libraryName, 2048, "%s", dlInfo.dli_fname != NULL ? dlInfo.dli_fname : "<unknown library>");
        snprintf(
            functionName, 2048, "%s", dlInfo.dli_sname != NULL ? dlInfo.dli_sname : "<unknown function>"
        );
    } else {
        strcpy(libraryName, "<unknown library>");
        strcpy(functionName, "<unknown function>");
    }
    *functionAddress = callout;
}

// Native __CFRunLoopDoTimer, 0x1c01e47b4, with __CFRunLoopDoTimer.cold.1 (0x1c030bf18) and .cold.2
// (0x1c030bf1c). Non-external. A valid, due, not firing timer of this run loop is marked firing with its
// information retained, the mode's deadlines are reset, the old fire time read under the timer lock
// (data-synchronization option 0x10000) and the next timer armed; with both locks dropped the callout runs
// inside a pool (the timer_fired probe first when enabled, describing the callout in two 2,048-byte calloc
// buffers), a zero interval invalidates, then the information is released. With the locks retaken a timer
// still valid and not rescheduled meanwhile gets its next fire time: past the old one by whole intervals
// beyond now (the interval is clamped to 504911232 s; overflow: INT64_MAX; a zero interval TSR or a
// non-positive interval with an old time below INT64_MAX stores "A CFRunLoopTimer with an interval of 0 is
// set to repeat" and traps), set with its absolute date under the timer lock and repositioned in every mode
// of its run loop (each locked meanwhile). The result is whether the timer was handled. The skipped path
// performs a discarded atomic load of the info word, as natively.
static Boolean __CFRunLoopDoTimer(OCFRunLoopRef rl, RunLoopMode *rlm, OCFRunLoopTimerRef rlt) {
    Boolean timerHandled = false;
    OCFRetain(rlt);
    pthread_mutex_lock(&rlt->lock);
    if ((loadInfo(rlt) & kInfoValid) == 0 || rlt->fireTSR > mach_absolute_time() || (rlt->bits & 1) != 0 ||
        rlt->runLoop != rl) {
        (void)loadInfo(rlt);
        pthread_mutex_unlock(&rlt->lock);
        OCFRelease(rlt);
        return timerHandled;
    }
    void *contextInfo = rlt->context.info;
    void (*contextRelease)(const void *) = NULL;
    if (rlt->context.retain != NULL) {
        contextInfo = (void *)rlt->context.retain(rlt->context.info);
        contextRelease = rlt->context.release;
    }
    OCFTimeInterval oldInterval = rlt->interval;
    rlt->bits |= 1;
    rlm->timerSoftDeadline = UINT64_MAX;
    rlm->timerHardDeadline = UINT64_MAX;
    pthread_mutex_unlock(&rlt->lock);
    os_unfair_lock_lock_with_options(&rl->timerTSRLock, kUnfairLockDataSynchronization);
    uint64_t oldFireTSR = rlt->fireTSR;
    os_unfair_lock_unlock(&rl->timerTSRLock);
    __CFArmNextTimerInMode(rlm, rl);
    pthread_mutex_unlock(&rlm->lock);
    pthread_mutex_unlock(&rl->lock);
    if (CFRUNLOOP_TIMER_FIRED_ENABLED()) {
        char *libraryName = malloc_type_calloc(2048, 1, 0x100004077774924);
        char *functionName = malloc_type_calloc(2048, 1, 0x100004077774924);
        strcpy(libraryName, "<unknown>");
        strcpy(functionName, "<unknown>");
        void *functionAddress = NULL;
        timerDebugInfo(rlt, contextInfo, libraryName, functionName, &functionAddress);
        CFRUNLOOP_TIMER_FIRED(
            rlt,
            (unsigned long)(OCFAbsoluteTimeGetCurrent() * 1000.0),
            libraryName,
            functionName,
            functionAddress
        );
        free(libraryName);
        free(functionName);
    }
    OCFRunLoopTimerCallBack callout = rlt->callout;
    void *pool = _OCFAutoreleasePoolPush();
    kdebug_trace(0x141300C9, (uintptr_t)callout, (uintptr_t)rlt, (uintptr_t)contextInfo, 0);
    __CFRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__(callout, rlt, contextInfo);
    kdebug_trace(0x141300CA, (uintptr_t)callout, (uintptr_t)rlt, (uintptr_t)contextInfo, 0);
    __CFRunLoopPerCalloutARPEnd(pool);
    checkForFork();
    if (oldInterval == 0.0) {
        OCFRunLoopTimerInvalidate(rlt);
    }
    if (contextRelease != NULL) {
        contextRelease(contextInfo);
    }
    pthread_mutex_lock(&rl->lock);
    pthread_mutex_lock(&rlm->lock);
    pthread_mutex_lock(&rlt->lock);
    rlt->bits &= 0xFFFE;
    if ((loadInfo(rlt) & kInfoValid) == 0) {
        pthread_mutex_unlock(&rlt->lock);
        OCFRelease(rlt);
        return true;
    }
    if (oldFireTSR < rlt->fireTSR) {
        pthread_mutex_unlock(&rlt->lock);
        __CFArmNextTimerInMode(rlm, rl);
        OCFRelease(rlt);
        return true;
    }
    uint64_t nextFireTSR;
    OCFTimeInterval interval = rlt->interval;
    if (interval <= 0.0) {
        nextFireTSR = INT64_MAX;
        if (oldFireTSR < INT64_MAX) {
            crashWithMessage("A CFRunLoopTimer with an interval of 0 is set to repeat");
        }
    } else {
        if (interval > 504911232.0) {
            interval = 504911232.0;
        }
        uint64_t intervalTSR = __CFTimeIntervalToTSR(interval);
        if ((intervalTSR ^ INT64_MAX) <= oldFireTSR) {
            nextFireTSR = INT64_MAX;
        } else {
            if (intervalTSR == 0) {
                crashWithMessage("A CFRunLoopTimer with an interval of 0 is set to repeat");
            }
            uint64_t currentTSR = mach_absolute_time();
            nextFireTSR = oldFireTSR - intervalTSR;
            do {
                nextFireTSR += intervalTSR;
            } while (nextFireTSR <= currentTSR);
        }
    }
    OCFRunLoopRef rlTimer = rlt->runLoop;
    if (rlTimer == NULL) {
        pthread_mutex_unlock(&rlt->lock);
        os_unfair_lock_lock(&rl->timerTSRLock);
        rlt->fireTSR = nextFireTSR;
        OCFAbsoluteTime now = OCFAbsoluteTimeGetCurrent();
        rlt->nextFireDate = now + __CFTimeIntervalUntilTSR(nextFireTSR);
        os_unfair_lock_unlock(&rl->timerTSRLock);
        OCFRelease(rlt);
        return true;
    }
    OCFRetain(rlTimer);
    OCFIndex cnt = OCFSetGetCount(rlt->rlModes);
    const void *modes[cnt > 0 ? cnt : 1];
    bzero(modes, (size_t)(8 * cnt) < 512 ? (size_t)(8 * cnt) : 512);
    OCFSetGetValues(rlt->rlModes, modes);
    if (cnt < 1) {
        pthread_mutex_unlock(&rlt->lock);
    } else {
        for (OCFIndex idx = 0; idx < cnt; idx++) {
            OCFRetain(modes[idx]);
        }
        pthread_mutex_unlock(&rlt->lock);
        for (OCFIndex idx = 0; idx < cnt; idx++) {
            OCFStringRef name = modes[idx];
            RunLoopMode *mode = __CFRunLoopCopyMode(rlTimer, name, false);
            modes[idx] = mode;
            if (mode != NULL) {
                pthread_mutex_lock(&mode->lock);
            }
            OCFRelease(name);
        }
    }
    os_unfair_lock_lock(&rl->timerTSRLock);
    rlt->fireTSR = nextFireTSR;
    OCFAbsoluteTime now = OCFAbsoluteTimeGetCurrent();
    rlt->nextFireDate = now + __CFTimeIntervalUntilTSR(nextFireTSR);
    if (cnt < 1) {
        os_unfair_lock_unlock(&rl->timerTSRLock);
    } else {
        for (OCFIndex idx = 0; idx < cnt; idx++) {
            RunLoopMode *mode = (RunLoopMode *)modes[idx];
            if (mode != NULL) {
                __CFRepositionTimerInMode(mode, rlt, true);
            }
        }
        os_unfair_lock_unlock(&rl->timerTSRLock);
        for (OCFIndex idx = cnt; idx > 0; idx--) {
            RunLoopMode *mode = (RunLoopMode *)modes[idx - 1];
            if (mode != NULL) {
                pthread_mutex_unlock(&mode->lock);
                OCFRelease(mode);
            }
        }
    }
    OCFRelease(rlTimer);
    timerHandled = true;
    OCFRelease(rlt);
    return timerHandled;
}

// Native __CFRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__, 0x1c01e4e1c. Non-external; a frame for
// backtraces.
static void __CFRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__(
    OCFRunLoopTimerCallBack func, OCFRunLoopTimerRef timer, void *info
) {
    if (func != NULL) {
        func(timer, info);
        __asm__ volatile("");
    }
}

// Native __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__, 0x1c01e4e40. Non-external; stores the
// perform callback's reply.
static void __CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__(
    void *(*perform)(void *, OCFIndex, OCFAllocatorRef, void *),
    mach_msg_header_t *msg,
    OCFIndex size,
    mach_msg_header_t **reply,
    void *info
) {
    if (perform != NULL) {
        *reply = perform(msg, size, kOCFAllocatorSystemDefault, info);
    }
}

// Native __CFRunLoopRemoveSourcesFromCommonMode, 0x1c01e4e88. Non-external; a set applier over common mode
// names.
static void __CFRunLoopRemoveSourcesFromCommonMode(const void *value, void *ctx) {
    __CFRunLoopRemoveAllSources((OCFRunLoopRef)ctx, (OCFStringRef)value);
}

// Native __CFRunLoopRemoveSourceFromMode, 0x1c01e4e98. Non-external; a set applier with {run loop, mode
// name}.
static void __CFRunLoopRemoveSourceFromMode(const void *value, void *ctx) {
    const void **params = ctx;
    OCFRunLoopRemoveSource((OCFRunLoopRef)params[0], (OCFRunLoopSourceRef)value, (OCFStringRef)params[1]);
}

// Native __CFRunLoopInsertionIndexInTimerArray, 0x1c01e4ea8. Non-external natively; hidden here (reached from
// __CFRepositionTimerInMode). Past 256 timers the ends are checked first; then a binary search over a power
// of two (2 << flsl(count)) returns the index after the last timer firing no later than the given one.
OCFIndex __CFRunLoopInsertionIndexInTimerArray(OCFArrayRef array, OCFRunLoopTimerRef rlt) {
    OCFIndex cnt = OCFArrayGetCount(array);
    if (cnt <= 0) {
        return 0;
    }
    if (cnt > 256) {
        OCFRunLoopTimerRef item = (OCFRunLoopTimerRef)OCFArrayGetValueAtIndex(array, cnt - 1);
        if (item->fireTSR <= rlt->fireTSR) {
            return cnt;
        }
        item = (OCFRunLoopTimerRef)OCFArrayGetValueAtIndex(array, 0);
        if (rlt->fireTSR < item->fireTSR) {
            return 0;
        }
    }
    OCFIndex add = (OCFIndex)2 << flsl(cnt);
    OCFIndex idx = 0;
    OCFIndex lg;
    for (;;) {
        OCFIndex half = add / 2;
        lg = 0;
        if (idx + half < cnt) {
            OCFRunLoopTimerRef item = (OCFRunLoopTimerRef)OCFArrayGetValueAtIndex(array, idx + half);
            if (item->fireTSR <= rlt->fireTSR) {
                idx = idx + half;
                lg = 1;
            }
        }
        bool more = add > 1;
        add = half;
        if (!more) {
            break;
        }
    }
    return lg + idx;
}

// Native __THE_SYSTEM_HAS_NO_PORTS_AVAILABLE__, 0x1c030b8b0 (BRK #1). Non-external, also called by the mach
// port units natively; hidden. A name outside the CF/NS prefix rule, kept as native: hidden until the owner
// records a naming decision. Its name is the frame a crash report shows.
__attribute__((noinline, noreturn)) void __THE_SYSTEM_HAS_NO_PORTS_AVAILABLE__(void) {
    __builtin_trap();
}

// Native __NO_SPACE__ and __RESOURCE_SHORTAGE__, 0x1c030b8d8 and 0x1c030b8dc (BRK #1 each). Non-external;
// frames for crash reports.
__attribute__((noinline, noreturn)) static void __NO_SPACE__(void) {
    __builtin_trap();
}

__attribute__((noinline, noreturn)) static void __RESOURCE_SHORTAGE__(void) {
    __builtin_trap();
}

// Native __THE_SYSTEM_HAS_NO_PORT_SETS_AVAILABLE__, 0x1c030b8b4. Non-external; hidden, named as natively
// (hidden until the owner records a naming decision). KERN_RESOURCE_SHORTAGE (6) calls __RESOURCE_SHORTAGE__,
// KERN_NO_SPACE (3) __NO_SPACE__; any other result traps here.
__attribute__((noinline, noreturn)) void __THE_SYSTEM_HAS_NO_PORT_SETS_AVAILABLE__(kern_return_t ret) {
    if (ret == 6) {
        __RESOURCE_SHORTAGE__();
    }
    if (ret == 3) {
        __NO_SPACE__();
    }
    __builtin_trap();
}
