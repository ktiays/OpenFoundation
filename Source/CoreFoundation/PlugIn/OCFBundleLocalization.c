//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFBundleLocalization.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/PlugIn/OCFBundleMain.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <os/log.h>
#include <stdlib.h>
#include <string.h>
#include <xpc/xpc.h>

// Inferred organization: the functions below lie in one contiguous iOS Simulator __text cluster
// (0x1c01f3b1c-0x1c01f7130) whose functions log the source path ".../PlugIn.subproj/CFBundle_Locale.c"; they
// are kept in one translation unit in native address order, with each block literal's invoke function written
// as the block literal inside its owner. _copyAppleLocalizations, _copyBundleLocalizationsFromResources,
// _onelineFormatArray and _onelineFormatArrayApply were restored by the second CFBundle sub-track;
// _CFBundleSetDefaultLocalization is not in scope on this branch. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationBundle.md (pending; the coordinator writes
// the reading).

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call here passes 0x10000
// (OS_UNFAIR_LOCK_DATA_SYNCHRONIZATION).
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

// dyld's SPI (libdyld, re-exported by libSystem); _copyBundleLocalizationsFromResources passes
// 0x07E60901FFFFFFFF in X0: every platform (0xFFFFFFFF), version 0x07E60901.
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);
static const OCFBuildVersion kBaseLocalizationVersion = {0xFFFFFFFF, 0x07E60901};

// libobjc's objc_opt_class (private SPI), the class of the receiver.
extern Class objc_opt_class(id object);

// libxpc's xpc_copy_bootstrap (private SPI), the bootstrap dictionary of the process or NULL.
extern xpc_object_t xpc_copy_bootstrap(void);

// libicucore's ualoc_localizationsToUse (Apple ICU SPI; UErrorCode is an int-sized enumeration).
extern int32_t ualoc_localizationsToUse(
    const char *const *preferredLanguages,
    int32_t preferredLanguagesCount,
    const char *const *availableLocalizations,
    int32_t availableLocalizationsCount,
    char *localizationsToUse[],
    int32_t localizationsToUseCapacity,
    int *status
);

// The literal that every _CFBundleLogSourceLocation call of this unit passes (__cstring, 168 bytes with NUL).
static const char kLocaleSourcePath[] = "/Library/Caches/com.apple.xbs/C9620487-17E9-4D33-8BF5-CCA0DA6AFB18/"
                                        "TemporaryDirectory.R3jFjg/Sources/CoreFoundation_Sim/CoreFoundation/"
                                        "PlugIn.subproj/CFBundle_Locale.c";

// The TSD slot that marks a CFBundle entry point on the current thread (0x12 at every native use).
enum {
    kBundleEntryTSDSlot = 0x12,
};

// Native _CFBundleAllowMixedLocalizations.once and .allowMixed (__DATA,__bss 0x22e6d2fb0 and 0x22e6d2fa8),
// _CFBundleFollowParentLocalization.once and .followParent (0x22e6d2fd8 and 0x22e6d2fd0),
// __CFBundleUserLanguages and __CFBundleUserLanguagesLock (0x22e6d2fc0 and 0x22e6d2fb8),
// __defaultLocalization (0x22e6d2fc8),
// __CFBundleCopyXPCBootstrapMainBundleLanguages.onceToken and .result (0x22e6d2fe8 and 0x22e6d2fe0), and
// __CFBundleCopyDefaultLanguageForLanguage.cache and .defaultLanguageLock (0x22e6d2ff0 and 0x22e6d2ff8). All
// non-external. __defaultLocalization is written only by _CFBundleSetDefaultLocalization (0x1c01f5a2c,
// outside the scope), so it stays NULL here.
static dispatch_once_t allowMixedLocalizationsOnce;
static uint8_t allowMixedLocalizations;
static dispatch_once_t followParentLocalizationOnce;
static uint8_t followParentLocalization;
static OCFArrayRef _CFBundleUserLanguages;
static os_unfair_lock _CFBundleUserLanguagesLock;
static OCFStringRef _defaultLocalization;
static dispatch_once_t xpcBootstrapMainBundleLanguagesOnce;
static OCFMutableArrayRef xpcBootstrapMainBundleLanguages;
static OCFMutableDictionaryRef defaultLanguageCache;
static os_unfair_lock defaultLanguageLock;

static Boolean _CFBundleGetInfoDictionaryBoolean(OCFStringRef key);
static OCFArrayRef _CFBundleCopyBundleLocalizations(OCFBundleRef bundle, const void *mode);
static OCFMutableArrayRef
_CFBundleCopyLocalizationsForPreferences(OCFArrayRef locArray, OCFArrayRef prefArray, Boolean considerMain);
static OCFMutableArrayRef _CFBundleCopyPreferredLanguagesInList(
    OCFArrayRef locArray,
    OCFStringRef devLang,
    OCFArrayRef userLanguages,
    Boolean considerMain,
    OCFURLRef bundleURL,
    OCFBundleRef bundle
);
static OCFArrayRef _CFBundleCopyXPCBootstrapMainBundleLanguages(void);
static OCFArrayRef _copyAppleLocalizations(OCFDictionaryRef infoDict);
static OCFArrayRef _copyBundleLocalizationsFromResources(OCFBundleRef bundle, const void *mode);
static OCFMutableArrayRef _CFBundleCopyLProjDirectoriesForURL(OCFAllocatorRef allocator, OCFURLRef url);
static OCFStringRef _CFBundleGetAlternateNameForLanguage(OCFStringRef language);
static Boolean _CFBundleIsLanguageDefaultLanguageOfOther(OCFStringRef language, OCFStringRef other);
static void _onelineFormatArray(OCFMutableStringRef string, OCFArrayRef array);
static void _onelineFormatArrayApply(const void *value, void *context);
static OCFStringRef _CFBundleCopyDefaultLanguageForLanguage(OCFStringRef language);
static OCFMutableArrayRef
_CFBundleCreateMutableArrayOfFallbackLanguages(OCFArrayRef availableLocalizations, OCFArrayRef preferences);

// Native CFBundleAllowMixedLocalizations, 0x1c01f3b1c; its .cold.1 (0x1c030c348) branches to
// _OUTLINED_FUNCTION_1_18 (0x1c01f7144, dispatch_once of ___block_literal_global_18 on the once token).
// Hidden (was private external). The block, ___CFBundleAllowMixedLocalizations_block_invoke (0x1c01f3b4c),
// stores _CFBundleGetInfoDictionaryBoolean(_kCFBundleAllowMixedLocalizationsKey) (the key is loaded from the
// exported constant).
Boolean CFBundleAllowMixedLocalizations(void) {
    dispatch_once(&allowMixedLocalizationsOnce, ^{
      allowMixedLocalizations = _CFBundleGetInfoDictionaryBoolean(_kOCFBundleAllowMixedLocalizationsKey);
    });
    return allowMixedLocalizations;
}

// Native _CFBundleGetInfoDictionaryBoolean, 0x1c01f3b74. Non-external. The main bundle's Info.plist value for
// key: a CFBoolean answers its value; a CFString answers true for "true" or "YES" (kCFCompareCaseInsensitive,
// "true" first); a CFNumber answers true when its SInt32 value (3) is read and nonzero; anything else, and a
// missing main bundle, dictionary or value, answers false.
static Boolean _CFBundleGetInfoDictionaryBoolean(OCFStringRef key) {
    OCFBundleRef mainBundle = OCFBundleGetMainBundle();
    if (mainBundle == NULL) {
        return false;
    }
    OCFDictionaryRef infoDict = OCFBundleGetInfoDictionary(mainBundle);
    if (infoDict == NULL) {
        return false;
    }
    OCFTypeRef value = OCFDictionaryGetValue(infoDict, key);
    if (value == NULL) {
        return false;
    }
    OCFTypeID typeID = OCFGetTypeID(value);
    if (typeID == OCFBooleanGetTypeID()) {
        return OCFBooleanGetValue((OCFBooleanRef)value);
    }
    if (typeID == OCFStringGetTypeID()) {
        if (OCFStringCompare((OCFStringRef)value, OF_STR("true"), kOCFCompareCaseInsensitive) ==
            kOCFCompareEqualTo) {
            return true;
        }
        return OCFStringCompare((OCFStringRef)value, OF_STR("YES"), kOCFCompareCaseInsensitive) ==
               kOCFCompareEqualTo;
    }
    if (typeID == OCFNumberGetTypeID()) {
        SInt32 number = 0;
        return OCFNumberGetValue((OCFNumberRef)value, kOCFNumberSInt32Type, &number) && number != 0;
    }
    return false;
}

// Native _CFBundleCopyLanguageSearchListInBundle, 0x1c01f3fd0. Hidden (was private external). Answers the
// bundle's cached search list (+40) retained, under the lock at +104 (taken plainly first, with options
// 0x10000 to publish). Otherwise the list is computed unlocked: the preferred languages among the bundle
// localizations for the user languages and the development region (each call passes considerMain 1, the
// bundle URL and the bundle), retried with "en_US" and then with the first localization while it comes out
// empty; the development region is appended when absent, or without one the first of "en", "English", "en_US"
// that the localizations contain; an empty list gets __defaultLocalization or "en". A list published
// meanwhile by another thread wins and the computed one is released.
OCFArrayRef _CFBundleCopyLanguageSearchListInBundle(OCFBundleRef bundle) {
    os_unfair_lock_lock(&bundle->_lock104);
    OCFMutableArrayRef languages = (OCFMutableArrayRef)bundle->_searchLanguages;
    if (languages == NULL) {
        os_unfair_lock_unlock(&bundle->_lock104);
        OCFArrayRef localizations = OCFBundleCopyBundleLocalizations(bundle);
        OCFArrayRef userLanguages = _CFBundleCopyUserLanguages();
        OCFStringRef devLang = OCFBundleGetDevelopmentRegion(bundle);
        languages = _CFBundleCopyPreferredLanguagesInList(
            localizations, devLang, userLanguages, true, bundle->_url, bundle
        );
        if (OCFArrayGetCount(languages) == 0) {
            OCFRelease(languages);
            languages = _CFBundleCopyPreferredLanguagesInList(
                localizations, OF_STR("en_US"), userLanguages, true, bundle->_url, bundle
            );
        }
        if (OCFArrayGetCount(languages) == 0 && localizations != NULL &&
            OCFArrayGetCount(localizations) >= 1) {
            OCFStringRef first = OCFArrayGetValueAtIndex(localizations, 0);
            OCFRelease(languages);
            languages = _CFBundleCopyPreferredLanguagesInList(
                localizations, first, userLanguages, true, bundle->_url, bundle
            );
        }
        if (userLanguages != NULL) {
            OCFRelease(userLanguages);
        }
        if (devLang != NULL) {
            if (!OCFArrayContainsValue(languages, (OCFRange){0, OCFArrayGetCount(languages)}, devLang)) {
                OCFArrayAppendValue(languages, devLang);
            }
            if (localizations != NULL) {
                OCFRelease(localizations);
            }
        } else if (localizations != NULL) {
            OCFIndex count = OCFArrayGetCount(localizations);
            OCFStringRef fallback = NULL;
            if (OCFArrayContainsValue(localizations, (OCFRange){0, count}, OF_STR("en"))) {
                fallback = OF_STR("en");
            } else if (OCFArrayContainsValue(localizations, (OCFRange){0, count}, OF_STR("English"))) {
                fallback = OF_STR("English");
            } else if (OCFArrayContainsValue(localizations, (OCFRange){0, count}, OF_STR("en_US"))) {
                fallback = OF_STR("en_US");
            }
            if (fallback != NULL &&
                !OCFArrayContainsValue(languages, (OCFRange){0, OCFArrayGetCount(languages)}, fallback)) {
                OCFArrayAppendValue(languages, fallback);
            }
            OCFRelease(localizations);
        }
        if (OCFArrayGetCount(languages) == 0) {
            OCFArrayAppendValue(
                languages, _defaultLocalization != NULL ? _defaultLocalization : OF_STR("en")
            );
        }
        os_unfair_lock_lock_with_options(&bundle->_lock104, 0x10000);
        if (bundle->_searchLanguages != NULL) {
            OCFRelease(languages);
            languages = (OCFMutableArrayRef)bundle->_searchLanguages;
        } else {
            bundle->_searchLanguages = languages;
        }
    }
    OCFArrayRef result = OCFRetain(languages);
    os_unfair_lock_unlock(&bundle->_lock104);
    return result;
}

// Native _CFBundleCopyUserLanguages, 0x1c01f4260. Hidden (was private external). Marks the thread's bundle
// entry (TSD slot 0x12) and logs the source location (line 905) as the other entry points do. With the
// CoreFoundation/CFSwiftBundle feature it answers +[_NSSwiftBundle bundleUserLanguages] retained (NULL kept).
// Otherwise, under __CFBundleUserLanguagesLock, the cache is filled once from the AppleLanguages preference,
// or NSLanguages when that is absent (kCFPreferencesCurrentApplication, loaded from the exported constant),
// dropped again unless it is a CFArray, and answered retained (or NULL).
OCFArrayRef _CFBundleCopyUserLanguages(void) {
    void *entered = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean log = false;
    if (entered != (void *)1) {
        log = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(log, kLocaleSourcePath, 905, "_CFBundleCopyUserLanguages");
    OCFArrayRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        Class swiftBundle = objc_opt_class((id)objc_lookUpClass(OF_NAME_STRING(_ONSSwiftBundle)));
        result = ((OCFArrayRef (*)(id, SEL))objc_msgSend)((id)swiftBundle, sel_getUid("bundleUserLanguages"));
        if (result != NULL) {
            OCFRetain(result);
        }
    } else {
        os_unfair_lock_lock(&_CFBundleUserLanguagesLock);
        if (_CFBundleUserLanguages == NULL) {
            OCFStringRef applicationID = kOCFPreferencesCurrentApplication;
            OCFPropertyListRef preference =
                OCFPreferencesCopyAppValue(OF_STR("AppleLanguages"), applicationID);
            if (preference == NULL) {
                preference = OCFPreferencesCopyAppValue(OF_STR("NSLanguages"), applicationID);
            }
            if (_CFBundleUserLanguages == NULL && preference != NULL) {
                _CFBundleUserLanguages = OCFRetain(preference);
            }
            if (_CFBundleUserLanguages != NULL &&
                OCFGetTypeID(_CFBundleUserLanguages) != OCFArrayGetTypeID()) {
                OCFRelease(_CFBundleUserLanguages);
                _CFBundleUserLanguages = NULL;
            }
            if (preference != NULL) {
                OCFRelease(preference);
            }
        }
        result = _CFBundleUserLanguages != NULL ? OCFRetain(_CFBundleUserLanguages) : NULL;
        os_unfair_lock_unlock(&_CFBundleUserLanguagesLock);
    }
    if (entered != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native CFBundleCopyBundleLocalizations, 0x1c01f4c34. External. The entry-point pattern (line 666); with
// CFSwiftBundle it sends -localizations to the bundle and retains the answer unconditionally, otherwise
// _CFBundleCopyBundleLocalizations(bundle, NULL).
OCFArrayRef OCFBundleCopyBundleLocalizations(OCFBundleRef bundle) {
    void *entered = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean log = false;
    if (entered != (void *)1) {
        log = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(log, kLocaleSourcePath, 666, OF_NAME_STRING(OCFBundleCopyBundleLocalizations));
    OCFArrayRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = OCFRetain(((OCFArrayRef (*)(id, SEL))objc_msgSend)((id)bundle, sel_getUid("localizations")));
    } else {
        result = _CFBundleCopyBundleLocalizations(bundle, NULL);
    }
    if (entered != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// The inlined CFBundleFollowParentLocalization: its once token and flag, and its block
// ___CFBundleFollowParentLocalization_block_invoke (0x1c01f68ac, through _OUTLINED_FUNCTION_0_26 at
// 0x1c01f7130 and the .cold fragments of its inliners). The flag becomes 1 when the main Info.plist value
// "CFBundleFollowParentLocalization" is true, or when the main bundle's Info.plist has an "NSExtension"
// value; callers compare the flag with 1.
static Boolean followsParentLocalization(void) {
    dispatch_once(&followParentLocalizationOnce, ^{
      if (_CFBundleGetInfoDictionaryBoolean(OF_STR("CFBundleFollowParentLocalization"))) {
          followParentLocalization = 1;
          return;
      }
      OCFBundleRef mainBundle = OCFBundleGetMainBundle();
      if (mainBundle == NULL) {
          return;
      }
      OCFDictionaryRef infoDict = OCFBundleGetInfoDictionary(mainBundle);
      if (infoDict == NULL) {
          return;
      }
      if (OCFDictionaryGetValue(infoDict, OF_STR("NSExtension")) != NULL) {
          followParentLocalization = 1;
      }
    });
    return followParentLocalization == 1;
}

// Native _CFBundleCopyBundleLocalizations, 0x1c01f4d20; its .cold.1 (0x1c030c34c) is the inlined
// CFBundleFollowParentLocalization's dispatch_once. Non-external. Asserts type 31; under the lock at +104 a
// computed list (+112, flagged by the byte at +120) is answered retained. Otherwise, unlocked, the main
// bundle that follows its parent's localization takes the XPC bootstrap languages when there are any, and any
// other case _copyBundleLocalizationsFromResources(bundle, mode); the first list published under the lock
// wins.
static OCFArrayRef _CFBundleCopyBundleLocalizations(OCFBundleRef bundle, const void *mode) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(bundle);
    if (typeID != 31) {
        _CFAssertMismatchedTypeID(31, typeID);
    }
    os_unfair_lock_lock_with_options(&bundle->_lock104, 0x10000);
    if (bundle->_lookedForLocalizations == 0) {
        os_unfair_lock_unlock(&bundle->_lock104);
        OCFArrayRef localizations = NULL;
        if (bundle == OCFBundleGetMainBundle() && followsParentLocalization()) {
            localizations = _CFBundleCopyXPCBootstrapMainBundleLanguages();
        }
        if (localizations == NULL) {
            localizations = _copyBundleLocalizationsFromResources(bundle, mode);
        }
        os_unfair_lock_lock_with_options(&bundle->_lock104, 0x10000);
        if (bundle->_lookedForLocalizations == 0) {
            bundle->_localizations = OCFRetain(localizations);
            bundle->_lookedForLocalizations = 1;
            os_unfair_lock_unlock(&bundle->_lock104);
            return localizations;
        }
        OCFRelease(localizations);
    }
    OCFArrayRef result = OCFRetain(bundle->_localizations);
    os_unfair_lock_unlock(&bundle->_lock104);
    return result;
}

// Native _copyAppleLocalizations, 0x1c01f5190. Non-external. NULL unless the Info.plist value for
// _CFBundleUseAppleLocalizations is kCFBooleanTrue (pointer comparison). Otherwise the localizations of a
// bundle created (system default allocator) for the directory URL
// "/System/Library/Frameworks/CoreFoundation.framework" (POSIX style), through
// _copyBundleLocalizationsFromResources with mode 0; the URL and the bundle are released (the bundle is not
// tested for NULL).
static OCFArrayRef _copyAppleLocalizations(OCFDictionaryRef infoDict) {
    if (OCFDictionaryGetValue(infoDict, _kCFBundleUseAppleLocalizationsKey) != kOCFBooleanTrue) {
        return NULL;
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFURLRef url = OCFURLCreateWithFileSystemPath(
        allocator, OF_STR("/System/Library/Frameworks/CoreFoundation.framework"), kOCFURLPOSIXPathStyle, true
    );
    OCFBundleRef bundle = OCFBundleCreate(allocator, url);
    OCFArrayRef result = _copyBundleLocalizationsFromResources(bundle, NULL);
    OCFRelease(url);
    OCFRelease(bundle);
    return result;
}

// Native CFBundleCopyLocalizationsForPreferences, 0x1c01f56ac. External. The entry-point pattern (line 1292);
// with CFSwiftBundle it answers +[_NSSwiftBundle preferredLocalizationsFromArray:forPreferences:] retained
// unconditionally. Otherwise a non-NULL prefArray is used directly; without one the user languages are used
// (an empty kCFAllocatorSystemDefault array when there are none) and released afterwards. considerMain is 0.
OCFArrayRef OCFBundleCopyLocalizationsForPreferences(OCFArrayRef locArray, OCFArrayRef prefArray) {
    void *entered = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean log = false;
    if (entered != (void *)1) {
        log = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        log, kLocaleSourcePath, 1292, OF_NAME_STRING(OCFBundleCopyLocalizationsForPreferences)
    );
    OCFArrayRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        Class swiftBundle = objc_opt_class((id)objc_lookUpClass(OF_NAME_STRING(_ONSSwiftBundle)));
        result = OCFRetain(((OCFArrayRef (*)(id, SEL, OCFArrayRef, OCFArrayRef))objc_msgSend)(
            (id)swiftBundle,
            sel_getUid("preferredLocalizationsFromArray:forPreferences:"),
            locArray,
            prefArray
        ));
    } else if (prefArray != NULL) {
        result = _CFBundleCopyLocalizationsForPreferences(locArray, prefArray, false);
    } else {
        OCFArrayRef userLanguages = _CFBundleCopyUserLanguages();
        if (userLanguages == NULL) {
            userLanguages = OCFArrayCreate(kOCFAllocatorSystemDefault, NULL, 0, &kOCFTypeArrayCallBacks);
        }
        result = _CFBundleCopyLocalizationsForPreferences(locArray, userLanguages, false);
        OCFRelease(userLanguages);
    }
    if (entered != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native _CFBundleCopyLocalizationsForPreferences, 0x1c01f5810. Non-external. A NULL or empty locArray
// answers
// ["en"] (a new kCFAllocatorSystemDefault array). Otherwise the preferred languages in locArray for prefArray
// (no development language, URL or bundle); an empty answer gets locArray's first element appended, or "en"
// when locArray has become empty meanwhile. The final NULL test is native; the earlier count of a NULL answer
// would not return.
static OCFMutableArrayRef
_CFBundleCopyLocalizationsForPreferences(OCFArrayRef locArray, OCFArrayRef prefArray, Boolean considerMain) {
    OCFMutableArrayRef result = NULL;
    if (locArray != NULL && OCFArrayGetCount(locArray) >= 1) {
        result = _CFBundleCopyPreferredLanguagesInList(locArray, NULL, prefArray, considerMain, NULL, NULL);
        if (OCFArrayGetCount(result) == 0 && OCFArrayGetCount(locArray) >= 1) {
            OCFArrayAppendValue(result, OCFArrayGetValueAtIndex(locArray, 0));
        } else if (OCFArrayGetCount(result) == 0) {
            OCFArrayAppendValue(result, OF_STR("en"));
        }
    }
    if (result == NULL) {
        result = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
        OCFArrayAppendValue(result, OF_STR("en"));
    }
    return result;
}

// Native CFBundleCopyPreferredLocalizationsFromArray, 0x1c01f58e8. External. The entry-point pattern (line
// 1311); with CFSwiftBundle it answers +[_NSSwiftBundle preferredLocalizationsFromArray:] retained
// unconditionally. Otherwise the localizations for the user languages (an empty kCFAllocatorSystemDefault
// array when there are none) with considerMain 1.
OCFArrayRef OCFBundleCopyPreferredLocalizationsFromArray(OCFArrayRef locArray) {
    void *entered = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean log = false;
    if (entered != (void *)1) {
        log = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        log, kLocaleSourcePath, 1311, OF_NAME_STRING(OCFBundleCopyPreferredLocalizationsFromArray)
    );
    OCFArrayRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        Class swiftBundle = objc_opt_class((id)objc_lookUpClass(OF_NAME_STRING(_ONSSwiftBundle)));
        result = OCFRetain(((OCFArrayRef (*)(id, SEL, OCFArrayRef))objc_msgSend)(
            (id)swiftBundle, sel_getUid("preferredLocalizationsFromArray:"), locArray
        ));
    } else {
        OCFArrayRef userLanguages = _CFBundleCopyUserLanguages();
        if (userLanguages == NULL) {
            userLanguages = OCFArrayCreate(kOCFAllocatorSystemDefault, NULL, 0, &kOCFTypeArrayCallBacks);
        }
        result = _CFBundleCopyLocalizationsForPreferences(locArray, userLanguages, true);
        OCFRelease(userLanguages);
    }
    if (entered != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native _CFBundleCopyPreferredLanguagesInList, 0x1c01f5b34; .cold.1 (0x1c030c350) and .cold.3 (0x1c030c358)
// are the inlined CFBundleAllowMixedLocalizations' dispatch_once, .cold.2 (0x1c030c354) the inlined
// CFBundleFollowParentLocalization's. Non-external. With considerMain and mixed localizations not allowed,
// the main bundle's languages come first: the XPC bootstrap languages when the main bundle follows its
// parent's localization, otherwise the main bundle's search list unless the main bundle URL equals bundleURL
// (a main bundle without a URL gives none); when they are not empty, their fallback languages in locArray are
// the answer if there are any. Otherwise the fallbacks for userLanguages (when not empty), then for [devLang]
// (when devLang is not NULL), then for ["en_US"], and finally a new empty array. With the resource logger's
// debug level enabled and a bundleURL or bundle, a "Language lookup" record is logged: the bundle (or URL),
// the four arrays through _onelineFormatArray, the mixed-localization flag and "(use Apple Locs)" when the
// bundle's _CFBundleUseAppleLocalizations value is kCFBooleanTrue. The main-bundle list is released last.
static OCFMutableArrayRef _CFBundleCopyPreferredLanguagesInList(
    OCFArrayRef locArray,
    OCFStringRef devLang,
    OCFArrayRef userLanguages,
    Boolean considerMain,
    OCFURLRef bundleURL,
    OCFBundleRef bundle
) {
    const void *values = devLang;
    OCFArrayRef mainLanguages = NULL;
    OCFMutableArrayRef result = NULL;
    if (considerMain && !CFBundleAllowMixedLocalizations()) {
        if (followsParentLocalization()) {
            mainLanguages = _CFBundleCopyXPCBootstrapMainBundleLanguages();
        } else {
            OCFBundleRef mainBundle = OCFBundleGetMainBundle();
            if (mainBundle != NULL) {
                OCFURLRef mainURL = OCFBundleCopyBundleURL(mainBundle);
                if (mainURL != NULL) {
                    if (bundleURL != NULL && OCFEqual(bundleURL, mainURL)) {
                        mainLanguages = NULL;
                    } else {
                        mainLanguages = _CFBundleCopyLanguageSearchListInBundle(mainBundle);
                    }
                    OCFRelease(mainURL);
                }
            }
        }
        if (mainLanguages != NULL && OCFArrayGetCount(mainLanguages) >= 1) {
            result = _CFBundleCreateMutableArrayOfFallbackLanguages(locArray, mainLanguages);
        }
    }
    if (result == NULL) {
        if (userLanguages != NULL && OCFArrayGetCount(userLanguages) >= 1) {
            result = _CFBundleCreateMutableArrayOfFallbackLanguages(locArray, userLanguages);
        }
        if (result == NULL && devLang != NULL) {
            OCFArrayRef devLangArray =
                OCFArrayCreate(kOCFAllocatorSystemDefault, &values, 1, &kOCFTypeArrayCallBacks);
            result = _CFBundleCreateMutableArrayOfFallbackLanguages(locArray, devLangArray);
            OCFRelease(devLangArray);
        }
        if (result == NULL) {
            const void *defaultValues[1] = {OF_STR("en_US")};
            OCFArrayRef defaultArray =
                OCFArrayCreate(kOCFAllocatorSystemDefault, defaultValues, 1, &kOCFTypeArrayCallBacks);
            result = _CFBundleCreateMutableArrayOfFallbackLanguages(locArray, defaultArray);
            OCFRelease(defaultArray);
            if (result == NULL) {
                result = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
            }
        }
    }
    if (os_log_type_enabled(_CFBundleResourceLogger(), OS_LOG_TYPE_DEBUG) &&
        ((uintptr_t)bundleURL | (uintptr_t)bundle) != 0) {
        const void *devLangValue = values;
        OCFMutableStringRef bundleDescription = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
        OCFMutableStringRef localizationsDescription = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
        OCFMutableStringRef userDescription = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
        OCFMutableStringRef mainDescription = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
        OCFMutableStringRef resultDescription = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
        OCFStringAppendFormat(
            bundleDescription, NULL, OF_STR("%@"), bundle != NULL ? (OCFTypeRef)bundle : (OCFTypeRef)bundleURL
        );
        _onelineFormatArray(localizationsDescription, locArray);
        _onelineFormatArray(userDescription, userLanguages);
        _onelineFormatArray(mainDescription, mainLanguages);
        _onelineFormatArray(resultDescription, result);
        unsigned long allowMixed = CFBundleAllowMixedLocalizations() != 0;
        const char *useAppleLocalizations = "";
        if (bundle != NULL) {
            OCFDictionaryRef infoDict = OCFBundleGetInfoDictionary(bundle);
            if (infoDict != NULL &&
                OCFDictionaryGetValue(infoDict, _kCFBundleUseAppleLocalizationsKey) == kOCFBooleanTrue) {
                useAppleLocalizations = "(use Apple Locs)";
            }
        }
        os_log_debug(
            _CFBundleResourceLogger(),
            "Language lookup at %{private}@\n\tLocalizations : %{public}@\n\tDev language  : "
            "%{public}@\n\tUser "
            "prefs    : %{public}@\n\tMain bundle   : %{public}@\n\tAllow mixed   : %{public}lu "
            "%{public}s\n\tResult "
            "       : %{public}@\n",
            bundleDescription,
            localizationsDescription,
            devLangValue,
            userDescription,
            mainDescription,
            allowMixed,
            useAppleLocalizations,
            resultDescription
        );
        OCFRelease(bundleDescription);
        OCFRelease(localizationsDescription);
        OCFRelease(userDescription);
        OCFRelease(resultDescription);
        OCFRelease(mainDescription);
    }
    if (mainLanguages != NULL) {
        OCFRelease(mainLanguages);
    }
    return result;
}

// Native _CFBundleCopyLanguageSearchListInDirectory, 0x1c01f5f8c. Hidden (was private external). Reads the
// directory's Info.plist (kCFAllocatorSystemDefault, which also reports the bundle version): a nonempty
// string development region (kCFBundleDevelopmentRegionKey) and the kCFBundleLocalizationsKey array, or
// _copyAppleLocalizations(infoDict) when that value is missing or not an array. The .lproj names of the
// resources directory for that version are appended to a mutable copy of the localizations (or stand alone;
// an empty array when there are neither). The answer is the preferred languages for the user languages
// (considerMain 1, the URL, no bundle), with the development region appended when absent and "en" when empty;
// the version is stored through version when it is not NULL.
OCFMutableArrayRef _CFBundleCopyLanguageSearchListInDirectory(OCFURLRef url, uint8_t *version) {
    uint8_t localVersion = 0;
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFDictionaryRef infoDict = _CFBundleCopyInfoDictionaryInDirectory(allocator, url, &localVersion);
    OCFStringRef devLang = NULL;
    OCFArrayRef predefinedLocalizations = NULL;
    if (infoDict != NULL) {
        devLang = OCFDictionaryGetValue(infoDict, kOCFBundleDevelopmentRegionKey);
        if (devLang != NULL &&
            (OCFGetTypeID(devLang) != OCFStringGetTypeID() || OCFStringGetLength(devLang) == 0)) {
            devLang = NULL;
        }
        predefinedLocalizations = OCFDictionaryGetValue(infoDict, kOCFBundleLocalizationsKey);
        if (predefinedLocalizations != NULL) {
            OCFRetain(predefinedLocalizations);
            if (OCFGetTypeID(predefinedLocalizations) != OCFArrayGetTypeID()) {
                OCFRelease(predefinedLocalizations);
                predefinedLocalizations = _copyAppleLocalizations(infoDict);
            }
        } else {
            predefinedLocalizations = _copyAppleLocalizations(infoDict);
        }
    }
    OCFURLRef resourcesURL = _CFBundleCopyResourcesDirectoryURLInDirectory(url, localVersion);
    OCFArrayRef localizations = _CFBundleCopyLProjDirectoriesForURL(allocator, resourcesURL);
    OCFRelease(resourcesURL);
    if (predefinedLocalizations != NULL && localizations != NULL) {
        OCFMutableArrayRef combined = OCFArrayCreateMutableCopy(allocator, 0, predefinedLocalizations);
        OCFArrayAppendArray(combined, localizations, (OCFRange){0, OCFArrayGetCount(localizations)});
        OCFRelease(localizations);
        localizations = combined;
        OCFRelease(predefinedLocalizations);
    } else if (predefinedLocalizations != NULL) {
        localizations = OCFRetain(predefinedLocalizations);
        OCFRelease(predefinedLocalizations);
    } else if (localizations == NULL) {
        localizations = OCFArrayCreate(allocator, NULL, 0, &kOCFTypeArrayCallBacks);
    }
    OCFArrayRef userLanguages = _CFBundleCopyUserLanguages();
    OCFMutableArrayRef result =
        _CFBundleCopyPreferredLanguagesInList(localizations, devLang, userLanguages, true, url, NULL);
    if (userLanguages != NULL) {
        OCFRelease(userLanguages);
    }
    OCFRelease(localizations);
    if (devLang != NULL &&
        OCFArrayGetFirstIndexOfValue(result, (OCFRange){0, OCFArrayGetCount(result)}, devLang) < 0) {
        OCFArrayAppendValue(result, devLang);
    }
    if (OCFArrayGetCount(result) == 0) {
        OCFArrayAppendValue(result, OF_STR("en"));
    }
    if (infoDict != NULL) {
        OCFRelease(infoDict);
    }
    if (version != NULL) {
        *version = localVersion;
    }
    return result;
}

// Native _CFBundleCopyLProjDirectoriesForURL, 0x1c01f620c, with its block
// ___CFBundleCopyLProjDirectoriesForURL_block_invoke (0x1c01f7068; descriptor 0x2227a4bd8, copy/dispose and
// signature flags 0xc2000000). Non-external. Iterates the POSIX path of the absolute URL (no slashes
// appended, no prefix); every name longer than ".lproj" that ends with it (CFStringCompareWithOptions over
// the tail range, options 0) contributes its stem (a kCFAllocatorSystemDefault substring) to an array created
// on the first match with the given allocator. The block always answers true. NULL when nothing matched.
static OCFMutableArrayRef _CFBundleCopyLProjDirectoriesForURL(OCFAllocatorRef allocator, OCFURLRef url) {
    __block OCFMutableArrayRef result = NULL;
    OCFURLRef absoluteURL = OCFURLCopyAbsoluteURL(url);
    OCFStringRef directoryPath = OCFURLCopyFileSystemPath(absoluteURL, kOCFURLPOSIXPathStyle);
    OCFRelease(absoluteURL);
    OCFStringRef extension = OF_STR(".lproj");
    OCFIndex extensionLength = OCFStringGetLength(extension);
    _CFIterateDirectory(
        directoryPath,
        false,
        NULL,
        ^Boolean(OCFStringRef fileName, OCFStringRef fileNameWithPrefix, uint8_t fileType) {
          (void)fileNameWithPrefix;
          (void)fileType;
          OCFIndex length = OCFStringGetLength(fileName);
          if (length > extensionLength &&
              OCFStringCompareWithOptions(
                  fileName, extension, (OCFRange){length - extensionLength, extensionLength}, 0
              ) == kOCFCompareEqualTo) {
              OCFStringRef language = OCFStringCreateWithSubstring(
                  kOCFAllocatorSystemDefault, fileName, (OCFRange){0, length - extensionLength}
              );
              if (result == NULL) {
                  result = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
              }
              OCFArrayAppendValue(result, language);
              OCFRelease(language);
          }
          return true;
        }
    );
    OCFRelease(directoryPath);
    return result;
}

// Native _CFBundleCopyXPCBootstrapMainBundleLanguages, 0x1c01f6338, with its .cold.1 (0x1c030c37c, the
// dispatch_once of ___block_literal_global.143), the block
// ___CFBundleCopyXPCBootstrapMainBundleLanguages_block_invoke (0x1c01f68fc) and its applier
// ___CFBundleCopyXPCBootstrapMainBundleLanguages_block_invoke_2 (0x1c01f699c,
// ___block_literal_global.145). Non-external. Once, the bootstrap dictionary's "_NSBundleEntry" dictionary's
// "MainBundleLanguages" value, when it is an XPC array, becomes a default-allocator mutable array (capacity
// the XPC count) of its XPC strings as UTF-8 CFStrings (strings that fail to convert are skipped; the applier
// always continues); the bootstrap object is released. The cached array is answered retained, or NULL.
static OCFArrayRef _CFBundleCopyXPCBootstrapMainBundleLanguages(void) {
    dispatch_once(&xpcBootstrapMainBundleLanguagesOnce, ^{
      xpc_object_t bootstrap = xpc_copy_bootstrap();
      if (bootstrap == NULL) {
          return;
      }
      xpc_object_t entry = xpc_dictionary_get_value(bootstrap, "_NSBundleEntry");
      if (entry != NULL) {
          xpc_object_t languages = xpc_dictionary_get_value(entry, "MainBundleLanguages");
          if (languages != NULL && xpc_get_type(languages) == XPC_TYPE_ARRAY) {
              xpcBootstrapMainBundleLanguages = OCFArrayCreateMutable(
                  NULL, (OCFIndex)xpc_array_get_count(languages), &kOCFTypeArrayCallBacks
              );
              xpc_array_apply(languages, ^bool(size_t index, xpc_object_t value) {
                (void)index;
                if (xpc_get_type(value) == XPC_TYPE_STRING) {
                    const char *bytes = xpc_string_get_string_ptr(value);
                    size_t length = xpc_string_get_length(value);
                    OCFStringRef language = OCFStringCreateWithBytes(
                        NULL, (const UInt8 *)bytes, (OCFIndex)length, kOCFStringEncodingUTF8, false
                    );
                    if (language != NULL) {
                        OCFArrayAppendValue(xpcBootstrapMainBundleLanguages, language);
                        OCFRelease(language);
                    }
                }
                return true;
              });
          }
      }
      xpc_release(bootstrap);
    });
    if (xpcBootstrapMainBundleLanguages == NULL) {
        return NULL;
    }
    return OCFRetain(xpcBootstrapMainBundleLanguages);
}

// Native _copyBundleLocalizationsFromResources, 0x1c01f6370. Non-external. The second argument (X1) is a mode
// compared with 0 and 1: 1 always requires a "Base" localization before the development region is added, 0
// requires it only for programs built with an SDK older than 0x07E60901 (dyld_program_sdk_at_least, every
// platform), and any other value never adds the region. The list starts as the string elements of positive
// length of the Info.plist CFBundleLocalizations array (copied in the bundle's allocator), or, without such
// an array, _copyAppleLocalizations; the .lproj directories of the resources directory are appended (a
// mutable copy in kCFAllocatorDefault). Without any list, the development region alone (or an empty array) in
// the bundle's allocator. With a development region that no listed language matches (directly, through its
// alternate name, through its "_"/"-" variant at index 2 of a name of four or more characters, as the default
// language of a listed language or of its alternate name, or by canonical language identifier), the region is
// appended to a mutable copy (kCFAllocatorDefault). A match retains and releases the matched string
// (CFRetain, then CFRelease when not NULL). The counts read after the "Base" test are those of the list at
// that point.
static OCFArrayRef _copyBundleLocalizationsFromResources(OCFBundleRef bundle, const void *mode) {
    OCFArrayRef localizations = NULL;
    OCFDictionaryRef infoDict = OCFBundleGetInfoDictionary(bundle);
    if (infoDict != NULL) {
        OCFArrayRef predefined = OCFDictionaryGetValue(infoDict, kOCFBundleLocalizationsKey);
        if (predefined != NULL && OCFGetTypeID(predefined) == OCFArrayGetTypeID()) {
            OCFMutableArrayRef strings = OCFArrayCreateMutable(
                OCFGetAllocator(bundle), OCFArrayGetCount(predefined), &kOCFTypeArrayCallBacks
            );
            for (OCFIndex index = 0; index < OCFArrayGetCount(predefined); index++) {
                OCFStringRef value = OCFArrayGetValueAtIndex(predefined, index);
                if (OCFGetTypeID(value) == OCFStringGetTypeID() && OCFStringGetLength(value) >= 1) {
                    OCFArrayAppendValue(strings, value);
                }
            }
            localizations = OCFArrayCreateCopy(OCFGetAllocator(bundle), strings);
            OCFRelease(strings);
        } else {
            localizations = _copyAppleLocalizations(infoDict);
        }
    }
    OCFURLRef resourcesURL = OCFBundleCopyResourcesDirectoryURL(bundle);
    if (resourcesURL != NULL) {
        OCFArrayRef lprojs = _CFBundleCopyLProjDirectoriesForURL(OCFGetAllocator(bundle), resourcesURL);
        if (lprojs != NULL) {
            if (localizations != NULL) {
                OCFMutableArrayRef merged = OCFArrayCreateMutableCopy(kOCFAllocatorDefault, 0, localizations);
                OCFRelease(localizations);
                OCFArrayAppendArray(merged, lprojs, (OCFRange){0, OCFArrayGetCount(lprojs)});
                OCFRelease(lprojs);
                localizations = merged;
            } else {
                localizations = lprojs;
            }
        }
        OCFRelease(resourcesURL);
    }
    OCFStringRef devLang = OCFBundleGetDevelopmentRegion(bundle);
    if (localizations == NULL) {
        OCFAllocatorRef allocator = OCFGetAllocator(bundle);
        if (devLang != NULL) {
            return OCFArrayCreate(allocator, (const void **)&devLang, 1, &kOCFTypeArrayCallBacks);
        }
        return OCFArrayCreate(allocator, NULL, 0, &kOCFTypeArrayCallBacks);
    }
    if (devLang == NULL) {
        return localizations;
    }
    OCFIndex count = OCFArrayGetCount(localizations);
    if ((uintptr_t)mode == 1) {
        if (!OCFArrayContainsValue(localizations, (OCFRange){0, count}, OF_STR("Base"))) {
            return localizations;
        }
    } else if (mode != NULL) {
        return localizations;
    } else if (!dyld_program_sdk_at_least(kBaseLocalizationVersion)) {
        if (!OCFArrayContainsValue(localizations, (OCFRange){0, count}, OF_STR("Base"))) {
            return localizations;
        }
    }
    count = OCFArrayGetCount(localizations);
    OCFStringRef match = NULL;
    OCFStringRef created = NULL;
    if (OCFArrayContainsValue(localizations, (OCFRange){0, count}, devLang)) {
        match = devLang;
    } else {
        OCFStringRef alternate = _CFBundleGetAlternateNameForLanguage(devLang);
        if (alternate != NULL && OCFArrayContainsValue(localizations, (OCFRange){0, count}, alternate)) {
            match = alternate;
        }
    }
    if (match == NULL) {
        OCFIndex length = OCFStringGetLength(devLang);
        if (length >= 4) {
            UniChar separator = OCFStringGetCharacterAtIndex(devLang, 2);
            if (separator == '_' || separator == '-') {
                OCFMutableStringRef variant =
                    OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, length, devLang);
                OCFStringReplace(variant, (OCFRange){2, 1}, separator == '-' ? OF_STR("_") : OF_STR("-"));
                if (variant != NULL) {
                    if (OCFArrayContainsValue(localizations, (OCFRange){0, count}, variant)) {
                        created = variant;
                    } else {
                        OCFRelease(variant);
                    }
                }
            }
        }
    }
    if (match == NULL && created == NULL) {
        OCFStringRef candidate = NULL;
        for (OCFIndex index = 0; index < OCFArrayGetCount(localizations); index++) {
            OCFStringRef language = OCFArrayGetValueAtIndex(localizations, index);
            OCFStringRef devAlternate = _CFBundleGetAlternateNameForLanguage(devLang);
            OCFStringRef languageAlternate = _CFBundleGetAlternateNameForLanguage(language);
            if (_CFBundleIsLanguageDefaultLanguageOfOther(devLang, language) ||
                _CFBundleIsLanguageDefaultLanguageOfOther(devAlternate, language) ||
                _CFBundleIsLanguageDefaultLanguageOfOther(devLang, languageAlternate)) {
                candidate = language;
                break;
            }
        }
        if (candidate != NULL && OCFArrayContainsValue(localizations, (OCFRange){0, count}, candidate)) {
            match = candidate;
        }
    }
    if (match == NULL && created == NULL) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFStringRef canonical = OCFLocaleCreateCanonicalLanguageIdentifierFromString(allocator, devLang);
        if (canonical != NULL) {
            if (OCFArrayContainsValue(localizations, (OCFRange){0, count}, canonical)) {
                created = canonical;
            } else {
                for (OCFIndex index = 0; index < count; index++) {
                    OCFStringRef language = OCFArrayGetValueAtIndex(localizations, index);
                    OCFStringRef other =
                        OCFLocaleCreateCanonicalLanguageIdentifierFromString(allocator, language);
                    if (other != NULL) {
                        Boolean equal = OCFEqual(other, canonical);
                        OCFRelease(other);
                        if (equal) {
                            OCFRelease(canonical);
                            match = language;
                            break;
                        }
                    }
                }
                if (match == NULL) {
                    OCFRelease(canonical);
                }
            }
        }
    }
    if (match != NULL) {
        OCFTypeRef retained = OCFRetain(match);
        if (retained != NULL) {
            OCFRelease(retained);
            return localizations;
        }
    } else if (created != NULL) {
        OCFRelease(created);
        return localizations;
    }
    OCFMutableArrayRef result = OCFArrayCreateMutableCopy(kOCFAllocatorDefault, 0, localizations);
    OCFRelease(localizations);
    OCFArrayAppendValue(result, devLang);
    return result;
}

// Native __CFBundleGetAlternateNameForLanguage.__CFBundleCommonLanguageAbbreviationsArray and
// .__CFBundleCommonLanguageNamesArray (__DATA_CONST,__const 0x2227a4b98 and 0x2227a4b60), seven constant
// strings each (__cfstring, flags 0x7c8); non-external. The constant strings are built as OF_STR builds them.
#define LANGUAGE_CONSTANT_STRING(name, literal)                                                              \
    static const OCFConstantString name = {                                                                  \
        (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "" literal, sizeof(literal) - 1                 \
    }

LANGUAGE_CONSTANT_STRING(kAbbreviationEnglish, "en");
LANGUAGE_CONSTANT_STRING(kAbbreviationFrench, "fr");
LANGUAGE_CONSTANT_STRING(kAbbreviationGerman, "de");
LANGUAGE_CONSTANT_STRING(kAbbreviationItalian, "it");
LANGUAGE_CONSTANT_STRING(kAbbreviationDutch, "nl");
LANGUAGE_CONSTANT_STRING(kAbbreviationSpanish, "es");
LANGUAGE_CONSTANT_STRING(kAbbreviationJapanese, "ja");
LANGUAGE_CONSTANT_STRING(kNameEnglish, "English");
LANGUAGE_CONSTANT_STRING(kNameFrench, "French");
LANGUAGE_CONSTANT_STRING(kNameGerman, "German");
LANGUAGE_CONSTANT_STRING(kNameItalian, "Italian");
LANGUAGE_CONSTANT_STRING(kNameDutch, "Dutch");
LANGUAGE_CONSTANT_STRING(kNameSpanish, "Spanish");
LANGUAGE_CONSTANT_STRING(kNameJapanese, "Japanese");

#undef LANGUAGE_CONSTANT_STRING

// Native _CFBundleGetAlternateNameForLanguage, 0x1c01f6a24. Non-external. For
// each of the seven pairs in order, a language equal (CFEqual) to the abbreviation answers the name, else one
// equal to the name answers the abbreviation; NULL otherwise.
static OCFStringRef _CFBundleGetAlternateNameForLanguage(OCFStringRef language) {
    static const OCFStringRef abbreviations[7] = {
        (OCFStringRef)&kAbbreviationEnglish,
        (OCFStringRef)&kAbbreviationFrench,
        (OCFStringRef)&kAbbreviationGerman,
        (OCFStringRef)&kAbbreviationItalian,
        (OCFStringRef)&kAbbreviationDutch,
        (OCFStringRef)&kAbbreviationSpanish,
        (OCFStringRef)&kAbbreviationJapanese,
    };
    static const OCFStringRef names[7] = {
        (OCFStringRef)&kNameEnglish,
        (OCFStringRef)&kNameFrench,
        (OCFStringRef)&kNameGerman,
        (OCFStringRef)&kNameItalian,
        (OCFStringRef)&kNameDutch,
        (OCFStringRef)&kNameSpanish,
        (OCFStringRef)&kNameJapanese,
    };
    for (OCFIndex index = 0; index < 7; index++) {
        OCFStringRef abbreviation = abbreviations[index];
        Boolean isAbbreviation = OCFEqual(language, abbreviation);
        OCFStringRef name = names[index];
        if (isAbbreviation) {
            return name;
        }
        if (OCFEqual(language, name)) {
            return abbreviation;
        }
    }
    return NULL;
}

// Native _CFBundleIsLanguageDefaultLanguageOfOther, 0x1c01f6aa4. Non-external.
// False for a NULL argument or a missing default language; otherwise whether the two default languages
// compare equal case-insensitively (kCFCompareCaseInsensitive). Both copies are released.
static Boolean _CFBundleIsLanguageDefaultLanguageOfOther(OCFStringRef language, OCFStringRef other) {
    if (language == NULL || other == NULL) {
        return false;
    }
    OCFStringRef languageDefault = _CFBundleCopyDefaultLanguageForLanguage(language);
    if (languageDefault == NULL) {
        return false;
    }
    Boolean result = false;
    OCFStringRef otherDefault = _CFBundleCopyDefaultLanguageForLanguage(other);
    if (otherDefault != NULL) {
        result =
            OCFStringCompare(languageDefault, otherDefault, kOCFCompareCaseInsensitive) == kOCFCompareEqualTo;
        OCFRelease(languageDefault);
        languageDefault = otherDefault;
    }
    OCFRelease(languageDefault);
    return result;
}

// Native _CFBundleCopyDefaultLanguageForLanguage, 0x1c01f6b30. Non-external. Under defaultLanguageLock
// (options 0x10000) the cache (a kCFAllocatorSystemDefault dictionary with
// kCFCopyStringDictionaryKeyCallBacks and kCFTypeDictionaryValueCallBacks, created on first use) answers a
// kCFAllocatorDefault copy of a cached value. Otherwise, unlocked, the identifier with likely subtags added
// is computed; a non-NULL one is cached under the lock again and answered as created (+1, the cache holding
// its own retain).
static OCFStringRef _CFBundleCopyDefaultLanguageForLanguage(OCFStringRef language) {
    os_unfair_lock_lock_with_options(&defaultLanguageLock, 0x10000);
    if (defaultLanguageCache == NULL) {
        defaultLanguageCache = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault,
            0,
            &kOCFCopyStringDictionaryKeyCallBacks,
            &kOCFTypeDictionaryValueCallBacks
        );
    }
    OCFStringRef cached = OCFDictionaryGetValue(defaultLanguageCache, language);
    OCFStringRef result;
    if (cached != NULL) {
        result = OCFStringCreateCopy(kOCFAllocatorDefault, cached);
    } else {
        os_unfair_lock_unlock(&defaultLanguageLock);
        result = _CFLocaleCopyLocaleIdentifierByAddingLikelySubtags(language);
        if (result == NULL) {
            return NULL;
        }
        os_unfair_lock_lock_with_options(&defaultLanguageLock, 0x10000);
        OCFDictionarySetValue(defaultLanguageCache, language, result);
    }
    os_unfair_lock_unlock(&defaultLanguageLock);
    return result;
}

// Native _CFBundleCreateMutableArrayOfFallbackLanguages, 0x1c01f6c0c, with its block
// ___CFBundleCreateMutableArrayOfFallbackLanguages_block_invoke (0x1c01f6e18), which captures nothing and is
// called directly natively. Non-external. Each array gets a pointer table (8 bytes per element, malloc type
// 0x10040436913F5, unchecked) that the block fills with ASCII C strings packed into one block of 157 bytes
// per element (malloc type 0x30182EBA): only CFString elements are converted, each through at most 156
// characters and at most the bytes left in the block minus one, and the whole conversion fails (NULL, block
// freed) when a conversion does not cover the string's full length; a table with no string at all also fails,
// and an empty array gives NULL without allocating the block. With both blocks, ualoc_localizationsToUse
// (preferences first, then the available localizations, counts as 32-bit values, a 512-entry answer table
// whose first 512 bytes are cleared, status 0) gives the localizations to use, each appended as a UTF-8
// kCFAllocatorSystemDefault string when it converts. Both tables and both blocks are freed; an empty answer
// is released and NULL is returned. CoreFoundation defect: the table slot of a non-string element is never
// written, yet the full element count is passed to ualoc_localizationsToUse, which reads the uninitialized
// pointer (malloc does not clear the table). Kept native pending an owner decision; the divergence would be
// clearing or compacting the table.
static OCFMutableArrayRef
_CFBundleCreateMutableArrayOfFallbackLanguages(OCFArrayRef availableLocalizations, OCFArrayRef preferences) {
    char * (^createCStringArray)(OCFArrayRef, const char **) =
        ^char *(OCFArrayRef array, const char **table) {
          OCFIndex count = OCFArrayGetCount(array);
          if (count == 0) {
              return NULL;
          }
          OCFIndex size = count * 157;
          char *buffer = malloc_type_malloc((size_t)size, 0x30182EBA);
          if (buffer == NULL) {
              return NULL;
          }
          if (count - 1 < 0) {
              free(buffer);
              return NULL;
          }
          char *end = buffer + size;
          Boolean noneConverted = true;
          char *cursor = buffer;
          for (OCFIndex index = 0; index < count; index++) {
              OCFStringRef string = OCFArrayGetValueAtIndex(array, index);
              if (OCFGetTypeID(string) != OCFStringGetTypeID()) {
                  continue;
              }
              OCFIndex usedLength = 0;
              OCFIndex length = OCFStringGetLength(string);
              OCFIndex converted = OCFStringGetBytes(
                  string,
                  (OCFRange){0, length < 156 ? length : 156},
                  kOCFStringEncodingASCII,
                  0,
                  false,
                  (UInt8 *)cursor,
                  (OCFIndex)(end - cursor - 1),
                  &usedLength
              );
              if (converted != length) {
                  free(buffer);
                  return NULL;
              }
              noneConverted = false;
              table[index] = cursor;
              cursor += usedLength;
              *cursor++ = 0;
              if (index == count - 1) {
                  return buffer;
              }
          }
          if (noneConverted) {
              free(buffer);
              return NULL;
          }
          return buffer;
        };
    OCFIndex availableCount = OCFArrayGetCount(availableLocalizations);
    const char **availableTable = malloc_type_malloc((size_t)(availableCount << 3), 0x10040436913F5);
    char *availableBuffer = createCStringArray(availableLocalizations, availableTable);
    OCFIndex preferredCount = OCFArrayGetCount(preferences);
    const char **preferredTable = malloc_type_malloc((size_t)(preferredCount << 3), 0x10040436913F5);
    char *preferredBuffer = createCStringArray(preferences, preferredTable);
    OCFMutableArrayRef result = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
    if (availableBuffer != NULL && preferredBuffer != NULL) {
        char *localizationsToUse[512];
        memset(localizationsToUse, 0, 512);
        int status = 0;
        int32_t usedCount = ualoc_localizationsToUse(
            preferredTable,
            (int32_t)preferredCount,
            availableTable,
            (int32_t)availableCount,
            localizationsToUse,
            512,
            &status
        );
        if (usedCount >= 1) {
            for (uint32_t index = 0; index < (uint32_t)usedCount; index++) {
                OCFStringRef language = OCFStringCreateWithCString(
                    kOCFAllocatorSystemDefault, localizationsToUse[index], kOCFStringEncodingUTF8
                );
                if (language != NULL) {
                    OCFArrayAppendValue(result, language);
                    OCFRelease(language);
                }
            }
        }
    }
    if (availableTable != NULL) {
        free(availableTable);
    }
    if (availableBuffer != NULL) {
        free(availableBuffer);
    }
    if (preferredTable != NULL) {
        free(preferredTable);
    }
    if (preferredBuffer != NULL) {
        free(preferredBuffer);
    }
    if (OCFArrayGetCount(result) == 0) {
        OCFRelease(result);
        return NULL;
    }
    return result;
}

// Native _onelineFormatArray, 0x1c01f6f9c. Non-external. Appends "[", then every element followed by ", "
// (through _onelineFormatArrayApply) with the final two characters deleted when the array is not empty, or
// "<null>" for a NULL array, then "]".
static void _onelineFormatArray(OCFMutableStringRef string, OCFArrayRef array) {
    OCFStringAppend(string, OF_STR("["));
    if (array != NULL) {
        OCFArrayApplyFunction(
            array, (OCFRange){0, OCFArrayGetCount(array)}, _onelineFormatArrayApply, string
        );
        if (OCFArrayGetCount(array) >= 1) {
            OCFStringDelete(string, (OCFRange){OCFStringGetLength(string) - 2, 2});
        }
    } else {
        OCFStringAppend(string, OF_STR("<null>"));
    }
    OCFStringAppend(string, OF_STR("]"));
}

// Native _onelineFormatArrayApply, 0x1c01f7038. Non-external. CFStringAppendFormat(context, NULL, "%@, ",
// value).
static void _onelineFormatArrayApply(const void *value, void *context) {
    OCFStringAppendFormat((OCFMutableStringRef)context, NULL, OF_STR("%@, "), value);
}
