//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Preferences/OCFPrefsSearchListSource.h"
#import "CoreFoundation/Base/OCFPlatform.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#import "CoreFoundation/Preferences/OCFPreferencesDescription.h"
#import "CoreFoundation/Preferences/OCFPrefsPendingKVONotification.h"
#import "CoreFoundation/Preferences/OCFPreferences.h"
#import "CoreFoundation/Preferences/OCFPrefsSource.h"
#import "CoreFoundation/Preferences/OCFXPreferences.h"

#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <assert.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <mach/mach_time.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <os/lock.h>
#include <os/log.h>
#include <os/signpost.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Inferred organization: eduModeEnabled (0x1c02af354) through LSApplicationRecordFunction (0x1c02b41a0) lie
// with the CFPrefsSearchListSource methods in the iOS Simulator __text section, and the assertions of
// -addSource: and normalizeQuintuplet name "CFPrefsSearchListSource.m"; the search-list direct methods placed
// with the cold code (0x1c0310728-0x1c0311700) belong to the same unit (inferred from their callers and block
// symbols). The restored functions of that unit are kept in this translation unit; the class's single
// @implementation holds its methods first, then the _CFXPreferences(SearchListAdditions) category. The unit
// is Objective-C compiled with manual reference counting. ___Block_byref_object_copy__4 (0x1c02b2f9c) and
// ___Block_byref_object_dispose__4 (0x1c02b2fac), the helpers of a __block object variable (flags 131), are
// emitted by the compiler for the __block search-list variables below; they have no source definition. The
// .cold fragments (0x1c0311844-0x1c0311cac) and the machine-outlined fragments they and the direct methods
// call are kept in their parents. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationPreferences.md.
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_1_37 (0x1c02b4334).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_5_13 (0x1c02b438c), _OUTLINED_FUNCTION_16_2
// (0x1c02b4468), _OUTLINED_FUNCTION_17_2 (0x1c02b447c), _OUTLINED_FUNCTION_19_2 (0x1c02b4498),
// _OUTLINED_FUNCTION_21_2 (0x1c02b44a8).

// Constants as the native call sites pass them: kCFStringEncodingUTF8 is not used here; kCFURLPOSIXPathStyle,
// NSLibraryDirectory (5) with NSSystemDomainMask (8), and the CFDictionary type ID (18, a builtin class). The
// thread-specific slot 15 marks a log in progress.
#define kPOSIXPathStyle ((OCFIndex)0)
#define kLibraryDirectory ((OCFIndex)5)
#define kSystemDomainMask ((OCFIndex)8)
#define kDictionaryTypeID ((OCFTypeID)18)
#define kLogInProgressSlot 15

static OCFURLRef _CFPrefsCopyCloudConfigurationURLForIdentifier(OCFStringRef identifier);
static void notifySearchListOfRemoteChangesCallback(const void *key, const void *value, void *context);
static bool _CFPrefsWritingToExternalAppCloudPreferencesEnabled(void);

// Native clientHandle, daemonHandle, longLivedLogHandle, signpostHandle, signpostDetailedHandle and
// signpostHandleMinimumThreshold (__DATA,__bss, written once by the log-handle block). Non-external.
static os_log_t clientHandle;
static os_log_t daemonHandle;
static os_log_t longLivedLogHandle;
static os_log_t signpostHandle;
static os_log_t signpostDetailedHandle;
static uint64_t signpostHandleMinimumThreshold;

// Native makeLogHandles: no symbol; inlined into _CFPrefsClientLog, _CFPrefsDaemonLog and their callers,
// where only its once token (makeLogHandles.logToken, __DATA,__bss 0x22e6d39b8) and the dispatch_once survive
// (the .cold.1 fragments of the two log functions and OUTLINED_FUNCTION_1_37/_2_31). Its block,
// ___makeLogHandles_block_invoke (0x1c02b41ac, on the notify-error-misc list), is restored here because it
// can only be expressed inside this function: the five com.apple.defaults log handles, and the signpost
// threshold 1,000,000 * denom / numer of the Mach timebase in 32-bit unsigned arithmetic.
static inline void makeLogHandles(void) {
    static dispatch_once_t logToken;
    dispatch_once(&logToken, ^{
      clientHandle = os_log_create("com.apple.defaults", "User Defaults");
      daemonHandle = os_log_create("com.apple.defaults", "cfprefsd");
      longLivedLogHandle = os_log_create("com.apple.defaults", "audit");
      signpostHandle = os_log_create("com.apple.defaults", "signposts");
      signpostDetailedHandle = os_log_create("com.apple.defaults", "signpostsDetailed");
      mach_timebase_info_data_t info = {0, 0};
      mach_timebase_info(&info);
      signpostHandleMinimumThreshold = (uint32_t)(info.denom * 1000000u) / info.numer;
    });
}

// Native eduModeOverride (__DATA,__bss 0x22e6d3958). Non-external; its writer is outside the evidence, so it
// stays zero (not in education mode) in this restoration.
static uint8_t eduModeOverride;

// Native eduModeEnabled, 0x1c02af354. Non-external natively (hidden here: the other preference units call
// it). Answers the override byte.
Boolean eduModeEnabled(void) {
    return eduModeOverride;
}

// Native _CFPrefsCreateConfigurationFileNameFromIdentifier, 0x1c02af360. Hidden (was a private external).
// NULL answers NULL; otherwise a mutable copy with "-cloud-users.defaults" appended in education mode (the
// override byte is read before the copy; eduModeEnabled is inlined natively) or ".defaults".
OCFMutableStringRef _CFPrefsCreateConfigurationFileNameFromIdentifier(OCFStringRef identifier) {
    if (identifier == NULL) {
        return NULL;
    }
    Boolean education = eduModeEnabled();
    OCFMutableStringRef name = OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, identifier);
    OCFStringAppend(name, education ? OF_STR("-cloud-users.defaults") : OF_STR(".defaults"));
    return name;
}

// Native _CFPrefsCopyCloudConfigurationURLFromBundle, 0x1c02af3c8. Hidden (was a private external).
// The configuration file name (from the identifier, or the bundle's identifier when NULL) inside the bundle's
// resources directory; NULL without a resources directory or a name.
OCFURLRef _CFPrefsCopyCloudConfigurationURLFromBundle(OCFBundleRef bundle, OCFStringRef identifier) {
    OCFURLRef resources = OCFBundleCopyResourcesDirectoryURL(bundle);
    if (resources == NULL) {
        return NULL;
    }
    if (identifier == NULL) {
        identifier = OCFBundleGetIdentifier(bundle);
    }
    OCFStringRef name = _CFPrefsCreateConfigurationFileNameFromIdentifier(identifier);
    OCFURLRef result = NULL;
    if (name != NULL) {
        result = OCFURLCreateCopyAppendingPathComponent(kOCFAllocatorSystemDefault, resources, name, false);
        OCFRelease(name);
    }
    OCFRelease(resources);
    return result;
}

extern char searchListStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char searchListExceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

@interface OCFPrefsSearchListSource ()
- (xpc_object_t)createMultiMessageWithContainedMessages:(xpc_object_t)messages __attribute__((objc_direct));
- (void)synchronouslySendSystemMessage:(xpc_object_t)system
                        andUserMessage:(xpc_object_t)user
                      andDirectMessage:(xpc_object_t)direct
                          replyHandler:
                              (void (^)(
                                  xpc_object_t message, xpc_object_t reply, xpc_connection_t connection
                              ))replyHandler __attribute__((objc_direct));
- (void)alreadylocked_getDictionary:(void (^)(OCFDictionaryRef dictionary))block __attribute__((objc_direct));
- (BOOL)alreadylocked_useCloudForKey:(OCFStringRef)key __attribute__((objc_direct));
- (BOOL)alreadylocked_requestNewData __attribute__((objc_direct));
- (int64_t)alreadylocked_generationCountFromListOfSources:(const void **)sources
                                                    count:(OCFIndex)count __attribute__((objc_direct));
- (OCFMutableArrayRef)_getPendingNotifications __attribute__((objc_direct));
- (id)descriptionRedactingContainer:(BOOL)redact __attribute__((objc_direct));
@end

@implementation OCFPrefsSearchListSource

// Native -[CFPrefsSearchListSource dealloc], 0x1c02b2858. Reached when the runtime deallocates (round-3
// scope). The source list, identifier, cloud keys, cloud prefix keys and key map are released in that order;
// the identifier and source list are cleared. Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device divergence: unread).
- (void)dealloc {
    if (_sourceList != NULL) {
        OCFRelease(_sourceList);
    }
    if (_identifier != NULL) {
        OCFRelease(_identifier);
    }
    if (_cloudKeys != NULL) {
        OCFRelease(_cloudKeys);
    }
    if (_cloudPrefixKeys != NULL) {
        OCFRelease(_cloudPrefixKeys);
    }
    if (_keysToSources != NULL) {
        OCFRelease(_keysToSources);
    }
    _identifier = NULL;
    _sourceList = NULL;
    [super dealloc];
}

// Native -[CFPrefsSearchListSource initWithIdentifier:containingPreferences:], 0x1c02af28c. Listed method.
- (instancetype)initWithIdentifier:(OCFStringRef)identifier
             containingPreferences:(_OCFXPreferences *)containingPreferences {
    self = [super initWithContainingPreferences:containingPreferences];
    if (self != nil) {
        _identifier = _CFStringCopyBundleUnloadingProtectedString(identifier);
        _isSearchList = YES;
        __atomic_store_n(&_generationCount, -1, __ATOMIC_RELEASE);
    }
    return self;
}

// Native -[CFPrefsSearchListSource copyCloudConfigurationWithURL:outConfigFileSource:outStoreName:],
// 0x1c02af470, with its .cold.1 and .cold.3 (0x1c0311844, 0x1c031188c, the inlined log-handle dispatch_once)
// and .cold.2 and .cold.4 (0x1c0311848, 0x1c03118a0, the debug logs). Direct method. Only in education mode
// (eduModeEnabled inlined natively). The configuration is the given URL, else (for an identifier other than
// any-application) the first DefaultsConfigurations file under the system Library directories, else the main
// bundle's (current application) or, when writing to other applications' cloud preferences is enabled, the
// application record's bundle configuration. A dictionary configuration enables cloud sync for every key
// whose dictionary value is not NSUserDefaultsUbiquitous = false (a trailing "*" makes a prefix), and yields
// the configuration-file source and the store name (default: the identifier) when asked. The configuration
// path is answered only when some entry was a dictionary; every miss logs a debug message with the
// log-in-progress slot set. CoreFoundation defect: the native search loop leaks the DefaultsConfigurations
// directory URL of the directory that held the file (0x1c02af600-0x1c02af620 release the search path array
// and the file name only); the restoration releases it after the loop (confidence: verified).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_2_31 (0x1c02b4348).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_9_5 (0x1c02b43f0), _OUTLINED_FUNCTION_14_2
// (0x1c02b4440), _OUTLINED_FUNCTION_18_2 (0x1c02b4490).
- (OCFStringRef)copyCloudConfigurationWithURL:(OCFURLRef)url
                          outConfigFileSource:(OCFPrefsSource **)outConfigFileSource
                                 outStoreName:(OCFStringRef *)outStoreName {
    if (!eduModeEnabled()) {
        return NULL;
    }
    OCFURLRef configurationURL = NULL;
    OCFDataRef data = NULL;
    bool urlIsMissing;
    if (url != NULL) {
        configurationURL = OCFRetain(url);
    } else {
        if (_CFPrefsGetCacheStringForBundleID(kOCFPreferencesAnyApplication) ==
            _CFPrefsGetCacheStringForBundleID(_identifier)) {
            return NULL;
        }
        OCFStringRef fileName = _CFPrefsCreateConfigurationFileNameFromIdentifier(_identifier);
        if (fileName != NULL) {
            OCFArrayRef directories =
                OCFCopySearchPathForDirectoriesInDomains(kLibraryDirectory, kSystemDomainMask, false);
            OCFIndex count = OCFArrayGetCount(directories);
            for (OCFIndex index = 0; index < count; ++index) {
                OCFURLRef directory = OCFURLCreateCopyAppendingPathComponent(
                    kOCFAllocatorSystemDefault,
                    OCFArrayGetValueAtIndex(directories, index),
                    OF_STR("DefaultsConfigurations"),
                    true
                );
                configurationURL = OCFURLCreateCopyAppendingPathComponent(
                    kOCFAllocatorSystemDefault, directory, fileName, false
                );
                data = _CFDataCreateFromURL(configurationURL, NULL);
                if (data != NULL) {
                    OCFRelease(directory);
                    break;
                }
                OCFRelease(configurationURL);
                OCFRelease(directory);
                configurationURL = NULL;
            }
            OCFRelease(directories);
            OCFRelease(fileName);
        }
        if (data == NULL) {
            if (OCFEqual(_identifier, _CFPrefsGetCacheStringForBundleID(kOCFPreferencesCurrentApplication))) {
                OCFBundleRef mainBundle = OCFBundleGetMainBundle();
                if (mainBundle != NULL) {
                    if (configurationURL != NULL) {
                        OCFRelease(configurationURL);
                    }
                    configurationURL = _CFPrefsCopyCloudConfigurationURLFromBundle(mainBundle, _identifier);
                }
            } else if (_CFPrefsWritingToExternalAppCloudPreferencesEnabled()) {
                if (configurationURL != NULL) {
                    OCFRelease(configurationURL);
                }
                configurationURL = _CFPrefsCopyCloudConfigurationURLForIdentifier(_identifier);
            } else if (configurationURL != NULL) {
                OCFRelease(configurationURL);
                configurationURL = NULL;
            }
        }
    }
    if (data == NULL && configurationURL != NULL) {
        data = _CFDataCreateFromURL(configurationURL, NULL);
    }
    urlIsMissing = configurationURL == NULL;
    OCFStringRef result = NULL;
    bool found = false;
    if (data != NULL) {
        OCFDictionaryRef configuration =
            OCFPropertyListCreateWithData(kOCFAllocatorSystemDefault, data, 0, NULL, NULL);
        OCFRelease(data);
        if (configuration != NULL && OCFGetTypeID(configuration) != kDictionaryTypeID) {
            OCFRelease(configuration);
            configuration = NULL;
        }
        if (configuration != NULL) {
            _OCFSetTSD(kLogInProgressSlot, (void *)kOCFBooleanTrue, NULL);
            @try {
                os_log_t log = _CFPrefsClientLog();
                os_log_debug(
                    log,
                    "Found defaults configuration for %{public}@ at %{public}@",
                    (id)_identifier,
                    (id)configurationURL
                );
            } @finally {
                _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
            }
            OCFIndex count = OCFDictionaryGetCount(configuration);
            const void *keys[count];
            const void *values[count];
            size_t clearSize =
                (size_t)count * sizeof(void *) < 0x200 ? (size_t)count * sizeof(void *) : 0x200;
            bzero(keys, clearSize);
            bzero(values, clearSize);
            OCFDictionaryGetKeysAndValues(configuration, keys, values);
            const void **key = keys;
            const void **value = values;
            for (OCFIndex remaining = count; remaining >= 1; --remaining) {
                if (OCFGetTypeID(*value) == kDictionaryTypeID) {
                    if (OCFDictionaryGetValue(*value, OF_STR("NSUserDefaultsUbiquitous")) !=
                        kOCFBooleanFalse) {
                        OCFStringRef name = *key;
                        if (OCFStringHasSuffix(name, OF_STR("*"))) {
                            OCFStringRef prefix = OCFStringCreateWithSubstring(
                                kOCFAllocatorSystemDefault, name, (OCFRange){0, OCFStringGetLength(name) - 1}
                            );
                            [self setCloudEnabled:YES forKeyPrefix:prefix];
                            OCFRelease(prefix);
                        } else {
                            [self setCloudEnabled:YES forKey:name];
                        }
                    }
                    found = true;
                }
                ++key;
                ++value;
            }
            if (outConfigFileSource != NULL) {
                *outConfigFileSource = [[OCFPrefsConfigurationFileSource alloc]
                    initWithConfigurationPropertyList:configuration
                                containingPreferences:_containingPreferences];
            }
            if (outStoreName != NULL) {
                OCFStringRef storeName =
                    OCFDictionaryGetValue(configuration, OF_STR("NSUserDefaultsCloudStoreIdentifier"));
                if (storeName == NULL) {
                    storeName = _identifier;
                }
                *outStoreName = OCFStringCreateCopy(kOCFAllocatorSystemDefault, storeName);
            }
            OCFRelease(configuration);
        }
    }
    if (found) {
        OCFURLRef absolute = OCFURLCopyAbsoluteURL(configurationURL);
        if (absolute != NULL) {
            result = OCFURLCopyFileSystemPath(absolute, kPOSIXPathStyle);
            OCFRelease(absolute);
        }
    } else {
        _OCFSetTSD(kLogInProgressSlot, (void *)kOCFBooleanTrue, NULL);
        @try {
            os_log_t log = _CFPrefsClientLog();
            os_log_debug(
                log,
                "No defaults configuration file found for %{public}@ (looked at %{public}@)",
                (id)_identifier,
                (id)configurationURL
            );
        } @finally {
            _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
        }
    }
    if (!urlIsMissing) {
        OCFRelease(configurationURL);
    }
    return result;
}

// Native _CFPrefsCopyCloudConfigurationURLForIdentifier, 0x1c02afad4, with its .cold.1 (0x1c03118e4, the
// inlined log-handle dispatch_once) and .cold.2 (0x1c03118f8, the error log). Non-external.
// Under a lock, a per-identifier cache (created on first use with the CFType callbacks) answers retained
// URLs. A miss looks the application record up (soft-linked LSApplicationRecord, no placeholder) in an
// autorelease pool; a lookup error is logged with the log-in-progress slot set. The record's bundle container
// URL is retained across the pool, opened as a unique bundle, and its configuration URL (bundle identifier)
// is cached and answered. CoreFoundation defect: the native code never releases the +1 application record it
// allocates and initializes (no release between 0x1c02afb84 and the pool pops, in a unit compiled with manual
// reference counting); the restoration releases it before the pool is popped (confidence: verified).
static OCFURLRef _CFPrefsCopyCloudConfigurationURLForIdentifier(OCFStringRef identifier) {
    static os_unfair_lock cachedMappingLock = OS_UNFAIR_LOCK_INIT;
    static OCFMutableDictionaryRef cachedMapping;
    os_unfair_lock_lock(&cachedMappingLock);
    if (cachedMapping == NULL) {
        cachedMapping = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    OCFURLRef result = OCFDictionaryGetValue(cachedMapping, identifier);
    if (result != NULL) {
        OCFRetain(result);
        os_unfair_lock_unlock(&cachedMappingLock);
        return result;
    }
    OCFURLRef containerURL = NULL;
    @autoreleasepool {
        id error = nil;
        id record = [[getLSApplicationRecordClass() alloc] initWithBundleIdentifier:identifier
                                                                   allowPlaceholder:NO
                                                                              error:&error];
        if (record == nil) {
            if (error != nil) {
                _OCFSetTSD(kLogInProgressSlot, (void *)kOCFBooleanTrue, NULL);
                @try {
                    os_log_t log = _CFPrefsClientLog();
                    os_log_error(
                        log,
                        "Could not look up bundle for identifier %{public}@ due to: %{public}@",
                        (id)identifier,
                        error
                    );
                } @finally {
                    _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
                }
            }
        } else {
            containerURL = [record bundleContainerURL];
            if (containerURL != NULL) {
                OCFRetain(containerURL);
            }
            [record release];
        }
    }
    if (containerURL != NULL) {
        OCFBundleRef bundle = _OCFBundleCreateUnique(kOCFAllocatorSystemDefault, containerURL);
        OCFRelease(containerURL);
        if (bundle != NULL) {
            result = _CFPrefsCopyCloudConfigurationURLFromBundle(bundle, NULL);
            OCFRelease(bundle);
            if (result != NULL) {
                OCFDictionarySetValue(cachedMapping, identifier, result);
            }
        }
    }
    os_unfair_lock_unlock(&cachedMappingLock);
    return result;
}

// Native _CFPrefsClientLog, 0x1c02afd0c, with its .cold.1 (0x1c0311950, the dispatch_once of makeLogHandles).
// Hidden (was a private external). The client ("User Defaults") log handle.
os_log_t _CFPrefsClientLog(void) {
    makeLogHandles();
    return clientHandle;
}

// Native -[CFPrefsSearchListSource addSource:], 0x1c02b0358, with its .cold.1 (0x1c03119a4, the
// "!initialized" assertion, line 1406). Direct method. NULL is ignored. A source is appended once to the
// source list (created on first use with the CFType callbacks), and the generation count is invalidated with
// a release store of -1.
// Folds the compiler-outlined pieces -[CFPrefsSearchListSource initWithIdentifier:containingPreferences:]
// (0x1c02af28c).
- (void)addSource:(OCFPrefsSource *)source {
    if (source == nil) {
        return;
    }
    if (initialized) {
        __assert_rtn(
            "-[CFPrefsSearchListSource addSource:]", "CFPrefsSearchListSource.m", 1406, "!initialized"
        );
    }
    OCFMutableArrayRef sourceList = _sourceList;
    if (sourceList == NULL) {
        sourceList = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
        _sourceList = sourceList;
    }
    if (!OCFArrayContainsValue(sourceList, (OCFRange){0, OCFArrayGetCount(sourceList)}, source)) {
        OCFArrayAppendValue(_sourceList, source);
    }
    __atomic_store_n(&_generationCount, -1, __ATOMIC_RELEASE);
}

// Native -[CFPrefsSearchListSource setCloudEnabled:forKeyPrefix:], 0x1c0310728 (with OUTLINED_FUNCTION_19_2,
// _5_13, _17_2, _16_2 and _21_2). Direct method.
// Under the source lock: enabling appends the prefix (array created on first use with the CFType callbacks)
// and passes the flag to the cloud set target; disabling removes the prefix's first occurrence and passes the
// flag on only when no prefix remains.
- (void)setCloudEnabled:(BOOL)enabled forKeyPrefix:(OCFStringRef)prefix {
    os_unfair_lock_lock(&_lock);
    OCFMutableArrayRef prefixes = _cloudPrefixKeys;
    if (enabled) {
        if (prefixes == NULL) {
            prefixes = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
            _cloudPrefixKeys = prefixes;
        }
        OCFArrayAppendValue(prefixes, prefix);
        [(OCFPrefsCloudSource *)_cloudSetTarget setEnabled:enabled];
    } else if (prefixes != NULL) {
        OCFIndex index =
            OCFArrayGetFirstIndexOfValue(prefixes, (OCFRange){0, OCFArrayGetCount(prefixes)}, prefix);
        if (index != -1) {
            OCFArrayRemoveValueAtIndex(_cloudPrefixKeys, index);
            if (OCFArrayGetCount(_cloudPrefixKeys) == 0) {
                [(OCFPrefsCloudSource *)_cloudSetTarget setEnabled:enabled];
            }
        }
    }
    os_unfair_lock_unlock(&_lock);
}

// Native -[CFPrefsSearchListSource setCloudEnabled:forKey:], 0x1c03107e0 (with OUTLINED_FUNCTION_19_2, _7_9,
// _17_2 and _21_2). Direct method.
// Under the source lock: enabling adds the key (set created on first use with kCFCopyStringSetCallBacks) and
// passes the flag to the cloud set target; disabling removes the key and passes the flag on only when no key
// remains.
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_7_9 (0x1c02b43d0).
- (void)setCloudEnabled:(BOOL)enabled forKey:(OCFStringRef)key {
    os_unfair_lock_lock(&_lock);
    OCFMutableSetRef keys = _cloudKeys;
    if (enabled) {
        if (keys == NULL) {
            keys = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFCopyStringSetCallBacks);
            _cloudKeys = keys;
        }
        OCFSetAddValue(keys, key);
        [(OCFPrefsCloudSource *)_cloudSetTarget setEnabled:enabled];
    } else if (keys != NULL) {
        OCFSetRemoveValue(keys, key);
        if (OCFSetGetCount(_cloudKeys) == 0) {
            [(OCFPrefsCloudSource *)_cloudSetTarget setEnabled:enabled];
        }
    }
    os_unfair_lock_unlock(&_lock);
}

// Native -[CFPrefsSearchListSource addManagedSourceForIdentifier:user:], 0x1c0310b44, with
// ___62-[CFPrefsSearchListSource addManagedSourceForIdentifier:user:]_block_invoke (0x1c02b0350). Direct
// method. Adds the containing preferences' managed source for the pair.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_3_15 (0x1c02b435c), _OUTLINED_FUNCTION_20_2
// (0x1c02b44a0), _OUTLINED_FUNCTION_30_0 (0x1c02b4528).
- (void)addManagedSourceForIdentifier:(OCFStringRef)identifier user:(OCFStringRef)user {
    [_containingPreferences withManagedSourceForIdentifier:identifier
                                                      user:user
                                                   perform:^(OCFPrefsPlistSource *source) {
                                                     [self addSource:source];
                                                   }];
}

// Native -[CFPrefsSearchListSource addSimulatorBackstopSourceForIdentifier:], 0x1c0310ba4, with the
// ___72-[CFPrefsSearchListSource addSourceForIdentifier:user:byHost:container:]_block_invoke body (descriptor
// shared). Direct method.
// With a nonempty SIMULATOR_SHARED_RESOURCES_DIRECTORY (getenv), adds the current user's source for the
// identifier in that directory as container (not by host, not cloud).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_8_8 (0x1c02b43e0), _OUTLINED_FUNCTION_26_1
// (0x1c02b44f0), _OUTLINED_FUNCTION_27_1 (0x1c02b44fc).
- (void)addSimulatorBackstopSourceForIdentifier:(OCFStringRef)identifier {
    const char *directory = getenv("SIMULATOR_SHARED_RESOURCES_DIRECTORY");
    if (directory == NULL) {
        return;
    }
    size_t length = strlen(directory);
    if (length == 0) {
        return;
    }
    OCFURLRef url = OCFURLCreateFromFileSystemRepresentation(
        kOCFAllocatorSystemDefault, (const UInt8 *)directory, (OCFIndex)length, true
    );
    OCFStringRef path = OCFURLCopyFileSystemPath(url, kPOSIXPathStyle);
    OCFRelease(url);
    [_containingPreferences withSourceForIdentifier:identifier
                                               user:kOCFPreferencesCurrentUser
                                             byHost:NO
                                          container:path
                                              cloud:NO
                                            perform:^(OCFPrefsSource *source) {
                                              [self addSource:source];
                                            }];
    OCFRelease(path);
}

// Native -[CFPrefsSearchListSource addSourceForIdentifier:user:byHost:container:], 0x1c0310c94, with
// ___72-[CFPrefsSearchListSource addSourceForIdentifier:user:byHost:container:]_block_invoke (0x1c02b0420).
// Direct method. Adds the containing preferences' (non-cloud) source for the quadruple.
- (void)addSourceForIdentifier:(OCFStringRef)identifier
                          user:(OCFStringRef)user
                        byHost:(BOOL)byHost
                     container:(OCFStringRef)container {
    [_containingPreferences withSourceForIdentifier:identifier
                                               user:user
                                             byHost:byHost
                                          container:container
                                              cloud:NO
                                            perform:^(OCFPrefsSource *source) {
                                              [self addSource:source];
                                            }];
}

// Native -[CFPrefsSearchListSource addCloudSourceForIdentifier:configurationPath:storeName:container:],
// 0x1c0310cf8, with ___93-[CFPrefsSearchListSource addCloudSourceForIdentifier:configurationPath:storeName:
// container:]_block_invoke (0x1c02b0428). Direct method.
// The current user's cloud source (not by host) is configured with the path and the store name, then added.
- (void)addCloudSourceForIdentifier:(OCFStringRef)identifier
                  configurationPath:(OCFStringRef)configurationPath
                          storeName:(OCFStringRef)storeName
                          container:(OCFStringRef)container {
    [_containingPreferences
        withSourceForIdentifier:identifier
                           user:kOCFPreferencesCurrentUser
                         byHost:NO
                      container:container
                          cloud:YES
                        perform:^(OCFPrefsSource *source) {
                          [(OCFPrefsCloudSource *)source setConfigurationPath:configurationPath];
                          [(OCFPrefsCloudSource *)source setStoreName:storeName];
                          [self addSource:source];
                        }];
}

// Native -[CFPrefsSearchListSource addSuiteSourceForIdentifier:user:], 0x1c0310d8c, with
// ___60-[CFPrefsSearchListSource addSuiteSourceForIdentifier:user:]_block_invoke (0x1c02b0468). Direct
// method. Adds the suite search list for the pair; the search-list lock is already held (locked:YES).
- (void)addSuiteSourceForIdentifier:(OCFStringRef)identifier user:(OCFStringRef)user {
    [_containingPreferences withSuiteSearchListForIdentifier:identifier
                                                        user:user
                                                      locked:YES
                                                     perform:^(OCFPrefsSearchListSource *searchList) {
                                                       [self addSource:searchList];
                                                     }];
}

// Native -[CFPrefsSearchListSource addNamedVolatileSourceForIdentifier:], 0x1c0310df0, with
// ___63-[CFPrefsSearchListSource addNamedVolatileSourceForIdentifier:]_block_invoke (0x1c02b0704). Direct
// method. Adds the named volatile source.
- (void)addNamedVolatileSourceForIdentifier:(OCFStringRef)identifier {
    [_containingPreferences withNamedVolatileSourceForIdentifier:identifier
                                                         perform:^(OCFPrefsSource *source) {
                                                           [self addSource:source];
                                                         }];
}

// Native -[CFPrefsSearchListSource copyWithZone:], 0x1c02b0188. Listed method.
- (id)copyWithZone:(struct _NSZone *)zone {
    (void)zone;
    os_unfair_lock_lock(&_lock);
    OCFPrefsSearchListSource *copy = [[[self class] alloc] initWithIdentifier:_identifier
                                                        containingPreferences:_containingPreferences];
    if (copy != nil) {
        if (copy->_sourceList != NULL) {
            OCFRelease(copy->_sourceList);
        }
        copy->_sourceList = _sourceList != NULL
                                ? OCFArrayCreateMutableCopy(kOCFAllocatorSystemDefault, 0, _sourceList)
                                : NULL;
        if (copy->_observers != nil) {
            OCFRelease(copy->_observers);
        }
        copy->_observers = [_observers mutableCopy];
        if (copy->_cloudKeys != NULL) {
            OCFRelease(copy->_cloudKeys);
        }
        copy->_cloudKeys =
            _cloudKeys != NULL ? OCFSetCreateMutableCopy(kOCFAllocatorSystemDefault, 0, _cloudKeys) : NULL;
        if (_cloudPrefixKeys != NULL) {
            copy->_cloudPrefixKeys =
                OCFArrayCreateMutableCopy(kOCFAllocatorSystemDefault, 0, _cloudPrefixKeys);
        }
        copy->_standardSetTarget = _standardSetTarget;
        copy->_cloudSetTarget = _cloudSetTarget;
        if (_dict != NULL) {
            copy->_dict = OCFDictionaryCreateMutableCopy(kOCFAllocatorSystemDefault, 0, _dict);
        }
        if (_keysToSources != NULL) {
            copy->_keysToSources =
                OCFDictionaryCreateMutableCopy(kOCFAllocatorSystemDefault, 0, _keysToSources);
        }
    }
    os_unfair_lock_unlock(&_lock);
    return copy;
}

// Native -[CFPrefsSearchListSource lock], 0x1c02b0328. Listed, natively empty.
- (void)lock {
}

// Native -[CFPrefsSearchListSource unlock], 0x1c02b032c. Listed, natively empty.
- (void)unlock {
}

// Native -[CFPrefsSearchListSource alreadylocked_copyValueForKey:], 0x1c02b070c. Listed method.
// Folds .cold.1 through .cold.4 (0x1c03119cc, 0x1c03119e0, 0x1c03119f4, 0x1c0311a08).
// The log cleanup regions follow the rethrow paths (confidence: medium).
- (OCFPropertyListRef)alreadylocked_copyValueForKey:(OCFStringRef)key {
    if (_sourceList == NULL) {
        return NULL;
    }
    __block OCFPropertyListRef value = NULL;
    // Native ___57-[CFPrefsSearchListSource alreadylocked_copyValueForKey:]_block_invoke, 0x1c02b0a08.
    [self alreadylocked_getDictionary:^(OCFDictionaryRef dictionary) {
      if (dictionary != NULL) {
          value = OCFDictionaryGetValue(dictionary, key);
          if (value != NULL) {
              OCFRetain(value);
          }
      }
    }];
    makeLogHandles();
    if (os_log_type_enabled(clientHandle, OS_LOG_TYPE_DEBUG)) {
        const void *source = value != NULL ? OCFDictionaryGetValue(_keysToSources, key) : NULL;
        _OCFSetTSD(kLogInProgressSlot, (void *)kOCFBooleanTrue, NULL);
        @try {
            makeLogHandles();
            if (value != NULL) {
                os_log_debug(
                    clientHandle,
                    "looked up value %{private}@ for key %{private}@ in %{private}@ via %{private}@",
                    (id)value,
                    (id)key,
                    (id)source,
                    self
                );
            } else {
                os_log_debug(
                    clientHandle, "found no value for key %{private}@ in %{private}@", (id)key, self
                );
            }
        } @finally {
            _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
        }
    }
    return value;
}

// Native -[CFPrefsSearchListSource alreadylocked_setPrecopiedValues:forKeys:count:from:], 0x1c02b0a58.
// Listed method. Folds .cold.1 (0x1c0311a40) and .cold.2 (0x1c0311a68).
- (void)alreadylocked_setPrecopiedValues:(const void **)values
                                 forKeys:(OCFStringRef *)keys
                                   count:(OCFIndex)count
                                    from:(id)from {
    (void)from;
    if (_standardSetTarget == nil && _cloudSetTarget == nil) {
        __assert_rtn(
            "-[CFPrefsSearchListSource alreadylocked_setPrecopiedValues:forKeys:count:from:]",
            "CFPrefsSearchListSource.m",
            730,
            "_standardSetTarget || _cloudSetTarget"
        );
    }
    if (_dict == NULL) {
        // The native reload block is shared with -generationCount (0x1c02b2124).
        [self alreadylocked_getDictionary:^(OCFDictionaryRef dictionary) {
          (void)dictionary;
        }];
    }
    if (_keysToSources == NULL) {
        __assert_rtn(
            "-[CFPrefsSearchListSource alreadylocked_setPrecopiedValues:forKeys:count:from:]",
            "CFPrefsSearchListSource.m",
            736,
            "_keysToSources"
        );
    }
    unsigned char cloud[count != 0 ? (size_t)count : 1];
    bzero(cloud, (size_t)count);
    bool anyCloud = false;
    for (OCFIndex index = 0; index < count; ++index) {
        if (![self validateValue:values[index] forKey:keys[index] inDict:NULL forWriting:YES]) {
            keys[index] = NULL;
        } else if (_cloudKeys != NULL || _cloudPrefixKeys != NULL) {
            cloud[index] = [self alreadylocked_useCloudForKey:keys[index]];
            anyCloud |= cloud[index] != 0;
        }
    }
    bool affectsCache[count != 0 ? (size_t)count : 1];
    bzero(affectsCache, (size_t)count);
    OCFIndex standardIndex = OCFArrayGetFirstIndexOfValue(
        _sourceList, (OCFRange){0, OCFArrayGetCount(_sourceList)}, _standardSetTarget
    );
    OCFIndex cloudIndex = -1;
    if (anyCloud) {
        cloudIndex = OCFArrayGetFirstIndexOfValue(
            _sourceList, (OCFRange){0, OCFArrayGetCount(_sourceList)}, _cloudSetTarget
        );
    }
    bool anyCache = false;
    for (OCFIndex index = 0; index < count; ++index) {
        OCFTypeRef source = OCFDictionaryGetValue(_keysToSources, keys[index]);
        bool affects = true;
        if (source != NULL) {
            OCFIndex oldIndex = OCFArrayGetFirstIndexOfValue(
                _sourceList, (OCFRange){0, OCFArrayGetCount(_sourceList)}, source
            );
            affects = oldIndex == -1 || oldIndex >= (cloud[index] != 0 ? cloudIndex : standardIndex);
        }
        affectsCache[index] = affects;
        anyCache |= affects;
    }
    unsigned long observerCount = [(id<PreferencesUnrestoredMessages>)_observers approximateCount];
    OCFMutableDictionaryRef oldValues = NULL;
    if (_dict != NULL && observerCount != 0 && anyCache) {
        oldValues = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
        for (OCFIndex index = 0; index < count; ++index) {
            OCFTypeRef oldValue = OCFDictionaryGetValue(_dict, keys[index]);
            if (oldValue != NULL && affectsCache[index]) {
                OCFDictionarySetValue(oldValues, keys[index], oldValue);
            }
        }
    }
    if (anyCloud && [(OCFPrefsCloudSource *)_cloudSetTarget enabled]) {
        size_t pairCount = 2 * (size_t)count;
        if (((uint64_t)count >> 59) & 15) {
            OCFStringRef reason = OCFStringCreateWithFormat(
                NULL,
                NULL,
                OF_STR(
                    "*** attempt to create a temporary id buffer which is too large or with a negative count "
                    "(%lu) -- possibly data is corrupt"
                ),
                (unsigned long)pairCount
            );
            id exception = ((id (*)(Class, SEL, id, id, id))objc_msgSend)(
                (Class)(void *)searchListExceptionClass,
                @selector(exceptionWithName:reason:userInfo:),
                (id)ONSGenericException,
                (id)reason,
                nil
            );
            OCFRelease(reason);
            objc_exception_throw(exception);
        }
        size_t capacity = pairCount != 0 ? pairCount : 1;
        const void *stack[pairCount < 257 ? capacity : 1];
        size_t allocatedSize = 0;
        void *allocated = NULL;
        if (pairCount >= 257) {
            allocated = _OCFCreateArrayStorage(capacity, false, &allocatedSize);
        }
        OCFStringRef *selectedKeys = (OCFStringRef *)(pairCount < 257 ? stack : allocated);
        const void **selectedValues = (const void **)(selectedKeys + count);
        @try {
            OCFIndex selected = 0;
            for (OCFIndex index = 0; index < count; ++index) {
                if (cloud[index] == 1) {
                    selectedKeys[selected] = keys[index];
                    selectedValues[selected++] = values[index];
                }
            }
            [_cloudSetTarget setValues:selectedValues
                               forKeys:selectedKeys
                                 count:selected
                            copyValues:NO
                                  from:self];
            selected = 0;
            for (OCFIndex index = 0; index < count; ++index) {
                if ((cloud[index] & 1) == 0) {
                    selectedKeys[selected] = keys[index];
                    selectedValues[selected++] = values[index];
                }
            }
            [_standardSetTarget setValues:selectedValues
                                  forKeys:selectedKeys
                                    count:selected
                               copyValues:NO
                                     from:self];
        } @finally {
            free(allocated);
        }
    } else {
        [_standardSetTarget setValues:values forKeys:keys count:count copyValues:NO from:self];
    }
    if (!anyCache) {
        return;
    }
    for (OCFIndex index = 0; index < count; ++index) {
        OCFStringRef key = keys[index];
        if (!affectsCache[index] || key == NULL) {
            continue;
        }
        if (values[index] != NULL) {
            OCFDictionarySetValue(_dict, key, values[index]);
            OCFDictionarySetValue(
                _keysToSources, keys[index], cloud[index] == 1 ? _cloudSetTarget : _standardSetTarget
            );
        } else {
            OCFIndex sourceCount = _sourceList != NULL ? OCFArrayGetCount(_sourceList) : 0;
            const void *sources[sourceCount > 0 ? sourceCount : 1];
            if (sourceCount > 0) {
                OCFArrayGetValues(_sourceList, (OCFRange){0, sourceCount}, sources);
            }
            OCFIndex start = cloud[index] != 0 ? cloudIndex : standardIndex;
            for (OCFIndex sourceIndex = start + 1; sourceIndex < sourceCount; ++sourceIndex) {
                OCFPrefsSource *source = (OCFPrefsSource *)sources[sourceIndex];
                OCFPropertyListRef fallback = [source copyValueForKey:keys[index]];
                if (fallback != NULL) {
                    OCFDictionarySetValue(_dict, keys[index], fallback);
                    OCFDictionarySetValue(_keysToSources, keys[index], source);
                    OCFRelease(fallback);
                    break;
                }
                OCFDictionaryRemoveValue(_dict, keys[index]);
                OCFDictionaryRemoveValue(_keysToSources, keys[index]);
            }
        }
        if (observerCount != 0) {
            OCFTypeRef before = OCFDictionaryGetValue(oldValues, keys[index]);
            OCFTypeRef after = OCFDictionaryGetValue(_dict, keys[index]);
            [self deferredNotifyCausedByLocalWriteOfChangesToKey:keys[index] fromValue:before toValue:after];
        }
    }
    if (oldValues != NULL) {
        OCFRelease(oldValues);
    }
}

// Native -[CFPrefsSearchListSource handleReply:toRequestNewDataMessage:onConnection:retryCount:error:],
// 0x1c02b14b4. Listed method. Folds .cold.1, 0x1c0311a90. XPC GOT bindings follow OCFPrefsDaemon.m
// (confidence: medium).
- (void)handleReply:(xpc_object_t)reply
    toRequestNewDataMessage:(xpc_object_t)message
               onConnection:(xpc_connection_t)connection
                 retryCount:(int)retryCount
                      error:(bool *)error {
    (void)retryCount;
    (void)error;
    if (_sourceList == NULL) {
        return;
    }
    if (reply == NULL || xpc_get_type(reply) != XPC_TYPE_DICTIONARY) {
        for (OCFIndex index = 0; index < OCFArrayGetCount(_sourceList); ++index) {
            [(OCFPrefsSource *)OCFArrayGetValueAtIndex(_sourceList, index) generationCount];
        }
        return;
    }
    xpc_object_t requests = xpc_dictionary_get_value(message, "CFPreferencesMessages");
    xpc_object_t replies = xpc_dictionary_get_value(reply, "CFPreferencesMessages");
    if (replies == NULL) {
        return;
    }
    OCFIndex count = OCFArrayGetCount(_sourceList);
    if ((size_t)count != xpc_array_get_count(replies)) {
        __assert_rtn(
            "-[CFPrefsSearchListSource handleReply:toRequestNewDataMessage:onConnection:retryCount:error:]",
            "CFPrefsSearchListSource.m",
            905,
            "count == xpc_array_get_count(replyList)"
        );
    }
    // Native ___93-[CFPrefsSearchListSource
    // handleReply:toRequestNewDataMessage:onConnection:retryCount:error:]_block_invoke, 0x1c02b1614.
    // Non-external.
    xpc_array_apply(replies, ^bool(size_t index, xpc_object_t entry) {
      if (xpc_get_type(entry) != XPC_TYPE_NULL) {
          bool failed = false;
          OCFPrefsSource *source =
              (OCFPrefsSource *)OCFArrayGetValueAtIndex(self->_sourceList, (OCFIndex)index);
          os_unfair_lock_lock(&source->_lock);
          [source handleReply:entry
              toRequestNewDataMessage:xpc_array_get_value(requests, index)
                         onConnection:connection
                           retryCount:0
                                error:&failed];
          os_unfair_lock_unlock(&source->_lock);
      }
      return true;
    });
}

// Native -[CFPrefsSearchListSource createRequestNewContentMessageForDaemon:], 0x1c02b1704. Listed method.
- (xpc_object_t)createRequestNewContentMessageForDaemon:(int)role {
    if (_sourceList == NULL) {
        return NULL;
    }
    OCFIndex count = OCFArrayGetCount(_sourceList);
    const void *entries[count > 0 ? count : 1];
    OCFArrayGetValues(_sourceList, (OCFRange){0, count}, entries);
    xpc_object_t null = xpc_null_create();
    bool any = false;
    for (OCFIndex index = 0; index < count; ++index) {
        OCFPrefsSource *source = (OCFPrefsSource *)entries[index];
        os_unfair_lock_lock(&source->_lock);
        xpc_object_t request = [source createRequestNewContentMessageForDaemon:role];
        entries[index] = request != NULL ? request : null;
        os_unfair_lock_unlock(&source->_lock);
        any |= entries[index] != null;
    }
    xpc_object_t message = NULL;
    if (any) {
        xpc_object_t requests = xpc_array_create((xpc_object_t *)entries, (size_t)count);
        message = [self createMultiMessageWithContainedMessages:requests];
        xpc_release(requests);
        for (OCFIndex index = 0; index < count; ++index) {
            if (entries[index] != null) {
                xpc_release((xpc_object_t)entries[index]);
            }
        }
    }
    xpc_release(null);
    return message;
}

// Native CFPREFERENCES_IS_WAITING_FOR_SYSTEM_AND_USER_CFPREFSDS, 0x1c02b18b0. Hidden (was private external).
void CFPREFERENCES_IS_WAITING_FOR_SYSTEM_AND_USER_CFPREFSDS(
    _OCFXPreferences *preferences, void (^block)(void)
) {
    os_unfair_lock_assert_not_owner(&preferences->_sourcesLock);
    os_unfair_lock_assert_not_owner(&preferences->_searchListsLock);
    os_unfair_lock_assert_not_owner(&preferences->_namedVolatileSourcesLock);
    block();
}

// Native generationCountFromListOfSources, 0x1c02b1cf8. Non-external.
static int64_t generationCountFromListOfSources(const void **sources, OCFIndex count, BOOL *stale) {
    uint64_t generation = 0;
    for (OCFIndex index = 0; index < count; ++index) {
        OCFPrefsSource *source = (OCFPrefsSource *)sources[index];
        if (source->_isSearchList == YES) {
            OCFPrefsSearchListSource *searchList = (OCFPrefsSearchListSource *)source;
            if (searchList->_sourceList != NULL) {
                OCFIndex nestedCount = OCFArrayGetCount(searchList->_sourceList);
                if (nestedCount > 0) {
                    const void *nested[nestedCount];
                    OCFArrayGetValues(searchList->_sourceList, (OCFRange){0, nestedCount}, nested);
                    generation += (uint64_t)generationCountFromListOfSources(nested, nestedCount, stale);
                }
            }
            continue;
        }
        uint32_t *entry = __atomic_load_n(&source->shmemEntry, __ATOMIC_ACQUIRE);
        bool needsRefresh = true;
        if (entry != NULL) {
            uint32_t current = __atomic_load_n(entry, __ATOMIC_ACQUIRE);
            uint32_t sentinel = __atomic_load_n(&sentinelGeneration, __ATOMIC_ACQUIRE);
            needsRefresh = current != sentinel &&
                           current != __atomic_load_n(&source->lastKnownShmemState, __ATOMIC_ACQUIRE);
        }
        generation += (uint64_t)__atomic_load_n(&source->_generationCount, __ATOMIC_ACQUIRE);
        if (needsRefresh) {
            *stale = YES;
        }
    }
    return (int64_t)generation;
}

// Native -[CFPrefsSearchListSource handleChangeNotificationForDomainIdentifier:isRemote:], 0x1c02b1f34.
// Listed method.
- (void)handleChangeNotificationForDomainIdentifier:(OCFStringRef)identifier isRemote:(BOOL)isRemote {
    if (_sourceList == NULL) {
        return;
    }
    if (OCFEqual(identifier, OF_STR("Volatile"))) {
        [self generationCount];
        return;
    }
    if (isRemote && _identifier != identifier && !OCFEqual(_identifier, identifier)) {
        return;
    }
    OCFIndex count = _sourceList != NULL ? OCFArrayGetCount(_sourceList) : 0;
    const void *sources[count > 0 ? count : 1];
    if (count > 0) {
        OCFArrayGetValues(_sourceList, (OCFRange){0, count}, sources);
    }
    BOOL stale = NO;
    int64_t generation = generationCountFromListOfSources(sources, count, &stale);
    if (stale || (!isRemote && generation != __atomic_load_n(&_generationCount, __ATOMIC_ACQUIRE))) {
        [self generationCount];
    }
}

// Native -[CFPrefsSearchListSource generationCount], 0x1c02b20bc. Listed method. Folds .cold.1, 0x1c0311ac4,
// including OUTLINED_FUNCTION_23_2 (0x1c02b44c8), its epilogue.
- (int64_t)generationCount {
    if (_sourceList == NULL) {
        return 0;
    }
    os_unfair_lock_lock(&_lock);
    // Native ___59-[CFPrefsSearchListSource alreadylocked_reloadCacheIfStale]_block_invoke, 0x1c02b2124.
    [self alreadylocked_getDictionary:^(OCFDictionaryRef dictionary) {
      (void)dictionary;
    }];
    os_unfair_lock_unlock(&_lock);
    return __atomic_load_n(&_generationCount, __ATOMIC_ACQUIRE);
}

// Native -[CFPrefsSearchListSource alreadylocked_copyDictionary], 0x1c02b2128. Listed method.
- (OCFMutableDictionaryRef)alreadylocked_copyDictionary {
    __block OCFMutableDictionaryRef copy = NULL;
    // Native ___55-[CFPrefsSearchListSource alreadylocked_copyDictionary]_block_invoke, 0x1c02b21f4.
    [self alreadylocked_getDictionary:^(OCFDictionaryRef dictionary) {
      if (dictionary != NULL) {
          copy = OCFDictionaryCreateMutableCopy(kOCFAllocatorSystemDefault, 0, dictionary);
      }
    }];
    return copy;
}

// Native -[CFPrefsSearchListSource _getPendingNotifications], 0x1c02b2238. Direct method.
// Folds .cold.1, 0x1c0311b18, and .cold.2, 0x1c0311b40.
- (OCFMutableArrayRef)_getPendingNotifications {
    void *pending = _OCFGetTSD(16);
    if (pending == (void *)16) {
        pending = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
        _OCFSetTSD(16, pending, pendingKVODestructor);
    } else if (pending == NULL) {
        __assert_rtn(
            "-[CFPrefsSearchListSource _getPendingNotifications]",
            "CFPrefsSearchListSource.m",
            1309,
            "pendingNotifications"
        );
    }
    return pending;
}

// Native -[CFPrefsSearchListSource deferredNotifyCausedByLocalWriteOfChangesToKey:fromValue:toValue:],
// 0x1c02b22a4. Listed method.
- (void)deferredNotifyCausedByLocalWriteOfChangesToKey:(OCFStringRef)key
                                             fromValue:(OCFPropertyListRef)oldValue
                                               toValue:(OCFPropertyListRef)newValue {
    os_unfair_lock_assert_owner(&_lock);
    const void *keys[] = {key};
    const void *oldValues[] = {oldValue};
    const void *newValues[] = {newValue};
    OCFDictionaryRef before = oldValue != NULL ? OCFDictionaryCreate(
                                                     kOCFAllocatorSystemDefault,
                                                     keys,
                                                     oldValues,
                                                     1,
                                                     &kOCFTypeDictionaryKeyCallBacks,
                                                     &kOCFTypeDictionaryValueCallBacks
                                                 )
                                               : NULL;
    OCFDictionaryRef after = newValue != NULL ? OCFDictionaryCreate(
                                                    kOCFAllocatorSystemDefault,
                                                    keys,
                                                    newValues,
                                                    1,
                                                    &kOCFTypeDictionaryKeyCallBacks,
                                                    &kOCFTypeDictionaryValueCallBacks
                                                )
                                              : NULL;
    _OCFPrefsPendingKVONotification *notification = [[_OCFPrefsPendingKVONotification alloc] init];
    [notification setSource:self];
    [notification setValues:after];
    [notification setOldValues:before];
    if (before != NULL) {
        OCFRelease(before);
    }
    if (after != NULL) {
        OCFRelease(after);
    }
    OCFArrayAppendValue([self _getPendingNotifications], notification);
    [notification release];
}

// Native -[CFPrefsSearchListSource deferredNotifyCausedByLoadingOfChangesFromDictionary:toDictionary:],
// 0x1c02b23e4. Listed method.
- (void)deferredNotifyCausedByLoadingOfChangesFromDictionary:(OCFDictionaryRef)oldValues
                                                toDictionary:(OCFDictionaryRef)newValues {
    os_unfair_lock_assert_owner(&_lock);
    if (oldValues != NULL) {
        OCFRetain(oldValues);
    }
    OCFDictionaryRef after =
        newValues != NULL ? OCFDictionaryCreateCopy(kOCFAllocatorSystemDefault, newValues) : NULL;
    _OCFPrefsPendingKVONotification *notification = [[_OCFPrefsPendingKVONotification alloc] init];
    [notification setSource:self];
    [notification setValues:after];
    [notification setOldValues:oldValues];
    if (oldValues != NULL) {
        OCFRelease(oldValues);
    }
    if (after != NULL) {
        OCFRelease(after);
    }
    OCFArrayAppendValue([self _getPendingNotifications], notification);
    [notification release];
}

// Native -[CFPrefsSearchListSource domainIdentifier], 0x1c02b25e0. Listed method.
- (OCFStringRef)domainIdentifier {
    return _identifier;
}

// Native -[CFPrefsSearchListSource debugDescription], 0x1c02b25f0. Listed method.
// The super selector at 0x2387aa04d is inferred to be description (confidence: medium).
- (id)debugDescription {
    OCFMutableStringRef description = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    os_unfair_lock_lock(&_lock);
    OCFIndex count = _sourceList != NULL ? OCFArrayGetCount(_sourceList) : 0;
    if (count > 0) {
        const void *sources[count];
        OCFArrayGetValues(_sourceList, (OCFRange){0, count}, sources);
        OCFStringAppend(description, (OCFStringRef)[super description]);
        OCFStringAppend(description, OF_STR("Sources: [\n"));
        for (OCFIndex index = 0; index < count; ++index) {
            void *pool = _OCFAutoreleasePoolPush();
            OCFStringAppend(description, OF_STR("    "));
            OCFStringAppend(description, (OCFStringRef)[(id)sources[index] description]);
            OCFStringAppend(description, OF_STR("\n"));
            _OCFAutoreleasePoolPop(pool);
        }
        OCFStringAppend(description, OF_STR("]\n"));
    } else {
        OCFStringAppend(description, OF_STR("Sources: []\n"));
    }
    OCFStringAppend(description, OF_STR("Cloud Keys: ["));
    OCFIndex keyCount = _cloudKeys != NULL ? OCFSetGetCount(_cloudKeys) : 0;
    const void *keys[keyCount > 1 ? keyCount : 1];
    size_t bytes = sizeof(keys);
    bzero(keys, bytes < 512 ? bytes : 512);
    if (keyCount > 0) {
        OCFSetGetValues(_cloudKeys, keys);
        for (OCFIndex index = 0; index < keyCount; ++index) {
            OCFStringAppend(description, keys[index]);
            OCFStringAppend(description, OF_STR(", "));
        }
    }
    OCFStringAppend(description, OF_STR("]\n"));
    os_unfair_lock_unlock(&_lock);
    return (id)OCFAutorelease(description);
}

// Native -[CFPrefsSearchListSource createMultiMessageWithContainedMessages:], 0x1c0310884. Direct method.
- (xpc_object_t)createMultiMessageWithContainedMessages:(xpc_object_t)messages {
    xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(message, "CFPreferencesOperation", 5);
    xpc_dictionary_set_value(message, "CFPreferencesMessages", messages);
    return message;
}

// Native -[CFPrefsSearchListSource
// synchronouslySendSystemMessage:andUserMessage:andDirectMessage:replyHandler:], 0x1c03108e8. Direct method.
// Folds OUTLINED_FUNCTION_10_4 (0x1c02b4404), _11_4 (0x1c02b4410), _12_4 (0x1c02b441c), _22_2 (0x1c02b44b8)
// and _28_1 (0x1c02b4508). OUTLINED_FUNCTION_6_11 (0x1c02b43a8) sets up the signpost record.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_0_51 (0x1c02b4320), _OUTLINED_FUNCTION_4_13
// (0x1c02b437c).
- (void)synchronouslySendSystemMessage:(xpc_object_t)system
                        andUserMessage:(xpc_object_t)user
                      andDirectMessage:(xpc_object_t)direct
                          replyHandler:
                              (void (^)(
                                  xpc_object_t message, xpc_object_t reply, xpc_connection_t connection
                              ))replyHandler {
    // Native ..._block_invoke, 0x1c02b1b08.
    void (^send)(xpc_object_t, int) = ^(xpc_object_t message, int role) {
      // Native ..._block_invoke_2, 0x1c02b1ba0.
      [self->_containingPreferences
          withConnectionForRole:role
              andUserIdentifier:NULL
                   performBlock:^(xpc_connection_t connection) {
                     if (connection != NULL) {
                         xpc_object_t reply =
                             xpc_connection_send_message_with_reply_sync(connection, message);
                         if (replyHandler != NULL) {
                             replyHandler(message, reply, connection);
                         }
                         xpc_release(reply);
                     }
                   }];
    };
    uint64_t start = mach_continuous_time();
    if (system != NULL && user != NULL) {
        // Native ..._block_invoke_3, 0x1c02b1c00.
        CFPREFERENCES_IS_WAITING_FOR_SYSTEM_AND_USER_CFPREFSDS(_containingPreferences, ^{
          // Native ..._block_invoke_4, 0x1c02b1c90.
          dispatch_apply(2, DISPATCH_APPLY_AUTO, ^(size_t index) {
            send(index == 0 ? system : user, index == 0 ? 2 : 1);
          });
        });
        uint64_t end = mach_continuous_time();
        os_log_t log = _CFPrefsSignpostsLogForDuration(end - start);
        os_signpost_id_t signpost = os_signpost_id_make_with_pointer(log, self);
        os_signpost_event_emit(
            log,
            signpost,
            "sendSystemAndUserMessage",
            "Domain: %{public}@ %{public, signpost.description:begin_time}llu %{public, "
            "signpost.description:end_time}llu",
            (id)_identifier,
            start,
            end
        );
    } else if (system != NULL) {
        // Native ..._block_invoke.37, 0x1c02b1cbc.
        CFPREFERENCES_IS_WAITING_FOR_SYSTEM_CFPREFSD(_containingPreferences, ^{
          send(system, 2);
        });
        uint64_t end = mach_continuous_time();
        os_log_t log = _CFPrefsSignpostsLogForDuration(end - start);
        os_signpost_id_t signpost = os_signpost_id_make_with_pointer(log, self);
        os_signpost_event_emit(
            log,
            signpost,
            "sendSystemMessage",
            "Domain: %{public}@ %{public, signpost.description:begin_time}llu %{public, "
            "signpost.description:end_time}llu",
            (id)_identifier,
            start,
            end
        );
    } else if (user != NULL) {
        // Native ..._block_invoke.38, 0x1c02b1cd0.
        CFPREFERENCES_IS_WAITING_FOR_USER_CFPREFSD(_containingPreferences, ^{
          send(user, 1);
        });
        uint64_t end = mach_continuous_time();
        os_log_t log = _CFPrefsSignpostsLogForDuration(end - start);
        os_signpost_id_t signpost = os_signpost_id_make_with_pointer(log, self);
        os_signpost_event_emit(
            log,
            signpost,
            "sendUserMessage",
            "Domain: %{public}@ %{public, signpost.description:begin_time}llu %{public, "
            "signpost.description:end_time}llu",
            (id)_identifier,
            start,
            end
        );
    }
    if (direct != NULL) {
        // Native ..._block_invoke.39, 0x1c02b1ce4.
        CFPREFERENCES_IS_WAITING_FOR_DIRECT_CFPREFSD(_containingPreferences, ^{
          send(direct, 3);
        });
    }
}

// Native -[CFPrefsSearchListSource alreadylocked_getDictionary:], 0x1c0310e50. Direct method. Folds
// OUTLINED_FUNCTION_13_4 (0x1c02b4428), _15_2 (0x1c02b4454) and _29_0 (0x1c02b4514).
- (void)alreadylocked_getDictionary:(void (^)(OCFDictionaryRef dictionary))block {
    os_unfair_lock_assert_owner(&_lock);
    if (_sourceList == NULL) {
        OCFDictionaryRef empty = OCFDictionaryCreate(
            kOCFAllocatorSystemDefault,
            NULL,
            NULL,
            0,
            &kOCFTypeDictionaryKeyCallBacks,
            &kOCFTypeDictionaryValueCallBacks
        );
        block(empty);
        OCFRelease(empty);
        return;
    }
    if (_OCFPrefsTestingFlags[1] != 0) {
        __atomic_store_n(&_generationCount, -1, __ATOMIC_RELEASE);
        if (_dict != NULL) {
            OCFRelease(_dict);
        }
        _dict = NULL;
    }
    OCFIndex count = _sourceList != NULL ? OCFArrayGetCount(_sourceList) : 0;
    const void *sources[count > 1 ? count : 1];
    if (count > 0) {
        OCFArrayGetValues(_sourceList, (OCFRange){0, count}, sources);
    }
    int64_t generation = [self alreadylocked_generationCountFromListOfSources:sources count:count];
    OCFDictionaryRef retained;
    if (generation == __atomic_load_n(&_generationCount, __ATOMIC_ACQUIRE)) {
        if (_dict == NULL) {
            _dict = OCFDictionaryCreateMutable(
                kOCFAllocatorSystemDefault,
                0,
                &kOCFTypeDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
        }
        retained = OCFRetain(_dict);
    } else {
        OCFMutableDictionaryRef values = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
        OCFMutableDictionaryRef origins = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
        // Native ___55-[CFPrefsSearchListSource alreadylocked_getDictionary:]_block_invoke, 0x1c0311520.
        BOOL (^evaluate)(OCFStringRef) = ^BOOL(OCFStringRef key) {
          return [self alreadylocked_useCloudForKey:key];
        };
        for (OCFIndex index = 0; index < count; ++index) {
            [(OCFPrefsSource *)sources[index] mergeIntoDictionary:values
                                                 sourceDictionary:origins
                                                cloudKeyEvaluator:evaluate];
        }
        OCFDictionaryRef oldValues = _dict;
        _dict = values;
        OCFDictionaryRef oldOrigins = _keysToSources;
        _keysToSources = origins;
        __atomic_store_n(&_generationCount, generation, __ATOMIC_RELEASE);
        retained = OCFRetain(_dict);
        if ([(id<PreferencesUnrestoredMessages>)_observers approximateCount] != 0 &&
            (oldValues != NULL || _dict != NULL)) {
            [self deferredNotifyCausedByLoadingOfChangesFromDictionary:oldValues toDictionary:_dict];
        }
        if (oldValues != NULL) {
            OCFRelease(oldValues);
        }
        if (oldOrigins != NULL) {
            OCFRelease(oldOrigins);
        }
    }
    block(retained);
    OCFRelease(retained);
}

// Native -[CFPrefsSearchListSource alreadylocked_useCloudForKey:], 0x1c0311168. Direct method.
- (BOOL)alreadylocked_useCloudForKey:(OCFStringRef)key {
    if (_cloudKeys != NULL && OCFSetContainsValue(_cloudKeys, key)) {
        return YES;
    }
    if (_cloudPrefixKeys != NULL) {
        OCFIndex count = OCFArrayGetCount(_cloudPrefixKeys);
        for (OCFIndex index = 0; index < count; ++index) {
            if (OCFStringHasPrefix(key, OCFArrayGetValueAtIndex(_cloudPrefixKeys, index))) {
                return YES;
            }
        }
    }
    return NO;
}

// Native -[CFPrefsSearchListSource alreadylocked_requestNewData], 0x1c0311384. Direct method.
- (BOOL)alreadylocked_requestNewData {
    if (_sourceList != NULL) {
        OCFIndex count = OCFArrayGetCount(_sourceList);
        for (OCFIndex index = 0; index < count; ++index) {
            if (![(OCFPrefsSource *)OCFArrayGetValueAtIndex(_sourceList, index) synchronize]) {
                return NO;
            }
        }
    }
    return YES;
}

// Native -[CFPrefsSearchListSource synchronize], 0x1c03113f8. Listed method.
- (BOOL)synchronize {
    return [self alreadylocked_requestNewData];
}

// Native -[CFPrefsSearchListSource alreadylocked_generationCountFromListOfSources:count:], 0x1c03113fc.
// Direct method.
- (int64_t)alreadylocked_generationCountFromListOfSources:(const void **)sources count:(OCFIndex)count {
    BOOL stale = NO;
    int64_t generation = generationCountFromListOfSources(sources, count, &stale);
    if (stale == YES) {
        xpc_object_t system = [self createRequestNewContentMessageForDaemon:2];
        xpc_object_t user = [self createRequestNewContentMessageForDaemon:1];
        xpc_object_t direct = [self createRequestNewContentMessageForDaemon:3];
        // Native ___80-[CFPrefsSearchListSource
        // alreadylocked_generationCountFromListOfSources:count:]_block_invoke, 0x1c02b1ed4. Non-external.
        [self synchronouslySendSystemMessage:system
                              andUserMessage:user
                            andDirectMessage:direct
                                replyHandler:^(
                                    xpc_object_t message, xpc_object_t reply, xpc_connection_t connection
                                ) {
                                  bool failed = false;
                                  [self handleReply:reply
                                      toRequestNewDataMessage:message
                                                 onConnection:connection
                                                   retryCount:0
                                                        error:&failed];
                                }];
        if (system != NULL) {
            xpc_release(system);
        }
        if (user != NULL) {
            xpc_release(user);
        }
        if (direct != NULL) {
            xpc_release(direct);
        }
        generation = generationCountFromListOfSources(sources, count, &stale);
    }
    return generation;
}

// Native -[CFPrefsSearchListSource descriptionRedactingContainer:], 0x1c03115d0. Direct method.
// GOT 0x22229c638 is NSString (binding confidence: medium, see OCFPDSource.m).
- (id)descriptionRedactingContainer:(BOOL)redact {
    uint32_t persona = [_containingPreferences nonLaunchPersonaUID];
    OCFStringRef container = [_standardSetTarget container];
    if (redact) {
        container = CFPreferencesRedactedContainerPath(container);
    }
    Class stringClass = (Class)(void *)searchListStringClass;
    const char *className = object_getClassName(self);
    OCFStringRef domain = [self domainIdentifier];
    if (persona == UINT32_MAX) {
        return ((id (*)(Class, SEL, OCFStringRef, ...))objc_msgSend)(
            stringClass,
            @selector(stringWithFormat:),
            OF_STR("%s<%p> (Domain: %@, Container: %@)"),
            className,
            self,
            domain,
            container
        );
    }
    return ((id (*)(Class, SEL, OCFStringRef, ...))objc_msgSend)(
        stringClass,
        @selector(stringWithFormat:),
        OF_STR("%s<%p> (Domain: %@, Container: %@ Non-launch persona: %d)"),
        className,
        self,
        domain,
        container,
        (int32_t)persona
    );
}

// Native -[CFPrefsSearchListSource description], 0x1c0311694. Listed method.
- (id)description {
    return [self descriptionRedactingContainer:NO];
}

// Native -[CFPrefsSearchListSource redactedDescription], 0x1c031169c. Listed method.
- (id)redactedDescription {
    return [self descriptionRedactingContainer:YES];
}

@end

// Native __CFPrefsOverrideEnableWritingToExternalAppCloudPreferences (__DATA,__bss 0x22e6d3959).
// Non-external; its other writer, _CFPrefsEnableWritingToExternalAppCloudPreferences (0x1c02af460), is
// outside this restoration.
static bool _CFPrefsOverrideEnableWritingToExternalAppCloudPreferences;

@implementation _OCFXPreferences (SearchListAdditions)

// Native -[_CFXPreferences withSuiteSearchListForIdentifier:user:locked:perform:], 0x1c02b0470, with
// ___93-[_CFXPreferences(SearchListAdditions) withSuiteSearchListForIdentifier:user:locked:perform:]
// _block_invoke (0x1c02b3bc0). Direct method.
// The user is canonicalized (_CFPrefsCopyUserConstantForUserName inlined). The "suite/<cache string>/<user>"
// search list is looked up (on the search-list dictionary directly when locked, else under its lock through
// withSearchLists:, inlined natively), created as an initialized CFPrefsSuiteSearchListSource when missing
// (stored even when creation answers nil), and retained. The block receives it; the release and the user's
// release form the @finally (region bounds confidence medium). The __block domain variable is written by the
// block and not read afterwards, as natively.
- (void)withSuiteSearchListForIdentifier:(OCFStringRef)identifier
                                    user:(OCFStringRef)user
                                  locked:(BOOL)locked
                                 perform:(void (^)(OCFPrefsSearchListSource *searchList))block {
    __block OCFStringRef domain = identifier;
    __block OCFPrefsSearchListSource *searchList = nil;
    OCFStringRef suiteUser = _CFPrefsCopyUserConstantForUserName(user);
    void (^lookUp)(OCFMutableDictionaryRef) = ^(OCFMutableDictionaryRef searchLists) {
      domain = _CFPrefsGetCacheStringForBundleID(identifier);
      OCFStringRef key = OCFStringCreateWithFormat(
          kOCFAllocatorSystemDefault,
          NULL,
          OF_STR("suite/%@/%@"),
          _CFPrefsGetCacheStringForBundleID(identifier),
          suiteUser
      );
      searchList = (OCFPrefsSearchListSource *)OCFDictionaryGetValue(searchLists, key);
      if (searchList == nil) {
          searchList = [[OCFPrefsSuiteSearchListSource alloc] initWithIdentifier:identifier
                                                           containingPreferences:self];
          if (searchList != nil) {
              searchList->initialized = YES;
          }
          OCFDictionarySetValue(searchLists, key, searchList);
          [searchList release];
      }
      [searchList retain];
      OCFRelease(key);
    };
    if (locked) {
        lookUp(_searchLists);
    } else {
        [self withSearchLists:lookUp];
    }
    @try {
        block(searchList);
    } @finally {
        [searchList release];
        OCFRelease(suiteUser);
    }
    (void)domain;
}

// Native CFPREFERENCES_IS_WAITING_FOR_SYSTEM_CFPREFSD, 0x1c02b1918. Hidden (was a private external); hidden
// until the owner records a naming decision. None of the preferences' sources, search-list and
// named-volatile-sources locks may be owned; the block is then called (a tail call), so that a thread waiting
// for the system daemon shows this frame.
void CFPREFERENCES_IS_WAITING_FOR_SYSTEM_CFPREFSD(_OCFXPreferences *preferences, void (^block)(void)) {
    os_unfair_lock_assert_not_owner(&preferences->_sourcesLock);
    os_unfair_lock_assert_not_owner(&preferences->_searchListsLock);
    os_unfair_lock_assert_not_owner(&preferences->_namedVolatileSourcesLock);
    block();
}

// Native CFPREFERENCES_IS_WAITING_FOR_USER_CFPREFSD, 0x1c02b1980. As the system variant, for the user session
// daemon.
void CFPREFERENCES_IS_WAITING_FOR_USER_CFPREFSD(_OCFXPreferences *preferences, void (^block)(void)) {
    os_unfair_lock_assert_not_owner(&preferences->_sourcesLock);
    os_unfair_lock_assert_not_owner(&preferences->_searchListsLock);
    os_unfair_lock_assert_not_owner(&preferences->_namedVolatileSourcesLock);
    block();
}

// Native CFPREFERENCES_IS_WAITING_FOR_DIRECT_CFPREFSD, 0x1c02b19e8. As the system variant, for the direct
// (in-process) daemon.
void CFPREFERENCES_IS_WAITING_FOR_DIRECT_CFPREFSD(_OCFXPreferences *preferences, void (^block)(void)) {
    os_unfair_lock_assert_not_owner(&preferences->_sourcesLock);
    os_unfair_lock_assert_not_owner(&preferences->_searchListsLock);
    os_unfair_lock_assert_not_owner(&preferences->_namedVolatileSourcesLock);
    block();
}

// Native _CFPrefsDaemonLog, 0x1c02b1a50, with its .cold.1 (0x1c0311ab8, the dispatch_once of makeLogHandles).
// Hidden (was a private external). The daemon ("cfprefsd") log handle.
os_log_t _CFPrefsDaemonLog(void) {
    makeLogHandles();
    return daemonHandle;
}

// Native _CFPrefsLongLivedLog, 0x1c02b1a80, with its .cold.1 (0x1c0311abc, the dispatch_once of
// makeLogHandles through OUTLINED_FUNCTION_1_37). Hidden (was a private external). The audit log handle.
os_log_t _CFPrefsLongLivedLog(void) {
    makeLogHandles();
    return longLivedLogHandle;
}

// Native _CFPrefsSignpostsLogForDuration, 0x1c02b1ab0, with its .cold.1 (0x1c0311ac0, the dispatch_once of
// makeLogHandles through OUTLINED_FUNCTION_1_37). Hidden (was a private external). A duration (in mach
// continuous time units) below the threshold (one millisecond) answers the detailed signpost handle; any
// other the signpost handle (unsigned compare).
os_log_t _CFPrefsSignpostsLogForDuration(uint64_t duration) {
    makeLogHandles();
    return duration < signpostHandleMinimumThreshold ? signpostDetailedHandle : signpostHandle;
}

// Native normalizeQuintuplet, 0x1c02b2be8, with its .cold.1 (0x1c0311c34, the assertion "!cloud",
// CFPrefsSearchListSource.m line 62). Non-external natively (hidden here: Preferences/OCFPrefsSource.m's
// units call it). With a container, a container under /private/var/containers/Shared/SystemGroup or
// /var/containers/Shared/SystemGroup makes the user kCFPreferencesAnyUser and sets byHost. Without one, an
// identifier starting with '/' (character 0 is read without a length check, as natively) is a path: cloud
// asserts, and _CFPrefsExtractQuadrupleFromPathIfPossible fills the identifier, user, container and byHost
// (its managed and failed flags are discarded). When no identifier was extracted, an identifier ending in
// ".plist" loses that suffix (a copy, released at the end). The extracted container, identifier and user
// replace the given ones when not NULL. The user is mapped through the container
// (_CFPrefsCopyUserForContainer, +1); an AnyUser result sets byHost. The block receives the identifier's
// cache string, that user, byHost, the container and cloud; afterwards the identifier copy, the extracted
// container and user, and the mapped user are released, each when not NULL.
void normalizeQuintuplet(
    OCFStringRef identifier,
    OCFStringRef user,
    bool byHost,
    OCFStringRef container,
    bool cloud,
    void (^block)(OCFStringRef identifier, OCFStringRef user, bool byHost, OCFStringRef container, bool cloud)
) {
    bool normalizedByHost = byHost;
    OCFStringRef extractedUser = NULL;
    OCFStringRef extractedIdentifier = NULL;
    OCFStringRef extractedContainer = NULL;
    if (container != NULL) {
        if (OCFStringHasPrefix(container, OF_STR("/private/var/containers/Shared/SystemGroup")) ||
            OCFStringHasPrefix(container, OF_STR("/var/containers/Shared/SystemGroup"))) {
            user = kOCFPreferencesAnyUser;
            normalizedByHost = true;
        }
    } else if (OCFStringGetCharacterAtIndex(identifier, 0) == '/') {
        if (cloud) {
            __assert_rtn("normalizeQuintuplet", "CFPrefsSearchListSource.m", 62, "!cloud");
        }
        bool managed = false;
        bool failed = false;
        _OCFPrefsExtractQuadrupleFromPathIfPossible(
            identifier,
            &extractedIdentifier,
            &extractedUser,
            &extractedContainer,
            &normalizedByHost,
            &managed,
            &failed
        );
    }
    if (extractedIdentifier == NULL && OCFStringHasSuffix(identifier, OF_STR(".plist"))) {
        extractedIdentifier = OCFStringCreateWithSubstring(
            kOCFAllocatorSystemDefault, identifier, (OCFRange){0, OCFStringGetLength(identifier) - 6}
        );
    }
    if (extractedContainer != NULL) {
        container = extractedContainer;
    }
    if (extractedIdentifier != NULL) {
        identifier = extractedIdentifier;
    }
    OCFStringRef mappedUser =
        _OCFPrefsCopyUserForContainer(extractedUser != NULL ? extractedUser : user, container);
    if (OCFEqual(kOCFPreferencesAnyUser, mappedUser) && !normalizedByHost) {
        normalizedByHost = true;
    }
    block(_CFPrefsGetCacheStringForBundleID(identifier), mappedUser, normalizedByHost, container, cloud);
    if (extractedIdentifier != NULL) {
        OCFRelease(extractedIdentifier);
    }
    if (extractedContainer != NULL) {
        OCFRelease(extractedContainer);
    }
    if (extractedUser != NULL) {
        OCFRelease(extractedUser);
    }
    if (mappedUser != NULL) {
        OCFRelease(mappedUser);
    }
}

// Native -[_CFXPreferences updateSearchListsForIdentifier:], 0x1c02b326c. Direct method. A copy of the
// search-list dictionary is taken under the search-list lock (withSearchLists: inlined natively, the copy
// held in a __block variable), and every search list is told, remotely, that the identifier's domain changed.
// The copy is released.
- (void)updateSearchListsForIdentifier:(OCFStringRef)identifier {
    __block OCFDictionaryRef searchLists = NULL;
    [self withSearchLists:^(OCFMutableDictionaryRef lists) {
      searchLists = OCFDictionaryCreateCopy(kOCFAllocatorSystemDefault, lists);
    }];
    if (searchLists != NULL) {
        OCFDictionaryApplyFunction(searchLists, notifySearchListOfRemoteChangesCallback, (void *)identifier);
        OCFRelease(searchLists);
    }
}

// Native notifySearchListOfRemoteChangesCallback, 0x1c02b3378. Non-external. Tells one search list that the
// domain in the context changed remotely.
static void notifySearchListOfRemoteChangesCallback(const void *key, const void *value, void *context) {
    (void)key;
    [(id<PreferencesUnrestoredMessages>)value
        handleChangeNotificationForDomainIdentifier:(OCFStringRef)context
                                           isRemote:YES];
}

// Native _CFPrefsWritingToExternalAppCloudPreferencesEnabled, 0x1c02b4090, with its .cold.1 (0x1c0311cac, the
// dispatch_once) and ____CFPrefsWritingToExternalAppCloudPreferencesEnabled_block_invoke (0x1c02b40dc).
// Non-external. An override answers true at once; otherwise the override is computed once (true for
// /usr/bin/defaults, else whether the current application's cache string is com.apple.Preferences') and
// answered.
static bool _CFPrefsWritingToExternalAppCloudPreferencesEnabled(void) {
    static dispatch_once_t onceToken;
    if (_CFPrefsOverrideEnableWritingToExternalAppCloudPreferences) {
        return true;
    }
    dispatch_once(&onceToken, ^{
      if (strcmp(_OCFProcessPath(), "/usr/bin/defaults") == 0) {
          _CFPrefsOverrideEnableWritingToExternalAppCloudPreferences = true;
      } else {
          OCFStringRef current = _CFPrefsGetCacheStringForBundleID(kOCFPreferencesCurrentApplication);
          _CFPrefsOverrideEnableWritingToExternalAppCloudPreferences =
              current == _CFPrefsGetCacheStringForBundleID(OF_STR("com.apple.Preferences"));
      }
    });
    return _CFPrefsOverrideEnableWritingToExternalAppCloudPreferences;
}

// Native MobileCoreServicesLibrary: no symbol (inlined); its function static frameworkLibrary
// (MobileCoreServicesLibrary.frameworkLibrary, __DATA,__bss 0x22e6d39b0) holds the dlopen handle (mode 2,
// RTLD_NOW; the framework is never linked), retried while NULL.
static inline void *MobileCoreServicesLibrary(void) {
    static void *frameworkLibrary;
    if (frameworkLibrary == NULL) {
        frameworkLibrary =
            dlopen("/System/Library/Frameworks/MobileCoreServices.framework/MobileCoreServices", RTLD_NOW);
    }
    return frameworkLibrary;
}

// Native classLSApplicationRecord (__DATA,__bss 0x22e6d39a8). Non-external.
static Class classLSApplicationRecord;

static Class LSApplicationRecordFunction(void);

// Native initLSApplicationRecord, 0x1c02b4140. Non-external.
// With the framework open, resolves LSApplicationRecord (objc_getClass) and repoints the getter to
// LSApplicationRecordFunction; answers the class (Nil when the framework cannot be opened, the getter kept).
static Class initLSApplicationRecord(void) {
    if (MobileCoreServicesLibrary() == NULL) {
        return Nil;
    }
    classLSApplicationRecord = (Class)objc_getClass("LSApplicationRecord");
    getLSApplicationRecordClass = LSApplicationRecordFunction;
    return classLSApplicationRecord;
}

// Native LSApplicationRecordFunction, 0x1c02b41a0. Non-external. Answers the resolved class.
static Class LSApplicationRecordFunction(void) {
    return classLSApplicationRecord;
}

// Native getLSApplicationRecordClass (__DATA,__data 0x22e6cfee8), initialized to initLSApplicationRecord.
// Non-external natively; hidden (declared in Preferences/OCFPreferences.h).
Class (*getLSApplicationRecordClass)(void) = initLSApplicationRecord;

// Native -[_CFXPreferences withSearchLists:], 0x1c03116a4 (with OUTLINED_FUNCTION_19_2 and _25_2). Direct
// method (hidden, was a private external). Runs the block on the search-list dictionary under the search-list
// lock.
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_25_2 (0x1c02b44e0).
- (void)withSearchLists:(void (^)(OCFMutableDictionaryRef searchLists))block {
    os_unfair_lock_lock(&_searchListsLock);
    block(_searchLists);
    os_unfair_lock_unlock(&_searchListsLock);
}

// Native -[_CFXPreferences withSearchListForIdentifier:container:cloudConfigurationURL:perform:],
// 0x1c0311700, with ___108-[_CFXPreferences(SearchListAdditions)
// withSearchListForIdentifier:container:cloudConfigurationURL: perform:]_block_invoke (0x1c02b3384) and
// _block_invoke_2 (0x1c02b35c0). Direct method. normalizeQuintuplet (kCFPreferencesCurrentUser, not by host;
// whether a configuration URL was given) calls back with the normalized quintuplet. The search list is keyed
// by the identifier, with the container appended when there is one. A pending-KVO slot is armed, then under
// the search-list lock the search list is retained or built: managed sources (user, any user),
// NSArgumentDomain, the cloud source when a cloud configuration is found (made the cloud set target), the
// user's source (made the standard set target), the suite, the any-application managed source, the simulator
// backstops, the by-host any-user sources, the NSRegistrationDomain volatile source and the
// configuration-file source; it is marked initialized, stored and released to the dictionary's reference.
// With cloud keys, the cloud set target is enabled. The block receives the search list; its release and the
// KVO delivery form the @finally (region bounds confidence medium). The store name copied by
// -copyCloudConfigurationWithURL:outConfigFileSource:outStoreName: is not released after
// -addCloudSourceForIdentifier:configurationPath:storeName:container:, as natively (whether the cloud source
// takes ownership is unread: -[CFPrefsCloudSource setStoreName:] 0x1c028fe30 is outside this restoration).
- (void)withSearchListForIdentifier:(OCFStringRef)identifier
                          container:(OCFStringRef)container
              cloudConfigurationURL:(OCFURLRef)cloudConfigurationURL
                            perform:(void (^)(OCFPrefsSearchListSource *searchList))block {
    normalizeQuintuplet(
        identifier,
        kOCFPreferencesCurrentUser,
        false,
        container,
        cloudConfigurationURL != NULL,
        ^(OCFStringRef normalizedIdentifier,
          OCFStringRef normalizedUser,
          bool byHost,
          OCFStringRef normalizedContainer,
          bool cloud) {
          (void)cloud;
          __block OCFPrefsSearchListSource *searchList = nil;
          __block bool cloudEnabled = false;
          OCFStringRef key = normalizedIdentifier;
          if (normalizedContainer != NULL) {
              OCFMutableStringRef containerKey =
                  OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, normalizedIdentifier);
              OCFStringAppend(containerKey, normalizedContainer);
              key = containerKey;
          }
          bool armed = _CFPrefsArmPendingKVOSlot();
          [self withSearchLists:^(OCFMutableDictionaryRef searchLists) {
            searchList = (OCFPrefsSearchListSource *)OCFDictionaryGetValue(searchLists, key);
            if (searchList == nil) {
                searchList = [[OCFPrefsSearchListSource alloc] initWithIdentifier:normalizedIdentifier
                                                            containingPreferences:self];
                OCFStringRef anyUser = kOCFPreferencesAnyUser;
                [searchList addManagedSourceForIdentifier:normalizedIdentifier user:normalizedUser];
                [searchList addManagedSourceForIdentifier:normalizedIdentifier user:anyUser];
                [searchList addNamedVolatileSourceForIdentifier:OF_STR("NSArgumentDomain")];
                OCFStringRef storeName = NULL;
                OCFPrefsSource *configurationFileSource = nil;
                OCFStringRef configurationPath =
                    [searchList copyCloudConfigurationWithURL:cloudConfigurationURL
                                          outConfigFileSource:&configurationFileSource
                                                 outStoreName:&storeName];
                if (configurationPath != NULL) {
                    [searchList addCloudSourceForIdentifier:normalizedIdentifier
                                          configurationPath:configurationPath
                                                  storeName:storeName
                                                  container:normalizedContainer];
                    OCFArrayRef sourceList = searchList->_sourceList;
                    searchList->_cloudSetTarget = (OCFPrefsSource *)OCFArrayGetValueAtIndex(
                        sourceList, OCFArrayGetCount(sourceList) - 1
                    );
                    OCFRelease(configurationPath);
                }
                [searchList addSourceForIdentifier:normalizedIdentifier
                                              user:normalizedUser
                                            byHost:byHost
                                         container:normalizedContainer];
                OCFArrayRef sourceList = searchList->_sourceList;
                searchList->_standardSetTarget =
                    (OCFPrefsSource *)OCFArrayGetValueAtIndex(sourceList, OCFArrayGetCount(sourceList) - 1);
                [searchList addSuiteSourceForIdentifier:normalizedIdentifier user:normalizedUser];
                OCFStringRef anyApplication = kOCFPreferencesAnyApplication;
                [searchList addManagedSourceForIdentifier:anyApplication user:normalizedUser];
                [searchList addSimulatorBackstopSourceForIdentifier:normalizedIdentifier];
                [searchList addSourceForIdentifier:anyApplication
                                              user:normalizedUser
                                            byHost:byHost
                                         container:normalizedContainer];
                [searchList addSimulatorBackstopSourceForIdentifier:anyApplication];
                [searchList addSourceForIdentifier:normalizedIdentifier
                                              user:anyUser
                                            byHost:YES
                                         container:NULL];
                [searchList addSuiteSourceForIdentifier:normalizedIdentifier user:anyUser];
                [searchList addSourceForIdentifier:anyApplication user:anyUser byHost:YES container:NULL];
                [searchList addNamedVolatileSourceForIdentifier:OF_STR("NSRegistrationDomain")];
                if (configurationFileSource != nil) {
                    [searchList addSource:configurationFileSource];
                    [configurationFileSource release];
                }
                OCFSetRef cloudKeys = searchList->_cloudKeys;
                cloudEnabled = cloudKeys != NULL && OCFSetGetCount(cloudKeys) > 0;
                if (searchList != nil) {
                    searchList->initialized = YES;
                }
                OCFDictionarySetValue(searchLists, key, searchList);
                [searchList release];
            }
            [searchList retain];
          }];
          if (normalizedContainer != NULL) {
              OCFRelease(key);
          }
          if (cloudEnabled) {
              [(OCFPrefsCloudSource *)searchList->_cloudSetTarget setEnabled:YES];
          }
          @try {
              block(searchList);
          } @finally {
              [searchList release];
              if (armed) {
                  [self _deliverPendingKVONotifications];
              }
          }
        }
    );
}

@end

// Native class CFPrefsSuiteSearchListSource's methods: -deferredNotifyCausedByLoadingOfChangesFromDictionary:
// toDictionary: (0x1c02b3fb8), -_notifyObserversOfChangeFromValuesForKeys:toValuesForKeys: (0x1c02b3fcc),
// -handleChangeNotificationForDomainIdentifier:isRemote: (0x1c02b3fe0) and
// -deferredNotifyCausedByLocalWriteOfChangesToKey:fromValue:toValue: (0x1c0311838). Natively they lie between
// -[_CFXPreferences updateSearchListsForIdentifier:] and _CFPrefsWritingToExternalAppCloudPreferencesEnabled;
// they are kept in their own @implementation after the category so that the class's @implementation blocks
// stay whole.
@implementation OCFPrefsSuiteSearchListSource

// Native -[CFPrefsSuiteSearchListSource deferredNotifyCausedByLoadingOfChangesFromDictionary:toDictionary:],
// 0x1c02b3fb8. Only asserts that the source lock is owned (a tail call).
- (void)deferredNotifyCausedByLoadingOfChangesFromDictionary:(OCFDictionaryRef)oldValues
                                                toDictionary:(OCFDictionaryRef)newValues {
    (void)oldValues;
    (void)newValues;
    os_unfair_lock_assert_owner(&_lock);
}

// Native -[CFPrefsSuiteSearchListSource _notifyObserversOfChangeFromValuesForKeys:toValuesForKeys:],
// 0x1c02b3fcc. Only asserts that the source lock is owned (a tail call).
- (void)_notifyObserversOfChangeFromValuesForKeys:(OCFDictionaryRef)oldValues
                                  toValuesForKeys:(OCFDictionaryRef)newValues {
    (void)oldValues;
    (void)newValues;
    os_unfair_lock_assert_owner(&_lock);
}

// Native -[CFPrefsSuiteSearchListSource handleChangeNotificationForDomainIdentifier:isRemote:], 0x1c02b3fe0,
// with ___85-[CFPrefsSuiteSearchListSource handleChangeNotificationForDomainIdentifier:isRemote:]
// _block_invoke (0x1c02b405c, descriptor ___block_descriptor_33_e8_v16_?0_8l). Every observer is told that
// its own domain changed, with the remote flag passed on; the identifier is not used.
- (void)handleChangeNotificationForDomainIdentifier:(OCFStringRef)identifier isRemote:(BOOL)isRemote {
    (void)identifier;
    [self forEachObserver:^(id observer) {
      [(id<PreferencesUnrestoredMessages>)observer
          handleChangeNotificationForDomainIdentifier:[observer domainIdentifier]
                                             isRemote:isRemote];
    }];
}

// Native -[CFPrefsSuiteSearchListSource deferredNotifyCausedByLocalWriteOfChangesToKey:fromValue:toValue:],
// 0x1c0311838. Aborts: suites are never written locally.
- (void)deferredNotifyCausedByLocalWriteOfChangesToKey:(OCFStringRef)key
                                             fromValue:(OCFPropertyListRef)oldValue
                                               toValue:(OCFPropertyListRef)newValue {
    (void)key;
    (void)oldValue;
    (void)newValue;
    abort();
}

@end

// Native pendingKVODestructor, 0x1c0311528. Non-external natively; hidden for the C1 TLS setter.
// iOS Device divergence: unread.
void pendingKVODestructor(void *value) {
    (void)value;
    gCRAnnotations[1] = (uint64_t)(uintptr_t)"Unexpected pending notifications upon thread exit";
    abort();
}
