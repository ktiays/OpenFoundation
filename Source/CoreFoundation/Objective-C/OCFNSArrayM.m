//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSArrayM.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Objective-C/OCFNSArrayHelpers.h"
#include "CoreFoundation/Objective-C/OCFNSDictionaryConcurrentEnumeration.h"
#include "CoreFoundation/Objective-C/OCFNSFastEnumerationEnumerator.h"
#include "CoreFoundation/Objective-C/OCFNSPlaceholderArray.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <alloca.h>
#include <mach-o/loader.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <os/lock.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the __NSArrayM unit spans 0x1c015b374-0x1c0160e67 of the iOS Simulator __text
// section, after _CFCreateArrayStorage (Collections/OCFArray.c): the __NSArrayM methods with the non-external
// __NSArrayM_copy, __NSArrayM_new, __NSArrayM_transferNew and __RELEASE_OBJECTS_IN_THE_ARRAY__ among them,
// the __NSFrozenArrayM methods (0x1c015fedc-0x1c0160bff; their -dealloc .cold.1 at 0x1c0306390), the array
// copy-on-write callbacks cow_copy_instance, cow_copy_storage and cow_set_cow (0x1c0160c88-0x1c0160e27,
// listed without a suffix; the set, ordered set and dictionary units have their own), the block
// ___deque_enumerateObjectsWithOptionsBlock_block_invoke and OUTLINED_FUNCTION_0_0 (0x1c0160e5c,
// __cow_failure with "Attempting to mutate immutable collection!"; folded into its callers) and
// OUTLINED_FUNCTION_1 (0x1c0160e68, the "size unreasonably big" crash; not in the capture, read through its
// callers' .cold pieces and __NSArrayM_new.cold.1). The methods' .cold pieces lie in the cold region
// (0x1c0306210-0x1c030638f) and are folded into their methods. Following the owner's decision, the class
// has this translation unit, which also holds the __NSFrozenArrayM methods (a second @implementation) and
// the copy-on-write callbacks after it; the methods follow in native address order, the C functions at their
// native positions inside the @implementation. The unit's statics: ___NSArray_cowCallbacks (0x22279c860), the
// literal pool doubles qword_1C0372740 (the golden ratio), qword_1C0372748 (4294967295.0) and
// qword_1C0372750 (the golden ratio squared), and __deque_countByEnumeratingWithStateObjectsCount.const_mu
// (0x1c0372758, the value 1, read with Local/Scratch/cf-core/macho.py; the function-local static of
// -[__NSFrozenArrayM countByEnumeratingWithState:objects:count:], its only reader). Objective-C compiled with
// manual reference counting (objc_retain, objc_release, objc_autorelease). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json (family H-array-concrete; __NSArrayM_new from
// iphone-simulator-corpus-delta8.json) and iphone-simulator-corpus-delta11.json (family
// L-collection-leftovers: the __NSFrozenArrayM methods and the callbacks). Unless a definition says
// otherwise: iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device
// 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written
// yet).
//
// Shared shape of the methods. Each first calls the ThreadSanitizer read or write hook (when set) with the
// receiver, the method's return address (X30 at entry) and __CFTSANTagMutableArray. Argument errors raise
// through +[NSException exceptionWithName:reason:userInfo:osLogPack:size:] with an os_log pack built on the
// stack: _os_log_pack_size of the format's buffer size (12 for "%s", 22 for "%s %lu", 32 for three
// arguments, 42 for four), the pack filled through _os_log_pack_fill (errno 0, the image's mach header, the
// format), the arguments encoded by the compiler's __builtin_os_log_format (natively inline for one or two
// arguments, __os_log_helper_1_2_3_8_32_8_0_8_0 and __os_log_helper_1_2_4_8_32_8_0_8_0_8_0 for three and
// four), and the reason made with kCFAllocatorSystemDefault and the same format as a constant string,
// autoreleased through _CFAutoreleasePoolAddObject, with no user info. The first format argument is the
// method's name as a literal C string ("-[__NSArrayM objectAtIndex:]"), spelled here with the class's
// selected name so that the system-name build carries the native bytes.

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// The NSException class symbol (off_2227CE300), sent to directly.
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

// The messages this unit sends that the shared declarations do not carry; declared for the sends only.
@protocol ArrayMMessages

+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;

@end

// The ThreadSanitizer checks every method inlines: load the hook, and when it is set, call it with the
// object, the method's return address and __CFTSANTagMutableArray. arrayTSANRead and arrayTSANWrite are local
// descriptive names.
static inline void arrayTSANRead(const void *object, void *returnAddress) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanReadFunction;
    if (hook != NULL) {
        hook(object, returnAddress, __OCFTSANTagMutableArray);
    }
}

static inline void arrayTSANWrite(const void *object, void *returnAddress) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanWriteFunction;
    if (hook != NULL) {
        hook(object, returnAddress, __OCFTSANTagMutableArray);
    }
}

#define methodName(selectorText) "-[" OF_NAME_STRING(__ONSArrayM) " " selectorText "]"

#define frozenMethodName(selectorText) "-[" OF_NAME_STRING(__ONSFrozenArrayM) " " selectorText "]"

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

// The index check's raise: NSRangeException, "beyond bounds for empty array" (buffer 0x16) for an empty
// array, otherwise "beyond bounds [0 .. %lu]" (buffer 0x20) with the count minus one.
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

// The range check's raise: NSRangeException, "for empty array" (buffer 0x20) for an empty array, otherwise
// "[0 .. %lu]" (buffer 0x2A) with the count minus one.
#define RAISE_RANGE_EXCEPTION(method, location, length, count)                                               \
    do {                                                                                                     \
        if ((count) == 0) {                                                                                  \
            RAISE_LOGGED_EXCEPTION(                                                                          \
                ONSRangeException,                                                                           \
                0x20,                                                                                        \
                "*** %s: range {%lu, %lu} extends beyond bounds for empty array",                            \
                method,                                                                                      \
                (unsigned long)(location),                                                                   \
                (unsigned long)(length)                                                                      \
            );                                                                                               \
        }                                                                                                    \
        RAISE_LOGGED_EXCEPTION(                                                                              \
            ONSRangeException,                                                                               \
            0x2A,                                                                                            \
            "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",                                     \
            method,                                                                                          \
            (unsigned long)(location),                                                                       \
            (unsigned long)(length),                                                                         \
            (unsigned long)(count) - 1                                                                       \
        );                                                                                                   \
    } while (0)

// The objects-argument checks of the methods that take a buffer and a count (buffer 0x16 each).
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
        if (OF_WORD_SHIFT((count), 61) != 0) {                                                               \
            RAISE_LOGGED_EXCEPTION(                                                                          \
                ONSInvalidArgumentException,                                                                 \
                0x16,                                                                                        \
                "*** %s: count (%lu) of objects array is ridiculous",                                        \
                method,                                                                                      \
                (unsigned long)(count)                                                                       \
            );                                                                                               \
        }                                                                                                    \
    } while (0)

static void __RELEASE_OBJECTS_IN_THE_ARRAY__(id object, int kind);

static void cow_copy_instance(id source, int kind, id copy, int mode);

static bool cow_copy_storage(id object);

static void cow_set_cow(id object, CopyOnWriteBlock *cow);

// Native ___NSArray_cowCallbacks, 0x22279c860 (__DATA_CONST,__const). Non-external. Read with
// Local/Scratch/cf-core/macho.py: cow_copy_instance (0x1c0160c88), cow_copy_storage (0x1c0160cfc),
// cow_set_cow (0x1c0160e14) and ___RELEASE_OBJECTS_IN_THE_ARRAY__ (0x1c015fbe0), all defined in this unit.
static const CopyOnWriteCallbacks __NSArray_cowCallbacks = {
    cow_copy_instance,
    cow_copy_storage,
    cow_set_cow,
    __RELEASE_OBJECTS_IN_THE_ARRAY__,
};

// The growth factors: qword_1C0372740 (0x3FF9E3779B97F4A8, the golden ratio), qword_1C0372748
// (0x41EFFFFFFFE00000, 4294967295.0) and qword_1C0372750 (0x4004F1BBCDCBFA54, the golden ratio squared),
// __TEXT,__const literal-pool doubles read with Local/Scratch/cf-core/macho.py. kGrowthFactor,
// kCapacityLimit and kShrinkFactor are local descriptive names.
static const double kGrowthFactor = 0x1.9e3779b97f4a8p+0;
static const double kCapacityLimit = 0x1.fffffffep+31;
static const double kShrinkFactor = 0x1.4f1bbcdcbfa54p+1;

// The capacity the growing methods inline: the count times the golden ratio (UCVTF, FMUL), clamped to
// 4294967295.0 when it reaches it (FCMP, FCSEL GE), converted to 32 bits (FCVTZU), and at least the required
// count. grownCapacity is a local descriptive name.
static inline uint32_t grownCapacity(uint32_t count, uint32_t required) {
    double grown = (double)count * kGrowthFactor;
    if (grown >= kCapacityLimit) {
        grown = kCapacityLimit;
    }
    uint32_t capacity = (uint32_t)grown;
    return required > capacity ? required : capacity;
}

// The 32-bit slot of an object index the accessors inline: offset plus index, less the capacity when that
// is not below it (32-bit arithmetic). slotOfIndex is a local descriptive name.
static inline uint32_t slotOfIndex(const ArrayMStorage *storage, NSUInteger index) {
    uint32_t position = storage->offset + (uint32_t)index;
    return position - (storage->capacity > position ? 0 : storage->capacity);
}

// The slot pointer the storing paths inline: list + offset + index, less the capacity when it does not lie
// below the buffer's end (pointer comparison). slotPointer is a local descriptive name.
static inline id *slotPointer(const ArrayMStorage *storage, NSUInteger index) {
    id *list = storage->list;
    id *slot = list + storage->offset + index;
    return list + storage->capacity > slot ? slot : slot - storage->capacity;
}

// The ring position the shifting loops inline, for a possibly negative position and a capacity: natively
// the quotient (-position - 1) / max(capacity, 1) + 1 for a negative position (UDIV), 0 otherwise, times the
// capacity is added, and the capacity is then subtracted while the result is not below it (signed). The
// result is the position modulo the capacity. dequePosition is a local descriptive name.
static inline NSInteger dequePosition(NSInteger position, NSInteger capacity) {
    NSUInteger divisor = (NSUInteger)capacity > 1 ? (NSUInteger)capacity : 1;
    NSInteger quotient = position < 0 ? (NSInteger)((NSUInteger)(-1 - position) / divisor) + 1 : 0;
    NSInteger result = position + capacity * quotient;
    while (result >= capacity) {
        result -= capacity;
    }
    return result;
}

// The move of count objects from ring position source to ring position destination the inserting and
// removing methods inline (both positions unreduced, possibly negative). A destination below the source
// moves forward: each round reduces both positions, moves the objects up to the end of the buffer for the
// larger of the two (at most the remaining count) with memmove, and advances both. Otherwise it moves
// backward from the last objects: each round reduces both last positions, moves at most the smaller of the
// two plus one objects so that both runs end there, and steps both back. dequeMove is a local descriptive
// name.
static inline void
dequeMove(id *list, NSInteger capacity, NSInteger source, NSInteger destination, NSInteger count) {
    if (destination < source) {
        while (count > 0) {
            NSInteger from = dequePosition(source, capacity);
            NSInteger to = dequePosition(destination, capacity);
            NSInteger room = capacity - (to > from ? to : from);
            NSInteger moved = count < room ? count : room;
            memmove(list + to, list + from, (size_t)moved * sizeof(id));
            source += moved;
            destination += moved;
            count -= moved;
        }
    } else {
        NSInteger lastSource = source + count - 1;
        NSInteger lastDestination = destination + count - 1;
        while (count > 0) {
            NSInteger from = dequePosition(lastSource, capacity);
            NSInteger to = dequePosition(lastDestination, capacity);
            NSInteger nearest = from < to ? from : to;
            NSInteger moved = count < nearest + 1 ? count : nearest + 1;
            memmove(list + to + 1 - moved, list + from + 1 - moved, (size_t)moved * sizeof(id));
            lastSource -= moved;
            lastDestination -= moved;
            count -= moved;
        }
    }
}

// The copy of count objects (at least one) starting at object index location into a flat buffer that
// -getObjects:range: and the growing and shrinking paths inline. The source pointer is list + offset +
// location, less the capacity when it does not lie below the buffer's end; one to four objects are copied
// one by one, wrapping the source at the buffer's end after each. More are copied with memmove: first the
// objects up to the buffer's end (at most count; the low 32 bits of the counts are used), then the rest from
// the buffer's start; the native code then falls through into the four-object copy and copies the first four
// objects again onto themselves (harmless; kept). dequeCopyOut is a local descriptive name.
static inline void
dequeCopyOut(id *destination, const ArrayMStorage *storage, NSUInteger location, NSUInteger count) {
    id *list = storage->list;
    NSUInteger capacity = storage->capacity;
    id *end = list + capacity;
    id *source = list + storage->offset + location;
    if (!(end > source)) {
        source -= capacity;
    }
    switch (count) {
        case 1:
            break;
        case 2:
            goto copyTwo;
        case 3:
            goto copyThree;
        case 4:
            goto copyFour;
        default: {
            NSUInteger room = (NSUInteger)(end - source);
            NSUInteger first = room < count ? room : count;
            if ((uint32_t)first != 0) {
                memmove(destination, source, (size_t)(uint32_t)first * sizeof(id));
            }
            uint32_t rest = (uint32_t)(count - first);
            if (rest != 0) {
                memmove(destination + (uint32_t)first, list, (size_t)rest * sizeof(id));
            }
            goto copyFour;
        }
    }
    *destination = *source;
    return;
copyFour:
    *destination++ = *source++;
    if (!(end > source)) {
        source -= capacity;
    }
copyThree:
    *destination++ = *source++;
    if (!(end > source)) {
        source -= capacity;
    }
copyTwo:
    *destination++ = *source++;
    if (!(end > source)) {
        source -= capacity;
    }
    *destination = *source;
}

// The release of every object of a storage the removing paths inline: the objects from the first slot up
// to the buffer's end (at most count), then, when the count runs past the end, the rest (at least one) from
// the buffer's start; tagged pointers (bit 63) are skipped. releaseStorageObjects is a local descriptive
// name.
static inline void releaseStorageObjects(id *list, uint32_t offset, uint32_t capacity, uint32_t count) {
    uint32_t room = capacity - offset;
    uint32_t first = room < count ? room : count;
    if (first != 0) {
        id *item = list + offset;
        NSUInteger remaining = first;
        do {
            id object = *item++;
            if ((intptr_t)object >= 0) {
                [object release];
            }
        } while (--remaining != 0);
    }
    if (count > room) {
        uint32_t rest = count - first;
        if (rest <= 1) {
            rest = 1;
        }
        id *item = list;
        NSUInteger remaining = rest;
        do {
            id object = *item++;
            if ((intptr_t)object >= 0) {
                [object release];
            }
        } while (--remaining != 0);
    }
}

// The copy-on-write and mutation-counter prologue the mutating methods inline (and -_mutate is): the control
// block is loaded with acquire; the next counter is the current one plus one, or 0 once bits 9-31 of it
// reach 125. Under the block's lock a block without a mutable reference crashes (the methods' .cold.1:
// OUTLINED_FUNCTION_0_0, __cow_failure with "Attempting to mutate immutable collection!"); unless this array
// is the block's only reference, its mutable count drops and __cow_mutate_slow gives the array its own
// storage. The counter is then stored. mutateStorage is a local descriptive name.
static inline void mutateStorage(__ONSArrayM *array) {
    CopyOnWriteBlock *block = __atomic_load_n(&array->cow, __ATOMIC_ACQUIRE);
    uint32_t mutations = array->storage.mutations;
    uint32_t next = (mutations >> 9) >= 125 ? 0 : mutations + 1;
    if (block != NULL) {
        os_unfair_lock_lock(&block->lock);
        uint16_t mutableReferences = block->mutableReferences;
        if (mutableReferences == 0) {
            _cow_failure(OF_STR("Attempting to mutate immutable collection!"));
        }
        uint16_t remaining = mutableReferences - 1;
        if (remaining != 0 || block->immutableReferences != 0) {
            block->mutableReferences = remaining;
            _cow_mutate_slow((id)array, block, &__NSArray_cowCallbacks);
        }
        os_unfair_lock_unlock(&block->lock);
    }
    array->storage.mutations = next;
}

// Native _deque_enumerateObjectsWithOptionsBlock, inlined into -[__NSArrayM
// enumerateObjectsWithOptions:usingBlock:] (no symbol of its own; the name comes from its block,
// ____deque_enumerateObjectsWithOptionsBlock_block_invoke, 0x1c0160e28). The capacity is read once and the
// block literal (descriptor 0x22279c888: the enumeration block, the storage and the capacity) is handed to
// __NSCollectionHandleConcurrentEnumerationIfSpecified with an autorelease pool per call and the count. When
// that does not handle it, the objects are walked forward, or backward with bit 1 of the options, each call
// inside its own autorelease pool, until the stop flag byte is 1; the forward walk rereads the count after
// every call, the backward walk starts from the count read once. The list and offset are reread per object.
static inline void _deque_enumerateObjectsWithOptionsBlock(
    ArrayMStorage *storage, NSUInteger options, void (^block)(id object, NSUInteger index, BOOL *stop)
) {
    uint32_t capacity = storage->capacity;
    // Native ____deque_enumerateObjectsWithOptionsBlock_block_invoke, 0x1c0160e28. Non-external. The object
    // at the index (32-bit slot arithmetic with the captured capacity) and the callout.
    BOOL handled = __NSCollectionHandleConcurrentEnumerationIfSpecified(
        options, YES, storage->used, ^(size_t index, BOOL *stop) {
          uint32_t position = storage->offset + (uint32_t)index;
          position -= capacity > position ? 0 : capacity;
          __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__(block, storage->list[position], index, stop);
        }
    );
    if (handled) {
        return;
    }
    BOOL stop = NO;
    if (!(options & 2)) {
        if (storage->used == 0) {
            return;
        }
        NSUInteger index = 0;
        do {
            uint32_t offset = storage->offset;
            uint32_t position = (uint32_t)index + offset;
            uint32_t slot = (uint32_t)index + (offset - (capacity > position ? 0 : capacity));
            id object = storage->list[slot];
            void *pool = _OCFAutoreleasePoolPush();
            __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__(block, object, index, &stop);
            _OCFAutoreleasePoolPop(pool);
            if (stop == YES) {
                return;
            }
            index++;
        } while (index < storage->used);
    } else {
        NSUInteger index = storage->used;
        if (index == 0) {
            return;
        }
        do {
            index--;
            uint32_t offset = storage->offset;
            uint32_t position = (uint32_t)index + offset;
            uint32_t slot = (uint32_t)index + (offset - (capacity > position ? 0 : capacity));
            id object = storage->list[slot];
            void *pool = _OCFAutoreleasePoolPush();
            __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__(block, object, (uint32_t)index, &stop);
            _OCFAutoreleasePoolPop(pool);
        } while (stop != YES && index != 0);
    }
}

static id __NSArrayM_copy(__ONSArrayM *array);

@implementation __ONSArrayM

// Native -[__NSArrayM count], 0x1c015b374. Non-external. The used word.
- (NSUInteger)count {
    arrayTSANRead(self, __builtin_return_address(0));
    return storage.used;
}

// Native -[__NSArrayM objectAtIndex:], 0x1c015b3cc. Non-external. An index with bit 63 clear below the count
// answers the object at its 32-bit slot; otherwise the index raise.
// Folds the compiler-outlined pieces -[__NSArrayM insertObject:atIndex:].cold.2 (0x1c0306228), -[__NSArrayM
// insertObject:atIndex:].cold.3 (0x1c030623c), -[__NSArrayM insertObject:atIndex:].cold.4 (0x1c030624c),
// -[__NSArrayM insertObjects:count:atIndex:].cold.2 (0x1c0306284), -[__NSArrayM
// insertObjects:count:atIndex:].cold.3 (0x1c0306298), -[__NSArrayM insertObjects:count:atIndex:].cold.4
// (0x1c03062a8), -[__NSArrayM setObject:atIndex:].cold.2 (0x1c03062f0), -[__NSArrayM
// setObject:atIndex:].cold.3 (0x1c0306304), -[__NSArrayM setObject:atIndex:].cold.4 (0x1c0306314),
// -[__NSArrayM setObject:atIndexedSubscript:].cold.2 (0x1c0306334), -[__NSArrayM
// setObject:atIndexedSubscript:].cold.3 (0x1c0306348), -[__NSArrayM setObject:atIndexedSubscript:].cold.4
// (0x1c0306358).
- (id)objectAtIndex:(NSUInteger)index {
    arrayTSANRead(self, __builtin_return_address(0));
    uint32_t count = storage.used;
    if ((NSInteger)index < 0 || index >= count) {
        RAISE_INDEX_EXCEPTION(methodName("objectAtIndex:"), index, count);
    }
    return storage.list[slotOfIndex(&storage, index)];
}

// Native -[__NSArrayM getObjects:range:], 0x1c015b5ec. Non-external. Order: the range check (location or
// length with bit 63 set, or past the count), the NULL buffer with a nonzero length, a length of 2^61 or
// more; then a NULL buffer or an empty range copies nothing, any other range is copied out of the ring.
- (void)getObjects:(id *)objects range:(OCFRange)range {
    arrayTSANRead(self, __builtin_return_address(0));
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    uint32_t count = storage.used;
    if ((NSInteger)location < 0 || (NSInteger)length < 0 || location + length > count) {
        RAISE_RANGE_EXCEPTION(methodName("getObjects:range:"), location, length, count);
    }
    RAISE_OBJECTS_EXCEPTIONS(methodName("getObjects:range:"), objects, length);
    if (objects == NULL || length == 0) {
        return;
    }
    dequeCopyOut(objects, &storage, location, length);
}

// Native -[__NSArrayM allObjects], 0x1c015ba4c. Non-external. The copy, autoreleased.
- (id)allObjects {
    arrayTSANRead(self, __builtin_return_address(0));
    return [__NSArrayM_copy(self) autorelease];
}

// Native __NSArrayM_copy, 0x1c015ba9c. Non-external. The immutable copy -copy and -copyWithZone: answer:
// after the read hook, an array of at most 5 objects without a control block, or at most 3 with one, is
// -initWithArray:range:copyItems: (the whole range, the count reread, NO) of the immutable placeholder (a
// tail call). Otherwise a receiver without a control block gets one (__cow_create mode 1, installed with a
// compare-and-swap from NULL, acquire-release; the loser's block is freed) and the copy is
// __CFAllocateObject(__NSFrozenArrayM, 0), sharing the storage through __cow_copy (kind 1, mode 0).
static id __NSArrayM_copy(__ONSArrayM *array) {
    arrayTSANRead(array, __builtin_return_address(0));
    CopyOnWriteBlock *block = __atomic_load_n(&array->cow, __ATOMIC_ACQUIRE);
    uint32_t count = array->storage.used;
    if (block == NULL ? count <= 5 : count < 4) {
        return [__NSArrayImmutablePlaceholder() initWithArray:array
                                                        range:(OCFRange){0, array->storage.used}
                                                    copyItems:NO];
    }
    if (__atomic_load_n(&array->cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create((id)array, 1);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &array->cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject([__ONSFrozenArrayM self], 0);
    _cow_copy((id)array, 1, __atomic_load_n(&array->cow, __ATOMIC_ACQUIRE), &__NSArray_cowCallbacks, copy, 0);
    return copy;
}

// Native -[__NSArrayM _mutate], 0x1c015bbc0, with its .cold.1 (0x1c0306210). Non-external. The mutation
// prologue alone.
- (void)_mutate {
    mutateStorage(self);
}

// Native -[__NSArrayM insertObject:atIndex:], 0x1c015bc5c, with .cold.1-4 (0x1c030621c-0x1c030625f).
// Non-external. Order: the write hook, a nil object ("object cannot be nil", buffer 0xC), the index check
// (bit 63 or above the count), the mutation prologue, the object retained (unless tagged, bit 63), and a
// count of 2^32 - 1 crashes ("array size overflow", .cold.4). A full buffer grows to grownCapacity
// (malloc_good_size of 2^32 or more crashes with "size unreasonably big", .cold.3; a failed
// malloc_type_malloc(size, 0x80040B8603338) with "failure to grow array", .cold.2): the objects before the
// index are copied out, the object stored, the rest copied after it, the remainder zeroed, the storage
// replaced (offset 0, capacity size / 8) and the old buffer freed (a tail call); an array without a buffer
// just stores the object. Otherwise the shorter side moves: the objects before the index one slot down (the
// offset then steps back by one, modulo the capacity, the store unconditional and then corrected) when more
// objects follow the index than precede it, else the objects from the index one slot up. The slot is then
// stored when it does not already hold the object, and the count grows.
- (void)insertObject:(id)object atIndex:(NSUInteger)index {
    arrayTSANWrite(self, __builtin_return_address(0));
    if (object == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: object cannot be nil",
            methodName("insertObject:atIndex:")
        );
    }
    uint32_t count = storage.used;
    if ((NSInteger)index < 0 || index > count) {
        RAISE_INDEX_EXCEPTION(methodName("insertObject:atIndex:"), index, count);
    }
    mutateStorage(self);
    if ((intptr_t)object >= 0) {
        [object retain];
    }
    uint32_t newCount = count + 1;
    if (newCount == 0) {
        crashWithMessage("array size overflow");
    }
    uint32_t capacity = storage.capacity;
    if (capacity < newCount) {
        uint32_t grown = grownCapacity(count, newCount);
        id *oldList = storage.list;
        size_t size = malloc_good_size((size_t)grown * sizeof(id));
        if (((uint64_t)size >> 32) != 0) {
            crashWithMessage("size unreasonably big");
        }
        id *list = (id *)malloc_type_malloc(size, 0x80040B8603338ULL);
        if (list == NULL) {
            crashWithMessage("failure to grow array");
        }
        if (oldList == NULL) {
            list[0] = object;
        } else {
            if (index != 0) {
                dequeCopyOut(list, &storage, 0, index);
            }
            list[index] = object;
            uint32_t tail = count - (uint32_t)index;
            if (tail != 0) {
                dequeCopyOut(list + index + 1, &storage, index, tail);
            }
        }
        bzero(list + newCount, size - ((size_t)newCount * sizeof(id)));
        storage.list = list;
        storage.offset = 0;
        storage.capacity = (uint32_t)size / (uint32_t)sizeof(id);
        storage.used = newCount;
        free(oldList);
        return;
    }
    uint32_t tail = count - (uint32_t)index;
    if (tail > (uint32_t)index) {
        if (index != 0) {
            NSInteger offset = storage.offset;
            dequeMove(storage.list, capacity, offset, offset - 1, (NSInteger)index);
        }
        uint32_t offset = capacity + storage.offset - 1;
        storage.offset = offset;
        if (offset >= capacity) {
            storage.offset = offset - capacity;
        }
    } else if (count != (uint32_t)index) {
        NSInteger position = (NSInteger)index + storage.offset;
        dequeMove(storage.list, capacity, position, position + 1, tail);
    }
    id *slot = slotPointer(&storage, index);
    if (*slot != object) {
        *slot = object;
    }
    storage.used = newCount;
}

// Native -[__NSArrayM removeObjectAtIndex:], 0x1c015c4e4. Non-external. -removeObjectsInRange: of the one
// index (a tail call).
- (void)removeObjectAtIndex:(NSUInteger)index {
    arrayTSANWrite(self, __builtin_return_address(0));
    [self removeObjectsInRange:(OCFRange){(OCFIndex)index, 1}];
}

// Native -[__NSArrayM replaceObjectAtIndex:withObject:], 0x1c015c53c, with its .cold.1 (0x1c0306260).
// Non-external. Order: the write hook, the mutation prologue, a nil object, the index check. The same object
// is left alone; otherwise the new one is retained (unless tagged), stored, and the old one released (unless
// nil or tagged; a tail call).
- (void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)object {
    arrayTSANWrite(self, __builtin_return_address(0));
    mutateStorage(self);
    if (object == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: object cannot be nil",
            methodName("replaceObjectAtIndex:withObject:")
        );
    }
    uint32_t count = storage.used;
    if ((NSInteger)index < 0 || index >= count) {
        RAISE_INDEX_EXCEPTION(methodName("replaceObjectAtIndex:withObject:"), index, count);
    }
    uint32_t slot = slotOfIndex(&storage, index);
    id old = storage.list[slot];
    if (old == object) {
        return;
    }
    if ((intptr_t)object >= 0) {
        [object retain];
    }
    storage.list[slot] = object;
    if ((intptr_t)old >= 1) {
        [old release];
    }
}

// Native -[__NSArrayM addObject:], 0x1c015c8e4. Non-external. -insertObject:atIndex: at the count (a tail
// call).
- (void)addObject:(id)object {
    arrayTSANWrite(self, __builtin_return_address(0));
    [self insertObject:object atIndex:storage.used];
}

// Native +[__NSArrayM automaticallyNotifiesObserversForKey:], 0x1c015c948. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSArrayM countByEnumeratingWithState:objects:count:], 0x1c015c950. Non-external. The control
// block is loaded with acquire and discarded. A state of -1 answers 0. A first call (state 0) sets the
// mutations pointer to the mutation counter (read as a 64-bit word) and the items pointer to the first slot;
// when the objects run past the buffer's end, the state keeps the number that wrapped and the call answers
// the rest, otherwise the state becomes -1 and the call answers the count. A later call (any other state)
// points the items at the buffer's start, sets the state to -1 and answers the old state, the wrapped
// objects. The buffer is not used.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    arrayTSANRead(self, __builtin_return_address(0));
    FastEnumerationState *enumerationState = (FastEnumerationState *)state;
    (void)__atomic_load_n(&cow, __ATOMIC_ACQUIRE);
    unsigned long current = enumerationState->state;
    if (current == (unsigned long)-1) {
        return 0;
    }
    if (current != 0) {
        enumerationState->state = (unsigned long)-1;
        enumerationState->itemsPtr = storage.list;
        return current;
    }
    enumerationState->mutationsPtr = (unsigned long *)&storage.mutations;
    uint32_t count = storage.used;
    uint32_t offset = storage.offset;
    enumerationState->itemsPtr = storage.list + offset;
    uint32_t end = offset + count;
    if (end <= storage.capacity) {
        enumerationState->state = (unsigned long)-1;
        return count;
    }
    uint32_t wrapped = end - storage.capacity;
    enumerationState->state = wrapped;
    return (NSUInteger)count - wrapped;
}

// Native -[__NSArrayM enumerateObjectsWithOptions:usingBlock:], 0x1c015ca1c. Non-external. A nil block raises
// ("block cannot be nil", buffer 0xC); otherwise the inlined _deque_enumerateObjectsWithOptionsBlock.
- (void)enumerateObjectsWithOptions:(NSUInteger)options
                         usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    arrayTSANRead(self, __builtin_return_address(0));
    if (block == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            methodName("enumerateObjectsWithOptions:usingBlock:")
        );
    }
    _deque_enumerateObjectsWithOptionsBlock(&storage, options, block);
}

// Native -[__NSArrayM exchangeObjectAtIndex:withObjectAtIndex:], 0x1c015cca8, with its .cold.1
// (0x1c030626c). Non-external. Order: the write hook, the mutation prologue, the first index check (the index
// raise), the second (always "[0 .. %lu]", the array not being empty). Equal indexes change nothing;
// otherwise both slots are computed in 32 bits (the second one indexed sign-extended, SXTW), both objects
// loaded, and the second slot stored before the first.
- (void)exchangeObjectAtIndex:(NSUInteger)index1 withObjectAtIndex:(NSUInteger)index2 {
    arrayTSANWrite(self, __builtin_return_address(0));
    mutateStorage(self);
    uint32_t count = storage.used;
    if ((NSInteger)index1 < 0 || index1 >= count) {
        RAISE_INDEX_EXCEPTION(methodName("exchangeObjectAtIndex:withObjectAtIndex:"), index1, count);
    }
    if ((NSInteger)index2 < 0 || index2 >= count) {
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x20,
            "*** %s: index %lu beyond bounds [0 .. %lu]",
            methodName("exchangeObjectAtIndex:withObjectAtIndex:"),
            (unsigned long)index2,
            (unsigned long)count - 1
        );
    }
    if (index1 == index2) {
        return;
    }
    uint32_t offset = storage.offset;
    uint32_t capacity = storage.capacity;
    uint32_t position1 = offset + (uint32_t)index1;
    uint32_t slot1 = position1 - (capacity > position1 ? 0 : capacity);
    id *list = storage.list;
    id first = list[slot1];
    uint32_t position2 = offset + (uint32_t)index2;
    int32_t slot2 = (int32_t)(position2 - (capacity > position2 ? 0 : capacity));
    id second = list[slot2];
    list[slot2] = first;
    list[slot1] = second;
}

// Native -[__NSArrayM indexOfObjectIdenticalTo:], 0x1c015d028. Non-external. Pointer comparison over the
// objects up to the buffer's end, then (when the count runs past it) over at least one object from the
// buffer's start; NSNotFound otherwise.
- (NSUInteger)indexOfObjectIdenticalTo:(id)object {
    arrayTSANRead(self, __builtin_return_address(0));
    id *list = storage.list;
    uint32_t count = storage.used;
    uint32_t offset = storage.offset;
    uint32_t room = storage.capacity - offset;
    uint32_t first = count < room ? count : room;
    NSUInteger index = 0;
    if (first != 0) {
        id *items = list + offset;
        for (index = 0; index != first; index++) {
            if (items[index] == object) {
                return index;
            }
        }
    }
    index = first;
    if (count > room) {
        uint32_t rest = count - first;
        if (rest <= 1) {
            rest = 1;
        }
        id *item = list;
        NSUInteger remaining = rest;
        do {
            if (*item++ == object) {
                return index;
            }
            index++;
        } while (--remaining != 0);
    }
    return kNotFound;
}

// Native -[__NSArrayM insertObjects:count:atIndex:], 0x1c015d0f4, with .cold.1-4 (0x1c0306278-0x1c03062bb).
// Non-external. Order: the write hook, the mutation prologue, the buffer checks, a nil object ("attempt to
// insert nil object from objects[%lu]", buffer 0x16, over the full count), the index check; then only the
// low 32 bits of the count are used: none inserts nothing; the objects are retained (unless tagged), and a
// new count past 2^32 - 1 crashes ("array size overflow"). A full buffer grows to grownCapacity (the crashes
// of -insertObject:atIndex:): an array without a buffer takes the objects with memmove, otherwise the objects
// before the index are copied out, the new objects moved in after them and the rest copied after those; the
// remainder is zeroed, the storage replaced and the old buffer freed (a tail call). Otherwise the shorter
// side moves by the inserted count (the offset then steps back by it, modulo the capacity) and the objects
// are stored one by one into their slots (the slot pointer wrapped at the buffer's end before each store,
// stored only when it does not already hold the object); the count grows.
- (void)insertObjects:(const id *)objects count:(NSUInteger)count atIndex:(NSUInteger)index {
    arrayTSANWrite(self, __builtin_return_address(0));
    mutateStorage(self);
    RAISE_OBJECTS_EXCEPTIONS(methodName("insertObjects:count:atIndex:"), objects, count);
    for (NSUInteger item = 0; item < count; item++) {
        if (objects[item] == nil) {
            RAISE_LOGGED_EXCEPTION(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: attempt to insert nil object from objects[%lu]",
                methodName("insertObjects:count:atIndex:"),
                (unsigned long)item
            );
        }
    }
    uint32_t used = storage.used;
    if ((NSInteger)index < 0 || index > used) {
        RAISE_INDEX_EXCEPTION(methodName("insertObjects:count:atIndex:"), index, used);
    }
    uint32_t inserted = (uint32_t)count;
    if (inserted == 0) {
        return;
    }
    const id *item = objects;
    for (NSUInteger remaining = inserted; remaining != 0; remaining--) {
        id object = *item++;
        if ((intptr_t)object >= 0) {
            [object retain];
        }
    }
    uint32_t newCount = used + inserted;
    if (newCount < used) {
        crashWithMessage("array size overflow");
    }
    uint32_t capacity = storage.capacity;
    uint32_t position = (uint32_t)index;
    if (capacity < newCount) {
        uint32_t grown = grownCapacity(used, newCount);
        id *oldList = storage.list;
        size_t size = malloc_good_size((size_t)grown * sizeof(id));
        if (((uint64_t)size >> 32) != 0) {
            crashWithMessage("size unreasonably big");
        }
        id *list = (id *)malloc_type_malloc(size, 0x80040B8603338ULL);
        if (list == NULL) {
            crashWithMessage("failure to grow array");
        }
        if (oldList == NULL) {
            memmove(list, objects, (size_t)inserted * sizeof(id));
        } else {
            if (position != 0) {
                dequeCopyOut(list, &storage, 0, position);
            }
            memmove(list + position, objects, (size_t)inserted * sizeof(id));
            uint32_t tail = used - position;
            if (tail != 0) {
                dequeCopyOut(list + position + inserted, &storage, position, tail);
            }
        }
        bzero(list + newCount, size - ((size_t)newCount * sizeof(id)));
        storage.list = list;
        storage.offset = 0;
        storage.capacity = (uint32_t)size / (uint32_t)sizeof(id);
        storage.used = newCount;
        free(oldList);
        return;
    }
    uint32_t tail = used - position;
    if (tail > position) {
        if (position != 0) {
            NSInteger offset = storage.offset;
            dequeMove(storage.list, capacity, offset, offset - inserted, position);
        }
        uint32_t offset = storage.offset + (capacity - inserted);
        storage.offset = offset;
        if (offset >= capacity) {
            storage.offset = offset - capacity;
        }
    } else if (used != position) {
        NSInteger source = (NSInteger)position + storage.offset;
        dequeMove(storage.list, capacity, source, source + inserted, tail);
    }
    id *list = storage.list;
    id *end = list + storage.capacity;
    id *slot = list + storage.offset + position;
    const id *source = objects;
    for (NSUInteger remaining = inserted; remaining != 0; remaining--) {
        if (!(end > slot)) {
            slot -= storage.capacity;
        }
        id object = *source++;
        if (*slot != object) {
            *slot = object;
        }
        slot++;
    }
    storage.used = newCount;
}

// Native -[__NSArrayM objectAtIndexedSubscript:], 0x1c015dc68. Non-external. The body of -objectAtIndex:
// with this method's name in the raise.
- (id)objectAtIndexedSubscript:(NSUInteger)index {
    arrayTSANRead(self, __builtin_return_address(0));
    uint32_t count = storage.used;
    if ((NSInteger)index < 0 || index >= count) {
        RAISE_INDEX_EXCEPTION(methodName("objectAtIndexedSubscript:"), index, count);
    }
    return storage.list[slotOfIndex(&storage, index)];
}

// Native -[__NSArrayM removeAllObjects], 0x1c015de88, with its .cold.1 (0x1c03062bc). Non-external. After the
// mutation prologue the storage is read and cleared (the counter kept, the count, buffer, offset and
// capacity zeroed); a buffer then has its objects released and is freed (a tail call).
- (void)removeAllObjects {
    arrayTSANWrite(self, __builtin_return_address(0));
    mutateStorage(self);
    id *list = storage.list;
    uint32_t offset = storage.offset;
    uint32_t capacity = storage.capacity;
    uint32_t count = storage.used;
    storage.used = 0;
    storage.list = NULL;
    storage.offset = 0;
    storage.capacity = 0;
    if (list == NULL) {
        return;
    }
    releaseStorageObjects(list, offset, capacity, count);
    free(list);
}

// Native -[__NSArrayM removeLastObject], 0x1c015dfdc. Non-external. The count, then
// _CFExecutableLinkedOnOrAfter(7): apps linked on or after it remove the last object of a nonempty array and
// do nothing for an empty one; older apps send -removeObjectAtIndex: with the last index, 0 for an empty
// array (a tail call).
- (void)removeLastObject {
    arrayTSANWrite(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    NSUInteger index;
    if (_OCFExecutableLinkedOnOrAfter(7)) {
        if (count == 0) {
            return;
        }
        index = count - 1;
    } else {
        index = count == 0 ? 0 : count - 1;
    }
    [self removeObjectAtIndex:index];
}

// Native -[__NSArrayM removeObjectsInRange:], 0x1c015e060, with .cold.1 (0x1c03062c8) and .cold.2
// (0x1c03062d4, "size unreasonably big"). Non-external. Order: the write hook, the mutation prologue, -count,
// the range check against it; an empty range changes nothing. The removed objects are released (unless
// tagged): those up to the buffer's end, then the low 32 bits of the rest from its start. When the new count
// is not 0 and times the golden ratio squared lies below the capacity, a buffer of malloc_good_size(new count
// * 8) (2^32 or more crashes) is tried: on success the objects before and after the range are copied out
// into it, the remainder zeroed, the storage replaced (offset 0, capacity size / 8) and the old buffer freed
// (a tail call); a failed allocation falls back to the move. The move shifts the shorter side by the range's
// length: the objects before it one way (the offset then advances by the length, modulo the capacity read
// before the release), otherwise the objects after it. The count then becomes the new one; vacated slots are
// not cleared.
- (void)removeObjectsInRange:(OCFRange)range {
    arrayTSANWrite(self, __builtin_return_address(0));
    mutateStorage(self);
    NSUInteger arrayCount = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    NSUInteger end = location + length;
    if ((NSInteger)location < 0 || (NSInteger)length < 0 || arrayCount < end) {
        RAISE_RANGE_EXCEPTION(methodName("removeObjectsInRange:"), location, length, arrayCount);
    }
    if (length == 0) {
        return;
    }
    {
        id *list = storage.list;
        NSUInteger capacity = storage.capacity;
        id *listEnd = list + capacity;
        id *first = list + storage.offset + location;
        id *removed = listEnd > first ? first : first - capacity;
        NSUInteger room = (NSUInteger)(listEnd - removed);
        NSUInteger firstCount = room < length ? room : length;
        uint32_t remaining = (uint32_t)firstCount;
        if (remaining != 0) {
            id *item = removed;
            do {
                id object = *item++;
                if ((intptr_t)object >= 0) {
                    [object release];
                }
            } while (--remaining != 0);
        }
        uint32_t rest = (uint32_t)length - (uint32_t)firstCount;
        if (rest != 0) {
            id *item = list;
            do {
                id object = *item++;
                if ((intptr_t)object >= 0) {
                    [object release];
                }
            } while (--rest != 0);
        }
    }
    uint32_t capacity = storage.capacity;
    uint32_t used = storage.used;
    uint32_t newCount = used - (uint32_t)length;
    if (newCount != 0 && (double)newCount * kShrinkFactor < (double)capacity) {
        size_t requested = (size_t)newCount * sizeof(id);
        size_t size = malloc_good_size(requested);
        if (((uint64_t)size >> 32) != 0) {
            crashWithMessage("size unreasonably big");
        }
        id *list = (id *)malloc_type_malloc(size, 0x80040B8603338ULL);
        if (list != NULL) {
            if (location != 0) {
                dequeCopyOut(list, &storage, 0, location);
            }
            NSUInteger tail = (NSUInteger)used - end;
            if (tail != 0) {
                dequeCopyOut(list + location, &storage, end, tail);
            }
            bzero(list + newCount, size - requested);
            id *oldList = storage.list;
            storage.list = list;
            storage.offset = 0;
            storage.capacity = (uint32_t)size / (uint32_t)sizeof(id);
            storage.used = newCount;
            free(oldList);
            return;
        }
    }
    NSUInteger tail = (NSUInteger)used - end;
    if (location < tail) {
        uint32_t offset = storage.offset;
        if (location != 0) {
            dequeMove(storage.list, storage.capacity, offset, (NSInteger)offset + length, location);
            offset = storage.offset;
        }
        uint32_t moved = offset + (uint32_t)length;
        storage.offset = moved;
        if (moved >= capacity) {
            storage.offset = moved - capacity;
        }
    } else if (tail != 0 && (NSInteger)tail >= 1) {
        NSInteger offset = storage.offset;
        dequeMove(
            storage.list, storage.capacity, (NSInteger)end + offset, (NSInteger)location + offset, tail
        );
    }
    storage.used = newCount;
}

// Native -[__NSArrayM replaceObjectsInRange:withObjects:count:], 0x1c015e860. Non-external. Order: the write
// hook, -count, the range check, the buffer checks, a nil object (over the count); the new objects are
// retained (unless tagged), the overlapping part replaced one by one with -replaceObjectAtIndex:withObject:,
// then the surplus range removed (-removeObjectsInRange:) or the surplus objects inserted
// (-insertObjects:count:atIndex:), and the new objects released again (unless tagged).
- (void)replaceObjectsInRange:(OCFRange)range withObjects:(const id *)objects count:(NSUInteger)count {
    arrayTSANWrite(self, __builtin_return_address(0));
    NSUInteger arrayCount = [self count];
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if ((NSInteger)location < 0 || (NSInteger)length < 0 || arrayCount < location + length) {
        RAISE_RANGE_EXCEPTION(
            methodName("replaceObjectsInRange:withObjects:count:"), location, length, arrayCount
        );
    }
    RAISE_OBJECTS_EXCEPTIONS(methodName("replaceObjectsInRange:withObjects:count:"), objects, count);
    if (count != 0) {
        for (NSUInteger item = 0; item != count; item++) {
            if (objects[item] == nil) {
                RAISE_LOGGED_EXCEPTION(
                    ONSInvalidArgumentException,
                    0x16,
                    "*** %s: attempt to insert nil object from objects[%lu]",
                    methodName("replaceObjectsInRange:withObjects:count:"),
                    (unsigned long)item
                );
            }
        }
        const id *item = objects;
        for (NSUInteger remaining = count; remaining != 0; remaining--) {
            id object = *item++;
            if ((intptr_t)object >= 0) {
                [object retain];
            }
        }
    }
    NSUInteger replaced = length < count ? length : count;
    for (NSUInteger item = 0; item != replaced; item++) {
        [self replaceObjectAtIndex:location + item withObject:objects[item]];
    }
    if (length > count) {
        [self
            removeObjectsInRange:(OCFRange){(OCFIndex)(replaced + location), (OCFIndex)(length - replaced)}];
    } else if (count > length) {
        [self insertObjects:objects + replaced count:count - replaced atIndex:replaced + location];
    }
    const id *item = objects;
    for (NSUInteger remaining = count; remaining != 0; remaining--) {
        id object = *item++;
        if ((intptr_t)object >= 0) {
            [object release];
        }
    }
}

// The body -setObject:atIndex: and -setObject:atIndexedSubscript: share (the two native methods are
// identical apart from the name in their raises; each inlines it). Order: the mutation prologue, a nil
// object, the index check (the count itself allowed). Below the count it replaces: the same object is left
// alone, otherwise the new one is retained (unless tagged), the old one reread, the new one stored and the
// old one released (unless nil or tagged; a tail call). At the count it appends: the object retained
// (unless tagged), a count of 2^32 - 1 crashes, a free slot is stored (when it does not already hold the
// object) and the count grows; a full buffer grows as in -insertObject:atIndex:, the objects copied out
// before the new one. setObjectAtIndex is a local descriptive name.
static inline void setObjectAtIndex(__ONSArrayM *array, id object, NSUInteger index, const char *method) {
    ArrayMStorage *storage = &array->storage;
    mutateStorage(array);
    if (object == nil) {
        RAISE_LOGGED_EXCEPTION(ONSInvalidArgumentException, 0xC, "*** %s: object cannot be nil", method);
    }
    uint32_t count = storage->used;
    if ((NSInteger)index < 0 || index > count) {
        RAISE_INDEX_EXCEPTION(method, index, count);
    }
    if (count != (uint32_t)index) {
        id *list = storage->list;
        uint32_t slot = slotOfIndex(storage, index);
        if (list[slot] == object) {
            return;
        }
        if ((intptr_t)object >= 0) {
            [object retain];
        }
        id old = list[slot];
        list[slot] = object;
        if ((intptr_t)old >= 1) {
            [old release];
        }
        return;
    }
    if ((intptr_t)object >= 0) {
        [object retain];
    }
    uint32_t newCount = count + 1;
    if (newCount == 0) {
        crashWithMessage("array size overflow");
    }
    if (storage->capacity >= newCount) {
        id *slot = slotPointer(storage, index);
        if (*slot != object) {
            *slot = object;
        }
        storage->used = newCount;
        return;
    }
    uint32_t grown = grownCapacity(count, newCount);
    id *oldList = storage->list;
    size_t size = malloc_good_size((size_t)grown * sizeof(id));
    if (((uint64_t)size >> 32) != 0) {
        crashWithMessage("size unreasonably big");
    }
    id *list = (id *)malloc_type_malloc(size, 0x80040B8603338ULL);
    if (list == NULL) {
        crashWithMessage("failure to grow array");
    }
    if (oldList == NULL) {
        list[0] = object;
    } else {
        if (index != 0) {
            dequeCopyOut(list, storage, 0, index);
        }
        list[index] = object;
    }
    bzero(list + newCount, size - ((size_t)newCount * sizeof(id)));
    storage->list = list;
    storage->offset = 0;
    storage->capacity = (uint32_t)size / (uint32_t)sizeof(id);
    storage->used = newCount;
    free(oldList);
}

// Native -[__NSArrayM setObject:atIndex:], 0x1c015ed5c, with .cold.1-4 (0x1c03062e4-0x1c0306327).
// Non-external. The write hook, then setObjectAtIndex.
- (void)setObject:(id)object atIndex:(NSUInteger)index {
    arrayTSANWrite(self, __builtin_return_address(0));
    setObjectAtIndex(self, object, index, methodName("setObject:atIndex:"));
}

// Native -[__NSArrayM setObject:atIndexedSubscript:], 0x1c015f358, with .cold.1-4 (0x1c0306328-0x1c030636b).
// Non-external. The write hook, then setObjectAtIndex.
- (void)setObject:(id)object atIndexedSubscript:(NSUInteger)index {
    arrayTSANWrite(self, __builtin_return_address(0));
    setObjectAtIndex(self, object, index, methodName("setObject:atIndexedSubscript:"));
}

// Native __NSArrayM_new, 0x1c015f954, with .cold.1 (0x1c030636c: "size unreasonably big", through
// OUTLINED_FUNCTION_1). Hidden (was a private external). The object is __CFAllocateObject of the class
// (objc_opt_self of its class reference) with no extra bytes; NULL answers NULL. A count of 2^61 or more
// releases the object and answers NULL; malloc_good_size(count * 8) of 2^32 or more crashes. With objects,
// the list is malloc_type_malloc(size, 0x80040B8603338), the objects are moved in, retained unless noRetain
// (bit 0; tagged pointers, bit 63 set, are skipped) and the rest of the block is zeroed; without objects it
// is malloc_type_calloc(1, size, 0x80040B8603338) and the count becomes 0. An allocation failure releases the
// object and answers NULL. The storage gets the list, offset 0, capacity size / 8 (32 bits), the mutation
// counter 1 and the count (its low 32 bits); an empty array gets a NULL list and capacity 0. Evidence:
// iphone-simulator-corpus-delta8.json.
id __NSArrayM_new(const id *objects, NSUInteger count, BOOL noRetain) {
    __ONSArrayM *object = __CFAllocateObject([__ONSArrayM self], 0);
    if (object == nil) {
        return nil;
    }
    id *list;
    uint32_t capacity;
    if (count != 0) {
        if ((OF_WORD_SHIFT(count, 61)) != 0) {
            [object release];
            return nil;
        }
        size_t size = malloc_good_size(count * sizeof(id));
        if (((uint64_t)size >> 32) != 0) {
            crashWithMessage("size unreasonably big");
        }
        if (objects != NULL) {
            list = (id *)malloc_type_malloc(size, 0x80040B8603338ULL);
            if (list == NULL) {
                [object release];
                return nil;
            }
            memmove(list, objects, count * sizeof(id));
            if (!(noRetain & 1)) {
                id *item = list;
                NSUInteger remaining = count;
                do {
                    id value = *item++;
                    if ((intptr_t)value >= 0) {
                        [value retain];
                    }
                    remaining--;
                } while (remaining != 0);
            }
            bzero(list + count, size - (count * sizeof(id)));
        } else {
            list = (id *)malloc_type_calloc(1, size, 0x80040B8603338ULL);
            if (list == NULL) {
                [object release];
                return nil;
            }
            count = 0;
        }
        capacity = (uint32_t)size / (uint32_t)sizeof(id);
    } else {
        list = NULL;
        capacity = 0;
    }
    object->storage.list = list;
    object->storage.offset = 0;
    object->storage.capacity = capacity;
    object->storage.mutations = 1;
    object->storage.used = (uint32_t)count;
    return object;
}

// Native __NSArrayM_transferNew, 0x1c015fa8c. Hidden (was a private external). __CFAllocateObject of the
// class (objc_opt_self of ___SCR___NSArrayM) with no extra bytes; NULL frees the buffer and answers NULL.
// Otherwise the storage adopts the buffer: offset 0, the capacity's and the count's low 32 bits, the
// mutation counter 1.
id __NSArrayM_transferNew(id *list, NSUInteger count, NSUInteger capacity) {
    __ONSArrayM *array = __CFAllocateObject([__ONSArrayM self], 0);
    if (array == nil) {
        free(list);
        return nil;
    }
    array->storage.list = list;
    array->storage.offset = 0;
    array->storage.capacity = (uint32_t)capacity;
    array->storage.mutations = 1;
    array->storage.used = (uint32_t)count;
    return array;
}

// Native -[__NSArrayM dealloc], 0x1c015fb00, with its .cold.1 (0x1c030637c). Non-external. With a control
// block: under its lock a block without a mutable reference crashes (__cow_failure with "Attempting to
// over-release (mutable)"); the mutable count drops, and when no reference remains the objects are released
// and the block freed. Without one the objects are released. Then super.
- (void)dealloc {
    CopyOnWriteBlock *block = __atomic_load_n(&cow, __ATOMIC_ACQUIRE);
    if (block != NULL) {
        os_unfair_lock_lock(&block->lock);
        uint16_t mutableReferences = block->mutableReferences;
        if (mutableReferences == 0) {
            _cow_failure(OF_STR("Attempting to over-release (mutable)"));
        }
        uint16_t immutableReferences = block->immutableReferences;
        mutableReferences -= 1;
        block->mutableReferences = mutableReferences;
        uint16_t remaining = mutableReferences | immutableReferences;
        os_unfair_lock_unlock(&block->lock);
        if (remaining == 0) {
            __RELEASE_OBJECTS_IN_THE_ARRAY__(self, 1);
            free(block);
        }
    } else {
        __RELEASE_OBJECTS_IN_THE_ARRAY__(self, 1);
    }
    [super dealloc];
}

// Native ___RELEASE_OBJECTS_IN_THE_ARRAY__, 0x1c015fbe0. Non-external; the cleanup slot of
// ___NSArray_cowCallbacks. Kind 1 is a __NSArrayM, any other a __NSFrozenArrayM (its storage ivar offset).
// The storage is read and detached (the count, buffer, offset and capacity zeroed, the counter kept); a
// buffer then has its objects released (unless tagged) and is freed (a tail call).
static void __RELEASE_OBJECTS_IN_THE_ARRAY__(id object, int kind) {
    ArrayMStorage *storage =
        kind == 1 ? &((__ONSArrayM *)object)->storage : &((__ONSFrozenArrayM *)object)->storage;
    id *list = storage->list;
    uint32_t offset = storage->offset;
    uint32_t capacity = storage->capacity;
    uint32_t count = storage->used;
    storage->used = 0;
    storage->list = NULL;
    storage->offset = 0;
    storage->capacity = 0;
    if (list == NULL) {
        return;
    }
    releaseStorageObjects(list, offset, capacity, count);
    free(list);
}

// Native +[__NSArrayM allocWithZone:], 0x1c015fcb0. Non-external. The mutable placeholder (a tail call).
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSArrayMutablePlaceholder();
}

// Native -[__NSArrayM copy], 0x1c015fcb4. Non-external. __NSArrayM_copy (a tail call).
- (id)copy {
    arrayTSANRead(self, __builtin_return_address(0));
    return __NSArrayM_copy(self);
}

// Native -[__NSArrayM copyWithZone:], 0x1c015fd00. Non-external. The body of -copy; the zone is not read.
- (id)copyWithZone:(struct _NSZone *)zone {
    arrayTSANRead(self, __builtin_return_address(0));
    return __NSArrayM_copy(self);
}

// Native -[__NSArrayM mutableCopy], 0x1c015fd4c. Non-external. A receiver without a control block gets one
// (__cow_create mode 1, installed with a compare-and-swap from NULL, acquire-release; the loser's block is
// freed). The copy is __CFAllocateObject(__NSArrayM, 0), sharing the storage through __cow_copy (kind 1, mode
// 1).
- (id)mutableCopy {
    arrayTSANRead(self, __builtin_return_address(0));
    if (__atomic_load_n(&cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create(self, 1);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject([__ONSArrayM self], 0);
    _cow_copy(self, 1, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSArray_cowCallbacks, copy, 1);
    return copy;
}

// Native -[__NSArrayM mutableCopyWithZone:], 0x1c015fe14. Non-external. The body of -mutableCopy; the zone
// is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    arrayTSANRead(self, __builtin_return_address(0));
    if (__atomic_load_n(&cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create(self, 1);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject([__ONSArrayM self], 0);
    _cow_copy(self, 1, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSArray_cowCallbacks, copy, 1);
    return copy;
}

@end

// The __NSFrozenArrayM methods share the unit's shapes without the ThreadSanitizer checks: none of them calls
// a hook. Their raises name the method with the class's selected name.
@implementation __ONSFrozenArrayM

// Native +[__NSFrozenArrayM automaticallyNotifiesObserversForKey:], 0x1c015fedc. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSFrozenArrayM count], 0x1c015fee4. Non-external. The used word.
- (NSUInteger)count {
    return storage.used;
}

// Native -[__NSFrozenArrayM objectAtIndex:], 0x1c015fef8. Non-external. An index with bit 63 clear below the
// count answers the object at its 32-bit slot; otherwise the index raise.
- (id)objectAtIndex:(NSUInteger)index {
    uint32_t count = storage.used;
    if ((NSInteger)index < 0 || index >= count) {
        RAISE_INDEX_EXCEPTION(frozenMethodName("objectAtIndex:"), index, count);
    }
    return storage.list[slotOfIndex(&storage, index)];
}

// Native -[__NSFrozenArrayM objectAtIndexedSubscript:], 0x1c01600e8. Non-external. The body of
// -objectAtIndex: with its own name.
- (id)objectAtIndexedSubscript:(NSUInteger)index {
    uint32_t count = storage.used;
    if ((NSInteger)index < 0 || index >= count) {
        RAISE_INDEX_EXCEPTION(frozenMethodName("objectAtIndexedSubscript:"), index, count);
    }
    return storage.list[slotOfIndex(&storage, index)];
}

// Native -[__NSFrozenArrayM countByEnumeratingWithState:objects:count:], 0x1c01602d8. Non-external. The
// control block is loaded with acquire and discarded. A state of -1 answers 0. A first call (state 0) points
// the mutations pointer at the unit's constant const_mu (a frozen array never mutates) and the items pointer
// at the first slot; when the objects run past the buffer's end, the state keeps the number that wrapped and
// the call answers the rest, otherwise the state becomes -1 and the call answers the count. A later call
// (any other state) points the items at the buffer's start, sets the state to -1 and answers the old state,
// the wrapped objects. The buffer is not used.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    // Native __deque_countByEnumeratingWithStateObjectsCount.const_mu, 0x1c0372758 (__TEXT,__const,
    // non-external): the value 1.
    static const unsigned long const_mu = 1;
    FastEnumerationState *enumerationState = (FastEnumerationState *)state;
    (void)__atomic_load_n(&cow, __ATOMIC_ACQUIRE);
    unsigned long current = enumerationState->state;
    if (current == (unsigned long)-1) {
        return 0;
    }
    if (current != 0) {
        enumerationState->state = (unsigned long)-1;
        enumerationState->itemsPtr = storage.list;
        return current;
    }
    enumerationState->mutationsPtr = (unsigned long *)&const_mu;
    uint32_t count = storage.used;
    uint32_t offset = storage.offset;
    enumerationState->itemsPtr = storage.list + offset;
    uint32_t end = offset + count;
    if (end <= storage.capacity) {
        enumerationState->state = (unsigned long)-1;
        return count;
    }
    uint32_t wrapped = end - storage.capacity;
    enumerationState->state = wrapped;
    return (NSUInteger)count - wrapped;
}

// Native -[__NSFrozenArrayM enumerateObjectsWithOptions:usingBlock:], 0x1c0160364. Non-external. A nil block
// raises ("block cannot be nil", buffer 0xC); otherwise the inlined _deque_enumerateObjectsWithOptionsBlock
// on the frozen storage (the same block invoke and descriptor as -[__NSArrayM
// enumerateObjectsWithOptions:usingBlock:]).
- (void)enumerateObjectsWithOptions:(NSUInteger)options
                         usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    if (block == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            frozenMethodName("enumerateObjectsWithOptions:usingBlock:")
        );
    }
    _deque_enumerateObjectsWithOptionsBlock(&storage, options, block);
}

// Native -[__NSFrozenArrayM getObjects:range:], 0x1c01605c0. Non-external. Order: the buffer checks (a NULL
// buffer with a nonzero length, a length of 2^61 or more), then the range check (location with bit 63 set,
// or location plus length past the count); then a NULL buffer or an empty range copies nothing, any other
// range is copied out of the ring as -[__NSArrayM getObjects:range:] does.
- (void)getObjects:(id *)objects range:(OCFRange)range {
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    RAISE_OBJECTS_EXCEPTIONS(frozenMethodName("getObjects:range:"), objects, length);
    uint32_t count = storage.used;
    if ((NSInteger)location < 0 || location + length > count) {
        RAISE_RANGE_EXCEPTION(frozenMethodName("getObjects:range:"), location, length, count);
    }
    if (objects == NULL || length == 0) {
        return;
    }
    dequeCopyOut(objects, &storage, location, length);
}

// Native -[__NSFrozenArrayM indexOfObjectIdenticalTo:], 0x1c01609ec. Non-external. Pointer comparison over
// the objects up to the buffer's end; then, when the count runs past it, over the rest from the buffer's
// start, the index counted in 32 bits; NSNotFound otherwise.
- (NSUInteger)indexOfObjectIdenticalTo:(id)object {
    id *list = storage.list;
    uint32_t count = storage.used;
    uint32_t offset = storage.offset;
    uint32_t room = storage.capacity - offset;
    uint32_t first = count < room ? count : room;
    if (first != 0) {
        id *items = list + offset;
        for (NSUInteger index = 0; index != first; index++) {
            if (items[index] == object) {
                return index;
            }
        }
    }
    if (count > room) {
        uint32_t index = first;
        do {
            if (*list++ == object) {
                return index;
            }
            index++;
        } while (index != count);
    }
    return kNotFound;
}

// Native +[__NSFrozenArrayM allocWithZone:], 0x1c0160a64. Non-external. The immutable placeholder (a tail
// call); the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSArrayImmutablePlaceholder();
}

// Native -[__NSFrozenArrayM dealloc], 0x1c0160a68, with its .cold.1 (0x1c0306390). Non-external. With a
// control block: under its lock a block without an immutable reference crashes (__cow_failure with
// "Attempting to over-release (immutable)"); the immutable count drops, and when neither count remains the
// objects are released (__RELEASE_OBJECTS_IN_THE_ARRAY__, kind 0) and the block freed, after the unlock.
// Without one the objects are released. Then super's -dealloc.
- (void)dealloc {
    CopyOnWriteBlock *block = __atomic_load_n(&cow, __ATOMIC_ACQUIRE);
    if (block != NULL) {
        os_unfair_lock_lock(&block->lock);
        uint16_t immutableReferences = block->immutableReferences;
        if (immutableReferences == 0) {
            _cow_failure(OF_STR("Attempting to over-release (immutable)"));
        }
        uint16_t mutableReferences = block->mutableReferences;
        immutableReferences -= 1;
        block->immutableReferences = immutableReferences;
        uint16_t remaining = mutableReferences | immutableReferences;
        os_unfair_lock_unlock(&block->lock);
        if (remaining == 0) {
            __RELEASE_OBJECTS_IN_THE_ARRAY__(self, 0);
            free(block);
        }
    } else {
        __RELEASE_OBJECTS_IN_THE_ARRAY__(self, 0);
    }
    [super dealloc];
}

// Native -[__NSFrozenArrayM copy], 0x1c0160b48. Non-external. The receiver retained (objc_retain, a tail
// call).
- (id)copy {
    return [self retain];
}

// Native -[__NSFrozenArrayM copyWithZone:], 0x1c0160b4c. Non-external. The receiver retained (a tail call).
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native -[__NSFrozenArrayM mutableCopy], 0x1c0160b50. Non-external. A receiver without a control block gets
// one (__cow_create mode 0, installed with a compare-and-swap from NULL, acquire-release; the loser's block
// is freed). The copy is __CFAllocateObject(__NSArrayM, 0), sharing the storage through __cow_copy (kind 0,
// mode 1).
- (id)mutableCopy {
    if (__atomic_load_n(&cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create(self, 0);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject([__ONSArrayM self], 0);
    _cow_copy(self, 0, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSArray_cowCallbacks, copy, 1);
    return copy;
}

// Native -[__NSFrozenArrayM mutableCopyWithZone:], 0x1c0160bec. Non-external. The body of -mutableCopy; the
// zone is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    if (__atomic_load_n(&cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create(self, 0);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject([__ONSArrayM self], 0);
    _cow_copy(self, 0, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSArray_cowCallbacks, copy, 1);
    return copy;
}

@end

// Native cow_copy_instance, 0x1c0160c88 (listed as _cow_copy_instance). Non-external; the first slot of
// ___NSArray_cowCallbacks. Kind and mode 1 select __NSArrayM's ivars, any other value __NSFrozenArrayM's
// (their offset symbols): the source's control block is loaded with acquire and stored into the copy's with
// release, then the source's 24-byte storage is copied into the copy's.
static void cow_copy_instance(id source, int kind, id copy, int mode) {
    CopyOnWriteBlock **sourceCow =
        kind == 1 ? &((__ONSArrayM *)source)->cow : &((__ONSFrozenArrayM *)source)->cow;
    ArrayMStorage *sourceStorage =
        kind == 1 ? &((__ONSArrayM *)source)->storage : &((__ONSFrozenArrayM *)source)->storage;
    CopyOnWriteBlock *block = __atomic_load_n(sourceCow, __ATOMIC_ACQUIRE);
    CopyOnWriteBlock **copyCow = mode == 1 ? &((__ONSArrayM *)copy)->cow : &((__ONSFrozenArrayM *)copy)->cow;
    ArrayMStorage *copyStorage =
        mode == 1 ? &((__ONSArrayM *)copy)->storage : &((__ONSFrozenArrayM *)copy)->storage;
    __atomic_store_n(copyCow, block, __ATOMIC_RELEASE);
    *copyStorage = *sourceStorage;
}

// Native cow_copy_storage, 0x1c0160cfc (listed as _cow_copy_storage). Non-external; the second slot of
// ___NSArray_cowCallbacks, for a __NSArrayM. An array without a buffer keeps a NULL buffer and answers YES.
// Otherwise a new buffer of malloc_type_malloc(8 * capacity, 0x80040B8603338); a failed allocation installs
// NULL (the offset, capacity and count are left as they were) and answers NO. The objects keep their slots:
// the objects up to the old buffer's end (at most the count) are copied with memcpy to the same slots and
// retained there (tagged pointers, bit 63, skipped); when the count runs past the end, the rest are copied to
// the buffer's start and retained (at least one). The new buffer is installed and YES answered; the old one
// stays with the other holders of the control block.
static bool cow_copy_storage(id object) {
    ArrayMStorage *storage = &((__ONSArrayM *)object)->storage;
    id *list = storage->list;
    id *copy = NULL;
    bool result = true;
    if (list != NULL) {
        uint32_t capacity = storage->capacity;
        copy = (id *)malloc_type_malloc((size_t)capacity * sizeof(id), 0x80040B8603338);
        if (copy == NULL) {
            result = false;
        } else {
            uint32_t count = storage->used;
            uint32_t offset = storage->offset;
            uint32_t room = capacity - offset;
            uint32_t first = room < count ? room : count;
            if (first != 0) {
                id *items = copy + offset;
                memcpy(items, list + offset, (size_t)first * sizeof(id));
                NSUInteger remaining = first;
                do {
                    id item = *items++;
                    if ((intptr_t)item >= 0) {
                        [item retain];
                    }
                } while (--remaining != 0);
            }
            if (count > room) {
                uint32_t rest = count - first;
                memcpy(copy, list, (size_t)rest * sizeof(id));
                NSUInteger total = rest > 1 ? rest : 1;
                for (NSUInteger index = 0; index != total; index++) {
                    id item = copy[index];
                    if ((intptr_t)item >= 0) {
                        [item retain];
                    }
                }
            }
        }
    }
    storage->list = copy;
    return result;
}

// Native cow_set_cow, 0x1c0160e14 (listed as _cow_set_cow). Non-external; the third slot of
// ___NSArray_cowCallbacks. The control block is stored into the __NSArrayM's cow ivar with release.
static void cow_set_cow(id object, CopyOnWriteBlock *cow) {
    __atomic_store_n(&((__ONSArrayM *)object)->cow, cow, __ATOMIC_RELEASE);
}
