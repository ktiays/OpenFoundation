//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSArrayI.h"
#include "CoreFoundation/Objective-C/OCFNSArrayHelpers.h"
#include "CoreFoundation/Objective-C/OCFNSArrayM.h"
#include "CoreFoundation/Objective-C/OCFNSDictionaryConcurrentEnumeration.h"
#include "CoreFoundation/Objective-C/OCFNSFastEnumerationEnumerator.h"
#include "CoreFoundation/Objective-C/OCFNSPlaceholderArray.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#import <objc/NSObject.h>

#include <alloca.h>
#include <mach-o/loader.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the __NSArrayI methods lie together in the iOS Simulator __text section
// (0x1c02ab4b4-0x1c02abeef) with __NSArrayI_new (0x1c02abd0c) among them; the __NSArrayI_Transfer methods
// follow (0x1c02abef0-0x1c02acc43) with their enumeration block (0x1c02ac8fc) and __NSArrayI_transferNew
// (0x1c02acb00) among them, then
// __NSCollectionHandleConcurrentEnumerationIfSpecified (0x1c02acc44, restored in
// Objective-C/OCFNSDictionaryConcurrentEnumeration.m) and the unit's OUTLINED_FUNCTION_0_49 (0x1c02ace04,
// _CFAutoreleasePoolAddObject of the reason) and OUTLINED_FUNCTION_1_35 (0x1c02ace10, _os_log_pack_fill with
// errno 0), which only __boundsFail (0x1c0310488, in the cold region) calls; they are folded into it.
// Following the owner's decision, the class has this translation unit, which also holds the
// __NSArrayI_Transfer methods (a second @implementation) with __NSArrayI_transferNew; the methods follow in
// native address order, the C functions at their native positions inside the @implementation, __boundsFail
// after them. The transfer class's countByEnumeratingWithState:objects:count:.const_mu.39 (0x1c046c978,
// __TEXT,__const, the value 1, read with Local/Scratch/cf-core/macho.py) is the function-local static of its
// only reader. Objective-C compiled with manual reference counting (objc_retain, objc_release). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json (family H-array-concrete; __NSArrayI_new from
// iphone-simulator-corpus-delta8.json; __NSArrayI_transferNew) and iphone-simulator-corpus-delta11.json
// (family L-collection-leftovers: the __NSArrayI_Transfer methods). Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).
//
// Argument errors raise as the other bridged array classes do: an os_log pack of the format's buffer size
// built on the stack (_os_log_pack_size, _os_log_pack_fill with errno 0 and the image's header, the
// arguments encoded as __builtin_os_log_format does), the reason made with kCFAllocatorSystemDefault and
// autoreleased through _CFAutoreleasePoolAddObject, +[NSException
// exceptionWithName:reason:userInfo:osLogPack:size:] with no user info thrown. The first format argument is
// the method's name as a literal C string, spelled with the class's selected name.

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// The NSException class symbol (off_2227CE300), sent to directly.
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));

// The message this unit sends that the shared declarations do not carry; declared for the send only.
@protocol ArrayIMessages

+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;

@end

#define methodName(selectorText) "-[" OF_NAME_STRING(__ONSArrayI) " " selectorText "]"

#define transferMethodName(selectorText) "-[" OF_NAME_STRING(__ONSArrayI_Transfer) " " selectorText "]"

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

// The objects-argument checks (buffer 0x16 each).
#define RAISE_OBJECTS_EXCEPTIONS(method, objects, count)                                                     \
    do {                                                                                                     \
        if ((objects) == NULL && (count) != 0) {                                                             \
            RAISE_LOGGED_EXCEPTION(                                                                          \
                ONSInvalidArgumentException,                                                                 \
                0x16,                                                                                        \
                "*** %s: pointer to objects array is NULL but length is %lu",                                \
                method,                                                                                      \
                (unsigned long)(count)                                                                       \
            );                                                                                               \
        }                                                                                                    \
        if (((count) >> 61) != 0) {                                                                          \
            RAISE_LOGGED_EXCEPTION(                                                                          \
                ONSInvalidArgumentException,                                                                 \
                0x16,                                                                                        \
                "*** %s: count (%lu) of objects array is ridiculous",                                        \
                method,                                                                                      \
                (unsigned long)(count)                                                                       \
            );                                                                                               \
        }                                                                                                    \
    } while (0)

// The index raise of the __NSArrayI_Transfer accessors (natively inline): NSRangeException, "beyond bounds
// for empty array" (buffer 0x16) for an empty array, otherwise "beyond bounds [0 .. %lu]" (buffer 0x20) with
// the count minus one.
#define RAISE_INDEX_EXCEPTION(method, index, count)                                                          \
    do {                                                                                                     \
        if ((count) == 0) {                                                                                  \
            RAISE_LOGGED_EXCEPTION(                                                                          \
                ONSRangeException,                                                                           \
                0x16,                                                                                        \
                "*** %s: index %lu beyond bounds for empty array",                                           \
                method,                                                                                      \
                (unsigned long)(index)                                                                       \
            );                                                                                               \
        }                                                                                                    \
        RAISE_LOGGED_EXCEPTION(                                                                              \
            ONSRangeException,                                                                               \
            0x20,                                                                                            \
            "*** %s: index %lu beyond bounds [0 .. %lu]",                                                    \
            method,                                                                                          \
            (unsigned long)(index),                                                                          \
            (unsigned long)(count) - 1                                                                       \
        );                                                                                                   \
    } while (0)

static __attribute__((noreturn)) void __boundsFail(NSUInteger index, NSUInteger count);

@implementation __ONSArrayI

// Native -[__NSArrayI count], 0x1c02ab4b4. Non-external. The used word.
- (NSUInteger)count {
    return used;
}

// Native -[__NSArrayI objectAtIndex:], 0x1c02ab4bc. Non-external. An index below the count (unsigned)
// answers the object; otherwise __boundsFail with the index and the count.
- (id)objectAtIndex:(NSUInteger)index {
    NSUInteger count = used;
    if (index >= count) {
        __boundsFail(index, count);
    }
    return list[index];
}

// Native -[__NSArrayI nothingAtIndex:], 0x1c02ab4e4. Non-external. Answers nil.
- (id)nothingAtIndex:(NSUInteger)index {
    return nil;
}

// Native -[__NSArrayI getObjects:range:], 0x1c02ab4ec. Non-external. Order: the buffer checks (a NULL buffer
// with a nonzero length, a length of 2^61 or more), then the range check (location with bit 63 set or past
// the count with the length; "for empty array", buffer 0x20, or "[0 .. %lu]", buffer 0x2A). Lengths above 4
// are copied with memmove (a tail call); 1-4 objects one by one.
- (void)getObjects:(id *)objects range:(OCFRange)range {
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    RAISE_OBJECTS_EXCEPTIONS(methodName("getObjects:range:"), objects, length);
    NSUInteger count = used;
    if ((NSInteger)location < 0 || count < location + length) {
        if (count == 0) {
            RAISE_LOGGED_EXCEPTION(
                ONSRangeException,
                0x20,
                "*** %s: range {%lu, %lu} extends beyond bounds for empty array",
                methodName("getObjects:range:"),
                (unsigned long)location,
                (unsigned long)length
            );
        }
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x2A,
            "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
            methodName("getObjects:range:"),
            (unsigned long)location,
            (unsigned long)length,
            (unsigned long)count - 1
        );
    }
    const id *source = list + location;
    if (length > 4) {
        memmove(objects, source, length << 3);
        return;
    }
    for (NSUInteger index = 0; index < length; index++) {
        objects[index] = source[index];
    }
}

// Native +[__NSArrayI automaticallyNotifiesObserversForKey:], 0x1c02ab8b8. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSArrayI countByEnumeratingWithState:objects:count:], 0x1c02ab8c0. Non-external. The buffer
// checks on the objects buffer and its length; then a state other than 0 answers 0, and a first call sets
// the state to -1, the items pointer to the list and the mutations pointer to the unit's const_mu, and
// answers the count. The const_mu is one of the image's "countByEnumeratingWithState:objects:count:.const_mu"
// constants (__TEXT,__const), named const_mu_6 in the capture without its address (unread); every one of them
// holds 1 (read with Local/Scratch/cf-core/macho.py).
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    static const unsigned long const_mu = 1;
    RAISE_OBJECTS_EXCEPTIONS(methodName("countByEnumeratingWithState:objects:count:"), buffer, length);
    FastEnumerationState *enumerationState = (FastEnumerationState *)state;
    if (enumerationState->state != 0) {
        return 0;
    }
    enumerationState->state = (unsigned long)-1;
    enumerationState->itemsPtr = list;
    enumerationState->mutationsPtr = (unsigned long *)&const_mu;
    return used;
}

// Native -[__NSArrayI enumerateObjectsWithOptions:usingBlock:], 0x1c02abaa4, with its block
// ___53-[__NSArrayI enumerateObjectsWithOptions:usingBlock:]_block_invoke (0x1c02abcc4, descriptor
// 0x2224297e0: the receiver and the block; the callout with the object at the index). A nil block raises
// ("block cannot be nil", buffer 0xC). The block goes to __NSCollectionHandleConcurrentEnumerationIfSpecified
// with an autorelease pool per call and the count; when that does not handle it, the objects are walked
// forward (the count reread after every call) or, with bit 1 of the options, backward from the count read
// once, each call inside its own autorelease pool, until the stop flag byte is 1.
- (void)enumerateObjectsWithOptions:(NSUInteger)options
                         usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    if (block == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            methodName("enumerateObjectsWithOptions:usingBlock:")
        );
    }
    BOOL handled =
        __NSCollectionHandleConcurrentEnumerationIfSpecified(options, YES, used, ^(size_t index, BOOL *stop) {
          __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__(block, self->list[index], index, stop);
        });
    if (handled) {
        return;
    }
    BOOL stop = NO;
    if (!(options & 2)) {
        if (used == 0) {
            return;
        }
        NSUInteger index = 0;
        do {
            id object = list[index];
            void *pool = _OCFAutoreleasePoolPush();
            __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__(block, object, index, &stop);
            _OCFAutoreleasePoolPop(pool);
            if (stop == YES) {
                return;
            }
            index++;
        } while (index < used);
    } else {
        NSUInteger index = used;
        if (index == 0) {
            return;
        }
        do {
            id object = list[index - 1];
            index--;
            void *pool = _OCFAutoreleasePoolPush();
            __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__(block, object, index, &stop);
            _OCFAutoreleasePoolPop(pool);
        } while (stop != YES && index != 0);
    }
}

// Native -[__NSArrayI objectAtIndexedSubscript:], 0x1c02abce0. Non-external. The body of -objectAtIndex:.
- (id)objectAtIndexedSubscript:(NSUInteger)index {
    NSUInteger count = used;
    if (index >= count) {
        __boundsFail(index, count);
    }
    return list[index];
}

// Native +[__NSArrayI allocWithZone:], 0x1c02abd08. Non-external. The immutable placeholder (a tail call).
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSArrayImmutablePlaceholder();
}

// Native __NSArrayI_new, 0x1c02abd0c. Hidden (was a private external). The object is __CFAllocateObject of
// the class (objc_opt_self of its class reference) with count * 8 extra bytes (not checked for NULL). For a
// nonzero count: with variable arguments the first object is *objects and the rest are read from the argument
// list in turn (the native code keeps the va_list in a stack slot and advances it by 8); otherwise the
// objects, when not NULL, are moved in. Unless noRetain (bit 0), each object that is not a tagged pointer
// (bit 63 clear) is retained; then used is set to the count. An empty array is answered as allocated.
// Evidence: iphone-simulator-corpus-delta8.json.
id __NSArrayI_new(const id *objects, va_list arguments, NSUInteger count, BOOL noRetain) {
    __ONSArrayI *object = __CFAllocateObject([__ONSArrayI self], count << 3);
    if (count != 0) {
        id *items = object->list;
        if (arguments != NULL) {
            items[0] = objects[0];
            for (NSUInteger index = 1; index < count; index++) {
                items[index] = va_arg(arguments, id);
            }
        } else if (objects != NULL) {
            memmove(items, objects, count << 3);
        }
        if (!(noRetain & 1)) {
            id *item = items;
            NSUInteger remaining = count;
            do {
                id value = *item++;
                if ((intptr_t)value >= 0) {
                    [value retain];
                }
                remaining--;
            } while (remaining != 0);
        }
        object->used = count;
    }
    return object;
}

// Native -[__NSArrayI dealloc], 0x1c02abe2c. Non-external. Each object is released (tagged pointers, bit
// 63, skipped), then super.
- (void)dealloc {
    NSUInteger remaining = used;
    if (remaining != 0) {
        id *item = list;
        do {
            id object = *item++;
            if ((intptr_t)object >= 0) {
                [object release];
            }
        } while (--remaining != 0);
    }
    [super dealloc];
}

// Native -[__NSArrayI copy], 0x1c02abec0. Non-external. The receiver retained (objc_retain, a tail call).
- (id)copy {
    return [self retain];
}

// Native -[__NSArrayI copyWithZone:], 0x1c02abec4. Non-external. The receiver retained (a tail call).
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native -[__NSArrayI mutableCopy], 0x1c02abec8. Non-external. __NSArrayM_new of the list and the count,
// retaining (a tail call).
- (id)mutableCopy {
    return __NSArrayM_new(list, used, NO);
}

// Native -[__NSArrayI mutableCopyWithZone:], 0x1c02abedc. Non-external. The body of -mutableCopy.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return __NSArrayM_new(list, used, NO);
}

@end

@implementation __ONSArrayI_Transfer

// Native -[__NSArrayI_Transfer count], 0x1c02abef0. Non-external. The _used word.
- (NSUInteger)count {
    return _used;
}

// Native -[__NSArrayI_Transfer objectAtIndex:], 0x1c02abf00. Non-external. An index below the count
// (unsigned) answers the object; otherwise the index raise.
- (id)objectAtIndex:(NSUInteger)index {
    NSUInteger count = _used;
    if (index >= count) {
        RAISE_INDEX_EXCEPTION(transferMethodName("objectAtIndex:"), index, count);
    }
    return _list[index];
}

// Native -[__NSArrayI_Transfer getObjects:range:], 0x1c02ac0dc. Non-external. Order: the buffer checks (a
// NULL buffer with a nonzero length, a length of 2^61 or more), then the range check (location with bit 63
// set, or location plus length past the count; "for empty array", buffer 0x20, or "[0 .. %lu]", buffer 0x2A).
// Lengths above 4 are copied with memmove (a tail call); 1-4 objects one by one.
- (void)getObjects:(id *)objects range:(OCFRange)range {
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    RAISE_OBJECTS_EXCEPTIONS(transferMethodName("getObjects:range:"), objects, length);
    NSUInteger count = _used;
    if ((NSInteger)location < 0 || count < location + length) {
        if (count == 0) {
            RAISE_LOGGED_EXCEPTION(
                ONSRangeException,
                0x20,
                "*** %s: range {%lu, %lu} extends beyond bounds for empty array",
                transferMethodName("getObjects:range:"),
                (unsigned long)location,
                (unsigned long)length
            );
        }
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x2A,
            "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
            transferMethodName("getObjects:range:"),
            (unsigned long)location,
            (unsigned long)length,
            (unsigned long)count - 1
        );
    }
    const id *source = _list + location;
    if (length > 4) {
        memmove(objects, source, length << 3);
        return;
    }
    for (NSUInteger index = 0; index < length; index++) {
        objects[index] = source[index];
    }
}

// Native +[__NSArrayI_Transfer automaticallyNotifiesObserversForKey:], 0x1c02ac4b8. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSArrayI_Transfer countByEnumeratingWithState:objects:count:], 0x1c02ac4c0. Non-external. The
// buffer checks on the objects buffer and its length; then a state other than 0 answers 0, and a first call
// points the mutations pointer at the unit's const_mu, sets the state to -1 and the items pointer to the
// buffer, and answers the count.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    // Native countByEnumeratingWithState:objects:count:.const_mu.39, 0x1c046c978 (__TEXT,__const,
    // non-external): the value 1.
    static const unsigned long const_mu = 1;
    RAISE_OBJECTS_EXCEPTIONS(
        transferMethodName("countByEnumeratingWithState:objects:count:"), buffer, length
    );
    FastEnumerationState *enumerationState = (FastEnumerationState *)state;
    if (enumerationState->state != 0) {
        return 0;
    }
    enumerationState->mutationsPtr = (unsigned long *)&const_mu;
    enumerationState->state = (unsigned long)-1;
    enumerationState->itemsPtr = _list;
    return _used;
}

// Native -[__NSArrayI_Transfer enumerateObjectsWithOptions:usingBlock:], 0x1c02ac6b4, with its block
// ___62-[__NSArrayI_Transfer enumerateObjectsWithOptions:usingBlock:]_block_invoke (0x1c02ac8fc, descriptor
// 0x2224297e0: the receiver and the block; the callout with the object at the index, the buffer reread). A
// nil block raises ("block cannot be nil", buffer 0xC). The block goes to
// __NSCollectionHandleConcurrentEnumerationIfSpecified with an autorelease pool per call and the count; when
// that does not handle it, the objects are walked forward (the count reread after every call) or, with bit 1
// of the options, backward from the count read once, each call inside its own autorelease pool and the
// buffer reread for each object, until the stop flag byte is 1.
- (void)enumerateObjectsWithOptions:(NSUInteger)options
                         usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    if (block == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            transferMethodName("enumerateObjectsWithOptions:usingBlock:")
        );
    }
    BOOL handled = __NSCollectionHandleConcurrentEnumerationIfSpecified(
        options, YES, _used, ^(size_t index, BOOL *stop) {
          __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__(block, self->_list[index], index, stop);
        }
    );
    if (handled) {
        return;
    }
    BOOL stop = NO;
    if (!(options & 2)) {
        if (_used == 0) {
            return;
        }
        NSUInteger index = 0;
        do {
            id object = _list[index];
            void *pool = _OCFAutoreleasePoolPush();
            __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__(block, object, index, &stop);
            _OCFAutoreleasePoolPop(pool);
            if (stop == YES) {
                return;
            }
            index++;
        } while (index < _used);
    } else {
        NSUInteger count = _used;
        if (count == 0) {
            return;
        }
        NSUInteger index = count - 1;
        do {
            id object = _list[index];
            void *pool = _OCFAutoreleasePoolPush();
            __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__(block, object, index, &stop);
            _OCFAutoreleasePoolPop(pool);
            if (stop == YES) {
                return;
            }
            index--;
        } while (index != (NSUInteger)-1);
    }
}

// Native -[__NSArrayI_Transfer objectAtIndexedSubscript:], 0x1c02ac920. Non-external. The body of
// -objectAtIndex: with its own name.
- (id)objectAtIndexedSubscript:(NSUInteger)index {
    NSUInteger count = _used;
    if (index >= count) {
        RAISE_INDEX_EXCEPTION(transferMethodName("objectAtIndexedSubscript:"), index, count);
    }
    return _list[index];
}

// Native +[__NSArrayI_Transfer allocWithZone:], 0x1c02acafc. Non-external. The immutable placeholder (a tail
// call); the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSArrayImmutablePlaceholder();
}

// Native __NSArrayI_transferNew, 0x1c02acb00. Hidden (was a private external). __CFAllocateObject of
// __NSArrayI_Transfer (objc_opt_self of ___SCR___NSArrayI_Transfer) with no extra bytes; NULL frees the
// buffer and answers NULL. Otherwise _used takes the count and _list the buffer. The capacity is not read.
id __NSArrayI_transferNew(id *list, NSUInteger count, __attribute__((unused)) NSUInteger capacity) {
    __ONSArrayI_Transfer *array = __CFAllocateObject([__ONSArrayI_Transfer self], 0);
    if (array == nil) {
        free(list);
        return nil;
    }
    array->_used = count;
    array->_list = list;
    return array;
}

// Native -[__NSArrayI_Transfer dealloc], 0x1c02acb6c. Non-external. The buffer and the count are read; each
// object is released (tagged pointers, bit 63, skipped) and the buffer reread; the buffer is freed; then
// super's -dealloc.
- (void)dealloc {
    id *list = _list;
    NSUInteger remaining = _used;
    if (remaining != 0) {
        id *item = list;
        do {
            id object = *item++;
            if ((intptr_t)object >= 0) {
                [object release];
            }
        } while (--remaining != 0);
        list = _list;
    }
    free(list);
    [super dealloc];
}

// Native -[__NSArrayI_Transfer copyWithZone:], 0x1c02acc1c. Non-external. The receiver retained (objc_retain,
// a tail call).
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native -[__NSArrayI_Transfer mutableCopyWithZone:], 0x1c02acc20. Non-external. __NSArrayM_new of the buffer
// and the count, retaining (a tail call).
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return __NSArrayM_new(_list, _used, NO);
}

@end

// Native __boundsFail, 0x1c0310488, with OUTLINED_FUNCTION_0_49 and _1_35 folded in. Non-external. Raises
// NSRangeException for the index and count with "__boundsFail" as the name: "*** %s: index %lu beyond bounds
// for empty array" (buffer 0x16) for a count of 0, otherwise "*** %s: index %lu beyond bounds [0 .. %lu]"
// (buffer 0x20) with the count minus one.
static void __boundsFail(NSUInteger index, NSUInteger count) {
    if (count == 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x16,
            "*** %s: index %lu beyond bounds for empty array",
            "__boundsFail",
            (unsigned long)index
        );
    }
    RAISE_LOGGED_EXCEPTION(
        ONSRangeException,
        0x20,
        "*** %s: index %lu beyond bounds [0 .. %lu]",
        "__boundsFail",
        (unsigned long)index,
        (unsigned long)count - 1
    );
}
