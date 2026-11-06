//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include <OpenCoreFoundation/Date/OCFDate.h>
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/PlugIn/OCFBundleMain.h"

#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <math.h>
#include <os/log.h>
#include <os/lock.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <asl.h>
#pragma clang diagnostic pop

// Native _CFOSLog, iOS Simulator 24A434, 0x1c02560d8, with its .cold.1 fragment (0x1c030d9b8, the
// dispatch_once) and ____CFOSLog_block_invoke (0x1c0256108, the global block ___block_literal_global_26),
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden (was a private external); the
// statics _CFOSLog.logger and .onceToken (0x22e6d33b8 and 0x22e6d33c0, __DATA,__bss) belong to it. iOS Device
// 24A446: unread. The handle is os_log_create("com.apple.foundation", "general"), made once.
os_log_t _CFOSLog(void) {
    static os_log_t logger;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      logger = os_log_create("com.apple.foundation", "general");
    });
    return logger;
}

// libdyld (/usr/lib/system/libdyld.dylib, re-exported by libSystem) is a library the native image links;
// it is supplied, never reimplemented. The SDK installs no declaration of these two entry points, so they
// are declared here. dyld's build-version pair has OCFBuildVersion's layout and is passed in X0, as at
// the native call sites; the platform is a 32-bit number returned in W0.
extern uint32_t dyld_get_active_platform(void);

extern bool dyld_program_sdk_at_least(OCFBuildVersion version);

// Native _CFBuildVersionForCFSystemVersion, iOS Simulator 24A434, 0x1c0256b10,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External; its callers are
// _CFPlatformExecutableLinkedOnOrAfter (0x1c0256c7c) and _CFExecutableLinkedOnOrAfter (0x1c0256ca8, with
// dyld_get_active_platform()).
// iOS Device 24A446: 0x18060c1c8, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the
// branches are laid out in another order and its one caller is _CFPlatformExecutableLinkedOnOrAfter;
// the results agree for every input.
// Platform 1 is macOS, 2 iOS and 6 Mac Catalyst (dyld platform numbers). Versions 0..4 map to version 0
// (Mac Catalyst becomes iOS). Versions 5..20 index macos_map for macOS and Mac Catalyst, and otherwise
// give 0 below 10 and 0xffff0000 from 10. Versions 1005..1015 index the second table for macOS only
// (its native bound of eleven entries makes the 0xffff0000 fallback after it unreachable); 1000..1004,
// or other platforms in 1005..1015, map to version 0 (Mac Catalyst becomes iOS). Any
// other version passes its low 32 bits through, except that Mac Catalyst maps its macOS version to an
// iOS version: below 10.15.1 to 13.0, 10.15.1 to 13.2, 10.16 and 11.0 to 14.0 and anything else to
// 0xffff0000. Mac Catalyst entries of macos_map below index 11 (10.5 through 10.15) all map to 13.0.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
OCFBuildVersion _OCFBuildVersionForCFSystemVersion(uint64_t systemVersion, uint32_t platform) {
    // Native _CFBuildVersionForCFSystemVersion.macos_map, iOS Simulator 24A434, 0x1c0456ae0; confidence:
    // verified; sixteen 32-bit entries. iOS Device 24A446: 0x1809651d8; the sixty-four bytes agree.
    static const uint32_t macos_map[16] = {
        0x0a0500,
        0x0a0600,
        0x0a0700,
        0x0a0800,
        0x0a0900,
        0x0a0a00,
        0x0a0b00,
        0x0a0c00,
        0x0a0d00,
        0x0a0e00,
        0x0a0f00,
        0x0a1000,
        0x0c0000,
        0x0d0000,
        0x0e0000,
        0x0f0000,
    };
    // Native unnamed table, iOS Simulator 24A434, 0x1c0456b28 (inside the span nm attributes to
    // _CFLog_os_trace_type_map, which starts eight bytes earlier); confidence: verified for the eleven
    // 32-bit entries the bound admits. iOS Device 24A446: 0x180965220; the forty-four bytes agree.
    // The name is a local descriptive spelling.
    static const uint32_t versionMap1005[11] = {
        0x0a0600,
        0x0a0600,
        0x0a0600,
        0x0a0600,
        0xffff0000,
        0x0a0700,
        0xffff0000,
        0x0a0700,
        0xffff0000,
        0xffff0000,
        0x0a0800,
    };
    uint32_t version = (uint32_t)systemVersion;
    if (systemVersion > 0x14) {
        if (systemVersion - 1000 > 0xf) {
            if (platform != 6) {
                return (OCFBuildVersion){platform, version};
            }
            if (version < 0xa0f01) {
                return (OCFBuildVersion){2, 0xd0000};
            }
        } else {
            if (systemVersion <= 0x3ec || platform != 1) {
                return (OCFBuildVersion){platform == 6 ? 2 : platform, 0};
            }
            uint64_t index = systemVersion - 0x3ed;
            if (index >= 0xb) {
                return (OCFBuildVersion){1, 0xffff0000};
            }
            return (OCFBuildVersion){1, versionMap1005[index]};
        }
    } else {
        if (systemVersion <= 4) {
            return (OCFBuildVersion){platform == 6 ? 2 : platform, 0};
        }
        uint32_t unmapped = systemVersion < 0xa ? 0 : 0xffff0000;
        if (platform != 6 && platform != 1) {
            return (OCFBuildVersion){platform, unmapped};
        }
        uint64_t index = systemVersion - 5;
        version = macos_map[index];
        if (platform != 6) {
            return (OCFBuildVersion){platform, version};
        }
        if (index < 0xb) {
            return (OCFBuildVersion){2, 0xd0000};
        }
    }
    if (version == 0xa0f01) {
        return (OCFBuildVersion){2, 0xd0200};
    }
    if (version == 0xb0000 || version == 0xa1000) {
        return (OCFBuildVersion){2, 0xe0000};
    }
    return (OCFBuildVersion){2, 0xffff0000};
}

// Native _CFPlatformExecutableLinkedOnOrAfter, iOS Simulator 24A434, 0x1c0256c34,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External; its one caller in the image is
// _CFAppVersionCheckLessThan (0x1c0259df0).
// iOS Device 24A446: 0x180703f48, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified for the
// body, which places the Mac Catalyst block after the call; the operations agree. Its branch to
// 0x188138360 leaves the image (a dyld shared-cache direct branch); it is taken to be
// dyld_program_sdk_at_least by position against the simulator; confidence: medium.
// Mac Catalyst (6) does not call _CFBuildVersionForCFSystemVersion: the version is computed inline as
// iOS (2) with 0 for 0..9, 0xffff0000 for 10..20, 0 for 1000..1015 and the low 32 bits otherwise, so
// macos_map and the Mac Catalyst rewrite are not applied. Every other platform maps through
// _CFBuildVersionForCFSystemVersion. dyld_program_sdk_at_least is called last (BL, then the epilogue);
// its result is returned unchanged.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool _OCFPlatformExecutableLinkedOnOrAfter(uint64_t systemVersion, uint32_t platform) {
    OCFBuildVersion version;
    if (platform != 6) {
        version = _OCFBuildVersionForCFSystemVersion(systemVersion, platform);
    } else {
        uint32_t passed = systemVersion - 1000 < 0x10 ? 0 : (uint32_t)systemVersion;
        uint32_t small = systemVersion < 0xa ? 0 : 0xffff0000;
        if (systemVersion < 5) {
            small = 0;
        }
        version = (OCFBuildVersion){2, systemVersion > 0x14 ? passed : small};
    }
    return dyld_program_sdk_at_least(version);
}

// Native _CFExecutableLinkedOnOrAfter, iOS Simulator 24A434, 0x1c0256c8c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External, called from seventy sites in
// the image.
// iOS Device divergence: _CFExecutableLinkedOnOrAfter at 0x18060bfb0 (UUID
// 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified) inlines _CFBuildVersionForCFSystemVersion,
// with the same branches and tables (0x1809651d8, 0x180965220), instead of calling it. Its branches to
// 0x18813ac80 and 0x188138360 leave the image; they are taken to be dyld_get_active_platform and
// dyld_program_sdk_at_least by position against the simulator; confidence: medium.
// The active platform comes from dyld, the version is mapped by _CFBuildVersionForCFSystemVersion and
// dyld_program_sdk_at_least's result is returned unchanged.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool _OCFExecutableLinkedOnOrAfter(uint64_t systemVersion) {
    uint32_t platform = dyld_get_active_platform();
    return dyld_program_sdk_at_least(_OCFBuildVersionForCFSystemVersion(systemVersion, platform));
}

// Native __CFLookupCFNetworkFunction, iOS Simulator 24A434, 0x1c0256cbc, with its .cold.1 fragment
// (0x1c030da48, the dispatch_once) and _____CFLookupCFNetworkFunction_block_invoke (0x1c0256d0c, the global
// block ___block_literal_global.54), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden
// (was a private external); the statics __CFLookupCFNetworkFunction.image and .onceToken (0x22e6d3448 and
// 0x22e6d3450, __DATA,__bss) belong to it.
// iOS Device 24A446: unread.
// CFNetwork is opened once with dlopen mode 5 (RTLD_LAZY | RTLD_LOCAL); without an image the answer is NULL,
// otherwise dlsym's (a tail call).
void *__CFLookupCFNetworkFunction(const char *name) {
    static void *image;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      image = dlopen("/System/Library/Frameworks/CFNetwork.framework/CFNetwork", 5);
    });
    if (image == NULL) {
        return NULL;
    }
    return dlsym(image, name);
}

// Native __CFActiveProcessorCount, iOS Simulator 24A434, 0x1c0256d34, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden (was a private external).
// iOS Device 24A446: unread.
// The byte at the commpage address 0xFFFFFC034 (_COMM_PAGE_ACTIVE_CPUS), zero-extended (LDRB).
int32_t __CFActiveProcessorCount(void) {
    return *(volatile const uint8_t *)(uintptr_t)0xFFFFFC034;
}

// libsystem interfaces the native image calls whose prototypes the iOS Simulator SDK does not install
// (libsystem_pthread pthread_getugid_np and libproc proc_pidinfo). They are supplied, never reimplemented;
// the SDK's libSystem.B.tbd exports both.
extern int pthread_getugid_np(uid_t *uid, gid_t *gid);

extern int proc_pidinfo(int pid, int flavor, uint64_t arg, void *buffer, int buffersize);

// The 64-byte result of proc_pidinfo flavor 13 (PROC_PIDT_SHORTBSDINFO): also_do_stderr reads only the
// flags word at offset 32 (bit 1, a traced process). The type and field names are local descriptive
// spellings of the libproc record; confidence: verified for the offset and size the native code uses.
typedef struct {
    uint32_t pid;
    uint32_t parentPID;
    uint32_t processGroupID;
    uint32_t status;
    char command[16];
    uint32_t flags;
    uint32_t words[7];
} ShortBSDInfo;

_Static_assert(sizeof(ShortBSDInfo) == 64, "proc_pidinfo flavor 13 returns 64 bytes.");

// Native __CFGetUGIDs_cachedUGIDs, iOS Simulator 24A434, 0x22e6cfbf0 (__DATA,__data, initial value
// 0xffffffffffffffff), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external; the
// user ID in the low and the group ID in the high 32 bits. iOS Device 24A446: unread.
static uint64_t __CFGetUGIDs_cachedUGIDs = UINT64_C(0xffffffffffffffff);

// Native __CFGetUGIDs_onceToken and _CFCanChangeEUIDs.onceToken/.canChangeEUIDs, iOS Simulator 24A434;
// confidence: verified. Non-external.
// iOS Device 24A446: unread.
static dispatch_once_t __CFGetUGIDs_onceToken;
static dispatch_once_t canChangeEUIDsOnceToken;
static bool canChangeEUIDs;

// Native _CFCanChangeEUIDs (inlined into __CFGetUGIDs; its dispatch_once is __CFGetUGIDs.cold.1, 0x1c030da5c)
// and ___CFCanChangeEUIDs_block_invoke (0x1c02598a4), iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: unread.
// The effective user ID can change when the KERN_PROC_PID sysctl fails, when the saved user ID
// (kinfo_proc.kp_eproc.e_pcred.p_svuid, offset 396 of the 648-byte record) differs from it, or when the real
// user ID is 0 or differs from it.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
static bool _CFCanChangeEUIDs(void) {
    dispatch_once(&canChangeEUIDsOnceToken, ^{
      uid_t euid = geteuid();
      uid_t uid = getuid();
      struct kinfo_proc information;
      memset(&information, 0, sizeof(information));
      int name[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};
      size_t size = sizeof(information);
      int status = sysctl(name, 4, &information, &size, NULL, 0);
      bool changeable = status != 0;
      uid_t savedUID = status != 0 ? (uid_t)-1 : information.kp_eproc.e_pcred.p_svuid;
      if (savedUID != euid) {
          changeable = true;
      }
      if (uid == 0 || uid != euid) {
          changeable = true;
      }
      canChangeEUIDs = changeable;
    });
    return canChangeEUIDs;
}

_Static_assert(sizeof(struct kinfo_proc) == 648, "The native sysctl buffer is 648 bytes.");

// Native __CFGetUGIDs, iOS Simulator 24A434, 0x1c0256d44, with ____CFGetUGIDs_block_invoke (0x1c0256e48, the
// global block literal 0x2227a6cc8) and ____CFGetUGIDs_block_invoke_2 (0x1c0256eb8, the dispatch_once block
// that captures it), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Hidden (was private
// external). iOS Device 24A446: unread. When the IDs can change they are read each time (the thread's IDs
// from pthread_getugid_np, else geteuid and getegid); otherwise they are read once and cached. Reading:
// Documentation/Reference/Research/RuntimePrerequisites.md.
void __CFGetUGIDs(uint32_t *euid, uint32_t *egid) {
    uint64_t (^read)(void) = ^uint64_t {
      uid_t uid = 0;
      gid_t gid = 0;
      if (pthread_getugid_np(&uid, &gid) != 0) {
          uid = geteuid();
          gid = getegid();
      }
      return (uint64_t)uid | (uint64_t)gid << 32;
    };
    uint64_t ugids;
    if (_CFCanChangeEUIDs()) {
        ugids = read();
    } else {
        dispatch_once(&__CFGetUGIDs_onceToken, ^{
          __CFGetUGIDs_cachedUGIDs = read();
        });
        ugids = __CFGetUGIDs_cachedUGIDs;
    }
    if (euid != NULL) {
        *euid = (uint32_t)ugids;
    }
    if (egid != NULL) {
        *egid = (uint32_t)(ugids >> 32);
    }
}

// Native _CFGetEUID, iOS Simulator 24A434, 0x1c0256ee0, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. External.
// iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
uint32_t _OCFGetEUID(void) {
    uint32_t euid = 0;
    __CFGetUGIDs(&euid, NULL);
    return euid;
}

// Native also_do_stderr, iOS Simulator 24A434, 0x1c02576b8, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external.
// iOS Device 24A446: unread.
// With a nonzero argument: CFLOG_FORCE_STDERR (unless issetugid) answers yes; otherwise fd 2 must be a
// regular file, a socket, a FIFO or a character device. With zero, a traced process (proc_pidinfo flavor
// 13, flag bit 1), OS_ACTIVITY_DT_MODE or CFLOG_FORCE_DISABLE_STDERR answer no first, and
// CFLOG_FORCE_STDERR is honored unless __CFProcessIsRestricted. The st_mode tests are kept as the native
// signed sixteen-bit comparisons.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
static bool also_do_stderr(bool withoutTraceChecks) {
    struct stat status;
    if (withoutTraceChecks) {
        if (issetugid() == 0 && getenv("CFLOG_FORCE_STDERR") != NULL) {
            return true;
        }
        memset(&status, 0, sizeof(status));
        if (fstat(2, &status) < 0) {
            return false;
        }
        int16_t mode = (int16_t)status.st_mode;
        if (mode < -0x7000) {
            return true;
        }
        uint32_t type = (uint16_t)mode & 0xf000;
        if (type == 0xc000) {
            return true;
        }
        if (type == 0x1000 || type == 0x2000) {
            return withoutTraceChecks;
        }
        return false;
    }
    ShortBSDInfo information;
    memset(&information, 0, sizeof(information));
    if (proc_pidinfo(getpid(), 13, 0, &information, 64) == 64 && (information.flags & 2) != 0) {
        return false;
    }
    if (getenv("OS_ACTIVITY_DT_MODE") != NULL || getenv("CFLOG_FORCE_DISABLE_STDERR") != NULL) {
        return false;
    }
    if (!__CFProcessIsRestricted() && getenv("CFLOG_FORCE_STDERR") != NULL) {
        return true;
    }
    memset(&status, 0, sizeof(status));
    if (fstat(2, &status) < 0) {
        return false;
    }
    int32_t mode = (int16_t)status.st_mode;
    uint32_t type = (uint32_t)mode & 0xf000;
    bool result = type == 0xc000 ? true : withoutTraceChecks;
    if (type == 0x2000) {
        result = true;
    }
    if (((uint32_t)mode & 0xfffff000) == 0x1000) {
        result = true;
    }
    return mode < -0x7000 ? true : result;
}

// Native _populateBanner, iOS Simulator 24A434, 0x1c0259a70, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external.
// iOS Device 24A446: unread.
// The seconds are floor(now + kCFAbsoluteTimeIntervalSince1970) (FRINTM, FCVTZS) in local time and the
// milliseconds floor(fraction(now) * 1000) (FCVTMS); the thread ID is pthread_threadid_np, else the Mach
// thread port.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
static void _populateBanner(char **banner, char **localTime, char **thread) {
    double now = OCFAbsoluteTimeGetCurrent();
    time_t seconds = (time_t)floor(now + kOCFAbsoluteTimeIntervalSince1970);
    struct tm parts;
    memset(&parts, 0, sizeof(parts));
    localtime_r(&seconds, &parts);
    int month = parts.tm_mon;
    int year = parts.tm_year;
    int hour = parts.tm_hour;
    int day = parts.tm_mday;
    uint64_t threadID = 0;
    int second = parts.tm_sec;
    int minute = parts.tm_min;
    if (pthread_threadid_np(NULL, &threadID) != 0) {
        threadID = pthread_mach_thread_np(pthread_self());
    }
    double whole;
    int milliseconds = (int)floor(modf(now, &whole) * 1000.0);
    asprintf(
        banner,
        "%04d-%02d-%02d %02d:%02d:%02d.%03d %s[%d:%llu] ",
        year + 1900,
        month + 1,
        day,
        hour,
        minute,
        second,
        milliseconds,
        *_OCFGetProgname(),
        getpid(),
        (unsigned long long)threadID
    );
    asprintf(thread, "%x", pthread_mach_thread_np(pthread_self()));
    asprintf(
        localTime,
        "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        year + 1900,
        month + 1,
        day,
        hour,
        minute,
        second,
        milliseconds
    );
}

// Native _logToStderr.lock, iOS Simulator 24A434, 0x22e6d3480 (__DATA,__bss); confidence: verified.
// Non-external.
// iOS Device 24A446: unread.
static os_unfair_lock _logToStderrLock = OS_UNFAIR_LOCK_INIT;

// Native _logToStderr, iOS Simulator 24A434, 0x1c0259bf4, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external.
// iOS Device 24A446: unread.
// One writev(2) of the banner (when present), the message and a newline when the message does not end with
// one, under a private lock. The message's last byte is read without a length check, as natively.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
static void _logToStderr(const char *banner, const char *message, size_t length) {
    struct iovec vectors[4];
    vectors[0].iov_base = (void *)banner;
    int count;
    if (banner != NULL) {
        vectors[0].iov_len = strlen(banner);
        count = 2;
    } else {
        vectors[0].iov_len = 0;
        count = 1;
    }
    vectors[1].iov_base = (void *)message;
    vectors[1].iov_len = length;
    vectors[2].iov_base = "\n";
    uint8_t last = (uint8_t)message[length - 1];
    vectors[2].iov_len = last != '\n';
    if (last != '\n') {
        ++count;
    }
    os_unfair_lock_lock(&_logToStderrLock);
    writev(2, banner != NULL ? &vectors[0] : &vectors[1], count);
    os_unfair_lock_unlock(&_logToStderrLock);
}

// Native _bundleInfo.info.0, .info.1, .info.2 and .onceToken (__DATA,__bss), iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external: the main bundle's identifier,
// short version string and that string's double value, filled once. _bundleInfo itself has no native body;
// _CFAppVersionCheckLessThan inlines it. iOS Device 24A446: unread.
static OCFStringRef _bundleInfo_identifier;
static OCFStringRef _bundleInfo_shortVersion;
static double _bundleInfo_version;
static dispatch_once_t _bundleInfo_onceToken;

// Native ____bundleInfo_block_invoke, 0x1c0259f9c: the global block ___block_literal_global_27 that the
// inlined _bundleInfo runs once, shared by _CFAppVersionCheck and _CFAppVersionCheckLessThan. It reads the
// main bundle's info dictionary: the identifier and short version string are copied with
// kCFAllocatorSystemDefault, or are the empty constant string when absent (both when there is no bundle or
// dictionary), and the version is CFStringGetDoubleValue of the short version string. bundleInfoInitBlock is
// a local descriptive name.
static void (^const bundleInfoInitBlock)(void) = ^{
  OCFBundleRef mainBundle = OCFBundleGetMainBundle();
  OCFDictionaryRef infoDictionary = mainBundle != NULL ? OCFBundleGetInfoDictionary(mainBundle) : NULL;
  OCFStringRef shortVersion;
  if (infoDictionary == NULL) {
      shortVersion = OF_STR("");
      _bundleInfo_identifier = OF_STR("");
  } else {
      OCFStringRef identifier = OCFDictionaryGetValue(infoDictionary, kOCFBundleIdentifierKey);
      OCFStringRef versionString = OCFDictionaryGetValue(infoDictionary, _kOCFBundleShortVersionStringKey);
      _bundleInfo_identifier =
          identifier != NULL ? OCFStringCreateCopy(kOCFAllocatorSystemDefault, identifier) : OF_STR("");
      shortVersion =
          versionString != NULL ? OCFStringCreateCopy(kOCFAllocatorSystemDefault, versionString) : OF_STR("");
  }
  _bundleInfo_shortVersion = shortVersion;
  _bundleInfo_version = OCFStringGetDoubleValue(shortVersion);
};

// Native _CFAppVersionCheck, iOS Simulator 24A434, 0x1c0259ce8, with .cold.1 (0x1c030da74, the dispatch_once
// through OUTLINED_FUNCTION_0_33), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json. iOS Device 24A446: unread. Unlike
// _CFAppVersionCheckLessThan it has no NULL-identifier crash check: a different identifier answers CFEqual's
// false; a NULL version answers 1; otherwise whether the stored short version string equals it.
bool _OCFAppVersionCheck(OCFStringRef bundleID, OCFStringRef version) {
    dispatch_once(&_bundleInfo_onceToken, bundleInfoInitBlock);
    if (!OCFEqual(_bundleInfo_identifier, bundleID)) {
        return false;
    }
    if (version == NULL) {
        return true;
    }
    return OCFEqual(_bundleInfo_shortVersion, version);
}

// Native _CFAppVersionCheckLessThan, iOS Simulator 24A434, 0x1c0259dbc, with .cold.1 (0x1c030da7c, the
// dispatch_once through OUTLINED_FUNCTION_0_33, 0x1c025a06c), .cold.2 and .cold.3 (0x1c030da80, 0x1c030da94,
// the crash messages), running bundleInfoInitBlock (____bundleInfo_block_invoke) once, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json and, for the block,
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json. iOS Device 24A446: unread.
// A NULL bundle identifier crashes ("invalid bundleID provided, must be non-null"). Unless systemVersion is
// -1 (CMN W1, #1), it is sign-extended and an executable linked on or after it (the active platform) answers
// false. A NULL stored
// identifier crashes ("version check: bundle ID corrupted"). A different identifier answers CFEqual's false;
// otherwise the answer is 1 unless version <= stored version (FCMP; CSINC on LS, so an unordered comparison
// answers 1), else whether version is below 0.0 (CSET LT, also true when unordered). The decompiler's
// "version > stored || version < 0.0" loses both unordered cases; the assembly is followed.
bool _OCFAppVersionCheckLessThan(OCFStringRef bundleID, uint32_t systemVersion, double version) {
    if (bundleID == NULL) {
        crashWithMessage("invalid bundleID provided, must be non-null");
    }
    if (systemVersion != 0xFFFFFFFF) {
        uint64_t linkedVersion = (uint64_t)(int64_t)(int32_t)systemVersion;
        if (_OCFPlatformExecutableLinkedOnOrAfter(linkedVersion, dyld_get_active_platform())) {
            return false;
        }
    }
    dispatch_once(&_bundleInfo_onceToken, bundleInfoInitBlock);
    if (_bundleInfo_identifier == NULL) {
        crashWithMessage("version check: bundle ID corrupted");
    }
    if (!OCFEqual(_bundleInfo_identifier, bundleID)) {
        return false;
    }
    if (!(version <= _bundleInfo_version)) {
        return true;
    }
    return !(version >= 0.0);
}

// Native __CFLogCStringLegacy.levstr, iOS Simulator 24A434, 0x2227a6d20 (__DATA_CONST,__const), UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; eight pointers to "0" through "7".
// Non-external. iOS Device 24A446: unread.
static const char *const levstr[8] = {"0", "1", "2", "3", "4", "5", "6", "7"};

// Native __CFLogCStringLegacy, iOS Simulator 24A434, 0x1c025790c, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external.
// iOS Device 24A446: unread.
// Sends an ASL message (facility: the main bundle identifier when nonempty, else com.apple.console; client
// option 2) with the local time, thread, effective user ID, level and message keys, then repeats the message
// on stderr when also_do_stderr(true) allows it.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
void __CFLogCStringLegacy(int32_t level, const char *message, size_t length, bool withBanner) {
    char *banner = NULL;
    char *localTime = NULL;
    char *thread = NULL;
    char *uid = NULL;
    if (withBanner) {
        _populateBanner(&banner, &localTime, &thread);
    }
    uint32_t euid = 0;
    __CFGetUGIDs(&euid, NULL);
    asprintf(&uid, "%d", euid);
    const char *facility = "com.apple.console";
    if (__CFBundleMainID != NULL && *__CFBundleMainID != 0) {
        facility = __CFBundleMainID;
    }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    asl_object_t client = asl_open(NULL, facility, 2);
    asl_object_t entry = asl_new(0);
    asl_set(entry, "CFLog Local Time", localTime);
    asl_set(entry, "CFLog Thread", thread);
    asl_set(entry, "ReadUID", uid);
    asl_set(entry, "Level", levstr[level]);
    asl_set(entry, "Message", message);
    asl_send(client, entry);
    asl_free(entry);
    asl_close(client);
#pragma clang diagnostic pop
    if (also_do_stderr(true)) {
        _logToStderr(banner, message, length);
    }
    if (thread != NULL) {
        free(thread);
    }
    if (localTime != NULL) {
        free(localTime);
    }
    if (banner != NULL) {
        free(banner);
    }
    if (uid != NULL) {
        free(uid);
    }
}

// Native _CFLogSimple, iOS Simulator 24A434, 0x1c0257884, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Hidden (was private external).
// iOS Device 24A446: unread.
// Formats into a 1,024-byte buffer and logs a positive result without a banner.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
void _CFLogSimple(int32_t level, const char *format, ...) {
    char buffer[1024];
    va_list arguments;
    va_start(arguments, format);
    int length = vsnprintf(buffer, sizeof(buffer), format, arguments);
    va_end(arguments);
    if (length >= 1) {
        __CFLogCStringLegacy(level, buffer, (size_t)length, false);
    }
}

// Native ___CFLogCString, iOS Simulator 24A434, 0x1c02599d0, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; the default log function of __CFLogvEx2Predicate when os_log is in use.
// iOS Device 24A446: unread.
// Writes the message to stderr after a fresh banner and frees the three banner strings (thread, local time,
// banner, in that order).
static void __CFLogCString(const char *message, size_t length) {
    char *banner = NULL;
    char *localTime = NULL;
    char *thread = NULL;
    _populateBanner(&banner, &localTime, &thread);
    _logToStderr(banner, message, length);
    if (thread != NULL) {
        free(thread);
    }
    if (localTime != NULL) {
        free(localTime);
    }
    if (banner != NULL) {
        free(banner);
    }
}

// Native __CFLogvEx2Predicate, iOS Simulator 24A434, 0x1c025738c, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external.
// iOS Device 24A446: unread.
// Thread-specific slot 2 counts nested logging; at depth four or more nothing happens. With a format, the
// message is formatted (system default allocator, _CFStringCreateWithFormatAndArgumentsAux2 with no implicit
// argument), converted to UTF-8 (0x8000100) into a malloc'd buffer of the maximum size plus one (type
// 0x100004077774924), and delivered when the level is at most 7, the conversion succeeded and its strlen is
// 1..0x1000000: to logit when given, else to __CFLogCStringLegacy (useLegacy) or __CFLogCString, with the
// banner. strlen runs on the buffer whether or not the conversion succeeded, as natively.
static void __CFLogvEx2Predicate(
    OCFLogFunc logit,
    OCFStringRef (*copyDescFunc)(void *, const void *),
    OCFStringRef (*contextDescFunc)(void *, const void *, const void *, bool, bool *),
    OCFDictionaryRef formatOptions,
    int32_t level,
    OCFStringRef format,
    va_list arguments,
    bool useLegacy
) {
    uintptr_t depth = (uintptr_t)_OCFGetTSD(2);
    if (depth > 3) {
        return;
    }
    _OCFSetTSD(2, (void *)(depth + 1), NULL);
    if (format != NULL) {
        OCFStringRef string = _OCFStringCreateWithFormatAndArgumentsAux2(
            kOCFAllocatorSystemDefault, copyDescFunc, contextDescFunc, formatOptions, NULL, format, arguments
        );
        if (string != NULL) {
            OCFIndex size = OCFStringGetMaximumSizeForEncoding(OCFStringGetLength(string), 0x8000100);
            char *buffer = malloc_type_malloc((size_t)size + 1, 0x100004077774924);
            if (buffer != NULL) {
                Boolean converted = OCFStringGetCString(string, buffer, size + 1, 0x8000100);
                size_t length = strlen(buffer);
                if ((uint32_t)level <= 7 && converted && length - 0x1000001 >= UINT64_C(0xffffffffff000000)) {
                    if (logit != NULL) {
                        logit(level, buffer, length, 1);
                    } else if (useLegacy) {
                        __CFLogCStringLegacy(level, buffer, length, true);
                    } else {
                        __CFLogCString(buffer, length);
                    }
                }
                free(buffer);
            }
            OCFRelease(string);
        }
    }
    _OCFSetTSD(2, (void *)depth, NULL);
}

// libsystem_trace's NSLog shim (re-exported by libSystem); supplied, never reimplemented. The SDK installs no
// declaration of these two private entry points; they receive the caller's return address, and the shim also
// receives the CF format string and its arguments (a boundary that hands a CF object to the system library).
extern bool os_log_shim_enabled(void *returnAddress);

extern void os_log_shim_with_CFString_4NSLog(
    void *returnAddress, os_log_t log, uint8_t type, OCFStringRef format, va_list arguments, void *context
);

// Native __CFLog_os_trace_type_map, iOS Simulator 24A434, 0x1c0456b20 (__TEXT,__const, eight bytes c1 c1 c1
// 41 01 01 01 02), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external. iOS Device
// 24A446: unread.
static const uint8_t _CFLog_os_trace_type_map[8] = {0xc1, 0xc1, 0xc1, 0x41, 0x01, 0x01, 0x01, 0x02};

// Native __CFLogvEx3, iOS Simulator 24A434, 0x1c025752c, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. External. iOS Device 24A446: unread. Without the os_log shim for the caller, logging
// goes through __CFLogvEx2Predicate in legacy (ASL) mode. With it, and below logging depth four, levels 0..7
// go to os_log_shim_with_CFString_4NSLog with the level's shim type, and the message is repeated on stderr
// through __CFLogvEx2Predicate when also_do_stderr(false) allows it. The shim's log handle is the GOT slot
// 0x22241b4d8 outside the image, used as an os_log_t by every native site; it is taken to be OS_LOG_DEFAULT;
// confidence: medium.
void _OCFLogvEx3(
    OCFLogFunc logit,
    OCFStringRef (*copyDescFunc)(void *, const void *),
    OCFStringRef (*contextDescFunc)(void *, const void *, const void *, bool, bool *),
    OCFDictionaryRef formatOptions,
    int32_t level,
    OCFStringRef format,
    va_list arguments,
    void *returnAddress
) {
    if (!os_log_shim_enabled(returnAddress)) {
        __CFLogvEx2Predicate(
            logit, copyDescFunc, contextDescFunc, formatOptions, level, format, arguments, true
        );
        return;
    }
    uintptr_t depth = (uintptr_t)_OCFGetTSD(2);
    if (depth > 3) {
        return;
    }
    _OCFSetTSD(2, (void *)(depth + 1), NULL);
    if ((uint32_t)level <= 7) {
        va_list copied;
        va_copy(copied, arguments);
        os_log_shim_with_CFString_4NSLog(
            returnAddress, OS_LOG_DEFAULT, _CFLog_os_trace_type_map[level], format, copied, NULL
        );
        va_end(copied);
    }
    _OCFSetTSD(2, (void *)depth, NULL);
    if (also_do_stderr(false)) {
        __CFLogvEx2Predicate(
            logit, copyDescFunc, contextDescFunc, formatOptions, level, format, arguments, false
        );
    }
}

// Native __CFLogSimpleCString, iOS Simulator 24A434, 0x1c0257ac4, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Hidden (was a private external).
// iOS Device 24A446: unread.
void _CFLogSimpleCString(int32_t level, const char *message, size_t length) {
    __CFLogCStringLegacy(level, message, length, false);
}

// Native CFLog, iOS Simulator 24A434, 0x1c0257acc, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified. External.
// iOS Device 24A446: unread.
// _CFLogvEx3 with no log function, description callbacks or options, and this function's return address.
void OCFLog(int32_t level, OCFStringRef format, ...) {
    va_list arguments;
    va_start(arguments, format);
    _OCFLogvEx3(NULL, NULL, NULL, NULL, level, format, arguments, __builtin_return_address(0));
    va_end(arguments);
}

// Native _CFSuddenTerminationDisable and _CFSuddenTerminationEnable, iOS Simulator 24A434, 0x1c0257fcc and
// 0x1c0257fd0, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External; each is a single
// RET. iOS Device 24A446: unread.
void _OCFSuddenTerminationDisable(void) {}

void _OCFSuddenTerminationEnable(void) {}

// The fortified stpcpy the native code calls (the SDK declares it only under _FORTIFY_SOURCE).
extern char *__stpcpy_chk(char *destination, const char *source, size_t destinationLength);

// Native __CFGenerateReport, 0x1c02f8f14, with ____CFGenerateReport_block_invoke (0x1c02f934c). Hidden (was a
// private external). A malloc'd report of the date and time, the OS version, the program name and a backtrace
// of up to 128 frames, or NULL. Each line is formatted into its own slot of a (frames + 5)-pointer array; the
// block advances the slot cursor, fails on a missing line, and adds the line's length to the total (starting
// at 1 for the terminator), failing on overflow. The time is written by ctime_r over the 25 reserved spaces
// after "Date/Time:     ", and the OS version by sysctlbyname over as many spaces as it needs, its terminator
// replaced by a newline. The lines are then copied into one allocation. Every line, the slot array and the
// symbol strings are freed. iOS Simulator 24A434, confidence: verified; iOS Device divergence: unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3).
char *__CFGenerateReport(void) {
    static const char header_datetime[] = "Date/Time:     ";
    static const char header_osversion[] = "OS Version:    ";
    static const char header_initial[] = "Application:   ";
    __block char **cursor = NULL;
    __block size_t total = 1;
    Boolean (^appended)(long) = ^Boolean(long length) {
      char *line = *cursor++;
      if (line == NULL) {
          return true;
      }
      return __builtin_add_overflow(total, (size_t)length, &total);
    };
    void *frames[128];
    memset(frames, 0, sizeof(frames));
    int count = backtrace(frames, 128);
    char **symbols = backtrace_symbols(frames, count);
    char **lines = NULL;
    char *report = NULL;
    if ((unsigned int)count > 0xfffffffa || (((uint64_t)(int64_t)count + 5) >> 61) != 0) {
        goto cleanup;
    }
    lines = malloc_type_malloc(((int64_t)count + 5) * 8, 0x10040436913F5);
    if (lines == NULL) {
        goto cleanup;
    }
    cursor = lines;
    time_t now = time(NULL);
    if (now == -1) {
        goto cleanup;
    }
    if (appended(asprintf(cursor, "%s%25s", header_datetime, ""))) {
        goto cleanup;
    }
    ctime_r(&now, cursor[-1] + 15);
    size_t size = 0;
    if (sysctlbyname("kern.osversion", NULL, &size, NULL, 0) != 0) {
        goto cleanup;
    }
    if (appended(asprintf(cursor, "%s%*s", header_osversion, (int)size, ""))) {
        goto cleanup;
    }
    if (sysctlbyname("kern.osversion", cursor[-1] + 15, &size, NULL, 0) != 0) {
        goto cleanup;
    }
    cursor[-1][size + 14] = '\n';
    if (appended(asprintf(cursor, "%s%s\n", header_initial, getprogname()))) {
        goto cleanup;
    }
    if (appended(asprintf(cursor, "\n"))) {
        goto cleanup;
    }
    if (appended(asprintf(cursor, "Backtrace:\n"))) {
        goto cleanup;
    }
    for (int frame = 0; frame < count; frame++) {
        if (appended(asprintf(cursor, "%s\n", symbols[frame]))) {
            goto cleanup;
        }
    }
    report = malloc_type_malloc(total, 0xD79353B1);
    if (report != NULL) {
        char *end = report;
        for (char **line = lines; line < cursor; line++) {
            end = __stpcpy_chk(end, *line, (size_t)-1);
        }
    }
cleanup:
    if (symbols != NULL) {
        free(symbols);
    }
    if (lines != NULL) {
        while (lines < cursor) {
            free(*--cursor);
        }
        free(lines);
    }
    return report;
}
