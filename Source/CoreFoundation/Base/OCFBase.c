//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/OFPortHooks.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <mach/vm_page_size.h>
#include <malloc/malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Inferred organization: the allocator functions (0x1c01502e8-0x1c0150dec), __CFRangeMake (0x1c0150dec),
// CFNull (0x1c0150df0-0x1c0150e54) and the allocator zone callbacks (0x1c0150e5c-0x1c0150f98) are contiguous
// in the iOS Simulator __text section and are kept in one translation unit; the native source path is not in
// the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: constants and calls compared
// for the entry points named in the reading, bodies unread. Reading:
// Documentation/Reference/Research/RuntimePrerequisites.md.

// kCFAllocatorUseContext is the literal 0x3ab (939) at every native use.
#define kAllocatorUseContext ((OCFAllocatorRef)(uintptr_t)0x3ab)

// Bridged classes stored in static objects: __NSCFType for the static allocators, NSNull for kCFNull.
extern char bridgedTypeClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSCFType));
extern char bridgedNullClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSNull));

static size_t __CFAllocatorCustomSize(void *zone, const void *ptr);
static void *__CFAllocatorCustomMalloc(void *zone, size_t size);
static void *__CFAllocatorCustomCalloc(void *zone, size_t count, size_t size);
static void *__CFAllocatorCustomValloc(void *zone, size_t size);
static void __CFAllocatorCustomFree(void *zone, void *ptr);
static void *__CFAllocatorCustomRealloc(void *zone, void *ptr, size_t size);
static void __CFAllocatorCustomDestroy(void *zone);
static void __CFAllocatorNullDestroy(void *zone);
static uintptr_t __CFAllocatorZoneIntrospectNoOp(void);
static size_t __CFAllocatorCustomGoodSize(void *zone, size_t size);
static uintptr_t __CFAllocatorZoneIntrospectTrue(void);
static size_t __CFAllocatorNullSize(void *zone, const void *ptr);
static void *__CFAllocatorNullMalloc(void *zone, size_t size);
static void *__CFAllocatorNullCalloc(void *zone, size_t count, size_t size);
static void *__CFAllocatorNullValloc(void *zone, size_t size);
static void __CFAllocatorNullFree(void *zone, void *ptr);
static void *__CFAllocatorNullRealloc(void *zone, void *ptr, size_t size);
static void *__CFAllocatorNullAllocate(OCFIndex size, OCFOptionFlags hint, void *info);
static void *__CFAllocatorNullReallocate(void *ptr, OCFIndex newsize, OCFOptionFlags hint, void *info);
static size_t __CFAllocatorNullGoodSize(void *zone, size_t size);
static void *__CFAllocatorSystemAllocate(OCFIndex size, OCFOptionFlags hint, void *info);
static void *__CFAllocatorSystemReallocate(void *ptr, OCFIndex newsize, OCFOptionFlags hint, void *info);
static void __CFAllocatorSystemDeallocate(void *ptr, void *info);
static void *
__CFAllocatorSystemAllocateTyped(OCFIndex size, uint64_t typeID, OCFOptionFlags hint, void *info);
static void *__CFAllocatorSystemReallocateTyped(
    void *ptr, OCFIndex newsize, uint64_t typeID, OCFOptionFlags hint, void *info
);
static OCFStringRef __CFAllocatorCopyDescription(OCFTypeRef cf);
static void __CFNullDeallocate(OCFTypeRef cf);
static OCFHashCode __CFNullHash(OCFTypeRef cf);
static OCFStringRef __CFNullCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions);
static OCFStringRef __CFNullCopyDescription(OCFTypeRef cf);

// Native __MallocDefaultZoneInfoPlaceholder, 0x1c03212d0 (__TEXT,__const, the sixteen bytes
// 00000000 00000000 02032008 00000000). Non-external; the system allocators' info, replaced by
// malloc_default_zone() at each use. Only its address is significant.
static const uint8_t __MallocDefaultZoneInfoPlaceholder[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0x02, 0x03, 0x20, 0x08
};

// Native __CFAllocatorZoneIntrospect and __CFAllocatorNullZoneIntrospect, 0x222429428 and 0x222429750
// (__DATA_CONST,__const, 136 bytes each). Non-external. Enumerator, good size, check, print, log, force lock,
// force unlock and statistics, then nine NULL slots; the custom table's good size is
// __CFAllocatorCustomGoodSize, the null table's __CFAllocatorNullGoodSize, and the check answers true.
static const void *const __CFAllocatorZoneIntrospect[17] = {
    (const void *)__CFAllocatorZoneIntrospectNoOp,
    (const void *)__CFAllocatorCustomGoodSize,
    (const void *)__CFAllocatorZoneIntrospectTrue,
    (const void *)__CFAllocatorZoneIntrospectNoOp,
    (const void *)__CFAllocatorZoneIntrospectNoOp,
    (const void *)__CFAllocatorZoneIntrospectNoOp,
    (const void *)__CFAllocatorZoneIntrospectNoOp,
    (const void *)__CFAllocatorZoneIntrospectNoOp,
};

static const void *const __CFAllocatorNullZoneIntrospect[17] = {
    (const void *)__CFAllocatorZoneIntrospectNoOp,
    (const void *)__CFAllocatorNullGoodSize,
    (const void *)__CFAllocatorZoneIntrospectTrue,
    (const void *)__CFAllocatorZoneIntrospectNoOp,
    (const void *)__CFAllocatorZoneIntrospectNoOp,
    (const void *)__CFAllocatorZoneIntrospectNoOp,
    (const void *)__CFAllocatorZoneIntrospectNoOp,
    (const void *)__CFAllocatorZoneIntrospectNoOp,
};

// Native __kCFAllocatorSystemDefault, __kCFAllocatorMalloc, __kCFAllocatorMallocZone and __kCFAllocatorNull,
// 0x222429348, 0x2224294b0, 0x222429590 and 0x222429670 (__DATA_CONST,__const, 224 bytes each). Non-external.
// Static instances: isa __NSCFType, info 0x280 (type 2, static), owning allocator the system default.
// kCFAllocatorMalloc's context callbacks are libsystem_malloc's malloc, realloc and free and its typed slots
// malloc_type_malloc and malloc_type_realloc (out-of-image addresses 0x1c00e8964, 0x1c00e8bd4, 0x1c00e8a00,
// 0x1c00c32f0, 0x1c00c33dc; named by the reference probe in Local/Evidence/cf-core/probe-malloc-allocator.md;
// confidence: medium for that binding).
#define StaticZoneFields(                                                                                    \
    sizeFunction,                                                                                            \
    mallocFunction,                                                                                          \
    callocFunction,                                                                                          \
    vallocFunction,                                                                                          \
    freeFunction,                                                                                            \
    reallocFunction,                                                                                         \
    name,                                                                                                    \
    introspection                                                                                            \
)                                                                                                            \
    .base = {(uintptr_t)bridgedTypeClass, 0x280}, .size = (void *)sizeFunction,                              \
    .malloc = (void *)mallocFunction, .calloc = (void *)callocFunction, .valloc = (void *)vallocFunction,    \
    .free = (void *)freeFunction, .realloc = (void *)reallocFunction,                                        \
    .destroy = (void *)__CFAllocatorNullDestroy, .zoneName = name, .introspect = introspection,              \
    .version = 6, .allocator = &__kCFAllocatorSystemDefault

static const struct __OCFAllocator __kCFAllocatorSystemDefault;

static const struct __OCFAllocator __kCFAllocatorSystemDefault = {
    StaticZoneFields(
        __CFAllocatorCustomSize,
        __CFAllocatorCustomMalloc,
        __CFAllocatorCustomCalloc,
        __CFAllocatorCustomValloc,
        __CFAllocatorCustomFree,
        __CFAllocatorCustomRealloc,
        OF_NAME_STRING(kOCFAllocatorSystemDefault),
        __CFAllocatorZoneIntrospect
    ),
    .context =
        {0,
         (void *)__MallocDefaultZoneInfoPlaceholder,
         NULL,
         NULL,
         NULL,
         __CFAllocatorSystemAllocate,
         __CFAllocatorSystemReallocate,
         __CFAllocatorSystemDeallocate,
         NULL},
    .typedAllocate = __CFAllocatorSystemAllocateTyped,
    .typedReallocate = __CFAllocatorSystemReallocateTyped,
};

static const struct __OCFAllocator __kCFAllocatorMalloc = {
    StaticZoneFields(
        __CFAllocatorCustomSize,
        __CFAllocatorCustomMalloc,
        __CFAllocatorCustomCalloc,
        __CFAllocatorCustomValloc,
        __CFAllocatorCustomFree,
        __CFAllocatorCustomRealloc,
        OF_NAME_STRING(kOCFAllocatorMalloc),
        __CFAllocatorZoneIntrospect
    ),
    .context =
        {0,
         NULL,
         NULL,
         NULL,
         NULL,
         OF_PORT_FUNCTION(mallocAllocatorAllocate, (OCFAllocatorAllocateCallBack)(void *)malloc),
         OF_PORT_FUNCTION(mallocAllocatorReallocate, (OCFAllocatorReallocateCallBack)(void *)realloc),
         OF_PORT_FUNCTION(mallocAllocatorDeallocate, (OCFAllocatorDeallocateCallBack)(void *)free),
         NULL},
    .typedAllocate = OF_PORT_FUNCTION(
        mallocAllocatorTypedAllocate, (OCFAllocatorTypedAllocateCallBack)(void *)malloc_type_malloc
    ),
    .typedReallocate = OF_PORT_FUNCTION(
        mallocAllocatorTypedReallocate, (OCFAllocatorTypedReallocateCallBack)(void *)malloc_type_realloc
    ),
};

static const struct __OCFAllocator __kCFAllocatorMallocZone = {
    StaticZoneFields(
        __CFAllocatorCustomSize,
        __CFAllocatorCustomMalloc,
        __CFAllocatorCustomCalloc,
        __CFAllocatorCustomValloc,
        __CFAllocatorCustomFree,
        __CFAllocatorCustomRealloc,
        OF_NAME_STRING(kOCFAllocatorMallocZone),
        __CFAllocatorZoneIntrospect
    ),
    .context =
        {0,
         (void *)__MallocDefaultZoneInfoPlaceholder,
         NULL,
         NULL,
         NULL,
         __CFAllocatorSystemAllocate,
         __CFAllocatorSystemReallocate,
         __CFAllocatorSystemDeallocate,
         NULL},
    .typedAllocate = __CFAllocatorSystemAllocateTyped,
    .typedReallocate = __CFAllocatorSystemReallocateTyped,
};

static const struct __OCFAllocator __kCFAllocatorNull = {
    StaticZoneFields(
        __CFAllocatorNullSize,
        __CFAllocatorNullMalloc,
        __CFAllocatorNullCalloc,
        __CFAllocatorNullValloc,
        __CFAllocatorNullFree,
        __CFAllocatorNullRealloc,
        OF_NAME_STRING(kOCFAllocatorNull),
        __CFAllocatorNullZoneIntrospect
    ),
    .context = {
        0, NULL, NULL, NULL, NULL, __CFAllocatorNullAllocate, __CFAllocatorNullReallocate, NULL, NULL
    },
};

// Native kCFAllocatorDefault (0x1c03212b0, NULL), kCFAllocatorSystemDefault (0x222429250), kCFAllocatorMalloc
// (0x222429258), kCFAllocatorMallocZone (0x222429260) and kCFAllocatorNull (0x222429268). External.
const OCFAllocatorRef kOCFAllocatorDefault = NULL;
const OCFAllocatorRef kOCFAllocatorSystemDefault = &__kCFAllocatorSystemDefault;
const OCFAllocatorRef kOCFAllocatorMalloc = &__kCFAllocatorMalloc;
const OCFAllocatorRef kOCFAllocatorMallocZone = &__kCFAllocatorMallocZone;
const OCFAllocatorRef kOCFAllocatorNull = &__kCFAllocatorNull;

// Native __CFAllocatorClass, 0x222429270 (__DATA_CONST,__const). Hidden (was private external); version 0,
// name "CFAllocator", only the debug description callback.
const OCFRuntimeClass __CFAllocatorClass = {
    0,
    "CFAllocator",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    __CFAllocatorCopyDescription,
    NULL,
    NULL,
    0,
};

static bool isAllocator(OCFAllocatorRef allocator) {
    return allocator->base.isa == __CFRuntimeBuiltinObjCClassTable[2];
}

static OCFAllocatorRef defaultAllocator(void) {
    OCFAllocatorRef allocator = _OCFGetTSD(1);
    return allocator != NULL ? allocator : &__kCFAllocatorSystemDefault;
}

// Native __CFAllocatorGetAllocator, 0x1c01502e8. Hidden (was private external).
OCFAllocatorRef __CFAllocatorGetAllocator(OCFTypeRef cf) {
    OCFAllocatorRef allocator = cf;
    if (allocator->allocator != kAllocatorUseContext) {
        return allocator->allocator;
    }
    return allocator;
}

// Native __CFAllocatorDeallocate, 0x1c01502f8. Hidden (was private external); _CFRelease's path for type 2.
// An allocator that allocated itself through its context frees itself with the context's deallocate and then
// releases the info; otherwise the info is released first, the object is freed with its owning allocator and
// that allocator is released.
void __CFAllocatorDeallocate(OCFTypeRef cf) {
    struct __OCFAllocator *self = (struct __OCFAllocator *)cf;
    OCFAllocatorRef allocator = self->allocator;
    OCFAllocatorReleaseCallBack release = self->context.release;
    if (allocator == kAllocatorUseContext) {
        OCFAllocatorDeallocateCallBack deallocate = self->context.deallocate;
        void *info = self->context.info;
        if (deallocate != NULL) {
            deallocate(self, info);
        }
        if (release != NULL) {
            release(info);
        }
    } else {
        if (release != NULL) {
            release(self->context.info);
        }
        OCFAllocatorDeallocate(allocator, self);
        OCFRelease(allocator);
    }
}

// Native CFAllocatorDeallocate, 0x1c0150390. External.
void OCFAllocatorDeallocate(OCFAllocatorRef allocator, void *ptr) {
    if (allocator == NULL) {
        allocator = defaultAllocator();
    }
    if (isAllocator(allocator)) {
        if (ptr != NULL) {
            OCFAllocatorDeallocateCallBack deallocate = allocator->context.deallocate;
            if (deallocate != NULL) {
                deallocate(ptr, allocator->context.info);
            }
        }
    } else {
        malloc_zone_free((malloc_zone_t *)allocator, ptr);
    }
}

// Native __CFAllocatorCopyDescription, 0x1c015040c. Non-external; the allocator class's debug description.
static OCFStringRef __CFAllocatorCopyDescription(OCFTypeRef cf) {
    const struct __OCFAllocator *self = cf;
    OCFAllocatorRef allocator = self->allocator;
    if (allocator == kAllocatorUseContext) {
        allocator = self;
    }
    return OCFStringCreateWithFormat(
        allocator, NULL, OF_STR("<CFAllocator %p [%p]>{info = %p}"), self, allocator, self->context.info
    );
}

// Native CFAllocatorGetDefault, 0x1c0150458. External.
OCFAllocatorRef OCFAllocatorGetDefault(void) {
    return defaultAllocator();
}

static void *
_CFAllocatorAllocateImpl(OCFAllocatorRef allocator, OCFIndex size, uint64_t typeID, OCFOptionFlags hint);

// Native CFAllocatorCreate, 0x1c01504fc. External.
// Accepts NULL, kCFAllocatorUseContext or a CFAllocator. The context's info is retained first; a
// kCFAllocatorUseContext allocator allocates its own 224 bytes through the context's allocate (hint 0), any
// other through _CFAllocatorAllocateImpl (malloc type 0x10F00406A531AB1) from the given or default allocator,
// which is then retained. The header is zeroed, the retain count set to 1 and the type to 2, the isa set to
// the allocator class, the malloc-zone part filled with the custom zone functions, and the context copied
// with both typed slots cleared.
OCFAllocatorRef OCFAllocatorCreate(OCFAllocatorRef allocator, OCFAllocatorContext *context) {
    if (allocator != NULL && allocator != kAllocatorUseContext && !isAllocator(allocator)) {
        return NULL;
    }
    OCFAllocatorAllocateCallBack allocate = context->allocate;
    void *info = context->info;
    OCFAllocatorRetainCallBack retain = context->retain;
    if (retain != NULL) {
        info = (void *)retain(info);
    }
    struct __OCFAllocator *self;
    if (allocator == kAllocatorUseContext) {
        if (allocate == NULL) {
            return NULL;
        }
        self = allocate(0xe0, 0, info);
        if (self == NULL) {
            return NULL;
        }
    } else {
        if (allocator == NULL) {
            allocator = defaultAllocator();
        }
        self = _CFAllocatorAllocateImpl(allocator, 0xe0, 0x10F00406A531AB1, 0);
        if (self == NULL) {
            return NULL;
        }
        if (__OCFOASafe == 1) {
            __OCFSetLastAllocationEventName(self, "CFAllocator");
        }
        OCFRetain(allocator);
    }
    self->base.isa = 0;
    self->base.info = 0;
    __CFRuntimeSetRC(self, 1);
    _OCFRuntimeSetInstanceTypeID(self, 2);
    self->base.isa = __CFRuntimeBuiltinObjCClassTable[2];
    self->size = (void *)__CFAllocatorCustomSize;
    self->malloc = (void *)__CFAllocatorCustomMalloc;
    self->calloc = (void *)__CFAllocatorCustomCalloc;
    self->valloc = (void *)__CFAllocatorCustomValloc;
    self->free = (void *)__CFAllocatorCustomFree;
    self->realloc = (void *)__CFAllocatorCustomRealloc;
    self->destroy = (void *)__CFAllocatorCustomDestroy;
    self->zoneName = "Custom CFAllocator";
    self->batchMalloc = NULL;
    self->batchFree = NULL;
    self->introspect = __CFAllocatorZoneIntrospect;
    self->version = 6;
    self->memalign = NULL;
    self->freeDefiniteSize = NULL;
    self->allocator = allocator;
    self->context.version = context->version;
    self->context.info = info;
    self->context.retain = retain;
    self->context.release = context->release;
    self->context.copyDescription = context->copyDescription;
    self->context.allocate = allocate;
    self->context.reallocate = context->reallocate;
    self->context.deallocate = context->deallocate;
    self->context.preferredSize = context->preferredSize;
    self->typedAllocate = NULL;
    self->typedReallocate = NULL;
    return self;
}

// Native __CFAllocatorSystemAllocate, 0x1c015075c. Non-external. The placeholder info stands for the default
// malloc zone; hint 1 allocates zeroed memory.
static void *__CFAllocatorSystemAllocate(OCFIndex size, OCFOptionFlags hint, void *info) {
    malloc_zone_t *zone = info;
    if (zone == (malloc_zone_t *)__MallocDefaultZoneInfoPlaceholder) {
        zone = malloc_default_zone();
    }
    if (hint == 1) {
        return malloc_type_zone_calloc(zone, 1, (size_t)size, 0xB480B5E);
    }
    return malloc_type_zone_malloc(zone, (size_t)size, 0x513E9A69);
}

// Native __CFAllocatorSystemReallocate, 0x1c01507c8. Non-external.
static void *__CFAllocatorSystemReallocate(void *ptr, OCFIndex newsize, OCFOptionFlags hint, void *info) {
    (void)hint;
    malloc_zone_t *zone = info;
    if (zone == (malloc_zone_t *)__MallocDefaultZoneInfoPlaceholder) {
        zone = malloc_default_zone();
    }
    return malloc_type_zone_realloc(zone, ptr, (size_t)newsize, 0xD69374A8);
}

// Native __CFAllocatorSystemDeallocate, 0x1c0150814. Non-external.
static void __CFAllocatorSystemDeallocate(void *ptr, void *info) {
    malloc_zone_t *zone = info;
    if (zone == (malloc_zone_t *)__MallocDefaultZoneInfoPlaceholder) {
        zone = malloc_default_zone();
    }
    malloc_zone_free(zone, ptr);
}

// Native __CFAllocatorSystemAllocateTyped, 0x1c0150850. Non-external.
static void *
__CFAllocatorSystemAllocateTyped(OCFIndex size, uint64_t typeID, OCFOptionFlags hint, void *info) {
    malloc_zone_t *zone = info;
    if (zone == (malloc_zone_t *)__MallocDefaultZoneInfoPlaceholder) {
        zone = malloc_default_zone();
    }
    if (hint == 1) {
        return malloc_type_zone_calloc(zone, 1, (size_t)size, typeID);
    }
    return malloc_type_zone_malloc(zone, (size_t)size, typeID);
}

// Native __CFAllocatorSystemReallocateTyped, 0x1c01508c4. Non-external.
static void *__CFAllocatorSystemReallocateTyped(
    void *ptr, OCFIndex newsize, uint64_t typeID, OCFOptionFlags hint, void *info
) {
    (void)hint;
    malloc_zone_t *zone = info;
    if (zone == (malloc_zone_t *)__MallocDefaultZoneInfoPlaceholder) {
        zone = malloc_default_zone();
    }
    return malloc_type_zone_realloc(zone, ptr, (size_t)newsize, typeID);
}

// Native _CFAllocatorAllocateImpl, 0x1c0150924. Non-external. A zero size returns NULL; a plain malloc zone
// uses malloc_type_zone_malloc; a CFAllocator prefers its typed allocate, else its context allocate.
static void *
_CFAllocatorAllocateImpl(OCFAllocatorRef allocator, OCFIndex size, uint64_t typeID, OCFOptionFlags hint) {
    if (allocator == NULL) {
        allocator = defaultAllocator();
    }
    if (size == 0) {
        return NULL;
    }
    if (!isAllocator(allocator)) {
        return malloc_type_zone_malloc((malloc_zone_t *)allocator, (size_t)size, typeID);
    }
    if (allocator->typedAllocate != NULL) {
        return allocator->typedAllocate(size, typeID, hint, allocator->context.info);
    }
    if (allocator->context.allocate != NULL) {
        return allocator->context.allocate(size, hint, allocator->context.info);
    }
    return NULL;
}

// Native CFAllocatorReallocateImpl, 0x1c0150a00. Non-external. A NULL pointer with a positive size allocates;
// a pointer with size 0 frees; NULL with size 0 returns NULL; otherwise (including a negative size) it
// reallocates. Each path uses the malloc zone, the typed slot or the context callback.
static void *CFAllocatorReallocateImpl(
    OCFAllocatorRef allocator, void *ptr, OCFIndex newsize, uint64_t typeID, OCFOptionFlags hint
) {
    if (allocator == NULL) {
        allocator = defaultAllocator();
    }
    if (ptr == NULL && newsize >= 1) {
        if (!isAllocator(allocator)) {
            return malloc_type_zone_malloc((malloc_zone_t *)allocator, (size_t)newsize, typeID);
        }
        if (allocator->typedAllocate != NULL) {
            return allocator->typedAllocate(newsize, typeID, hint, allocator->context.info);
        }
        if (allocator->context.allocate != NULL) {
            return allocator->context.allocate(newsize, hint, allocator->context.info);
        }
        return NULL;
    }
    if (ptr != NULL && newsize == 0) {
        if (!isAllocator(allocator)) {
            malloc_zone_free((malloc_zone_t *)allocator, ptr);
        } else if (allocator->context.deallocate != NULL) {
            allocator->context.deallocate(ptr, allocator->context.info);
        }
        return NULL;
    }
    if (ptr == NULL && newsize == 0) {
        return NULL;
    }
    if (!isAllocator(allocator)) {
        return malloc_type_zone_realloc((malloc_zone_t *)allocator, ptr, (size_t)newsize, typeID);
    }
    if (allocator->typedReallocate != NULL) {
        return allocator->typedReallocate(ptr, newsize, typeID, hint, allocator->context.info);
    }
    if (allocator->context.reallocate != NULL) {
        return allocator->context.reallocate(ptr, newsize, hint, allocator->context.info);
    }
    return NULL;
}

// Native CFAllocatorAllocateTyped, 0x1c0150b98. External; a tail call of _CFAllocatorAllocateImpl.
void *OCFAllocatorAllocateTyped(
    OCFAllocatorRef allocator, OCFIndex size, OCFAllocatorTypeID descriptor, OCFOptionFlags hint
) {
    return _CFAllocatorAllocateImpl(allocator, size, descriptor, hint);
}

// Native CFAllocatorGetPreferredSizeForSize, 0x1c0150bb8. External. A plain malloc zone answers
// malloc_good_size; a CFAllocator asks its preferredSize callback for positive sizes and never answers less
// than the size.
OCFIndex OCFAllocatorGetPreferredSizeForSize(OCFAllocatorRef allocator, OCFIndex size, OCFOptionFlags hint) {
    if (allocator == NULL) {
        allocator = defaultAllocator();
    }
    if (!isAllocator(allocator)) {
        return (OCFIndex)malloc_good_size((size_t)size);
    }
    OCFIndex preferred = 0;
    if (size >= 1 && allocator->context.preferredSize != NULL) {
        preferred = allocator->context.preferredSize(size, hint, allocator->context.info);
    }
    return preferred <= size ? size : preferred;
}

// Native __CFSafelyReallocateImpl, 0x1c0150cf8. Non-external. A failed realloc reports through
// __CFReallocationFailed with the original pointer, the size and the callback, and returns NULL.
static void *__CFSafelyReallocateImpl(
    void *ptr, size_t size, uint64_t typeID, ReallocationFailureBlock reallocationFailed
) {
    void *result = malloc_type_realloc(ptr, size, typeID);
    if (result == NULL) {
        __CFReallocationFailed(ptr, size, reallocationFailed);
    }
    return result;
}

// Native __CFSafelyReallocateWithAllocatorImpl, 0x1c0150d58. Non-external. Reports a failure only when a
// pointer or a size was given. Its fourth argument (X3) is forwarded as the realloc hint and its fifth (X4)
// as the malloc type ID (MOV X3, X4; MOV X4, X8). Every native caller of
// __CFSafelyReallocateWithAllocatorTyped passes its malloc type ID constant in X3 and 0 in X4, so natively
// the reallocation receives type ID 0 and the constant as the hint; the parameter names describe that
// register flow, which the restoration keeps.
static void *__CFSafelyReallocateWithAllocatorImpl(
    OCFAllocatorRef allocator,
    void *ptr,
    OCFIndex size,
    OCFOptionFlags hint,
    uint64_t typeID,
    ReallocationFailureBlock reallocationFailed
) {
    void *result = CFAllocatorReallocateImpl(allocator, ptr, size, typeID, hint);
    if (result == NULL && (ptr != NULL || size != 0)) {
        __CFReallocationFailed(ptr, (size_t)size, reallocationFailed);
        return NULL;
    }
    return result;
}

// Native __CFSafelyReallocateTyped, 0x1c0150dc4. External; a tail call.
void *__OCFSafelyReallocateTyped(
    void *ptr, size_t size, uint64_t typeID, ReallocationFailureBlock reallocationFailed
) {
    return __CFSafelyReallocateImpl(ptr, size, typeID, reallocationFailed);
}

// Native __CFSafelyReallocateWithAllocatorTyped, 0x1c0150dc8. External; a tail call.
void *__OCFSafelyReallocateWithAllocatorTyped(
    OCFAllocatorRef allocator,
    void *ptr,
    OCFIndex size,
    OCFOptionFlags hint,
    uint64_t typeID,
    ReallocationFailureBlock reallocationFailed
) {
    return __CFSafelyReallocateWithAllocatorImpl(allocator, ptr, size, hint, typeID, reallocationFailed);
}

// Native __CFAllocatorRespectsHintZeroWhenAllocating, 0x1c0150dcc. External.
bool __OCFAllocatorRespectsHintZeroWhenAllocating(OCFAllocatorRef allocator) {
    return allocator == &__kCFAllocatorSystemDefault || allocator == &__kCFAllocatorMallocZone;
}

// Native __CFRangeMake: 0x1c0150dec, iOS Simulator 27.0 (24A434), arm64.
// CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device comparison: __CFRangeMake, 0x18072d198, iOS 27.0.1 (24A446), arm64e,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; same register return; confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationBase.md.
OCFRange __OCFRangeMake(OCFIndex location, OCFIndex length) {
    OCFRange result;
    result.location = location;
    result.length = length;
    return result;
}

// Native __CFNullClass, 0x2224292e8 (__DATA_CONST,__const). Hidden (was private external); version 0, name
// "CFNull", finalize, hash and both description callbacks.
const OCFRuntimeClass __CFNullClass = {
    0,
    "CFNull",
    NULL,
    NULL,
    __CFNullDeallocate,
    NULL,
    __CFNullHash,
    __CFNullCopyFormattingDescription,
    __CFNullCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __kCFNull (0x2224292d0, __DATA_CONST,__const; isa NSNull, info 0x1080: type 16, static) and kCFNull
// (0x2224292e0). External.
const OCFRuntimeBase __kOCFNull = {(uintptr_t)bridgedNullClass, 0x1080};
const OCFNullRef kOCFNull = (OCFNullRef)&__kOCFNull;

// Native __CFNullDeallocate, 0x1c0150df0. Non-external; no effect.
static void __CFNullDeallocate(OCFTypeRef cf) {
    (void)cf;
}

// Native __CFNullHash, 0x1c0150df4. Non-external; always 1984.
static OCFHashCode __CFNullHash(OCFTypeRef cf) {
    (void)cf;
    return 1984;
}

// Native __CFNullCopyFormattingDescription, 0x1c0150dfc. Non-external.
static OCFStringRef __CFNullCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions) {
    (void)cf;
    (void)formatOptions;
    return OCFRetain(OF_STR("null"));
}

// Native __CFNullCopyDescription, 0x1c0150e08. Non-external.
static OCFStringRef __CFNullCopyDescription(OCFTypeRef cf) {
    return OCFStringCreateWithFormat(
        &__kCFAllocatorSystemDefault, NULL, OF_STR("<CFNull %p [%p]>"), cf, OCFGetAllocator(cf)
    );
}

// Native CFNullGetTypeID, 0x1c0150e4c. External.
OCFTypeID OCFNullGetTypeID(void) {
    return 16;
}

// Native __CFAllocatorCustomSize, 0x1c0150e5c. Non-external; always 0.
static size_t __CFAllocatorCustomSize(void *zone, const void *ptr) {
    (void)zone;
    (void)ptr;
    return 0;
}

// Native __CFAllocatorCustomMalloc, 0x1c0150e64. Non-external.
static void *__CFAllocatorCustomMalloc(void *zone, size_t size) {
    return _CFAllocatorAllocateImpl(zone, (OCFIndex)size, 0x4ECD5AD1, 0);
}

// Native __CFAllocatorCustomCalloc, 0x1c0150e74. Non-external.
// CoreFoundation defect: the native function allocates and zeroes only size bytes and ignores count (X1), so
// a count above 1 under-allocates; confidence: verified (disassembly). The restoration allocates count * size
// bytes when count is above 1 (NULL when that overflows) and is unchanged for counts 0 and 1.
static void *__CFAllocatorCustomCalloc(void *zone, size_t count, size_t size) {
    size_t total = size;
    if (count > 1 && __builtin_mul_overflow(count, size, &total)) {
        return NULL;
    }
    void *ptr = _CFAllocatorAllocateImpl(zone, (OCFIndex)total, 0x83C93EDD, 0);
    if (ptr != NULL) {
        bzero(ptr, total);
    }
    return ptr;
}

// Native __CFAllocatorCustomValloc, 0x1c0150ebc. Non-external.
// CoreFoundation defect (kept): the native function returns the page-aligned address inside a block of
// size + vm_page_size, so freeing the result through the zone frees an interior pointer; confidence: verified
// (disassembly). A fix needs another block layout; the native behavior is kept and never exercised by tests.
static void *__CFAllocatorCustomValloc(void *zone, size_t size) {
    if (size >= ~((uintptr_t)vm_page_size << 1)) {
        return NULL;
    }
    uintptr_t block =
        (uintptr_t)_CFAllocatorAllocateImpl(zone, (OCFIndex)(vm_page_size + size), 0x3EB7D1D2, 0);
    uintptr_t page = vm_page_size;
    return (void *)((block + page - 1) & -page);
}

// Native __CFAllocatorCustomFree, 0x1c0150f20. Non-external; a tail call of CFAllocatorDeallocate.
static void __CFAllocatorCustomFree(void *zone, void *ptr) {
    OCFAllocatorDeallocate(zone, ptr);
}

// Native __CFAllocatorCustomRealloc, 0x1c0150f24. Non-external.
static void *__CFAllocatorCustomRealloc(void *zone, void *ptr, size_t size) {
    return CFAllocatorReallocateImpl(zone, ptr, (OCFIndex)size, 0x15311330, 0);
}

// Native __CFAllocatorNullDestroy, 0x1c0150f34. Non-external; the static allocators' destroy; no effect.
static void __CFAllocatorNullDestroy(void *zone) {
    (void)zone;
}

// Native __CFAllocatorZoneIntrospectNoOp, 0x1c0150f38. Non-external; six introspection slots of both tables;
// returns 0.
static uintptr_t __CFAllocatorZoneIntrospectNoOp(void) {
    return 0;
}

// Native __CFAllocatorCustomGoodSize, 0x1c0150f40. Non-external.
static size_t __CFAllocatorCustomGoodSize(void *zone, size_t size) {
    return (size_t)OCFAllocatorGetPreferredSizeForSize(zone, (OCFIndex)size, 0);
}

// Native __CFAllocatorZoneIntrospectTrue, 0x1c0150f48. Non-external; the introspection check.
static uintptr_t __CFAllocatorZoneIntrospectTrue(void) {
    return 1;
}

// Native __CFAllocatorNullSize (0x1c0150f50), Malloc (0x1c0150f58), Calloc (0x1c0150f60), Valloc
// (0x1c0150f68), Free (0x1c0150f70), Realloc (0x1c0150f74), Allocate (0x1c0150f7c), Reallocate (0x1c0150f84)
// and GoodSize (0x1c0150f8c). Non-external; kCFAllocatorNull's zone and context functions return 0 (GoodSize
// returns its size argument) and Free does nothing.
static size_t __CFAllocatorNullSize(void *zone, const void *ptr) {
    (void)zone;
    (void)ptr;
    return 0;
}

static void *__CFAllocatorNullMalloc(void *zone, size_t size) {
    (void)zone;
    (void)size;
    return NULL;
}

static void *__CFAllocatorNullCalloc(void *zone, size_t count, size_t size) {
    (void)zone;
    (void)count;
    (void)size;
    return NULL;
}

static void *__CFAllocatorNullValloc(void *zone, size_t size) {
    (void)zone;
    (void)size;
    return NULL;
}

static void __CFAllocatorNullFree(void *zone, void *ptr) {
    (void)zone;
    (void)ptr;
}

static void *__CFAllocatorNullRealloc(void *zone, void *ptr, size_t size) {
    (void)zone;
    (void)ptr;
    (void)size;
    return NULL;
}

static void *__CFAllocatorNullAllocate(OCFIndex size, OCFOptionFlags hint, void *info) {
    (void)size;
    (void)hint;
    (void)info;
    return NULL;
}

static void *__CFAllocatorNullReallocate(void *ptr, OCFIndex newsize, OCFOptionFlags hint, void *info) {
    (void)ptr;
    (void)newsize;
    (void)hint;
    (void)info;
    return NULL;
}

static size_t __CFAllocatorNullGoodSize(void *zone, size_t size) {
    (void)zone;
    return size;
}

// Native __CFAllocatorCustomDestroy, 0x1c0150f94. Non-external; a tail call of __CFAllocatorDeallocate.
static void __CFAllocatorCustomDestroy(void *zone) {
    __CFAllocatorDeallocate(zone);
}

// Native __CFReallocationFailed, 0x1c0305f80 (among the .cold entries). Hidden (was a private external);
// reached from the safe reallocation functions above. A failure callback that sets *stop (bit 0) handles the
// failure. Otherwise "Failed to grow buffer to %ld" is formatted into a zeroed 256-byte stack buffer and made
// an ASCII string of the system default allocator without copying (NULL contents deallocator), and
// _CFRaiseMemoryException raises with it, or with "Failed to grow buffer - malloc failure" when the string
// cannot be made. The native code follows the second raise with a release of the string, the crash message
// "Failed to grow buffer" and BRK #1; _CFRaiseMemoryException throws and never returns, so that tail is
// unreachable natively and is not reproduced after the noreturn call.
void __CFReallocationFailed(void *ptr, size_t size, ReallocationFailureBlock reallocationFailed) {
    bool stop = false;
    if (reallocationFailed != NULL) {
        reallocationFailed(ptr, &stop);
        if (stop) {
            return;
        }
    }
    char buffer[256];
    bzero(buffer, 256);
    int length = snprintf(buffer, 256, "Failed to grow buffer to %ld", (long)size);
    OCFStringRef reason = OCFStringCreateWithBytesNoCopy(
        &__kCFAllocatorSystemDefault, (const UInt8 *)buffer, length, kOCFStringEncodingASCII, false, NULL
    );
    if (reason == NULL) {
        _CFRaiseMemoryException(OF_STR("Failed to grow buffer - malloc failure"));
    }
    _CFRaiseMemoryException(reason);
}
