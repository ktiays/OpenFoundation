//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Preferences/OCFPDCloudSource.h"
#import "CoreFoundation/Preferences/OCFPrefsBuffers.h"
#include "CoreFoundation/Preferences/OCFXPCBridge.h"

#import "CoreFoundation/Preferences/OCFPDSource.h"

#include <objc/message.h>
#include <dlfcn.h>
#include <dispatch/dispatch.h>
#include <fcntl.h>
#include <notify.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <unistd.h>

// Inferred organization: initSYDRemotePreferencesSource (0x1c01cbe38) and SYDRemotePreferencesSourceFunction
// (0x1c01cbe98) follow the CFPDCloudSource methods and ___remoteStoreChanged_block_invoke in the iOS
// Simulator
// __text section, so they are kept in an Objective-C translation unit for that native unit; the native source
// path is not in the evidence. The pattern is a soft link: SyncedDefaults is opened with dlopen (never
// linked). Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationPreferences.md.

// Native classSYDRemotePreferencesSource (__DATA,__bss 0x22e6d2e08). Non-external.
static Class classSYDRemotePreferencesSource;

// Native remoteStoreChanged, 0x1c01cbb44. Non-external natively; hidden for the C1 registration block.
// iOS Device divergence: unread. The captured CFPDCloudSource has its remote store ivar at 0x98.
void remoteStoreChanged(const void *object, void *context) {
    (void)context;
    OCFPDSource *source = (OCFPDSource *)object;
    // Native __remoteStoreChanged_block_invoke, 0x1c01cbdc8, with .cold.1 at 0x1c030ae0c.
    [source lockedAsync:^{
      id remote = *(id const *)((const uint8_t *)source + 0x98);
      ((void (*)(id, SEL, BOOL))objc_msgSend)(remote, sel_getUid("synchronizeForced:"), YES);
      os_log_t log = _CFPrefsDaemonLog();
      if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
          os_log_debug(log, "cloud preferences changed for %{public}@", (id)[source domain]);
      }
      [source updateShmemEntry];
      [source asyncNotifyObserversOfWriteFromConnection:NULL message:NULL];
    }];
}

// Native SyncedDefaultsLibrary: no symbol (inlined); its function static frameworkLibrary
// (SyncedDefaultsLibrary.frameworkLibrary, __DATA,__bss 0x22e6d2e10) holds the dlopen handle (mode 2,
// RTLD_NOW), retried while NULL.
static inline void *SyncedDefaultsLibrary(void) {
    static void *frameworkLibrary;
    if (frameworkLibrary == NULL) {
        frameworkLibrary =
            dlopen("/System/Library/PrivateFrameworks/SyncedDefaults.framework/SyncedDefaults", RTLD_NOW);
    }
    return frameworkLibrary;
}

// Native SYDRemotePreferencesSourceFunction, 0x1c01cbe98. Non-external. Answers the resolved class.
static Class SYDRemotePreferencesSourceFunction(void) {
    return classSYDRemotePreferencesSource;
}

// Native initSYDRemotePreferencesSource, 0x1c01cbe38. Non-external.
// With the framework open, resolves SYDRemotePreferencesSource (objc_getClass) and repoints the getter to
// SYDRemotePreferencesSourceFunction; answers the class (NULL when the framework cannot be opened).
static Class initSYDRemotePreferencesSource(void) {
    if (SyncedDefaultsLibrary() == NULL) {
        return Nil;
    }
    classSYDRemotePreferencesSource = (Class)objc_getClass("SYDRemotePreferencesSource");
    getSYDRemotePreferencesSourceClass = SYDRemotePreferencesSourceFunction;
    return classSYDRemotePreferencesSource;
}

// Native getSYDRemotePreferencesSourceClass (__DATA,__data 0x22e6cf750), initialized to
// initSYDRemotePreferencesSource. Non-external natively; hidden here for the other users of the soft link.
Class (*getSYDRemotePreferencesSourceClass)(void) = initSYDRemotePreferencesSource;

@protocol SYDRemotePreferencesSourceMessages
- (id)initWithApplicationID:(OCFStringRef)application
                    storeID:(OCFTypeRef)store
                     shared:(BOOL)shared
           additionalSource:(BOOL)additionalSource
              containerPath:(OCFStringRef)container;
- (void)registerForSynchronizedDefaults;
- (BOOL)synchronize;
- (OCFDictionaryRef)copyDictionary;
@end

@interface OCFPDCloudSource ()
- (OCFPDDataBuffer *)copyConfigurationFromPath:(OCFStringRef)path __attribute__((objc_direct));
- (void)registerForChangeNotifications __attribute__((objc_direct));
@end

// Native __MergedGlobals_2 and qword_22E6D2E20. Non-external watcher state.
static os_unfair_lock cloudSourcesLock = OS_UNFAIR_LOCK_INIT;
static OCFMutableSetRef cloudSources;

@implementation OCFPDCloudSource

// Native -[CFPDCloudSource
// initWithDomain:userName:storeName:configurationPath:containerPath:shmemIndex:daemon:], 0x1c01cbbc0. Listed
// method. Folds .cold.1, 0x1c030acf4. The incoming shmem index stays in W7 for the super send, omitted in
// Hex-Rays.
- (instancetype)initWithDomain:(OCFStringRef)domain
                      userName:(OCFStringRef)user
                     storeName:(xpc_object_t)store
             configurationPath:(OCFStringRef)configuration
                 containerPath:(OCFStringRef)container
                    shmemIndex:(int16_t)index
                        daemon:(OCFPrefsDaemon *)daemon {
    self = [super initWithDomain:domain
                        userName:user
                       container:container
                          byHost:NO
                         managed:NO
                      shmemIndex:index
                          daemon:daemon];
    if (self == nil) {
        return nil;
    }
    OCFTypeRef storeID = _OCFXPCCreateCFObjectFromXPCObject(store);
    _configurationPath = configuration != NULL ? OCFRetain(configuration) : NULL;
    Class sourceClass = getSYDRemotePreferencesSourceClass();
    cloudSource =
        [(id<SYDRemotePreferencesSourceMessages>)[sourceClass alloc] initWithApplicationID:OF_STR(
                                                                                               "NS"
                                                                                               "UserDefaults"
                                                                                           )
                                                                                   storeID:storeID
                                                                                    shared:YES
                                                                          additionalSource:YES
                                                                             containerPath:container];
    if (storeID != NULL) {
        OCFRelease(storeID);
    }
    [(id<SYDRemotePreferencesSourceMessages>)cloudSource registerForSynchronizedDefaults];
    OCFPDDataBuffer *buffer = [self copyConfigurationFromPath:configuration];
    OCFPropertyListRef plist = [buffer copyPropertyListWithMutability:0 error:NULL];
    [buffer release];
    if (plist != NULL) {
        if (OCFGetTypeID(plist) == 18) {
            SEL setter = sel_registerName("setDefaultsConfiguration:");
            if ([cloudSource respondsToSelector:setter]) {
                [cloudSource performSelector:setter withObject:(id)plist];
            }
        }
        OCFRelease(plist);
    }
    [self registerForChangeNotifications];
    return self;
}

// Native -[CFPDCloudSource synchronizeWithCloud], 0x1c01cbc74. Listed method.
- (BOOL)synchronizeWithCloud {
    return [(id<SYDRemotePreferencesSourceMessages>)cloudSource synchronize] != NO;
}

// Native -[CFPDCloudSource cloudConfigurationPath], 0x1c01cbd4c. Listed method.
- (OCFStringRef)cloudConfigurationPath {
    return _configurationPath;
}

// Native -[CFPDCloudSource
// validateMessage:withNewKey:newValue:plistIsAvailableToRead:containerPath:fileUID:mode:diagnosticMessage:],
// 0x1c01cbd5c. Listed method. Native cloud-source validation always answers 1.
- (int)validateMessage:(xpc_object_t)message
                withNewKey:(xpc_object_t)key
                  newValue:(xpc_object_t)value
    plistIsAvailableToRead:(BOOL)plistIsAvailableToRead
             containerPath:(const char *)containerPath
                   fileUID:(uid_t)fileUID
                      mode:(mode_t)mode
         diagnosticMessage:(const char **)diagnosticMessage {
    (void)message;
    (void)key;
    (void)value;
    (void)plistIsAvailableToRead;
    (void)containerPath;
    (void)fileUID;
    (void)mode;
    (void)diagnosticMessage;
    return 1;
}

// Native -[CFPDCloudSource processEndOfMessageIntendingToRemoveSource:replacingWithTombstone:],
// 0x1c01cbd64. Listed method. The assembly keeps X3 for the super send, omitted in Hex-Rays.
- (void)processEndOfMessageIntendingToRemoveSource:(BOOL *)remove replacingWithTombstone:(id *)tombstone {
    *remove = NO;
    [super processEndOfMessageIntendingToRemoveSource:remove replacingWithTombstone:tombstone];
}

// Native -[CFPDCloudSource copyConfigurationFromPath:], 0x1c030ab34. Direct method.
// Folds OUTLINED_FUNCTION_0_19 (0x1c01cbea4) and OUTLINED_FUNCTION_1_14 (0x1c01cbeb8).
- (OCFPDDataBuffer *)copyConfigurationFromPath:(OCFStringRef)path {
    char buffer[1026] = {0};
    if (!OCFStringGetFileSystemRepresentation(path, buffer, sizeof(buffer))) {
        return nil;
    }
    int fd = open(buffer, O_RDONLY | O_NOFOLLOW);
    if (fd < 0) {
        return nil;
    }
    OCFPDDataBuffer *configuration = [OCFPDDataBuffer newBufferFromFile:fd allowMappingIfSafe:NO];
    close(fd);
    return configuration;
}

// Native -[CFPDCloudSource registerForChangeNotifications], 0x1c030abdc. Direct method.
// Folds OUTLINED_FUNCTION_0_19 (0x1c01cbea4), OUTLINED_FUNCTION_1_14 (0x1c01cbeb8)
// and OUTLINED_FUNCTION_2_12 (0x1c01cbec8).
- (void)registerForChangeNotifications {
    os_unfair_lock_lock(&cloudSourcesLock);
    if (cloudSources == NULL) {
        cloudSources = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeSetCallBacks);
        int token = 0;
        dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
        // Native ___49-[CFPDCloudSource registerForChangeNotifications]_block_invoke, 0x1c01cbb04.
        notify_register_dispatch(
            "SYDRemotePreferencesSourceDidChangeExternally", &token, queue, ^(int notification) {
              (void)notification;
              os_unfair_lock_lock(&cloudSourcesLock);
              OCFSetApplyFunction(cloudSources, remoteStoreChanged, NULL);
              os_unfair_lock_unlock(&cloudSourcesLock);
            }
        );
    }
    OCFSetAddValue(cloudSources, self);
    os_unfair_lock_unlock(&cloudSourcesLock);
}

// Native -[CFPDCloudSource copyPropertyListWithoutDrainingPendingChangesValidatingPlist:], 0x1c030ac94.
// Direct method.
- (id)copyPropertyListWithoutDrainingPendingChangesValidatingPlist:(BOOL)validate {
    (void)validate;
    OCFDictionaryRef dictionary = [(id<SYDRemotePreferencesSourceMessages>)cloudSource copyDictionary];
    if (dictionary == NULL) {
        return nil;
    }
    OCFPDDataBuffer *buffer = [OCFPDDataBuffer newBufferFromPropertyList:dictionary];
    OCFRelease(dictionary);
    return buffer;
}

// Native -[CFPDCloudSource copyPropertyListValidatingPlist:], 0x1c030acf0. Listed method.
- (id)copyPropertyListValidatingPlist:(BOOL)validate {
    return [self copyPropertyListWithoutDrainingPendingChangesValidatingPlist:validate];
}

@end
