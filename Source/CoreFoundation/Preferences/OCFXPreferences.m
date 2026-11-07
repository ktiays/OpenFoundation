//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Native _OUTLINED_FUNCTION_1_42, 0x1c02f2354. Non-external; folded register/frame/logging setup in its
// callers.
#import "CoreFoundation/Preferences/OCFXPreferences.h"
#import "CoreFoundation/Base/OCFPlatform.h"
#import "CoreFoundation/Base/OCFUtilities.h"
#import "CoreFoundation/Preferences/OCFPreferences.h"
#import "CoreFoundation/Preferences/OCFPrefsDaemon.h"
#import "CoreFoundation/Preferences/OCFPDSource.h"
#import "CoreFoundation/Preferences/OCFPrefsPlistSource.h"
#import "CoreFoundation/Preferences/OCFPrefsSearchListSource.h"
#import "CoreFoundation/Preferences/OCFPrefsSource.h"
#import "CoreFoundation/Preferences/OCFXPCBridge.h"
#include "CoreFoundation/Stream/OCFConcreteStreams.h"
#include "CoreFoundation/URL/OCFURLDeletingLastPathComponent.h"

#include <assert.h>
#include <dispatch/dispatch.h>
#include <fcntl.h>
#include <malloc/malloc.h>
#include <mach/mach_time.h>
#include <notify.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/activity.h>
#include <os/lock.h>
#include <os/log.h>
#include <os/signpost.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// Inferred organization: applyBlockApplier (0x1c02ea124) through
// ___sharedPreferencesHandle_block_invoke (0x1c02f231c) lie with the _CFXPreferences, _CFXPreferencesHandle
// and NSUserDefaults methods in the iOS Simulator __text section, together with the CFPreferences C entry
// points; -[_CFXPreferences _deliverPendingKVONotifications] (0x1c0315ed0) sits with the other
// _CFXPreferences direct methods. The restored functions of that unit are kept in this translation unit; the
// C entry points call direct methods, so the unit is Objective-C (compiled with manual reference counting).
// The kCFPreferences constants (__DATA_CONST,__const 0x2227aafa8-0x2227ab578, strings
// 0x2227bdf20-0x2227be000) are placed here. The .cold fragments (0x1c0316e6c, 0x1c03170b8, 0x1c03170d4,
// 0x1c0316cb4-0x1c0316f38) and OUTLINED_FUNCTION_0_59, _18_4, _29_2, _8_9, _20_3, _38 and _2_36-_30_2
// (0x1c02f2368-0x1c02f2570) are kept in their parents. The _CFXPreferencesHandle methods have their own
// @implementation in native address order (the handle's direct method, placed natively with the cold code,
// stays in it). Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full).
// iOS Device 24A446: unread. Reading: Local/Scratch/cf-core-prefs-daemon/REPORT.md (main checkout;
// integration reading pending).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_0_59 (0x1c02f2340).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_18_4 (0x1c02f24a8), _OUTLINED_FUNCTION_29_2
// (0x1c02f255c).

extern char userDefaultsClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSUserDefaults));

// libxpc SPI used by -[_CFXPreferences init] (0x1c02eaa9c).
extern xpc_object_t xpc_copy_entitlement_for_self(const char *entitlement);

// libsystem feature-flag entry point (os/feature_private.h, not in the SDK); supplied, never reimplemented.
extern bool _os_feature_enabled_impl(const char *domain, const char *feature);

// The pending-KVO thread-specific slot (16) and its "armed, nothing pending" marker value (16); the builtin
// type IDs of CFString (7) and CFNumber (22); kCFNumberDoubleType (13) and kCFNumberCFIndexType (14).
#define kPendingKVOSlot 16
#define kLogInProgressSlot 15
#define kArmedMarker ((void *)16)
#define kStringTypeID ((OCFTypeID)7)
#define kNumberTypeID ((OCFTypeID)22)
#define kDoubleNumberType ((OCFNumberType)13)
#define kIndexNumberType ((OCFNumberType)14)

// Native kCFPreferencesAnyApplication 0x2227ab0b0, kCFPreferencesCurrentApplication 0x2227aafa8,
// kCFPreferencesAnyUser 0x2227aafe0, kCFPreferencesCurrentUser 0x2227aafb0,
// kCFPreferencesDidChangeNotificationName 0x2227ab578 and kCFPreferencesDidChangeObject 0x2227ab570
// (__DATA_CONST,__const), pointing at the constant strings 0x2227b3ec0, 0x2227bdf40, 0x2227bdf20,
// 0x2227bdf80, 0x2227be000 and 0x2227bdfe0. External. The values are the native strings in both naming modes;
// the literals are split so that the prefix checker does not read them as runtime names of the prefixed
// declarations.
static const OCFConstantString anyApplicationString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFPreferences"
    "AnyApplication",
    28
};
static const OCFConstantString currentApplicationString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFPreferences"
    "CurrentApplication",
    32
};
static const OCFConstantString anyUserString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFPreferences"
    "AnyUser",
    21
};
static const OCFConstantString currentUserString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFPreferences"
    "CurrentUser",
    25
};
static const OCFConstantString didChangeNotificationNameString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "com.apple.CFPreferences._didChange", 34
};
static const OCFConstantString didChangeObjectString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "com.apple.CFPreferences", 23
};
const OCFStringRef kOCFPreferencesAnyApplication = (OCFStringRef)&anyApplicationString;
const OCFStringRef kOCFPreferencesCurrentApplication = (OCFStringRef)&currentApplicationString;
const OCFStringRef kOCFPreferencesAnyUser = (OCFStringRef)&anyUserString;
const OCFStringRef kOCFPreferencesCurrentUser = (OCFStringRef)&currentUserString;
const OCFStringRef kOCFPreferencesDidChangeNotificationName = (OCFStringRef)&didChangeNotificationNameString;
const OCFStringRef kOCFPreferencesDidChangeObject = (OCFStringRef)&didChangeObjectString;

// Native kCFPreferencesByteCountLimitReachedNotificationName (__DATA_CONST,__const 0x2227ab580), pointing at
// the constant string 0x2227be020. External.
static const OCFConstantString byteCountLimitReachedString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "com.apple.CFPreferences.byteCountLimitReached", 45
};
const OCFStringRef kOCFPreferencesByteCountLimitReachedNotificationName =
    (OCFStringRef)&byteCountLimitReachedString;

// Native _CFPrefsDictionaryApplyBlock, 0x1c02ea114. Hidden (was a private external). A tail call of
// CFDictionaryApplyFunction with applyBlockApplier and the block as context.
void _CFPrefsDictionaryApplyBlock(
    OCFDictionaryRef dictionary, void (^block)(const void *key, const void *value)
) {
    OCFDictionaryApplyFunction(dictionary, applyBlockApplier, (void *)block);
}

// Native applyBlockApplier, 0x1c02ea124. Non-external natively; hidden here (its callers are in this unit,
// declared in Preferences/OCFPreferences.h for the preference units). A CFDictionaryApplierFunction whose
// context is a block taking the key and the value; a tail call of the block's invoke function.
void applyBlockApplier(const void *key, const void *value, void *context) {
    ((void (^)(const void *, const void *))context)(key, value);
}

// Native _CFPrefsArmPendingKVOSlot, 0x1c02ea218. Hidden (was a private external).
// An empty pending-KVO slot is armed with the marker 16 and dummyKVODestructor; true when this call armed it.
bool _CFPrefsArmPendingKVOSlot(void) {
    void *pending = _OCFGetTSD(kPendingKVOSlot);
    if (pending == NULL) {
        _OCFSetTSD(kPendingKVOSlot, kArmedMarker, dummyKVODestructor);
    }
    return pending == NULL;
}

// Native dummyKVODestructor, 0x1c02ea25c. Non-external natively; hidden here (declared in
// Preferences/OCFPreferences.h). The destructor of the armed pending-KVO slot: it does nothing.
void dummyKVODestructor(void *value) {
    (void)value;
}

// Native _CFPNLPO (__DATA,__data 0x22e6cff68). Hidden (was a private external); hidden until the owner
// records a naming decision. The non-launch persona override, -1 (none) initially; written by
// -[_CFXPreferencesHandle setNonLaunchPersonaOverride:].
int _CFPNLPO = -1;

// libsystem_kernel's persona interface (sys/persona.h, not in the SDK); supplied. The native callers pass a
// 348-byte zeroed kpersona_info with version 2 and read the persona ID (offset 4) and type (offset 8); the
// remaining fields are spelled as their bytes (unread).
typedef struct {
    uint32_t version;
    uint32_t personaID;
    int personaType;
    uint8_t unread[336];
} PersonaInfo;

_Static_assert(sizeof(PersonaInfo) == 348, "The native persona buffer is 348 bytes.");

extern int kpersona_info(uint32_t personaID, PersonaInfo *info);
extern int kpersona_pidinfo(pid_t pid, PersonaInfo *info);

// libsystem_sandbox's sandbox_check and its no-report flag (GOT slot 0x22241b290, the binding inferred as in
// Preferences/OCFPrefsSource.m: confidence medium); supplied. The persona checks query the syscall-unix
// operation with filter type 14 and system call 494 (persona).
extern const uint32_t SANDBOX_CHECK_NO_REPORT;
extern int sandbox_check(pid_t pid, const char *operation, int type, ...);

// libxpc's xpc_connection_create_mach_service, which the iOS SDK marks unavailable although the native image
// imports it; declared under a local name bound to the libxpc symbol (as in
// Notification/OCFNotificationCenter.c).
extern xpc_connection_t createMachServiceConnection(
    const char *name, dispatch_queue_t targetQueue, uint64_t flags
) __asm(OF_SYMBOL("xpc_connection_create_mach_service"));

// libxpc's user-session and error-string interfaces (not in the SDK); supplied. Signatures from the native
// register use: the session UID query takes no argument the caller sets (confidence medium), the foreground
// query writes an error code through its argument and answers the UID.
extern bool xpc_user_sessions_enabled(void);
extern uid_t xpc_user_sessions_get_session_uid(void);
extern uid_t xpc_user_sessions_get_foreground_uid(int *error);
extern void xpc_connection_set_target_user_session_uid(xpc_connection_t connection, uid_t uid);
extern const char *xpc_strerror(int error);

// KAUTH_UID_NONE (sys/kauth.h, not in the iOS SDK): the "no user" UID the foreground query answers (-101).
#define kUIDNone ((uid_t)0xffffff9b)

// libdispatch's voucher interface (os/voucher_private.h, not in the SDK); supplied.
extern void *voucher_copy(void);
extern void voucher_release(void *voucher);

@implementation _OCFXPreferencesHandle

// Native -[_CFXPreferencesHandle init], 0x1c02ea260. After -[NSObject init] (selector reference 0x2387aa0ea,
// outside the extracted image; -init inferred, confidence medium): new preferences (objc_alloc_init), the
// lock cleared with the EUID override -1 (one 64-bit store), the non-launch persona ID -1 and no checked
// voucher
// (-1). When the sandbox allows the persona system call, the process can have multiple personas when its
// persona type, with bit 1 cleared, is 4.
- (id)init {
    self = [super init];
    if (self != nil) {
        prefs = [[_OCFXPreferences alloc] init];
        lock = OS_UNFAIR_LOCK_INIT;
        overrideEUID = (uint32_t)-1;
        nonLaunchPersonaID = (uint32_t)-1;
        lastCheckedVoucher = (void *)-1;
        if (sandbox_check(getpid(), "syscall-unix", SANDBOX_CHECK_NO_REPORT | 14, 494) == 0) {
            PersonaInfo info;
            memset(&info, 0, sizeof(info));
            info.version = 2;
            if (kpersona_pidinfo(getpid(), &info) == 0) {
                processCanHaveMultiplePersonas = (info.personaType & ~2) == 4;
            }
        }
    }
    return self;
}

// Native -[_CFXPreferencesHandle dealloc], 0x1c02ea3a8. A checked voucher is released, then the preferences,
// then the superclass's -dealloc (selector reference 0x2387a9f83).
// CoreFoundation defect: -[_CFXPreferencesHandle dealloc] (0x1c02ea3a8, confidence verified from the
// assembly) releases only the ivar at offset 8 (prefs) and never the non-launch persona preferences (offset
// 16), an unintended leak. Divergence (owner decision): nonLaunchPersonaPrefs is released after prefs.
- (void)dealloc {
    if (lastCheckedVoucher != (void *)-1) {
        voucher_release(lastCheckedVoucher);
    }
    [prefs release];
    [nonLaunchPersonaPrefs release];
    [super dealloc];
}

// Native -[_CFXPreferencesHandle setEUIDOverride:], 0x1c02ea42c.
- (void)setEUIDOverride:(uint32_t)euid {
    overrideEUID = euid;
}

// Native -[_CFXPreferencesHandle setNonLaunchPersonaOverride:], 0x1c02ea434. Stores the process-wide
// override, not an ivar.
- (void)setNonLaunchPersonaOverride:(uint32_t)personaID {
    _CFPNLPO = (int)personaID;
}

// Native -[_CFXPreferencesHandle copyPrefs], 0x1c02ea440, with its .cold.1 (0x1c0316cb4, the EUID debug log).
// Under the lock: when the effective user (the override, or _CFGetEUID) differs from the preferences', new
// preferences ingest the old ones' volatile state, the non-launch persona ID is reset and the change logged.
// A process that can have multiple personas uses its cached persona decision when the voucher check allows
// (it never does: -_canUseCachedPersonaInfo answers NO), else asks the sandbox and kpersona_info (persona 0):
// the launch persona is used unless the persona is managed (type 2). The override, when set, names the
// persona. Without an override and with the launch persona, the preferences are answered; otherwise the
// non-launch persona preferences, replaced (ingesting the previous ones, or the launch preferences) and
// logged when the persona changed. The answer is retained; replaced objects are released after unlocking.
- (_OCFXPreferences *)copyPrefs {
    os_unfair_lock_lock(&lock);
    uint32_t euid = overrideEUID;
    if (euid == (uint32_t)-1) {
        euid = _OCFGetEUID();
    }
    _OCFXPreferences *previous = prefs;
    if (previous->_euid == euid) {
        previous = nil;
    } else {
        prefs = [[_OCFXPreferences alloc] init];
        [prefs ingestVolatileStateFromPreferences:previous];
        nonLaunchPersonaID = (uint32_t)-1;
        os_log_debug(
            _CFPrefsClientLog(),
            "Replacing preferences for EUID change: %u -> %u %{public}@",
            previous->_euid,
            prefs->_euid,
            prefs
        );
    }
    uint32_t personaID;
    BOOL useLaunchPersona;
    if (processCanHaveMultiplePersonas == 1) {
        if ([self _canUseCachedPersonaInfo]) {
            useLaunchPersona = cachedUseLaunchPersona;
            personaID = nonLaunchPersonaID;
        } else {
            if (sandbox_check(getpid(), "syscall-unix", SANDBOX_CHECK_NO_REPORT | 14, 494) != 0) {
                personaID = 0;
                useLaunchPersona = YES;
            } else {
                PersonaInfo info;
                memset(&info, 0, sizeof(info));
                info.version = 2;
                int result = kpersona_info(0, &info);
                personaID = result != 0 ? 0 : info.personaID;
                useLaunchPersona = info.personaType != 2 || result != 0;
            }
            cachedUseLaunchPersona = useLaunchPersona;
        }
    } else {
        personaID = 0;
        useLaunchPersona = YES;
    }
    uint32_t persona = _CFPNLPO == -1 ? personaID : (uint32_t)_CFPNLPO;
    _OCFXPreferences *previousNonLaunch = nil;
    _OCFXPreferences **answer = &prefs;
    if (_CFPNLPO != -1 || !useLaunchPersona) {
        if (nonLaunchPersonaID != persona) {
            previousNonLaunch = nonLaunchPersonaPrefs;
            nonLaunchPersonaPrefs = [[_OCFXPreferences alloc] init];
            nonLaunchPersonaPrefs->_nonLaunchPersonaUID = persona;
            [nonLaunchPersonaPrefs
                ingestVolatileStateFromPreferences:previousNonLaunch != nil ? previousNonLaunch : prefs];
            os_log_debug(
                _CFPrefsClientLog(),
                "Replacing preferences for non-launch persona UID change: %u -> %u %{public}@",
                nonLaunchPersonaID,
                persona,
                nonLaunchPersonaPrefs
            );
            nonLaunchPersonaID = persona;
        }
        answer = &nonLaunchPersonaPrefs;
    }
    _OCFXPreferences *result = [*answer retain];
    os_unfair_lock_unlock(&lock);
    if (previous != nil) {
        [previous release];
    }
    if (previousNonLaunch != nil) {
        [previousNonLaunch release];
    }
    return result;
}

// Native -[_CFXPreferencesHandle forEachPrefsPerformBlock:], 0x1c02ea770. Both preferences objects are
// retained under the lock, passed to the block in order (the non-launch one possibly nil), and released.
- (void)forEachPrefsPerformBlock:(void (^)(_OCFXPreferences *preferences))block {
    os_unfair_lock_lock(&lock);
    _OCFXPreferences *launch = [prefs retain];
    _OCFXPreferences *nonLaunch = [nonLaunchPersonaPrefs retain];
    os_unfair_lock_unlock(&lock);
    block(launch);
    block(nonLaunch);
    [launch release];
    [nonLaunch release];
}

// Native -[_CFXPreferencesHandle _canUseCachedPersonaInfo], 0x1c0315b60. Direct method. The last checked
// voucher (unless none, NULL or -1) is released and the current voucher copied in its place; NO is always
// answered, as natively.
- (BOOL)_canUseCachedPersonaInfo {
    void *voucher = lastCheckedVoucher;
    if ((uintptr_t)voucher + 1 >= 2) {
        voucher_release(voucher);
    }
    lastCheckedVoucher = voucher_copy();
    return NO;
}

@end

// Native managedPreferencesLock (__DATA,__bss 0x22e6d3d58) and the function statics of
// locked_getManagedStateCache, knownManagedPrefsStates and knownContainerizedManagedPrefsStates (__DATA,__bss
// 0x22e6d3d60 and 0x22e6d3d68). Non-external. They are spelled at file scope.
static os_unfair_lock managedPreferencesLock = OS_UNFAIR_LOCK_INIT;
static OCFMutableDictionaryRef knownManagedPrefsStates;
static OCFMutableDictionaryRef knownContainerizedManagedPrefsStates;

static OCFMutableDictionaryRef locked_getManagedStateCache(bool containerized);

// Native _CFPrefsResetManagedPreferencesStateCache, 0x1c02eb020 (from
// Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json). Hidden (was a private external). Under the
// managed-preferences lock, the containerized cache, then the plain one, is emptied (each created first when
// missing; locked_getManagedStateCache is inlined here natively).
void _CFPrefsResetManagedPreferencesStateCache(void) {
    os_unfair_lock_lock(&managedPreferencesLock);
    OCFDictionaryRemoveAllValues(locked_getManagedStateCache(true));
    OCFDictionaryRemoveAllValues(locked_getManagedStateCache(false));
    os_unfair_lock_unlock(&managedPreferencesLock);
}

// Native __CFPrefsCurrentAppIdentifierCache (__DATA,__bss 0x22e6d3d50). Non-external; read plainly, published
// with a compare-and-swap (CASAL).
static OCFStringRef _CFPrefsCurrentAppIdentifierCache;

// Native _CFPrefsGetCacheStringForBundleID, 0x1c02eb9cc. Hidden (was a private external).
// The current application (by identity, by the cache's identity or by equality) answers the cached
// identifier, computed on first use from _NSSwiftBundle's main-bundle compatibility identifier
// (CoreFoundation/CFSwiftBundle feature) or the main bundle's identifier, falling back to the process name;
// the protected copy is published once, a losing copy is released and the winner answered. The cached string
// itself answers itself; any application, "Apple Global Domain" and ".GlobalPreferences" answer
// kCFPreferencesAnyApplication; any other identifier answers itself.
OCFStringRef _CFPrefsGetCacheStringForBundleID(OCFStringRef identifier) {
    OCFStringRef cache = _CFPrefsCurrentAppIdentifierCache;
    if (identifier == kOCFPreferencesCurrentApplication || cache == identifier ||
        OCFEqual(identifier, kOCFPreferencesCurrentApplication)) {
        if (cache != NULL) {
            return cache;
        }
        OCFStringRef bundleIdentifier = NULL;
        if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
            bundleIdentifier = ((OCFStringRef (*)(id, SEL))objc_msgSend)(
                [objc_lookUpClass(OF_NAME_STRING(_ONSSwiftBundle)) class],
                sel_getUid("mainBundleCompatibilityIdentifier")
            );
        } else {
            OCFBundleRef mainBundle = OCFBundleGetMainBundle();
            if (mainBundle != NULL) {
                bundleIdentifier = OCFBundleGetIdentifier(mainBundle);
            }
        }
        if (bundleIdentifier == NULL || OCFStringGetLength(bundleIdentifier) == 0) {
            bundleIdentifier = _CFProcessNameString();
            if (bundleIdentifier == NULL) {
                return NULL;
            }
        }
        OCFStringRef protectedIdentifier = _CFStringCopyBundleUnloadingProtectedString(bundleIdentifier);
        OCFStringRef expected = NULL;
        if (!__atomic_compare_exchange_n(
                &_CFPrefsCurrentAppIdentifierCache,
                &expected,
                protectedIdentifier,
                false,
                __ATOMIC_SEQ_CST,
                __ATOMIC_SEQ_CST
            )) {
            OCFRelease(protectedIdentifier);
            return _CFPrefsCurrentAppIdentifierCache;
        }
        return protectedIdentifier;
    }
    if (cache != NULL && OCFEqual(cache, identifier)) {
        return cache;
    }
    OCFStringRef anyApplication = kOCFPreferencesAnyApplication;
    if (OCFEqual(identifier, anyApplication) || OCFEqual(identifier, OF_STR("Apple Global Domain")) ||
        OCFEqual(identifier, OF_STR(".GlobalPreferences"))) {
        return anyApplication;
    }
    return identifier;
}

// Native _CFPrefsDeliverPendingKVONotificationsGuts, 0x1c02ee190, with its .cold.1 (0x1c0316e6c, the grouping
// loop with OUTLINED_FUNCTION_29_2 and _18_4) and ____CFPrefsDeliverPendingKVONotificationsGuts_block_invoke
// (0x1c02f2060, the global block), _block_invoke_2 (0x1c02f2268) and _block_invoke_3 (0x1c02f2270).
// Non-external. The pending notifications are grouped per source (pointer keys, CFType value callbacks; the
// source is read through its atomic direct getter, inlined natively). For each group, the old values of every
// notification are added, and every new value adds kCFNull as its old value and sets the new value; the
// source then notifies its observers inside an autorelease pool. Dictionaries are released in @finally blocks
// (region bounds inferred from the rethrow structure: confidence medium).
static void _CFPrefsDeliverPendingKVONotificationsGuts(OCFArrayRef pending) {
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableDictionaryRef bySource =
        OCFDictionaryCreateMutable(allocator, 0, NULL, &kOCFTypeDictionaryValueCallBacks);
    OCFIndex count = OCFArrayGetCount(pending);
    for (OCFIndex index = 0; index < count; ++index) {
        _OCFPrefsPendingKVONotification *notification =
            (_OCFPrefsPendingKVONotification *)OCFArrayGetValueAtIndex(pending, index);
        OCFMutableArrayRef notifications =
            (OCFMutableArrayRef)OCFDictionaryGetValue(bySource, (const void *)notification.source);
        if (notifications == NULL) {
            notifications = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
            OCFDictionarySetValue(bySource, (const void *)notification.source, notifications);
            OCFRelease(notifications);
        }
        OCFArrayAppendValue(notifications, notification);
    }
    @try {
        OCFDictionaryApplyFunction(
            bySource, applyBlockApplier, (void *)^(OCFStringRef sourceKey, void *value) {
              (void)sourceKey;
              OCFArrayRef notifications = value;
              OCFMutableDictionaryRef oldValues = OCFDictionaryCreateMutable(
                  kOCFAllocatorSystemDefault,
                  0,
                  &kOCFTypeDictionaryKeyCallBacks,
                  &kOCFTypeDictionaryValueCallBacks
              );
              OCFMutableDictionaryRef newValues = OCFDictionaryCreateMutable(
                  kOCFAllocatorSystemDefault,
                  0,
                  &kOCFTypeDictionaryKeyCallBacks,
                  &kOCFTypeDictionaryValueCallBacks
              );
              id source = nil;
              OCFIndex notificationCount = OCFArrayGetCount(notifications);
              for (OCFIndex index = 0; index < notificationCount; ++index) {
                  _OCFPrefsPendingKVONotification *notification =
                      (_OCFPrefsPendingKVONotification *)OCFArrayGetValueAtIndex(notifications, index);
                  source = notification->source;
                  OCFDictionaryRef previous = notification->oldValues;
                  OCFDictionaryRef current = notification->values;
                  if (previous != NULL) {
                      OCFDictionaryApplyFunction(
                          previous, applyBlockApplier, (void *)^(OCFStringRef key, void *old) {
                            OCFDictionaryAddValue(oldValues, key, old);
                          }
                      );
                  }
                  if (current != NULL) {
                      OCFDictionaryApplyFunction(
                          current, applyBlockApplier, (void *)^(OCFStringRef key, void *new) {
                            OCFDictionaryAddValue(oldValues, key, kOCFNull);
                            OCFDictionarySetValue(newValues, key, new);
                          }
                      );
                  }
              }
              @try {
                  @autoreleasepool {
                      [(id<PreferencesUnrestoredMessages>)source
                          _notifyObserversOfChangeFromValuesForKeys:oldValues
                                                    toValuesForKeys:newValues];
                  }
              } @finally {
                  OCFRelease(oldValues);
                  OCFRelease(newValues);
              }
            }
        );
    } @finally {
        OCFRelease(bySource);
    }
}

// Native _CFXPreferencesGetByHostIdentifierString, 0x1c02efda8 (from
// Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json). External. The empty constant string
// (0x2227ac180, length 0).
OCFStringRef _OCFXPreferencesGetByHostIdentifierString(void) {
    return OF_STR("");
}

// Native _NSUserDefaultsRegisteredAtLeastOnce (__DATA,__bss 0x22e6d3d48). Non-external; its writer,
// _CFPreferencesRegisterStandardUserDefaultsExists (0x1c02efc2c), is outside this restoration.
static bool _NSUserDefaultsRegisteredAtLeastOnce;

// Native _CFPreferencesDoesNSUserDefaultsExist, 0x1c02efc64. Hidden (was a private external).
Boolean _CFPreferencesDoesNSUserDefaultsExist(void) {
    return _NSUserDefaultsRegisteredAtLeastOnce;
}

// Native _CFPreferencesCopyStandardUserDefaults, 0x1c02efc70. Hidden (was a private external).
// Once NSUserDefaults has registered, +_copyStandardUserDefaultsIfPresent (selector registered at run time,
// sent through objc_msgSend) of the NSUserDefaults class object; else NULL.
void *_CFPreferencesCopyStandardUserDefaults(void) {
    if (!_NSUserDefaultsRegisteredAtLeastOnce) {
        return NULL;
    }
    id userDefaults = [(Class)(void *)userDefaultsClass class];
    return ((id (*)(id, SEL))objc_msgSend)(
        userDefaults, sel_registerName("_copyStandardUserDefaultsIfPresent")
    );
}

// Native directCFPrefsD: no symbol; inlined into -shmemForRole:name: and
// -_copyDaemonConnectionSettingUpIfNecessaryForRole:andUserIdentifier: (its once token and daemon are
// __MergedGlobals 0x22e6d3da0 and 0x22e6d3d98; OUTLINED_FUNCTION_15_4 is the dispatch_once). Its block,
// ___directCFPrefsD_block_invoke (0x1c02f22bc, the global block ___block_literal_global.229), creates the
// in-process daemon (objc_alloc and the direct initializer) with role 3, in test mode when the process is not
// restricted and __CFPreferencesTestDaemon is set (getenv).
static OCFPrefsDaemon *directCFPrefsD(void) {
    static dispatch_once_t onceToken;
    static OCFPrefsDaemon *daemon;
    dispatch_once(&onceToken, ^{
      daemon = [[OCFPrefsDaemon alloc]
          initWithRole:3
              testMode:!__CFProcessIsRestricted() && getenv("__CFPreferencesTestDaemon") != NULL];
    });
    return daemon;
}

// Native sharedPreferencesHandle: no symbol; inlined into the C entry points (its once token and handle are
// __MergedGlobals 0x22e6d3db0 and 0x22e6d3da8; OUTLINED_FUNCTION_0_59 is the dispatch_once). Its block,
// ___sharedPreferencesHandle_block_invoke (0x1c02f231c), allocates and initializes the handle
// (objc_alloc_init).
static _OCFXPreferencesHandle *sharedPreferencesHandle(void) {
    static dispatch_once_t onceToken;
    static _OCFXPreferencesHandle *handle;
    dispatch_once(&onceToken, ^{
      handle = [[_OCFXPreferencesHandle alloc] init];
    });
    return handle;
}

// Native ___block_literal_global.174, the global block of _CFPrefsResetUserSessionPreferences, with its
// invoke ____CFPrefsResetUserSessionPreferences_block_invoke (0x1c02efc24, from
// Local/Evidence/cf-core/iphone-simulator-corpus-delta12.json): a tail call of -resetUserPreferences on the
// preferences object it receives. The spelling is a local descriptive name.
static void (^const resetUserSessionPreferencesBlock)(_OCFXPreferences *) = ^(_OCFXPreferences *preferences) {
  [preferences resetUserPreferences];
};

// Native _CFPrefsResetUserSessionPreferences, 0x1c02efb54, with its .cold.1 (0x1c0317070, the once of
// sharedPreferencesHandle through OUTLINED_FUNCTION_1_42). Hidden (was a private external); placed after
// sharedPreferencesHandle, which it inlines natively. Inside an activity "Resetting
// CFPreferences/NSUserDefaults" (current activity as parent, default flags; its scope state zeroed), both
// preferences objects of the shared handle are reset with the block above; the activity is released before
// its scope is left. The parent is read through GOT slot 0x22241b4a8, outside the extracted image; the
// binding to OS_ACTIVITY_CURRENT is inferred from the call (confidence medium).
void _CFPrefsResetUserSessionPreferences(void) {
    os_activity_t activity = os_activity_create(
        "Resetting CFPreferences/NSUserDefaults", OS_ACTIVITY_CURRENT, OS_ACTIVITY_FLAG_DEFAULT
    );
    struct os_activity_scope_state_s state = {0, 0};
    os_activity_scope_enter(activity, &state);
    [sharedPreferencesHandle() forEachPrefsPerformBlock:resetUserSessionPreferencesBlock];
    [activity release];
    os_activity_scope_leave(&state);
}

// Native _CFPreferencesCopyAppValueWithContainerAndConfiguration, 0x1c02f0128, with its .cold.1
// (0x1c03170b8). External. Outside the preferences daemon, the shared handle's preferences answer the value
// (released after the message); in the daemon the message goes to nil.
OCFPropertyListRef _OCFPreferencesCopyAppValueWithContainerAndConfiguration(
    OCFStringRef key, OCFStringRef applicationID, OCFStringRef container, OCFURLRef cloudConfigurationURL
) {
    _OCFXPreferences *preferences =
        _CFPrefsCurrentProcessIsCFPrefsD() ? nil : [sharedPreferencesHandle() copyPrefs];
    OCFPropertyListRef value = [preferences copyAppValueForKey:key
                                                    identifier:applicationID
                                                     container:container
                                              configurationURL:cloudConfigurationURL];
    [preferences release];
    return value;
}

// Native CFPreferencesCopyAppValue, 0x1c02f01c4. External. No container, no configuration URL.
OCFPropertyListRef OCFPreferencesCopyAppValue(OCFStringRef key, OCFStringRef applicationID) {
    return _OCFPreferencesCopyAppValueWithContainerAndConfiguration(key, applicationID, NULL, NULL);
}

// Native _CFPreferencesGetAppBooleanValueWithContainer, 0x1c02f01d0. External.
Boolean _OCFPreferencesGetAppBooleanValueWithContainer(
    OCFStringRef key, OCFStringRef applicationID, OCFStringRef container, Boolean *keyExistsAndHasValidFormat
) {
    OCFPropertyListRef value =
        _OCFPreferencesCopyAppValueWithContainerAndConfiguration(key, applicationID, container, NULL);
    Boolean result = _OCFPreferencesGetBooleanValueWithValue(value, keyExistsAndHasValidFormat);
    if (value != NULL) {
        OCFRelease(value);
    }
    return result;
}

// Native _CFPreferencesGetBooleanValueWithValue, 0x1c02f0214. External.
// kCFBooleanTrue/False answer themselves (valid). Numbers: float types compare the double with 0.0 (NaN is
// true), others the CFIndex value with 0; validity is CFNumberGetValue's result. Strings: "YES"/"true" and
// "NO"/"false" case-insensitively, then "1" and "0" by equality. Anything else (and NULL) is invalid and
// false.
Boolean
_OCFPreferencesGetBooleanValueWithValue(OCFPropertyListRef value, Boolean *keyExistsAndHasValidFormat) {
    if (value == NULL) {
        goto invalid;
    }
    if (value == kOCFBooleanTrue) {
        goto yes;
    }
    if (value == kOCFBooleanFalse) {
        goto no;
    }
    OCFTypeID type = OCFGetTypeID(value);
    if (type == kNumberTypeID) {
        if (OCFNumberIsFloatType(value)) {
            double number = 0.0;
            Boolean valid = OCFNumberGetValue(value, kDoubleNumberType, &number);
            if (keyExistsAndHasValidFormat != NULL) {
                *keyExistsAndHasValidFormat = valid;
            }
            return number != 0.0;
        }
        OCFIndex number = 0;
        Boolean valid = OCFNumberGetValue(value, kIndexNumberType, &number);
        if (keyExistsAndHasValidFormat != NULL) {
            *keyExistsAndHasValidFormat = valid;
        }
        return number != 0;
    }
    if (type != kStringTypeID) {
        goto invalid;
    }
    if (OCFStringCompare(value, OF_STR("YES"), 1) == kOCFCompareEqualTo ||
        OCFStringCompare(value, OF_STR("true"), 1) == kOCFCompareEqualTo) {
        goto yes;
    }
    if (OCFStringCompare(value, OF_STR("NO"), 1) == kOCFCompareEqualTo ||
        OCFStringCompare(value, OF_STR("false"), 1) == kOCFCompareEqualTo) {
        goto no;
    }
    if (OCFEqual(value, OF_STR("1"))) {
        goto yes;
    }
    if (OCFEqual(value, OF_STR("0"))) {
        goto no;
    }
invalid:
    if (keyExistsAndHasValidFormat != NULL) {
        *keyExistsAndHasValidFormat = false;
    }
    return false;
yes:
    if (keyExistsAndHasValidFormat != NULL) {
        *keyExistsAndHasValidFormat = true;
    }
    return true;
no:
    if (keyExistsAndHasValidFormat != NULL) {
        *keyExistsAndHasValidFormat = true;
    }
    return false;
}

// Native CFPreferencesGetAppBooleanValue, 0x1c02f03cc. External. No container.
Boolean OCFPreferencesGetAppBooleanValue(
    OCFStringRef key, OCFStringRef applicationID, Boolean *keyExistsAndHasValidFormat
) {
    return _OCFPreferencesGetAppBooleanValueWithContainer(
        key, applicationID, NULL, keyExistsAndHasValidFormat
    );
}

// Native CFPreferencesAppSynchronize, 0x1c02f08f8, with its .cold.1 (0x1c03170d4). External. Outside the
// preferences daemon, the shared handle's preferences synchronize the application (no container); in the
// daemon the direct method's nil check answers false.
Boolean OCFPreferencesAppSynchronize(OCFStringRef applicationID) {
    _OCFXPreferences *preferences =
        _CFPrefsCurrentProcessIsCFPrefsD() ? nil : [sharedPreferencesHandle() copyPrefs];
    Boolean result = [preferences appSynchronizeWithIdentifier:applicationID container:NULL];
    [preferences release];
    return result;
}

static bool _CFPrefsIsUserManaged(OCFStringRef user, bool containerized);

// Native CFPreferencesIsManaged, 0x1c02f15a0 (from
// Local/Evidence/cf-core/iphone-simulator-corpus-delta12.json, as the rest of this group). Hidden (was a
// private external). kCFPreferencesAnyUser (CFEqual with the constant string), or kCFPreferencesCurrentUser
// in a process whose cfprefsd effective user is root, asks for kCFPreferencesAnyUser. Otherwise
// kCFPreferencesCurrentUser is replaced by the current user name (_CFPrefsDCopyCurrentUserName inside
// cfprefsd, CFCopyUserName elsewhere) and any other user retained; the answer comes from
// _CFPrefsIsUserManaged and the user is released.
bool CFPreferencesIsManaged(OCFStringRef user, bool containerized) {
    if (OCFEqual(user, (OCFStringRef)&anyUserString) ||
        (OCFEqual(user, (OCFStringRef)&currentUserString) && cfprefsdEuid() == 0)) {
        return _CFPrefsIsUserManaged((OCFStringRef)&anyUserString, containerized);
    }
    if (OCFEqual(user, (OCFStringRef)&currentUserString)) {
        if (_CFPrefsCurrentProcessIsCFPrefsD()) {
            user = _CFPrefsDCopyCurrentUserName();
        } else {
            user = OCFCopyUserName();
        }
    } else {
        OCFRetain(user);
    }
    bool managed = _CFPrefsIsUserManaged(user, containerized);
    OCFRelease(user);
    return managed;
}

// Native _CFPrefsIsUserManaged, 0x1c02f1658, with .cold.1 (0x1c0317120, the debug log). Non-external.
// Under the managed-preferences lock, the cached state of the user (a CFNumber of kCFNumberIntType, 9) is
// answered when nonzero. Otherwise the state is computed: 0 without a managed preferences path (a zeroed
// 1,024-byte buffer, no identifier), else 1 when stat of the path succeeds and 2 when it fails; it is logged
// at debug level ("enabled" for 1) and cached (system default allocator). The answer is whether the state is
// 1.
static bool _CFPrefsIsUserManaged(OCFStringRef user, bool containerized) {
    os_unfair_lock_lock(&managedPreferencesLock);
    OCFNumberRef cached = OCFDictionaryGetValue(locked_getManagedStateCache(containerized), user);
    int state = 0;
    if (cached == NULL || !OCFNumberGetValue(cached, kOCFNumberIntType, &state) || state == 0) {
        char path[1024];
        bzero(path, sizeof(path));
        if (_CFPrefsGetPathForManagedBundleID(NULL, user, containerized, path)) {
            struct stat info;
            memset(&info, 0, sizeof(info));
            state = stat(path, &info) != 0 ? 2 : 1;
        } else {
            state = 0;
        }
        os_log_debug(
            _CFPrefsClientLog(),
            "Managed preferences are %{public}s for %{private}@",
            state == 1 ? "enabled" : "disabled",
            user
        );
        int value = state;
        OCFMutableDictionaryRef cache = locked_getManagedStateCache(containerized);
        OCFNumberRef number = OCFNumberCreate(kOCFAllocatorSystemDefault, kOCFNumberIntType, &value);
        OCFDictionarySetValue(cache, user, number);
        OCFRelease(number);
    }
    os_unfair_lock_unlock(&managedPreferencesLock);
    return state == 1;
}

// Native locked_getManagedStateCache, 0x1c02f17d4. Non-external (also inlined into
// _CFPrefsResetManagedPreferencesStateCache). Each cache is created on first use (system default allocator,
// CFType key and value callbacks). The lock must be held.
static OCFMutableDictionaryRef locked_getManagedStateCache(bool containerized) {
    OCFMutableDictionaryRef *cache =
        containerized ? &knownContainerizedManagedPrefsStates : &knownManagedPrefsStates;
    if (*cache == NULL) {
        *cache = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    return *cache;
}

// Native __MergedGlobals 0x22e6d3d74 (__DATA,__bss): the daemon connection lock of
// -_copyDaemonConnectionSettingUpIfNecessaryForRole:andUserIdentifier:, also taken by
// -resetUserPreferences. Non-external; connectionLock is a local descriptive name.
static os_unfair_lock connectionLock = OS_UNFAIR_LOCK_INIT;

// Native _CFPreferencesWriteDomainDictionaryToPath, 0x1c02f1984. External.
// The adjusted file URL is returned autoreleased by -_URLByInsertingResolveFlags:.
Boolean _OCFPreferencesWriteDomainDictionaryToPath(
    OCFDictionaryRef dictionary, OCFStringRef directory, OCFStringRef identifier
) {
    if (dictionary == NULL) {
        return false;
    }
    SInt32 errorCode = 0;
    OCFStringRef managedPrefix = OF_STR("/Library/Managed Preferences/");
    if (OCFStringHasPrefix(directory, managedPrefix)) {
        OCFIndex prefixLength = OCFStringGetLength(managedPrefix);
        OCFIndex suffixLength = OCFStringGetLength(directory) - prefixLength;
        OCFStringRef suffix = OCFStringCreateWithSubstring(
            kOCFAllocatorSystemDefault, directory, (OCFRange){prefixLength, suffixLength}
        );
        OCFStringRef rewritten =
            OCFStringCreateWithFormat(NULL, NULL, OF_STR("/private/var/Managed Preferences/%@"), suffix);
        OCFRelease(suffix);
        directory = OCFAutorelease(rewritten);
    }
    OCFStringRef path = OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault, NULL, OF_STR("%@/%@%@"), directory, identifier, OF_STR(".plist")
    );
    bool result = false;
    if (path != NULL) {
        OCFURLRef originalURL =
            OCFURLCreateWithFileSystemPath(kOCFAllocatorSystemDefault, path, kOCFURLPOSIXPathStyle, false);
        OCFURLRef url = (OCFURLRef)((id (*)(id, SEL, unsigned int))objc_msgSend)(
            (id)originalURL, sel_getUid("_URLByInsertingResolveFlags:"), 1
        );
        if (originalURL != NULL) {
            OCFRelease(originalURL);
        }
        if (url != NULL) {
            if (OCFDictionaryGetCount(dictionary) != 0) {
                OCFWriteStreamRef stream = OCFWriteStreamCreateWithAllocatedBuffers(
                    kOCFAllocatorSystemDefault, kOCFAllocatorSystemDefault
                );
                OCFWriteStreamOpen(stream);
                OCFPropertyListWrite(dictionary, stream, kOCFPropertyListBinaryFormat_v1_0, 0, NULL);
                OCFWriteStreamClose(stream);
                OCFDataRef data = OCFWriteStreamCopyProperty(stream, kOCFStreamPropertyDataWritten);
                OCFRelease(stream);
                if (data != NULL) {
                    result = OCFURLWriteDataAndPropertiesToResource(url, data, NULL, &errorCode);
                    OCFRelease(data);
                }
            } else {
                UInt8 bytes[1026] = {0};
                if (OCFURLGetFileSystemRepresentation(url, true, bytes, 1024)) {
                    result = unlink((const char *)bytes) == 0;
                    OCFURLRef parent =
                        OCFURLCreateCopyDeletingLastPathComponent(kOCFAllocatorSystemDefault, url);
                    if (parent != NULL) {
                        bzero(bytes, sizeof(bytes));
                        if (OCFURLGetFileSystemRepresentation(parent, true, bytes, 1024)) {
                            rmdir((const char *)bytes);
                        }
                        OCFRelease(parent);
                    }
                }
            }
        }
        OCFRelease(path);
    }
    const void *identifiers[] = {identifier};
    OCFArrayRef domains = OCFArrayCreate(kOCFAllocatorSystemDefault, identifiers, 1, &kOCFTypeArrayCallBacks);
    _OCFPreferencesManagementStatusChangedForDomains(domains);
    OCFRelease(domains);
    return result;
}

static uint32_t _postCFPreferencesChangedDomainsNotification(void);

// Native _CFPreferencesManagementStatusChangedForDomains, 0x1c02f1c34. External.
// Folds .cold.1, 0x1c0317194, into the existing shared handle accessor.
void _OCFPreferencesManagementStatusChangedForDomains(OCFArrayRef domains) {
    (void)domains;
    if ((__CF_FORK_STATE & 2) == 0) {
        __CF_USED();
    }
    if ((__CF_FORK_STATE & 1) != 0) {
        __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__();
    }
    if ((__CF_FORK_STATE & 1) != 0) {
        return;
    }
    _OCFXPreferences *preferences =
        _CFPrefsCurrentProcessIsCFPrefsD() ? nil : [sharedPreferencesHandle() copyPrefs];
    [preferences flushManagedSources];
    [preferences release];
    OCFNotificationCenterRef center = OCFNotificationCenterGetDarwinNotifyCenter();
    OCFNotificationCenterPostNotificationWithOptions(
        center, OF_STR("com.apple.MCX._managementStatusChangedForDomains"), NULL, NULL, 0
    );
    notify_post("com.apple.MCX._managementStatusChangedForDomains");
    _postCFPreferencesChangedDomainsNotification();
}

// Native _postCFPreferencesChangedDomainsNotification, 0x1c02f1fa0. Non-external.
// The first notification's name is the empty constant string at 0x2227ac180 (kept).
static uint32_t _postCFPreferencesChangedDomainsNotification(void) {
    OCFNotificationCenterRef center = OCFNotificationCenterGetDarwinNotifyCenter();
    OCFNotificationCenterPostNotificationWithOptions(center, OF_STR(""), NULL, NULL, 0);
    return notify_post("com.apple.CFPreferences._domainsChangedExternally");
}

@implementation _OCFXPreferences

// Native -[_CFXPreferences ingestVolatileStateFromPreferences:], 0x1c02ea710, with its .cold.1 (0x1c0316d20,
// the assertion "prefs != self", CFXPreferences.m line 1353), .cold.2 (0x1c0316d48, the block setup and
// call), and the blocks ___54-[_CFXPreferences ingestVolatileStateFromPreferences:]_block_invoke
// (0x1c02ee260), _block_invoke_2 (0x1c02ee2e4) and _block_invoke_3 (0x1c02ee374, with its .cold.1
// 0x1c0316f38, the assertion "copy != NULL", line 1358). Direct method. Under this object's
// named-volatile-sources lock and then the other's, each of the other's named volatile sources is copied for
// this object and stored under the same name (_CFPrefsDictionaryApplyBlock inlined natively).
- (void)ingestVolatileStateFromPreferences:(_OCFXPreferences *)preferences {
    if (preferences == self) {
        __assert_rtn(
            "-[_CFXPreferences ingestVolatileStateFromPreferences:]",
            "CFXPreferences.m",
            1353,
            "prefs != self"
        );
    }
    [self withNamedVolatileSources:^(OCFMutableDictionaryRef namedVolatileSources) {
      [preferences withNamedVolatileSources:^(OCFMutableDictionaryRef otherSources) {
        _CFPrefsDictionaryApplyBlock(otherSources, ^(const void *name, const void *source) {
          OCFPrefsSource *copy = [(OCFPrefsSource *)source copyVolatileSourceWithContainingPreferences:self];
          if (copy == nil) {
              __assert_rtn(
                  "-[_CFXPreferences ingestVolatileStateFromPreferences:]_block_invoke_3",
                  "CFXPreferences.m",
                  1358,
                  "copy != NULL"
              );
          }
          OCFDictionarySetValue(namedVolatileSources, name, copy);
          [copy release];
        });
      }];
    }];
}

// Native -[_CFXPreferences currentUserHasInvalidHomeDirectory], 0x1c02ea7f0, with its .cold.1 (0x1c0316d8c,
// the error log). Direct method. The state byte (acquire loads, release stores) caches the answer: the
// unsandboxed home directory is valid (1) unless it is NULL, empty or starts with /var/empty (2); an invalid
// one is logged as an error with the log-in-progress slot set (cleared in a @finally; region bounds inferred,
// confidence medium). The returned path is freed (also when NULL).
- (BOOL)currentUserHasInvalidHomeDirectory {
    if (__atomic_load_n(&_userHomeDirectoryState, __ATOMIC_ACQUIRE) == 0) {
        char *home = (char *)_CFUnsandboxedHomeDirectoryForCurrentUser();
        const char *reason = NULL;
        if (home == NULL) {
            __atomic_store_n(&_userHomeDirectoryState, 2, __ATOMIC_RELEASE);
            reason = "is null";
        } else if (*home != 0 && strncmp(home, "/var/empty", 10) != 0) {
            __atomic_store_n(&_userHomeDirectoryState, 1, __ATOMIC_RELEASE);
        } else {
            __atomic_store_n(&_userHomeDirectoryState, 2, __ATOMIC_RELEASE);
            reason = *home != 0 ? "starts with /var/empty" : "is empty";
        }
        if (reason != NULL) {
            _OCFSetTSD(kLogInProgressSlot, (void *)kOCFBooleanTrue, NULL);
            @try {
                os_log_error(
                    _CFPrefsClientLog(),
                    "All kCFPreferencesCurrentUser domains in this process will be volatile, because "
                    "homeDirPath "
                    "%{public}s",
                    reason
                );
            } @finally {
                _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
            }
        }
        free(home);
    }
    return __atomic_load_n(&_userHomeDirectoryState, __ATOMIC_ACQUIRE) == 2;
}

// Native -[_CFXPreferences init], 0x1c02eaa9c. Listed method.
// GOT 0x22241b968 is XPC_TYPE_ARRAY (confidence: medium, see OCFPrefsDaemon.m).
- (instancetype)init {
    self = [super init];
    if (self == nil) {
        return nil;
    }
    _sources = OCFDictionaryCreateMutable(
        kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    _sourcesLock = OS_UNFAIR_LOCK_INIT;
    _namedVolatileSources = OCFDictionaryCreateMutable(
        kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    _namedVolatileSourcesLock = OS_UNFAIR_LOCK_INIT;
    _searchLists = OCFDictionaryCreateMutable(
        kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    _searchListsLock = OS_UNFAIR_LOCK_INIT;
    _launchdUID = UINT32_MAX;
    _euid = _OCFGetEUID();
    xpc_object_t groups = xpc_copy_entitlement_for_self("com.apple.security.application-groups");
    size_t count = groups != NULL && xpc_get_type(groups) == XPC_TYPE_ARRAY ? xpc_array_get_count(groups) : 0;
    if (count == 0) {
        _groupContainersForCurrentUser = NULL;
    } else {
        _groupContainersForCurrentUser = OCFSetCreateMutable(NULL, 0, &kOCFTypeSetCallBacks);
        for (size_t index = 0; index < count; ++index) {
            const char *group = xpc_array_get_string(groups, index);
            OCFStringRef name = OCFStringCreateWithCString(NULL, group, 0x08000100);
            OCFSetAddValue(_groupContainersForCurrentUser, name);
            OCFRelease(name);
        }
    }
    if (groups != NULL) {
        xpc_release(groups);
    }
    return self;
}

// Native -[_CFXPreferences copyAppValueForKey:identifier:container:configurationURL:], 0x1c02ed730.
// Listed method.
- (OCFPropertyListRef)copyAppValueForKey:(OCFStringRef)key
                              identifier:(OCFStringRef)identifier
                               container:(OCFStringRef)container
                        configurationURL:(OCFURLRef)configurationURL {
    __block OCFPropertyListRef value = NULL;
    // Native ___76-[_CFXPreferences copyAppValueForKey:identifier:container:configurationURL:]_block_invoke,
    // 0x1c02ed80c. Non-external.
    [self withSearchListForIdentifier:identifier
                            container:container
                cloudConfigurationURL:configurationURL
                              perform:^(OCFPrefsSearchListSource *searchList) {
                                value = [searchList copyValueForKey:key];
                              }];
    return value;
}

// Native -[_CFXPreferences resetUserPreferences], 0x1c02ee77c, with its blocks
// ___39-[_CFXPreferences resetUserPreferences]_block_invoke (0x1c02ee850, descriptor 0x2227a2f30),
// _block_invoke_2 (0x1c02ee8d8, descriptor 0x2227a30a0), _block_invoke_3 (0x1c02ee964, descriptor
// 0x2227ab3e0, with .cold.1 and .cold.2, 0x1c0316f60 and 0x1c0316f88: the assertion "sourceKeys &&
// sourceValues", CFXPreferences.m lines 1452 and 1435) and _block_invoke_4 (0x1c02eec08, descriptor
// 0x2227ab340), from Local/Evidence/cf-core/iphone-simulator-corpus-delta12.json. Direct method (was a
// private external). Under the search-list, named-volatile-source and source locks (nested withSearchLists:,
// withNamedVolatileSources: and withSources:): under the connection lock the user-session daemon connection
// is released and cleared; every source served by the user-session daemon (-servedByUserSessionDaemon) is
// removed from the sources, and every such named volatile source except NSRegistrationDomain and
// NSArgumentDomain from the named volatile sources (keys and values copied into malloc'd arrays of the
// dictionary's 32-bit count, sign-extended; both must be allocated). A nonempty search-list dictionary has
// its values collected into a new CFType array (default allocator, capacity the count) held in a __block
// variable, which is released after the locks; the search lists are then removed.
- (void)resetUserPreferences {
    __block OCFMutableArrayRef searchListValues = NULL;
    [self withSearchLists:^(OCFMutableDictionaryRef searchLists) {
      [self withNamedVolatileSources:^(OCFMutableDictionaryRef namedVolatileSources) {
        [self withSources:^(OCFMutableDictionaryRef sources) {
          os_unfair_lock_lock(&connectionLock);
          if (self->_userSessionDaemonConnection != NULL) {
              xpc_release(self->_userSessionDaemonConnection);
          }
          self->_userSessionDaemonConnection = NULL;
          os_unfair_lock_unlock(&connectionLock);
          int count = (int)OCFDictionaryGetCount(sources);
          const void **sourceKeys = malloc_type_malloc(8 * (size_t)(int64_t)count, 0x6004044C4A2DF);
          const void **sourceValues = malloc_type_malloc(8 * (size_t)(int64_t)count, 0x80040B8603338);
          if (sourceKeys == NULL || sourceValues == NULL) {
              __assert_rtn(
                  "-[_CFXPreferences resetUserPreferences]_block_invoke_3",
                  "CFXPreferences.m",
                  1435,
                  "sourceKeys && sourceValues"
              );
          }
          OCFDictionaryGetKeysAndValues(sources, sourceKeys, sourceValues);
          for (int index = 0; index < count; ++index) {
              if ([(OCFPrefsPlistSource *)sourceValues[index] servedByUserSessionDaemon]) {
                  OCFDictionaryRemoveValue(sources, sourceKeys[index]);
              }
          }
          free(sourceKeys);
          free(sourceValues);
          if (namedVolatileSources != NULL) {
              int volatileCount = (int)OCFDictionaryGetCount(namedVolatileSources);
              const void **volatileKeys =
                  malloc_type_malloc(8 * (size_t)(int64_t)volatileCount, 0x6004044C4A2DF);
              const void **volatileValues =
                  malloc_type_malloc(8 * (size_t)(int64_t)volatileCount, 0x80040B8603338);
              if (volatileKeys == NULL || volatileValues == NULL) {
                  __assert_rtn(
                      "-[_CFXPreferences resetUserPreferences]_block_invoke_3",
                      "CFXPreferences.m",
                      1452,
                      "sourceKeys && sourceValues"
                  );
              }
              OCFDictionaryGetKeysAndValues(namedVolatileSources, volatileKeys, volatileValues);
              for (int index = 0; index < volatileCount; ++index) {
                  OCFStringRef key = volatileKeys[index];
                  if ([(OCFPrefsPlistSource *)volatileValues[index] servedByUserSessionDaemon] &&
                      !OCFEqual(key, OF_STR("NSRegistrationDomain")) &&
                      !OCFEqual(key, OF_STR("NSArgumentDomain"))) {
                      OCFDictionaryRemoveValue(namedVolatileSources, volatileKeys[index]);
                  }
              }
              free(volatileKeys);
              free(volatileValues);
          }
          if (searchLists != NULL) {
              OCFIndex searchListCount = OCFDictionaryGetCount(searchLists);
              if (searchListCount != 0) {
                  searchListValues = OCFArrayCreateMutable(NULL, searchListCount, &kOCFTypeArrayCallBacks);
                  OCFDictionaryApplyFunction(
                      searchLists, applyBlockApplier, (void *)^(const void *key, const void *value) {
                        (void)key;
                        OCFArrayAppendValue(searchListValues, value);
                      }
                  );
              }
              OCFDictionaryRemoveAllValues(searchLists);
          }
        }];
      }];
    }];
    if (searchListValues != NULL) {
        OCFRelease(searchListValues);
    }
}

// Native -[_CFXPreferences _setupNewDaemonConnection:invalidationHandler:], 0x1c02eec5c, with
// ___65-[_CFXPreferences _setupNewDaemonConnection:invalidationHandler:]_block_invoke (0x1c02eed54,
// descriptor 0x2227ab450: the invalidation handler as a block and the preferences weakly), its .cold.1
// (0x1c0316ff8, the dispatch_once of the observation queue), _block_invoke.98 (0x1c02eef98, the global block
// ___block_literal_global_49 that creates the queue), _block_invoke_2 (0x1c02eefcc, descriptor 0x2227ab428:
// the event as an object and the preferences weakly) and _block_invoke.105 (0x1c02ef1fc, descriptor
// 0x2227a3470: the pending notifications as an object). Direct method; the preferences are captured weakly
// (objc_initWeak, objc_copyWeak, objc_loadWeak), which this manual-reference-counting unit compiles with
// -fobjc-weak (CLANG_ENABLE_OBJC_WEAK). The connection's event handler answers a dictionary event with a
// reply on its remote connection (an event that has none is logged at info level with its description, the
// log-in-progress slot set and cleared in a @finally; region bounds inferred, confidence medium) and then, on
// the "CFPreferences Remote KVO Callback Queue" (serial, autorelease per work item; created once, statics in
// __DATA,__bss named __CFPNLPO_block_invoke.onceToken and .observationCallBackQueue), logs the change
// (debug), arms the pending-KVO slot, updates the search lists for the event's domain when the preferences
// still exist, and moves pending notifications (anything but the armed marker) to the main run loop (common
// modes), waking it; the pending array is released after scheduling, the block keeping its own reference. An
// invalid connection runs the invalidation handler when there is one. The connection is activated.
- (void)_setupNewDaemonConnection:(xpc_connection_t)connection
              invalidationHandler:(void (^)(void))invalidationHandler {
    __weak _OCFXPreferences *weakSelf = self;
    xpc_connection_set_event_handler(connection, ^(xpc_object_t event) {
      if (xpc_get_type(event) == XPC_TYPE_DICTIONARY) {
          static dispatch_once_t onceToken;
          static dispatch_queue_t observationCallBackQueue;
          xpc_connection_t remote = xpc_dictionary_get_remote_connection(event);
          xpc_object_t reply = xpc_dictionary_create_reply(event);
          if (reply != NULL) {
              xpc_connection_send_message(remote, reply);
              xpc_release(reply);
          } else {
              char *description = xpc_copy_description(event);
              _OCFSetTSD(kLogInProgressSlot, (void *)kOCFBooleanTrue, NULL);
              @try {
                  os_log_info(
                      _CFPrefsClientLog(), "CFPrefs client received non-reply message: %s", description
                  );
              } @finally {
                  _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
              }
              free(description);
          }
          dispatch_once(&onceToken, ^{
            observationCallBackQueue = dispatch_queue_create(
                "CFPreferences Remote KVO Callback Queue",
                dispatch_queue_attr_make_with_autorelease_frequency(
                    NULL, DISPATCH_AUTORELEASE_FREQUENCY_WORK_ITEM
                )
            );
          });
          dispatch_async(observationCallBackQueue, ^{
            if (os_log_type_enabled(_CFPrefsClientLog(), OS_LOG_TYPE_DEBUG)) {
                int64_t pid = xpc_dictionary_get_int64(event, "CFPreferencesPID");
                const char *domain = xpc_dictionary_get_string(event, "CFPreferencesDomain");
                const char *path = xpc_dictionary_get_string(event, "CFPreferencesUncanonicalizedPath");
                bool byHost = xpc_dictionary_get_bool(event, "CFPreferencesIsByHost");
                const char *cloudPath = xpc_dictionary_get_string(event, "CFPreferencesCloudConfig");
                const char *user = xpc_dictionary_get_string(event, "CFPreferencesUser");
                os_log_debug(
                    _CFPrefsClientLog(),
                    "Received remote change notification from process %lld for { domain: %s, user: %s, "
                    "byHost: "
                    "%d, path: %s, cloud configuration path: %s }",
                    pid,
                    domain,
                    user,
                    byHost,
                    path,
                    cloudPath
                );
            }
            _CFPrefsArmPendingKVOSlot();
            OCFTypeRef identifier =
                _OCFXPCCreateCFObjectFromXPCObject(xpc_dictionary_get_value(event, "CFPreferencesDomain"));
            if (identifier != NULL) {
                if (weakSelf != nil) {
                    [weakSelf updateSearchListsForIdentifier:identifier];
                }
                OCFRelease(identifier);
            }
            id pending = (id)_OCFGetTSD(kPendingKVOSlot);
            if (pending != nil) {
                _OCFSetTSD(kPendingKVOSlot, NULL, NULL);
                if (pending != (id)kArmedMarker) {
                    OCFRunLoopPerformBlock(OCFRunLoopGetMain(), kOCFRunLoopCommonModes, ^{
                      _CFPrefsDeliverPendingKVONotificationsGuts((OCFArrayRef)pending);
                    });
                    OCFRunLoopWakeUp(OCFRunLoopGetMain());
                    OCFRelease(pending);
                }
            }
          });
      } else if (event == XPC_ERROR_CONNECTION_INVALID) {
          if (invalidationHandler != NULL) {
              invalidationHandler();
          }
      }
    });
    xpc_connection_activate(connection);
}

// Native -[_CFXPreferences appSynchronizeWithIdentifier:container:], 0x1c02ef270, with
// ___58-[_CFXPreferences appSynchronizeWithIdentifier:container:]_block_invoke (0x1c02ef350). Direct method.
// The search list's -synchronize result (true when no search list answers).
- (BOOL)appSynchronizeWithIdentifier:(OCFStringRef)identifier container:(OCFStringRef)container {
    __block BOOL synchronized = YES;
    [self withSearchListForIdentifier:identifier
                            container:container
                cloudConfigurationURL:NULL
                              perform:^(OCFPrefsSearchListSource *searchList) {
                                synchronized = [searchList synchronize];
                              }];
    return synchronized;
}

// Native -[_CFXPreferences euid], 0x1c0315ba0. Direct method.
- (uint32_t)euid {
    return _euid;
}

// Native -[_CFXPreferences withConnectionForRole:andUserIdentifier:performBlock:], 0x1c0315cd0. Direct
// method. The block receives the connection for the role (set up if necessary, +1, possibly NULL), which is
// released afterwards.
- (void)withConnectionForRole:(int)role
            andUserIdentifier:(OCFStringRef)userIdentifier
                 performBlock:(void (^)(xpc_connection_t connection))block {
    xpc_connection_t connection = [self _copyDaemonConnectionSettingUpIfNecessaryForRole:role
                                                                       andUserIdentifier:userIdentifier];
    block(connection);
    if (connection != NULL) {
        xpc_release(connection);
    }
}

// Native -[_CFXPreferences nonLaunchPersonaUID], 0x1c0315bac. Direct method (was a private external).
- (uint32_t)nonLaunchPersonaUID {
    return _nonLaunchPersonaUID;
}

// Native -[_CFXPreferences flushManagedSources], 0x1c0315bb8. Direct method.
// The os_activity parent at GOT 0x22241b4a8 is OS_ACTIVITY_CURRENT (binding confidence: medium).
// Folds the compiler-outlined pieces OUTLINED_FUNCTION_3_18 (0x1c02f237c), OUTLINED_FUNCTION_14_4
// (0x1c02f2454).
- (void)flushManagedSources {
    uint64_t start = mach_continuous_time();
    // Native ___38-[_CFXPreferences flushManagedSources]_block_invoke, 0x1c02eae58. Non-external.
    [self withConnectionForRole:1
              andUserIdentifier:NULL
                   performBlock:^(xpc_connection_t connection) {
                     if (connection != NULL) {
                         os_activity_t activity = os_activity_create(
                             "Notifying CFPrefsD Of Updated Managed Preferences",
                             OS_ACTIVITY_CURRENT,
                             OS_ACTIVITY_FLAG_DEFAULT
                         );
                         struct os_activity_scope_state_s state = {0, 0};
                         os_activity_scope_enter(activity, &state);
                         xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
                         xpc_dictionary_set_int64(message, "CFPreferencesOperation", 3);
                         // This byref variable emits ___Block_byref_object_copy__8 (0x1c02eafd0)
                         // and ___Block_byref_object_dispose__8 (0x1c02eafe0), with flags 131.
                         __block xpc_object_t reply = NULL;
                         // Native ___38-[_CFXPreferences flushManagedSources]_block_invoke.61, 0x1c02eafec.
                         // Non-external.
                         CFPREFERENCES_IS_WAITING_FOR_SYSTEM_CFPREFSD(self, ^{
                           reply = xpc_connection_send_message_with_reply_sync(connection, message);
                         });
                         xpc_release(message);
                         if (reply != NULL) {
                             xpc_release(reply);
                         }
                         [activity release];
                         os_activity_scope_leave(&state);
                     }
                   }];
    uint64_t end = mach_continuous_time();
    os_log_t log = _CFPrefsSignpostsLogForDuration(end - start);
    os_signpost_id_t signpost = os_signpost_id_make_with_pointer(log, self);
    os_signpost_event_emit(
        log,
        signpost,
        "flushManagedSources",
        "%{public, signpost.description:begin_time}llu %{public, signpost.description:end_time}llu",
        start,
        end
    );
    _CFPrefsResetManagedPreferencesStateCache();
}

// Native -[_CFXPreferences _deliverPendingKVONotifications], 0x1c0315ed0. Direct method.
// None of the three preference locks may be held. A pending array in the KVO slot (not the armed marker) is
// taken, the slot cleared, delivered and released; the marker alone just clears the slot.
- (void)_deliverPendingKVONotifications {
    os_unfair_lock_assert_not_owner(&_sourcesLock);
    os_unfair_lock_assert_not_owner(&_searchListsLock);
    os_unfair_lock_assert_not_owner(&_namedVolatileSourcesLock);
    void *pending = _OCFGetTSD(kPendingKVOSlot);
    if (pending == NULL) {
        return;
    }
    _OCFSetTSD(kPendingKVOSlot, NULL, NULL);
    if (pending == kArmedMarker) {
        return;
    }
    _CFPrefsDeliverPendingKVONotificationsGuts(pending);
    OCFRelease(pending);
}

// Native -[_CFXPreferences shmemForRole:name:], 0x1c03161ac (with OUTLINED_FUNCTION_3_18, _2_36, _15_4 and
// _26_3), with ___37-[_CFXPreferences shmemForRole:name:]_block_invoke (0x1c02eec1c, descriptor
// ___block_descriptor_40_e5_v8?0l) and its .cold.1 (0x1c0316fb0, the fault log, with OUTLINED_FUNCTION_14_4).
// The shared memory lock and pages of __MergedGlobals (0x22e6d3d78, 0x22e6d3d80 and 0x22e6d3d88), which
// -destroyConnections clears too.
static os_unfair_lock shmemLock = OS_UNFAIR_LOCK_INIT;
static void *systemShmem;
static void *userShmem;

// Direct method. The direct role (3), or direct mode, answers the in-process daemon's page. Otherwise a role
// whose mapping failed before answers NULL; under the shmem lock the role's page (the system page for role 2,
// else the user page) is answered when mapped, or the named shared memory is opened read-only (O_RDONLY, mode
// 0) and 16 KiB of it mapped shared read-only; a failed map marks the role failed. The page (or NULL) is
// stored and the lock released; a missing page logs a fault once (with the name). The statics are
// __MergedGlobals 0x22e6d3d70 (system failed), 0x22e6d3d71 (user failed), 0x22e6d3d78 (lock), 0x22e6d3d80 and
// 0x22e6d3d88 (pages) and 0x22e6d3d90 (the fault's once token).
// Folds the compiler-outlined pieces OUTLINED_FUNCTION_15_4 (0x1c02f2460), OUTLINED_FUNCTION_26_3
// (0x1c02f2544).
- (uint32_t *)shmemForRole:(int)role name:(const char *)name {
    static bool systemShmemFailed;
    static bool userShmemFailed;
    static dispatch_once_t faultOnceToken;
    if (role == 3 || _OCFPrefsDirectMode()) {
        return [directCFPrefsD() shmem];
    }
    if ((role == 2 && systemShmemFailed) || (role != 2 && userShmemFailed)) {
        return NULL;
    }
    os_unfair_lock_lock(&shmemLock);
    void *page = role == 2 ? systemShmem : userShmem;
    if (page != NULL) {
        os_unfair_lock_unlock(&shmemLock);
        return page;
    }
    int fd = shm_open(name, O_RDONLY, 0);
    if (fd != -1) {
        page = mmap(NULL, 0x4000, PROT_READ, MAP_SHARED, fd, 0);
        close(fd);
        if (page == MAP_FAILED) {
            page = NULL;
            if (role == 2) {
                systemShmemFailed = true;
            } else {
                userShmemFailed = true;
            }
        }
    }
    if (role == 2) {
        systemShmem = page;
    } else {
        userShmem = page;
    }
    os_unfair_lock_unlock(&shmemLock);
    if (page == NULL) {
        dispatch_once(&faultOnceToken, ^{
          os_log_fault(_CFPrefsClientLog(), "Couldn't map preferences shmem named %{public}s", name);
        });
    }
    return page;
}

// Native -[_CFXPreferences _copyDaemonConnectionSettingUpIfNecessaryForRole:andUserIdentifier:], 0x1c03164ec
// (with OUTLINED_FUNCTION_3_18, _21_4, _13_7, _2_36, _15_4, _27_3 and _14_4), with
// ___86-[_CFXPreferences _copyDaemonConnectionSettingUpIfNecessaryForRole:andUserIdentifier:]_block_invoke
// (0x1c02ef26c, the global block ___block_literal_global.106, a tail call of
// _CFPrefsResetUserSessionPreferences). Direct method; the user identifier is unused. Under the connection
// lock
// (__MergedGlobals 0x22e6d3d74), direct mode (asked first) or the direct role (3) uses the direct connection,
// created from an endpoint of the in-process daemon's listener. Otherwise the system (role 2) or user session
// connection is created on first use for the cfprefsd daemon service (".system" for role 2, ".test" appended
// when the process is not restricted and __CFPreferencesTestDaemon is set). For role 1 with user sessions
// enabled and no session UID, the connection targets the foreground user session and resets the user-session
// preferences when invalidated; a failed lookup or no active session (KAUTH_UID_NONE) logs an error and
// answers NULL. A new connection is set up with its invalidation handler. The connection is answered
// retained.
// Folds the compiler-outlined pieces OUTLINED_FUNCTION_13_7 (0x1c02f2448), OUTLINED_FUNCTION_21_4
// (0x1c02f24dc), OUTLINED_FUNCTION_27_3 (0x1c02f2550).
- (xpc_connection_t)_copyDaemonConnectionSettingUpIfNecessaryForRole:(int)role
                                                   andUserIdentifier:(OCFStringRef)userIdentifier {
    (void)userIdentifier;
    os_unfair_lock_lock(&connectionLock);
    xpc_connection_t *slot;
    if (_OCFPrefsDirectMode() || role == 3) {
        slot = &_directConnection;
        if (*slot == NULL) {
            xpc_endpoint_t endpoint = xpc_endpoint_create([directCFPrefsD() listener]);
            *slot = xpc_connection_create_from_endpoint(endpoint);
            xpc_release(endpoint);
            [self _setupNewDaemonConnection:*slot invalidationHandler:NULL];
        }
    } else {
        slot = role == 2 ? &_systemSessionDaemonConnection : &_userSessionDaemonConnection;
        if (*slot == NULL) {
            void (^invalidationHandler)(void) = NULL;
            uid_t targetUID = kUIDNone;
            if (role == 1 && xpc_user_sessions_enabled() && xpc_user_sessions_get_session_uid() == 0) {
                int error = 0;
                uid_t foregroundUID = xpc_user_sessions_get_foreground_uid(&error);
                if (error != 0) {
                    os_log_error(
                        _CFPrefsClientLog(),
                        "xpc_user_sessions_get_foreground_uid() failed with error %d - %s",
                        error,
                        xpc_strerror(error)
                    );
                    os_unfair_lock_unlock(&connectionLock);
                    return NULL;
                }
                if (foregroundUID == kUIDNone) {
                    os_log_error(
                        _CFPrefsClientLog(),
                        "Cant' target daemon in foreground user session because no user session is active."
                    );
                    os_unfair_lock_unlock(&connectionLock);
                    return NULL;
                }
                targetUID = foregroundUID;
                invalidationHandler = ^{
                  _CFPrefsResetUserSessionPreferences();
                };
            }
            const char *userService = "com.apple.cfprefsd.daemon";
            const char *systemService = "com.apple.cfprefsd.daemon.system";
            if (!__CFProcessIsRestricted() && getenv("__CFPreferencesTestDaemon") != NULL) {
                systemService = "com.apple.cfprefsd.daemon.system.test";
                userService = "com.apple.cfprefsd.daemon.test";
            }
            *slot = createMachServiceConnection(role == 2 ? systemService : userService, NULL, 0);
            if (targetUID != kUIDNone) {
                xpc_connection_set_target_user_session_uid(*slot, targetUID);
            }
            [self _setupNewDaemonConnection:*slot invalidationHandler:invalidationHandler];
        }
    }
    xpc_connection_t connection = *slot;
    xpc_retain(connection);
    os_unfair_lock_unlock(&connectionLock);
    return connection;
}

// Native -[_CFXPreferences destroyConnections], 0x1c0316138. Direct (not in the method list; it checks
// for nil itself). Under the shared memory lock the system, user and direct connections are released and
// cleared and both pages forgotten (not unmapped). Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device divergence: unread).
- (void)destroyConnections {
    os_unfair_lock_lock(&shmemLock);
    if (_systemSessionDaemonConnection != NULL) {
        xpc_release(_systemSessionDaemonConnection);
    }
    _systemSessionDaemonConnection = NULL;
    if (_userSessionDaemonConnection != NULL) {
        xpc_release(_userSessionDaemonConnection);
    }
    _userSessionDaemonConnection = NULL;
    systemShmem = NULL;
    userShmem = NULL;
    if (_directConnection != NULL) {
        xpc_release(_directConnection);
    }
    _directConnection = NULL;
    os_unfair_lock_unlock(&shmemLock);
}

// Native -[_CFXPreferences dealloc], 0x1c03163a8. Reached when the runtime deallocates (round-3 scope). The
// connections are destroyed, then the sources, named volatile sources, search lists and group containers are
// released. Reading: Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device
// divergence: unread).
- (void)dealloc {
    [self destroyConnections];
    if (_sources != NULL) {
        OCFRelease(_sources);
    }
    if (_namedVolatileSources != NULL) {
        OCFRelease(_namedVolatileSources);
    }
    if (_searchLists != NULL) {
        OCFRelease(_searchLists);
    }
    if (_groupContainersForCurrentUser != NULL) {
        OCFRelease(_groupContainersForCurrentUser);
    }
    [super dealloc];
}

@end
