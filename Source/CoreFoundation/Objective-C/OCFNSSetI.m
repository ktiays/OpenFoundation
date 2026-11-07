//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSSetI.h"
#import "CoreFoundation/Base/OCFUtilities.h"
#import "CoreFoundation/Objective-C/OCFNSFastEnumerationEnumerator.h"
#import "CoreFoundation/Objective-C/OCFNSPlaceholderSet.h"
#import "CoreFoundation/Objective-C/OCFNSSetEnumerate.h"
#import "CoreFoundation/Objective-C/OCFNSSetM.h"
#import "CoreFoundation/Objective-C/OCFNSEmptyCollections.h"

#include <Block.h>
#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

// Inferred organization: the __NSSetI methods (0x1c01e94d4-0x1c01ea743) lie together in the iOS Simulator
// __text section with the unit's C functions among them: the compiler's __block helpers of -member: (listed
// as
// ___Block_byref_object_copy__0 and ___Block_byref_object_dispose__0, nm ___Block_byref_object_copy_ and
// ___Block_byref_object_dispose_, 0x1c01e963c and 0x1c01e964c), the block of
// -enumerateObjectsWithOptions:usingBlock: (0x1c01ea220), __NSSetI_new and its block (0x1c01ea2e8,
// 0x1c01ea45c) before +allocWithZone:, __NSSetI_mutableCopyWithZone (0x1c01ea530) and _compare_clumpiness
// (0x1c01ea72c, listed as __compare_clumpiness_0). Following the owner's decision, the class has its own
// translation unit, which keeps that order (the C functions, which read the instance variables, are defined
// inside the @implementation). The unit's __TEXT,__const statics __NSSetSizes (0x1c03765f8), the const_mu of
// -countByEnumeratingWithState:objects:count: (0x1c0376738, listed as const_mu_3) and __NSSetCapacities
// (0x1c0376740) lie together; the __NSSetM unit's tables are the other pair (0x1c046c520 and 0x1c046c660,
// followed by _mset_countByEnumeratingWithStateObjectsCount.const_mu, 0x1c046c7a0). The two pairs hold the
// same 40 values each (read with Local/Scratch/cf-core/macho.py); their attribution follows the
// __TEXT,__const order of the units' statics and the mset_ name (confidence: medium; batch 4 attributed the
// pairs the other way round). Objective-C compiled with manual reference counting (objc_retain, objc_release,
// objc_autorelease) and blocks. Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json
// (__NSSetI_new) and iphone-simulator-corpus-delta10.json (the methods). Unless a definition says otherwise:
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in
// full). iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md
// (not written yet).

// The __NSSetI class symbol (loaded through the __SCR___NSSetI GOT slot) and the offsets of its ivars _used
// (8: the low 58 bits of a 64-bit word), _szidx (15: the top six bits of that word's last byte) and _list
// (16: the slot array), defined by this unit's @implementation; __NSSetI_new reaches them through the
// symbols, as natively.
extern char setIClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSSetI));
extern const int32_t setIUsedOffset __asm(OF_SYMBOL("OBJC_IVAR_$_") OF_NAME_STRING(__ONSSetI) "._used");
extern const int32_t setISizeIndexOffset __asm(OF_SYMBOL("OBJC_IVAR_$_") OF_NAME_STRING(__ONSSetI) "._szidx");
extern const int32_t setIListOffset __asm(OF_SYMBOL("OBJC_IVAR_$_") OF_NAME_STRING(__ONSSetI) "._list");

// The __NSFastEnumerationEnumerator class symbol (class reference off_2227CE440), sent +allocWithZone:.
extern char setIFastEnumerationEnumeratorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                           OF_NAME_STRING(__ONSFastEnumerationEnumerator));

// Native __NSSetCapacities, 0x1c0376740 (__TEXT,__const), non-external: the largest count of each size index.
static const uint64_t __NSSetCapacities[40] = {
    0,       3,       6,        11,       19,       32,       52,       85,        118,       155,
    237,     390,     672,      1065,     1732,     2795,     4543,     7391,      12019,     19302,
    31324,   50629,   81956,    132580,   214215,   346784,   561026,   907847,    1468567,   2376414,
    3844982, 6221390, 10066379, 16287773, 26354132, 42641916, 68996399, 111638327, 180634415, 292272755,
};

// Native __NSSetSizes, 0x1c03765f8 (__TEXT,__const), non-external: the prime slot count of each size index.
static const uint64_t __NSSetSizes[40] = {
    0,       3,        7,        13,       23,       41,       71,        127,       191,       251,
    383,     631,      1087,     1723,     2803,     4523,     7351,      11959,     19447,     31231,
    50683,   81919,    132607,   214519,   346607,   561109,   907759,    1468927,   2376191,   3845119,
    6221311, 10066421, 16287743, 26354171, 42641881, 68996069, 111638519, 180634607, 292272623, 472907251,
};

static int _compare_clumpiness(const void *first, const void *second);

@implementation __ONSSetI

// Native -[__NSSetI count], 0x1c01e94d4. Non-external. The low 58 bits of the _used word.
- (NSUInteger)count {
    return self->_used;
}

// Native -[__NSSetI member:], 0x1c01e94e8. Non-external. The answer lives in a __block variable (the native
// byref record with the compiler's copy and dispose helpers, 0x1c01e963c and 0x1c01e964c, disposed with
// _Block_object_dispose(..., 8) on return; no block captures it). With a size index, the probe starts at the
// argument's -hash modulo the slot count and steps linearly (wrapping) at most slot-count times; an empty
// slot or the argument itself ends it, as does a slot whose -isEqual: (sent to the stored object) answers
// YES. A probe that ended inside the table answers that slot (nil when it is empty).
- (id)member:(id)object {
    __block id member = nil;
    if (self->_szidx != 0) {
        NSUInteger size = __NSSetSizes[self->_szidx];
        NSUInteger index = [object hash] % size;
        NSUInteger remaining = size;
        id *list = self->_list;
        while (1) {
            id slot = list[index];
            if (slot == nil || slot == object || [slot isEqual:object]) {
                if (index < size) {
                    member = list[index];
                }
                break;
            }
            NSUInteger next = index + 1;
            index = next - (size > next ? 0 : size);
            if (--remaining == 0) {
                break;
            }
        }
    }
    return member;
}

// Native -[__NSSetI objectEnumerator], 0x1c01e9658. Non-external. A new __NSFastEnumerationEnumerator
// (objc_allocWithZone, -initWithObject: the receiver), autoreleased (a tail call of objc_autorelease).
- (id)objectEnumerator {
    id enumerator = [(Class)setIFastEnumerationEnumeratorClass allocWithZone:NULL];
    return [[enumerator initWithObject:self] autorelease];
}

// Native +[__NSSetI automaticallyNotifiesObserversForKey:], 0x1c01e9688. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSSetI clumpingFactor], 0x1c01e9690. Non-external. The objects are fetched with
// -getObjects:count: into a temporary buffer (on the stack for fewer than 257, max(count, 1) entries;
// otherwise _CFCreateArrayStorage(max(count, 1), false, &size), freed at the end; no overflow check), and the
// count is read again. With at least two objects, their -hash values modulo the slot count go into
// malloc_type_malloc(8 * count + 8, 0x100004000313F17), are sorted with qsort (_compare_clumpiness) and
// closed with the first plus the slot count; the squares of each gap's shortfall below the average gap (slots
// / count) are summed with fused multiply-adds, and the factor is sum * count * count / (slots * slots) /
// (count + -1.0), in that operation order. Fewer objects answer 0.
- (double)clumpingFactor {
    NSUInteger count = self->_used;
    unsigned long bufferCount = count > 1 ? count : 1;
    id stackObjects[count < 0x101 ? bufferCount : 1];
    size_t storageSize = 0;
    id *heapObjects = NULL;
    id *objects = stackObjects;
    if (count >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(bufferCount, false, &storageSize);
        count = self->_used;
        heapObjects = objects;
    }
    [self getObjects:objects count:count];
    count = self->_used;
    double factor = 0.0;
    if (count >= 2) {
        uint64_t slots = __NSSetSizes[self->_szidx];
        uint64_t *hashes = (uint64_t *)malloc_type_malloc(count * 8 + 8, 0x100004000313F17);
        for (NSUInteger index = 0; index != count; index++) {
            hashes[index] = [objects[index] hash] % slots;
        }
        qsort(hashes, count, 8, _compare_clumpiness);
        hashes[count] = hashes[0] + slots;
        double size = (double)slots;
        double objectCount = (double)count;
        double average = size / objectCount;
        double sum = 0.0;
        for (NSUInteger index = 0; index != count; index++) {
            double gap = (double)(hashes[index + 1] - hashes[index]);
            double shortfall = average > gap ? average - gap : 0.0;
            sum = fma(shortfall, shortfall, sum);
        }
        free(hashes);
        factor = sum * objectCount * objectCount / (size * size) / (objectCount + -1.0);
    }
    free(heapObjects);
    return factor;
}

// Native -[__NSSetI clumpingInterestingThreshold], 0x1c01e98a4. Non-external. With count and slots as
// doubles: (1 - (count + count) / slots) + count * count / slots / slots, in that operation order.
- (double)clumpingInterestingThreshold {
    double count = (double)self->_used;
    double size = (double)__NSSetSizes[self->_szidx];
    double linear = 1.0 - (count + count) / size;
    return linear + count * count / size / size;
}

// Native -[__NSSetI countByEnumeratingWithState:objects:count:], 0x1c01e98fc. Non-external. A NULL buffer
// with a nonzero length raises first, then a length with any of its top three bits set (os_log pack buffer
// 0x16 each: the method name and the length). state->state is the next slot; the first call points
// mutationsPtr at the constant const_mu (1). Past the table, or for a zero length, the answer is 0 (itemsPtr
// is the buffer once the state is inside the table). Each slot visited advances and stores state->state; an
// occupied slot is written to the buffer and the state reread; the walk ends at the table's end or when the
// buffer is full.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    // Native countByEnumeratingWithState:objects:count:.const_mu (listed as const_mu_3), 0x1c0376738
    // (__TEXT,__const; confidence: medium for the address).
    static const unsigned long const_mu = 1;
    FastEnumerationState *enumerationState = state;
    if (buffer == NULL && length != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(__ONSSetI) " countByEnumeratingWithState:objects:count:]",
            (unsigned long)length
        );
    }
    if ((OF_WORD_SHIFT(length, 61)) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSSetI) " countByEnumeratingWithState:objects:count:]",
            (unsigned long)length
        );
    }
    uint64_t size = __NSSetSizes[self->_szidx];
    unsigned long position = enumerationState->state;
    if (position == 0) {
        enumerationState->mutationsPtr = (unsigned long *)&const_mu;
    }
    if (size <= position) {
        return 0;
    }
    enumerationState->itemsPtr = buffer;
    if (length == 0) {
        return 0;
    }
    NSUInteger produced = 0;
    do {
        id object = self->_list[position];
        position += 1;
        enumerationState->state = position;
        if (object != nil) {
            buffer[produced++] = object;
            position = enumerationState->state;
        }
    } while (position < size && produced < length);
    return produced;
}

// Native -[__NSSetI containsObject:], 0x1c01e9b3c. Non-external. The probe of -member:, answering whether the
// slot it ended on inside the table is occupied; the answer lives in a __block BOOL (byref record without
// helpers, disposed with _Block_object_dispose on return; no block captures it).
- (BOOL)containsObject:(id)object {
    __block BOOL contains = NO;
    if (self->_szidx != 0) {
        NSUInteger size = __NSSetSizes[self->_szidx];
        NSUInteger index = [object hash] % size;
        NSUInteger remaining = size;
        id *list = self->_list;
        while (1) {
            id slot = list[index];
            if (slot == nil || slot == object || [slot isEqual:object]) {
                if (index < size) {
                    contains = list[index] != nil;
                }
                break;
            }
            NSUInteger next = index + 1;
            index = next - (size > next ? 0 : size);
            if (--remaining == 0) {
                break;
            }
        }
    }
    return contains;
}

// Native -[__NSSetI countForObject:], 0x1c01e9c84. Non-external. The body of -containsObject: with a __block
// NSUInteger (1 or 0).
- (NSUInteger)countForObject:(id)object {
    __block NSUInteger count = 0;
    if (self->_szidx != 0) {
        NSUInteger size = __NSSetSizes[self->_szidx];
        NSUInteger index = [object hash] % size;
        NSUInteger remaining = size;
        id *list = self->_list;
        while (1) {
            id slot = list[index];
            if (slot == nil || slot == object || [slot isEqual:object]) {
                if (index < size) {
                    count = list[index] != nil;
                }
                break;
            }
            NSUInteger next = index + 1;
            index = next - (size > next ? 0 : size);
            if (--remaining == 0) {
                break;
            }
        }
    }
    return count;
}

// Native -[__NSSetI getObjects:count:], 0x1c01e9dcc. Non-external. The argument checks of
// -countByEnumeratingWithState:objects:count: (with this method's name); then, with a size index, the
// occupied slots are copied in order until min(count, _used) objects are written or the table ends.
- (void)getObjects:(id *)objects count:(NSUInteger)count {
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(__ONSSetI) " getObjects:count:]",
            (unsigned long)count
        );
    }
    if ((OF_WORD_SHIFT(count, 61)) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSSetI) " getObjects:count:]",
            (unsigned long)count
        );
    }
    NSUInteger sizeIndex = self->_szidx;
    NSUInteger remaining = self->_used;
    if (count < remaining) {
        remaining = count;
    }
    if (sizeIndex == 0 || remaining == 0) {
        return;
    }
    uint64_t size = __NSSetSizes[sizeIndex];
    id *list = self->_list;
    for (NSUInteger slot = 1;; slot++) {
        id object = *list++;
        if (object != nil) {
            *objects++ = object;
            remaining -= 1;
        }
        if (slot >= size || remaining == 0) {
            break;
        }
    }
}

// Native -[__NSSetI enumerateObjectsWithOptions:usingBlock:], 0x1c01e9ff4, with its block
// ___51-[__NSSetI enumerateObjectsWithOptions:usingBlock:]_block_invoke (0x1c01ea220). Non-external. A nil
// block raises (os_log pack buffer 0xC: the method name). The concurrent option (bit 0) is dropped when
// __CFActiveProcessorCount answers fewer than 2. Concurrently, dispatch_apply (slot-count iterations, NULL
// queue) runs the block, which returns at once when the shared __block stop flag reads set (acquire), skips
// empty slots, and calls out each object with a fresh stop flag inside an autorelease pool, setting the
// shared flag (release) when that flag is exactly 1; the shared flag's byref record is disposed afterwards.
// Otherwise, with a size index, the occupied slots are called out in order, each inside an autorelease pool,
// until the stop flag's bit 0 is set or the table ends.
- (void)enumerateObjectsWithOptions:(NSUInteger)options usingBlock:(void (^)(id object, BOOL *stop))block {
    if (block == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: block cannot be nil",
            "-[" OF_NAME_STRING(__ONSSetI) " enumerateObjectsWithOptions:usingBlock:]"
        );
    }
    NSUInteger sizeIndex = self->_szidx;
    uint64_t size = __NSSetSizes[sizeIndex];
    if ((options & 1) != 0 && (NSInteger)__CFActiveProcessorCount() < 2) {
        options = 0;
    }
    if ((options & 1) != 0) {
        __block BOOL stop = NO;
        dispatch_apply(size, NULL, ^(size_t index) {
          if ((__atomic_load_n(&stop, __ATOMIC_ACQUIRE) & 1) != 0) {
              return;
          }
          id object = self->_list[index];
          if (object == nil) {
              return;
          }
          void *pool = _OCFAutoreleasePoolPush();
          BOOL localStop = NO;
          __NSSET_IS_CALLING_OUT_TO_A_BLOCK__(block, object, &localStop);
          _OCFAutoreleasePoolPop(pool);
          if (localStop == 1) {
              __atomic_store_n(&stop, YES, __ATOMIC_RELEASE);
          }
        });
        return;
    }
    BOOL stop = NO;
    if (sizeIndex == 0) {
        return;
    }
    id *list = self->_list;
    for (NSUInteger slot = 1;; slot++) {
        id object = *list++;
        if (object != nil) {
            void *pool = _OCFAutoreleasePoolPush();
            __NSSET_IS_CALLING_OUT_TO_A_BLOCK__(block, object, &stop);
            _OCFAutoreleasePoolPop(pool);
            if ((stop & 1) != 0) {
                return;
            }
        }
        if (slot >= size) {
            return;
        }
    }
}

// Native __NSSetI_new, 0x1c01ea2e8, with ____NSSetI_new_block_invoke (0x1c01ea45c). Hidden (was a private
// external). The size index is the first capacity not below the count; the object is __CFAllocateObject of
// +[__NSSetI self] with room for that many slots, and the index is stored into the top six bits of the
// _szidx byte. Each object is inserted by the block: open addressing from -hash modulo the slot count,
// probing linearly (wrapping) at most slot-count times past slots that are neither empty, identical nor
// -isEqual:; an empty slot takes the object (retained unless it is a tagged pointer or nil, or bit 0 of
// noRetain is set) and increments the 58-bit _used count, an equal object is skipped. The first object is
// passed to the block's invoke function directly. CoreFoundation defect: the native search runs to 64
// entries of the 40-entry capacity table before trapping, reading the bytes that follow it (in this unit not
// another capacity table: the entry after the last is 0x412E848000000000, so a count above the largest
// capacity selects index 40 and reads a slot count from the 8 bytes past __NSSetSizes, the const_mu word 1);
// this restoration traps for any count above the largest capacity without reading past the table
// (confidence: verified for the code, medium for which bytes follow the table, see the unit's organization
// note). An exhausted probe would select slot index slot-count (past the list); the capacities keep a free
// slot, so that path is unreachable and kept.
id __NSSetI_new(const id *objects, va_list args, NSUInteger count, BOOL noRetain) {
    NSUInteger sizeIndex = 0;
    while (count > __NSSetCapacities[sizeIndex]) {
        if (++sizeIndex == 40) {
            __builtin_trap();
        }
    }
    NSUInteger size = __NSSetSizes[sizeIndex];
    id set = __CFAllocateObject([(Class)setIClass self], size * sizeof(id));
    uint8_t *sizeIndexByte = (uint8_t *)set + setISizeIndexOffset;
    *sizeIndexByte = (uint8_t)((*sizeIndexByte & 3) | (sizeIndex << 2));
    if (count == 0) {
        return set;
    }
    id *list = (id *)((uint8_t *)set + setIListOffset);
    void (^addObject)(id object) = ^(id object) {
      NSUInteger index = (NSUInteger)[object hash] % size;
      NSUInteger remaining = size;
      while (1) {
          id slot = list[index];
          if (slot == nil || slot == object) {
              break;
          }
          if ([slot isEqual:object]) {
              break;
          }
          index = index + 1 - (size > index + 1 ? 0 : size);
          if (--remaining == 0) {
              index = size;
              break;
          }
      }
      if (list[index] == nil) {
          list[index] = object;
          if ((intptr_t)object >= 1 && !noRetain) {
              objc_retain(object);
          }
          uint64_t *used = (uint64_t *)((uint8_t *)set + setIUsedOffset);
          *used = (*used & 0xFC00000000000000ULL) | ((*used + 1) & 0x3FFFFFFFFFFFFFFULL);
      }
    };
    if (args != NULL) {
        addObject(objects[0]);
        while (--count != 0) {
            addObject(va_arg(args, id));
        }
    } else {
        do {
            addObject(*objects++);
        } while (--count != 0);
    }
    return set;
}

// Native +[__NSSetI allocWithZone:], 0x1c01ea520. Non-external. The immutable set placeholder (a tail call);
// the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSSetImmutablePlaceholder();
}

// Native -[__NSSetI copy], 0x1c01ea524. Non-external. A tail call of objc_retain.
- (id)copy {
    return [self retain];
}

// Native -[__NSSetI copyWithZone:], 0x1c01ea528. Non-external. A tail call of objc_retain.
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

static id _NSSetI_mutableCopyWithZone(__ONSSetI *set);

// Native -[__NSSetI mutableCopy], 0x1c01ea52c. Non-external. A tail call of __NSSetI_mutableCopyWithZone.
- (id)mutableCopy {
    return _NSSetI_mutableCopyWithZone(self);
}

// Native __NSSetI_mutableCopyWithZone, 0x1c01ea530. Non-external. The objects are fetched with
// -getObjects:count: into a temporary buffer (on the stack for fewer than 257, max(count, 1) entries;
// otherwise _CFCreateArrayStorage(max(count, 1), false, &size), freed afterwards; no overflow check) and
// handed to __NSSetM_new, which retains them.
static id _NSSetI_mutableCopyWithZone(__ONSSetI *set) {
    NSUInteger count = set->_used;
    unsigned long bufferCount = count > 1 ? count : 1;
    id stackObjects[count < 0x101 ? bufferCount : 1];
    size_t storageSize = 0;
    id *heapObjects = NULL;
    id *objects = stackObjects;
    if (count >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(bufferCount, false, &storageSize);
        heapObjects = objects;
    }
    [set getObjects:objects count:count];
    id copy = __NSSetM_new(objects, count, NO);
    free(heapObjects);
    return copy;
}

// Native -[__NSSetI mutableCopyWithZone:], 0x1c01ea634. Non-external. A tail call of
// __NSSetI_mutableCopyWithZone (the zone is not passed).
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return _NSSetI_mutableCopyWithZone(self);
}

// Native -[__NSSetI dealloc], 0x1c01ea638. Non-external. The empty set __NSSet0__ is a BRK #1. Otherwise the
// size index and the count are cleared (the _szidx bits first, then the _used bits), and with a size index
// every slot of the table (at least one) is released unless it is nil or a tagged pointer (signed value below
// 1); then super's -dealloc.
- (void)dealloc {
    if (self == __NSSet0__) {
        __builtin_trap();
    }
    NSUInteger sizeIndex = self->_szidx;
    uint64_t size = __NSSetSizes[sizeIndex];
    self->_szidx = 0;
    self->_used = 0;
    if (sizeIndex != 0) {
        uint64_t slots = size > 1 ? size : 1;
        id *list = self->_list;
        do {
            id object = *list++;
            if ((intptr_t)object >= 1) {
                [object release];
            }
        } while (--slots != 0);
    }
    [super dealloc];
}

// Native _compare_clumpiness, 0x1c01ea72c (listed as __compare_clumpiness_0). Non-external. The qsort order
// of the hash values: -1, 0 or 1 by unsigned comparison.
static int _compare_clumpiness(const void *first, const void *second) {
    uint64_t left = *(const uint64_t *)first;
    uint64_t right = *(const uint64_t *)second;
    if (left < right) {
        return -1;
    }
    return left > right;
}

@end
