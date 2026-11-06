//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Preferences/OCFPrefsCloudSource.h"
#import "CoreFoundation/Base/OCFPlatform.h"
#import "CoreFoundation/Preferences/OCFPreferences.h"
#import "CoreFoundation/Preferences/OCFXPreferences.h"

#include <errno.h>
#include <fcntl.h>
#include <os/activity.h>
#include <os/lock.h>
#include <os/log.h>
#include <pthread/qos.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

// Inferred organization: the CFPrefsCloudSource methods (-initWithDomain:..., 0x1c028f4e8, through -dealloc,
// 0x1c028ff00) and their blocks are contiguous in the iOS Simulator __text section; the source path is not in
// the evidence (CFPrefsCloudSource.m inferred from the class). _CFPrefsCreatePreferencesDirectory
// (0x1c028ff94) and _safe_recursive_mkdir_chown (0x1c0290180) follow them, before
// _CFPrefsCopyDirectoryURLForTriplet_testing (0x1c02904e0); the unit boundary between them is unread, and
// they are kept with the preceding unit (inferred; the directory function uses @finally, so its unit is
// Objective-C). The unit is compiled with manual reference counting. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full).
// iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationPreferences.md.

// Constants as the native call sites pass them: kCFStringEncodingUTF8, the log-in-progress slot (15), and the
// preference roles (1, the user session daemon; 3, the direct daemon).
#define kUTF8Encoding ((OCFStringEncoding)0x08000100)
#define kLogInProgressSlot 15
#define kUserSessionDaemonRole 1
#define kDirectDaemonRole 3

@implementation OCFPrefsCloudSource

// Native -[CFPrefsCloudSource initWithDomain:user:byHost:containerPath:containingPreferences:], 0x1c028f4e8.
// The superclass initializer (selector reference 0x2387ab04d, the overridden selector), then the shmem entry
// points at the sentinel generation (release store).
- (id)initWithDomain:(OCFStringRef)domain
                     user:(OCFStringRef)user
                   byHost:(BOOL)byHost
            containerPath:(OCFStringRef)containerPath
    containingPreferences:(_OCFXPreferences *)containingPreferences {
    self = [super initWithDomain:domain
                            user:user
                          byHost:byHost
                   containerPath:containerPath
           containingPreferences:containingPreferences];
    if (self != nil) {
        __atomic_store_n(&shmemEntry, (uint32_t *)&sentinelGeneration, __ATOMIC_RELEASE);
    }
    return self;
}

// Native -[CFPrefsCloudSource createSynchronizeMessage], 0x1c028f568. The superclass message (selector
// reference 0x2387ae072, the overridden selector) gains the cloud configuration path and store identifier
// when a configuration path is set.
- (xpc_object_t)createSynchronizeMessage {
    xpc_object_t message = [super createSynchronizeMessage];
    if (message != NULL && _configPath != NULL) {
        xpc_dictionary_set_string(message, "CFPreferencesCloudConfig", _configPath);
        xpc_dictionary_set_string(message, "CFPreferencesCloudStoreIdentifier", _storeName);
    }
    return message;
}

// Native -[CFPrefsCloudSource alreadylocked_updateObservingRemoteChanges], 0x1c028f61c. A volatile (disabled)
// cloud source answers 0; otherwise the superclass decides (selector reference 0x2387ad264, the overridden
// selector).
- (int)alreadylocked_updateObservingRemoteChanges {
    if (__atomic_load_n(&_volatile, __ATOMIC_ACQUIRE)) {
        return 0;
    }
    return [super alreadylocked_updateObservingRemoteChanges];
}

// Native -[CFPrefsCloudSource fullCloudSynchronizeWithCompletionHandler:], 0x1c028f69c, with
// ___64-[CFPrefsCloudSource fullCloudSynchronizeWithCompletionHandler:]_block_invoke (0x1c028f77c) and
// _block_invoke_2 (0x1c028f8d8). The source lock is taken here and released by the block. On the
// connection for the source's role (3 in direct mode, else 1), a synchronize message marked FullCloudSync is
// sent with a reply handler on the global queue of the current QoS class; the reply handler calls the
// completion handler with an empty CFType dictionary (released afterwards). Without a connection or a
// message, the completion handler is called with NULL.
- (void)fullCloudSynchronizeWithCompletionHandler:(void (^)(OCFDictionaryRef changes))completionHandler {
    os_unfair_lock_lock(&_lock);
    int role = [self isDirectModeEnabled] ? kDirectDaemonRole : kUserSessionDaemonRole;
    _OCFXPreferences *preferences = _containingPreferences;
    [preferences withConnectionForRole:role
                     andUserIdentifier:[self userIdentifier]
                          performBlock:^(xpc_connection_t connection) {
                            if (connection != NULL) {
                                xpc_object_t message = [self createSynchronizeMessage];
                                if (message != NULL) {
                                    xpc_dictionary_set_bool(message, "FullCloudSync", true);
                                    os_unfair_lock_unlock(&self->_lock);
                                    xpc_connection_send_message_with_reply(
                                        connection,
                                        message,
                                        dispatch_get_global_queue(qos_class_self(), 0),
                                        ^(xpc_object_t reply) {
                                          (void)reply;
                                          OCFDictionaryRef changes = OCFDictionaryCreate(
                                              kOCFAllocatorSystemDefault,
                                              NULL,
                                              NULL,
                                              0,
                                              &kOCFTypeDictionaryKeyCallBacks,
                                              &kOCFTypeDictionaryValueCallBacks
                                          );
                                          completionHandler(changes);
                                          OCFRelease(changes);
                                        }
                                    );
                                    xpc_release(message);
                                    return;
                                }
                            }
                            os_unfair_lock_unlock(&self->_lock);
                            completionHandler(NULL);
                          }];
}

// Native -[CFPrefsCloudSource mergeIntoDictionary:sourceDictionary:cloudKeyEvaluator:], 0x1c028f938, with
// ___77-[CFPrefsCloudSource mergeIntoDictionary:sourceDictionary:cloudKeyEvaluator:]_block_invoke
// (0x1c028fa30, descriptor signature v24@?0^{__CFString=}8^v16). Only an enabled source (a plain byte load
// equal to 1) merges: its dictionary is copied under the lock, and every key the evaluator accepts is added
// to the dictionary, with this source recorded for it in the source dictionary when one is given. The
// evaluator is invoked with X1 and X2 still holding the key and value; its parameters beyond the key are
// unread (confidence medium for the evaluator's signature).
- (void)mergeIntoDictionary:(OCFMutableDictionaryRef)dictionary
           sourceDictionary:(OCFMutableDictionaryRef)sourceDictionary
          cloudKeyEvaluator:(BOOL (^)(OCFStringRef key))cloudKeyEvaluator {
    if (_enabled != 1) {
        return;
    }
    os_unfair_lock_lock(&_lock);
    OCFDictionaryRef values = [self alreadylocked_copyDictionary];
    os_unfair_lock_unlock(&_lock);
    if (values == NULL) {
        return;
    }
    _CFPrefsDictionaryApplyBlock(values, ^(const void *key, const void *value) {
      if (cloudKeyEvaluator((OCFStringRef)key)) {
          OCFDictionaryAddValue(dictionary, key, value);
          if (sourceDictionary != NULL) {
              OCFDictionaryAddValue(sourceDictionary, key, self);
          }
      }
    });
    OCFRelease(values);
}

// Native -[CFPrefsCloudSource enabled], 0x1c028fa9c.
- (BOOL)enabled {
    return _enabled;
}

// Native -[CFPrefsCloudSource setEnabled:], 0x1c028faac, with ___33-[CFPrefsCloudSource setEnabled:]
// _block_invoke (0x1c028fc10) and _block_invoke_2 (0x1c028fd38). Under the source lock: the volatile flag
// becomes the inverse (release store) and the enabled byte the value. Disabling points the shmem entry at the
// sentinel. Enabling clears the entry and, when the observing state changes, sends the observer-update
// message synchronously on the connection for the reported role, inside an activity ("Updating Key-Value
// Observers Of Preferences") and the waiting marker of that role's daemon (1 user, 3 direct, else system).
// The activity is released before its scope is left, as natively. The message is released.
- (void)setEnabled:(BOOL)enabled {
    os_unfair_lock_lock(&_lock);
    bool disabled = enabled ^ 1;
    __atomic_store_n(&_volatile, disabled, __ATOMIC_RELEASE);
    _enabled = enabled;
    if (disabled) {
        __atomic_store_n(&shmemEntry, (uint32_t *)&sentinelGeneration, __ATOMIC_RELEASE);
    } else {
        __atomic_store_n(&shmemEntry, NULL, __ATOMIC_RELEASE);
        int role = 0;
        xpc_object_t message = [self alreadylocked_createObserverUpdateMessageWithOperation:
                                         [self alreadylocked_updateObservingRemoteChanges]
                                                                                    forRole:&role];
        if (message != NULL) {
            _OCFXPreferences *preferences = _containingPreferences;
            int messageRole = role;
            [preferences withConnectionForRole:role
                             andUserIdentifier:[self userIdentifier]
                                  performBlock:^(xpc_connection_t connection) {
                                    if (connection == NULL) {
                                        return;
                                    }
                                    os_activity_t activity = os_activity_create(
                                        "Updating Key-Value Observers Of Preferences",
                                        OS_ACTIVITY_CURRENT,
                                        OS_ACTIVITY_FLAG_DEFAULT
                                    );
                                    struct os_activity_scope_state_s state = {{0, 0}};
                                    os_activity_scope_enter(activity, &state);
                                    _OCFXPreferences *containing = self->_containingPreferences;
                                    void (^send)(void) = ^{
                                      xpc_object_t reply =
                                          xpc_connection_send_message_with_reply_sync(connection, message);
                                      if (reply != NULL) {
                                          xpc_release(reply);
                                      }
                                    };
                                    if (messageRole == kUserSessionDaemonRole) {
                                        CFPREFERENCES_IS_WAITING_FOR_USER_CFPREFSD(containing, send);
                                    } else if (messageRole == kDirectDaemonRole) {
                                        CFPREFERENCES_IS_WAITING_FOR_DIRECT_CFPREFSD(containing, send);
                                    } else {
                                        CFPREFERENCES_IS_WAITING_FOR_SYSTEM_CFPREFSD(containing, send);
                                    }
                                    [activity release];
                                    os_activity_scope_leave(&state);
                                  }];
            xpc_release(message);
        }
    }
    os_unfair_lock_unlock(&_lock);
}

// Native -[CFPrefsCloudSource setConfigurationPath:], 0x1c028fd60. Under the source lock the previous path is
// freed and the new one stored as a strdup of its UTF-8 form (at most 1,026 bytes).
// CoreFoundation defect (fixed): natively the freed pointer stays stored when the new path is NULL or does
// not convert (0x1c028fdb4 frees it; the store at 0x1c028fdec is skipped), so -createSynchronizeMessage reads
// freed memory and a later set or -dealloc frees it again. The restoration clears the ivar after freeing it.
- (void)setConfigurationPath:(OCFStringRef)configurationPath {
    os_unfair_lock_lock(&_lock);
    if (_configPath != NULL) {
        free(_configPath);
        _configPath = NULL;
    }
    char buffer[1026];
    bzero(buffer, sizeof(buffer));
    if (configurationPath != NULL && OCFStringGetCString(configurationPath, buffer, 1026, kUTF8Encoding)) {
        _configPath = strdup(buffer);
    }
    os_unfair_lock_unlock(&_lock);
}

// Native -[CFPrefsCloudSource setStoreName:], 0x1c028fe30. As -setConfigurationPath: for the store name.
// CoreFoundation defect (fixed): the same stale pointer after free (0x1c028fe84), cleared here.
- (void)setStoreName:(OCFStringRef)storeName {
    os_unfair_lock_lock(&_lock);
    if (_storeName != NULL) {
        free(_storeName);
        _storeName = NULL;
    }
    char buffer[1026];
    bzero(buffer, sizeof(buffer));
    if (storeName != NULL && OCFStringGetCString(storeName, buffer, 1026, kUTF8Encoding)) {
        _storeName = strdup(buffer);
    }
    os_unfair_lock_unlock(&_lock);
}

// Native -[CFPrefsCloudSource dealloc], 0x1c028ff00. Frees the store name, then the configuration path, then
// the superclass's -dealloc (selector reference 0x2387a9f83).
- (void)dealloc {
    if (_storeName != NULL) {
        free(_storeName);
    }
    if (_configPath != NULL) {
        free(_configPath);
    }
    [super dealloc];
}

@end

static int _safe_recursive_mkdir_chown(char *path, int dirfd, mode_t mode, uid_t uid, gid_t gid, int *outFD);

// Native _CFPrefsCreatePreferencesDirectory, 0x1c028ff94. Hidden (was a private external). Walking back from
// the end of the path, each slash-terminated prefix is opened (O_SEARCH) until one exists (ENOENT continues;
// another error, or reaching the start, fails). The remainder is created below it by
// _safe_recursive_mkdir_chown (errno preserved across the close). On failure with a nonzero errno an error is
// logged (with the log-in-progress slot set, cleared in a @finally; region bounds inferred, confidence
// medium). Answers whether the directories exist.
bool _CFPrefsCreatePreferencesDirectory(char *path, mode_t mode, uid_t uid, gid_t gid, int *outFD) {
    size_t length = strlen(path);
    if (length != 0) {
        char *cursor = path + length;
        for (;;) {
            if (*cursor == '/') {
                *cursor = 0;
                int fd = open(path, O_SEARCH);
                *cursor = '/';
                if (fd == -1) {
                    --cursor;
                    if (errno == ENOENT && cursor != path) {
                        continue;
                    }
                    break;
                }
                if (fd >= 0) {
                    int result = _safe_recursive_mkdir_chown(cursor + 1, fd, mode, uid, gid, outFD);
                    int savedErrno = errno;
                    close(fd);
                    errno = savedErrno;
                    if (result == 0) {
                        return true;
                    }
                    break;
                }
            }
            if (--cursor == path) {
                break;
            }
        }
    }
    if (errno != 0) {
        int failure = errno;
        _OCFSetTSD(kLogInProgressSlot, (void *)kOCFBooleanTrue, NULL);
        @try {
            os_log_error(
                _CFPrefsClientLog(),
                "Failed to create directory %{private}s (mode %d, UID %d, GID %d) because of "
                "%{darwin.errno}d.",
                path,
                mode,
                uid,
                gid,
                failure
            );
        } @finally {
            _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
        }
    }
    return false;
}

// Native _safe_recursive_mkdir_chown, 0x1c0290180. Non-external. The first component of path (terminated in
// place) is opened below dirfd (O_DIRECTORY). A missing component (ENOENT; other errors answer -1), or an
// existing one on a read-only volume (MNT_RDONLY) whose parent is a union mount (MNT_UNION), is created
// (mkdirat with mode), reopened (O_DIRECTORY | O_NOFOLLOW) and given to uid and gid; a failed chown that left
// another owner closes, removes it and answers -1 with that errno. The component's character is restored and
// following slashes skipped. A further component recurses (a created directory is removed when the recursion
// fails; errno is preserved). At the end the descriptor is stored in *outFD when given, else closed, and 0
// answered. A negative descriptor other than -1 from the first open is carried on as natively. The statfs
// and stat buffers are local; the native code zero-fills their first 512 bytes (stat: all 144), which no
// read depends on, consistent with the compiler's automatic initialization of large locals (no source
// statement inferred).
static int _safe_recursive_mkdir_chown(char *path, int dirfd, mode_t mode, uid_t uid, gid_t gid, int *outFD) {
    char *end = path;
    char saved;
    while ((saved = *end) != 0 && saved != '/') {
        ++end;
    }
    *end = 0;
    int fd = openat(dirfd, path, O_DIRECTORY);
    bool created = false;
    bool create = false;
    if (fd < 0) {
        if (fd == -1) {
            if (errno != ENOENT) {
                return -1;
            }
            create = true;
        }
    } else {
        struct statfs volume;
        if (fstatfs(fd, &volume) != -1 && (volume.f_flags & MNT_RDONLY) != 0) {
            struct statfs parentVolume;
            if (fstatfs(dirfd, &parentVolume) != -1 && (parentVolume.f_flags & MNT_UNION) != 0) {
                close(fd);
                create = true;
            }
        }
    }
    if (create) {
        if (mkdirat(dirfd, path, mode) == -1) {
            return -1;
        }
        fd = openat(dirfd, path, O_DIRECTORY | O_NOFOLLOW);
        if (fd == -1) {
            return -1;
        }
        if (fchown(fd, uid, gid) == -1) {
            int failure = errno;
            struct stat info;
            if (fstat(fd, &info) != 0 || info.st_uid != uid || info.st_gid != gid) {
                close(fd);
                unlinkat(dirfd, path, AT_REMOVEDIR);
                errno = failure;
                return -1;
            }
        }
        created = true;
    }
    *end = saved;
    while (saved == '/') {
        saved = *++end;
    }
    if (saved != 0) {
        int result = _safe_recursive_mkdir_chown(end, fd, mode, uid, gid, outFD);
        int savedErrno = errno;
        close(fd);
        if (result == -1 && created) {
            unlinkat(dirfd, path, AT_REMOVEDIR);
        }
        errno = savedErrno;
        return result;
    }
    if (outFD != NULL) {
        *outFD = fd;
        return 0;
    }
    close(fd);
    return 0;
}
