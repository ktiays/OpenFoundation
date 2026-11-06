//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFBundleBinary.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/PlugIn/OCFBundleGrok.h"
#include "CoreFoundation/PlugIn/OCFBundleExecutable.h"

#include <crt_externs.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: _CFBundleCurrentArchitecture (0x1c01a1960) through
// _CFBundleDYLDCreateLoadedImagePathVariants (0x1c01a4000) lie in one cluster of the iOS Simulator __text
// section with the other dyld and dlfcn loading functions of CFBundle (not on this task list); the native
// source path is not in the evidence. Their .cold fragments (0x1c030835c-0x1c0308660) and the shared
// OUTLINED_FUNCTION_0_11, _1_7, _4_2, _5_2 and _6_2 (0x1c01a2f50-0x1c01a2fc4, os_log argument packing) are
// folded into their callers. iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified; iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/CoreFoundationBundle.md (pending; the coordinator writes the
// reading).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_4_2 (0x1c01a2fa8), _OUTLINED_FUNCTION_5_2
// (0x1c01a2fb4), _OUTLINED_FUNCTION_6_2 (0x1c01a2fbc).

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call here passes 0x10000
// (OS_UNFAIR_LOCK_DATA_SYNCHRONIZATION).
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

// libdyld and libmacho interfaces the native code imports whose prototypes the iOS Simulator SDK does not
// install or marks deprecated (dyld_priv.h's dyld_image_path_containing_address; mach-o/arch.h's
// NXGetLocalArchInfo). They are supplied, never reimplemented. NXGetLocalArchInfo answers an NXArchInfo whose
// first fields are the name pointer and the 32-bit cputype at offset 8.
extern const char *dyld_image_path_containing_address(const void *address);

typedef struct {
    const char *name;
    cpu_type_t cputype;
} ArchInfoPrefix;

extern const ArchInfoPrefix *NXGetLocalArchInfo(void);

// Native _CFBundleCurrentArchitecture, 0x1c01a1960. Hidden (was private external).
cpu_type_t _CFBundleCurrentArchitecture(void) {
    const ArchInfoPrefix *info = NXGetLocalArchInfo();
    if (info == NULL) {
        return 0;
    }
    return info->cputype;
}

// Native _CFBundleDYLDCopyLoadedImagePathsForHint, 0x1c01a197c. Hidden (was private external).
// A hint of at least 15 UTF-16 units is searched (options 0xD: case-insensitive, backwards, anchored) for
// ".framework", then "framework", then "fw"; the first match with a positive location and length bounds a
// second, prefix search. Every loaded image except the main executable (by normalized path against
// _CFProcessPath, or by its header against _NSGetMachExecuteHeader) contributes its normalized path once
// for each file-name variant that the hint ends with, or that the hint's prefix before the match ends with.
OCFArrayRef _CFBundleDYLDCopyLoadedImagePathsForHint(OCFStringRef hint) {
    uint32_t imageCount = _dyld_image_count();
    OCFMutableArrayRef result = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
    OCFIndex hintLength = OCFStringGetLength(hint);
    OCFRange found = {0, 0};
    const char *processPath = _OCFProcessPath();
    const void *executeHeader = _NSGetMachExecuteHeader();
    OCFIndex prefixLength = 0;
    if (hintLength >= 15) {
        Boolean matched =
            OCFStringFindWithOptions(hint, OF_STR(".framework"), (OCFRange){0, hintLength}, 0xD, &found);
        if (matched) {
            prefixLength = found.location;
        }
        if (!matched || prefixLength < 1 || found.length <= 0) {
            matched =
                OCFStringFindWithOptions(hint, OF_STR("framework"), (OCFRange){0, hintLength}, 0xD, &found);
            if (matched) {
                prefixLength = found.location;
            }
            if (!matched || prefixLength < 1 || found.length <= 0) {
                matched =
                    OCFStringFindWithOptions(hint, OF_STR("fw"), (OCFRange){0, hintLength}, 0xD, &found);
                prefixLength = (matched && found.length > 0 && found.location > 0) ? found.location : 0;
            }
        }
    }
    const char *suffix = __OCFGetDYLDImageSuffix();
    for (uint32_t index = 0; index != imageCount; index++) {
        const char *imagePath = _CFBundleNormalizedPath(_dyld_get_image_name(index));
        if (imagePath == NULL) {
            continue;
        }
        if (processPath != NULL && strcmp(imagePath, processPath) == 0) {
            continue;
        }
        if (executeHeader == (const void *)_dyld_get_image_header(index)) {
            continue;
        }
        const char *lastSlash = strrchr(imagePath, '/');
        if (lastSlash == NULL) {
            continue;
        }
        OCFStringRef fileName =
            OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, lastSlash + 1);
        if (fileName == NULL) {
            continue;
        }
        OCFMutableArrayRef variants = _CFBundleDYLDCreateLoadedImagePathVariants(suffix, fileName);
        OCFIndex variantCount = OCFArrayGetCount(variants);
        for (OCFIndex variantIndex = 0; variantIndex < variantCount; variantIndex++) {
            OCFStringRef variant = OCFArrayGetValueAtIndex(variants, variantIndex);
            if (!OCFStringFindWithOptions(hint, variant, (OCFRange){0, hintLength}, 0xD, NULL)) {
                if (prefixLength < 1 ||
                    !OCFStringFindWithOptions(hint, variant, (OCFRange){0, prefixLength}, 0xD, NULL)) {
                    continue;
                }
            }
            OCFStringRef path =
                OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, imagePath);
            if (path != NULL) {
                OCFArrayAppendValue(result, path);
                OCFRelease(path);
            }
        }
        OCFRelease(variants);
        OCFRelease(fileName);
    }
    return result;
}

// Native _CFBundleDYLDCopyLoadedImagePathsIfChanged, 0x1c01a1c44, with its static _cachedDYLDImageCount
// (__DATA,__data 0x22e6cf4d4, initialized to 0xFFFFFFFF, so the first call always collects). Hidden (was
// private external).
// NULL while the image count equals the cached count. Otherwise every image's normalized path except the
// main executable's is collected and the count is cached. Only image 0's path is also compared in a copy
// with "//" runs and "/./" segments collapsed (the scan stops one or two units before the end, as below); a
// failed strdup skips that comparison. No lock guards the cached count.
OCFArrayRef _CFBundleDYLDCopyLoadedImagePathsIfChanged(void) {
    static uint32_t _cachedDYLDImageCount = 0xFFFFFFFF;
    uint32_t imageCount = _dyld_image_count();
    if (imageCount == _cachedDYLDImageCount) {
        return NULL;
    }
    const char *processPath = _OCFProcessPath();
    const void *executeHeader = _NSGetMachExecuteHeader();
    OCFMutableArrayRef result = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
    for (uint32_t index = 0; index != imageCount; index++) {
        const char *imagePath = _CFBundleNormalizedPath(_dyld_get_image_name(index));
        char *collapsedPath = NULL;
        if (index == 0 && imagePath != NULL) {
            collapsedPath = strdup(imagePath);
            if (collapsedPath != NULL) {
                size_t length = strlen(collapsedPath);
                ssize_t last = (ssize_t)length - 1;
                ssize_t write = 0;
                if (last >= 0) {
                    ssize_t read = 0;
                    ssize_t beforeLast = (ssize_t)length - 2;
                    do {
                        collapsedPath[write] = collapsedPath[read];
                        while (read < last) {
                            const char *cursor = collapsedPath + read;
                            if (cursor[0] != '/') {
                                break;
                            }
                            char next = cursor[1];
                            if (next != '/') {
                                if (next != '.' || read >= beforeLast) {
                                    break;
                                }
                                if (cursor[2] != '/') {
                                    break;
                                }
                            }
                            read += next == '/' ? 1 : 2;
                        }
                        write++;
                        read++;
                    } while (read < (ssize_t)length);
                }
                collapsedPath[write] = 0;
            }
        }
        if (imagePath != NULL) {
            Boolean skip = false;
            if (processPath != NULL) {
                if (strcmp(imagePath, processPath) == 0) {
                    skip = true;
                } else if (collapsedPath != NULL && strcmp(collapsedPath, processPath) == 0) {
                    skip = true;
                }
            }
            if (!skip && executeHeader != (const void *)_dyld_get_image_header(index)) {
                OCFStringRef path =
                    OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, imagePath);
                if (path != NULL) {
                    OCFArrayAppendValue(result, path);
                    OCFRelease(path);
                }
            }
        }
        if (collapsedPath != NULL) {
            free(collapsedPath);
        }
    }
    _cachedDYLDImageCount = imageCount;
    return result;
}

// Native _CFBundleDlfcnCheckLoaded, 0x1c01a1e18, with its .cold.1 fragment (0x1c030835c, the failure log).
// Hidden (was private external).
// With the loading lock asserted and the bundle not loaded, the executable's file system representation
// (1,026 bytes) is opened with mode 0x115 (RTLD_FIRST | RTLD_NOLOAD | RTLD_LOCAL | RTLD_LAZY), so only an
// image already loaded answers a handle. A handle is stored when none is (and logged), and the bundle is
// marked loaded even when a handle was already present; a NULL handle is logged. Answers the loaded flag.
Boolean _CFBundleDlfcnCheckLoaded(OCFBundleRef bundle) {
    char buffer[1026];
    os_unfair_lock_assert_owner(&bundle->_bundleLoadingLock);
    if (bundle->_isLoaded) {
        return bundle->_isLoaded;
    }
    OCFURLRef executableURL = OCFBundleCopyExecutableURL(bundle);
    if (executableURL == NULL) {
        return bundle->_isLoaded;
    }
    if (OCFURLGetFileSystemRepresentation(executableURL, true, (UInt8 *)buffer, 1026)) {
        void *handle = dlopen(buffer, 0x115);
        if (handle != NULL) {
            if (bundle->_handleCookie == NULL) {
                bundle->_handleCookie = handle;
                os_log_t log = _CFBundleLoadingLogger();
                os_log_debug(
                    log,
                    "dlfcn check load bundle %{public}@, dlopen of %{public}s mode 0x%x getting handle %p",
                    bundle,
                    buffer,
                    0x115,
                    bundle->_handleCookie
                );
            }
            bundle->_isLoaded = 1;
        } else {
            os_log_t log = _CFBundleLoadingLogger();
            os_log_debug(
                log,
                "dlfcn check load bundle %{public}@, dlopen of %{public}s mode 0x%x no handle",
                bundle,
                buffer,
                0x115
            );
        }
    }
    OCFRelease(executableURL);
    return bundle->_isLoaded;
}

// Native _CFBundleDlfcnPreflight, 0x1c01a1f7c. Hidden (was a private external).
// A loaded bundle (the flag read under the loading lock) answers true. Otherwise the executable's file system
// representation (1,026 bytes) is preflighted with dlopen_preflight. Without an executable or representation
// the error, when asked for, is _CFBundleCreateError code 4. A failed preflight with an error pointer builds
// the error from the executable architectures and dlerror (a nonempty message becomes a system-default
// string, the debug description): no architectures give 3587; none matching the local cputype (CFNumber
// SInt32 values against NXGetLocalArchInfo, 0 without one) 3585; a main executable whose Objective-C image
// flags have bit 1 (garbage collection supported) with a bundle whose flags lack it 3586; otherwise 3587.
// The error is stored only for a false result.
Boolean _CFBundleDlfcnPreflight(OCFBundleRef bundle, OCFErrorRef *error) {
    UInt8 buffer[1026];
    os_unfair_lock_lock_with_options(&bundle->_bundleLoadingLock, 0x10000);
    Boolean loaded = bundle->_isLoaded;
    os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
    if (loaded) {
        return true;
    }
    Boolean result;
    OCFErrorRef localError;
    OCFURLRef executableURL = OCFBundleCopyExecutableURL(bundle);
    if (executableURL != NULL && OCFURLGetFileSystemRepresentation(executableURL, true, buffer, 1026)) {
        result = dlopen_preflight((const char *)buffer);
        localError = NULL;
        if (error != NULL && !result) {
            OCFArrayRef architectures = OCFBundleCopyExecutableArchitectures(bundle);
            const char *message = dlerror();
            OCFStringRef debugString = NULL;
            if (message != NULL && *message != 0) {
                debugString =
                    OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, message);
            }
            if (architectures == NULL) {
                localError = _CFBundleCreateErrorDebug(OCFGetAllocator(bundle), bundle, 3587, debugString);
            } else {
                OCFIndex count = OCFArrayGetCount(architectures);
                int32_t architecture = 0;
                const ArchInfoPrefix *info = NXGetLocalArchInfo();
                cpu_type_t cputype = info != NULL ? info->cputype : 0;
                Boolean mismatch = true;
                for (OCFIndex index = 0; mismatch && index < count; index++) {
                    OCFNumberRef number = OCFArrayGetValueAtIndex(architectures, index);
                    mismatch = !OCFNumberGetValue(number, kOCFNumberSInt32Type, &architecture) ||
                               architecture != cputype;
                }
                OCFIndex code;
                if (mismatch) {
                    code = 3585;
                } else {
                    uint32_t mainFlags = 0;
                    uint32_t bundleFlags;
                    if (_CFBundleGrokObjCImageInfoFromMainExecutable(NULL, &mainFlags) &&
                        (mainFlags & 2) != 0 &&
                        (bundleFlags = 0, _CFBundleGetObjCImageInfo(bundle, NULL, &bundleFlags)) &&
                        (bundleFlags & 2) == 0) {
                        code = 3586;
                    } else {
                        code = 3587;
                    }
                }
                localError = _CFBundleCreateErrorDebug(OCFGetAllocator(bundle), bundle, code, debugString);
                OCFRelease(architectures);
            }
            if (debugString != NULL) {
                OCFRelease(debugString);
            }
            result = false;
        }
    } else {
        localError = error != NULL ? _OCFBundleCreateError(OCFGetAllocator(bundle), bundle, 4) : NULL;
        result = false;
    }
    if (executableURL != NULL) {
        OCFRelease(executableURL);
    }
    if (error != NULL && !result) {
        *error = localError;
    }
    return result;
}

// Native _CFBundleDlfcnLoadBundle, 0x1c01a21f0, with its .cold.1 (0x1c03083b8), .cold.2 (0x1c03083e4) and
// .cold.3 (0x1c0308440) logs. Hidden (was a private external).
// A loaded bundle answers true. The executable's file system representation is opened with mode 0x109
// (RTLD_FIRST | RTLD_GLOBAL | RTLD_LAZY) when forcing global, else 0x106 (RTLD_FIRST | RTLD_LOCAL |
// RTLD_NOW), and the result logged (debug, loading logger). Under the loading lock a handle equal to the one
// held is logged and released with dlclose outside the lock; the handle is stored and the bundle marked
// loaded. A NULL handle clears the stored one; dlerror is logged (error level, with the path's length and
// "(no additional info)" for no message), and with an error pointer the error is the preflight's or else code
// 3588 with the dlerror text as the debug description. Without an executable or representation it logs
// "Cannot find executable for bundle" and the error is code 4. The error is stored only for a false result
// and released otherwise.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_2_5 (0x1c01a2f84), _OUTLINED_FUNCTION_3_2
// (0x1c01a2f98), _OUTLINED_FUNCTION_7_2 (0x1c01a2fc8), _OUTLINED_FUNCTION_8_1 (0x1c01a2fd8),
// _OUTLINED_FUNCTION_9_0 (0x1c01a2fe8).
Boolean _CFBundleDlfcnLoadBundle(OCFBundleRef bundle, Boolean forceGlobal, OCFErrorRef *error) {
    UInt8 buffer[1026];
    os_unfair_lock_lock_with_options(&bundle->_bundleLoadingLock, 0x10000);
    Boolean loaded = bundle->_isLoaded;
    os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
    if (loaded) {
        return true;
    }
    OCFErrorRef localError = NULL;
    Boolean result;
    OCFURLRef executableURL = OCFBundleCopyExecutableURL(bundle);
    if (executableURL != NULL && OCFURLGetFileSystemRepresentation(executableURL, true, buffer, 1026)) {
        int mode = forceGlobal ? 0x109 : 0x106;
        void *handle = dlopen((const char *)buffer, mode);
        os_log_t log = _CFBundleLoadingLogger();
        os_log_debug(
            log,
            "dlfcn load bundle %{public}@, dlopen of %{public}s mode 0x%x returns handle %p",
            bundle,
            buffer,
            mode,
            handle
        );
        os_unfair_lock_lock_with_options(&bundle->_bundleLoadingLock, 0x10000);
        if (handle != NULL) {
            if (handle == bundle->_handleCookie) {
                log = _CFBundleLoadingLogger();
                os_log_debug(
                    log,
                    "dlfcn load bundle %{public}@ closing existing reference to handle %p",
                    bundle,
                    handle
                );
                os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
                dlclose(handle);
                os_unfair_lock_lock_with_options(&bundle->_bundleLoadingLock, 0x10000);
            }
            bundle->_handleCookie = handle;
            bundle->_isLoaded = 1;
            os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
            result = true;
        } else {
            bundle->_handleCookie = NULL;
            os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
            const char *message = dlerror();
            log = _CFBundleLoadingLogger();
            os_log_error(
                log,
                "Error loading %{public}s (%lu):  %{public}s",
                buffer,
                strlen((const char *)buffer),
                message != NULL ? message : "(no additional info)"
            );
            OCFStringRef debugString =
                message != NULL
                    ? OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, message)
                    : NULL;
            if (error != NULL) {
                _CFBundleDlfcnPreflight(bundle, &localError);
                if (localError == NULL) {
                    localError =
                        _CFBundleCreateErrorDebug(OCFGetAllocator(bundle), bundle, 3588, debugString);
                }
            }
            if (debugString != NULL) {
                OCFRelease(debugString);
            }
            result = false;
        }
    } else {
        os_log_t log = _CFBundleLoadingLogger();
        os_log_error(log, "Cannot find executable for bundle %{public}@", bundle);
        if (error != NULL) {
            localError = _OCFBundleCreateError(OCFGetAllocator(bundle), bundle, 4);
        }
        result = false;
    }
    if (executableURL != NULL) {
        OCFRelease(executableURL);
    }
    if (error != NULL && !result) {
        *error = localError;
    } else if (localError != NULL) {
        OCFRelease(localError);
    }
    return result;
}

// Native _CFBundleDlfcnLoadFramework, 0x1c01a24f0, with its .cold.1 (0x1c0308474), .cold.2 (0x1c03084a0)
// and .cold.3 (0x1c03084fc) logs. Hidden (was a private external).
// _CFBundleDlfcnLoadBundle's sequence for a framework, always with mode 0x109 (RTLD_FIRST | RTLD_GLOBAL |
// RTLD_LAZY) and "framework" in the debug messages; the dlerror failure is logged on the CoreFoundation log
// (_CFOSLog) instead of the loading logger.
Boolean _CFBundleDlfcnLoadFramework(OCFBundleRef bundle, OCFErrorRef *error) {
    UInt8 buffer[1026];
    os_unfair_lock_lock_with_options(&bundle->_bundleLoadingLock, 0x10000);
    Boolean loaded = bundle->_isLoaded;
    os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
    if (loaded) {
        return true;
    }
    OCFErrorRef localError = NULL;
    Boolean result;
    OCFURLRef executableURL = OCFBundleCopyExecutableURL(bundle);
    if (executableURL != NULL && OCFURLGetFileSystemRepresentation(executableURL, true, buffer, 1026)) {
        void *handle = dlopen((const char *)buffer, 0x109);
        os_log_t log = _CFBundleLoadingLogger();
        os_log_debug(
            log,
            "dlfcn load framework %{public}@, dlopen of %{public}s mode 0x%x returns handle %p",
            bundle,
            buffer,
            0x109,
            handle
        );
        os_unfair_lock_lock_with_options(&bundle->_bundleLoadingLock, 0x10000);
        if (handle != NULL) {
            if (handle == bundle->_handleCookie) {
                log = _CFBundleLoadingLogger();
                os_log_debug(
                    log,
                    "dlfcn load framework %{public}@ closing existing reference to handle %p",
                    bundle,
                    handle
                );
                os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
                dlclose(handle);
                os_unfair_lock_lock_with_options(&bundle->_bundleLoadingLock, 0x10000);
            }
            bundle->_handleCookie = handle;
            bundle->_isLoaded = 1;
            os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
            result = true;
        } else {
            bundle->_handleCookie = NULL;
            os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
            const char *message = dlerror();
            log = _CFOSLog();
            os_log_error(
                log,
                "Error loading %{public}s (%lu):  %{public}s",
                buffer,
                strlen((const char *)buffer),
                message != NULL ? message : "(no additional info)"
            );
            OCFStringRef debugString =
                message != NULL
                    ? OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, message)
                    : NULL;
            if (error != NULL) {
                _CFBundleDlfcnPreflight(bundle, &localError);
                if (localError == NULL) {
                    localError =
                        _CFBundleCreateErrorDebug(OCFGetAllocator(bundle), bundle, 3588, debugString);
                }
            }
            if (debugString != NULL) {
                OCFRelease(debugString);
            }
            result = false;
        }
    } else {
        os_log_t log = _CFBundleLoadingLogger();
        os_log_error(log, "Cannot find executable for bundle %{public}@", bundle);
        if (error != NULL) {
            localError = _OCFBundleCreateError(OCFGetAllocator(bundle), bundle, 4);
        }
        result = false;
    }
    if (executableURL != NULL) {
        OCFRelease(executableURL);
    }
    if (error != NULL && !result) {
        *error = localError;
    } else if (localError != NULL) {
        OCFRelease(localError);
    }
    return result;
}

// Native _CFBundleDlfcnUnload, 0x1c01a27e0, with its .cold.1 (0x1c0308530, the debug log) and .cold.2
// (0x1c030855c, the error log with dlerror) fragments. Hidden (was private external).
// With the loading lock asserted, a loaded bundle with a handle is marked unloaded and the lock is dropped
// around dlclose; a failed dlclose restores the handle and the loaded flag.
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_1_7 (0x1c01a2f64).
void _CFBundleDlfcnUnload(OCFBundleRef bundle) {
    os_unfair_lock_assert_owner(&bundle->_bundleLoadingLock);
    void *handle = bundle->_handleCookie;
    if (!bundle->_isLoaded || handle == NULL) {
        return;
    }
    os_log_t log = _CFBundleLoadingLogger();
    os_log_debug(log, "dlfcn unload bundle %{public}@, handle %p", bundle, handle);
    bundle->_handleCookie = NULL;
    bundle->_isLoaded = 0;
    os_unfair_lock_unlock(&bundle->_bundleLoadingLock);
    int status = dlclose(handle);
    os_unfair_lock_lock(&bundle->_bundleLoadingLock);
    if (status != 0) {
        log = _CFBundleLoadingLogger();
        os_log_error(log, "Internal error unloading bundle %{public}@ : %s", bundle, dlerror());
        bundle->_handleCookie = handle;
        bundle->_isLoaded = 1;
    }
}

// Native _CFBundleCopyLoadedImagePathForPointer, 0x1c01a2a00, with its .cold.1 fragment (0x1c0308628, the
// debug log). Hidden (was private external).
OCFStringRef _CFBundleCopyLoadedImagePathForPointer(const void *pointer) {
    const char *imagePath = _CFBundleNormalizedPath(dyld_image_path_containing_address(pointer));
    OCFStringRef result = NULL;
    if (imagePath != NULL) {
        result = OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, imagePath);
    }
    os_log_t log = _CFBundleLoadingLogger();
    os_log_debug(log, "dyld image path for pointer %p is %{public}@", pointer, result);
    return result;
}

// Native _CFBundleDYLDCreateLoadedImagePathVariants, 0x1c01a4000. Hidden (was private external).
// For each nonempty ":"-separated entry of a nonempty suffix, the entry is compared (options 8, non-literal)
// with the units just before the name's extension (the last "."; the whole length without one); a match at
// a positive location adds a copy of the name with that range removed. The name itself is appended last.
OCFMutableArrayRef _CFBundleDYLDCreateLoadedImagePathVariants(const char *suffix, OCFStringRef name) {
    OCFMutableArrayRef result = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
    if (suffix != NULL && *suffix != 0) {
        OCFStringRef suffixString = OCFStringCreateWithCStringNoCopy(
            kOCFAllocatorSystemDefault, suffix, kOCFStringEncodingUTF8, kOCFAllocatorNull
        );
        if (suffixString != NULL) {
            OCFArrayRef suffixes = OCFStringCreateArrayBySeparatingStrings(
                kOCFAllocatorSystemDefault, suffixString, OF_STR(":")
            );
            OCFRelease(suffixString);
            if (suffixes != NULL) {
                OCFIndex nameLength = OCFStringGetLength(name);
                OCFIndex count = OCFArrayGetCount(suffixes);
                for (OCFIndex index = 0; index < count; index++) {
                    OCFStringRef entry = OCFArrayGetValueAtIndex(suffixes, index);
                    OCFIndex entryLength = OCFStringGetLength(entry);
                    if (entryLength == 0) {
                        continue;
                    }
                    OCFRange dot = {0, 0};
                    Boolean hasDot = OCFStringFindWithOptions(
                        name, OF_STR("."), (OCFRange){0, nameLength}, kOCFCompareBackwards, &dot
                    );
                    OCFIndex location = (hasDot ? dot.location : nameLength) - entryLength;
                    if (location < 1) {
                        continue;
                    }
                    if (OCFStringCompareWithOptions(name, entry, (OCFRange){location, entryLength}, 8) !=
                        kOCFCompareEqualTo) {
                        continue;
                    }
                    OCFMutableStringRef variant =
                        OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, name);
                    if (variant != NULL) {
                        OCFStringReplace(variant, (OCFRange){location, entryLength}, OF_STR(""));
                        OCFArrayAppendValue(result, variant);
                        OCFRelease(variant);
                    }
                }
                OCFRelease(suffixes);
            }
        }
    }
    OCFArrayAppendValue(result, name);
    return result;
}
