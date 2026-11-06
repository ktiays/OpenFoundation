//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFPlugIn.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <dispatch/dispatch.h>

// Inferred organization: the factory functions (_CFPFactoryCommonCreateLocked 0x1c01d0760 through
// _CFPFactoryRemoveTypeLocked 0x1c01d112c), the plug-in functions (_CFPlugInUnscheduleForUnloading
// 0x1c01d1790 through _CFPFactoryDeallocate 0x1c01d25b0) and ____CFBundlePluginLogger_block_invoke
// (0x1c01d32a8) lie in one cluster of the iOS Simulator __text section, with the factory class record,
// kCFPlugIn*Key and the plug-in tables beside them in data; the logged source path ends in
// PlugIn.subproj/CFPlugIn.c. Only the functions of the bundle-1 task are restored here; the other functions
// of the cluster (the CFPlugIn API and the instance functions other than the class callbacks) are not; the
// second CFBundle sub-track restored the dictionary appliers and the instance class record. The .cold
// fragments (0x1c030b2bc-0x1c030b7ac) and the shared OUTLINED_FUNCTION_0_21, _1_16, _2_14, _3_8, _4_6, _5_6,
// _6_5, _7_4, _8_3, _9_2, _10_1, _11_1 and _13_1 (0x1c01d32d4-0x1c01d33b4: the logger's dispatch_once, os_log
// argument packing, __CFBundleGetPlugInData of X19) are folded into their callers. iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/CoreFoundationBundle.md (pending; the coordinator writes the
// reading).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_3_8 (0x1c01d331c).

// libsystem_platform's recursive unfair lock (os/lock_private.h), as the native code imports it; every
// native call passes 0x10000 (OS_UNFAIR_LOCK_DATA_SYNCHRONIZATION). The type and its zero initializer are
// those of OS_UNFAIR_RECURSIVE_LOCK_INIT; the field names are local descriptive spellings.
typedef struct {
    os_unfair_lock lock;
    uint32_t count;
} RecursiveUnfairLock;

extern void os_unfair_recursive_lock_lock_with_options(RecursiveUnfairLock *lock, uint32_t options);

extern void os_unfair_recursive_lock_unlock(RecursiveUnfairLock *lock);

// libdyld's dyld_program_sdk_at_least (dyld_priv.h). _CFBundleInitPlugIn passes 0x07E40901FFFFFFFF: every
// platform (0xFFFFFFFF), version 0x07E40901.
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);

static const OCFBuildVersion kExistingFactoryCheckVersion = {0xFFFFFFFF, 0x07E40901};

// Native CFPlugInGlobalDataLock, 0x22e6d2e70 (__DATA,__bss). Non-external.
static RecursiveUnfairLock CFPlugInGlobalDataLock;

// Native _plugInsToUnload, _factoriesByTypeID and _factoriesByFactoryID, 0x22e6d2e78, 0x22e6d2e88 and
// 0x22e6d2ea0 (__DATA,__bss). Non-external. The factory table maps factory UUIDs (retained) to factories (not
// retained); the type table maps type UUIDs to arrays of factories that retain nothing.
static OCFMutableSetRef _plugInsToUnload;
static OCFMutableDictionaryRef _factoriesByTypeID;
static OCFMutableDictionaryRef _factoriesByFactoryID;

// Native _CFBundlePluginLogger._log and .onceToken, 0x22e6d2e90 and 0x22e6d2e98 (__DATA,__bss), and
// ____CFBundlePluginLogger_block_invoke, 0x1c01d32a8 (the block of ___block_literal_global_13; its
// dispatch_once is OUTLINED_FUNCTION_0_21 and _3_8, reached through the .cold.1 fragments). Non-external.
// _CFBundlePluginLogger itself has no native body: every caller inlines the once check and the load of the
// log, which this local function spells once.
static os_log_t _CFBundlePluginLogger_log;
static dispatch_once_t _CFBundlePluginLogger_onceToken;

static os_log_t pluginLogger(void) {
    dispatch_once(&_CFBundlePluginLogger_onceToken, ^{
      _CFBundlePluginLogger_log = os_log_create("com.apple.CFBundle", "plugin");
    });
    return _CFBundlePluginLogger_log;
}

// Native kCFPlugInFactoriesKey, 0x2227a3990 (__DATA_CONST,__const), pointing at the __cfstring record
// 0x2227b0200 ("CFPlugInFactories"). External. _CFBundleInitPlugIn loads the record itself.
static const OCFConstantString kPlugInFactoriesKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "CFPlugInFactories", 17
};

const OCFStringRef kOCFPlugInFactoriesKey = (OCFStringRef)&kPlugInFactoriesKeyString;

static void _CFPFactoryDeallocate(OCFTypeRef cf);
static void _searchForDummyUUID(const void *key, const void *value, void *context);
static void _searchForExistingFactoryLocked(const void *key, const void *value, void *context);
static void _registerFactoryLocked(const void *key, const void *value, void *context);
static void _registerTypeLocked(const void *key, const void *value, void *context);
static void __CFPlugInInstanceDeallocate(OCFTypeRef cf);
static OCFStringRef __CFPlugInInstanceCopyDescription(OCFTypeRef cf);

// Native __CFPFactoryClass, 0x2227a39a0 (__DATA_CONST,__const). Hidden (was private external). Version 0,
// "_CFPFactory", finalize _CFPFactoryDeallocate; every other callback NULL.
const OCFRuntimeClass __CFPFactoryClass = {
    0,
    "_CFPFactory",
    NULL,
    NULL,
    _CFPFactoryDeallocate,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
};

// Native __CFPlugInInstanceClass, 0x2227a3a00 (__DATA_CONST,__const), entry 33 of
// __CFRuntimeBuiltinClassTable (0x2227a40c8). Hidden (was private external). Version 0, "CFPlugInInstance"
// (0x1c0485d70), finalize
// __CFPlugInInstanceDeallocate (slot 4) and copyDebugDesc __CFPlugInInstanceCopyDescription (slot 8); every
// other slot is NULL.
const OCFRuntimeClass __CFPlugInInstanceClass = {
    0,
    "CFPlugInInstance",
    NULL,
    NULL,
    __CFPlugInInstanceDeallocate,
    NULL,
    NULL,
    NULL,
    __CFPlugInInstanceCopyDescription,
    NULL,
    NULL,
    0,
};

// The plug-in instance, as __CFPlugInInstanceDeallocate reads it. Offsets are those of the native loads; the
// struct tag and field names are local descriptive spellings. The instance data begins at +40.
struct __CFPlugInInstance {
    OCFRuntimeBase _base;                    // +0
    struct __CFPFactory *_factory;           // +16
    OCFStringRef _unread24;                  // +24
    void (*_deallocateInstanceData)(void *); // +32: called with the address of the instance data.
    uint8_t _instanceData[];                 // +40
};

// Native _CFPFactoryCommonCreateLocked, 0x1c01d0760, with its .cold.1 (0x1c030b2bc) and .cold.2 (0x1c030b2d0)
// fragments. Non-external.
// A type-32 instance retains the factory UUID, is enabled, gets a mutable type array with
// kCFTypeArrayCallBacks in the given allocator and is entered in the factory table (created on first use with
// kCFTypeDictionaryKeyCallBacks and all-zero value callbacks).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_1_16 (0x1c01d32e8), _OUTLINED_FUNCTION_2_14
// (0x1c01d3308), _OUTLINED_FUNCTION_10_1 (0x1c01d3384).
static struct __CFPFactory *_CFPFactoryCommonCreateLocked(OCFAllocatorRef allocator, OCFUUIDRef factoryID) {
    struct __CFPFactory *factory = (struct __CFPFactory *)_OCFRuntimeCreateInstance(allocator, 32, 48, NULL);
    if (factory == NULL) {
        return NULL;
    }
    factory->_uuid = OCFRetain(factoryID);
    factory->_enabled = 1;
    factory->_types = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
    OCFUUIDRef uuid = factory->_uuid;
    if (_factoriesByFactoryID == NULL) {
        OCFDictionaryValueCallBacks valueCallBacks = {0, NULL, NULL, NULL, NULL};
        _factoriesByFactoryID = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &valueCallBacks
        );
    }
    OCFDictionarySetValue(_factoriesByFactoryID, uuid, factory);
    os_log_debug(pluginLogger(), "Registered factory %{public}@ (%{public}@)", factory, uuid);
    return factory;
}

// Native _CFPlugInRegisterFactoryFunctionByNameLocked, 0x1c01d09dc. Non-external.
// The factory gets no function pointer, retains the plug-in (CFRetain is reached before the NULL test) and
// joins the plug-in's factory array when the bundle is a plug-in; the function name is copied when present.
static void _CFPlugInRegisterFactoryFunctionByNameLocked(
    OCFUUIDRef factoryID, OCFPlugInRef plugIn, OCFStringRef functionName
) {
    struct __CFPFactory *factory = _CFPFactoryCommonCreateLocked(kOCFAllocatorSystemDefault, factoryID);
    factory->_function = NULL;
    factory->_plugIn = (OCFPlugInRef)OCFRetain(plugIn);
    if (plugIn != NULL && __CFBundleGetPlugInData(plugIn)->_isPlugIn) {
        OCFArrayAppendValue(__CFBundleGetPlugInData(plugIn)->_factories, factory);
    }
    factory->_functionName =
        functionName != NULL ? OCFStringCreateCopy(kOCFAllocatorSystemDefault, functionName) : NULL;
}

// Native _CFPFactoryDisableLocked, 0x1c01d0c0c, with its .cold.1 (0x1c030b32c) and .cold.2 (0x1c030b330)
// fragments. Non-external. Clears the enabled flag (before the logger's once check), logs and releases the
// factory.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_7_4 (0x1c01d3360), _OUTLINED_FUNCTION_8_3
// (0x1c01d3370), _OUTLINED_FUNCTION_9_2 (0x1c01d337c), _OUTLINED_FUNCTION_13_1 (0x1c01d33a8).
static void _CFPFactoryDisableLocked(struct __CFPFactory *factory) {
    factory->_enabled = 0;
    os_log_debug(pluginLogger(), "Factory %{public}@ has been disabled", factory->_uuid);
    OCFRelease(factory);
}

// Native _CFPlugInRegisterPlugInTypeLocked, 0x1c01d0dcc, with its .cold.1 through .cold.4 fragments
// (0x1c030b360-0x1c030b3bc). Non-external.
// An enabled factory in the factory table records the type and joins the type's array (an array with
// all-zero callbacks, created in a type table made on first use with kCFType dictionary callbacks); an
// unknown or disabled factory logs an error.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_4_6 (0x1c01d3330), _OUTLINED_FUNCTION_6_5
// (0x1c01d3350).
// Folds the compiler-outlined pieces __CFPlugInRegisterPlugInTypeLocked.cold.2 (0x1c030b374),
// __CFPlugInRegisterPlugInTypeLocked.cold.3 (0x1c030b3b8).
static void _CFPlugInRegisterPlugInTypeLocked(OCFUUIDRef factoryID, OCFUUIDRef typeID) {
    struct __CFPFactory *factory = NULL;
    if (_factoriesByFactoryID != NULL) {
        factory = (struct __CFPFactory *)OCFDictionaryGetValue(_factoriesByFactoryID, factoryID);
    }
    if (factory == NULL || !factory->_enabled) {
        os_log_error(
            pluginLogger(), "RegisterPlugInType: No factory registered for id %{public}@", factoryID
        );
        return;
    }
    OCFArrayAppendValue(factory->_types, typeID);
    if (_factoriesByTypeID == NULL) {
        _factoriesByTypeID = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    OCFMutableArrayRef factories = (OCFMutableArrayRef)OCFDictionaryGetValue(_factoriesByTypeID, typeID);
    if (factories == NULL) {
        OCFArrayCallBacks callBacks = {0, NULL, NULL, NULL, NULL};
        factories = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &callBacks);
        OCFDictionarySetValue(_factoriesByTypeID, typeID, factories);
        OCFRelease(factories);
    }
    OCFArrayAppendValue(factories, factory);
    os_log_debug(pluginLogger(), "Type %{public}@ added to factory %{public}@", typeID, factory->_uuid);
}

// Native _CFPFactoryRemoveTypeLocked, 0x1c01d112c, with its .cold.1 (0x1c030b418) and .cold.2 (0x1c030b41c)
// fragments. Non-external.
// Indices are tested as 32-bit values (TBNZ W0, #31) and masked with 0x7FFFFFFF; a type array left empty is
// removed from the type table.
static void _CFPFactoryRemoveTypeLocked(struct __CFPFactory *factory, OCFUUIDRef typeID) {
    OCFMutableArrayRef types = factory->_types;
    int32_t index =
        (int32_t)OCFArrayGetFirstIndexOfValue(types, (OCFRange){0, OCFArrayGetCount(types)}, typeID);
    if (index >= 0) {
        OCFArrayRemoveValueAtIndex(factory->_types, index & 0x7FFFFFFF);
    }
    if (_factoriesByTypeID != NULL) {
        OCFMutableArrayRef factories = (OCFMutableArrayRef)OCFDictionaryGetValue(_factoriesByTypeID, typeID);
        if (factories != NULL) {
            int32_t factoryIndex = (int32_t)OCFArrayGetFirstIndexOfValue(
                factories, (OCFRange){0, OCFArrayGetCount(factories)}, factory
            );
            if (factoryIndex >= 0) {
                OCFArrayRemoveValueAtIndex(factories, factoryIndex & 0x7FFFFFFF);
                if (OCFArrayGetCount(factories) == 0) {
                    OCFDictionaryRemoveValue(_factoriesByTypeID, typeID);
                }
            }
        }
    }
    os_log_debug(pluginLogger(), "Type %{public}@ removed from factory %{public}@", typeID, factory->_uuid);
}

// Native _CFPFactoryRemoveInstanceLocked, 0x1c01d1648, with its .cold.1 (0x1c030b520, the logger's
// dispatch_once through OUTLINED_FUNCTION_3_8) and .cold.2 (0x1c030b534, the debug log through
// OUTLINED_FUNCTION_4_6, _7_4 and _9_2) fragments. Non-external. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json. Without a plug-in (+40), or for a bundle that
// is not a plug-in, nothing happens. Otherwise the plug-in data's word at +8 is decremented; when it reaches
// 0 and the byte at +1 is set, under the global lock, the plug-in joins the unload set (created on first use
// with kCFTypeSetCallBacks minus retain and release, kCFAllocatorSystemDefault) and is logged. The plug-in is
// then released (the instance's reference). __CFBundleGetPlugInData is called again for each field access.
static void _CFPFactoryRemoveInstanceLocked(struct __CFPFactory *factory) {
    OCFPlugInRef plugIn = factory->_plugIn;
    if (plugIn == NULL || !__CFBundleGetPlugInData(plugIn)->_isPlugIn) {
        return;
    }
    __CFBundleGetPlugInData(plugIn)->_word8 -= 1;
    if (__CFBundleGetPlugInData(plugIn)->_word8 == 0 && __CFBundleGetPlugInData(plugIn)->_flag1 != 0) {
        os_unfair_recursive_lock_lock_with_options(&CFPlugInGlobalDataLock, 0x10000);
        if (_plugInsToUnload == NULL) {
            OCFSetCallBacks callBacks = kOCFTypeSetCallBacks;
            callBacks.retain = NULL;
            callBacks.release = NULL;
            _plugInsToUnload = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &callBacks);
        }
        OCFSetAddValue(_plugInsToUnload, plugIn);
        os_log_debug(pluginLogger(), "PlugIn %{public}@ is now scheduled for unloading", plugIn);
        os_unfair_recursive_lock_unlock(&CFPlugInGlobalDataLock);
    }
    OCFRelease(plugIn);
}

// Native _CFPlugInUnscheduleForUnloading, 0x1c01d1790, with its .cold.1 (0x1c030b560) and .cold.2
// (0x1c030b564) fragments. Hidden (was private external).
void _CFPlugInUnscheduleForUnloading(OCFPlugInRef plugIn) {
    os_unfair_recursive_lock_lock_with_options(&CFPlugInGlobalDataLock, 0x10000);
    if (_plugInsToUnload != NULL) {
        OCFSetRemoveValue(_plugInsToUnload, plugIn);
    }
    os_log_debug(pluginLogger(), "PlugIn %{public}@ is now unscheduled for unloading", plugIn);
    os_unfair_recursive_lock_unlock(&CFPlugInGlobalDataLock);
}

// Native _CFPlugInUnloadScheduledPlugIns, 0x1c01d181c, with its .cold.1 fragment (0x1c030b590). Hidden (was
// private external).
// The scheduled plug-ins are copied into a system-default block of malloc type 0x2004093837F09 (count * 8
// bytes); each one without instances (+12 of the plug-in data) is logged and unloaded with its scheduled
// plug-ins kept. The set itself is not changed here.
void _CFPlugInUnloadScheduledPlugIns(void) {
    os_unfair_recursive_lock_lock_with_options(&CFPlugInGlobalDataLock, 0x10000);
    if (_plugInsToUnload != NULL) {
        OCFIndex count = OCFSetGetCount(_plugInsToUnload);
        if (count >= 1) {
            const void **plugIns =
                OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, count * 8, 0x2004093837F09, 0);
            OCFSetGetValues(_plugInsToUnload, plugIns);
            for (OCFIndex index = 0; index != count; index++) {
                OCFBundleRef plugIn = (OCFBundleRef)plugIns[index];
                if (__CFBundleGetPlugInData(plugIn)->_instanceCount == 0) {
                    os_log_debug(pluginLogger(), "PlugIn %{public}@ is about to be unloaded", plugIn);
                    _CFBundleUnloadExecutable(plugIn, true);
                }
            }
            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, plugIns);
        }
    }
    os_unfair_recursive_lock_unlock(&CFPlugInGlobalDataLock);
}

// Native _CFBundleInitPlugIn, 0x1c01d1988, with its .cold.1 (0x1c030b5a4), .cold.2 (0x1c030b5b8, the
// dummy-UUID log) and .cold.3 (0x1c030b5e4, the registration) fragments. Hidden (was private external). Only
// a CFPlugInFactories dictionary makes a plug-in. A factory with the dummy UUID logs and answers true. Under
// the global lock, a bundle already registered (+4 of the plug-in data) answers true; for programs built with
// the 0x07E40901 SDK or later, a factory UUID that another bundle registered answers false and stores that
// bundle. Otherwise .cold.3 marks the plug-in data (+0 = 1, +1 = 1, +2 = 0, +3 = 0, the 32-bit words at +8
// and +12 = 0, +4 = 1, in this order), makes the factory array in the bundle's allocator with all-zero
// callbacks, registers the factories and, when CFPlugInTypes is a dictionary, the types, unlocks and answers
// true.
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_11_1 (0x1c01d338c).
Boolean _CFBundleInitPlugIn(OCFBundleRef bundle, OCFDictionaryRef infoDict, OCFBundleRef *existingPlugIn) {
    OCFArrayCallBacks callBacks = {0, NULL, NULL, NULL, NULL};
    if (infoDict == NULL) {
        return true;
    }
    OCFDictionaryRef factories = OCFDictionaryGetValue(infoDict, (OCFStringRef)&kPlugInFactoriesKeyString);
    if (factories == NULL || OCFGetTypeID(factories) != OCFDictionaryGetTypeID()) {
        return true;
    }
    Boolean hasDummyUUID = false;
    OCFDictionaryApplyFunction(factories, _searchForDummyUUID, &hasDummyUUID);
    if (hasDummyUUID) {
        os_log_debug(
            pluginLogger(),
            "Bundle %{public}@ contains a factory UUID of 00000000-0000-0000-0000-000000000000 with function "
            "'MyFactoryFunction'. This bundle is not a valid plugin.",
            bundle
        );
        return true;
    }
    os_unfair_recursive_lock_lock_with_options(&CFPlugInGlobalDataLock, 0x10000);
    if (__CFBundleGetPlugInData(bundle)->_registeredFactories) {
        os_unfair_recursive_lock_unlock(&CFPlugInGlobalDataLock);
        return true;
    }
    if (dyld_program_sdk_at_least(kExistingFactoryCheckVersion)) {
        OCFBundleRef existing = NULL;
        OCFDictionaryApplyFunction(factories, _searchForExistingFactoryLocked, &existing);
        if (existing != NULL) {
            if (existingPlugIn != NULL) {
                *existingPlugIn = existing;
            }
            os_unfair_recursive_lock_unlock(&CFPlugInGlobalDataLock);
            return false;
        }
    }
    __CFBundleGetPlugInData(bundle)->_isPlugIn = 1;
    __CFBundleGetPlugInData(bundle)->_flag1 = 1;
    __CFBundleGetPlugInData(bundle)->_flag2 = 0;
    __CFBundleGetPlugInData(bundle)->_flag3 = 0;
    __CFBundleGetPlugInData(bundle)->_word8 = 0;
    __CFBundleGetPlugInData(bundle)->_instanceCount = 0;
    __CFBundleGetPlugInData(bundle)->_registeredFactories = 1;
    OCFMutableArrayRef factoryArray = OCFArrayCreateMutable(OCFGetAllocator(bundle), 0, &callBacks);
    __CFBundleGetPlugInData(bundle)->_factories = factoryArray;
    OCFDictionaryApplyFunction(factories, _registerFactoryLocked, bundle);
    OCFDictionaryRef types = OCFDictionaryGetValue(infoDict, OF_STR("CFPlugInTypes"));
    if (types != NULL && OCFGetTypeID(types) == OCFDictionaryGetTypeID()) {
        OCFDictionaryApplyFunction(types, _registerTypeLocked, bundle);
    }
    os_unfair_recursive_lock_unlock(&CFPlugInGlobalDataLock);
    return true;
}

// Native _searchForDummyUUID, 0x1c01d1b20. Non-external; the CFDictionaryApplyFunction applier of
// _CFBundleInitPlugIn over the CFPlugInFactories dictionary. Nothing happens once the Boolean the context
// addresses is set. A string key that compares equal (options 0) to the null UUID, with a string value that
// compares equal to "MyFactoryFunction", sets it. The key comparison runs before the value is tested for
// NULL.
static void _searchForDummyUUID(const void *key, const void *value, void *context) {
    Boolean *found = (Boolean *)context;
    if (*found) {
        return;
    }
    if (OCFGetTypeID(key) != OCFStringGetTypeID()) {
        return;
    }
    OCFComparisonResult result =
        OCFStringCompare((OCFStringRef)key, OF_STR("00000000-0000-0000-0000-000000000000"), 0);
    if (value == NULL || result != kOCFCompareEqualTo) {
        return;
    }
    if (OCFGetTypeID(value) != OCFStringGetTypeID()) {
        return;
    }
    if (OCFStringCompare((OCFStringRef)value, OF_STR("MyFactoryFunction"), 0) == kOCFCompareEqualTo) {
        *found = true;
    }
}

// Native _searchForExistingFactoryLocked, 0x1c01d1bc0. Non-external; an applier of _CFBundleInitPlugIn. Once
// the bundle pointer the context addresses is set nothing happens. A string key is converted with
// CFUUIDCreateFromString (system default allocator); any other key, or a string that does not convert, is
// retained instead. A factory registered under that UUID stores its plug-in (+40), retained. The UUID is
// released when not NULL.
static void _searchForExistingFactoryLocked(const void *key, const void *value, void *context) {
    (void)value;
    OCFBundleRef *existing = (OCFBundleRef *)context;
    if (*existing != NULL) {
        return;
    }
    OCFTypeRef factoryID = NULL;
    if (OCFGetTypeID(key) == OCFStringGetTypeID()) {
        factoryID = OCFUUIDCreateFromString(kOCFAllocatorSystemDefault, (OCFStringRef)key);
    }
    if (factoryID == NULL) {
        factoryID = OCFRetain(key);
    }
    if (_factoriesByFactoryID != NULL) {
        struct __CFPFactory *factory =
            (struct __CFPFactory *)OCFDictionaryGetValue(_factoriesByFactoryID, factoryID);
        if (factory != NULL) {
            *existing = (OCFBundleRef)OCFRetain(factory->_plugIn);
        }
    }
    if (factoryID != NULL) {
        OCFRelease(factoryID);
    }
}

// Native _registerFactoryLocked, 0x1c01d1c68, with its .cold.1 (0x1c030b6d8, the logger's dispatch_once
// through _OUTLINED_FUNCTION_0_21). Non-external; an applier of _CFBundleInitPlugIn with the bundle as the
// context. The factory UUID is made as in _searchForExistingFactoryLocked; a value that is not a string of
// positive length becomes NULL. The debug log shows "<no func>" for a NULL name.
static void _registerFactoryLocked(const void *key, const void *value, void *context) {
    OCFTypeRef factoryID = NULL;
    if (OCFGetTypeID(key) == OCFStringGetTypeID()) {
        factoryID = OCFUUIDCreateFromString(kOCFAllocatorSystemDefault, (OCFStringRef)key);
    }
    if (factoryID == NULL) {
        factoryID = OCFRetain(key);
    }
    OCFStringRef functionName = (OCFStringRef)value;
    if (OCFGetTypeID(value) != OCFStringGetTypeID() || OCFStringGetLength(functionName) <= 0) {
        functionName = NULL;
    }
    os_log_debug(
        pluginLogger(),
        "Registering static factory %{public}@ %{public}@ bundle %{public}p",
        factoryID,
        functionName == NULL ? OF_STR("<no func>") : functionName,
        context
    );
    _CFPlugInRegisterFactoryFunctionByNameLocked((OCFUUIDRef)factoryID, (OCFPlugInRef)context, functionName);
    if (factoryID != NULL) {
        OCFRelease(factoryID);
    }
}

// Native _registerTypeLocked, 0x1c01d1db0, with its .cold.1 and .cold.2 (0x1c030b6dc, 0x1c030b6f0: the
// logger's dispatch_once) and .cold.3 (0x1c030b6f4, the debug log of the single-factory path). Non-external;
// an applier of _CFBundleInitPlugIn over CFPlugInTypes with the bundle as the context. The key is the type
// UUID string (converted with the system default allocator, otherwise retained); the value is an array of
// factory UUID strings or one factory UUID. Array elements and a lone value are converted with the bundle's
// allocator, otherwise retained, and registered with _CFPlugInRegisterPlugInTypeLocked(factory, type). The
// array count is kept in a 32-bit register: zero takes the single-factory path unless the value is an (empty)
// array, and a negative count registers nothing.
static void _registerTypeLocked(const void *key, const void *value, void *context) {
    int32_t count = 0;
    if (OCFGetTypeID(value) == OCFArrayGetTypeID()) {
        count = (int32_t)OCFArrayGetCount((OCFArrayRef)value);
    }
    OCFTypeRef typeID = NULL;
    if (OCFGetTypeID(key) == OCFStringGetTypeID()) {
        typeID = OCFUUIDCreateFromString(kOCFAllocatorSystemDefault, (OCFStringRef)key);
    }
    if (typeID == NULL) {
        typeID = OCFRetain(key);
    }
    if (count != 0) {
        if (count >= 1) {
            for (OCFIndex index = 0; index != (OCFIndex)(uint32_t)count; index++) {
                OCFTypeRef factoryString = OCFArrayGetValueAtIndex((OCFArrayRef)value, index);
                OCFTypeRef factoryID = NULL;
                if (OCFGetTypeID(factoryString) == OCFStringGetTypeID()) {
                    factoryID =
                        OCFUUIDCreateFromString(OCFGetAllocator(context), (OCFStringRef)factoryString);
                }
                if (factoryID == NULL) {
                    factoryID = OCFRetain(factoryString);
                }
                os_log_debug(
                    pluginLogger(), "Registering factory %{public}@ type %{public}@", factoryID, typeID
                );
                _CFPlugInRegisterPlugInTypeLocked((OCFUUIDRef)factoryID, (OCFUUIDRef)typeID);
                if (factoryID != NULL) {
                    OCFRelease(factoryID);
                }
            }
        }
    } else if (OCFGetTypeID(value) != OCFArrayGetTypeID()) {
        OCFTypeRef factoryID = NULL;
        if (OCFGetTypeID(value) == OCFStringGetTypeID()) {
            factoryID = OCFUUIDCreateFromString(OCFGetAllocator(context), (OCFStringRef)value);
        }
        if (factoryID == NULL) {
            factoryID = OCFRetain(value);
        }
        os_log_debug(pluginLogger(), "Registering factory %{public}@ type %{public}@", factoryID, typeID);
        _CFPlugInRegisterPlugInTypeLocked((OCFUUIDRef)factoryID, (OCFUUIDRef)typeID);
        if (factoryID != NULL) {
            OCFRelease(factoryID);
        }
    }
    if (typeID != NULL) {
        OCFRelease(typeID);
    }
}

// Native _CFPlugInHandleDynamicRegistration, 0x1c01d202c. Hidden (was private external). A lone RET.
void _CFPlugInHandleDynamicRegistration(OCFPlugInRef plugIn) {
    (void)plugIn;
}

// Native _CFBundleDeallocatePlugIn, 0x1c01d2030, with its .cold.1 (0x1c030b720) and .cold.2 (0x1c030b724)
// fragments. Hidden (was private external).
// For a plug-in, the factories are disabled from the last to the first (the count is tested as a 32-bit
// value), the array is released and the plug-in flag cleared, under the global lock.
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_5_6 (0x1c01d3344).
void _CFBundleDeallocatePlugIn(OCFBundleRef bundle) {
    _CFPlugInData *plugInData = __CFBundleGetPlugInData(bundle);
    os_unfair_recursive_lock_lock_with_options(&CFPlugInGlobalDataLock, 0x10000);
    if (plugInData->_isPlugIn) {
        os_log_debug(
            pluginLogger(),
            "Disabling factories in array %{public}p for bundle %{public}p",
            __CFBundleGetPlugInData(bundle)->_factories,
            bundle
        );
        int32_t count = (int32_t)OCFArrayGetCount(plugInData->_factories);
        for (OCFIndex index = (OCFIndex)count - 1; index >= 0; index--) {
            _CFPFactoryDisableLocked(
                (struct __CFPFactory *)OCFArrayGetValueAtIndex(plugInData->_factories, index)
            );
        }
        OCFRelease(plugInData->_factories);
        plugInData->_isPlugIn = 0;
    }
    os_unfair_recursive_lock_unlock(&CFPlugInGlobalDataLock);
}

// Native _CFPlugInWillUnload, 0x1c01d2510. Hidden (was private external).
// For a plug-in, every factory's function pointer is cleared (last to first), under the global lock.
void _CFPlugInWillUnload(OCFPlugInRef plugIn) {
    if (!__CFBundleGetPlugInData(plugIn)->_isPlugIn) {
        return;
    }
    os_unfair_recursive_lock_lock_with_options(&CFPlugInGlobalDataLock, 0x10000);
    int32_t count = (int32_t)OCFArrayGetCount(__CFBundleGetPlugInData(plugIn)->_factories);
    for (OCFIndex index = (OCFIndex)count - 1; index >= 0; index--) {
        struct __CFPFactory *factory = (struct __CFPFactory *)OCFArrayGetValueAtIndex(
            __CFBundleGetPlugInData(plugIn)->_factories, index
        );
        factory->_function = NULL;
    }
    os_unfair_recursive_lock_unlock(&CFPlugInGlobalDataLock);
}

// Native _CFPFactoryDeallocate, 0x1c01d25b0, with its .cold.1 (0x1c030b780) and .cold.2 (0x1c030b784)
// fragments. Non-external; the finalize callback of __CFPFactoryClass.
// Under the global lock the factory leaves the factory table, its plug-in's factory array (the plug-in is
// released) and every type it registered (last to first); the type array is released. The function name and
// the UUID are released after the unlock.
// CoreFoundation defect: the native code removes the factory from _factoriesByFactoryID only when
// _factoriesByTypeID exists (CMP X20, #0; CCMP X8, #0 with X8 the type table, then a removal from the
// factory table). A factory freed before any type was ever registered stays in the factory table, which does
// not retain its values: a dangling pointer for every later lookup by that UUID. The restoration tests the
// factory table it removes from. Confidence: high (the loads are of __factoriesByTypeID and
// __factoriesByFactoryID respectively).
static void _CFPFactoryDeallocate(OCFTypeRef cf) {
    struct __CFPFactory *factory = (struct __CFPFactory *)cf;
    os_unfair_recursive_lock_lock_with_options(&CFPlugInGlobalDataLock, 0x10000);
    OCFUUIDRef uuid = factory->_uuid;
    if (uuid != NULL && _factoriesByFactoryID != NULL) {
        OCFDictionaryRemoveValue(_factoriesByFactoryID, uuid);
    }
    os_log_debug(pluginLogger(), "Unregistered factory %{public}@ (%{public}@)", factory, uuid);
    OCFPlugInRef plugIn = factory->_plugIn;
    if (plugIn != NULL) {
        if (__CFBundleGetPlugInData(plugIn)->_isPlugIn) {
            OCFMutableArrayRef plugInFactories = __CFBundleGetPlugInData(plugIn)->_factories;
            int32_t index = (int32_t)OCFArrayGetFirstIndexOfValue(
                plugInFactories,
                (OCFRange){0, OCFArrayGetCount(__CFBundleGetPlugInData(plugIn)->_factories)},
                factory
            );
            if (index >= 0) {
                OCFArrayRemoveValueAtIndex(__CFBundleGetPlugInData(plugIn)->_factories, index & 0x7FFFFFFF);
            }
        }
        OCFRelease(factory->_plugIn);
    }
    int32_t count = (int32_t)OCFArrayGetCount(factory->_types);
    for (OCFIndex index = (OCFIndex)count - 1; index >= 0; index--) {
        _CFPFactoryRemoveTypeLocked(factory, OCFArrayGetValueAtIndex(factory->_types, index));
    }
    OCFRelease(factory->_types);
    os_unfair_recursive_lock_unlock(&CFPlugInGlobalDataLock);
    if (factory->_functionName != NULL) {
        OCFRelease(factory->_functionName);
    }
    if (factory->_uuid != NULL) {
        OCFRelease(factory->_uuid);
    }
}

// Native __CFPlugInInstanceDeallocate, 0x1c01d2d70. Non-external; the finalize callback of
// __CFPlugInInstanceClass. Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta.json. With the
// CFSwiftBundle feature nothing happens. Otherwise, under the global lock, the instance-data finalizer (+32)
// receives the address of the instance data (+40) when set, and the factory (+16), when set, drops the
// instance through _CFPFactoryRemoveInstanceLocked.
static void __CFPlugInInstanceDeallocate(OCFTypeRef cf) {
    struct __CFPlugInInstance *instance = (struct __CFPlugInInstance *)cf;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        return;
    }
    os_unfair_recursive_lock_lock_with_options(&CFPlugInGlobalDataLock, 0x10000);
    if (instance->_deallocateInstanceData != NULL) {
        instance->_deallocateInstanceData(instance->_instanceData);
    }
    if (instance->_factory != NULL) {
        _CFPFactoryRemoveInstanceLocked(instance->_factory);
    }
    os_unfair_recursive_lock_unlock(&CFPlugInGlobalDataLock);
}

// Native __CFPlugInInstanceCopyDescription, 0x1c01d2de4. Non-external; the copyDebugDesc callback of
// __CFPlugInInstanceClass. Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta.json. Answers the
// constant string "Some CFPlugInInstance" without a retain (ADRL X0; RET).
static OCFStringRef __CFPlugInInstanceCopyDescription(OCFTypeRef cf) {
    (void)cf;
    return OF_STR("Some CFPlugInInstance");
}
