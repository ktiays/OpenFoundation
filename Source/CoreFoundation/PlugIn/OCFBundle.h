//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/URL/OCFURL.h"
#include "CoreFoundation/String/OCFString.h"

#include <os/lock.h>
#include <os/log.h>
#include <stdbool.h>
#include <stdint.h>

// Public types, SDK CFBundle.h spelled OCF.
typedef struct __OCFBundle *OCFBundleRef;
typedef struct __OCFBundle *OCFPlugInRef;

// The plug-in data embedded at offset 64 of a bundle (__CFBundleGetPlugInData answers bundle + 64), iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. Offsets are those of the native loads and
// stores; field names are local descriptive spellings.
typedef struct {
    uint8_t _isPlugIn;             // +0: tested by every plug-in function.
    uint8_t _flag1;                // +1: set to 1 by _CFBundleInitPlugIn.
    uint8_t _flag2;                // +2: cleared by _CFBundleInitPlugIn.
    uint8_t _flag3;                // +3: cleared by _CFBundleInitPlugIn.
    uint8_t _registeredFactories;  // +4: set by _CFBundleInitPlugIn once the factories are registered.
    uint8_t _unread5[3];           // +5..+7
    uint32_t _word8;               // +8: cleared by _CFBundleInitPlugIn.
    uint32_t _instanceCount;       // +12: tested by _CFPlugInUnloadScheduledPlugIns.
    OCFMutableArrayRef _factories; // +16: the factories registered by the plug-in.
} _CFPlugInData;

OF_ABI_ASSERT(sizeof(_CFPlugInData) == 24, "The plug-in data spans bundle offsets 64 to 88.");

// The CFBundle instance, iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC: _CFBundleCreate
// (0x1c016b748) asks _CFRuntimeCreateInstance for 200 bytes after the 16-byte header (216 in all). Offsets
// are those of the native loads and stores; field names are local descriptive spellings; `_unread` fields
// are touched by no function restored here.
struct __OCFBundle {
    OCFRuntimeBase _base;              // +0
    OCFURLRef _url;                    // +16
    OCFDictionaryRef _infoDict;        // +24
    OCFDictionaryRef _localInfoDict;   // +32: released by __CFBundleDeallocate.
    OCFArrayRef _searchLanguages;      // +40: released by __CFBundleDeallocate.
    uint32_t _binaryType;              // +48
    uint8_t _isLoaded;                 // +52
    uint8_t _version;                  // +53
    uint8_t _isUnique;                 // +54: _CFBundleAddToTablesLocked skips such a bundle.
    uint8_t _createOption1;            // +55: bit 0 of the _CFBundleCreate options.
    void *_handleCookie;               // +56: the dlopen handle (RTLD_MAIN_ONLY, -5, for the main bundle).
    _CFPlugInData _plugInData;         // +64
    os_unfair_lock _bundleLoadingLock; // +88: guards _isLoaded.
    uint32_t _unread92;                // +92
    OCFStringRef _executablePath;      // +96: released by __CFBundleDeallocate.
    os_unfair_lock _lock104;           // +104: guards +40, +96 and +112/+120.
    uint32_t _unread108;               // +108
    OCFArrayRef _localizations;        // +112: the cached bundle localizations (under _lock104).
    uint8_t _lookedForLocalizations;   // +120: set once _localizations is computed.
    uint8_t _unread121[7];             // +121
    OCFTypeRef _field128;              // +128: released by __CFBundleDeallocate.
    OCFMutableDictionaryRef
        _stringTables;                // +136: table name -> cached strings (_copyStringTable, under +104).
    OCFMutableSetRef _completeTables; // +144: tables whose strings are all cached (under +104).
    OCFMutableDictionaryRef _missingKeys;    // +152: table name -> keys known to be absent (under +104).
    OCFMutableDictionaryRef _stringsSources; // +160: table name -> cached strings sources (under +104).
    os_unfair_lock _lock168;                 // +168: cleared by _CFBundleCreate; guards +176.
    uint32_t _unread172;                     // +172
    OCFMutableDictionaryRef
        _queryTables;             // +176: resource query tables by path (_copyQueryTable, under +168).
    OCFStringRef _bundleBasePath; // +184: the POSIX path of the absolute bundle URL.
    os_unfair_lock _lock192;      // +192: cleared by _CFBundleCreate.
    uint32_t _unread196;          // +196
    OCFDictionaryRef _additionalResourceBundles; // +200: bundles searched for resources (under _lock192).
    OCFURLRef _infoPlistURL;                     // +208: the Info.plist URL (the assertion text of
                                                 // _CFBundleLoadLimitedInfoDictionary names it).
};

OF_ABI_ASSERT(sizeof(struct __OCFBundle) == 216, "A CFBundle instance is 216 bytes.");

// The bundle class record (type 31), iOS Simulator 24A434, 0x22279cee8. Original private-external name is
// preserved (hidden).
OF_EXTERN const OCFRuntimeClass __CFBundleClass;

// Native exported Info.plist keys referenced by the restored functions (the key table at __DATA_CONST,__const
// 0x22279cce8-0x22279ce60 of the CFBundle.c unit).
OF_EXPORT const OCFStringRef kOCFBundleIdentifierKey;
OF_EXPORT const OCFStringRef kOCFBundleVersionKey;
OF_EXPORT const OCFStringRef _kOCFBundleNumericVersionKey;
OF_EXPORT const OCFStringRef kOCFBundleDevelopmentRegionKey;
OF_EXPORT const OCFStringRef kOCFBundleLocalizationsKey;
OF_EXPORT const OCFStringRef _kOCFBundleAllowMixedLocalizationsKey;
OF_EXPORT const OCFStringRef _kOCFBundleShortVersionStringKey;
// Was a private external (hidden; native name).
OF_EXTERN const OCFStringRef _kCFBundleUseAppleLocalizationsKey;

// The bundle global lock (__DATA,__bss 0x22e6d2740). Non-external natively; shared by the CFBundle units.
OF_EXTERN os_unfair_lock CFBundleGlobalDataLock;

// Public interfaces of the CFBundle.c unit (SDK CFBundle.h, spelled OCF).
OF_EXPORT OCFBundleRef OCFBundleCreate(OCFAllocatorRef allocator, OCFURLRef bundleURL);
OF_EXPORT OCFBundleRef OCFBundleGetBundleWithIdentifier(OCFStringRef bundleID);
OF_EXPORT OCFURLRef OCFBundleCopyBundleURL(OCFBundleRef bundle);
OF_EXPORT UInt32 OCFBundleGetVersionNumber(OCFBundleRef bundle);
OF_EXPORT OCFStringRef OCFBundleGetDevelopmentRegion(OCFBundleRef bundle);
OF_EXPORT OCFURLRef OCFBundleCopyResourcesDirectoryURL(OCFBundleRef bundle);
OF_EXPORT Boolean OCFBundleIsExecutableLoaded(OCFBundleRef bundle);
OF_EXPORT void OCFBundleUnloadExecutable(OCFBundleRef bundle);
OF_EXPORT OCFArrayRef OCFBundleCopyExecutableArchitectures(OCFBundleRef bundle);

// Natively exported private interfaces of the CFBundle.c unit.
OF_EXPORT OCFBundleRef _OCFBundleCreateUnique(OCFAllocatorRef allocator, OCFURLRef bundleURL);
OF_EXPORT OCFBundleRef _OCFBundleGetBundleWithIdentifierWithHint(OCFStringRef bundleID, void *pointer);
OF_EXPORT OCFBundleRef _OCFBundleCreateUniqueWithOptions(
    OCFAllocatorRef allocator, OCFURLRef bundleURL, OCFSetRef limitedKeyPaths, OCFOptionFlags options
);
OF_EXPORT OCFErrorRef _OCFBundleCreateError(OCFAllocatorRef allocator, OCFBundleRef bundle, OCFIndex code);
OF_EXPORT Boolean
_OCFBundleLoadExecutableAndReturnError(OCFBundleRef bundle, Boolean forceGlobal, OCFErrorRef *error);

// Hidden interfaces of the CFBundle.c unit (were private externals; original names are preserved).
OF_EXTERN os_log_t _CFBundleResourceLogger(void);
OF_EXTERN os_log_t _CFBundleLocalizedStringLogger(void);
OF_EXTERN os_log_t _CFBundleLoadingLogger(void);
OF_EXTERN const char *_CFBundleNormalizedPath(const char *path);
OF_EXTERN OCFURLRef _CFBundleCreateNormalizedURL(OCFAllocatorRef allocator, OCFURLRef url, Boolean keepPath);
OF_EXTERN Boolean _CFBundleResourceURLIsSafeForMapping(OCFURLRef url);
OF_EXTERN OCFDataRef _CFBundleCreateMappedOrLoadedPlistData(
    OCFURLRef url, Boolean allowMapping, Boolean flag, Boolean *mapped, OCFErrorRef *error
);
OF_EXTERN uint8_t _CFBundleEffectiveLayoutVersion(OCFBundleRef bundle);
OF_EXTERN void _CFBundleLogSourceLocation(Boolean enabled, const char *file, int line, const char *function);
OF_EXTERN OCFBundleRef _CFBundleCreateMain(OCFAllocatorRef allocator, OCFURLRef mainBundleURL);
OF_EXTERN OCFURLRef _CFBundleCopyResourcesDirectoryURLInDirectory(OCFURLRef bundleURL, uint8_t version);
OF_EXTERN OCFStringRef
_CFBundleCopyExecutableName(OCFBundleRef bundle, OCFURLRef url, OCFDictionaryRef infoDict);
OF_EXTERN void _CFBundleUnloadExecutable(OCFBundleRef bundle, Boolean keepScheduledPlugIns);
OF_EXTERN _CFPlugInData *__CFBundleGetPlugInData(OCFBundleRef bundle);
OF_EXTERN Boolean _CFBundleCouldBeBundle(OCFURLRef url);
OF_EXTERN OCFErrorRef _CFBundleCreateErrorDebug(
    OCFAllocatorRef allocator, OCFBundleRef bundle, OCFIndex code, OCFStringRef debugString
);
OF_EXTERN uint8_t _CFBundleLayoutVersion(OCFBundleRef bundle);

// Interfaces of CFBundle_InfoPlist.c and CFBundle_Strings.m that the CFBundle.c unit calls (restored in
// PlugIn/OCFBundleInfoPlist.c and PlugIn/OCFBundleStrings.c).
OF_EXPORT OCFDictionaryRef OCFBundleGetInfoDictionary(OCFBundleRef bundle);
OF_EXPORT OCFDictionaryRef OCFBundleGetLocalInfoDictionary(OCFBundleRef bundle);
OF_EXPORT OCFTypeRef OCFBundleGetValueForInfoDictionaryKey(OCFBundleRef bundle, OCFStringRef key);
OF_EXPORT OCFStringRef OCFBundleGetIdentifier(OCFBundleRef bundle);
OF_EXPORT OCFStringRef OCFBundleCopyLocalizedStringForLocalization(
    OCFBundleRef bundle,
    OCFStringRef key,
    OCFStringRef value,
    OCFStringRef tableName,
    OCFStringRef localizationName
);
// _CFBundleLoadLimitedInfoDictionary answers the dictionary it stores at +24 (X0 at the return).
OF_EXTERN OCFDictionaryRef _CFBundleLoadLimitedInfoDictionary(OCFBundleRef bundle, OCFSetRef keyPaths);
OF_EXTERN OCFDataRef _CFBundleCreateMappedBPlistFile(OCFURLRef url, Boolean flag);

// Interfaces of families not yet restored on this branch (SDK or native signatures; their owners' headers
// replace these declarations).
OF_EXTERN bool _os_feature_enabled_impl(const char *domain, const char *feature);
