//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSOrderedSet.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFSortFunctions.h"
#include "CoreFoundation/Objective-C/OCFNSArray.h"
#include "CoreFoundation/Objective-C/OCFNSOrderedSetHelpers.h"
#include "CoreFoundation/Objective-C/OCFNSOrderedSetI.h"
#include "CoreFoundation/Objective-C/OCFNSOrderedSetM.h"
#include "CoreFoundation/Objective-C/OCFNSOrderedSetPlaceholder.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/Objective-C/OCFRequireConcreteImplementation.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <alloca.h>
#include <mach-o/loader.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// Inferred organization: the NSOrderedSet methods lie together in the iOS Simulator __text section
// (0x1c02c4430-0x1c02caecc, with __createOrderedSet at 0x1c02ca8e0 and the unit's OUTLINED_FUNCTION_0_55,
// _1_39 and _2_33 at 0x1c02caed0-0x1c02caef0), and the four primitive methods that only raise (-count,
// -indexOfObject:, -objectAtIndex:, -initWithObjects:count:; 0x1c031221c-0x1c031227c) in the cold region;
// following the owner's decision, the class has its own translation unit. Objective-C with manual reference
// counting (objc_retain, objc_release, objc_autorelease on explicit buffers). The os_log argument packing of
// the raises is the compiler's __os_log_helper_1_2_3_8_32_8_0_8_0 and __os_log_helper_1_2_4_8_32_8_0_8_0_8_0,
// or the same stores inline; both are restored as __builtin_os_log_format. The block literal of
// -sortedArrayFromRange:options:usingComparator: keeps the native captures; its descriptor (unk_22279D5B0)
// lies outside the extracted image and the layout clang computes stands for it. Many methods match the
// NSArray or NSSet methods of the same selector instruction for instruction (Objective-C/OCFNSArray.m,
// Objective-C/OCFNSSet.m); the differences are named at each definition. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full).
// iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not
// written yet).
//
// The native unit inlines four checks at many methods; each is spelled once here:
// - the ThreadSanitizer read check (orderedSetTSANRead): the hook __cf_tsanReadFunction, when set, is called
//   with the receiver, the method's return address and __CFTSANTagMutableOrderedSet;
// - the logged raise (RAISE_ORDERED_SET_EXCEPTION): _os_log_pack_size of the format's argument buffer, the
//   pack on the stack filled with errno 0 and the image's header, the arguments packed, the reason made with
//   kCFAllocatorSystemDefault and autoreleased through _CFAutoreleasePoolAddObject, and +[NSException
//   exceptionWithName:reason:userInfo:osLogPack:size:] (no user info) thrown;
// - the range raise (RAISE_ORDERED_SET_RANGE_EXCEPTION): NSRangeException "*** %s: range {%lu, %lu} extends
//   beyond bounds for empty ordered set" (buffer 0x20) for an empty collection, else "*** %s: range {%lu,
//   %lu} extends beyond bounds [0 .. %lu]" (buffer 0x2A) with the count minus one;
// - the temporary buffer limit (raiseOrderedSetBufferTooLarge): a count of 2^60 or more raises
//   NSGenericException "*** attempt to create a temporary id buffer which is too large or with a negative
//   count (%lu) -- possibly data is corrupt" (reason with the NULL allocator,
//   +exceptionWithName:reason:userInfo:, the reason released after the exception is made).

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// libobjc entry points without SDK declarations (objc-internal.h); supplied, never reimplemented.
extern id objc_alloc(Class cls);
extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *context);

// The class symbols the methods compare against or send to directly. NSOrderedSet's own symbol is this unit's
// class, referenced by its assembler name for the comparisons +allocWithZone: and
// +newOrderedSetWithObjects:count: make without messaging. NSMutableOrderedSet and the enumerator and view
// classes are restored in their own units (Objective-C/OCFNSMutableOrderedSet.m and siblings).
extern char orderedSetExceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char orderedSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSOrderedSet));
extern char mutableOrderedSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableOrderedSet));
extern char
    orderedSetFastEnumerationEnumeratorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                         OF_NAME_STRING(__ONSFastEnumerationEnumerator));
extern char orderedSetReverseEnumeratorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                         OF_NAME_STRING(__ONSOrderedSetReverseEnumerator));
extern char orderedSetReversedClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                OF_NAME_STRING(__ONSOrderedSetReversed));
extern char orderedSetArrayProxyClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                  OF_NAME_STRING(__ONSOrderedSetArrayProxy));
extern char orderedSetSetProxyClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                OF_NAME_STRING(__ONSOrderedSetSetProxy));

// The messages the methods send to objects of other classes (index sets, sets, enumerators, exceptions),
// declared for the sends only; no protocol object is emitted.
@protocol OrderedSetMessages

+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;
- (NSUInteger)lastIndex;
- (NSUInteger)rangeCount;
- (OCFRange)rangeAtIndex:(NSUInteger)index;
- (void)getObjects:(id *)objects count:(NSUInteger)count;
- (id)_stringRepresentation;
- (id)descriptionWithLocale:(id)locale indent:(NSUInteger)level;
- (id)descriptionWithLocale:(id)locale;
- (id)initWithObject:(id)object;
- (id)initWithOrderedSet:(id)orderedSet;

@end

// The fast-enumeration state the native -countByEnumeratingWithState:objects:count: fills (the layout of
// NSFastEnumerationState; OrderedSetEnumerationState is a local name).
typedef struct {
    unsigned long state;
    id *itemsPtr;
    unsigned long *mutationsPtr;
    unsigned long extra[5];
} OrderedSetEnumerationState;

// The ThreadSanitizer read check; orderedSetTSANRead is a local descriptive name.
static inline void orderedSetTSANRead(const void *object, void *returnAddress) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanReadFunction;
    if (hook != NULL) {
        hook(object, returnAddress, __OCFTSANTagMutableOrderedSet);
    }
}

#define RAISE_ORDERED_SET_EXCEPTION(name, format, ...)                                                       \
    do {                                                                                                     \
        size_t packSize = _os_log_pack_size(__builtin_os_log_format_buffer_size(format, __VA_ARGS__));       \
        void *pack = alloca((packSize + 15) & ~(size_t)15);                                                  \
        uint8_t *packBuffer = _os_log_pack_fill(pack, packSize, 0, &__dso_handle, format);                   \
        __builtin_os_log_format(packBuffer, format, __VA_ARGS__);                                            \
        OCFStringRef reason =                                                                                \
            OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR(format), __VA_ARGS__);        \
        objc_exception_throw([(id<OrderedSetMessages>)(id)orderedSetExceptionClass                           \
            exceptionWithName:(id)(name)                                                                     \
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)                                     \
                     userInfo:nil                                                                            \
                    osLogPack:pack                                                                           \
                         size:packSize]);                                                                    \
    } while (0)

#define RAISE_ORDERED_SET_RANGE_EXCEPTION(method, location, length, count)                                   \
    do {                                                                                                     \
        if ((count) == 0) {                                                                                  \
            RAISE_ORDERED_SET_EXCEPTION(                                                                     \
                ONSRangeException,                                                                           \
                "*** %s: range {%lu, %lu} extends beyond bounds for empty ordered set",                      \
                method,                                                                                      \
                (unsigned long)(location),                                                                   \
                (unsigned long)(length)                                                                      \
            );                                                                                               \
        }                                                                                                    \
        RAISE_ORDERED_SET_EXCEPTION(                                                                         \
            ONSRangeException,                                                                               \
            "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",                                     \
            method,                                                                                          \
            (unsigned long)(location),                                                                       \
            (unsigned long)(length),                                                                         \
            (unsigned long)((count) - 1)                                                                     \
        );                                                                                                   \
    } while (0)

// raiseOrderedSetBufferTooLarge is a local descriptive name.
static __attribute__((noreturn)) void raiseOrderedSetBufferTooLarge(NSUInteger count) {
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
    id exception =
        [(id<OrderedSetMessages>)(id)orderedSetExceptionClass exceptionWithName:(id)ONSGenericException
                                                                         reason:(id)reason
                                                                       userInfo:nil];
    OCFRelease(reason);
    objc_exception_throw(exception);
}

// The cleanup of the buffers the native code frees when an exception unwinds through it (landing pads that
// free the variable's current value); freeOrderedSetBufferOnExit is a local descriptive name.
static void freeOrderedSetBufferOnExit(id **buffer) {
    free(*buffer);
}

// Native ___createOrderedSet, 0x1c02ca8e0; defined after +[NSOrderedSet orderedSetWithObject:], in native
// address order.
static id __createOrderedSet(id orderedSet, id object);

@implementation ONSOrderedSet

// Native -[NSOrderedSet isNSOrderedSet__], 0x1c02c4430. Non-external. Answers YES.
- (BOOL)isNSOrderedSet__ {
    return YES;
}

// Native -[NSOrderedSet allObjects], 0x1c02c4438. Non-external. The read check, -count and the buffer limit;
// the objects are copied with -getObjects:range: into _CFCreateArrayStorage of the count (at least one, not
// zeroed), each retained (objc_retain, no tagged-pointer test), and the buffer is adopted by a new NSArray
// (objc_alloc, -_initByAdoptingBuffer:count:size: with the count twice); a NULL buffer (not produced by
// _CFCreateArrayStorage, which crashes instead) would make -initWithObjects:NULL count:. Autoreleased. The
// instructions of -[NSArray allObjects].
- (id)allObjects {
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if ((OF_WORD_SHIFT(count, 60)) != 0) {
        raiseOrderedSetBufferTooLarge(count);
    }
    size_t capacity = 0;
    id *buffer = (id *)_OCFCreateArrayStorage(count > 1 ? count : 1, false, &capacity);
    [self getObjects:buffer range:(OCFRange){0, (OCFIndex)count}];
    id result;
    if (buffer != NULL) {
        for (NSUInteger index = 0; index < count; index++) {
            [buffer[index] retain];
        }
        result = [[ONSArray alloc] _initByAdoptingBuffer:buffer count:count size:count];
    } else {
        result = [[ONSArray alloc] initWithObjects:NULL count:count];
    }
    return [result autorelease];
}

// Native -[NSOrderedSet containsObject:inRange:], 0x1c02c45dc. Non-external. -count and the range raise
// (location or length with bit 63 set, or a count below their sum) come before the read check; then whether
// -indexOfObject:inRange: answers an index other than NSNotFound.
- (BOOL)containsObject:(id)object inRange:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (range.location < 0 || range.length < 0 || count < location + length) {
        RAISE_ORDERED_SET_RANGE_EXCEPTION("-[NSOrderedSet containsObject:inRange:]", location, length, count);
    }
    orderedSetTSANRead(self, returnAddress);
    return [self indexOfObject:object inRange:range] != kNotFound;
}

// Native -[NSOrderedSet containsObject:], 0x1c02c4808. Non-external. The read check; whether -indexOfObject:
// answers an index other than NSNotFound.
- (BOOL)containsObject:(id)object {
    orderedSetTSANRead(self, __builtin_return_address(0));
    return [self indexOfObject:object] != kNotFound;
}

// Native -[NSOrderedSet countByEnumeratingWithState:objects:count:], 0x1c02c486c. Non-external. A NULL buffer
// with a nonzero length raises NSInvalidArgumentException "*** %s: pointer to objects array is NULL but
// length is %lu", a length of 2^61 or more "*** %s: count (%lu) of objects array is ridiculous" (both buffer
// 0x16). A state of -1 answers 0 before the read check (the NSArray method checks first). The first call
// points mutationsPtr at extra[0] and stores -count there; later calls read the count from extra[0].
// itemsPtr is the buffer; the batch is the smaller of the length and the count minus the state (unsigned),
// copied with -getObjects:range: from the state when nonzero; the state advances by the batch, or becomes -1
// when that reaches the count.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    if (buffer == NULL && length != 0) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[NSOrderedSet countByEnumeratingWithState:objects:count:]",
            (unsigned long)length
        );
    }
    if ((OF_WORD_SHIFT(length, 61)) != 0) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[NSOrderedSet countByEnumeratingWithState:objects:count:]",
            (unsigned long)length
        );
    }
    OrderedSetEnumerationState *enumeration = (OrderedSetEnumerationState *)state;
    if (enumeration->state == (unsigned long)-1) {
        return 0;
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger count;
    if (enumeration->state == 0) {
        enumeration->mutationsPtr = &enumeration->extra[0];
        count = [self count];
        enumeration->extra[0] = count;
    } else {
        count = enumeration->extra[0];
    }
    enumeration->itemsPtr = buffer;
    NSUInteger remaining = count - enumeration->state;
    NSUInteger batch = length < remaining ? length : remaining;
    if (batch != 0) {
        [self getObjects:buffer range:(OCFRange){(OCFIndex)enumeration->state, (OCFIndex)batch}];
    }
    NSUInteger next = enumeration->state + batch;
    enumeration->state = count > next ? next : (unsigned long)-1;
    return batch;
}

// Native -[NSOrderedSet countForObject:inRange:], 0x1c02c4adc. Non-external. As -containsObject:inRange:
// (the range raise before the read check), answering 1 or 0.
- (NSUInteger)countForObject:(id)object inRange:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (range.location < 0 || range.length < 0 || count < location + length) {
        RAISE_ORDERED_SET_RANGE_EXCEPTION("-[NSOrderedSet countForObject:inRange:]", location, length, count);
    }
    orderedSetTSANRead(self, returnAddress);
    return [self indexOfObject:object inRange:range] != kNotFound;
}

// Native -[NSOrderedSet countForObject:], 0x1c02c4d08. Non-external. The read check; 1 when -indexOfObject:
// answers an index other than NSNotFound, else 0.
- (NSUInteger)countForObject:(id)object {
    orderedSetTSANRead(self, __builtin_return_address(0));
    return [self indexOfObject:object] != kNotFound;
}

// Native -[NSOrderedSet enumerateObjectsAtIndexes:options:usingBlock:], 0x1c02c4d6c. Non-external. A nil
// index set raises NSInvalidArgumentException "*** %s: index set cannot be nil", one that is not an
// NSIndexSet "*** %s: index set argument is not an NSIndexSet", a nil block "*** %s: block cannot be nil"
// (buffer 0x0C each); then the read check. Reverse drops bits 0 and 60; bit 60 is set; then
// __NSOrderedSetParameterCheckIterate and __NSOrderedSetEnumerate (a tail call).
- (void)enumerateObjectsAtIndexes:(id)indexes
                          options:(NSUInteger)options
                       usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    if (indexes == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set cannot be nil",
            "-[NSOrderedSet enumerateObjectsAtIndexes:options:usingBlock:]"
        );
    }
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSOrderedSet enumerateObjectsAtIndexes:options:usingBlock:]"
        );
    }
    if (block == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: block cannot be nil",
            "-[NSOrderedSet enumerateObjectsAtIndexes:options:usingBlock:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    if (options & 2) {
        options &= 0xEFFFFFFFFFFFFFFEULL;
    }
    __NSOrderedSetParameterCheckIterate(self, _cmd, block, options | 0x1000000000000000ULL, indexes);
    __NSOrderedSetEnumerate(self, block, options | 0x1000000000000000ULL, indexes);
}

// Native -[NSOrderedSet enumerateObjectsWithOptions:usingBlock:], 0x1c02c5004. Non-external. A nil block
// raises "*** %s: block cannot be nil"; the read check; reverse drops bit 0;
// __NSOrderedSetParameterCheckIterate and __NSOrderedSetEnumerate with no index set.
- (void)enumerateObjectsWithOptions:(NSUInteger)options
                         usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    if (block == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: block cannot be nil",
            "-[NSOrderedSet enumerateObjectsWithOptions:usingBlock:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    if (options & 2) {
        options &= ~(NSUInteger)1;
    }
    __NSOrderedSetParameterCheckIterate(self, _cmd, block, options, nil);
    __NSOrderedSetEnumerate(self, block, options, nil);
}

// Native -[NSOrderedSet enumerateObjectsUsingBlock:], 0x1c02c5188. Non-external. A nil block raises; the read
// check; -enumerateObjectsWithOptions:0 usingBlock:.
- (void)enumerateObjectsUsingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    if (block == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: block cannot be nil",
            "-[NSOrderedSet enumerateObjectsUsingBlock:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    [self enumerateObjectsWithOptions:0 usingBlock:block];
}

// Native -[NSOrderedSet firstObject], 0x1c02c52dc. Non-external. The read check; nil (the zero count) for an
// empty ordered set, else -objectAtIndex: 0 (a tail call).
- (id)firstObject {
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (count != 0) {
        return [self objectAtIndex:0];
    }
    return (id)count;
}

// Native -[NSOrderedSet getObjects:range:], 0x1c02c5344. Non-external. A NULL buffer with a nonzero length
// raises NSInvalidArgumentException "*** %s: pointer to objects array is NULL but length is %lu", a length
// of 2^61 or more "*** %s: count (%lu) of objects array is ridiculous" (buffer 0x16 each); -count; a location
// with bit 63 set or a count below the range's end is the range raise; then the read check (after the range
// check, unlike the NSArray method), and each object is copied with -objectAtIndex:.
- (void)getObjects:(id *)objects range:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (objects == NULL && length != 0) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[NSOrderedSet getObjects:range:]",
            (unsigned long)length
        );
    }
    if ((OF_WORD_SHIFT(length, 61)) != 0) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[NSOrderedSet getObjects:range:]",
            (unsigned long)length
        );
    }
    NSUInteger count = [self count];
    if ((intptr_t)location < 0 || count < location + length) {
        RAISE_ORDERED_SET_RANGE_EXCEPTION("-[NSOrderedSet getObjects:range:]", location, length, count);
    }
    orderedSetTSANRead(self, returnAddress);
    for (; length != 0; length--) {
        *objects++ = [self objectAtIndex:location++];
    }
}

// Native -[NSOrderedSet getObjects:], 0x1c02c56d0. Non-external. The read check; -count; a NULL buffer for a
// nonempty ordered set raises "*** %s: pointer to objects array is NULL but length is %lu", a count of 2^61
// or more "*** %s: count (%lu) of objects array is ridiculous"; then -getObjects:range: (0, the count read
// once; the NSArray method reads it again), a tail call.
- (void)getObjects:(id *)objects {
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (objects == NULL && count != 0) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[NSOrderedSet getObjects:]",
            (unsigned long)count
        );
    }
    if ((OF_WORD_SHIFT(count, 61)) != 0) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[NSOrderedSet getObjects:]",
            (unsigned long)count
        );
    }
    [self getObjects:objects range:(OCFRange){0, (OCFIndex)count}];
}

// Native -[NSOrderedSet indexOfObject:inRange:], 0x1c02c58d4. Non-external. The read check, -count and the
// range raise; then -indexOfObject:: NSNotFound is answered as is, and an index outside the range
// (index - location not below the length, unsigned) answers NSNotFound.
- (NSUInteger)indexOfObject:(id)object inRange:(OCFRange)range {
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if ((intptr_t)location < 0 || (intptr_t)length < 0 || count < location + length) {
        RAISE_ORDERED_SET_RANGE_EXCEPTION("-[NSOrderedSet indexOfObject:inRange:]", location, length, count);
    }
    NSUInteger index = [self indexOfObject:object];
    NSUInteger inside = index - location >= length ? kNotFound : index;
    if (index == kNotFound) {
        return index;
    }
    return inside;
}

// Native -[NSOrderedSet indexOfObject:inSortedRange:options:usingComparator:], 0x1c02c5b04. Non-external.
// The instructions of the NSArray method (Objective-C/OCFNSArray.m, where the search is described) with this
// class's messages: a nil object raises "*** %s: object cannot be nil", a nil comparator "*** %s: comparator
// cannot be nil" (buffer 0x0C each); the read check, -count and the range raise; both
// NSBinarySearchingFirstEqual (0x100) and NSBinarySearchingLastEqual (0x200) raise NSInvalidArgumentException
// "%@: both NSBinarySearchingFirstEqual and NSBinarySearchingLastEqual options cannot be specified"
// (__CFExceptionProem; reason with kCFAllocatorSystemDefault, autoreleased;
// +exceptionWithName:reason:userInfo:).
- (NSUInteger)indexOfObject:(id)object
              inSortedRange:(OCFRange)range
                    options:(NSUInteger)options
            usingComparator:(NSInteger (^)(id object1, id object2))comparator {
    if (object == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: object cannot be nil",
            "-[NSOrderedSet indexOfObject:inSortedRange:options:usingComparator:]"
        );
    }
    if (comparator == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: comparator cannot be nil",
            "-[NSOrderedSet indexOfObject:inSortedRange:options:usingComparator:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    NSUInteger end = location + length;
    if (range.location < 0 || range.length < 0 || count < end) {
        RAISE_ORDERED_SET_RANGE_EXCEPTION(
            "-[NSOrderedSet indexOfObject:inSortedRange:options:usingComparator:]", location, length, count
        );
    }
    NSUInteger equalOptions = options & 0x300;
    if (equalOptions == 0x300) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFStringRef reason = OCFStringCreateWithFormat(
            allocator,
            NULL,
            OF_STR(
                "%@: both NSBinarySearchingFirstEqual and NSBinarySearchingLastEqual options cannot be "
                "specified"
            ),
            __CFExceptionProem(self, _cmd)
        );
        objc_exception_throw([(id<OrderedSetMessages>)(id)orderedSetExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                     userInfo:nil]);
    }
    if (length == 0) {
        return (options & 0x400) == 0 ? kNotFound : location;
    }
    NSUInteger lastEqual = options & 0x200;
    if (length >= 0x101) {
        NSInteger order = comparator([self objectAtIndex:end - 1], object);
        if (order < 0 || (order | (NSInteger)(options & 0x100)) == 0) {
            if ((options & 0x400) == 0) {
                return order < 0 ? kNotFound : end - 1;
            }
            return end;
        }
        order = comparator(object, [self objectAtIndex:location]);
        if (order < 0 || (order | (NSInteger)lastEqual) == 0) {
            if (order < 0 && (options & 0x400) == 0) {
                return kNotFound;
            }
            return location;
        }
    }
    NSUInteger savedOptions = options;
    NSInteger step = (NSInteger)2 << flsl((long)length);
    NSInteger order = 0;
    BOOL sawEqual = NO;
    BOOL more;
    do {
        NSInteger half = step / 2;
        NSUInteger probe = location + (NSUInteger)half;
        if (probe < end) {
            order = comparator([self objectAtIndex:probe], object);
            if ((order | (NSInteger)equalOptions) == 0) {
                return probe;
            }
            sawEqual |= order == 0;
            BOOL advance = order < 0 ? YES : (order != 0 ? NO : lastEqual != 0);
            if (advance) {
                location = probe;
            }
        }
        more = step > 1;
        step = half;
    } while (more);
    BOOL advance = order < 0 ? YES : (order != 0 ? NO : lastEqual != 0);
    NSUInteger insertion = location + (NSUInteger)advance;
    NSUInteger found = sawEqual & 1 ? location + ((uint64_t)order >> 63) : kNotFound;
    return (savedOptions & 0x400) != 0 ? insertion : found;
}

// Native -[NSOrderedSet indexOfObjectAtIndexes:options:passingTest:], 0x1c02c6048. Non-external. The index
// set and predicate checks of -enumerateObjectsAtIndexes:options:usingBlock: ("*** %s: predicate cannot be
// nil"); the read check; reverse drops bits 0, 60 and 61; bits 60 and 61 are set;
// __NSOrderedSetParameterCheckIterate and __NSOrderedSetGetIndexPassingTest (a tail call).
- (NSUInteger)indexOfObjectAtIndexes:(id)indexes
                             options:(NSUInteger)options
                         passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (indexes == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set cannot be nil",
            "-[NSOrderedSet indexOfObjectAtIndexes:options:passingTest:]"
        );
    }
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSOrderedSet indexOfObjectAtIndexes:options:passingTest:]"
        );
    }
    if (predicate == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSOrderedSet indexOfObjectAtIndexes:options:passingTest:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    if (options & 2) {
        options &= 0xCFFFFFFFFFFFFFFEULL;
    }
    __NSOrderedSetParameterCheckIterate(self, _cmd, predicate, options | 0x3000000000000000ULL, indexes);
    return __NSOrderedSetGetIndexPassingTest(self, predicate, options | 0x3000000000000000ULL, indexes);
}

// Native -[NSOrderedSet indexOfObjectWithOptions:passingTest:], 0x1c02c62e0. Non-external. A nil predicate
// raises; the read check; reverse drops bits 0 and 61; bit 61 is set; __NSOrderedSetParameterCheckIterate and
// __NSOrderedSetGetIndexPassingTest with no index set.
- (NSUInteger)indexOfObjectWithOptions:(NSUInteger)options
                           passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSOrderedSet indexOfObjectWithOptions:passingTest:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    if (options & 2) {
        options &= 0xDFFFFFFFFFFFFFFEULL;
    }
    __NSOrderedSetParameterCheckIterate(self, _cmd, predicate, options | 0x2000000000000000ULL, nil);
    return __NSOrderedSetGetIndexPassingTest(self, predicate, options | 0x2000000000000000ULL, nil);
}

// Native -[NSOrderedSet indexOfObjectPassingTest:], 0x1c02c6468. Non-external. A nil predicate raises; the
// read check; -indexOfObjectWithOptions:0 passingTest:.
- (NSUInteger)indexOfObjectPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSOrderedSet indexOfObjectPassingTest:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    return [self indexOfObjectWithOptions:0 passingTest:predicate];
}

// Native -[NSOrderedSet indexesOfObjectsAtIndexes:options:passingTest:], 0x1c02c65bc. Non-external. The
// checks of -indexOfObjectAtIndexes:options:passingTest:; the read check; reverse drops bits 0, 60 and 62;
// bits 60 and 62 are set; __NSOrderedSetParameterCheckIterate and __NSOrderedSetGetIndexesPassingTest (a tail
// call).
- (id)indexesOfObjectsAtIndexes:(id)indexes
                        options:(NSUInteger)options
                    passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (indexes == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set cannot be nil",
            "-[NSOrderedSet indexesOfObjectsAtIndexes:options:passingTest:]"
        );
    }
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSOrderedSet indexesOfObjectsAtIndexes:options:passingTest:]"
        );
    }
    if (predicate == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSOrderedSet indexesOfObjectsAtIndexes:options:passingTest:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    if (options & 2) {
        options &= 0xAFFFFFFFFFFFFFFEULL;
    }
    __NSOrderedSetParameterCheckIterate(self, _cmd, predicate, options | 0x5000000000000000ULL, indexes);
    return __NSOrderedSetGetIndexesPassingTest(self, predicate, options | 0x5000000000000000ULL, indexes);
}

// Native -[NSOrderedSet indexesOfObjectsWithOptions:passingTest:], 0x1c02c685c. Non-external. A nil predicate
// raises; the read check; reverse drops bits 0 and 62; bit 62 is set; __NSOrderedSetParameterCheckIterate and
// __NSOrderedSetGetIndexesPassingTest with no index set.
- (id)indexesOfObjectsWithOptions:(NSUInteger)options
                      passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSOrderedSet indexesOfObjectsWithOptions:passingTest:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    if (options & 2) {
        options &= 0xBFFFFFFFFFFFFFFEULL;
    }
    __NSOrderedSetParameterCheckIterate(self, _cmd, predicate, options | 0x4000000000000000ULL, nil);
    return __NSOrderedSetGetIndexesPassingTest(self, predicate, options | 0x4000000000000000ULL, nil);
}

// Native -[NSOrderedSet indexesOfObjectsPassingTest:], 0x1c02c69e4. Non-external. A nil predicate raises; the
// read check; -indexesOfObjectsWithOptions:0 passingTest:.
- (id)indexesOfObjectsPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSOrderedSet indexesOfObjectsPassingTest:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    return [self indexesOfObjectsWithOptions:0 passingTest:predicate];
}

// Native -[NSOrderedSet intersectsOrderedSet:], 0x1c02c6b38. Non-external. A non-NSOrderedSet raises
// NSInvalidArgumentException "*** %s: ordered set argument is not an NSOrderedSet" (buffer 0x0C); the read
// check. The smaller collection (the receiver when its count is below the other's) is enumerated and each
// object tested with the other's -containsObject:. The instructions of -[NSSet intersectsOrderedSet:].
- (BOOL)intersectsOrderedSet:(id)other {
    if (other != nil && !(_ONSIsNSOrderedSet(other) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSOrderedSet intersectsOrderedSet:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger selfCount = [self count];
    NSUInteger otherCount = [other count];
    id enumerated = selfCount >= otherCount ? other : self;
    id tested = selfCount >= otherCount ? self : other;
    for (id object in enumerated) {
        if ([tested containsObject:object]) {
            return YES;
        }
    }
    return NO;
}

// Native -[NSOrderedSet intersectsSet:], 0x1c02c6d48. Non-external. A non-NSSet raises "*** %s: set argument
// is not an NSSet" (no forgiveness, unlike the NSSet method); then the pattern of -intersectsOrderedSet:.
- (BOOL)intersectsSet:(id)set {
    if (set != nil && !(_ONSIsNSSet(set) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: set argument is not an NSSet",
            "-[NSOrderedSet intersectsSet:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger selfCount = [self count];
    NSUInteger otherCount = [set count];
    id enumerated = selfCount >= otherCount ? set : self;
    id tested = selfCount >= otherCount ? self : set;
    for (id object in enumerated) {
        if ([tested containsObject:object]) {
            return YES;
        }
    }
    return NO;
}

// Native -[NSOrderedSet isSubsetOfOrderedSet:], 0x1c02c6f58. Non-external. A non-NSOrderedSet raises "*** %s:
// ordered set argument is not an NSOrderedSet"; the read check. A receiver larger than the other answers NO;
// otherwise every object of the receiver must have a nonzero -countForObject: in the other, and the answer is
// that result and a non-nil other.
- (BOOL)isSubsetOfOrderedSet:(id)other {
    if (other != nil && !(_ONSIsNSOrderedSet(other) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSOrderedSet isSubsetOfOrderedSet:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger selfCount = [self count];
    if (selfCount > [other count]) {
        return NO;
    }
    BOOL subset = YES;
    for (id object in self) {
        if ([other countForObject:object] == 0) {
            subset = NO;
            break;
        }
    }
    return other != nil && subset;
}

// Native -[NSOrderedSet isSubsetOfSet:], 0x1c02c7178. Non-external. A non-NSSet raises "*** %s: set argument
// is not an NSSet" (no forgiveness); then the pattern of -isSubsetOfOrderedSet:.
- (BOOL)isSubsetOfSet:(id)set {
    if (set != nil && !(_ONSIsNSSet(set) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: set argument is not an NSSet",
            "-[NSOrderedSet isSubsetOfSet:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger selfCount = [self count];
    if (selfCount > [set count]) {
        return NO;
    }
    BOOL subset = YES;
    for (id object in self) {
        if ([set countForObject:object] == 0) {
            subset = NO;
            break;
        }
    }
    return set != nil && subset;
}

// Native -[NSOrderedSet lastObject], 0x1c02c7398. Non-external. The read check; nil for an empty ordered set,
// else -objectAtIndex: of the count minus one (a tail call).
- (id)lastObject {
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (count != 0) {
        return [self objectAtIndex:count - 1];
    }
    return (id)count;
}

// Native -[NSOrderedSet objectAtIndexedSubscript:], 0x1c02c7400. Non-external. The read check;
// -objectAtIndex: (a tail call).
- (id)objectAtIndexedSubscript:(NSUInteger)index {
    orderedSetTSANRead(self, __builtin_return_address(0));
    return [self objectAtIndex:index];
}

// Native -[NSOrderedSet objectAtIndexes:options:passingTest:], 0x1c02c7454. Non-external. The checks of
// -indexOfObjectAtIndexes:options:passingTest:; the read check; that method's answer, nil for NSNotFound,
// else -objectAtIndex: of it (a tail call).
- (id)objectAtIndexes:(id)indexes
              options:(NSUInteger)options
          passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (indexes == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set cannot be nil",
            "-[NSOrderedSet objectAtIndexes:options:passingTest:]"
        );
    }
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSOrderedSet objectAtIndexes:options:passingTest:]"
        );
    }
    if (predicate == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSOrderedSet objectAtIndexes:options:passingTest:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger index = [self indexOfObjectAtIndexes:indexes options:options passingTest:predicate];
    if (index == kNotFound) {
        return nil;
    }
    return [self objectAtIndex:index];
}

// Native -[NSOrderedSet objectWithOptions:passingTest:], 0x1c02c770c. Non-external. A nil predicate raises;
// the read check; -indexOfObjectWithOptions:passingTest:, nil for NSNotFound, else -objectAtIndex:.
- (id)objectWithOptions:(NSUInteger)options
            passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSOrderedSet objectWithOptions:passingTest:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger index = [self indexOfObjectWithOptions:options passingTest:predicate];
    if (index == kNotFound) {
        return nil;
    }
    return [self objectAtIndex:index];
}

// Native -[NSOrderedSet objectPassingTest:], 0x1c02c78ac. Non-external. A nil predicate raises; the read
// check; -objectWithOptions:0 passingTest: (a tail call).
- (id)objectPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSOrderedSet objectPassingTest:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    return [self objectWithOptions:0 passingTest:predicate];
}

// Native -[NSOrderedSet objectsAtIndexes:], 0x1c02c7a00. Non-external. A nil index set raises "*** %s: index
// set cannot be nil", one that is not an NSIndexSet "*** %s: index set argument is not an NSIndexSet" (buffer
// 0x0C each); the read check; the NSIndexSet check again (the same raise). The bound is -count plus -count
// sent to nil (0, as the native code does); a last index with bit 63 set, or neither NSNotFound nor below the
// bound, raises NSRangeException "*** %s: index %lu in index set beyond bounds for empty ordered set" (buffer
// 0x16) for a zero bound, else "*** %s: index %lu in index set beyond bounds [0 .. %lu]" (buffer 0x20, the
// bound minus one). Then the index set's -count, the buffer limit and _CFCreateArrayStorage (at least one
// slot); while the range position is below the set's -rangeCount (asked each time), -rangeAtIndex: is copied
// with -getObjects:range: at the running position, which advances by the range's length (no block, unlike the
// NSArray method). The objects are retained (objc_retain) and adopted by a new NSArray
// (-_initByAdoptingBuffer:count:size:, the set's count twice), or -initWithObjects:NULL count: for a NULL
// buffer; autoreleased.
- (id)objectsAtIndexes:(id)indexes {
    if (indexes == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set cannot be nil",
            "-[NSOrderedSet objectsAtIndexes:]"
        );
    }
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSOrderedSet objectsAtIndexes:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSOrderedSet objectsAtIndexes:]"
        );
    }
    NSUInteger count = [self count];
    NSUInteger extra = ((NSUInteger (*)(id, SEL))objc_msgSend)(nil, sel_getUid("count"));
    NSUInteger lastIndex = [indexes lastIndex];
    NSUInteger bound = extra + count;
    if ((intptr_t)lastIndex < 0 || (lastIndex != kNotFound && bound <= lastIndex)) {
        if (bound == 0) {
            RAISE_ORDERED_SET_EXCEPTION(
                ONSRangeException,
                "*** %s: index %lu in index set beyond bounds for empty ordered set",
                "-[NSOrderedSet objectsAtIndexes:]",
                (unsigned long)lastIndex
            );
        }
        RAISE_ORDERED_SET_EXCEPTION(
            ONSRangeException,
            "*** %s: index %lu in index set beyond bounds [0 .. %lu]",
            "-[NSOrderedSet objectsAtIndexes:]",
            (unsigned long)lastIndex,
            (unsigned long)(bound - 1)
        );
    }
    NSUInteger indexCount = [indexes count];
    if ((OF_WORD_SHIFT(indexCount, 60)) != 0) {
        raiseOrderedSetBufferTooLarge(indexCount);
    }
    size_t capacity = 0;
    id *buffer = (id *)_OCFCreateArrayStorage(indexCount > 1 ? indexCount : 1, false, &capacity);
    NSUInteger position = 0;
    for (NSUInteger rangeIndex = 0; rangeIndex < [indexes rangeCount]; rangeIndex++) {
        OCFRange range = [indexes rangeAtIndex:rangeIndex];
        [self getObjects:buffer + position range:range];
        position += (NSUInteger)range.length;
    }
    id result;
    if (buffer != NULL) {
        for (NSUInteger index = 0; index < indexCount; index++) {
            [buffer[index] retain];
        }
        result = [[ONSArray alloc] _initByAdoptingBuffer:buffer count:indexCount size:indexCount];
    } else {
        result = [[ONSArray alloc] initWithObjects:NULL count:indexCount];
    }
    return [result autorelease];
}

// Native -[NSOrderedSet objectsAtIndexes:options:passingTest:], 0x1c02c8030. Non-external. The checks of
// -indexOfObjectAtIndexes:options:passingTest:; the read check; +[NSOrderedSet orderedSetWithArray:] (sent to
// the class symbol) of -objectsAtIndexes: of -indexesOfObjectsAtIndexes:options:passingTest: (a tail call).
- (id)objectsAtIndexes:(id)indexes
               options:(NSUInteger)options
           passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (indexes == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set cannot be nil",
            "-[NSOrderedSet objectsAtIndexes:options:passingTest:]"
        );
    }
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSOrderedSet objectsAtIndexes:options:passingTest:]"
        );
    }
    if (predicate == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSOrderedSet objectsAtIndexes:options:passingTest:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    id matching = [self indexesOfObjectsAtIndexes:indexes options:options passingTest:predicate];
    return [(Class)(void *)orderedSetClass orderedSetWithArray:[self objectsAtIndexes:matching]];
}

// Native -[NSOrderedSet objectsWithOptions:passingTest:], 0x1c02c82bc. Non-external. A nil predicate raises;
// the read check; +[NSOrderedSet orderedSetWithArray:] of -objectsAtIndexes: of
// -indexesOfObjectsWithOptions:passingTest: (a tail call).
- (id)objectsWithOptions:(NSUInteger)options
             passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSOrderedSet objectsWithOptions:passingTest:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    id matching = [self indexesOfObjectsWithOptions:options passingTest:predicate];
    return [(Class)(void *)orderedSetClass orderedSetWithArray:[self objectsAtIndexes:matching]];
}

// Native -[NSOrderedSet objectsPassingTest:], 0x1c02c8434. Non-external. A nil predicate raises; the read
// check; -objectsWithOptions:0 passingTest: (a tail call).
- (id)objectsPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSOrderedSet objectsPassingTest:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    return [self objectsWithOptions:0 passingTest:predicate];
}

// Native -[NSOrderedSet objectEnumerator], 0x1c02c8588. Non-external. The read check (the NSArray method has
// none); an autoreleased __NSFastEnumerationEnumerator (objc_alloc of its class reference, -initWithObject:
// self).
- (id)objectEnumerator {
    orderedSetTSANRead(self, __builtin_return_address(0));
    id enumerator = objc_alloc((Class)(void *)orderedSetFastEnumerationEnumeratorClass);
    return [[enumerator initWithObject:self] autorelease];
}

// Native -[NSOrderedSet reverseObjectEnumerator], 0x1c02c85e4. Non-external. The read check; an autoreleased
// __NSOrderedSetReverseEnumerator (objc_alloc, -initWithObject: self).
- (id)reverseObjectEnumerator {
    orderedSetTSANRead(self, __builtin_return_address(0));
    id enumerator = objc_alloc((Class)(void *)orderedSetReverseEnumeratorClass);
    return [[enumerator initWithObject:self] autorelease];
}

// Native -[NSOrderedSet reversedOrderedSet], 0x1c02c8640. Non-external. No read check; an autoreleased
// __NSOrderedSetReversed (objc_alloc, -initWithOrderedSet: self).
- (id)reversedOrderedSet {
    id reversed = objc_alloc((Class)(void *)orderedSetReversedClass);
    return [[reversed initWithOrderedSet:self] autorelease];
}

// Native -[NSOrderedSet sortedArrayFromRange:options:usingComparator:], 0x1c02c8670, with its block
// (___61-[NSOrderedSet sortedArrayFromRange:options:usingComparator:]_block_invoke, 0x1c02c8bbc: the
// comparator on the buffer's objects at the two indexes, a tail call). Non-external. The instructions of the
// NSArray method except one: a one-object receiver answers its own -array (a tail call) where the NSArray
// method answers the receiver retained and autoreleased. A nil comparator raises "*** %s: comparator cannot
// be nil" (buffer 0x0C); the read check; -count. An empty range or an empty ordered set answers +[NSArray
// array] (a tail call); then the range raise ("[0 .. %lu]" only, the count being nonzero); a one-object range
// answers
// +[NSArray arrayWithObject:] of it. Then the buffer limit; the objects are copied with -getObjects:range:
// into a stack buffer (at most 256) or _CFCreateArrayStorage; the sorted buffer is _CFCreateArrayStorage of
// the length; the index buffer is on the stack (at most 0x1000 entries) or malloc_type_malloc(length * 8,
// 0x100004000313F17). CFSortIndexes with the options and the block fills the index buffer; the sorted buffer
// takes the objects in that order; a heap index buffer is freed. The sorted objects are retained, adopted by
// a new NSArray and autoreleased; the heap object buffer is freed. When an exception unwinds through the
// sort, the heap object buffer and the sorted buffer are freed (the native landing pads;
// freeOrderedSetBufferOnExit stands for them).
- (id)sortedArrayFromRange:(OCFRange)range
                   options:(NSUInteger)options
           usingComparator:(NSInteger (^)(id object1, id object2))comparator {
    if (comparator == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: comparator cannot be nil",
            "-[NSOrderedSet sortedArrayFromRange:options:usingComparator:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (length == 0 || count == 0) {
        return [ONSArray array];
    }
    if (range.location < 0 || range.length < 0 || count < location + length) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSRangeException,
            "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
            "-[NSOrderedSet sortedArrayFromRange:options:usingComparator:]",
            (unsigned long)location,
            (unsigned long)length,
            (unsigned long)(count - 1)
        );
    }
    if (count == 1) {
        return [self array];
    }
    if (length == 1) {
        return [ONSArray arrayWithObject:[self objectAtIndex:location]];
    }
    if ((OF_WORD_SHIFT(length, 60)) != 0) {
        raiseOrderedSetBufferTooLarge(length);
    }
    id *objects = (id *)alloca((length < 0x101 ? length : 1) * sizeof(id));
    size_t objectCapacity = 0;
    __attribute__((cleanup(freeOrderedSetBufferOnExit))) id *heapObjects;
    if (length >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(length, false, &objectCapacity);
        heapObjects = objects;
    } else {
        heapObjects = NULL;
    }
    size_t sortedCapacity = 0;
    __attribute__((cleanup(freeOrderedSetBufferOnExit))) id *sorted =
        (id *)_OCFCreateArrayStorage(length, false, &sortedCapacity);
    [self getObjects:objects range:range];
    OCFIndex *stackIndexes = (OCFIndex *)alloca((length <= 0x1000 ? length : 1) * sizeof(OCFIndex));
    OCFIndex *indexBuffer = stackIndexes;
    if (length > 0x1000) {
        indexBuffer = (OCFIndex *)malloc_type_malloc(length * sizeof(OCFIndex), 0x100004000313F17ULL);
    }
    OCFSortIndexes(
        indexBuffer, (OCFIndex)length, options, ^OCFComparisonResult(OCFIndex first, OCFIndex second) {
          return (OCFComparisonResult)comparator(objects[first], objects[second]);
        }
    );
    for (NSUInteger index = 0; index != length; index++) {
        sorted[index] = objects[indexBuffer[index]];
    }
    if (stackIndexes != indexBuffer) {
        free(indexBuffer);
    }
    for (NSUInteger index = 0; index != length; index++) {
        [sorted[index] retain];
    }
    id result = [[ONSArray alloc] _initByAdoptingBuffer:sorted count:length size:length];
    sorted = NULL;
    return [result autorelease];
}

// Native -[NSOrderedSet sortedArrayWithOptions:usingComparator:], 0x1c02c8bd4. Non-external. A nil comparator
// raises; the read check; -sortedArrayFromRange: (0, -count) options: usingComparator: (a tail call).
- (id)sortedArrayWithOptions:(NSUInteger)options
             usingComparator:(NSInteger (^)(id object1, id object2))comparator {
    if (comparator == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: comparator cannot be nil",
            "-[NSOrderedSet sortedArrayWithOptions:usingComparator:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    return [self sortedArrayFromRange:(OCFRange){0, (OCFIndex)[self count]}
                              options:options
                      usingComparator:comparator];
}

// Native -[NSOrderedSet sortedArrayUsingComparator:], 0x1c02c8d3c. Non-external. As
// -sortedArrayWithOptions:usingComparator: with options 0.
- (id)sortedArrayUsingComparator:(NSInteger (^)(id object1, id object2))comparator {
    if (comparator == nil) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: comparator cannot be nil",
            "-[NSOrderedSet sortedArrayUsingComparator:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    return [self sortedArrayFromRange:(OCFRange){0, (OCFIndex)[self count]}
                              options:0
                      usingComparator:comparator];
}

// Native -[NSOrderedSet array], 0x1c02c8ea0. Non-external. The read check; an autoreleased
// __NSOrderedSetArrayProxy (objc_alloc, -initWithOrderedSet: self).
- (id)array {
    orderedSetTSANRead(self, __builtin_return_address(0));
    id proxy = objc_alloc((Class)(void *)orderedSetArrayProxyClass);
    return [[proxy initWithOrderedSet:self] autorelease];
}

// Native -[NSOrderedSet set], 0x1c02c8efc. Non-external. The read check; an autoreleased
// __NSOrderedSetSetProxy (objc_alloc, -initWithOrderedSet: self).
- (id)set {
    orderedSetTSANRead(self, __builtin_return_address(0));
    id proxy = objc_alloc((Class)(void *)orderedSetSetProxyClass);
    return [[proxy initWithOrderedSet:self] autorelease];
}

// Native -[NSOrderedSet descriptionWithLocale:indent:], 0x1c02c8f58. Non-external. No read check. The
// instructions of -[NSSet descriptionWithLocale:indent:] with -getObjects:range: (0, count) for
// -getObjects:count:: the level is capped at 100; -count and the buffer limit; the objects are copied into a
// stack buffer (at most 256 objects, at least one slot) or _CFCreateArrayStorage (freed at the end). A
// mutable string is made with kCFAllocatorSystemDefault, and inside an autorelease pool
// (objc_autoreleasePoolPush/Pop) each slot is replaced by its description: an NSString's
// -_stringRepresentation, else -descriptionWithLocale:indent: (level + 1) when it responds, else
// -descriptionWithLocale: when it responds, else -description; nil becomes "(null)". The indent string holds
// four spaces per level (only when the uncapped level is not 0); the result is the indent, "{(\n", then per
// object the indent, four spaces, the description and ",\n" (the last "\n"), then the indent and ")}". The
// indent string is released before the pool is popped; the result is autoreleased after it.
- (id)descriptionWithLocale:(id)locale indent:(NSUInteger)level {
    NSUInteger indentLevel = level < 100 ? level : 100;
    NSUInteger count = [self count];
    if ((OF_WORD_SHIFT(count, 60)) != 0) {
        raiseOrderedSetBufferTooLarge(count);
    }
    NSUInteger slots = count > 1 ? count : 1;
    id *objects = (id *)alloca((count < 0x101 ? slots : 1) * sizeof(id));
    size_t capacity = 0;
    id *heapBuffer;
    if (count >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(slots, false, &capacity);
        heapBuffer = objects;
    } else {
        heapBuffer = NULL;
    }
    [self getObjects:objects range:(OCFRange){0, (OCFIndex)count}];
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableStringRef result = OCFStringCreateMutable(allocator, 0);
    void *pool = objc_autoreleasePoolPush();
    id *slot = objects;
    for (NSUInteger remaining = count; remaining != 0; remaining--) {
        id object = *slot;
        id description;
        if (_ONSIsNSString(object)) {
            description = [object _stringRepresentation];
        } else if ([object respondsToSelector:@selector(descriptionWithLocale:indent:)]) {
            description = [object descriptionWithLocale:locale indent:indentLevel + 1];
        } else if ([object respondsToSelector:@selector(descriptionWithLocale:)]) {
            description = [object descriptionWithLocale:locale];
        } else {
            description = [object description];
        }
        *slot++ = description != nil ? description : (id)OF_STR("(null)");
    }
    OCFMutableStringRef indent = OCFStringCreateMutable(allocator, 0);
    if (level != 0) {
        do {
            OCFStringAppend(indent, OF_STR("    "));
        } while (--indentLevel != 0);
    }
    OCFStringAppend(result, indent);
    OCFStringAppend(result, OF_STR("{(\n"));
    id *line = objects;
    NSUInteger position = 1;
    for (NSUInteger remaining = count; remaining != 0; remaining--) {
        OCFStringAppend(result, indent);
        OCFStringAppend(result, OF_STR("    "));
        OCFStringAppend(result, (OCFStringRef)*line++);
        OCFStringAppend(result, position < count ? OF_STR(",\n") : OF_STR("\n"));
        position++;
    }
    OCFStringAppend(result, indent);
    OCFStringAppend(result, OF_STR(")}"));
    OCFRelease(indent);
    objc_autoreleasePoolPop(pool);
    id description = [(id)result autorelease];
    free(heapBuffer);
    return description;
}

// Native -[NSOrderedSet descriptionWithLocale:], 0x1c02c92a0. Non-external. The read check, then
// -descriptionWithLocale:indent: 0.
- (id)descriptionWithLocale:(id)locale {
    orderedSetTSANRead(self, __builtin_return_address(0));
    return [self descriptionWithLocale:locale indent:0];
}

// Native -[NSOrderedSet description], 0x1c02c92f8. Non-external. The read check, then
// -descriptionWithLocale:nil indent:0.
- (id)description {
    orderedSetTSANRead(self, __builtin_return_address(0));
    return [self descriptionWithLocale:nil indent:0];
}

// Native -[NSOrderedSet hash], 0x1c02c934c. Non-external. The read check; the count (a tail call of -count).
- (NSUInteger)hash {
    orderedSetTSANRead(self, __builtin_return_address(0));
    return [self count];
}

// Native -[NSOrderedSet isEqualToOrderedSet:], 0x1c02c9398. Non-external. An argument that is not nil and not
// an NSOrderedSet raises "*** %s: ordered set argument is not an NSOrderedSet" (buffer 0x0C); the read check.
// The instructions of -[NSArray isEqualToArray:]: the receiver itself answers YES and nil NO; different
// counts answer NO. Batches of at most 256 objects of both (-getObjects:range: into two stack buffers of 256,
// the receiver's first) are compared pairwise: different pointers ask [own isEqual:other]; a NO answers NO.
// The batch length shrinks to the remainder at the end.
- (BOOL)isEqualToOrderedSet:(id)other {
    if (other != nil && !(_ONSIsNSOrderedSet(other) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSOrderedSet isEqualToOrderedSet:]"
        );
    }
    orderedSetTSANRead(self, __builtin_return_address(0));
    if (other == self) {
        return YES;
    }
    if (other == nil) {
        return NO;
    }
    NSUInteger count = [self count];
    if ([other count] != count) {
        return NO;
    }
    if (count == 0) {
        return YES;
    }
    id otherObjects[256];
    id ownObjects[256];
    NSUInteger position = 0;
    NSUInteger batch = count < 0x100 ? count : 0x100;
    do {
        [self getObjects:ownObjects range:(OCFRange){(OCFIndex)position, (OCFIndex)batch}];
        [other getObjects:otherObjects range:(OCFRange){(OCFIndex)position, (OCFIndex)batch}];
        for (NSUInteger index = 0; index < batch; index++) {
            id own = ownObjects[index];
            id otherObject = otherObjects[index];
            if (own != otherObject && ![own isEqual:otherObject]) {
                return NO;
            }
        }
        position += batch;
        if (count < position + batch) {
            batch = count - position;
        }
    } while (count > position);
    return YES;
}

// Native -[NSOrderedSet isEqual:], 0x1c02c95d0. Non-external. The read check; the receiver itself answers
// YES, nil NO, a non-ordered set (_NSIsNSOrderedSet) NO; otherwise -isEqualToOrderedSet: (a tail call).
- (BOOL)isEqual:(id)object {
    orderedSetTSANRead(self, __builtin_return_address(0));
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    BOOL isOrderedSet = _ONSIsNSOrderedSet(object);
    if (!isOrderedSet) {
        return isOrderedSet;
    }
    return [self isEqualToOrderedSet:object];
}

// Native +[NSOrderedSet allocWithZone:], 0x1c02c9654. Non-external. NSOrderedSet itself answers
// __NSOrderedSetImmutablePlaceholder and NSMutableOrderedSet __NSOrderedSetMutablePlaceholder (tail calls;
// the receiver is compared with the class symbols, no message); any other class goes to super
// (objc_msgSendSuper2 with NSOrderedSet's metaclass).
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == (Class)(void *)orderedSetClass) {
        return __NSOrderedSetImmutablePlaceholder();
    }
    if (self == (Class)(void *)mutableOrderedSetClass) {
        return __NSOrderedSetMutablePlaceholder();
    }
    return [super allocWithZone:zone];
}

// Native +[NSOrderedSet newOrderedSetWithObjects:count:], 0x1c02c971c. Non-external. A NULL array with a
// non-zero count raises NSInvalidArgumentException "*** %s: pointer to objects array is NULL but length is
// %lu", a count of 2^61 or more "*** %s: count (%lu) of objects array is ridiculous", a nil object "*** %s:
// attempt to insert nil object from objects[%lu]" (buffer 0x16 each). NSOrderedSet itself answers
// __NSOrderedSetI_new and NSMutableOrderedSet __NSOrderedSetM_new, both taking over the objects' references
// (noRetain 1); any other class reaches __CFRequireConcreteImplementation (__CFLookUpClass("NSOrderedSet"))
// and answers nil.
+ (id)newOrderedSetWithObjects:(const id *)objects count:(NSUInteger)count {
    if (objects == NULL && count != 0) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "+[NSOrderedSet newOrderedSetWithObjects:count:]",
            (unsigned long)count
        );
    }
    if ((OF_WORD_SHIFT(count, 61)) != 0) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "+[NSOrderedSet newOrderedSetWithObjects:count:]",
            (unsigned long)count
        );
    }
    for (NSUInteger index = 0; index != count; index++) {
        if (objects[index] == nil) {
            RAISE_ORDERED_SET_EXCEPTION(
                ONSInvalidArgumentException,
                "*** %s: attempt to insert nil object from objects[%lu]",
                "+[NSOrderedSet newOrderedSetWithObjects:count:]",
                (unsigned long)index
            );
        }
    }
    if (self == (Class)(void *)orderedSetClass) {
        return __NSOrderedSetI_new(objects, count, YES);
    }
    if (self == (Class)(void *)mutableOrderedSetClass) {
        return __NSOrderedSetM_new(objects, count, YES);
    }
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSOrderedSet)), self, _cmd);
    return nil;
}

// Native -[NSOrderedSet initWithArray:range:copyItems:], 0x1c02c9a2c. Non-external. A non-NSArray raises
// NSInvalidArgumentException "*** %s: array argument is not an NSArray"; a negative location or length, or a
// range past the count, is the range raise. The range is copied with -getObjects:range: into a temporary
// buffer (a stack buffer up to 256 objects, at least one slot, else _CFCreateArrayStorage after the buffer
// limit); with flag (and objects) each entry is replaced by its -copyWithZone: NULL, -initWithObjects:count:
// is sent, the copies are released, and the heap buffer is freed. The instructions of
// -[NSSet initWithArray:range:copyItems:].
- (id)initWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)flag {
    if (array != nil && !(_ONSIsNSArray(array) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: array argument is not an NSArray",
            "-[NSOrderedSet initWithArray:range:copyItems:]"
        );
    }
    NSUInteger count = [array count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (range.location < 0 || range.length < 0 || count < location + length) {
        RAISE_ORDERED_SET_RANGE_EXCEPTION(
            "-[NSOrderedSet initWithArray:range:copyItems:]", location, length, count
        );
    }
    if ((OF_WORD_SHIFT(length, 60)) != 0) {
        raiseOrderedSetBufferTooLarge(length);
    }
    NSUInteger slots = length > 1 ? length : 1;
    id *objects = (id *)alloca((length < 0x101 ? slots : 1) * sizeof(id));
    size_t capacity = 0;
    id *heapBuffer = NULL;
    if (length >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(slots, false, &capacity);
        heapBuffer = objects;
    }
    [array getObjects:objects range:range];
    BOOL copyItems = length != 0 && flag;
    if (copyItems) {
        for (NSUInteger index = 0; index != length; index++) {
            objects[index] = [objects[index] copyWithZone:NULL];
        }
    }
    self = [self initWithObjects:objects count:length];
    if (copyItems) {
        for (NSUInteger index = 0; index != length; index++) {
            [objects[index] release];
        }
    }
    free(heapBuffer);
    return self;
}

// Native -[NSOrderedSet initWithArray:range:], 0x1c02c9e74. Non-external. Without copying (a tail call).
- (id)initWithArray:(id)array range:(OCFRange)range {
    return [self initWithArray:array range:range copyItems:NO];
}

// Native -[NSOrderedSet initWithArray:copyItems:], 0x1c02c9e7c. Non-external. The whole array.
- (id)initWithArray:(id)array copyItems:(BOOL)flag {
    return [self initWithArray:array range:(OCFRange){0, (OCFIndex)[array count]} copyItems:flag];
}

// Native -[NSOrderedSet initWithArray:], 0x1c02c9ec4. Non-external. The whole array, without copying.
- (id)initWithArray:(id)array {
    return [self initWithArray:array range:(OCFRange){0, (OCFIndex)[array count]} copyItems:NO];
}

// Native -[NSOrderedSet initWithObject:], 0x1c02c9f00. Non-external. -initWithObjects:count: with the one
// object, from a stack slot.
- (id)initWithObject:(id)object {
    id objects[1] = {object};
    return [self initWithObjects:objects count:1];
}

// Native -[NSOrderedSet initWithObjects:], 0x1c02c9f54. Non-external. The objects up to the first nil are
// counted (the buffer limit applies to a nonzero count); a temporary buffer (a stack buffer up to 256
// objects, at least one slot, else _CFCreateArrayStorage) receives the first object and the rest of the
// variadic list; -initWithObjects:count: is sent and the heap buffer freed. Every class takes this path (no
// special case, unlike the NSSet method).
- (id)initWithObjects:(id)firstObject, ... {
    va_list args;
    va_start(args, firstObject);
    NSUInteger count = 0;
    if (firstObject != nil) {
        va_list counter;
        va_copy(counter, args);
        id next = firstObject;
        do {
            count++;
            next = va_arg(counter, id);
        } while (next != nil);
        va_end(counter);
        if ((OF_WORD_SHIFT(count, 60)) != 0) {
            raiseOrderedSetBufferTooLarge(count);
        }
    }
    NSUInteger slots = count > 1 ? count : 1;
    id *objects = (id *)alloca((count < 0x101 ? slots : 1) * sizeof(id));
    size_t capacity = 0;
    id *heapBuffer = NULL;
    if (count >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(slots, false, &capacity);
        heapBuffer = objects;
    }
    if (count != 0) {
        objects[0] = firstObject;
        for (NSUInteger index = 1; index != count; index++) {
            objects[index] = va_arg(args, id);
        }
    }
    va_end(args);
    self = [self initWithObjects:objects count:count];
    free(heapBuffer);
    return self;
}

// Native -[NSOrderedSet initWithOrderedSet:range:copyItems:], 0x1c02ca11c. Non-external. The pattern of
// -initWithArray:range:copyItems: with _NSIsNSOrderedSet ("*** %s: ordered set argument is not an
// NSOrderedSet") and the ordered set's -getObjects:range:.
- (id)initWithOrderedSet:(id)orderedSet range:(OCFRange)range copyItems:(BOOL)flag {
    if (orderedSet != nil && !(_ONSIsNSOrderedSet(orderedSet) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSOrderedSet initWithOrderedSet:range:copyItems:]"
        );
    }
    NSUInteger count = [orderedSet count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (range.location < 0 || range.length < 0 || count < location + length) {
        RAISE_ORDERED_SET_RANGE_EXCEPTION(
            "-[NSOrderedSet initWithOrderedSet:range:copyItems:]", location, length, count
        );
    }
    if ((OF_WORD_SHIFT(length, 60)) != 0) {
        raiseOrderedSetBufferTooLarge(length);
    }
    NSUInteger slots = length > 1 ? length : 1;
    id *objects = (id *)alloca((length < 0x101 ? slots : 1) * sizeof(id));
    size_t capacity = 0;
    id *heapBuffer = NULL;
    if (length >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(slots, false, &capacity);
        heapBuffer = objects;
    }
    [orderedSet getObjects:objects range:range];
    BOOL copyItems = length != 0 && flag;
    if (copyItems) {
        for (NSUInteger index = 0; index != length; index++) {
            objects[index] = [objects[index] copyWithZone:NULL];
        }
    }
    self = [self initWithObjects:objects count:length];
    if (copyItems) {
        for (NSUInteger index = 0; index != length; index++) {
            [objects[index] release];
        }
    }
    free(heapBuffer);
    return self;
}

// Native -[NSOrderedSet initWithOrderedSet:range:], 0x1c02ca564. Non-external. Without copying (a tail call).
- (id)initWithOrderedSet:(id)orderedSet range:(OCFRange)range {
    return [self initWithOrderedSet:orderedSet range:range copyItems:NO];
}

// Native -[NSOrderedSet initWithOrderedSet:copyItems:], 0x1c02ca56c. Non-external. The whole ordered set.
- (id)initWithOrderedSet:(id)orderedSet copyItems:(BOOL)flag {
    return [self initWithOrderedSet:orderedSet
                              range:(OCFRange){0, (OCFIndex)[orderedSet count]}
                          copyItems:flag];
}

// Native -[NSOrderedSet initWithOrderedSet:], 0x1c02ca5b4. Non-external. The whole ordered set, without
// copying.
- (id)initWithOrderedSet:(id)orderedSet {
    return [self initWithOrderedSet:orderedSet
                              range:(OCFRange){0, (OCFIndex)[orderedSet count]}
                          copyItems:NO];
}

// Native -[NSOrderedSet initWithSet:copyItems:], 0x1c02ca5f0. Non-external. A non-NSSet raises "*** %s: set
// argument is not an NSSet" (no forgiveness). The set's objects are copied with -getObjects:count: into a
// temporary buffer of its -count (the buffer limit first); with flag (and objects) each is replaced by its
// -copyWithZone: NULL, -initWithObjects:count: is sent, the copies are released, and the heap buffer freed.
- (id)initWithSet:(id)set copyItems:(BOOL)flag {
    if (set != nil && !(_ONSIsNSSet(set) & 1)) {
        RAISE_ORDERED_SET_EXCEPTION(
            ONSInvalidArgumentException,
            "*** %s: set argument is not an NSSet",
            "-[NSOrderedSet initWithSet:copyItems:]"
        );
    }
    NSUInteger count = [set count];
    if ((OF_WORD_SHIFT(count, 60)) != 0) {
        raiseOrderedSetBufferTooLarge(count);
    }
    NSUInteger slots = count > 1 ? count : 1;
    id *objects = (id *)alloca((count < 0x101 ? slots : 1) * sizeof(id));
    size_t capacity = 0;
    id *heapBuffer = NULL;
    if (count >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(slots, false, &capacity);
        heapBuffer = objects;
    }
    [set getObjects:objects count:count];
    BOOL copyItems = count != 0 && flag;
    if (copyItems) {
        for (NSUInteger index = 0; index != count; index++) {
            objects[index] = [objects[index] copyWithZone:NULL];
        }
    }
    self = [self initWithObjects:objects count:count];
    if (copyItems) {
        for (NSUInteger index = 0; index != count; index++) {
            [objects[index] release];
        }
    }
    free(heapBuffer);
    return self;
}

// Native -[NSOrderedSet initWithSet:], 0x1c02ca880. Non-external. Without copying (a tail call).
- (id)initWithSet:(id)set {
    return [self initWithSet:set copyItems:NO];
}

// Native +[NSOrderedSet orderedSetWithObjects:count:], 0x1c02ca888. Non-external. objc_alloc,
// -initWithObjects:count:, autoreleased.
+ (id)orderedSetWithObjects:(const id *)objects count:(NSUInteger)count {
    return [[[self alloc] initWithObjects:objects count:count] autorelease];
}

// Native +[NSOrderedSet orderedSetWithObject:], 0x1c02ca8b8. Non-external. __createOrderedSet of objc_alloc,
// autoreleased.
+ (id)orderedSetWithObject:(id)object {
    return [__createOrderedSet([self alloc], object) autorelease];
}

// Native ___createOrderedSet, 0x1c02ca8e0. Non-external (a static of this unit). -initWithObjects:count: of
// the one object, from a stack slot.
static id __createOrderedSet(id orderedSet, id object) {
    id objects[1] = {object};
    return [orderedSet initWithObjects:objects count:1];
}

// Native +[NSOrderedSet orderedSet], 0x1c02ca934. Non-external. objc_alloc, -initWithObjects:NULL count:0,
// autoreleased.
+ (id)orderedSet {
    return [[[self alloc] initWithObjects:NULL count:0] autorelease];
}

// Native +[NSOrderedSet orderedSetWithObjects:], 0x1c02ca954. Non-external. The variadic pattern of
// -initWithObjects: (no special case for any class); objc_alloc, -initWithObjects:count:, autoreleased, and
// then the heap buffer freed.
+ (id)orderedSetWithObjects:(id)firstObject, ... {
    va_list args;
    va_start(args, firstObject);
    NSUInteger count = 0;
    if (firstObject != nil) {
        va_list counter;
        va_copy(counter, args);
        id next = firstObject;
        do {
            count++;
            next = va_arg(counter, id);
        } while (next != nil);
        va_end(counter);
        if ((OF_WORD_SHIFT(count, 60)) != 0) {
            raiseOrderedSetBufferTooLarge(count);
        }
    }
    NSUInteger slots = count > 1 ? count : 1;
    id *objects = (id *)alloca((count < 0x101 ? slots : 1) * sizeof(id));
    size_t capacity = 0;
    id *heapBuffer = NULL;
    if (count >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(slots, false, &capacity);
        heapBuffer = objects;
    }
    if (count != 0) {
        objects[0] = firstObject;
        for (NSUInteger index = 1; index != count; index++) {
            objects[index] = va_arg(args, id);
        }
    }
    va_end(args);
    id orderedSet = [[[self alloc] initWithObjects:objects count:count] autorelease];
    free(heapBuffer);
    return orderedSet;
}

// Native +[NSOrderedSet orderedSetWithArray:range:copyItems:], 0x1c02cab28. Non-external. objc_alloc,
// -initWithArray:range:copyItems:, autoreleased.
+ (id)orderedSetWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)flag {
    return [[[self alloc] initWithArray:array range:range copyItems:flag] autorelease];
}

// Native +[NSOrderedSet orderedSetWithArray:range:], 0x1c02cab70. Non-external. Without copying.
+ (id)orderedSetWithArray:(id)array range:(OCFRange)range {
    return [[[self alloc] initWithArray:array range:range copyItems:NO] autorelease];
}

// Native +[NSOrderedSet orderedSetWithArray:copyItems:], 0x1c02cabb4. Non-external. The whole array (its
// -count read after objc_alloc).
+ (id)orderedSetWithArray:(id)array copyItems:(BOOL)flag {
    id orderedSet = [self alloc];
    return [[orderedSet initWithArray:array range:(OCFRange){0, (OCFIndex)[array count]}
                            copyItems:flag] autorelease];
}

// Native +[NSOrderedSet orderedSetWithArray:], 0x1c02cac04. Non-external. The whole array, without copying.
+ (id)orderedSetWithArray:(id)array {
    id orderedSet = [self alloc];
    return [[orderedSet initWithArray:array range:(OCFRange){0, (OCFIndex)[array count]}
                            copyItems:NO] autorelease];
}

// Native +[NSOrderedSet orderedSetWithOrderedSet:range:copyItems:], 0x1c02cac48. Non-external. objc_alloc,
// -initWithOrderedSet:range:copyItems:, autoreleased.
+ (id)orderedSetWithOrderedSet:(id)orderedSet range:(OCFRange)range copyItems:(BOOL)flag {
    return [[[self alloc] initWithOrderedSet:orderedSet range:range copyItems:flag] autorelease];
}

// Native +[NSOrderedSet orderedSetWithOrderedSet:range:], 0x1c02cac90. Non-external. Without copying.
+ (id)orderedSetWithOrderedSet:(id)orderedSet range:(OCFRange)range {
    return [[[self alloc] initWithOrderedSet:orderedSet range:range copyItems:NO] autorelease];
}

// Native +[NSOrderedSet orderedSetWithOrderedSet:copyItems:], 0x1c02cacd4. Non-external. The whole ordered
// set (its -count read after objc_alloc).
+ (id)orderedSetWithOrderedSet:(id)orderedSet copyItems:(BOOL)flag {
    id newOrderedSet = [self alloc];
    return [[newOrderedSet initWithOrderedSet:orderedSet
                                        range:(OCFRange){0, (OCFIndex)[orderedSet count]}
                                    copyItems:flag] autorelease];
}

// Native +[NSOrderedSet orderedSetWithOrderedSet:], 0x1c02cad24. Non-external. The whole ordered set, without
// copying.
+ (id)orderedSetWithOrderedSet:(id)orderedSet {
    id newOrderedSet = [self alloc];
    return [[newOrderedSet initWithOrderedSet:orderedSet
                                        range:(OCFRange){0, (OCFIndex)[orderedSet count]}
                                    copyItems:NO] autorelease];
}

// Native +[NSOrderedSet orderedSetWithSet:copyItems:], 0x1c02cad68. Non-external. objc_alloc,
// -initWithSet:copyItems:, autoreleased.
+ (id)orderedSetWithSet:(id)set copyItems:(BOOL)flag {
    return [[[self alloc] initWithSet:set copyItems:flag] autorelease];
}

// Native +[NSOrderedSet orderedSetWithSet:], 0x1c02cad98. Non-external. Without copying.
+ (id)orderedSetWithSet:(id)set {
    return [[[self alloc] initWithSet:set copyItems:NO] autorelease];
}

// Native -[NSOrderedSet copyWithZone:], 0x1c02cadc4. Non-external. The read check; +[NSOrderedSet
// allocWithZone:] (sent to the class symbol), then -initWithOrderedSet:self range:(0, -count) copyItems:NO (a
// tail call; the items are never copied, and __NSCollectionsShouldCopy is not asked).
- (id)copyWithZone:(struct _NSZone *)zone {
    orderedSetTSANRead(self, __builtin_return_address(0));
    id copy = [(Class)(void *)orderedSetClass allocWithZone:zone];
    return [copy initWithOrderedSet:self range:(OCFRange){0, (OCFIndex)[self count]} copyItems:NO];
}

// Native -[NSOrderedSet mutableCopyWithZone:], 0x1c02cae40. Non-external. As -copyWithZone: with
// +[NSMutableOrderedSet allocWithZone:].
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    orderedSetTSANRead(self, __builtin_return_address(0));
    id copy = [(Class)(void *)mutableOrderedSetClass allocWithZone:zone];
    return [copy initWithOrderedSet:self range:(OCFRange){0, (OCFIndex)[self count]} copyItems:NO];
}

// Native +[NSOrderedSet supportsSecureCoding], 0x1c02caebc. Non-external. Answers YES.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSOrderedSet initWithCoder:], 0x1c02caec4. Non-external. Answers nil (the receiver is not
// released).
- (id)initWithCoder:(id)coder {
    return nil;
}

// Native -[NSOrderedSet encodeWithCoder:], 0x1c02caecc. Non-external. Does nothing.
- (void)encodeWithCoder:(id)coder {
}

// Native -[NSOrderedSet count], 0x1c031221c, with OUTLINED_FUNCTION_0_55 (0x1c02caed0:
// __CFLookUpClass("NSOrderedSet")), _1_39 (0x1c02caee4: the tail call of __CFRequireConcreteImplementation
// with the receiver and selector) and _2_33 (0x1c02caef0: the epilogue). Non-external. The abstract method;
// answers 0.
- (NSUInteger)count {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSOrderedSet)), self, _cmd);
    return 0;
}

// Native -[NSOrderedSet indexOfObject:], 0x1c031223c, with the same outlined fragments. Non-external. The
// abstract method; answers 0.
- (NSUInteger)indexOfObject:(id)object {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSOrderedSet)), self, _cmd);
    return 0;
}

// Native -[NSOrderedSet objectAtIndex:], 0x1c031225c, with the same outlined fragments. Non-external. The
// abstract method; answers nil.
- (id)objectAtIndex:(NSUInteger)index {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSOrderedSet)), self, _cmd);
    return nil;
}

// Native -[NSOrderedSet initWithObjects:count:], 0x1c031227c, with the same outlined fragments. Non-external.
// The abstract method; answers nil.
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSOrderedSet)), self, _cmd);
    return nil;
}

@end
