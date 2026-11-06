//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Preferences/OCFPreferences.h"
#include "CoreFoundation/Preferences/OCFPrefsPendingKVONotification.h"

#include <os/lock.h>
#include <xpc/xpc.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

@class ONSException;
@class ONSUserDefaults;
@class OCFPrefsSource;
@class OCFPrefsPlistSource;
@class OCFPrefsSearchListSource;

// Native class _CFXPreferences (__DATA,__objc_data 0x22e6cf1d8), a hidden NSObject subclass with instance
// size
// 96. Ivar offsets and sizes from its class_ro_t (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified); the type encodings lie outside the extracted
// image, so the types below are the ones the restored code establishes (dictionaries, locks, user IDs, the
// daemon connections, the group-container set), or byte-size placeholders where no restored code reads the
// ivar (type unread). The ivars are public because
// -[_CFXPreferencesHandle copyPrefs] reads and writes _euid and _nonLaunchPersonaUID directly; their declared
// visibility is unread. Only the direct methods this family restores are defined; the class's other methods
// are unread.
@interface _OCFXPreferences : NSObject {
    @public
    OCFMutableDictionaryRef _sources;
    OCFMutableDictionaryRef _namedVolatileSources;
    OCFMutableDictionaryRef _searchLists;
    xpc_connection_t _systemSessionDaemonConnection;
    xpc_connection_t _userSessionDaemonConnection;
    xpc_connection_t _directConnection;
    uint32_t _launchdUID;
    uint32_t _euid;
    uint32_t _nonLaunchPersonaUID;
    uint8_t _userHomeDirectoryState;
    os_unfair_lock _sourcesLock;
    os_unfair_lock _searchListsLock;
    os_unfair_lock _namedVolatileSourcesLock;
    OCFMutableSetRef _groupContainersForCurrentUser;
}

- (void)destroyConnections __attribute__((objc_direct));

- (BOOL)appSynchronizeWithIdentifier:(OCFStringRef)identifier
                           container:(OCFStringRef)container __attribute__((objc_direct));

- (void)_deliverPendingKVONotifications __attribute__((objc_direct));

- (void)ingestVolatileStateFromPreferences:(_OCFXPreferences *)preferences __attribute__((objc_direct));

// Native -[_CFXPreferences _setupNewDaemonConnection:invalidationHandler:], 0x1c02eec5c (direct; was a
// private external).
- (void)_setupNewDaemonConnection:(xpc_connection_t)connection
              invalidationHandler:(void (^)(void))invalidationHandler __attribute__((objc_direct));

- (BOOL)currentUserHasInvalidHomeDirectory __attribute__((objc_direct));

// Native -[_CFXPreferences resetUserPreferences], 0x1c02ee77c (direct; was a private external).
- (void)resetUserPreferences __attribute__((objc_direct));

- (uint32_t)euid __attribute__((objc_direct));

- (void)withConnectionForRole:(int)role
            andUserIdentifier:(OCFStringRef)userIdentifier
                 performBlock:(void (^)(xpc_connection_t connection))block __attribute__((objc_direct));

- (uint32_t *)shmemForRole:(int)role name:(const char *)name __attribute__((objc_direct));

- (xpc_connection_t)_copyDaemonConnectionSettingUpIfNecessaryForRole:(int)role
                                                   andUserIdentifier:(OCFStringRef)userIdentifier
    __attribute__((objc_direct));

- (OCFPropertyListRef)copyAppValueForKey:(OCFStringRef)key
                              identifier:(OCFStringRef)identifier
                               container:(OCFStringRef)container
                        configurationURL:(OCFURLRef)configurationURL;

- (uint32_t)nonLaunchPersonaUID __attribute__((objc_direct));

- (void)flushManagedSources __attribute__((objc_direct));

@end

// Native _CFPrefsResetUserSessionPreferences, 0x1c02efb54 (Preferences/OCFXPreferences.m). Hidden (was a
// private external). The invalidation handler of the foreground user-session connection tail-calls it.
OF_EXTERN void _CFPrefsResetUserSessionPreferences(void);

// Native CFPreferencesIsManaged, 0x1c02f15a0 (Preferences/OCFXPreferences.m). Hidden (was a private
// external). Whether managed preferences exist for the user (kCFPreferencesAnyUser, kCFPreferencesCurrentUser
// or a user name), containerized or not.
OF_EXTERN bool CFPreferencesIsManaged(OCFStringRef user, bool containerized);

// Native _CFPNLPO (__DATA,__data 0x22e6cff68, Preferences/OCFXPreferences.m): the non-launch persona
// override. Hidden (was a private external); hidden until the owner records a naming decision.
OF_EXTERN int _CFPNLPO;

// Native CFPREFERENCES_IS_WAITING_FOR_SYSTEM_CFPREFSD (0x1c02b1918), _USER_CFPREFSD (0x1c02b1980) and
// _DIRECT_CFPREFSD (0x1c02b19e8), Preferences/OCFPrefsSearchListSource.m. Hidden (were private externals);
// hidden until the owner records a naming decision. Each asserts that none of the preferences' three locks is
// owned and runs the block: the frames name the daemon a thread waits for.
OF_EXTERN void
CFPREFERENCES_IS_WAITING_FOR_SYSTEM_CFPREFSD(_OCFXPreferences *preferences, void (^block)(void));
OF_EXTERN void CFPREFERENCES_IS_WAITING_FOR_USER_CFPREFSD(_OCFXPreferences *preferences, void (^block)(void));
OF_EXTERN void
CFPREFERENCES_IS_WAITING_FOR_DIRECT_CFPREFSD(_OCFXPreferences *preferences, void (^block)(void));

// Native CFPREFERENCES_IS_WAITING_FOR_SYSTEM_AND_USER_CFPREFSDS, 0x1c02b18b0. Hidden (was private external).
OF_EXTERN void
CFPREFERENCES_IS_WAITING_FOR_SYSTEM_AND_USER_CFPREFSDS(_OCFXPreferences *preferences, void (^block)(void));

// Native category PlistSourceAdditions (CFPrefsPlistSource.m, inferred from the block symbol of
// withManagedSourceForIdentifier:user:perform:).
@interface _OCFXPreferences (PlistSourceAdditions)

- (void)withManagedSourceForIdentifier:(OCFStringRef)identifier
                                  user:(OCFStringRef)user
                               perform:(void (^)(OCFPrefsPlistSource *source))block
    __attribute__((objc_direct));

@end

// Native class _CFXPreferencesHandle (__DATA,__objc_data 0x22e6cf1b0), a hidden NSObject subclass with
// instance size 49. Ivar names, offsets and sizes from its class_ro_t (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified); the type encodings lie outside the extracted
// image, so the types below are the ones the restored code establishes (lastCheckedVoucher holds a voucher,
// -1 meaning none checked). The class's six listed methods and its direct method are restored in
// Preferences/OCFXPreferences.m.
@interface _OCFXPreferencesHandle : NSObject {
    @protected
    _OCFXPreferences *prefs;
    _OCFXPreferences *nonLaunchPersonaPrefs;
    os_unfair_lock lock;
    uint32_t overrideEUID;
    uint32_t nonLaunchPersonaID;
    BOOL cachedUseLaunchPersona;
    void *lastCheckedVoucher;
    BOOL processCanHaveMultiplePersonas;
}

- (id)init;
- (void)dealloc;
- (void)setEUIDOverride:(uint32_t)euid;
- (void)setNonLaunchPersonaOverride:(uint32_t)personaID;
- (_OCFXPreferences *)copyPrefs;
- (void)forEachPrefsPerformBlock:(void (^)(_OCFXPreferences *preferences))block;

- (BOOL)_canUseCachedPersonaInfo __attribute__((objc_direct));

@end

// Selectors the restored code sends to receivers typed id natively (observer collections, observers and
// application records); the protocol is never adopted or referenced at run time, it only declares them.
@protocol PreferencesUnrestoredMessages

- (unsigned long)approximateCount;
- (unsigned long)borrowObjects:(id *)objects count:(unsigned long)count;
- (void)handleChangeNotificationForDomainIdentifier:(OCFStringRef)identifier isRemote:(BOOL)isRemote;
- (void)_notifyObserversOfChangeFromValuesForKeys:(OCFDictionaryRef)oldValues
                                  toValuesForKeys:(OCFDictionaryRef)newValues;
- (id)initWithBundleIdentifier:(OCFStringRef)bundleIdentifier
              allowPlaceholder:(BOOL)allowPlaceholder
                         error:(id *)error;
- (OCFURLRef)bundleContainerURL;

@end
#endif
