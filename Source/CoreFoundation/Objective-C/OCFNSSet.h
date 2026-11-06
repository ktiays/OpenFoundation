//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/Objective-C/OCFRequireConcreteImplementation.h"
#include "CoreFoundation/Objective-C/OCFNSCollectionsShouldCopy.h"

#include <mach-o/loader.h>
#include <objc/objc-exception.h>
#include <objc/objc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __OBJC__
#import <objc/NSObject.h>
#endif

// Shared declarations of the NSSet and NSMutableSet units (Objective-C/OCFNSSet.m and
// Objective-C/OCFNSMutableSet.m), iOS Simulator 24A434, arm64, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified unless a declaration says otherwise. iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).
//
// Objective-C/OCFBridgedClasses.h includes this header (through Objective-C/OCFNSMutableSet.h) for the
// superclasses of the bridged sets, so this header does not include it; setTSANRead and setTSANWrite spell
// that header's inline ThreadSanitizer checks locally for that reason.

// Classes of this image the set units reference by name (class symbols and runtime names). __NSSetI,
// __NSSetM, __NSSingleObjectSetI and __NSPlaceholderSet are not restored in this batch; their class symbols
// and ivar offsets stay undefined.
#ifdef __OBJC__
@class __ONSSetI;
@class __ONSSetM;
@class __ONSSingleObjectSetI;
@class __ONSPlaceholderSet;
@class ONSOrderedSet;
#endif

// Interfaces of families not yet restored on this branch.

// Native __CFRequireConcreteImplementation, 0x1c030fa8c (was a private external), beside the other
// Objective-C runtime glue: reports that the class cluster's abstract method was reached. Signature from the
// callers' register use (the abstract class, the receiver, the selector); the callers continue after it.

// Native __NSCollectionsShouldCopy, 0x1c0287374 (was a private external): whether -copyWithZone: of an
// immutable collection copies its items (bit 0 of the result is used; no argument is passed).

// Native _NSIsNSArray (0x1c029e0b4), _NSIsNSOrderedSet (0x1c029e35c), _NSIsNSSet (0x1c029e4d0) and
// _NSIsNSString (0x1c029e544), external, in the Objective-C runtime glue unit beside _NSIsNSNumber; bit 0 of
// the result is tested (a 32-bit test for _NSIsNSString).

// Native NSRangeException (0x2227a8448, __DATA_CONST,__const, external), in the NSException unit's run of
// exception names (Objective-C/OCFException.m restores the others).

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// libobjc's objc_retain, which the native image imports (the explicit retains of the constructors); the SDK
// installs no declaration for it.
extern id objc_retain(id object);

#ifdef __OBJC__
// Native class NSSet (__DATA,__objc_data 0x22e6ce440, external), an NSObject subclass; class_ro_t
// 0x2227c4f10: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The superclass pointer 0x22e6b6088 lies
// outside the extracted image; NSObject is the SDK's hierarchy (confidence: medium). The method and protocol
// lists (0x23d88dc89, 0x23d88dec1) also lie outside it: the categories the methods belong to are unread, so
// every method is declared and defined on the class. NSRange parameters are spelled OCFRange (two 64-bit
// words in X registers, the native passing).
__attribute__((visibility("default")))
@interface ONSSet : NSObject

+ (id)allocWithZone:(struct _NSZone *)zone;
+ (id)set;
+ (id)setWithObject:(id)object;
+ (id)setWithObjects:(const id *)objects count:(NSUInteger)count;
+ (id)setWithObjects:(id)firstObject, ...;
+ (id)newSetWithObjects:(const id *)objects count:(NSUInteger)count;
+ (id)setWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)flag;
+ (id)setWithArray:(id)array range:(OCFRange)range;
+ (id)setWithArray:(id)array copyItems:(BOOL)flag;
+ (id)setWithArray:(id)array;
+ (id)setWithOrderedSet:(id)orderedSet range:(OCFRange)range copyItems:(BOOL)flag;
+ (id)setWithOrderedSet:(id)orderedSet range:(OCFRange)range;
+ (id)setWithOrderedSet:(id)orderedSet copyItems:(BOOL)flag;
+ (id)setWithOrderedSet:(id)orderedSet;
+ (id)setWithSet:(id)set copyItems:(BOOL)flag;
+ (id)setWithSet:(id)set;
+ (BOOL)supportsSecureCoding;

- (OCFTypeID)_cfTypeID;
- (BOOL)isNSSet__;
- (NSUInteger)count;
- (id)member:(id)object;
- (id)objectEnumerator;
- (id)allObjects;
- (id)anyObject;
- (BOOL)containsObject:(id)object;
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length;
- (NSUInteger)countForObject:(id)object;
- (id)descriptionWithLocale:(id)locale indent:(NSUInteger)level;
- (id)descriptionWithLocale:(id)locale;
- (void)enumerateObjectsWithOptions:(NSUInteger)options usingBlock:(void (^)(id object, BOOL *stop))block;
- (void)enumerateObjectsUsingBlock:(void (^)(id object, BOOL *stop))block;
- (void)getObjects:(id *)objects count:(NSUInteger)count;
- (void)getObjects:(id *)objects;
- (void)getObjects:(id *)objects range:(OCFRange)range;
- (BOOL)intersectsOrderedSet:(id)orderedSet;
- (BOOL)intersectsSet:(id)otherSet;
- (BOOL)isEqualToSet:(id)otherSet;
- (BOOL)isSubsetOfOrderedSet:(id)orderedSet;
- (BOOL)isSubsetOfSet:(id)otherSet;
- (void)makeObjectsPerformSelector:(SEL)selector;
- (void)makeObjectsPerformSelector:(SEL)selector withObject:(id)argument;
- (id)members:(id)objects notFoundMarker:(id)marker;
- (id)objectWithOptions:(NSUInteger)options passingTest:(BOOL (^)(id object, BOOL *stop))predicate;
- (id)objectPassingTest:(BOOL (^)(id object, BOOL *stop))predicate;
- (id)objectsWithOptions:(NSUInteger)options passingTest:(BOOL (^)(id object, BOOL *stop))predicate;
- (id)objectsPassingTest:(BOOL (^)(id object, BOOL *stop))predicate;
- (id)sortedArrayWithOptions:(NSUInteger)options usingComparator:(OCFComparisonResult (^)(id, id))comparator;
- (id)sortedArrayUsingComparator:(OCFComparisonResult (^)(id, id))comparator;
- (void)__applyValues:(void (*)(const void *value, void *context))applier context:(void *)context;
- (BOOL)__getValue:(id *)value forObj:(id)object;
- (id)initWithObject:(id)object;
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count;
- (id)initWithObjects:(id)firstObject, ...;
- (id)initWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)flag;
- (id)initWithArray:(id)array range:(OCFRange)range;
- (id)initWithArray:(id)array copyItems:(BOOL)flag;
- (id)initWithArray:(id)array;
- (id)initWithOrderedSet:(id)orderedSet range:(OCFRange)range copyItems:(BOOL)flag;
- (id)initWithOrderedSet:(id)orderedSet range:(OCFRange)range;
- (id)initWithOrderedSet:(id)orderedSet copyItems:(BOOL)flag;
- (id)initWithOrderedSet:(id)orderedSet;
- (id)initWithSet:(id)set copyItems:(BOOL)flag;
- (id)initWithSet:(id)set;
- (id)setByAddingObject:(id)object;
- (id)setByAddingObjectsFromArray:(id)array;
- (id)setByAddingObjectsFromSet:(id)otherSet;
- (id)copyWithZone:(struct _NSZone *)zone;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
- (id)initWithCoder:(id)coder;
- (void)encodeWithCoder:(id)coder;

@end

// Selectors the set methods send to objects of classes outside the two units (arrays, ordered sets,
// enumerators, NSException, NSArray), declared for the message sends only; no protocol object is emitted.
@protocol SetUnrestoredMessages

- (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
- (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;
- (id)array;
- (id)_initByAdoptingBuffer:(id *)buffer count:(NSUInteger)count size:(NSUInteger)size;
- (id)objectAtIndex:(NSUInteger)index;
- (id)nextObject;
- (id)_stringRepresentation;
- (id)initWithCapacity:(NSUInteger)capacity;
- (void)addObject:(id)object;
- (void)removeObject:(id)object;

@end

// The ThreadSanitizer checks every set method inlines: load the hook, and when it is set, call it with the
// object, the method's return address (X30 natively) and __CFTSANTagMutableSet (the tag both classes pass).
// setTSANRead and setTSANWrite are local descriptive names for the inline code of tsanRead and tsanWrite in
// Objective-C/OCFBridgedClasses.h.
static inline void setTSANRead(const void *object, void *returnAddress) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanReadFunction;
    if (hook != NULL) {
        hook(object, returnAddress, __OCFTSANTagMutableSet);
    }
}

static inline void setTSANWrite(const void *object, void *returnAddress) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanWriteFunction;
    if (hook != NULL) {
        hook(object, returnAddress, __OCFTSANTagMutableSet);
    }
}

// The NSException class symbol (_OBJC_CLASS_$_NSException, loaded natively through the class reference
// 0x2227ce300), the NSArray class symbol (0x2227ce2a0) and the NSMutableSet and NSSet class symbols
// (0x2227ce390, 0x2227ce3b8; the __SCR_ GOT slots hold the same symbols).
extern char setExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char setArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSArray));

// The raise every argument check of the two units inlines: an os_log pack sized by the format's buffer
// (_os_log_pack_size of 12, 22, 32 or 42 bytes for the formats here), on the stack, filled with errno 0 and
// the image's header, its arguments written as the compiler's __builtin_os_log_format encodes them (the
// %s argument as a pointer: 0x20 0x08; the %lu arguments: 0x00 0x08); then the reason made with
// kCFAllocatorSystemDefault, autoreleased through _CFAutoreleasePoolAddObject, and
// +[NSException exceptionWithName:reason:userInfo:osLogPack:size:] (no user info) thrown.
// raiseWithLogPack is a local descriptive name.
#define raiseWithLogPack(exceptionName, format, ...)                                                         \
    do {                                                                                                     \
        size_t packSize = _os_log_pack_size(__builtin_os_log_format_buffer_size(format, __VA_ARGS__));       \
        void *pack = __builtin_alloca((packSize + 15) & ~(size_t)15);                                        \
        uint8_t *packBuffer = _os_log_pack_fill(pack, packSize, 0, &__dso_handle, format);                   \
        __builtin_os_log_format(packBuffer, format, __VA_ARGS__);                                            \
        OCFStringRef packReason =                                                                            \
            OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR(format), __VA_ARGS__);        \
        objc_exception_throw([(id<SetUnrestoredMessages>)(id)setExceptionClass                               \
            exceptionWithName:(id)(exceptionName)                                                            \
                       reason:_OCFAutoreleasePoolAddObject(NULL, packReason)                                 \
                     userInfo:nil                                                                            \
                    osLogPack:pack                                                                           \
                         size:packSize]);                                                                    \
    } while (0)

// The overflow raise of the native temporary id buffers (the new_id_array pattern): a count with any of the
// top four bits set throws NSGenericException; the reason (default allocator) is released before the throw.
// raiseTemporaryBufferTooLarge is a local descriptive name for the inlined code.
static inline __attribute__((noreturn)) void raiseTemporaryBufferTooLarge(unsigned long count) {
    OCFStringRef reason = OCFStringCreateWithFormat(
        NULL,
        NULL,
        OF_STR(
            "*** attempt to create a temporary id buffer which is too large or with a negative count (%lu) "
            "-- "
            "possibly data is corrupt"
        ),
        count
    );
    id exception = [(id<SetUnrestoredMessages>)(id)setExceptionClass exceptionWithName:(id)ONSGenericException
                                                                                reason:(id)reason
                                                                              userInfo:nil];
    OCFRelease(reason);
    objc_exception_throw(exception);
}

// The temporary id buffer (new_id_array pattern) without an unwinding cleanup: the overflow check, then a
// stack array of max(count, 1) entries up to 256 entries (one entry above), and above 256 entries
// _CFCreateArrayStorage(max(count, 1), false, &size), whose storage the caller frees with free(nameHeap).
// temporaryIDBuffer is a local descriptive name for the inlined code.
#define temporaryIDBuffer(name, count)                                                                       \
    if ((unsigned long)(count) >> 60 != 0) {                                                                 \
        raiseTemporaryBufferTooLarge(count);                                                                 \
    }                                                                                                        \
    unsigned long name##Count = (unsigned long)(count) > 1 ? (unsigned long)(count) : 1;                     \
    id name##Stack[(unsigned long)(count) < 0x101 ? name##Count : 1];                                        \
    size_t name##Size = 0;                                                                                   \
    id *name##Heap = NULL;                                                                                   \
    id *name = name##Stack;                                                                                  \
    if ((unsigned long)(count) >= 0x101) {                                                                   \
        name = (id *)_OCFCreateArrayStorage(name##Count, false, &name##Size);                                \
        name##Heap = name;                                                                                   \
    }

// The cleanup of the temporary id buffers that the native code frees on unwinding as well (the landing pads
// free the storage and resume). freeTemporaryStorage is a local descriptive name.
static inline void freeTemporaryStorage(id **storage) {
    free(*storage);
}

#endif
