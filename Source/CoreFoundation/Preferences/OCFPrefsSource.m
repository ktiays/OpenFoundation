//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Preferences/OCFPrefsSource.h"
#import "CoreFoundation/Preferences/OCFPrefsCloudSource.h"
#import "CoreFoundation/Preferences/OCFPrefsPlistSource.h"
#import "CoreFoundation/Base/OCFPlatform.h"
#import "CoreFoundation/Preferences/OCFPreferences.h"
#import "CoreFoundation/Preferences/OCFPreferencesDescription.h"
#import "CoreFoundation/Preferences/OCFXPreferences.h"
#import "CoreFoundation/Runtime/OCFRuntime.h"

#include <assert.h>
#include <dispatch/dispatch.h>
#include <errno.h>
#include <malloc/malloc.h>
#include <notify.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <os/log.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

// Inferred organization: _CFAuditTokenForSelf (0x1c023a248) through ___getDefaultArguments_block_invoke
// (0x1c023e3dc) are contiguous in the iOS Simulator __text section with the CFPrefsSource methods, and the
// assertion of withSourceForIdentifier:user:byHost:container:cloud:perform: names "CFPrefsSource.m";
// -[_CFXPreferences withSources:] (0x1c030d064) and -[_CFXPreferences withNamedVolatileSources:]
// (0x1c030d0ac) sit with CFPrefsSource's description methods. The restored functions of that unit are kept in
// this translation unit. The unit is Objective-C compiled with manual reference counting (block captures with
// copy helpers, objc_release for release messages, @finally rethrow paths). ___Block_byref_object_copy__1
// (0x1c023d7ac) and ___Block_byref_object_dispose__1 (0x1c023d7bc), the helpers of a __block object variable
// under manual reference counting (_Block_object_assign and _Block_object_dispose with flags 131, which
// neither retain nor release), are emitted by the compiler for the __block source variables below; they have
// no source definition. The .cold fragments (0x1c030d0f4-0x1c030d418) are kept in their parents. Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationPreferences.md.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_1_20 (0x1c023e634), _OUTLINED_FUNCTION_3_9
// (0x1c023e650), _OUTLINED_FUNCTION_6_6 (0x1c023e674).

extern void objc_release(id object);
extern char sourceStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

// The classes the default-arguments block messages. NSMutableDictionary is an Objective-C class of this image
// (__SCR_NSMutableDictionary GOT slot 0x2227ce378). The receivers of +processInfo and
// +whitespaceAndNewlineCharacterSet are loaded from class references 0x22229c598 and 0x22229c238, which lie
// outside the extracted image; the bindings NSProcessInfo and NSCharacterSet are inferred from the selectors
// (confidence: medium).
extern char mutableDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableDictionary));
extern char processInfoClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSProcessInfo));
extern char characterSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSCharacterSet));

// libobjc's objc_opt_new (no SDK declaration); supplied. The block creates the dictionary with it, as
// natively.
extern id objc_opt_new(Class cls);

// libsystem_sandbox entry points and flag constants the native code reads; supplied, never reimplemented. The
// SDK installs no declarations. The two flag constants are read through GOT slots 0x22241b278 and
// 0x22241b290, which lie outside the extracted image; the binding (SANDBOX_CHECK_ALLOW_APPROVAL,
// SANDBOX_CHECK_NO_REPORT) is inferred from the alphabetical, 8-byte-stride order of the image's libSystem
// data imports (confirmed on the _NSConcrete*Block slots 0x222417df8-0x222417e20) and from the approval
// query: confidence medium.
extern const uint32_t SANDBOX_CHECK_ALLOW_APPROVAL;
extern const uint32_t SANDBOX_CHECK_NO_REPORT;
extern int sandbox_check_and_query_approval_category(
    audit_token_t token, char **approvalCategory, const char *operation, int type, ...
);
extern int sandbox_container_path_for_audit_token(audit_token_t token, char *buffer, size_t size);

// Constants as the native call sites pass them: kCFNotFound, kCFCompareBackwards (4), kCFCompareAnchored (8),
// kCFStringEncodingUTF8, kCFURLPOSIXPathStyle, SANDBOX_FILTER_PREFERENCE_DOMAIN (6) and
// kCFPropertyListBinaryFormat_v1_0 (200). The thread-specific slot 15 marks a log or throw in progress.
#define kNotFound ((OCFIndex) - 1)
#define kCompareBackwards ((OCFOptionFlags)4)
#define kCompareAnchored ((OCFOptionFlags)8)
#define kUTF8Encoding ((OCFStringEncoding)0x08000100)
#define kPOSIXPathStyle ((OCFIndex)0)
#define kPreferenceDomainFilter 6
#define kBinaryPropertyListFormat ((OCFIndex)200)
#define kLogInProgressSlot 15

static int _CFPrefsSandboxCheck(audit_token_t token, const char *operation, int type, const char *path);

// The cleanup of the native temporary id buffers (new_id_array pattern): the heap storage, if any, is freed
// when the buffer goes out of scope, including on unwinding (the landing pads free it and resume). The
// cleanup function is inlined natively; releaseTemporaryStorage is a local descriptive name.
static inline void releaseTemporaryStorage(void **storage) {
    free(*storage);
}

// Native _CFAuditTokenForSelf, 0x1c023a248, with its .cold.1 (0x1c030d0f4, the dispatch_once) and
// ____CFAuditTokenForSelf_block_invoke (0x1c023a28c). Hidden (was a private external).
// The function statics .auditToken and .onceToken (__DATA,__bss 0x22e6d32c0 and 0x22e6d32e0) are shared
// natively with the inlined copy in _CFPrefsExtractQuadrupleFromPathIfPossible, which calls this function
// here. Once, task_info(mach_task_self(), TASK_AUDIT_TOKEN (15)) fills the token with a count of 8 (its
// result is not examined); the token is answered by value.
audit_token_t _CFAuditTokenForSelf(void) {
    static audit_token_t auditToken;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      mach_msg_type_number_t count = 8;
      task_info(mach_task_self(), 15, (task_info_t)&auditToken, &count);
    });
    return auditToken;
}

// Native _CFPrefsCopyUserForContainer, 0x1c023a2f8. External.
// A container under /private/var/containers/Shared/SystemGroup/ answers kCFPreferencesAnyUser; otherwise the
// user is canonicalized by _CFPrefsCopyUserConstantForUserName (inlined natively).
OCFStringRef _OCFPrefsCopyUserForContainer(OCFStringRef user, OCFStringRef container) {
    if (container != NULL &&
        OCFStringHasPrefix(container, OF_STR("/private/var/containers/Shared/SystemGroup/"))) {
        return kOCFPreferencesAnyUser;
    }
    return _CFPrefsCopyUserConstantForUserName(user);
}

// Native _CFPrefsCopyUserConstantForUserName, 0x1c023a3dc. Non-external, inlined into callers in this unit
// and in CFPrefsSearchListSource.m; hidden. kCFPreferencesCurrentUser and kCFPreferencesAnyUser answer
// themselves unretained; the current user's name answers kCFPreferencesCurrentUser (when that constant is not
// NULL); any other name answers a copy.
OCFStringRef _CFPrefsCopyUserConstantForUserName(OCFStringRef user) {
    OCFStringRef currentUser = kOCFPreferencesCurrentUser;
    if (OCFEqual(user, currentUser)) {
        return currentUser;
    }
    OCFStringRef anyUser = kOCFPreferencesAnyUser;
    if (OCFEqual(user, anyUser)) {
        return anyUser;
    }
    OCFStringRef userName = OCFCopyUserName();
    Boolean isCurrentUser = OCFEqual(user, userName);
    OCFRelease(userName);
    if (currentUser != NULL && isCurrentUser) {
        return currentUser;
    }
    return OCFStringCreateCopy(kOCFAllocatorSystemDefault, user);
}

// Native _CFPrefsExtractQuadrupleFromPathIfPossible, 0x1c023a490, with its .cold.1 (0x1c030d108, the crash),
// .cold.2 and .cold.4 (0x1c030d11c and 0x1c030d164, the dispatch_once calls), the inlined
// _CFAuditTokenForSelf
// (.cold.3, 0x1c030d140) and its blocks ____CFPrefsExtractQuadrupleFromPathIfPossible_block_invoke
// (0x1c023aeec) and _block_invoke_2 (0x1c023b078). External. Only absolute paths or paths containing ".." are
// examined; ".plist" is appended when missing. System group and app group containers
// ("/containers/Shared/SystemGroup/", "/Containers/Shared/AppGroup/" with a backwards
// "/Library/Preferences/") yield the identifier and the container directly. Otherwise the path is normalized
// (/private/var/preferences/ to /Library/Preferences/, /private/var/ to /var/), a containerized home prefix
// or a CoreSimulator device prefix is cut, and managed preferences or a "/Library/Preferences/" component
// decide the identifier. The user extracted from the path is released unused: the reported user always starts
// from kCFPreferencesCurrentUser and is mapped through the container (well-defined native behavior, kept).
// The container is cleared when the process's own sandbox container path or the sandbox's preference-domain
// checks say the domain is the process's own. In education mode the mobile user maps to its constant. An
// AnyUser result sets *outByHost. Without an identifier, *outFailed is set and the container is released.
void _OCFPrefsExtractQuadrupleFromPathIfPossible(
    OCFStringRef path,
    OCFStringRef *outIdentifier,
    OCFStringRef *outUser,
    OCFStringRef *outContainer,
    bool *outByHost,
    bool *outManaged,
    bool *outFailed
) {
    static dispatch_once_t onceToken;
    static OCFStringRef maybeContainerizedHome;
    static dispatch_once_t mobileHomeOnceToken;
    static OCFStringRef mobileHome;
    *outFailed = false;
    if (path == NULL) {
        return;
    }
    if (OCFStringGetCharacterAtIndex(path, 0) != '/' &&
        OCFStringFind(path, OF_STR(".."), 0).location == kNotFound) {
        return;
    }
    OCFStringRef plistPath;
    if (OCFStringHasSuffix(path, OF_STR(".plist"))) {
        plistPath = OCFRetain(path);
    } else {
        plistPath = OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR("%@.plist"), path);
    }
    OCFStringRef identifier = NULL;
    OCFStringRef container = NULL;
    OCFStringRef user = NULL;
    if (OCFStringFind(plistPath, OF_STR("/containers/Shared/SystemGroup/"), 0).location != kNotFound) {
        OCFRange preferences = OCFStringFind(plistPath, OF_STR("/Library/Preferences/"), kCompareBackwards);
        if (preferences.location != kNotFound) {
            OCFIndex start = preferences.location + preferences.length;
            identifier = OCFStringCreateWithSubstring(
                kOCFAllocatorSystemDefault,
                plistPath,
                (OCFRange){start, OCFStringGetLength(plistPath) - start}
            );
            container = OCFStringCreateWithSubstring(
                kOCFAllocatorSystemDefault, plistPath, (OCFRange){0, preferences.location + 1}
            );
            user = kOCFPreferencesAnyUser;
        }
    }
    OCFIndex groupLocation = OCFStringFind(plistPath, OF_STR("/Containers/Shared/AppGroup/"), 0).location;
    if (groupLocation >= 1) {
        OCFRange preferences = OCFStringFind(plistPath, OF_STR("/Library/Preferences/"), kCompareBackwards);
        if (preferences.location != kNotFound) {
            OCFIndex start = preferences.location + preferences.length;
            OCFIndex length = OCFStringGetLength(plistPath) - start;
            if (identifier != NULL) {
                OCFRelease(identifier);
            }
            identifier = OCFStringCreateWithSubstring(
                kOCFAllocatorSystemDefault, plistPath, (OCFRange){start, length}
            );
            if (container != NULL) {
                OCFRelease(container);
            }
            container = OCFStringCreateWithSubstring(
                kOCFAllocatorSystemDefault, plistPath, (OCFRange){0, preferences.location + 1}
            );
            OCFRange slash = {0, 0};
            if (!OCFStringFindWithOptions(
                    plistPath, OF_STR("/"), (OCFRange){0, groupLocation}, kCompareBackwards, &slash
                )) {
                crashWithMessage("Mismatched string find result");
            }
            user = OCFStringCreateWithSubstring(
                kOCFAllocatorSystemDefault,
                plistPath,
                (OCFRange){slash.location + 1, groupLocation - (slash.location + 1)}
            );
        }
    }
    bool userFromPath;
    OCFStringRef pathToRelease;
    if (identifier != NULL) {
        userFromPath = false;
        pathToRelease = plistPath;
    } else {
        OCFStringRef working = OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, plistPath);
        OCFRelease(plistPath);
        OCFRange prefix = OCFStringFind(working, OF_STR("/private/var/preferences/"), kCompareAnchored);
        if (prefix.location != kNotFound) {
            OCFStringReplace((OCFMutableStringRef)working, prefix, OF_STR("/Library/Preferences/"));
        } else {
            prefix = OCFStringFind(working, OF_STR("/private/var/"), kCompareAnchored);
            if (prefix.location != kNotFound) {
                OCFStringReplace((OCFMutableStringRef)working, prefix, OF_STR("/var/"));
            }
        }
        dispatch_once(&onceToken, ^{
          OCFURLRef home = OCFCopyHomeDirectoryURLForUser(NULL);
          if (home != NULL) {
              maybeContainerizedHome = OCFURLCopyFileSystemPath(home, kPOSIXPathStyle);
              OCFRelease(home);
          }
        });
        if (OCFStringFind(working, OF_STR("/Containers/Data"), 0).location == kNotFound &&
            OCFStringHasPrefix(working, maybeContainerizedHome)) {
            OCFIndex homeLength = OCFStringGetLength(maybeContainerizedHome);
            OCFStringRef relative = OCFStringCreateWithSubstring(
                kOCFAllocatorSystemDefault,
                working,
                (OCFRange){homeLength, OCFStringGetLength(working) - homeLength}
            );
            OCFRelease(working);
            working = relative;
        } else if (
            OCFStringFind(working, OF_STR("Developer/CoreSimulator/Devices/"), 0).location != kNotFound
        ) {
            OCFIndex location = OCFStringFind(working, OF_STR("/Library/Preferences/"), 0).location;
            if (location != kNotFound) {
                OCFStringRef relative = OCFStringCreateWithSubstring(
                    kOCFAllocatorSystemDefault,
                    working,
                    (OCFRange){location, OCFStringGetLength(working) - location}
                );
                if (container != NULL) {
                    OCFRelease(container);
                }
                container = OCFStringCreateWithSubstring(
                    kOCFAllocatorSystemDefault, working, (OCFRange){0, location}
                );
                OCFRelease(working);
                working = relative;
            }
        }
        OCFRange managed = OCFStringFind(working, OF_STR("/var/Managed Preferences/"), kCompareAnchored);
        if (managed.location == kNotFound) {
            managed = OCFStringFind(working, OF_STR("/Library/Managed Preferences/"), kCompareAnchored);
        }
        if (managed.location != kNotFound) {
            *outManaged = true;
            OCFStringRef rest = OCFStringCreateWithSubstring(
                kOCFAllocatorSystemDefault,
                working,
                (OCFRange){managed.length, OCFStringGetLength(working) - managed.length}
            );
            OCFRelease(working);
            OCFIndex slash = OCFStringFind(rest, OF_STR("/"), 0).location;
            if (slash != kNotFound) {
                if (user != NULL) {
                    OCFRelease(user);
                }
                user = OCFStringCreateWithSubstring(kOCFAllocatorSystemDefault, rest, (OCFRange){0, slash});
                OCFIndex start = slash + 1;
                identifier = OCFStringCreateWithSubstring(
                    kOCFAllocatorSystemDefault, rest, (OCFRange){start, OCFStringGetLength(rest) - start}
                );
                userFromPath = true;
            } else {
                identifier = OCFRetain(rest);
                if (user != NULL) {
                    OCFRelease(user);
                }
                userFromPath = false;
                user = kOCFPreferencesAnyUser;
            }
            pathToRelease = rest;
        } else {
            *outManaged = false;
            OCFStringRef home = OCFStringCreateWithFileSystemRepresentation(
                kOCFAllocatorSystemDefault, _CFGetCachedUnsandboxedHomeDirectoryForCurrentUser()
            );
            OCFRange preferences = OCFStringFind(working, OF_STR("/Library/Preferences/"), 0);
            if (preferences.location == kNotFound) {
                userFromPath = false;
            } else if (preferences.location == 0) {
                if (user != NULL) {
                    OCFRelease(user);
                }
                userFromPath = false;
                user = container == NULL ? kOCFPreferencesAnyUser : kOCFPreferencesCurrentUser;
            } else if (OCFStringHasPrefix(working, OF_STR("/var/db/"))) {
                if (user != NULL) {
                    OCFRelease(user);
                }
                user = OCFStringCreateWithSubstring(
                    kOCFAllocatorSystemDefault, working, (OCFRange){8, preferences.location - 8}
                );
                userFromPath = true;
            } else if (OCFStringHasPrefix(working, OF_STR("/var/"))) {
                if (user != NULL) {
                    OCFRelease(user);
                }
                user = OCFStringCreateWithSubstring(
                    kOCFAllocatorSystemDefault, working, (OCFRange){5, preferences.location - 5}
                );
                userFromPath = true;
            } else if (home != NULL && OCFStringHasPrefix(working, home)) {
                if (user != NULL) {
                    OCFRelease(user);
                }
                userFromPath = false;
                user = kOCFPreferencesCurrentUser;
            } else {
                userFromPath = false;
            }
            if (home != NULL) {
                OCFRelease(home);
            }
            identifier = NULL;
            if (preferences.location != kNotFound && user != NULL) {
                OCFIndex start = preferences.location + preferences.length;
                identifier = OCFStringCreateWithSubstring(
                    kOCFAllocatorSystemDefault,
                    working,
                    (OCFRange){start, OCFStringGetLength(working) - start}
                );
                OCFRelease(working);
                pathToRelease = NULL;
            } else {
                pathToRelease = working;
            }
        }
    }
    if (pathToRelease != NULL) {
        OCFRelease(pathToRelease);
    }
    if (user != NULL) {
        OCFRelease(user);
    }
    OCFStringRef currentUser = kOCFPreferencesCurrentUser;
    if (identifier == NULL) {
        *outFailed = true;
        if (container != NULL) {
            OCFRelease(container);
        }
        if (currentUser != NULL) {
            OCFRelease(currentUser);
        }
        return;
    }
    if (OCFStringHasSuffix(identifier, OF_STR(".plist"))) {
        OCFStringRef trimmed = OCFStringCreateWithSubstring(
            kOCFAllocatorSystemDefault, identifier, (OCFRange){0, OCFStringGetLength(identifier) - 6}
        );
        OCFRelease(identifier);
        identifier = trimmed;
    }
    *outIdentifier = identifier;
    *outContainer = container != NULL ? container : OF_STR("kCFPreferencesNoContainer");
    char containerPath[1024];
    bzero(containerPath, sizeof(containerPath));
    audit_token_t token = {0};
    if (!_CFPrefsGetImpersonatedApplicationAuditToken(&token)) {
        token = _CFAuditTokenForSelf();
    }
    int lookupFailed = sandbox_container_path_for_audit_token(token, containerPath, sizeof(containerPath));
    Boolean noContainer = OCFEqual(*outContainer, OF_STR("kCFPreferencesNoContainer"));
    OCFStringRef resolvedUser;
    if (lookupFailed != 0 && noContainer) {
        *outContainer = NULL;
        resolvedUser = currentUser;
    } else {
        if (lookupFailed == 0) {
            if (noContainer) {
                char identifierBuffer[1024];
                bzero(identifierBuffer, sizeof(identifierBuffer));
                if (OCFStringGetCString(
                        *outIdentifier, identifierBuffer, sizeof(identifierBuffer), kUTF8Encoding
                    )) {
                    int flags = (int)(SANDBOX_CHECK_NO_REPORT | SANDBOX_CHECK_ALLOW_APPROVAL);
                    if (!_CFPrefsSandboxCheck(
                            token, "user-preference-read", flags | kPreferenceDomainFilter, identifierBuffer
                        ) ||
                        !_CFPrefsSandboxCheck(
                            token, "user-preference-write", flags | kPreferenceDomainFilter, identifierBuffer
                        )) {
                        *outContainer = NULL;
                    }
                }
            } else {
                OCFStringRef ownContainer = OCFStringCreateWithCStringNoCopy(
                    kOCFAllocatorSystemDefault, containerPath, OCFStringGetSystemEncoding(), kOCFAllocatorNull
                );
                if (OCFEqual(*outContainer, ownContainer)) {
                    OCFRelease(*outContainer);
                    *outContainer = NULL;
                }
                OCFRelease(ownContainer);
            }
        }
        resolvedUser = currentUser;
        if (*outContainer != NULL) {
            resolvedUser = _OCFPrefsCopyUserForContainer(currentUser, *outContainer);
            OCFRelease(currentUser);
        }
    }
    if (OCFEqual(resolvedUser, OF_STR("wireless"))) {
        OCFRelease(resolvedUser);
        resolvedUser = OF_STR("_wireless");
    }
    if (eduModeEnabled()) {
        OCFStringRef mobileUserName = _CFPrefsCopyUsernameForMobileUser();
        if (OCFEqual(resolvedUser, mobileUserName)) {
            OCFRelease(resolvedUser);
            resolvedUser = _CFPrefsCopyUserConstantForUserName(OF_STR("mobile"));
        } else if (userFromPath) {
            dispatch_once(&mobileHomeOnceToken, ^{
              OCFURLRef home = OCFCopyHomeDirectoryURLForUser(OF_STR("mobile"));
              if (home != NULL) {
                  mobileHome = OCFURLCopyFileSystemPath(home, kPOSIXPathStyle);
                  OCFRelease(home);
              }
            });
            if (mobileHome != NULL && OCFStringFind(mobileHome, resolvedUser, 0).location != kNotFound) {
                OCFRelease(resolvedUser);
                resolvedUser = _CFPrefsCopyUserConstantForUserName(OF_STR("mobile"));
            }
        }
        OCFRelease(mobileUserName);
    }
    *outUser = resolvedUser;
    if (OCFEqual(resolvedUser, kOCFPreferencesAnyUser)) {
        *outByHost = true;
    }
}

// Native _CFPrefsSandboxCheck, 0x1c023af34. Non-external (one of three same-named statics in the image).
// The approval flag is dropped while running on the primary queue. errno is cleared before the query; a -1
// result logs the operation, the path and errno to the daemon log. The approval category, if any, is freed.
static int _CFPrefsSandboxCheck(audit_token_t token, const char *operation, int type, const char *path) {
    uint32_t allowApproval = SANDBOX_CHECK_ALLOW_APPROVAL;
    if ((allowApproval & (uint32_t)type) != 0) {
        type ^= (int)(_CFPrefsRunningOnPrimaryQueue() ? allowApproval : 0);
    }
    char *approvalCategory = NULL;
    errno = 0;
    int result = sandbox_check_and_query_approval_category(token, &approvalCategory, operation, type, path);
    if (result == -1) {
        int error = errno;
        os_log_t log = _CFPrefsDaemonLog();
        os_log_error(
            log,
            "sandbox check for %{public}s on %{private}s failed with error: %{darwin.errno}d",
            operation,
            path,
            error
        );
    }
    if (approvalCategory != NULL) {
        free(approvalCategory);
    }
    return result;
}

// Native _CFPrefsCreateCacheKeyForQuintuplet, 0x1c023b0d4. Hidden (was a private external).
// The application is "C" for the current application and "A" for any application; the user is canonicalized
// (_CFPrefsCopyUserConstantForUserName inlined) to "C" or "A" for the two user constants (the canonical copy
// is released), else the copied name. Without a container (or with kCFPreferencesNoContainer) and not cloud,
// the constant keys "C/C//*/L" and the like are meant for the user constants; the native tests compare the
// abbreviated user key ("C"/"A") with kCFPreferencesCurrentUser and kCFPreferencesAnyUser, so they never
// match and every key is built: application, user, container, "B" or "*", and "C" (cloud) or "L", joined by
// "/" (strncat with the native running limits). Up to 1,018 bytes are built on the stack; larger keys are
// calloc'd (malloc type 0x100004077774924) and handed to the string with kCFAllocatorMalloc. The user key is
// released at the end (a no-op for the "C"/"A" constants).
OCFStringRef _CFPrefsCreateCacheKeyForQuintuplet(
    OCFStringRef identifier, OCFStringRef user, Boolean byHost, OCFStringRef container, Boolean cloud
) {
    OCFStringRef application = _CFPrefsGetCacheStringForBundleID(identifier);
    if (OCFEqual(application, _CFPrefsGetCacheStringForBundleID(kOCFPreferencesCurrentApplication))) {
        application = OF_STR("C");
    } else if (OCFEqual(application, kOCFPreferencesAnyApplication)) {
        application = OF_STR("A");
    }
    OCFStringRef canonicalUser = _CFPrefsCopyUserConstantForUserName(user);
    OCFStringRef userKey;
    if (canonicalUser == kOCFPreferencesCurrentUser) {
        userKey = OF_STR("C");
        if (canonicalUser != NULL) {
            OCFRelease(canonicalUser);
        }
    } else if (canonicalUser == kOCFPreferencesAnyUser) {
        userKey = OF_STR("A");
        if (canonicalUser != NULL) {
            OCFRelease(canonicalUser);
        }
    } else {
        userKey = canonicalUser;
        if (userKey == NULL) {
            return NULL;
        }
    }
    const char *suffix;
    if (cloud) {
        suffix = "C";
    } else {
        if (container == NULL || OCFEqual(container, OF_STR("kCFPreferencesNoContainer"))) {
            OCFStringRef result = NULL;
            if (OCFEqual(application, OF_STR("C"))) {
                if (userKey == kOCFPreferencesCurrentUser) {
                    result = byHost ? OF_STR("C/C//B/L") : OF_STR("C/C//*/L");
                    OCFRelease(userKey);
                    return result;
                }
                result = byHost ? OF_STR("C/A//B/L") : OF_STR("C/A//*/L");
                if (userKey == kOCFPreferencesAnyUser) {
                    OCFRelease(userKey);
                    return result;
                }
            } else if (OCFEqual(application, kOCFPreferencesAnyApplication)) {
                if (userKey == kOCFPreferencesCurrentUser) {
                    result = byHost ? OF_STR("A/C//B/L") : OF_STR("A/C//*/L");
                    OCFRelease(userKey);
                    return result;
                }
                result = byHost ? OF_STR("A/A//B/L") : OF_STR("A/A//*/L");
                if (userKey == kOCFPreferencesAnyUser) {
                    OCFRelease(userKey);
                    return result;
                }
            }
        }
        suffix = "L";
    }
    OCFStringRef containerKey = container != NULL ? container : OF_STR("");
    const char *host = byHost ? "B" : "*";
    OCFIndex total = OCFStringGetMaximumSizeForEncoding(OCFStringGetLength(application), kUTF8Encoding) +
                     OCFStringGetMaximumSizeForEncoding(OCFStringGetLength(userKey), kUTF8Encoding) +
                     OCFStringGetMaximumSizeForEncoding(OCFStringGetLength(containerKey), kUTF8Encoding);
    OCFIndex bound = total + 2;
    size_t size = (size_t)(total + 7);
    UInt8 stackBuffer[bound < 1019 ? size : 1];
    UInt8 *buffer = stackBuffer;
    if (bound < 1019) {
        bzero(buffer, size);
    } else {
        buffer = malloc_type_calloc(size, 1, 0x100004077774924);
    }
    OCFIndex used = 0;
    OCFStringGetBytes(
        application,
        (OCFRange){0, OCFStringGetLength(application)},
        kUTF8Encoding,
        0,
        false,
        buffer,
        (OCFIndex)size,
        &used
    );
    size_t remaining = size + ~(size_t)used;
    UInt8 *cursor = (UInt8 *)strncat((char *)buffer + used, "/", remaining) + 1;
    OCFStringGetBytes(
        userKey,
        (OCFRange){0, OCFStringGetLength(userKey)},
        kUTF8Encoding,
        0,
        false,
        cursor,
        (OCFIndex)remaining,
        &used
    );
    size_t limit = remaining - (size_t)used;
    remaining = limit - 1;
    cursor = (UInt8 *)strncat((char *)cursor + used, "/", remaining) + 1;
    OCFStringGetBytes(
        containerKey,
        (OCFRange){0, OCFStringGetLength(containerKey)},
        kUTF8Encoding,
        0,
        false,
        cursor,
        (OCFIndex)remaining,
        &used
    );
    char *tail = (char *)cursor + used;
    strncat(tail, "/", limit - 2);
    strncat(tail + 1, host, limit - 3);
    strncat(tail + 2, "/", limit - 4);
    strncat(tail + 3, suffix, limit - 5);
    OCFIndex length = (OCFIndex)strlen((const char *)buffer);
    OCFStringRef result;
    if (bound < 1019) {
        result = OCFStringCreateWithBytes(kOCFAllocatorSystemDefault, buffer, length, kUTF8Encoding, false);
    } else {
        result = OCFStringCreateWithBytesNoCopy(
            kOCFAllocatorSystemDefault, buffer, length, kUTF8Encoding, false, kOCFAllocatorMalloc
        );
    }
    OCFRelease(userKey);
    return result;
}

@interface OCFPrefsSource ()
- (id)_descriptionRedactingValues:(BOOL)redact __attribute__((objc_direct));
@end

@implementation OCFPrefsSource

// Native -[CFPrefsSource dealloc], 0x1c023d16c. Reached when the runtime deallocates (round-3 scope). The
// dictionary is released and cleared, the observers released. Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device divergence: unread).
- (void)dealloc {
    if (_dict != NULL) {
        OCFRelease(_dict);
    }
    _dict = NULL;
    objc_release(_observers);
    [super dealloc];
}

// Native -[CFPrefsSource initWithContainingPreferences:], 0x1c023b600. Listed method.
- (instancetype)initWithContainingPreferences:(_OCFXPreferences *)containingPreferences {
    self = [super init];
    if (self != nil) {
        _lock = OS_UNFAIR_LOCK_INIT;
        __atomic_store_n(&shmemEntry, (uint32_t *)&sentinelGeneration, __ATOMIC_RELEASE);
        _containingPreferences = containingPreferences;
        _isSearchList = NO;
    }
    return self;
}

// Native -[CFPrefsSource copyVolatileSourceWithContainingPreferences:], 0x1c023b68c. Listed method.
- (OCFPrefsSource *)copyVolatileSourceWithContainingPreferences:(_OCFXPreferences *)containingPreferences {
    OCFPrefsSource *copy = nil;
    os_unfair_lock_lock(&_lock);
    if ([self isVolatile]) {
        copy = [[OCFPrefsSource alloc] initWithContainingPreferences:containingPreferences];
        if (_dict != NULL) {
            OCFMutableDictionaryRef dictionary =
                OCFDictionaryCreateMutableCopy(kOCFAllocatorSystemDefault, 0, _dict);
            if (dictionary != NULL) {
                if (copy->_dict != NULL) {
                    OCFRelease(copy->_dict);
                }
                copy->_dict = dictionary;
            }
        }
        int64_t generation = __atomic_load_n(&_generationCount, __ATOMIC_ACQUIRE);
        __atomic_store_n(&copy->_generationCount, generation, __ATOMIC_RELEASE);
    }
    os_unfair_lock_unlock(&_lock);
    return copy;
}

// Native -[CFPrefsSource forEachObserver:], 0x1c023b79c. Direct method (no symbol export). Under the lock, a
// temporary id buffer (on the stack up to 256 entries, else _CFCreateArrayStorage, freed on scope exit and on
// unwinding) borrows the observers (+1 each); after unlocking, each observer is passed to the block and then
// released. A count with any of the top four bits set throws NSGenericException (the native new_id_array
// pattern) while the lock is still held (kept: the exceptional path leaves the lock owned).
- (void)forEachObserver:(void (^)(id observer))block {
    os_unfair_lock_lock(&_lock);
    if (_observers == nil) {
        os_unfair_lock_unlock(&_lock);
        return;
    }
    unsigned long count = [(id<PreferencesUnrestoredMessages>)_observers approximateCount];
    if (count >> 60 != 0) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            NULL,
            NULL,
            OF_STR(
                "*** attempt to create a temporary id buffer which is too large or with a negative count "
                "(%lu) -- "
                "possibly data is corrupt"
            ),
            count
        );
        id exception = ((id (*)(id, SEL, id, id, id))objc_msgSend)(
            (id)exceptionClass,
            sel_getUid("exceptionWithName:reason:userInfo:"),
            (id)ONSGenericException,
            (id)reason,
            nil
        );
        OCFRelease(reason);
        objc_exception_throw(exception);
    }
    size_t bufferCount = count > 1 ? count : 1;
    id stackBuffer[count < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    __attribute__((cleanup(releaseTemporaryStorage))) void *allocated = NULL;
    id *objects = stackBuffer;
    if (count >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        allocated = objects;
    }
    unsigned long borrowed = [(id<PreferencesUnrestoredMessages>)_observers borrowObjects:objects
                                                                                    count:count];
    os_unfair_lock_unlock(&_lock);
    for (; borrowed != 0; --borrowed) {
        block(*objects);
        [*objects++ release];
    }
}

// Native -[CFPrefsSource alreadylocked_updateObservingRemoteChanges], 0x1c023b9bc. Listed base method.
- (int)alreadylocked_updateObservingRemoteChanges {
    return 0;
}

// Native -[CFPrefsSource alreadylocked_createObserverUpdateMessageWithOperation:forRole:], 0x1c023b9c4.
// Listed base method.
- (xpc_object_t)alreadylocked_createObserverUpdateMessageWithOperation:(int)operation forRole:(int *)role {
    (void)operation;
    (void)role;
    return NULL;
}

// Native _CFPrefsValidateValueForKey, 0x1c023b9cc, with its .cold.1 (0x1c030d188, the fault log) and .cold.2
// (0x1c030d1d8, the abort). Hidden (was a private external); a C function inside the CFPrefsSource unit.
// NULL and valid binary property lists answer true. Otherwise, with the log-in-progress slot set, the client
// log receives a fault naming the value and key; if NSUserDefaults has registered, NSInvalidArgumentException
// is thrown (with the slot set), else the process aborts with a crash-report message. The slot is cleared on
// both paths (@finally); the exception table ranges are not in the evidence, the try regions follow the
// rethrow structure (confidence medium for the region bounds).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_8_4 (0x1c023e694).
Boolean _CFPrefsValidateValueForKey(OCFPropertyListRef value, OCFStringRef key) {
    if (value == NULL || OCFPropertyListIsValid(value, kBinaryPropertyListFormat)) {
        return true;
    }
    OCFBooleanRef trueValue = kOCFBooleanTrue;
    _OCFSetTSD(kLogInProgressSlot, (void *)trueValue, NULL);
    @try {
        os_log_t log = _CFPrefsClientLog();
        os_log_fault(
            log,
            "Attempt to set a non-property-list object %{public}@ as an NSUserDefaults/CFPreferences value "
            "for key "
            "%{public}@",
            (id)value,
            (id)key
        );
    } @finally {
        _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
    }
    if (_CFPreferencesDoesNSUserDefaultsExist()) {
        _OCFSetTSD(kLogInProgressSlot, (void *)trueValue, NULL);
        @try {
            _CFThrowFormattedException(
                ONSInvalidArgumentException,
                OF_STR("Attempt to insert non-property list object %@ for key %@"),
                value,
                key
            );
        } @finally {
            _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
        }
    }
    gCRAnnotations[1] = (uint64_t)(uintptr_t)"Attempt to set a non-property-list object as an "
                                             "NSUserDefaults/CFPreferences value";
    abort();
}

// Native -[CFPrefsSource alreadylocked_setPrecopiedValues:forKeys:count:from:], 0x1c023bb04. Listed method.
// The log's cleanup region is inferred from the rethrow path (confidence: medium).
- (void)alreadylocked_setPrecopiedValues:(const void **)values
                                 forKeys:(OCFStringRef *)keys
                                   count:(OCFIndex)count
                                    from:(id)from {
    (void)from;
    os_unfair_lock_assert_owner(&_lock);
    if (_dict == NULL) {
        _dict = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    bool wrote = false;
    for (OCFIndex index = 0; index < count; ++index) {
        OCFStringRef key = keys[index];
        OCFTypeRef value = values[index];
        if (![self validateValue:value forKey:key inDict:NULL forWriting:YES]) {
            continue;
        }
        if (value != NULL) {
            OCFDictionarySetValue(_dict, key, value);
        } else {
            OCFDictionaryRemoveValue(_dict, key);
        }
        if (os_log_type_enabled(_CFPrefsClientLog(), OS_LOG_TYPE_DEBUG)) {
            _OCFSetTSD(kLogInProgressSlot, (void *)kOCFBooleanTrue, NULL);
            @try {
                os_log_debug(
                    _CFPrefsClientLog(),
                    "setting new value %{private}@ for key %{private}@ in %{private}@",
                    (id)value,
                    (id)key,
                    self
                );
            } @finally {
                _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
            }
        }
        wrote = true;
    }
    if (wrote) {
        __atomic_fetch_add(&_generationCount, 1, __ATOMIC_ACQ_REL);
    }
}

// Native -[CFPrefsSource validateValue:forKey:inDict:forWriting:], 0x1c023bce0. Direct method. A NULL key is
// valid. For AppleLanguages the value (looked up in the dictionary when NULL and a dictionary is given) must
// be an array of strings; for AppleLocale (the value looked up likewise when still NULL) a string. A missing
// value is valid. An invalid value is logged as an error, with the log-in-progress slot set (cleared in a
// @finally; region bounds inferred from the rethrow structure: confidence medium), to the audit log when
// writing and to the client log otherwise, and NO is answered.
- (BOOL)validateValue:(OCFPropertyListRef)value
               forKey:(OCFStringRef)key
               inDict:(OCFDictionaryRef)dict
           forWriting:(BOOL)forWriting {
    if (key == NULL) {
        return YES;
    }
    OCFPropertyListRef checked = value;
    bool valid = true;
    if (OCFEqual(key, OF_STR("AppleLanguages"))) {
        if (checked == NULL && dict != NULL) {
            checked = OCFDictionaryGetValue(dict, key);
        }
        if (checked == NULL) {
            return YES;
        }
        if (OCFGetTypeID(checked) != OCFArrayGetTypeID()) {
            valid = false;
        } else {
            OCFIndex count = OCFArrayGetCount(checked);
            for (OCFIndex index = 0; index < count; ++index) {
                if (OCFGetTypeID(OCFArrayGetValueAtIndex(checked, index)) != OCFStringGetTypeID()) {
                    valid = false;
                    break;
                }
            }
        }
    }
    if (valid) {
        if (!OCFEqual(key, OF_STR("AppleLocale"))) {
            return YES;
        }
        if (dict != NULL && checked == NULL) {
            checked = OCFDictionaryGetValue(dict, key);
        }
        if (checked == NULL || OCFGetTypeID(checked) == OCFStringGetTypeID()) {
            return YES;
        }
    }
    if (forWriting) {
        _OCFSetTSD(kLogInProgressSlot, (void *)kOCFBooleanTrue, NULL);
        @try {
            os_log_error(
                _CFPrefsLongLivedLog(),
                "attempted to write invalid value %{private}@ for key %{public}@ in %{private}@. Replacing "
                "with NULL.",
                (id)value,
                (id)key,
                self
            );
        } @finally {
            _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
        }
    } else {
        _OCFSetTSD(kLogInProgressSlot, (void *)kOCFBooleanTrue, NULL);
        @try {
            os_log_error(
                _CFPrefsClientLog(),
                "looked up invalid value %{private}@ for key %{public}@ in %{private}@. Replacing with NULL.",
                (id)value,
                (id)key,
                self
            );
        } @finally {
            _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
        }
    }
    return NO;
}

// Native -[CFPrefsSource setValues:forKeys:count:copyValues:removeValuesForKeys:count:from:], 0x1c023bfb0,
// with
// ___83-[CFPrefsSource setValues:forKeys:count:copyValues:removeValuesForKeys:count:from:]_block_invoke
// (0x1c023c434). Direct method.
// Two temporary id buffers (new_id_array pattern, each throwing NSGenericException for a count with any of
// the top four bits set): zeroed values for the removed keys (stack up to 256, else zeroed
// _CFCreateArrayStorage), and the stored values (stack below 257, else _CFCreateArrayStorage), deep-copied
// per key or retained. Under the lock, the stored values (only when count is not 0) and then the removals
// (only when removedCount is not 0) go to -alreadylocked_setPrecopiedValues:forKeys:count:from:; the unlock
// and the release of the stored values form the @finally (region bounds inferred from the rethrow structure:
// confidence medium). Then every observer other than the sender that responds to
// -handleChangeNotificationForDomainIdentifier:isRemote: is told about this domain (not remote). The heap
// buffers are freed on scope exit, stored values first.
- (void)setValues:(const void **)values
                forKeys:(OCFStringRef *)keys
                  count:(OCFIndex)count
             copyValues:(BOOL)copyValues
    removeValuesForKeys:(OCFStringRef *)removedKeys
                  count:(OCFIndex)removedCount
                   from:(id)from {
    size_t nullCount = (size_t)removedCount;
    if (nullCount >> 60 != 0) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            NULL,
            NULL,
            OF_STR(
                "*** attempt to create a temporary id buffer which is too large or with a negative count "
                "(%lu) -- "
                "possibly data is corrupt"
            ),
            nullCount
        );
        id exception = ((id (*)(id, SEL, id, id, id))objc_msgSend)(
            (id)exceptionClass,
            sel_getUid("exceptionWithName:reason:userInfo:"),
            (id)ONSGenericException,
            (id)reason,
            nil
        );
        OCFRelease(reason);
        objc_exception_throw(exception);
    }
    size_t nullBufferCount = nullCount > 1 ? nullCount : 1;
    const void *nullStack[nullCount < 0x101 ? nullBufferCount : 1];
    __attribute__((cleanup(releaseTemporaryStorage))) void *nullAllocated = NULL;
    size_t nullAllocatedSize = 0;
    const void **nulls = nullStack;
    if (nullCount > 0x100) {
        nulls = _OCFCreateArrayStorage(nullBufferCount, true, &nullAllocatedSize);
        nullAllocated = nulls;
    } else {
        bzero(nulls, nullBufferCount * sizeof(const void *));
    }
    size_t copyCount = (size_t)count;
    if (copyCount >> 60 != 0) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            NULL,
            NULL,
            OF_STR(
                "*** attempt to create a temporary id buffer which is too large or with a negative count "
                "(%lu) -- "
                "possibly data is corrupt"
            ),
            copyCount
        );
        id exception = ((id (*)(id, SEL, id, id, id))objc_msgSend)(
            (id)exceptionClass,
            sel_getUid("exceptionWithName:reason:userInfo:"),
            (id)ONSGenericException,
            (id)reason,
            nil
        );
        OCFRelease(reason);
        objc_exception_throw(exception);
    }
    size_t copyBufferCount = copyCount > 1 ? copyCount : 1;
    const void *copyStack[copyCount < 0x101 ? copyBufferCount : 1];
    size_t copyAllocatedSize = 0;
    __attribute__((cleanup(releaseTemporaryStorage))) void *copyAllocated = NULL;
    const void **copies = copyStack;
    if (copyCount >= 0x101) {
        copies = _OCFCreateArrayStorage(copyBufferCount, false, &copyAllocatedSize);
        copyAllocated = copies;
    }
    for (size_t index = 0; index != copyCount; ++index) {
        OCFTypeRef value = values[index];
        if (value != NULL) {
            value = copyValues ? createDeepCopyOfValueForKey(value, keys[index]) : OCFRetain(value);
        }
        copies[index] = value;
    }
    os_unfair_lock_lock(&_lock);
    @try {
        if (count != 0) {
            [self alreadylocked_setPrecopiedValues:copies forKeys:keys count:count from:from];
        }
        if (removedCount != 0) {
            [self alreadylocked_setPrecopiedValues:nulls forKeys:removedKeys count:removedCount from:from];
        }
    } @finally {
        os_unfair_lock_unlock(&_lock);
        const void **copy = copies;
        for (size_t remaining = copyCount; remaining != 0; --remaining) {
            OCFTypeRef value = *copy++;
            if (value != NULL) {
                OCFRelease(value);
            }
        }
    }
    [self forEachObserver:^(id observer) {
      if (observer != from &&
          [observer respondsToSelector:@selector(handleChangeNotificationForDomainIdentifier:isRemote:)]) {
          [(id<PreferencesUnrestoredMessages>)observer
              handleChangeNotificationForDomainIdentifier:[self domainIdentifier]
                                                 isRemote:NO];
      }
    }];
}

// Native -[CFPrefsSource _notifyObserversOfChangeFromValuesForKeys:toValuesForKeys:], 0x1c023c494.
// Listed method.
- (void)_notifyObserversOfChangeFromValuesForKeys:(OCFDictionaryRef)oldValues
                                  toValuesForKeys:(OCFDictionaryRef)newValues {
    // Native ___75-[CFPrefsSource _notifyObserversOfChangeFromValuesForKeys:toValuesForKeys:]_block_invoke,
    // 0x1c023c510. Non-external.
    [self forEachObserver:^(id observer) {
      [observer _notifyObserversOfChangeFromValuesForKeys:oldValues toValuesForKeys:newValues];
    }];
}

// Native -[CFPrefsSource alreadylocked_copyValueForKey:], 0x1c023c928, with .cold.1 at 0x1c030d1f4.
// Listed method. The log's cleanup region is inferred from the rethrow paths (confidence: medium).
- (OCFPropertyListRef)alreadylocked_copyValueForKey:(OCFStringRef)key {
    os_unfair_lock_assert_owner(&_lock);
    OCFPropertyListRef value = _dict != NULL ? OCFDictionaryGetValue(_dict, key) : NULL;
    if (os_log_type_enabled(_CFPrefsClientLog(), OS_LOG_TYPE_DEBUG)) {
        _OCFSetTSD(kLogInProgressSlot, (void *)kOCFBooleanTrue, NULL);
        @try {
            if (value != NULL) {
                os_log_debug(
                    _CFPrefsClientLog(),
                    "looked up value %{private}@ for key %{private}@ in %{private}@",
                    (id)value,
                    (id)key,
                    self
                );
            } else {
                os_log_debug(
                    _CFPrefsClientLog(), "found no value for key %{private}@ in %{private}@", (id)key, self
                );
            }
        } @finally {
            _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
        }
    }
    return value != NULL ? OCFRetain(value) : NULL;
}

// Native -[CFPrefsSource copyValueForKey:], 0x1c023cae8. Listed method.
- (OCFPropertyListRef)copyValueForKey:(OCFStringRef)key {
    os_unfair_lock_lock(&_lock);
    OCFPropertyListRef value = [self alreadylocked_copyValueForKey:key];
    os_unfair_lock_unlock(&_lock);
    return value;
}

// Native -[CFPrefsSource synchronize], 0x1c023cb2c. Listed base method for volatile sources.
- (BOOL)synchronize {
    return YES;
}

// Native -[CFPrefsSource alreadylocked_recordError:], 0x1c023cb34. Listed base method.
- (void)alreadylocked_recordError:(OCFErrorRef)error {
    (void)error;
    os_unfair_lock_assert_owner(&_lock);
}

// Native -[CFPrefsSource setConfigurationPath:], 0x1c023cbcc. Listed, natively empty.
- (void)setConfigurationPath:(OCFStringRef)configurationPath {
    (void)configurationPath;
}

// Native -[CFPrefsSource setStoreName:], 0x1c023cbd0. Listed, natively empty.
- (void)setStoreName:(OCFStringRef)storeName {
    (void)storeName;
}

// Native -[CFPrefsSource setEnabled:], 0x1c023cbd4. Listed, natively empty.
- (void)setEnabled:(BOOL)enabled {
    (void)enabled;
}

// Native -[CFPrefsSource alreadylocked_copyDictionary], 0x1c023cdac. Listed method.
- (OCFMutableDictionaryRef)alreadylocked_copyDictionary {
    os_unfair_lock_assert_owner(&_lock);
    return _dict != NULL ? (OCFMutableDictionaryRef)OCFDictionaryCreateCopy(kOCFAllocatorSystemDefault, _dict)
                         : NULL;
}

// Native -[CFPrefsSource mergeIntoDictionary:sourceDictionary:cloudKeyEvaluator:], 0x1c023cdf4. Listed
// method. The base implementation ignores the cloud-key evaluator.
- (void)mergeIntoDictionary:(OCFMutableDictionaryRef)dictionary
           sourceDictionary:(OCFMutableDictionaryRef)sourceDictionary
          cloudKeyEvaluator:(BOOL (^)(OCFStringRef key))cloudKeyEvaluator {
    (void)cloudKeyEvaluator;
    os_unfair_lock_lock(&_lock);
    OCFDictionaryRef values = [self alreadylocked_copyDictionary];
    if (values != NULL) {
        // Native ___72-[CFPrefsSource mergeIntoDictionary:sourceDictionary:cloudKeyEvaluator:]_block_invoke,
        // 0x1c023ceb8. Non-external.
        _CFPrefsDictionaryApplyBlock(values, ^(const void *key, const void *value) {
          OCFDictionaryAddValue(dictionary, key, value);
          if (sourceDictionary != NULL) {
              OCFDictionaryAddValue(sourceDictionary, key, self);
          }
        });
        OCFRelease(values);
    }
    os_unfair_lock_unlock(&_lock);
}

// Native -[CFPrefsSource createRequestNewContentMessageForDaemon:], 0x1c023cefc. Listed base method.
- (xpc_object_t)createRequestNewContentMessageForDaemon:(int)role {
    (void)role;
    os_unfair_lock_assert_owner(&_lock);
    return NULL;
}

// Native -[CFPrefsSource handleReply:toRequestNewDataMessage:onConnection:retryCount:error:], 0x1c023cf18.
// Listed, natively empty.
- (void)handleReply:(xpc_object_t)reply
    toRequestNewDataMessage:(xpc_object_t)message
               onConnection:(xpc_connection_t)connection
                 retryCount:(int)retryCount
                      error:(bool *)error {
    (void)reply;
    (void)message;
    (void)connection;
    (void)retryCount;
    (void)error;
}

// Native -[CFPrefsSource generationCount], 0x1c023cf28. Listed method.
- (int64_t)generationCount {
    return __atomic_load_n(&_generationCount, __ATOMIC_ACQUIRE);
}

// Native -[CFPrefsSource lock], 0x1c023cf34. Listed method.
- (void)lock {
    os_unfair_lock_lock(&_lock);
}

// Native -[CFPrefsSource unlock], 0x1c023cf3c. Listed method.
- (void)unlock {
    os_unfair_lock_unlock(&_lock);
}

// Native -[CFPrefsSource managed], 0x1c023cf44. Listed base method.
- (BOOL)managed {
    return NO;
}

// Native -[CFPrefsSource isVolatile], 0x1c023cf4c. Listed base method.
- (BOOL)isVolatile {
    return YES;
}

// Native -[CFPrefsSource alreadylocked_clearCache], 0x1c023cf60. Listed, natively empty.
- (void)alreadylocked_clearCache {
}

// Native -[CFPrefsSource userIdentifier], 0x1c023cf74. Listed method.
- (OCFStringRef)userIdentifier {
    return OF_STR("");
}

// Native -[CFPrefsSource servedByUserSessionDaemon], 0x1c023cf80. Listed base method.
- (BOOL)servedByUserSessionDaemon {
    return NO;
}

// Native -[CFPrefsSource domainIdentifier], 0x1c023cf88. Listed method.
- (OCFStringRef)domainIdentifier {
    return OF_STR("Volatile");
}

// Native -[CFPrefsSource isByHost], 0x1c023cf94. Listed base method.
- (BOOL)isByHost {
    return NO;
}

// Native -[CFPrefsSource container], 0x1c023cf9c. Listed method.
- (OCFStringRef)container {
    return OF_STR("");
}

// Native -[CFPrefsSource debugDescription], 0x1c023cfa8. Listed method.
- (id)debugDescription {
    OCFMutableStringRef description = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    os_unfair_lock_lock(&_lock);
    const char *className = object_getClassName(self);
    OCFStringRef domain = [self domainIdentifier];
    OCFStringRef user = [self userIdentifier];
    const char *byHost = [self isByHost] ? "Yes" : "No";
    OCFStringRef container = [self container];
    OCFStringAppendFormat(
        description,
        NULL,
        OF_STR("%s<%p> (Domain: %@, User: %@, ByHost: %s, Container: %@)"),
        className,
        self,
        domain,
        user,
        byHost,
        container
    );
    OCFIndex count = _dict != NULL ? OCFDictionaryGetCount(_dict) : 0;
    const void *keys[count > 0 ? count : 1];
    size_t bytes = sizeof(keys);
    bzero(keys, bytes < 512 ? bytes : 512);
    if (count > 0) {
        OCFDictionaryGetKeysAndValues(_dict, keys, NULL);
    }
    OCFStringAppend(description, OF_STR(" Keys: ["));
    for (OCFIndex index = 0; index < count; ++index) {
        OCFStringAppend(description, keys[index]);
        OCFStringAppend(description, OF_STR(", "));
    }
    OCFStringAppend(description, OF_STR("]\n"));
    os_unfair_lock_unlock(&_lock);
    return (id)OCFAutorelease(description);
}

// Native -[CFPrefsSource setValues:forKeys:count:copyValues:from:], 0x1c030ce08. Direct method.
- (void)setValues:(const void **)values
          forKeys:(OCFStringRef *)keys
            count:(OCFIndex)count
       copyValues:(BOOL)copyValues
             from:(id)from {
    [self setValues:values
                    forKeys:keys
                      count:count
                 copyValues:copyValues
        removeValuesForKeys:NULL
                      count:0
                       from:from];
}

// Native -[CFPrefsSource setValue:forKey:from:], 0x1c030ce20 (with OUTLINED_FUNCTION_7_5 and _2_17, the
// stack-guard check and epilogue). Direct method. The one value and key, copied, with no removals.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_2_17 (0x1c023e648), _OUTLINED_FUNCTION_7_5
// (0x1c023e680).
- (void)setValue:(OCFPropertyListRef)value forKey:(OCFStringRef)key from:(id)from {
    const void *values[1] = {value};
    OCFStringRef keys[1] = {key};
    [self setValues:values forKeys:keys count:1 copyValues:YES removeValuesForKeys:NULL count:0 from:from];
}

// Native -[CFPrefsSource copyDictionary], 0x1c030ceac. Direct method. Folds the lock and unlock helpers
// OUTLINED_FUNCTION_5_7 (0x1c023e668) and OUTLINED_FUNCTION_4_7 (0x1c023e65c).
- (OCFDictionaryRef)copyDictionary {
    os_unfair_lock_lock(&_lock);
    OCFDictionaryRef values = [self alreadylocked_copyDictionary];
    os_unfair_lock_unlock(&_lock);
    return values;
}

// Native -[CFPrefsSource _descriptionRedactingValues:], 0x1c030cee8. Direct method.
// GOT 0x22229c638 is NSString (binding confidence: medium, see OCFPDSource.m).
- (id)_descriptionRedactingValues:(BOOL)redact {
    OCFStringRef user = [self userIdentifier];
    if (redact) {
        user = CFPreferencesRedactedUserIdentifier(user);
    }
    OCFStringRef container = [self container];
    if (redact) {
        container = CFPreferencesRedactedContainerPath(container);
    }
    uint32_t newest = 0;
    if (__atomic_load_n(&shmemEntry, __ATOMIC_ACQUIRE) != NULL) {
        uint32_t *entry = __atomic_load_n(&shmemEntry, __ATOMIC_ACQUIRE);
        newest = __atomic_load_n(entry, __ATOMIC_ACQUIRE);
    }
    Class stringClass = (Class)(void *)sourceStringClass;
    id shmemDescription;
    if (newest == UINT32_MAX) {
        shmemDescription = (id)OF_STR("Sentinel");
    } else {
        int32_t cached = (int32_t)__atomic_load_n(&lastKnownShmemState, __ATOMIC_ACQUIRE);
        long long index = (long long)__atomic_load_n(&shmemIndex, __ATOMIC_ACQUIRE);
        shmemDescription = ((id (*)(Class, SEL, OCFStringRef, ...))objc_msgSend)(
            stringClass,
            @selector(stringWithFormat:),
            OF_STR("Cached: %i, Newest: %i, Index: %lli"),
            cached,
            (int32_t)newest,
            index
        );
    }
    const char *className = object_getClassName(self);
    OCFStringRef domain = [self domainIdentifier];
    const char *byHost = [self isByHost] ? "Yes" : "No";
    uint32_t *entry = __atomic_load_n(&shmemEntry, __ATOMIC_ACQUIRE);
    bool needsRefresh = true;
    if (entry != NULL) {
        uint32_t current = __atomic_load_n(entry, __ATOMIC_ACQUIRE);
        needsRefresh =
            current != UINT32_MAX && current != __atomic_load_n(&lastKnownShmemState, __ATOMIC_ACQUIRE);
    }
    return ((id (*)(Class, SEL, OCFStringRef, ...))objc_msgSend)(
        stringClass,
        @selector(stringWithFormat:),
        OF_STR("%s<%p> (Domain: %@, User: %@, ByHost: %s, Container: %@, Contents Need Refresh: %s (%@)"),
        className,
        self,
        domain,
        user,
        byHost,
        container,
        needsRefresh ? "Yes" : "No",
        shmemDescription
    );
}

// Native -[CFPrefsSource description], 0x1c030d054. Listed method.
- (id)description {
    return [self _descriptionRedactingValues:NO];
}

// Native -[CFPrefsSource redactedDescription], 0x1c030d05c. Listed method.
- (id)redactedDescription {
    return [self _descriptionRedactingValues:YES];
}

@end

// Native createDeepCopyOfValueForKey, 0x1c023c380. Non-external natively (hidden here: declared in
// Preferences/OCFPreferences.h). Tagged pointers and the two CFBoolean constants are answered as they are
// (not retained); a string is copied (CFStringCreateCopy); anything else is deep-copied as an immutable
// property list, and a failed copy is reported through _CFPrefsValidateValueForKey and answers NULL.
OCFTypeRef createDeepCopyOfValueForKey(OCFTypeRef value, OCFStringRef key) {
    if ((intptr_t)value < 0 || value == kOCFBooleanFalse || value == kOCFBooleanTrue) {
        return value;
    }
    OCFTypeID typeID = OCFGetTypeID(value);
    if (typeID == OCFStringGetTypeID()) {
        return OCFStringCreateCopy(kOCFAllocatorSystemDefault, value);
    }
    OCFPropertyListRef copy = OCFPropertyListCreateDeepCopy(kOCFAllocatorSystemDefault, value, 0);
    if (copy == NULL) {
        _CFPrefsValidateValueForKey(value, key);
        return NULL;
    }
    return copy;
}

// Native __useVolatileDomainsForUser.volatileHomeDirs and .volatileHomeDirsLock (__DATA,__bss 0x22e6d3318 and
// 0x22e6d3320) and __overrideUseVolatileDomainsForUser (0x22e6d3308). Non-external; the override's writer is
// outside the evidence, so it stays false in this restoration.
static bool __overrideUseVolatileDomainsForUser;

// Native __useVolatileDomainsForUser: no symbol; inlined into the source-creating block of
// withSourceForIdentifier:user:byHost:container:cloud:perform: (0x1c023d880-0x1c023d9a8), which this function
// reproduces. Under a lock, a per-user cache (created on first use with the CFType callbacks) records whether
// the user's home directory is missing; a missing home logs a debug message (its path and user-name buffers
// are filled and unused, as natively) and makes the user's domains volatile. The override forces volatility.
static bool __useVolatileDomainsForUser(OCFStringRef user) {
    static os_unfair_lock volatileHomeDirsLock = OS_UNFAIR_LOCK_INIT;
    static OCFMutableDictionaryRef volatileHomeDirs;
    os_unfair_lock_lock(&volatileHomeDirsLock);
    if (volatileHomeDirs == NULL) {
        volatileHomeDirs = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    OCFBooleanRef isVolatile = OCFDictionaryGetValue(volatileHomeDirs, user);
    if (isVolatile == NULL) {
        OCFURLRef home =
            OCFCopyHomeDirectoryURLForUser(OCFEqual(kOCFPreferencesCurrentUser, user) ? NULL : user);
        if (_CFURLExists(home)) {
            isVolatile = kOCFBooleanFalse;
        } else {
            UInt8 homePath[1024];
            bzero(homePath, sizeof(homePath));
            OCFURLGetFileSystemRepresentation(home, true, homePath, sizeof(homePath));
            char userName[100];
            OCFStringGetCString(user, userName, sizeof(userName), kUTF8Encoding);
            os_log_t log = _CFPrefsClientLog();
            os_log_debug(
                log, "CFPreferences: user home directory is unavailable. User domains will be volatile."
            );
            isVolatile = kOCFBooleanTrue;
        }
        if (home != NULL) {
            OCFRelease(home);
        }
        OCFDictionarySetValue(volatileHomeDirs, user, isVolatile);
    }
    os_unfair_lock_unlock(&volatileHomeDirsLock);
    return isVolatile == kOCFBooleanTrue || __overrideUseVolatileDomainsForUser;
}

@implementation _OCFXPreferences (SourceAdditions)

// Native -[_CFXPreferences withSourceForIdentifier:user:byHost:container:cloud:perform:], 0x1c023d1f0, with
// its .cold.1 (0x1c030d244, the dispatch_once) and .cold.2 (0x1c030d258, the "!cloud" assertion, line 1193),
// and the blocks ___96-[_CFXPreferences(SourceAdditions)
// withSourceForIdentifier:user:byHost:container:cloud:perform:] _block_invoke (0x1c023d5f0, the cleanup),
// _block_invoke_2 (0x1c023d644, the registration), _block_invoke_3 (0x1c023d6d8, the notification handler)
// and _block_invoke.189 (0x1c023d7c8, the source lookup, with its .cold.1 0x1c030d280 and .cold.2 0x1c030d2bc
// logs). Direct method. byHost survives only for kCFPreferencesAnyUser. The identifier is parsed as a path; a
// path identifier must not be cloud. The user (kCFPreferencesCurrentUser when kCFPreferencesAnyUser was
// asked) or the path's user is mapped through the container; an AnyUser result forces byHost. Managed paths
// go to the managed source. Other requests register (once) for the external-change notification, build the
// cache key and look the source up under the sources lock, creating a cloud or plist source, or a volatile
// CFPrefsSource when the user's home directory is missing (logged with the log-in-progress slot set). The
// block receives the source (+1, released afterwards in @finally together with the pending-KVO delivery;
// region bounds confidence medium).
- (void)withSourceForIdentifier:(OCFStringRef)identifier
                           user:(OCFStringRef)user
                         byHost:(BOOL)byHost
                      container:(OCFStringRef)container
                          cloud:(BOOL)cloud
                        perform:(void (^)(OCFPrefsSource *source))block {
    static dispatch_once_t registerOnce;
    OCFStringRef anyUser = kOCFPreferencesAnyUser;
    if (byHost && !OCFEqual(anyUser, user)) {
        byHost = NO;
    }
    Boolean isAnyUser = OCFEqual(anyUser, user);
    OCFStringRef extractedIdentifier = NULL;
    OCFStringRef extractedUser = NULL;
    OCFStringRef extractedContainer = NULL;
    bool managed = false;
    bool failed;
    _OCFPrefsExtractQuadrupleFromPathIfPossible(
        identifier, &extractedIdentifier, &extractedUser, &extractedContainer, &byHost, &managed, &failed
    );
    OCFStringRef domain = extractedIdentifier != NULL ? extractedIdentifier : identifier;
    if (cloud && extractedIdentifier != NULL) {
        __assert_rtn(
            "-[_CFXPreferences(SourceAdditions) "
            "withSourceForIdentifier:user:byHost:container:cloud:perform:]",
            "CFPrefsSource.m",
            1193,
            "!cloud"
        );
    }
    OCFStringRef requestedUser = isAnyUser ? kOCFPreferencesCurrentUser : user;
    OCFStringRef sourceContainer = extractedContainer != NULL ? extractedContainer : container;
    OCFStringRef containerUser =
        _OCFPrefsCopyUserForContainer(extractedUser != NULL ? extractedUser : requestedUser, sourceContainer);
    BOOL sourceByHost = (!OCFEqual(anyUser, containerUser) || byHost) ? byHost : YES;
    void (^cleanup)(void) = ^{
      if (extractedContainer != NULL) {
          OCFRelease(extractedContainer);
      }
      if (extractedIdentifier != NULL) {
          OCFRelease(extractedIdentifier);
      }
      if (extractedUser != NULL) {
          OCFRelease(extractedUser);
      }
      if (containerUser != NULL) {
          OCFRelease(containerUser);
      }
    };
    if (managed) {
        [self withManagedSourceForIdentifier:domain user:containerUser perform:block];
        cleanup();
        return;
    }
    OCFStringRef cacheDomain = _CFPrefsGetCacheStringForBundleID(domain);
    OCFStringRef sourceUser = _CFPrefsCopyUserConstantForUserName(containerUser);
    dispatch_once(&registerOnce, ^{
      if ((*(volatile uint8_t *)&__CF_FORK_STATE & 2) == 0) {
          __CF_USED();
      }
      if ((*(volatile uint8_t *)&__CF_FORK_STATE & 1) != 0) {
          __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__();
      }
      if ((*(volatile uint8_t *)&__CF_FORK_STATE & 1) != 0) {
          return;
      }
      int token = 0;
      notify_register_dispatch(
          "com.apple.CFPreferences._domainsChangedExternally",
          &token,
          dispatch_get_main_queue(),
          ^(int changed) {
            (void)changed;
            if ((*(volatile uint8_t *)&__CF_FORK_STATE & 2) == 0) {
                __CF_USED();
            }
            if ((*(volatile uint8_t *)&__CF_FORK_STATE & 1) != 0) {
                __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__();
            }
            if ((*(volatile uint8_t *)&__CF_FORK_STATE & 1) != 0) {
                return;
            }
            if (_CFPreferencesDoesNSUserDefaultsExist()) {
                id defaults = (id)_CFPreferencesCopyStandardUserDefaults();
                OCFNotificationCenterPostNotification(
                    OCFNotificationCenterGetLocalCenter(),
                    OF_STR("NSUserDefaultsDidChangeNotification"),
                    defaults,
                    NULL,
                    true
                );
                [defaults release];
            }
            OCFDictionaryRef userInfo = OCFDictionaryCreate(
                kOCFAllocatorSystemDefault,
                NULL,
                NULL,
                0,
                &kOCFTypeDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
            OCFNotificationCenterPostNotification(
                OCFNotificationCenterGetLocalCenter(),
                kOCFPreferencesDidChangeNotificationName,
                kOCFPreferencesDidChangeObject,
                userInfo,
                true
            );
            OCFRelease(userInfo);
          }
      );
    });
    OCFStringRef cacheKey =
        _CFPrefsCreateCacheKeyForQuintuplet(cacheDomain, sourceUser, sourceByHost, sourceContainer, cloud);
    __block OCFPrefsSource *source = nil;
    [self withSources:^(OCFMutableDictionaryRef sources) {
      source = (OCFPrefsSource *)OCFDictionaryGetValue(sources, cacheKey);
      if (source != nil) {
          [source retain];
          return;
      }
      if (!OCFEqual(sourceUser, kOCFPreferencesAnyUser) && __useVolatileDomainsForUser(sourceUser)) {
          _OCFSetTSD(kLogInProgressSlot, (void *)kOCFBooleanTrue, NULL);
          @try {
              os_log_t log = _CFPrefsClientLog();
              os_log_error(
                  log,
                  "Disabling saving preferences for { %{public}@, %{private}@, %{public}s, %{private}@ } "
                  "because "
                  "home directory is inaccessible",
                  (id)cacheDomain,
                  (id)sourceUser,
                  sourceByHost ? "current host" : "any host",
                  (id)sourceContainer
              );
          } @finally {
              _OCFSetTSD(kLogInProgressSlot, NULL, NULL);
          }
          source = [[OCFPrefsSource alloc] initWithContainingPreferences:self];
      } else if (cloud) {
          source = [[OCFPrefsCloudSource alloc] initWithDomain:cacheDomain
                                                          user:sourceUser
                                                        byHost:sourceByHost
                                                 containerPath:sourceContainer
                                         containingPreferences:self];
      } else {
          source = [[OCFPrefsPlistSource alloc] initWithDomain:cacheDomain
                                                          user:sourceUser
                                                        byHost:sourceByHost
                                                 containerPath:sourceContainer
                                         containingPreferences:self];
      }
      OCFDictionarySetValue(sources, cacheKey, source);
    }];
    if (cacheKey != NULL) {
        OCFRelease(cacheKey);
    }
    OCFRelease(sourceUser);
    cleanup();
    bool armed = _CFPrefsArmPendingKVOSlot();
    @try {
        block(source);
    } @finally {
        [source release];
        if (armed) {
            [self _deliverPendingKVONotifications];
        }
    }
}

// Native -[_CFXPreferences withNamedVolatileSourceForIdentifier:perform:], 0x1c023db08, with
// ___81-[_CFXPreferences(SourceAdditions) withNamedVolatileSourceForIdentifier:perform:]_block_invoke
// (0x1c023dcd8). Direct method.
// Under the named-volatile lock (-[_CFXPreferences withNamedVolatileSources:], 0x1c030d0ac, is inlined
// natively, so its lock, call and unlock are written here), the source for the
// cache string is retained or created (a CFPrefsSource registered under a copy of the cache string and seeded
// with the backstop values); the copy is released. The block receives the source (+1, released in @finally
// with the pending-KVO delivery; region bounds confidence medium).
- (void)withNamedVolatileSourceForIdentifier:(OCFStringRef)identifier
                                     perform:(void (^)(OCFPrefsSource *source))block {
    __block OCFPrefsSource *source = nil;
    OCFStringRef cacheDomain = _CFPrefsGetCacheStringForBundleID(identifier);
    OCFStringRef key = OCFStringCreateCopy(kOCFAllocatorSystemDefault, cacheDomain);
    void (^lookUp)(OCFMutableDictionaryRef) = ^(OCFMutableDictionaryRef namedVolatileSources) {
      source = (OCFPrefsSource *)OCFDictionaryGetValue(namedVolatileSources, key);
      if (source != nil) {
          [source retain];
      } else {
          source = [[OCFPrefsSource alloc] initWithContainingPreferences:self];
          OCFDictionarySetValue(namedVolatileSources, key, source);
          _addBackstopValuesForIdentifierAndSource(cacheDomain, source);
      }
      OCFRelease(key);
    };
    os_unfair_lock_lock(&_namedVolatileSourcesLock);
    lookUp(_namedVolatileSources);
    os_unfair_lock_unlock(&_namedVolatileSourcesLock);
    bool armed = _CFPrefsArmPendingKVOSlot();
    @try {
        block(source);
    } @finally {
        [source release];
        if (armed) {
            [self _deliverPendingKVONotifications];
        }
    }
}

// Native getDefaultArguments: no symbol; inlined into _addBackstopValuesForIdentifierAndSource (its
// dispatch_once is that function's .cold.2, 0x1c030d418). The function statics .__NSDefaultArguments and
// .onceToken (__DATA,__bss 0x22e6d3328 and 0x22e6d3330) and the global block ___block_literal_global.230,
// whose invoke is ___getDefaultArguments_block_invoke (0x1c023e3dc, non-external).
// The block does nothing in a setugid process. Otherwise it creates the dictionary (+new) and walks the
// process arguments from index 1 while the index is below count - 1: an argument starting with "-" followed
// by one that does not is an argument-domain pair (the index advances by 2), anything else advances by 1. The
// value of a pair is "" when it is all white space; when its first other character is '"', '(', '<', '[' or
// '{' it is parsed as an old-style or XML property list (_CFPropertyListCreateFromXMLString, autoreleased)
// and, after a parse error (the error string is autoreleased), as property-list data from its UTF-8 bytes
// (CFPropertyListCreateWithData, autoreleased; an error is released); a pair whose value cannot be parsed is
// skipped. Otherwise the value is the string itself. The value is stored under the argument without its "-".
// Any Objective-C exception from the value's handling is caught and the pair skipped (the catch region's
// bounds are inferred from the landing pad, which continues with the next pair: confidence medium).
static id getDefaultArguments(void) {
    static id __NSDefaultArguments;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      if (issetugid() != 0) {
          return;
      }
      __NSDefaultArguments = objc_opt_new((Class)mutableDictionaryClass);
      id processInfo = ((id (*)(id, SEL))objc_msgSend)((id)processInfoClass, sel_getUid("processInfo"));
      id arguments = ((id (*)(id, SEL))objc_msgSend)(processInfo, sel_getUid("arguments"));
      OCFIndex last = ((OCFIndex (*)(id, SEL))objc_msgSend)(arguments, sel_getUid("count")) - 1;
      if (last < 2) {
          return;
      }
      OCFIndex index = 1;
      do {
          id argument =
              ((id (*)(id, SEL, OCFIndex))objc_msgSend)(arguments, sel_getUid("objectAtIndex:"), index);
          OCFIndex next = index + 1;
          if (((BOOL (*)(id, SEL, id))objc_msgSend)(argument, sel_getUid("hasPrefix:"), (id)OF_STR("-"))) {
              id value = ((id (*)(id, SEL, OCFIndex))objc_msgSend)(
                  arguments, sel_getUid("objectAtIndex:"), index + 1
              );
              if (!((BOOL (*)(id, SEL, id))objc_msgSend)(value, sel_getUid("hasPrefix:"), (id)OF_STR("-"))) {
                  @try {
                      id whitespace = ((id (*)(id, SEL))objc_msgSend)(
                          (id)characterSetClass, sel_getUid("whitespaceAndNewlineCharacterSet")
                      );
                      id characters = ((id (*)(id, SEL))objc_msgSend)(whitespace, sel_getUid("invertedSet"));
                      OCFIndex length = ((OCFIndex (*)(id, SEL))objc_msgSend)(value, sel_getUid("length"));
                      OCFRange range = ((OCFRange (*)(id, SEL, id, OCFOptionFlags, OCFRange))objc_msgSend)(
                          value,
                          sel_getUid("rangeOfCharacterFromSet:options:range:"),
                          characters,
                          0,
                          (OCFRange){0, length}
                      );
                      id result;
                      if (range.length == 0) {
                          result = (id)OF_STR("");
                      } else {
                          UniChar first = ((UniChar (*)(id, SEL, OCFIndex))objc_msgSend)(
                              value, sel_getUid("characterAtIndex:"), range.location
                          );
                          if (first == '"' || first == '(' || first == '<' || first == '[' || first == '{') {
                              OCFStringRef errorString = NULL;
                              result = [(id)_OCFPropertyListCreateFromXMLString(
                                  kOCFAllocatorSystemDefault, (OCFStringRef)value, 0, &errorString, true, NULL
                              ) autorelease];
                              if (errorString != NULL) {
                                  [(id)errorString autorelease];
                                  id data = ((id (*)(id, SEL, OCFIndex, BOOL))objc_msgSend)(
                                      value, sel_getUid("dataUsingEncoding:allowLossyConversion:"), 4, NO
                                  );
                                  OCFErrorRef error = NULL;
                                  result = [(id)OCFPropertyListCreateWithData(
                                      kOCFAllocatorSystemDefault, (OCFDataRef)data, 0, NULL, &error
                                  ) autorelease];
                                  if (result == nil && error != NULL) {
                                      OCFRelease(error);
                                      result = nil;
                                  }
                              }
                          } else {
                              result = value;
                          }
                      }
                      if (result != nil) {
                          id key = ((id (*)(id, SEL, OCFIndex))objc_msgSend)(
                              argument, sel_getUid("substringFromIndex:"), 1
                          );
                          ((void (*)(id, SEL, id, id))objc_msgSend)(
                              __NSDefaultArguments, sel_getUid("setObject:forKeyedSubscript:"), result, key
                          );
                      }
                  } @catch (id exception) {
                  }
                  next = index + 2;
              }
          }
          index = next;
      } while (index < last);
    });
    return __NSDefaultArguments;
}

// Native _addBackstopValuesForIdentifierAndSource, 0x1c023dd74, with its .cold.1 (0x1c030d348, the
// registration-domain values) and .cold.2 (0x1c030d418, the dispatch_once of getDefaultArguments).
// Non-external natively (hidden here: declared in Preferences/OCFPreferences.h).
// For NSRegistrationDomain, AppleLanguages ["en-001"], AppleLocale "en_001", NSLanguages ["en-001"] and
// NSInterfaceStyle "macintosh" (one array, the keys from the __DATA_CONST,__const table 0x2227a5f20) are set
// on the source when it is not nil, copying the values, and the array is released. For NSArgumentDomain, the
// default arguments (when there are any) are read into two temporary id buffers (new_id_array pattern:
// NSGenericException for a count with any of the top four bits set; on the stack up to 256 entries, else
// _CFCreateArrayStorage, freed on scope exit and on unwinding, keys first) with -getObjects:andKeys:count:,
// and set on the source when it is not nil, without copying. Other identifiers add nothing.
void _addBackstopValuesForIdentifierAndSource(OCFStringRef identifier, void *source) {
    OCFPrefsSource *prefsSource = (OCFPrefsSource *)source;
    if (OCFEqual(identifier, OF_STR("NSRegistrationDomain"))) {
        const void *languageValues[1] = {OF_STR("en-001")};
        OCFArrayRef languages =
            OCFArrayCreate(kOCFAllocatorSystemDefault, languageValues, 1, &kOCFTypeArrayCallBacks);
        OCFStringRef keys[4] = {
            OF_STR("AppleLanguages"), OF_STR("AppleLocale"), OF_STR("NSLanguages"), OF_STR("NSInterfaceStyle")
        };
        const void *values[4] = {languages, OF_STR("en_001"), languages, OF_STR("macintosh")};
        if (prefsSource != nil) {
            [prefsSource setValues:values
                            forKeys:keys
                              count:4
                         copyValues:YES
                removeValuesForKeys:NULL
                              count:0
                               from:nil];
        }
        OCFRelease(languages);
        return;
    }
    if (!OCFEqual(identifier, OF_STR("NSArgumentDomain"))) {
        return;
    }
    id defaults = getDefaultArguments();
    if (defaults == nil) {
        return;
    }
    size_t count = ((size_t (*)(id, SEL))objc_msgSend)(defaults, sel_getUid("count"));
    if (count >> 60 != 0) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            NULL,
            NULL,
            OF_STR(
                "*** attempt to create a temporary id buffer which is too large or with a negative count "
                "(%lu) -- "
                "possibly data is corrupt"
            ),
            count
        );
        id exception = ((id (*)(id, SEL, id, id, id))objc_msgSend)(
            (id)exceptionClass,
            sel_getUid("exceptionWithName:reason:userInfo:"),
            (id)ONSGenericException,
            (id)reason,
            nil
        );
        OCFRelease(reason);
        objc_exception_throw(exception);
    }
    size_t bufferCount = count > 1 ? count : 1;
    const void *valueStack[count < 0x101 ? bufferCount : 1];
    __attribute__((cleanup(releaseTemporaryStorage))) void *valueAllocated = NULL;
    size_t valueAllocatedSize = 0;
    const void **values = valueStack;
    OCFStringRef keyStack[count < 0x101 ? bufferCount : 2];
    __attribute__((cleanup(releaseTemporaryStorage))) void *keyAllocated = NULL;
    size_t keyAllocatedSize = 0;
    OCFStringRef *keys = keyStack;
    if (count > 0x100) {
        values = _OCFCreateArrayStorage(bufferCount, false, &valueAllocatedSize);
        valueAllocated = values;
        keys = _OCFCreateArrayStorage(bufferCount, false, &keyAllocatedSize);
        keyAllocated = keys;
    }
    ((void (*)(id, SEL, const void **, OCFStringRef *, size_t))objc_msgSend)(
        defaults, sel_getUid("getObjects:andKeys:count:"), values, keys, count
    );
    if (prefsSource != nil) {
        [prefsSource setValues:values
                        forKeys:keys
                          count:(OCFIndex)count
                     copyValues:NO
            removeValuesForKeys:NULL
                          count:0
                           from:nil];
    }
}

// Native -[_CFXPreferences withSources:], 0x1c030d064 (with OUTLINED_FUNCTION_3_9, _1_20 and _6_6). Direct
// method (hidden, was a private external). Runs the block on the sources dictionary under the sources lock.
- (void)withSources:(void (^)(OCFMutableDictionaryRef sources))block {
    os_unfair_lock_lock(&_sourcesLock);
    block(_sources);
    os_unfair_lock_unlock(&_sourcesLock);
}

// Native -[_CFXPreferences withNamedVolatileSources:], 0x1c030d0ac (with OUTLINED_FUNCTION_3_9, _1_20 and
// _6_6). Direct method (hidden, was a private external). Runs the block on the named-volatile sources
// dictionary under the named-volatile lock. withNamedVolatileSourceForIdentifier:perform: inlines it
// natively.
- (void)withNamedVolatileSources:(void (^)(OCFMutableDictionaryRef namedVolatileSources))block {
    os_unfair_lock_lock(&_namedVolatileSourcesLock);
    block(_namedVolatileSources);
    os_unfair_lock_unlock(&_namedVolatileSourcesLock);
}

@end
