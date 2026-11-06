//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Preferences/OCFPreferences.h"
#include "CoreFoundation/Preferences/OCFPrefsDaemon.h"

#include <os/lock.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <xpc/xpc.h>

// Native _CFPrefsApplyChangesQueueToDictionary, 0x1c02deb88 (Preferences/OCFPDSource.m). Hidden (was a
// private external). Applies a change queue (an XPC array of key/value pairs) to a mutable dictionary.
OF_EXTERN void _CFPrefsApplyChangesQueueToDictionary(xpc_object_t queue, OCFMutableDictionaryRef dictionary);

// Native _CFPrefsDCopyCurrentUserName, 0x1c02dc918, and cfprefsdEuid, 0x1c02dc940
// (Preferences/OCFPDSource.m). Hidden (were private externals). The current user name ("mobile", not
// retained) and the effective UID the daemon acts for.
OF_EXTERN OCFStringRef _CFPrefsDCopyCurrentUserName(void);
OF_EXTERN uid_t cfprefsdEuid(void);

// C2 supplies the FSEvents soft-link initializer and deletion callback. The native slots
// are shared with the C1 watcher recreation at 0x1c02e3df4.
typedef struct FSEventStream *PreferenceEventStreamRef;
typedef void (*PreferenceEventStreamCallback)(
    const struct FSEventStream *stream,
    void *context,
    size_t eventCount,
    void *eventPaths,
    const uint32_t *eventFlags,
    const uint64_t *eventIDs
);
typedef PreferenceEventStreamRef PreferenceEventStreamCreateFunction(
    OCFAllocatorRef allocator,
    PreferenceEventStreamCallback callback,
    const void *context,
    OCFArrayRef paths,
    uint64_t since,
    double latency,
    uint32_t flags
);
typedef void PreferenceEventStreamOperation(PreferenceEventStreamRef stream);
typedef void PreferenceEventStreamSetQueueFunction(PreferenceEventStreamRef stream, dispatch_queue_t queue);
typedef Boolean PreferenceEventStreamStartFunction(PreferenceEventStreamRef stream);
typedef uint64_t PreferenceEventStreamLatestIDFunction(PreferenceEventStreamRef stream);

OF_EXTERN PreferenceEventStreamCreateFunction *fsEventStreamCreate;
OF_EXTERN PreferenceEventStreamOperation *fsEventStreamInvalidate;
OF_EXTERN PreferenceEventStreamOperation *fsEventStreamRelease;
OF_EXTERN PreferenceEventStreamSetQueueFunction *fsEventStreamSetDispatchQueue;
OF_EXTERN PreferenceEventStreamStartFunction *fsEventStreamStart;
OF_EXTERN PreferenceEventStreamOperation *fsEventStreamStop;
OF_EXTERN PreferenceEventStreamLatestIDFunction *fsEventStreamGetLatestEventId;
OF_EXTERN void deletionStreamCallback(
    const struct FSEventStream *stream,
    void *context,
    size_t eventCount,
    void *eventPaths,
    const uint32_t *eventFlags,
    const uint64_t *eventIDs
);
OF_EXTERN PreferenceEventStreamRef _onqueue_locked_CFPrefsRecreateFileWatcher(void);
OF_EXTERN os_unfair_lock dirWatcherLock;
OF_EXTERN OCFMutableDictionaryRef dirWatcherTargets;
OF_EXTERN OCFMutableArrayRef watchedDirs;
OF_EXTERN dispatch_queue_t fseventsQueue;

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class CFPDSource (__DATA,__objc_data 0x22e6cee40, was a private external), an NSObject subclass;
// class_ro_t 0x2227c74d8: instance size 144 (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified). Ivar names from the image's _OBJC_IVAR_$_CFPDSource.* symbols (nm), offsets and
// sizes from the class_ro_t: _cfprefsd 8, _plist 16, _pendingChangesQueue 24, _pendingChangesSize 32,
// _userName 40, _domain 48, _actualPath 56, _fileName 64, _observingConnections 72, _dirtyTransaction 80,
// _uncanonicalizedPathCache 88 (8 bytes each), _uncanonicalizedPathCacheLock 96, _observingConnectionsLock
// 100, _writeLock 104, _lock 108, _plistFileOwner 112, _lastEuid 116, _lastEgid 120, _fileProtectionClass
// 124, _parentFD 128, _cachedPlistFD 132 (4 bytes each), _generationShmemIndex 136 (2 bytes), _byHost 138,
// _managed 139, _managedUsesContainer 140, _watchingParentDirectory 141 (1 byte each) and the one-bit fields
// at 142 (_handlingRequest, _dirty, _neverCache, _checkedForNonPrefsPlist, _restrictedReadability,
// _waitingForDeviceUnlock, _disableBackup, _plistFileOwnerReadable) and 143 (_plistFileWorldReadable,
// _actualPathRedirectedByTerminalSymlink), in that bit order. The type encodings lie outside the extracted
// image; the types are the ones the restored code establishes (the daemon, the plist buffer object
// (CFPDDataBuffer), the change queue (an XPC array), its byte count, the interned user and domain, two
// malloc'd paths, a CFMutableSet of connections, an os_transaction, the cached CFString path, four locks, the
// owner, UIDs and descriptors). The ivars are package-visible: the tombstones and the daemon read them as the
// native code does. Listed methods are the 42 of the class's method list; the others are direct methods
// (called with BL natively).
@interface OCFPDSource : NSObject {
    @package
    OCFPrefsDaemon *_cfprefsd;
    id _plist;
    xpc_object_t _pendingChangesQueue;
    size_t _pendingChangesSize;
    OCFStringRef _userName;
    OCFStringRef _domain;
    char *_actualPath;
    char *_fileName;
    OCFMutableSetRef _observingConnections;
    id _dirtyTransaction;
    OCFStringRef _uncanonicalizedPathCache;
    os_unfair_lock _uncanonicalizedPathCacheLock;
    os_unfair_lock _observingConnectionsLock;
    os_unfair_lock _writeLock;
    os_unfair_lock _lock;
    uid_t _plistFileOwner;
    uid_t _lastEuid;
    gid_t _lastEgid;
    int _fileProtectionClass;
    int _parentFD;
    int _cachedPlistFD;
    int16_t _generationShmemIndex;
    BOOL _byHost;
    BOOL _managed;
    BOOL _managedUsesContainer;
    BOOL _watchingParentDirectory;
    unsigned char _handlingRequest : 1;
    unsigned char _dirty : 1;
    unsigned char _neverCache : 1;
    unsigned char _checkedForNonPrefsPlist : 1;
    unsigned char _restrictedReadability : 1;
    unsigned char _waitingForDeviceUnlock : 1;
    unsigned char _disableBackup : 1;
    unsigned char _plistFileOwnerReadable : 1;
    unsigned char _plistFileWorldReadable : 1;
    unsigned char _actualPathRedirectedByTerminalSymlink : 1;
}

- (void)beginHandlingRequest __attribute__((objc_direct));

- (void)endHandlingRequest __attribute__((objc_direct));

- (int)cacheFileInfo;

- (int)cacheFileInfoForWriting:(BOOL)forWriting euid:(uid_t)euid egid:(gid_t)egid didCreate:(BOOL *)didCreate;

- (OCFStringRef)copyUncanonicalizedPath;

- (void)setUncanonicalizedPathCached:(BOOL)cached;

- (instancetype)initWithDomain:(OCFStringRef)domain
                      userName:(OCFStringRef)userName
                        byHost:(BOOL)byHost
                       managed:(BOOL)managed
                    shmemIndex:(int16_t)shmemIndex
                        daemon:(OCFPrefsDaemon *)daemon;

- (BOOL)managed;

- (void)setManagedPreferencesUseContainer:(BOOL)useContainer;

- (OCFStringRef)domain;

- (OCFStringRef)user;

- (OCFStringRef)container;

- (OCFStringRef)cloudConfigurationPath;

- (void)respondToFileWrittenToBehindOurBack;

- (BOOL)byHost;

- (int)shmemIndex;

- (void)updateShmemEntry;

- (void)setDirty:(BOOL)dirty;

- (OCFDictionaryRef (^)(void))createDiskWrite;

- (void)handleWritingResult:(OCFDictionaryRef)result;

- (void)setPlist:(id)plist owner:(uid_t)owner mode:(mode_t)mode __attribute__((objc_direct));

- (void)syncWriteToDiskAndFlushCacheForReason:(OCFStringRef)reason;

- (void)syncWriteToDisk;

- (id)copyPropertyListWithoutDrainingPendingChangesValidatingPlist:(BOOL)validate
                                                  andReturnFileUID:(uid_t *)fileUID
                                                           andMode:(mode_t *)mode;

- (id)copyPropertyListValidatingPlist:(BOOL)validate;

- (void)asyncNotifyObserversOfWriteFromConnection:(xpc_connection_t)connection message:(xpc_object_t)message;

- (void)drainPendingChanges;

- (BOOL)enqueueNewKey:(xpc_object_t)key
                value:(xpc_object_t)value
             encoding:(int)encoding
              inBatch:(BOOL)inBatch
          fromMessage:(xpc_object_t)message;

- (OCFStringRef)debugDump;

- (void)lock;

- (void)unlock;

- (void)lockedAsync:(void (^)(void))block;

- (void)lockedSync:(void (^)(void))block;

- (int)validateSandboxForWrite:(xpc_object_t)message
                 containerPath:(const char *)containerPath __attribute__((objc_direct));

- (int)validateSandboxForRead:(xpc_object_t)message
                containerPath:(const char *)containerPath __attribute__((objc_direct));

- (int)validatePOSIXPermissionsForMessage:(xpc_object_t)message
                               accessType:(int)accessType
                                  fileUID:(uid_t)fileUID
                                     mode:(mode_t)mode
                           fullyValidated:(BOOL *)fullyValidated __attribute__((objc_direct));

- (int)validateMessage:(xpc_object_t)message
                withNewKey:(xpc_object_t)key
                  newValue:(xpc_object_t)value
    plistIsAvailableToRead:(BOOL)plistIsAvailableToRead
             containerPath:(const char *)containerPath
                   fileUID:(uid_t)fileUID
                      mode:(mode_t)mode
         diagnosticMessage:(const char **)diagnosticMessage;

- (void)setObserved:(BOOL)observed bySenderOfMessage:(xpc_object_t)message __attribute__((objc_direct));

- (void)acceptMessage:(xpc_object_t)message populatingReply:(xpc_object_t)reply;

- (void)processEndOfMessageIntendingToRemoveSource:(BOOL *)remove replacingWithTombstone:(id *)tombstone;

- (void)cleanUpAfterAcceptingMessage:(xpc_object_t)message;

- (BOOL)shouldBePurgable __attribute__((objc_direct));

- (void)tryEndAccessingPlist __attribute__((objc_direct));

- (void)clearPlist __attribute__((objc_direct));

- (int)openActualPath __attribute__((objc_direct));

- (bool)getUncanonicalizedPath:(char *)path __attribute__((objc_direct));

- (OCFStringRef)_copyUncanonicalizedPath __attribute__((objc_direct));

- (bool)_getUncanonicalizedPath:(char *)path __attribute__((objc_direct));

- (id)description;

- (id)redactedDescription;

- (BOOL)shouldStayDirtyAfterOpenForWritingFailureWithErrno:(int)error __attribute__((objc_direct));

- (void)cleanUpIfNecessaryAfterCreatingPlist __attribute__((objc_direct));

- (void)finishedNonRequestWriteWithResult:(OCFDictionaryRef)result;

- (int)openPropertyListWithoutDrainingPendingChangesOrValidatingPlistAndReturnFileUID:(uid_t *)fileUID
                                                                              andMode:(mode_t *)mode
    __attribute__((objc_direct));

- (BOOL)hasObservers __attribute__((objc_direct));

- (void)observingConnectionsLockedSync:(void (^)(void))block __attribute__((objc_direct));

- (void)observingConnectionWasInvalidated:(xpc_connection_t)connection;

- (void)stopNotifyingObserver:(xpc_connection_t)connection __attribute__((objc_direct));

- (void)clearCacheForReason:(OCFStringRef)reason;

- (int)validateAccessToken:(int)token accessType:(int)accessType __attribute__((objc_direct));

- (BOOL)_canTrustUserPropertyForPOSIXPermissionCheckForAccessType:(int)accessType
    __attribute__((objc_direct));

- (int)validateSandboxPermissionsForMessage:(xpc_object_t)message
                              containerPath:(const char *)containerPath
                                 accessType:(int)accessType __attribute__((objc_direct));

- (void)attachSizeWarningsToReply:(xpc_object_t)reply
                     forByteCount:(size_t)byteCount __attribute__((objc_direct));

- (size_t)approximatePlistSizeIncludingPendingChanges __attribute__((objc_direct));

- (BOOL)isEmpty __attribute__((objc_direct));

- (void)closeFileDescriptors __attribute__((objc_direct));

@end

// Native class CFPDContainerSource (__DATA,__objc_data 0x22e6cee90, was a private external), a CFPDSource
// subclass; class_ro_t 0x2227c7590: instance size 152, ivar _containerPath at 144 (8 bytes; the name from
// _OBJC_IVAR_$_CFPDContainerSource._containerPath, the type, a retained CFString, from the restored code).
@interface OCFPDContainerSource : OCFPDSource {
    @package
    OCFStringRef _containerPath;
}

- (instancetype)initWithDomain:(OCFStringRef)domain
                      userName:(OCFStringRef)userName
                     container:(OCFStringRef)container
                        byHost:(BOOL)byHost
                       managed:(BOOL)managed
                    shmemIndex:(int16_t)shmemIndex
                        daemon:(OCFPrefsDaemon *)daemon;

@end

// Native class CFPDMirroredSource (__DATA,__objc_data 0x22e6ceeb8, was a private external), a CFPDSource
// subclass; class_ro_t 0x2227c7800: instance size 160, ivars _mirroredKeys 144 and _mirrorSource 152 (8 bytes
// each; the names from the _OBJC_IVAR_$_CFPDMirroredSource.* symbols, the types, a CFMutableSet of key
// strings and the mirror CFPDSource, from the restored code).
@interface OCFPDMirroredSource : OCFPDSource {
    @package
    OCFMutableSetRef _mirroredKeys;
    OCFPDSource *_mirrorSource;
}

- (instancetype)initWithDomain:(OCFStringRef)domain
                      userName:(OCFStringRef)userName
                        byHost:(BOOL)byHost
                       managed:(BOOL)managed
                  mirroredKeys:(OCFArrayRef)mirroredKeys
                    shmemIndex:(int16_t)shmemIndex
                        daemon:(OCFPrefsDaemon *)daemon;

- (void)enqueueMirrorSignatureForKey:(OCFStringRef)key
                         fromMessage:(xpc_object_t)message __attribute__((objc_direct));

@end

// Native class CFPDSourceLookUpKey (__DATA,__objc_data 0x22e6cef80, was a private external), an NSObject
// subclass; class_ro_t 0x2227c78d8: instance size 24, ivars uncanonicalizedPath 8 and cloudPath 16 (8 bytes
// each; the names from the _OBJC_IVAR_$_CFPDSourceLookUpKey.* symbols, the types, a retained CFString path
// and a retained CF cloud configuration path, from the restored code). Its runtime-invoked -dealloc
// (0x1c02e36c4) is outside the captured scope.
@interface OCFPDSourceLookUpKey : NSObject {
    @package
    OCFStringRef uncanonicalizedPath;
    OCFTypeRef cloudPath;
}

@end

// Native class CFPDObserverOnlyTombstone (__DATA,__objc_data 0x22e6cee68, was a private external), an
// NSObject subclass; class_ro_t 0x2227c7728: instance size 39. Ivar names from the image's
// _OBJC_IVAR_$_CFPDObserverOnlyTombstone.* symbols, offsets and sizes from the class_ro_t: _domain 8,
// _userOrContainer 16, _observers 24 (8 bytes each), _lock 32 (4 bytes), _shmemIndex 36 (2 bytes) and the
// one-bit fields at 38 (_isManaged, _notificationInProgress, _needToNotify). The types are the ones the
// restored code establishes: the observers are a single retained XPC connection or a CFMutableSet of them.
@interface OCFPDObserverOnlyTombstone : NSObject {
    @package
    OCFStringRef _domain;
    OCFStringRef _userOrContainer;
    OCFTypeRef _observers;
    os_unfair_lock _lock;
    uint16_t _shmemIndex;
    unsigned char _isManaged : 1;
    unsigned char _notificationInProgress : 1;
    unsigned char _needToNotify : 1;
}

- (instancetype)initMatchingSource:(OCFPDSource *)source;

+ (BOOL)replaceObserved:(id)observed
           withObserved:(id)replacement
          forConnection:(xpc_connection_t)connection __attribute__((objc_direct));

- (void)alreadyLocked_transferObservingConnectionsFromSource:(OCFPDSource *)source;

- (void)_operateOnObservingConnectionsAsMultiple:(void (^)(OCFMutableSetRef observers))multiple
                                        asSingle:(void (^)(xpc_object_t *observer))single
    __attribute__((objc_direct));

- (BOOL)managed;

- (void)removeObservingConnection:(xpc_connection_t)connection daemon:(OCFPrefsDaemon *)daemon;

- (void)transferObservingConnectionsToSource:(OCFPDSource *)source;

- (void)_sendNotificationToConnection:(xpc_connection_t)connection __attribute__((objc_direct));

- (void)notifyObservers;

@end
#endif

// C1-prefs-sources owns the FSEvents slots and watcher rebuild. C2 supplies their bindings and callback.
typedef struct FSEventStream *PreferenceEventStreamRef;
typedef void (*PreferenceEventStreamCallback)(
    const struct FSEventStream *stream,
    void *context,
    size_t eventCount,
    void *eventPaths,
    const uint32_t *eventFlags,
    const uint64_t *eventIDs
);
typedef PreferenceEventStreamRef PreferenceEventStreamCreateFunction(
    OCFAllocatorRef allocator,
    PreferenceEventStreamCallback callback,
    const void *context,
    OCFArrayRef paths,
    uint64_t since,
    double latency,
    uint32_t flags
);
typedef void PreferenceEventStreamOperation(PreferenceEventStreamRef stream);
typedef void PreferenceEventStreamSetQueueFunction(PreferenceEventStreamRef stream, dispatch_queue_t queue);
typedef Boolean PreferenceEventStreamStartFunction(PreferenceEventStreamRef stream);
typedef uint64_t PreferenceEventStreamLatestIDFunction(PreferenceEventStreamRef stream);

OF_EXTERN PreferenceEventStreamCreateFunction *fsEventStreamCreate;
OF_EXTERN PreferenceEventStreamOperation *fsEventStreamInvalidate;
OF_EXTERN PreferenceEventStreamOperation *fsEventStreamRelease;
OF_EXTERN PreferenceEventStreamSetQueueFunction *fsEventStreamSetDispatchQueue;
OF_EXTERN PreferenceEventStreamStartFunction *fsEventStreamStart;
OF_EXTERN PreferenceEventStreamOperation *fsEventStreamStop;
OF_EXTERN PreferenceEventStreamLatestIDFunction *fsEventStreamGetLatestEventId;
OF_EXTERN void deletionStreamCallback(
    const struct FSEventStream *stream,
    void *context,
    size_t eventCount,
    void *eventPaths,
    const uint32_t *eventFlags,
    const uint64_t *eventIDs
);
OF_EXTERN PreferenceEventStreamRef _onqueue_locked_CFPrefsRecreateFileWatcher(void);
