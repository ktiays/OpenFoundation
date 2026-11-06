//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSArray.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFSortFunctions.h"
#include "CoreFoundation/Objective-C/OCFNSArrayHelpers.h"
#include "CoreFoundation/Objective-C/OCFNSArrayI.h"
#include "CoreFoundation/Objective-C/OCFNSArrayM.h"
#include "CoreFoundation/Objective-C/OCFNSCollectionsShouldCopy.h"
#include "CoreFoundation/Objective-C/OCFNSEmptyCollections.h"
#include "CoreFoundation/Objective-C/OCFNSPlaceholderArray.h"
#include "CoreFoundation/Objective-C/OCFNSSingleObjectArrayI.h"
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

// Inferred organization: the NSArray methods lie together in the iOS Simulator __text section
// (0x1c0276c1c-0x1c027fe78, followed by the unit's OUTLINED_FUNCTION_0_37, _1_25 and _2_21 at
// 0x1c027fe7c-0x1c027fea0), with the three primitive methods that only raise (-count, -objectAtIndex:,
// -initWithObjects:count:; 0x1c030f030-0x1c030f08c) in the cold region; following the owner's decision, the
// class has its own translation unit. The image defines no category on NSArray. Objective-C with manual
// reference counting (objc_retain, objc_release, objc_autorelease and their register variants on explicit
// buffers). The os_log argument packing of the raises is the compiler's __os_log_helper_1_2_3_8_32_8_0_8_0
// (0x1c01512e0) and __os_log_helper_1_2_4_8_32_8_0_8_0_8_0 (0x1c0151660), or the same stores inline; both are
// restored as __builtin_os_log_format. Block literals keep the native captures; their descriptors
// (unk_2227A81D0, unk_2227A81F8, unk_2227A8220, unk_22279D5B0, ___block_descriptor_48_e11_q24__0_8_16l,
// ___block_descriptor_40_e11_q24__0_8_16l) lie outside the extracted image or are compiler-emitted, and the
// layout clang computes stands for them. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full).
// iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not
// written yet).
//
// The native unit inlines four checks at many methods; each is spelled once here:
// - the ThreadSanitizer read check (arrayTSANRead): the hook __cf_tsanReadFunction, when set, is called with
//   the receiver, the method's return address and __CFTSANTagMutableArray, the same check
//   Objective-C/OCFBridgedClasses.h spells as tsanRead;
// - the logged raise (RAISE_LOGGED_EXCEPTION): _os_log_pack_size of the format's argument buffer, the pack on
//   the stack filled with errno 0 and the image's header, the arguments packed, the reason made with
//   kCFAllocatorSystemDefault and autoreleased through _CFAutoreleasePoolAddObject, and +[NSException
//   exceptionWithName:reason:userInfo:osLogPack:size:] (no user info) thrown;
// - the range raise (RAISE_RANGE_EXCEPTION): NSRangeException "*** %s: range {%lu, %lu} extends beyond bounds
//   for empty array" (buffer 0x20) for an empty array, else "*** %s: range {%lu, %lu} extends beyond bounds
//   [0 .. %lu]" (buffer 0x2A) with the count minus one;
// - the temporary buffer limit (raiseTemporaryBufferTooLarge): a count of 2^60 or more raises
//   NSGenericException "*** attempt to create a temporary id buffer which is too large or with a negative
//   count
//   (%lu) -- possibly data is corrupt" (reason with the NULL allocator, +exceptionWithName:reason:userInfo:,
//   the reason released after the exception is made).

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

// The class symbols the methods compare against or send to directly. NSArray's own symbol is this unit's
// class, referenced by its assembler name for the comparisons +allocWithZone: makes without messaging.
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char arrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSArray));
extern char mutableArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char setClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSSet));
extern char placeholderArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSPlaceholderArray));
extern char
    fastEnumerationEnumeratorClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSFastEnumerationEnumerator));
extern char arrayReverseEnumeratorClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSArrayReverseEnumerator));
extern char arrayReversedClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSArrayReversed));

// The messages the methods send to objects of other classes (index sets, sets, enumerators, exceptions),
// declared for the sends only; no protocol object is emitted.
@protocol ArrayMessages

+ (id)setWithArray:(id)array;
+ (id)indexSetWithIndexesInRange:(OCFRange)range;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;
- (NSUInteger)lastIndex;
- (void)__forwardEnumerateRanges:(void (^)(OCFRange range, BOOL *stop))block;
- (void)getObjects:(id *)objects count:(NSUInteger)count;
- (id)_stringRepresentation;
- (id)initWithObject:(id)object;

@end

// The fast-enumeration state the native -countByEnumeratingWithState:objects:count: fills (the layout of
// NSFastEnumerationState; ArrayEnumerationState is a local name).
typedef struct {
    unsigned long state;
    id *itemsPtr;
    unsigned long *mutationsPtr;
    unsigned long extra[5];
} ArrayEnumerationState;

// The ThreadSanitizer read check; arrayTSANRead is a local descriptive name.
static inline void arrayTSANRead(const void *object, void *returnAddress) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanReadFunction;
    if (hook != NULL) {
        hook(object, returnAddress, __OCFTSANTagMutableArray);
    }
}

#define RAISE_LOGGED_EXCEPTION(name, bufferSize, format, ...)                                                \
    do {                                                                                                     \
        size_t packSize = _os_log_pack_size(bufferSize);                                                     \
        void *pack = alloca((packSize + 15) & ~(size_t)15);                                                  \
        uint8_t *packBuffer = _os_log_pack_fill(pack, packSize, 0, &__dso_handle, format);                   \
        __builtin_os_log_format(packBuffer, format, __VA_ARGS__);                                            \
        OCFStringRef reason =                                                                                \
            OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR(format), __VA_ARGS__);        \
        objc_exception_throw([(id)exceptionClass                                                             \
            exceptionWithName:(id)(name)                                                                     \
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)                                     \
                     userInfo:nil                                                                            \
                    osLogPack:pack                                                                           \
                         size:packSize]);                                                                    \
    } while (0)

#define RAISE_RANGE_EXCEPTION(method, location, length, count)                                               \
    do {                                                                                                     \
        if ((count) == 0) {                                                                                  \
            RAISE_LOGGED_EXCEPTION(                                                                          \
                ONSRangeException,                                                                           \
                0x20,                                                                                        \
                "*** %s: range {%lu, %lu} extends beyond bounds for empty array",                            \
                method,                                                                                      \
                location,                                                                                    \
                length                                                                                       \
            );                                                                                               \
        }                                                                                                    \
        RAISE_LOGGED_EXCEPTION(                                                                              \
            ONSRangeException,                                                                               \
            0x2A,                                                                                            \
            "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",                                     \
            method,                                                                                          \
            location,                                                                                        \
            length,                                                                                          \
            (count) - 1                                                                                      \
        );                                                                                                   \
    } while (0)

// raiseTemporaryBufferTooLarge is a local descriptive name.
static __attribute__((noreturn)) void raiseTemporaryBufferTooLarge(NSUInteger count) {
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
    id exception = [(id)exceptionClass exceptionWithName:(id)ONSGenericException
                                                  reason:(id)reason
                                                userInfo:nil];
    OCFRelease(reason);
    objc_exception_throw(exception);
}

// The cleanup of the buffers the native code frees when an exception unwinds through it (landing pads that
// free the variable's current value); freeBufferOnExit is a local descriptive name.
static void freeBufferOnExit(id **buffer) {
    free(*buffer);
}

// Native ___createArray, 0x1c027e560. Non-external (a static of this unit). -initWithObjects:count: of the
// one object, from a stack slot.
static id __createArray(id array, id object) {
    id objects[1] = {object};
    return [array initWithObjects:objects count:1];
}

@implementation ONSArray

// Native -[NSArray _cfTypeID], 0x1c0276c1c. Non-external. The CFArray type ID, 19.
- (unsigned long)_cfTypeID {
    return 19;
}

// Native -[NSArray isNSArray__], 0x1c0276c24. Non-external.
- (BOOL)isNSArray__ {
    return YES;
}

// Native -[NSArray allObjects], 0x1c0276c2c. Non-external. The read check, -count and the buffer limit; the
// objects are copied with -getObjects:range: into _CFCreateArrayStorage of the count (at least one, not
// zeroed), each retained (objc_retain, no tagged-pointer test), and the buffer is adopted by a new NSArray
// (objc_alloc, -_initByAdoptingBuffer:count:size: with the count twice); a NULL buffer (not produced by
// _CFCreateArrayStorage, which crashes instead) would make -initWithObjects:NULL count:. Autoreleased.
- (id)allObjects {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferTooLarge(count);
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

// Native -[NSArray componentsJoinedByString:], 0x1c0276dd0. Non-external. A separator that is not nil and not
// an NSString (_NSIsNSString) raises NSInvalidArgumentException "*** %s: string argument is not an NSString"
// (buffer 0x0C); then the read check. A nil separator is the empty constant string. A mutable string
// (kCFAllocatorSystemDefault) gets each object's -description in fast-enumeration order, followed by the
// separator while the running position plus one is below the count read before the walk. Autoreleased.
- (id)componentsJoinedByString:(id)separator {
    if (separator != nil && !(_ONSIsNSString(separator) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: string argument is not an NSString",
            "-[NSArray componentsJoinedByString:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    OCFStringRef joiner = separator != nil ? (OCFStringRef)separator : OF_STR("");
    NSUInteger count = [self count];
    OCFMutableStringRef result = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    NSUInteger position = 0;
    for (id object in self) {
        OCFStringAppend(result, (OCFStringRef)[object description]);
        if (position + 1 < count) {
            OCFStringAppend(result, joiner);
        }
        position++;
    }
    return [(id)result autorelease];
}

// Native -[NSArray containsObject:inRange:], 0x1c027702c. Non-external. The read check, -count and the range
// raise (location or length with bit 63 set, or a count below their sum); then each object of the range
// (-objectAtIndex:) is the object itself or [object isEqual:].
- (BOOL)containsObject:(id)object inRange:(OCFRange)range {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (range.location < 0 || range.length < 0 || count < location + length) {
        RAISE_RANGE_EXCEPTION("-[NSArray containsObject:inRange:]", location, length, count);
    }
    for (; length != 0; length--, location++) {
        id candidate = [self objectAtIndex:location];
        if (candidate == object || [object isEqual:candidate]) {
            return YES;
        }
    }
    return NO;
}

// Native -[NSArray containsObject:], 0x1c0277278. Non-external. The read check; fast enumeration until an
// object is the argument itself or [object isEqual:].
- (BOOL)containsObject:(id)object {
    arrayTSANRead(self, __builtin_return_address(0));
    for (id candidate in self) {
        if (candidate == object || ([object isEqual:candidate] & 1)) {
            return YES;
        }
    }
    return NO;
}

// Native -[NSArray containsObjectIdenticalTo:inRange:], 0x1c02773a0. Non-external. As
// -containsObject:inRange: by identity.
- (BOOL)containsObjectIdenticalTo:(id)object inRange:(OCFRange)range {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (range.location < 0 || range.length < 0 || count < location + length) {
        RAISE_RANGE_EXCEPTION("-[NSArray containsObjectIdenticalTo:inRange:]", location, length, count);
    }
    for (; length != 0; length--, location++) {
        if ([self objectAtIndex:location] == object) {
            return YES;
        }
    }
    return NO;
}

// Native -[NSArray containsObjectIdenticalTo:], 0x1c02775e4. Non-external. The read check; fast enumeration
// by identity.
- (BOOL)containsObjectIdenticalTo:(id)object {
    arrayTSANRead(self, __builtin_return_address(0));
    for (id candidate in self) {
        if (candidate == object) {
            return YES;
        }
    }
    return NO;
}

// Native -[NSArray countByEnumeratingWithState:objects:count:], 0x1c0277700. Non-external. A NULL buffer with
// a nonzero length raises NSInvalidArgumentException "*** %s: pointer to objects array is NULL but length is
// %lu", a length of 2^61 or more "*** %s: count (%lu) of objects array is ridiculous" (both buffer 0x16);
// then the read check. A state of -1 answers 0. The first call points mutationsPtr at extra[0] and stores
// -count there; later calls read the count from extra[0]. itemsPtr is the buffer; the batch is the smaller of
// the length and the count minus the state (unsigned), copied with -getObjects:range: from the state when
// nonzero; the state advances by the batch, or becomes -1 when that reaches the count.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    if (buffer == NULL && length != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[NSArray countByEnumeratingWithState:objects:count:]",
            length
        );
    }
    if ((length >> 61) != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[NSArray countByEnumeratingWithState:objects:count:]",
            length
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    ArrayEnumerationState *enumeration = (ArrayEnumerationState *)state;
    if (enumeration->state == (unsigned long)-1) {
        return 0;
    }
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

// Native -[NSArray countForObject:inRange:], 0x1c027796c. Non-external. The read check, -count and the range
// raise; then the objects of the range that are the object itself or [object isEqual:] are counted.
- (NSUInteger)countForObject:(id)object inRange:(OCFRange)range {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (range.location < 0 || range.length < 0 || count < location + length) {
        RAISE_RANGE_EXCEPTION("-[NSArray countForObject:inRange:]", location, length, count);
    }
    NSUInteger matches = 0;
    for (; length != 0; length--, location++) {
        id candidate = [self objectAtIndex:location];
        if (candidate == object || [object isEqual:candidate]) {
            matches++;
        }
    }
    return matches;
}

// Native -[NSArray countForObject:], 0x1c0277bb8. Non-external. The read check; fast enumeration counting the
// objects that are the argument itself or [object isEqual:].
- (NSUInteger)countForObject:(id)object {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger matches = 0;
    for (id candidate in self) {
        if (candidate == object || [object isEqual:candidate]) {
            matches++;
        }
    }
    return matches;
}

// Native -[NSArray descriptionWithLocale:indent:], 0x1c0277cf4, with its block
// (___40-[NSArray descriptionWithLocale:indent:]_block_invoke, 0x1c027809c). Non-external. No read check. The
// level is capped at 100 for the nested calls and the indentation; -count and the buffer limit. The objects
// are copied with -getObjects:range: into a stack buffer (at most 256 objects, at least one slot) or, above
// 256, _CFCreateArrayStorage (freed at the end). A __block int (-1) caches whether NSString exists
// (objc_lookUpClass of OF_NAME_STRING(ONSString), the prefixed class in the prefixed build). Inside an
// autorelease pool (objc_autoreleasePoolPush/Pop) each slot is replaced by its description: an NSString's
// -_stringRepresentation; a dictionary's or array's -descriptionWithLocale:indent: (level + 1); NSData's
// -description; anything else's -description through the block, which answers its -_stringRepresentation
// when the app is linked on or after version 6 and NSString exists, and the description otherwise; nil
// becomes "(null)". The text is the indentation (four spaces per level, only when the uncapped level is not
// 0), "(\n", then per object the indentation, four spaces, the description and ",\n" (or "\n" after the
// last), then the indentation and ")". The indentation string (kCFAllocatorSystemDefault) is released, the
// pool popped, the result autoreleased, the __block variable disposed, and the heap buffer freed.
- (id)descriptionWithLocale:(id)locale indent:(NSUInteger)level {
    NSUInteger cappedLevel = level < 100 ? level : 100;
    NSUInteger count = [self count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferTooLarge(count);
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
    id result;
    {
        __block int hasStringClass = -1;
        id (^representation)(id) = ^id(id description) {
          if (hasStringClass == -1) {
              hasStringClass = objc_lookUpClass(OF_NAME_STRING(ONSString)) != Nil;
          }
          if (_OCFExecutableLinkedOnOrAfter(6) && hasStringClass != 0) {
              return [description _stringRepresentation];
          }
          return description;
        };
        OCFMutableStringRef text = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
        void *pool = objc_autoreleasePoolPush();
        id *slot = objects;
        for (NSUInteger remaining = count; remaining != 0; remaining--) {
            id object = *slot;
            id description;
            if (_ONSIsNSString(object)) {
                description = [object _stringRepresentation];
            } else if ((_ONSIsNSDictionary(object) & 1) || _ONSIsNSArray(object)) {
                description = [object descriptionWithLocale:locale indent:cappedLevel + 1];
            } else if (_ONSIsNSData(object)) {
                description = [object description];
            } else {
                description = representation([object description]);
            }
            *slot++ = description == nil ? (id)OF_STR("(null)") : description;
        }
        OCFMutableStringRef indentation = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
        if (level != 0) {
            NSUInteger steps = cappedLevel;
            do {
                OCFStringAppend(indentation, OF_STR("    "));
                steps--;
            } while (steps != 0);
        }
        OCFStringAppend(text, indentation);
        OCFStringAppend(text, OF_STR("(\n"));
        id *line = objects;
        NSUInteger position = 1;
        for (NSUInteger remaining = count; remaining != 0; remaining--) {
            OCFStringAppend(text, indentation);
            OCFStringAppend(text, OF_STR("    "));
            OCFStringAppend(text, (OCFStringRef)*line++);
            OCFStringAppend(text, position < count ? OF_STR(",\n") : OF_STR("\n"));
            position++;
        }
        OCFStringAppend(text, indentation);
        OCFStringAppend(text, OF_STR(")"));
        OCFRelease(indentation);
        objc_autoreleasePoolPop(pool);
        result = [(id)text autorelease];
    }
    free(heapBuffer);
    return result;
}

// Native -[NSArray descriptionWithLocale:], 0x1c0278120. Non-external. The read check, then
// -descriptionWithLocale:indent: 0.
- (id)descriptionWithLocale:(id)locale {
    arrayTSANRead(self, __builtin_return_address(0));
    return [self descriptionWithLocale:locale indent:0];
}

// Native -[NSArray description], 0x1c0278178. Non-external. The read check, then -descriptionWithLocale:nil
// indent:0.
- (id)description {
    arrayTSANRead(self, __builtin_return_address(0));
    return [self descriptionWithLocale:nil indent:0];
}

// Native -[NSArray enumerateObjectsAtIndexes:options:usingBlock:], 0x1c02781cc. Non-external. A nil index set
// raises NSInvalidArgumentException "*** %s: index set cannot be nil", one that is not an NSIndexSet "*** %s:
// index set argument is not an NSIndexSet", a nil block "*** %s: block cannot be nil" (buffer 0x0C each);
// then the read check. Reverse drops bits 0 and 60; bit 60 is set; then __NSArrayParameterCheckIterate and
// __NSArrayEnumerate (a tail call).
- (void)enumerateObjectsAtIndexes:(id)indexes
                          options:(NSUInteger)options
                       usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    if (indexes == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set cannot be nil",
            "-[NSArray enumerateObjectsAtIndexes:options:usingBlock:]"
        );
    }
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSArray enumerateObjectsAtIndexes:options:usingBlock:]"
        );
    }
    if (block == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: block cannot be nil",
            "-[NSArray enumerateObjectsAtIndexes:options:usingBlock:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    if (options & 2) {
        options &= 0xEFFFFFFFFFFFFFFEULL;
    }
    __NSArrayParameterCheckIterate(self, _cmd, block, options | 0x1000000000000000ULL, indexes);
    __NSArrayEnumerate(self, block, options | 0x1000000000000000ULL, indexes);
}

// Native -[NSArray enumerateObjectsWithOptions:usingBlock:], 0x1c0278464. Non-external. A nil block raises
// "*** %s: block cannot be nil"; the read check; reverse drops bit 0; __NSArrayParameterCheckIterate and
// __NSArrayEnumerate with no index set.
- (void)enumerateObjectsWithOptions:(NSUInteger)options
                         usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    if (block == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: block cannot be nil",
            "-[NSArray enumerateObjectsWithOptions:usingBlock:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    if (options & 2) {
        options &= ~(NSUInteger)1;
    }
    __NSArrayParameterCheckIterate(self, _cmd, block, options, nil);
    __NSArrayEnumerate(self, block, options, nil);
}

// Native -[NSArray enumerateObjectsUsingBlock:], 0x1c02785e8. Non-external. A nil block raises; the read
// check; -enumerateObjectsWithOptions:0 usingBlock:.
- (void)enumerateObjectsUsingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    if (block == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: block cannot be nil",
            "-[NSArray enumerateObjectsUsingBlock:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    [self enumerateObjectsWithOptions:0 usingBlock:block];
}

// Native -[NSArray firstObject], 0x1c027873c. Non-external. The read check; nil (the zero count) for an empty
// array, else -objectAtIndex: 0 (a tail call).
- (id)firstObject {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (count != 0) {
        return [self objectAtIndex:0];
    }
    return (id)count;
}

// Native -[NSArray firstObjectCommonWithArray:], 0x1c02787a4. Non-external. The read check; an argument that
// is not nil and not an NSArray raises "*** %s: array argument is not an NSArray" (buffer 0x0C). +[NSSet
// setWithArray:] of the argument; the first object in fast-enumeration order the set -containsObject:, or
// nil.
- (id)firstObjectCommonWithArray:(id)otherArray {
    arrayTSANRead(self, __builtin_return_address(0));
    if (otherArray != nil && !(_ONSIsNSArray(otherArray) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: array argument is not an NSArray",
            "-[NSArray firstObjectCommonWithArray:]"
        );
    }
    id set = [(Class)(void *)setClass setWithArray:otherArray];
    for (id object in self) {
        if ([set containsObject:object] & 1) {
            return object;
        }
    }
    return nil;
}

// Native -[NSArray getObjects:range:], 0x1c02789b4. Non-external. A NULL buffer with a nonzero length raises
// NSInvalidArgumentException "*** %s: pointer to objects array is NULL but length is %lu", a length of 2^61
// or more "*** %s: count (%lu) of objects array is ridiculous" (buffer 0x16 each); the read check; -count; a
// location with bit 63 clear whose end is within the count copies each object with -objectAtIndex:; anything
// else is the range raise.
- (void)getObjects:(id *)objects range:(OCFRange)range {
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (objects == NULL && length != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[NSArray getObjects:range:]",
            length
        );
    }
    if ((length >> 61) != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[NSArray getObjects:range:]",
            length
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if ((intptr_t)location >= 0 && count >= location + length) {
        for (; length != 0; length--) {
            *objects++ = [self objectAtIndex:location++];
        }
        return;
    }
    RAISE_RANGE_EXCEPTION("-[NSArray getObjects:range:]", location, length, count);
}

// Native -[NSArray getObjects:], 0x1c0278d38. Non-external. The read check; -count; a NULL buffer for a
// nonempty array raises "*** %s: pointer to objects array is NULL but length is %lu", a count of 2^61 or more
// "*** %s: count (%lu) of objects array is ridiculous"; then -getObjects:range: (0, -count read again).
- (void)getObjects:(id *)objects {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (objects == NULL && count != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[NSArray getObjects:]",
            count
        );
    }
    if ((count >> 61) != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[NSArray getObjects:]",
            count
        );
    }
    [self getObjects:objects range:(OCFRange){0, (OCFIndex)[self count]}];
}

// Native -[NSArray hash], 0x1c0278f44. Non-external. The read check; the count (a tail call of -count).
- (NSUInteger)hash {
    arrayTSANRead(self, __builtin_return_address(0));
    return [self count];
}

// Native -[NSArray indexOfObject:inRange:], 0x1c0278f90. Non-external. The read check, -count and the range
// raise; an empty range answers NSNotFound; otherwise the first index of the range whose object is the
// argument itself or [object isEqual:].
- (NSUInteger)indexOfObject:(id)object inRange:(OCFRange)range {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if ((intptr_t)location < 0 || (intptr_t)length < 0 || count < location + length) {
        RAISE_RANGE_EXCEPTION("-[NSArray indexOfObject:inRange:]", location, length, count);
    }
    if (length == 0) {
        return kNotFound;
    }
    for (NSUInteger index = location;; index++) {
        id candidate = [self objectAtIndex:index];
        if (candidate == object || [object isEqual:candidate]) {
            return index;
        }
        if (--length == 0) {
            return kNotFound;
        }
    }
}

// Native -[NSArray indexOfObject:], 0x1c02791ec. Non-external. The read check; the running index of the first
// object in fast-enumeration order that is the argument itself or [object isEqual:], or NSNotFound.
- (NSUInteger)indexOfObject:(id)object {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger index = 0;
    for (id candidate in self) {
        if (candidate == object || ([object isEqual:candidate] & 1)) {
            return index;
        }
        index++;
    }
    return kNotFound;
}

// Native -[NSArray indexOfObjectIdenticalTo:inRange:], 0x1c027932c. Non-external. As -indexOfObject:inRange:
// by identity.
- (NSUInteger)indexOfObjectIdenticalTo:(id)object inRange:(OCFRange)range {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if ((intptr_t)location < 0 || (intptr_t)length < 0 || count < location + length) {
        RAISE_RANGE_EXCEPTION("-[NSArray indexOfObjectIdenticalTo:inRange:]", location, length, count);
    }
    if (length == 0) {
        return kNotFound;
    }
    while ([self objectAtIndex:location] != object) {
        location++;
        if (--length == 0) {
            return kNotFound;
        }
    }
    return location;
}

// Native -[NSArray indexOfObjectIdenticalTo:], 0x1c0279564. Non-external. As -indexOfObject: by identity.
- (NSUInteger)indexOfObjectIdenticalTo:(id)object {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger index = 0;
    for (id candidate in self) {
        if (candidate == object) {
            return index;
        }
        index++;
    }
    return kNotFound;
}

// Native -[NSArray indexesOfObject:inRange:], 0x1c0279698, with its block
// (___35-[NSArray indexesOfObject:inRange:]_block_invoke, 0x1c0279900: YES for the object itself, else
// [object isEqual:candidate]). Non-external. The read check, -count and the range raise; then
// -indexesOfObjectsAtIndexes: (+[NSIndexSet indexSetWithIndexesInRange:] of the range, the class from
// __CFLookUpClass) options:0 passingTest: the block.
- (id)indexesOfObject:(id)object inRange:(OCFRange)range {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (range.location < 0 || range.length < 0 || count < location + length) {
        RAISE_RANGE_EXCEPTION("-[NSArray indexesOfObject:inRange:]", location, length, count);
    }
    id indexes = [(Class)__CFLookUpClass(OF_NAME_STRING(ONSIndexSet)) indexSetWithIndexesInRange:range];
    return [self indexesOfObjectsAtIndexes:indexes
                                   options:0
                               passingTest:^BOOL(
                                   id candidate,
                                   __attribute__((unused)) NSUInteger index,
                                   __attribute__((unused)) BOOL *stop
                               ) {
                                 if (candidate == object) {
                                     return YES;
                                 }
                                 return [object isEqual:candidate];
                               }];
}

// Native -[NSArray indexesOfObject:], 0x1c027991c, with its block (___27-[NSArray
// indexesOfObject:]_block_invoke, 0x1c02799dc, the test of -indexesOfObject:inRange:). Non-external. The read
// check; -indexesOfObjectsWithOptions:0 passingTest: the block.
- (id)indexesOfObject:(id)object {
    arrayTSANRead(self, __builtin_return_address(0));
    return [self indexesOfObjectsWithOptions:0
                                 passingTest:^BOOL(
                                     id candidate,
                                     __attribute__((unused)) NSUInteger index,
                                     __attribute__((unused)) BOOL *stop
                                 ) {
                                   if (candidate == object) {
                                       return YES;
                                   }
                                   return [object isEqual:candidate];
                                 }];
}

// Native -[NSArray indexesOfObjectIdenticalTo:inRange:], 0x1c02799f8, with its block
// (___46-[NSArray indexesOfObjectIdenticalTo:inRange:]_block_invoke, 0x1c0279c60: identity). Non-external. As
// -indexesOfObject:inRange: by identity.
- (id)indexesOfObjectIdenticalTo:(id)object inRange:(OCFRange)range {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (range.location < 0 || range.length < 0 || count < location + length) {
        RAISE_RANGE_EXCEPTION("-[NSArray indexesOfObjectIdenticalTo:inRange:]", location, length, count);
    }
    id indexes = [(Class)__CFLookUpClass(OF_NAME_STRING(ONSIndexSet)) indexSetWithIndexesInRange:range];
    return [self indexesOfObjectsAtIndexes:indexes
                                   options:0
                               passingTest:^BOOL(
                                   id candidate,
                                   __attribute__((unused)) NSUInteger index,
                                   __attribute__((unused)) BOOL *stop
                               ) {
                                 return candidate == object;
                               }];
}

// Native -[NSArray indexesOfObjectIdenticalTo:], 0x1c0279c70, with its block
// (___38-[NSArray indexesOfObjectIdenticalTo:]_block_invoke, 0x1c0279d30: identity). Non-external. The read
// check; -indexesOfObjectsWithOptions:0 passingTest: the block.
- (id)indexesOfObjectIdenticalTo:(id)object {
    arrayTSANRead(self, __builtin_return_address(0));
    return [self indexesOfObjectsWithOptions:0
                                 passingTest:^BOOL(
                                     id candidate,
                                     __attribute__((unused)) NSUInteger index,
                                     __attribute__((unused)) BOOL *stop
                                 ) {
                                   return candidate == object;
                                 }];
}

// Native -[NSArray indexOfObject:inSortedRange:options:usingComparator:], 0x1c0279d40. Non-external. A nil
// object raises "*** %s: object cannot be nil", a nil comparator "*** %s: comparator cannot be nil" (buffer
// 0x0C each); the read check, -count and the range raise. Both NSBinarySearchingFirstEqual (0x100) and
// NSBinarySearchingLastEqual (0x200) raise NSInvalidArgumentException "%@: both NSBinarySearchingFirstEqual
// and NSBinarySearchingLastEqual options cannot be specified" (__CFExceptionProem; reason with
// kCFAllocatorSystemDefault, autoreleased; +exceptionWithName:reason:userInfo:). An empty range answers the
// location for NSBinarySearchingInsertionIndex (0x400), else NSNotFound. Above 256 objects, the last object
// is compared first (comparator(last, object)): ascending, or same without FirstEqual, answers the end with
// InsertionIndex, else NSNotFound when ascending and the last index when same; then comparator(object,
// first): ascending answers the location with InsertionIndex and NSNotFound without, same without LastEqual
// answers the location. The search probes location + step / 2 for step = 2 << flsl(length), halving (signed)
// while the step was above 1; probes past the end are skipped. A same result with neither option answers the
// probe; a probe moves the location when ascending, or when same with LastEqual; a same result is remembered.
// At the end, the location plus one when the last result was ascending (or same with LastEqual) answers
// InsertionIndex; otherwise the location plus one when the last result was ascending, or NSNotFound when no
// probe was same.
- (NSUInteger)indexOfObject:(id)object
              inSortedRange:(OCFRange)range
                    options:(NSUInteger)options
            usingComparator:(NSInteger (^)(id object1, id object2))comparator {
    if (object == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: object cannot be nil",
            "-[NSArray indexOfObject:inSortedRange:options:usingComparator:]"
        );
    }
    if (comparator == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: comparator cannot be nil",
            "-[NSArray indexOfObject:inSortedRange:options:usingComparator:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    NSUInteger end = location + length;
    if (range.location < 0 || range.length < 0 || count < end) {
        RAISE_RANGE_EXCEPTION(
            "-[NSArray indexOfObject:inSortedRange:options:usingComparator:]", location, length, count
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
        objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
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

// Native -[NSArray indexOfObjectAtIndexes:options:passingTest:], 0x1c027a284. Non-external. The index set and
// predicate checks of -enumerateObjectsAtIndexes:options:usingBlock: ("*** %s: predicate cannot be nil"); the
// read check; reverse drops bits 0, 60 and 61; bits 60 and 61 are set; __NSArrayParameterCheckIterate and
// __NSArrayGetIndexPassingTest (a tail call).
- (NSUInteger)indexOfObjectAtIndexes:(id)indexes
                             options:(NSUInteger)options
                         passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (indexes == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set cannot be nil",
            "-[NSArray indexOfObjectAtIndexes:options:passingTest:]"
        );
    }
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSArray indexOfObjectAtIndexes:options:passingTest:]"
        );
    }
    if (predicate == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: predicate cannot be nil",
            "-[NSArray indexOfObjectAtIndexes:options:passingTest:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    if (options & 2) {
        options &= 0xCFFFFFFFFFFFFFFEULL;
    }
    __NSArrayParameterCheckIterate(self, _cmd, predicate, options | 0x3000000000000000ULL, indexes);
    return __NSArrayGetIndexPassingTest(self, predicate, options | 0x3000000000000000ULL, indexes);
}

// Native -[NSArray indexOfObjectWithOptions:passingTest:], 0x1c027a51c. Non-external. A nil predicate raises;
// the read check; reverse drops bits 0 and 61; bit 61 is set; __NSArrayParameterCheckIterate and
// __NSArrayGetIndexPassingTest with no index set.
- (NSUInteger)indexOfObjectWithOptions:(NSUInteger)options
                           passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: predicate cannot be nil",
            "-[NSArray indexOfObjectWithOptions:passingTest:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    if (options & 2) {
        options &= 0xDFFFFFFFFFFFFFFEULL;
    }
    __NSArrayParameterCheckIterate(self, _cmd, predicate, options | 0x2000000000000000ULL, nil);
    return __NSArrayGetIndexPassingTest(self, predicate, options | 0x2000000000000000ULL, nil);
}

// Native -[NSArray indexOfObjectPassingTest:], 0x1c027a6a4. Non-external. A nil predicate raises; the read
// check; -indexOfObjectWithOptions:0 passingTest:.
- (NSUInteger)indexOfObjectPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: predicate cannot be nil",
            "-[NSArray indexOfObjectPassingTest:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    return [self indexOfObjectWithOptions:0 passingTest:predicate];
}

// Native -[NSArray indexesOfObjectsAtIndexes:options:passingTest:], 0x1c027a7f8. Non-external. The checks of
// -indexOfObjectAtIndexes:options:passingTest:; the read check; reverse drops bits 0, 60 and 62; bits 60 and
// 62 are set; __NSArrayParameterCheckIterate and __NSArrayGetIndexesPassingTest (a tail call).
- (id)indexesOfObjectsAtIndexes:(id)indexes
                        options:(NSUInteger)options
                    passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (indexes == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set cannot be nil",
            "-[NSArray indexesOfObjectsAtIndexes:options:passingTest:]"
        );
    }
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSArray indexesOfObjectsAtIndexes:options:passingTest:]"
        );
    }
    if (predicate == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: predicate cannot be nil",
            "-[NSArray indexesOfObjectsAtIndexes:options:passingTest:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    if (options & 2) {
        options &= 0xAFFFFFFFFFFFFFFEULL;
    }
    __NSArrayParameterCheckIterate(self, _cmd, predicate, options | 0x5000000000000000ULL, indexes);
    return __NSArrayGetIndexesPassingTest(self, predicate, options | 0x5000000000000000ULL, indexes);
}

// Native -[NSArray indexesOfObjectsWithOptions:passingTest:], 0x1c027aa98. Non-external. A nil predicate
// raises; the read check; reverse drops bits 0 and 62; bit 62 is set; __NSArrayParameterCheckIterate and
// __NSArrayGetIndexesPassingTest with no index set.
- (id)indexesOfObjectsWithOptions:(NSUInteger)options
                      passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: predicate cannot be nil",
            "-[NSArray indexesOfObjectsWithOptions:passingTest:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    if (options & 2) {
        options &= 0xBFFFFFFFFFFFFFFEULL;
    }
    __NSArrayParameterCheckIterate(self, _cmd, predicate, options | 0x4000000000000000ULL, nil);
    return __NSArrayGetIndexesPassingTest(self, predicate, options | 0x4000000000000000ULL, nil);
}

// Native -[NSArray indexesOfObjectsPassingTest:], 0x1c027ac20. Non-external. A nil predicate raises; the read
// check; -indexesOfObjectsWithOptions:0 passingTest:.
- (id)indexesOfObjectsPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: predicate cannot be nil",
            "-[NSArray indexesOfObjectsPassingTest:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    return [self indexesOfObjectsWithOptions:0 passingTest:predicate];
}

// Native -[NSArray isEqualToArray:], 0x1c027ad74. Non-external. An argument that is not nil and not an
// NSArray raises "*** %s: array argument is not an NSArray" (buffer 0x0C); the read check. The receiver
// itself answers YES and nil NO; different counts answer NO. Batches of at most 256 objects of both arrays
// (-getObjects:range: into two stack buffers of 256, the receiver's first) are compared pairwise: different
// pointers ask [own isEqual:other]; a NO answers NO. The batch length shrinks to the remainder at the end.
- (BOOL)isEqualToArray:(id)otherArray {
    if (otherArray != nil && !(_ONSIsNSArray(otherArray) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: array argument is not an NSArray",
            "-[NSArray isEqualToArray:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    if (otherArray == self) {
        return YES;
    }
    if (otherArray == nil) {
        return NO;
    }
    NSUInteger count = [self count];
    if ([otherArray count] != count) {
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
        [otherArray getObjects:otherObjects range:(OCFRange){(OCFIndex)position, (OCFIndex)batch}];
        for (NSUInteger index = 0; index < batch; index++) {
            id own = ownObjects[index];
            id other = otherObjects[index];
            if (own != other && ![own isEqual:other]) {
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

// Native -[NSArray isEqual:], 0x1c027afac. Non-external. The read check; the receiver itself answers YES, nil
// NO, a non-array (_NSIsNSArray) NO; otherwise -isEqualToArray: (a tail call).
- (BOOL)isEqual:(id)object {
    arrayTSANRead(self, __builtin_return_address(0));
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    BOOL isArray = _ONSIsNSArray(object);
    if (!isArray) {
        return isArray;
    }
    return [self isEqualToArray:object];
}

// Native -[NSArray lastObject], 0x1c027b030. Non-external. The read check; nil for an empty array, else
// -objectAtIndex: of the count minus one (a tail call).
- (id)lastObject {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (count != 0) {
        return [self objectAtIndex:count - 1];
    }
    return (id)count;
}

// Native -[NSArray makeObjectsPerformSelector:], 0x1c027b098. Non-external. A NULL selector sends
// -doesNotRecognizeSelector: with it (NULL); the read check; above 128 objects, fast enumeration sends
// -performSelector:; otherwise the objects are first copied into a stack buffer (at least one slot) with
// -objectAtIndex: and then sent -performSelector: in order.
- (void)makeObjectsPerformSelector:(SEL)selector {
    if (selector == NULL) {
        [self doesNotRecognizeSelector:selector];
    }
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (count > 0x80) {
        for (id object in self) {
            [object performSelector:selector];
        }
        return;
    }
    id *objects = (id *)alloca((count > 1 ? count : 1) * sizeof(id));
    if (count != 0) {
        for (NSUInteger index = 0; index != count; index++) {
            objects[index] = [self objectAtIndex:index];
        }
        for (NSUInteger remaining = count; remaining != 0; remaining--) {
            [*objects++ performSelector:selector];
        }
    }
}

// Native -[NSArray makeObjectsPerformSelector:withObject:], 0x1c027b240. Non-external. As
// -makeObjectsPerformSelector: with -performSelector:withObject:.
- (void)makeObjectsPerformSelector:(SEL)selector withObject:(id)argument {
    if (selector == NULL) {
        [self doesNotRecognizeSelector:selector];
    }
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (count > 0x80) {
        for (id object in self) {
            [object performSelector:selector withObject:argument];
        }
        return;
    }
    id *objects = (id *)alloca((count > 1 ? count : 1) * sizeof(id));
    if (count != 0) {
        for (NSUInteger index = 0; index != count; index++) {
            objects[index] = [self objectAtIndex:index];
        }
        for (NSUInteger remaining = count; remaining != 0; remaining--) {
            [*objects++ performSelector:selector withObject:argument];
        }
    }
}

// Native -[NSArray objectAtIndexedSubscript:], 0x1c027b3f4. Non-external. The read check; -objectAtIndex: (a
// tail call).
- (id)objectAtIndexedSubscript:(NSUInteger)index {
    arrayTSANRead(self, __builtin_return_address(0));
    return [self objectAtIndex:index];
}

// Native -[NSArray objectsAtIndexes:], 0x1c027b448, with its block
// (___28-[NSArray objectsAtIndexes:]_block_invoke, 0x1c027ba9c). Non-external. A nil index set raises "***
// %s: index set cannot be nil", one that is not an NSIndexSet "*** %s: index set argument is not an
// NSIndexSet" (buffer 0x0C each); the read check; the NSIndexSet check again (the same raise). The bound is
// -count plus -count sent to nil (0, as the native code does); a last index with bit 63 set, or neither
// NSNotFound nor below the bound, raises NSRangeException "*** %s: index %lu in index set beyond bounds for
// empty array" (buffer 0x16) for a zero bound, else "*** %s: index %lu in index set beyond bounds [0 .. %lu]"
// (buffer 0x20, the bound minus one). Then the index set's -count, the buffer limit and _CFCreateArrayStorage
// (at least one slot); the set's -__forwardEnumerateRanges: calls the block for each range, which copies the
// range with -getObjects:range: at the __block position and advances it by the range's length. The objects
// are retained (objc_retain) and adopted by a new NSArray (-_initByAdoptingBuffer:count:size:, the set's
// count twice), or -initWithObjects:NULL count: for a NULL buffer; autoreleased; the __block variable
// disposed.
- (id)objectsAtIndexes:(id)indexes {
    if (indexes == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set cannot be nil",
            "-[NSArray objectsAtIndexes:]"
        );
    }
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSArray objectsAtIndexes:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSArray objectsAtIndexes:]"
        );
    }
    NSUInteger count = [self count];
    NSUInteger extra = ((NSUInteger (*)(id, SEL))objc_msgSend)(nil, sel_getUid("count"));
    NSUInteger lastIndex = [indexes lastIndex];
    NSUInteger bound = extra + count;
    if ((intptr_t)lastIndex < 0 || (lastIndex != kNotFound && bound <= lastIndex)) {
        if (bound == 0) {
            RAISE_LOGGED_EXCEPTION(
                ONSRangeException,
                0x16,
                "*** %s: index %lu in index set beyond bounds for empty array",
                "-[NSArray objectsAtIndexes:]",
                lastIndex
            );
        }
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x20,
            "*** %s: index %lu in index set beyond bounds [0 .. %lu]",
            "-[NSArray objectsAtIndexes:]",
            lastIndex,
            bound - 1
        );
    }
    NSUInteger indexCount = [indexes count];
    if ((indexCount >> 60) != 0) {
        raiseTemporaryBufferTooLarge(indexCount);
    }
    size_t capacity = 0;
    id *buffer = (id *)_OCFCreateArrayStorage(indexCount > 1 ? indexCount : 1, false, &capacity);
    id result;
    {
        __block NSUInteger position = 0;
        [indexes __forwardEnumerateRanges:^(OCFRange range, __attribute__((unused)) BOOL *stop) {
          [self getObjects:buffer + position range:range];
          position += (NSUInteger)range.length;
        }];
        if (buffer != NULL) {
            for (NSUInteger index = 0; index < indexCount; index++) {
                [buffer[index] retain];
            }
            result = [[ONSArray alloc] _initByAdoptingBuffer:buffer count:indexCount size:indexCount];
        } else {
            result = [[ONSArray alloc] initWithObjects:NULL count:indexCount];
        }
        result = [result autorelease];
    }
    return result;
}

// Native -[NSArray objectAtIndexes:options:passingTest:], 0x1c027baf4. Non-external. The checks of
// -indexOfObjectAtIndexes:options:passingTest:; the read check; that method's answer, nil for NSNotFound,
// else -objectAtIndex: of it (a tail call).
- (id)objectAtIndexes:(id)indexes
              options:(NSUInteger)options
          passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (indexes == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set cannot be nil",
            "-[NSArray objectAtIndexes:options:passingTest:]"
        );
    }
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSArray objectAtIndexes:options:passingTest:]"
        );
    }
    if (predicate == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: predicate cannot be nil",
            "-[NSArray objectAtIndexes:options:passingTest:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger index = [self indexOfObjectAtIndexes:indexes options:options passingTest:predicate];
    if (index == kNotFound) {
        return nil;
    }
    return [self objectAtIndex:index];
}

// Native -[NSArray objectWithOptions:passingTest:], 0x1c027bdac. Non-external. A nil predicate raises; the
// read check; -indexOfObjectWithOptions:passingTest:, nil for NSNotFound, else -objectAtIndex:.
- (id)objectWithOptions:(NSUInteger)options
            passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: predicate cannot be nil",
            "-[NSArray objectWithOptions:passingTest:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger index = [self indexOfObjectWithOptions:options passingTest:predicate];
    if (index == kNotFound) {
        return nil;
    }
    return [self objectAtIndex:index];
}

// Native -[NSArray objectPassingTest:], 0x1c027bf4c. Non-external. A nil predicate raises; the read check;
// -objectWithOptions:0 passingTest: (a tail call).
- (id)objectPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: predicate cannot be nil",
            "-[NSArray objectPassingTest:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    return [self objectWithOptions:0 passingTest:predicate];
}

// Native -[NSArray objectsAtIndexes:options:passingTest:], 0x1c027c0a0. Non-external. The checks of
// -indexOfObjectAtIndexes:options:passingTest:; the read check; -objectsAtIndexes: of
// -indexesOfObjectsAtIndexes:options:passingTest: (a tail call).
- (id)objectsAtIndexes:(id)indexes
               options:(NSUInteger)options
           passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (indexes == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set cannot be nil",
            "-[NSArray objectsAtIndexes:options:passingTest:]"
        );
    }
    if (!(_ONSIsNSIndexSet(indexes) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: index set argument is not an NSIndexSet",
            "-[NSArray objectsAtIndexes:options:passingTest:]"
        );
    }
    if (predicate == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: predicate cannot be nil",
            "-[NSArray objectsAtIndexes:options:passingTest:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    return [self objectsAtIndexes:[self indexesOfObjectsAtIndexes:indexes
                                                          options:options
                                                      passingTest:predicate]];
}

// Native -[NSArray objectsWithOptions:passingTest:], 0x1c027c318. Non-external. A nil predicate raises; the
// read check; -objectsAtIndexes: of -indexesOfObjectsWithOptions:passingTest: (a tail call).
- (id)objectsWithOptions:(NSUInteger)options
             passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: predicate cannot be nil",
            "-[NSArray objectsWithOptions:passingTest:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    return [self objectsAtIndexes:[self indexesOfObjectsWithOptions:options passingTest:predicate]];
}

// Native -[NSArray objectsPassingTest:], 0x1c027c47c. Non-external. A nil predicate raises; the read check;
// -objectsWithOptions:0 passingTest: (a tail call).
- (id)objectsPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate {
    if (predicate == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: predicate cannot be nil",
            "-[NSArray objectsPassingTest:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    return [self objectsWithOptions:0 passingTest:predicate];
}

// Native -[NSArray objectEnumerator], 0x1c027c5d0. Non-external. No read check. An autoreleased
// __NSFastEnumerationEnumerator (objc_alloc of its class reference, -initWithObject: self).
- (id)objectEnumerator {
    id enumerator = objc_alloc((Class)(void *)fastEnumerationEnumeratorClass);
    return [[enumerator initWithObject:self] autorelease];
}

// Native -[NSArray reverseObjectEnumerator], 0x1c027c600. Non-external. No read check. An autoreleased
// __NSArrayReverseEnumerator (objc_alloc, -initWithObject: self).
- (id)reverseObjectEnumerator {
    id enumerator = objc_alloc((Class)(void *)arrayReverseEnumeratorClass);
    return [[enumerator initWithObject:self] autorelease];
}

// Native -[NSArray reversedArray], 0x1c027c630. Non-external. The read check; an autoreleased
// __NSArrayReversed (objc_alloc, -initWithArray: self).
- (id)reversedArray {
    arrayTSANRead(self, __builtin_return_address(0));
    id reversed = objc_alloc((Class)(void *)arrayReversedClass);
    return [[reversed initWithArray:self] autorelease];
}

// Native -[NSArray sortedArrayFromRange:options:usingComparator:], 0x1c027c68c, with its block
// (___56-[NSArray sortedArrayFromRange:options:usingComparator:]_block_invoke, 0x1c027cbf4: the comparator on
// the buffer's objects at the two indexes, a tail call). Non-external. A nil comparator raises "*** %s:
// comparator cannot be nil" (buffer 0x0C); the read check; -count. An empty range or an empty array answers
// +[NSArray array] (a tail call); then the range raise; a one-object array answers the receiver retained and
// autoreleased; a one-object range answers +[NSArray arrayWithObject:] of it. Then the buffer limit; the
// objects are copied with -getObjects:range: into a stack buffer (at most 256) or _CFCreateArrayStorage; the
// sorted buffer is _CFCreateArrayStorage of the length; the index buffer is on the stack (at most 0x1000
// entries) or malloc_type_malloc(length * 8, 0x100004000313F17). CFSortIndexes with the options and the block
// fills the index buffer; the sorted buffer takes the objects in that order; a heap index buffer is freed.
// The sorted objects are retained, adopted by a new NSArray and autoreleased; the heap object buffer is
// freed. When an exception unwinds through the sort, the heap object buffer and the sorted buffer are freed
// (the native landing pads; freeBufferOnExit stands for them).
- (id)sortedArrayFromRange:(OCFRange)range
                   options:(NSUInteger)options
           usingComparator:(NSInteger (^)(id object1, id object2))comparator {
    if (comparator == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: comparator cannot be nil",
            "-[NSArray sortedArrayFromRange:options:usingComparator:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (length == 0 || count == 0) {
        return [ONSArray array];
    }
    if (range.location < 0 || range.length < 0 || count < location + length) {
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x2A,
            "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
            "-[NSArray sortedArrayFromRange:options:usingComparator:]",
            location,
            length,
            count - 1
        );
    }
    if (count == 1) {
        return [[self retain] autorelease];
    }
    if (length == 1) {
        return [ONSArray arrayWithObject:[self objectAtIndex:location]];
    }
    if ((length >> 60) != 0) {
        raiseTemporaryBufferTooLarge(length);
    }
    id *objects = (id *)alloca((length < 0x101 ? length : 1) * sizeof(id));
    size_t objectCapacity = 0;
    __attribute__((cleanup(freeBufferOnExit))) id *heapObjects;
    if (length >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(length, false, &objectCapacity);
        heapObjects = objects;
    } else {
        heapObjects = NULL;
    }
    size_t sortedCapacity = 0;
    __attribute__((cleanup(freeBufferOnExit))) id *sorted =
        (id *)_OCFCreateArrayStorage(length, false, &sortedCapacity);
    [self getObjects:objects range:range];
    OCFIndex *stackIndexes = (OCFIndex *)alloca((length <= 0x1000 ? length : 1) * sizeof(OCFIndex));
    OCFIndex *indexBuffer = stackIndexes;
    if (length > 0x1000) {
        indexBuffer = (OCFIndex *)malloc_type_malloc(length << 3, 0x100004000313F17ULL);
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

// Native -[NSArray sortedArrayWithOptions:usingComparator:], 0x1c027cc0c. Non-external. A nil comparator
// raises; the read check; -sortedArrayFromRange: (0, -count) options: usingComparator: (a tail call).
- (id)sortedArrayWithOptions:(NSUInteger)options
             usingComparator:(NSInteger (^)(id object1, id object2))comparator {
    if (comparator == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: comparator cannot be nil",
            "-[NSArray sortedArrayWithOptions:usingComparator:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    return [self sortedArrayFromRange:(OCFRange){0, (OCFIndex)[self count]}
                              options:options
                      usingComparator:comparator];
}

// Native -[NSArray sortedArrayUsingComparator:], 0x1c027cd74. Non-external. As
// -sortedArrayWithOptions:usingComparator: with options 0.
- (id)sortedArrayUsingComparator:(NSInteger (^)(id object1, id object2))comparator {
    if (comparator == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: comparator cannot be nil",
            "-[NSArray sortedArrayUsingComparator:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    return [self sortedArrayFromRange:(OCFRange){0, (OCFIndex)[self count]}
                              options:0
                      usingComparator:comparator];
}

// Native -[NSArray sortedArrayUsingFunction:context:], 0x1c027ced8, with its block
// (___44-[NSArray sortedArrayUsingFunction:context:]_block_invoke, 0x1c027d000: the function on the two
// objects and the context, a tail call). Non-external. A NULL function raises NSInvalidArgumentException "***
// %s: function pointer cannot be NULL" (no os_log pack; reason with kCFAllocatorSystemDefault, autoreleased;
// +exceptionWithName:reason:userInfo:); the read check; -sortedArrayWithOptions:0 usingComparator: the block.
- (id)sortedArrayUsingFunction:(NSInteger (*)(id object1, id object2, void *context))comparator
                       context:(void *)context {
    if (comparator == NULL) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("*** %s: function pointer cannot be NULL"),
            "-[NSArray sortedArrayUsingFunction:context:]"
        );
        objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                            reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                                          userInfo:nil]);
    }
    arrayTSANRead(self, __builtin_return_address(0));
    return [self sortedArrayWithOptions:0
                        usingComparator:^NSInteger(id object1, id object2) {
                          return comparator(object1, object2, context);
                        }];
}

// Native -[NSArray sortedArrayUsingSelector:], 0x1c027d014, with its block
// (___36-[NSArray sortedArrayUsingSelector:]_block_invoke, 0x1c027d0ec: objc_msgSend of the selector to the
// first object with the second, a tail call). Non-external. A NULL selector sends -doesNotRecognizeSelector:
// with it; the read check; -sortedArrayWithOptions:0 usingComparator: the block.
- (id)sortedArrayUsingSelector:(SEL)comparator {
    if (comparator == NULL) {
        [self doesNotRecognizeSelector:comparator];
    }
    arrayTSANRead(self, __builtin_return_address(0));
    return [self sortedArrayWithOptions:0
                        usingComparator:^NSInteger(id object1, id object2) {
                          return ((NSInteger (*)(id, SEL, id))objc_msgSend)(object1, comparator, object2);
                        }];
}

// Native -[NSArray subarrayWithRange:], 0x1c027d0fc. Non-external. -count and the range raise come before the
// read check. An empty range answers +[NSArray array]; then the buffer limit, _CFCreateArrayStorage of the
// length, -getObjects:range:, each object retained (objc_retain), and the buffer adopted by a new NSArray (or
// -initWithObjects:NULL count: for a NULL buffer); autoreleased.
- (id)subarrayWithRange:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    NSUInteger count = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if ((intptr_t)location < 0 || (intptr_t)length < 0 || count < location + length) {
        RAISE_RANGE_EXCEPTION("-[NSArray subarrayWithRange:]", location, length, count);
    }
    arrayTSANRead(self, returnAddress);
    if (length == 0) {
        return [ONSArray array];
    }
    if ((length >> 60) != 0) {
        raiseTemporaryBufferTooLarge(length);
    }
    size_t capacity = 0;
    id *buffer = (id *)_OCFCreateArrayStorage(length, false, &capacity);
    [self getObjects:buffer range:range];
    id result;
    if (buffer != NULL) {
        for (NSUInteger index = 0; index != length; index++) {
            [buffer[index] retain];
        }
        result = [[ONSArray alloc] _initByAdoptingBuffer:buffer count:length size:length];
    } else {
        result = [[ONSArray alloc] initWithObjects:NULL count:length];
    }
    return [result autorelease];
}

// Native +[NSArray allocWithZone:], 0x1c027d4a8. Non-external. NSArray itself answers
// __NSArrayImmutablePlaceholder and NSMutableArray __NSArrayMutablePlaceholder (tail calls; the receiver is
// compared with the class symbols, no message); any other class goes to super (objc_msgSendSuper2 with
// NSArray's metaclass).
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == (Class)(void *)arrayClass) {
        return __NSArrayImmutablePlaceholder();
    }
    if (self == (Class)(void *)mutableArrayClass) {
        return __NSArrayMutablePlaceholder();
    }
    return [super allocWithZone:zone];
}

// Native -[NSArray initWithArray:], 0x1c027d570. Non-external. -initWithArray:range: (0, the argument's
// -count) copyItems:NO (a tail call).
- (id)initWithArray:(id)array {
    return [self initWithArray:array range:(OCFRange){0, (OCFIndex)[array count]} copyItems:NO];
}

// Native -[NSArray initWithArray:copyItems:], 0x1c027d5ac. Non-external. -initWithArray:range: (0, -count)
// copyItems:.
- (id)initWithArray:(id)array copyItems:(BOOL)copyItems {
    return [self initWithArray:array range:(OCFRange){0, (OCFIndex)[array count]} copyItems:copyItems];
}

// Native -[NSArray initWithArray:range:], 0x1c027d5f4. Non-external. -initWithArray:range:copyItems:NO (a
// tail call).
- (id)initWithArray:(id)array range:(OCFRange)range {
    return [self initWithArray:array range:range copyItems:NO];
}

// Native -[NSArray initWithArray:range:copyItems:], 0x1c027d5fc. Non-external. An argument that is not nil
// and not an NSArray raises "*** %s: array argument is not an NSArray" (buffer 0x0C); the argument's -count
// and the range raise; the buffer limit. The objects are copied with -getObjects:range: into a stack buffer
// (at most 256, at least one slot) or _CFCreateArrayStorage. An empty range clears copyItems; copyItems
// (tested equal to 1) replaces each object by its -copyWithZone:NULL. A heap buffer's objects are retained
// unless tagged (bit 63 set) and adopted (-_initByAdoptingBuffer:count:size:, the length twice); a stack
// buffer goes to -initWithObjects:count:. With copyItems (nonzero) the copies are then released through the
// buffer pointer.
- (id)initWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)copyItems {
    if (array != nil && !(_ONSIsNSArray(array) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: array argument is not an NSArray",
            "-[NSArray initWithArray:range:copyItems:]"
        );
    }
    NSUInteger count = [array count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (range.location < 0 || (intptr_t)length < 0 || count < location + length) {
        RAISE_RANGE_EXCEPTION("-[NSArray initWithArray:range:copyItems:]", location, length, count);
    }
    if ((length >> 60) != 0) {
        raiseTemporaryBufferTooLarge(length);
    }
    NSUInteger slots = length > 1 ? length : 1;
    id *objects = (id *)alloca((length < 0x101 ? slots : 1) * sizeof(id));
    size_t capacity = 0;
    id *heapBuffer;
    if (length >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(slots, false, &capacity);
        heapBuffer = objects;
    } else {
        heapBuffer = NULL;
    }
    [array getObjects:objects range:range];
    signed char copying = length == 0 ? 0 : (signed char)copyItems;
    if (copying == 1) {
        id *item = objects;
        NSUInteger remaining = length;
        do {
            *item = [*item copyWithZone:NULL];
            item++;
            remaining--;
        } while (remaining != 0);
    }
    id result;
    if (heapBuffer != NULL) {
        for (NSUInteger index = 0; index != length; index++) {
            id value = heapBuffer[index];
            if ((intptr_t)value >= 0) {
                [value retain];
            }
        }
        result = [self _initByAdoptingBuffer:heapBuffer count:length size:length];
    } else {
        result = [self initWithObjects:objects count:length];
    }
    if (copying != 0) {
        NSUInteger remaining = length;
        do {
            [*objects++ release];
            remaining--;
        } while (remaining != 0);
    }
    return result;
}

// Native -[NSArray initWithObject:], 0x1c027da88. Non-external. -initWithObjects:count: of the one object,
// from a stack slot.
- (id)initWithObject:(id)object {
    id objects[1] = {object};
    return [self initWithObjects:objects count:1];
}

// Native -[NSArray initWithObjects:], 0x1c027dadc. Non-external. The nil-terminated arguments are counted
// (the first object included) through a copy of the argument list. The immutable placeholder (the receiver
// compared with __NSArrayImmutablePlaceholder()) answers __NSArrayI_new of the first object and the arguments
// (noRetain 0: the objects are retained) or, for no objects, __NSArray0__struct;
// any other receiver checks the buffer limit, gathers the objects into a stack buffer (at most 256, at least
// one slot) or _CFCreateArrayStorage, and answers -initWithObjects:count:; the heap buffer is freed.
- (id)initWithObjects:(id)firstObject, ... {
    va_list arguments;
    va_start(arguments, firstObject);
    id first = firstObject;
    id placeholder = __NSArrayImmutablePlaceholder();
    NSUInteger count;
    if (firstObject != nil) {
        count = 0;
        va_list counter;
        va_copy(counter, arguments);
        id next;
        do {
            next = va_arg(counter, id);
            count++;
        } while (next != nil);
        va_end(counter);
        if (self == placeholder) {
            id result = __NSArrayI_new(&first, arguments, count, NO);
            va_end(arguments);
            return result;
        }
        if ((count >> 60) != 0) {
            raiseTemporaryBufferTooLarge(count);
        }
    } else {
        if (self == placeholder) {
            va_end(arguments);
            return (id)__ONSArray0__struct;
        }
        count = 0;
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
            objects[index] = va_arg(arguments, id);
        }
    }
    va_end(arguments);
    id result = [self initWithObjects:objects count:count];
    free(heapBuffer);
    return result;
}

// Native -[NSArray initWithSet:copyItems:], 0x1c027dcf0. Non-external. An argument that is not nil and not an
// NSSet raises "*** %s: set argument is not an NSSet" (buffer 0x0C); the set's -count and the buffer limit;
// the objects are gathered with -getObjects:count: into a stack buffer (at most 256) or
// _CFCreateArrayStorage; then as -initWithArray:range:copyItems:.
- (id)initWithSet:(id)set copyItems:(BOOL)copyItems {
    if (set != nil && !(_ONSIsNSSet(set) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: set argument is not an NSSet",
            "-[NSArray initWithSet:copyItems:]"
        );
    }
    NSUInteger count = [set count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferTooLarge(count);
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
    [set getObjects:objects count:count];
    signed char copying = count == 0 ? 0 : (signed char)copyItems;
    if (copying == 1) {
        id *item = objects;
        NSUInteger remaining = count;
        do {
            *item = [*item copyWithZone:NULL];
            item++;
            remaining--;
        } while (remaining != 0);
    }
    id result;
    if (heapBuffer != NULL) {
        for (NSUInteger index = 0; index != count; index++) {
            id value = heapBuffer[index];
            if ((intptr_t)value >= 0) {
                [value retain];
            }
        }
        result = [self _initByAdoptingBuffer:heapBuffer count:count size:count];
    } else {
        result = [self initWithObjects:objects count:count];
    }
    if (copying != 0) {
        NSUInteger remaining = count;
        do {
            [*objects++ release];
            remaining--;
        } while (remaining != 0);
    }
    return result;
}

// Native -[NSArray initWithSet:], 0x1c027dfc4. Non-external. -initWithSet:copyItems:NO (a tail call).
- (id)initWithSet:(id)set {
    return [self initWithSet:set copyItems:NO];
}

// Native -[NSArray initWithOrderedSet:range:copyItems:], 0x1c027dfcc. Non-external. -initWithArray:range:
// copyItems: for an ordered set: one that is not nil and not an NSOrderedSet raises "*** %s: ordered set
// argument is not an NSOrderedSet" (buffer 0x0C).
- (id)initWithOrderedSet:(id)orderedSet range:(OCFRange)range copyItems:(BOOL)copyItems {
    if (orderedSet != nil && !(_ONSIsNSOrderedSet(orderedSet) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSArray initWithOrderedSet:range:copyItems:]"
        );
    }
    NSUInteger count = [orderedSet count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (range.location < 0 || (intptr_t)length < 0 || count < location + length) {
        RAISE_RANGE_EXCEPTION("-[NSArray initWithOrderedSet:range:copyItems:]", location, length, count);
    }
    if ((length >> 60) != 0) {
        raiseTemporaryBufferTooLarge(length);
    }
    NSUInteger slots = length > 1 ? length : 1;
    id *objects = (id *)alloca((length < 0x101 ? slots : 1) * sizeof(id));
    size_t capacity = 0;
    id *heapBuffer;
    if (length >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(slots, false, &capacity);
        heapBuffer = objects;
    } else {
        heapBuffer = NULL;
    }
    [orderedSet getObjects:objects range:range];
    signed char copying = length == 0 ? 0 : (signed char)copyItems;
    if (copying == 1) {
        id *item = objects;
        NSUInteger remaining = length;
        do {
            *item = [*item copyWithZone:NULL];
            item++;
            remaining--;
        } while (remaining != 0);
    }
    id result;
    if (heapBuffer != NULL) {
        for (NSUInteger index = 0; index != length; index++) {
            id value = heapBuffer[index];
            if ((intptr_t)value >= 0) {
                [value retain];
            }
        }
        result = [self _initByAdoptingBuffer:heapBuffer count:length size:length];
    } else {
        result = [self initWithObjects:objects count:length];
    }
    if (copying != 0) {
        NSUInteger remaining = length;
        do {
            [*objects++ release];
            remaining--;
        } while (remaining != 0);
    }
    return result;
}

// Native -[NSArray initWithOrderedSet:range:], 0x1c027e458. Non-external.
// -initWithOrderedSet:range:copyItems:NO (a tail call).
- (id)initWithOrderedSet:(id)orderedSet range:(OCFRange)range {
    return [self initWithOrderedSet:orderedSet range:range copyItems:NO];
}

// Native -[NSArray initWithOrderedSet:copyItems:], 0x1c027e460. Non-external. -initWithOrderedSet:range: (0,
// -count) copyItems:.
- (id)initWithOrderedSet:(id)orderedSet copyItems:(BOOL)copyItems {
    return [self initWithOrderedSet:orderedSet
                              range:(OCFRange){0, (OCFIndex)[orderedSet count]}
                          copyItems:copyItems];
}

// Native -[NSArray initWithOrderedSet:], 0x1c027e4a8. Non-external. -initWithOrderedSet:range: (0, -count)
// copyItems:NO.
- (id)initWithOrderedSet:(id)orderedSet {
    return [self initWithOrderedSet:orderedSet
                              range:(OCFRange){0, (OCFIndex)[orderedSet count]}
                          copyItems:NO];
}

// Native -[NSArray _initByAdoptingBuffer:count:size:], 0x1c027e4e4. Non-external. -initWithObjects:count: of
// the buffer, then each of the count objects released (objc_release) and the buffer freed; size is not read.
- (id)_initByAdoptingBuffer:(id *)buffer count:(NSUInteger)count size:(NSUInteger)size {
    id result = [self initWithObjects:buffer count:count];
    id *item = buffer;
    for (NSUInteger remaining = count; remaining != 0; remaining--) {
        [*item++ release];
    }
    free(buffer);
    return result;
}

// Native +[NSArray arrayWithObject:], 0x1c027e538. Non-external. __createArray of objc_alloc(self) and the
// object, autoreleased.
+ (id)arrayWithObject:(id)object {
    return [__createArray([self alloc], object) autorelease];
}

// Native +[NSArray arrayWithObjects:count:], 0x1c027e5b4. Non-external. objc_alloc(self),
// -initWithObjects:count:, autoreleased.
+ (id)arrayWithObjects:(const id *)objects count:(NSUInteger)count {
    return [[[self alloc] initWithObjects:objects count:count] autorelease];
}

// Native +[NSArray array], 0x1c027e5e4. Non-external. objc_alloc(self), -initWithObjects:NULL count:0,
// autoreleased.
+ (id)array {
    return [[[self alloc] initWithObjects:NULL count:0] autorelease];
}

// Native +[NSArray newArrayWithObjects:count:], 0x1c027e604. Non-external. A NULL buffer with a nonzero count
// raises NSInvalidArgumentException "*** %s: pointer to objects array is NULL but length is %lu", a count of
// 2^61 or more "*** %s: count (%lu) of objects array is ridiculous", a nil object "*** %s: attempt to insert
// nil object from objects[%lu]" with its index (buffer 0x16 each). NSArray answers __NSSingleObjectArrayI_new
// for one object, __NSArrayI_new for more and __NSArray0__struct retained for none, NSMutableArray
// __NSArrayM_new (all without retaining the objects: the new object owns the caller's references); any other
// class raises through __CFRequireConcreteImplementation with NSArray (__CFLookUpClass) and answers nil.
+ (id)newArrayWithObjects:(const id *)objects count:(NSUInteger)count {
    if (objects == NULL && count != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "+[NSArray newArrayWithObjects:count:]",
            count
        );
    }
    if ((count >> 61) != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "+[NSArray newArrayWithObjects:count:]",
            count
        );
    }
    for (NSUInteger index = 0; index != count; index++) {
        if (objects[index] == nil) {
            RAISE_LOGGED_EXCEPTION(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: attempt to insert nil object from objects[%lu]",
                "+[NSArray newArrayWithObjects:count:]",
                index
            );
        }
    }
    if (self == (Class)(void *)arrayClass) {
        if (count == 1) {
            return __NSSingleObjectArrayI_new(objects[0], YES);
        }
        if (count != 0) {
            return __NSArrayI_new(objects, NULL, count, YES);
        }
        return [(id)__ONSArray0__struct retain];
    }
    if (self == (Class)(void *)mutableArrayClass) {
        return __NSArrayM_new(objects, count, YES);
    }
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSArray)), self, _cmd);
    return nil;
}

// Native +[NSArray arrayWithObjects:], 0x1c027e990. Non-external. The nil-terminated arguments are counted as
// in -initWithObjects:. A receiver that is the __NSPlaceholderArray class answers __NSArrayI_new of them
// (autoreleased) or __NSArray0__struct for none; otherwise the buffer limit, a stack buffer (at most 256) or
// _CFCreateArrayStorage, objc_alloc(self) and -initWithObjects:count:, the heap buffer freed, and the result
// autoreleased.
+ (id)arrayWithObjects:(id)firstObject, ... {
    va_list arguments;
    va_start(arguments, firstObject);
    id first = firstObject;
    NSUInteger count;
    if (firstObject != nil) {
        count = 0;
        va_list counter;
        va_copy(counter, arguments);
        id next;
        do {
            next = va_arg(counter, id);
            count++;
        } while (next != nil);
        va_end(counter);
        if (self == (Class)(void *)placeholderArrayClass) {
            id result = __NSArrayI_new(&first, arguments, count, NO);
            va_end(arguments);
            [result autorelease];
            return result;
        }
        if ((count >> 60) != 0) {
            raiseTemporaryBufferTooLarge(count);
        }
    } else {
        if (self == (Class)(void *)placeholderArrayClass) {
            va_end(arguments);
            return (id)__ONSArray0__struct;
        }
        count = 0;
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
            objects[index] = va_arg(arguments, id);
        }
    }
    va_end(arguments);
    id result = [[self alloc] initWithObjects:objects count:count];
    free(heapBuffer);
    [result autorelease];
    return result;
}

// Native +[NSArray arrayWithArray:range:copyItems:], 0x1c027ebb8. Non-external. objc_alloc(self),
// -initWithArray:range:copyItems:, autoreleased.
+ (id)arrayWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)copyItems {
    return [[[self alloc] initWithArray:array range:range copyItems:copyItems] autorelease];
}

// Native +[NSArray arrayWithArray:range:], 0x1c027ec00. Non-external. As above with copyItems:NO.
+ (id)arrayWithArray:(id)array range:(OCFRange)range {
    return [[[self alloc] initWithArray:array range:range copyItems:NO] autorelease];
}

// Native +[NSArray arrayWithArray:copyItems:], 0x1c027ec44. Non-external. The range is (0, the argument's
// -count, read after objc_alloc).
+ (id)arrayWithArray:(id)array copyItems:(BOOL)copyItems {
    id allocated = [self alloc];
    return [[allocated initWithArray:array range:(OCFRange){0, (OCFIndex)[array count]}
                           copyItems:copyItems] autorelease];
}

// Native +[NSArray arrayWithArray:], 0x1c027ec94. Non-external. As above with copyItems:NO.
+ (id)arrayWithArray:(id)array {
    id allocated = [self alloc];
    return [[allocated initWithArray:array range:(OCFRange){0, (OCFIndex)[array count]}
                           copyItems:NO] autorelease];
}

// Native +[NSArray arrayWithOrderedSet:range:copyItems:], 0x1c027ecd8. Non-external. objc_alloc(self),
// -initWithOrderedSet:range:copyItems:, autoreleased.
+ (id)arrayWithOrderedSet:(id)orderedSet range:(OCFRange)range copyItems:(BOOL)copyItems {
    return [[[self alloc] initWithOrderedSet:orderedSet range:range copyItems:copyItems] autorelease];
}

// Native +[NSArray arrayWithOrderedSet:range:], 0x1c027ed20. Non-external. As above with copyItems:NO.
+ (id)arrayWithOrderedSet:(id)orderedSet range:(OCFRange)range {
    return [[[self alloc] initWithOrderedSet:orderedSet range:range copyItems:NO] autorelease];
}

// Native +[NSArray arrayWithOrderedSet:copyItems:], 0x1c027ed64. Non-external. The range is (0, the
// argument's -count, read after objc_alloc).
+ (id)arrayWithOrderedSet:(id)orderedSet copyItems:(BOOL)copyItems {
    id allocated = [self alloc];
    return [[allocated initWithOrderedSet:orderedSet
                                    range:(OCFRange){0, (OCFIndex)[orderedSet count]}
                                copyItems:copyItems] autorelease];
}

// Native +[NSArray arrayWithOrderedSet:], 0x1c027edb4. Non-external. As above with copyItems:NO.
+ (id)arrayWithOrderedSet:(id)orderedSet {
    id allocated = [self alloc];
    return [[allocated initWithOrderedSet:orderedSet
                                    range:(OCFRange){0, (OCFIndex)[orderedSet count]}
                                copyItems:NO] autorelease];
}

// Native +[NSArray arrayWithSet:copyItems:], 0x1c027edf8. Non-external. objc_alloc(self),
// -initWithSet:copyItems:, autoreleased.
+ (id)arrayWithSet:(id)set copyItems:(BOOL)copyItems {
    return [[[self alloc] initWithSet:set copyItems:copyItems] autorelease];
}

// Native +[NSArray arrayWithSet:], 0x1c027ee28. Non-external. As above with copyItems:NO.
+ (id)arrayWithSet:(id)set {
    return [[[self alloc] initWithSet:set copyItems:NO] autorelease];
}

// Native -[NSArray arrayByAddingObject:], 0x1c027ee54. Non-external. A nil object raises "*** %s: object
// cannot be nil" (buffer 0x0C); the read check; the count plus one against the buffer limit;
// _CFCreateArrayStorage of it (at least one); -getObjects:range: (0, count); the object appended; every
// object retained (objc_retain); adopted by a new NSArray and autoreleased.
- (id)arrayByAddingObject:(id)object {
    if (object == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: object cannot be nil",
            "-[NSArray arrayByAddingObject:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger total = count + 1;
    if ((total >> 60) != 0) {
        raiseTemporaryBufferTooLarge(total);
    }
    size_t capacity = 0;
    id *buffer = (id *)_OCFCreateArrayStorage(total > 1 ? total : 1, false, &capacity);
    [self getObjects:buffer range:(OCFRange){0, (OCFIndex)count}];
    buffer[count] = object;
    for (NSUInteger index = 0; index != total; index++) {
        [buffer[index] retain];
    }
    return [[[ONSArray alloc] _initByAdoptingBuffer:buffer count:total size:total] autorelease];
}

// Native -[NSArray arrayByAddingObjectsFromArray:], 0x1c027f0b4. Non-external. An argument that is not nil
// and not an NSArray raises "*** %s: array argument is not an NSArray" (buffer 0x0C); the read check. An
// empty argument (-count) answers the receiver retained and autoreleased. Otherwise both counts against the
// buffer limit, _CFCreateArrayStorage of the sum (at least one), the receiver's objects (when any) and the
// argument's after them with -getObjects:range:, all retained, adopted by a new NSArray and autoreleased. The
// buffer is freed when an exception unwinds through the copies (the native landing pad; freeBufferOnExit
// stands for it).
- (id)arrayByAddingObjectsFromArray:(id)otherArray {
    if (otherArray != nil && !(_ONSIsNSArray(otherArray) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: array argument is not an NSArray",
            "-[NSArray arrayByAddingObjectsFromArray:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger otherCount = [otherArray count];
    if (otherCount == 0) {
        return [[self retain] autorelease];
    }
    NSUInteger count = [self count];
    NSUInteger total = count + otherCount;
    if ((total >> 60) != 0) {
        raiseTemporaryBufferTooLarge(total);
    }
    size_t capacity = 0;
    __attribute__((cleanup(freeBufferOnExit))) id *buffer =
        (id *)_OCFCreateArrayStorage(total > 1 ? total : 1, false, &capacity);
    if (count != 0) {
        [self getObjects:buffer range:(OCFRange){0, (OCFIndex)count}];
    }
    [otherArray getObjects:buffer + count range:(OCFRange){0, (OCFIndex)otherCount}];
    id result;
    if (buffer != NULL) {
        for (NSUInteger index = 0; index != total; index++) {
            [buffer[index] retain];
        }
        result = [[ONSArray alloc] _initByAdoptingBuffer:buffer count:total size:total];
    } else {
        result = [[ONSArray alloc] initWithObjects:NULL count:total];
    }
    buffer = NULL;
    return [result autorelease];
}

// Native -[NSArray arrayByApplyingSelector:], 0x1c027f3b0. Non-external. A NULL selector sends
// -doesNotRecognizeSelector: with it; the read check; -count and the buffer limit; a stack buffer (at most
// 256, at least one slot) or _CFCreateArrayStorage. Fast enumeration keeps each nonnil -performSelector:
// result. A heap buffer's results are retained (when the kept count is at least 1, as a signed word) and
// adopted by a new NSArray; a stack buffer goes to -initWithObjects:count:. Autoreleased.
- (id)arrayByApplyingSelector:(SEL)selector {
    if (selector == NULL) {
        [self doesNotRecognizeSelector:selector];
    }
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferTooLarge(count);
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
    NSInteger kept = 0;
    for (id object in self) {
        id value = [object performSelector:selector];
        if (value != nil) {
            objects[kept++] = value;
        }
    }
    id result;
    if (heapBuffer != NULL) {
        if (kept >= 1) {
            for (NSInteger index = 0; index != kept; index++) {
                [heapBuffer[index] retain];
            }
        }
        result = [[ONSArray alloc] _initByAdoptingBuffer:heapBuffer
                                                   count:(NSUInteger)kept
                                                    size:(NSUInteger)kept];
    } else {
        result = [[ONSArray alloc] initWithObjects:objects count:(NSUInteger)kept];
    }
    return [result autorelease];
}

// Native -[NSArray arrayByExcludingObjectsInArray:], 0x1c027f658. Non-external. An argument that is not nil
// and not an NSArray raises "*** %s: array argument is not an NSArray" (buffer 0x0C); the read check; -count
// and the buffer limit; a stack buffer (at most 256) or _CFCreateArrayStorage; +[NSSet setWithArray:] of the
// argument; fast enumeration keeps the objects the set does not -containsObject:; then as
// -arrayByApplyingSelector:.
- (id)arrayByExcludingObjectsInArray:(id)otherArray {
    if (otherArray != nil && !(_ONSIsNSArray(otherArray) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: array argument is not an NSArray",
            "-[NSArray arrayByExcludingObjectsInArray:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferTooLarge(count);
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
    id set = [(Class)(void *)setClass setWithArray:otherArray];
    NSInteger kept = 0;
    for (id object in self) {
        if (!([set containsObject:object] & 1)) {
            objects[kept++] = object;
        }
    }
    id result;
    if (heapBuffer != NULL) {
        if (kept >= 1) {
            for (NSInteger index = 0; index != kept; index++) {
                [heapBuffer[index] retain];
            }
        }
        result = [[ONSArray alloc] _initByAdoptingBuffer:heapBuffer
                                                   count:(NSUInteger)kept
                                                    size:(NSUInteger)kept];
    } else {
        result = [[ONSArray alloc] initWithObjects:objects count:(NSUInteger)kept];
    }
    return [result autorelease];
}

// Native -[NSArray arrayByExcludingToObjectsInArray:], 0x1c027f9dc. Non-external. As
// -arrayByExcludingObjectsInArray:, keeping the objects the set does -containsObject:.
- (id)arrayByExcludingToObjectsInArray:(id)otherArray {
    if (otherArray != nil && !(_ONSIsNSArray(otherArray) & 1)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x0C,
            "*** %s: array argument is not an NSArray",
            "-[NSArray arrayByExcludingToObjectsInArray:]"
        );
    }
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferTooLarge(count);
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
    id set = [(Class)(void *)setClass setWithArray:otherArray];
    NSInteger kept = 0;
    for (id object in self) {
        if ([set containsObject:object]) {
            objects[kept++] = object;
        }
    }
    id result;
    if (heapBuffer != NULL) {
        if (kept >= 1) {
            for (NSInteger index = 0; index != kept; index++) {
                [heapBuffer[index] retain];
            }
        }
        result = [[ONSArray alloc] _initByAdoptingBuffer:heapBuffer
                                                   count:(NSUInteger)kept
                                                    size:(NSUInteger)kept];
    } else {
        result = [[ONSArray alloc] initWithObjects:objects count:(NSUInteger)kept];
    }
    return [result autorelease];
}

// Native -[NSArray copyWithZone:], 0x1c027fd60. Non-external. The read check; __NSCollectionsShouldCopy;
// +[NSArray allocWithZone:] and -initWithArray:self range:(0, -count) copyItems: its answer (a tail call).
- (id)copyWithZone:(struct _NSZone *)zone {
    arrayTSANRead(self, __builtin_return_address(0));
    BOOL copyItems = __NSCollectionsShouldCopy();
    id allocated = [ONSArray allocWithZone:zone];
    return [allocated initWithArray:self range:(OCFRange){0, (OCFIndex)[self count]} copyItems:copyItems];
}

// Native -[NSArray mutableCopyWithZone:], 0x1c027fdec. Non-external. The read check; +[NSMutableArray
// allocWithZone:] and -initWithArray:self range:(0, -count) copyItems:NO (a tail call).
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    arrayTSANRead(self, __builtin_return_address(0));
    id allocated = [(Class)(void *)mutableArrayClass allocWithZone:zone];
    return [allocated initWithArray:self range:(OCFRange){0, (OCFIndex)[self count]} copyItems:NO];
}

// Native +[NSArray supportsSecureCoding], 0x1c027fe68. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSArray initWithCoder:], 0x1c027fe70. Non-external. Answers nil (MOV X0, #0); the receiver is not
// released.
- (id)initWithCoder:(id)coder {
    return nil;
}

// Native -[NSArray encodeWithCoder:], 0x1c027fe78. Non-external. Does nothing.
- (void)encodeWithCoder:(id)coder {
}

// Native -[NSArray count], 0x1c030f030, with OUTLINED_FUNCTION_0_37 (0x1c027fe7c: __CFLookUpClass("NSArray")
// with the receiver and selector kept in X20 and X19), _1_25 (0x1c027fe90: __CFRequireConcreteImplementation
// of that class, the receiver and the selector) and _2_21 (0x1c027fe9c: the epilogue). Non-external. The
// abstract primitive: raises, then answers 0.
- (NSUInteger)count {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSArray)), self, _cmd);
    return 0;
}

// Native -[NSArray objectAtIndex:], 0x1c030f050. Non-external. The abstract primitive, as -count; answers
// nil.
- (id)objectAtIndex:(NSUInteger)index {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSArray)), self, _cmd);
    return nil;
}

// Native -[NSArray initWithObjects:count:], 0x1c030f070. Non-external. The abstract primitive, as -count;
// answers nil.
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSArray)), self, _cmd);
    return nil;
}

@end
