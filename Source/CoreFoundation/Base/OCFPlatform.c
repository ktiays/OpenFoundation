//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Preferences/OCFPreferences.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/URL/OCFURL.h"
#include "CoreFoundation/String/OCFStringFormat.h"

#include <mach-o/dyld.h>
#include <malloc/malloc.h>
#include <dispatch/dispatch.h>
#include <errno.h>
#include <limits.h>
#include <mach/mach.h>
#include <os/log.h>
#include <pthread.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sysdir.h>
#include <unistd.h>

// Inferred organization: _CFGetCurrentDirectory (0x1c01cdf84), _CFGetProgname (0x1c01cdfa4), _CFProcessPath
// (0x1c01cdfcc), the home-directory lookup (_CFCopyHomeDirURLForUser 0x1c01ce288,
// CFCopyHomeDirectoryURLForUser 0x1c01ce650 and their blocks 0x1c01ce808 and 0x1c01ceaf8) and the per-thread
// data functions (0x1c01ce8c0-0x1c01ceaf4) lie together in the iOS Simulator __text section, with
// _CFProcessNameString and CFCopyUserName (0x1c01ce14c-0x1c01ce27c) between them.
// CFCopySearchPathForDirectoriesInDomains (0x1c0235b88) and the unsandboxed home-directory functions
// (0x1c02ea940, 0x1c02ef9fc) lie elsewhere and join this unit by the track's assignment; the native source
// paths are not in the evidence.

// libsystem interfaces the native image calls whose prototypes the iOS Simulator SDK does not install
// (libsystem_kernel csops and proc_pidpath, libsystem_pthread pthread_key_init_np and
// _pthread_setspecific_static). They are supplied, never reimplemented; the SDK's libSystem.B.tbd exports
// all four.
extern int csops(pid_t pid, unsigned int ops, void *useraddr, size_t usersize);

extern int proc_pidpath(int pid, void *buffer, uint32_t buffersize);

extern int pthread_key_init_np(int key, void (*destructor)(void *));

extern int _pthread_setspecific_static(unsigned long key, const void *value);

// Native __CFMainThreadHasExited, iOS Simulator 24A434, 0x22e6d3eb8 (__DATA,__common), UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden (was private external); stored with
// STLRB.
// iOS Device 24A446: unread.
uint8_t __CFMainThreadHasExited;

// Native __CFProcessPath and __CFprogname, iOS Simulator 24A434, 0x22e6d2e40 and 0x22e6d2e38 (__DATA,__bss),
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: unread.
static const char *__CFProcessPath;
static const char *__CFprogname;

// Native _CFGetCurrentDirectory, iOS Simulator 24A434, 0x1c01cdf84, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden (was a private external). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json. iOS Device 24A446: unread. getcwd with the
// sign-extended length; whether it answered a buffer.
bool _CFGetCurrentDirectory(char *path, int maxlen) {
    return getcwd(path, (size_t)(int64_t)maxlen) != NULL;
}

// Native _CFGetProgname, iOS Simulator 24A434, 0x1c01cdfa4, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. External.
// iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
const char **_OCFGetProgname(void) {
    if (__CFprogname == NULL) {
        _OCFProcessPath();
    }
    return &__CFprogname;
}

// Native _CFProcessPath, iOS Simulator 24A434, 0x1c01cdfcc, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. External.
// iOS Device 24A446: unread.
// csops(getpid(), 0 (CS_OPS_STATUS), &flags, 4) selects proc_pidpath into a 4,096-byte buffer when it
// succeeds with both 0x4000000 and 0x800 set; otherwise _NSGetExecutablePath fills a 1,026-byte buffer. The
// path is duplicated and the program name points after its last '/'; when neither source succeeds, both
// become the empty string.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
const char *_OCFProcessPath(void) {
    if (__CFProcessPath != NULL) {
        return __CFProcessPath;
    }
    pid_t pid = getpid();
    uint32_t flags = 0;
    if (csops(pid, 0, &flags, 4) == 0 && (flags & 0x4000800) == 0x4000800) {
        char buffer[0x1000];
        if (proc_pidpath(pid, buffer, 0x1000) >= 1) {
            char *path = strdup(buffer);
            __CFProcessPath = path;
            char *slash = strrchr(path, '/');
            __CFprogname = slash != NULL ? slash + 1 : path;
        }
    } else {
        char buffer[0x402];
        uint32_t size = 0x402;
        if (_NSGetExecutablePath(buffer, &size) == 0) {
            char *path = strdup(buffer);
            __CFProcessPath = path;
            char *slash = strrchr(path, '/');
            __CFprogname = slash != NULL ? slash + 1 : path;
        }
    }
    if (__CFProcessPath == NULL) {
        __CFprogname = "";
        __CFProcessPath = "";
    }
    return __CFProcessPath;
}

// Native _CFGetProcessPath, 0x1c01ce124. External. The address of the cached process path, computing it first
// when it is unset. iOS Simulator 24A434, confidence: verified; iOS Device divergence: unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3).
const char **_OCFGetProcessPath(void) {
    if (__CFProcessPath == NULL) {
        _OCFProcessPath();
    }
    return &__CFProcessPath;
}

// Native _CFProcessNameString, iOS Simulator 24A434, 0x1c01ce14c, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Hidden (was a private external); the static
// _CFProcessNameString.__CFProcessNameString (__DATA,__bss) belongs to it.
// iOS Device 24A446: unread.
// On first use the program name (computed by _CFProcessPath when unset; "" when still NULL) becomes a UTF-8
// string of kCFAllocatorSystemDefault, published with a sequentially consistent compare-and-swap (CASAL); a
// losing thread releases its string. The cache is then read again.
OCFStringRef _CFProcessNameString(void) {
    static OCFStringRef __CFProcessNameString;
    if (__CFProcessNameString == NULL) {
        if (__CFprogname == NULL) {
            _OCFProcessPath();
        }
        const char *name = __CFprogname != NULL ? __CFprogname : "";
        OCFStringRef string =
            OCFStringCreateWithCString(kOCFAllocatorSystemDefault, name, kOCFStringEncodingUTF8);
        OCFStringRef expected = NULL;
        if (!__atomic_compare_exchange_n(
                &__CFProcessNameString, &expected, string, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
            )) {
            OCFRelease(string);
        }
    }
    return __CFProcessNameString;
}

// Native CFCopyUserName, iOS Simulator 24A434, 0x1c01ce1d0, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. External.
// iOS Device 24A446: unread.
// The effective user ID from __CFGetUGIDs (getuid when it is 0) selects the password entry; without one or
// without its pw_name, USER (__CFgetenv) is used. A missing name or a failed UTF-8 conversion
// (kCFAllocatorSystemDefault) answers the retained empty constant string (0x2227ac180).
OCFStringRef OCFCopyUserName(void) {
    uint32_t euid = 0;
    __CFGetUGIDs(&euid, NULL);
    uid_t uid = euid;
    if (uid == 0) {
        uid = getuid();
    }
    struct passwd *entry = getpwuid(uid);
    const char *name;
    if (entry == NULL || (name = entry->pw_name) == NULL) {
        name = __CFgetenv("USER");
        if (name == NULL) {
            return OCFRetain(OF_STR(""));
        }
    }
    OCFStringRef result =
        OCFStringCreateWithCString(kOCFAllocatorSystemDefault, name, kOCFStringEncodingUTF8);
    if (result == NULL) {
        return OCFRetain(OF_STR(""));
    }
    return result;
}

// libsystem_darwin's os_variant_has_internal_diagnostics (os/variant_private.h), as the native code imports
// it.
extern bool os_variant_has_internal_diagnostics(const char *subsystem);

// Native _CFCopyHomeDirURLForUser, iOS Simulator 24A434, 0x1c01ce288, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external. Its .cold.1 fragment
// (0x1c030b138, the error log) and ____CFCopyHomeDirURLForUser_block_invoke (0x1c01ceaf8, the block that
// creates the error string with a NULL allocator on first use and otherwise appends "\n") belong to it.
// iOS Device 24A446: unread.
// Unless the process is setugid, CFFIXED_USER_HOME (__CFgetenv) wins when it makes a directory URL. Then the
// password entry of the named user (getpwnam), or of the effective user (__CFGetUGIDs, getuid when it is 0;
// getpwuid) supplies pw_dir. A named user without an entry, or with no usable directory, records no error;
// for the current user each failure appends a message (internal-diagnostics builds include the failing
// value). With fallBackToHome, HOME is tried last. When any message was recorded and no URL was made, the
// messages are logged at the error level of _CFOSLog and "/var/empty" is answered.
static OCFURLRef _CFCopyHomeDirURLForUser(const char *username, Boolean fallBackToHome) {
    const char *fixedHome = issetugid() != 0 ? NULL : __CFgetenv("CFFIXED_USER_HOME");
    __block OCFMutableStringRef errors = NULL;
    void (^beginError)(void) = ^{
      if (errors != NULL) {
          OCFStringAppend(errors, OF_STR("\n"));
      } else {
          errors = OCFStringCreateMutable(NULL, 0);
      }
    };
    OCFURLRef home = NULL;
    if (issetugid() == 0 && fixedHome != NULL) {
        home = OCFURLCreateFromFileSystemRepresentation(
            kOCFAllocatorSystemDefault, (const UInt8 *)fixedHome, strlen(fixedHome), true
        );
        if (home != NULL) {
            goto done;
        }
        beginError();
        if (os_variant_has_internal_diagnostics("com.apple.foundation")) {
            OCFStringAppendFormat(
                errors,
                NULL,
                OF_STR(
                    "CFURLCreateFromFileSystemRepresentation failed to create URL for CFFIXED_USER_HOME "
                    "value: %s"
                ),
                fixedHome
            );
        } else {
            OCFStringAppend(
                errors,
                OF_STR(
                    "CFURLCreateFromFileSystemRepresentation failed to create URL for CFFIXED_USER_HOME value"
                )
            );
        }
    }
    struct passwd *entry;
    if (username != NULL) {
        errno = 0;
        entry = getpwnam(username);
        if (entry == NULL) {
            goto fallBack;
        }
    } else {
        uint32_t euid = 0;
        __CFGetUGIDs(&euid, NULL);
        errno = 0;
        entry = getpwuid(euid != 0 ? euid : getuid());
        if (entry == NULL) {
            int code = errno;
            beginError();
            OCFStringAppendFormat(errors, NULL, OF_STR("getpwuid failed with code: %d"), code);
            goto fallBack;
        }
    }
    if (entry->pw_dir != NULL) {
        home = OCFURLCreateFromFileSystemRepresentation(
            kOCFAllocatorSystemDefault, (const UInt8 *)entry->pw_dir, strlen(entry->pw_dir), true
        );
    }
    if (username == NULL && home == NULL) {
        beginError();
        if (entry->pw_dir != NULL) {
            if (os_variant_has_internal_diagnostics("com.apple.foundation")) {
                OCFStringAppendFormat(
                    errors,
                    NULL,
                    OF_STR(
                        "CFURLCreateFromFileSystemRepresentation failed to create URL for upwd->pw_dir "
                        "value: %s"
                    ),
                    entry->pw_dir
                );
            } else {
                OCFStringAppend(
                    errors,
                    OF_STR(
                        "CFURLCreateFromFileSystemRepresentation failed to create URL for upwd->pw_dir value"
                    )
                );
            }
        } else {
            OCFStringAppend(errors, OF_STR("upwd->pw_dir is NULL"));
        }
    }
    goto checkFallBack;
fallBack:
    home = NULL;
checkFallBack:
    if (fallBackToHome && home == NULL) {
        const char *homeVariable = __CFgetenv("HOME");
        if (homeVariable != NULL) {
            home = OCFURLCreateFromFileSystemRepresentation(
                kOCFAllocatorSystemDefault, (const UInt8 *)homeVariable, strlen(homeVariable), true
            );
            if (home != NULL) {
                goto done;
            }
            beginError();
            if (os_variant_has_internal_diagnostics("com.apple.foundation")) {
                OCFStringAppendFormat(
                    errors,
                    NULL,
                    OF_STR("CFURLCreateFromFileSystemRepresentation failed to create URL for HOME value: %s"),
                    homeVariable
                );
            } else {
                OCFStringAppend(
                    errors,
                    OF_STR("CFURLCreateFromFileSystemRepresentation failed to create URL for HOME value")
                );
            }
        }
        home = NULL;
    }
done:
    if (errors != NULL) {
        if (home == NULL) {
            os_log_error(
                _CFOSLog(),
                "_CFCopyHomeDirURLForUser failed to create a proper home directory. Falling back to "
                "/var/empty. "
                "Error(s):\n%{public}@",
                errors
            );
            home = OCFURLCreateFromFileSystemRepresentation(
                kOCFAllocatorSystemDefault, (const UInt8 *)"/var/empty", 10, true
            );
        }
        OCFRelease(errors);
    }
    return home;
}

// Native CFCopyHomeDirectoryURLForUser, iOS Simulator 24A434, 0x1c01ce650, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External. Its .cold.1 fragment (0x1c030b188,
// the dispatch_once), ___CFCopyHomeDirectoryURLForUser_block_invoke (0x1c01ce808) and the statics
// CFCopyHomeDirectoryURLForUser.home and .once (0x22e6d2e50 and 0x22e6d2e58, __DATA,__bss) belong to it.
// iOS Device 24A446: unread.
// A user name is converted to UTF-8 (the length is truncated to 32 bits first; a stack buffer of 128 bytes
// unless the maximum size is at least 127, then a system-default block of malloc type 0x787CC6D6) and looked
// up without the HOME fallback; a lossy conversion looks up the current user. Without a name, the URL made
// once from CFFIXED_USER_HOME or HOME (getenv, not __CFgetenv) is retained, or the current user is looked up
// with the HOME fallback.
OCFURLRef OCFCopyHomeDirectoryURLForUser(OCFStringRef uName) {
    static OCFURLRef home;
    static dispatch_once_t once;
    if (uName == NULL) {
        dispatch_once(&once, ^{
          const char *path = getenv("CFFIXED_USER_HOME");
          if (path == NULL) {
              path = getenv("HOME");
              if (path == NULL) {
                  return;
              }
          }
          OCFStringRef pathString =
              OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, path);
          if (pathString != NULL) {
              home = OCFURLCreateWithFileSystemPath(
                  kOCFAllocatorSystemDefault, pathString, kOCFURLPOSIXPathStyle, true
              );
              OCFRelease(pathString);
          }
        });
        if (home != NULL) {
            return (OCFURLRef)OCFRetain(home);
        }
        return _CFCopyHomeDirURLForUser(NULL, true);
    }
    UInt8 buffer[128];
    OCFIndex length = (int32_t)OCFStringGetLength(uName);
    int32_t maximumSize = (int32_t)OCFStringGetMaximumSizeForEncoding(length, kOCFStringEncodingUTF8);
    OCFIndex usedLength = 0;
    UInt8 *bytes = buffer;
    if (maximumSize >= 127) {
        bytes =
            OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, (uint32_t)(maximumSize + 1), 0x787CC6D6, 0);
    }
    const char *username = NULL;
    if (OCFStringGetBytes(
            uName, (OCFRange){0, length}, kOCFStringEncodingUTF8, 0, true, bytes, maximumSize, &usedLength
        ) == length) {
        bytes[usedLength] = 0;
        username = (const char *)bytes;
    }
    OCFURLRef result = _CFCopyHomeDirURLForUser(username, false);
    if (bytes != buffer) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, bytes);
    }
    return result;
}

// Native __CFTSDFinalize, iOS Simulator 24A434, 0x1c01ce8c0, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; installed as the destructor of key 55 by __CFTSDGetTable.
// iOS Device 24A446: 0x180675798; constants and calls compared only, body unread.
// Marks the main thread's exit, reinstalls the table under key 55, and unless the value is 0 or 4096 runs
// one destructor pass inside an autorelease pool: each slot with a value and a destructor is cleared and
// its destructor is called with the old value. After the third pass the table is freed and key 55 holds
// 4096 (torn down).
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
void __CFTSDFinalize(void *table) {
    if (pthread_main_np() == 1) {
        __atomic_store_n(&__CFMainThreadHasExited, 1, __ATOMIC_RELEASE);
    }
    _pthread_setspecific_static(kOCFTSDKey, table);
    if (((uintptr_t)table & ~(uintptr_t)0x1000) == 0) {
        return;
    }
    OCFTSDTable *data = table;
    ++data->destructorCount;
    void *pool = _OCFAutoreleasePoolPush();
    for (unsigned slot = 0; slot != kOCFTSDSlotCount; ++slot) {
        uintptr_t value = data->data[slot];
        if (value != 0) {
            OCFTSDDestructor destructor = data->destructors[slot];
            if (destructor != NULL) {
                data->data[slot] = 0;
                destructor((void *)value);
            }
        }
    }
    _OCFAutoreleasePoolPop(pool);
    if (data->destructorCount == 3) {
        free(data);
        _pthread_setspecific_static(kOCFTSDKey, (const void *)0x1000);
    }
}

// Native __CFTSDGetTable, iOS Simulator 24A434, 0x1c01ce9f4, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external.
// iOS Device 24A446: inlined into its callers; unread.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
OCFTSDTable *__CFTSDGetTable(bool create) {
    OCFTSDTable *table = pthread_getspecific(kOCFTSDKey);
    if (table == (OCFTSDTable *)0x1000) {
        return NULL;
    }
    if (create && table == NULL) {
        table = malloc_type_calloc(1, 0x468, 0x1080040FF2AA448);
        pthread_key_init_np(kOCFTSDKey, __CFTSDFinalize);
        _pthread_setspecific_static(kOCFTSDKey, table);
    }
    return table;
}

// Native _CFGetTSDCreateIfNeeded, iOS Simulator 24A434, 0x1c01ce988, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External. The out-of-range path is
// _CFGetTSDCreateIfNeeded.cold.1 (0x1c030b19c), which logs at level 3 and executes BRK #1. iOS Device 24A446:
// 0x1805fed10; constants and calls compared only, body unread. Reading:
// Documentation/Reference/Research/RuntimePrerequisites.md.
void *_OCFGetTSDCreateIfNeeded(uint32_t slot, bool create) {
    if (slot >= kOCFTSDSlotCount) {
        _CFLogSimple(3, "Error: TSD slot %d out of range (get)", slot);
        __builtin_trap();
    }
    OCFTSDTable *table = __CFTSDGetTable(create);
    if (table != NULL) {
        return (void *)table->data[slot];
    }
    if (create) {
        _CFLogSimple(
            4, "Warning: TSD slot %d retrieved but the thread data has already been torn down.", slot
        );
    }
    return NULL;
}

// Native _CFGetTSD, iOS Simulator 24A434, 0x1c01cea74, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. External.
// iOS Device 24A446: 0x1805fec58; the table lookup is inlined; constants compared only, body unread.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
void *_OCFGetTSD(uint32_t slot) {
    return _OCFGetTSDCreateIfNeeded(slot, true);
}

// Native _CFSetTSD, iOS Simulator 24A434, 0x1c01cea7c, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. External. The out-of-range path is _CFSetTSD.cold.1 (0x1c030b1c0), which logs at
// level 3 and executes BRK #1.
// iOS Device 24A446: 0x180612fc8; constants compared only, body unread.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
void *_OCFSetTSD(uint32_t slot, void *value, OCFTSDDestructor destructor) {
    if (slot >= kOCFTSDSlotCount) {
        _CFLogSimple(3, "Error: TSD slot %d out of range (set)", slot);
        __builtin_trap();
    }
    OCFTSDTable *table = __CFTSDGetTable(true);
    if (table == NULL) {
        _CFLogSimple(4, "Warning: TSD slot %d set but the thread data has already been torn down.", slot);
        return NULL;
    }
    void *previous = (void *)table->data[slot];
    table->data[slot] = (uintptr_t)value;
    table->destructors[slot] = destructor;
    return previous;
}

// libsystem_c's private search-path enumeration entry point (the native import; sysdir.h declares only the
// public form). Supplied, never reimplemented.
extern sysdir_search_path_enumeration_state sysdir_start_search_path_enumeration_private(
    sysdir_search_path_directory_t dir, sysdir_search_path_domain_mask_t domainMask
);

// Native CFCopySearchPathForDirectoriesInDomains, iOS Simulator 24A434, 0x1c0235b88, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// iOS Device 24A446: unread.
// A mutable array of kCFAllocatorSystemDefault (kCFTypeArrayCallBacks) collects a directory URL for each path
// of the private sysdir enumeration. With expandTilde, a path starting with '~' is rewritten into the
// 1,026-byte buffer: the home directory's file-system representation (CFCopyHomeDirectoryURLForUser(NULL),
// looked up once per call and only when needed), then the path after '~'; a path whose length plus the home
// length exceeds 1,025 (an unsigned comparison) is skipped. URLs that cannot be made are skipped.
// CoreFoundation defect: when the home directory URL cannot be made, the home length stays -1 and the native
// code stores the terminator one byte before the buffer and appends the path to its uninitialized contents.
// The restoration starts from an empty buffer in that case; the length test is unchanged.
OCFArrayRef
OCFCopySearchPathForDirectoriesInDomains(OCFIndex directory, OCFIndex domainMask, Boolean expandTilde) {
    char path[PATH_MAX];
    char buffer[1026];
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableArrayRef result = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
    sysdir_search_path_enumeration_state state = sysdir_start_search_path_enumeration_private(
        (sysdir_search_path_directory_t)directory, (sysdir_search_path_domain_mask_t)domainMask
    );
    state = sysdir_get_next_search_path_enumeration(state, path);
    if (state == 0) {
        return result;
    }
    OCFIndex homeLength = -1;
    do {
        const char *selected = path;
        Boolean skip = false;
        if (expandTilde && path[0] == '~') {
            if (homeLength < 0) {
                OCFURLRef home = OCFCopyHomeDirectoryURLForUser(NULL);
                if (home != NULL) {
                    OCFURLGetFileSystemRepresentation(home, true, (UInt8 *)buffer, 1026);
                    homeLength = (OCFIndex)strlen(buffer);
                    OCFRelease(home);
                }
            }
            if ((size_t)(strlen(path) + homeLength) > 0x401) {
                skip = true;
            } else {
                buffer[homeLength < 0 ? 0 : homeLength] = 0;
                __builtin___strlcat_chk(buffer, path + 1, 1026, 1026);
                selected = buffer;
            }
        }
        if (!skip) {
            OCFURLRef url = OCFURLCreateFromFileSystemRepresentation(
                allocator, (const UInt8 *)selected, strlen(selected), true
            );
            if (url != NULL) {
                OCFArrayAppendValue(result, url);
                OCFRelease(url);
            }
        }
        state = sysdir_get_next_search_path_enumeration(state, path);
    } while (state != 0);
    return result;
}

// Native _CFUnsandboxedHomeDirectoryForCurrentUser, iOS Simulator 24A434, 0x1c02ea940, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden (was a private external).
// iOS Device 24A446: unread.
// The password entry of the effective user (the real user when the effective one is 0) supplies pw_dir
// (strnlen 1,024) for a directory URL of kCFAllocatorSystemDefault; without an entry, a directory or a URL,
// HOME (__CFgetenv) does; without HOME the URL is NULL and is still passed to
// CFURLGetFileSystemRepresentation. The zeroed 1,024-byte representation (strnlen 1,024) is copied into a
// malloc block (type ID 0x100004077774924) of its length plus one and terminated; the caller owns the block.
const char *_CFUnsandboxedHomeDirectoryForCurrentUser(void) {
    char buffer[1024];
    uid_t euid = geteuid();
    uid_t uid = getuid();
    struct passwd *entry = getpwuid(euid == 0 ? uid : euid);
    OCFURLRef url = NULL;
    Boolean found = false;
    if (entry != NULL && entry->pw_dir != NULL) {
        const char *directory = entry->pw_dir;
        url = OCFURLCreateFromFileSystemRepresentation(
            kOCFAllocatorSystemDefault, (const UInt8 *)directory, (OCFIndex)strnlen(directory, 1024), true
        );
        found = url != NULL;
    }
    if (!found) {
        const char *home = __CFgetenv("HOME");
        url = NULL;
        if (home != NULL) {
            url = OCFURLCreateFromFileSystemRepresentation(
                kOCFAllocatorSystemDefault, (const UInt8 *)home, (OCFIndex)strnlen(home, 1024), true
            );
        }
    }
    bzero(buffer, 1024);
    OCFURLGetFileSystemRepresentation(url, true, (UInt8 *)buffer, 1024);
    size_t length = strnlen(buffer, 1024);
    char *result = malloc_type_malloc(length + 1, 0x100004077774924);
    memmove(result, buffer, length);
    result[length] = 0;
    if (url != NULL) {
        OCFRelease(url);
    }
    return result;
}

// Native _CFGetCachedUnsandboxedHomeDirectoryForCurrentUser, iOS Simulator 24A434, 0x1c02ef9fc, with its
// .cold.1 fragment (0x1c0317048, the dispatch_once) and
// ____CFGetCachedUnsandboxedHomeDirectoryForCurrentUser_block_invoke (0x1c02efa2c, the global block
// ___block_literal_global.171_0), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden
// (was a private external); the statics .unsandboxedHomePath and .onceToken (__DATA,__bss) belong to it. iOS
// Device 24A446: unread.
const char *_CFGetCachedUnsandboxedHomeDirectoryForCurrentUser(void) {
    static const char *unsandboxedHomePath;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      unsandboxedHomePath = _CFUnsandboxedHomeDirectoryForCurrentUser();
    });
    return unsandboxedHomePath;
}
