//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/UUID/OCFUUID.h"

// The plug-in unit (CFPlugIn and its factories, iOS Simulator 0x1c01d0760-0x1c01d33b8). Hidden (were private
// externals; original names are preserved).
OF_EXTERN void _CFPlugInUnscheduleForUnloading(OCFPlugInRef plugIn);
OF_EXTERN void _CFPlugInUnloadScheduledPlugIns(void);
// Answers true when the bundle is not a plug-in to register (no CFPlugInFactories dictionary, the dummy
// factory UUID, already registered) or was registered now; false, storing the bundle that already registered
// one of its factories in *existingPlugIn (when not NULL), when such a bundle exists.
OF_EXTERN Boolean
_CFBundleInitPlugIn(OCFBundleRef bundle, OCFDictionaryRef infoDict, OCFBundleRef *existingPlugIn);
OF_EXTERN void _CFPlugInHandleDynamicRegistration(OCFPlugInRef plugIn);
OF_EXTERN void _CFBundleDeallocatePlugIn(OCFBundleRef bundle);
OF_EXTERN void _CFPlugInWillUnload(OCFPlugInRef plugIn);

// Native exported key (__DATA_CONST,__const 0x2227a3990).
OF_EXPORT const OCFStringRef kOCFPlugInFactoriesKey;

// The factory class record (type 32), iOS Simulator 24A434, 0x2227a39a0. Original private-external name is
// preserved (hidden).
OF_EXTERN const OCFRuntimeClass __CFPFactoryClass;

// The factory instance: _CFPFactoryCommonCreateLocked asks _CFRuntimeCreateInstance for 48 bytes after the
// 16-byte header. Offsets are those of the native loads and stores; the struct tag and field names are local
// descriptive spellings.
struct __CFPFactory {
    OCFRuntimeBase _base;       // +0
    OCFUUIDRef _uuid;           // +16: retained; the key in the factory table.
    uint8_t _enabled;           // +24
    uint8_t _unread25[7];       // +25
    void *_function;            // +32: the factory function pointer.
    OCFPlugInRef _plugIn;       // +40: retained.
    OCFStringRef _functionName; // +48
    OCFMutableArrayRef _types;  // +56: the type UUIDs (retained by kCFTypeArrayCallBacks).
};

_Static_assert(sizeof(struct __CFPFactory) == 64, "A CFPFactory instance is 64 bytes.");

// The plug-in instance class record (type 33, the builtin class table's entry at 0x2227a40c8 + 33 * 8),
// iOS Simulator 24A434, 0x2227a3a00. Original private-external name is preserved (hidden).
OF_EXTERN const OCFRuntimeClass __CFPlugInInstanceClass;
