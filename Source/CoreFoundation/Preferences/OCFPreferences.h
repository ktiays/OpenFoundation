//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Notification/OCFNotificationCenter.h"
#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/PropertyList/OCFPropertyList.h"
#include "CoreFoundation/URL/OCFURL.h"
#include "CoreFoundation/URL/OCFURLFileUtilities.h"
#include "CoreFoundation/PlugIn/OCFBundleMain.h"

#include <dispatch/dispatch.h>
#include <mach/message.h>
#include <objc/objc.h>
#include <os/log.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

// The CFPreferences family: the C entry points, the CFPrefs path, user, container and cache-key helpers, the
// soft-linked SyncedDefaults hooks, and the Objective-C preference sources (Preferences/OCFPrefsSource.h,
// Preferences/OCFPrefsSearchListSource.h, Preferences/OCFXPreferences.h). iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. Reading:
// Local/Scratch/cf-core-prefs-daemon/REPORT.md (main checkout; integration reading pending) (to be written by
// the coordinating session from this track's report).

// Public CFPreferences interface, iOS Simulator SDK CFPreferences.h, spelled with the OCF prefix.
OF_EXPORT const OCFStringRef kOCFPreferencesAnyApplication;
OF_EXPORT const OCFStringRef kOCFPreferencesCurrentApplication;
OF_EXPORT const OCFStringRef kOCFPreferencesAnyUser;
OF_EXPORT const OCFStringRef kOCFPreferencesCurrentUser;

OF_EXPORT OCFPropertyListRef OCFPreferencesCopyAppValue(OCFStringRef key, OCFStringRef applicationID);

OF_EXPORT Boolean OCFPreferencesGetAppBooleanValue(
    OCFStringRef key, OCFStringRef applicationID, Boolean *keyExistsAndHasValidFormat
);

OF_EXPORT Boolean OCFPreferencesAppSynchronize(OCFStringRef applicationID);

// Exported natively, absent from the public SDK headers; signatures from the native register use.
OF_EXPORT const OCFStringRef kOCFPreferencesDidChangeNotificationName;
OF_EXPORT const OCFStringRef kOCFPreferencesDidChangeObject;

OF_EXPORT OCFPropertyListRef _OCFPreferencesCopyAppValueWithContainerAndConfiguration(
    OCFStringRef key, OCFStringRef applicationID, OCFStringRef container, OCFURLRef cloudConfigurationURL
);

OF_EXPORT Boolean _OCFPreferencesGetAppBooleanValueWithContainer(
    OCFStringRef key, OCFStringRef applicationID, OCFStringRef container, Boolean *keyExistsAndHasValidFormat
);

OF_EXPORT Boolean
_OCFPreferencesGetBooleanValueWithValue(OCFPropertyListRef value, Boolean *keyExistsAndHasValidFormat);

OF_EXPORT OCFStringRef _OCFPrefsCopyUserForContainer(OCFStringRef user, OCFStringRef container);

OF_EXPORT void _OCFPrefsExtractQuadrupleFromPathIfPossible(
    OCFStringRef path,
    OCFStringRef *outIdentifier,
    OCFStringRef *outUser,
    OCFStringRef *outContainer,
    bool *outByHost,
    bool *outManaged,
    bool *outFailed
);

// Hidden natively (private externals or non-external names used by more than one restored unit).
OF_EXTERN OCFStringRef _CFPrefsCopyUserConstantForUserName(OCFStringRef user);

OF_EXTERN OCFStringRef _CFPrefsCreateCacheKeyForQuintuplet(
    OCFStringRef identifier, OCFStringRef user, Boolean byHost, OCFStringRef container, Boolean cloud
);

OF_EXTERN Boolean _CFPrefsValidateValueForKey(OCFPropertyListRef value, OCFStringRef key);

OF_EXTERN OCFStringRef _CFPrefsCopyUsernameForMobileUser(void);

OF_EXTERN OCFURLRef _CFPreferencesCopyManagedPreferencesContainerURL(void);

OF_EXTERN OCFMutableStringRef _CFPrefsCreateConfigurationFileNameFromIdentifier(OCFStringRef identifier);

OF_EXTERN OCFURLRef _CFPrefsCopyCloudConfigurationURLFromBundle(OCFBundleRef bundle, OCFStringRef identifier);

OF_EXTERN os_log_t _CFPrefsClientLog(void);

OF_EXTERN os_log_t _CFPrefsDaemonLog(void);

OF_EXTERN bool _CFPrefsCurrentProcessIsCFPrefsD(void);

OF_EXTERN bool _CFPrefsRunningOnPrimaryQueue(void);

OF_EXTERN bool _CFPrefsArmPendingKVOSlot(void);

OF_EXTERN OCFStringRef _CFPrefsGetCacheStringForBundleID(OCFStringRef identifier);

OF_EXTERN Boolean _CFPreferencesDoesNSUserDefaultsExist(void);

OF_EXTERN void *_CFPreferencesCopyStandardUserDefaults(void);

OF_EXTERN bool _CFPrefsGetImpersonatedApplicationAuditToken(audit_token_t *outToken);

// The primary-queue specific key and value (__DATA,__data __MergedGlobals 0x22e6cff58 and 0x22e6cff60), each
// initialized to its own address; the queue they mark is configured outside this family. The native names
// were merged away; primaryQueueSpecificKey and primaryQueueSpecificValue are local descriptive spellings.
OF_EXTERN void *primaryQueueSpecificKey;
OF_EXTERN void *primaryQueueSpecificValue;

// Soft-linked SyncedDefaults class getter (__DATA,__data 0x22e6cf750), initialized to
// initSYDRemotePreferencesSource and replaced by SYDRemotePreferencesSourceFunction once resolved.
OF_EXTERN Class (*getSYDRemotePreferencesSourceClass)(void);

// Non-external helpers of the preference units used across their translation units (hidden here), with
// signatures from the native register use: eduModeEnabled, normalizeQuintuplet and
// getLSApplicationRecordClass (Preferences/OCFPrefsSearchListSource.m), applyBlockApplier and
// dummyKVODestructor (Preferences/OCFXPreferences.m), createDeepCopyOfValueForKey and
// _addBackstopValuesForIdentifierAndSource (Preferences/OCFPrefsSource.m; the source is a CFPrefsSource). The
// normalized-quintuplet block receives the cache string, the user (+0, released after the call), the by-host
// flag, the container and the cloud flag normalizeQuintuplet was given.
OF_EXTERN Boolean eduModeEnabled(void);
OF_EXTERN void applyBlockApplier(const void *key, const void *value, void *context);
OF_EXTERN void dummyKVODestructor(void *value);
OF_EXTERN OCFTypeRef createDeepCopyOfValueForKey(OCFTypeRef value, OCFStringRef key);
OF_EXTERN void _addBackstopValuesForIdentifierAndSource(OCFStringRef identifier, void *source);
OF_EXTERN Class (*getLSApplicationRecordClass)(void);
OF_EXTERN void normalizeQuintuplet(
    OCFStringRef identifier,
    OCFStringRef user,
    bool byHost,
    OCFStringRef container,
    bool cloud,
    void (^block)(OCFStringRef identifier, OCFStringRef user, bool byHost, OCFStringRef container, bool cloud)
);

// Native _CFPrefsDirectMode, 0x1c02e578c (Preferences/OCFPreferences.c). Exported natively, absent from the
// public SDK headers: whether the process talks to an in-process (direct) daemon.
OF_EXPORT bool _OCFPrefsDirectMode(void);

// Native _CFPrefsProcessPathRequiresDirectMode: no symbol (inlined into _CFPrefsDirectMode natively; its data
// and block lie with the CFPrefsDaemon unit, Preferences/OCFPrefsDaemon.m). Hidden here: whether the process
// path is on the direct-mode list.
OF_EXTERN bool _CFPrefsProcessPathRequiresDirectMode(void);

// Native _CFPrefsSynchronizeIsSynchronous (__DATA,__bss 0x22e6d3de8, Preferences/OCFPreferences.c).
// Non-external natively; hidden here because -[CFPrefsPlistSource createSynchronizeMessage]
// (Preferences/OCFPrefsPlistSource.m) reads it.
OF_EXTERN bool _CFPrefsSynchronizeIsSynchronous;

// Native kCFPreferencesByteCountLimitReachedNotificationName (__DATA_CONST,__const 0x2227ab580,
// Preferences/OCFXPreferences.m). External natively, absent from the public SDK headers.
OF_EXPORT const OCFStringRef kOCFPreferencesByteCountLimitReachedNotificationName;

// Native _CFPrefsReadOnly, 0x1c02e58b8 (Preferences/OCFPreferences.c). Hidden (was a private external).
OF_EXTERN bool _CFPrefsReadOnly(void);

// Native isCFPrefsD (__DATA,__bss 0x22e6d3cb0, Preferences/OCFPreferences.c). Non-external natively; hidden
// here because the CFPrefsDaemon methods (Preferences/OCFPrefsDaemon.m) read it.
OF_EXTERN bool isCFPrefsD;

// Native __CFPrefsAuditTokenToImpersonate (__DATA,__bss 0x22e6d3dec, Preferences/OCFPreferences.c).
// Non-external natively; hidden here because -[CFPrefsPlistSource addPIDImpersonationIfNecessary:]
// (Preferences/OCFPrefsPlistSource.m) reads it.
OF_EXTERN audit_token_t _CFPrefsAuditTokenToImpersonate;

// Native _CFPrefsCreatePreferencesDirectory, 0x1c028ff94 (Preferences/OCFPrefsCloudSource.m). Hidden (was a
// private external). Creates the missing directories of path below its deepest existing ancestor, with the
// given mode and owner; answers whether it succeeded.
OF_EXTERN bool _CFPrefsCreatePreferencesDirectory(char *path, mode_t mode, uid_t uid, gid_t gid, int *outFD);

// Native _CFPrefsDictionaryApplyBlock, 0x1c02ea114 (Preferences/OCFXPreferences.m). Hidden (was a private
// external). Applies the block (key, value) to every entry through applyBlockApplier.
OF_EXTERN void
_CFPrefsDictionaryApplyBlock(OCFDictionaryRef dictionary, void (^block)(const void *key, const void *value));

// Native _CFPrefsLongLivedLog, 0x1c02b1a80 (Preferences/OCFPrefsSearchListSource.m). Hidden (was a private
// external). The audit log handle.
OF_EXTERN os_log_t _CFPrefsLongLivedLog(void);

// Native _CFPrefsSignpostsLogForDuration, 0x1c02b1ab0 (Preferences/OCFPrefsSearchListSource.m). Hidden (was a
// private external). The signpost log handle for a duration in mach continuous time units.
OF_EXTERN os_log_t _CFPrefsSignpostsLogForDuration(uint64_t duration);

// Native _CFPrefsResetManagedPreferencesStateCache, 0x1c02eb020 (Preferences/OCFXPreferences.m). Hidden (was
// a private external). Empties the managed-preferences state caches.
OF_EXTERN void _CFPrefsResetManagedPreferencesStateCache(void);

// Native _CFXPreferencesGetByHostIdentifierString, 0x1c02efda8 (Preferences/OCFXPreferences.m). External.
OF_EXPORT OCFStringRef _OCFXPreferencesGetByHostIdentifierString(void);

// Native _CFAuditTokenForSelf, 0x1c023a248 (Preferences/OCFPrefsSource.m). Hidden (was a private external).
OF_EXTERN audit_token_t _CFAuditTokenForSelf(void);

// Native _CFPrefsGetPathForTriplet, 0x1c029132c (Preferences/OCFPreferences.c). Hidden (was a private
// external). Writes the plist path of an identifier, user, by-host flag and container path (at most 1024
// bytes) into buffer.
OF_EXTERN bool _CFPrefsGetPathForTriplet(
    OCFStringRef identifier, OCFStringRef user, bool byHost, OCFStringRef containerPath, char *buffer
);

// Native _CFPrefsGetPathForManagedBundleID, 0x1c0290cec (Preferences/OCFPreferences.c). Hidden (was a
// private external). Writes the managed preferences path (at most 1024 bytes) into buffer.
OF_EXTERN bool _CFPrefsGetPathForManagedBundleID(
    OCFStringRef identifier, OCFStringRef user, bool containerized, char *buffer
);

// Native _CFPrefsTemporaryFDToWriteTo, 0x1c0291514, and _CFPrefsWritePlistToFDThenClose, 0x1c029180c
// (Preferences/OCFPreferences.c). Hidden (were private externals). The first answers a new temporary file's
// descriptor beside url (owner, mode and data protection class applied) or -1 with errno set; the second
// writes the data object (-length, -bytes, -validatePlist) to the descriptor, closes it and renames its file
// to destination relative to directoryFD, answering 0 or an errno-style code (0xbadd for invalid data).
OF_EXTERN int
_CFPrefsTemporaryFDToWriteTo(OCFURLRef url, uid_t uid, gid_t gid, mode_t mode, int protectionClass);

OF_EXTERN int _CFPrefsWritePlistToFDThenClose(
    id data,
    int fd,
    uid_t uid,
    gid_t gid,
    mode_t mode,
    bool doNotBackUp,
    int directoryFD,
    const char *destination,
    const char *displayPath
);

// Native _CFPreferencesWriteDomainDictionaryToPath, 0x1c02f1984.
OF_EXPORT Boolean _OCFPreferencesWriteDomainDictionaryToPath(
    OCFDictionaryRef dictionary, OCFStringRef directory, OCFStringRef identifier
);

// Native _CFPreferencesManagementStatusChangedForDomains, 0x1c02f1c34. External.
OF_EXPORT void _OCFPreferencesManagementStatusChangedForDomains(OCFArrayRef domains);
