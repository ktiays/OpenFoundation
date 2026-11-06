//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/Base/OCFFileUtilities.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/PlugIn/OCFBundleBinary.h"
#include "CoreFoundation/PlugIn/OCFBundleExecutable.h"
#include "CoreFoundation/PlugIn/OCFBundleGrok.h"
#include "CoreFoundation/PlugIn/OCFBundleMain.h"
#include "CoreFoundation/PlugIn/OCFBundleResources.h"
#include "CoreFoundation/PlugIn/OCFBundleStrings.h"
#include "CoreFoundation/PlugIn/OCFPlugIn.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/URL/OCFURLBundleResources.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <assert.h>
#include <dispatch/dispatch.h>
#include <fcntl.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

// Inferred organization: the bundle loggers (0x1c016a128-0x1c016a238), _CFBundleNormalizedPath (0x1c016a39c)
// through _CFBundleLayoutVersion (0x1c016e93c) and _CFBundleEnsureBundleExistsForImagePath (0x1c016f0d8)
// through ___CFBundleFunctionCallLog_block_invoke (0x1c016f7f4) are the CFBundle.c functions of the iOS
// Simulator __text section in scope; the public functions log the source path PlugIn.subproj/CFBundle.c,
// which names this unit. _CFURLCreateResolvedDirectoryWithString (0x1c016a23c) lives in
// URL/OCFURLBundleResources.c. The .cold fragments (0x1c030645c-0x1c03067e4) and the OUTLINED_FUNCTION
// fragments they reach are folded into their parents. Unless a definition says otherwise: iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/CoreFoundationBundle.md (pending; the coordinator writes the
// reading).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_7 (0x1c016f898).
// Folds the compiler-outlined pieces __CFBundleEnsureBundleExistsForImagePath.cold.1 (0x1c0306734),
// __CFBundleEnsureBundleExistsForImagePath.cold.2 (0x1c0306738),
// __CFBundleEnsureBundleExistsForImagePath.cold.3 (0x1c0306764),
// __CFBundleEnsureBundleExistsForImagePath.cold.4 (0x1c0306768).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_5 (0x1c016f880).

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call here passes 0x10000
// (OS_UNFAIR_LOCK_DATA_SYNCHRONIZATION).
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

// libobjc's objc_opt_class and objc_retain, as the native code imports them.
extern Class objc_opt_class(id object);
extern id objc_retain(id object);

// The source path the native _CFBundleLogSourceLocation calls of this unit pass (__cstring).
static const char kSourcePath[] =
    "/Library/Caches/com.apple.xbs/C9620487-17E9-4D33-8BF5-CCA0DA6AFB18/TemporaryDirectory.R3jFjg/Sources/"
    "CoreFoundation_Sim/CoreFoundation/PlugIn.subproj/CFBundle.c";

// The native creation modes of _CFBundleCreate (W2): 0 ordinary, 1 unique, 2 main bundle, 3 a framework
// found for a loaded image. Local descriptive names.
enum {
    kCreationModeOrdinary = 0,
    kCreationModeUnique = 1,
    kCreationModeMain = 2,
    kCreationModeLoadedImage = 3,
};

static OCFBundleRef _CFBundleGetBundleWithIdentifier(OCFStringRef bundleID, void *pointer);
static void __CFBundleDeallocate(OCFTypeRef cf);
static Boolean __CFBundleEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFBundleHash(OCFTypeRef cf);
static OCFStringRef __CFBundleCopyDescription(OCFTypeRef cf);
static OCFBundleRef _CFBundleCreate(
    OCFAllocatorRef allocator,
    OCFURLRef bundleURL,
    int32_t creationMode,
    OCFOptionFlags options,
    OCFSetRef limitedKeyPaths
);
static OCFURLRef __CFBundleCopyFrameworkURLForExecutablePath(OCFStringRef executablePath, Boolean permissive);
static void _CFBundleEnsureBundlesUpToDateWithHint(OCFStringRef hint);
static void _CFBundleEnsureAllBundlesUpToDate(void);
static void _CFBundleEnsureBundleExistsForImagePath(OCFStringRef imagePath, Boolean permissive);
static OCFBundleRef _CFBundleCopyFromTablesForURLLocked(OCFURLRef url);
static OCFBundleRef _CFBundleGetFromTablesLocked(OCFStringRef bundleID);
static Boolean _CFBundlesHaveEquivalentURL(OCFBundleRef bundle1, OCFBundleRef bundle2);
static void _CFBundleAddToTablesLocked(OCFBundleRef bundle, OCFStringRef bundleID);
static void _CFBundleEnsureBundlesExistForImagePaths(OCFArrayRef imagePaths);

// Native CFBundle Info.plist key constants, __DATA_CONST,__const 0x22279ccf8, 0x22279cd08, 0x22279cd10,
// 0x22279cd18, 0x22279cd20 and 0x22279cd70, each pointing at its __cfstring record (0x2227ac9c0, 0x2227aca00,
// 0x2227aca20, 0x2227aca40, 0x2227aca60, 0x2227acba0). kCFBundleIdentifierKey, kCFBundleDevelopmentRegionKey,
// kCFBundleLocalizationsKey, _kCFBundleAllowMixedLocalizationsKey and _kCFBundleShortVersionStringKey are
// external; _kCFBundleUseAppleLocalizationsKey was a private external. Only the keys the restored functions
// load are defined; the rest of the table (0x22279cce8-0x22279ce60) is not referenced here.
static const OCFConstantString kIdentifierKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "CFBundleIdentifier", 18
};
static const OCFConstantString kDevelopmentRegionKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "CFBundleDevelopmentRegion", 25
};
static const OCFConstantString kLocalizationsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "CFBundleLocalizations", 21
};
static const OCFConstantString kUseAppleLocalizationsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "_CFBundleUseAppleLocalizations", 30
};
static const OCFConstantString kAllowMixedLocalizationsKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "CFBundleAllowMixedLocalizations", 31
};
static const OCFConstantString kShortVersionStringKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "CFBundleShortVersionString", 26
};

// Native kCFBundleVersionKey (0x22279cd00, record 0x2227ac9e0) and _kCFBundleNumericVersionKey (0x22279ce40,
// record 0x2227acec0), __DATA_CONST,__const; both external. Defined by the second CFBundle sub-track for
// _CFBundleRefreshInfoDictionaryAlreadyLocked (PlugIn/OCFBundleInfoPlist.c).
static const OCFConstantString kVersionKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "CFBundleVersion", 15
};
static const OCFConstantString kNumericVersionKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "CFBundleNumericVersion", 22
};

const OCFStringRef kOCFBundleIdentifierKey = (OCFStringRef)&kIdentifierKeyString;
const OCFStringRef kOCFBundleDevelopmentRegionKey = (OCFStringRef)&kDevelopmentRegionKeyString;
const OCFStringRef kOCFBundleLocalizationsKey = (OCFStringRef)&kLocalizationsKeyString;
const OCFStringRef _kCFBundleUseAppleLocalizationsKey = (OCFStringRef)&kUseAppleLocalizationsKeyString;
const OCFStringRef _kOCFBundleAllowMixedLocalizationsKey = (OCFStringRef)&kAllowMixedLocalizationsKeyString;
const OCFStringRef _kOCFBundleShortVersionStringKey = (OCFStringRef)&kShortVersionStringKeyString;
const OCFStringRef kOCFBundleVersionKey = (OCFStringRef)&kVersionKeyString;
const OCFStringRef _kOCFBundleNumericVersionKey = (OCFStringRef)&kNumericVersionKeyString;

// Native __CFBundleClass, 0x22279cee8 (__DATA_CONST,__const). Non-external (was a private external).
// Version 1, "CFBundle", finalize __CFBundleDeallocate, equal __CFBundleEqual, hash __CFBundleHash, debug
// description __CFBundleCopyDescription; every other slot is zero.
const OCFRuntimeClass __CFBundleClass = {
    1,
    "CFBundle",
    NULL,
    NULL,
    __CFBundleDeallocate,
    __CFBundleEqual,
    __CFBundleHash,
    NULL,
    __CFBundleCopyDescription,
    NULL,
    NULL,
    0,
};

// Native CFBundleGlobalDataLock (0x22e6d2740), _allBundles (0x22e6d2750), _bundlesByURL (0x22e6d2758) and
// _bundlesByIdentifier (0x22e6d2760), __DATA,__bss. Non-external.
os_unfair_lock CFBundleGlobalDataLock = OS_UNFAIR_LOCK_INIT;
static OCFMutableArrayRef _allBundles = NULL;
static OCFMutableDictionaryRef _bundlesByURL = NULL;
static OCFMutableDictionaryRef _bundlesByIdentifier = NULL;

// Native _CFBundleResourceLogger._log and .onceToken and the two other loggers' pairs (__DATA,__bss).
// Non-external.
static os_log_t _CFBundleResourceLogger_log;
static dispatch_once_t _CFBundleResourceLogger_onceToken;
static os_log_t _CFBundleLocalizedStringLogger_log;
static dispatch_once_t _CFBundleLocalizedStringLogger_onceToken;
static os_log_t _CFBundleLoadingLogger_log;
static dispatch_once_t _CFBundleLoadingLogger_onceToken;

// Native _CFBundleFunctionCallLog.logger and .onceToken (__DATA,__bss). Non-external. The function
// _CFBundleFunctionCallLog itself exists only inlined into _CFBundleLogSourceLocation.
static os_log_t _CFBundleFunctionCallLog_logger;
static dispatch_once_t _CFBundleFunctionCallLog_onceToken;

// Native _CFBundleResourceLogger, 0x1c016a128. Hidden (was private external).
// When the once token is not -1, .cold.1 (0x1c030645c, through _OUTLINED_FUNCTION_7) runs dispatch_once with
// the global block whose invoke is ___CFBundleResourceLogger_block_invoke (0x1c016a158, below); the log
// object is then returned.
os_log_t _CFBundleResourceLogger(void) {
    // Native ___CFBundleResourceLogger_block_invoke, 0x1c016a158. Non-external.
    // os_log_create("com.apple.CFBundle", "resources") into the static log.
    dispatch_once(&_CFBundleResourceLogger_onceToken, ^{
      _CFBundleResourceLogger_log = os_log_create("com.apple.CFBundle", "resources");
    });
    return _CFBundleResourceLogger_log;
}

// Native _CFBundleLocalizedStringLogger, 0x1c016a184. Hidden (was private external).
// .cold.1 (0x1c0306460) runs dispatch_once with ___block_literal_global.96.
os_log_t _CFBundleLocalizedStringLogger(void) {
    // Native ___CFBundleLocalizedStringLogger_block_invoke, 0x1c016a1b4. Non-external.
    // os_log_create("com.apple.CFBundle", "strings").
    dispatch_once(&_CFBundleLocalizedStringLogger_onceToken, ^{
      _CFBundleLocalizedStringLogger_log = os_log_create("com.apple.CFBundle", "strings");
    });
    return _CFBundleLocalizedStringLogger_log;
}

// Native _CFBundleLoadingLogger, 0x1c016a1e0. Hidden (was private external).
// .cold.1 (0x1c0306474) runs dispatch_once through _OUTLINED_FUNCTION_3.
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_0_1 (0x1c016f820).
os_log_t _CFBundleLoadingLogger(void) {
    // Native ___CFBundleLoadingLogger_block_invoke, 0x1c016a210. Non-external.
    // os_log_create("com.apple.CFBundle", "loading").
    dispatch_once(&_CFBundleLoadingLogger_onceToken, ^{
      _CFBundleLoadingLogger_log = os_log_create("com.apple.CFBundle", "loading");
    });
    return _CFBundleLoadingLogger_log;
}

// Native _CFBundleNormalizedPath, iOS Simulator 24A434, 0x1c016a39c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external (was a private
// external). The two literals are the __cstring bytes at 0x1c047d8ee (31 bytes with NUL) and
// 0x1c047d90d (22 bytes with NUL), which precede the "PlugIn.subproj/CFBundle.c" source path.
// iOS Device 24A446: 0x18063e8d4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// the same literals and lengths, strncmp through a stub, authenticated frame; the results agree.
// NULL is returned unchanged. A path that begins with the 30-byte "/private/preboot/Cryptexes/OS/" is
// advanced by 29 bytes, one that begins with the 21-byte "/System/Cryptexes/OS/" by 20, keeping the
// slash before the remainder; any other path is returned unchanged.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
const char *_CFBundleNormalizedPath(const char *path) {
    if (path == NULL) {
        return path;
    }
    if (strncmp(path, "/private/preboot/Cryptexes/OS/", 30) == 0) {
        return path + 29;
    }
    return path + (strncmp(path, "/System/Cryptexes/OS/", 21) == 0 ? 20 : 0);
}

// Native _CFBundleCreateNormalizedURL, 0x1c016a3fc. Hidden (was private external).
// The resolved file system representation (1,026 bytes) of url, passed through _CFBundleNormalizedPath unless
// keepPath (W2) is nonzero, becomes a directory URL with the given allocator; no representation answers NULL.
OCFURLRef _CFBundleCreateNormalizedURL(OCFAllocatorRef allocator, OCFURLRef url, Boolean keepPath) {
    UInt8 buffer[1026];
    if (!OCFURLGetFileSystemRepresentation(url, true, buffer, 1026)) {
        return NULL;
    }
    const char *path = keepPath ? (const char *)buffer : _CFBundleNormalizedPath((const char *)buffer);
    return OCFURLCreateFromFileSystemRepresentation(
        allocator, (const UInt8 *)path, (OCFIndex)strlen(path), true
    );
}

// Native _CFBundleResourceURLIsSafeForMapping, 0x1c016a4ac. Hidden (was private external).
// True when the resolved file system representation (1,026 bytes) exists, statfs (on a zeroed 512-byte
// struct statfs) succeeds and f_flags & 0x1200 (MNT_LOCAL | MNT_DONTBROWSE) is exactly 0x1000 (MNT_LOCAL).
Boolean _CFBundleResourceURLIsSafeForMapping(OCFURLRef url) {
    UInt8 buffer[1026];
    if (!OCFURLGetFileSystemRepresentation(url, true, buffer, 1026)) {
        return false;
    }
    struct statfs fileSystem;
    memset(&fileSystem, 0, sizeof(fileSystem));
    int result = statfs((const char *)buffer, &fileSystem);
    return result == 0 && (fileSystem.f_flags & 0x1200) == 0x1000;
}

// Native _CFBundleCreateMappedOrLoadedPlistData, 0x1c016a578. Hidden (was private external).
// _CFBundleResourceURLIsSafeForMapping runs first in every case. When allowMapping (W1) is nonzero and the
// URL is safe, _CFBundleCreateMappedBPlistFile(url, flag) is tried and *mapped records whether it answered;
// otherwise *mapped is cleared. Without a mapped result the data is read with _CFDataCreateFromURL(url,
// error) (a tail call; X4 is passed through as its second argument).
OCFDataRef _CFBundleCreateMappedOrLoadedPlistData(
    OCFURLRef url, Boolean allowMapping, Boolean flag, Boolean *mapped, OCFErrorRef *error
) {
    Boolean safe = _CFBundleResourceURLIsSafeForMapping(url);
    if (allowMapping && safe) {
        OCFDataRef data = _CFBundleCreateMappedBPlistFile(url, flag);
        *mapped = data != NULL;
        if (data != NULL) {
            return data;
        }
    } else {
        *mapped = false;
    }
    return _CFDataCreateFromURL(url, error);
}

// Native _CFBundleEffectiveLayoutVersion, 0x1c016a5fc. Hidden (was private external).
// A nonzero stored version (+53) is the answer. Otherwise a nonempty info dictionary answers 0; without one,
// no executable URL answers 4; with one, the binary type (grokked and stored at +48 when still 0) answers 4
// for types 6 and 1 and 0 for any other.
uint8_t _CFBundleEffectiveLayoutVersion(OCFBundleRef bundle) {
    uint8_t version = bundle->_version;
    if (version != 0) {
        return version;
    }
    OCFDictionaryRef infoDict = OCFBundleGetInfoDictionary(bundle);
    if (infoDict != NULL && OCFDictionaryGetCount(infoDict) != 0) {
        return 0;
    }
    OCFURLRef executableURL = OCFBundleCopyExecutableURL(bundle);
    if (executableURL == NULL) {
        return 4;
    }
    uint32_t binaryType = bundle->_binaryType;
    if (binaryType == 0) {
        binaryType = _OCFBundleGrokBinaryType(executableURL);
        bundle->_binaryType = binaryType;
    }
    version = (binaryType == 6 || binaryType == 1) ? 4 : 0;
    OCFRelease(executableURL);
    return version;
}

// Native _CFBundleLogSourceLocation, 0x1c016a7cc. Hidden (was private external).
// The CoreFoundation/CFSwiftBundleLogging feature is queried first in every case. When it is enabled and
// `enabled` (W0) is nonzero, the part of `file` after its last '/' (the whole string without one), the line
// and the function name are logged at the debug level of the function-call logger (format header
// qword_1C0373160: 3 arguments, public string, int, public string). The logger is
// _CFBundleFunctionCallLog, inlined: its dispatch_once is .cold.1 (0x1c03064e8, ___block_literal_global.296).
void _CFBundleLogSourceLocation(Boolean enabled, const char *file, int line, const char *function) {
    bool featureEnabled = _os_feature_enabled_impl("CoreFoundation", "CFSwiftBundleLogging");
    if (!enabled || !featureEnabled) {
        return;
    }
    const char *slash = strrchr(file, '/');
    const char *fileName = slash != NULL ? slash + 1 : file;
    // Native ___CFBundleFunctionCallLog_block_invoke, 0x1c016f7f4. Non-external.
    // os_log_create("com.apple.corefoundation", "cfbundle-functions").
    dispatch_once(&_CFBundleFunctionCallLog_onceToken, ^{
      _CFBundleFunctionCallLog_logger = os_log_create("com.apple.corefoundation", "cfbundle-functions");
    });
    os_log_t logger = _CFBundleFunctionCallLog_logger;
    if (os_log_type_enabled(logger, OS_LOG_TYPE_DEBUG)) {
        os_log_debug(
            logger,
            "CFBundle Function Call: Source location: %{public}s:%d %{public}s",
            fileName,
            line,
            function
        );
    }
}

// Native _CFBundleCreateUnique, 0x1c016a8b4. External.
// Per-thread slot 18 marks a nested CFBundle call: the outermost call sets it to 1, logs its source location
// (line 1218) and clears it on the way out. Under the CoreFoundation/CFSwiftBundle feature a non-NULL URL is
// passed to +[_NSSwiftBundle _bundleWithURL:mode:options:filteredInfoKeyPaths:] (mode 1, options 0, no key
// paths) and the answer retained; otherwise _CFBundleCreate runs in the unique mode.
OCFBundleRef _OCFBundleCreateUnique(OCFAllocatorRef allocator, OCFURLRef bundleURL) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 1218, OF_NAME_STRING(_OCFBundleCreateUnique));
    OCFBundleRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = NULL;
        if (bundleURL != NULL) {
            Class swiftBundleClass = objc_opt_class((id)objc_lookUpClass(OF_NAME_STRING(_ONSSwiftBundle)));
            result = ((OCFBundleRef (*)(id, SEL, OCFURLRef, int32_t, uint64_t, OCFArrayRef))objc_msgSend)(
                (id)swiftBundleClass,
                sel_getUid("_bundleWithURL:mode:options:filteredInfoKeyPaths:"),
                bundleURL,
                1,
                0,
                NULL
            );
            if (result != NULL) {
                OCFRetain(result);
            }
        }
    } else {
        result = _CFBundleCreate(allocator, bundleURL, kCreationModeUnique, 0, NULL);
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native CFBundleCreate, 0x1c016ab24. External.
// A NULL URL answers NULL before the slot-18 prelude (line 1198). The Swift path sends
// _bundleWithURL:mode:options:filteredInfoKeyPaths: with mode 0 and retains the answer. Otherwise the main
// bundle is answered (retained) when its URL is non-NULL and CFEqual to bundleURL; any other URL goes to
// _CFBundleCreate in the ordinary mode.
OCFBundleRef OCFBundleCreate(OCFAllocatorRef allocator, OCFURLRef bundleURL) {
    if (bundleURL == NULL) {
        return NULL;
    }
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 1198, OF_NAME_STRING(OCFBundleCreate));
    OCFBundleRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        Class swiftBundleClass = objc_opt_class((id)objc_lookUpClass(OF_NAME_STRING(_ONSSwiftBundle)));
        result = ((OCFBundleRef (*)(id, SEL, OCFURLRef, int32_t, uint64_t, OCFArrayRef))objc_msgSend)(
            (id)swiftBundleClass,
            sel_getUid("_bundleWithURL:mode:options:filteredInfoKeyPaths:"),
            bundleURL,
            0,
            0,
            NULL
        );
        if (result != NULL) {
            OCFRetain(result);
        }
    } else {
        OCFBundleRef mainBundle = OCFBundleGetMainBundle();
        if (mainBundle != NULL && mainBundle->_url != NULL && OCFEqual(mainBundle->_url, bundleURL)) {
            OCFRetain(mainBundle);
            result = mainBundle;
        } else {
            result = _CFBundleCreate(allocator, bundleURL, kCreationModeOrdinary, 0, NULL);
        }
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native CFBundleGetBundleWithIdentifier, 0x1c016ae84. External.
// The caller's return address (X30 on entry) is the hint pointer. After the slot-18 prelude (line 755) the
// Swift path asks _CFBundleGetBundleWithIdentifierWithHint and the other path
// _CFBundleGetBundleWithIdentifier, both with that pointer.
OCFBundleRef OCFBundleGetBundleWithIdentifier(OCFStringRef bundleID) {
    void *returnAddress = __builtin_return_address(0);
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 755, OF_NAME_STRING(OCFBundleGetBundleWithIdentifier));
    OCFBundleRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = _OCFBundleGetBundleWithIdentifierWithHint(bundleID, returnAddress);
    } else {
        result = _CFBundleGetBundleWithIdentifier(bundleID, returnAddress);
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native _CFBundleGetBundleWithIdentifierWithHint, 0x1c016af74. External.
// The slot-18 prelude (line 764); the Swift path sends _bundleWithIdentifier:hint: to the class of
// _NSSwiftBundle (the answer is not retained), the other path asks _CFBundleGetBundleWithIdentifier.
OCFBundleRef _OCFBundleGetBundleWithIdentifierWithHint(OCFStringRef bundleID, void *pointer) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        outermost, kSourcePath, 764, OF_NAME_STRING(_OCFBundleGetBundleWithIdentifierWithHint)
    );
    OCFBundleRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        Class swiftBundleClass = objc_opt_class((id)objc_lookUpClass(OF_NAME_STRING(_ONSSwiftBundle)));
        result = ((OCFBundleRef (*)(id, SEL, OCFStringRef, void *))objc_msgSend)(
            (id)swiftBundleClass, sel_getUid("_bundleWithIdentifier:hint:"), bundleID, pointer
        );
    } else {
        result = _CFBundleGetBundleWithIdentifier(bundleID, pointer);
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native _CFBundleGetBundleWithIdentifier, 0x1c016b074. Non-external.
// For a non-NULL identifier: the main bundle answers when its info dictionary's CFBundleIdentifier (the
// literal, not the key global) is a string CFEqual to it. Otherwise the tables are searched under the global
// lock; when nothing is found and a pointer is given, the image containing it gets a bundle
// (_CFBundleEnsureBundleExistsForImagePath, not permissive) and the tables are searched again; then the
// images matching the identifier hint are loaded and searched. A NULL identifier, or a miss after all that,
// loads every image (_CFBundleEnsureAllBundlesUpToDate) and searches once more.
static OCFBundleRef _CFBundleGetBundleWithIdentifier(OCFStringRef bundleID, void *pointer) {
    OCFBundleRef result;
    if (bundleID != NULL) {
        OCFBundleRef mainBundle = OCFBundleGetMainBundle();
        if (mainBundle != NULL) {
            OCFDictionaryRef infoDict = OCFBundleGetInfoDictionary(mainBundle);
            if (infoDict != NULL) {
                OCFTypeRef identifier = OCFDictionaryGetValue(infoDict, OF_STR("CFBundleIdentifier"));
                if (identifier != NULL && OCFGetTypeID(identifier) == OCFStringGetTypeID() &&
                    OCFEqual(identifier, bundleID)) {
                    return mainBundle;
                }
            }
        }
        os_unfair_lock_lock(&CFBundleGlobalDataLock);
        result = _CFBundleGetFromTablesLocked(bundleID);
        os_unfair_lock_unlock(&CFBundleGlobalDataLock);
        if (pointer != NULL && result == NULL) {
            OCFStringRef imagePath = _CFBundleCopyLoadedImagePathForPointer(pointer);
            if (imagePath != NULL) {
                _CFBundleEnsureBundleExistsForImagePath(imagePath, false);
                OCFRelease(imagePath);
            }
            os_unfair_lock_lock(&CFBundleGlobalDataLock);
            result = _CFBundleGetFromTablesLocked(bundleID);
            os_unfair_lock_unlock(&CFBundleGlobalDataLock);
        }
        if (result != NULL) {
            return result;
        }
        _CFBundleEnsureBundlesUpToDateWithHint(bundleID);
        os_unfair_lock_lock(&CFBundleGlobalDataLock);
        result = _CFBundleGetFromTablesLocked(bundleID);
        os_unfair_lock_unlock(&CFBundleGlobalDataLock);
        if (result != NULL) {
            return result;
        }
    }
    _CFBundleEnsureAllBundlesUpToDate();
    os_unfair_lock_lock(&CFBundleGlobalDataLock);
    result = _CFBundleGetFromTablesLocked(bundleID);
    os_unfair_lock_unlock(&CFBundleGlobalDataLock);
    return result;
}

// Native __CFBundleDeallocate, 0x1c016b1cc. Non-external (the class finalize callback).
// The slot-18 prelude (line 834). Without the Swift feature: the URL is detached (+16 cleared first), the
// info dictionary's CFBundleIdentifier is looked up and discarded, the executable is unloaded and the plug-in
// data deallocated; then the URL and the objects at +24, +32, +40, +96, +208, +136, +144, +152, +160, +184,
// +176, +112, +128 and +200 are released in that order when non-NULL.
static void __CFBundleDeallocate(OCFTypeRef cf) {
    OCFBundleRef bundle = (OCFBundleRef)cf;
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 834, "__CFBundleDeallocate");
    if (!_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        OCFURLRef url = bundle->_url;
        OCFDictionaryRef infoDict = bundle->_infoDict;
        bundle->_url = NULL;
        if (infoDict != NULL) {
            OCFDictionaryGetValue(infoDict, OF_STR("CFBundleIdentifier"));
        }
        OCFBundleUnloadExecutable(bundle);
        _CFBundleDeallocatePlugIn(bundle);
        if (url != NULL) {
            OCFRelease(url);
        }
        if (bundle->_infoDict != NULL) {
            OCFRelease(bundle->_infoDict);
        }
        if (bundle->_localInfoDict != NULL) {
            OCFRelease(bundle->_localInfoDict);
        }
        if (bundle->_searchLanguages != NULL) {
            OCFRelease(bundle->_searchLanguages);
        }
        if (bundle->_executablePath != NULL) {
            OCFRelease(bundle->_executablePath);
        }
        if (bundle->_infoPlistURL != NULL) {
            OCFRelease(bundle->_infoPlistURL);
        }
        if (bundle->_stringTables != NULL) {
            OCFRelease(bundle->_stringTables);
        }
        if (bundle->_completeTables != NULL) {
            OCFRelease(bundle->_completeTables);
        }
        if (bundle->_missingKeys != NULL) {
            OCFRelease(bundle->_missingKeys);
        }
        if (bundle->_stringsSources != NULL) {
            OCFRelease(bundle->_stringsSources);
        }
        if (bundle->_bundleBasePath != NULL) {
            OCFRelease(bundle->_bundleBasePath);
        }
        if (bundle->_queryTables != NULL) {
            OCFRelease(bundle->_queryTables);
        }
        if (bundle->_localizations != NULL) {
            OCFRelease(bundle->_localizations);
        }
        if (bundle->_field128 != NULL) {
            OCFRelease(bundle->_field128);
        }
        if (bundle->_additionalResourceBundles != NULL) {
            OCFRelease(bundle->_additionalResourceBundles);
        }
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
}

// Native __CFBundleEqual, 0x1c016b380. Non-external (the class equal callback).
// The slot-18 prelude (line 773); the Swift path sends isEqual:, otherwise identity decides.
static Boolean __CFBundleEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 773, "__CFBundleEqual");
    Boolean result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((BOOL (*)(id, SEL, id))objc_msgSend)((id)cf1, sel_getUid("isEqual:"), (id)cf2);
    } else {
        result = cf1 == cf2;
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native __CFBundleHash, 0x1c016b46c. Non-external (the class hash callback).
// The slot-18 prelude (line 783); the Swift path sends hash, otherwise the address is the hash.
static OCFHashCode __CFBundleHash(OCFTypeRef cf) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 783, "__CFBundleHash");
    OCFHashCode result = (OCFHashCode)cf;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((OCFHashCode (*)(id, SEL))objc_msgSend)((id)cf, sel_getUid("hash"));
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native __CFBundleCopyDescription, 0x1c016b544. Non-external (the class debug-description callback).
// The slot-18 prelude (line 793); the Swift path answers objc_retain of debugDescription. Otherwise the
// resolved file system representation (1,026 bytes) of the URL becomes a string (NULL without a URL or
// representation); the binary type minus 2, when at most 3 (unsigned), selects "executable, ", "bundle, ",
// "framework, " or "DLL, " from the table at 0x22279cf68 (otherwise the empty string 0x2227ac180); "not " is
// added when +52 is clear. A plug-in (+64 set) uses "CFBundle/CFPlugIn %p <%@> (%@%@loaded)", any other
// bundle "CFBundle %p <%@> (%@%@loaded)", both with the system default allocator.
static OCFStringRef __CFBundleCopyDescription(OCFTypeRef cf) {
    OCFBundleRef bundle = (OCFBundleRef)cf;
    UInt8 buffer[1026];
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 793, "__CFBundleCopyDescription");
    OCFStringRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = (OCFStringRef)objc_retain(
            ((id (*)(id, SEL))objc_msgSend)((id)cf, sel_getUid("debugDescription"))
        );
    } else {
        OCFStringRef path = NULL;
        if (bundle->_url != NULL && OCFURLGetFileSystemRepresentation(bundle->_url, true, buffer, 1026)) {
            path =
                OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, (const char *)buffer);
        }
        uint32_t typeIndex = bundle->_binaryType - 2;
        OCFStringRef loaded = OF_STR("");
        OCFStringRef binaryType = OF_STR("");
        if (typeIndex <= 3) {
            const OCFStringRef binaryTypes[4] = {
                OF_STR("executable, "),
                OF_STR("bundle, "),
                OF_STR("framework, "),
                OF_STR("DLL, "),
            };
            binaryType = binaryTypes[typeIndex];
        }
        if (!bundle->_isLoaded) {
            loaded = OF_STR("not ");
        }
        if (bundle->_plugInData._isPlugIn) {
            result = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("CFBundle/CFPlugIn %p <%@> (%@%@loaded)"),
                bundle,
                path,
                binaryType,
                loaded
            );
        } else {
            result = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("CFBundle %p <%@> (%@%@loaded)"),
                bundle,
                path,
                binaryType,
                loaded
            );
        }
        if (path != NULL) {
            OCFRelease(path);
        }
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native _CFBundleCreate, 0x1c016b748. Non-external.
// options bit 1 or 2 (X3 & 6) means bundleURL is an executable: its bundle URL is found first (none answers
// NULL). The URL is normalized (_CFBundleCreateNormalizedURL with options & 8); no normalized URL logs
// "Unable to create bundle at URL (%@): normalized URL null" (.cold.11) at the error level of the resource
// logger and answers NULL. Every mode but unique asserts that options bits 1 and 2 are clear (.cold.1, line
// 979; .cold.2, line 983; __assert_rtn "creationMode == _CFBundleCreationModeUnique"), and the ordinary and
// loaded-image modes first answer a bundle already in the URL table. Layout version 3 requires an existing
// directory (_CFGetFileProperties): an error logs .cold.4 (error level, "...unable to create file system
// representation of URL (%d)" with the error), a missing or non-directory path logs .cold.9 (debug level,
// "...does not exist or not a directory (%d)" with the mode); both answer NULL. The 200-byte instance (type
// 31; failure logs .cold.7, "...malloc failure") receives the normalized URL, the version, the unique flag,
// options bit 0, the cleared locks and the POSIX path of the absolute URL; the limited key paths (X4) are
// loaded when given, and the info dictionary and identifier are read before a full barrier (DMB ISH). Except
// in the unique mode the tables are consulted again under the global lock: an existing bundle for the URL,
// or one for the identifier with an equivalent file, is answered instead. The ordinary mode initializes the
// plug-in; a factory conflict logs "More than one bundle with the same factory UUID detected" (error level,
// the factories dictionary or NULL, the new bundle and the existing plug-in, which is then released) and
// answers NULL. The ordinary and loaded-image modes add the bundle to the tables. Executable URLs must then
// resolve to the bundle's own executable (strcmp of both 1,026-byte representations), or the bundle is
// released; in the unique mode with options bit 2 a layout version of 3 or 4 also requires a nonempty info
// dictionary.
// Folds the compiler-outlined pieces __CFBundleCreate.cold.1 (0x1c03064fc), __CFBundleCreate.cold.2
// (0x1c0306524), __CFBundleCreate.cold.3 (0x1c030654c), __CFBundleCreate.cold.4 (0x1c0306560),
// __CFBundleCreate.cold.5 (0x1c03065b4), __CFBundleCreate.cold.6 (0x1c03065c8), __CFBundleCreate.cold.7
// (0x1c03065dc), __CFBundleCreate.cold.8 (0x1c0306608), __CFBundleCreate.cold.9 (0x1c030661c),
// __CFBundleCreate.cold.10 (0x1c0306674), __CFBundleCreate.cold.11 (0x1c0306678).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_1_0 (0x1c016f834), _OUTLINED_FUNCTION_2
// (0x1c016f848), _OUTLINED_FUNCTION_6 (0x1c016f890).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_3 (0x1c016f858).
static OCFBundleRef _CFBundleCreate(
    OCFAllocatorRef allocator,
    OCFURLRef bundleURL,
    int32_t creationMode,
    OCFOptionFlags options,
    OCFSetRef limitedKeyPaths
) {
    UInt8 executableBuffer[1032];
    UInt8 buffer[1026];
    OCFOptionFlags executableOptions = options & 6;
    OCFURLRef normalizedURL;
    if (executableOptions == 0) {
        normalizedURL = _CFBundleCreateNormalizedURL(allocator, bundleURL, (options & 8) != 0);
    } else {
        OCFURLRef containingURL = _OCFBundleCopyBundleURLForExecutableURL(bundleURL);
        if (containingURL == NULL) {
            return NULL;
        }
        normalizedURL = _CFBundleCreateNormalizedURL(allocator, containingURL, (options & 8) != 0);
        OCFRelease(containingURL);
    }
    if (normalizedURL == NULL) {
        os_log_t log = _CFBundleResourceLogger();
        if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
            os_log_error(log, "Unable to create bundle at URL (%@): normalized URL null", bundleURL);
        }
        return NULL;
    }
    Boolean addToTables;
    Boolean skipPlugIn;
    Boolean consultTables;
    Boolean unique;
    Boolean checkLayout;
    if (creationMode == kCreationModeUnique) {
        consultTables = false;
        checkLayout = (options >> 2) & 1;
        unique = true;
        addToTables = false;
        skipPlugIn = false;
    } else {
        Boolean checkTablesFirst;
        if (creationMode > kCreationModeUnique) {
            checkTablesFirst = creationMode != kCreationModeMain;
            addToTables = creationMode != kCreationModeMain;
            skipPlugIn = true;
        } else {
            checkTablesFirst = true;
            addToTables = true;
            skipPlugIn = false;
        }
        if (options & 2) {
            __assert_rtn("_CFBundleCreate", "CFBundle.c", 979, "creationMode == _CFBundleCreationModeUnique");
        }
        if (options & 4) {
            __assert_rtn("_CFBundleCreate", "CFBundle.c", 983, "creationMode == _CFBundleCreationModeUnique");
        }
        if (checkTablesFirst) {
            os_unfair_lock_lock(&CFBundleGlobalDataLock);
            OCFBundleRef existing = _CFBundleCopyFromTablesForURLLocked(normalizedURL);
            os_unfair_lock_unlock(&CFBundleGlobalDataLock);
            if (existing != NULL) {
                OCFRelease(normalizedURL);
                return existing;
            }
        }
        unique = false;
        checkLayout = false;
        consultTables = true;
    }
    uint8_t version = _CFBundleGetBundleVersionForURL(normalizedURL);
    if (version == 3) {
        Boolean exists = false;
        SInt32 mode = 0;
        SInt32 error = _CFGetFileProperties(allocator, normalizedURL, &exists, &mode, NULL, NULL, NULL, NULL);
        if (error != 0) {
            OCFRelease(normalizedURL);
            os_log_t log = _CFBundleLoadingLogger();
            if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
                os_log_error(
                    log,
                    "Unable to create bundle at URL (%@): unable to create file system representation of URL "
                    "(%d)",
                    bundleURL,
                    (int)error
                );
            }
            return NULL;
        }
        if (!exists || (mode & S_IFMT) != S_IFDIR) {
            os_log_t log = _CFBundleLoadingLogger();
            if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
                os_log_debug(
                    log,
                    "Unable to create bundle at URL (%@): does not exist or not a directory (%d)",
                    bundleURL,
                    (int)mode
                );
            }
            OCFRelease(normalizedURL);
            return NULL;
        }
    }
    OCFBundleRef bundle = (OCFBundleRef)_OCFRuntimeCreateInstance(allocator, 31, 200, NULL);
    if (bundle == NULL) {
        OCFRelease(normalizedURL);
        os_log_t log = _CFBundleLoadingLogger();
        if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
            os_log_error(log, "Unable to create bundle at URL (%@): malloc failure", bundleURL);
        }
        return NULL;
    }
    bundle->_url = normalizedURL;
    bundle->_version = version;
    bundle->_binaryType = 0;
    bundle->_isUnique = unique;
    bundle->_createOption1 = options & 1;
    bundle->_bundleLoadingLock = OS_UNFAIR_LOCK_INIT;
    bundle->_lock104 = OS_UNFAIR_LOCK_INIT;
    bundle->_lock168 = OS_UNFAIR_LOCK_INIT;
    OCFURLRef absoluteURL = OCFURLCopyAbsoluteURL(normalizedURL);
    bundle->_bundleBasePath = OCFURLCopyFileSystemPath(absoluteURL, kOCFURLPOSIXPathStyle);
    OCFRelease(absoluteURL);
    bundle->_lock192 = OS_UNFAIR_LOCK_INIT;
    if (limitedKeyPaths != NULL) {
        _CFBundleLoadLimitedInfoDictionary(bundle, limitedKeyPaths);
    }
    OCFDictionaryRef infoDict = OCFBundleGetInfoDictionary(bundle);
    OCFStringRef identifier = OCFBundleGetIdentifier(bundle);
    atomic_thread_fence(memory_order_seq_cst);
    if (consultTables) {
        os_unfair_lock_lock(&CFBundleGlobalDataLock);
        OCFBundleRef existing = _CFBundleCopyFromTablesForURLLocked(bundle->_url);
        if (existing != NULL) {
            os_unfair_lock_unlock(&CFBundleGlobalDataLock);
            OCFRelease(bundle);
            return existing;
        }
        OCFBundleRef sameIdentifier = _CFBundleGetFromTablesLocked(identifier);
        if (sameIdentifier != NULL && _CFBundlesHaveEquivalentURL(bundle, sameIdentifier)) {
            os_unfair_lock_unlock(&CFBundleGlobalDataLock);
            OCFRelease(bundle);
            return (OCFBundleRef)OCFRetain(sameIdentifier);
        }
        if (!skipPlugIn) {
            OCFBundleRef existingPlugIn = NULL;
            if (!_CFBundleInitPlugIn(bundle, infoDict, &existingPlugIn)) {
                OCFTypeRef factories = OCFDictionaryGetValue(infoDict, kOCFPlugInFactoriesKey);
                OCFTypeRef factoryDict = NULL;
                if (factories != NULL) {
                    factoryDict = OCFGetTypeID(factories) == OCFDictionaryGetTypeID() ? factories : NULL;
                }
                os_log_t log = _CFBundleLoadingLogger();
                if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
                    os_log_error(
                        log,
                        "More than one bundle with the same factory UUID detected: %{public}@ in %{public}@ "
                        "and "
                        "%{public}@",
                        factoryDict,
                        bundle,
                        existingPlugIn
                    );
                }
                os_unfair_lock_unlock(&CFBundleGlobalDataLock);
                OCFRelease(bundle);
                if (existingPlugIn != NULL) {
                    OCFRelease(existingPlugIn);
                }
                return NULL;
            }
        }
        if (addToTables) {
            _CFBundleAddToTablesLocked(bundle, identifier);
        }
        os_unfair_lock_unlock(&CFBundleGlobalDataLock);
        if (!skipPlugIn) {
            _CFPlugInHandleDynamicRegistration(bundle);
        }
    }
    if (executableOptions == 0) {
        return bundle;
    }
    OCFURLRef resolvedURL = _CFBundleCopyResolvedURLForExecutableURL(bundleURL);
    if (resolvedURL == NULL) {
        OCFRelease(bundle);
        return NULL;
    }
    OCFURLRef executableURL = OCFBundleCopyAuxiliaryExecutableURL(bundle, NULL);
    OCFURLRef normalizedExecutableURL =
        _CFBundleCreateNormalizedURL(allocator, resolvedURL, (options & 8) != 0);
    OCFRelease(resolvedURL);
    if (executableURL != NULL && normalizedExecutableURL != NULL) {
        if (!OCFURLGetFileSystemRepresentation(executableURL, true, buffer, 1026) ||
            !OCFURLGetFileSystemRepresentation(normalizedExecutableURL, true, executableBuffer, 1026) ||
            strcmp((const char *)buffer, (const char *)executableBuffer) != 0) {
            OCFRelease(bundle);
            bundle = NULL;
        }
    } else {
        OCFRelease(bundle);
        bundle = NULL;
    }
    if (executableURL != NULL) {
        OCFRelease(executableURL);
    }
    if (normalizedExecutableURL != NULL) {
        OCFRelease(normalizedExecutableURL);
    }
    if (bundle != NULL && checkLayout) {
        uint8_t layoutVersion = _CFBundleEffectiveLayoutVersion(bundle);
        if ((uint32_t)layoutVersion - 3 <= 1) {
            OCFDictionaryRef effectiveInfoDict = OCFBundleGetInfoDictionary(bundle);
            if (effectiveInfoDict == NULL || OCFDictionaryGetCount(effectiveInfoDict) == 0) {
                OCFRelease(bundle);
                return NULL;
            }
        }
    }
    return bundle;
}

// Native _CFBundleCreateUniqueWithOptions, 0x1c016bd80. External.
// The slot-18 prelude of _CFBundleCreateUnique (line 1236). Under the CoreFoundation/CFSwiftBundle feature
// the URL, NULL included, is passed to +[_NSSwiftBundle _bundleWithURL:mode:options:filteredInfoKeyPaths:]
// (mode 1, the options, no key paths) and the answer retained; otherwise _CFBundleCreate runs in the unique
// mode with the options and the limited key paths.
OCFBundleRef _OCFBundleCreateUniqueWithOptions(
    OCFAllocatorRef allocator, OCFURLRef bundleURL, OCFSetRef limitedKeyPaths, OCFOptionFlags options
) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        outermost, kSourcePath, 1236, OF_NAME_STRING(_OCFBundleCreateUniqueWithOptions)
    );
    OCFBundleRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        Class swiftBundleClass = objc_opt_class((id)objc_lookUpClass(OF_NAME_STRING(_ONSSwiftBundle)));
        result = ((OCFBundleRef (*)(id, SEL, OCFURLRef, int32_t, OCFOptionFlags, OCFArrayRef))objc_msgSend)(
            (id)swiftBundleClass,
            sel_getUid("_bundleWithURL:mode:options:filteredInfoKeyPaths:"),
            bundleURL,
            1,
            options,
            NULL
        );
        if (result != NULL) {
            OCFRetain(result);
        }
    } else {
        result = _CFBundleCreate(allocator, bundleURL, kCreationModeUnique, options, limitedKeyPaths);
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native _CFBundleCreateMain, 0x1c016beb8. Hidden (was private external).
// _CFBundleCreate in the main-bundle mode, without options or key paths.
OCFBundleRef _CFBundleCreateMain(OCFAllocatorRef allocator, OCFURLRef mainBundleURL) {
    return _CFBundleCreate(allocator, mainBundleURL, kCreationModeMain, 0, NULL);
}

// Native CFBundleCopyBundleURL, 0x1c016bf84. External.
// The slot-18 prelude (line 1278); the Swift path retains bundleURL. Otherwise the type is asserted (31) and
// the URL at +16 is retained and answered.
OCFURLRef OCFBundleCopyBundleURL(OCFBundleRef bundle) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 1278, OF_NAME_STRING(OCFBundleCopyBundleURL));
    OCFURLRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result =
            (OCFURLRef)OCFRetain(((OCFURLRef (*)(id, SEL))objc_msgSend)((id)bundle, sel_getUid("bundleURL")));
    } else {
        OCFTypeID typeID = _CFGetNonObjCTypeID(bundle);
        if (typeID != 31) {
            _CFAssertMismatchedTypeID(31, typeID);
        }
        result = NULL;
        if (bundle->_url != NULL) {
            OCFRetain(bundle->_url);
            result = bundle->_url;
        }
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native CFBundleGetVersionNumber, 0x1c016c09c. External.
// The slot-18 prelude (line 1289); the Swift path answers versionNumber. Otherwise a CFNumber under the
// CFBundleNumericVersion literal is read as SInt32; a missing value or another type answers 0.
UInt32 OCFBundleGetVersionNumber(OCFBundleRef bundle) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 1289, OF_NAME_STRING(OCFBundleGetVersionNumber));
    UInt32 result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((UInt32 (*)(id, SEL))objc_msgSend)((id)bundle, sel_getUid("versionNumber"));
    } else {
        OCFDictionaryRef infoDict = OCFBundleGetInfoDictionary(bundle);
        OCFTypeRef value = OCFDictionaryGetValue(infoDict, OF_STR("CFBundleNumericVersion"));
        result = 0;
        if (value != NULL && OCFGetTypeID(value) == OCFNumberGetTypeID()) {
            SInt32 number = 0;
            OCFNumberGetValue((OCFNumberRef)value, kOCFNumberSInt32Type, &number);
            result = (UInt32)number;
        }
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native CFBundleGetDevelopmentRegion, 0x1c016c21c. External.
// The slot-18 prelude (line 1304); the Swift path answers developmentLocalization. Otherwise the type is
// asserted (31); without an info dictionary the answer is NULL; a value under the CFBundleDevelopmentRegion
// literal that is not a string, or is empty, answers NULL; a missing value answers NULL.
OCFStringRef OCFBundleGetDevelopmentRegion(OCFBundleRef bundle) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 1304, OF_NAME_STRING(OCFBundleGetDevelopmentRegion));
    OCFStringRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)bundle, sel_getUid("developmentLocalization"));
    } else {
        OCFTypeID typeID = _CFGetNonObjCTypeID(bundle);
        if (typeID != 31) {
            _CFAssertMismatchedTypeID(31, typeID);
        }
        OCFDictionaryRef infoDict = OCFBundleGetInfoDictionary(bundle);
        result = NULL;
        if (infoDict != NULL) {
            result = OCFDictionaryGetValue(infoDict, OF_STR("CFBundleDevelopmentRegion"));
            if (result != NULL &&
                (OCFGetTypeID(result) != OCFStringGetTypeID() || OCFStringGetLength(result) == 0)) {
                result = NULL;
            }
        }
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native _CFBundleCopyResourcesDirectoryURLInDirectory, 0x1c016c524. Hidden (was private external).
// A NULL URL answers NULL. Version 0 answers "Resources/", 1 "Support%20Files/Resources/" and 2
// "Contents/Resources/" relative to the bundle URL (CFURLCreateWithString, system default allocator);
// 12 and 13 resolve "WrappedBundle/Contents/Resources/" and "WrappedBundle/" through
// _CFURLCreateResolvedDirectoryWithString; any other version answers the bundle URL retained.
OCFURLRef _CFBundleCopyResourcesDirectoryURLInDirectory(OCFURLRef bundleURL, uint8_t version) {
    if (bundleURL == NULL) {
        return bundleURL;
    }
    OCFStringRef relative;
    switch (version) {
        case 0:
            relative = OF_STR("Resources/");
            break;
        case 1:
            relative = OF_STR("Support%20Files/Resources/");
            break;
        case 2:
            relative = OF_STR("Contents/Resources/");
            break;
        case 12:
            return _CFURLCreateResolvedDirectoryWithString(
                kOCFAllocatorSystemDefault, OF_STR("WrappedBundle/Contents/Resources/"), bundleURL
            );
        case 13:
            return _CFURLCreateResolvedDirectoryWithString(
                kOCFAllocatorSystemDefault, OF_STR("WrappedBundle/"), bundleURL
            );
        default:
            return (OCFURLRef)OCFRetain(bundleURL);
    }
    return OCFURLCreateWithString(kOCFAllocatorSystemDefault, relative, bundleURL);
}

// Native CFBundleCopyResourcesDirectoryURL, 0x1c016c5dc. External.
// The slot-18 prelude (line 1384); the Swift path retains resourceURL. Otherwise the type is asserted (31)
// and the resources directory of the bundle URL is computed for the stored version.
OCFURLRef OCFBundleCopyResourcesDirectoryURL(OCFBundleRef bundle) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        outermost, kSourcePath, 1384, OF_NAME_STRING(OCFBundleCopyResourcesDirectoryURL)
    );
    OCFURLRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((OCFURLRef (*)(id, SEL))objc_msgSend)((id)bundle, sel_getUid("resourceURL"));
        if (result != NULL) {
            OCFRetain(result);
        }
    } else {
        OCFTypeID typeID = _CFGetNonObjCTypeID(bundle);
        if (typeID != 31) {
            _CFAssertMismatchedTypeID(31, typeID);
        }
        result = _CFBundleCopyResourcesDirectoryURLInDirectory(bundle->_url, bundle->_version);
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native _CFBundleCopyExecutableName, 0x1c016cac4. Hidden (was private external).
// The info dictionary defaults to the bundle's and the URL to the bundle URL. A nonempty string under the
// CFBundleExecutable literal, or else NSExecutable, is retained and answered. Otherwise the URL's absolute
// POSIX path gives the last component without its extension (when start <= length, end <= length and
// end > start); no URL or path answers NULL.
OCFStringRef _CFBundleCopyExecutableName(OCFBundleRef bundle, OCFURLRef url, OCFDictionaryRef infoDict) {
    if (bundle != NULL && infoDict == NULL) {
        infoDict = OCFBundleGetInfoDictionary(bundle);
    }
    if (bundle != NULL && url == NULL) {
        url = bundle->_url;
    }
    if (infoDict != NULL) {
        OCFStringRef name = OCFDictionaryGetValue(infoDict, OF_STR("CFBundleExecutable"));
        if (name == NULL) {
            name = OCFDictionaryGetValue(infoDict, OF_STR("NSExecutable"));
        }
        if (name != NULL && OCFGetTypeID(name) == OCFStringGetTypeID() && OCFStringGetLength(name) >= 1) {
            OCFRetain(name);
            return name;
        }
    }
    if (url == NULL) {
        return NULL;
    }
    OCFURLRef absoluteURL = OCFURLCopyAbsoluteURL(url);
    OCFStringRef path = OCFURLCopyFileSystemPath(absoluteURL, kOCFURLPOSIXPathStyle);
    OCFRelease(absoluteURL);
    if (path == NULL) {
        return NULL;
    }
    OCFIndex length = OCFStringGetLength(path);
    OCFIndex start = _CFStartOfLastPathComponent2(path);
    OCFIndex end = _CFLengthAfterDeletingPathExtension2(path);
    OCFStringRef name = NULL;
    if (start <= length && end <= length && end > start) {
        name = OCFStringCreateWithSubstring(kOCFAllocatorSystemDefault, path, (OCFRange){start, end - start});
    }
    OCFRelease(path);
    return name;
}

// Native CFBundleIsExecutableLoaded, 0x1c016cc04. External.
// The slot-18 prelude (line 1500); the Swift path answers isLoaded. Otherwise the type is asserted (31) and
// +52 is read under the loading lock (options 0x10000).
Boolean OCFBundleIsExecutableLoaded(OCFBundleRef bundle) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 1500, OF_NAME_STRING(OCFBundleIsExecutableLoaded));
    Boolean result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((BOOL (*)(id, SEL))objc_msgSend)((id)bundle, sel_getUid("isLoaded"));
    } else {
        OCFTypeID typeID = _CFGetNonObjCTypeID(bundle);
        if (typeID != 31) {
            _CFAssertMismatchedTypeID(31, typeID);
        }
        os_unfair_lock_lock_with_options(&bundle->_bundleLoadingLock, 0x10000);
        result = bundle->_isLoaded;
        os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native _CFBundleCreateErrorDebug, 0x1c016ce68. Hidden (was a private external).
// The bundle URL, its absolute URL, the executable URL and the CoreFoundation bundle are fetched first, then
// the POSIX paths of the absolute bundle URL and of the executable URL. With the CoreFoundation bundle, the
// bundle's name is its CFBundleName (retained), else the last path component (_CFStartOfLastPathComponent
// over at most 1026 characters of the bundle URL's POSIX path, none for an empty path; a system-default
// string), else "<unknown>" without a bundle URL. Codes 4, 3584, 3585, 3586, 3587 and 3588 copy a description
// format, a failure reason and a recovery suggestion from the CoreFoundation bundle's Error table (keys
// BundleErrN, -C and -R; the defaults keep their literal \U escapes), the description being formatted with
// the name through CFStringCreateStringWithValidatedFormat ("%@"); other codes have none. The name is then
// released. The user info holds, in order and only when present, NSBundlePath, NSFilePath, the localized
// description, failure reason and recovery suggestion and the debug description; the error has the Cocoa
// domain and the code. Every copied object is released afterwards.
OCFErrorRef _CFBundleCreateErrorDebug(
    OCFAllocatorRef allocator, OCFBundleRef bundle, OCFIndex code, OCFStringRef debugString
) {
    OCFURLRef bundleURL = OCFBundleCopyBundleURL(bundle);
    OCFURLRef absoluteURL = OCFURLCopyAbsoluteURL(bundleURL);
    OCFURLRef executableURL = OCFBundleCopyExecutableURL(bundle);
    OCFBundleRef coreFoundationBundle = OCFBundleGetBundleWithIdentifier(OF_STR("com.apple.CoreFoundation"));
    OCFStringRef bundlePath = OCFURLCopyFileSystemPath(absoluteURL, kOCFURLPOSIXPathStyle);
    OCFStringRef executablePath =
        executableURL != NULL ? OCFURLCopyFileSystemPath(executableURL, kOCFURLPOSIXPathStyle) : NULL;
    const void *userInfoKeys[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
    const void *userInfoValues[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
    OCFStringRef description = NULL;
    OCFStringRef reason = NULL;
    OCFStringRef suggestion = NULL;
    if (coreFoundationBundle != NULL) {
        OCFStringRef name;
        OCFTypeRef bundleName = OCFBundleGetValueForInfoDictionaryKey(bundle, OF_STR("CFBundleName"));
        if (bundleName != NULL) {
            name = OCFRetain(bundleName);
        } else {
            OCFURLRef url = OCFBundleCopyBundleURL(bundle);
            if (url == NULL) {
                name = OF_STR("<unknown>");
            } else {
                UniChar buffer[1026];
                OCFStringRef path = OCFURLCopyFileSystemPath(url, kOCFURLPOSIXPathStyle);
                OCFIndex length = OCFStringGetLength(path);
                OCFRelease(url);
                OCFIndex count = length < 0x402 ? length : 0x402;
                OCFStringGetCharacters(path, (OCFRange){0, count}, buffer);
                OCFRelease(path);
                OCFIndex start = length >= 1 ? _CFStartOfLastPathComponent(buffer, count) : 0;
                name =
                    OCFStringCreateWithCharacters(kOCFAllocatorSystemDefault, &buffer[start], count - start);
            }
        }
        OCFStringRef format = NULL;
        Boolean known = true;
        OCFStringRef suggestionKey = NULL;
        OCFStringRef suggestionValue = OF_STR("Try reinstalling the bundle.");
        switch (code) {
            case 4:
                format = OCFBundleCopyLocalizedString(
                    coreFoundationBundle,
                    OF_STR("BundleErr4"),
                    OF_STR(
                        "The bundle \\U201c%@\\U201d couldn\\U2019t be loaded because its executable "
                        "couldn\\U2019t be located."
                    ),
                    OF_STR("Error")
                );
                reason = OCFBundleCopyLocalizedString(
                    coreFoundationBundle,
                    OF_STR("BundleErr4-C"),
                    OF_STR("The bundle\\U2019s executable couldn\\U2019t be located."),
                    OF_STR("Error")
                );
                suggestionKey = OF_STR("BundleErr4-R");
                break;
            case 3584:
                format = OCFBundleCopyLocalizedString(
                    coreFoundationBundle,
                    OF_STR("BundleErr3584"),
                    OF_STR(
                        "The bundle \\U201c%@\\U201d couldn\\U2019t be loaded because its executable "
                        "isn\\U2019t loadable."
                    ),
                    OF_STR("Error")
                );
                reason = OCFBundleCopyLocalizedString(
                    coreFoundationBundle,
                    OF_STR("BundleErr3584-C"),
                    OF_STR("The bundle\\U2019s executable isn\\U2019t loadable."),
                    OF_STR("Error")
                );
                suggestionKey = OF_STR("BundleErr3584-R");
                break;
            case 3585:
                format = OCFBundleCopyLocalizedString(
                    coreFoundationBundle,
                    OF_STR("BundleErr3585"),
                    OF_STR(
                        "The bundle \\U201c%@\\U201d couldn\\U2019t be loaded because it doesn\\U2019t "
                        "contain a version for the current architecture."
                    ),
                    OF_STR("Error")
                );
                reason = OCFBundleCopyLocalizedString(
                    coreFoundationBundle,
                    OF_STR("BundleErr3585-C"),
                    OF_STR("The bundle doesn\\U2019t contain a version for the current architecture."),
                    OF_STR("Error")
                );
                suggestionKey = OF_STR("BundleErr3585-R");
                suggestionValue = OF_STR("Try installing a universal version of the bundle.");
                break;
            case 3586:
                format = OCFBundleCopyLocalizedString(
                    coreFoundationBundle,
                    OF_STR("BundleErr3586"),
                    OF_STR(
                        "The bundle \\U201c%@\\U201d couldn\\U2019t be loaded because it isn\\U2019t "
                        "compatible with the current application."
                    ),
                    OF_STR("Error")
                );
                reason = OCFBundleCopyLocalizedString(
                    coreFoundationBundle,
                    OF_STR("BundleErr3586-C"),
                    OF_STR("The bundle isn\\U2019t compatible with this application."),
                    OF_STR("Error")
                );
                suggestionKey = OF_STR("BundleErr3586-R");
                suggestionValue = OF_STR("Try installing a newer version of the bundle.");
                break;
            case 3587:
                format = OCFBundleCopyLocalizedString(
                    coreFoundationBundle,
                    OF_STR("BundleErr3587"),
                    OF_STR(
                        "The bundle \\U201c%@\\U201d couldn\\U2019t be loaded because it is damaged or "
                        "missing necessary resources."
                    ),
                    OF_STR("Error")
                );
                reason = OCFBundleCopyLocalizedString(
                    coreFoundationBundle,
                    OF_STR("BundleErr3587-C"),
                    OF_STR("The bundle is damaged or missing necessary resources."),
                    OF_STR("Error")
                );
                suggestionKey = OF_STR("BundleErr3587-R");
                break;
            case 3588:
                format = OCFBundleCopyLocalizedString(
                    coreFoundationBundle,
                    OF_STR("BundleErr3588"),
                    OF_STR("The bundle \\U201c%@\\U201d couldn\\U2019t be loaded."),
                    OF_STR("Error")
                );
                reason = OCFBundleCopyLocalizedString(
                    coreFoundationBundle,
                    OF_STR("BundleErr3588-C"),
                    OF_STR("The bundle couldn\\U2019t be loaded."),
                    OF_STR("Error")
                );
                suggestionKey = OF_STR("BundleErr3588-R");
                break;
            default:
                known = false;
                break;
        }
        if (known) {
            suggestion = OCFBundleCopyLocalizedString(
                coreFoundationBundle, suggestionKey, suggestionValue, OF_STR("Error")
            );
            if (format != NULL) {
                description = OCFStringCreateStringWithValidatedFormat(
                    allocator, NULL, OF_STR("%@"), format, NULL, name
                );
                OCFRelease(format);
            }
        }
        OCFRelease(name);
    }
    OCFIndex count = 0;
    if (bundlePath != NULL) {
        userInfoKeys[count] = OF_STR("NSBundlePath");
        userInfoValues[count] = bundlePath;
        count += 1;
    }
    if (executablePath != NULL) {
        userInfoKeys[count] = OF_STR("NSFilePath");
        userInfoValues[count] = executablePath;
        count += 1;
    }
    if (description != NULL) {
        userInfoKeys[count] = kOCFErrorLocalizedDescriptionKey;
        userInfoValues[count] = description;
        count += 1;
    }
    if (reason != NULL) {
        userInfoKeys[count] = kOCFErrorLocalizedFailureReasonKey;
        userInfoValues[count] = reason;
        count += 1;
    }
    if (suggestion != NULL) {
        userInfoKeys[count] = kOCFErrorLocalizedRecoverySuggestionKey;
        userInfoValues[count] = suggestion;
        count += 1;
    }
    if (debugString != NULL) {
        userInfoKeys[count] = kOCFErrorDebugDescriptionKey;
        userInfoValues[count] = debugString;
        count += 1;
    }
    OCFErrorRef error = OCFErrorCreateWithUserInfoKeysAndValues(
        allocator, kOCFErrorDomainCocoa, code, userInfoKeys, userInfoValues, count
    );
    if (bundleURL != NULL) {
        OCFRelease(bundleURL);
    }
    if (absoluteURL != NULL) {
        OCFRelease(absoluteURL);
    }
    if (executableURL != NULL) {
        OCFRelease(executableURL);
    }
    if (bundlePath != NULL) {
        OCFRelease(bundlePath);
    }
    if (executablePath != NULL) {
        OCFRelease(executablePath);
    }
    if (description != NULL) {
        OCFRelease(description);
    }
    if (reason != NULL) {
        OCFRelease(reason);
    }
    if (suggestion != NULL) {
        OCFRelease(suggestion);
    }
    return error;
}

// Native _CFBundleCreateError, 0x1c016d444. External. _CFBundleCreateErrorDebug without a debug string.
OCFErrorRef _OCFBundleCreateError(OCFAllocatorRef allocator, OCFBundleRef bundle, OCFIndex code) {
    return _CFBundleCreateErrorDebug(allocator, bundle, code, NULL);
}

// Native _CFBundleLoadExecutableAndReturnError, 0x1c016d44c, with its .cold.1 to .cold.6 (0x1c03066a4,
// 0x1c03066a8, 0x1c03066d4, 0x1c03066d8, 0x1c0306704, 0x1c0306708: the loading logger's once and three error
// logs). External.
// The slot-18 prelude (line 1658). Under the CoreFoundation/CFSwiftBundle feature the bundle is sent
// -loadForcingGlobalAndReturnError: (the error retained into the pointer for a result other than YES) or
// -loadAndReturnError: with the caller's pointer. Otherwise (type 31 asserted) the executable URL is copied
// and, under the loading lock (os_unfair_lock_lock without options), a missing executable sets binary type 7,
// _CFBundleDlfcnCheckLoaded runs, an unknown type (0) is grokked from the URL, and the URL is released. A
// loaded bundle is unscheduled for unloading and answers true. Types 0, 3, 6 and 8 load as a bundle (with
// forceGlobal), 4 as a framework; 7 logs "Cannot find executable" and gives error 4; 2 logs "Attempt to load
// executable of a type that cannot be dynamically loaded", 1, 5 and others "Cannot recognize type of
// executable", both error 3584 (no debug description). A load failure passes its error through; a loaded
// plug-in bundle handles dynamic registration.
Boolean _OCFBundleLoadExecutableAndReturnError(OCFBundleRef bundle, Boolean forceGlobal, OCFErrorRef *error) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        outermost, kSourcePath, 1658, OF_NAME_STRING(_OCFBundleLoadExecutableAndReturnError)
    );
    Boolean result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        OCFErrorRef swiftError = NULL;
        if (forceGlobal) {
            result = ((BOOL (*)(id, SEL, OCFErrorRef *))objc_msgSend)(
                (id)bundle, sel_getUid("loadForcingGlobalAndReturnError:"), &swiftError
            );
            if (error != NULL && result != 1 && swiftError != NULL) {
                *error = (OCFErrorRef)OCFRetain(swiftError);
            }
        } else {
            result = ((BOOL (*)(id, SEL, OCFErrorRef *))objc_msgSend)(
                (id)bundle, sel_getUid("loadAndReturnError:"), error
            );
        }
    } else {
        OCFTypeID typeID = _CFGetNonObjCTypeID(bundle);
        if (typeID != 31) {
            _CFAssertMismatchedTypeID(31, typeID);
        }
        OCFErrorRef localError = NULL;
        OCFErrorRef *errorPointer = error != NULL ? &localError : NULL;
        OCFURLRef executableURL = OCFBundleCopyExecutableURL(bundle);
        os_unfair_lock_lock(&bundle->_bundleLoadingLock);
        if (executableURL == NULL) {
            bundle->_binaryType = 7;
        }
        _CFBundleDlfcnCheckLoaded(bundle);
        if (bundle->_binaryType == 0) {
            bundle->_binaryType = _OCFBundleGrokBinaryType(executableURL);
        }
        if (executableURL != NULL) {
            OCFRelease(executableURL);
        }
        Boolean loaded = bundle->_isLoaded;
        os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
        if (loaded) {
            _CFPlugInUnscheduleForUnloading(bundle);
            result = true;
        } else {
            OCFIndex code = 0;
            Boolean attempted = true;
            switch (bundle->_binaryType) {
                case 0:
                case 3:
                case 6:
                case 8:
                    result = _CFBundleDlfcnLoadBundle(bundle, forceGlobal, errorPointer);
                    break;
                case 4:
                    result = _CFBundleDlfcnLoadFramework(bundle, errorPointer);
                    break;
                case 7: {
                    os_log_t log = _CFBundleLoadingLogger();
                    if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
                        os_log_error(log, "Cannot find executable for %{public}@", bundle);
                    }
                    attempted = false;
                    code = 4;
                    break;
                }
                case 2: {
                    os_log_t log = _CFBundleLoadingLogger();
                    if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
                        os_log_error(
                            log,
                            "Attempt to load executable of a type that cannot be dynamically loaded for "
                            "%{public}@",
                            bundle
                        );
                    }
                    attempted = false;
                    code = 3584;
                    break;
                }
                default: {
                    os_log_t log = _CFBundleLoadingLogger();
                    if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
                        os_log_error(log, "Cannot recognize type of executable for %{public}@", bundle);
                    }
                    attempted = false;
                    code = 3584;
                    break;
                }
            }
            if (!attempted) {
                if (error != NULL) {
                    localError = _CFBundleCreateErrorDebug(OCFGetAllocator(bundle), bundle, code, NULL);
                    *error = localError;
                }
                result = false;
            } else if (!result) {
                if (error != NULL) {
                    *error = localError;
                }
            } else if (bundle->_plugInData._isPlugIn) {
                _CFPlugInHandleDynamicRegistration(bundle);
            }
        }
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native CFBundleCopyExecutableArchitectures, 0x1c016dc40. External.
// The slot-18 prelude (line 1855). Under the CoreFoundation/CFSwiftBundle feature the bundle's
// -executableArchitectures is retained; otherwise the architectures of the executable URL, NULL without one.
OCFArrayRef OCFBundleCopyExecutableArchitectures(OCFBundleRef bundle) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        outermost, kSourcePath, 1855, OF_NAME_STRING(OCFBundleCopyExecutableArchitectures)
    );
    OCFArrayRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((OCFArrayRef (*)(id, SEL))objc_msgSend)((id)bundle, sel_getUid("executableArchitectures"));
        if (result != NULL) {
            OCFRetain(result);
        }
    } else {
        OCFURLRef executableURL = OCFBundleCopyExecutableURL(bundle);
        if (executableURL != NULL) {
            result = _CFBundleCopyArchitecturesForExecutable(executableURL);
            OCFRelease(executableURL);
        } else {
            result = NULL;
        }
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native CFBundleUnloadExecutable, 0x1c016dd5c. External.
// The slot-18 prelude (line 1876); the Swift path sends unload, otherwise _CFBundleUnloadExecutable runs
// without keeping the scheduled plug-ins.
void OCFBundleUnloadExecutable(OCFBundleRef bundle) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 1876, OF_NAME_STRING(OCFBundleUnloadExecutable));
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        ((void (*)(id, SEL))objc_msgSend)((id)bundle, sel_getUid("unload"));
    } else {
        _CFBundleUnloadExecutable(bundle, false);
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
}

// Native _CFBundleUnloadExecutable, 0x1c016de48. Hidden (was private external).
// The type is asserted (31). Unless keepScheduledPlugIns (W1), the scheduled plug-ins are unloaded first. The
// loaded flag is read under the loading lock; a loaded bundle is unscheduled and its plug-in told it will
// unload, then under the lock, when still loaded and either not of binary type 4 or linked on or after
// system version 5, the dlopen handle is closed.
void _CFBundleUnloadExecutable(OCFBundleRef bundle, Boolean keepScheduledPlugIns) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(bundle);
    if (typeID != 31) {
        _CFAssertMismatchedTypeID(31, typeID);
    }
    if (!keepScheduledPlugIns) {
        _CFPlugInUnloadScheduledPlugIns();
    }
    os_unfair_lock_lock_with_options(&bundle->_bundleLoadingLock, 0x10000);
    uint8_t loaded = bundle->_isLoaded;
    os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
    if (loaded) {
        _CFPlugInUnscheduleForUnloading(bundle);
        _CFPlugInWillUnload(bundle);
        os_unfair_lock_lock_with_options(&bundle->_bundleLoadingLock, 0x10000);
        if (bundle->_isLoaded && (bundle->_binaryType != 4 || _OCFExecutableLinkedOnOrAfter(5))) {
            _CFBundleDlfcnUnload(bundle);
        }
        os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
    }
}

// Native __CFBundleGetPlugInData, 0x1c016dfe4. Hidden (was private external).
// Answers bundle + 64.
_CFPlugInData *__CFBundleGetPlugInData(OCFBundleRef bundle) {
    return &bundle->_plugInData;
}

// Native _CFBundleCouldBeBundle, 0x1c016dfec. Hidden (was private external).
// _CFGetFileProperties (system default allocator) must succeed and report an existing directory
// ((mode & 0xF000) == 0x4000) whose mode has a read bit (mode & 0444).
Boolean _CFBundleCouldBeBundle(OCFURLRef url) {
    Boolean exists = false;
    SInt32 mode = 0;
    SInt32 error =
        _CFGetFileProperties(kOCFAllocatorSystemDefault, url, &exists, &mode, NULL, NULL, NULL, NULL);
    if (error != 0 || !exists) {
        return false;
    }
    Boolean readable = (mode & 0x124) != 0;
    return (mode & 0xF000) == 0x4000 ? readable : false;
}

// Native __CFBundleCopyFrameworkURLForExecutablePath, 0x1c016e1a8. Non-external.
// The path (at most 1,026 UTF-16 units, both 2,052-byte buffers zeroed first) loses its extension; the last
// component of the rest is copied to a second buffer and the parent directories are walked upward through a
// mutable string over external characters. Unless permissive (W1), the copied component's UTF-16 bytes are
// passed to CFStringCreateWithFileSystemRepresentation as a C string (natively so: an ASCII name yields its
// first character), and a "Resources" directory stops the walk with NULL. A "Support Files" or "Contents"
// directory answers its parent (in the strict mode only when the directory's own name starts with that
// string); a ".framework" directory (in the strict mode starting with it) answers itself. The candidate
// becomes a directory URL that must pass _CFBundleCouldBeBundle; a failing candidate ends the walk with NULL.
static OCFURLRef
__CFBundleCopyFrameworkURLForExecutablePath(OCFStringRef executablePath, Boolean permissive) {
    UniChar buffer[1026];
    UniChar nameBuffer[1026];
    bzero(buffer, sizeof(buffer));
    bzero(nameBuffer, sizeof(nameBuffer));
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableStringRef component =
        OCFStringCreateMutableWithExternalCharactersNoCopy(allocator, NULL, 0, 0, NULL);
    OCFIndex length = OCFStringGetLength(executablePath);
    if (length >= 1026) {
        length = 1026;
    }
    OCFStringGetCharacters(executablePath, (OCFRange){0, length}, buffer);
    OCFIndex nameEnd = _CFLengthAfterDeletingPathExtension(buffer, length);
    OCFIndex nameStart = _CFStartOfLastPathComponent(buffer, nameEnd);
    __builtin___memmove_chk(
        nameBuffer, &buffer[nameStart], (size_t)(nameEnd - nameStart) * 2, sizeof(nameBuffer)
    );
    OCFIndex directoryLength = _CFLengthAfterDeletingLastPathComponent(buffer, nameEnd);
    OCFStringRef executableName;
    if (permissive) {
        executableName = OF_STR("");
    } else {
        executableName = OCFStringCreateWithFileSystemRepresentation(allocator, (const char *)nameBuffer);
    }
    OCFURLRef result = NULL;
    while (directoryLength >= 1) {
        OCFIndex start = _CFStartOfLastPathComponent(buffer, directoryLength);
        if (directoryLength - start <= 0) {
            break;
        }
        OCFStringSetExternalCharactersNoCopy(
            component, &buffer[start], directoryLength - start, 1026 - start
        );
        if (!permissive && OCFEqual(component, OF_STR("Resources"))) {
            OCFRelease(executableName);
            OCFStringSetExternalCharactersNoCopy(component, NULL, 0, 0);
            OCFRelease(component);
            return NULL;
        }
        OCFIndex bundleLength = -1;
        if (OCFEqual(component, OF_STR("Support Files")) || OCFEqual(component, OF_STR("Contents"))) {
            Boolean matches = permissive;
            if (!matches) {
                OCFIndex componentStart = _CFStartOfLastPathComponent(buffer, directoryLength);
                OCFStringSetExternalCharactersNoCopy(
                    component,
                    &buffer[componentStart],
                    directoryLength - componentStart,
                    1026 - componentStart
                );
                matches = OCFStringHasPrefix(component, executableName);
            }
            if (matches) {
                bundleLength = _CFLengthAfterDeletingLastPathComponent(buffer, directoryLength);
            }
        } else if (
            OCFStringHasSuffix(component, OF_STR(".framework")) &&
            (permissive || OCFStringHasPrefix(component, executableName))
        ) {
            bundleLength = directoryLength;
        }
        if (bundleLength >= 0) {
            OCFStringSetExternalCharactersNoCopy(component, buffer, bundleLength, 1026);
            OCFURLRef candidate =
                OCFURLCreateWithFileSystemPath(allocator, component, kOCFURLPOSIXPathStyle, true);
            if (_CFBundleCouldBeBundle(candidate)) {
                result = candidate;
            } else {
                OCFRelease(candidate);
            }
            break;
        }
        directoryLength = _CFLengthAfterDeletingLastPathComponent(buffer, directoryLength);
    }
    if (!permissive) {
        OCFRelease(executableName);
    }
    OCFStringSetExternalCharactersNoCopy(component, NULL, 0, 0);
    OCFRelease(component);
    return result;
}

// Native _CFBundleEnsureBundlesUpToDateWithHint, 0x1c016e5a0. Non-external.
// The main bundle is materialized first; the image paths matching the hint get bundles.
static void _CFBundleEnsureBundlesUpToDateWithHint(OCFStringRef hint) {
    OCFBundleGetMainBundle();
    OCFArrayRef imagePaths = _CFBundleDYLDCopyLoadedImagePathsForHint(hint);
    if (imagePaths != NULL) {
        _CFBundleEnsureBundlesExistForImagePaths(imagePaths);
        OCFRelease(imagePaths);
    }
}

// Native _CFBundleEnsureAllBundlesUpToDate, 0x1c016e900. Non-external.
// The main bundle is materialized first; the loaded image paths, when they changed, get bundles.
static void _CFBundleEnsureAllBundlesUpToDate(void) {
    OCFBundleGetMainBundle();
    OCFArrayRef imagePaths = _CFBundleDYLDCopyLoadedImagePathsIfChanged();
    if (imagePaths != NULL) {
        _CFBundleEnsureBundlesExistForImagePaths(imagePaths);
        OCFRelease(imagePaths);
    }
}

// Native _CFBundleLayoutVersion, 0x1c016e93c. Hidden (was private external).
// Answers the byte at +53.
uint8_t _CFBundleLayoutVersion(OCFBundleRef bundle) {
    return bundle->_version;
}

// Native _CFBundleEnsureBundleExistsForImagePath, 0x1c016f0d8. Non-external.
// The framework URL of the image path (permissive passed through) is created as a bundle in the loaded-image
// mode (system default allocator). Under the plain loading lock an unloaded bundle asks dlfcn whether it is
// loaded, takes binary type 4 when it has none, and when still unloaded logs "Ensure bundle %{private}@ set
// loaded fallback" (.cold.2, debug level). The plug-in is then initialized and registered. A failed creation
// logs "Ensure bundle exists failed for path %{private}@" (.cold.4, debug level). The created bundle is not
// released: the tables do not retain it, and this reference keeps it alive.
static void _CFBundleEnsureBundleExistsForImagePath(OCFStringRef imagePath, Boolean permissive) {
    OCFURLRef frameworkURL = __CFBundleCopyFrameworkURLForExecutablePath(imagePath, permissive);
    if (frameworkURL == NULL) {
        return;
    }
    OCFBundleRef bundle =
        _CFBundleCreate(kOCFAllocatorSystemDefault, frameworkURL, kCreationModeLoadedImage, 0, NULL);
    if (bundle != NULL) {
        os_unfair_lock_lock(&bundle->_bundleLoadingLock);
        if (!bundle->_isLoaded) {
            _CFBundleDlfcnCheckLoaded(bundle);
            if (bundle->_binaryType == 0) {
                bundle->_binaryType = 4;
            }
            if (!bundle->_isLoaded) {
                os_log_t log = _CFBundleLoadingLogger();
                if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
                    os_log_debug(log, "Ensure bundle %{private}@ set loaded fallback", bundle);
                }
            }
        }
        os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
        OCFDictionaryRef infoDict = OCFBundleGetInfoDictionary(bundle);
        _CFBundleInitPlugIn(bundle, infoDict, NULL);
        _CFPlugInHandleDynamicRegistration(bundle);
    } else {
        os_log_t log = _CFBundleLoadingLogger();
        if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
            os_log_debug(log, "Ensure bundle exists failed for path %{private}@", imagePath);
        }
    }
    OCFRelease(frameworkURL);
}

// Native _CFBundleCopyFromTablesForURLLocked, 0x1c016f214. Non-external.
// A bundle in the URL table is retained and answered while its URL (+16) is set; a bundle being deallocated
// (URL cleared) is removed from the table and NULL answered.
static OCFBundleRef _CFBundleCopyFromTablesForURLLocked(OCFURLRef url) {
    if (_bundlesByURL == NULL) {
        return NULL;
    }
    OCFBundleRef bundle = (OCFBundleRef)OCFDictionaryGetValue(_bundlesByURL, url);
    if (bundle != NULL) {
        if (bundle->_url != NULL) {
            OCFRetain(bundle);
            return bundle;
        }
        OCFDictionaryRemoveValue(_bundlesByURL, url);
        return NULL;
    }
    return bundle;
}

// Native _CFBundleGetFromTablesLocked, 0x1c016f27c. Non-external.
// The first bundle of the identifier's array whose executable is loaded answers; when none is, the first
// element answers. A NULL identifier, no table, no array or an empty array answers NULL.
static OCFBundleRef _CFBundleGetFromTablesLocked(OCFStringRef bundleID) {
    OCFBundleRef bundle = NULL;
    if (bundleID == NULL || _bundlesByIdentifier == NULL) {
        return bundle;
    }
    OCFArrayRef bundles = OCFDictionaryGetValue(_bundlesByIdentifier, bundleID);
    if (bundles == NULL) {
        return NULL;
    }
    OCFIndex count = OCFArrayGetCount(bundles);
    if (count < 1) {
        return NULL;
    }
    OCFIndex index = 0;
    Boolean notLoaded;
    do {
        bundle = (OCFBundleRef)OCFArrayGetValueAtIndex(bundles, index);
        Boolean loaded = OCFBundleIsExecutableLoaded(bundle);
        index++;
        notLoaded = !loaded || bundle == NULL;
    } while (notLoaded && index < count);
    if (notLoaded) {
        return (OCFBundleRef)OCFArrayGetValueAtIndex(bundles, 0);
    }
    return bundle;
}

// Native _CFBundlesHaveEquivalentURL, 0x1c016f328. Non-external.
// Both base paths (+184) need file system representations (1,024 bytes); both are opened read-only and,
// when both descriptors are at least 1, fstat must succeed on both with equal st_dev and st_ino. Every
// descriptor of at least 1 is closed.
static Boolean _CFBundlesHaveEquivalentURL(OCFBundleRef bundle1, OCFBundleRef bundle2) {
    char path1[1024];
    char path2[1024];
    OCFStringRef basePath2 = bundle2->_bundleBasePath;
    if (!OCFStringGetFileSystemRepresentation(bundle1->_bundleBasePath, path1, 1024) ||
        !OCFStringGetFileSystemRepresentation(basePath2, path2, 1024)) {
        return false;
    }
    int descriptor1 = open(path1, O_RDONLY);
    int descriptor2 = open(path2, O_RDONLY);
    Boolean result = false;
    if (descriptor1 >= 1 && descriptor2 >= 1) {
        struct stat status1;
        struct stat status2;
        memset(&status1, 0, sizeof(status1));
        memset(&status2, 0, sizeof(status2));
        Boolean equivalent = fstat(descriptor1, &status1) == 0 && fstat(descriptor2, &status2) == 0 &&
                             status1.st_dev == status2.st_dev && status1.st_ino == status2.st_ino;
        close(descriptor1);
        result = equivalent;
    } else if (descriptor1 >= 1) {
        close(descriptor1);
    }
    if (descriptor2 >= 1) {
        close(descriptor2);
    }
    return result;
}

// Native _CFBundleAddToTablesLocked, 0x1c016f48c. Non-external.
// A unique bundle (+54) is not added. Otherwise the bundle joins _allBundles (kCFTypeArrayCallBacks) and the
// URL table (type keys; values with the type value callbacks less retain and release); with an identifier it
// joins that identifier's array (the type array callbacks less retain and release) at the first position
// whose bundle's version number is not above its own (unsigned). When the array already existed and the
// resource logger's debug level is enabled, "Bundle %p at %@" lines are collected and "More than one bundle
// with the same identifier has been added: %{public}@" is logged (.cold.3).
// Folds the compiler-outlined pieces __CFBundleAddToTablesLocked.cold.1 (0x1c0306794),
// __CFBundleAddToTablesLocked.cold.2 (0x1c03067a8), __CFBundleAddToTablesLocked.cold.3 (0x1c03067bc).
static void _CFBundleAddToTablesLocked(OCFBundleRef bundle, OCFStringRef bundleID) {
    if (bundle->_isUnique) {
        return;
    }
    if (_allBundles == NULL) {
        OCFArrayCallBacks callBacks = kOCFTypeArrayCallBacks;
        callBacks.equal = kOCFTypeArrayCallBacks.equal;
        _allBundles = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &callBacks);
    }
    OCFArrayAppendValue(_allBundles, bundle);
    if (_bundlesByURL == NULL) {
        OCFDictionaryValueCallBacks callBacks = kOCFTypeDictionaryValueCallBacks;
        callBacks.equal = kOCFTypeDictionaryValueCallBacks.equal;
        callBacks.retain = NULL;
        callBacks.release = NULL;
        _bundlesByURL = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &callBacks
        );
    }
    OCFDictionarySetValue(_bundlesByURL, bundle->_url, bundle);
    if (bundleID == NULL) {
        return;
    }
    if (_bundlesByIdentifier == NULL) {
        _bundlesByIdentifier = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    OCFMutableArrayRef bundles = (OCFMutableArrayRef)OCFDictionaryGetValue(_bundlesByIdentifier, bundleID);
    if (bundles == NULL) {
        OCFArrayCallBacks callBacks = kOCFTypeArrayCallBacks;
        callBacks.equal = kOCFTypeArrayCallBacks.equal;
        callBacks.retain = NULL;
        callBacks.release = NULL;
        bundles = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &callBacks);
        OCFArrayAppendValue(bundles, bundle);
        OCFDictionarySetValue(_bundlesByIdentifier, bundleID, bundles);
        OCFRelease(bundles);
        return;
    }
    OCFIndex count = OCFArrayGetCount(bundles);
    UInt32 version = OCFBundleGetVersionNumber(bundle);
    OCFIndex index = 0;
    if (count >= 1) {
        while (version < OCFBundleGetVersionNumber((OCFBundleRef)OCFArrayGetValueAtIndex(bundles, index))) {
            index++;
            if (count == index) {
                index = count;
                break;
            }
        }
    }
    OCFArrayInsertValueAtIndex(bundles, index, bundle);
    if (!os_log_type_enabled(_CFBundleResourceLogger(), OS_LOG_TYPE_DEBUG)) {
        return;
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableArrayRef descriptions = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
    for (OCFIndex i = 0; i < OCFArrayGetCount(bundles); i++) {
        OCFBundleRef other = (OCFBundleRef)OCFArrayGetValueAtIndex(bundles, i);
        OCFStringRef description =
            OCFStringCreateWithFormat(allocator, NULL, OF_STR("Bundle %p at %@"), other, other->_url);
        OCFArrayAppendValue(descriptions, description);
        OCFRelease(description);
    }
    os_log_t log = _CFBundleResourceLogger();
    if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
        os_log_debug(
            log, "More than one bundle with the same identifier has been added: %{public}@", descriptions
        );
    }
    OCFRelease(descriptions);
}

// Native _CFBundleEnsureBundlesExistForImagePaths, 0x1c016f79c. Non-external.
// Every path gets a bundle, permissively.
static void _CFBundleEnsureBundlesExistForImagePaths(OCFArrayRef imagePaths) {
    OCFIndex count = OCFArrayGetCount(imagePaths);
    if (count < 1) {
        return;
    }
    for (OCFIndex i = 0; i != count; i++) {
        _CFBundleEnsureBundleExistsForImagePath((OCFStringRef)OCFArrayGetValueAtIndex(imagePaths, i), true);
    }
}
