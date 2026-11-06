//
//  Created by ktiays on 2026/11/04.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Collections/ONSPointerFunctions.h"
#import "Foundation/Archiving/ONSObjCRuntimeNames.h"
#import "Foundation/String/ONSAllocateObject.h"
#import "Foundation/String/ONSLog.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/String/OCFString.h"

#include <dispatch/dispatch.h>
#include <mach/mach.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <os/log.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: one native unit spans 0x1c0595674-0x1c0598463 of the iOS Simulator Foundation
// __text section: the NSPointerArray methods, the NSConcretePointerArray methods, NSSliceInitWithSlice and
// NSSliceDealloc, the NSConcretePointerFunctions class and instance methods with the acquisition and
// personality functions between them, the coding helpers and the backing store's barrier functions; its cold
// pieces are raiseError (0x1c0fde5f0, linked as _raiseError_1), allocateInitialCapacity (0x1c0fde668, linked
// as _allocateInitialCapacity_1) and cStringHash (0x1c0fde6c4), which follow each other in the cold region
// and so belong to one translation unit. The NSHashTable and NSMapTable units have their own raiseError and
// allocateInitialCapacity. Only the methods and functions the captured groups reach are restored; the other
// methods of these classes lie outside the scope. Objective-C compiled with manual reference counting
// (objc_retain, objc_release; no ARC entry points). Evidence: Local/Evidence/cf-core link capture, round 3
// (scratch pc/ captures of the pointer collections). Unless a definition says otherwise: iOS Simulator
// 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in
// full). iOS Device 24A446: unread.
//
// Shared-cache bindings, identified from their use (confidence: medium): 0x22227FAE0 the NSException class
// (receiver of +exceptionWithName:reason:userInfo: and of objc_alloc before -initWithName:reason:userInfo:),
// 0x22227F4F8 NSInternalInconsistencyException, 0x22227F500 NSInvalidArgumentException, 0x22227F5D8
// NSMallocException, 0x22227FB18 the NSMutableArray class (+array, then -addObject:), 0x22241A5B0 libsystem's
// vm_page_size and 0x222419E80 mach_task_self_.
//
// Class metadata divergence: the native class_ro_t of NSConcretePointerFunctions and NSConcretePointerArray
// carries flags 0x10 (has weak ivars without ARC); the restored classes are compiled without weak ivars and
// carry flags 0 (confidence: verified for the native flags).

extern id objc_retain(id object);
extern void objc_release(id object);
extern id objc_autorelease(id object);
extern id objc_loadWeakRetained(id *location);

extern char pointerFunctionsStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));
extern char pointerFunctionsErrorClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSError));
extern char pointerFunctionsExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char pointerFunctionsMutableArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char pointerFunctionsKeyedUnarchiverClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSKeyedUnarchiver));

// The messages the unit sends to objects of other classes, declared for the sends only.
@protocol PointerFunctionsMessages
+ (id)stringWithFormat:(id)format, ...;
+ (id)stringWithUTF8String:(const char *)string;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)_readCorruptErrorWithFormat:(id)format, ...;
+ (id)array;
- (id)initWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
- (void)addObject:(id)object;
- (id)copy;
- (NSUInteger)hash;
- (BOOL)isEqual:(id)object;
- (id)description;
- (void)decodeValueOfObjCType:(const char *)type at:(void *)address size:(NSUInteger)size;
- (void)encodeValueOfObjCType:(const char *)type at:(const void *)address;
- (BOOL)_containsNextUnkeyedObject;
- (id)decodeObject;
- (void)encodeObject:(id)object;
- (void)encodeConditionalObject:(id)object;
- (void)failWithError:(id)error;
@end

// Native constant tables (__TEXT,__const and __DATA_CONST,__const; non-external), one per memory option,
// personality and backing store. The memory tables zeroingWeakMemory ... weakAutoreleasingMemory lie at
// 0x1c100dc70-0x1c100dccf, the others at 0x2227d61d8-0x2227d64c7; each is named
// initializeSlice:withOptions:.<name> natively (function-local statics of +initializeSlice:withOptions:).
static void *retainingAcquire(const void *item, NSUInteger (*size)(const void *item), BOOL copyIn);
static void releasingRelinquish(const void *item, NSUInteger (*size)(const void *item));
static void *mallocAcquire(const void *item, NSUInteger (*size)(const void *item), BOOL copyIn);
static void freeingRelinquish(const void *item, NSUInteger (*size)(const void *item));
static void *machAcquire(const void *item, NSUInteger (*size)(const void *item), BOOL copyIn);
static void machRelinquish(const void *item, NSUInteger (*size)(const void *item));
static NSUInteger objectHash(const void *item, NSUInteger (*size)(const void *item));
static BOOL objectEqual(const void *item1, const void *item2, NSUInteger (*size)(const void *item));
static id objectDescription(const void *item);
static NSUInteger pointerHash(const void *item, NSUInteger (*size)(const void *item));
static BOOL pointerEqual(const void *item1, const void *item2, NSUInteger (*size)(const void *item));
static id pointerDescription(const void *item);
static NSUInteger cStringSize(const void *item);
static BOOL valueEqual(const void *item1, const void *item2, NSUInteger (*size)(const void *item));
static id cStringDescription(const void *item);
static NSUInteger valueHash(const void *item, NSUInteger (*size)(const void *item));
static NSUInteger integerHash(const void *item, NSUInteger (*size)(const void *item));
static id integerDescription(const void *item);
static NSUInteger cStringHash(const void *item, NSUInteger (*size)(const void *item));
static void **allocateStorageWithSentinel(NSUInteger count);
static void freeWeakStorage(void **storage, NSUInteger count);
static void *startReadARCWeakAutoreleasingWithSentinelAt(void **slot, bool *isEmpty);
static void finishReadARCWeakAutoreleasingAt(void *item);
static void *continueReadARCWeakAutoreleasingAt(void *item);
static void clearARCWeakAtWithSentinel(void **slot);
static void storeARCWeakAt(void **storage, NSUInteger index, void *item);
static void *startReadARCWeakWithSentinelAt(void **slot, bool *isEmpty);
static void finishReadARCWeakAt(void *item);
static void *continueReadARCWeakAt(void *item);
static void **allocateStorage(NSUInteger count);
static void *startReadARCWeakAutoreleasingAt(void **slot, bool *isEmpty);
static void clearARCWeakAt(void **slot);
static void *startReadARCWeakAt(void **slot, bool *isEmpty);
static void freeStorage(void **storage, NSUInteger count);
static void *startReadPointerAt(void **slot, bool *isEmpty);
static void finishReadPointerAt(void *item);
static void *continueReadPointerAt(void *item);
static void clearPointerAt(void **slot);
static void storePointerAt(void **storage, NSUInteger index, void *item);

static const ONSSliceExternalAcquisitionProperties zeroingWeakMemory = {0, 1, 0, 0, 0, NULL, NULL};
static const ONSSliceExternalAcquisitionProperties opaqueMemory = {0, 0, 0, 0, 0, NULL, NULL};
static const ONSSliceExternalAcquisitionProperties weakNonautoreleasingMemory = {0, 1, 1, 0, 0, NULL, NULL};
static const ONSSliceExternalAcquisitionProperties weakAutoreleasingMemory = {0, 1, 1, 1, 0, NULL, NULL};
static const ONSSliceExternalAcquisitionProperties strongMemory_copyObjects = {
    0, 0, 0, 0, 1, retainingAcquire, releasingRelinquish
};
static const ONSSliceExternalAcquisitionProperties strongMemory_copyObjectPointers = {
    0, 0, 0, 0, 1, retainingAcquire, releasingRelinquish
};
static const ONSSliceExternalAcquisitionProperties strongMemory_objects = {
    0, 0, 0, 0, 0, retainingAcquire, releasingRelinquish
};
static const ONSSliceExternalAcquisitionProperties strongMemory_objectPointers = {
    0, 0, 0, 0, 0, retainingAcquire, releasingRelinquish
};
static const ONSSliceExternalAcquisitionProperties mallocMemory_copy = {
    0, 0, 0, 0, 1, mallocAcquire, freeingRelinquish
};
static const ONSSliceExternalAcquisitionProperties mallocMemory = {
    0, 0, 0, 0, 0, mallocAcquire, freeingRelinquish
};
static const ONSSliceExternalAcquisitionProperties machMemory_copy = {
    0, 0, 0, 0, 1, machAcquire, machRelinquish
};
static const ONSSliceExternalAcquisitionProperties machMemory = {0, 0, 0, 0, 0, machAcquire, machRelinquish};

static const ONSSliceExternalPersonalityProperties objectPers = {
    0, 0, NULL, objectHash, objectEqual, objectDescription
};
static const ONSSliceExternalPersonalityProperties objectPtrPers = {
    1, 0, NULL, pointerHash, pointerEqual, objectDescription
};
static const ONSSliceExternalPersonalityProperties opaquePers = {
    1, 0, NULL, pointerHash, pointerEqual, pointerDescription
};
static const ONSSliceExternalPersonalityProperties cStringPers = {
    0, 0, cStringSize, cStringHash, valueEqual, cStringDescription
};
static const ONSSliceExternalPersonalityProperties structPers = {0, 0, NULL, valueHash, valueEqual, NULL};
static const ONSSliceExternalPersonalityProperties intPers = {
    0, 1, NULL, integerHash, pointerEqual, integerDescription
};

// Native _weakBarriersSentinelAutoreleasing (0x2227d6388), _weakBarriersSentinel (0x2227d63c8),
// _weakBarriersAutoreleasing (0x2227d6408), _weakBarriers (0x2227d6448) and _noWriteBarriers (0x2227d6488).
static const ONSSliceInternalProperties weakBarriersSentinelAutoreleasing = {
    0,
    1,
    1,
    1,
    0,
    allocateStorageWithSentinel,
    freeWeakStorage,
    startReadARCWeakAutoreleasingWithSentinelAt,
    finishReadARCWeakAutoreleasingAt,
    continueReadARCWeakAutoreleasingAt,
    clearARCWeakAtWithSentinel,
    storeARCWeakAt,
};
static const ONSSliceInternalProperties weakBarriersSentinel = {
    0,
    1,
    1,
    1,
    0,
    allocateStorageWithSentinel,
    freeWeakStorage,
    startReadARCWeakWithSentinelAt,
    finishReadARCWeakAt,
    continueReadARCWeakAt,
    clearARCWeakAtWithSentinel,
    storeARCWeakAt,
};
static const ONSSliceInternalProperties weakBarriersAutoreleasing = {
    0,
    1,
    1,
    0,
    0,
    allocateStorage,
    freeWeakStorage,
    startReadARCWeakAutoreleasingAt,
    finishReadARCWeakAutoreleasingAt,
    continueReadARCWeakAutoreleasingAt,
    clearARCWeakAt,
    storeARCWeakAt,
};
static const ONSSliceInternalProperties weakBarriers = {
    0,
    1,
    1,
    0,
    0,
    allocateStorage,
    freeWeakStorage,
    startReadARCWeakAt,
    finishReadARCWeakAt,
    continueReadARCWeakAt,
    clearARCWeakAt,
    storeARCWeakAt,
};
static const ONSSliceInternalProperties noWriteBarriers = {
    0,
    0,
    0,
    0,
    1,
    allocateStorage,
    freeStorage,
    startReadPointerAt,
    finishReadPointerAt,
    continueReadPointerAt,
    clearPointerAt,
    storePointerAt,
};

static void raiseError(id object, SEL selector) __attribute__((noreturn));
static void allocateInitialCapacity(ONSSlice *slice, NSUInteger capacity);

// The format of the unsupported-configuration log (cfstr at the two NSLog calls).
#define UNSUPPORTED_CONFIGURATION_FORMAT                                                                     \
    "*** An unsupported PointerFunctions configuration was requested, probably for use by NSMapTable, "      \
    "NSHashTable, or NSPointerArray.  The requested configuration %s"

@implementation ONSPointerArray

// Native -[NSPointerArray init], 0x1c0595674. Non-external.
- (id)init {
    raiseError(self, _cmd);
}

// Native -[NSPointerArray initWithOptions:], 0x1c0595680. Non-external.
- (id)initWithOptions:(NSUInteger)options {
    raiseError(self, _cmd);
}

// Native -[NSPointerArray initWithPointerFunctions:], 0x1c059568c. Non-external.
- (id)initWithPointerFunctions:(id)functions {
    raiseError(self, _cmd);
}

// Native +[NSPointerArray allocWithZone:], 0x1c0595710. Non-external. The concrete class, whatever the
// receiver; the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return ONSAllocateObject([ONSConcretePointerArray self], 0, NULL);
}

// Native -[NSPointerArray initWithCoder:], 0x1c0595734. Non-external. The receiver is deallocated through
// the super send (not released) before a concrete array decodes.
- (id)initWithCoder:(id)coder {
    [super dealloc];
    return [[ONSConcretePointerArray alloc] initWithCoder:coder];
}

// Native -[NSPointerArray classForCoder], 0x1c05957bc. Non-external.
- (Class)classForCoder {
    return [ONSPointerArray self];
}

// Native +[NSPointerArray _subclassesMustBeExplicitlyMentionedWhenDecoded], 0x1c05957c8. Non-external.
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded {
    return YES;
}

// Native -[NSPointerArray pointerFunctions], 0x1c05957d0. Non-external.
- (id)pointerFunctions {
    raiseError(self, _cmd);
}

// Native -[NSPointerArray countByEnumeratingWithState:objects:count:], 0x1c05957dc. Non-external.
- (NSUInteger)countByEnumeratingWithState:(PointerCollectionEnumerationState *)state
                                  objects:(id *)buffer
                                    count:(NSUInteger)length {
    raiseError(self, _cmd);
}

// Native -[NSPointerArray addPointer:], 0x1c05957f4. Non-external.
- (void)addPointer:(void *)pointer {
    raiseError(self, _cmd);
}

// Native -[NSPointerArray count], 0x1c0595830. Non-external.
- (NSUInteger)count {
    raiseError(self, _cmd);
}

// Native -[NSPointerArray copyWithZone:], 0x1c0595848. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    raiseError(self, _cmd);
}

// Native -[NSPointerArray encodeWithCoder:], 0x1c0595854. Non-external.
- (void)encodeWithCoder:(id)coder {
    raiseError(self, _cmd);
}

// Native -[NSPointerArray(NSArrayConveniences) allObjects], 0x1c05958ec. Non-external. NULL pointers are
// skipped.
- (id)allObjects {
    id objects = [(id)pointerFunctionsMutableArrayClass array];
    for (id object in self) {
        if (object != nil) {
            [objects addObject:object];
        }
    }
    return objects;
}

@end

@implementation ONSConcretePointerArray

// Native -[NSConcretePointerArray init], 0x1c05959e8. Non-external.
- (id)init {
    return [self initWithOptions:0];
}

// Native -[NSConcretePointerArray initWithOptions:], 0x1c05959f0. Non-external. Count 0, capacity 4.
- (id)initWithOptions:(NSUInteger)arrayOptions {
    count = 0;
    capacity = 4;
    options = arrayOptions;
    mutations = 0;
    if (![ONSConcretePointerFunctions initializeSlice:&slice withOptions:arrayOptions]) {
        id reason = [(id)pointerFunctionsStringClass
            stringWithFormat:(id)OF_STR("*** -[NSPointerArray %@] Requested configuration not supported."),
                             ONSStringFromSelector(_cmd)];
        objc_release(self);
        objc_exception_throw([(id)pointerFunctionsExceptionClass
            exceptionWithName:(id)ONSInternalInconsistencyException
                       reason:reason
                     userInfo:nil]);
    }
    [ONSConcretePointerFunctions initializeBackingStore:&slice
                                               sentinel:NO
                                      weakAutoreleasing:slice.acquisitionProps->weakAutoreleasing
                                                dynamic:NO];
    allocateInitialCapacity(&slice, capacity);
    return self;
}

// Native -[NSConcretePointerArray initWithPointerFunctions:], 0x1c0595ac4. Non-external. Count 0, capacity
// 16, options NSUIntegerMax, a dynamic slice copied from the functions.
- (id)initWithPointerFunctions:(id)functions {
    if (functions == nil) {
        id reason = [(id)pointerFunctionsStringClass
            stringWithFormat:(id)OF_STR("*** -[NSPointerArray %@] Functions cannot be NULL."),
                             ONSStringFromSelector(_cmd)];
        objc_release(self);
        objc_exception_throw([(id)pointerFunctionsExceptionClass
            exceptionWithName:(id)ONSInternalInconsistencyException
                       reason:reason
                     userInfo:nil]);
    }
    NSSliceInitWithSlice(&slice, &((ONSConcretePointerFunctions *)functions)->slice);
    hasDynamicSlice = YES;
    count = 0;
    capacity = 16;
    mutations = 0;
    [ONSConcretePointerFunctions initializeBackingStore:&slice
                                               sentinel:NO
                                      weakAutoreleasing:slice.acquisitionProps->weakAutoreleasing
                                                dynamic:YES];
    options = (NSUInteger)-1;
    allocateInitialCapacity(&slice, capacity);
    return self;
}

// Native -[NSConcretePointerArray initWithCoder:], 0x1c0595ba8. Non-external. The options and the count are
// decoded as 32-bit integers; keyed archives must hold every element.
- (id)initWithCoder:(id)coder {
    int decodedOptions = 0;
    [coder decodeValueOfObjCType:"i" at:&decodedOptions size:4];
    if (!_NSPointerFunctionCoding_isValidOptions((NSUInteger)(NSInteger)decodedOptions)) {
        objc_release(self);
        [coder
            failWithError:[(id)pointerFunctionsErrorClass
                              _readCorruptErrorWithFormat:(id)OF_STR("Invalid NSPointerArray options for %d"),
                                                          decodedOptions]];
        return nil;
    }
    int decodedCount = 0;
    [coder decodeValueOfObjCType:"i" at:&decodedCount size:4];
    if (decodedCount < 0) {
        objc_release(self);
        [coder
            failWithError:[(id)pointerFunctionsErrorClass
                              _readCorruptErrorWithFormat:(id)OF_STR(
                                                              "NSPointerArray archive contains negative count"
                                                          )]];
        return nil;
    }
    id array = [self initWithOptions:(NSUInteger)(NSInteger)decodedOptions];
    if ([coder isKindOfClass:(Class)pointerFunctionsKeyedUnarchiverClass]) {
        if (decodedCount >= 1) {
            int index = 0;
            while ([coder _containsNextUnkeyedObject]) {
                [array addPointer:[coder decodeObject]];
                if (++index >= decodedCount) {
                    return array;
                }
            }
            id error = [(id)pointerFunctionsErrorClass
                _readCorruptErrorWithFormat:(id)OF_STR("Missing expected key while decoding NSPointerArray")];
            objc_release(array);
            [coder failWithError:error];
            return nil;
        }
    } else if (decodedCount >= 1) {
        for (int index = 0; index < decodedCount; ++index) {
            [array addPointer:[coder decodeObject]];
        }
    }
    return array;
}

// Native -[NSConcretePointerArray dealloc], 0x1c0595dfc. Non-external. Relinquishes and clears every slot
// when the store has read and clear functions, frees the store, then the dynamic slice.
- (void)dealloc {
    ONSSliceInternalProperties *internal = slice.internalProps;
    if (internal != NULL) {
        NSUInteger storeCapacity = capacity;
        if (internal->startRead != NULL && storeCapacity != 0 && internal->clear != NULL) {
            NSUInteger remaining = storeCapacity;
            NSUInteger offset = 0;
            do {
                void *item = slice.internalProps->startRead((void **)((char *)slice.items + offset), NULL);
                if (item != NULL) {
                    void (*relinquish)(const void *, NSUInteger (*)(const void *)) =
                        slice.acquisitionProps->relinquish;
                    if (relinquish != NULL) {
                        relinquish(item, slice.personalityProps->size);
                    }
                }
                slice.internalProps->clear((void **)((char *)slice.items + offset));
                slice.internalProps->finishRead(item);
                offset += 8;
                --remaining;
            } while (remaining != 0);
            internal = slice.internalProps;
        }
        if (internal->free != NULL) {
            internal->free(slice.items, storeCapacity);
        }
        slice.items = NULL;
    }
    if (hasDynamicSlice == YES) {
        NSSliceDealloc(&slice);
    }
    [super dealloc];
}

// Native -[NSConcretePointerArray hash], 0x1c0595f40. Non-external. The count, or the receiver itself for
// weak storage.
- (NSUInteger)hash {
    if (slice.internalProps->isWeak != 1) {
        return count;
    }
    return (NSUInteger)self;
}

// Native -[NSConcretePointerArray isEqual:], 0x1c0595f58. Non-external. Same class, count, isEqual and size
// functions, then slot by slot: a slot holding an item on one side only is unequal (the read item is
// finished first).
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    if ([object class] != [self class]) {
        return NO;
    }
    ONSConcretePointerArray *other = object;
    if (other->count != count) {
        return NO;
    }
    if (slice.personalityProps->isEqual != other->slice.personalityProps->isEqual ||
        slice.personalityProps->size != other->slice.personalityProps->size) {
        return NO;
    }
    if (other->count == 0) {
        return YES;
    }
    NSUInteger offset = 0;
    NSUInteger index = 0;
    ONSSliceInternalProperties *const *owner = &slice.internalProps;
    void *item;
    while (1) {
        item = slice.internalProps->startRead((void **)((char *)slice.items + offset), NULL);
        void *otherItem =
            other->slice.internalProps->startRead((void **)((char *)other->slice.items + offset), NULL);
        if (item != NULL && otherItem == NULL) {
            break;
        }
        if (otherItem != NULL && item == NULL) {
            owner = &other->slice.internalProps;
            item = otherItem;
            break;
        }
        if (((uintptr_t)item | (uintptr_t)otherItem) != 0) {
            BOOL equal = slice.personalityProps->isEqual(item, otherItem, slice.personalityProps->size);
            slice.internalProps->finishRead(item);
            other->slice.internalProps->finishRead(otherItem);
            if (!equal) {
                return NO;
            }
        }
        ++index;
        offset += 8;
        if (index >= count) {
            return YES;
        }
    }
    (*owner)->finishRead(item);
    return NO;
}

// Native -[NSConcretePointerArray encodeWithCoder:], 0x1c05960e8. Non-external. A fault is logged once
// (the global block ___42-[NSConcretePointerArray encodeWithCoder:]_block_invoke, 0x1c0596350, under the
// token encodeWithCoder:.onceToken). The options and the count are encoded as 32-bit integers, then the
// elements (conditionally for weak memory) and a nil terminator. The modification check precedes the store,
// so an enumeration yielding one element more than the count writes the buffer's slot count before the
// exception (Foundation defect: none established; the enumeration of the restored class yields exactly
// count elements unless it raises a mutation).
- (void)encodeWithCoder:(id)coder {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      os_log_t log = _NSOSLog();
      os_log_fault(log, "`NSPointerArray` should not be used in archives.");
    });
    NSUInteger encodedOptions = options;
    if (!_NSPointerFunctionCoding_isValidOptions(encodedOptions)) {
        objc_exception_throw([(id)pointerFunctionsExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:(id)OF_STR("NSHashTable has invalid options for encoding")
                     userInfo:nil]);
    }
    NSUInteger expected = count;
    void **buffer = _NSPointerFunctionCoding_bufferForCount(expected);
    if (buffer == NULL) {
        objc_exception_throw([(id)pointerFunctionsExceptionClass
            exceptionWithName:(id)ONSMallocException
                       reason:(id)OF_STR("NSPointerArray is too big to encode")
                     userInfo:nil]);
    }
    NSUInteger encodedCount = 0;
    for (id object in self) {
        if (encodedCount > expected) {
            free(buffer);
            objc_exception_throw([(id)pointerFunctionsExceptionClass
                exceptionWithName:(id)ONSInvalidArgumentException
                           reason:(id)OF_STR("NSPointerArray was modified while encoding")
                         userInfo:nil]);
        }
        buffer[encodedCount++] = object;
    }
    NSUInteger memory = encodedOptions & 7;
    bool incomplete = memory != 5;
    if (encodedCount >= expected) {
        incomplete = false;
    }
    if (encodedCount > expected || incomplete) {
        free(buffer);
        objc_exception_throw([(id)pointerFunctionsExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:(id)OF_STR("NSPointerArray was modified while encoding")
                     userInfo:nil]);
    }
    [coder encodeValueOfObjCType:"i" at:&options];
    [coder encodeValueOfObjCType:"i" at:&encodedCount];
    for (NSUInteger index = 0; index < encodedCount; ++index) {
        if (memory == 5) {
            [coder encodeConditionalObject:(id)buffer[index]];
        } else {
            [coder encodeObject:(id)buffer[index]];
        }
    }
    [coder encodeObject:nil];
    free(buffer);
}

// Native -[NSConcretePointerArray countByEnumeratingWithState:objects:count:], 0x1c05963b0. Non-external.
// Strong storage is handed out in one batch (the store itself, count items); weak storage one read at a time
// through the state's extra[0], finishing the previous read first.
- (NSUInteger)countByEnumeratingWithState:(PointerCollectionEnumerationState *)state
                                  objects:(id *)buffer
                                    count:(NSUInteger)length {
    NSUInteger index;
    if (state->state != 0) {
        slice.internalProps->finishRead((void *)state->extra[0]);
        index = state->state;
    } else {
        state->mutationsPtr = (unsigned long *)&mutations;
        if ((slice.internalProps->isWeak & 1) == 0) {
            void **items = slice.items;
            state->state = count;
            state->itemsPtr = (id *)items;
            return count;
        }
        index = 0;
        state->itemsPtr = (id *)&state->extra[0];
    }
    if (index >= count) {
        return 0;
    }
    void *item = slice.internalProps->startRead(&slice.items[index], NULL);
    ++state->state;
    state->extra[0] = (unsigned long)item;
    return 1;
}

// Native -[NSConcretePointerArray pointerFunctions], 0x1c0596464. Non-external. A new functions object
// over a copy of the slice; no initializer runs.
- (id)pointerFunctions {
    ONSConcretePointerFunctions *functions = [ONSConcretePointerFunctions alloc];
    NSSliceInitWithSlice(&functions->slice, &slice);
    return objc_autorelease(functions);
}

// Native -[NSConcretePointerArray arrayGrow:], 0x1c05964a8. Non-external. Moves the readable items of the old
// store to the same indexes of a new one, frees the old store with the old capacity, then records the new
// capacity.
- (void)arrayGrow:(NSUInteger)newCapacity {
    void **oldItems = slice.items;
    slice.items = slice.internalProps->allocate(newCapacity);
    if (slice.items == NULL) {
        objc_exception_throw([(id)pointerFunctionsExceptionClass
            exceptionWithName:(id)ONSMallocException
                       reason:(id)OF_STR(
                                  "*** NSPointerArray unable to allocate memory -- allocation function failed"
                              )
                     userInfo:nil]);
    }
    NSUInteger oldCapacity;
    if (capacity != 0) {
        NSUInteger index = 0;
        void **slot = oldItems;
        do {
            if (*slot != NULL) {
                void *item = slice.internalProps->startRead(slot, NULL);
                if (item != NULL) {
                    slice.internalProps->store(slice.items, index, item);
                    slice.internalProps->finishRead(item);
                }
            }
            ++index;
            oldCapacity = capacity;
            ++slot;
        } while (index < oldCapacity);
    } else {
        oldCapacity = 0;
    }
    slice.internalProps->free(oldItems, oldCapacity);
    capacity = newCapacity;
}

// Native -[NSConcretePointerArray addPointer:], 0x1c0596688. Non-external. Doubles a full store, acquires a
// non-NULL pointer, stores it at the end; a NULL pointer marks the array for compaction.
- (void)addPointer:(void *)pointer {
    if (count == capacity) {
        [self arrayGrow:2 * count];
    }
    if (pointer != NULL) {
        ONSSliceExternalAcquisitionProperties *acquisition = slice.acquisitionProps;
        void *(*acquire)(const void *, NSUInteger (*)(const void *), BOOL) = acquisition->acquire;
        if (acquire != NULL) {
            pointer = acquire(pointer, slice.personalityProps->size, acquisition->copyIn);
        }
    }
    slice.internalProps->store(slice.items, count, pointer);
    if (pointer == NULL) {
        needsCompaction = YES;
    }
    ++count;
    ++mutations;
}

// Native -[NSConcretePointerArray count], 0x1c0596cbc. Non-external.
- (NSUInteger)count {
    return count;
}

// Native -[NSConcretePointerArray copyWithZone:], 0x1c0596f08. Non-external. A new array over the receiver's
// pointer functions, filled slot by slot (NULL slots included).
- (id)copyWithZone:(struct _NSZone *)zone {
    ONSConcretePointerArray *copy =
        [[ONSConcretePointerArray allocWithZone:zone] initWithPointerFunctions:[self pointerFunctions]];
    NSUInteger remaining = count;
    if (remaining != 0) {
        NSUInteger offset = 0;
        do {
            void *item = slice.internalProps->startRead((void **)((char *)slice.items + offset), NULL);
            [copy addPointer:item];
            slice.internalProps->finishRead(item);
            offset += 8;
            --remaining;
        } while (remaining != 0);
    }
    return copy;
}

@end

// Native NSSliceInitWithSlice, 0x1c0597508. Hidden (was a private external).
void NSSliceInitWithSlice(ONSSlice *slice, const ONSSlice *source) {
    slice->acquisitionProps = malloc_type_malloc(0x18, 0x1080040B03C7EFD);
    *slice->acquisitionProps = *source->acquisitionProps;
    slice->personalityProps = malloc_type_malloc(0x28, 0x10800408D251DEB);
    *slice->personalityProps = *source->personalityProps;
    if (source->internalProps != NULL) {
        slice->internalProps = malloc_type_malloc(0x40, 0x1080040E27AD052);
        *slice->internalProps = *source->internalProps;
    }
    slice->items = NULL;
}

// Native NSSliceDealloc, 0x1c05975c4. Hidden (was a private external).
void NSSliceDealloc(ONSSlice *slice) {
    free(slice->acquisitionProps);
    free(slice->personalityProps);
    free(slice->internalProps);
}

@implementation ONSPointerFunctions
@end

@implementation ONSConcretePointerFunctions

// Native +[NSConcretePointerFunctions initializeBackingStore:sentinel:weakAutoreleasing:dynamic:],
// 0x1c05975f4. Non-external. Weak memory stored through the ARC weak functions gets weak barriers (with the
// sentinel and autoreleasing reads as asked); everything else the plain store. A dynamic slice receives a
// malloc'd copy, replacing (and freeing) its previous one.
+ (void)initializeBackingStore:(ONSSlice *)slice
                      sentinel:(BOOL)sentinel
             weakAutoreleasing:(BOOL)weakAutoreleasing
                       dynamic:(BOOL)dynamic {
    const ONSSliceInternalProperties *internal;
    if (slice->acquisitionProps->isWeak == 1) {
        const ONSSliceInternalProperties *weak =
            weakAutoreleasing ? &weakBarriersAutoreleasing : &weakBarriers;
        const ONSSliceInternalProperties *weakSentinel =
            weakAutoreleasing ? &weakBarriersSentinelAutoreleasing : &weakBarriersSentinel;
        if (sentinel) {
            weak = weakSentinel;
        }
        internal = slice->acquisitionProps->usesARCWeak == 1 ? weak : &noWriteBarriers;
    } else {
        internal = &noWriteBarriers;
    }
    if (!dynamic) {
        slice->internalProps = (ONSSliceInternalProperties *)internal;
        return;
    }
    ONSSliceInternalProperties *copy = malloc_type_malloc(0x40, 0x1080040E27AD052);
    *copy = *internal;
    if (slice->internalProps != NULL) {
        free(slice->internalProps);
    }
    slice->internalProps = copy;
}

// Native +[NSConcretePointerFunctions initializeSlice:withOptions:], 0x1c05976c4. Non-external. Clears items,
// rejects unsupported combinations for executables linked on or after version 6 (logging which), selects the
// memory table from the low byte and the personality table from the second byte; NO when either byte has no
// table (the memory table is stored before the personality is checked).
+ (BOOL)initializeSlice:(ONSSlice *)slice withOptions:(NSUInteger)options {
    NSUInteger personality = options & 0xFF00;
    slice->items = NULL;
    NSUInteger copyIn = options & 0xFFFFFFFFFFFF0000;
    uint8_t memory = (uint8_t)options;
    const char *failure;
    if (_OCFExecutableLinkedOnOrAfter(6)) {
        if (copyIn == 0x10000 && (personality == 0x100 || (memory <= 5 && ((1 << memory) & 0x26) != 0))) {
            failure = "fails due to copyin with opaque personality or opaque or weak memory";
            goto unsupported;
        }
        if ((options & 0xFD00) == 0 && (memory > 6 || ((1 << memory) & 0x67) == 0)) {
            failure = "fails due to objects with none of strong, weak, or opaque memory\n";
            goto unsupported;
        }
        if (personality == 0x500 && memory != 2) {
            failure = "fails due to integer personality not using opaque memory\n";
            goto unsupported;
        }
    }
    const ONSSliceExternalAcquisitionProperties *acquisition;
    switch (memory) {
        case 0:
            if (copyIn == 0x10000) {
                if (personality == 0x200) {
                    acquisition = &strongMemory_copyObjectPointers;
                } else if (personality != 0) {
                    failure = "fails due to strong copyin not used with objects\n";
                    goto unsupported;
                } else {
                    acquisition = &strongMemory_copyObjects;
                }
            } else if (personality == 0x200) {
                acquisition = &strongMemory_objectPointers;
            } else if (personality == 0) {
                acquisition = &strongMemory_objects;
            } else {
                acquisition = &opaqueMemory;
            }
            break;
        case 1:
            acquisition = &zeroingWeakMemory;
            break;
        case 2:
            acquisition = &opaqueMemory;
            break;
        case 3:
            acquisition = copyIn == 0x10000 ? &mallocMemory_copy : &mallocMemory;
            break;
        case 4:
            acquisition = copyIn == 0x10000 ? &machMemory_copy : &machMemory;
            break;
        case 5:
            acquisition = &weakAutoreleasingMemory;
            break;
        case 6:
            acquisition = &weakNonautoreleasingMemory;
            break;
        default:
            return NO;
    }
    slice->acquisitionProps = (ONSSliceExternalAcquisitionProperties *)acquisition;
    const ONSSliceExternalPersonalityProperties *personalityProps;
    switch ((uint8_t)(options >> 8)) {
        case 0:
        case 2:
            personalityProps = personality != 0 ? &objectPtrPers : &objectPers;
            break;
        case 1:
            personalityProps = &opaquePers;
            break;
        case 3:
            personalityProps = &cStringPers;
            break;
        case 4:
            personalityProps = &structPers;
            break;
        case 5:
            personalityProps = &intPers;
            break;
        default:
            return NO;
    }
    slice->personalityProps = (ONSSliceExternalPersonalityProperties *)personalityProps;
    return YES;

unsupported:
    ONSLog((id)OF_STR(UNSUPPORTED_CONFIGURATION_FORMAT), failure);
    return NO;
}

// Native retainingAcquire, 0x1c0597948. Non-external.
static void *retainingAcquire(const void *item, NSUInteger (*size)(const void *item), BOOL copyIn) {
    (void)size;
    if (copyIn) {
        return [(id)item copy];
    }
    return objc_retain((id)item);
}

// Native releasingRelinquish, 0x1c0597954. Non-external.
static void releasingRelinquish(const void *item, NSUInteger (*size)(const void *item)) {
    (void)size;
    objc_release((id)item);
}

// Native mallocAcquire, 0x1c0597958. Non-external. A copy-in duplicates size(item) bytes into zeroed memory.
static void *mallocAcquire(const void *item, NSUInteger (*size)(const void *item), BOOL copyIn) {
    if (copyIn) {
        NSUInteger length = size(item);
        void *copy = malloc_type_calloc(1, length, 0xCD470835);
        return memmove(copy, item, length);
    }
    return (void *)item;
}

// Native freeingRelinquish, 0x1c05979b0. Non-external.
static void freeingRelinquish(const void *item, NSUInteger (*size)(const void *item)) {
    (void)size;
    free((void *)item);
}

// Native machAcquire, 0x1c05979b4. Non-external. A copy-in allocates whole pages and vm_copies the item; NULL
// when either call fails.
static void *machAcquire(const void *item, NSUInteger (*size)(const void *item), BOOL copyIn) {
    if (!copyIn) {
        return (void *)item;
    }
    vm_address_t destination = 0;
    NSUInteger length = size(item);
    NSUInteger remainder = length % vm_page_size;
    NSUInteger padding = remainder != 0 ? vm_page_size - remainder : 0;
    if (vm_allocate(mach_task_self_, &destination, padding + length, VM_FLAGS_ANYWHERE) != KERN_SUCCESS) {
        return NULL;
    }
    if (vm_copy(mach_task_self_, (vm_address_t)item, padding + length, destination) != KERN_SUCCESS) {
        vm_deallocate(mach_task_self_, destination, padding + length);
        return NULL;
    }
    return (void *)destination;
}

// Native machRelinquish, 0x1c0597aa0. Non-external.
static void machRelinquish(const void *item, NSUInteger (*size)(const void *item)) {
    NSUInteger length = size(item);
    NSUInteger remainder = length % vm_page_size;
    NSUInteger padding = remainder != 0 ? vm_page_size - remainder : 0;
    vm_deallocate(mach_task_self_, (vm_address_t)item, padding + length);
}

// Native objectHash, 0x1c0597af8. Non-external.
static NSUInteger objectHash(const void *item, NSUInteger (*size)(const void *item)) {
    (void)size;
    return [(id)item hash];
}

// Native objectEqual, 0x1c0597afc. Non-external.
static BOOL objectEqual(const void *item1, const void *item2, NSUInteger (*size)(const void *item)) {
    (void)size;
    if (item1 == item2) {
        return YES;
    }
    return [(id)item1 isEqual:(id)item2];
}

// Native objectDescription, 0x1c0597b14. Non-external.
static id objectDescription(const void *item) {
    return [(id)item description];
}

// Native pointerHash, 0x1c0597b18. Non-external.
static NSUInteger pointerHash(const void *item, NSUInteger (*size)(const void *item)) {
    (void)size;
    return (uintptr_t)item >> 3;
}

// Native pointerEqual, 0x1c0597b20. Non-external.
static BOOL pointerEqual(const void *item1, const void *item2, NSUInteger (*size)(const void *item)) {
    (void)size;
    return item1 == item2;
}

// Native pointerDescription, 0x1c0597b2c. Non-external.
static id pointerDescription(const void *item) {
    return [(id)pointerFunctionsStringClass stringWithFormat:(id)OF_STR("%p"), item];
}

// Native cStringSize, 0x1c0597b60. Non-external. The length with its terminator.
static NSUInteger cStringSize(const void *item) {
    return strlen(item) + 1;
}

// Native valueEqual, 0x1c0597b78. Non-external.
static BOOL valueEqual(const void *item1, const void *item2, NSUInteger (*size)(const void *item)) {
    NSUInteger length = size(item1);
    return length == size(item2) && memcmp(item1, item2, length) == 0;
}

// Native cStringDescription, 0x1c0597bdc. Non-external.
static id cStringDescription(const void *item) {
    return [(id)pointerFunctionsStringClass stringWithUTF8String:item];
}

// Native valueHash, 0x1c0597bec. Non-external. The size plus 17 times each (signed) byte.
static NSUInteger valueHash(const void *item, NSUInteger (*size)(const void *item)) {
    NSUInteger hash = size(item);
    if (hash != 0) {
        const char *byte = item;
        const char *end = byte + hash;
        do {
            hash += 17 * (NSInteger)*byte++;
        } while (byte < end);
    }
    return hash;
}

// Native integerHash, 0x1c0597c28. Non-external. The item itself.
static NSUInteger integerHash(const void *item, NSUInteger (*size)(const void *item)) {
    (void)size;
    return (NSUInteger)item;
}

// Native integerDescription, 0x1c0597c2c. Non-external.
static id integerDescription(const void *item) {
    return [(id)pointerFunctionsStringClass stringWithFormat:(id)OF_STR("%ld"), item];
}

// Native -[NSConcretePointerFunctions init], 0x1c0597c60. Non-external.
- (id)init {
    return [self initWithOptions:0];
}

// Native -[NSConcretePointerFunctions initWithOptions:], 0x1c0597c68. Non-external. An unsupported
// configuration deallocates the receiver for executables linked on or after version 6; older executables
// copy the partially initialized slice.
- (id)initWithOptions:(NSUInteger)options {
    ONSSlice proposed = {NULL, NULL, NULL, NULL};
    if ([[self class] initializeSlice:&proposed withOptions:options] || !_OCFExecutableLinkedOnOrAfter(6)) {
        NSSliceInitWithSlice(&slice, &proposed);
        return self;
    }
    ONSLog(
        (id)OF_STR(UNSUPPORTED_CONFIGURATION_FORMAT),
        "would create only a partially initialized NSPointerFunctions object"
    );
    ONSDeallocateObject(self);
    return nil;
}

// Native -[NSConcretePointerFunctions dealloc], 0x1c0597d24. Non-external.
- (void)dealloc {
    NSSliceDealloc(&slice);
    [super dealloc];
}

// Native -[NSConcretePointerFunctions copyWithZone:], 0x1c0597da0. Non-external. The zone is not read; no
// initializer runs on the copy.
- (id)copyWithZone:(struct _NSZone *)zone {
    ONSConcretePointerFunctions *copy = [ONSConcretePointerFunctions alloc];
    NSSliceInitWithSlice(&copy->slice, &slice);
    return copy;
}

// Native -[NSConcretePointerFunctions hash], 0x1c0597de4. Non-external. The receiver itself.
- (NSUInteger)hash {
    return (NSUInteger)self;
}

// Native -[NSConcretePointerFunctions isEqual:], 0x1c0597de8. Non-external. Identity.
- (BOOL)isEqual:(id)object {
    return self == object;
}

@end

// Native __NSPointerFunctionCoding_isValidOptions, 0x1c05980bc. Hidden (was a private external). The memory
// (the low three bits) must be strong (0) or weak (5); exactly one of the six personalities (bits 8-10, at
// most 0x500) is present, and it must be the object (0) or object pointer (0x200) personality. The native
// code counts the personality matches with a vector compare.
BOOL _NSPointerFunctionCoding_isValidOptions(NSUInteger options) {
    NSUInteger memory = options & 7;
    BOOL validMemory = memory <= 5 && (memory == 5 || memory == 0);
    NSUInteger personality = options & 0x700;
    if (personality > 0x500) {
        return NO;
    }
    int matches = (personality == 0) + (personality == 0x200) + (personality == 0x100) +
                  (personality == 0x300) + (personality == 0x400) + (personality == 0x500);
    if (matches != 1) {
        return NO;
    }
    BOOL validPersonality = personality == 0 || personality == 0x200;
    return validMemory && validPersonality;
}

// Native __NSPointerFunctionCoding_bufferForCount, 0x1c0598178. Hidden (was a private external). NULL when
// count pointers overflow, the allocation fails, or the buffer address plus count wraps.
void **_NSPointerFunctionCoding_bufferForCount(NSUInteger count) {
    if (count >> 61 != 0) {
        return NULL;
    }
    void **buffer = malloc_type_malloc(8 * count, 0x665F25F);
    if (buffer != NULL && (uintptr_t)buffer + count < (uintptr_t)buffer) {
        free(buffer);
        return NULL;
    }
    return buffer;
}

// Native allocateStorageWithSentinel, 0x1c05981c8. Non-external. Every slot starts as the sentinel 1.
static void **allocateStorageWithSentinel(NSUInteger count) {
    if (count >> 61 != 0) {
        return NULL;
    }
    void **storage = malloc_type_malloc(8 * count, 0x80040B8603338);
    if (storage != NULL && count != 0) {
        for (NSUInteger index = 0; index != count; ++index) {
            storage[index] = (void *)1;
        }
    }
    return storage;
}

// Native freeWeakStorage, 0x1c059822c. Non-external. Unregisters every weak slot (neither empty nor the
// sentinel), then frees the storage.
static void freeWeakStorage(void **storage, NSUInteger count) {
    if (count != 0) {
        NSUInteger remaining = count;
        void **slot = storage;
        do {
            if ((uintptr_t)*slot >= 2) {
                objc_storeWeak((id *)slot, nil);
            }
            ++slot;
            --remaining;
        } while (remaining != 0);
    }
    free(storage);
}

// Native startReadARCWeakAutoreleasingWithSentinelAt, 0x1c0598284. Non-external. Empty means never used (the
// sentinel); a cleared slot reads nil without being empty.
static void *startReadARCWeakAutoreleasingWithSentinelAt(void **slot, bool *isEmpty) {
    void *raw = *slot;
    if (isEmpty != NULL) {
        *isEmpty = raw == (void *)1;
    }
    if ((uintptr_t)raw >= 2) {
        return objc_loadWeak((id *)slot);
    }
    return NULL;
}

// Native finishReadARCWeakAutoreleasingAt, 0x1c05982ac. Non-external.
static void finishReadARCWeakAutoreleasingAt(void *item) {
    (void)item;
}

// Native continueReadARCWeakAutoreleasingAt, 0x1c05982b0. Non-external.
static void *continueReadARCWeakAutoreleasingAt(void *item) {
    return item;
}

// Native clearARCWeakAtWithSentinel, 0x1c05982b4. Non-external. A cleared slot becomes the sentinel.
static void clearARCWeakAtWithSentinel(void **slot) {
    if ((uintptr_t)*slot >= 2) {
        objc_storeWeak((id *)slot, nil);
    }
    *slot = (void *)1;
}

// Native storeARCWeakAt, 0x1c05982f0. Non-external. The sentinel is replaced by nil before the weak store.
static void storeARCWeakAt(void **storage, NSUInteger index, void *item) {
    void **slot = &storage[index];
    if (*slot == (void *)1) {
        *slot = NULL;
    }
    objc_storeWeak((id *)slot, (id)item);
}

// Native startReadARCWeakWithSentinelAt, 0x1c059830c. Non-external. The item is returned retained.
static void *startReadARCWeakWithSentinelAt(void **slot, bool *isEmpty) {
    void *raw = *slot;
    if (isEmpty != NULL) {
        *isEmpty = raw == (void *)1;
    }
    if ((uintptr_t)raw >= 2) {
        return objc_loadWeakRetained((id *)slot);
    }
    return NULL;
}

// Native finishReadARCWeakAt, 0x1c0598334. Non-external.
static void finishReadARCWeakAt(void *item) {
    if (item != NULL) {
        objc_release((id)item);
    }
}

// Native continueReadARCWeakAt, 0x1c0598340. Non-external.
static void *continueReadARCWeakAt(void *item) {
    if (item != NULL) {
        return objc_autorelease((id)item);
    }
    return item;
}

// Native allocateStorage, 0x1c059834c. Non-external.
static void **allocateStorage(NSUInteger count) {
    if (count >> 61 != 0) {
        return NULL;
    }
    return malloc_type_calloc(count, 8, 0x80040B8603338);
}

// Native startReadARCWeakAutoreleasingAt, 0x1c0598374. Non-external. Empty means the read gave nil.
static void *startReadARCWeakAutoreleasingAt(void **slot, bool *isEmpty) {
    void *item = NULL;
    if (*slot != NULL) {
        item = objc_loadWeak((id *)slot);
    }
    if (isEmpty != NULL) {
        *isEmpty = item == NULL;
    }
    return item;
}

// Native clearARCWeakAt, 0x1c05983b8. Non-external.
static void clearARCWeakAt(void **slot) {
    if (*slot != NULL) {
        objc_storeWeak((id *)slot, nil);
    }
    *slot = NULL;
}

// Native startReadARCWeakAt, 0x1c05983ec. Non-external. The item is returned retained.
static void *startReadARCWeakAt(void **slot, bool *isEmpty) {
    void *item = NULL;
    if (*slot != NULL) {
        item = objc_loadWeakRetained((id *)slot);
    }
    if (isEmpty != NULL) {
        *isEmpty = item == NULL;
    }
    return item;
}

// Native freeStorage, 0x1c0598430. Non-external.
static void freeStorage(void **storage, NSUInteger count) {
    (void)count;
    free(storage);
}

// Native startReadPointerAt, 0x1c0598434. Non-external.
static void *startReadPointerAt(void **slot, bool *isEmpty) {
    void *item = *slot;
    if (isEmpty != NULL) {
        *isEmpty = item == NULL;
    }
    return item;
}

// Native finishReadPointerAt, 0x1c059844c. Non-external.
static void finishReadPointerAt(void *item) {
    (void)item;
}

// Native continueReadPointerAt, 0x1c0598450. Non-external.
static void *continueReadPointerAt(void *item) {
    return item;
}

// Native clearPointerAt, 0x1c0598454. Non-external.
static void clearPointerAt(void **slot) {
    *slot = NULL;
}

// Native storePointerAt, 0x1c059845c. Non-external.
static void storePointerAt(void **storage, NSUInteger index, void *item) {
    storage[index] = item;
}

// Native raiseError, 0x1c0fde5f0 (cold region, linked as _raiseError_1). Non-external. The abstract method
// exception of this unit's abstract classes.
static void raiseError(id object, SEL selector) {
    id exception = [(id)pointerFunctionsExceptionClass alloc];
    const char *name = sel_getName(selector);
    id reason = [(id)pointerFunctionsStringClass
        stringWithFormat:(id)OF_STR("abstract method %s called directly on class %@"), name, [object class]];
    objc_exception_throw([exception initWithName:(id)OF_STR("NSPointerFunctionsAbstractImplementationError")
                                          reason:reason
                                        userInfo:nil]);
}

// Native allocateInitialCapacity, 0x1c0fde668 (cold region, linked as _allocateInitialCapacity_1).
// Non-external.
static void allocateInitialCapacity(ONSSlice *slice, NSUInteger capacity) {
    slice->items = slice->internalProps->allocate(capacity);
    if (slice->items == NULL) {
        objc_exception_throw([(id)pointerFunctionsExceptionClass
            exceptionWithName:(id)ONSMallocException
                       reason:(id)OF_STR(
                                  "*** NSPointerArray unable to allocate memory -- allocation function failed"
                              )
                     userInfo:nil]);
    }
}

// Native cStringHash, 0x1c0fde6c4 (cold region). Non-external. CFStringHashISOLatin1CString over the C
// string's size (its terminator included).
static NSUInteger cStringHash(const void *item, NSUInteger (*size)(const void *item)) {
    NSUInteger length = size(item);
    return OCFStringHashISOLatin1CString(item, (OCFIndex)length);
}
