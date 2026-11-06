//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Collections/OCFBag.h"
#include "CoreFoundation/Preferences/OCFPreferences.h"

#include <dispatch/dispatch.h>

#include <os/lock.h>
#include <stdbool.h>
#include <stdint.h>
#include <xpc/xpc.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class CFPrefsDaemon (__DATA,__objc_data 0x22e6cf0c0), a hidden NSObject subclass with instance size
// 608. Ivar names, offsets and sizes from its class_ro_t 0x2227c7eb8 (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified): _listener 8, _listenerWorkloops 16,
// _listenerWorkloopsCount 24 (4 bytes), _secondaryWorkloop 32, _name 40, _role 48 (4 bytes), _sourceCache 56,
// _sourceCacheUsageCounts 64, _sourceCacheLock 72 (4 bytes), _deadManagedSourceBitSet 80 (512 bytes,
// 8-byte aligned), _testMode 592 (1 byte) and _shmem 600. The type encodings lie outside the extracted image,
// so the types below are the ones the restored code establishes: the bit set is read as 64 64-bit words
// (-_setSource:isDead:), the workloops as an array of dispatch workloops, the name as a C string, the cache
// as a CFMutableSet and the usage counts as a CFMutableBag (-initWithRole:testMode:). -enterSandbox
// (0x1c02e94a4, the class's only listed method) is unread. The other methods are direct methods (called
// with BL natively). The ivars are package-visible: the CFPDSource.m unit (Preferences/OCFPDSource.m) reads
// the source cache and its lock directly, as the native code does with fixed offsets.
@interface OCFPrefsDaemon : NSObject {
    @package
    xpc_connection_t _listener;
    dispatch_workloop_t *_listenerWorkloops;
    int _listenerWorkloopsCount;
    dispatch_workloop_t _secondaryWorkloop;
    const char *_name;
    int _role;
    OCFMutableSetRef _sourceCache;
    OCFMutableBagRef _sourceCacheUsageCounts;
    os_unfair_lock _sourceCacheLock;
    uint64_t _deadManagedSourceBitSet[64];
    BOOL _testMode;
    void *_shmem;
}

- (void *)shmem __attribute__((objc_direct));

- (int)role __attribute__((objc_direct));

- (void)_initializeShmemPage:(void **)page __attribute__((objc_direct));

- (void)getShmemName:(char *)name bufLen:(size_t)length __attribute__((objc_direct));

- (xpc_connection_t)listener __attribute__((objc_direct));

- (id)initWithRole:(int)role testMode:(BOOL)testMode __attribute__((objc_direct));

- (void)logDomainInconsistencyForProcess:(int)pid
                                 message:(xpc_object_t)message
                                  source:(id)source __attribute__((objc_direct));

- (void)handleMessage:(xpc_object_t)message
      originalMessage:(xpc_object_t)originalMessage
             fromPeer:(xpc_connection_t)peer
         replyHandler:(void (^)(xpc_object_t reply))replyHandler __attribute__((objc_direct));

- (void)handleSourceMessage:(xpc_object_t)message
            originalMessage:(xpc_object_t)originalMessage
               replyHandler:(void (^)(xpc_object_t reply))replyHandler __attribute__((objc_direct));

- (void)handleMultiMessage:(xpc_object_t)message
           originalMessage:(xpc_object_t)originalMessage
              replyHandler:(void (^)(xpc_object_t reply))replyHandler __attribute__((objc_direct));

- (void)_handleSubMessagesFromArray:(xpc_object_t)messages
                          withCount:(size_t)count
                           fromPeer:(xpc_connection_t)peer
                         startingAt:(size_t)index
                    originalMessage:(xpc_object_t)originalMessage
                            replies:(xpc_object_t *)replies
                       replyHandler:(void (^)(void))replyHandler __attribute__((objc_direct));

- (void)handleFlushManagedMessage:(xpc_object_t)message
                     replyHandler:(void (^)(xpc_object_t reply))replyHandler __attribute__((objc_direct));

- (void)handleAgentCheckInMessage:(xpc_object_t)message __attribute__((objc_direct));

- (void)handleFlushSourceForDomainMessage:(xpc_object_t)message
                             replyHandler:(void (^)(xpc_object_t reply))replyHandler
    __attribute__((objc_direct));

// These three factory methods return +1 objects in the captures; record that ownership for analysis.
- (dispatch_queue_t)createSecondaryQueueForClientWithPID:(int)pid
    __attribute__((objc_direct, ns_returns_retained));

- (void)updateShmemIndex:(int)index __attribute__((objc_direct));

- (void)updateShmemForDomain:(OCFStringRef)domain __attribute__((objc_direct));

- (dispatch_queue_t)createQueueForClientWithPID:(int)pid
                                            uid:(uid_t)uid __attribute__((objc_direct, ns_returns_retained));

- (void)synchronousWithSourceCache:(void (^)(OCFMutableSetRef cache, OCFMutableBagRef usageCounts))block
    __attribute__((objc_direct));

- (void)_setSource:(id)source isDead:(BOOL)isDead __attribute__((objc_direct));

- (id)_createSourceWithDomain:(OCFStringRef)domain
                         user:(OCFStringRef)user
                    container:(OCFStringRef)container
                       byHost:(BOOL)byHost
                      managed:(BOOL)managed
                   shmemIndex:(int16_t)shmemIndex __attribute__((objc_direct, ns_returns_retained));

@end

// Classes of this image the daemon units reference by class symbol: CFPDSource, CFPDObserverOnlyTombstone,
// CFPDContainerSource and CFPDMirroredSource (Preferences/OCFPDSource.h) and CFPDCloudSource (not restored on
// this branch), declared so that the prefix tool maps them.
@class OCFPDSource;
@class OCFPDObserverOnlyTombstone;
@class OCFPDContainerSource;
@class OCFPDMirroredSource;
@class OCFPDCloudSource;

// Native class _CFPrefsClientContext (__DATA,__objc_data 0x22e6cf098, was a private external), an NSObject
// subclass without methods (defined in Preferences/OCFPrefsDaemon.m); class_ro_t 0x2227c7ca0: instance size
// 55 (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Ivar names from
// the image's _OBJC_IVAR_$__CFPrefsClientContext.* symbols, offsets and sizes from the class_ro_t: _cfprefsd
// 8, _suiteCache 16, _observedSources 24, _entitlements 32, _appGroups 40 (8 bytes each), _lock 48 (4 bytes),
// _sandboxed 52 and _isPlatformBinary 53 (1 byte each) and the one-bit fields at 54 in this order. The type
// encodings lie outside the extracted image; the types are the ones the restored code establishes (as
// ClientContextLayout in Preferences/OCFPrefsDaemon.m, which reaches the same storage by offset): the daemon,
// the suite container cache, the observed sources (a CFMutableSet), the entitlements, the application groups,
// the lock, the sandboxed and platform-binary states (0xff unknown) and the flags. The ivars are
// package-visible for the CFPDSource.m unit.
@interface _OCFPrefsClientContext : NSObject {
    @package
    OCFPrefsDaemon *_cfprefsd;
    OCFMutableDictionaryRef _suiteCache;
    OCFMutableSetRef _observedSources;
    xpc_object_t _entitlements;
    OCFArrayRef _appGroups;
    os_unfair_lock _lock;
    uint8_t _sandboxed;
    uint8_t _isPlatformBinary;
    unsigned char _impersonatingAnotherProcess : 1;
    unsigned char _hasInFlightKVONotifications : 1;
    unsigned char _didDeferKVONotifications : 1;
    unsigned char _valid : 1;
    unsigned char _canImpersonate : 1;
    unsigned char _hasPopulatedEntitlements : 1;
}

@end

// Native category SourceSupport (the block symbols name it), restored in Preferences/OCFPDSource.m, the
// native unit CFPDSource.m. Direct methods.
@interface OCFPrefsDaemon (SourceSupport)

- (void)withSnapshotOfSourcesForDomainIdentifier:(OCFStringRef)domain
                                    performBlock:(void (^)(id *sources, OCFIndex count))block
    __attribute__((objc_direct));

- (void)withAllKnownManagedSources:
    (void (^)(
        id *sources, OCFIndex sourceCount, id *tombstones, OCFIndex tombstoneCount, uint64_t *deadSources
    ))block __attribute__((objc_direct));

// Native -[CFPrefsDaemon(SourceSupport)
// withSourceForDomain:inContainer:user:byHost:managed:managedUsesContainer:cloudStoreEntitlement:cloudConfigurationPath:constructReplyWithSourceLock:afterReleasingSourceLock:],
// 0x1c02dc180. The signature is the one the call site in the
// ___66-[CFPrefsDaemon handleSourceMessage:originalMessage:replyHandler:]_block_invoke_3 establishes: the
// first block answers the reply under the source's lock (the source is nil when none was found), the second
// receives the source and that reply after the lock is released.
- (void)withSourceForDomain:(OCFStringRef)domain
                     inContainer:(OCFStringRef)container
                            user:(OCFStringRef)user
                          byHost:(BOOL)byHost
                         managed:(BOOL)managed
            managedUsesContainer:(BOOL)managedUsesContainer
           cloudStoreEntitlement:(xpc_object_t)cloudStoreEntitlement
          cloudConfigurationPath:(OCFStringRef)cloudConfigurationPath
    constructReplyWithSourceLock:(xpc_object_t (^)(id source))constructReply
        afterReleasingSourceLock:(void (^)(id source, xpc_object_t reply))afterReleasing
    __attribute__((objc_direct));

+ (bool)_getUncanonicalizedSourcePath:(char *)path
                           withDomain:(OCFStringRef)domain
                                 user:(OCFStringRef)user
                               byHost:(bool)byHost
                        containerPath:(OCFStringRef)containerPath
                              managed:(bool)managed
                 managedUsesContainer:(bool)managedUsesContainer __attribute__((objc_direct));

+ (OCFStringRef)_copyUncanonicalizedSourcePathWithDomain:(OCFStringRef)domain
                                                    user:(OCFStringRef)user
                                                  byHost:(bool)byHost
                                           containerPath:(OCFStringRef)containerPath
                                                 managed:(bool)managed
                                    managedUsesContainer:(bool)managedUsesContainer
    __attribute__((objc_direct));

@end

// Native class _CFPrefsSynchronizer (__DATA,__objc_data 0x22e6cedf0, was a private external), an NSObject
// subclass; class_ro_t 0x2227c6fc0: instance size 37 (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Ivar names from the image's
// _OBJC_IVAR_$__CFPrefsSynchronizer.* symbols (nm), offsets and sizes from the class_ro_t: _synchTimer 8,
// _synchQueue 16, _dirtySources 24, _CFPrefsDirtySourcesLock 32 (4 bytes) and _active 36 (1 byte). The type
// encodings lie outside the extracted image; the types are the ones the restored methods establish (the
// interval source of -init, its serial queue, a CFMutableSet, the lock and the armed flag). The lock ivar is
// spelled _dirtySourcesLock here: the native spelling would be read as a CF name by the naming switch. Its
// methods are restored in Preferences/OCFPDSource.m; -init is the only listed method, the others are direct.
@interface _OCFPrefsSynchronizer : NSObject {
    @protected
    dispatch_source_t _synchTimer;
    dispatch_queue_t _synchQueue;
    OCFMutableSetRef _dirtySources;
    os_unfair_lock _dirtySourcesLock;
    BOOL _active;
}

+ (instancetype)sharedInstance __attribute__((objc_direct));

- (void)synchronize __attribute__((objc_direct));

- (void)noteDirtySource:(id)source __attribute__((objc_direct));

- (OCFSetRef)copyDirtySourcesSnapshotAndClear __attribute__((objc_direct));

- (void)clear_alreadyLocked __attribute__((objc_direct));

- (void)synchronizeForDaemonTermination __attribute__((objc_direct));

@end
#endif

// Native _CFPrefsDInternString, 0x1c02db624 (Preferences/OCFPDSource.m). Hidden (was a private external). The
// interned (uniqued, retained) copy of a domain string.
OF_EXTERN OCFStringRef _CFPrefsDInternString(OCFStringRef string);

// Native _CFPrefsCopyUserNameForUID, 0x1c02dc934 (Preferences/OCFPDSource.m). Hidden (was a private
// external). "mobile" for every UID on this platform (not retained).
OF_EXTERN OCFStringRef _CFPrefsCopyUserNameForUID(uid_t uid);

// Native populateErrorReply, withClientContext, _CFPrefsFixUpIncomingMessageForPIDImpersonationIfNeeded,
// _CFPrefsGetFixedUpDomainForMessage and _CFPrefsCopyFixedUpUserForMessage (Preferences/OCFPrefsDaemon.m).
// Hidden (were private externals). The context block receives the connection's client context object.
OF_EXTERN void populateErrorReply(const char *description, xpc_object_t reply, int64_t errorType);
OF_EXTERN bool withClientContext(xpc_connection_t connection, void (^block)(id context));
OF_EXTERN void _CFPrefsFixUpIncomingMessageForPIDImpersonationIfNeeded(xpc_object_t message);
OF_EXTERN OCFStringRef _CFPrefsGetFixedUpDomainForMessage(
    xpc_object_t message, OCFStringRef user, bool byHost, const char **outDomain
);
OF_EXTERN OCFStringRef
_CFPrefsCopyFixedUpUserForMessage(xpc_object_t message, bool *outUserWasSpecified, bool *outIsRootOrRole);

// Native _CFPrefsCopyFixedUpContainerForMessage, 0x1c02e6290 (Preferences/OCFPrefsDaemon.m). Hidden (was a
// private external). The canonical container path of a message (+1), or NULL. The parameter names are local
// descriptive spellings of the native register use.
OF_EXTERN OCFStringRef _CFPrefsCopyFixedUpContainerForMessage(
    xpc_object_t message,
    OCFStringRef domain,
    const char *domainString,
    bool userWasSpecified,
    bool sandboxed,
    bool *outNeedsApproval
);
