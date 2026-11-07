//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Layouts read from the iOS Simulator image (24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC); confidence:
// verified for every offset named here. Type and field names are local descriptive spellings.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.

// Every CF object starts with an Objective-C isa and an atomically accessed info word: bits 0-6 are
// type-specific, bit 7 marks the system default allocator (and static instances), bits 8-17 hold the type ID,
// bit 21 (0x200000) marks a deallocated object, bit 22 (0x400000) a deallocation in progress, bit 23
// (0x800000) a class with custom reference counting, and bits 44-63 the retain count (zero: immortal).
typedef struct {
    uintptr_t isa;
    uint64_t info;
} OCFRuntimeBase;

_Static_assert(sizeof(OCFRuntimeBase) == 16, "The native object header is 16 bytes.");

enum {
    kOCFInfoDefaultAllocator = 0x80,
    kOCFInfoDeallocated = 0x200000,
    kOCFInfoDeallocating = 0x400000,
    kOCFInfoCustomRefCount = 0x800000,
    kOCFInfoRetainShift = 44,
};

// Class record flags (the version word): bit 2 reclaim, bit 3 custom reference counting, bit 4 required
// alignment.
enum {
    kOCFRuntimeClassReclaim = 0x4,
    kOCFRuntimeClassCustomRefCount = 0x8,
    kOCFRuntimeClassRequiresAlignment = 0x10,
};

typedef struct __OCFRuntimeClass {
    OCFIndex version;
    const char *className;
    void (*init)(OCFTypeRef cf);
    OCFTypeRef (*copy)(OCFAllocatorRef allocator, OCFTypeRef cf);
    void (*finalize)(OCFTypeRef cf);
    Boolean (*equal)(OCFTypeRef cf1, OCFTypeRef cf2);
    OCFHashCode (*hash)(OCFTypeRef cf);
    OCFStringRef (*copyFormattingDesc)(OCFTypeRef cf, const void *formatOptions);
    OCFStringRef (*copyDebugDesc)(OCFTypeRef cf);
    void (*reclaim)(OCFTypeRef cf);
    uint32_t (*refcount)(intptr_t op, OCFTypeRef cf);
    uintptr_t requiredAlignment;
} OCFRuntimeClass;

OF_ABI_ASSERT(sizeof(OCFRuntimeClass) == 96, "The native class record is 96 bytes.");

enum {
    kOCFRuntimeBuiltinClassCount = 72,
    kOCFRuntimeClassTablePageSize = 64,
    kOCFRuntimeClassTablePageCount = 16,
    kOCFRuntimeMaximumDynamicClassCount = 1024,
};

// The first 128 bytes of a CFAllocator are a malloc_zone_t (the object header overlaps reserved1 and
// reserved2); the CF part follows. Offsets: owning allocator 128 (kCFAllocatorUseContext when the object
// allocated itself through its context), context version 136, info 144, retain 152, release 160,
// copyDescription 168, allocate 176, reallocate 184, deallocate 192, preferredSize 200, typed allocate 208,
// typed reallocate 216.
typedef void *(*OCFAllocatorTypedAllocateCallBack)(
    OCFIndex size, uint64_t typeID, OCFOptionFlags hint, void *info
);
typedef void *(*OCFAllocatorTypedReallocateCallBack)(
    void *ptr, OCFIndex newsize, uint64_t typeID, OCFOptionFlags hint, void *info
);

struct __OCFAllocator {
    OCFRuntimeBase base;
    size_t (*size)(void *zone, const void *ptr);
    void *(*malloc)(void *zone, size_t size);
    void *(*calloc)(void *zone, size_t count, size_t size);
    void *(*valloc)(void *zone, size_t size);
    void (*free)(void *zone, void *ptr);
    void *(*realloc)(void *zone, void *ptr, size_t size);
    void (*destroy)(void *zone);
    const char *zoneName;
    void *batchMalloc;
    void *batchFree;
    const void *introspect;
    uint32_t version;
    uint32_t padding;
    void *memalign;
    void *freeDefiniteSize;
    OCFAllocatorRef allocator;
    OCFAllocatorContext context;
    OCFAllocatorTypedAllocateCallBack typedAllocate;
    OCFAllocatorTypedReallocateCallBack typedReallocate;
};

OF_ABI_ASSERT(sizeof(struct __OCFAllocator) == 224, "The native allocator object is 224 bytes.");
OF_ABI_ASSERT(offsetof(struct __OCFAllocator, allocator) == 128, "Owning allocator at 128.");
OF_ABI_ASSERT(offsetof(struct __OCFAllocator, context.info) == 144, "Context info at 144.");
OF_ABI_ASSERT(offsetof(struct __OCFAllocator, typedAllocate) == 208, "Typed allocate at 208.");

// Class tables: 72 builtin class records and their bridged Objective-C classes, then up to 16 pages of 64
// dynamically registered records (__CFRuntimeClassTables[0..15]) and their Objective-C classes
// (__CFRuntimeClassTables[16..31]), counted by __CFRuntimeClassTableCount.
OF_EXTERN const OCFRuntimeClass *const __CFRuntimeBuiltinClassTable[kOCFRuntimeBuiltinClassCount];
OF_EXTERN uintptr_t __CFRuntimeBuiltinObjCClassTable[kOCFRuntimeBuiltinClassCount];
OF_EXTERN void *__CFRuntimeClassTables[2 * kOCFRuntimeClassTablePageCount];
OF_EXTERN int32_t __CFRuntimeClassTableCount;

// The checks for a CFAllocator compare the isa with __CFRuntimeBuiltinObjCClassTable[2] (0x22e6d3ff0), and
// dynamic registration stores __CFRuntimeBuiltinObjCClassTable[1] (0x22e6d3fe8) as the default bridged class.
// The dynamic class count (0x22e6d3fd8, after the 32 page pointers) has no native symbol; its name is a local
// descriptive spelling.

// Allocation-statistics hooks, set by __CFOAInitialize when OAKeepAllocationStatistics is present.
OF_EXPORT bool __OCFOASafe;

// Native __CFInitialize, 0x1c01e7bd4 (external), the image initializer; defined in Runtime/OCFRuntime.c.
OF_EXPORT void __OCFInitialize(void);
typedef void (*AllocationEventNameFunction)(void *ptr, const char *name);
OF_EXPORT AllocationEventNameFunction __OCFObjectAllocSetLastAllocEventNameFunction;

// The address of the constant-string class (the isa of compiled constant strings).
OF_EXPORT const void *__OCFConstantStringClassReferencePtr;

OF_EXTERN uint8_t __CF_IS_OBJC(OCFTypeID typeID, OCFTypeRef cf);

OF_EXTERN bool __CFProcessIsRestricted(void);

OF_EXPORT OCFTypeRef _OCFRetain(OCFTypeRef cf, bool tryR);

OF_EXPORT void _OCFRelease(OCFTypeRef cf);

// The static CFNull instance (external natively as __kCFNull).
OF_EXPORT const OCFRuntimeBase __kOCFNull;

// Builtin class records defined by their families' units.
OF_EXTERN const OCFRuntimeClass __CFAllocatorClass;
OF_EXTERN const OCFRuntimeClass __CFNullClass;

// Header state: the retain count (bits 44-63) and the type ID (bits 8-17, eight bits written).
OF_EXTERN void __CFRuntimeSetRC(OCFTypeRef cf, uint32_t retainCount);
OF_EXPORT void _OCFRuntimeSetInstanceTypeID(OCFTypeRef cf, OCFTypeID typeID);

OF_EXPORT void __OCFSetLastAllocationEventName(void *ptr, const char *name);

// The allocator class's paths from the runtime.
OF_EXTERN OCFAllocatorRef __CFAllocatorGetAllocator(OCFTypeRef cf);
OF_EXTERN void __CFAllocatorDeallocate(OCFTypeRef cf);
OF_EXPORT bool __OCFAllocatorRespectsHintZeroWhenAllocating(OCFAllocatorRef allocator);

// The failure callback of the safe reallocation functions: called with the original pointer, it sets *stop to
// handle the failure itself.
typedef void (^ReallocationFailureBlock)(void *ptr, bool *stop);

OF_EXTERN void __CFReallocationFailed(void *ptr, size_t size, ReallocationFailureBlock reallocationFailed);

OF_EXPORT void *__OCFSafelyReallocateTyped(
    void *ptr, size_t size, uint64_t typeID, ReallocationFailureBlock reallocationFailed
);

OF_EXPORT void *__OCFSafelyReallocateWithAllocatorTyped(
    OCFAllocatorRef allocator,
    void *ptr,
    OCFIndex size,
    OCFOptionFlags hint,
    uint64_t typeID,
    ReallocationFailureBlock reallocationFailed
);

// Crash annotations (__DATA,__crash_info, version 7). The native crash paths (mostly .cold entries) store
// their message at offset 8 and execute BRK #1; crashWithMessage does both.
OF_EXTERN uint64_t gCRAnnotations[41];

__attribute__((noreturn)) static inline void crashWithMessage(const char *message) {
    gCRAnnotations[1] = (uint64_t)(uintptr_t)message;
    __builtin_trap();
}

OF_EXTERN uint8_t __CFZombieEnabled;
OF_EXTERN uint8_t __CFDeallocateZombies;
OF_EXTERN uint8_t __CFInitialized;
OF_EXTERN uint8_t __CFProphylacticAutofsAccess;
OF_EXTERN uint8_t __CF_FORK_STATE;

OF_EXTERN const char *__CFgetenv(const char *name);
OF_EXPORT void
__OCFRecordAllocationEvent(int32_t event, void *ptr, intptr_t size, intptr_t count, const char *name);
OF_EXTERN uint8_t CF_IS_OBJC_CHECKING_CONSTANT_STRING(OCFTypeID typeID, OCFTypeRef cf, bool *isConstant);

OF_EXPORT OCFTypeID _OCFRuntimeRegisterClass(const OCFRuntimeClass *cls);
OF_EXPORT void _OCFRuntimeBridgeClasses(OCFTypeID typeID, const char *name);
OF_EXPORT const OCFRuntimeClass *_OCFRuntimeGetClassWithTypeID(OCFTypeID typeID);
OF_EXPORT OCFTypeRef _OCFRuntimeCreateInstance(
    OCFAllocatorRef allocator, OCFTypeID typeID, OCFIndex extraBytes, const char *category
);
OF_EXPORT void _OCFRuntimeInitStaticInstance(void *memory, OCFTypeID typeID);
OF_EXPORT void _OCFRuntimeSetInstanceTypeIDAndIsa(OCFTypeRef cf, OCFTypeID typeID);

OF_EXTERN OCFTypeID _CFGetNonObjCTypeID(OCFTypeRef cf);
OF_EXPORT const char *_OCFGetTypeIDDescription(OCFTypeID typeID);
OF_EXPORT OCFTypeRef _OCFNonObjCRetain(OCFTypeRef cf);
OF_EXPORT void _OCFNonObjCRelease(OCFTypeRef cf);
OF_EXPORT Boolean _OCFNonObjCEqual(OCFTypeRef cf1, OCFTypeRef cf2);
OF_EXPORT OCFHashCode _OCFNonObjCHash(OCFTypeRef cf);
OF_EXPORT OCFStringRef __OCFCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions);
OF_EXPORT OCFTypeRef _OCFTryRetain(OCFTypeRef cf);
OF_EXPORT bool _OCFIsDeallocating(OCFTypeRef cf);

OF_EXTERN OCFStringRef __CFStringCollectionCopy(OCFAllocatorRef allocator, OCFStringRef string);
OF_EXTERN OCFTypeRef __CFTypeCollectionRetain(OCFAllocatorRef allocator, OCFTypeRef cf);
OF_EXTERN void __CFTypeCollectionRelease(OCFAllocatorRef allocator, OCFTypeRef cf);
OF_EXTERN bool __CFRuntimeIsImmortal(OCFTypeRef cf);
OF_EXTERN void __CFRuntimeSetImmortal(OCFTypeRef cf);

OF_EXTERN void __CF_USED(void);
// Native names outside the CF/NS prefix rule, exported natively; hidden until the owner records a naming
// decision.
OF_EXTERN void
__THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__(void);
OF_EXPORT const char *__OCFGetDYLDImageSuffix(void);

OF_EXPORT void *__OCFTSANTagMutableArray;
OF_EXPORT void *__OCFTSANTagMutableDictionary;
OF_EXPORT void *__OCFTSANTagMutableSet;
OF_EXPORT void *__OCFTSANTagMutableOrderedSet;
OF_EXPORT void *__OCFTSANTagMutableData;

// Type-ID functions of the families __CFInitialize bridges, defined by their units (the URL, locale, error,
// Mach port and stream ones are declared in their families' headers).
OF_EXPORT OCFTypeID OCFRunLoopTimerGetTypeID(void);
OF_EXPORT OCFTypeID OCFAttributedStringGetTypeID(void);

// The handle of an inserted or interposing library whose path matches, or NULL (bundle unit).
OF_EXPORT void *_OCFGetHandleForInsertedOrInterposingLibrary(const char *name);

// Native _CFAssertMismatchedTypeID, 0x1c030bf30 (Runtime/OCFRuntime.c). Hidden (was a private external): the
// type-mismatch crash; the native callers emit nothing after the call.
OF_EXTERN void _CFAssertMismatchedTypeID(OCFTypeID expected, OCFTypeID actual) __attribute__((noreturn));

// The ThreadSanitizer read and write hooks (native __cf_tsanReadFunction, __DATA,__common 0x22e6d4240, and
// __cf_tsanWriteFunction, 0x22e6d4248; both external) that the bridged collection methods consult. Each holds
// a __tsan_external_read/__tsan_external_write entry point or NULL and is called with the object, the
// caller's return address and the type's tag. The names fall outside the CF/NS prefix rule (lower-case "cf");
// hidden until the owner records a naming decision.
OF_EXTERN void *__cf_tsanReadFunction;
OF_EXTERN void *__cf_tsanWriteFunction;
