//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFBag.h"
#include "CoreFoundation/Collections/OCFBasicHash.h"
#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Collections/OCFRunArray.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/Collections/OCFStorage.h"
#include "CoreFoundation/Date/OCFDateInterval.h"
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Locale/OCFDateComponents.h"
#include "CoreFoundation/Locale/OCFListFormatter.h"
#include "CoreFoundation/Locale/OCFLocale.h"
#include "CoreFoundation/Locale/OCFDateFormatter.h"
#include "CoreFoundation/Locale/OCFNumberFormatter.h"
#include "CoreFoundation/Notification/OCFNotificationCenter.h"
#include "CoreFoundation/Notification/OCFXNotificationRegistrar.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/PlugIn/OCFPlugIn.h"
#include "CoreFoundation/PropertyList/OCFKeyedArchiverUID.h"
#include "CoreFoundation/RunLoop/OCFFileDescriptor.h"
#include "CoreFoundation/RunLoop/OCFMachPort.h"
#include "CoreFoundation/RunLoop/OCFRunLoop.h"
#include "CoreFoundation/Stream/OCFStream.h"
#include "CoreFoundation/String/OCFAttributedString.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringTokenizer.h"
#include "CoreFoundation/URL/OCFURL.h"
#include "CoreFoundation/UUID/OCFUUID.h"
#include <OpenCoreFoundation/Date/OCFDate.h>

#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Inferred organization: __CFOAInitialize (0x1c01e506c) through __CFIsDeallocating (0x1c01e80cc) are
// contiguous in the iOS Simulator __text section, between the run-loop and NSArray code, and are kept in one
// translation unit; the native source path is not in the evidence. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446:
// constants and calls compared for the entry points named in the reading (the Device build signs isa and info
// words and inlines the per-thread table), bodies unread. Reading:
// Documentation/Reference/Research/RuntimePrerequisites.md.

// libobjc interfaces the native image uses (re-exported libobjc.A.dylib); supplied, never reimplemented. The
// debug variables are read for each decode, as natively; their slot-to-symbol binding is by use (confidence:
// medium).
extern uintptr_t objc_debug_taggedpointer_obfuscator;
extern const uint8_t objc_debug_tag60_permutations[8];
extern const uintptr_t objc_debug_isa_class_mask;
extern void objc_setEnumerationMutationHandler(void (*handler)(id));
extern void instrumentObjcMessageSends(int enable);
extern void _objc_setBadAllocHandler(id (*handler)(Class));
extern id objc_retain(id value);
extern void objc_release(id value);
extern id objc_autorelease(id value);
extern Class objc_getFutureClass(const char *name);
extern Class object_getClass(id object);
extern Class object_setClass(id object, Class cls);
extern void *objc_destructInstance(id object);
extern BOOL objc_opt_respondsToSelector(id object, SEL selector);

// libdispatch's autorelease-pool hooks (imported as _dispatch_begin_NSAutoReleasePool and
// _dispatch_end_NSAutoReleasePool through libSystem); __CFInitialize stores CF's wrappers in them. The
// binding of the two out-of-image slots 0x2224170d0 and 0x222417128 is by the image's imports and their order
// of use; confidence: medium.
extern void *(*_dispatch_begin_NSAutoReleasePool)(void);
extern void (*_dispatch_end_NSAutoReleasePool)(void *context);

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call passes 0x10000
// (kUnfairLockDataSynchronization).
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);
static const uint32_t kUnfairLockDataSynchronization = 0x10000;

// libdyld and libsystem_trace interfaces without SDK declarations; supplied.
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);
extern void _os_log_set_nscf_formatter(void *formatter);
extern int pthread_is_threaded_np(void);

// The build version every tag-0 decision asks dyld about (MOV X0, #0x7E30901FFFFFFFF): any platform, version
// 0x07e30901.
static const OCFBuildVersion kTagZeroVersion = {0xffffffff, 0x07e30901};

// Native gCRAnnotations, 0x22e6d2390 (__DATA,__crash_info, 0x148 bytes, version 7). Hidden (was private
// external). The crash paths store their message at offset 8 and execute BRK #1.
__attribute__((section("__DATA,__crash_info"))) uint64_t gCRAnnotations[41] = {7};

// Native __CFRuntimeBuiltinClassTable, 0x2227a40c8 (__DATA_CONST,__const, 72 pointers). Hidden (was private
// external). Slots 0 and 1 hold the "Not A Type" and "CFType" records whose callbacks are __HALT; a slot
// whose class no consumer instantiates is empty (owner decision of 2026-10-30,
// Documentation/Reference/Scope.md#decisions). Every restored record sits at its native slot (read from the
// pinned table through IDA). Native slots left empty because their class is not restored: 23 CFBinaryHeap,
// 24 CFBitVector, 28 CFTree, 30 CFURLComponents, 35 CFMessagePort, 48 CFTimeZone,
// 49 CFCalendar, 53 CFPasteboard, 54 CFUserNotification, 55 CFLocale, 61 CFSocket,
// 64 CFFileSecurity, 67 CFRelativeDateTimeFormatter, 69 CFDateIntervalFormatter.
static void __HALT(void) {
    __builtin_trap();
}

// Native __CFNotATypeClass and __CFTypeClass, 0x2227a4008 and 0x2227a4068 (__DATA_CONST,__const).
// Non-external.
static const OCFRuntimeClass __CFNotATypeClass = {
    0,
    "Not A Type",
    (void *)__HALT,
    (void *)__HALT,
    (void *)__HALT,
    (void *)__HALT,
    (void *)__HALT,
    (void *)__HALT,
    (void *)__HALT,
    NULL,
    NULL,
    0,
};

static const OCFRuntimeClass __CFTypeClass = {
    0,
    "CFType",
    (void *)__HALT,
    (void *)__HALT,
    (void *)__HALT,
    (void *)__HALT,
    (void *)__HALT,
    (void *)__HALT,
    (void *)__HALT,
    NULL,
    NULL,
    0,
};

const OCFRuntimeClass *const __CFRuntimeBuiltinClassTable[kOCFRuntimeBuiltinClassCount] = {
    [0] = &__CFNotATypeClass,
    [1] = &__CFTypeClass,
    [2] = &__CFAllocatorClass,
    [3] = &__CFBasicHashClass,
    [4] = &__CFBagClass,
    [7] = &__CFStringClass,
    [16] = &__CFNullClass,
    [17] = &__CFSetClass,
    [18] = &__CFDictionaryClass,
    [19] = &__CFArrayClass,
    [20] = &__CFDataClass,
    [21] = &__CFBooleanClass,
    [22] = &__CFNumberClass,
    [25] = &__CFCharacterSetClass,
    [26] = &__CFStorageClass,
    [27] = &__CFErrorClass,
    [29] = &__CFURLClass,
    [31] = &__CFBundleClass,
    [32] = &__CFPFactoryClass,
    [33] = &__CFPlugInInstanceClass,
    [34] = &__CFUUIDClass,
    [36] = &__CFMachPortClass,
    [38] = &__CFReadStreamClass,
    [39] = &__CFWriteStreamClass,
    // CFStringTokenizerCreate (0x1c022ee70) instantiates type 40.
    [40] = &__CFStringTokenizerRuntimeClass,
    [41] = &__CFKeyedArchiverUIDClass,
    [42] = &__CFDateClass,
    [43] = &__CFRunLoopClass,
    [44] = &__CFRunLoopModeClass,
    [45] = &__CFRunLoopObserverClass,
    [46] = &__CFRunLoopSourceClass,
    [47] = &__CFRunLoopTimerClass,
    [52] = &__CFNotificationCenterClass,
    [56] = &__CFDateFormatterClass,
    [57] = &__CFNumberFormatterClass,
    [60] = &__CFFileDescriptorClass,
    [62] = &__CFAttributedStringClass,
    [63] = &__CFRunArrayClass,
    [66] = &__CFDateComponentsClass,
    [68] = &__CFListFormatterClass,
    [70] = &__CFXNotificationRegistrarClass,
    [71] = &__CFDateIntervalClass,
};

// Native __CFEnv, 0x22e6cf770 (__DATA,__data): eleven name/value pairs filled by __CFInitialize; the last
// name is NULL. Non-external; the name is a local descriptive spelling (the native image has no symbol for
// it).
static const char *__CFEnv[11][2] = {
    {"USER", NULL},
    {"TZFILE", NULL},
    {"TZ", NULL},
    {"NEXT_ROOT", NULL},
    {"DYLD_IMAGE_SUFFIX", NULL},
    {"CFBundleDisableStringsSharing", NULL},
    {"CF_CHARSET_PATH", NULL},
    {"__CF_USER_TEXT_ENCODING", NULL},
    {"APPLE_FRAMEWORKS_ROOT", NULL},
    {"IPHONE_SIMULATOR_ROOT", NULL},
    {NULL, NULL},
};

// Native runtime state, iOS Simulator 24A434: __CFRuntimeBuiltinObjCClassTable 0x22e6d3fe0,
// __CFRuntimeClassTables 0x22e6d3ed8 and the dynamic count 0x22e6d3fd8 (__DATA,__common, hidden),
// __CFBigRuntimeFunnel 0x22e6d2f48
// (__DATA,__bss, non-external), __CFOASafe 0x22e6d3ec0 and __CFObjectAllocSetLastAllocEventNameFunction
// 0x22e6d3ed0 (external), __CFObjectAllocRecordAllocationFunction, __CFZombieEnabled 0x22e6d4220,
// __CFDeallocateZombies 0x22e6d4221, __CFInitialized 0x22e6d4232, __CFProphylacticAutofsAccess 0x22e6d4231
// (hidden) and
// __CFInitializing 0x22e6d2f30 (non-external).
uintptr_t __CFRuntimeBuiltinObjCClassTable[kOCFRuntimeBuiltinClassCount];
void *__CFRuntimeClassTables[2 * kOCFRuntimeClassTablePageCount];
int32_t __CFRuntimeClassTableCount;
static os_unfair_lock __CFBigRuntimeFunnel = OS_UNFAIR_LOCK_INIT;
bool __OCFOASafe;
AllocationEventNameFunction __OCFObjectAllocSetLastAllocEventNameFunction;
static void (*__CFObjectAllocRecordAllocationFunction)(
    int32_t event, void *ptr, intptr_t size, intptr_t count, const char *name
);
uint8_t __CFZombieEnabled;
uint8_t __CFDeallocateZombies;
uint8_t __CFInitialized;
uint8_t __CFProphylacticAutofsAccess;
static uint8_t __CFInitializing;

// Native __CFConstantStringClassReferencePtr, 0x22e6cf768 (__DATA,__data). External; its value is the address
// of the constant-string class.
const void *__OCFConstantStringClassReferencePtr = __OCFConstantStringClassReference;

// Native __CF_FORK_STATE (bit 0 forked after use, bit 1 used, bit 2 threaded at fork). Hidden (was private
// external).
uint8_t __CF_FORK_STATE;

static const OCFRuntimeClass *classForTypeID(OCFTypeID typeID);

// The tag of a tagged pointer, decoded inline as the native functions do: the obfuscator is skipped for
// pointers with bits 63, 62 and 0-2 set, the low three bits are found in the seven-entry permutation table,
// and an index of 7 selects the extended tag ((index | value) >> 55, low byte, plus 8).
static unsigned taggedPointerTag(OCFTypeRef cf) {
    uintptr_t pointer = (uintptr_t)cf;
    uintptr_t obfuscator =
        (pointer & 0xC000000000000007) == 0xC000000000000007 ? 0 : objc_debug_taggedpointer_obfuscator;
    uintptr_t value = obfuscator ^ pointer;
    uintptr_t index = 0;
    while (index != 7 && (value & 7) != objc_debug_tag60_permutations[index]) {
        ++index;
    }
    if ((index & 7) == 7) {
        return (uint8_t)((index | value) >> 55) + 8;
    }
    return (unsigned)(index & 7);
}

static bool isTagged(OCFTypeRef cf) {
    return ((uintptr_t)cf >> 63) != 0;
}

static uintptr_t sendMessage(OCFTypeRef cf, const char *selector) {
    return ((uintptr_t (*)(OCFTypeRef, SEL))objc_msgSend)(cf, sel_getUid(selector));
}

// The native dispatch prelude before a tagged object receives a message: tags 2-6, 20 and 22 go straight to
// the message; tag 0 first asks dyld; any other tag first sends -_cfTypeID, whose result is unused.
static void taggedDispatchPrelude(OCFTypeRef cf) {
    unsigned tag = taggedPointerTag(cf);
    if (tag <= 0x16 && ((1u << tag) & 0x50007C) != 0) {
        return;
    }
    if (tag == 0) {
        dyld_program_sdk_at_least(kTagZeroVersion);
        return;
    }
    sendMessage(cf, "_cfTypeID");
}

// The type ID of a tagged pointer: tag 0 a string from the 2019 SDKs on (else CFType), tag 2 and extended tag
// 22 a string, tag 3 a number, tags 4, 5 and 20 CFType, tag 6 a date; any other tag asks the object.
static OCFTypeID taggedTypeID(OCFTypeRef cf) {
    unsigned tag = taggedPointerTag(cf);
    if ((int)tag <= 3) {
        if (tag == 0) {
            return dyld_program_sdk_at_least(kTagZeroVersion) ? 7 : 1;
        }
        if (tag == 2) {
            return 7;
        }
        if (tag == 3) {
            return 22;
        }
        return sendMessage(cf, "_cfTypeID");
    }
    if ((int)tag <= 19) {
        if (tag - 4 < 2) {
            return 1;
        }
        if (tag == 6) {
            return 42;
        }
        return sendMessage(cf, "_cfTypeID");
    }
    if (tag == 20) {
        return 1;
    }
    if (tag == 22) {
        return 7;
    }
    return sendMessage(cf, "_cfTypeID");
}

// As taggedTypeID, reporting whether the tag was known (no message sent).
static OCFTypeID knownTaggedTypeID(OCFTypeRef cf, bool *known) {
    unsigned tag = taggedPointerTag(cf);
    *known = true;
    switch (tag) {
        case 0:
            return dyld_program_sdk_at_least(kTagZeroVersion) ? 7 : 1;
        case 2:
        case 22:
            return 7;
        case 3:
            return 22;
        case 4:
        case 5:
        case 20:
            return 1;
        case 6:
            return 42;
        default:
            *known = false;
            return sendMessage(cf, "_cfTypeID");
    }
}

static uint64_t infoOf(OCFTypeRef cf) {
    return __atomic_load_n(&((const OCFRuntimeBase *)cf)->info, __ATOMIC_ACQUIRE);
}

static OCFTypeID typeOf(OCFTypeRef cf) {
    return (infoOf(cf) >> 8) & 0x3ff;
}

// The class record for an object's type ID, without the bound check (CFHash, CFEqual and the description
// paths).
static const OCFRuntimeClass *classForTypeIDUnchecked(OCFTypeID typeID) {
    if (typeID > 0x47) {
        unsigned index = (unsigned)(typeID - 72);
        return ((const OCFRuntimeClass **)__CFRuntimeClassTables[index >> 6])[index & 0x3f];
    }
    return __CFRuntimeBuiltinClassTable[typeID];
}

// The class record for a type ID with the dynamic count checked (NULL beyond it or for an empty page).
static const OCFRuntimeClass *classForTypeID(OCFTypeID typeID) {
    if (typeID > 0x47) {
        int index = (int)typeID - 72;
        if (index >= __CFRuntimeClassTableCount || __CFRuntimeClassTables[index >> 6] == NULL) {
            return NULL;
        }
        return ((const OCFRuntimeClass **)__CFRuntimeClassTables[index >> 6])[index & 0x3f];
    }
    return __CFRuntimeBuiltinClassTable[typeID];
}

// The bridged Objective-C class for a type ID (0 beyond the dynamic count or for an empty page).
static uintptr_t objcClassForTypeID(OCFTypeID typeID) {
    if (typeID > 0x47) {
        int index = (int)typeID - 72;
        uintptr_t *page =
            index < __CFRuntimeClassTableCount ? __CFRuntimeClassTables[(index >> 6) + 16] : NULL;
        return page != NULL ? page[index & 0x3f] : 0;
    }
    return __CFRuntimeBuiltinObjCClassTable[typeID];
}

// Native __CFOAInitialize, 0x1c01e506c. Non-external. Only with OAKeepAllocationStatistics set, resolves
// _OAInitialize once (the cached value -1 means unresolved), calls it, resolves the two recording functions
// with dlsym(RTLD_DEFAULT), initializes NSObject's allocation statistics and sets __CFOASafe.
static void *__CFOAInitialize_dyfunc = (void *)-1;

static void __CFOAInitialize(void) {
    if (__CFgetenv("OAKeepAllocationStatistics") == NULL) {
        return;
    }
    void *function = __CFOAInitialize_dyfunc;
    if (function == (void *)-1) {
        function = dlsym(RTLD_DEFAULT, "_OAInitialize");
        __CFOAInitialize_dyfunc = function;
    }
    if (function == NULL) {
        return;
    }
    ((void (*)(void))function)();
    __CFObjectAllocRecordAllocationFunction = dlsym(RTLD_DEFAULT, "_OARecordAllocationEvent");
    __OCFObjectAllocSetLastAllocEventNameFunction = dlsym(RTLD_DEFAULT, "_OASetLastAllocationEventName");
    __CFOAInitializeNSObject();
    __OCFOASafe = 1;
}

// Native __CFgetenv, 0x1c01e5100. External natively; hidden here until the owner decides how a CF name
// followed by a lower-case letter is spelled in prefixed mode (AGENTS.md, "Naming and Coexistence"; recorded
// in the reading). The value __CFInitialize captured for one of the ten names, else getenv.
const char *__CFgetenv(const char *name) {
    for (unsigned index = 0; index != 11; ++index) {
        if (__CFEnv[index][0] != NULL && strcmp(name, __CFEnv[index][0]) == 0) {
            return __CFEnv[index][1];
        }
    }
    return getenv(name);
}

// Native __CFRecordAllocationEvent, 0x1c01e5168. External.
void __OCFRecordAllocationEvent(int32_t event, void *ptr, intptr_t size, intptr_t count, const char *name) {
    if (__OCFOASafe == 1 && __CFObjectAllocRecordAllocationFunction != NULL) {
        __CFObjectAllocRecordAllocationFunction(event, ptr, size, count, name);
    }
}

// Native __CFSetLastAllocationEventName, 0x1c01e518c. External.
void __OCFSetLastAllocationEventName(void *ptr, const char *name) {
    if (__OCFOASafe == 1 && __OCFObjectAllocSetLastAllocEventNameFunction != NULL) {
        __OCFObjectAllocSetLastAllocEventNameFunction(ptr, name);
    }
}

// Native CF_IS_OBJC_CHECKING_CONSTANT_STRING, 0x1c01e533c. Hidden (was private external). Like __CF_IS_OBJC,
// and tagged pointers answer yes; *isConstant is set for a constant string.
// Folds the compiler-outlined piece _CF_IS_OBJC_CHECKING_CONSTANT_STRING.cold.1 (0x1c030bfa0).
uint8_t CF_IS_OBJC_CHECKING_CONSTANT_STRING(OCFTypeID typeID, OCFTypeRef cf, bool *isConstant) {
    *isConstant = false;
    if (isTagged(cf)) {
        return 1;
    }
    uintptr_t isa = ((const OCFRuntimeBase *)cf)->isa;
    if (isa == 0) {
        if (typeID != 0) {
            crashWithMessage("CF objects must have a non-zero isa");
        }
        return (uint8_t)typeID;
    }
    if (typeID == 7 && isa == (uintptr_t)__OCFConstantStringClassReferencePtr) {
        *isConstant = true;
        return 0;
    }
    uintptr_t expected = objcClassForTypeID(typeID);
    if (isa == expected) {
        return 0;
    }
    if ((isa & 1) == 0) {
        return 1;
    }
    return (objc_debug_isa_class_mask & isa) != expected;
}

// Native __CF_IS_OBJC, 0x1c01e5410. Hidden (was private external). Whether an untagged object of the given
// type is an Objective-C object: its isa is not the bridged class (compared directly, or as a non-pointer isa
// through objc_debug_isa_class_mask), except that a constant string's isa counts as CF.
// Folds the compiler-outlined piece ___CF_IS_OBJC.cold.1 (0x1c030bfb4).
uint8_t __CF_IS_OBJC(OCFTypeID typeID, OCFTypeRef cf) {
    uintptr_t isa = ((const OCFRuntimeBase *)cf)->isa;
    if (isa == 0) {
        if (typeID != 0) {
            crashWithMessage("CF objects must have a non-zero isa");
        }
        return (uint8_t)typeID;
    }
    if (typeID == 7 && isa == (uintptr_t)__OCFConstantStringClassReferencePtr) {
        return 0;
    }
    uintptr_t expected = objcClassForTypeID(typeID);
    if (isa == expected) {
        return 0;
    }
    return (isa & 1) == 0 || (objc_debug_isa_class_mask & isa) != expected;
}

// Native _CFRuntimeRegisterClass, 0x1c01e54dc. External. Registration failure reasons are logged at level 4;
// an allocation failure for a new page is _CFRuntimeRegisterClass.cold.1 (crash annotation, BRK #1).
// Folds the compiler-outlined piece __CFRuntimeRegisterClass.cold.1 (0x1c030bfc8).
OCFTypeID _OCFRuntimeRegisterClass(const OCFRuntimeClass *cls) {
    if ((cls->version & kOCFRuntimeClassCustomRefCount) != 0 && cls->refcount == NULL) {
        OCFLog(
            4,
            OF_STR("*** _CFRuntimeRegisterClass() given inconsistent class '%s'.  Program will crash soon."),
            cls->className
        );
        return 0;
    }
    os_unfair_lock_lock_with_options(&__CFBigRuntimeFunnel, kUnfairLockDataSynchronization);
    if (__CFRuntimeClassTableCount >= kOCFRuntimeMaximumDynamicClassCount) {
        OCFLog(
            4,
            OF_STR(
                "*** CoreFoundation class table full; registration failing for class '%s'.  Program will "
                "crash "
                "soon."
            ),
            cls->className
        );
        os_unfair_lock_unlock(&__CFBigRuntimeFunnel);
        return 0;
    }
    int32_t count = __CFRuntimeClassTableCount;
    int32_t page = count >> 6;
    const OCFRuntimeClass **classes = __CFRuntimeClassTables[page];
    if (classes == NULL) {
        classes = malloc_type_calloc(0x40, 8, 0x2004093837F09);
        void *objcClasses = malloc_type_calloc(0x40, 8, 0x100004000313F17);
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        __CFRuntimeClassTables[page] = classes;
        __CFRuntimeClassTables[page + 16] = objcClasses;
        if (classes == NULL) {
            crashWithMessage("Unable to allocate memory for CF runtime type registration");
        }
    }
    classes[count & 0x3f] = cls;
    ((uintptr_t *)__CFRuntimeClassTables[page + 16])[count & 0x3f] = __CFRuntimeBuiltinObjCClassTable[1];
    __atomic_fetch_add(&__CFRuntimeClassTableCount, 1, __ATOMIC_RELAXED);
    os_unfair_lock_unlock(&__CFBigRuntimeFunnel);
    return (OCFTypeID)count + 72;
}

// Native _CFRuntimeBridgeClasses, 0x1c01e5638. External.
void _OCFRuntimeBridgeClasses(OCFTypeID typeID, const char *name) {
    os_unfair_lock_lock_with_options(&__CFBigRuntimeFunnel, kUnfairLockDataSynchronization);
    if (typeID > 0x47) {
        int index = (int)typeID - 72;
        ((uintptr_t *)__CFRuntimeClassTables[(index >> 6) + 16])[index & 0x3f] =
            (uintptr_t)objc_getFutureClass(name);
    } else {
        __CFRuntimeBuiltinObjCClassTable[typeID] = (uintptr_t)objc_getFutureClass(name);
    }
    os_unfair_lock_unlock(&__CFBigRuntimeFunnel);
}

// Native _CFRuntimeGetClassWithTypeID, 0x1c01e56c0 (corpus delta capture 7). External. A builtin type ID
// (at most 0x47, unsigned) indexes __CFRuntimeBuiltinClassTable; a dynamic one (typeID - 72, 32-bit) answers
// NULL at or beyond __CFRuntimeClassTableCount (signed compare) or for an empty page, else its page entry.
const OCFRuntimeClass *_OCFRuntimeGetClassWithTypeID(OCFTypeID typeID) {
    return classForTypeID(typeID);
}

// Native _CFRuntimeCreateInstance, 0x1c01e578c. External.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md ("Instance creation and registration").
OCFTypeRef _OCFRuntimeCreateInstance(
    OCFAllocatorRef allocator, OCFTypeID typeID, OCFIndex extraBytes, const char *category
) {
    const OCFRuntimeClass *cls;
    if (typeID > 0x47) {
        os_unfair_lock_lock_with_options(&__CFBigRuntimeFunnel, kUnfairLockDataSynchronization);
        if (typeID >= (OCFTypeID)__CFRuntimeClassTableCount + 72) {
            __builtin_trap();
        }
        cls = classForTypeID(typeID);
        os_unfair_lock_unlock(&__CFBigRuntimeFunnel);
        if (cls == NULL) {
            return NULL;
        }
    } else {
        cls = __CFRuntimeBuiltinClassTable[typeID];
        if (cls == NULL) {
            return NULL;
        }
    }
    OCFIndex version = cls->version;
    OCFAllocatorRef requested =
        (version & kOCFRuntimeClassRequiresAlignment) != 0 ? kOCFAllocatorSystemDefault : allocator;
    if ((version & kOCFRuntimeClassCustomRefCount) != 0 && cls->refcount == NULL) {
        OCFLog(4, OF_STR("*** _CFRuntimeCreateInstance() found inconsistent class '%s'."), cls->className);
        return NULL;
    }
    OCFAllocatorRef effective = requested;
    if (effective == NULL) {
        effective = _OCFGetTSD(1);
        if (effective == NULL) {
            effective = kOCFAllocatorSystemDefault;
        }
    }
    if (effective == kOCFAllocatorNull) {
        return NULL;
    }
    bool storesAllocator;
    if (effective == kOCFAllocatorSystemDefault) {
        storesAllocator = false;
    } else {
        storesAllocator = effective != NULL || kOCFAllocatorSystemDefault != OCFAllocatorGetDefault();
    }
    size_t alignment = (version & kOCFRuntimeClassRequiresAlignment) != 0 ? cls->requiredAlignment : 16;
    size_t size = ((size_t)extraBytes + (storesAllocator ? 16 : 0) + 31) & ~(size_t)15;
    uint64_t mallocType = (uint64_t)((403 * ((uint32_t)typeID ^ 0xC9DC5)) & 0xFFFFF) | 0xCF000000;
    uint8_t *memory;
    if ((version & kOCFRuntimeClassRequiresAlignment) != 0) {
        memory = malloc_type_zone_memalign(malloc_default_zone(), alignment, size, mallocType);
    } else if (!__OCFAllocatorRespectsHintZeroWhenAllocating(requested)) {
        memory = OCFAllocatorAllocateTyped(requested, (OCFIndex)size, mallocType, 0);
        if (memory == NULL) {
            return NULL;
        }
        bzero(memory, size);
    } else {
        // 0x1c01e5970: an allocator that honors the zero hint is asked for zeroed memory (hint 1).
        memory = OCFAllocatorAllocateTyped(requested, (OCFIndex)size, mallocType, 1);
    }
    if (memory == NULL) {
        return NULL;
    }
    if (category != NULL && __OCFOASafe) {
        if (__OCFObjectAllocSetLastAllocEventNameFunction != NULL) {
            __OCFObjectAllocSetLastAllocEventNameFunction(memory, category);
        }
    } else if (__OCFOASafe && __OCFObjectAllocSetLastAllocEventNameFunction != NULL) {
        __OCFObjectAllocSetLastAllocEventNameFunction(memory, cls->className);
    }
    uint64_t info;
    if (storesAllocator) {
        *(OCFTypeRef *)memory = OCFRetain(effective);
        memory += 16;
        info = 0;
    } else {
        info = kOCFInfoDefaultAllocator;
    }
    info = (info & 0xFFFFFFFF000000FF) | ((typeID & 0xFFFFFF) << 8);
    info |= (version & kOCFRuntimeClassCustomRefCount) != 0 ? 0xFFFFFFFFFF800000 : 0x100000000000;
    OCFRuntimeBase *object = (OCFRuntimeBase *)memory;
    __atomic_store_n(&object->info, info, __ATOMIC_SEQ_CST);
    object->isa = 0;
    object_setClass((id)object, (Class)objcClassForTypeID(typeID));
    if (cls->init != NULL) {
        cls->init(object);
    }
    return object;
}

// Native CFRetain, 0x1c01e5abc. External. NULL crashes; tagged pointers and constant strings are returned
// unchanged; a zero isa crashes unless the type is 0; CF objects retain through _CFRetain, others through
// objc_retain.
// Folds the compiler-outlined pieces _CFRetain.cold.1 (0x1c030bff0), _CFRetain.cold.2 (0x1c030c004).
OCFTypeRef OCFRetain(OCFTypeRef cf) {
    if (cf == NULL) {
        crashWithMessage("*** CFRetain() called with NULL ***");
    }
    if (isTagged(cf)) {
        return cf;
    }
    OCFTypeID typeID = typeOf(cf);
    uintptr_t isa = ((const OCFRuntimeBase *)cf)->isa;
    if (isa == 0) {
        if (typeID != 0) {
            crashWithMessage("CF objects must have a non-zero isa");
        }
        return _OCFRetain(cf, false);
    }
    if (typeID == 7 && isa == (uintptr_t)__OCFConstantStringClassReferencePtr) {
        return cf;
    }
    uintptr_t expected = objcClassForTypeID(typeID);
    if (isa == expected) {
        return _OCFRetain(cf, false);
    }
    if ((isa & 1) == 0) {
        return objc_retain((id)cf);
    }
    if ((objc_debug_isa_class_mask & isa) == expected) {
        return _OCFRetain(cf, false);
    }
    return objc_retain((id)cf);
}

// Native _CFRuntimeInitStaticInstance, 0x1c01e5b98. External. An empty or out-of-range slot traps; a
// custom-refcount class is refused through CFLog.
void _OCFRuntimeInitStaticInstance(void *memory, OCFTypeID typeID) {
    const OCFRuntimeClass *cls = classForTypeID(typeID);
    if (cls == NULL) {
        __builtin_trap();
    }
    if ((cls->version & kOCFRuntimeClassCustomRefCount) != 0) {
        OCFLog(
            3,
            OF_STR("*** Cannot initialize a static instance to a class (%s) with custom ref counting"),
            cls->className
        );
        return;
    }
    OCFRuntimeBase *object = memory;
    __atomic_store_n(&object->info, ((typeID & 0xFFFFFF) << 8) | kOCFInfoDefaultAllocator, __ATOMIC_SEQ_CST);
    object->isa = 0;
    object_setClass((id)object, (Class)objcClassForTypeID(typeID));
    if (cls->init != NULL) {
        cls->init(object);
    }
}

// Native _CFRuntimeSetInstanceTypeID, 0x1c01e5cc0. External. An empty or out-of-range new slot traps; a
// custom-refcount class on either side is refused through CFLog. Only the low eight bits of the new type ID
// are stored (UBFIZ #8, #8) while the whole ten-bit field is cleared; kept as native behavior.
void _OCFRuntimeSetInstanceTypeID(OCFTypeRef cf, OCFTypeID typeID) {
    const OCFRuntimeClass *target = classForTypeID(typeID);
    if (target == NULL) {
        __builtin_trap();
    }
    OCFRuntimeBase *object = (OCFRuntimeBase *)cf;
    OCFTypeID current = typeOf(cf);
    const OCFRuntimeClass *previous = classForTypeIDUnchecked(current);
    if ((previous->version & kOCFRuntimeClassCustomRefCount) != 0 ||
        (current != 0 && (target->version & kOCFRuntimeClassCustomRefCount) != 0)) {
        OCFLog(
            3,
            OF_STR("*** Cannot change the CFTypeID of a %s to a %s due to custom ref counting"),
            previous->className,
            target->className
        );
        return;
    }
    uint64_t info = __atomic_load_n(&object->info, __ATOMIC_ACQUIRE);
    uint64_t updated;
    do {
        updated = (info & 0xFFFFFFFFFFFC00FF) | ((uint64_t)(uint8_t)typeID << 8);
    } while (!__atomic_compare_exchange_n(
        &object->info, &info, updated, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    ));
}

// Native _CFRuntimeSetInstanceTypeIDAndIsa, 0x1c01e5dd0. External.
void _OCFRuntimeSetInstanceTypeIDAndIsa(OCFTypeRef cf, OCFTypeID typeID) {
    _OCFRuntimeSetInstanceTypeID(cf, typeID);
    uintptr_t isa = ((const OCFRuntimeBase *)cf)->isa;
    uintptr_t expected = objcClassForTypeID(typeID);
    if (isa != expected) {
        object_setClass((id)cf, (Class)expected);
    }
}

// Native CFGetTypeID, 0x1c01e5fb0. External.
OCFTypeID OCFGetTypeID(OCFTypeRef cf) {
    if (!isTagged(cf)) {
        if (__CF_IS_OBJC(typeOf(cf), cf) == 0) {
            return typeOf(cf);
        }
        return sendMessage(cf, "_cfTypeID");
    }
    return taggedTypeID(cf);
}

// Native _CFGetNonObjCTypeID, 0x1c01e60e0. Hidden (was private external).
OCFTypeID _CFGetNonObjCTypeID(OCFTypeRef cf) {
    if (!isTagged(cf)) {
        return typeOf(cf);
    }
    return taggedTypeID(cf);
}

// Native _CFGetTypeIDDescription, 0x1c01e61e4. External. NULL for type 1 and for empty or out-of-range slots.
const char *_OCFGetTypeIDDescription(OCFTypeID typeID) {
    const OCFRuntimeClass *cls = classForTypeID(typeID);
    if (typeID == 1 || cls == NULL) {
        return NULL;
    }
    return cls->className;
}

// Native CFCopyTypeIDDescription, 0x1c01e6254. External. The record is read without a check (an empty slot is
// dereferenced, as for the native holes).
OCFStringRef OCFCopyTypeIDDescription(OCFTypeID typeID) {
    const OCFRuntimeClass *cls = classForTypeIDUnchecked(typeID);
    return OCFStringCreateWithCString(kOCFAllocatorSystemDefault, cls->className, 0x600);
}

// Native _CFNonObjCRetain, 0x1c01e62ac. External.
OCFTypeRef _OCFNonObjCRetain(OCFTypeRef cf) {
    return _OCFRetain(cf, false);
}

// Native _CFRetain, 0x1c01e62b4. Non-external. Increments the retain count (bits 44-63) by compare-and-swap,
// saturating at 0xfffff; an immortal object (count 0) is returned unchanged; with tryR, a deallocating or
// deallocated object answers NULL. A custom-refcount object calls its class's refcount(1, object) (crash
// paths _CFRetain.cold.1/.2 for an inconsistent class or header). With __CFOASafe, _CFRetain.cold.3 records
// allocation event 0x1c.
// Folds the compiler-outlined pieces __CFRetain.cold.1 (0x1c030c018), __CFRetain.cold.2 (0x1c030c02c),
// __CFRetain.cold.3 (0x1c030c040).
OCFTypeRef _OCFRetain(OCFTypeRef cf, bool tryR) {
    OCFRuntimeBase *object = (OCFRuntimeBase *)cf;
    uint64_t info = __atomic_load_n(&object->info, __ATOMIC_ACQUIRE);
    if ((info & kOCFInfoCustomRefCount) == 0) {
        // The desired value persists across iterations, as natively: at the saturated count 0xfffff the new
        // value is the previous desired value with its count cleared, which makes the object immortal.
        uint64_t current = info;
        uint64_t desired = info;
        for (;;) {
            if (tryR && (current & (kOCFInfoDeallocated | kOCFInfoDeallocating)) != 0) {
                return NULL;
            }
            if (current >> kOCFInfoRetainShift == 0) {
                return cf;
            }
            desired &= 0xFFFFFFFFFFF;
            if (current >> kOCFInfoRetainShift != 0xFFFFF) {
                desired = current + 0x100000000000;
            }
            if (__atomic_compare_exchange_n(
                    &object->info, &current, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
                )) {
                break;
            }
        }
    } else {
        if (tryR) {
            return NULL;
        }
        const OCFRuntimeClass *cls = classForTypeID((info >> 8) & 0x3ff);
        if (cls->refcount == NULL || (cls->version & kOCFRuntimeClassCustomRefCount) == 0) {
            crashWithMessage("Detected bogus CFTypeRef");
        }
        if (info <= 0xFFFFEFFFFFFFFFFF) {
            crashWithMessage("Detected bogus CFTypeRef");
        }
        cls->refcount(1, cf);
    }
    if (__OCFOASafe == 1) {
        OCFIndex count = OCFGetRetainCount(cf);
        if (__CFObjectAllocRecordAllocationFunction != NULL) {
            __CFObjectAllocRecordAllocationFunction(0x1c, (void *)cf, 0, count, NULL);
        }
    }
    return cf;
}

// Native CFAutorelease, 0x1c01e6414. External.
// Folds the compiler-outlined piece _CFAutorelease.cold.1 (0x1c030c09c).
OCFTypeRef OCFAutorelease(OCFTypeRef arg) {
    if (arg == NULL) {
        crashWithMessage("*** CFAutorelease() called with NULL ***");
    }
    objc_autorelease((id)arg);
    return arg;
}

// Native _CFNonObjCRelease, 0x1c01e6440. External; a tail call of _CFRelease.
void _OCFNonObjCRelease(OCFTypeRef cf) {
    _OCFRelease(cf);
}

// Native _CFRelease, 0x1c01e6444. Non-external.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md ("Release").
// Folds the compiler-outlined pieces __CFRelease.cold.1 (0x1c030c0b0), __CFRelease.cold.2 (0x1c030c104),
// __CFRelease.cold.3 (0x1c030c118), __CFRelease.cold.4 (0x1c030c12c), __CFRelease.cold.5 (0x1c030c154).
void _OCFRelease(OCFTypeRef cf) {
    OCFRuntimeBase *object = (OCFRuntimeBase *)cf;
    uint64_t info = __atomic_load_n(&object->info, __ATOMIC_ACQUIRE);
    OCFTypeID typeID = (info >> 8) & 0x3ff;
    const OCFRuntimeClass *cls = classForTypeID(typeID);
    if ((info & kOCFInfoDeallocated) != 0) {
        char message[256];
        if (cls == NULL) {
            snprintf(message, sizeof(message), "Detected over-release of a CFTypeRef %p (unknown type)", cf);
        } else {
            snprintf(
                message,
                sizeof(message),
                "Detected over-release of a CFTypeRef %p (%lu / %s)",
                cf,
                (unsigned long)typeID,
                cls->className
            );
        }
        crashWithMessage(message);
    }
    OCFIndex count = -1;
    if (__OCFOASafe == 1) {
        count = OCFGetRetainCount(cf) - 1;
    }
    if ((info & kOCFInfoCustomRefCount) != 0) {
        if (cls->refcount == NULL || (cls->version & kOCFRuntimeClassCustomRefCount) == 0) {
            crashWithMessage("Detected bogus CFTypeRef");
        }
        if (info <= 0xFFFFEFFFFFFFFFFF) {
            crashWithMessage("Detected bogus CFTypeRef");
        }
        cls->refcount(-1, cf);
        if (__OCFOASafe == 1 && __CFObjectAllocRecordAllocationFunction != NULL) {
            __CFObjectAllocRecordAllocationFunction(0x1d, (void *)cf, 0, count, NULL);
        }
        return;
    }
    uint64_t current;
    for (;;) {
        for (;;) {
            if (info >> kOCFInfoRetainShift == 1) {
                break;
            }
            if (info >> kOCFInfoRetainShift == 0) {
                return;
            }
            uint64_t expected = info;
            if (__atomic_compare_exchange_n(
                    &object->info, &expected, info - 0x100000000000, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
                )) {
                if (__OCFOASafe == 1 && __CFObjectAllocRecordAllocationFunction != NULL) {
                    __CFObjectAllocRecordAllocationFunction(0x1d, (void *)cf, 0, count, NULL);
                }
                return;
            }
            info = expected;
        }
        if ((cls->version & kOCFRuntimeClassReclaim) != 0 && cls->reclaim != NULL) {
            cls->reclaim(cf);
        }
        uint64_t expected = info;
        if (!__atomic_compare_exchange_n(
                &object->info,
                &expected,
                info | kOCFInfoDeallocating,
                false,
                __ATOMIC_SEQ_CST,
                __ATOMIC_SEQ_CST
            )) {
            info = expected;
            continue;
        }
        if (cls->finalize != NULL) {
            cls->finalize(cf);
        }
        current = __atomic_load_n(&object->info, __ATOMIC_ACQUIRE);
        if (typeID == 2 || (current & 0xFFFFF00000000000) == 0x100000000000) {
            break;
        }
        do {
            info = __atomic_load_n(&object->info, __ATOMIC_ACQUIRE);
            expected = info;
        } while (!__atomic_compare_exchange_n(
            &object->info,
            &expected,
            info & ~(uint64_t)kOCFInfoDeallocating,
            false,
            __ATOMIC_SEQ_CST,
            __ATOMIC_SEQ_CST
        ));
    }
    uint64_t expected = current;
    while (!__atomic_compare_exchange_n(
        &object->info,
        &expected,
        (current | kOCFInfoDeallocated) - 0x100000000000,
        false,
        __ATOMIC_SEQ_CST,
        __ATOMIC_SEQ_CST
    )) {
        current = expected;
    }
    if (__OCFOASafe == 1 && __CFObjectAllocRecordAllocationFunction != NULL) {
        __CFObjectAllocRecordAllocationFunction(0x1d, (void *)cf, 0, 0, NULL);
    }
    if (typeID == 2) {
        __CFAllocatorDeallocate(cf);
        return;
    }
    OCFAllocatorRef allocator;
    bool systemDefault;
    OCFAllocatorRef owned;
    if ((int8_t)__atomic_load_n(&object->info, __ATOMIC_ACQUIRE) < 0) {
        owned = NULL;
        systemDefault = true;
        allocator = kOCFAllocatorSystemDefault;
    } else {
        allocator = OCFGetAllocator(cf);
        if (allocator == kOCFAllocatorSystemDefault) {
            systemDefault = true;
        } else {
            systemDefault = allocator == NULL && kOCFAllocatorSystemDefault == OCFAllocatorGetDefault();
        }
        if (isTagged(cf) || (__CF_IS_OBJC(typeOf(cf), cf) & 1) != 0 || (infoOf(cf) & 0x3FF00) == 0x200) {
            owned = NULL;
        } else if ((int8_t)infoOf(cf) < 0) {
            owned = kOCFAllocatorSystemDefault;
        } else {
            owned = *(OCFAllocatorRef *)((const uint8_t *)cf - 16);
        }
    }
    uint8_t zombies = __CFZombieEnabled;
    Class cls_ = object_getClass((id)cf);
    if (zombies == 0) {
        if (object_getClass((id)cls_) != Nil) {
            objc_destructInstance((id)cf);
        }
    } else {
        os_unfair_lock_lock_with_options(&__CFBigRuntimeFunnel, kUnfairLockDataSynchronization);
        const char *name = NULL;
        for (int32_t index = 0; index < __CFRuntimeClassTableCount + 72; ++index) {
            const OCFRuntimeClass *candidate = NULL;
            if (index >= 72) {
                int dynamicIndex = index - 72;
                uintptr_t *page = dynamicIndex < __CFRuntimeClassTableCount
                                      ? __CFRuntimeClassTables[(dynamicIndex >> 6) + 16]
                                      : NULL;
                uintptr_t bridged = page != NULL ? page[dynamicIndex & 0x3f] : 0;
                if (bridged == (uintptr_t)cls_ && dynamicIndex < __CFRuntimeClassTableCount &&
                    __CFRuntimeClassTables[dynamicIndex >> 6] != NULL) {
                    candidate = ((const OCFRuntimeClass **)
                                     __CFRuntimeClassTables[dynamicIndex >> 6])[dynamicIndex & 0x3f];
                }
            } else if (__CFRuntimeBuiltinObjCClassTable[index] == (uintptr_t)cls_) {
                candidate = __CFRuntimeBuiltinClassTable[index];
            }
            name = candidate != NULL ? candidate->className : NULL;
            if (name != NULL) {
                break;
            }
        }
        os_unfair_lock_unlock(&__CFBigRuntimeFunnel);
        if (name == NULL && object_getClass((id)cls_) != Nil) {
            name = class_getName(cls_);
        }
        char *zombieName = NULL;
        asprintf(&zombieName, "_NSZombie_%s", name != NULL ? name : "$class-unknown$");
        Class zombie = objc_lookUpClass(zombieName);
        if (zombie == Nil) {
            zombie = objc_duplicateClass(objc_lookUpClass("_NSZombie_"), zombieName, 0);
        }
        free(zombieName);
        if (object_getClass((id)cls_) != Nil) {
            objc_destructInstance((id)cf);
        }
        uint8_t deallocate = __CFDeallocateZombies;
        object_setClass((id)cf, zombie);
        if (deallocate == 0) {
            goto releaseOwner;
        }
    }
    OCFAllocatorDeallocate(allocator, (uint8_t *)cf - (systemDefault ? 0 : 16));
releaseOwner:
    if (owned != NULL) {
        OCFRelease(owned);
    }
}

// Native CFRelease, 0x1c01e6984. External; the dispatch of CFRetain, ending in _CFRelease or objc_release.
// Folds the compiler-outlined pieces _CFRelease.cold.1 (0x1c030c17c), _CFRelease.cold.2 (0x1c030c190).
void OCFRelease(OCFTypeRef cf) {
    if (cf == NULL) {
        crashWithMessage("*** CFRelease() called with NULL ***");
    }
    if (isTagged(cf)) {
        return;
    }
    OCFTypeID typeID = typeOf(cf);
    uintptr_t isa = ((const OCFRuntimeBase *)cf)->isa;
    if (isa == 0) {
        if (typeID != 0) {
            crashWithMessage("CF objects must have a non-zero isa");
        }
        _OCFRelease(cf);
        return;
    }
    if (typeID == 7 && isa == (uintptr_t)__OCFConstantStringClassReferencePtr) {
        return;
    }
    uintptr_t expected = objcClassForTypeID(typeID);
    if (isa == expected || ((isa & 1) != 0 && (objc_debug_isa_class_mask & isa) == expected)) {
        _OCFRelease(cf);
        return;
    }
    objc_release((id)cf);
}

// Native __CFStringCollectionCopy, 0x1c01e6a5c. Hidden (was private external); the copy callback of
// kCFTypeDictionaryKeyCallBacks-style string keys. NULL crashes (cold.1).
// Folds the compiler-outlined piece ___CFStringCollectionCopy.cold.1 (0x1c030c1a4).
OCFStringRef __CFStringCollectionCopy(OCFAllocatorRef allocator, OCFStringRef string) {
    if (string == NULL) {
        crashWithMessage("*** __CFStringCollectionCopy() called with NULL ***");
    }
    return OCFStringCreateCopy(allocator, string);
}

// Native __CFTypeCollectionRetain, 0x1c01e6a70. Hidden (was private external). NULL crashes (cold.1).
// Folds the compiler-outlined piece ___CFTypeCollectionRetain.cold.1 (0x1c030c1b8).
OCFTypeRef __CFTypeCollectionRetain(OCFAllocatorRef allocator, OCFTypeRef cf) {
    (void)allocator;
    if (cf == NULL) {
        crashWithMessage(
            "*** __CFTypeCollectionRetain() called with NULL; likely a collection has been corrupted ***"
        );
    }
    return OCFRetain(cf);
}

// Native __CFTypeCollectionRelease, 0x1c01e6a88. Hidden (was private external). NULL crashes (cold.1).
// Folds the compiler-outlined piece ___CFTypeCollectionRelease.cold.1 (0x1c030c1cc).
void __CFTypeCollectionRelease(OCFAllocatorRef allocator, OCFTypeRef cf) {
    (void)allocator;
    if (cf == NULL) {
        crashWithMessage(
            "*** __CFTypeCollectionRelease() called with NULL; likely a collection has been corrupted ***"
        );
    }
    OCFRelease(cf);
}

// Native __CFRuntimeIsImmortal, 0x1c01e6aa0. Hidden (was private external).
bool __CFRuntimeIsImmortal(OCFTypeRef cf) {
    return infoOf(cf) >> kOCFInfoRetainShift == 0;
}

// Native __CFRuntimeSetImmortal, 0x1c01e6ab8. Hidden (was private external). Load-acquire, then a release
// store.
void __CFRuntimeSetImmortal(OCFTypeRef cf) {
    OCFRuntimeBase *object = (OCFRuntimeBase *)cf;
    uint64_t info = __atomic_load_n(&object->info, __ATOMIC_ACQUIRE);
    __atomic_store_n(&object->info, info & 0xFFFFFFFFFFF, __ATOMIC_RELEASE);
}

// Native __CFRuntimeSetRC, 0x1c01e6acc. Hidden (was private external).
void __CFRuntimeSetRC(OCFTypeRef cf, uint32_t retainCount) {
    OCFRuntimeBase *object = (OCFRuntimeBase *)cf;
    uint64_t info = __atomic_load_n(&object->info, __ATOMIC_ACQUIRE);
    __atomic_store_n(
        &object->info,
        (info & 0xFFFFFFFFFFF) | ((uint64_t)retainCount << kOCFInfoRetainShift),
        __ATOMIC_RELEASE
    );
}

// Native CFGetRetainCount, 0x1c01e6ae0. External. NULL crashes; tagged pointers answer INT64_MAX; Objective-C
// objects answer -retainCount; custom-refcount objects their class's refcount(0, object) (W0 zero-extended);
// an immortal object 0x0fffffffffffffff.
// Folds the compiler-outlined pieces _CFGetRetainCount.cold.1 (0x1c030c1e0), _CFGetRetainCount.cold.2
// (0x1c030c1f4).
OCFIndex OCFGetRetainCount(OCFTypeRef cf) {
    if (cf == NULL) {
        crashWithMessage("*** CFGetRetainCount() called with NULL ***");
    }
    if (isTagged(cf)) {
        return 0x7FFFFFFFFFFFFFFF;
    }
    if (__CF_IS_OBJC(typeOf(cf), cf) != 0) {
        return (OCFIndex)sendMessage(cf, "retainCount");
    }
    uint64_t info = infoOf(cf);
    if ((info & kOCFInfoCustomRefCount) != 0) {
        const OCFRuntimeClass *cls = classForTypeID((info >> 8) & 0x3ff);
        if (cls->refcount == NULL || (cls->version & kOCFRuntimeClassCustomRefCount) == 0) {
            __builtin_trap();
        }
        if (info <= 0xFFFFEFFFFFFFFFFF) {
            crashWithMessage("Detected bogus CFTypeRef");
        }
        return (OCFIndex)(uint32_t)cls->refcount(0, cf);
    }
    OCFIndex count = (OCFIndex)(infoOf(cf) >> kOCFInfoRetainShift);
    return count != 0 ? count : 0xFFFFFFFFFFFFFFF;
}

// Native _CFNonObjCEqual, 0x1c01e6bec. External. As CFEqual, with the second argument deciding Objective-C
// dispatch first; a tagged first argument whose tag is not known sends -_cfTypeID twice (type comparison,
// then class lookup).
Boolean _OCFNonObjCEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    if (cf1 == cf2) {
        return 1;
    }
    if (cf2 == NULL) {
        // Native _CFNonObjCEqual.cold.1, 0x1c030c208. Non-external. iOS Device divergence: unread.
        crashWithMessage("*** CFEqual() called with NULL second argument ***");
    }
    if (isTagged(cf2)) {
        taggedDispatchPrelude(cf2);
        return (Boolean)((uintptr_t (*)(OCFTypeRef, SEL, OCFTypeRef))objc_msgSend)(
            cf2, sel_getUid("isEqual:"), cf1
        );
    }
    if (__CF_IS_OBJC(typeOf(cf2), cf2) != 0) {
        return (Boolean)((uintptr_t (*)(OCFTypeRef, SEL, OCFTypeRef))objc_msgSend)(
            cf2, sel_getUid("isEqual:"), cf1
        );
    }
    OCFTypeID typeID;
    if (!isTagged(cf1)) {
        if (((infoOf(cf1) ^ infoOf(cf2)) & 0x3FF00) != 0) {
            return 0;
        }
        typeID = typeOf(cf1);
    } else {
        if (taggedTypeID(cf1) != typeOf(cf2)) {
            return 0;
        }
        // A second decode: a known tag's type indexes the builtin table directly; an unknown tag asks the
        // object again and goes through the general lookup.
        bool known;
        typeID = knownTaggedTypeID(cf1, &known);
        if (known) {
            Boolean (*equal)(OCFTypeRef, OCFTypeRef) = __CFRuntimeBuiltinClassTable[typeID]->equal;
            return equal != NULL ? equal(cf1, cf2) : 0;
        }
    }
    Boolean (*equal)(OCFTypeRef, OCFTypeRef) = classForTypeIDUnchecked(typeID)->equal;
    return equal != NULL ? equal(cf1, cf2) : 0;
}

// Native CFEqual, 0x1c01e6f60. External.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
// Folds the compiler-outlined pieces _CFEqual.cold.1 (0x1c030c21c), _CFEqual.cold.2 (0x1c030c230).
Boolean OCFEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    if (cf1 == NULL) {
        crashWithMessage("*** CFEqual() called with NULL first argument ***");
    }
    if (cf1 == cf2) {
        return 1;
    }
    if (cf2 == NULL) {
        crashWithMessage("*** CFEqual() called with NULL second argument ***");
    }
    Boolean (*isEqual)(OCFTypeRef, SEL, OCFTypeRef) = (Boolean (*)(OCFTypeRef, SEL, OCFTypeRef))objc_msgSend;
    if (!isTagged(cf1)) {
        if (__CF_IS_OBJC(typeOf(cf1), cf1) != 0) {
            return isEqual(cf1, sel_getUid("isEqual:"), cf2);
        }
        if (isTagged(cf2)) {
            taggedDispatchPrelude(cf2);
            return isEqual(cf2, sel_getUid("isEqual:"), cf1);
        }
        if (__CF_IS_OBJC(typeOf(cf2), cf2) != 0) {
            return isEqual(cf2, sel_getUid("isEqual:"), cf1);
        }
        if (((infoOf(cf1) ^ infoOf(cf2)) & 0x3FF00) != 0) {
            return 0;
        }
        Boolean (*equal)(OCFTypeRef, OCFTypeRef) = classForTypeIDUnchecked(typeOf(cf1))->equal;
        return equal != NULL ? equal(cf1, cf2) : 0;
    }
    if (isTagged(cf2)) {
        unsigned tag1 = taggedPointerTag(cf1);
        unsigned tag2 = taggedPointerTag(cf2);
        if (tag1 == tag2) {
            if (tag1 <= 6 && ((1u << tag1) & 0x54) != 0) {
                return 0;
            }
        } else {
            if (tag1 == 2) {
                return __NSInlineTaggedPointerStringIsEqualToTaggedPointer(cf1, cf2);
            }
            if (tag2 == 2) {
                return __NSInlineTaggedPointerStringIsEqualToTaggedPointer(cf2, cf1);
            }
            if (tag1 == 0) {
                return __NSIndirectTaggedPointerStringIsEqualToTaggedPointer(cf1, cf2);
            }
            if (tag2 != 0) {
                return 0;
            }
            return __NSIndirectTaggedPointerStringIsEqualToTaggedPointer(cf2, cf1);
        }
    }
    taggedDispatchPrelude(cf1);
    return isEqual(cf1, sel_getUid("isEqual:"), cf2);
}

// Native _CFNonObjCHash, 0x1c01e72d8. External. Without a hash callback the object's address is the hash.
OCFHashCode _OCFNonObjCHash(OCFTypeRef cf) {
    OCFTypeID typeID = isTagged(cf) ? taggedTypeID(cf) : typeOf(cf);
    OCFHashCode (*hash)(OCFTypeRef) = classForTypeIDUnchecked(typeID)->hash;
    return hash != NULL ? hash(cf) : (OCFHashCode)cf;
}

// Native CFHash, 0x1c01e7444. External. NULL crashes; tagged numbers hash with __CFNumberHash; other tagged
// pointers and Objective-C objects answer -hash; without a hash callback the address is the hash.
// Folds the compiler-outlined piece _CFHash.cold.1 (0x1c030c244).
OCFHashCode OCFHash(OCFTypeRef cf) {
    if (cf == NULL) {
        crashWithMessage("*** CFHash() called with NULL ***");
    }
    if (isTagged(cf)) {
        unsigned tag = taggedPointerTag(cf);
        if (tag == 3) {
            return __CFNumberHash(cf);
        }
        if (tag == 0) {
            dyld_program_sdk_at_least(kTagZeroVersion);
        }
        taggedDispatchPrelude(cf);
        return sendMessage(cf, "hash");
    }
    if (__CF_IS_OBJC(typeOf(cf), cf) != 0) {
        return sendMessage(cf, "hash");
    }
    OCFHashCode (*hash)(OCFTypeRef) = classForTypeIDUnchecked(typeOf(cf))->hash;
    return hash != NULL ? hash(cf) : (OCFHashCode)cf;
}

// Native CFCopyDescription, 0x1c01e7614. External. NULL answers NULL; tagged dates and numbers use their
// family's description; other tagged pointers and Objective-C objects answer -_copyDescription; a missing or
// NULL class description becomes "<%s %p [%p]>" with the class name, the object and its allocator.
OCFStringRef OCFCopyDescription(OCFTypeRef cf) {
    if (cf == NULL) {
        return NULL;
    }
    if (isTagged(cf)) {
        unsigned tag = taggedPointerTag(cf);
        if (tag == 6) {
            return __CFDateCopyDescription(cf);
        }
        if (tag == 3) {
            return __CFNumberCopyDescription(cf);
        }
        if (tag == 0) {
            dyld_program_sdk_at_least(kTagZeroVersion);
        }
        taggedDispatchPrelude(cf);
        return (OCFStringRef)sendMessage(cf, "_copyDescription");
    }
    if (__CF_IS_OBJC(typeOf(cf), cf) != 0) {
        return (OCFStringRef)sendMessage(cf, "_copyDescription");
    }
    const OCFRuntimeClass *cls = classForTypeIDUnchecked(typeOf(cf));
    OCFStringRef description = NULL;
    if (cls->copyDebugDesc != NULL) {
        description = cls->copyDebugDesc(cf);
    }
    if (description == NULL) {
        return OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault, NULL, OF_STR("<%s %p [%p]>"), cls->className, cf, OCFGetAllocator(cf)
        );
    }
    return description;
}

// Native CFGetAllocator, 0x1c01e783c. External. NULL, tagged and Objective-C objects answer the system
// default; allocators answer their owning allocator; other objects the system default (info bit 7) or the
// allocator stored 16 bytes before them.
OCFAllocatorRef OCFGetAllocator(OCFTypeRef cf) {
    if (cf == NULL || isTagged(cf) || __CF_IS_OBJC(typeOf(cf), cf) != 0) {
        return kOCFAllocatorSystemDefault;
    }
    if ((infoOf(cf) & 0x3FF00) == 0x200) {
        return __CFAllocatorGetAllocator(cf);
    }
    if ((int8_t)infoOf(cf) < 0) {
        return kOCFAllocatorSystemDefault;
    }
    return *(OCFAllocatorRef *)((const uint8_t *)cf - 16);
}

// Native __CFCopyFormattingDescription, 0x1c01e78c4. External. Tagged numbers use
// __CFNumberCopyFormattingDescription (through the data pointer 0x2227a2530); tagged dates answer NULL; other
// tagged pointers and Objective-C objects answer -_copyFormattingDescription: when they respond to it.
OCFStringRef __OCFCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions) {
    if (cf == NULL) {
        return NULL;
    }
    if (isTagged(cf)) {
        unsigned tag = taggedPointerTag(cf);
        if (tag == 0) {
            dyld_program_sdk_at_least(kTagZeroVersion);
        } else if (tag == 6) {
            return NULL;
        } else if (tag == 3) {
            return __CFNumberCopyFormattingDescription(cf, formatOptions);
        }
        taggedDispatchPrelude(cf);
    } else if (__CF_IS_OBJC(typeOf(cf), cf) == 0) {
        OCFStringRef (*format)(OCFTypeRef, const void *) =
            classForTypeIDUnchecked(typeOf(cf))->copyFormattingDesc;
        return format != NULL ? format(cf, formatOptions) : NULL;
    }
    SEL selector = sel_getUid("_copyFormattingDescription:");
    if ((objc_opt_respondsToSelector((id)cf, selector) & 1) == 0) {
        return NULL;
    }
    return ((OCFStringRef (*)(OCFTypeRef, SEL, const void *))objc_msgSend)(cf, selector, formatOptions);
}

// Native __CF_USED, 0x1c01e7ac8. Hidden (was private external).
void __CF_USED(void) {
    __atomic_fetch_or(&__CF_FORK_STATE, 2, __ATOMIC_SEQ_CST);
}

// Native __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__,
// 0x1c01e7adc. External; two writes to stderr.
void __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__(void) {
    write(
        2,
        "The process has forked and you cannot use this CoreFoundation functionality safely. You MUST "
        "exec().\n",
        0x65
    );
    write(
        2,
        "Break on "
        "__THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__() "
        "to debug.\n",
        0x75
    );
}

// Native __CFProcessIsRestricted, 0x1c01e7b4c. Hidden (was private external).
bool __CFProcessIsRestricted(void) {
    return (uint8_t)issetugid() != 0;
}

// Native __CFGetDYLDImageSuffix, 0x1c01e7b64, with ____CFGetDYLDImageSuffix_block_invoke (0x1c01e7b94; its
// dispatch_once is __CFGetDYLDImageSuffix.cold.1). External. DYLD_IMAGE_SUFFIX unless the process is setugid.
// Folds the compiler-outlined piece ___CFGetDYLDImageSuffix.cold.1 (0x1c030c258).
const char *__OCFGetDYLDImageSuffix(void) {
    static dispatch_once_t onceToken;
    static const char *suffix;
    dispatch_once(&onceToken, ^{
      suffix = (uint8_t)issetugid() != 0 ? NULL : __CFgetenv("DYLD_IMAGE_SUFFIX");
    });
    return suffix;
}

// Native os_log_callback, 0x1c01e7bcc. External natively, a name outside CF/NS; hidden here until the owner
// records a naming decision (AGENTS.md, "Naming and Coexistence"). The os_log CF formatter installed by
// __CFInitialize; returns 0.
static void *os_log_callback(void) {
    return NULL;
}

// Native __cf_atfork_prepare, 0x1c01e8068. Non-external.
static void __cf_atfork_prepare(void) {
    if (pthread_is_threaded_np() != 0) {
        __atomic_fetch_or(&__CF_FORK_STATE, 4, __ATOMIC_SEQ_CST);
    } else {
        __atomic_fetch_and(&__CF_FORK_STATE, 0xfb, __ATOMIC_SEQ_CST);
    }
}

// Native __cf_atfork_child, 0x1c01e8098. Non-external.
static void __cf_atfork_child(void) {
    if ((__atomic_load_n(&__CF_FORK_STATE, __ATOMIC_RELAXED) & 2) != 0) {
        __atomic_fetch_or(&__CF_FORK_STATE, 1, __ATOMIC_SEQ_CST);
    }
}

// Native ThreadSanitizer hooks resolved by __CFInitialize: __cf_tsanRegisterTagFunction and
// __cf_tsanAssignTagFunction (__DATA,__bss, non-external), __cf_tsanReadFunction and __cf_tsanWriteFunction
// (__DATA,__common, external; declared by Runtime/OCFRuntime.h) and the five __OCFTSANTagMutable* tags
// (__DATA,__common, external).
static void *(*__cf_tsanRegisterTagFunction)(const char *name);
static void *__cf_tsanAssignTagFunction;
void *__cf_tsanReadFunction;
void *__cf_tsanWriteFunction;
void *__OCFTSANTagMutableArray;
void *__OCFTSANTagMutableDictionary;
void *__OCFTSANTagMutableSet;
void *__OCFTSANTagMutableOrderedSet;
void *__OCFTSANTagMutableData;

// Native __CFInitialize, 0x1c01e7bd4, the image's only initializer (its __init_offsets entry). External: the
// native image exports ___CFInitialize, and Foundation's platform initialization calls it.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md ("Load-time initialization").
// It registers process-global runtime state (tagged-pointer classes, the bridged class table, libobjc and
// libdispatch hooks), so, as for +[NSObject(NSObject) load], only the isolated system-name build runs it at
// load time; the prefixed build, which shares its process with the system CoreFoundation, keeps the
// definition without the initializer entry (AGENTS.md, "Trace comparison").
#if OF_SYSTEM_NAMES
__attribute__((constructor))
#endif
void __OCFInitialize(void) {
    if (__CFInitialized != 0 || (__CFInitializing & 1) != 0) {
        return;
    }
    __CFProphylacticAutofsAccess = 1;
    __CFInitializing = 1;
    for (unsigned index = 0; index != 11; ++index) {
        if (__CFEnv[index][0] != NULL) {
            const char *value = getenv(__CFEnv[index][0]);
            if (value != NULL) {
                __CFEnv[index][1] = value;
            }
        }
    }
    __exceptionInit();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
    objc_setForwardHandler((void *)_CF_forwarding_prep_0, NULL);
#pragma clang diagnostic pop
    objc_setEnumerationMutationHandler((void (*)(id))__NSFastEnumerationMutationHandler);
    uint32_t encodings[2] = {0, 0};
    _OCFStringGetUserDefaultEncoding(&encodings[1], &encodings[0]);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
    pthread_atfork(__cf_atfork_prepare, NULL, __cf_atfork_child);
#pragma clang diagnostic pop
    const char *logging = getenv("NSObjCMessageLoggingEnabled");
    if (logging != NULL && ((uint8_t)*logging | 0x20) == 'y') {
        instrumentObjcMessageSends(1);
    }
    _OCFRuntimeBridgeClasses(0, OF_NAME_STRING(__ONSCFType));
    uintptr_t typeClass = __CFRuntimeBuiltinObjCClassTable[0];
    for (unsigned index = 1; index != 72; ++index) {
        __CFRuntimeBuiltinObjCClassTable[index] = typeClass;
    }
    _OCFRuntimeBridgeClasses(7, OF_NAME_STRING(__ONSCFString));
    _OCFRuntimeBridgeClasses(16, OF_NAME_STRING(ONSNull));
    _OCFRuntimeBridgeClasses(17, OF_NAME_STRING(__ONSCFSet));
    _OCFRuntimeBridgeClasses(18, OF_NAME_STRING(__ONSCFDictionary));
    _OCFRuntimeBridgeClasses(19, OF_NAME_STRING(__ONSCFArray));
    _OCFRuntimeBridgeClasses(20, OF_NAME_STRING(__ONSCFData));
    _OCFRuntimeBridgeClasses(21, OF_NAME_STRING(__ONSCFBoolean));
    _OCFRuntimeBridgeClasses(22, OF_NAME_STRING(__ONSCFNumber));
    OCFNumberGetTypeID();
    __CFCharacterSetInitialize();
    _OCFRuntimeBridgeClasses(OCFMachPortGetTypeID(), OF_NAME_STRING(ONSMachPort));
    _OCFRuntimeBridgeClasses(OCFURLGetTypeID(), OF_NAME_STRING(ONSURL));
    _OCFRuntimeBridgeClasses(OCFRunLoopTimerGetTypeID(), OF_NAME_STRING(__ONSCFTimer));
    _OCFRuntimeBridgeClasses(OCFAttributedStringGetTypeID(), OF_NAME_STRING(__ONSCFAttributedString));
    _OCFRuntimeBridgeClasses(OCFLocaleGetTypeID(), OF_NAME_STRING(__ONSCFLocale));
    _OCFRuntimeBridgeClasses(OCFErrorGetTypeID(), OF_NAME_STRING(__ONSCFError));
    _OCFRuntimeBridgeClasses(OCFCharacterSetGetTypeID(), OF_NAME_STRING(__ONSCFCharacterSet));
    _OCFRuntimeBridgeClasses(OCFReadStreamGetTypeID(), OF_NAME_STRING(__ONSCFInputStream));
    _OCFRuntimeBridgeClasses(OCFWriteStreamGetTypeID(), OF_NAME_STRING(__ONSCFOutputStream));
    _OCFProcessPath();
    _dispatch_begin_NSAutoReleasePool = _OCFAutoreleasePoolPush;
    _dispatch_end_NSAutoReleasePool = (void (*)(void *))(void *)_OCFAutoreleasePoolPop;
    __CFOAInitialize();
    __CFStringInitializeSystemEncoding();
    __CFMakeNSBlockClasses();
    const char *zombies = getenv("NSZombieEnabled");
    if (zombies != NULL && ((uint8_t)*zombies | 0x20) == 'y' && __CFZombieEnabled == 0) {
        __CFZombieEnabled = 0xff;
        __CFZombifyNSObject();
    }
    const char *deallocate = getenv("NSDeallocateZombies");
    if (deallocate != NULL && ((uint8_t)*deallocate | 0x20) == 'y') {
        __CFDeallocateZombies = 0xff;
    }
    __CFProphylacticAutofsAccess = 0;
    _os_log_set_nscf_formatter((void *)os_log_callback);
    void *library = _OCFGetHandleForInsertedOrInterposingLibrary("libclang_rt.tsan");
    if (library != NULL) {
        __cf_tsanRegisterTagFunction = dlsym(library, "__tsan_external_register_tag");
        __cf_tsanAssignTagFunction = dlsym(library, "__tsan_external_assign_tag");
        __cf_tsanReadFunction = dlsym(library, "__tsan_external_read");
        __cf_tsanWriteFunction = dlsym(library, "__tsan_external_write");
        bool complete = __cf_tsanRegisterTagFunction != NULL && __cf_tsanAssignTagFunction != NULL &&
                        __cf_tsanReadFunction != NULL && __cf_tsanWriteFunction != NULL;
        if (complete) {
            __OCFTSANTagMutableArray = __cf_tsanRegisterTagFunction(OF_NAME_STRING(ONSMutableArray));
            __OCFTSANTagMutableDictionary =
                __cf_tsanRegisterTagFunction(OF_NAME_STRING(ONSMutableDictionary));
            __OCFTSANTagMutableSet = __cf_tsanRegisterTagFunction(OF_NAME_STRING(ONSMutableSet));
            __OCFTSANTagMutableOrderedSet =
                __cf_tsanRegisterTagFunction(OF_NAME_STRING(ONSMutableOrderedSet));
            __OCFTSANTagMutableData = __cf_tsanRegisterTagFunction(OF_NAME_STRING(ONSMutableData));
            complete = __OCFTSANTagMutableArray != NULL && __OCFTSANTagMutableDictionary != NULL &&
                       __OCFTSANTagMutableSet != NULL && __OCFTSANTagMutableOrderedSet != NULL &&
                       __OCFTSANTagMutableData != NULL;
        }
        if (!complete) {
            __cf_tsanRegisterTagFunction = NULL;
            __cf_tsanAssignTagFunction = NULL;
            __cf_tsanReadFunction = NULL;
            __cf_tsanWriteFunction = NULL;
            __OCFTSANTagMutableArray = NULL;
            __OCFTSANTagMutableDictionary = NULL;
            __OCFTSANTagMutableSet = NULL;
            __OCFTSANTagMutableOrderedSet = NULL;
            __OCFTSANTagMutableData = NULL;
        }
    }
    __CFInitializing = 0;
    __CFInitialized = 1;
}

// Native _CFTryRetain, 0x1c01e80b8. External.
OCFTypeRef _OCFTryRetain(OCFTypeRef cf) {
    if (cf != NULL && !isTagged(cf)) {
        return _OCFRetain(cf, true);
    }
    return cf;
}

// Native _CFIsDeallocating, 0x1c01e80cc. External. Signed values below 1 answer false.
bool _OCFIsDeallocating(OCFTypeRef cf) {
    if ((intptr_t)cf < 1) {
        return false;
    }
    uint64_t info = infoOf(cf);
    return (info & kOCFInfoCustomRefCount) != 0 || (info & kOCFInfoDeallocating) != 0;
}

// Native _CFAssertMismatchedTypeID, 0x1c030bf30 (among the .cold entries). Hidden (was a private external);
// declared by Runtime/OCFRuntime.h. The two type descriptions
// (_CFGetTypeIDDescription, "<unknown>" when NULL) fill "Expected typeID %lu (%s) does not match actual
// typeID %lu (%s)" in a 255-byte stack buffer, which becomes the crash message before BRK #1.
void _CFAssertMismatchedTypeID(OCFTypeID expected, OCFTypeID actual) {
    char message[256];
    const char *expectedName = _OCFGetTypeIDDescription(expected);
    if (expectedName == NULL) {
        expectedName = "<unknown>";
    }
    const char *actualName = _OCFGetTypeIDDescription(actual);
    if (actualName == NULL) {
        actualName = "<unknown>";
    }
    snprintf(
        message,
        0xFF,
        "Expected typeID %lu (%s) does not match actual typeID %lu (%s)",
        expected,
        expectedName,
        actual,
        actualName
    );
    crashWithMessage(message);
}
