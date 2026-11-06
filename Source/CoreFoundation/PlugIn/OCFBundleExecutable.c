//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFBundleExecutable.h"
#include "CoreFoundation/Base/OCFFileUtilities.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/PlugIn/OCFBundleResources.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/URL/OCFURLBundleResources.h"
#include "CoreFoundation/URL/OCFURL.h"
#include "CoreFoundation/URL/OCFURLFileUtilities.h"

#include <mach-o/dyld.h>
#include <objc/message.h>
#include <objc/runtime.h>

// Inferred organization: _CFBundleCopyResolvedURLForExecutableURL (0x1c018c870) through
// _CFBundleCopyExecutableURLRaw (0x1c018d3a0) and _binaryLoadable (0x1c018d508, restored by the second
// CFBundle sub-track) are contiguous in the iOS Simulator __text section; the public functions log the source
// path PlugIn.subproj/CFBundle_Executable.c, which names this unit. iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/CoreFoundationBundle.md (pending; the coordinator writes the
// reading).

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call here passes 0x10000
// (OS_UNFAIR_LOCK_DATA_SYNCHRONIZATION).
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

// libobjc's objc_opt_class (objc-internal.h), as the native code imports it.
extern Class objc_opt_class(id object);

// The source path the native _CFBundleLogSourceLocation calls of this unit pass (__cstring).
static const char kSourcePath[] =
    "/Library/Caches/com.apple.xbs/C9620487-17E9-4D33-8BF5-CCA0DA6AFB18/TemporaryDirectory.R3jFjg/Sources/"
    "CoreFoundation_Sim/CoreFoundation/PlugIn.subproj/CFBundle_Executable.c";

static OCFURLRef _CFBundleCopyExecutableURLInDirectory2(
    OCFBundleRef bundle, OCFURLRef url, OCFStringRef executableName, Boolean ignoreCache
);
static OCFURLRef _CFBundleCopyExecutableURLRaw(OCFURLRef urlPath, OCFStringRef exeName);
static Boolean _binaryLoadable(OCFURLRef url);

// Native _CFBundleCopyResolvedURLForExecutableURL, 0x1c018c870. Hidden (was private external).
// The absolute URL's POSIX path (at most 1,026 UTF-16 units) is split at its last path component; when that
// leaves a nonempty directory and a nonempty name (length after deletion >= 1 and + 1 < length), the name is
// resolved against a directory URL of the directory and the absolute form of that URL replaces the
// absolute URL. Any failure answers the absolute URL.
OCFURLRef _CFBundleCopyResolvedURLForExecutableURL(OCFURLRef url) {
    UniChar buffer[1026];
    OCFURLRef absoluteURL = OCFURLCopyAbsoluteURL(url);
    OCFStringRef path = OCFURLCopyFileSystemPath(absoluteURL, kOCFURLPOSIXPathStyle);
    if (path == NULL) {
        return absoluteURL;
    }
    OCFIndex length = OCFStringGetLength(path);
    if (length >= 1026) {
        length = 1026;
    }
    OCFStringGetCharacters(path, (OCFRange){0, length}, buffer);
    OCFIndex directoryLength = _CFLengthAfterDeletingLastPathComponent(buffer, length);
    if (directoryLength < 1 || directoryLength + 1 >= length) {
        OCFRelease(path);
        return absoluteURL;
    }
    OCFStringRef directory =
        OCFStringCreateWithCharacters(kOCFAllocatorSystemDefault, buffer, directoryLength);
    OCFStringRef name = OCFStringCreateWithCharacters(
        kOCFAllocatorSystemDefault, buffer + directoryLength + 1, length + ~directoryLength
    );
    OCFURLRef resolvedURL = NULL;
    if (directory != NULL && name != NULL) {
        OCFURLRef directoryURL = OCFURLCreateWithFileSystemPath(
            kOCFAllocatorSystemDefault, directory, kOCFURLPOSIXPathStyle, true
        );
        if (directoryURL != NULL) {
            OCFURLRef relativeURL = OCFURLCreateWithFileSystemPathRelativeToBase(
                kOCFAllocatorSystemDefault, name, kOCFURLPOSIXPathStyle, false, directoryURL
            );
            if (relativeURL != NULL) {
                resolvedURL = OCFURLCopyAbsoluteURL(relativeURL);
                OCFRelease(relativeURL);
            }
            OCFRelease(directoryURL);
        }
    }
    if (directory != NULL) {
        OCFRelease(directory);
    }
    if (name != NULL) {
        OCFRelease(name);
    }
    OCFRelease(path);
    if (resolvedURL != NULL) {
        OCFRelease(absoluteURL);
        return resolvedURL;
    }
    return absoluteURL;
}

// Native _CFBundleCopyExecutableURLInDirectory2, 0x1c018cb40. Non-external.
// With a bundle, its Info.plist and layout version (+53) are used; without one,
// _CFBundleCopyInfoDictionaryInDirectory supplies a dictionary (owned, released on the way out) and stores
// the version. Only a bundle asked for its own executable (no name, ignoreCache false) consults and fills the
// executable-path cache (+96, under the lock at +104); a cached path that gives no URL falls back to the
// search. Versions 1 ("Support%20Files/Executables/"), 2 ("Contents/"), 12 ("WrappedBundle/Contents/",
// resolved) and 13 ("WrappedBundle/", resolved) search a subdirectory, first its platform-executables
// subdirectory, then the subdirectory itself; any other nonzero version searches the URL with the same two
// steps; every miss ends with a search of the URL itself (version 0 starts there). A bundle searched for its
// own executable that finds nothing gets binary type 7 (+48). The executable name is released only when it
// was derived here. CoreFoundation defect: kept. When the subdirectory URL cannot be created, the native code
// returns NULL without releasing a derived executable name or the dictionary it copied (a leak, not a
// memory-safety defect).
static OCFURLRef _CFBundleCopyExecutableURLInDirectory2(
    OCFBundleRef bundle, OCFURLRef url, OCFStringRef executableName, Boolean ignoreCache
) {
    uint8_t version = 0;
    OCFDictionaryRef infoDict;
    Boolean useCache;
    OCFStringRef name;
    OCFURLRef executableURL;
    if (bundle == NULL) {
        infoDict = _CFBundleCopyInfoDictionaryInDirectory(kOCFAllocatorSystemDefault, url, &version);
        useCache = false;
        name = executableName;
    } else {
        infoDict = OCFBundleGetInfoDictionary(bundle);
        version = bundle->_version;
        if (executableName != NULL || ignoreCache) {
            useCache = false;
            name = executableName;
        } else {
            os_unfair_lock_lock_with_options(&bundle->_lock104, 0x10000);
            OCFStringRef executablePath = bundle->_executablePath;
            if (executablePath != NULL) {
                OCFRetain(executablePath);
                os_unfair_lock_unlock(&bundle->_lock104);
                executableURL = OCFURLCreateWithFileSystemPath(
                    kOCFAllocatorSystemDefault, executablePath, kOCFURLPOSIXPathStyle, false
                );
                OCFRelease(executablePath);
                if (executableURL != NULL) {
                    return executableURL;
                }
            } else {
                os_unfair_lock_unlock(&bundle->_lock104);
            }
            useCache = true;
            name = NULL;
        }
    }
    if (name == NULL) {
        name = _CFBundleCopyExecutableName(bundle, url, infoDict);
        if (name == NULL) {
            if (bundle == NULL && infoDict != NULL) {
                OCFRelease(infoDict);
            }
            return NULL;
        }
    }
    if (version == 0) {
        executableURL = _CFBundleCopyExecutableURLRaw(url, name);
    } else {
        OCFURLRef directoryURL;
        if (version == 1) {
            directoryURL = OCFURLCreateWithString(
                kOCFAllocatorSystemDefault, OF_STR("Support%20Files/Executables/"), url
            );
        } else if (version == 2) {
            directoryURL = OCFURLCreateWithString(kOCFAllocatorSystemDefault, OF_STR("Contents/"), url);
        } else if (version == 12) {
            directoryURL = _CFURLCreateResolvedDirectoryWithString(
                kOCFAllocatorSystemDefault, OF_STR("WrappedBundle/Contents/"), url
            );
        } else if (version == 13) {
            directoryURL = _CFURLCreateResolvedDirectoryWithString(
                kOCFAllocatorSystemDefault, OF_STR("WrappedBundle/"), url
            );
        } else {
            directoryURL = (OCFURLRef)OCFRetain(url);
        }
        if (directoryURL == NULL) {
            return NULL;
        }
        OCFURLRef platformURL = OCFURLCreateWithFileSystemPathRelativeToBase(
            kOCFAllocatorSystemDefault,
            _CFBundleGetPlatformExecutablesSubdirectoryName(),
            kOCFURLPOSIXPathStyle,
            true,
            directoryURL
        );
        executableURL = _CFBundleCopyExecutableURLRaw(platformURL, name);
        OCFRelease(platformURL);
        if (executableURL != NULL) {
            OCFRelease(directoryURL);
        } else {
            executableURL = _CFBundleCopyExecutableURLRaw(directoryURL, name);
            OCFRelease(directoryURL);
            if (executableURL == NULL) {
                executableURL = _CFBundleCopyExecutableURLRaw(url, name);
            }
        }
    }
    if (executableURL != NULL && useCache) {
        OCFURLRef absoluteURL = OCFURLCopyAbsoluteURL(executableURL);
        OCFStringRef executablePath = OCFURLCopyFileSystemPath(absoluteURL, kOCFURLPOSIXPathStyle);
        OCFRelease(absoluteURL);
        os_unfair_lock_lock_with_options(&bundle->_lock104, 0x10000);
        if (bundle->_executablePath == NULL) {
            bundle->_executablePath = OCFRetain(executablePath);
        }
        os_unfair_lock_unlock(&bundle->_lock104);
        OCFRelease(executablePath);
        if (executableName != NULL) {
            if (bundle == NULL && infoDict != NULL) {
                OCFRelease(infoDict);
            }
            return executableURL;
        }
    } else if (bundle == NULL || executableName != NULL || executableURL != NULL) {
        if (executableName != NULL) {
            if (bundle == NULL && infoDict != NULL) {
                OCFRelease(infoDict);
            }
            return executableURL;
        }
    } else {
        bundle->_binaryType = 7;
    }
    OCFRelease(name);
    if (bundle == NULL && infoDict != NULL) {
        OCFRelease(infoDict);
    }
    return executableURL;
}

// Native CFBundleCopyExecutableURL, 0x1c018ce64. External.
// Per-thread slot 18 marks a nested CFBundle call: the outermost call sets it to 1, logs its source location
// (line 379) and clears it on the way out. Under the CoreFoundation/CFSwiftBundle feature the bundle is sent
// executableURL and the answer retained; otherwise the executable is searched in the bundle URL.
OCFURLRef OCFBundleCopyExecutableURL(OCFBundleRef bundle) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 379, OF_NAME_STRING(OCFBundleCopyExecutableURL));
    OCFURLRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((OCFURLRef (*)(id, SEL))objc_msgSend)((id)bundle, sel_getUid("executableURL"));
        if (result != NULL) {
            OCFRetain(result);
        }
    } else {
        result = _CFBundleCopyExecutableURLInDirectory2(bundle, bundle->_url, NULL, false);
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native CFBundleCopyAuxiliaryExecutableURL, 0x1c018cf68. External.
// The slot-18 prelude of CFBundleCopyExecutableURL (line 390); the Swift path sends
// URLForAuxiliaryExecutable:, otherwise the named executable is searched with the cache bypassed.
OCFURLRef OCFBundleCopyAuxiliaryExecutableURL(OCFBundleRef bundle, OCFStringRef executableName) {
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        outermost, kSourcePath, 390, OF_NAME_STRING(OCFBundleCopyAuxiliaryExecutableURL)
    );
    OCFURLRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((OCFURLRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
            (id)bundle, sel_getUid("URLForAuxiliaryExecutable:"), executableName
        );
        if (result != NULL) {
            OCFRetain(result);
        }
    } else {
        result = _CFBundleCopyExecutableURLInDirectory2(bundle, bundle->_url, executableName, true);
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native _CFBundleCopyBundleURLForExecutableURL, 0x1c018d074. External.
// The slot-18 prelude (line 402); the Swift path sends bundleURLForExecutableAtURL: to the class of
// _NSSwiftBundle and retains the answer. Otherwise the resolved executable path (at most 1,026 UTF-16 units)
// loses its last component; when the enclosing directory's name equals the platform-executables
// subdirectory, that component is removed too, and then an "Executables" component above it; one more
// component is removed after the platform match. The remaining prefix (when nonempty) becomes a directory
// URL; an empty remainder answers NULL.
OCFURLRef _OCFBundleCopyBundleURLForExecutableURL(OCFURLRef url) {
    UniChar buffer[1026];
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        outermost, kSourcePath, 402, OF_NAME_STRING(_OCFBundleCopyBundleURLForExecutableURL)
    );
    OCFURLRef result = NULL;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        Class swiftBundleClass = objc_opt_class((id)objc_lookUpClass(OF_NAME_STRING(_ONSSwiftBundle)));
        result = ((OCFURLRef (*)(id, SEL, OCFURLRef))objc_msgSend)(
            (id)swiftBundleClass, sel_getUid("bundleURLForExecutableAtURL:"), url
        );
        if (result != NULL) {
            OCFRetain(result);
        }
    } else {
        OCFURLRef resolvedURL = _CFBundleCopyResolvedURLForExecutableURL(url);
        OCFStringRef path = OCFURLCopyFileSystemPath(resolvedURL, kOCFURLPOSIXPathStyle);
        if (path != NULL) {
            OCFIndex length = OCFStringGetLength(path);
            if (length >= 1026) {
                length = 1026;
            }
            OCFStringGetCharacters(path, (OCFRange){0, length}, buffer);
            length = _CFLengthAfterDeletingLastPathComponent(buffer, length);
            if (length >= 1) {
                OCFIndex start = _CFStartOfLastPathComponent(buffer, length);
                Boolean haveBundlePath = true;
                if (start >= 1 && length > start) {
                    OCFStringRef component = OCFStringCreateWithCharacters(
                        kOCFAllocatorSystemDefault, buffer + start, length - start
                    );
                    if (!OCFEqual(component, _CFBundleGetPlatformExecutablesSubdirectoryName())) {
                        OCFRelease(component);
                    } else {
                        length = _CFLengthAfterDeletingLastPathComponent(buffer, length);
                        Boolean removeNext = true;
                        if (length < 1) {
                            haveBundlePath = false;
                            removeNext = false;
                        } else {
                            start = _CFStartOfLastPathComponent(buffer, length);
                            if (start >= 1 && length > start) {
                                OCFStringRef executables = OCFStringCreateWithCharacters(
                                    kOCFAllocatorSystemDefault, buffer + start, length - start
                                );
                                if (OCFEqual(executables, OF_STR("Executables"))) {
                                    length = _CFLengthAfterDeletingLastPathComponent(buffer, length);
                                    OCFRelease(executables);
                                    if (length < 1) {
                                        haveBundlePath = false;
                                        removeNext = false;
                                    }
                                } else {
                                    OCFRelease(executables);
                                }
                            }
                        }
                        if (removeNext) {
                            length = _CFLengthAfterDeletingLastPathComponent(buffer, length);
                            if (length < 1) {
                                haveBundlePath = false;
                            }
                        }
                        OCFRelease(component);
                    }
                }
                if (haveBundlePath) {
                    OCFStringRef bundlePath = OCFStringCreateWithCharactersNoCopy(
                        kOCFAllocatorSystemDefault, buffer, length, kOCFAllocatorNull
                    );
                    result = OCFURLCreateWithFileSystemPath(
                        kOCFAllocatorSystemDefault, bundlePath, kOCFURLPOSIXPathStyle, true
                    );
                    OCFRelease(bundlePath);
                }
            }
            OCFRelease(path);
        }
        OCFRelease(resolvedURL);
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

// Native _CFBundleCopyExecutableURLRaw, 0x1c018d3a0. Non-external.
// NULL for a NULL directory. With a DYLD_IMAGE_SUFFIX, a name ending in ".dylib" is tried as
// "<name><suffix>.dylib" and any other as "<name><suffix>"; the native code formats the whole name (the
// substring without ".dylib" it creates is released unused), so "x.dylib" becomes "x.dylib<suffix>.dylib".
// A URL that _binaryLoadable rejects is dropped; without a loadable suffixed URL the plain name is tried the
// same way.
static OCFURLRef _CFBundleCopyExecutableURLRaw(OCFURLRef urlPath, OCFStringRef exeName) {
    if (urlPath == NULL) {
        return NULL;
    }
    const char *suffix = __OCFGetDYLDImageSuffix();
    OCFURLRef executableURL = NULL;
    if (suffix != NULL) {
        OCFStringRef suffixString =
            OCFStringCreateWithCString(kOCFAllocatorSystemDefault, suffix, kOCFStringEncodingUTF8);
        OCFStringRef suffixedName;
        if (OCFStringHasSuffix(exeName, OF_STR(".dylib"))) {
            OCFStringRef baseName = OCFStringCreateWithSubstring(
                kOCFAllocatorSystemDefault, exeName, (OCFRange){0, OCFStringGetLength(exeName) - 6}
            );
            suffixedName = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault, NULL, OF_STR("%@%@.dylib"), exeName, suffixString
            );
            OCFRelease(baseName);
        } else {
            suffixedName = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault, NULL, OF_STR("%@%@"), exeName, suffixString
            );
        }
        executableURL = OCFURLCreateWithFileSystemPathRelativeToBase(
            kOCFAllocatorSystemDefault, suffixedName, kOCFURLPOSIXPathStyle, false, urlPath
        );
        if (executableURL != NULL && !_binaryLoadable(executableURL)) {
            OCFRelease(executableURL);
            executableURL = NULL;
        }
        OCFRelease(suffixedName);
        OCFRelease(suffixString);
        if (executableURL != NULL) {
            return executableURL;
        }
    }
    executableURL = OCFURLCreateWithFileSystemPathRelativeToBase(
        kOCFAllocatorSystemDefault, exeName, kOCFURLPOSIXPathStyle, false, urlPath
    );
    if (executableURL != NULL && !_binaryLoadable(executableURL)) {
        OCFRelease(executableURL);
        return NULL;
    }
    return executableURL;
}

// Native _binaryLoadable, 0x1c018d508. Non-external. True when _CFURLExists answers true (bit 0); otherwise
// the file-system representation (resolved against the base, 1,024 bytes) is looked up with
// _dyld_shared_cache_contains_path; false when the representation fails.
static Boolean _binaryLoadable(OCFURLRef url) {
    UInt8 buffer[1024];
    if (_CFURLExists(url)) {
        return true;
    }
    if (!OCFURLGetFileSystemRepresentation(url, true, buffer, 1024)) {
        return false;
    }
    return _dyld_shared_cache_contains_path((const char *)buffer);
}
