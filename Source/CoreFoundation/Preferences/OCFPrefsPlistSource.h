//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Preferences/OCFPreferences.h"
#include "CoreFoundation/Preferences/OCFPrefsSource.h"
#include "CoreFoundation/Preferences/OCFXPreferences.h"

#include <stdbool.h>
#include <stdint.h>
#include <xpc/xpc.h>

// Native sentinelGeneration (__TEXT,__const 0x1c03a42a0). Hidden (was a private external). The generation
// word a source's shmem entry points at while it has no shared-memory slot: 0xffffffff, never equal to a
// published generation. Its first four bytes are read (32-bit atomic loads); the rest of its 32-byte slot is
// zero padding.
OF_EXTERN const uint32_t sentinelGeneration;

// Native _CFPrefsTestingFlags (__DATA,__common 0x22e6d4280, 16 bytes). External. The testing switches the
// preference sources read: the bytes at offset 0 (clear the cache before every write; synchronous,
// cache-avoiding synchronize messages), 1 (clear the cache before every read), 2 (randomly synchronous
// synchronize messages), 3 (simulate a slow file system), 4 (mark retried messages as following an injected
// failure) and 8 (fail access-token attachment), and the 32-bit word at offset 12 (simulate running out of
// disk space, read with acquire loads and written with release stores). The daemon captures establish
// byte 6 (force daemon-style file lookup), 7 (force access-token checks) and 9 (one-shot write/cache refresh
// before token validation). Other bytes remain unread. The 16-byte alignment matches its native common
// storage at 0x22e6d4280 and the definition in OCFPrefsPlistSource.m; atomic word accesses require it here
// too.
OF_EXPORT __attribute__((aligned(16))) uint8_t _OCFPrefsTestingFlags[16];

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class CFPrefsPlistSource (__DATA,__objc_data 0x22e6cf3e0), a hidden CFPrefsSource subclass with
// instance start 72 and size 144. Ivar names, offsets and sizes from its class_ro_t (iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified); the type encodings lie outside the
// extracted image, so the types below are the ones the restored code establishes. The flag bytes and the
// locally-set dictionary and access path are read and written with acquire loads and release stores
// (LDAR/STLR). The ivars are public because the CFPrefsCloudSource subclass and blocks of this family read
// them directly; their declared visibility is unread.
@interface OCFPrefsPlistSource : OCFPrefsSource {
    @public
    OCFStringRef userIdentifier;
    OCFStringRef domainIdentifier;
    OCFStringRef container;
    OCFMutableDictionaryRef _locallySetDict;
    char *accessPath;
    uint32_t userUID;
    uint32_t userGID;
    bool _isByHost;
    bool _volatile;
    bool _readonly;
    bool _avoidsDaemonCache;
    bool _restrictedAccess;
    bool _checkedInvalidHome;
    bool _lastWriteFailed;
    bool _observing;
    bool _byteCountLimitExceeded;
    bool _directMode;
    bool _disableBackup;
    int _fileProtectionClass;
    OCFErrorRef _lastError;
}

- (OCFStringRef)userIdentifier;
- (OCFStringRef)domainIdentifier;
- (OCFStringRef)container;
- (BOOL)isByHost;
- (id)initWithDomain:(OCFStringRef)domain
                     user:(OCFStringRef)user
                   byHost:(BOOL)byHost
            containerPath:(OCFStringRef)containerPath
    containingPreferences:(_OCFXPreferences *)containingPreferences;
- (void)setBackupDisabled:(BOOL)disabled;
- (void)setFileProtectionClass:(int)protectionClass;
- (void)setAccessRestricted:(BOOL)restricted;
- (void)setDaemonCacheEnabled:(BOOL)enabled;
- (OCFPropertyListRef)alreadylocked_copyValueForKey:(OCFStringRef)key;
- (OCFPropertyListRef)copyValueForKey:(OCFStringRef)key;
- (OCFArrayRef)alreadylocked_copyKeyList;
- (OCFMutableDictionaryRef)alreadylocked_copyDictionary;
- (void)alreadylocked_setPrecopiedValues:(const void **)values
                                 forKeys:(OCFStringRef *)keys
                                   count:(OCFIndex)count
                                    from:(id)from;
- (BOOL)isVolatile;
- (BOOL)isDirectModeEnabled;
- (BOOL)servedByUserSessionDaemon;
- (xpc_object_t)createRequestNewContentMessageForDaemon:(int)role;
- (void)handleReply:(xpc_object_t)reply
    toRequestNewDataMessage:(xpc_object_t)message
               onConnection:(xpc_connection_t)connection
                 retryCount:(int)retryCount
                      error:(bool *)error;
- (void)alreadylocked_clearCache;
- (BOOL)synchronize;
- (void)alreadylocked_recordError:(OCFErrorRef)error;
- (void)alreadylocked_clearError;
- (OCFErrorRef)alreadylocked_lastError;
- (int64_t)alreadylocked_generationCount;
- (int64_t)generationCount;
- (int)alreadylocked_updateObservingRemoteChanges;
- (xpc_object_t)alreadylocked_createObserverUpdateMessageWithOperation:(int)operation forRole:(int *)role;
- (void)transitionIntoDirectModeIfNeededWithRetryBlock:(void (^)(void))retryBlock;
- (void)dealloc;

- (void)setDomainIdentifier:(OCFStringRef)domainIdentifier __attribute__((objc_direct));

- (long)sendMessageSettingValues:(const void **)values
                         forKeys:(OCFStringRef *)keys
                           count:(OCFIndex)count __attribute__((objc_direct));

- (void)writeFailedForKeys:(OCFStringRef *)keys
                    values:(const void **)values
                     count:(OCFIndex)count __attribute__((objc_direct));

- (BOOL)alreadylocked_requestNewData __attribute__((objc_direct));

- (void)sendRequestNewDataMessage:(xpc_object_t)message
                     toConnection:(xpc_connection_t)connection
                       retryCount:(int)retryCount
                            error:(bool *)error __attribute__((objc_direct));

- (BOOL)volatilizeIfInvalidHomeDir __attribute__((objc_direct));

- (void)setUserIdentifier:(OCFStringRef)userIdentifier __attribute__((objc_direct));

- (void)setContainer:(OCFStringRef)container __attribute__((objc_direct));

- (BOOL)_logLoudlyAboutSettingKey:(OCFStringRef)key __attribute__((objc_direct));

- (BOOL)createPreferencesDirectory:(char *)path __attribute__((objc_direct));

- (void)alreadylocked_withUncanonicalizedPaths:(void (^)(const char *path))block __attribute__((objc_direct));

- (BOOL)attachAccessTokenToMessage:(xpc_object_t)message
                        accessType:(int)accessType __attribute__((objc_direct));

- (void)addPIDImpersonationIfNecessary:(xpc_object_t)message __attribute__((objc_direct));

- (int)role __attribute__((objc_direct));

- (void)sendFullyPreparedMessage:(xpc_object_t)message
                    toConnection:(xpc_connection_t)connection
                   settingValues:(const void **)values
                         forKeys:(OCFStringRef *)keys
                           count:(OCFIndex)count
                      retryCount:(int)retryCount __attribute__((objc_direct));

- (void)_sharedCleanup __attribute__((objc_direct));

- (xpc_object_t)createSynchronizeMessage;

- (BOOL)_isSharedInTheiOSSimulator __attribute__((objc_direct));

- (BOOL)handleErrorReply:(xpc_object_t)reply
               toMessage:(xpc_object_t)message
             settingKeys:(OCFStringRef *)keys
                toValues:(const void **)values
                   count:(OCFIndex)count
              retryCount:(int)retryCount
       retryContinuation:(void (^)(int retryCount))continuation __attribute__((objc_direct));

- (void)handlePossibleOversizedMessage:(int)acceptability
                        forWritingKeys:(OCFStringRef *)keys
                                values:(const void **)values
                                 count:(OCFIndex)count __attribute__((objc_direct));

- (void)requestPlistValidation __attribute__((objc_direct));

@end

// Native class _CFPrefsOversizedPlistDescription (__DATA,__objc_data 0x22e6cf408), a hidden NSObject subclass
// with instance size 24: existingValues (8) and newValues (16), from its class_ro_t. Its five listed methods
// and its direct initializer are restored in Preferences/OCFPrefsPlistSource.m.
@interface _OCFPrefsOversizedPlistDescription : NSObject {
    @protected
    OCFDictionaryRef existingValues;
    OCFDictionaryRef newValues;
}

- (id)_descriptionOfDictionary:(OCFDictionaryRef)dict withKeyRedaction:(BOOL)redact;
- (id)_descriptionWithKeyRedaction:(BOOL)redact;
- (id)redactedDescription;
- (id)description;
- (void)dealloc;

- (id)initWithDict:(OCFDictionaryRef)dict
           setKeys:(const void **)keys
         andValues:(const void **)values
             count:(OCFIndex)count __attribute__((objc_direct));

@end

// Native __CFPREFERENCES_HAS_DETECTED_THIS_APP_TRYING_TO_STORE_TOO_MUCH_DATA__, 0x1c0317f48. Non-external
// natively; hidden here (its caller, -handlePossibleOversizedMessage:forWritingKeys:values:count:, is defined
// in the class's @implementation before it). Hidden until the owner records a naming decision.
OF_EXTERN void __CFPREFERENCES_HAS_DETECTED_THIS_APP_TRYING_TO_STORE_TOO_MUCH_DATA__(
    OCFPrefsPlistSource *source, OCFStringRef *keys, const void **values, OCFIndex count
);

// Native CFPrefsManagedSource, class_ro_t 0x2227c8cc0. CFPrefsPlistSource subclass, size 144, no added ivars.
@interface OCFPrefsManagedSource : OCFPrefsPlistSource
@end

#endif

// CFPreferencesRedactedName is declared in Preferences/OCFPreferencesDescription.h.

// Native _plistSizeIsAppropriateToRead, 0x1c0303ebc (Preferences/OCFPrefsPlistSource.m). Non-external
// natively; hidden here because -handleReply:toRequestNewDataMessage:onConnection:retryCount:error: uses it.
// The size is signed natively (a size outside 1 through LONG_MAX - 1 is invalid).
OF_EXTERN bool _plistSizeIsAppropriateToRead(long size);

// Native _approximateSizeOfPlistValue, 0x1c0304f98, with its .cold.1 (0x1c0318dbc, the assertion
// "outObjectCounter == NULL", CFPrefsPlistSource.m line 2042) and its two blocks, restored in
// Preferences/OCFPrefsPlistSource.m. Non-external natively; hidden here. Signature from its assembly: the
// value, an optional description out-parameter (a created string or a constant), an optional nested-object
// counter; the approximate encoded size is answered.
OF_EXTERN size_t _approximateSizeOfPlistValue(
    OCFPropertyListRef value, OCFStringRef *outDescription, OCFIndex *outObjectCounter
);
