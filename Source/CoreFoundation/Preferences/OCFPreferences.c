//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Native _OUTLINED_FUNCTION_5_10, 0x1c0291d30. Non-external; folded register/frame/logging setup in its
// callers. Native _OUTLINED_FUNCTION_7_8, 0x1c0291d4c. Non-external; folded register/frame/logging setup in
// its callers.
#include "CoreFoundation/Preferences/OCFPreferences.h"
#include "CoreFoundation/Preferences/OCFPrefsPlistSource.h"
#include "CoreFoundation/URL/OCFURLDeletingLastPathComponent.h"

#include <dispatch/dispatch.h>
#include <errno.h>
#include <fcntl.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <malloc/malloc.h>
#include <os/log.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Inferred organization: three clusters of the iOS Simulator __text section whose restored functions use no
// Objective-C, kept in one C translation unit; the native source paths and the native units' languages are
// not in the evidence. (1) _CFPrefsCopyUsernameForMobileUser (0x1c0290b34) through
// ____CFPrefsGetPathForManagedBundleID_block_invoke_3 (0x1c029104c), after the
// _CFPreferencesCopyApplicationMap entry points, continued by _CFPrefsGetPathForTriplet (0x1c029132c),
// Folded native __CFPrefsWritePlistToFDThenClose.cold.2, 0x1c030f83c.
// Folded native __CFPrefsWritePlistToFDThenClose.cold.3, 0x1c030f878.
// Folded native __CFPrefsWritePlistToFDThenClose.cold.4, 0x1c030f8a8.
// Folded native __CFPrefsWritePlistToFDThenClose.cold.5, 0x1c030f8e8.
// Folded native __CFPrefsWritePlistToFDThenClose.cold.6, 0x1c030f924.
// Folded native __CFPrefsWritePlistToFDThenClose.cold.7, 0x1c030f948.
// _CFPrefsTemporaryFDToWriteTo (0x1c0291514) and _CFPrefsWritePlistToFDThenClose (0x1c029180c, the last two
// from Local/Evidence/cf-core/iphone-simulator-corpus-delta12.json; _CFPrefsWritePlistToFDThenClose sends
// Objective-C messages through objc_msgSend, written from C here, so the native unit's language stays
// open), with their OUTLINED_FUNCTION_0_41, _5_10, _7_8 and _8_7 (0x1c0291ce4-0x1c0291d5c) kept in them; (2)
// _CFPrefsCurrentProcessIsCFPrefsD (0x1c02e5728) and _CFPrefsRunningOnPrimaryQueue (0x1c02e5758), around
// _CFPrefsSetDirectModeEnabled and _CFPrefsSetReadOnly, followed by _CFPrefsDirectMode (0x1c02e578c), its
// block (0x1c02e580c) and _CFPrefsReadOnly (0x1c02e58b8); (3) _CFPrefsGetImpersonatedApplicationAuditToken
// (0x1c02ff65c), after _CFPrefsImpersonateApplicationWithIdentifierAndAuditToken. The .cold fragments
// (0x1c030f618-0x1c030f6b0) and 0x1c03153c0-0x1c03153e8 are kept in their parents. The isCFPrefsD,
// forceDirect and forceReadOnly bytes (0x22e6d3cb0-0x22e6d3cb2) and _CFPrefsSynchronizeIsSynchronous
// (0x22e6d3de8) are this unit's data. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Local/Scratch/cf-core-prefs-daemon/REPORT.md (main checkout; integration reading pending).

// libsystem_containermanager entry point the native code calls; supplied, never reimplemented. The SDK
// installs no declaration. The first argument is passed as 0 at the only call site.
extern char *
container_system_group_path_for_identifier(void *reserved, const char *identifier, uint64_t *error);

// libsystem_sandbox's sandbox_check and its no-report flag (no SDK declaration); supplied. The flag is read
// through GOT slot 0x22241b290 (SANDBOX_CHECK_NO_REPORT, the binding inferred as in
// Preferences/OCFPrefsSource.m: confidence medium).
extern const uint32_t SANDBOX_CHECK_NO_REPORT;
extern int sandbox_check(pid_t pid, const char *operation, int type, ...);

// kCFStringEncodingUTF8 and kCFURLPOSIXPathStyle as the native call sites pass them.
#define kUTF8Encoding ((OCFStringEncoding)0x08000100)
#define kPOSIXPathStyle ((OCFIndex)0)

// Native _CFPrefsCreatePreferencesDirectoryURLForTriplet, 0x1c02904f0 (from
// Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json; it follows
// _CFPrefsCopyDirectoryURLForTriplet_testing in the __text section). Non-external. For kCFPreferencesAnyUser
// (CFEqual): inside cfprefsd with a nonzero effective user, NULL; with a container URL,
// "Library/Preferences/" relative to it; otherwise "/private/var/preferences" (directory URLs, POSIX style,
// system default allocator). For any other user the home URL is the container URL (retained) when given;
// else, for kCFPreferencesCurrentUser or "mobile", $SIMULATOR_SHARED_RESOURCES_DIRECTORY when set; for other
// users the passwd home directory (getpwuid(geteuid()) for kCFPreferencesCurrentUser, which the first test
// already took, else getpwnam of the UTF-8 name, converted in a 128-byte stack buffer or, when the maximum
// size is at least 127, in a malloc'd buffer of that size plus one, passed with the maximum size);
// "/var/empty" when none of these is available. The answer is "Library/Preferences/ByHost/" (byHost) or
// "Library/Preferences/" relative to the home URL, which is released; NULL when no home URL was made.
static OCFURLRef
_CFPrefsCreatePreferencesDirectoryURLForTriplet(OCFStringRef user, bool byHost, OCFURLRef container) {
    if (OCFEqual(user, kOCFPreferencesAnyUser)) {
        if (_CFPrefsCurrentProcessIsCFPrefsD() && geteuid() != 0) {
            return NULL;
        }
        if (container != NULL) {
            return OCFURLCreateWithFileSystemPathRelativeToBase(
                kOCFAllocatorSystemDefault, OF_STR("Library/Preferences/"), kPOSIXPathStyle, true, container
            );
        }
        return OCFURLCreateWithFileSystemPath(
            kOCFAllocatorSystemDefault, OF_STR("/private/var/preferences"), kPOSIXPathStyle, true
        );
    }
    OCFURLRef home;
    if (container != NULL) {
        home = (OCFURLRef)OCFRetain(container);
    } else if (OCFEqual(user, kOCFPreferencesCurrentUser) || OCFEqual(user, OF_STR("mobile"))) {
        const char *shared = getenv("SIMULATOR_SHARED_RESOURCES_DIRECTORY");
        if (shared != NULL) {
            home = OCFURLCreateFromFileSystemRepresentation(
                kOCFAllocatorSystemDefault, (const UInt8 *)shared, (OCFIndex)strlen(shared), true
            );
        } else {
            home = OCFURLCreateWithFileSystemPath(
                kOCFAllocatorSystemDefault, OF_STR("/var/empty"), kPOSIXPathStyle, true
            );
        }
    } else {
        struct passwd *entry;
        if (OCFEqual(user, kOCFPreferencesCurrentUser)) {
            entry = getpwuid(geteuid());
        } else {
            char buffer[128];
            OCFIndex size = OCFStringGetMaximumSizeForEncoding(OCFStringGetLength(user), kUTF8Encoding);
            char *name = (uint64_t)size >= 0x7F ? malloc_type_malloc((size_t)size + 1, 0x8F012185) : buffer;
            if (OCFStringGetCString(user, name, size, kUTF8Encoding)) {
                entry = getpwnam(name);
            } else {
                entry = NULL;
            }
            if (name != buffer) {
                free(name);
            }
        }
        if (entry != NULL && entry->pw_dir != NULL) {
            const char *directory = entry->pw_dir;
            home = OCFURLCreateFromFileSystemRepresentation(
                kOCFAllocatorSystemDefault, (const UInt8 *)directory, (OCFIndex)strlen(directory), true
            );
        } else {
            home = OCFURLCreateWithFileSystemPath(
                kOCFAllocatorSystemDefault, OF_STR("/var/empty"), kPOSIXPathStyle, true
            );
        }
    }
    if (home == NULL) {
        return NULL;
    }
    OCFURLRef result = OCFURLCreateWithFileSystemPathRelativeToBase(
        kOCFAllocatorSystemDefault,
        byHost ? OF_STR("Library/Preferences/ByHost/") : OF_STR("Library/Preferences/"),
        kPOSIXPathStyle,
        true,
        home
    );
    OCFRelease(home);
    return result;
}

// Native _CFPrefsCopyUsernameForMobileUser, 0x1c0290b34, with its .cold.1 (0x1c030f618, the dispatch_once)
// and
// ____CFPrefsCopyUsernameForMobileUser_block_invoke (0x1c0290b7c). Hidden (was a private external).
// Outside education mode the constant "mobile" is answered without a retain; in education mode the user name
// of the "mobile" passwd entry (created once, UTF-8) is answered retained. The block dereferences the
// getpwnam result without a NULL check, as natively.
OCFStringRef _CFPrefsCopyUsernameForMobileUser(void) {
    static dispatch_once_t onceToken;
    static OCFStringRef mobileUserName;
    if (!eduModeEnabled()) {
        return OF_STR("mobile");
    }
    dispatch_once(&onceToken, ^{
      struct passwd *entry = getpwnam("mobile");
      mobileUserName = OCFStringCreateWithCString(kOCFAllocatorSystemDefault, entry->pw_name, kUTF8Encoding);
    });
    return OCFRetain(mobileUserName);
}

// Native _CFPreferencesCopyManagedPreferencesContainerURL, 0x1c0290bc0, with its .cold.1 (0x1c030f62c) and
// .cold.2 (0x1c030f650), the two fault logs. Hidden (was a private external).
// The error out-parameter starts at 1. Without a group container path the daemon log receives a fault naming
// the error and NULL is answered. Otherwise the path becomes a directory URL, "Library/Managed Preferences/"
// is resolved against it, the base is released, a NULL result faults, and the container path is freed.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_1_29 (0x1c0291cf8), _OUTLINED_FUNCTION_3_12
// (0x1c0291d1c), _OUTLINED_FUNCTION_6_9 (0x1c0291d3c).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_2_25 (0x1c0291d0c), _OUTLINED_FUNCTION_4_10
// (0x1c0291d24).
OCFURLRef _CFPreferencesCopyManagedPreferencesContainerURL(void) {
    uint64_t error = 1;
    char *path = container_system_group_path_for_identifier(
        NULL, "systemgroup.com.apple.cfpreferences.managed", &error
    );
    if (path == NULL) {
        os_log_t log = _CFPrefsDaemonLog();
        os_log_fault(log, "Couldn't get container path for managed preferences due to error %llu", error);
        return NULL;
    }
    OCFURLRef result = NULL;
    OCFURLRef base = OCFURLCreateFromFileSystemRepresentation(
        kOCFAllocatorSystemDefault, (const UInt8 *)path, (OCFIndex)strlen(path), true
    );
    if (base != NULL) {
        result = OCFURLCreateWithFileSystemPathRelativeToBase(
            kOCFAllocatorSystemDefault, OF_STR("Library/Managed Preferences/"), kPOSIXPathStyle, true, base
        );
        OCFRelease(base);
        if (result == NULL) {
            os_log_t log = _CFPrefsDaemonLog();
            os_log_fault(log, "Couldn't create URL for managed preferences container");
        }
    }
    free(path);
    return result;
}

// Native _CFPrefsGetPathForManagedBundleID, 0x1c0290cec, with its .cold.1-.cold.3 (0x1c030f688, 0x1c030f69c,
// 0x1c030f6b0, the three dispatch_once calls) and the global blocks
// ____CFPrefsGetPathForManagedBundleID_block_invoke (0x1c0290f90), _block_invoke_2 (0x1c0291030) and
// _block_invoke_3 (0x1c029104c). Hidden (was a private external). The statics are the function statics
// .containingDirectoryPath, .containerizedContainingDirectoryURL, .ourUserName, .onceToken,
// .containerizedOnceToken and .onceToken.21 (__DATA,__bss 0x22e6d37c8, 0x22e6d37d0, 0x22e6d37e8, 0x22e6d37d8,
// 0x22e6d37e0, 0x22e6d37f0). The first block runs once on every call: with a nonempty
// SIMULATOR_SHARED_RESOURCES_DIRECTORY (getenv, not
// __CFgetenv), "Library/Managed Preferences/" is resolved against that directory and its POSIX path is kept;
// both URLs are released. For a containerized lookup the second block keeps the managed preferences container
// URL. The domain is the identifier's cache string (".GlobalPreferences" for kCFPreferencesAnyApplication),
// or none without an identifier or cache string. The base is a retain of the container URL (CFRetain of a
// NULL URL is not guarded, as natively) or a directory URL of the kept path; without a base, false is
// answered. kCFPreferencesAnyUser names "<domain>.plist" (the empty string without a domain).
// kCFPreferencesCurrentUser runs the third block once (in the preferences daemon it keeps the user name) and
// names "<user>/<domain>.plist" or "<user>" for that user name, else for CFCopyUserName's, which is released
// afterwards. Any other user is first mapped to "mobile" when it equals _CFPrefsCopyUsernameForMobileUser's
// answer (released; outside education mode that is the unretained constant, which its release leaves intact),
// then names
// "<user>/<domain>.plist" or a retain of the user. A NULL name answers false. The name is resolved against
// the base as a file, name and base are released, and the URL's file system representation (resolved against
// the base) is written into buffer (1024 bytes); whether it was written is answered after the URL is
// released.
bool _CFPrefsGetPathForManagedBundleID(
    OCFStringRef identifier, OCFStringRef user, bool containerized, char *buffer
) {
    static OCFStringRef containingDirectoryPath;
    static OCFURLRef containerizedContainingDirectoryURL;
    static OCFStringRef ourUserName;
    static dispatch_once_t onceToken;
    static dispatch_once_t containerizedOnceToken;
    static dispatch_once_t userNameOnceToken;
    dispatch_once(&onceToken, ^{
      const char *directory = getenv("SIMULATOR_SHARED_RESOURCES_DIRECTORY");
      if (directory == NULL) {
          return;
      }
      size_t length = strlen(directory);
      if (length == 0) {
          return;
      }
      OCFURLRef base = OCFURLCreateFromFileSystemRepresentation(
          kOCFAllocatorSystemDefault, (const UInt8 *)directory, (OCFIndex)length, true
      );
      OCFURLRef url = OCFURLCreateWithFileSystemPathRelativeToBase(
          kOCFAllocatorSystemDefault, OF_STR("Library/Managed Preferences/"), kPOSIXPathStyle, true, base
      );
      containingDirectoryPath = OCFURLCopyFileSystemPath(url, kPOSIXPathStyle);
      OCFRelease(base);
      OCFRelease(url);
    });
    if (containerized) {
        dispatch_once(&containerizedOnceToken, ^{
          containerizedContainingDirectoryURL = _CFPreferencesCopyManagedPreferencesContainerURL();
        });
    }
    OCFStringRef domain = NULL;
    if (identifier != NULL) {
        OCFStringRef cacheString = _CFPrefsGetCacheStringForBundleID(identifier);
        if (cacheString != NULL) {
            domain = OCFEqual(cacheString, kOCFPreferencesAnyApplication) ? OF_STR(".GlobalPreferences")
                                                                          : cacheString;
        }
    }
    OCFURLRef base;
    if (containerized) {
        base = OCFRetain(containerizedContainingDirectoryURL);
    } else {
        base = OCFURLCreateWithFileSystemPath(
            kOCFAllocatorSystemDefault, containingDirectoryPath, kPOSIXPathStyle, true
        );
    }
    if (base == NULL) {
        return false;
    }
    OCFStringRef name;
    if (OCFEqual(user, kOCFPreferencesAnyUser)) {
        if (domain == NULL) {
            name = OF_STR("");
        } else {
            name = OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR("%@.plist"), domain);
        }
    } else if (OCFEqual(user, kOCFPreferencesCurrentUser)) {
        dispatch_once(&userNameOnceToken, ^{
          if (_CFPrefsCurrentProcessIsCFPrefsD()) {
              ourUserName = OCFCopyUserName();
          }
        });
        OCFStringRef userName = ourUserName;
        if (userName == NULL) {
            userName = OCFCopyUserName();
        }
        if (domain != NULL) {
            name = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault, NULL, OF_STR("%@/%@.plist"), userName, domain
            );
        } else {
            name = OCFRetain(userName);
        }
        if (ourUserName == NULL && userName != NULL) {
            OCFRelease(userName);
        }
    } else {
        OCFStringRef mobileUserName = _CFPrefsCopyUsernameForMobileUser();
        if (mobileUserName != NULL) {
            if (OCFEqual(user, mobileUserName)) {
                user = OF_STR("mobile");
            }
            OCFRelease(mobileUserName);
        }
        if (domain != NULL) {
            name = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault, NULL, OF_STR("%@/%@.plist"), user, domain
            );
        } else {
            name = OCFRetain(user);
        }
    }
    if (name == NULL) {
        OCFRelease(base);
        return false;
    }
    OCFURLRef url = OCFURLCreateWithFileSystemPathRelativeToBase(
        kOCFAllocatorSystemDefault, name, kPOSIXPathStyle, false, base
    );
    OCFRelease(name);
    OCFRelease(base);
    if (url == NULL) {
        return false;
    }
    bool written = OCFURLGetFileSystemRepresentation(url, true, (UInt8 *)buffer, 1024) != 0;
    OCFRelease(url);
    return written;
}

// The simulated out-of-disk-space word of _CFPrefsTestingFlags (offset 12, acquire loads: 1 on creation, 2
// on write) and its slow-write byte (offset 3), as the two functions below read them.
static inline int32_t testingOutOfDiskSpace(void) {
    return __atomic_load_n((const int32_t *)&_OCFPrefsTestingFlags[12], __ATOMIC_ACQUIRE);
}

// Native _CFPrefsTemporaryFDToWriteTo, 0x1c0291514, with .cold.1-.cold.4 (0x1c030f718, 0x1c030f77c,
// 0x1c030f7a0 and 0x1c030f7dc, the log messages; OUTLINED_FUNCTION_5_10 and _7_8 are their argument and
// error-log fragments). Hidden (was a private external).
// With the simulated out-of-disk-space word 1, an error is logged, errno set to ENOSPC and -1 answered. A URL
// without a file-system representation (1,026 bytes, resolved) logs an error and answers -1. Otherwise the
// absolute URL's parent (system default allocator; retained once more) receives "<last path
// component>.XXXXXXX" (the component released when made), and the temporary path is that URL's
// representation (an error logged and -1 answered when there is none). With a protection class, the file is
// made by mkstemp_dprotected_np (flags 0); when that fails with ENOTSUP and the volume (statfs into a buffer
// whose first 512 bytes are zeroed) has MNT_CPROTECT (0x80) set, the creation fails; otherwise, and without a
// class, mkstemp makes it. A failed creation logs a fault with the error and path, restores errno and
// answers -1. A made descriptor gets the owner and mode (fchown, fchmod) and is answered. The URLs are
// released (the parent twice).
int _CFPrefsTemporaryFDToWriteTo(OCFURLRef url, uid_t uid, gid_t gid, mode_t mode, int protectionClass) {
    if (testingOutOfDiskSpace() == 1) {
        os_log_error(_CFPrefsDaemonLog(), "Simulating out of disk space on creation");
        *__error() = ENOSPC;
        return -1;
    }
    UInt8 representation[1026];
    if (!OCFURLGetFileSystemRepresentation(url, true, representation, 1026)) {
        os_log_error(
            _CFPrefsDaemonLog(), "failed to get file system representation for url: %{private}@", url
        );
        return -1;
    }
    OCFURLRef absolute = OCFURLCopyAbsoluteURL(url);
    OCFURLRef parent = OCFURLCreateCopyDeletingLastPathComponent(kOCFAllocatorSystemDefault, absolute);
    OCFURLRef directory = OCFRetain(parent);
    OCFStringRef lastComponent = OCFURLCopyLastPathComponent(absolute);
    OCFRelease(absolute);
    OCFStringRef templateName =
        OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR("%@.XXXXXXX"), lastComponent);
    OCFURLRef temporaryURL =
        OCFURLCreateCopyAppendingPathComponent(kOCFAllocatorSystemDefault, directory, templateName, false);
    OCFRelease(templateName);
    if (lastComponent != NULL) {
        OCFRelease(lastComponent);
    }
    char path[1026];
    if (!OCFURLGetFileSystemRepresentation(temporaryURL, true, (UInt8 *)path, 1026)) {
        os_log_error(
            _CFPrefsDaemonLog(),
            "failed to get file system representation for temporary file path appropriate"
        );
        OCFRelease(temporaryURL);
        OCFRelease(parent);
        OCFRelease(directory);
        return -1;
    }
    int fd;
    bool failed = false;
    if (protectionClass != 0) {
        fd = mkstemp_dprotected_np(path, protectionClass, 0);
        int *error = __error();
        if (fd == -1 && *error == ENOTSUP) {
            struct statfs volume;
            memset(&volume, 0, 512);
            if (statfs(path, &volume) == 0 && (volume.f_flags & 0x80) != 0) {
                failed = true;
            } else {
                fd = mkstemp(path);
            }
        }
    } else {
        fd = mkstemp(path);
    }
    if (failed || fd < 0) {
        int error = *__error();
        os_log_fault(
            _CFPrefsDaemonLog(), "error \"%{public}s\" creating file %{private}s", strerror(error), path
        );
        *__error() = error;
        OCFRelease(temporaryURL);
        OCFRelease(parent);
        OCFRelease(directory);
        return -1;
    }
    fchown(fd, uid, gid);
    fchmod(fd, mode);
    OCFRelease(temporaryURL);
    OCFRelease(parent);
    OCFRelease(directory);
    return fd;
}

// Native _CFPrefsWritePlistToFDThenClose, 0x1c029180c, with .cold.1-.cold.8 (0x1c030f800-0x1c030f988, the
// log messages) and OUTLINED_FUNCTION_0_41 (0x1c0291ce4, the 18-byte fault log) and _8_7 (0x1c0291d5c, the
// shared frame teardown). Hidden (was a private external). The data object answers -length, -bytes and
// -validatePlist (messages); the answer is 0 or an errno-style code.
// A negative descriptor logs a fault and answers EINVAL. No data logs a fault, closes the descriptor and
// answers EINVAL. The descriptor's path (fcntl F_GETPATH) is read first; failing that, the errno is answered
// after a fault. With the simulated out-of-disk-space word 2, a fault is logged and ENOSPC answered. Nonempty
// data is written in full (EINTR and EAGAIN retried, partial writes advanced; another error answers the
// errno after a fault) and fsynced (EINTR retried). The slow-write testing byte sleeps one second once.
// Then the owner and mode are set; data that fails -validatePlist logs a fault and answers 0xbadd. With
// doNotBackUp, the one-byte "com.apple.MobileBackup" attribute is set (a failure logs its return value and
// answers the errno). The descriptor is closed and the path renamed to the destination (relative to
// directoryFD); a failure logs a fault naming the display path and answers the errno, success logs at debug
// level and answers 0. Every failure path closes the descriptor (unless already closed) and unlinks the
// path when one is known (AT_FDCWD, AT_SYMLINK_NOFOLLOW_ANY 0x800).
// CoreFoundation defect: natively the path buffer is uninitialized until F_GETPATH fills it, yet the
// negative-descriptor, missing-data and failed-F_GETPATH paths read it and pass it to unlinkat (the
// negative-descriptor path without a logged fault calls unlinkat unconditionally); stack garbage could name
// a file to delete. Established from the assembly (0x1c0291958, 0x1c0291950, 0x1c02919cc read [SP+0x40]
// before any write on those paths): confidence verified. This restoration zero-initializes the buffer, so
// those paths unlink nothing (the unconditional call receives an empty path).
int _CFPrefsWritePlistToFDThenClose(
    id data,
    int fd,
    uid_t uid,
    gid_t gid,
    mode_t mode,
    bool doNotBackUp,
    int directoryFD,
    const char *destination,
    const char *displayPath
) {
    char path[1024];
    memset(path, 0, sizeof(path));
    int error;
    if (fd < 0) {
        os_log_t log = _CFPrefsDaemonLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
            os_log_fault(log, "Invalid file descriptor %d", fd);
            if (path[0] != 0) {
                unlinkat(AT_FDCWD, path, 0x800);
            }
        } else {
            unlinkat(AT_FDCWD, path, 0x800);
        }
        return EINVAL;
    }
    if (data == nil) {
        os_log_fault(_CFPrefsDaemonLog(), "Error creating data ref from plist, data was %p.", (void *)NULL);
        close(fd);
        if (path[0] != 0) {
            unlinkat(AT_FDCWD, path, 0x800);
        }
        return EINVAL;
    }
    size_t length = ((size_t (*)(id, SEL))objc_msgSend)(data, sel_getUid("length"));
    const uint8_t *bytes = ((const uint8_t *(*)(id, SEL))objc_msgSend)(data, sel_getUid("bytes"));
    if (fcntl(fd, F_GETPATH, path) < 0) {
        error = *__error();
        os_log_fault(_CFPrefsDaemonLog(), "Couldn't get path for fd %d", fd);
        goto closeAndUnlink;
    }
    if (testingOutOfDiskSpace() == 2) {
        os_log_fault(_CFPrefsDaemonLog(), "Simulating out of disk space on write");
        close(fd);
        if (path[0] != 0) {
            unlinkat(AT_FDCWD, path, 0x800);
        }
        return ENOSPC;
    }
    if (length != 0) {
        ssize_t written = write(fd, bytes, length);
        while ((ssize_t)(int)written != (ssize_t)length) {
            int *errorPointer = __error();
            if ((int)written == -1) {
                int writeError = *errorPointer;
                if (writeError != EINTR && writeError != EAGAIN) {
                    error = *__error();
                    os_log_fault(
                        _CFPrefsDaemonLog(),
                        "CFPreferences: error writing file %s: %{darwin.errno}d\n",
                        path,
                        writeError
                    );
                    goto closeAndUnlink;
                }
            } else {
                bytes += (ssize_t)(int)written;
                length -= (size_t)(ssize_t)(int)written;
            }
            written = write(fd, bytes, length);
        }
        while (fsync(fd) < 0) {
            int syncError = *__error();
            if (syncError != EINTR) {
                error = *__error();
                os_log_fault(
                    _CFPrefsDaemonLog(),
                    "CFPreferences: error fsyncing file %s: %{darwin.errno}d\n",
                    path,
                    syncError
                );
                goto closeAndUnlink;
            }
        }
    }
    if (_OCFPrefsTestingFlags[3] != 0) {
        sleep(1);
        _OCFPrefsTestingFlags[3] = 0;
    }
    fchown(fd, uid, gid);
    fchmod(fd, mode);
    if ((((BOOL (*)(id, SEL))objc_msgSend)(data, sel_getUid("validatePlist")) & 1) == 0) {
        os_log_fault(_CFPrefsDaemonLog(), "Invalid data written from preferences to path %s", path);
        close(fd);
        if (path[0] != 0) {
            unlinkat(AT_FDCWD, path, 0x800);
        }
        return 0xBADD;
    }
    if (doNotBackUp) {
        uint8_t value = 1;
        int result = fsetxattr(fd, "com.apple.MobileBackup", &value, 1, 0, 0);
        if (result != 0) {
            error = *__error();
            os_log_fault(
                _CFPrefsDaemonLog(), "error %d setting do-not-back-up attribute on file %s", result, path
            );
            goto closeAndUnlink;
        }
    }
    close(fd);
    int renamed = renameat(AT_FDCWD, path, directoryFD, destination);
    error = *__error();
    os_log_t log = _CFPrefsDaemonLog();
    if (renamed != 0) {
        os_log_fault(log, "error renaming file %s to %s (errno: %d).", path, displayPath, error);
        goto unlink;
    }
    if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
        os_log_debug(_CFPrefsDaemonLog(), "wrote file %{private}s", displayPath);
    }
    return 0;

closeAndUnlink:
    close(fd);
unlink:
    if (path[0] != 0) {
        unlinkat(AT_FDCWD, path, 0x800);
    }
    return error;
}

// Native _CFPrefsGetPathForTriplet, 0x1c029132c (from
// Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json). Hidden (was a private external). The
// preferences directory comes from _CFPrefsCreatePreferencesDirectoryURLForTriplet with the container path as
// a directory URL (made and released around the call) or none; without a directory, false. The identifier's
// cache string (_CFPrefsGetCacheStringForBundleID), ".GlobalPreferences" for kCFPreferencesAnyApplication,
// loses a trailing ".plist" (CFStringFind with case-insensitive, backwards and anchored options, 0xD; a
// substring copy) or is retained. The file name is "%@.%@.plist" with the by-host identifier string
// (_CFXPreferencesGetByHostIdentifierString) when byHost is set and the user is not kCFPreferencesAnyUser,
// and
// "%@.plist" otherwise; the name copy is released. The file URL (not a directory) relative to the directory
// writes its resolved file-system representation into buffer (1024 bytes); the answer is whether that
// succeeded. A failed name or URL answers false. The directory, name and URL are released.
bool _CFPrefsGetPathForTriplet(
    OCFStringRef identifier, OCFStringRef user, bool byHost, OCFStringRef containerPath, char *buffer
) {
    OCFURLRef directory;
    if (containerPath != NULL) {
        OCFURLRef container =
            OCFURLCreateWithFileSystemPath(kOCFAllocatorSystemDefault, containerPath, kPOSIXPathStyle, true);
        directory = _CFPrefsCreatePreferencesDirectoryURLForTriplet(user, byHost, container);
        if (container != NULL) {
            OCFRelease(container);
        }
    } else {
        directory = _CFPrefsCreatePreferencesDirectoryURLForTriplet(user, byHost, NULL);
    }
    if (directory == NULL) {
        return false;
    }
    OCFStringRef name = _CFPrefsGetCacheStringForBundleID(identifier);
    if (OCFEqual(name, kOCFPreferencesAnyApplication)) {
        name = OF_STR(".GlobalPreferences");
    }
    OCFRange suffix = OCFStringFind(name, OF_STR(".plist"), 0xD);
    if (suffix.location != kOCFNotFound) {
        name = OCFStringCreateWithSubstring(kOCFAllocatorSystemDefault, name, (OCFRange){0, suffix.location});
    } else {
        OCFRetain(name);
    }
    OCFStringRef fileName;
    if (byHost && !OCFEqual(user, kOCFPreferencesAnyUser)) {
        fileName = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("%@.%@.plist"),
            name,
            _OCFXPreferencesGetByHostIdentifierString()
        );
    } else {
        fileName = OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR("%@.plist"), name);
    }
    OCFRelease(name);
    if (fileName == NULL) {
        OCFRelease(directory);
        return false;
    }
    OCFURLRef url = OCFURLCreateWithFileSystemPathRelativeToBase(
        kOCFAllocatorSystemDefault, fileName, kPOSIXPathStyle, false, directory
    );
    OCFRelease(fileName);
    OCFRelease(directory);
    if (url == NULL) {
        return false;
    }
    bool written = OCFURLGetFileSystemRepresentation(url, true, (UInt8 *)buffer, 1024) != 0;
    OCFRelease(url);
    return written;
}

// Native isCFPrefsD (__DATA,__bss 0x22e6d3cb0). Non-external natively; hidden here (declared in
// Preferences/OCFPreferences.h: the CFPrefsDaemon methods read it). Its writer is outside the evidence, so it
// stays zero (a client process) in this restoration.
bool isCFPrefsD;

// Native forceDirect (__DATA,__bss 0x22e6d3cb1). Non-external. Set by the direct-mode once block of
// _CFPrefsDirectMode; its other writer, _CFPrefsSetDirectModeEnabled (0x1c02e5734), is outside this
// restoration.
static bool forceDirect;

// Native _CFPrefsCurrentProcessIsCFPrefsD, 0x1c02e5728. Hidden (was a private external). Answers the byte.
bool _CFPrefsCurrentProcessIsCFPrefsD(void) {
    return isCFPrefsD;
}

// Native __MergedGlobals (__DATA,__data 0x22e6cff58 and 0x22e6cff60): two pointers, each initialized to its
// own address, the primary queue's specific key and the value it carries. The native names were merged away.
void *primaryQueueSpecificKey = &primaryQueueSpecificKey;
void *primaryQueueSpecificValue = &primaryQueueSpecificValue;

// Native _CFPrefsRunningOnPrimaryQueue, 0x1c02e5758. Hidden (was a private external).
// True when the current queue hierarchy carries the primary queue's value for its key.
bool _CFPrefsRunningOnPrimaryQueue(void) {
    return dispatch_get_specific(primaryQueueSpecificKey) == primaryQueueSpecificValue;
}

// Native _CFPrefsDirectMode, 0x1c02e578c, with its .cold.1 (0x1c03153c0, the dispatch_once of
// _CFPrefsProcessPathRequiresDirectMode, inlined natively), .cold.2 (0x1c03153d4, the dispatch_once below),
// ____CFPrefsDirectMode_block_invoke (0x1c02e580c, the global block ___block_literal_global_48) and its
// .cold.1 (0x1c03153e8, the fault log, with OUTLINED_FUNCTION_4_15). External. Direct mode is on in the
// preferences daemon, when forced, or for a process path on the direct-mode list; otherwise the once block
// decides:
// __CFPREFERENCES_AVOID_DAEMON in the environment (getenv) forces it, and so does a sandbox that denies the
// mach-lookup of com.apple.cfprefsd.daemon (no-report flag ORed with filter type 2), which is logged as a
// fault. The answer is then the forced byte.
bool _OCFPrefsDirectMode(void) {
    static dispatch_once_t onceToken;
    if ((isCFPrefsD & 1) != 0 || (forceDirect & 1) != 0) {
        return true;
    }
    if (_CFPrefsProcessPathRequiresDirectMode()) {
        return true;
    }
    dispatch_once(&onceToken, ^{
      forceDirect = getenv("__CFPREFERENCES_AVOID_DAEMON") != NULL;
      int denied = sandbox_check(
          getpid(), "mach-lookup", (int)(SANDBOX_CHECK_NO_REPORT | 2), "com.apple.cfprefsd.daemon"
      );
      if (denied != 0) {
          os_log_fault(
              _CFPrefsClientLog(),
              "forcing direct access mode on for this process. It's slower, uses more memory, and doesn't "
              "respond to changes immediately. Update your sandbox profile to allow cfprefsd access!"
          );
      }
      forceDirect = forceDirect | (denied != 0);
    });
    return forceDirect;
}

// Native forceReadOnly (__DATA,__bss 0x22e6d3cb2). Non-external; its writer, _CFPrefsSetReadOnly
// (0x1c02e5744), is outside this restoration, so it stays zero (writable) here.
static bool forceReadOnly;

// Native _CFPrefsReadOnly, 0x1c02e58b8. Hidden (was a private external). Answers the byte.
bool _CFPrefsReadOnly(void) {
    return forceReadOnly;
}

// Native __CFPrefsAuditTokenToImpersonate (__DATA,__bss 0x22e6d3dec, 32 bytes). Non-external natively; hidden
// here (declared in Preferences/OCFPreferences.h: -[CFPrefsPlistSource addPIDImpersonationIfNecessary:] reads
// it). Its writer, _CFPrefsImpersonateApplicationWithIdentifierAndAuditToken (0x1c02ff644), is outside this
// restoration.
audit_token_t _CFPrefsAuditTokenToImpersonate;

// Native _CFPrefsSynchronizeIsSynchronous (__DATA,__bss 0x22e6d3de8). Non-external natively; hidden here
// (declared in Preferences/OCFPreferences.h: -[CFPrefsPlistSource createSynchronizeMessage] reads it). Its
// writer, _CFPrefsSetSynchronizeIsSynchronous (0x1c02ff62c), is outside this restoration.
bool _CFPrefsSynchronizeIsSynchronous;

// Native _CFPrefsGetImpersonatedApplicationAuditToken, 0x1c02ff65c. Hidden (was a private external).
// The token is compared with an all-zero token (four 64-bit exclusive ors, ored together); a nonzero token is
// copied out when outToken is not NULL, and whether it is nonzero is answered.
bool _CFPrefsGetImpersonatedApplicationAuditToken(audit_token_t *outToken) {
    audit_token_t empty = {0};
    bool impersonating = memcmp(&_CFPrefsAuditTokenToImpersonate, &empty, sizeof(audit_token_t)) != 0;
    if (outToken != NULL && impersonating) {
        *outToken = _CFPrefsAuditTokenToImpersonate;
    }
    return impersonating;
}
