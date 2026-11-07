//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFBundleStrings.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/PlugIn/OCFBundleInfoPlist.h"
#include "CoreFoundation/PlugIn/OCFBundleLocalization.h"
#include "CoreFoundation/PlugIn/OCFBundleResources.h"
#include "CoreFoundation/Preferences/OCFPreferences.h"
#include "CoreFoundation/PropertyList/OCFPropertyList.h"
#include "CoreFoundation/String/OCFStringBundleSupport.h"
#include "CoreFoundation/String/OCFStringFormatConfiguration.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/String/OCFString.h"

#include <assert.h>
#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <os/log.h>
#include <string.h>

// Inferred organization: the bundle-strings cluster of the iOS Simulator __text section,
// CFBundleCopyLocalizedString (0x1c02bf0c4) through ____copyIngestedCacheResult_block_invoke_2 (0x1c02c3d74),
// whose functions log the source path PlugIn.subproj/CFBundle_Strings.m and assert with "CFBundle_Strings.m".
// The native unit is Objective-C (it messages NSSet and NSURL and uses fast enumeration); this translation
// unit spells those messages through objc_msgSend. Functions are kept in native address order, each block
// literal's invoke written inside its owner (the blocks of the inlined
// _stringTableFromCacheSatisfyingRequest, _copyIngestedCacheResult and
// _CFBundleCopyLocTableProvenanceForDeviceAndPlatformVariants included); the .cold fragments
// (0x1c03120b4-0x1c03121dc) are folded into their callers. _CFStringCreateAppend3, _CFStringCreateAppend2 and
// __CFStringsDictAddFunction of this cluster are restored in String/OCFStringBundleSupport.c. Not in scope on
// this branch: _CFBundleSearchForLocalizedString and its blocks,
// _CFBundleFlushStringSourceCache, the localization-list and filtered-strings functions
// (0x1c02c173c-0x1c02c1eec). _releaseStringsSource (0x1c02c29f8) is restored from
// Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json. Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified; iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBundle.md
// (pending; the coordinator writes the reading).

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call here passes 0x10000
// (OS_UNFAIR_LOCK_DATA_SYNCHRONIZATION).
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

// libobjc's objc_alloc and objc_release (no SDK declarations), as the native code imports them.
extern id objc_alloc(Class cls);
extern void objc_release(id object);

// The class _CFBundleCopyLocalizedStringForLocalizationTableURLAndMarkdownOption, _loadLocTable and
// _CFBundleGetLocTableProvenanceForLanguage allocate (NSSet, an Objective-C class of this image).
extern char setClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSSet));

// The fast-enumeration state of -countByEnumeratingWithState:objects:count: (64 bytes, zeroed by the caller).
// FastEnumerationState is a local name for the layout of NSFastEnumerationState.
typedef struct {
    unsigned long state;
    id *itemsPtr;
    unsigned long *mutationsPtr;
    unsigned long extra[5];
} FastEnumerationState;

// The source path the native _CFBundleLogSourceLocation calls of this unit pass (__cstring 0x1c0498eb4).
static const char kSourcePath[] =
    "/Library/Caches/com.apple.xbs/C9620487-17E9-4D33-8BF5-CCA0DA6AFB18/TemporaryDirectory.R3jFjg/Sources/"
    "CoreFoundation_Sim/CoreFoundation/PlugIn.subproj/CFBundle_Strings.m";

// The per-thread slot 0x12 marks a CFBundle entry point in progress: only the outermost entry logs its source
// location, and it clears the slot on return.
#define kBundleEntryTSDSlot 0x12

// The table request of _copyStringTable: the strings sources followed by the load state (96 bytes on the
// native stack). Offsets are those of the native loads and stores; StringsRequest is a local name.
typedef struct {
    StringsSources sources;               // +0
    uint8_t cacheable;                    // +72
    uint8_t someDataUnmapped;             // +73: a present data was not mapped.
    uint8_t loadedWholeTable;             // +74
    uint8_t hasDictionaryValues;          // +75: a dictionary-valued entry needs the rule appliers.
    uint8_t hasStrings;                   // +76
    uint8_t hasStringsDict;               // +77
    uint8_t hasLocTable;                  // +78
    uint8_t _padding79;                   // +79
    OCFStringRef locTableLanguage;        // +80: retained once the loctable language was loaded.
    OCFMutableDictionaryRef stringsTable; // +88: the .strings entries.
} StringsRequest;

OF_ABI_ASSERT(sizeof(StringsRequest) == 96, "The table request is 96 bytes.");

// The context of the three rule appliers (40 bytes on the native stack). ApplierContext is a local name.
typedef struct {
    OCFDictionaryRef stringsTable;  // +0: the .strings entries (the fallback values).
    OCFMutableDictionaryRef result; // +8
    OCFBundleRef bundle;            // +16
    OCFURLRef stringsDictURL;       // +24
    uint8_t preventMarkdownParsing; // +32
    uint8_t _padding33[7];          // +33
} ApplierContext;

static void _CFBundleRetainStringsSources(StringsSources sources);
static void _CFBundleLoadNonLocTableData(
    OCFBundleRef bundle, OCFStringRef tableName, StringsSources *sources, Boolean allowMapping, Boolean flag
);
static void _CFBundleIngestResultForBundleCache(
    OCFBundleRef bundle, OCFStringRef tableName, Boolean isDefault, StringsSources sources
);
static OCFDictionaryRef _copyStringTable(
    OCFBundleRef bundle,
    OCFStringRef tableName,
    OCFSetRef keys,
    OCFArrayRef localizations,
    Boolean preventMarkdownParsing,
    OCFTypeRef *outURL
);
static void _CFBundleCopyMostAppropriateLocTableDeviceAndPlatformSpecificVariants(
    OCFDataRef data,
    OCFStringRef language,
    OCFStringRef *stringsVariant,
    uint32_t *stringsVersion,
    OCFStringRef *stringsDictVariant,
    uint32_t *stringsDictVersion
);
static void
_CFBundleAddProvenanceKeyPathIfPresent(OCFSetRef topLevelKeys, OCFMutableSetRef keyPaths, OCFStringRef key);
static Boolean _useLoctableInsteadOfFileBasedOnVersion(
    OCFURLRef *url, OCFStringRef *language, uint32_t version, OCFStringRef locTableLanguage
);
static uint32_t _CFBundleGetFileVersionForStringsResourceURL(OCFURLRef url);
static void _CFBundleReleaseStringsSources(StringsSources sources);
static void _releaseStringsSource(OCFAllocatorRef allocator, const void *value);
static OCFStringRef _CFBundleCopyLanguageForStringsResourceURL(OCFURLRef url);
static void _CFBundleGetLocTableProvenanceForLanguage(
    OCFDataRef data, OCFStringRef language, Boolean *strings, Boolean *stringsDict
);
static void _loadStringsDictTable(
    OCFBundleRef bundle,
    OCFStringRef tableName,
    OCFSetRef keys,
    OCFMutableDictionaryRef result,
    StringsRequest *request,
    OCFTypeRef *outURL
);
static void _loadLocTable(
    OCFBundleRef bundle,
    OCFStringRef tableName,
    OCFStringRef language,
    OCFSetRef keys,
    OCFMutableDictionaryRef result,
    StringsRequest *request,
    OCFTypeRef *outURL
);
static Boolean _loadStringsFromData(
    OCFBundleRef bundle,
    OCFStringRef tableName,
    OCFSetRef keys,
    OCFDataRef data,
    Boolean mapped,
    OCFMutableDictionaryRef result,
    StringsRequest *request
);
static Boolean _dictionaryContainsAllKeysInSet(OCFDictionaryRef dict, OCFSetRef keys);
static OCFPropertyListRef __CFBundleCreateStringsFromPlistData(
    OCFBundleRef bundle, OCFSetRef keyPaths, OCFDataRef data, Boolean mapped, OCFStringRef tableName
);
static void
__CFPlatformSpecificStringDictionaryApplyFunction(const void *key, const void *value, void *context);
static void __CFLocalizedFormatStringApplyFunction(const void *key, const void *value, void *context);
static void __CFVariableWidthStringDictionaryApplyFunction(const void *key, const void *value, void *context);

// Native CFBundleCopyLocalizedString, 0x1c02bf0c4. External. A tail call of
// CFBundleCopyLocalizedStringForLocalization with a NULL localization (MOV X4, #0; B).
OCFStringRef OCFBundleCopyLocalizedString(
    OCFBundleRef bundle, OCFStringRef key, OCFStringRef value, OCFStringRef tableName
) {
    return OCFBundleCopyLocalizedStringForLocalization(bundle, key, value, tableName, NULL);
}

// Native CFBundleCopyLocalizedStringForLocalization, 0x1c02bf0cc. External. The entry-point pattern (line
// 1545). With CFSwiftBundle the answer of -localizedStringForKey:value:table:localization: is retained;
// otherwise _CFBundleCopyLocalizedStringForLocalizationAndTableURL with no URL pointer.
OCFStringRef OCFBundleCopyLocalizedStringForLocalization(
    OCFBundleRef bundle,
    OCFStringRef key,
    OCFStringRef value,
    OCFStringRef tableName,
    OCFStringRef localizationName
) {
    void *nested = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        outermost, kSourcePath, 1545, OF_NAME_STRING(OCFBundleCopyLocalizedStringForLocalization)
    );
    OCFStringRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result =
            ((OCFStringRef (*)(id, SEL, OCFStringRef, OCFStringRef, OCFStringRef, OCFStringRef))objc_msgSend)(
                (id)bundle,
                sel_getUid("localizedStringForKey:value:table:localization:"),
                key,
                value,
                tableName,
                localizationName
            );
        if (result != NULL) {
            OCFRetain(result);
        }
    } else {
        result = _OCFBundleCopyLocalizedStringForLocalizationAndTableURL(
            bundle, key, value, tableName, localizationName, NULL
        );
    }
    if (nested != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native _mappedStringsFiles and _mappedStringsFilesLock (__DATA,__bss). Non-external.
static OCFMutableArrayRef _mappedStringsFiles;
static os_unfair_lock _mappedStringsFilesLock;

// Native _CFBundleGetMappedStringsFile, 0x1c02bf528. External. Answers the mapped data at the index under
// the mapped-strings lock (CFArrayGetValueAtIndex, not retained).
OCFDataRef _OCFBundleGetMappedStringsFile(OCFIndex index) {
    os_unfair_lock_lock_with_options(&_mappedStringsFilesLock, 0x10000);
    OCFDataRef result = OCFArrayGetValueAtIndex(_mappedStringsFiles, index);
    os_unfair_lock_unlock(&_mappedStringsFilesLock);
    return result;
}

// Native _CFBundleCreateMappedBPlistFile, 0x1c02bf578, with ____CFBundleCreateMappedBPlistFile_block_invoke
// (0x1c02bf618). Hidden (was private external). Foundation's __NSCreateBPlistMappedDataFromURL maps the file;
// its block, given the flag, enters each mapped data in _mappedStringsFiles (made on first use, system
// default allocator, kCFTypeArrayCallBacks) under its lock and answers the index, or -1 without the flag or
// for NULL data. An error stored by Foundation is released.
OCFDataRef _CFBundleCreateMappedBPlistFile(OCFURLRef url, Boolean flag) {
    OCFErrorRef error = NULL;
    OCFDataRef result = __ONSCreateBPlistMappedDataFromURL(
        url,
        ^OCFIndex(OCFDataRef data) {
          if (!flag) {
              return -1;
          }
          os_unfair_lock_lock_with_options(&_mappedStringsFilesLock, 0x10000);
          OCFIndex index = -1;
          if (data != NULL) {
              if (_mappedStringsFiles == NULL) {
                  _mappedStringsFiles =
                      OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
              }
              index = OCFArrayGetCount(_mappedStringsFiles);
              OCFArrayAppendValue(_mappedStringsFiles, data);
          }
          os_unfair_lock_unlock(&_mappedStringsFilesLock);
          return index;
        },
        &error
    );
    if (error != NULL) {
        OCFRelease(error);
    }
    return result;
}

// Native _CFBundleGetStringsSources, 0x1c02bf6b8, with its .cold.1 (0x1c03120b4, the debug log of an unused
// loctable). Hidden (was private external). Answers through X8. Under the lock at +104 the sources start
// zeroed (loadStringsDictFirst 1), or as a retained copy of the cache entry (+160) for the table. Unless a
// loctable data is already present, the loctable URL (a cached one retained, or CFBundleCopyResourceURL for
// "loctable" when not cached) is loaded with mapping allowed when not cached and either allowMapping or the
// flag "no localizations and not unique" holds, that flag being passed as well; a failure logs its
// unlocalized debug description at the error level and releases the error. With localizations, the file
// sources are dropped and the first localization with a .strings or .stringsdict resource supplies both URLs
// and languages; otherwise the first localization with a loctable variant (or, without one, the first
// localization) supplies the languages; a loctable then replaces the files whose version is older than its
// variant (_useLoctableInsteadOfFileBasedOnVersion), the loctable data is released when it replaces nothing,
// and a kept file is logged at the debug level. Without localizations, cached languages answer after the file
// data are loaded (dropping an unused loctable). Otherwise the default .strings and .stringsdict URLs are
// taken; with loctable data, the bundle's language search list is walked to choose, per kind, the file when
// its language matches and its version is not older than the loctable variant, else the loctable variant,
// stopping once both kinds are decided ("none" provenance counting as decided for a kind without a file);
// with no loctable variant used the loctable is marked unused (logged). Without loctable data the files'
// languages are used and loadStringsDictFirst is cleared when the .strings language comes first in the search
// list. The file data are then loaded and the result entered in the cache (as the default localization when
// no localizations were given).
StringsSources _CFBundleGetStringsSources(
    OCFBundleRef bundle, Boolean allowMapping, OCFStringRef tableName, OCFArrayRef localizations
) {
    StringsSources sources;
    os_unfair_lock_lock_with_options(&bundle->_lock104, 0x10000);
    memset(&sources, 0, sizeof(sources));
    Boolean notCached = true;
    sources.loadStringsDictFirst = 1;
    if (bundle->_stringsSources != NULL) {
        const StringsSources *cached = OCFDictionaryGetValue(bundle->_stringsSources, tableName);
        if (cached != NULL) {
            sources = *cached;
            _CFBundleRetainStringsSources(*cached);
            notCached = false;
        }
    }
    os_unfair_lock_unlock(&bundle->_lock104);
    Boolean unrestricted = localizations == NULL && !bundle->_isUnique;
    if (sources.locTableData == NULL) {
        OCFURLRef locTableURL = NULL;
        if (notCached) {
            locTableURL = OCFBundleCopyResourceURL(bundle, tableName, OF_STR("loctable"), NULL);
        } else if (sources.locTableURL != NULL) {
            locTableURL = OCFRetain(sources.locTableURL);
        }
        if (locTableURL != NULL) {
            Boolean mapping = (allowMapping || unrestricted) ? notCached : false;
            Boolean mapped = false;
            OCFErrorRef error = NULL;
            sources.locTableData =
                _CFBundleCreateMappedOrLoadedPlistData(locTableURL, mapping, unrestricted, &mapped, &error);
            if (sources.locTableData != NULL) {
                sources.locTableMapped = mapped;
            } else {
                OCFStringRef description = _OCFErrorCreateUnlocalizedDebugDescription(error);
                os_log_error(
                    _CFBundleLocalizedStringLogger(),
                    "loctable failed to load for bundle: %@, table: %@: %@",
                    bundle,
                    tableName,
                    description
                );
                OCFRelease(description);
                OCFRelease(error);
            }
            if (sources.locTableURL != NULL) {
                OCFRelease(sources.locTableURL);
            }
            sources.locTableURL = locTableURL;
        }
    }
    if (localizations != NULL) {
        if (sources.stringsLanguage != NULL) {
            OCFRelease(sources.stringsLanguage);
            sources.stringsLanguage = NULL;
        }
        if (sources.stringsURL != NULL) {
            OCFRelease(sources.stringsURL);
            sources.stringsURL = NULL;
        }
        if (sources.stringsData != NULL) {
            OCFRelease(sources.stringsData);
            sources.stringsData = NULL;
        }
        if (sources.stringsDictLanguage != NULL) {
            OCFRelease(sources.stringsDictLanguage);
            sources.stringsDictLanguage = NULL;
        }
        if (sources.stringsDictURL != NULL) {
            OCFRelease(sources.stringsDictURL);
            sources.stringsDictURL = NULL;
        }
        if (sources.stringsDictData != NULL) {
            OCFRelease(sources.stringsDictData);
            sources.stringsDictData = NULL;
        }
        for (OCFIndex index = 0; index < OCFArrayGetCount(localizations); index++) {
            OCFStringRef localization = OCFArrayGetValueAtIndex(localizations, index);
            OCFURLRef stringsURL = OCFBundleCopyResourceURLForLocalization(
                bundle, tableName, OF_STR("strings"), NULL, localization
            );
            OCFURLRef stringsDictURL = OCFBundleCopyResourceURLForLocalization(
                bundle, tableName, OF_STR("stringsdict"), NULL, localization
            );
            if (((uintptr_t)stringsURL | (uintptr_t)stringsDictURL) != 0) {
                sources.stringsURL = stringsURL;
                sources.stringsLanguage = OCFRetain(localization);
                sources.stringsDictURL = stringsDictURL;
                sources.stringsDictLanguage = OCFRetain(localization);
                break;
            }
        }
        if (sources.stringsLanguage == NULL) {
            Boolean useFirst;
            if (sources.locTableData != NULL) {
                if (OCFArrayGetCount(localizations) >= 1 && OCFArrayGetCount(localizations) >= 1) {
                    for (OCFIndex index = 0; index < OCFArrayGetCount(localizations); index++) {
                        OCFStringRef stringsVariant = NULL;
                        OCFStringRef stringsDictVariant = NULL;
                        uint32_t stringsVersion;
                        uint32_t stringsDictVersion;
                        _CFBundleCopyMostAppropriateLocTableDeviceAndPlatformSpecificVariants(
                            sources.locTableData,
                            OCFArrayGetValueAtIndex(localizations, index),
                            &stringsVariant,
                            &stringsVersion,
                            &stringsDictVariant,
                            &stringsDictVersion
                        );
                        if (((uintptr_t)stringsVariant | (uintptr_t)stringsDictVariant) != 0) {
                            sources.stringsLanguage = stringsVariant;
                            sources.stringsDictLanguage = stringsDictVariant;
                            break;
                        }
                    }
                }
                useFirst = ((uintptr_t)sources.stringsLanguage | (uintptr_t)sources.stringsDictLanguage) == 0;
            } else {
                useFirst = sources.stringsDictLanguage == NULL;
            }
            if (useFirst) {
                sources.stringsLanguage = OCFRetain(OCFArrayGetValueAtIndex(localizations, 0));
                sources.stringsDictLanguage = OCFRetain(OCFArrayGetValueAtIndex(localizations, 0));
            }
        }
        if (sources.locTableData != NULL) {
            OCFStringRef stringsVariant = NULL;
            uint32_t stringsVersion = 0;
            OCFStringRef stringsDictVariant = NULL;
            uint32_t stringsDictVersion = 0;
            OCFStringRef language =
                sources.stringsLanguage != NULL ? sources.stringsLanguage : sources.stringsDictLanguage;
            _CFBundleCopyMostAppropriateLocTableDeviceAndPlatformSpecificVariants(
                sources.locTableData,
                language,
                &stringsVariant,
                &stringsVersion,
                &stringsDictVariant,
                &stringsDictVersion
            );
            Boolean stringsReplaced = _useLoctableInsteadOfFileBasedOnVersion(
                &sources.stringsURL, &sources.stringsLanguage, stringsVersion, stringsVariant
            );
            Boolean stringsDictReplaced = _useLoctableInsteadOfFileBasedOnVersion(
                &sources.stringsDictURL, &sources.stringsDictLanguage, stringsDictVersion, stringsDictVariant
            );
            if (stringsVariant != NULL) {
                OCFRelease(stringsVariant);
            }
            if (stringsDictVariant != NULL) {
                OCFRelease(stringsDictVariant);
            }
            if (!(stringsReplaced | stringsDictReplaced)) {
                OCFRelease(sources.locTableData);
                sources.locTableData = NULL;
            }
            if ((!stringsReplaced && sources.stringsURL != NULL) ||
                (sources.stringsDictURL != NULL && !stringsDictReplaced)) {
                os_log_debug(
                    _CFBundleLocalizedStringLogger(),
                    "loctable overridden by installed files. Bundle: %@, table: %@, language: %@",
                    bundle,
                    tableName,
                    sources.stringsLanguage
                );
            }
        }
        _CFBundleLoadNonLocTableData(bundle, tableName, &sources, false, allowMapping);
        _CFBundleIngestResultForBundleCache(bundle, tableName, false, sources);
        return sources;
    }
    if (((uintptr_t)sources.stringsLanguage | (uintptr_t)sources.stringsDictLanguage) != 0) {
        if (sources.locTableUnused && sources.locTableData != NULL) {
            OCFRelease(sources.locTableData);
            sources.locTableData = NULL;
        }
        _CFBundleLoadNonLocTableData(bundle, tableName, &sources, false, allowMapping);
        return sources;
    }
    OCFURLRef stringsURL = OCFBundleCopyResourceURL(bundle, tableName, OF_STR("strings"), NULL);
    OCFURLRef stringsDictURL = OCFBundleCopyResourceURL(bundle, tableName, OF_STR("stringsdict"), NULL);
    OCFDataRef locTableData = sources.locTableData;
    if (locTableData != NULL) {
        OCFArrayRef languages = _CFBundleCopyLanguageSearchListInBundle(bundle);
        OCFIndex count = OCFArrayGetCount(languages);
        OCFStringRef stringsLanguage =
            stringsURL != NULL ? _CFBundleCopyLanguageForStringsResourceURL(stringsURL) : NULL;
        OCFStringRef stringsDictLanguage =
            stringsDictURL != NULL ? _CFBundleCopyLanguageForStringsResourceURL(stringsDictURL) : NULL;
        uint32_t stringsFileVersion = _CFBundleGetFileVersionForStringsResourceURL(stringsURL);
        uint32_t stringsDictFileVersion = _CFBundleGetFileVersionForStringsResourceURL(stringsDictURL);
        Boolean noneStrings = false;
        Boolean noneStringsDict = false;
        _CFBundleGetLocTableProvenanceForLanguage(
            locTableData, OF_STR("none"), &noneStrings, &noneStringsDict
        );
        Boolean usedLocTable = false;
        if (count - 1 >= 0) {
            Boolean stringsFound = false;
            Boolean stringsDictFound = false;
            Boolean stringsSettled = noneStrings && stringsLanguage == NULL;
            Boolean stringsDictSettled = noneStringsDict && stringsDictLanguage == NULL;
            for (OCFIndex index = 0;; index++) {
                OCFStringRef language = OCFArrayGetValueAtIndex(languages, index);
                Boolean skipStringsDict = stringsDictFound || noneStringsDict;
                OCFStringRef stringsVariant = NULL;
                uint32_t stringsVersion = 0;
                OCFStringRef stringsDictVariant = NULL;
                uint32_t stringsDictVersion = 0;
                if (!((stringsFound || noneStrings) && skipStringsDict)) {
                    _CFBundleCopyMostAppropriateLocTableDeviceAndPlatformSpecificVariants(
                        locTableData,
                        language,
                        &stringsVariant,
                        &stringsVersion,
                        &stringsDictVariant,
                        &stringsDictVersion
                    );
                }
                if (!stringsDictFound) {
                    if (stringsDictLanguage != NULL && OCFEqual(language, stringsDictLanguage) &&
                        stringsDictFileVersion >= stringsDictVersion) {
                        sources.stringsDictURL = OCFRetain(stringsDictURL);
                        sources.stringsDictLanguage = OCFRetain(language);
                        stringsDictFound = true;
                    } else if (stringsDictVariant != NULL) {
                        usedLocTable = true;
                        sources.stringsDictLanguage = OCFRetain(stringsDictVariant);
                        stringsDictFound = true;
                    }
                }
                Boolean stringsDecided;
                if (!stringsFound) {
                    if (stringsLanguage != NULL && OCFEqual(language, stringsLanguage) &&
                        stringsFileVersion >= stringsVersion) {
                        sources.stringsURL = OCFRetain(stringsURL);
                        sources.stringsLanguage = OCFRetain(language);
                        if (!stringsDictFound) {
                            sources.loadStringsDictFirst = 0;
                        }
                        stringsDecided = true;
                        stringsFound = true;
                    } else if (stringsVariant != NULL) {
                        usedLocTable = true;
                        sources.stringsLanguage = OCFRetain(stringsVariant);
                        if (!stringsDictFound) {
                            sources.loadStringsDictFirst = 0;
                        }
                        stringsDecided = true;
                        stringsFound = true;
                    } else {
                        stringsDecided = stringsSettled;
                    }
                } else {
                    stringsDecided = true;
                }
                if (stringsDictVariant != NULL) {
                    OCFRelease(stringsDictVariant);
                }
                if (stringsVariant != NULL) {
                    OCFRelease(stringsVariant);
                }
                if (stringsDecided && (stringsDictFound || stringsDictSettled)) {
                    break;
                }
                if (index == count - 1) {
                    break;
                }
            }
        }
        if (!usedLocTable) {
            os_log_debug(
                _CFBundleLocalizedStringLogger(),
                "loctable overridden by installed files. Bundle: %@, table: %@",
                bundle,
                tableName
            );
            sources.locTableUnused = 1;
        }
        if (stringsLanguage != NULL) {
            OCFRelease(stringsLanguage);
        }
        if (stringsDictLanguage != NULL) {
            OCFRelease(stringsDictLanguage);
        }
        OCFRelease(languages);
    } else {
        if (stringsURL != NULL) {
            sources.stringsURL = OCFRetain(stringsURL);
            sources.stringsLanguage = _CFBundleCopyLanguageForStringsResourceURL(stringsURL);
        }
        if (stringsDictURL != NULL) {
            sources.stringsDictURL = OCFRetain(stringsDictURL);
            sources.stringsDictLanguage = _CFBundleCopyLanguageForStringsResourceURL(stringsDictURL);
        }
        if (sources.stringsLanguage != NULL && sources.stringsDictLanguage != NULL &&
            !OCFEqual(sources.stringsLanguage, sources.stringsDictLanguage)) {
            OCFArrayRef languages = _CFBundleCopyLanguageSearchListInBundle(bundle);
            OCFIndex count = OCFArrayGetCount(languages);
            OCFIndex stringsIndex =
                OCFArrayGetFirstIndexOfValue(languages, (OCFRange){0, count}, sources.stringsLanguage);
            if (stringsIndex <
                OCFArrayGetFirstIndexOfValue(languages, (OCFRange){0, count}, sources.stringsDictLanguage)) {
                sources.loadStringsDictFirst = 0;
            }
            OCFRelease(languages);
        }
    }
    if (stringsURL != NULL) {
        OCFRelease(stringsURL);
    }
    if (stringsDictURL != NULL) {
        OCFRelease(stringsDictURL);
    }
    _CFBundleLoadNonLocTableData(bundle, tableName, &sources, true, allowMapping);
    _CFBundleIngestResultForBundleCache(bundle, tableName, true, sources);
    if (sources.locTableUnused && sources.locTableData != NULL) {
        OCFRelease(sources.locTableData);
        sources.locTableData = NULL;
    }
    return sources;
}

// Native _CFBundleRetainStringsSources, 0x1c02c00a8. Non-external. Retains the eight object fields in order,
// skipping NULL ones.
static void _CFBundleRetainStringsSources(StringsSources sources) {
    if (sources.stringsURL != NULL) {
        OCFRetain(sources.stringsURL);
    }
    if (sources.stringsLanguage != NULL) {
        OCFRetain(sources.stringsLanguage);
    }
    if (sources.stringsData != NULL) {
        OCFRetain(sources.stringsData);
    }
    if (sources.stringsDictURL != NULL) {
        OCFRetain(sources.stringsDictURL);
    }
    if (sources.stringsDictLanguage != NULL) {
        OCFRetain(sources.stringsDictLanguage);
    }
    if (sources.stringsDictData != NULL) {
        OCFRetain(sources.stringsDictData);
    }
    if (sources.locTableURL != NULL) {
        OCFRetain(sources.locTableURL);
    }
    if (sources.locTableData != NULL) {
        OCFRetain(sources.locTableData);
    }
}

// Native _CFBundleLoadNonLocTableData, 0x1c02c012c. Non-external. Mapping is disallowed for a unique bundle
// (+54). Missing .strings and .stringsdict data are loaded from their URLs (recording whether they were
// mapped); a failure logs the error at the error level of the localized-string logger and releases it.
static void _CFBundleLoadNonLocTableData(
    OCFBundleRef bundle, OCFStringRef tableName, StringsSources *sources, Boolean allowMapping, Boolean flag
) {
    Boolean mapping = bundle->_isUnique == 0 ? allowMapping : false;
    OCFErrorRef error = NULL;
    if (sources->stringsData == NULL && sources->stringsURL != NULL) {
        sources->stringsData = _CFBundleCreateMappedOrLoadedPlistData(
            sources->stringsURL, mapping, flag, (Boolean *)&sources->stringsMapped, &error
        );
        if (sources->stringsData == NULL) {
            os_log_error(
                _CFBundleLocalizedStringLogger(),
                "Unable to load .strings file: %@ / %@: %@",
                bundle,
                tableName,
                error
            );
            if (error != NULL) {
                OCFRelease(error);
            }
            error = NULL;
        }
    }
    if (sources->stringsDictData == NULL && sources->stringsDictURL != NULL) {
        sources->stringsDictData = _CFBundleCreateMappedOrLoadedPlistData(
            sources->stringsDictURL, mapping, flag, (Boolean *)&sources->stringsDictMapped, &error
        );
        if (sources->stringsDictData == NULL) {
            os_log_error(
                _CFBundleLocalizedStringLogger(),
                "Unable to load .stringsdict file: %@ / %@: %@",
                bundle,
                tableName,
                error
            );
            if (error != NULL) {
                OCFRelease(error);
            }
        }
    }
}

// Native _CFBundleIngestResultForBundleCache, 0x1c02c02ec, with its .cold.1 through .cold.4 assertions
// (__assert_rtn in _CFBundleIngestDefaultLocalizationResultsIntoExistingCachedResult, CFBundle_Strings.m
// lines 784, 785, 794 and 795). Non-external. Under the lock at +104: the cache at +160 is made on first use
// (system default allocator, kCFCopyStringDictionaryKeyCallBacks, value callbacks {0, NULL,
// _releaseStringsSource, NULL, NULL} copied from 0x2227a9f98). For the default localization, an existing
// entry without a .strings (or .stringsdict) language takes the new one, its URL and its data when mapped
// (asserting that the entry has neither URL nor data). Without an entry, a malloc'd copy (72 bytes, type
// 0x1060040C3D82AC4) keeps only mapped data, and only the loctable fields for a non-default result; it is
// retained and entered.
// Folds the compiler-outlined pieces __CFBundleIngestResultForBundleCache.cold.1 (0x1c03120f4),
// __CFBundleIngestResultForBundleCache.cold.2 (0x1c031211c), __CFBundleIngestResultForBundleCache.cold.3
// (0x1c0312144), __CFBundleIngestResultForBundleCache.cold.4 (0x1c031216c).
static void _CFBundleIngestResultForBundleCache(
    OCFBundleRef bundle, OCFStringRef tableName, Boolean isDefault, StringsSources sources
) {
    os_unfair_lock_lock_with_options(&bundle->_lock104, 0x10000);
    StringsSources *existing = NULL;
    if (bundle->_stringsSources != NULL) {
        existing = (StringsSources *)OCFDictionaryGetValue(bundle->_stringsSources, tableName);
        if (isDefault && existing != NULL) {
            OCFStringRef stringsLanguage = sources.stringsLanguage;
            OCFURLRef stringsDictURL = sources.stringsDictURL;
            OCFStringRef stringsDictLanguage = sources.stringsDictLanguage;
            OCFDataRef stringsDictData = sources.stringsDictData;
            uint8_t stringsDictMapped = sources.stringsDictMapped;
            if (existing->stringsLanguage == NULL && stringsLanguage != NULL) {
                if (existing->stringsURL != NULL) {
                    __assert_rtn(
                        "_CFBundleIngestDefaultLocalizationResultsIntoExistingCachedResult",
                        "CFBundle_Strings.m",
                        784,
                        "!existingCachedResult->stringsTableURL"
                    );
                }
                if (existing->stringsData != NULL) {
                    __assert_rtn(
                        "_CFBundleIngestDefaultLocalizationResultsIntoExistingCachedResult",
                        "CFBundle_Strings.m",
                        785,
                        "!existingCachedResult->stringsData"
                    );
                }
                OCFURLRef stringsURL = sources.stringsURL;
                if (sources.stringsData != NULL && sources.stringsMapped) {
                    existing->stringsData = OCFRetain(sources.stringsData);
                    existing->stringsMapped = 1;
                }
                if (stringsURL != NULL) {
                    existing->stringsURL = OCFRetain(stringsURL);
                }
                existing->stringsLanguage = OCFRetain(stringsLanguage);
            }
            if (existing->stringsDictLanguage == NULL && stringsDictLanguage != NULL) {
                if (existing->stringsDictURL != NULL) {
                    __assert_rtn(
                        "_CFBundleIngestDefaultLocalizationResultsIntoExistingCachedResult",
                        "CFBundle_Strings.m",
                        794,
                        "!existingCachedResult->stringsDictTableURL"
                    );
                }
                if (existing->stringsDictData != NULL) {
                    __assert_rtn(
                        "_CFBundleIngestDefaultLocalizationResultsIntoExistingCachedResult",
                        "CFBundle_Strings.m",
                        795,
                        "!existingCachedResult->stringsDictData"
                    );
                }
                if (stringsDictData != NULL && stringsDictMapped) {
                    existing->stringsDictData = OCFRetain(stringsDictData);
                    existing->stringsDictMapped = 1;
                }
                if (stringsDictURL != NULL) {
                    existing->stringsDictURL = OCFRetain(stringsDictURL);
                }
                existing->stringsDictLanguage = OCFRetain(stringsDictLanguage);
            }
            os_unfair_lock_unlock(&bundle->_lock104);
            return;
        }
        if (existing != NULL) {
            os_unfair_lock_unlock(&bundle->_lock104);
            return;
        }
    } else {
        OCFDictionaryValueCallBacks valueCallBacks = {0, NULL, _releaseStringsSource, NULL, NULL};
        bundle->_stringsSources = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFCopyStringDictionaryKeyCallBacks, &valueCallBacks
        );
    }
    StringsSources *entry = malloc_type_malloc(72, 0x1060040C3D82AC4);
    if (entry != NULL) {
        *entry = sources;
        if (!isDefault) {
            entry->stringsMapped = 0;
            entry->stringsDictMapped = 0;
            entry->stringsURL = NULL;
            entry->stringsLanguage = NULL;
            entry->stringsDictURL = NULL;
            entry->stringsDictLanguage = NULL;
            entry->stringsData = NULL;
        } else if (!sources.stringsMapped) {
            entry->stringsData = NULL;
        }
        if (!(sources.stringsDictMapped && isDefault)) {
            entry->stringsDictData = NULL;
        }
        if (!sources.locTableMapped) {
            entry->locTableData = NULL;
        }
        _CFBundleRetainStringsSources(*entry);
        OCFDictionarySetValue(bundle->_stringsSources, tableName, entry);
    }
    os_unfair_lock_unlock(&bundle->_lock104);
}

// Native _CFBundleCopyLocalizedStringForLocalizationTableURLAndMarkdownOption.capitalize, .once, .once.11,
// .doubleStrings, .accentuateStrings, .surroundStrings, .forceRightToLeftStrings and .tallStrings
// (__DATA,__bss 0x22e6d3b38-0x22e6d3b50). Non-external.
static dispatch_once_t capitalizeOnce;
static uint8_t capitalize;
static dispatch_once_t pseudolocalizationOnce;
static uint8_t doubleStrings;
static uint8_t accentuateStrings;
static uint8_t surroundStrings;
static uint8_t forceRightToLeftStrings;
static uint8_t tallStrings;

// Native _CFBundleCopyLocalizedStringForLocalizationTableURLAndMarkdownOption, 0x1c02c05c4, with its .cold.1
// and .cold.2 (0x1c0312194 and 0x1c03121b8, the dispatch_once calls) and the blocks
// ____CFBundleCopyLocalizedStringForLocalizationTableURLAndMarkdownOption_block_invoke (0x1c02c14dc) and
// _block_invoke.12 (0x1c02c1518). External. The entry-point pattern (line 1362). With CFSwiftBundle the
// answer of -_localizedStringForKey:value:table:bundleLocalizations:localization:preventMarkdownParsing: (the
// URL pointer passed as the localization argument) is retained, and so is a localization it stored. Otherwise
// the bundle must have type 31. A NULL key answers the value (or "") retained.
// CFBundleAllowMixedLocalizations is consulted (its answer is unused); a NULL or empty table means
// "Localizable". The value for the key comes from _copyStringTable with a one-key NSSet; missing, it is the
// value unless NULL or "" (then the key), and with NSShowNonLocalizedStrings an uppercased mutable copy,
// logged as an error. The NSDoubleLocalizedStrings, NSAccentuateLocalizedStrings,
// NSForceRightToLeftLocalizedStrings, NSSurroundLocalizedStrings ("[# ", " #]") and NSTallLocalizedStrings (a
// nonempty string) preferences transform a nonempty result in that order. The localization is the
// last-visited ".lproj" path component (reverse enumeration; at least seven units, the suffix removed) of the
// URL _copyStringTable stored; without preventMarkdownParsing the result is parsed with it. It is stored in
// *outLocalization, or released. The debug log shows the localization names (only when given) and the result,
// "<same as key>" or "<same as value>" when identical.
OCFStringRef _OCFBundleCopyLocalizedStringForLocalizationTableURLAndMarkdownOption(
    OCFBundleRef bundle,
    OCFStringRef key,
    OCFStringRef value,
    OCFStringRef tableName,
    OCFArrayRef localizations,
    Boolean preventMarkdownParsing,
    OCFTypeRef *outLocalization
) {
    void *nested = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        outermost,
        kSourcePath,
        1362,
        OF_NAME_STRING(_OCFBundleCopyLocalizedStringForLocalizationTableURLAndMarkdownOption)
    );
    OCFStringRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((OCFStringRef (*)(
            id, SEL, OCFStringRef, OCFStringRef, OCFStringRef, OCFArrayRef, OCFTypeRef *, BOOL
        ))objc_msgSend)(
            (id)bundle,
            sel_getUid(
                "_localizedStringForKey:value:table:bundleLocalizations:localization:preventMarkdownParsing:"
            ),
            key,
            value,
            tableName,
            localizations,
            outLocalization,
            preventMarkdownParsing != 0
        );
        if (result != NULL) {
            OCFRetain(result);
        }
        if (outLocalization != NULL && *outLocalization != NULL) {
            OCFRetain(*outLocalization);
        }
    } else {
        OCFTypeID typeID = _CFGetNonObjCTypeID(bundle);
        if (typeID != 31) {
            _CFAssertMismatchedTypeID(31, typeID);
        }
        if (key == NULL) {
            result = OCFRetain(value != NULL ? value : OF_STR(""));
        } else {
            CFBundleAllowMixedLocalizations();
            if (tableName == NULL || OCFEqual(tableName, OF_STR(""))) {
                tableName = OF_STR("Localizable");
            }
            id keySet = ((id (*)(id, SEL, const void **, unsigned long))objc_msgSend)(
                objc_alloc((Class)setClass), sel_getUid("initWithObjects:count:"), (const void **)&key, 1
            );
            OCFTypeRef stringsURL = NULL;
            OCFDictionaryRef table = _copyStringTable(
                bundle, tableName, (OCFSetRef)keySet, localizations, preventMarkdownParsing, &stringsURL
            );
            OCFStringRef found = NULL;
            if (table != NULL) {
                found = OCFDictionaryGetValue(table, key);
                if (found != NULL) {
                    OCFRetain(found);
                }
                OCFRelease(table);
            }
            objc_release(keySet);
            result = found;
            if (result == NULL) {
                OCFStringRef fallback;
                if (value != NULL) {
                    fallback = OCFEqual(value, OF_STR("")) ? key : value;
                } else {
                    fallback = key;
                }
                OCFStringRef retained = OCFRetain(fallback);
                dispatch_once(&capitalizeOnce, ^{
                  capitalize =
                      OCFPreferencesGetAppBooleanValue(
                          OF_STR("NSShowNonLocalizedStrings"), kOCFPreferencesCurrentApplication, NULL
                      ) != 0;
                });
                if (capitalize) {
                    OCFMutableStringRef uppercased =
                        OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, retained);
                    os_log_error(
                        _CFBundleLocalizedStringLogger(),
                        "ERROR: %@ not found in table %@ of bundle %@",
                        key,
                        tableName,
                        bundle
                    );
                    OCFStringUppercase(uppercased, NULL);
                    OCFRelease(retained);
                    result = uppercased;
                } else {
                    result = retained;
                }
            }
            dispatch_once(&pseudolocalizationOnce, ^{
              OCFStringRef application = kOCFPreferencesCurrentApplication;
              if (OCFPreferencesGetAppBooleanValue(OF_STR("NSDoubleLocalizedStrings"), application, NULL)) {
                  doubleStrings = 1;
              }
              if (OCFPreferencesGetAppBooleanValue(
                      OF_STR("NSForceRightToLeftLocalizedStrings"), application, NULL
                  )) {
                  forceRightToLeftStrings = 1;
              }
              if (OCFPreferencesGetAppBooleanValue(
                      OF_STR("NSAccentuateLocalizedStrings"), application, NULL
                  )) {
                  accentuateStrings = 1;
              }
              if (OCFPreferencesGetAppBooleanValue(OF_STR("NSSurroundLocalizedStrings"), application, NULL)) {
                  surroundStrings = 1;
              }
              OCFPropertyListRef tall =
                  OCFPreferencesCopyAppValue(OF_STR("NSTallLocalizedStrings"), application);
              if (tall != NULL) {
                  if (OCFGetTypeID(tall) == OCFStringGetTypeID() &&
                      OCFStringGetLength((OCFStringRef)tall) != 0) {
                      tallStrings = 1;
                  }
                  OCFRelease(tall);
              }
            });
            if (OCFStringGetLength(result) >= 1) {
                if (doubleStrings == 1) {
                    OCFStringRef transformed = _OCFDoubledStringCreate(result);
                    OCFRelease(result);
                    result = transformed;
                }
                if (accentuateStrings == 1) {
                    OCFStringRef transformed = _OCFAccentuatedStringCreate(result);
                    OCFRelease(result);
                    result = transformed;
                }
                if (forceRightToLeftStrings == 1) {
                    OCFStringRef transformed = _OCFRLORightToLeftStringCreate(result);
                    OCFRelease(result);
                    result = transformed;
                }
                if (surroundStrings == 1) {
                    OCFStringRef transformed = _OCFAffixedStringCreate(result, OF_STR("[# "), OF_STR(" #]"));
                    OCFRelease(result);
                    result = transformed;
                }
                if (tallStrings == 1) {
                    OCFStringRef transformed = _OCFTallStringCreate(result);
                    OCFRelease(result);
                    result = transformed;
                }
            }
            OCFTypeRef localization = NULL;
            if (stringsURL != NULL) {
                id components = ((id (*)(id, SEL))objc_msgSend)((id)stringsURL, sel_getUid("pathComponents"));
                FastEnumerationState state;
                memset(&state, 0, sizeof(state));
                id buffer[16];
                id enumerator =
                    ((id (*)(id, SEL))objc_msgSend)(components, sel_getUid("reverseObjectEnumerator"));
                SEL countSelector = sel_getUid("countByEnumeratingWithState:objects:count:");
                unsigned long count =
                    ((unsigned long (*)(id, SEL, FastEnumerationState *, id *, unsigned long))objc_msgSend)(
                        enumerator, countSelector, &state, buffer, 16
                    );
                if (count != 0) {
                    unsigned long mutations = *state.mutationsPtr;
                    do {
                        for (unsigned long index = 0; index != count; index++) {
                            if (*state.mutationsPtr != mutations) {
                                objc_enumerationMutation(enumerator);
                            }
                            id component = state.itemsPtr[index];
                            if (((BOOL (*)(id, SEL, OCFStringRef))objc_msgSend)(
                                    component, sel_getUid("hasSuffix:"), OF_STR(".lproj")
                                ) &&
                                ((unsigned long (*)(id, SEL))objc_msgSend)(component, sel_getUid("length")) >=
                                    7) {
                                if (localization != NULL) {
                                    OCFRelease(localization);
                                }
                                unsigned long length = ((unsigned long (*)(id, SEL))objc_msgSend)(
                                    component, sel_getUid("length")
                                );
                                localization =
                                    OCFRetain(((OCFTypeRef (*)(id, SEL, unsigned long))objc_msgSend)(
                                        component, sel_getUid("substringToIndex:"), length - 6
                                    ));
                            }
                        }
                        count = ((unsigned long (*)(id, SEL, FastEnumerationState *, id *, unsigned long))
                                     objc_msgSend)(enumerator, countSelector, &state, buffer, 16);
                    } while (count != 0);
                }
                OCFRelease(stringsURL);
            }
            if (!preventMarkdownParsing && result != NULL) {
                OCFStringRef parsed = _OCFStringCreateByParsingMarkdownAndInflectingIfNeeded(
                    result, bundle, (OCFStringRef)localization
                );
                OCFRelease(result);
                result = parsed;
            }
            if (outLocalization != NULL) {
                *outLocalization = localization;
            } else if (localization != NULL) {
                OCFRelease(localization);
            }
            OCFMutableStringRef names = NULL;
            if (localizations != NULL &&
                os_log_type_enabled(_CFBundleLocalizedStringLogger(), OS_LOG_TYPE_DEBUG)) {
                names = OCFStringCreateMutable(kOCFAllocatorDefault, 0);
                OCFStringAppend(names, OF_STR("["));
                for (OCFIndex index = 0; index < OCFArrayGetCount(localizations); index++) {
                    if (index != 0) {
                        OCFStringAppend(names, OF_STR(", "));
                    }
                    OCFStringAppend(names, OCFArrayGetValueAtIndex(localizations, index));
                }
                OCFStringAppend(names, OF_STR("]"));
            }
            if (os_log_type_enabled(_CFBundleLocalizedStringLogger(), OS_LOG_TYPE_DEBUG)) {
                OCFStringRef shown = result == value ? OF_STR("<same as value>") : result;
                if (result == key) {
                    shown = OF_STR("<same as key>");
                }
                os_log_debug(
                    _CFBundleLocalizedStringLogger(),
                    "Bundle: %{private}@, key: %@, value: %@, table: %{public}@, localizationNames: "
                    "%{public}@, "
                    "result: %{public}@",
                    bundle,
                    key,
                    value,
                    tableName,
                    names,
                    shown
                );
            }
            if (names != NULL) {
                OCFRelease(names);
            }
        }
    }
    if (nested != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native _copyStringTable, 0x1c02c0d38, with ____stringTableFromCacheSatisfyingRequest_block_invoke
// (0x1c02c2d90) and ____copyIngestedCacheResult_block_invoke and _block_invoke_2 (0x1c02c3d44, 0x1c02c3d74)
// of its inlined helpers. Non-external. The keys are retained. A table name with the suffix ".nocache" skips
// the cache only for executables linked on or after 5 (_CFExecutableLinkedOnOrAfter) without localizations;
// any table with localizations skips it. Otherwise, under the lock at +104: a table in the complete set
// (+144) answers a copy of its cached strings (+136), or an empty dictionary when none are cached; with keys,
// cached strings that hold every key answer a copy, and the keys neither cached nor known missing (+152) are
// logged as lazy cache misses and become the keys to load; without keys, uncached tables load all keys. A
// cached answer clears *outURL. Loading: the strings sources (mapping allowed) mark which kinds are present
// (and whether any present data is unmapped); stringsdict (or the loctable with the stringsdict language)
// loads before or after the .strings entries as loadStringsDictFirst says; .strings entries come from the
// data (storing the .strings URL in *outURL when the load matched and none is stored) or else from the
// loctable with the strings language; dictionary values run the platform-specific, localized-format and
// variable-width rule appliers. A cacheable result is merged into the cache under the lock (created with the
// bundle's allocator): with keys, every loaded pair is added and the keys not loaded are recorded as missing;
// without keys a nonempty result is added, and an empty one, or no source at all, marks the table complete
// and drops its cached strings; the answer is then a copy of the cached strings. An uncacheable result is
// answered itself.
static OCFDictionaryRef _copyStringTable(
    OCFBundleRef bundle,
    OCFStringRef tableName,
    OCFSetRef keys,
    OCFArrayRef localizations,
    Boolean preventMarkdownParsing,
    OCFTypeRef *outURL
) {
    OCFSetRef requestedKeys = keys != NULL ? OCFRetain(keys) : NULL;
    Boolean cacheable = false;
    Boolean useCache;
    if (OCFStringHasSuffix(tableName, OF_STR(".nocache"))) {
        Boolean linked = _OCFExecutableLinkedOnOrAfter(5);
        useCache = localizations == NULL && !linked;
    } else {
        useCache = localizations == NULL;
    }
    if (useCache) {
        os_unfair_lock_lock_with_options(&bundle->_lock104, 0x10000);
        OCFDictionaryRef cached = NULL;
        if (bundle->_stringTables != NULL) {
            cached = OCFDictionaryGetValue(bundle->_stringTables, tableName);
        }
        OCFDictionaryRef answer = NULL;
        OCFSetRef keysToLoad = NULL;
        if (cached != NULL) {
            if (bundle->_completeTables != NULL && OCFSetContainsValue(bundle->_completeTables, tableName)) {
                answer = OCFDictionaryCreateCopy(kOCFAllocatorSystemDefault, cached);
            } else if (requestedKeys != NULL) {
                __block OCFMutableSetRef missingKeys = NULL;
                CFSetApply(requestedKeys, ^(const void *requestedKey, Boolean *stop) {
                  (void)stop;
                  if (OCFDictionaryContainsKey(cached, requestedKey)) {
                      return;
                  }
                  if (bundle->_missingKeys != NULL) {
                      OCFSetRef known = OCFDictionaryGetValue(bundle->_missingKeys, tableName);
                      if (known != NULL && OCFSetContainsValue(known, requestedKey)) {
                          return;
                      }
                  }
                  if (missingKeys == NULL) {
                      missingKeys = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeSetCallBacks);
                  }
                  OCFSetAddValue(missingKeys, requestedKey);
                  os_log_debug(
                      _CFBundleLocalizedStringLogger(),
                      "Lazy cache miss for bundle: %@ key: %@ table: %@",
                      bundle,
                      requestedKey,
                      tableName
                  );
                });
                keysToLoad = missingKeys;
                if (keysToLoad == NULL) {
                    answer = OCFDictionaryCreateCopy(kOCFAllocatorSystemDefault, cached);
                }
            }
        } else if (
            bundle->_completeTables != NULL && OCFSetContainsValue(bundle->_completeTables, tableName)
        ) {
            answer = OCFDictionaryCreate(
                kOCFAllocatorSystemDefault,
                NULL,
                NULL,
                0,
                &kOCFTypeDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
        } else if (requestedKeys != NULL) {
            keysToLoad = OCFRetain(requestedKeys);
        }
        os_unfair_lock_unlock(&bundle->_lock104);
        if (answer != NULL) {
            if (outURL != NULL) {
                *outURL = NULL;
            }
            if (requestedKeys != NULL) {
                OCFRelease(requestedKeys);
            }
            return answer;
        }
        if (requestedKeys != NULL) {
            OCFRelease(requestedKeys);
        }
        cacheable = true;
        requestedKeys = keysToLoad;
    }
    StringsRequest request;
    memset(&request, 0, sizeof(request));
    request.sources = _CFBundleGetStringsSources(bundle, true, tableName, localizations);
    request.cacheable = cacheable;
    request.someDataUnmapped = 0;
    request.loadedWholeTable = 0;
    request.hasDictionaryValues = 0;
    request.hasStrings = 0;
    request.hasStringsDict = 0;
    request.hasLocTable = 0;
    request._padding79 = 0;
    request.locTableLanguage = NULL;
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    request.stringsTable = OCFDictionaryCreateMutable(
        allocator, 0, &kOCFCopyStringDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    if (request.sources.stringsData != NULL) {
        if (!request.sources.stringsMapped) {
            request.someDataUnmapped = 1;
        }
        request.hasStrings = 1;
    }
    if (request.sources.stringsDictData != NULL) {
        if (!request.sources.stringsDictMapped) {
            request.someDataUnmapped = 1;
        }
        request.hasStringsDict = 1;
    }
    if (request.sources.locTableData != NULL) {
        if (!request.sources.locTableMapped) {
            request.someDataUnmapped = 1;
        }
        request.hasLocTable = 1;
    }
    OCFMutableDictionaryRef result = OCFDictionaryCreateMutable(
        allocator, 0, &kOCFCopyStringDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    if (request.sources.loadStringsDictFirst) {
        if (request.hasStringsDict) {
            _loadStringsDictTable(bundle, tableName, requestedKeys, result, &request, outURL);
        } else if (request.hasLocTable) {
            _loadLocTable(
                bundle,
                tableName,
                request.sources.stringsDictLanguage,
                requestedKeys,
                result,
                &request,
                outURL
            );
        }
    }
    if (request.hasStrings) {
        Boolean loaded = _loadStringsFromData(
            bundle,
            tableName,
            requestedKeys,
            request.sources.stringsData,
            request.sources.stringsMapped,
            request.stringsTable,
            &request
        );
        if (outURL != NULL && loaded && *outURL == NULL) {
            *outURL = OCFRetain(request.sources.stringsURL);
        }
        OCFDictionaryApplyFunction(request.stringsTable, __CFStringsDictAddFunction, result);
    } else if (request.hasLocTable) {
        _loadLocTable(
            bundle, tableName, request.sources.stringsLanguage, requestedKeys, result, &request, outURL
        );
    }
    if (!request.sources.loadStringsDictFirst) {
        if (request.hasStringsDict) {
            _loadStringsDictTable(bundle, tableName, requestedKeys, result, &request, outURL);
        } else if (request.hasLocTable) {
            _loadLocTable(
                bundle,
                tableName,
                request.sources.stringsDictLanguage,
                requestedKeys,
                result,
                &request,
                outURL
            );
        }
    }
    if (request.hasDictionaryValues) {
        ApplierContext context;
        context.stringsTable = request.stringsTable;
        context.result = result;
        context.bundle = bundle;
        context.stringsDictURL = request.sources.stringsDictURL;
        context.preventMarkdownParsing = preventMarkdownParsing;
        memset(context._padding33, 0, sizeof(context._padding33));
        OCFDictionaryApplyFunction(result, __CFPlatformSpecificStringDictionaryApplyFunction, &context);
        OCFDictionaryApplyFunction(result, __CFLocalizedFormatStringApplyFunction, &context);
        OCFDictionaryApplyFunction(result, __CFVariableWidthStringDictionaryApplyFunction, &context);
    }
    OCFDictionaryRef answer;
    if (!cacheable) {
        answer = result;
    } else {
        os_unfair_lock_lock_with_options(&bundle->_lock104, 0x10000);
        if (bundle->_stringTables == NULL) {
            bundle->_stringTables = OCFDictionaryCreateMutable(
                OCFGetAllocator(bundle),
                0,
                &kOCFCopyStringDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
        }
        OCFMutableDictionaryRef cached =
            (OCFMutableDictionaryRef)OCFDictionaryGetValue(bundle->_stringTables, tableName);
        if (cached == NULL) {
            cached = OCFDictionaryCreateMutable(
                OCFGetAllocator(bundle),
                0,
                &kOCFCopyStringDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
            OCFDictionarySetValue(bundle->_stringTables, tableName, cached);
            OCFRelease(cached);
        }
        if (!request.hasStrings && !request.hasStringsDict && !request.hasLocTable) {
            request.loadedWholeTable = 1;
            OCFDictionaryRemoveValue(bundle->_stringTables, tableName);
            cached = NULL;
        } else if (keys != NULL) {
            OCFMutableSetRef unloadedKeys = OCFSetCreateMutableCopy(allocator, 0, keys);
            CFDictionaryApply(result, ^(const void *loadedKey, const void *loadedValue, Boolean *stop) {
              (void)stop;
              OCFDictionaryAddValue(cached, loadedKey, loadedValue);
              OCFSetRemoveValue(unloadedKeys, loadedKey);
            });
            if (OCFSetGetCount(unloadedKeys) >= 1) {
                if (bundle->_missingKeys == NULL) {
                    bundle->_missingKeys = OCFDictionaryCreateMutable(
                        allocator, 0, &kOCFCopyStringDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
                    );
                }
                OCFMutableSetRef known =
                    (OCFMutableSetRef)OCFDictionaryGetValue(bundle->_missingKeys, tableName);
                if (known == NULL) {
                    known = OCFSetCreateMutable(allocator, 0, &kOCFCopyStringSetCallBacks);
                    OCFDictionarySetValue(bundle->_missingKeys, tableName, known);
                    OCFRelease(known);
                }
                CFSetApply(unloadedKeys, ^(const void *unloadedKey, Boolean *stop) {
                  (void)stop;
                  OCFSetAddValue(known, unloadedKey);
                });
            }
            OCFRelease(unloadedKeys);
        } else if (OCFDictionaryGetCount(result) >= 1) {
            OCFDictionaryApplyFunction(result, __CFStringsDictAddFunction, cached);
        } else {
            request.loadedWholeTable = 1;
            OCFDictionaryRemoveValue(bundle->_stringTables, tableName);
            cached = NULL;
        }
        if (request.loadedWholeTable) {
            if (bundle->_completeTables == NULL) {
                bundle->_completeTables = OCFSetCreateMutable(allocator, 0, &kOCFCopyStringSetCallBacks);
            }
            OCFSetAddValue(bundle->_completeTables, tableName);
        }
        answer = cached != NULL ? OCFDictionaryCreateCopy(allocator, cached) : NULL;
        os_unfair_lock_unlock(&bundle->_lock104);
        OCFRelease(result);
    }
    _CFBundleReleaseStringsSources(request.sources);
    OCFRelease(request.stringsTable);
    if (request.locTableLanguage != NULL) {
        OCFRelease(request.locTableLanguage);
    }
    if (requestedKeys != NULL) {
        OCFRelease(requestedKeys);
    }
    return answer;
}

// Native _CFBundleCopyLocalizedStringForLocalizationAndTableURL, 0x1c02c1620. External. A localization
// becomes a one-element array (kCFAllocatorDefault, kCFTypeArrayCallBacks); the Markdown option is 0.
OCFStringRef _OCFBundleCopyLocalizedStringForLocalizationAndTableURL(
    OCFBundleRef bundle,
    OCFStringRef key,
    OCFStringRef value,
    OCFStringRef tableName,
    OCFStringRef localizationName,
    OCFTypeRef *outLocalization
) {
    const void *values[1] = {localizationName};
    if (localizationName == NULL) {
        return _OCFBundleCopyLocalizedStringForLocalizationTableURLAndMarkdownOption(
            bundle, key, value, tableName, NULL, false, outLocalization
        );
    }
    OCFArrayRef localizations = OCFArrayCreate(kOCFAllocatorDefault, values, 1, &kOCFTypeArrayCallBacks);
    OCFStringRef result = _OCFBundleCopyLocalizedStringForLocalizationTableURLAndMarkdownOption(
        bundle, key, value, tableName, localizations, false, outLocalization
    );
    if (localizations != NULL) {
        OCFRelease(localizations);
    }
    return result;
}

// Native CFBundleCopyLocalizedStringTableForLocalization, 0x1c02c18c4. External.
// The slot-18 prelude (line 1582). With CFSwiftBundle the answer of -localizedStringsForTable:localization:
// is retained. Otherwise (type 31 asserted) CFBundleAllowMixedLocalizations is called (its answer unused), a
// NULL or empty table is "Localizable", a localization becomes a one-element array (kCFAllocatorDefault,
// CFType callbacks), and _copyStringTable (no keys, Markdown parsed, no URL) answers the table, an empty
// immutable dictionary (system default, CFType callbacks) when it answers NULL.
OCFDictionaryRef OCFBundleCopyLocalizedStringTableForLocalization(
    OCFBundleRef bundle, OCFStringRef tableName, OCFStringRef localizationName
) {
    void *nested = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        outermost, kSourcePath, 1582, OF_NAME_STRING(OCFBundleCopyLocalizedStringTableForLocalization)
    );
    OCFDictionaryRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((OCFDictionaryRef (*)(id, SEL, OCFStringRef, OCFStringRef))objc_msgSend)(
            (id)bundle, sel_getUid("localizedStringsForTable:localization:"), tableName, localizationName
        );
        if (result != NULL) {
            OCFRetain(result);
        }
    } else {
        OCFTypeID typeID = _CFGetNonObjCTypeID(bundle);
        if (typeID != 31) {
            _CFAssertMismatchedTypeID(31, typeID);
        }
        CFBundleAllowMixedLocalizations();
        if (tableName == NULL || OCFEqual(tableName, OF_STR(""))) {
            tableName = OF_STR("Localizable");
        }
        OCFArrayRef localizations = NULL;
        if (localizationName != NULL) {
            localizations = OCFArrayCreate(
                kOCFAllocatorDefault, (const void **)&localizationName, 1, &kOCFTypeArrayCallBacks
            );
        }
        result = _copyStringTable(bundle, tableName, NULL, localizations, false, NULL);
        if (localizations != NULL) {
            OCFRelease(localizations);
        }
        if (result == NULL) {
            result = OCFDictionaryCreate(
                kOCFAllocatorSystemDefault,
                NULL,
                NULL,
                0,
                &kOCFTypeDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
        }
    }
    if (nested != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native _CFBundleCopyMostAppropriateLocTableDeviceAndPlatformSpecificVariants, 0x1c02c213c, with its block
// (0x1c02c2598) and, inlined, _CFBundleCopyLocTableProvenanceForDeviceAndPlatformVariants with its block
// (0x1c02c284c). Non-external. The loctable's top-level keys (system default allocator, no options) are
// probed for the language with the product and platform suffixes in the orders product-platform,
// platform-product, platform, product and none, each present one adding "LocProvenance:<variant>". Their
// filtered provenance dictionary is used when it has one entry per key path; otherwise every variant maps to
// the SInt8 number 3. Among the variants, a provenance with bit 0 (strings) or bit 1 (stringsdict) chooses
// the variant whose _CFBundleSplitFileName outcome is highest, retained; the outcomes are stored as the
// versions. Without top-level keys, key paths, a filtered dictionary or provenance entries, all four outputs
// are zero.
static void _CFBundleCopyMostAppropriateLocTableDeviceAndPlatformSpecificVariants(
    OCFDataRef data,
    OCFStringRef language,
    OCFStringRef *stringsVariant,
    uint32_t *stringsVersion,
    OCFStringRef *stringsDictVariant,
    uint32_t *stringsDictVersion
) {
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFSetRef topLevelKeys = _OCFPropertyListCopyTopLevelKeys(allocator, data, 0, NULL);
    OCFDictionaryRef provenance = NULL;
    if (topLevelKeys != NULL) {
        OCFMutableSetRef keyPaths = OCFSetCreateMutable(allocator, 5, &kOCFTypeSetCallBacks);
        OCFStringRef productSuffix = _CFBundleGetProductNameSuffix();
        OCFStringRef platformSuffix = _CFBundleGetPlatformNameSuffix();
        OCFMutableStringRef productPlatform = _CFStringCreateAppend3(language, productSuffix, platformSuffix);
        OCFMutableStringRef platformProduct = _CFStringCreateAppend3(language, platformSuffix, productSuffix);
        OCFMutableStringRef platformOnly = _CFStringCreateAppend2(language, platformSuffix);
        OCFMutableStringRef productOnly = _CFStringCreateAppend2(language, productSuffix);
        _CFBundleAddProvenanceKeyPathIfPresent(topLevelKeys, keyPaths, productPlatform);
        _CFBundleAddProvenanceKeyPathIfPresent(topLevelKeys, keyPaths, platformProduct);
        _CFBundleAddProvenanceKeyPathIfPresent(topLevelKeys, keyPaths, platformOnly);
        _CFBundleAddProvenanceKeyPathIfPresent(topLevelKeys, keyPaths, productOnly);
        _CFBundleAddProvenanceKeyPathIfPresent(topLevelKeys, keyPaths, language);
        if (OCFSetGetCount(keyPaths) >= 1) {
            OCFPropertyListRef filtered = NULL;
            bool ok = _OCFPropertyListCreateFiltered(allocator, data, 0, keyPaths, &filtered, NULL);
            if (filtered == NULL) {
                ok = false;
            }
            if (ok) {
                OCFDictionaryRef found = OCFDictionaryGetValue(filtered, OF_STR("LocProvenance"));
                if (found != NULL && OCFDictionaryGetCount(found) == OCFSetGetCount(keyPaths)) {
                    provenance = OCFRetain(found);
                } else {
                    OCFMutableDictionaryRef everywhere = OCFDictionaryCreateMutable(
                        allocator,
                        OCFSetGetCount(keyPaths),
                        &kOCFTypeDictionaryKeyCallBacks,
                        &kOCFTypeDictionaryValueCallBacks
                    );
                    int8_t three = 3;
                    OCFNumberRef number = OCFNumberCreate(allocator, kOCFNumberSInt8Type, &three);
                    OCFIndex prefixLength = OCFStringGetLength(OF_STR("LocProvenance")) + 1;
                    CFSetApply(keyPaths, ^(const void *value, Boolean *stop) {
                      (void)stop;
                      OCFStringRef keyPath = (OCFStringRef)value;
                      OCFStringRef variant = OCFStringCreateWithSubstring(
                          kOCFAllocatorSystemDefault,
                          keyPath,
                          (OCFRange){prefixLength, OCFStringGetLength(keyPath) - prefixLength}
                      );
                      if (OCFDictionaryGetValue(everywhere, variant) == NULL) {
                          OCFDictionarySetValue(everywhere, variant, number);
                      }
                      OCFRelease(variant);
                    });
                    OCFRelease(number);
                    provenance = everywhere;
                }
                OCFRelease(filtered);
            }
        }
        OCFRelease(productPlatform);
        OCFRelease(platformProduct);
        OCFRelease(platformOnly);
        OCFRelease(productOnly);
        OCFRelease(topLevelKeys);
        OCFRelease(keyPaths);
    }
    if (provenance == NULL) {
        *stringsVariant = NULL;
        *stringsDictVariant = NULL;
        *stringsVersion = 0;
        *stringsDictVersion = 0;
        return;
    }
    if (OCFDictionaryGetCount(provenance) == 0) {
        *stringsVariant = NULL;
        *stringsDictVariant = NULL;
        *stringsVersion = 0;
        *stringsDictVersion = 0;
        OCFRelease(provenance);
        return;
    }
    __block uint32_t bestStringsVersion = 0;
    __block OCFStringRef bestStringsVariant = NULL;
    __block uint32_t bestStringsDictVersion = 0;
    __block OCFStringRef bestStringsDictVariant = NULL;
    CFDictionaryApply(provenance, ^(const void *key, const void *value, Boolean *stop) {
      (void)value;
      (void)stop;
      OCFStringRef variant = (OCFStringRef)key;
      OCFStringRef stripped = NULL;
      int32_t outcome = 0;
      _CFBundleSplitFileName(
          variant,
          &stripped,
          NULL,
          NULL,
          _CFBundleGetProductNameSuffix(),
          _CFBundleGetPlatformNameSuffix(),
          2,
          &outcome
      );
      if (stripped != NULL) {
          OCFRelease(stripped);
      }
      uint8_t kinds = 0;
      OCFNumberGetValue(OCFDictionaryGetValue(provenance, variant), kOCFNumberSInt8Type, &kinds);
      if ((kinds & 1) && (uint32_t)outcome > bestStringsVersion) {
          if (bestStringsVariant != NULL) {
              OCFRelease(bestStringsVariant);
          }
          bestStringsVariant = OCFRetain(variant);
          bestStringsVersion = (uint32_t)outcome;
      }
      if ((kinds & 2) && (uint32_t)outcome > bestStringsDictVersion) {
          if (bestStringsDictVariant != NULL) {
              OCFRelease(bestStringsDictVariant);
          }
          bestStringsDictVariant = OCFRetain(variant);
          bestStringsDictVersion = (uint32_t)outcome;
      }
    });
    *stringsVariant = bestStringsVariant;
    *stringsVersion = bestStringsVersion;
    *stringsDictVariant = bestStringsDictVariant;
    *stringsDictVersion = bestStringsDictVersion;
    OCFRelease(provenance);
}

// Native _CFBundleAddProvenanceKeyPathIfPresent, 0x1c02c27e8. Non-external. A top-level key present in the
// set adds "LocProvenance:<key>" (_CFStringCreateAppend3, released after the add).
static void
_CFBundleAddProvenanceKeyPathIfPresent(OCFSetRef topLevelKeys, OCFMutableSetRef keyPaths, OCFStringRef key) {
    if (!OCFSetContainsValue(topLevelKeys, key)) {
        return;
    }
    OCFMutableStringRef keyPath = _CFStringCreateAppend3(OF_STR("LocProvenance"), OF_STR(":"), key);
    OCFSetAddValue(keyPaths, keyPath);
    OCFRelease(keyPath);
}

// Native _useLoctableInsteadOfFileBasedOnVersion, 0x1c02c28c4. Non-external. Without a loctable language,
// false. A file URL whose version (_CFBundleGetFileVersionForStringsResourceURL) is not older than the
// loctable variant keeps the file (false); otherwise the URL is released and cleared, and the language is
// replaced by the retained loctable language (true).
static Boolean _useLoctableInsteadOfFileBasedOnVersion(
    OCFURLRef *url, OCFStringRef *language, uint32_t version, OCFStringRef locTableLanguage
) {
    if (locTableLanguage == NULL) {
        return false;
    }
    if (*url != NULL) {
        if (version <= _CFBundleGetFileVersionForStringsResourceURL(*url)) {
            return false;
        }
        if (*url != NULL) {
            OCFRelease(*url);
        }
    }
    *url = NULL;
    if (*language != NULL) {
        OCFRelease(*language);
    }
    *language = OCFRetain(locTableLanguage);
    return true;
}

// Native _CFBundleGetFileVersionForStringsResourceURL, 0x1c02c294c. Non-external. The _CFBundleSplitFileName
// outcome of the URL's last path component against the product and platform suffixes; 0 for a NULL URL or
// component.
static uint32_t _CFBundleGetFileVersionForStringsResourceURL(OCFURLRef url) {
    if (url == NULL) {
        return 0;
    }
    int32_t outcome = 0;
    OCFStringRef lastComponent = OCFURLCopyLastPathComponent(url);
    if (lastComponent == NULL) {
        return 0;
    }
    OCFStringRef stripped = NULL;
    _CFBundleSplitFileName(
        lastComponent,
        &stripped,
        NULL,
        NULL,
        _CFBundleGetProductNameSuffix(),
        _CFBundleGetPlatformNameSuffix(),
        2,
        &outcome
    );
    if (stripped != NULL) {
        OCFRelease(stripped);
    }
    OCFRelease(lastComponent);
    return (uint32_t)outcome;
}

// Native _releaseStringsSource, 0x1c02c29f8. Non-external; the value release callback of the strings-sources
// cache. Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json. The 72-byte sources are passed
// by value (a stack copy) to _CFBundleReleaseStringsSources, then the heap block is freed (a tail call). The
// allocator is not read.
static void _releaseStringsSource(OCFAllocatorRef allocator, const void *value) {
    (void)allocator;
    _CFBundleReleaseStringsSources(*(const StringsSources *)value);
    free((void *)value);
}

// Native _CFBundleReleaseStringsSources, 0x1c02c2a6c. Non-external. Releases the eight object fields in
// order, skipping NULL ones.
static void _CFBundleReleaseStringsSources(StringsSources sources) {
    if (sources.stringsURL != NULL) {
        OCFRelease(sources.stringsURL);
    }
    if (sources.stringsLanguage != NULL) {
        OCFRelease(sources.stringsLanguage);
    }
    if (sources.stringsData != NULL) {
        OCFRelease(sources.stringsData);
    }
    if (sources.stringsDictURL != NULL) {
        OCFRelease(sources.stringsDictURL);
    }
    if (sources.stringsDictLanguage != NULL) {
        OCFRelease(sources.stringsDictLanguage);
    }
    if (sources.stringsDictData != NULL) {
        OCFRelease(sources.stringsDictData);
    }
    if (sources.locTableURL != NULL) {
        OCFRelease(sources.locTableURL);
    }
    if (sources.locTableData != NULL) {
        OCFRelease(sources.locTableData);
    }
}

// Native _CFBundleCopyLanguageForStringsResourceURL, 0x1c02c2af0. Non-external. The path component before the
// last ".lproj" of the URL's path (backwards searches for ".lproj" and then "/"), system default allocator;
// NULL when either is missing.
static OCFStringRef _CFBundleCopyLanguageForStringsResourceURL(OCFURLRef url) {
    OCFStringRef path = OCFURLCopyPath(url);
    OCFRange lproj = {0, 0};
    OCFRange slash = {0, 0};
    OCFStringRef result = NULL;
    if (OCFStringFindWithOptions(
            path, OF_STR(".lproj"), (OCFRange){0, OCFStringGetLength(path)}, kOCFCompareBackwards, &lproj
        ) &&
        OCFStringFindWithOptions(
            path, OF_STR("/"), (OCFRange){0, lproj.location}, kOCFCompareBackwards, &slash
        )) {
        OCFIndex start = slash.location + slash.length;
        result = OCFStringCreateWithSubstring(
            kOCFAllocatorSystemDefault, path, (OCFRange){start, lproj.location - start}
        );
    }
    OCFRelease(path);
    return result;
}

// Native _CFBundleGetLocTableProvenanceForLanguage, 0x1c02c2bd4. Non-external. The key path
// "LocProvenance:<language>" (a one-element NSSet) is filtered from the loctable (kCFAllocatorDefault, no
// options); a CFNumber (type ID 22) read as kCFNumberCharType gives the strings (bit 0) and stringsdict (bit
// 1) flags. Otherwise, unless the language is "none" (or when the filtering fails), a top-level key equal to
// the language sets both flags.
static void _CFBundleGetLocTableProvenanceForLanguage(
    OCFDataRef data, OCFStringRef language, Boolean *strings, Boolean *stringsDict
) {
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableStringRef keyPath = OCFStringCreateMutableCopy(allocator, 0, OF_STR("LocProvenance"));
    OCFStringAppend(keyPath, OF_STR(":"));
    OCFStringAppend(keyPath, language);
    OCFTypeRef keyPathObject = keyPath;
    id keyPaths = ((id (*)(id, SEL, const void **, unsigned long))objc_msgSend)(
        objc_alloc((Class)setClass), sel_getUid("initWithObjects:count:"), &keyPathObject, 1
    );
    OCFPropertyListRef filtered = NULL;
    Boolean checkTopLevel = true;
    if (_OCFPropertyListCreateFiltered(kOCFAllocatorDefault, data, 0, (OCFSetRef)keyPaths, &filtered, NULL)) {
        OCFTypeRef value = _OCFPropertyListGetValueWithKeyPath(filtered, keyPath);
        uint8_t kinds = 0;
        if (value != NULL && OCFGetTypeID(value) == 22 &&
            OCFNumberGetValue((OCFNumberRef)value, kOCFNumberCharType, &kinds)) {
            checkTopLevel = false;
            *strings = kinds & 1;
            *stringsDict = (kinds >> 1) & 1;
        } else {
            checkTopLevel = !OCFEqual(language, OF_STR("none"));
        }
        if (filtered != NULL) {
            OCFRelease(filtered);
        }
    }
    if (checkTopLevel) {
        OCFSetRef topLevelKeys = _OCFPropertyListCopyTopLevelKeys(allocator, data, 0, NULL);
        if (topLevelKeys != NULL) {
            if (OCFSetContainsValue(topLevelKeys, language)) {
                *strings = true;
                *stringsDict = true;
            }
            OCFRelease(topLevelKeys);
        }
    }
    OCFRelease(keyPathObject);
    objc_release(keyPaths);
}

// Native _loadStringsDictTable, 0x1c02c2ea0. Non-external. Loads the stringsdict data (+40, mapped flag +65)
// into the result; a matching load stores the stringsdict URL (+24) in *outURL when none is stored, and a
// grown result marks dictionary values (+75).
static void _loadStringsDictTable(
    OCFBundleRef bundle,
    OCFStringRef tableName,
    OCFSetRef keys,
    OCFMutableDictionaryRef result,
    StringsRequest *request,
    OCFTypeRef *outURL
) {
    OCFIndex before = OCFDictionaryGetCount(result);
    Boolean loaded = _loadStringsFromData(
        bundle,
        tableName,
        keys,
        request->sources.stringsDictData,
        request->sources.stringsDictMapped,
        result,
        request
    );
    if (outURL != NULL && loaded && *outURL == NULL) {
        *outURL = OCFRetain(request->sources.stringsDictURL);
    }
    if (OCFDictionaryGetCount(result) > before) {
        request->hasDictionaryValues = 1;
    }
}

// Native _loadLocTable, 0x1c02c2f48, with ____loadLocTable_block_invoke (0x1c02c3930) and _block_invoke_2
// (0x1c02c39c0). Non-external. Nothing for a NULL language or the language already loaded (+80, CFEqual).
// Without keys, or for a cacheable request with unmapped data, the whole language dictionary is filtered from
// the loctable (+56, mapped +66) with a one-element NSSet of the one-element key path; a nonempty dictionary
// (type ID 18) is added (its provenance for the stringsdict kind marks dictionary values), and the request
// records the whole table and the language. With keys already all present nothing is loaded; otherwise the
// key paths [language, key] are filtered and each found value is added under its key (a dictionary value
// marks dictionary values). A load stores "/LocTable/<language>.lproj" as a directory URL (system default
// allocator) in *outURL when none is stored.
static void _loadLocTable(
    OCFBundleRef bundle,
    OCFStringRef tableName,
    OCFStringRef language,
    OCFSetRef keys,
    OCFMutableDictionaryRef result,
    StringsRequest *request,
    OCFTypeRef *outURL
) {
    if (language == NULL) {
        return;
    }
    if (request->locTableLanguage != NULL && OCFEqual(language, request->locTableLanguage)) {
        return;
    }
    __block Boolean loaded = false;
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    if (keys == NULL || (request->someDataUnmapped && request->cacheable)) {
        OCFTypeRef languageValue = language;
        OCFArrayRef keyPath = OCFArrayCreate(allocator, &languageValue, 1, &kOCFTypeArrayCallBacks);
        OCFTypeRef keyPathObject = keyPath;
        id keyPaths = ((id (*)(id, SEL, const void **, unsigned long))objc_msgSend)(
            objc_alloc((Class)setClass), sel_getUid("initWithObjects:count:"), &keyPathObject, 1
        );
        OCFPropertyListRef strings = __CFBundleCreateStringsFromPlistData(
            bundle,
            (OCFSetRef)keyPaths,
            request->sources.locTableData,
            request->sources.locTableMapped,
            tableName
        );
        OCFTypeRef languageTable = _OCFPropertyListGetValueWithKeyPath(strings, language);
        if (languageTable != NULL && OCFGetTypeID(languageTable) == 18 &&
            OCFDictionaryGetCount((OCFDictionaryRef)languageTable) >= 1) {
            Boolean stringsKind;
            Boolean stringsDictKind = false;
            _CFBundleGetLocTableProvenanceForLanguage(
                request->sources.locTableData, language, &stringsKind, &stringsDictKind
            );
            if (stringsDictKind) {
                request->hasDictionaryValues = 1;
            }
            OCFDictionaryApplyFunction((OCFDictionaryRef)languageTable, __CFStringsDictAddFunction, result);
            loaded = true;
            request->loadedWholeTable = 1;
            request->locTableLanguage = OCFRetain(language);
        }
        OCFRelease(keyPath);
        objc_release(keyPaths);
        if (strings != NULL) {
            OCFRelease(strings);
        }
    } else {
        if (_dictionaryContainsAllKeysInSet(result, keys)) {
            return;
        }
        OCFMutableSetRef keyPaths =
            OCFSetCreateMutable(allocator, OCFSetGetCount(keys), &kOCFTypeSetCallBacks);
        CFSetApply(keys, ^(const void *value, Boolean *stop) {
          (void)stop;
          const void *pathValues[2] = {language, value};
          OCFArrayRef keyPath =
              OCFArrayCreate(kOCFAllocatorSystemDefault, pathValues, 2, &kOCFTypeArrayCallBacks);
          OCFSetAddValue(keyPaths, keyPath);
          OCFRelease(keyPath);
        });
        OCFPropertyListRef strings = __CFBundleCreateStringsFromPlistData(
            bundle, keyPaths, request->sources.locTableData, request->sources.locTableMapped, tableName
        );
        CFSetApply(keyPaths, ^(const void *value, Boolean *stop) {
          (void)stop;
          // The native call passes the CFArray key path where the restored declaration spells a string.
          OCFTypeRef found = _OCFPropertyListGetValueWithKeyPath(strings, (OCFStringRef)value);
          if (found == NULL) {
              return;
          }
          if (OCFGetTypeID(found) == 18) {
              request->hasDictionaryValues = 1;
          }
          OCFDictionaryAddValue(result, OCFArrayGetValueAtIndex((OCFArrayRef)value, 1), found);
          loaded = true;
        });
        if (strings != NULL) {
            OCFRelease(strings);
        }
        OCFRelease(keyPaths);
    }
    if (outURL != NULL && *outURL == NULL && loaded) {
        OCFStringRef path =
            OCFStringCreateWithFormat(allocator, NULL, OF_STR("/LocTable/%@.lproj"), language);
        *outURL = OCFURLCreateWithFileSystemPath(allocator, path, kOCFURLPOSIXPathStyle, true);
        OCFRelease(path);
    }
}

// Native _loadStringsFromData, 0x1c02c32a0, with ____loadStringsFromData_block_invoke (0x1c02c35e8) and
// _block_invoke_2 (0x1c02c3888). Non-external. The whole table is read when there are no keys, or when the
// request has unmapped data and is cacheable; otherwise nothing is read when the result already holds every
// key, and else only the one-element key paths of the keys are filtered. A whole dictionary (type ID 18) is
// added to the result, answering whether it grew and marking the whole table; a filtered one adds each key it
// holds, answering whether any was found.
static Boolean _loadStringsFromData(
    OCFBundleRef bundle,
    OCFStringRef tableName,
    OCFSetRef keys,
    OCFDataRef data,
    Boolean mapped,
    OCFMutableDictionaryRef result,
    StringsRequest *request
) {
    __block Boolean found = false;
    OCFMutableSetRef keyPaths = NULL;
    Boolean whole = true;
    if (keys != NULL && !(request->someDataUnmapped && request->cacheable)) {
        if (_dictionaryContainsAllKeysInSet(result, keys)) {
            return false;
        }
        keyPaths =
            OCFSetCreateMutable(kOCFAllocatorSystemDefault, OCFSetGetCount(keys), &kOCFTypeSetCallBacks);
        CFSetApply(keys, ^(const void *value, Boolean *stop) {
          (void)stop;
          OCFArrayRef keyPath =
              OCFArrayCreate(kOCFAllocatorSystemDefault, &value, 1, &kOCFTypeArrayCallBacks);
          OCFSetAddValue(keyPaths, keyPath);
          OCFRelease(keyPath);
        });
        whole = false;
    }
    OCFPropertyListRef strings =
        __CFBundleCreateStringsFromPlistData(bundle, keyPaths, data, mapped, tableName);
    if (strings != NULL) {
        if (OCFGetTypeID(strings) == 18) {
            if (whole) {
                OCFIndex before = OCFDictionaryGetCount(result);
                OCFDictionaryApplyFunction((OCFDictionaryRef)strings, __CFStringsDictAddFunction, result);
                found = OCFDictionaryGetCount(result) > before;
                request->loadedWholeTable = 1;
            } else {
                CFSetApply(keys, ^(const void *value, Boolean *stop) {
                  (void)stop;
                  OCFTypeRef string = OCFDictionaryGetValue((OCFDictionaryRef)strings, value);
                  if (string != NULL) {
                      found = true;
                      OCFDictionaryAddValue(result, value, string);
                  }
                });
            }
        }
        OCFRelease(strings);
    }
    if (keyPaths != NULL) {
        OCFRelease(keyPaths);
    }
    return found;
}

// Native _dictionaryContainsAllKeysInSet, 0x1c02c3510, with ____dictionaryContainsAllKeysInSet_block_invoke
// (0x1c02c38f0). Non-external. A key without a value clears the answer and stops the walk.
static Boolean _dictionaryContainsAllKeysInSet(OCFDictionaryRef dict, OCFSetRef keys) {
    __block Boolean containsAll = true;
    CFSetApply(keys, ^(const void *value, Boolean *stop) {
      if (OCFDictionaryGetValue(dict, value) == NULL) {
          containsAll = false;
          *stop = true;
      }
    });
    return containsAll;
}

// Native __CFBundleCreateStringsFromPlistData, 0x1c02c3674, with its .cold.1 (0x1c03121dc, the error log of a
// non-dictionary top level). Non-external. With key paths, _CFPropertyListCreateFiltered (the bundle's
// allocator, option 0x10001 when mapped, else 1) answers the filtered value or NULL after logging "Unable to
// read key-paths %@ from .strings file: %@ / %@: %@" with its error. Without key paths,
// CFPropertyListCreateWithData (option 0x10000 when mapped, else 0) must answer a dictionary; anything else
// is logged and released, and a parse failure logs its error. Errors are released; failures answer NULL.
static OCFPropertyListRef __CFBundleCreateStringsFromPlistData(
    OCFBundleRef bundle, OCFSetRef keyPaths, OCFDataRef data, Boolean mapped, OCFStringRef tableName
) {
    OCFErrorRef error = NULL;
    if (keyPaths != NULL) {
        OCFPropertyListRef filtered = NULL;
        OCFOptionFlags options = mapped ? 0x10001 : 1;
        if (_OCFPropertyListCreateFiltered(
                OCFGetAllocator(bundle), data, options, keyPaths, &filtered, &error
            )) {
            return filtered;
        }
        if (error == NULL) {
            return NULL;
        }
        os_log_error(
            _CFBundleLocalizedStringLogger(),
            "Unable to read key-paths %@ from .strings file: %@ / %@: %@",
            keyPaths,
            bundle,
            tableName,
            error
        );
        OCFRelease(error);
        return NULL;
    }
    OCFOptionFlags options = mapped ? 0x10000 : 0;
    OCFPropertyListRef plist =
        OCFPropertyListCreateWithData(OCFGetAllocator(bundle), data, options, NULL, &error);
    if (plist != NULL) {
        if (OCFDictionaryGetTypeID() == OCFGetTypeID(plist)) {
            return plist;
        }
        os_log_error(
            _CFBundleLocalizedStringLogger(),
            "Unable to load .strings file: %@ / %@: Top-level object was not a dictionary",
            bundle,
            tableName
        );
        OCFRelease(plist);
        return NULL;
    }
    if (error == NULL) {
        return NULL;
    }
    os_log_error(
        _CFBundleLocalizedStringLogger(),
        "Unable to load .strings file: %@ / %@: %@",
        bundle,
        tableName,
        error
    );
    OCFRelease(error);
    return NULL;
}

// Native __CFPlatformSpecificStringDictionaryApplyFunction, 0x1c02c3a3c. Non-external. A dictionary value
// with a nonempty kCFStringDeviceSpecificRuleType dictionary takes the entry for the product name, else
// "iphone" for an "ipod" or "ipad" product, else "other"; a string or dictionary found replaces the result
// entry. Otherwise (an empty rule, no entry, or an entry of another type) the result entry becomes the
// .strings value for the key, or is removed when there is none.
static void
__CFPlatformSpecificStringDictionaryApplyFunction(const void *key, const void *value, void *context) {
    ApplierContext *applier = (ApplierContext *)context;
    if (OCFDictionaryGetTypeID() != OCFGetTypeID(value)) {
        return;
    }
    OCFDictionaryRef rule = OCFDictionaryGetValue((OCFDictionaryRef)value, kOCFStringDeviceSpecificRuleType);
    if (rule == NULL) {
        return;
    }
    OCFStringRef product = _OCFGetProductName();
    if (OCFDictionaryGetCount(rule) != 0) {
        OCFTypeRef chosen = NULL;
        if (product != NULL) {
            chosen = OCFDictionaryGetValue(rule, product);
            if (chosen == NULL && (OCFEqual(product, OF_STR("ipod")) || OCFEqual(product, OF_STR("ipad")))) {
                chosen = OCFDictionaryGetValue(rule, OF_STR("iphone"));
            }
        }
        if (chosen == NULL) {
            chosen = OCFDictionaryGetValue(rule, OF_STR("other"));
        }
        if (chosen != NULL) {
            if (OCFGetTypeID(chosen) == OCFStringGetTypeID() ||
                OCFGetTypeID(chosen) == OCFDictionaryGetTypeID()) {
                OCFDictionarySetValue(applier->result, key, chosen);
            }
            if (OCFGetTypeID(chosen) == OCFStringGetTypeID() ||
                OCFGetTypeID(chosen) == OCFDictionaryGetTypeID()) {
                return;
            }
        }
    }
    OCFTypeRef fallback = OCFDictionaryGetValue(applier->stringsTable, key);
    if (fallback != NULL) {
        OCFDictionarySetValue(applier->result, key, fallback);
    } else {
        OCFDictionaryRemoveValue(applier->result, key);
    }
}

// Native __CFLocalizedFormatStringApplyFunction, 0x1c02c3bd0. Non-external. A dictionary value with a
// kCFStringLocalizedFormatKey value becomes _CFStringCopyWithFomatStringConfiguration(that value, the
// dictionary) when that answers a string (released after it replaces the result entry).
static void __CFLocalizedFormatStringApplyFunction(const void *key, const void *value, void *context) {
    ApplierContext *applier = (ApplierContext *)context;
    if (OCFDictionaryGetTypeID() != OCFGetTypeID(value)) {
        return;
    }
    OCFStringRef format = OCFDictionaryGetValue((OCFDictionaryRef)value, kOCFStringLocalizedFormatKey);
    if (format == NULL) {
        return;
    }
    OCFStringRef configured = _CFStringCopyWithFomatStringConfiguration(format, value);
    if (configured == NULL) {
        return;
    }
    OCFDictionarySetValue(applier->result, key, configured);
    OCFRelease(configured);
}

// Native __CFVariableWidthStringDictionaryApplyFunction, 0x1c02c3c60. Non-external. A dictionary value with a
// nonempty kCFStringVariableWidthRuleType dictionary becomes _CFStringCreateWithWidthContexts of it (when not
// NULL). An empty rule makes the result entry the .strings value (with a .strings table that has one), or
// removes it.
static void
__CFVariableWidthStringDictionaryApplyFunction(const void *key, const void *value, void *context) {
    ApplierContext *applier = (ApplierContext *)context;
    if (OCFDictionaryGetTypeID() != OCFGetTypeID(value)) {
        return;
    }
    OCFDictionaryRef rule = OCFDictionaryGetValue((OCFDictionaryRef)value, kOCFStringVariableWidthRuleType);
    if (rule == NULL) {
        return;
    }
    if (OCFDictionaryGetCount(rule) != 0) {
        OCFStringRef string = _CFStringCreateWithWidthContexts(rule);
        if (string == NULL) {
            return;
        }
        OCFDictionarySetValue(applier->result, key, string);
        OCFRelease(string);
        return;
    }
    OCFTypeRef fallback =
        applier->stringsTable != NULL ? OCFDictionaryGetValue(applier->stringsTable, key) : NULL;
    if (fallback != NULL) {
        OCFDictionarySetValue(applier->result, key, fallback);
    } else {
        OCFDictionaryRemoveValue(applier->result, key);
    }
}
