//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Notification/OCFNotificationCenter.h"
#include <OpenCoreFoundation/Date/OCFDate.h>

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Notification/OCFXNotificationRegistrar.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/PropertyList/OCFPropertyList.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <mach/mach_time.h>
#include <malloc/malloc.h>
#include <notify.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <os/log.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <xpc/xpc.h>

// Inferred organization: the notification center functions (___CFNotificationCenterDeallocate, 0x1c01a87e8,
// through ___CFNOTIFICATIONCENTER_IS_CALLING_OUT_TO_AN_OBSERVER__, 0x1c01ab9a0, with their .cold pieces at
// 0x1c0308a50-0x1c0308bbc) are one contiguous cluster of the iOS Simulator __text section and are kept in one
// translation unit; the native source path is not in the evidence. Only the functions of the misc-services
// task list are restored; the rest of the cluster (CFNotificationCenterGetTypeID, the distributed and Darwin
// centers, observer registration and removal, suspension, the debug info, the dependent notifications and the
// __CFNotification methods) is not. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; the pseudocode was a
// guide only). iOS Device 24A446: unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json.
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_1_11 (0x1c01aba88).

// libobjc, libsystem_trace and libsystem_featureflags entry points the native code calls; supplied, never
// reimplemented. The SDK installs no declarations of these.
extern id objc_alloc(Class cls);
extern id objc_opt_self(id object);
extern id objc_opt_new(Class cls);
extern id objc_retain(id value);
extern void objc_release(id value);
extern bool _os_feature_enabled_impl(const char *domain, const char *feature);
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);
extern struct mach_header __dso_handle;
extern int sandbox_check(pid_t pid, const char *operation, int type, ...);
extern dispatch_queue_attr_t
dispatch_queue_attr_make_with_overcommit(dispatch_queue_attr_t attr, bool overcommit);

// libxpc's xpc_connection_create_mach_service, which the iOS SDK marks unavailable although the native image
// imports it; declared under a local name bound to the libxpc symbol.
extern xpc_connection_t createMachServiceConnection(
    const char *name, dispatch_queue_t targetQueue, uint64_t flags
) __asm(OF_SYMBOL("xpc_connection_create_mach_service"));

// The os_log format literals, placed in __TEXT,__oslogstring like the compiler-generated native ones.
#define OSLOG_STRING __attribute__((section("__TEXT,__oslogstring,cstring_literals")))
OSLOG_STRING static const char exceededRestoreFormat[] =
    "DistributedNotificationCenter: Exceeded maximum recent restore attempts, will not attempt restore for "
    "now.";
OSLOG_STRING static const char restoreTimerFormat[] =
    "DistributedNotificationCenter: Unable to create restore timer";
OSLOG_STRING static const char schedulingRestoreFormat[] =
    "DistributedNotificationCenter: Scheduling restore in %llu ms";
OSLOG_STRING static const char sendingRestoreFormat[] =
    "DistributedNotificationCenter: Sending restore attempt (%d registrations) ...";
OSLOG_STRING static const char postRateFormat[] =
    "Detected potentially harmful notification post rate of %g notifications per second";

// The notification class of this image (natively __CFNotification; not restored on this branch).
extern char notificationClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__OCFNotification));

// The center instance (48 bytes): the registrar, the XPC connection and its queue (distributed centers) and
// the restore state. The info word's bit 0 marks a Darwin center and bit 1 a distributed one.
typedef struct {
    dispatch_source_t timer;
    uint64_t attempts[6];
} RestoreState;

_Static_assert(sizeof(RestoreState) == 0x38, "The native restore state is 56 bytes.");

struct __OCFNotificationCenter {
    OCFRuntimeBase base;
    OCFXNotificationRegistrarRef registrar;
    xpc_connection_t connection;
    dispatch_queue_t queue;
    RestoreState *restoreState;
};

OF_ABI_ASSERT(sizeof(struct __OCFNotificationCenter) == 0x30, "The native center is 48 bytes.");

enum {
    kCenterDarwin = 1,
    kCenterDistributed = 2,
};

// Post options: 0x100000 delivers locally only. Observer flags as in the registrar unit: 0x400 weak, 0x10000
// asynchronous on a global queue, 0x20000 block, 0x40000 selector, 0x80000 C callback.
enum {
    kPostLocalOnly = 0x100000,
    kObserverWeak = 0x400,
    kObserverAsynchronous = 0x10000,
    kHandlerBlock = 0x20000,
    kHandlerSelector = 0x40000,
    kHandlerCallback = 0x80000,
};

// kCFStringEncodingUTF8 and the restore-attempt window (30 s in nanoseconds, 0x6FC23AC00).
#define kUTF8Encoding ((OCFStringEncoding)0x08000100)
#define kRestoreWindow ((uint64_t)30000000000)

static OCFTypeRef __CFXRegistrationPost(
    OCFNotificationCenterRef center,
    NotificationRegistration *registration,
    Boolean onMainQueue,
    void *notification,
    void *observer
);
static void __CFXNotificationPostToken(
    OCFNotificationCenterRef center,
    uint64_t token,
    OCFStringRef name,
    OCFStringRef object,
    OCFTypeRef userInfo
);
static void __CFNOTIFICATIONCENTER_IS_CALLING_OUT_TO_AN_OBSERVER__(
    OCFNotificationCenterRef center,
    void *callout,
    OCFTypeRef context,
    void *observer,
    void *notification,
    uint64_t flags
);

// Native kCFNotificationAnyName, kCFNotificationAnyObject and kCFNotificationAnyObserver, 0x2227a21e0,
// 0x2227a21e8 and 0x2227a21f0 (__DATA_CONST,__const). External. Constant strings of their own names
// (0x2227aee80, 0x2227aeea0, 0x2227aeec0); the native posting code uses the coalesced
// "kCFNotificationAnyObject" literal directly, so each string is one object here. The values spell the mapped
// names (the native names in the system-name build).
static const OCFConstantString anyNameString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(kOCFNotificationAnyName),
    sizeof(OF_NAME_STRING(kOCFNotificationAnyName)) - 1
};
static const OCFConstantString anyObjectString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(kOCFNotificationAnyObject),
    sizeof(OF_NAME_STRING(kOCFNotificationAnyObject)) - 1
};
static const OCFConstantString anyObserverString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(kOCFNotificationAnyObserver),
    sizeof(OF_NAME_STRING(kOCFNotificationAnyObserver)) - 1
};

#define kAnyObjectString ((OCFStringRef) & anyObjectString)

const OCFStringRef kOCFNotificationAnyName = (OCFStringRef)&anyNameString;
const OCFStringRef kOCFNotificationAnyObject = (OCFStringRef)&anyObjectString;
const OCFStringRef kOCFNotificationAnyObserver = (OCFStringRef)&anyObserverString;

// Native _kCFXNotificationConfigurationStandardLocalConfiguration, 0x1c0375170 (__TEXT,__const). External.
// Kind 0, standard 1, every other byte zero.
const NotificationConfiguration _kOCFXNotificationConfigurationStandardLocalConfiguration = {
    .kind = 0,
    .standard = 1,
};

// Native _kCFXNotificationConfigurationStandardDarwinConfiguration, 0x1c03751d0 (__TEXT,__const).
// External. Kind 2, standard 1, every other byte zero.
const NotificationConfiguration _kOCFXNotificationConfigurationStandardDarwinConfiguration = {
    .kind = 2,
    .standard = 1,
};

// Native _kCFXNotificationConfigurationStandardDistributedConfiguration, 0x2227a2258 (__DATA_CONST,__const).
// External. Kind 1, standard 1, the service names "com.apple.distributed_notifications@1v3" (0x1c04831c7) and
// "com.apple.distributed_notifications@Uv3" (0x1c04831ef), every other byte zero.
const NotificationConfiguration _kOCFXNotificationConfigurationStandardDistributedConfiguration = {
    .kind = 1,
    .standard = 1,
    .serviceName = "com.apple.distributed_notifications@1v3",
    .alternateServiceName = "com.apple.distributed_notifications@Uv3",
};

// Native kCFXNotificationPlaceholderObserver, 0x22e6cf580 (__DATA,__data). Non-external. Its value is its own
// address; the observer callout passes NULL for it.
static void *kCFXNotificationPlaceholderObserver = &kCFXNotificationPlaceholderObserver;

// Native __CFIsDistNoteDaemon, 0x22e6d28b8 (__DATA,__bss). Non-external. Set when the process is distnoted.
static uint8_t __CFIsDistNoteDaemon;

// Native __taskCenter, 0x22e6d28c8 (__DATA,__bss). Non-external. The local center.
static OCFNotificationCenterRef __taskCenter;

// Native CFNotificationCenterGetLocalCenter.static_init, 0x22e6d28c0 (__DATA,__bss). Non-external. The once
// predicate CFNotificationCenterGetLocalCenter, _CFXNotificationGetTaskCenter and the dependent-notification
// initializer share; all three run the same global block (___block_literal_global_7) through
// _OUTLINED_FUNCTION_1_11 (0x1c0308a50 and 0x1c0308a7c fold it).
static dispatch_once_t localCenterStaticInit;

// The task center, created once. Native ___CFNotificationCenterGetLocalCenter_block_invoke, 0x1c01a8b08.
// Non-external. One block literal for every caller, as natively.
static inline __attribute__((always_inline)) OCFNotificationCenterRef taskCenter(void) {
    dispatch_once(&localCenterStaticInit, ^{
      __taskCenter = _OCFXNotificationCenterCreate(
          kOCFAllocatorSystemDefault, &_kOCFXNotificationConfigurationStandardLocalConfiguration
      );
    });
    return __taskCenter;
}

// Native __hostCenter, 0x22e6d28d8 (__DATA,__bss). Non-external. The distributed center.
static OCFNotificationCenterRef __hostCenter;

// Native __dependentNotificationLock (0x22e6d28f0) and __dependentNotifications (0x22e6d28f8), __DATA,__bss.
// Non-external. The set of registered dependent notifications, keyed by their name.
static os_unfair_lock _dependentNotificationLock = OS_UNFAIR_LOCK_INIT;
static OCFMutableSetRef _dependentNotifications;

// Native __cfDependentNotifications, 0x2227a2380 (__DATA_CONST,__const). Non-external. CoreFoundation's own
// list, NULL-terminated.
static const DependentNotification *const __cfDependentNotifications[] = {
    &__CFCalendarDayChangedDependentNotification,
    &__CFLocaleDidChangeDependentNotification,
    &__CFSystemClockDidChangeDependentNotification,
    &__CFTimeZoneDidChangeDependentNotification,
    NULL,
};

// Native __unprocessedDependentNotificationLists, 0x22e6cf538 (__DATA,__data). Non-external. Eight slots, the
// first holding CoreFoundation's own list.
static const DependentNotification *const *_unprocessedDependentNotificationLists[8] = {
    __cfDependentNotifications
};

static Boolean _dependentNotificationsEqual(const void *first, const void *second);
static OCFHashCode _dependentNotificationHash(const void *value);

// Native ___CFNotificationCenterDeallocate, 0x1c01a87e8. Non-external; the class's finalize callback.
// Releases the registrar (unconditionally) and clears it, cancels and releases the connection, releases the
// queue, and cancels and releases the restore timer before freeing the restore state (a tail call).
static void __CFNotificationCenterDeallocate(OCFTypeRef cf) {
    OCFNotificationCenterRef center = (OCFNotificationCenterRef)cf;
    OCFRelease(center->registrar);
    center->registrar = NULL;
    if (center->connection != NULL) {
        xpc_connection_cancel(center->connection);
        xpc_release(center->connection);
    }
    if (center->queue != NULL) {
        dispatch_release(center->queue);
    }
    if (center->restoreState != NULL) {
        if (center->restoreState->timer != NULL) {
            dispatch_source_cancel(center->restoreState->timer);
            dispatch_release(center->restoreState->timer);
        }
        free(center->restoreState);
    }
}

// Native __CFNotificationCenterClass, 0x2227a21f8 (__DATA_CONST,__const). Hidden (was a private external).
// Version 0, name "CFNotificationCenter", the finalize callback only.
const OCFRuntimeClass __CFNotificationCenterClass = {
    0,
    "CFNotificationCenter",
    NULL,
    NULL,
    __CFNotificationCenterDeallocate,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
};

// The UTF-8 bytes of a string as a malloc'd C string, inlined at every native site: the needed length
// (CFStringGetBytes without a buffer), a malloc of length + 1 (type 0x100004077774924), the bytes and a
// terminating zero. The length is asked twice, as natively.
static inline __attribute__((always_inline)) char *copyUTF8String(OCFStringRef string) {
    OCFIndex used = 0;
    OCFStringGetBytes(
        string, (OCFRange){0, OCFStringGetLength(string)}, kUTF8Encoding, 0, false, NULL, 0, &used
    );
    char *bytes = malloc_type_malloc((size_t)used + 1, 0x100004077774924);
    OCFStringGetBytes(
        string, (OCFRange){0, OCFStringGetLength(string)}, kUTF8Encoding, 0, false, (UInt8 *)bytes, used, NULL
    );
    bytes[used] = 0;
    return bytes;
}

// Native mach_time_to_nanoseconds, inlined at every native site, with its function-static
// mach_time_to_nanoseconds.mach_timebase_factor (__DATA,__bss; non-external): numer / denom of a zeroed
// mach_timebase_info record, computed on first use as 32 bits.
static uint32_t mach_timebase_factor;

static inline __attribute__((always_inline)) uint64_t mach_time_to_nanoseconds(uint64_t elapsed) {
    if (mach_timebase_factor == 0) {
        mach_timebase_info_data_t info = {0, 0};
        mach_timebase_info(&info);
        mach_timebase_factor = info.numer / info.denom;
    }
    return elapsed * mach_timebase_factor;
}

// Native send_xpc_message, inlined at every native site, with its global block ___block_literal_global.105
// (invoke ___send_xpc_message_block_invoke, 0x1c01ab8f4, non-external: a tail call of
// _CFSuddenTerminationEnable). Sudden termination is disabled until the barrier runs.
static void (^const sendBarrierBlock)(void) = ^{
  _OCFSuddenTerminationEnable();
};

static inline __attribute__((always_inline)) void
send_xpc_message(xpc_connection_t connection, xpc_object_t message) {
    _OCFSuddenTerminationDisable();
    xpc_connection_send_message(connection, message);
    xpc_connection_send_barrier(connection, sendBarrierBlock);
}

// The restore attempts of the last 30 s, inlined in the setup connection block: each of the six timestamps
// older than the window is cleared; the result is the number kept.
static inline __attribute__((always_inline)) uint32_t pruneRestoreAttempts(RestoreState *state) {
    uint64_t now = mach_absolute_time();
    uint32_t recent = 0;
    for (int slot = 0; slot < 6; slot++) {
        uint64_t attempt = state->attempts[slot];
        if (mach_time_to_nanoseconds(now - attempt) < kRestoreWindow) {
            recent++;
        } else {
            state->attempts[slot] = 0;
        }
    }
    return recent;
}

// Native __CFXNotificationCenterScheduleRestoreState, inlined in the setup connection block (its blocks keep
// its name).
// Behind the CoreFoundation/CFNotificationCenterRestoreState feature, for a center: an existing state is
// pruned; an empty one without a timer is freed and replaced, one with a timer is left alone. A new state is
// a zeroed 56-byte malloc (type 0x108004042E86B05). With all six attempt slots used, an error is logged
// (_CFXNotificationCenterSetupConnection_block_invoke.cold.1). Otherwise a timer source on the center's queue
// (the GOT slot 0x2224171E8 outside the image, used with dispatch_source_set_timer:
// DISPATCH_SOURCE_TYPE_TIMER; confidence: medium) is stored (an error, .cold.3, when it cannot be created),
// the attempts shift down and the newest is now; the delay is min(250 << (recent - 1), 10000) ms with recent
// counting the new attempt, plus a rand() jitter in [-delay / 4, delay / 4] (UCVTF #2, FCVTZU); a debug
// message (.cold.2) reports it. The timer fires once after the delay (leeway a tenth of it), with the restore
// and cancel blocks below.
static inline __attribute__((always_inline)) void
__CFXNotificationCenterScheduleRestoreState(OCFNotificationCenterRef center) {
    if (!_os_feature_enabled_impl("CoreFoundation", "CFNotificationCenterRestoreState")) {
        return;
    }
    if (center == NULL) {
        return;
    }
    RestoreState *state = center->restoreState;
    if (state != NULL) {
        uint32_t recent = pruneRestoreAttempts(state);
        if (state->timer == NULL && recent == 0) {
            free(state);
            center->restoreState = NULL;
            state = NULL;
        } else if (state->timer != NULL) {
            return;
        }
    }
    if (state == NULL) {
        state = malloc_type_malloc(0x38, 0x108004042E86B05);
        center->restoreState = state;
        memset(state, 0, 0x38);
        state = center->restoreState;
    }
    if (state->attempts[5] != 0) {
        os_log_t log = _CFOSLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
            uint8_t buffer[2] = {0, 0};
            _os_log_error_impl(&__dso_handle, log, OS_LOG_TYPE_ERROR, exceededRestoreFormat, buffer, 2);
        }
        return;
    }
    dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, center->queue);
    state = center->restoreState;
    state->timer = timer;
    if (timer == NULL) {
        os_log_t log = _CFOSLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
            uint8_t buffer[2] = {0, 0};
            _os_log_error_impl(&__dso_handle, log, OS_LOG_TYPE_ERROR, restoreTimerFormat, buffer, 2);
        }
        return;
    }
    uint64_t now = mach_absolute_time();
    memmove(&state->attempts[1], &state->attempts[0], 5 * sizeof(uint64_t));
    state->attempts[0] = now;
    uint32_t recent = 1;
    for (int slot = 1; slot < 6; slot++) {
        if (mach_time_to_nanoseconds(now - state->attempts[slot]) < kRestoreWindow) {
            recent++;
        }
    }
    uint32_t shifted = (uint32_t)250 << ((recent - 1) & 31);
    uint32_t base = shifted < 10000 ? shifted : 10000;
    uint64_t jitter = (uint64_t)((double)base / 4.0);
    uint64_t randomValue = (uint64_t)(int64_t)rand();
    uint64_t delay = randomValue % (1 | (jitter << 1)) - jitter + base;
    os_log_t log = _CFOSLog();
    if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
        uint8_t buffer[12] = {0x00, 0x01, 0x00, 0x08};
        memcpy(&buffer[4], &delay, sizeof(delay));
        _os_log_debug_impl(&__dso_handle, log, OS_LOG_TYPE_DEBUG, schedulingRestoreFormat, buffer, 12);
    }
    uint64_t nanoseconds = delay * 1000000;
    dispatch_source_set_timer(
        center->restoreState->timer,
        dispatch_time(DISPATCH_TIME_NOW, (int64_t)nanoseconds),
        DISPATCH_TIME_FOREVER,
        nanoseconds / 10
    );
    // Native _____CFXNotificationCenterScheduleRestoreState_block_invoke, 0x1c01ab3f4 (non-external), with
    // .cold.1 (0x1c0308bbc): {method: "restore", version: 1, registrations: [...]} built by enumerating the
    // registrar into a __block count; a debug message reports the count; the message is sent with
    // send_xpc_message, the message parts are released (the array first) and the timer is cancelled.
    dispatch_source_set_event_handler(center->restoreState->timer, ^{
      xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
      xpc_dictionary_set_string(message, "method", "restore");
      xpc_dictionary_set_uint64(message, "version", 1);
      xpc_object_t registrations = xpc_array_create(NULL, 0);
      __block int32_t count = 0;
      // Native _____CFXNotificationCenterScheduleRestoreState_block_invoke_2, 0x1c01ab5a4 (non-external): one
      // {name, object, options (the flags without 0x400 and 0x2000), token} dictionary per registration, the
      // name and object as UTF-8 C strings; the count is incremented last.
      OCFXNotificationRegistrarEnumerate(center->registrar, ^(const NotificationRegistration *registration) {
        char *name = copyUTF8String(registration->name);
        char *object = copyUTF8String((OCFStringRef)registration->object);
        xpc_object_t entry = xpc_dictionary_create(NULL, NULL, 0);
        xpc_dictionary_set_string(entry, "name", name);
        xpc_dictionary_set_string(entry, "object", object);
        xpc_dictionary_set_uint64(entry, "options", registration->flags & 0xFFFFFFFFFFFFDBFF);
        xpc_dictionary_set_uint64(entry, "token", registration->token);
        xpc_array_append_value(registrations, entry);
        xpc_release(entry);
        free(object);
        free(name);
        count++;
      });
      xpc_dictionary_set_value(message, "registrations", registrations);
      os_log_t restoreLog = _CFOSLog();
      if (os_log_type_enabled(restoreLog, OS_LOG_TYPE_DEBUG)) {
          uint8_t buffer[8] = {0x00, 0x01, 0x00, 0x04};
          int32_t registered = count;
          memcpy(&buffer[4], &registered, sizeof(registered));
          _os_log_debug_impl(&__dso_handle, restoreLog, OS_LOG_TYPE_DEBUG, sendingRestoreFormat, buffer, 8);
      }
      send_xpc_message(center->connection, message);
      xpc_release(registrations);
      xpc_release(message);
      dispatch_source_cancel(center->restoreState->timer);
    });
    // Native _____CFXNotificationCenterScheduleRestoreState_block_invoke.92, 0x1c01ab7dc (non-external): the
    // timer is released and cleared.
    dispatch_source_set_cancel_handler(center->restoreState->timer, ^{
      dispatch_release(center->restoreState->timer);
      center->restoreState->timer = NULL;
    });
    dispatch_resume(center->restoreState->timer);
}

// Native __CFXNotificationCenterSetupConnection, inlined in _CFXNotificationCenterCreate (its block keeps its
// name). A queue "com.apple.notification_center.server_conn" at the main thread's QoS; the connection from
// the configuration's endpoint (targeting the queue) or to its Mach service (privileged when byte 9 is set);
// the event handler below; activation. The event handler is
// _____CFXNotificationCenterSetupConnection_block_invoke (0x1c01aac50, non-external), whose descriptor
// signature natively names an NSObject<OS_xpc_object> argument (this C restoration's block has a C
// signature). A dictionary event first prunes the restore attempts (behind the restore feature; an empty
// state without a timer is freed), then handles {method: "post_token", version: 1}: the token, the name
// (strlen of a missing name is not guarded, as natively), the object (none for a missing object or
// "kCFNotificationAnyObject") and the user info (a CFData of the XPC data's bytes, read as a property list)
// are posted with __CFXNotificationPostToken; the strings and the property list are released. An error event
// is ignored for XPC_ERROR_CONNECTION_INVALID and XPC_ERROR_TERMINATION_IMMINENT, schedules a restore for
// XPC_ERROR_CONNECTION_INTERRUPTED, and logs "Unknown or mysterious error received: %s" (CFLog level 3) with
// the event's description otherwise. Any other event is BRK #1. The XPC type and error objects are read
// through GOT slots outside the image (0x22241B998 dictionary, 0x22241B9B0 error, 0x22241B908 interrupted,
// 0x22241B910 and 0x22241B928 the two ignored errors); their binding is by use (the dictionary accessors that
// follow, the error branch, the restore); confidence: medium, and the assignment of the two ignored errors to
// the two slots is not observable.
// Folds the compiler-outlined pieces _____CFXNotificationCenterSetupConnection_block_invoke.cold.1
// (0x1c0308b24), _____CFXNotificationCenterSetupConnection_block_invoke.cold.2 (0x1c0308b50),
// _____CFXNotificationCenterSetupConnection_block_invoke.cold.3 (0x1c0308b90).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_2_9 (0x1c01aba9c).
static inline __attribute__((always_inline)) void __CFXNotificationCenterSetupConnection(
    OCFNotificationCenterRef center, const NotificationConfiguration *configuration
) {
    void *endpoint = configuration->endpoint;
    const char *serviceName = configuration->serviceName;
    uint8_t privileged = configuration->privileged;
    dispatch_queue_attr_t attributes = dispatch_queue_attr_make_with_qos_class(NULL, qos_class_main(), 0);
    center->queue = dispatch_queue_create("com.apple.notification_center.server_conn", attributes);
    if (endpoint != NULL) {
        center->connection = xpc_connection_create_from_endpoint((xpc_endpoint_t)endpoint);
        xpc_connection_set_target_queue(center->connection, center->queue);
    } else {
        center->connection = createMachServiceConnection(
            serviceName, center->queue, privileged == 0 ? 0 : XPC_CONNECTION_MACH_SERVICE_PRIVILEGED
        );
    }
    xpc_connection_set_event_handler(center->connection, ^(xpc_object_t event) {
      if (xpc_get_type(event) == XPC_TYPE_DICTIONARY) {
          if (_os_feature_enabled_impl("CoreFoundation", "CFNotificationCenterRestoreState")) {
              RestoreState *state = center == NULL ? NULL : center->restoreState;
              if (state != NULL) {
                  uint32_t recent = pruneRestoreAttempts(state);
                  if (state->timer == NULL && recent == 0) {
                      free(center->restoreState);
                      center->restoreState = NULL;
                  }
              }
          }
          const char *method = xpc_dictionary_get_string(event, "method");
          if (method == NULL || strcmp("post_token", method) != 0) {
              return;
          }
          if (xpc_dictionary_get_uint64(event, "version") != 1) {
              return;
          }
          uint64_t token = xpc_dictionary_get_uint64(event, "token");
          const char *nameBytes = xpc_dictionary_get_string(event, "name");
          OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
          OCFStringRef name = OCFStringCreateWithBytes(
              allocator, (const UInt8 *)nameBytes, (OCFIndex)strlen(nameBytes), kUTF8Encoding, false
          );
          const char *objectBytes = xpc_dictionary_get_string(event, "object");
          OCFStringRef object = NULL;
          if (objectBytes != NULL && strcmp(objectBytes, OF_NAME_STRING(kOCFNotificationAnyObject)) != 0) {
              object = OCFStringCreateWithBytes(
                  allocator, (const UInt8 *)objectBytes, (OCFIndex)strlen(objectBytes), kUTF8Encoding, false
              );
          }
          xpc_object_t userInfo = xpc_dictionary_get_value(event, "userinfo");
          if (userInfo != NULL) {
              const void *bytes = xpc_data_get_bytes_ptr(userInfo);
              OCFDataRef data =
                  OCFDataCreate(allocator, (const UInt8 *)bytes, (OCFIndex)xpc_data_get_length(userInfo));
              OCFPropertyListRef plist = OCFPropertyListCreateWithData(allocator, data, 0, NULL, NULL);
              OCFRelease(data);
              __CFXNotificationPostToken(center, token, name, object, plist);
              if (plist != NULL) {
                  OCFRelease(plist);
              }
          } else {
              __CFXNotificationPostToken(center, token, name, object, NULL);
          }
          if (object != NULL) {
              OCFRelease(object);
          }
          OCFRelease(name);
          return;
      }
      if (xpc_get_type(event) != XPC_TYPE_ERROR) {
          __builtin_trap();
      }
      if (event == XPC_ERROR_CONNECTION_INVALID) {
          return;
      }
      if (event == XPC_ERROR_CONNECTION_INTERRUPTED) {
          __CFXNotificationCenterScheduleRestoreState(center);
          return;
      }
      if (event == XPC_ERROR_TERMINATION_IMMINENT) {
          return;
      }
      char *description = xpc_copy_description(event);
      OCFLog(
          3, OF_STR("Unknown or mysterious error received: %s"), description == NULL ? "(null)" : description
      );
      if (description != NULL) {
          free(description);
      }
    });
    xpc_connection_activate(center->connection);
}

// Native _CFXNotificationCenterCreate, 0x1c01a886c. External.
// A distributed configuration (kind 1) first checks the fork state (__CF_USED unless bit 1, the fork trap and
// NULL when bit 0). A type-52 instance with 0x20 extra bytes; the info word gains bit 1 for kind 1 and bit 0
// for kind 2 and loses bit 3 (atomic compare-and-swap loops). The registrar (system default allocator) has
// options 2 for kinds 1 and 2, else 1; without it the instance is released and NULL returned. For kind 1 the
// process path "/usr/sbin/distnoted" sets __CFIsDistNoteDaemon. The connection, queue and restore state start
// NULL; a distributed center outside distnoted sets up its connection.
OCFNotificationCenterRef
_OCFXNotificationCenterCreate(OCFAllocatorRef allocator, const NotificationConfiguration *configuration) {
    if (configuration->kind == 1) {
        if ((__CF_FORK_STATE & 2) == 0) {
            __CF_USED();
        }
        if ((__CF_FORK_STATE & 1) != 0) {
            __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__();
        }
        if ((__CF_FORK_STATE & 1) != 0) {
            return NULL;
        }
    }
    OCFNotificationCenterRef center =
        (OCFNotificationCenterRef)_OCFRuntimeCreateInstance(allocator, 52, 0x20, NULL);
    if (center == NULL) {
        return NULL;
    }
    uint32_t kind = configuration->kind;
    if (kind == 1) {
        __atomic_fetch_or(&center->base.info, kCenterDistributed, __ATOMIC_SEQ_CST);
        kind = configuration->kind;
    }
    if (kind == 2) {
        __atomic_fetch_or(&center->base.info, kCenterDarwin, __ATOMIC_SEQ_CST);
    }
    __atomic_fetch_and(&center->base.info, ~(uint64_t)8, __ATOMIC_SEQ_CST);
    OCFXNotificationRegistrarRef registrar =
        OCFXNotificationRegistrarCreate(kOCFAllocatorSystemDefault, configuration->kind - 1 < 2 ? 2 : 1);
    if (registrar == NULL) {
        OCFRelease(center);
        return NULL;
    }
    center->registrar = registrar;
    if (configuration->kind == 1) {
        const char *path = _OCFProcessPath();
        if (path != NULL && strcmp("/usr/sbin/distnoted", path) == 0) {
            __CFIsDistNoteDaemon = 1;
        }
    }
    center->connection = NULL;
    center->queue = NULL;
    center->restoreState = NULL;
    if (configuration->kind == 1 && (__CFIsDistNoteDaemon & 1) == 0) {
        __CFXNotificationCenterSetupConnection(center, configuration);
    }
    return center;
}

// Native CFNotificationCenterGetLocalCenter, 0x1c01a8ad8, with CFNotificationCenterGetLocalCenter.cold.1
// (0x1c0308a50, through _OUTLINED_FUNCTION_1_11: dispatch_once of the static_init predicate, 0x22e6d28c0, and
// ___block_literal_global_7, whose invoke ___CFNotificationCenterGetLocalCenter_block_invoke, 0x1c01a8b08, is
// non-external). External. The block creates the task center from the standard local configuration with the
// system default allocator.
OCFNotificationCenterRef OCFNotificationCenterGetLocalCenter(void) {
    return taskCenter();
}

// Native CFNotificationCenterGetDistributedCenter, 0x1c01a8b38, with .cold.1 (0x1c0308a54: dispatch_once of
// .static_init, 0x22e6d28d0, and ___block_literal_global.10). External. The fork checks of the Darwin center.
// Reading: Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device: unread).
OCFNotificationCenterRef OCFNotificationCenterGetDistributedCenter(void) {
    static dispatch_once_t static_init;
    if ((__CF_FORK_STATE & 2) == 0) {
        __CF_USED();
    }
    if ((__CF_FORK_STATE & 1) != 0) {
        __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__();
    }
    if ((__CF_FORK_STATE & 1) != 0) {
        return NULL;
    }
    // Native ___CFNotificationCenterGetDistributedCenter_block_invoke, 0x1c01a8b9c. Non-external.
    dispatch_once(&static_init, ^{
      __hostCenter = _OCFXNotificationCenterCreate(
          kOCFAllocatorSystemDefault, &_kOCFXNotificationConfigurationStandardDistributedConfiguration
      );
    });
    return __hostCenter;
}

static OCFNotificationCenterRef __darwinCenter;

// Native CFNotificationCenterGetDarwinNotifyCenter, 0x1c01a8bcc. External. Folds .cold.1, 0x1c0308a68.
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC, confidence: verified.
// iOS Device 24A446: unread. Reading: Local/Evidence/cf-core/iphone-simulator-corpus-delta13.json.
OCFNotificationCenterRef OCFNotificationCenterGetDarwinNotifyCenter(void) {
    static dispatch_once_t static_init;
    if ((__CF_FORK_STATE & 2) == 0) {
        __CF_USED();
    }
    if ((__CF_FORK_STATE & 1) != 0) {
        __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__();
    }
    if ((__CF_FORK_STATE & 1) != 0) {
        return NULL;
    }
    // Native ___CFNotificationCenterGetDarwinNotifyCenter_block_invoke, 0x1c01a8c30. Non-external.
    dispatch_once(&static_init, ^{
      __darwinCenter = _OCFXNotificationCenterCreate(
          kOCFAllocatorSystemDefault, &_kOCFXNotificationConfigurationStandardDarwinConfiguration
      );
    });
    return __darwinCenter;
}

// Native _CFXNotificationGetTaskCenter, 0x1c01a8c60, with .cold.1 (0x1c0308a7c). External.
OCFNotificationCenterRef _OCFXNotificationGetTaskCenter(void) {
    return taskCenter();
}

// Native _CFXNotificationCenterCopyDebugInfo, 0x1c01a8f5c. External. A new mutable string (system default
// allocator) describing every registration of the center's registrar.
OCFStringRef _OCFXNotificationCenterCopyDebugInfo(OCFNotificationCenterRef center) {
    OCFMutableStringRef info = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    // Native ___CFXNotificationCenterCopyDebugInfo_block_invoke, 0x1c01a9004. Non-external. The token, the
    // name, the object (an object only when the registration is weak or the registrar compares objects, else
    // a pointer), the observer (an object when weak), the options and the handler: a block's invoke symbol, a
    // selector name, or a callback's symbol.
    OCFXNotificationRegistrarEnumerate(center->registrar, ^(const NotificationRegistration *registration) {
      OCFStringAppend(info, OF_STR("{\n"));
      OCFStringAppendFormat(info, NULL, OF_STR("    token = %llx\n"), registration->token);
      OCFStringAppendFormat(info, NULL, OF_STR("    name = %@\n"), registration->name);
      OCFStringRef objectFormat = OF_STR("    object = %@\n");
      if ((registration->flags & kObserverWeak) == 0 &&
          (OCFXNotificationRegistrarGetOptions(center->registrar) & 2) == 0) {
          objectFormat = OF_STR("    object = %p\n");
      }
      OCFStringAppendFormat(info, NULL, objectFormat, registration->object);
      if ((registration->flags & kObserverWeak) != 0) {
          OCFStringAppendFormat(info, NULL, OF_STR("    observer = %@\n"), registration->observer);
      } else {
          OCFStringAppendFormat(info, NULL, OF_STR("    observer = %p\n"), registration->observer);
      }
      OCFStringAppendFormat(info, NULL, OF_STR("    options = 0x%lx\n"), registration->flags);
      uint64_t flags = registration->flags;
      if ((flags & kHandlerBlock) != 0) {
          const void *block = registration->handler.callout;
          Dl_info symbol;
          memset(&symbol, 0, sizeof(symbol));
          dladdr(((const void *const *)block)[2], &symbol);
          if (symbol.dli_sname != NULL) {
              OCFStringAppendFormat(info, NULL, OF_STR("    block = %s (%p)\n"), symbol.dli_sname, block);
          } else {
              OCFStringAppendFormat(info, NULL, OF_STR("    block = %p\n"), block);
          }
      } else if ((flags & kHandlerSelector) != 0) {
          OCFStringAppendFormat(
              info, NULL, OF_STR("    selector = %s\n"), sel_getName((SEL)registration->handler.callout)
          );
      } else if ((flags & kHandlerCallback) != 0) {
          Dl_info symbol;
          memset(&symbol, 0, sizeof(symbol));
          dladdr(registration->handler.callout, &symbol);
          if (symbol.dli_sname != NULL) {
              OCFStringAppendFormat(
                  info,
                  NULL,
                  OF_STR("    callback = %s (%p)\n"),
                  symbol.dli_sname,
                  registration->handler.callout
              );
          } else {
              OCFStringAppendFormat(info, NULL, OF_STR("    callback = %p\n"), registration->handler.callout);
          }
      }
      OCFStringAppend(info, OF_STR("}\n"));
    });
    return info;
}

// Native _CFNotificationCenterRegisterDependentNotificationList, 0x1c01a9200. External. The list takes the
// first free slot of eight under the lock; with none free, .cold.1 (0x1c0308a80) sets the crash message "Too
// many invocations of _CFNotificationCenterRegisterDependentNotifications." and traps.
void _OCFNotificationCenterRegisterDependentNotificationList(const DependentNotification *const *list) {
    os_unfair_lock_lock(&_dependentNotificationLock);
    uint64_t slot = 0;
    while (_unprocessedDependentNotificationLists[slot] != NULL) {
        if (++slot == 8) {
            gCRAnnotations[1] = (uint64_t)(uintptr_t)"Too many invocations of "
                                                     "_CFNotificationCenterRegisterDependentNotifications.";
            __builtin_trap();
        }
    }
    _unprocessedDependentNotificationLists[slot] = list;
    os_unfair_lock_unlock(&_dependentNotificationLock);
}

// Native _CFNotificationCenterInitializeDependentNotificationIfNecessary, 0x1c01a9258. External. Under the
// lock, every pending list's records join the set (created with the system default allocator, comparing and
// hashing the names their first words point to) and the slot is cleared. The record for the name, if any,
// then runs its first registration once.
void _OCFNotificationCenterInitializeDependentNotificationIfNecessary(OCFStringRef name) {
    os_unfair_lock_lock(&_dependentNotificationLock);
    for (uint64_t slot = 0; slot != 8; slot++) {
        const DependentNotification *const *list = _unprocessedDependentNotificationLists[slot];
        if (list == NULL) {
            break;
        }
        const DependentNotification *const *cursor = list;
        const DependentNotification *record = *cursor;
        if (record != NULL) {
            cursor++;
            do {
                OCFMutableSetRef set = _dependentNotifications;
                if (set == NULL) {
                    OCFSetCallBacks callBacks;
                    memset(&callBacks, 0, offsetof(OCFSetCallBacks, equal));
                    callBacks.equal = _dependentNotificationsEqual;
                    callBacks.hash = _dependentNotificationHash;
                    set = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &callBacks);
                    _dependentNotifications = set;
                }
                OCFSetAddValue(set, record);
                record = *cursor++;
            } while (record != NULL);
        }
        _unprocessedDependentNotificationLists[slot] = NULL;
    }
    DependentNotification key;
    memset((char *)&key + sizeof(key.name), 0, sizeof(key) - sizeof(key.name));
    key.name = &name;
    const DependentNotification *record = OCFSetGetValue(_dependentNotifications, &key);
    os_unfair_lock_unlock(&_dependentNotificationLock);
    if (record == NULL) {
        return;
    }
    // Native ___CFNotificationCenterInitializeDependentNotificationIfNecessary_block_invoke, 0x1c01a93d4.
    // Non-external. The record's initializer runs first; then each sub-notification registers the record's
    // callback with the local, distributed or Darwin center (observer 0x7b, any object, the record's
    // suspension behavior), or with notify(3) on the main queue (a UTF-8 name of at most 1,024 bytes).
    // Folds the compiler-outlined piece
    // ____CFNotificationCenterInitializeDependentNotificationIfNecessary_block_invoke.cold.1 (0x1c0308a94).
    dispatch_once(record->initOnce, ^{
      const DependentNotification *dependent = record;
      if (dependent->initialize != NULL) {
          dependent->initialize();
      }
      for (uint64_t index = 0; index < dependent->count; index++) {
          DependentSubnotification subnotification = dependent->subnotifications[index];
          OCFNotificationCenterRef center;
          if (subnotification.kind > 1) {
              if (subnotification.kind != 2) {
                  if (subnotification.kind == 3) {
                      int token = 0;
                      char buffer[1024];
                      const char *cName =
                          OCFStringGetCStringPtr(subnotification.name, kOCFStringEncodingUTF8);
                      if (cName != NULL ||
                          (cName = buffer,
                           OCFStringGetCString(subnotification.name, buffer, 1024, kOCFStringEncodingUTF8))) {
                          // Native ___registerSubNotifications_block_invoke, 0x1c01ab834. Non-external. The
                          // record's callback, with the Darwin center and the sub-notification's name.
                          notify_register_dispatch(cName, &token, dispatch_get_main_queue(), ^(int notified) {
                            (void)notified;
                            OCFNotificationCallback callback = dependent->callback;
                            callback(
                                OCFNotificationCenterGetDarwinNotifyCenter(),
                                NULL,
                                subnotification.name,
                                NULL,
                                NULL
                            );
                          });
                      } else {
                          // The native code passes the record's first word, the address of the name, for the
                          // first %@ (kept; the conversion of these ASCII names does not fail).
                          os_log_error(
                              _CFOSLog(),
                              "Failed to register dependent notification %{public}@ -> %{public}@",
                              (id)dependent->name,
                              (id)subnotification.name
                          );
                      }
                  }
                  continue;
              }
              center = OCFNotificationCenterGetDarwinNotifyCenter();
          } else if (subnotification.kind != 0) {
              center = OCFNotificationCenterGetDistributedCenter();
          } else {
              center = taskCenter();
          }
          OCFNotificationCenterAddObserver(
              center,
              (const void *)0x7b,
              dependent->callback,
              subnotification.name,
              NULL,
              (OCFIndex)dependent->suspensionBehavior
          );
      }
    });
}

// Native __CFXNotificationCenterDarwinQueue (inlined into _CFXNotificationRegisterObserver), its .initOnce
// (0x22e6d2c38) and .queue (0x22e6d2c40), __DATA,__bss, and .cold.1 (0x1c0308ab8). Non-external.
static inline __attribute__((always_inline)) dispatch_queue_t __CFXNotificationCenterDarwinQueue(void) {
    static dispatch_once_t initOnce;
    static dispatch_queue_t queue;
    // Native ____CFXNotificationCenterDarwinQueue_block_invoke, 0x1c01ab894. Non-external. A serial queue
    // with a work-item autorelease frequency at the main thread's QoS class, not overcommitting, with no
    // target.
    dispatch_once(&initOnce, ^{
      dispatch_queue_attr_t attributes =
          dispatch_queue_attr_make_with_autorelease_frequency(NULL, DISPATCH_AUTORELEASE_FREQUENCY_WORK_ITEM);
      attributes = dispatch_queue_attr_make_with_qos_class(attributes, qos_class_main(), 0);
      attributes = dispatch_queue_attr_make_with_overcommit(attributes, false);
      queue = dispatch_queue_create_with_target("com.apple.CFNotificationCenter", attributes, NULL);
    });
    return queue;
}

// The UTF-8 bytes of a string in a new NUL-terminated buffer, as the native registration code computes them
// inline (two CFStringGetBytes passes over the whole string).
static inline __attribute__((always_inline)) char *copyUTF8Name(OCFStringRef string) {
    OCFIndex length = 0;
    OCFStringGetBytes(
        string, (OCFRange){0, OCFStringGetLength(string)}, kOCFStringEncodingUTF8, 0, false, NULL, 0, &length
    );
    char *bytes = malloc_type_malloc(length + 1, 0x100004077774924);
    OCFStringGetBytes(
        string,
        (OCFRange){0, OCFStringGetLength(string)},
        kOCFStringEncodingUTF8,
        0,
        false,
        (UInt8 *)bytes,
        length,
        NULL
    );
    bytes[length] = 0;
    return bytes;
}

// Native _CFXNotificationRegisterObserver, 0x1c01a961c. External. A local center (neither Darwin nor
// distributed) adds option 0x1000. A NULL name or object, or the any-observer sentinel as observer, traps; a
// distributed center inside distnoted registers nothing (token 0). A weak observer that is a class is stored
// strongly. The dependent notification for the name is initialized, then the registrar adds the observer. A
// Darwin center also registers the name with notify(3) on its queue (the token 0 when that fails); a
// distributed center sends {method: "register", version: 1, name, object, options without 0x2400, token, pn:
// the process path} unless an any-name any-object registration is refused by the sandbox, which logs a report
// instead.
uint64_t _OCFXNotificationRegisterObserver(
    OCFNotificationCenterRef center,
    OCFStringRef name,
    const void *object,
    OCFOptionFlags options,
    const void *observer,
    const void *callout,
    const void *context
) {
    if ((__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDarwin) == 0) {
        options |= (~(__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) << 11)) & 0x1000;
    }
    if (name == NULL || object == NULL || observer == (const void *)kOCFNotificationAnyObserver) {
        __builtin_trap();
    }
    if ((__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDistributed) != 0 &&
        (__CFIsDistNoteDaemon & 1) != 0) {
        return 0;
    }
    if ((options & kObserverWeak) != 0) {
        if (class_isMetaClass(object_getClass((id)observer))) {
            options &= ~(OCFOptionFlags)kObserverWeak;
        }
        if ((options & kObserverWeak) != 0) {
            objc_opt_self((id)observer);
        }
    }
    _OCFNotificationCenterInitializeDependentNotificationIfNecessary(name);
    uint64_t token = OCFXNotificationRegistrarAdd(
        center->registrar, name, object, (void *)observer, options, (void *)callout, context
    );
    int notifyToken = 0;
    void *pool = _OCFAutoreleasePoolPush();
    if ((__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDarwin) != 0) {
        char *cName = copyUTF8Name(name);
        dispatch_queue_t queue = __CFXNotificationCenterDarwinQueue();
        // Native ____CFXNotificationRegisterObserver_block_invoke, 0x1c01ab87c. Non-external. Posts the
        // token.
        uint32_t status = notify_register_dispatch(cName, &notifyToken, queue, ^(int notified) {
          (void)notified;
          __CFXNotificationPostToken(center, token, 0, 0, 0);
        });
        OCFXNotificationRegisteredObserverSetNotifyToken(center->registrar, token, notifyToken);
        free(cName);
        if (status != 0) {
            token = 0;
        }
    } else if ((__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDistributed) != 0) {
        char *cName = copyUTF8Name(name);
        char *cObject = copyUTF8Name((OCFStringRef)object);
        Boolean send;
        if ((strcmp(OF_NAME_STRING(kOCFNotificationAnyName), cName) |
             strcmp(OF_NAME_STRING(kOCFNotificationAnyObject), cObject)) == 0 &&
            sandbox_check(getpid(), NULL, 0) != 0) {
            char *report = __CFGenerateReport();
            OCFLog(
                4,
                OF_STR(
                    "*** attempt to register for all distributed notifications thwarted by sandboxing.%s%s"
                ),
                report == NULL ? "" : "\n\n",
                report == NULL ? "" : report
            );
            free(report);
            send = false;
        } else {
            send = true;
        }
        xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
        xpc_dictionary_set_string(message, "method", "register");
        xpc_dictionary_set_uint64(message, "version", 1);
        xpc_dictionary_set_string(message, "name", cName);
        xpc_dictionary_set_string(message, "object", cObject);
        xpc_dictionary_set_uint64(message, "options", options & 0xffffffffffffdbff);
        xpc_dictionary_set_uint64(message, "token", token);
        xpc_dictionary_set_string(message, "pn", *_OCFGetProcessPath());
        OCFXNotificationRegisteredObserverSetDistributedConnection(
            center->registrar, token, center->connection
        );
        if (send) {
            send_xpc_message(center->connection, message);
        }
        xpc_release(message);
        free(cObject);
        free(cName);
    }
    _OCFAutoreleasePoolPop(pool);
    return token;
}

// Native _CFXNotificationCancelToken, 0x1c01a9b44. External.
void _OCFXNotificationCancelToken(OCFNotificationCenterRef center, uint64_t token) {
    OCFXNotificationRegistrarRemoveToken(center->registrar, token);
}

// Native _CFXNotificationRemoveObservers, 0x1c01a9b4c. External. A NULL name, object or observer traps;
// nothing for a distributed center inside distnoted. A Darwin center removes for any object. In an
// autorelease pool.
void _OCFXNotificationRemoveObservers(
    OCFNotificationCenterRef center, OCFStringRef name, const void *object, const void *observer
) {
    if (name == NULL || object == NULL || observer == NULL) {
        __builtin_trap();
    }
    if ((__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDistributed) != 0 &&
        (__CFIsDistNoteDaemon & 1) != 0) {
        return;
    }
    void *pool = _OCFAutoreleasePoolPush();
    const void *removed = (__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDarwin) == 0
                              ? object
                              : kAnyObjectString;
    OCFXNotificationRegistrarRemove(center->registrar, name, removed, (void *)observer);
    _OCFAutoreleasePoolPop(pool);
}

// Native _CFXNotificationPostXPC, 0x1c01a9bf0, with _CFXNotificationPostXPC.cold.1 (0x1c0308adc). External.
// Nothing for a distributed center inside distnoted. The object defaults to "kCFNotificationAnyObject"; a
// NULL name is BRK #1. Only a distributed center without the local-only option posts, after the fork check
// (as in _CFXNotificationCenterCreate). The user info data becomes XPC data through a dispatch data
// (released); the name, object and entitlement key become UTF-8 C strings. The message is {method: "post",
// version: 1, name, object, userinfo, entitlement_key (when given), options, audience}. The post rate is
// sampled under a lock (lock_with_options 0x10000): the first post records its time; later ones store the
// interval at (index + 1) % 100 of 100 samples, and with more than 10 nonzero samples, more than 10 s after
// the last warning and an average rate (nonzero count / sum) above 60 per second, the warning time is updated
// and an error is logged after the unlock ("Detected potentially harmful notification post rate of %g
// notifications per second", OS_LOG_DEFAULT: the GOT slot 0x22241b4d8, as in Base/OCFUtilities.c; confidence:
// medium). The message is sent with send_xpc_message to the center's connection and released with the strings
// and the data.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_0_15 (0x1c01aba80), _OUTLINED_FUNCTION_3_4
// (0x1c01abaac).
void _OCFXNotificationPostXPC(
    OCFNotificationCenterRef center,
    OCFStringRef name,
    OCFStringRef object,
    OCFDataRef userInfo,
    OCFStringRef entitlementKey,
    uint64_t options,
    uint64_t audience
) {
    // Native _CFXNotificationPostXPC.lastPost 0x22e6d2900, .samples 0x22e6d2908, .lastWarning 0x22e6d2c28 and
    // .sampleLock 0x22e6d2c30 (__DATA,__bss), and .sampleIndex 0x22e6cf578 (__DATA,__data, initially
    // 0xffffffffffffffff, so the first sample goes to index 0). Non-external.
    static double lastPost;
    static double lastWarning;
    static uint64_t sampleIndex = 0xffffffffffffffff;
    static double samples[100];
    static os_unfair_lock sampleLock = OS_UNFAIR_LOCK_INIT;
    if ((__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDistributed) != 0 &&
        (__CFIsDistNoteDaemon & 1) != 0) {
        return;
    }
    if (object == NULL) {
        object = kAnyObjectString;
    }
    if (name == NULL) {
        __builtin_trap();
    }
    uint64_t info = __atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE);
    if ((options & kPostLocalOnly) != 0 || (info & kCenterDistributed) == 0) {
        return;
    }
    if ((__CF_FORK_STATE & 2) == 0) {
        __CF_USED();
    }
    if ((__CF_FORK_STATE & 1) != 0) {
        __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__();
    }
    if ((__CF_FORK_STATE & 1) != 0) {
        return;
    }
    xpc_object_t userInfoData = NULL;
    if (userInfo != NULL) {
        dispatch_data_t dispatchData = _CFDataCreateDispatchData(userInfo);
        userInfoData = xpc_data_create_with_dispatch_data(dispatchData);
        dispatch_release(dispatchData);
    }
    char *nameBytes = copyUTF8String(name);
    char *objectBytes = copyUTF8String(object);
    char *entitlementBytes = NULL;
    if (entitlementKey != NULL) {
        entitlementBytes = copyUTF8String(entitlementKey);
    }
    xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_string(message, "method", "post");
    xpc_dictionary_set_uint64(message, "version", 1);
    xpc_dictionary_set_string(message, "name", nameBytes);
    xpc_dictionary_set_string(message, "object", objectBytes);
    xpc_dictionary_set_value(message, "userinfo", userInfoData);
    if (entitlementBytes != NULL) {
        xpc_dictionary_set_string(message, "entitlement_key", entitlementBytes);
    }
    xpc_dictionary_set_uint64(message, "options", options);
    xpc_dictionary_set_uint64(message, "audience", audience);
    double now = OCFAbsoluteTimeGetCurrent();
    os_unfair_lock_lock_with_options(&sampleLock, 0x10000);
    bool warned = false;
    double rate = 0.0;
    if (lastPost == 0.0) {
        lastPost = now;
    } else {
        uint64_t index = (sampleIndex + 1) % 100;
        double previous = lastPost;
        lastPost = now;
        sampleIndex = index;
        samples[index] = now - previous;
        double sum = 0.0;
        uint64_t nonzero = 0;
        for (int sample = 0; sample < 100; sample++) {
            double value = samples[sample];
            double added = sum + value;
            if (value != 0.0) {
                nonzero++;
                sum = added;
            }
        }
        if (nonzero > 10 && now - lastWarning > 10.0) {
            rate = (double)nonzero / sum;
            if (rate > 60.0) {
                lastWarning = now;
                warned = true;
            }
        }
    }
    os_unfair_lock_unlock(&sampleLock);
    if (warned && os_log_type_enabled(OS_LOG_DEFAULT, OS_LOG_TYPE_ERROR)) {
        uint8_t buffer[12] = {0x00, 0x01, 0x00, 0x08};
        memcpy(&buffer[4], &rate, sizeof(rate));
        _os_log_error_impl(&__dso_handle, OS_LOG_DEFAULT, OS_LOG_TYPE_ERROR, postRateFormat, buffer, 12);
    }
    send_xpc_message(center->connection, message);
    xpc_release(message);
    if (entitlementBytes != NULL) {
        free(entitlementBytes);
    }
    free(objectBytes);
    free(nameBytes);
    if (userInfoData != NULL) {
        xpc_release(userInfoData);
    }
}

// Native _CFXNotificationPost, 0x1c01aa0e0. External.
// Nothing for a distributed center inside distnoted. The name and object (default "kCFNotificationAnyObject")
// are asked first; a NULL notification or name is BRK #1. Inside an autorelease pool: without the local-only
// option a Darwin center posts the name's UTF-8 bytes with notify_post, and a distributed center posts
// through _CFXNotificationPostXPC with the user info as binary property list data (nothing when the data
// cannot be made). Otherwise, after the pool is popped, the registrar finds the observers into a 32-record
// stack buffer (header {0, 32} from 0x1c0373630); each registration whose token is still valid is posted with
// __CFXRegistrationPost (not on the main queue) and the operations it returns are collected (32 on the stack,
// else malloc type 0x80040B8603338), the buffer is destroyed, and each operation is waited for and released.
void _OCFXNotificationPost(OCFNotificationCenterRef center, void *notification, OCFOptionFlags options) {
    if ((__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDistributed) != 0 &&
        (__CFIsDistNoteDaemon & 1) != 0) {
        return;
    }
    OCFStringRef name = ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)notification, sel_getUid("name"));
    OCFTypeRef object = ((OCFTypeRef (*)(id, SEL))objc_msgSend)((id)notification, sel_getUid("object"));
    if (object == NULL) {
        object = kAnyObjectString;
    }
    if (notification == NULL || name == NULL) {
        __builtin_trap();
    }
    void *pool = _OCFAutoreleasePoolPush();
    if ((options & kPostLocalOnly) == 0 &&
        (__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDarwin) != 0) {
        char *bytes = copyUTF8String(name);
        notify_post(bytes);
        free(bytes);
        _OCFAutoreleasePoolPop(pool);
        return;
    }
    if ((options & kPostLocalOnly) == 0 &&
        (__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDistributed) != 0) {
        OCFDictionaryRef userInfo =
            ((OCFDictionaryRef (*)(id, SEL))objc_msgSend)((id)notification, sel_getUid("userInfo"));
        if (userInfo == NULL) {
            _OCFXNotificationPostXPC(center, name, (OCFStringRef)object, NULL, NULL, options, 0);
        } else {
            OCFDataRef data = OCFPropertyListCreateData(kOCFAllocatorSystemDefault, userInfo, 200, 0, NULL);
            if (data != NULL) {
                _OCFXNotificationPostXPC(center, name, (OCFStringRef)object, data, NULL, options, 0);
                OCFRelease(data);
            }
        }
        _OCFAutoreleasePoolPop(pool);
        return;
    }
    _OCFAutoreleasePoolPop(pool);
    OCFTypeRef operationStorage[32] = {NULL};

    struct {
        NotificationRegistrationBuffer header;
        NotificationRegistration storage[32];
    } found;

    memset((uint8_t *)&found + 8, 0, sizeof(found) - 8);
    found.header.count = 0;
    found.header.capacity = 32;
    found.header.records = found.storage;
    OCFXNotificationRegistrarFind(center->registrar, name, object, &found.header);
    OCFTypeRef *operations = operationStorage;
    if (found.header.count > 32) {
        operations = malloc_type_malloc((size_t)found.header.count << 3, 0x80040B8603338);
    }
    int64_t pending = 0;
    if (found.header.count != 0) {
        for (uint64_t index = 0; index < found.header.count; index++) {
            if (OCFXNotificationRegistrarValidateToken(
                    center->registrar, found.header.records[index].token
                )) {
                NotificationRegistration *registration = &found.header.records[index];
                OCFTypeRef operation =
                    __CFXRegistrationPost(center, registration, false, notification, registration->observer);
                if (operation != NULL) {
                    operations[pending] = operation;
                    pending++;
                }
            }
        }
        _OCFXNotificationRegistrationBufferDestroy(&found.header);
        for (int64_t index = 0; index < pending; index++) {
            id operation = (id)operations[index];
            ((void (*)(id, SEL))objc_msgSend)(operation, sel_getUid("waitUntilFinished"));
            objc_release(operation);
        }
    } else {
        _OCFXNotificationRegistrationBufferDestroy(&found.header);
    }
    if (operations != operationStorage) {
        free(operations);
    }
}

// Native __CFXRegistrationPost, 0x1c01aa490. Non-external.
// NULL for a NULL registration or notification. NSOperationQueue and NSBlockOperation are looked up once
// (function-static caches __CFXRegistrationPost.operationQueueClass and .blockOperationClass). The handler is
// copied; a block observer's context is its operation queue. A weak observer is retained. The block delivers
// with a token check when onMainQueue is set and the observer has neither the asynchronous flag nor a queue;
// the center is retained for it. With a queue: the block runs inline when the queue is the current queue, or
// the main queue on the main thread; otherwise a new NSBlockOperation with the block is added to the queue
// and returned (owned by the caller). Without a queue: the asynchronous flag dispatches the block to the
// global queue of the main thread's QoS (flags 2); onMainQueue dispatches it to the main queue (the GOT slot
// 0x222417150 outside the image, the only queue among the image's dispatch data imports: _dispatch_main_q;
// confidence: medium); else it runs inline.
// The block (____CFXRegistrationPost_block_invoke, 0x1c01ab8f8, non-external; no copy helpers, so the
// notification and observer are not retained by the block, as natively) calls out unless the token check
// fails, then releases its handler copy, the center and the retained weak observer.
static OCFTypeRef __CFXRegistrationPost(
    OCFNotificationCenterRef center,
    NotificationRegistration *registration,
    Boolean onMainQueue,
    void *notification,
    void *observer
) {
    static Class operationQueueClass;
    static Class blockOperationClass;
    if (registration == NULL || notification == NULL) {
        return NULL;
    }
    if (operationQueueClass == NULL) {
        operationQueueClass = objc_lookUpClass(OF_NAME_STRING(ONSOperationQueue));
    }
    if (blockOperationClass == NULL) {
        blockOperationClass = objc_lookUpClass(OF_NAME_STRING(ONSBlockOperation));
    }
    NotificationHandler handler = {NULL, NULL};
    uint64_t flags = registration->flags;
    OCFXNotificationHandlerCopy(
        &handler, registration->handler.callout, registration->handler.context, (uint32_t)flags
    );
    OCFTypeRef queue = (flags & kHandlerBlock) != 0 ? handler.context : NULL;
    if ((flags & kObserverWeak) != 0) {
        objc_retain((id)observer);
    }
    uint64_t token = registration->token;
    Boolean validates = ((flags & kObserverAsynchronous) | (uintptr_t)queue) == 0 ? onMainQueue : true;
    OCFRetain(center);
    void (^block)(void) = ^{
      NotificationHandler copied = handler;
      if (validates != 1 || OCFXNotificationRegistrarValidateToken(center->registrar, token)) {
          __CFNOTIFICATIONCENTER_IS_CALLING_OUT_TO_AN_OBSERVER__(
              center, handler.callout, handler.context, observer, notification, flags
          );
      }
      OCFXNotificationHandlerRelease(&copied, (uint32_t)flags);
      OCFRelease(center);
      if ((flags & kObserverWeak) != 0) {
          objc_release((id)observer);
      }
    };
    if (queue != NULL) {
        id current = ((id (*)(id, SEL))objc_msgSend)((id)operationQueueClass, sel_getUid("currentQueue"));
        if ((OCFTypeRef)current != queue) {
            bool mainQueue = false;
            if (pthread_main_np() == 1) {
                id main = ((id (*)(id, SEL))objc_msgSend)((id)operationQueueClass, sel_getUid("mainQueue"));
                mainQueue = (OCFTypeRef)main == queue;
            }
            if (!mainQueue) {
                id operation = objc_opt_new(blockOperationClass);
                ((void (*)(id, SEL, void (^)(void)))objc_msgSend)(
                    operation, sel_getUid("addExecutionBlock:"), block
                );
                ((void (*)(id, SEL, id))objc_msgSend)((id)queue, sel_getUid("addOperation:"), operation);
                return (OCFTypeRef)operation;
            }
        }
        block();
        return NULL;
    }
    if ((flags & kObserverAsynchronous) != 0) {
        dispatch_async(dispatch_get_global_queue(qos_class_main(), 2), block);
        return NULL;
    }
    if (onMainQueue) {
        dispatch_async(dispatch_get_main_queue(), block);
        return NULL;
    }
    block();
    return NULL;
}

// Native _CFXNotificationCenterIsEmpty, 0x1c01aa688. External.
Boolean _OCFXNotificationCenterIsEmpty(OCFNotificationCenterRef center) {
    return OCFXNotificationRegistrarIsEmpty(center->registrar);
}

// Native CFNotificationCenterAddObserver, 0x1c01aa920. External. The center must be a notification center
// (type 52). The suspension behavior's low four bits choose the options (1 drop: 2, 3 hold: 8, 4 coalesce: 1,
// 0xe: 0x10001, otherwise deliver immediately: 4); its 0x400 bit makes the observer weak. The name defaults
// to the any name, a NULL object or any object on a Darwin center to the any object, a NULL observer to the
// placeholder; the callback is registered with option 0x80000 and no context.
void OCFNotificationCenterAddObserver(
    OCFNotificationCenterRef center,
    const void *observer,
    OCFNotificationCallback callBack,
    OCFStringRef name,
    const void *object,
    OCFIndex suspensionBehavior
) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(center);
    if (typeID != 52) {
        _CFAssertMismatchedTypeID(52, typeID);
    }
    OCFOptionFlags options;
    switch (suspensionBehavior & 0xf) {
        case 1:
            options = 2;
            break;
        case 3:
            options = 8;
            break;
        case 4:
            options = 1;
            break;
        case 0xe:
            options = 0x10001;
            break;
        default:
            options = 4;
            break;
    }
    if ((suspensionBehavior & 0x400) != 0) {
        options |= kObserverWeak;
        objc_opt_self((id)observer);
    }
    OCFStringRef registered = name != NULL ? name : kOCFNotificationAnyName;
    const void *registeredObject = object;
    if (object == NULL || (__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDarwin) != 0) {
        registeredObject = kAnyObjectString;
    }
    const void *registeredObserver = observer != NULL ? observer : kCFXNotificationPlaceholderObserver;
    _OCFXNotificationRegisterObserver(
        center,
        registered,
        registeredObject,
        options | kHandlerCallback,
        registeredObserver,
        (const void *)callBack,
        NULL
    );
}

// Native CFNotificationCenterPostNotificationWithOptions, 0x1c01aab9c. External. A center whose
// non-Objective-C type ID is not 52 fails with __CFAssertMismatchedTypeID. The notification is a new
// __CFNotification
// (-initWithName:object:userInfo:foundation: with NO) whose object and user info are dropped for a Darwin
// center; it is posted and released.
void OCFNotificationCenterPostNotificationWithOptions(
    OCFNotificationCenterRef center,
    OCFStringRef name,
    const void *object,
    OCFDictionaryRef userInfo,
    OCFOptionFlags options
) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(center);
    if (typeID != 52) {
        _CFAssertMismatchedTypeID(52, typeID);
    }
    id allocated = objc_alloc((Class)notificationClass);
    const void *postedObject =
        (__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDarwin) == 0 ? object : NULL;
    OCFDictionaryRef postedUserInfo =
        (__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDarwin) == 0 ? userInfo : NULL;
    id notification = ((id (*)(id, SEL, OCFStringRef, const void *, OCFDictionaryRef, BOOL))objc_msgSend)(
        allocated,
        sel_getUid("initWithName:object:userInfo:foundation:"),
        name,
        postedObject,
        postedUserInfo,
        NO
    );
    _OCFXNotificationPost(center, notification, options);
    objc_release(notification);
}

// Native CFNotificationCenterPostNotification, 0x1c01aac44. External. Options 1 when deliverImmediately is
// nonzero, else 0 (a tail call).
void OCFNotificationCenterPostNotification(
    OCFNotificationCenterRef center,
    OCFStringRef name,
    const void *object,
    OCFDictionaryRef userInfo,
    Boolean deliverImmediately
) {
    OCFNotificationCenterPostNotificationWithOptions(center, name, object, userInfo, deliverImmediately != 0);
}

// Native ___CFXNotificationPostToken, 0x1c01ab2f4. Non-external. The registrar finds the token into a
// one-record stack buffer (header {0, 1} from 0x1c0375160); when found, a __CFNotification (foundation YES)
// with the name (the registration's name when NULL), the object and the user info is posted on the main queue
// and released, and the returned operation is released. The buffer is destroyed in every case.
static void __CFXNotificationPostToken(
    OCFNotificationCenterRef center,
    uint64_t token,
    OCFStringRef name,
    OCFStringRef object,
    OCFTypeRef userInfo
) {
    struct {
        NotificationRegistrationBuffer header;
        NotificationRegistration storage[1];
    } found;

    memset((uint8_t *)&found + 8, 0, sizeof(found) - 8);
    found.header.count = 0;
    found.header.capacity = 1;
    found.header.records = found.storage;
    if (OCFXNotificationRegistrarFindToken(center->registrar, token, &found.header)) {
        if (name == NULL) {
            name = found.header.records[0].name;
        }
        id allocated = objc_alloc((Class)notificationClass);
        id notification = ((id (*)(id, SEL, OCFStringRef, OCFStringRef, OCFTypeRef, BOOL))objc_msgSend)(
            allocated, sel_getUid("initWithName:object:userInfo:foundation:"), name, object, userInfo, YES
        );
        OCFTypeRef operation = __CFXRegistrationPost(
            center, &found.header.records[0], true, notification, found.header.records[0].observer
        );
        objc_release(notification);
        objc_release((id)operation);
    }
    _OCFXNotificationRegistrationBufferDestroy(&found.header);
}

// Native _dependentNotificationsEqual, 0x1c01ab814. Non-external. The dependent-notification set's equality:
// the names the records' first words point to.
static Boolean _dependentNotificationsEqual(const void *first, const void *second) {
    return OCFEqual(
        *((const DependentNotification *)first)->name, *((const DependentNotification *)second)->name
    );
}

// Native _dependentNotificationHash, 0x1c01ab828. Non-external.
static OCFHashCode _dependentNotificationHash(const void *value) {
    return OCFHash(*((const DependentNotification *)value)->name);
}

// Native ___CFNOTIFICATIONCENTER_IS_CALLING_OUT_TO_AN_OBSERVER__, 0x1c01ab9a0. Non-external (kept out of
// line; its name marks callouts in backtraces). The placeholder observer becomes NULL. A block handler is
// called with the notification; a selector handler is sent to the observer with the notification; a C
// callback gets the center, the observer, the name, the object and the user info (for a Darwin center the
// object is "kCFNotificationAnyObject" and the user info NULL, each without asking the notification). The
// context is not used.
static __attribute__((noinline)) void __CFNOTIFICATIONCENTER_IS_CALLING_OUT_TO_AN_OBSERVER__(
    OCFNotificationCenterRef center,
    void *callout,
    OCFTypeRef context,
    void *observer,
    void *notification,
    uint64_t flags
) {
    (void)context;
    void *target = observer == kCFXNotificationPlaceholderObserver ? NULL : observer;
    if ((flags & kHandlerBlock) != 0) {
        ((void (^)(void *))callout)(notification);
    } else if ((flags & kHandlerSelector) != 0) {
        ((void (*)(id, SEL, id))objc_msgSend)((id)target, (SEL)callout, (id)notification);
    } else if ((flags & kHandlerCallback) != 0) {
        OCFStringRef name = ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)notification, sel_getUid("name"));
        const void *object;
        if ((__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDarwin) != 0) {
            object = kAnyObjectString;
        } else {
            object = ((const void *(*)(id, SEL))objc_msgSend)((id)notification, sel_getUid("object"));
        }
        OCFDictionaryRef userInfo;
        if ((__atomic_load_n(&center->base.info, __ATOMIC_ACQUIRE) & kCenterDarwin) != 0) {
            userInfo = NULL;
        } else {
            userInfo =
                ((OCFDictionaryRef (*)(id, SEL))objc_msgSend)((id)notification, sel_getUid("userInfo"));
        }
        ((OCFNotificationCallback)callout)(center, target, name, object, userInfo);
    }
}
