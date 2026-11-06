//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFBundleResources.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/String/OCFString.h"

#include <fcntl.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <os/log.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Inferred organization: CFBundleCopyResourceURL (0x1c016f9b8) through _CFBundleAddValueForType (0x1c0173114)
// and their OUTLINED_FUNCTION fragments (0x1c0173240-0x1c0173268) are contiguous in the iOS Simulator __text
// section; the source path that the logged functions pass to _CFBundleLogSourceLocation is
// PlugIn.subproj/CFBundle_Resources.c. _CFIsResourceAtURL (0x1c016f8f4), _copyResourceURLsFromBundle
// (0x1c017199c), _copyQueryTable (0x1c01723b0) and _createQueryTableAtPath (0x1c017282c) lie in the same
// range and were restored by the second CFBundle sub-track. The .cold.1-.cold.4 fragments of
// _CFBundleGetBundleVersionForURL (0x1c03067e8-0x1c0306894; each an os_log_error through
// _OUTLINED_FUNCTION_1_1, _OUTLINED_FUNCTION_0_2 and _OUTLINED_FUNCTION_2_0 with one
// %{public}@ argument, the bundle URL) are folded into it. Unless a definition says otherwise: iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/CoreFoundationBundle.md (pending; the coordinator writes the
// reading).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_1_1 (0x1c0173250), _OUTLINED_FUNCTION_2_0
// (0x1c0173264).

// dyld's SPI (libdyld, re-exported by libSystem); _CFBundleCopyFindResources passes 0x07E00901FFFFFFFF in X0.
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);
static const OCFBuildVersion kSubPathValidationVersion = {0xFFFFFFFF, 0x07E00901};

// The source path literal (__cstring) that every logged function of this unit passes to
// _CFBundleLogSourceLocation.
#define kResourcesSourcePath                                                                                 \
    "/Library/Caches/com.apple.xbs/C9620487-17E9-4D33-8BF5-CCA0DA6AFB18/TemporaryDirectory.R3jFjg/Sources/"  \
    "CoreFoundation_Sim/CoreFoundation/PlugIn.subproj/CFBundle_Resources.c"

// The per-thread slot 0x12 marks a CFBundle entry point in progress: only the outermost entry logs its source
// location, and it clears the slot on return.
#define kBundleEntryTSDSlot 0x12

// Native _CFBundleCopyFindResources.extensionSep, 0x1c03731c0 (__TEXT,__const, 2 bytes: '.' then 0). The
// UniChar that precedes an extension that does not start with '.'. kExtensionSeparator is a local name.
static const UniChar kExtensionSeparator = '.';

static OCFTypeRef _copyResourceURLsFromBundle(
    OCFBundleRef bundle,
    OCFURLRef bundleURL,
    OCFArrayRef languages,
    OCFStringRef resourcesDirectory,
    OCFStringRef subDirectory,
    OCFStringRef searchName,
    OCFStringRef localization,
    Boolean returnArray,
    Boolean localized,
    uint8_t version,
    ResourcePredicate predicate
);
static OCFDictionaryRef _copyQueryTable(
    OCFBundleRef bundle,
    OCFURLRef bundleURL,
    OCFArrayRef languages,
    OCFStringRef resourcesDirectory,
    OCFStringRef subDirectory
);
static void _CFBundleFindResourcesWithPredicate(
    OCFMutableArrayRef interResult, OCFDictionaryRef queryTable, ResourcePredicate predicate, Boolean *stop
);
static OCFURLRef _CFBundleCreateRelativeURLFromBaseAndPath(
    OCFStringRef path, OCFURLRef base, UniChar slash, OCFStringRef slashStr
);
static OCFDictionaryRef _createQueryTableAtPath(
    OCFStringRef inPath,
    OCFArrayRef languages,
    OCFStringRef resourcesDirectory,
    OCFStringRef subdirectory,
    Boolean productAndPlatformFlag
);
static void _CFBundleReadDirectory(
    OCFStringRef pathOfDir,
    OCFStringRef subdirectoryPrefix,
    Boolean productAndPlatformFlag,
    OCFMutableArrayRef allFiles,
    Boolean hasFileAdded,
    OCFMutableDictionaryRef queryTable,
    OCFMutableDictionaryRef typeDir,
    OCFMutableDictionaryRef addedTypes,
    Boolean firstLproj,
    OCFStringRef lprojName
);

// Native _CFIsResourceAtURL, 0x1c016f8f4. Hidden (was private external). _CFGetFileProperties (system default
// allocator) with only the exists byte and the 32-bit mode requested; a nonzero result answers false without
// touching *isDirectory. *isDirectory (when given) is set to exists and a directory mode (mode & 0xF000 ==
// 0x4000); the answer is exists with any of the 0x124 read bits set.
Boolean _CFIsResourceAtURL(OCFURLRef url, Boolean *isDirectory) {
    Boolean exists = false;
    SInt32 mode = 0;
    if (_CFGetFileProperties(kOCFAllocatorSystemDefault, url, &exists, &mode, NULL, NULL, NULL, NULL) != 0) {
        return false;
    }
    if (isDirectory != NULL) {
        *isDirectory = exists && (mode & 0xF000) == 0x4000;
    }
    return (mode & 0x124) != 0 && exists;
}

// Native CFBundleCopyResourceURL, 0x1c016f9b8. External. NULL for a NULL bundle before the per-thread slot is
// touched; source line 113 (0x71). With the CFSwiftBundle feature: a name or type of nonzero length asks the
// bundle for URLForResource:withExtension:subdirectory: (retained); otherwise the first element of
// URLsForResourcesWithExtension:subdirectory: with a nil extension (retained), or NULL for a nil or empty
// array. Without it, _CFBundleCopyFindResources with a NULL URL and localization, no array, the localized
// byte 0 and no predicate. The native call leaves X2 unset; the callee never reads it, and NULL is passed
// here.
OCFURLRef OCFBundleCopyResourceURL(
    OCFBundleRef bundle, OCFStringRef resourceName, OCFStringRef resourceType, OCFStringRef subDirName
) {
    if (bundle == NULL) {
        return NULL;
    }
    void *previous = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean logLocation;
    if (previous == (void *)1) {
        logLocation = false;
    } else {
        logLocation = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        logLocation, kResourcesSourcePath, 113, OF_NAME_STRING(OCFBundleCopyResourceURL)
    );
    OCFURLRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        if ((resourceName != NULL && OCFStringGetLength(resourceName) != 0) ||
            (resourceType != NULL && OCFStringGetLength(resourceType) != 0)) {
            result = ((OCFURLRef (*)(id, SEL, OCFStringRef, OCFStringRef, OCFStringRef))objc_msgSend)(
                (id)bundle,
                sel_getUid("URLForResource:withExtension:subdirectory:"),
                resourceName,
                resourceType,
                subDirName
            );
            if (result != NULL) {
                OCFRetain(result);
            }
        } else {
            OCFArrayRef results = ((OCFArrayRef (*)(id, SEL, OCFStringRef, OCFStringRef))objc_msgSend)(
                (id)bundle, sel_getUid("URLsForResourcesWithExtension:subdirectory:"), NULL, subDirName
            );
            result = NULL;
            if (results != NULL && OCFArrayGetCount(results) >= 1) {
                result = (OCFURLRef)OCFRetain(OCFArrayGetValueAtIndex(results, 0));
            }
        }
    } else {
        result = (OCFURLRef)_OCFBundleCopyFindResources(
            bundle, NULL, NULL, resourceName, resourceType, subDirName, NULL, false, false, NULL
        );
    }
    if (previous != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native __CFBundleCopyFindResources, 0x1c016fb74. External (exported as _OCFBundleCopyFindResources). Source
// line 1225 (0x4C9). The CFSwiftBundle feature answers NULL. A non-NULL bundle must have type 31 (otherwise
// _CFAssertMismatchedTypeID(31, type), then BRK). When subPath is set and
// dyld_program_sdk_at_least(0x07E00901 FFFFFFFF) holds, subPath is split at "/" and walked: an empty first
// component and "." components are skipped,
// ".." lowers and anything else raises the depth; a negative depth stops the walk and answers NULL, or an
// empty mutable array when returnArray is set. The resource name is split into its last path component
// (normalized through its file-system representation, 1,026 bytes) and the directory part in front of it (a
// name ending in
// "/" keeps the directory part as its last component); the search name is that component with ".type"
// appended,
// "_CFBT_.type" without a component, or "_CFBAF_" without either; the subdirectory joins subPath and the
// directory part. The layout version (the bundle's, or the one _CFBundleCopyLanguageSearchListInDirectory
// reports for the URL) picks the resources directory. An empty result for versions 0 and 2 retries once from
// the bundle root (version 2 only for "/Library/Spotlight"), dropping a leading "Resources/" (version 0) or
// "Contents/Resources/" (version 2) from the subdirectory. The bundles in the bundle's
// _additionalResourceBundles table (locked by _lock192) are searched next, when an array is requested or
// nothing was found. Every native call leaves X2 unset; the function never reads it. The debug log is the
// native "Resource lookup" message with the bundle (or "CFURL" and the URL), the request and the result.
OCFTypeRef _OCFBundleCopyFindResources(
    OCFBundleRef bundle,
    OCFURLRef bundleURL,
    OCFArrayRef unused,
    OCFStringRef resourceName,
    OCFStringRef resourceType,
    OCFStringRef subPath,
    OCFStringRef localization,
    Boolean returnArray,
    Boolean localized,
    ResourcePredicate predicate
) {
    (void)unused;
    void *previous = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean logLocation;
    if (previous == (void *)1) {
        logLocation = false;
    } else {
        logLocation = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        logLocation, kResourcesSourcePath, 1225, OF_NAME_STRING(_OCFBundleCopyFindResources)
    );
    OCFTypeRef result = NULL;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        goto done;
    }
    if (bundle != NULL) {
        OCFTypeID typeID = _CFGetNonObjCTypeID(bundle);
        if (typeID != 31) {
            _CFAssertMismatchedTypeID(31, typeID);
        }
    }
    Boolean skipValidation = !dyld_program_sdk_at_least(kSubPathValidationVersion);
    if (subPath == NULL) {
        skipValidation = true;
    }
    if (!skipValidation) {
        OCFArrayRef components =
            OCFStringCreateArrayBySeparatingStrings(kOCFAllocatorSystemDefault, subPath, OF_STR("/"));
        OCFIndex count = OCFArrayGetCount(components);
        Boolean valid;
        if (count < 1) {
            valid = true;
        } else {
            int32_t depth = 0;
            for (OCFIndex index = 0; index != count; index++) {
                OCFStringRef component = (OCFStringRef)OCFArrayGetValueAtIndex(components, index);
                if (index == 0 && OCFStringCompare(component, OF_STR(""), 0) == kOCFCompareEqualTo) {
                    continue;
                }
                if (OCFStringCompare(component, OF_STR("."), 0) == kOCFCompareEqualTo) {
                    continue;
                }
                if (OCFStringCompare(component, OF_STR(".."), 0) == kOCFCompareEqualTo) {
                    depth--;
                } else {
                    depth++;
                }
                if (depth < 0) {
                    break;
                }
            }
            valid = depth >= 0;
        }
        OCFRelease(components);
        if (!valid) {
            if (returnArray) {
                result = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
            }
            goto done;
        }
    }

    OCFStringRef directoryPart = NULL;
    OCFStringRef lastComponent = NULL;
    Boolean lastComponentNotOwned = true;
    if (resourceName != NULL) {
        OCFIndex slashIndex = -1;
        OCFStringRef component =
            _CFCreateLastPathComponent(kOCFAllocatorSystemDefault, resourceName, &slashIndex);
        if (slashIndex >= 1) {
            directoryPart = OCFStringCreateWithSubstring(
                kOCFAllocatorSystemDefault, resourceName, (OCFRange){0, slashIndex}
            );
            if (OCFStringGetLength(component) == 0 && slashIndex == OCFStringGetLength(resourceName) - 1) {
                OCFRelease(component);
                component = OCFStringCreateCopy(kOCFAllocatorSystemDefault, directoryPart);
            }
        }
        char buffer[1026];
        lastComponent = component;
        if (OCFStringGetFileSystemRepresentation(component, buffer, 1026)) {
            OCFRelease(component);
            lastComponent = OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, buffer);
        }
    }
    OCFTypeRef searchName = NULL;
    if (lastComponent != NULL) {
        lastComponentNotOwned = false;
        OCFIndex length = OCFStringGetLength(lastComponent);
        if (resourceType != NULL && length >= 1 && OCFStringGetLength(resourceType) >= 1) {
            OCFMutableStringRef name =
                OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, lastComponent);
            if (OCFStringGetCharacterAtIndex(resourceType, 0) != '.') {
                OCFStringAppendCharacters(name, &kExtensionSeparator, 1);
            }
            OCFStringAppend(name, resourceType);
            searchName = name;
        } else if (OCFStringGetLength(lastComponent) >= 1) {
            searchName = OCFRetain(lastComponent);
        }
    }
    if (searchName == NULL) {
        if (resourceType != NULL && OCFStringGetLength(resourceType) >= 1) {
            OCFMutableStringRef name =
                OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, OF_STR("_CFBT_"));
            if (OCFStringGetCharacterAtIndex(resourceType, 0) != '.') {
                OCFStringAppendCharacters(name, &kExtensionSeparator, 1);
            }
            OCFStringAppend(name, resourceType);
            searchName = name;
        } else {
            searchName = OCFRetain(OF_STR("_CFBAF_"));
        }
    }

    Boolean hasSubPath = subPath != NULL && OCFStringGetLength(subPath) != 0;
    Boolean hasDirectoryPart = directoryPart != NULL && OCFStringGetLength(directoryPart) != 0;
    OCFStringRef subDirectory;
    if (hasSubPath && hasDirectoryPart) {
        OCFMutableStringRef joined = OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, subPath);
        _CFAppendPathComponent2(joined, directoryPart);
        subDirectory = joined;
    } else if (hasSubPath) {
        subDirectory = (OCFStringRef)OCFRetain(subPath);
    } else if (hasDirectoryPart) {
        subDirectory = (OCFStringRef)OCFRetain(directoryPart);
    } else {
        subDirectory = NULL;
    }

    uint8_t version = bundle != NULL ? _CFBundleLayoutVersion(bundle) : 0;
    OCFArrayRef languages;
    if (bundleURL != NULL) {
        languages = _CFBundleCopyLanguageSearchListInDirectory(bundleURL, &version);
    } else {
        languages = NULL;
    }
    OCFStringRef resourcesDirectory;
    switch (version) {
        case 0:
            resourcesDirectory = OF_STR("Resources");
            break;
        case 1:
            resourcesDirectory = OF_STR("Support Files/Resources");
            break;
        case 2:
            resourcesDirectory = OF_STR("Contents/Resources");
            break;
        case 12:
            resourcesDirectory = OF_STR("WrappedBundle/Contents/Resources");
            break;
        case 13:
            resourcesDirectory = OF_STR("WrappedBundle");
            break;
        default:
            resourcesDirectory = OF_STR("");
            break;
    }
    result = _copyResourceURLsFromBundle(
        bundle,
        bundleURL,
        languages,
        resourcesDirectory,
        subDirectory,
        (OCFStringRef)searchName,
        localization,
        returnArray,
        localized,
        version,
        predicate
    );
    Boolean found = result != NULL && (OCFGetTypeID(result) != OCFArrayGetTypeID() ||
                                       OCFArrayGetCount((OCFArrayRef)result) != 0);
    if (!found && (version & 0xFD) == 0) {
        OCFStringRef basePath;
        if (bundle != NULL) {
            basePath = bundle->_bundleBasePath;
            OCFRetain(basePath);
        } else {
            OCFURLRef absolute = OCFURLCopyAbsoluteURL(bundleURL);
            basePath = OCFURLCopyFileSystemPath(absolute, kOCFURLPOSIXPathStyle);
            OCFRelease(absolute);
        }
        if (version == 0 || OCFEqual(OF_STR("/Library/Spotlight"), basePath)) {
            if (result != NULL) {
                OCFRelease(result);
            }
            if ((version == 0 && subDirectory != NULL && OCFEqual(subDirectory, OF_STR("Resources"))) ||
                (version == 2 && subDirectory != NULL &&
                 OCFEqual(subDirectory, OF_STR("Contents/Resources")))) {
                OCFRelease(subDirectory);
                subDirectory = OF_STR("");
            } else if (
                version == 0 && subDirectory != NULL && OCFStringGetLength(subDirectory) >= 11 &&
                OCFStringHasPrefix(subDirectory, OF_STR("Resources/"))
            ) {
                OCFStringRef trimmed = OCFStringCreateWithSubstring(
                    kOCFAllocatorSystemDefault,
                    subDirectory,
                    (OCFRange){10, OCFStringGetLength(subDirectory) - 10}
                );
                OCFRelease(subDirectory);
                subDirectory = trimmed;
            } else if (
                version == 2 && subDirectory != NULL && OCFStringGetLength(subDirectory) >= 20 &&
                OCFStringHasPrefix(subDirectory, OF_STR("Contents/Resources/"))
            ) {
                OCFStringRef trimmed = OCFStringCreateWithSubstring(
                    kOCFAllocatorSystemDefault,
                    subDirectory,
                    (OCFRange){19, OCFStringGetLength(subDirectory) - 19}
                );
                OCFRelease(subDirectory);
                subDirectory = trimmed;
            } else {
                resourcesDirectory = OF_STR("");
            }
            result = _copyResourceURLsFromBundle(
                bundle,
                bundleURL,
                languages,
                resourcesDirectory,
                subDirectory,
                (OCFStringRef)searchName,
                localization,
                returnArray,
                localized,
                version,
                predicate
            );
        }
        OCFRelease(basePath);
    }

    if (bundle != NULL) {
        os_unfair_lock_lock(&bundle->_lock192);
        OCFDictionaryRef additional = (OCFDictionaryRef)bundle->_additionalResourceBundles;
        if (additional != NULL && (returnArray || result == NULL)) {
            OCFIndex count = OCFDictionaryGetCount(additional);
            const void **keys = malloc_type_malloc(8 * count, 0xC0040B8AA526D);
            const void **values = malloc_type_malloc(8 * count, 0xC0040B8AA526D);
            OCFDictionaryGetKeysAndValues((OCFDictionaryRef)bundle->_additionalResourceBundles, keys, values);
            OCFMutableArrayRef accumulated = NULL;
            OCFTypeRef single = result;
            if (count >= 1) {
                OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
                const void **cursor = values;
                single = NULL;
                OCFIndex remaining = count;
                Boolean stopped = false;
                do {
                    OCFTypeRef other = _OCFBundleCopyFindResources(
                        (OCFBundleRef)*cursor++,
                        NULL,
                        NULL,
                        resourceName,
                        resourceType,
                        subPath,
                        localization,
                        returnArray,
                        localized,
                        predicate
                    );
                    if (other != NULL) {
                        if (!returnArray) {
                            single = other;
                            stopped = true;
                            break;
                        }
                        if (accumulated == NULL) {
                            accumulated = OCFArrayCreateMutableCopy(allocator, 0, (OCFArrayRef)result);
                        }
                        OCFArrayAppendArray(
                            accumulated,
                            (OCFArrayRef)other,
                            (OCFRange){0, OCFArrayGetCount((OCFArrayRef)other)}
                        );
                        OCFRelease(other);
                    }
                } while (--remaining != 0);
                if (!stopped) {
                    single = result;
                }
                if (accumulated != NULL) {
                    if (single != NULL) {
                        OCFRelease(single);
                    }
                    single = accumulated;
                }
            }
            free(keys);
            free(values);
            result = single;
        }
        os_unfair_lock_unlock(&bundle->_lock192);
    }
    if (!lastComponentNotOwned) {
        OCFRelease(lastComponent);
    }
    if (subDirectory != NULL) {
        OCFRelease(subDirectory);
    }
    if (directoryPart != NULL) {
        OCFRelease(directoryPart);
    }
    if (languages != NULL) {
        OCFRelease(languages);
    }
    OCFRelease(searchName);
    if (os_log_type_enabled(_CFBundleResourceLogger(), OS_LOG_TYPE_DEBUG)) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFMutableStringRef target = OCFStringCreateMutable(allocator, 0);
        OCFMutableStringRef request = OCFStringCreateMutable(allocator, 0);
        OCFMutableStringRef outcome = OCFStringCreateMutable(allocator, 0);
        if (bundle != NULL || bundleURL != NULL) {
            OCFStringAppendFormat(
                target,
                NULL,
                bundle != NULL ? OF_STR("%@") : OF_STR("CFURL %@"),
                bundle != NULL ? (OCFTypeRef)bundle : (OCFTypeRef)bundleURL
            );
        }
        if (resourceName != NULL) {
            OCFStringAppend(request, resourceName);
        }
        if (resourceType != NULL) {
            OCFStringAppendFormat(request, NULL, OF_STR(" type: %@"), resourceType);
        }
        if (subPath != NULL) {
            OCFStringAppendFormat(request, NULL, OF_STR(" path: %@"), subPath);
        }
        if (localization != NULL) {
            OCFStringAppendFormat(request, NULL, OF_STR(" localization: %@"), localization);
        }
        if (result == NULL) {
            OCFStringAppendFormat(outcome, NULL, OF_STR("None"));
        } else if (OCFGetTypeID(result) == OCFArrayGetTypeID()) {
            OCFIndex count = OCFArrayGetCount((OCFArrayRef)result);
            for (OCFIndex index = 0; index < count; index++) {
                OCFStringAppendFormat(
                    outcome, NULL, OF_STR("%@, "), OCFArrayGetValueAtIndex((OCFArrayRef)result, index)
                );
                count = OCFArrayGetCount((OCFArrayRef)result);
            }
        } else {
            OCFStringAppendFormat(outcome, NULL, OF_STR("%@"), result);
        }
        os_log_debug(
            _CFBundleResourceLogger(),
            "Resource lookup at %{private}@\n\tRequest       : %{public}@\n\tResult        : %{public}@\n",
            target,
            request,
            outcome
        );
        OCFRelease(target);
        OCFRelease(request);
        OCFRelease(outcome);
    }
done:
    if (previous != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native CFBundleCopyResourceURLsOfType, 0x1c01707e4. External.
// A NULL bundle answers an empty mutable array (system default, CFType callbacks) before the slot-18 prelude
// (line 137). With CFSwiftBundle the answer of -URLsForResourcesWithExtension:subdirectory: is retained;
// otherwise _CFBundleCopyFindResources is asked for an array of every resource of the type in the
// subdirectory (no name, no localization, not localized, no predicate). The native call leaves the third
// (unused) argument register unset; NULL is passed here.
OCFArrayRef
OCFBundleCopyResourceURLsOfType(OCFBundleRef bundle, OCFStringRef resourceType, OCFStringRef subDirName) {
    if (bundle == NULL) {
        return OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
    }
    void *previous = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean logLocation;
    if (previous == (void *)1) {
        logLocation = false;
    } else {
        logLocation = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        logLocation, kResourcesSourcePath, 137, OF_NAME_STRING(OCFBundleCopyResourceURLsOfType)
    );
    OCFArrayRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((OCFArrayRef (*)(id, SEL, OCFStringRef, OCFStringRef))objc_msgSend)(
            (id)bundle, sel_getUid("URLsForResourcesWithExtension:subdirectory:"), resourceType, subDirName
        );
        if (result != NULL) {
            OCFRetain(result);
        }
    } else {
        result = (OCFArrayRef)_OCFBundleCopyFindResources(
            bundle, NULL, NULL, NULL, resourceType, subDirName, NULL, true, false, NULL
        );
    }
    if (previous != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native CFBundleCopyResourceURLForLocalization, 0x1c0170958. External. NULL for a NULL bundle; source line
// 154 (0x9A). With the CFSwiftBundle feature, URLForResource:withExtension:subdirectory:localization:
// (retained); otherwise _CFBundleCopyFindResources with a NULL URL, the localization, no array, the localized
// byte 1 and no predicate (X2 unset natively, NULL here; never read).
OCFURLRef OCFBundleCopyResourceURLForLocalization(
    OCFBundleRef bundle,
    OCFStringRef resourceName,
    OCFStringRef resourceType,
    OCFStringRef subDirName,
    OCFStringRef localizationName
) {
    if (bundle == NULL) {
        return NULL;
    }
    void *previous = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean logLocation;
    if (previous == (void *)1) {
        logLocation = false;
    } else {
        logLocation = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        logLocation, kResourcesSourcePath, 154, OF_NAME_STRING(OCFBundleCopyResourceURLForLocalization)
    );
    OCFURLRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result =
            ((OCFURLRef (*)(id, SEL, OCFStringRef, OCFStringRef, OCFStringRef, OCFStringRef))objc_msgSend)(
                (id)bundle,
                sel_getUid("URLForResource:withExtension:subdirectory:localization:"),
                resourceName,
                resourceType,
                subDirName,
                localizationName
            );
        if (result != NULL) {
            OCFRetain(result);
        }
    } else {
        result = (OCFURLRef)_OCFBundleCopyFindResources(
            bundle, NULL, NULL, resourceName, resourceType, subDirName, localizationName, false, true, NULL
        );
    }
    if (previous != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native _CFBundleGetBundleVersionForURL, 0x1c017105c. Hidden (was private external). The bundle directory
// (the POSIX path of the absolute URL) is iterated once; the block records directories or links named
// "Resources", "Contents" and "Support Files" (case-insensitive), "Wrapper" and, for links only,
// "WrappedBundle" (literal), and any entry of unknown type (0). With an unknown entry, "Wrapper" and then
// "WrappedBundle" relative to the URL are tested with _CFIsResourceAtURL; when both are directories both
// flags are set and the unknown flag is cleared. Without both Wrapper and WrappedBundle the version is 0 for
// Resources and 2 for Contents (Resources first when the URL string ends in ".framework/", Contents first
// otherwise), else 1 for Support Files, else 3. With both, the WrappedBundle link (readlink, 1,023 bytes)
// must be relative and at least 2 bytes, and the extensions after the last '.' of the link target and of the
// bundle path must match (or both be missing); the open link and the bundle must resolve (F_GETPATH) to
// different paths with the bundle path a prefix of the target's (strnstr at offset 0, 1,024 bytes). Then the
// version is 12 when the wrapped bundle holds a "Contents" directory or link (case-insensitive) and 13
// otherwise; any failure gives 3. The four failures log the native messages at the error level with the URL.
// The directory block is ____CFBundleGetBundleVersionForURL_block_invoke (0x1c0171730) and the
// wrapped-bundle block ____CFBundleGetBundleVersionForURL_block_invoke_2 (0x1c01718a0). The __block flags are
// released in reverse order (_Block_object_dispose natively).
// Folds the compiler-outlined pieces __CFBundleGetBundleVersionForURL.cold.2 (0x1c0306814),
// __CFBundleGetBundleVersionForURL.cold.3 (0x1c0306840), __CFBundleGetBundleVersionForURL.cold.4
// (0x1c030686c).
uint8_t _CFBundleGetBundleVersionForURL(OCFURLRef url) {
    OCFURLRef absolute = OCFURLCopyAbsoluteURL(url);
    OCFStringRef path = OCFURLCopyFileSystemPath(absolute, kOCFURLPOSIXPathStyle);
    Boolean isFramework = OCFStringHasSuffix(OCFURLGetString(url), OF_STR(".framework/"));
    OCFIndex resourcesLength = OCFStringGetLength(OF_STR("Resources"));
    OCFIndex contentsLength = OCFStringGetLength(OF_STR("Contents"));
    OCFIndex supportFilesLength = OCFStringGetLength(OF_STR("Support Files"));
    OCFIndex wrappedBundleLength = OCFStringGetLength(OF_STR("WrappedBundle"));
    OCFIndex wrapperLength = OCFStringGetLength(OF_STR("Wrapper"));
    __block Boolean hasResources = false;
    __block Boolean hasContents = false;
    __block Boolean hasSupportFiles = false;
    __block Boolean hasWrappedBundle = false;
    __block Boolean hasWrapper = false;
    __block Boolean hasUnknown = false;
    _CFIterateDirectory(
        path,
        false,
        NULL,
        ^Boolean(OCFStringRef fileName, OCFStringRef fileNameWithPrefix, uint8_t fileType) {
          (void)fileNameWithPrefix;
          if (fileType == 0) {
              hasUnknown = true;
              return true;
          }
          if (fileType != 10 && fileType != 4) {
              return true;
          }
          OCFIndex length = OCFStringGetLength(fileName);
          if (length == resourcesLength &&
              OCFStringCompareWithOptions(fileName, OF_STR("Resources"), (OCFRange){0, length}, 1) ==
                  kOCFCompareEqualTo) {
              hasResources = true;
          } else if (
              length == contentsLength &&
              OCFStringCompareWithOptions(fileName, OF_STR("Contents"), (OCFRange){0, length}, 1) ==
                  kOCFCompareEqualTo
          ) {
              hasContents = true;
          } else if (
              length == supportFilesLength &&
              OCFStringCompareWithOptions(fileName, OF_STR("Support Files"), (OCFRange){0, length}, 1) ==
                  kOCFCompareEqualTo
          ) {
              hasSupportFiles = true;
          } else if (
              length == wrapperLength &&
              OCFStringCompareWithOptions(fileName, OF_STR("Wrapper"), (OCFRange){0, length}, 0) ==
                  kOCFCompareEqualTo
          ) {
              hasWrapper = true;
          } else if (
              fileType == 10 && length == wrappedBundleLength &&
              OCFStringCompareWithOptions(fileName, OF_STR("WrappedBundle"), (OCFRange){0, length}, 0) ==
                  kOCFCompareEqualTo
          ) {
              hasWrappedBundle = true;
          }
          return true;
        }
    );
    if (hasUnknown) {
        Boolean isDirectory = false;
        OCFURLRef wrapperURL = OCFURLCreateWithString(kOCFAllocatorSystemDefault, OF_STR("Wrapper"), url);
        if (wrapperURL != NULL) {
            Boolean exists = _CFIsResourceAtURL(wrapperURL, &isDirectory);
            Boolean wrapperIsDirectory = isDirectory;
            OCFRelease(wrapperURL);
            if (exists && wrapperIsDirectory) {
                hasWrapper = true;
                OCFURLRef wrappedURL =
                    OCFURLCreateWithString(kOCFAllocatorSystemDefault, OF_STR("WrappedBundle"), url);
                isDirectory = false;
                if (_CFIsResourceAtURL(wrappedURL, &isDirectory) && isDirectory) {
                    hasWrappedBundle = true;
                }
                OCFRelease(wrappedURL);
                if (hasWrapper && hasWrappedBundle) {
                    hasUnknown = false;
                }
            }
        }
    }
    uint8_t version;
    if (!hasWrapper || !hasWrappedBundle) {
        if (isFramework) {
            if (hasResources) {
                version = 0;
            } else if (hasContents) {
                version = 2;
            } else {
                version = hasSupportFiles ? 1 : 3;
            }
        } else {
            if (hasContents) {
                version = 2;
            } else if (hasResources) {
                version = 0;
            } else {
                version = hasSupportFiles ? 1 : 3;
            }
        }
    } else {
        OCFURLRef wrappedURL = OCFURLCreateCopyAppendingPathComponent(
            kOCFAllocatorSystemDefault, absolute, OF_STR("WrappedBundle"), true
        );
        OCFStringRef wrappedPath = OCFURLCopyFileSystemPath(wrappedURL, kOCFURLPOSIXPathStyle);
        OCFRelease(wrappedURL);
        __block Boolean hasWrappedContents = false;
        _CFIterateDirectory(
            wrappedPath,
            false,
            NULL,
            ^Boolean(OCFStringRef fileName, OCFStringRef fileNameWithPrefix, uint8_t fileType) {
              (void)fileNameWithPrefix;
              if (fileType == 10 || fileType == 4) {
                  OCFIndex length = OCFStringGetLength(fileName);
                  if (length == contentsLength &&
                      OCFStringCompareWithOptions(fileName, OF_STR("Contents"), (OCFRange){0, length}, 1) ==
                          kOCFCompareEqualTo) {
                      hasWrappedContents = true;
                  }
              }
              return true;
            }
        );
        char wrappedBuffer[1024];
        char bundleBuffer[1024];
        char linkBuffer[1024];
        char wrappedResolved[1024];
        char bundleResolved[1024];
        Boolean linkValid = false;
        Boolean extensionsMatch = false;
        if (OCFStringGetFileSystemRepresentation(wrappedPath, wrappedBuffer, 1024) &&
            OCFStringGetFileSystemRepresentation(path, bundleBuffer, 1024)) {
            ssize_t linkLength = readlink(wrappedBuffer, linkBuffer, 0x3FF);
            if (linkLength < 2 || linkBuffer[0] == '/') {
                os_log_error(
                    _CFBundleResourceLogger(),
                    "`WrappedBundle` link too short or pointed outside bundle at %{public}@",
                    url
                );
            } else {
                linkBuffer[linkLength] = 0;
                const char *linkExtension = strrchr(linkBuffer, '.');
                const char *bundleExtension = strrchr(bundleBuffer, '.');
                if (bundleExtension != NULL && linkExtension != NULL) {
                    if (strcmp(linkExtension + 1, bundleExtension + 1) == 0) {
                        extensionsMatch = true;
                    } else {
                        os_log_error(
                            _CFBundleResourceLogger(),
                            "Extensions of wrapped bundles did not match at %{public}@",
                            url
                        );
                    }
                } else if (((uintptr_t)bundleExtension | (uintptr_t)linkExtension) != 0) {
                    os_log_error(
                        _CFBundleResourceLogger(),
                        "Extensions of wrapped bundles did not match (one missing) at %{public}@",
                        url
                    );
                } else {
                    extensionsMatch = true;
                }
                int wrappedDescriptor = open(wrappedBuffer, O_RDONLY);
                int bundleDescriptor = open(bundleBuffer, O_RDONLY);
                if (wrappedDescriptor >= 1 && bundleDescriptor >= 1) {
                    Boolean contained = fcntl(wrappedDescriptor, F_GETPATH, wrappedResolved) != -1 &&
                                        fcntl(bundleDescriptor, F_GETPATH, bundleResolved) != -1 &&
                                        strncmp(wrappedResolved, bundleResolved, 0x400) != 0 &&
                                        strnstr(wrappedResolved, bundleResolved, 0x400) == wrappedResolved;
                    close(wrappedDescriptor);
                    linkValid = contained;
                } else if (wrappedDescriptor >= 1) {
                    close(wrappedDescriptor);
                }
                if (bundleDescriptor >= 1) {
                    close(bundleDescriptor);
                }
                if (!linkValid) {
                    os_log_error(
                        _CFBundleResourceLogger(),
                        "`WrappedBundle` link invalid or pointed outside bundle at %{public}@",
                        url
                    );
                }
            }
        }
        OCFRelease(wrappedPath);
        if (linkValid && extensionsMatch) {
            version = hasWrappedContents ? 12 : 13;
        } else {
            version = 3;
        }
    }
    OCFRelease(absolute);
    OCFRelease(path);
    return version;
}

// Native _CFBundleGetPlatformExecutablesSubdirectoryName, 0x1c0171934. Hidden (was private external). The
// constant string "MacOS".
OCFStringRef _CFBundleGetPlatformExecutablesSubdirectoryName(void) {
    return OF_STR("MacOS");
}

// Native _copyResourceURLsFromBundle, 0x1c017199c. Non-external. The query tables come from _copyQueryTable
// for the resources directory (a mutable copy in kCFAllocatorDefault); for bundle version 1 the
// "Non-localized Resources" subdirectory of it is searched first and kept when it yields a value or stops the
// predicate (its table is released otherwise). With a predicate the matching names are collected; otherwise
// the value for searchName is used. A localized search turns a string value into a one-element array (an
// array is retained) and walks its names (all of them for returnArray, else only the first): a name without
// the subdirectory past its start, or (without a subdirectory) without ".lproj" before its last seven units,
// is appended; one that has it is appended only when the localization is found anchored (option 8) with "."
// right after it, which also cancels the separate .lproj search; any other name ends the walk. Without
// returnArray, any collected name cancels that search as well. The separate search queries the table of
// "<localization>.lproj[/<subDirectory>]". The names are made into URLs relative to the bundle base path (or
// the absolute file-system path of bundleURL) plus the resources directory, with a trailing slash: all of
// them for returnArray (an empty retained array when none), else the first, relative for "_CFBT_" or
// "_CFBAF_" search names and an appended file-system path otherwise (a directory when its last unit is the
// slash). Without a match and without returnArray the answer is NULL. The query table is released without a
// NULL test.
static OCFTypeRef _copyResourceURLsFromBundle(
    OCFBundleRef bundle,
    OCFURLRef bundleURL,
    OCFArrayRef languages,
    OCFStringRef resourcesDirectory,
    OCFStringRef subDirectory,
    OCFStringRef searchName,
    OCFStringRef localization,
    Boolean returnArray,
    Boolean localized,
    uint8_t version,
    ResourcePredicate predicate
) {
    Boolean stop = false;
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableArrayRef interResult = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
    OCFTypeRef value = NULL;
    OCFMutableStringRef resourcesPath =
        OCFStringCreateMutableCopy(kOCFAllocatorDefault, 0, resourcesDirectory);
    OCFDictionaryRef queryTable = NULL;
    Boolean foundNonLocalized = false;
    if (version == 1) {
        OCFIndex savedLength = OCFStringGetLength(resourcesPath);
        _CFAppendPathComponent2(resourcesPath, OF_STR("Non-localized Resources"));
        queryTable = _copyQueryTable(bundle, bundleURL, languages, resourcesPath, subDirectory);
        if (predicate != NULL) {
            _CFBundleFindResourcesWithPredicate(interResult, queryTable, predicate, &stop);
        } else {
            value = OCFDictionaryGetValue(queryTable, searchName);
        }
        OCFStringDelete(
            resourcesPath, (OCFRange){savedLength, OCFStringGetLength(resourcesPath) - savedLength}
        );
        if (value != NULL || stop) {
            foundNonLocalized = true;
        } else if (queryTable != NULL) {
            OCFRelease(queryTable);
        }
    }
    if (!foundNonLocalized) {
        queryTable = _copyQueryTable(bundle, bundleURL, languages, resourcesPath, subDirectory);
        if (predicate != NULL) {
            _CFBundleFindResourcesWithPredicate(interResult, queryTable, predicate, &stop);
        } else {
            value = OCFDictionaryGetValue(queryTable, searchName);
        }
    }
    Boolean hasSubDirectory = subDirectory != NULL && OCFStringGetLength(subDirectory) > 0;
    Boolean searchLproj = localization != NULL && OCFStringGetLength(localization) > 0;
    if (localized && value != NULL) {
        if (OCFGetTypeID(value) == OCFStringGetTypeID()) {
            value = OCFArrayCreate(allocator, &value, 1, &kOCFTypeArrayCallBacks);
        } else {
            OCFRetain(value);
        }
        OCFRange found = {0, 0};
        OCFIndex count = 1;
        Boolean walk = true;
        if (returnArray) {
            count = OCFArrayGetCount((OCFArrayRef)value);
            if (count <= 0) {
                OCFRelease(value);
                walk = false;
            }
        }
        if (walk) {
            for (OCFIndex index = 0;;) {
                OCFStringRef name = OCFArrayGetValueAtIndex((OCFArrayRef)value, index);
                OCFIndex length = OCFStringGetLength(name);
                Boolean checkLocalization;
                if (hasSubDirectory) {
                    checkLocalization =
                        OCFStringFindWithOptions(name, subDirectory, (OCFRange){0, length}, 0, &found) &&
                        found.location != 0;
                } else {
                    checkLocalization =
                        length != 0 &&
                        OCFStringFindWithOptions(name, OF_STR(".lproj"), (OCFRange){0, length}, 0, &found) &&
                        found.location + 7 < length;
                }
                if (checkLocalization) {
                    if (localization == NULL || OCFStringGetLength(localization) < 1 ||
                        !OCFStringFindWithOptions(name, localization, (OCFRange){0, length}, 8, &found) ||
                        !OCFStringFindWithOptions(
                            name, OF_STR("."), (OCFRange){found.location + found.length, 1}, 0, &found
                        )) {
                        break;
                    }
                    searchLproj = false;
                }
                OCFArrayAppendValue(interResult, name);
                index++;
                if (count == index) {
                    break;
                }
            }
            OCFRelease(value);
            if (!returnArray && OCFArrayGetCount(interResult) != 0) {
                searchLproj = false;
            }
        }
    } else if (value != NULL) {
        if (OCFGetTypeID(value) == OCFArrayGetTypeID()) {
            OCFArrayAppendArray(
                interResult, (OCFArrayRef)value, (OCFRange){0, OCFArrayGetCount((OCFArrayRef)value)}
            );
        } else {
            OCFArrayAppendValue(interResult, value);
        }
    }
    value = NULL;
    OCFRelease(queryTable);
    if (searchLproj) {
        OCFMutableStringRef lprojSubDirectory =
            OCFStringCreateMutableCopy(kOCFAllocatorDefault, 0, localization);
        _CFAppendPathExtension2(lprojSubDirectory, OF_STR("lproj"));
        if (subDirectory != NULL && OCFStringGetLength(subDirectory) >= 1) {
            _CFAppendPathComponent2(lprojSubDirectory, subDirectory);
        }
        OCFDictionaryRef lprojTable =
            _copyQueryTable(bundle, bundleURL, languages, resourcesPath, lprojSubDirectory);
        OCFRelease(lprojSubDirectory);
        value = OCFDictionaryGetValue(lprojTable, searchName);
        if (value != NULL) {
            if (OCFGetTypeID(value) == OCFStringGetTypeID()) {
                OCFArrayAppendValue(interResult, value);
            } else {
                OCFArrayAppendArray(
                    interResult, (OCFArrayRef)value, (OCFRange){0, OCFArrayGetCount((OCFArrayRef)value)}
                );
            }
        }
        OCFRelease(lprojTable);
    }
    OCFTypeRef result;
    if (OCFArrayGetCount(interResult) >= 1) {
        UniChar slash = _CFGetSlash();
        OCFMutableStringRef urlString;
        if (bundle != NULL) {
            urlString = OCFStringCreateMutableCopy(allocator, 0, bundle->_bundleBasePath);
        } else {
            OCFURLRef absoluteURL = OCFURLCopyAbsoluteURL(bundleURL);
            OCFStringRef bundlePath = OCFURLCopyFileSystemPath(absoluteURL, kOCFURLPOSIXPathStyle);
            urlString = OCFStringCreateMutableCopy(allocator, 0, bundlePath);
            OCFRelease(absoluteURL);
            OCFRelease(bundlePath);
        }
        if (resourcesDirectory != NULL && OCFStringGetLength(resourcesDirectory) != 0) {
            _CFAppendPathComponent2(urlString, resourcesDirectory);
        }
        _CFAppendTrailingPathSlash2(urlString);
        if (returnArray) {
            OCFIndex count = OCFArrayGetCount(interResult);
            OCFURLRef base =
                OCFURLCreateWithFileSystemPath(allocator, urlString, kOCFURLPOSIXPathStyle, true);
            OCFMutableArrayRef urls = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
            for (OCFIndex index = 0; index < count; index++) {
                OCFStringRef name = OCFArrayGetValueAtIndex(interResult, index);
                OCFURLRef url =
                    _CFBundleCreateRelativeURLFromBaseAndPath(name, base, slash, _CFGetSlashStr());
                OCFArrayAppendValue(urls, url);
                OCFRelease(url);
            }
            OCFRelease(base);
            result = urls;
        } else {
            Boolean relative = OCFStringHasPrefix(searchName, OF_STR("_CFBT_")) |
                               OCFStringHasPrefix(searchName, OF_STR("_CFBAF_"));
            OCFStringRef name = OCFArrayGetValueAtIndex(interResult, 0);
            if (relative) {
                OCFURLRef base =
                    OCFURLCreateWithFileSystemPath(allocator, urlString, kOCFURLPOSIXPathStyle, true);
                result = _CFBundleCreateRelativeURLFromBaseAndPath(name, base, slash, _CFGetSlashStr());
                OCFRelease(base);
            } else {
                OCFStringAppend(urlString, name);
                Boolean isDirectory =
                    OCFStringGetCharacterAtIndex(name, OCFStringGetLength(name) - 1) == slash;
                result =
                    OCFURLCreateWithFileSystemPath(allocator, urlString, kOCFURLPOSIXPathStyle, isDirectory);
            }
        }
        OCFRelease(urlString);
    } else if (returnArray) {
        result = OCFRetain(interResult);
    } else {
        result = NULL;
    }
    if (resourcesPath != NULL) {
        OCFRelease(resourcesPath);
    }
    OCFRelease(interResult);
    return result;
}

// Native _copyQueryTable, 0x1c01723b0. Non-external. Without languages, a bundle supplies its language search
// list (otherwise the languages are retained). For a bundle, the cache key is the resources directory with
// the subdirectory appended (a mutable copy in kCFAllocatorDefault) or the retained directory; under the lock
// at +168 the cache at +176 (created on first use: system default allocator,
// kCFCopyStringDictionaryKeyCallBacks and kCFTypeDictionaryValueCallBacks) answers a retained table, or a
// table made by _createQueryTableAtPath from the bundle base path (+184) and the creation option at +55 is
// entered. Without a bundle the table is made from the absolute file-system path of bundleURL with the flag
// 0. A NULL table is replaced by an empty dictionary (system default allocator, kCFType callbacks).
static OCFDictionaryRef _copyQueryTable(
    OCFBundleRef bundle,
    OCFURLRef bundleURL,
    OCFArrayRef languages,
    OCFStringRef resourcesDirectory,
    OCFStringRef subDirectory
) {
    OCFArrayRef searchLanguages = languages;
    OCFStringRef key;
    OCFDictionaryRef queryTable;
    if (bundle != NULL && languages == NULL) {
        searchLanguages = _CFBundleCopyLanguageSearchListInBundle(bundle);
    } else if (languages != NULL) {
        OCFRetain(languages);
    }
    if (bundle != NULL) {
        if (subDirectory != NULL) {
            OCFMutableStringRef path =
                OCFStringCreateMutableCopy(kOCFAllocatorDefault, 0, resourcesDirectory);
            _CFAppendPathComponent2(path, subDirectory);
            key = path;
        } else {
            key = OCFRetain(resourcesDirectory);
        }
        os_unfair_lock_lock(&bundle->_lock168);
        queryTable = NULL;
        if (bundle->_queryTables != NULL) {
            queryTable = OCFDictionaryGetValue(bundle->_queryTables, key);
            if (queryTable != NULL) {
                OCFRetain(queryTable);
            }
        } else {
            bundle->_queryTables = OCFDictionaryCreateMutable(
                kOCFAllocatorSystemDefault,
                0,
                &kOCFCopyStringDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
        }
        if (queryTable == NULL) {
            queryTable = _createQueryTableAtPath(
                bundle->_bundleBasePath,
                searchLanguages,
                resourcesDirectory,
                subDirectory,
                bundle->_createOption1
            );
            OCFDictionarySetValue(bundle->_queryTables, key, queryTable);
        }
        os_unfair_lock_unlock(&bundle->_lock168);
    } else {
        OCFURLRef absoluteURL = OCFURLCopyAbsoluteURL(bundleURL);
        key = OCFURLCopyFileSystemPath(absoluteURL, kOCFURLPOSIXPathStyle);
        OCFRelease(absoluteURL);
        queryTable = _createQueryTableAtPath(key, searchLanguages, resourcesDirectory, subDirectory, false);
    }
    OCFRelease(key);
    if (searchLanguages != NULL) {
        OCFRelease(searchLanguages);
    }
    if (queryTable != NULL) {
        return queryTable;
    }
    return OCFDictionaryCreate(
        kOCFAllocatorSystemDefault,
        NULL,
        NULL,
        0,
        &kOCFTypeDictionaryKeyCallBacks,
        &kOCFTypeDictionaryValueCallBacks
    );
}

// Native _CFBundleFindResourcesWithPredicate, 0x1c0172568. Non-external (its only caller is
// _copyResourceURLsFromBundle). Nothing happens for an empty table. Two arrays of 8 * count
// bytes (malloc type 0xC0040B8AA526D) receive the keys and values; when either allocation fails, the other is
// freed (keys first). For each key the predicate is invoked with the stop pointer; a match appends a string
// value, or all of an array value. The walk ends after the last entry or once *stop is nonzero.
static void _CFBundleFindResourcesWithPredicate(
    OCFMutableArrayRef interResult, OCFDictionaryRef queryTable, ResourcePredicate predicate, Boolean *stop
) {
    OCFIndex count = OCFDictionaryGetCount(queryTable);
    if (count == 0) {
        return;
    }
    const void **keys = malloc_type_malloc(8 * count, 0xC0040B8AA526D);
    const void **values = malloc_type_malloc(8 * count, 0xC0040B8AA526D);
    if (keys == NULL || values == NULL) {
        if (keys != NULL) {
            free(keys);
        }
        if (values != NULL) {
            free(values);
        }
        return;
    }
    OCFDictionaryGetKeysAndValues(queryTable, keys, values);
    OCFIndex remaining = count - 1;
    if (count >= 1) {
        const void **key = keys;
        const void **value = values;
        do {
            if (predicate((OCFStringRef)*key++, stop)) {
                OCFTypeID typeID = OCFGetTypeID(*value);
                if (typeID == OCFStringGetTypeID()) {
                    OCFArrayAppendValue(interResult, *value);
                } else {
                    OCFArrayRef array = (OCFArrayRef)*value;
                    OCFArrayAppendArray(interResult, array, (OCFRange){0, OCFArrayGetCount(array)});
                }
            }
            if (*stop != 0 || remaining == 0) {
                break;
            }
            remaining--;
            value++;
        } while (true);
    }
    free(keys);
    free(values);
}

// Native _CFBundleCreateRelativeURLFromBaseAndPath, 0x1c01726d0. Non-external (its only caller is
// _copyResourceURLsFromBundle). slashStr is searched backwards (option 4) in all but
// the last unit of path; when found, the part before it extends the base (a directory component) and the part
// after it becomes the path, both released afterwards. The URL is a POSIX path relative to that base, a
// directory when the last unit of the path equals slash.
static OCFURLRef _CFBundleCreateRelativeURLFromBaseAndPath(
    OCFStringRef path, OCFURLRef base, UniChar slash, OCFStringRef slashStr
) {
    OCFRange found = {0, 0};
    Boolean split = OCFStringFindWithOptions(
        path, slashStr, (OCFRange){0, OCFStringGetLength(path) - 1}, kOCFCompareBackwards, &found
    );
    if (split) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFStringRef directory = OCFStringCreateWithSubstring(allocator, path, (OCFRange){0, found.location});
        base = OCFURLCreateCopyAppendingPathComponent(allocator, base, directory, true);
        OCFIndex location = found.location;
        OCFIndex length = OCFStringGetLength(path);
        path =
            OCFStringCreateWithSubstring(allocator, path, (OCFRange){location + 1, length + ~found.location});
        OCFRelease(directory);
    }
    Boolean isDirectory = OCFStringGetCharacterAtIndex(path, OCFStringGetLength(path) - 1) == slash;
    OCFURLRef result = OCFURLCreateWithFileSystemPathRelativeToBase(
        kOCFAllocatorSystemDefault, path, kOCFURLPOSIXPathStyle, isDirectory, base
    );
    if (split) {
        OCFRelease(base);
        OCFRelease(path);
    }
    return result;
}

static void _CFBundleAddValueForType(
    OCFStringRef type,
    OCFMutableDictionaryRef queryTable,
    OCFMutableDictionaryRef typeDir,
    OCFTypeRef value,
    OCFMutableDictionaryRef addedTypes,
    Boolean firstLproj
);

// Native _createQueryTableAtPath, 0x1c017282c. Non-external. The query table, the list of all files and the
// type directory are made with the system default allocator (string-copy keys, kCFType values). The path to
// read is inPath plus resourcesDirectory (when set); its length is the restore point after each directory is
// read with subdirectory appended. The resources directory is read first; with languages, the first
// language's "<language>.lproj" directory is read (lproj name "<language>.lproj", first lproj) into an added
// types table, then "Base.lproj" (first lproj; files added as far as the first language added any), then the
// remaining languages (not first lproj). Each later read is told whether any earlier read added files: once
// known true it stays true; otherwise it is recomputed against the count after the resources directory.
// Without languages the added types table is still made and Base.lproj read. A nonempty file list is entered
// under "_CFBAF_".
static OCFDictionaryRef _createQueryTableAtPath(
    OCFStringRef inPath,
    OCFArrayRef languages,
    OCFStringRef resourcesDirectory,
    OCFStringRef subdirectory,
    Boolean productAndPlatformFlag
) {
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableDictionaryRef queryTable = OCFDictionaryCreateMutable(
        allocator, 0, &kOCFCopyStringDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    OCFMutableArrayRef allFiles = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
    OCFMutableDictionaryRef typeDir = OCFDictionaryCreateMutable(
        allocator, 0, &kOCFCopyStringDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    OCFMutableStringRef pathToRead = OCFStringCreateMutableCopy(allocator, 0, inPath);
    if (resourcesDirectory != NULL) {
        _CFAppendPathComponent2(pathToRead, resourcesDirectory);
    }
    OCFIndex basePathLength = OCFStringGetLength(pathToRead);
    if (subdirectory != NULL) {
        _CFAppendPathComponent2(pathToRead, subdirectory);
    }
    _CFBundleReadDirectory(
        pathToRead,
        subdirectory,
        productAndPlatformFlag,
        allFiles,
        false,
        queryTable,
        typeDir,
        NULL,
        false,
        NULL
    );
    OCFStringDelete(pathToRead, (OCFRange){basePathLength, OCFStringGetLength(pathToRead) - basePathLength});
    OCFIndex numOfAllFiles = OCFArrayGetCount(allFiles);
    OCFMutableDictionaryRef addedTypes;
    Boolean hasFileAdded = false;
    Boolean moreLanguages = false;
    OCFIndex languageCount = 0;
    if (languages != NULL) {
        languageCount = OCFArrayGetCount(languages);
        addedTypes = OCFDictionaryCreateMutable(
            allocator, 0, &kOCFCopyStringDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
        if (languageCount >= 1) {
            OCFStringRef language = OCFArrayGetValueAtIndex(languages, 0);
            OCFMutableStringRef lprojName =
                OCFStringCreateMutableCopy(allocator, OCFStringGetLength(language) + 5, language);
            OCFStringAppend(lprojName, OF_STR(".lproj"));
            _CFAppendPathComponent2(pathToRead, language);
            _CFAppendPathExtension2(pathToRead, OF_STR("lproj"));
            if (subdirectory != NULL) {
                _CFAppendPathComponent2(pathToRead, subdirectory);
            }
            _CFBundleReadDirectory(
                pathToRead,
                subdirectory,
                productAndPlatformFlag,
                allFiles,
                false,
                queryTable,
                typeDir,
                addedTypes,
                true,
                lprojName
            );
            OCFRelease(lprojName);
            OCFStringDelete(
                pathToRead, (OCFRange){basePathLength, OCFStringGetLength(pathToRead) - basePathLength}
            );
            hasFileAdded = numOfAllFiles < OCFArrayGetCount(allFiles);
            moreLanguages = languageCount != 1;
        }
    } else {
        addedTypes = OCFDictionaryCreateMutable(
            allocator, 0, &kOCFCopyStringDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    _CFAppendPathComponent2(pathToRead, OF_STR("Base"));
    _CFAppendPathExtension2(pathToRead, OF_STR("lproj"));
    if (subdirectory != NULL) {
        _CFAppendPathComponent2(pathToRead, subdirectory);
    }
    _CFBundleReadDirectory(
        pathToRead,
        subdirectory,
        productAndPlatformFlag,
        allFiles,
        hasFileAdded,
        queryTable,
        typeDir,
        addedTypes,
        true,
        OF_STR("Base.lproj")
    );
    OCFStringDelete(pathToRead, (OCFRange){basePathLength, OCFStringGetLength(pathToRead) - basePathLength});
    Boolean added = true;
    if (!hasFileAdded) {
        added = numOfAllFiles < OCFArrayGetCount(allFiles);
    }
    if (moreLanguages && OCFArrayGetCount(languages) > 1) {
        for (OCFIndex index = 1; index < OCFArrayGetCount(languages); index++) {
            OCFStringRef language = OCFArrayGetValueAtIndex(languages, index);
            OCFMutableStringRef lprojName =
                OCFStringCreateMutableCopy(allocator, OCFStringGetLength(language) + 5, language);
            OCFStringAppend(lprojName, OF_STR(".lproj"));
            _CFAppendPathComponent2(pathToRead, language);
            _CFAppendPathExtension2(pathToRead, OF_STR("lproj"));
            if (subdirectory != NULL) {
                _CFAppendPathComponent2(pathToRead, subdirectory);
            }
            Boolean wasAdded = added;
            _CFBundleReadDirectory(
                pathToRead,
                subdirectory,
                productAndPlatformFlag,
                allFiles,
                wasAdded,
                queryTable,
                typeDir,
                addedTypes,
                false,
                lprojName
            );
            OCFRelease(lprojName);
            OCFStringDelete(
                pathToRead, (OCFRange){basePathLength, OCFStringGetLength(pathToRead) - basePathLength}
            );
            if (wasAdded) {
                added = true;
            } else {
                added = numOfAllFiles < OCFArrayGetCount(allFiles);
            }
        }
    }
    OCFRelease(addedTypes);
    OCFRelease(pathToRead);
    if (OCFArrayGetCount(allFiles) >= 1) {
        OCFDictionarySetValue(queryTable, OF_STR("_CFBAF_"), allFiles);
    }
    OCFRelease(allFiles);
    OCFRelease(typeDir);
    return queryTable;
}

// Native _CFBundleReadDirectory, 0x1c0172c8c. Non-external (its only caller is _createQueryTableAtPath). The
// prefixes for _CFIterateDirectory (which appends a slash to directory names) are [lprojName,
// subdirectoryPrefix], [lprojName] or [subdirectoryPrefix] (kCFTypeArrayCallBacks) for whichever are set, or
// none. Each entry is split by _CFBundleSplitFileName against the product and platform suffixes (mode 1 when
// productAndPlatformFlag is set, 2 otherwise); unless hasFileAdded, its prefixed name is appended to
// allFiles; its start and end types are registered; the plain name maps to the prefixed name unless already
// present, and the name without product or platform maps to it as well when the existing entry does not
// already carry a more specific variant (a "~product" or "-platform" suffix of this device; an outcome other
// than 1 or 5 keeps any existing entry). The native call also loads W3 with productAndPlatformFlag for
// _CFBundleSupportedProductName and _CFBundleSupportedPlatformName, which never read X3.
static void _CFBundleReadDirectory(
    OCFStringRef pathOfDir,
    OCFStringRef subdirectoryPrefix,
    Boolean productAndPlatformFlag,
    OCFMutableArrayRef allFiles,
    Boolean hasFileAdded,
    OCFMutableDictionaryRef queryTable,
    OCFMutableDictionaryRef typeDir,
    OCFMutableDictionaryRef addedTypes,
    Boolean firstLproj,
    OCFStringRef lprojName
) {
    OCFStringRef productSuffix = _CFBundleGetProductNameSuffix();
    OCFStringRef platformSuffix = _CFBundleGetPlatformNameSuffix();
    OCFArrayRef stuffToPrefix;
    if (lprojName != NULL && subdirectoryPrefix != NULL) {
        const void *prefixes[2] = {lprojName, subdirectoryPrefix};
        stuffToPrefix = OCFArrayCreate(kOCFAllocatorSystemDefault, prefixes, 2, &kOCFTypeArrayCallBacks);
    } else if (lprojName != NULL) {
        stuffToPrefix =
            OCFArrayCreate(kOCFAllocatorSystemDefault, (const void **)&lprojName, 1, &kOCFTypeArrayCallBacks);
    } else if (subdirectoryPrefix != NULL) {
        stuffToPrefix = OCFArrayCreate(
            kOCFAllocatorSystemDefault, (const void **)&subdirectoryPrefix, 1, &kOCFTypeArrayCallBacks
        );
    } else {
        stuffToPrefix = NULL;
    }
    _CFIterateDirectory(
        pathOfDir,
        true,
        stuffToPrefix,
        ^Boolean(OCFStringRef fileName, OCFStringRef fileNameWithPrefix, uint8_t fileType) {
          // Native ___CFBundleReadDirectory_block_invoke, 0x1c0172e2c. Non-external. Always answers 1;
          // fileType is not read.
          (void)fileType;
          OCFStringRef endType = NULL;
          OCFStringRef startType = NULL;
          OCFStringRef noProductOrPlatform = NULL;
          int32_t outcome = 0;
          _CFBundleSplitFileName(
              fileName,
              &noProductOrPlatform,
              &endType,
              &startType,
              productSuffix,
              platformSuffix,
              productAndPlatformFlag ? 1 : 2,
              &outcome
          );
          if (!hasFileAdded) {
              OCFArrayAppendValue(allFiles, fileNameWithPrefix);
          }
          if (startType != NULL) {
              _CFBundleAddValueForType(
                  startType, queryTable, typeDir, fileNameWithPrefix, addedTypes, firstLproj
              );
          }
          if (endType != NULL) {
              _CFBundleAddValueForType(
                  endType, queryTable, typeDir, fileNameWithPrefix, addedTypes, firstLproj
              );
          }
          const void *existing = OCFDictionaryGetValue(queryTable, fileName);
          if ((outcome & ~4) == 1) {
              if (existing == NULL) {
                  OCFDictionarySetValue(queryTable, fileName, fileNameWithPrefix);
              }
          } else {
              if (existing == NULL) {
                  OCFDictionarySetValue(queryTable, fileName, fileNameWithPrefix);
              }
              if (noProductOrPlatform != NULL) {
                  OCFStringRef current = (OCFStringRef)OCFDictionaryGetValue(queryTable, noProductOrPlatform);
                  Boolean replace = false;
                  if (current == NULL) {
                      replace = true;
                  } else {
                      OCFIndex start;
                      OCFIndex length;
                      Boolean proceed = true;
                      if (lprojName != NULL) {
                          if (!OCFStringHasPrefix(current, lprojName)) {
                              proceed = false;
                          }
                      }
                      if (proceed) {
                          if (lprojName != NULL) {
                              start = OCFStringGetLength(lprojName);
                              length = OCFStringGetLength(current) - start;
                          } else {
                              length = OCFStringGetLength(current);
                              start = 0;
                          }
                          OCFStringRef product = productSuffix;
                          OCFStringRef platform = platformSuffix;
                          OCFRange range = {start, length};
                          int32_t kind;
                          int32_t fallbackKind;
                          Boolean decided = false;
                          if (OCFStringFindWithOptions(current, OF_STR("~"), range, 0, NULL)) {
                              if (OCFStringGetLength(product) != 1 &&
                                  OCFStringFindWithOptions(current, product, range, 0, NULL)) {
                                  kind = 4;
                                  fallbackKind = 2;
                              } else if (_CFBundleSupportedProductName(current, range)) {
                                  decided = true;
                              } else {
                                  kind = 3;
                                  fallbackKind = 1;
                              }
                          } else {
                              kind = 3;
                              fallbackKind = 1;
                          }
                          if (!decided) {
                              if (OCFStringFindWithOptions(current, OF_STR("-"), range, 0, NULL)) {
                                  if (!OCFStringFindWithOptions(current, platform, range, 0, NULL)) {
                                      Boolean supported = _CFBundleSupportedPlatformName(current, range);
                                      kind = fallbackKind;
                                      if (supported) {
                                          decided = true;
                                      }
                                  }
                              } else {
                                  kind = fallbackKind;
                              }
                          }
                          if (!decided) {
                              if (kind == 3 || kind == 1) {
                                  replace = true;
                              } else if (kind == 2 && outcome == 4) {
                                  replace = true;
                              }
                          }
                      }
                  }
                  if (replace) {
                      OCFDictionarySetValue(queryTable, noProductOrPlatform, fileNameWithPrefix);
                  }
              }
          }
          if (startType != NULL) {
              OCFRelease(startType);
          }
          if (endType != NULL) {
              OCFRelease(endType);
          }
          if (noProductOrPlatform != NULL) {
              OCFRelease(noProductOrPlatform);
          }
          return true;
        }
    );
    if (stuffToPrefix != NULL) {
        OCFRelease(stuffToPrefix);
    }
}

// Native _CFBundleAddValueForType, 0x1c0173114. Non-external (only the _CFBundleReadDirectory block calls
// it). A type seen for the first time gets a new mutable array (kCFTypeArrayCallBacks) stored under
// "_CFBT_.type" (the "%@.%@" format) in the query table and under the type in typeDir, then released (the
// tables keep it). With an addedTypes table, firstLproj records the type there, and otherwise a type already
// recorded is not appended; the value is appended to the type's array.
static void _CFBundleAddValueForType(
    OCFStringRef type,
    OCFMutableDictionaryRef queryTable,
    OCFMutableDictionaryRef typeDir,
    OCFTypeRef value,
    OCFMutableDictionaryRef addedTypes,
    Boolean firstLproj
) {
    OCFMutableArrayRef values = (OCFMutableArrayRef)OCFDictionaryGetValue(typeDir, type);
    if (values == NULL) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFStringRef key =
            OCFStringCreateWithFormat(allocator, NULL, OF_STR("%@.%@"), OF_STR("_CFBT_"), type);
        values = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
        OCFDictionarySetValue(queryTable, key, values);
        OCFDictionarySetValue(typeDir, type, values);
        OCFRelease(values);
        OCFRelease(key);
    }
    if (addedTypes != NULL) {
        if (firstLproj) {
            OCFDictionarySetValue(addedTypes, type, type);
        } else if (OCFDictionaryGetValue(addedTypes, type) != NULL) {
            return;
        }
    }
    OCFArrayAppendValue(values, value);
}
