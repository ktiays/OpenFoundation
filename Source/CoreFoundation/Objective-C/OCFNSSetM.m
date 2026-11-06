//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSSetM.h"
#import "CoreFoundation/Objective-C/OCFNSDictionaryConcurrentEnumeration.h"
#import "CoreFoundation/Objective-C/OCFNSFastEnumerationEnumerator.h"
#import "CoreFoundation/Objective-C/OCFNSPlaceholderSet.h"
#import "CoreFoundation/Objective-C/OCFNSSetEnumerate.h"

#include <malloc/malloc.h>
#include <math.h>
#include <os/lock.h>
#include <stdint.h>
#include <stdlib.h>

// Inferred organization: the __NSSetM unit spans 0x1c02a4708-0x1c02a752f of the iOS Simulator __text section:
// the __NSSetM methods with the unit's C functions among them (__rehashs 0x1c02a4d34 after -addObject:,
// __NSSetM_new 0x1c02a5edc after -removeObject:, __RELEASE_OBJECTS_IN_THE_SET__ 0x1c02a61d0 after -dealloc,
// __NSSetM_copy 0x1c02a62dc between -copy and -copyWithZone:), the __NSFrozenSetM methods
// (0x1c02a64b4-0x1c02a72c3, with the non-external __NSFrozenSetM_mutableCopy 0x1c02a71f8 among them; their
// -dealloc .cold.1 at 0x1c030fdfc), the copy-on-write callbacks cow_copy_instance, cow_copy_storage and
// cow_set_cow
// (0x1c02a72c4, 0x1c02a7328, 0x1c02a7414, listed with an _1 suffix: the set unit's own copies; the array,
// ordered set and dictionary units have theirs), _compare_clumpiness (0x1c02a7428, listed as
// __compare_clumpiness_1), the block of mset_enumerateObjectsWithOptionsUsingBlock (0x1c02a7440) and
// OUTLINED_FUNCTION_0-_6 (0x1c02a74b0-0x1c02a752f, listed as _0_45, _1_32, _2_28, _3_13, _4_11, _5_11, _6_10;
// folded into their callers). The cold region holds -mutableCopy and -mutableCopyWithZone: (0x1c030fca0,
// 0x1c030fd2c) and the methods' .cold.1 pieces (0x1c030fdb8-0x1c030fdfb, folded into their methods).
// Following the owner's decision, the class has its own translation unit, which keeps the native order (the C
// functions, which reach the instance variables, are defined inside the @implementation; the cold methods
// close it); the __NSFrozenSetM methods are a second @implementation at their native position, and the
// callbacks follow it. The unit's statics: __NSSetCapacities (0x1c046c520) and __NSSetSizes (0x1c046c660),
// followed by _mset_countByEnumeratingWithStateObjectsCount.const_mu (0x1c046c7a0, the value 1, read with
// Local/Scratch/cf-core/macho.py; the function-local static of -[__NSFrozenSetM
// countByEnumeratingWithState:objects:count:], its only reader),
// listed with an _0 suffix (the attribution of the two table pairs follows the __TEXT,__const order of the
// units' statics and the mset_ name, confidence: medium; batch 4 attributed the pairs the other way round),
// ___NSSetM_DeletedMarker (0x2227a8660) and ___NSSet_cowCallbacks (0x2227a8640, read with
// Local/Scratch/cf-core/macho.py). Objective-C compiled with manual reference counting (objc_retain,
// objc_release, objc_autorelease) and blocks. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json
// (__NSSetM_new), iphone-simulator-core-corpus.json (__RELEASE_OBJECTS_IN_THE_SET__) and
// iphone-simulator-corpus-delta10.json (the methods, the other functions and the callbacks) and
// iphone-simulator-corpus-delta11.json (family L-collection-leftovers: the __NSFrozenSetM methods). Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The __NSSetM class symbol (loaded through the __SCR___NSSetM GOT slot) and the offsets of its ivars cow and
// storage, defined by this unit's @implementation; __NSSetM_new reaches them through the symbols, as
// natively.
extern char setMClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSSetM));
extern const int32_t setMCowOffset __asm("_OBJC_IVAR_$_" OF_NAME_STRING(__ONSSetM) ".cow");
extern const int32_t setMStorageOffset __asm("_OBJC_IVAR_$_" OF_NAME_STRING(__ONSSetM) ".storage");

// The __NSFastEnumerationEnumerator class symbol (class reference off_2227CE440), sent +allocWithZone:.
extern char setMFastEnumerationEnumeratorClass[] __asm(
    "_OBJC_CLASS_$_" OF_NAME_STRING(__ONSFastEnumerationEnumerator)
);

// The NSObject class (libobjc's), the isa of the deleted-slot marker.
extern char setMObjectClass[] __asm("_OBJC_CLASS_$_NSObject");

// Native __NSSetCapacities, 0x1c046c520 (__TEXT,__const, listed as ___NSSetCapacities_0), non-external: the
// largest count of each size index.
static const uint64_t __NSSetCapacities[40] = {
    0,       3,       6,        11,       19,       32,       52,       85,        118,       155,
    237,     390,     672,      1065,     1732,     2795,     4543,     7391,      12019,     19302,
    31324,   50629,   81956,    132580,   214215,   346784,   561026,   907847,    1468567,   2376414,
    3844982, 6221390, 10066379, 16287773, 26354132, 42641916, 68996399, 111638327, 180634415, 292272755,
};

// Native __NSSetSizes, 0x1c046c660 (__TEXT,__const, listed as ___NSSetSizes_0), non-external: the prime slot
// count of each size index; the methods read its low 32 bits (LDR W) except __NSSetM_new and -enumerate....
static const uint64_t __NSSetSizes[40] = {
    0,       3,        7,        13,       23,       41,       71,        127,       191,       251,
    383,     631,      1087,     1723,     2803,     4523,     7351,      11959,     19447,     31231,
    50683,   81919,    132607,   214519,   346607,   561109,   907759,    1468927,   2376191,   3845119,
    6221311, 10066421, 16287743, 26354171, 42641881, 68996069, 111638519, 180634607, 292272623, 472907251,
};

// Native ___NSSetM_DeletedMarker, 0x2227a8660 (__DATA_CONST,__const, non-external): the marker a removed
// slot holds, a 16-byte object record whose isa is NSObject and whose second word is 0 (read with macho.py;
// confidence: verified; the isa 0x22e6b6088 lies outside the extracted image and is NSObject by the SDK
// hierarchy, confidence: medium). DeletedMarker is a local descriptive type name.
typedef struct {
    void *isa;
    uintptr_t reserved;
} DeletedMarker;

static const DeletedMarker ___NSSetM_DeletedMarker = {setMObjectClass, 0};

// The deleted marker as an object pointer. deletedMarker is a local descriptive name.
static inline id deletedMarker(void) {
    return (id)&___NSSetM_DeletedMarker;
}

// The low 32 bits of a slot count, as the methods load them. setSizeAtIndex is a local descriptive name.
static inline uint32_t setSizeAtIndex(NSUInteger index) {
    return (uint32_t)__NSSetSizes[index];
}

static void cow_copy_instance(id source, int kind, id copy, int mode);

static bool cow_copy_storage(id object);

static void cow_set_cow(id object, CopyOnWriteBlock *cow);

static void __RELEASE_OBJECTS_IN_THE_SET__(id object, int kind);

// Native ___NSSet_cowCallbacks, 0x2227a8640 (__DATA_CONST,__const). Non-external. The set unit's
// cow_copy_instance, cow_copy_storage and cow_set_cow, and __RELEASE_OBJECTS_IN_THE_SET__ as the cleanup slot
// (read with macho.py).
static const CopyOnWriteCallbacks __NSSet_cowCallbacks = {
    cow_copy_instance,
    cow_copy_storage,
    cow_set_cow,
    __RELEASE_OBJECTS_IN_THE_SET__,
};

static void __rehashs(__ONSSetM *set, uint32_t sizeIndex);

static id __NSSetM_copy(__ONSSetM *set);

static int _compare_clumpiness(const void *first, const void *second);

static id __NSFrozenSetM_mutableCopy(__ONSFrozenSetM *set);

// The probe the lookups inline, on the slot array loaded before the -hash: from the object's -hash modulo the
// slot count, at most size slots in order (wrapping to 0). An empty slot ends it; a deleted marker is counted
// (when deletedCount is not NULL) and skipped; a slot holding the object itself, or whose -isEqual: (sent to
// the stored object) answers YES, ends it. The answer is that slot, else the first deleted or empty slot
// passed, else size. setFindSlot is a local descriptive name.
static inline NSUInteger
setFindSlot(id *list, NSUInteger size, id object, NSUInteger hash, NSUInteger *deletedCount) {
    NSUInteger index = hash % size;
    NSUInteger target = size;
    for (NSUInteger probes = size; probes != 0; probes--) {
        id slot = list[index];
        if (slot == nil) {
            if (target == size) {
                target = index;
            }
            break;
        }
        if (slot == deletedMarker()) {
            if (deletedCount != NULL) {
                *deletedCount += 1;
            }
            if (target == size) {
                target = index;
            }
        } else if (slot == object || [slot isEqual:object]) {
            target = index;
            break;
        }
        NSUInteger next = index + 1;
        index = next - (size > next ? 0 : size);
    }
    return target;
}

// The copy-on-write and mutation-counter prologue every mutating method inlines (and -_mutate is): the
// control block is loaded with acquire and the next counter computed (0 once bits 9-31 of the current one
// reach 125). Under the block's lock a block without a mutable reference crashes (the methods' .cold.1:
// OUTLINED_FUNCTION_0, __cow_failure with "Attempting to mutate immutable collection!"); unless this set is
// the block's only reference, its mutable count drops and __cow_mutate_slow gives the set its own storage.
// The counter is then stored. setMutate is a local descriptive name.
static inline void setMutate(__ONSSetM *set) {
    CopyOnWriteBlock *cow = __atomic_load_n(&set->cow, __ATOMIC_ACQUIRE);
    uint32_t mutations = set->storage.mutations;
    uint32_t nextMutations = (mutations >> 9) >= 125 ? 0 : mutations + 1;
    if (cow != NULL) {
        os_unfair_lock_lock(&cow->lock);
        uint16_t mutableReferences = cow->mutableReferences;
        if (mutableReferences == 0) {
            _cow_failure(OF_STR("Attempting to mutate immutable collection!"));
        }
        uint16_t remaining = mutableReferences - 1;
        if (remaining != 0 || cow->immutableReferences != 0) {
            cow->mutableReferences = remaining;
            _cow_mutate_slow((id)set, cow, &__NSSet_cowCallbacks);
        }
        os_unfair_lock_unlock(&cow->lock);
    }
    set->storage.mutations = nextMutations;
}

// The control block -mutableCopy, -mutableCopyWithZone: and __NSSetM_copy make sure of (OUTLINED_FUNCTION_5
// and _6): without one, __cow_create(set, 1) is installed with a compare-and-swap from NULL
// (acquire-release); the loser's block is freed. setEnsureCow is a local descriptive name.
static inline void setEnsureCow(__ONSSetM *set) {
    if (__atomic_load_n(&set->cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create((id)set, 1);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &set->cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
}

// Native mset_enumerateObjectsWithOptionsUsingBlock, inlined into -[__NSSetM
// enumerateObjectsWithOptions:usingBlock:] and -[__NSFrozenSetM enumerateObjectsWithOptions:usingBlock:] on
// their storage (no out-of-line copy is in the image; its block,
// ___mset_enumerateObjectsWithOptionsUsingBlock_block_invoke 0x1c02a7440, keeps the name). The slot count
// (its low 32 bits) and the slot array are read once. __NSCollectionHandleConcurrentEnumerationIfSpecified
// (options, no per-call pool, slot-count iterations) runs the block, which skips empty and deleted slots and
// calls out each object inside an autorelease pool with the shared stop flag; when it does not handle the
// enumeration, the occupied slots are called out in order, each inside an autorelease pool, until the stop
// flag's bit 0 is set or the table ends.
static inline __attribute__((always_inline)) void mset_enumerateObjectsWithOptionsUsingBlock(
    SetMStorage *storage, NSUInteger options, void (^block)(id, BOOL *)
) {
    uint32_t size = (uint32_t)__NSSetSizes[storage->state.word >> 26];
    id *list = storage->list;
    if (__NSCollectionHandleConcurrentEnumerationIfSpecified(options, NO, size, ^(size_t index, BOOL *stop) {
          id object = list[index];
          if (object == nil || object == deletedMarker()) {
              return;
          }
          void *pool = _OCFAutoreleasePoolPush();
          __NSSET_IS_CALLING_OUT_TO_A_BLOCK__(block, object, stop);
          _OCFAutoreleasePoolPop(pool);
        })) {
        return;
    }
    BOOL stop = NO;
    if (size == 0) {
        return;
    }
    for (uint32_t slot = 1;; slot++) {
        id object = list[slot - 1];
        if (object != nil && object != deletedMarker()) {
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

// Native mset_getObjectsCount, inlined into -[__NSSetM getObjects:count:] and -[__NSFrozenSetM
// getObjects:count:] on their storage (its name is the %s of the raises).
// A NULL array with a nonzero count raises first, then a count with any of its top three bits set (os_log
// pack buffer 0x16 each). With a size index, the occupied slots (the slot array reread for each) are copied
// in order until min(count, used) objects are written (a 32-bit countdown) or the table ends.
static inline __attribute__((always_inline)) void
mset_getObjectsCount(SetMStorage *storage, id *objects, NSUInteger count) {
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "mset_getObjectsCount",
            (unsigned long)count
        );
    }
    if ((count >> 61) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "mset_getObjectsCount",
            (unsigned long)count
        );
    }
    uint32_t state = storage->state.word;
    NSUInteger used = state & 0x3FFFFFF;
    uint32_t remaining = (uint32_t)(count < used ? count : used);
    if (remaining == 0 || (state >> 26) == 0) {
        return;
    }
    uint32_t size = setSizeAtIndex(state >> 26);
    for (NSUInteger slot = 0;;) {
        id object = storage->list[slot];
        if (object != nil && object != deletedMarker()) {
            *objects++ = object;
            remaining -= 1;
        }
        if (remaining == 0) {
            return;
        }
        slot += 1;
        if (slot >= size) {
            return;
        }
    }
}

@implementation __ONSSetM

// Native -[__NSSetM count], 0x1c02a4708. Non-external. The ThreadSanitizer read check, then the 26-bit count.
- (NSUInteger)count {
    setTSANRead(self, __builtin_return_address(0));
    return self->storage.state.used;
}

// Native -[__NSSetM member:], 0x1c02a4764. Non-external. The ThreadSanitizer read check; an empty table (size
// index 0) answers nil. Otherwise the probe on the slot array loaded before -hash; a slot it found inside the
// table is reread from the current slot array and answered unless it is the deleted marker.
- (id)member:(id)object {
    setTSANRead(self, __builtin_return_address(0));
    uint32_t state = self->storage.state.word;
    if ((state >> 26) == 0) {
        return nil;
    }
    uint32_t size = setSizeAtIndex(state >> 26);
    id *list = self->storage.list;
    NSUInteger slot = setFindSlot(list, size, object, [object hash], NULL);
    if (slot >= size) {
        return nil;
    }
    id stored = self->storage.list[slot];
    return stored == deletedMarker() ? nil : stored;
}

// Native -[__NSSetM objectEnumerator], 0x1c02a4894. Non-external. The ThreadSanitizer read check, then a new
// __NSFastEnumerationEnumerator (objc_allocWithZone, -initWithObject: the receiver), autoreleased.
- (id)objectEnumerator {
    setTSANRead(self, __builtin_return_address(0));
    id enumerator = [(Class)setMFastEnumerationEnumeratorClass allocWithZone:NULL];
    return [[enumerator initWithObject:self] autorelease];
}

// Native -[__NSSetM _mutate], 0x1c02a48f0, with its .cold.1 (0x1c030fdb8). Non-external. The mutation
// prologue.
- (void)_mutate {
    setMutate(self);
}

// Native +[__NSSetM automaticallyNotifiesObserversForKey:], 0x1c02a498c. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSSetM addObject:], 0x1c02a4994, with its .cold.1 (0x1c030fdc4). Non-external. The
// ThreadSanitizer write check; a nil object raises (os_log pack buffer 0xC: the method name); the mutation
// prologue. A size index of 40 or more is BRK #1. An empty table gets size index 1 and a 24-byte slot array
// (malloc_type_calloc(1, 0x18, 0x80040B8603338), not checked). The state word is kept as read for the count.
// The probe (counting deleted markers) is repeated after __rehashs at the next size index when it found no
// slot, or at the same index when it passed more than 15 deleted markers. A slot holding an equal object
// leaves the set unchanged; otherwise the object is retained unless it is a tagged pointer (bit 63), stored,
// and the 26-bit count incremented; when the size index's capacity is below the count read before the
// insertion, __rehashs grows the table to the next index (a tail call).
- (void)addObject:(id)object {
    setTSANWrite(self, __builtin_return_address(0));
    if (object == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: object cannot be nil",
            "-[" OF_NAME_STRING(__ONSSetM) " addObject:]"
        );
    }
    setMutate(self);
    uint32_t state = self->storage.state.word;
    if ((state >> 29) >= 5) {
        __builtin_trap();
    }
    uint32_t sizeIndex;
    uint32_t savedState;
    uint32_t size;
    if ((state >> 26) == 0) {
        self->storage.state.word = state | 0x4000000;
        sizeIndex = 1;
        self->storage.list = (id *)malloc_type_calloc(1, 0x18, 0x80040B8603338);
        savedState = self->storage.state.word;
        size = 3;
    } else {
        savedState = state;
        sizeIndex = state >> 26;
        size = setSizeAtIndex(sizeIndex);
    }
    id *list;
    NSUInteger slot;
    while (1) {
        list = self->storage.list;
        NSUInteger deletedCount = 0;
        slot = setFindSlot(list, size, object, [object hash], &deletedCount);
        if (slot < size && deletedCount <= 15) {
            break;
        }
        __rehashs(self, sizeIndex + (slot >= size ? 1 : 0));
        sizeIndex = self->storage.state.word >> 26;
        size = setSizeAtIndex(sizeIndex);
    }
    id stored = list[slot];
    if (stored != deletedMarker() && stored != nil) {
        return;
    }
    uint32_t used = savedState & 0x3FFFFFF;
    if (((uintptr_t)object >> 63) == 0) {
        objc_retain(object);
    }
    list[slot] = object;
    self->storage.state.used += 1;
    if (__NSSetCapacities[sizeIndex] < used) {
        __rehashs(self, sizeIndex + 1);
    }
}

// Native __rehashs, 0x1c02a4d34. Non-external. A new slot array of the given size index's slot count (low 32
// bits; malloc_type_calloc(1, 8 * size, 0x80040B8603338), not checked); with a size index, every slot of the
// old table (at least one) that holds an object is placed at the first empty slot from the object's -hash
// modulo the new size (probing linearly, wrapping, without a bound). The old array is freed and the new one
// and the size index installed (the count is kept).
static void __rehashs(__ONSSetM *set, uint32_t sizeIndex) {
    uint32_t newSize = setSizeAtIndex(sizeIndex);
    id *newList = (id *)malloc_type_calloc(1, (size_t)newSize << 3, 0x80040B8603338);
    uint32_t state = set->storage.state.word;
    id *oldList = set->storage.list;
    if ((state >> 26) != 0) {
        uint32_t oldSize = setSizeAtIndex(state >> 26);
        NSUInteger slots = oldSize > 1 ? oldSize : 1;
        for (NSUInteger slot = 0; slot != slots; slot++) {
            id object = oldList[slot];
            if (object == nil || object == deletedMarker()) {
                continue;
            }
            NSUInteger index = [object hash] % newSize;
            while (newList[index] != nil) {
                NSUInteger next = index + 1;
                index = next - (newSize > next ? 0 : newSize);
            }
            newList[index] = object;
        }
    }
    free(oldList);
    set->storage.list = newList;
    set->storage.state.szidx = sizeIndex;
}

// Native -[__NSSetM containsObject:], 0x1c02a4e4c. Non-external. The lookup of -member:, answering whether
// the slot found holds an object (neither nil nor the deleted marker).
- (BOOL)containsObject:(id)object {
    setTSANRead(self, __builtin_return_address(0));
    uint32_t state = self->storage.state.word;
    if ((state >> 26) == 0) {
        return NO;
    }
    uint32_t size = setSizeAtIndex(state >> 26);
    id *list = self->storage.list;
    NSUInteger slot = setFindSlot(list, size, object, [object hash], NULL);
    if (slot >= size) {
        return NO;
    }
    id stored = self->storage.list[slot];
    return stored != deletedMarker() && stored != nil;
}

// Native -[__NSSetM countForObject:], 0x1c02a4f80. Non-external. The body of -containsObject: as a count.
- (NSUInteger)countForObject:(id)object {
    setTSANRead(self, __builtin_return_address(0));
    uint32_t state = self->storage.state.word;
    if ((state >> 26) == 0) {
        return 0;
    }
    uint32_t size = setSizeAtIndex(state >> 26);
    id *list = self->storage.list;
    NSUInteger slot = setFindSlot(list, size, object, [object hash], NULL);
    if (slot >= size) {
        return 0;
    }
    id stored = self->storage.list[slot];
    return stored != deletedMarker() && stored != nil;
}

// Native -[__NSSetM clumpingFactor], 0x1c02a50b4. Non-external. The 32-bit count is read once; the objects
// are fetched with -getObjects:count: into a temporary buffer (on the stack for fewer than 257, max(count, 1)
// entries; otherwise _CFCreateArrayStorage(count, false, &size), freed at the end; no overflow check). With
// at least two objects, their -hash values modulo the slot count (low 32 bits) go into malloc_type_malloc(8 *
// count + 8, 0x100004000313F17), are sorted with qsort (_compare_clumpiness) and closed with the first plus
// the slot count; the squares of each gap's shortfall below the average gap (slots / count) are summed with
// fused multiply-adds, and the factor is sum * count * count / (slots * slots as a 64-bit integer) / (count -
// 1 as a signed integer), in that operation order. Fewer objects answer 0.
- (double)clumpingFactor {
    uint32_t count = self->storage.state.used;
    uint32_t bufferCount = count != 0 ? count : 1;
    id stackObjects[count < 0x101 ? bufferCount : 1];
    size_t storageSize = 0;
    id *heapObjects = NULL;
    id *objects = stackObjects;
    if (count >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(count, false, &storageSize);
        heapObjects = objects;
    }
    [self getObjects:objects count:count];
    double factor = 0.0;
    if (count >= 2) {
        uint32_t slots = setSizeAtIndex(self->storage.state.word >> 26);
        uint64_t *hashes = (uint64_t *)malloc_type_malloc((uint64_t)count * 8 + 8, 0x100004000313F17);
        for (NSUInteger index = 0; index != count; index++) {
            hashes[index] = [objects[index] hash] % slots;
        }
        qsort(hashes, count, 8, _compare_clumpiness);
        hashes[count] = hashes[0] + slots;
        double objectCount = (double)count;
        double average = (double)(uint64_t)slots / objectCount;
        double sum = 0.0;
        for (NSUInteger index = 0; index != count; index++) {
            double gap = (double)(hashes[index + 1] - hashes[index]);
            double shortfall = average > gap ? average - gap : 0.0;
            sum = fma(shortfall, shortfall, sum);
        }
        free(hashes);
        factor = sum * objectCount * objectCount / (double)((uint64_t)slots * slots) /
                 (double)(int64_t)((uint64_t)count - 1);
    }
    free(heapObjects);
    return factor;
}

// Native -[__NSSetM clumpingInterestingThreshold], 0x1c02a52c4. Non-external. (1 - (2 count) / slots) +
// (count * count as a 64-bit integer) / slots / slots, in that operation order; no ThreadSanitizer check.
- (double)clumpingInterestingThreshold {
    uint32_t state = self->storage.state.word;
    uint32_t used = state & 0x3FFFFFF;
    double size = (double)setSizeAtIndex(state >> 26);
    double linear = 1.0 - (double)((uint64_t)used << 1) / size;
    return linear + (double)((uint64_t)used * used) / size / size;
}

// Native -[__NSSetM countByEnumeratingWithState:objects:count:], 0x1c02a531c. Non-external. The
// ThreadSanitizer read check, then the argument checks (os_log pack buffer 0x16 each: the method name and the
// length). state->state is the next slot; the first call points mutationsPtr at the storage's mutation
// counter. Past the table the answer is 0. Otherwise itemsPtr is the buffer, the occupied slots (the slot
// array reread for each) are written until the buffer is full or the table ends, and the next slot is stored
// once at the end (also for a zero length).
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    setTSANRead(self, __builtin_return_address(0));
    FastEnumerationState *enumerationState = state;
    if (buffer == NULL && length != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(__ONSSetM) " countByEnumeratingWithState:objects:count:]",
            (unsigned long)length
        );
    }
    if ((length >> 61) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSSetM) " countByEnumeratingWithState:objects:count:]",
            (unsigned long)length
        );
    }
    unsigned long position = enumerationState->state;
    uint32_t size = setSizeAtIndex(self->storage.state.word >> 26);
    if (position == 0) {
        enumerationState->mutationsPtr = (unsigned long *)&self->storage.mutations;
    }
    if (position >= size) {
        return 0;
    }
    enumerationState->itemsPtr = buffer;
    NSUInteger produced = 0;
    if (length != 0) {
        do {
            id object = self->storage.list[position];
            if (object != nil && object != deletedMarker()) {
                buffer[produced++] = object;
            }
            position += 1;
        } while (position < size && produced < length);
    }
    enumerationState->state = position;
    return produced;
}

// Native -[__NSSetM enumerateObjectsWithOptions:usingBlock:], 0x1c02a5598. Non-external. The ThreadSanitizer
// read check; a nil block raises (os_log pack buffer 0xC: the method name); then
// mset_enumerateObjectsWithOptionsUsingBlock.
- (void)enumerateObjectsWithOptions:(NSUInteger)options usingBlock:(void (^)(id object, BOOL *stop))block {
    setTSANRead(self, __builtin_return_address(0));
    if (block == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: block cannot be nil",
            "-[" OF_NAME_STRING(__ONSSetM) " enumerateObjectsWithOptions:usingBlock:]"
        );
    }
    mset_enumerateObjectsWithOptionsUsingBlock(&storage, options, block);
}

// Native -[__NSSetM getObjects:count:], 0x1c02a57c8. Non-external. The ThreadSanitizer read check, then
// mset_getObjectsCount.
- (void)getObjects:(id *)objects count:(NSUInteger)count {
    setTSANRead(self, __builtin_return_address(0));
    mset_getObjectsCount(&storage, objects, count);
}

// Native -[__NSSetM removeAllObjects], 0x1c02a5a28, with its .cold.1 (0x1c030fdd0). Non-external. The
// ThreadSanitizer write check and the mutation prologue. The slot array and the state word are taken and
// cleared; with a size index every slot (at least one) is emptied, an object being released first unless it
// is a tagged pointer (bit 63). If a release installed a new table (the state word then has a size index),
// the old array is freed (a tail call); otherwise the emptied array is reinstalled with the old size index
// combined into the current state word.
- (void)removeAllObjects {
    setTSANWrite(self, __builtin_return_address(0));
    setMutate(self);
    uint32_t state = self->storage.state.word;
    uint32_t size = setSizeAtIndex(state >> 26);
    id *list = self->storage.list;
    self->storage.list = NULL;
    self->storage.state.word = 0;
    uint32_t current = 0;
    if ((state >> 26) != 0) {
        NSUInteger slots = size > 1 ? size : 1;
        for (NSUInteger slot = 0; slot != slots; slot++) {
            id object = list[slot];
            if (object != nil && object != deletedMarker()) {
                if (((uintptr_t)object >> 63) == 0) {
                    [object release];
                }
                list[slot] = nil;
            } else if (object != nil) {
                list[slot] = nil;
            }
        }
        current = self->storage.state.word;
        if ((current >> 26) != 0) {
            free(list);
            return;
        }
    }
    self->storage.state.word = current | (state & 0xFC000000);
    self->storage.list = list;
}

// Native -[__NSSetM removeObject:], 0x1c02a5ba4, with its .cold.1 (0x1c030fddc). Non-external. The
// ThreadSanitizer write check and the mutation prologue come first; then a nil object raises (os_log pack
// buffer 0xC: the method name). An empty table, or an object the probe (counting deleted markers) does not
// find, changes nothing. The found slot takes the deleted marker and the 26-bit count drops. With more than
// 15 deleted markers passed, __rehashs rebuilds the table at the same size index; otherwise, when the next
// slot (wrapping) is empty, the marker and the markers before it (walking back, wrapping) are cleared. The
// removed object is released last unless it is a tagged pointer (bit 63; a tail call).
- (void)removeObject:(id)object {
    setTSANWrite(self, __builtin_return_address(0));
    setMutate(self);
    if (object == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: object cannot be nil",
            "-[" OF_NAME_STRING(__ONSSetM) " removeObject:]"
        );
    }
    uint32_t state = self->storage.state.word;
    if ((state >> 26) == 0) {
        return;
    }
    uint64_t sizeIndex = state >> 26;
    uint32_t size = setSizeAtIndex(sizeIndex);
    id *list = self->storage.list;
    NSUInteger deletedCount = 0;
    NSUInteger slot = setFindSlot(list, size, object, [object hash], &deletedCount);
    if (slot >= size) {
        return;
    }
    id removed = list[slot];
    if (removed == nil || removed == deletedMarker()) {
        return;
    }
    list[slot] = deletedMarker();
    self->storage.state.used -= 1;
    if (deletedCount > 15) {
        __rehashs(self, (uint32_t)sizeIndex);
    } else {
        NSUInteger next = slot + 1 >= size ? 0 : slot + 1;
        if (list[next] == nil) {
            do {
                list[slot] = nil;
                slot = (slot == 0 ? size : slot) - 1;
            } while (list[slot] == deletedMarker());
        }
    }
    if (((uintptr_t)removed >> 63) == 0) {
        [removed release];
    }
}

// Native __NSSetM_new, 0x1c02a5edc. Hidden (was a private external). The size index is the first capacity not
// below the count. The object is __CFAllocateObject of +[__NSSetM self] with no extra bytes; its cow word is
// cleared with a release store, the storage's slot array is set to NULL and its 32-bit word at 8 to 1, and
// the size index is stored into the top six bits of the word at 12. A non-zero count allocates the slots with
// malloc_type_calloc(1, 8 * size, 0x80040B8603338); with objects, each object is inserted by open addressing
// from -hash modulo the slot count, probing linearly (wrapping) at most max(size, 1) times: the first deleted
// marker seen is remembered, an empty slot ends the search (at the remembered slot if any), an identical or
// -isEqual: object selects its slot. A free selected slot takes the object (retained unless bit 0 of noRetain
// is set or it is a tagged pointer or nil) and increments the 26-bit used count. CoreFoundation defect: the
// native search runs to 64 entries of the 40-entry capacity table before trapping, reading the following
// __NSSetSizes table, whose entries are all below the largest capacity, so the native result for a count
// above it is the same trap, which this restoration reaches without reading past the table (confidence:
// verified for the code, medium for which bytes follow the table, see the unit's organization note). An
// exhausted probe would select slot index size (past the slots); the capacities keep a free slot, so that
// path is unreachable and kept.
id __NSSetM_new(const id *objects, NSUInteger count, BOOL noRetain) {
    NSUInteger sizeIndex = 0;
    while (count > __NSSetCapacities[sizeIndex]) {
        if (++sizeIndex == 40) {
            __builtin_trap();
        }
    }
    NSUInteger size = __NSSetSizes[sizeIndex];
    id set = __CFAllocateObject([(Class)setMClass self], 0);
    __atomic_store_n((uint64_t *)((uint8_t *)set + setMCowOffset), 0, __ATOMIC_RELEASE);
    uint8_t *storage = (uint8_t *)set + setMStorageOffset;
    *(id **)storage = NULL;
    *(uint32_t *)(storage + 8) = 1;
    uint32_t *state = (uint32_t *)((uint8_t *)set + setMStorageOffset + 12);
    *state = (*state & 0x3FFFFFF) | ((uint32_t)sizeIndex << 26);
    if (count == 0) {
        return set;
    }
    id *list = (id *)malloc_type_calloc(1, size * sizeof(id), 0x80040B8603338);
    *(id **)((uint8_t *)set + setMStorageOffset) = list;
    if (objects == NULL) {
        return set;
    }
    NSUInteger tries = size > 1 ? size : 1;
    for (NSUInteger objectIndex = 0; objectIndex != count; objectIndex++) {
        id object = objects[objectIndex];
        NSUInteger index = (NSUInteger)[object hash] % size;
        NSUInteger remaining = tries;
        NSUInteger target = size;
        while (1) {
            id slot = list[index];
            if (slot == nil) {
                if (target == size) {
                    target = index;
                }
                break;
            }
            if (slot == (id)&___NSSetM_DeletedMarker) {
                if (target == size) {
                    target = index;
                }
            } else if (slot == object || [slot isEqual:object]) {
                target = index;
                break;
            }
            index = index + 1 - (size > index + 1 ? 0 : size);
            if (--remaining == 0) {
                break;
            }
        }
        if (list[target] == nil) {
            id inserted = objects[objectIndex];
            list[target] = inserted;
            if (!noRetain && (intptr_t)inserted >= 1) {
                objc_retain(inserted);
            }
            uint32_t *used = (uint32_t *)((uint8_t *)set + setMStorageOffset + 12);
            *used = (*used & 0xFC000000) | ((*used + 1) & 0x3FFFFFF);
        }
    }
    return set;
}

// Native +[__NSSetM allocWithZone:], 0x1c02a60ec. Non-external. The mutable set placeholder (a tail call);
// the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSSetMutablePlaceholder();
}

// Native -[__NSSetM dealloc], 0x1c02a60f0, with its .cold.1 (0x1c030fde8). Non-external. With a control
// block: under its lock a block without a mutable reference crashes (__cow_failure with "Attempting to
// over-release (mutable)"); the mutable count drops, and when neither count remains the storage is released
// (__RELEASE_OBJECTS_IN_THE_SET__, kind 1) and the block freed, after the unlock. Without a block the storage
// is released. Then super's -dealloc.
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
            __RELEASE_OBJECTS_IN_THE_SET__(self, 1);
            free(block);
        }
    } else {
        __RELEASE_OBJECTS_IN_THE_SET__(self, 1);
    }
    [super dealloc];
}

// Native __RELEASE_OBJECTS_IN_THE_SET__, 0x1c02a61d0. Non-external; the cleanup slot of
// ___NSSet_cowCallbacks. Kind 1 is a __NSSetM, any other a __NSFrozenSetM (its storage ivar offset). The
// storage is detached (state word 0, slot array NULL); with a count and a size index the slots are walked in
// order, each object other than the deleted marker released unless it is a tagged pointer (bit 63) and
// counted, until the count is reached or the table ends; the slot array is freed (a tail call).
static void __RELEASE_OBJECTS_IN_THE_SET__(id object, int kind) {
    SetMStorage *storage =
        kind == 1 ? &((__ONSSetM *)object)->storage : &((__ONSFrozenSetM *)object)->storage;
    uint32_t state = storage->state.word;
    uint32_t size = setSizeAtIndex(state >> 26);
    uint32_t remaining = state & 0x3FFFFFF;
    id *list = storage->list;
    storage->state.word = 0;
    storage->list = NULL;
    if (remaining != 0 && (state >> 26) != 0) {
        id *slots = list;
        for (NSUInteger slot = 1;; slot++) {
            id stored = *slots++;
            if (stored != nil && stored != deletedMarker()) {
                if (((uintptr_t)stored >> 63) == 0) {
                    [stored release];
                }
                remaining -= 1;
            }
            if (remaining == 0 || slot >= size) {
                break;
            }
        }
    }
    free(list);
}

// Native -[__NSSetM copy], 0x1c02a6290. Non-external. The ThreadSanitizer read check, then __NSSetM_copy (a
// tail call).
- (id)copy {
    setTSANRead(self, __builtin_return_address(0));
    return __NSSetM_copy(self);
}

// Native __NSSetM_copy, 0x1c02a62dc. Non-external. The ThreadSanitizer read check. A set with a control block
// and at least two objects, or without one and more than three, is shared: the control block is made sure of,
// and a new __NSFrozenSetM (__CFAllocateObject of +[__NSFrozenSetM self], no extra bytes) shares the storage
// through __cow_copy (kind 1, mode 0). A smaller set is copied: its objects (-getObjects:count: into a stack
// buffer of max(count, 1) entries) make +[NSSet allocWithZone:NULL] -initWithObjects:count:.
static id __NSSetM_copy(__ONSSetM *set) {
    setTSANRead(set, __builtin_return_address(0));
    uint32_t count = set->storage.state.used;
    BOOL shared = __atomic_load_n(&set->cow, __ATOMIC_ACQUIRE) != NULL ? count >= 2 : count > 3;
    if (shared) {
        setEnsureCow(set);
        id copy = __CFAllocateObject([__ONSFrozenSetM self], 0);
        _cow_copy((id)set, 1, __atomic_load_n(&set->cow, __ATOMIC_ACQUIRE), &__NSSet_cowCallbacks, copy, 0);
        return copy;
    }
    id objects[count != 0 ? count : 1];
    [set getObjects:objects count:count];
    return [[ONSSet allocWithZone:NULL] initWithObjects:objects count:count];
}

// Native -[__NSSetM copyWithZone:], 0x1c02a6468. Non-external. The body of -copy; the zone is not read.
- (id)copyWithZone:(struct _NSZone *)zone {
    setTSANRead(self, __builtin_return_address(0));
    return __NSSetM_copy(self);
}

// Native cow_copy_instance, 0x1c02a72c4 (listed as _cow_copy_instance_1). Non-external; the first slot of
// ___NSSet_cowCallbacks. Kind and mode 1 select __NSSetM's ivars, any other value __NSFrozenSetM's (their
// offset symbols): the source's control block is loaded with acquire and stored into the copy's with release,
// then the source's 16-byte storage is copied into the copy's (one 128-bit load and store).
static void cow_copy_instance(id source, int kind, id copy, int mode) {
    SetMStorage *sourceStorage =
        kind == 1 ? &((__ONSSetM *)source)->storage : &((__ONSFrozenSetM *)source)->storage;
    CopyOnWriteBlock **sourceCow =
        kind == 1 ? &((__ONSSetM *)source)->cow : &((__ONSFrozenSetM *)source)->cow;
    CopyOnWriteBlock *block = __atomic_load_n(sourceCow, __ATOMIC_ACQUIRE);
    CopyOnWriteBlock **copyCow = mode == 1 ? &((__ONSSetM *)copy)->cow : &((__ONSFrozenSetM *)copy)->cow;
    SetMStorage *copyStorage =
        mode == 1 ? &((__ONSSetM *)copy)->storage : &((__ONSFrozenSetM *)copy)->storage;
    __atomic_store_n(copyCow, block, __ATOMIC_RELEASE);
    *copyStorage = *sourceStorage;
}

// Native cow_copy_storage, 0x1c02a7328 (listed as _cow_copy_storage_1). Non-external; the second slot of
// ___NSSet_cowCallbacks, for a __NSSetM. An empty set (count 0) gets a NULL slot array and a cleared state
// word (its size index too) and answers YES. Otherwise a new slot array of malloc_type_malloc(8 * size,
// 0x80040B8603338) for the low 32 bits of the size index's slot count; a failed allocation installs NULL and
// answers NO. With a size index, every slot (at least one) is copied, an object that is neither nil, a tagged
// pointer (signed value below 1) nor the deleted marker being retained; the new array is installed and YES
// answered. The old array stays with the other holders of the control block.
static bool cow_copy_storage(id object) {
    SetMStorage *storage = &((__ONSSetM *)object)->storage;
    uint32_t state = storage->state.word;
    id *copy = NULL;
    bool result = true;
    if ((state & 0x3FFFFFF) == 0) {
        storage->state.word = 0;
    } else {
        uint32_t size = setSizeAtIndex(state >> 26);
        copy = (id *)malloc_type_malloc((size_t)size << 3, 0x80040B8603338);
        if (copy == NULL) {
            result = false;
        } else if ((state >> 26) != 0) {
            NSUInteger slots = size > 1 ? size : 1;
            id *list = storage->list;
            for (NSUInteger slot = 0; slot != slots; slot++) {
                id stored = list[slot];
                if ((intptr_t)stored >= 1 && stored != deletedMarker()) {
                    objc_retain(stored);
                }
                copy[slot] = stored;
            }
        }
    }
    storage->list = copy;
    return result;
}

// Native cow_set_cow, 0x1c02a7414 (listed as _cow_set_cow_1). Non-external; the third slot of
// ___NSSet_cowCallbacks. The control block is stored into the __NSSetM's cow ivar with release.
static void cow_set_cow(id object, CopyOnWriteBlock *block) {
    __atomic_store_n(&((__ONSSetM *)object)->cow, block, __ATOMIC_RELEASE);
}

// Native _compare_clumpiness, 0x1c02a7428 (listed as __compare_clumpiness_1). Non-external. The qsort order
// of the hash values: -1, 0 or 1 by unsigned comparison.
static int _compare_clumpiness(const void *first, const void *second) {
    uint64_t left = *(const uint64_t *)first;
    uint64_t right = *(const uint64_t *)second;
    if (left < right) {
        return -1;
    }
    return left > right;
}

// Native -[__NSSetM mutableCopy], 0x1c030fca0 (cold region), with OUTLINED_FUNCTION_1-_6 folded in.
// Non-external. The ThreadSanitizer read check runs twice (the second only when the hook was set for the
// first); the control block is made sure of; the copy is __CFAllocateObject of +[__NSSetM self] (no extra
// bytes) sharing the storage through __cow_copy (kind 1, mode 1).
// Folds the compiler-outlined pieces OUTLINED_FUNCTION_1_32 (0x1c02a74bc), OUTLINED_FUNCTION_2_28
// (0x1c02a74e8), OUTLINED_FUNCTION_3_13 (0x1c02a74f8), OUTLINED_FUNCTION_4_11 (0x1c02a7504),
// OUTLINED_FUNCTION_5_11 (0x1c02a7510), OUTLINED_FUNCTION_6_10 (0x1c02a751c).
- (id)mutableCopy {
    if (__cf_tsanReadFunction != NULL) {
        setTSANRead(self, __builtin_return_address(0));
        setTSANRead(self, __builtin_return_address(0));
    }
    setEnsureCow(self);
    id copy = __CFAllocateObject([__ONSSetM self], 0);
    _cow_copy(self, 1, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSSet_cowCallbacks, copy, 1);
    return copy;
}

// Native -[__NSSetM mutableCopyWithZone:], 0x1c030fd2c (cold region). Non-external. The body of -mutableCopy;
// the zone is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    if (__cf_tsanReadFunction != NULL) {
        setTSANRead(self, __builtin_return_address(0));
        setTSANRead(self, __builtin_return_address(0));
    }
    setEnsureCow(self);
    id copy = __CFAllocateObject([__ONSSetM self], 0);
    _cow_copy(self, 1, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSSet_cowCallbacks, copy, 1);
    return copy;
}

@end

// Native position: the __NSFrozenSetM methods lie between -[__NSSetM copyWithZone:] and cow_copy_instance;
// Objective-C does not reopen __NSSetM's @implementation for its cold-region -mutableCopy and
// -mutableCopyWithZone:, so the frozen class's @implementation follows it here. Its methods read the storage
// as __NSSetM's do; only -objectEnumerator and __NSFrozenSetM_mutableCopy call the ThreadSanitizer read hook.
@implementation __ONSFrozenSetM

// Native -[__NSFrozenSetM count], 0x1c02a64b4. Non-external. The 26-bit count.
- (NSUInteger)count {
    return storage.state.used;
}

// Native -[__NSFrozenSetM member:], 0x1c02a64cc. Non-external. An empty table (size index 0) answers nil.
// Otherwise the probe on the slot array loaded before -hash; a slot it found inside the table is reread from
// the current slot array and answered unless it is the deleted marker.
- (id)member:(id)object {
    uint32_t state = storage.state.word;
    if ((state >> 26) == 0) {
        return nil;
    }
    uint32_t size = setSizeAtIndex(state >> 26);
    id *list = storage.list;
    NSUInteger slot = setFindSlot(list, size, object, [object hash], NULL);
    if (slot >= size) {
        return nil;
    }
    id stored = storage.list[slot];
    return stored == deletedMarker() ? nil : stored;
}

// Native -[__NSFrozenSetM containsObject:], 0x1c02a65cc. Non-external. The lookup of -member:, answering
// whether the slot found holds an object (neither nil nor the deleted marker).
- (BOOL)containsObject:(id)object {
    uint32_t state = storage.state.word;
    if ((state >> 26) == 0) {
        return NO;
    }
    uint32_t size = setSizeAtIndex(state >> 26);
    id *list = storage.list;
    NSUInteger slot = setFindSlot(list, size, object, [object hash], NULL);
    if (slot >= size) {
        return NO;
    }
    id stored = storage.list[slot];
    return stored != deletedMarker() && stored != nil;
}

// Native -[__NSFrozenSetM countForObject:], 0x1c02a66d0. Non-external. The body of -containsObject: as a
// count.
- (NSUInteger)countForObject:(id)object {
    uint32_t state = storage.state.word;
    if ((state >> 26) == 0) {
        return 0;
    }
    uint32_t size = setSizeAtIndex(state >> 26);
    id *list = storage.list;
    NSUInteger slot = setFindSlot(list, size, object, [object hash], NULL);
    if (slot >= size) {
        return 0;
    }
    id stored = storage.list[slot];
    return stored != deletedMarker() && stored != nil;
}

// Native -[__NSFrozenSetM objectEnumerator], 0x1c02a67d4. Non-external. The ThreadSanitizer read check, then
// a new __NSFastEnumerationEnumerator (objc_allocWithZone, -initWithObject: the receiver), autoreleased.
- (id)objectEnumerator {
    setTSANRead(self, __builtin_return_address(0));
    id enumerator = [(Class)setMFastEnumerationEnumeratorClass allocWithZone:NULL];
    return [[enumerator initWithObject:self] autorelease];
}

// Native -[__NSFrozenSetM clumpingFactor], 0x1c02a6830. Non-external. The body of -[__NSSetM clumpingFactor]
// on the frozen storage: the 26-bit count and the slot count (low 32 bits) are read once, before
// -getObjects:count:; the temporary buffer is on the stack for fewer than 257 objects (max(count, 1)
// entries), otherwise _CFCreateArrayStorage(count, false, &size), freed at the end. With at least two
// objects, their -hash values modulo the slot count go into malloc_type_malloc(8 * count + 8,
// 0x100004000313F17), are sorted with qsort (_compare_clumpiness) and closed with the first plus the slot
// count; the squares of each gap's shortfall below the average gap are summed with fused multiply-adds, and
// the factor is sum * count * count / (slots * slots as a 64-bit integer) / (count - 1 as a signed integer).
// Fewer objects answer 0.
- (double)clumpingFactor {
    uint32_t state = storage.state.word;
    uint32_t count = state & 0x3FFFFFF;
    uint32_t bufferCount = count != 0 ? count : 1;
    id stackObjects[count < 0x101 ? bufferCount : 1];
    uint32_t slots = setSizeAtIndex(state >> 26);
    size_t storageSize = 0;
    id *heapObjects = NULL;
    id *objects = stackObjects;
    if (count >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(count, false, &storageSize);
        heapObjects = objects;
    }
    [self getObjects:objects count:count];
    double factor = 0.0;
    if (count >= 2) {
        uint64_t *hashes = (uint64_t *)malloc_type_malloc((uint64_t)count * 8 + 8, 0x100004000313F17);
        for (NSUInteger index = 0; index != count; index++) {
            hashes[index] = [objects[index] hash] % slots;
        }
        qsort(hashes, count, 8, _compare_clumpiness);
        hashes[count] = hashes[0] + slots;
        double objectCount = (double)count;
        double average = (double)(uint64_t)slots / objectCount;
        double sum = 0.0;
        for (NSUInteger index = 0; index != count; index++) {
            double gap = (double)(hashes[index + 1] - hashes[index]);
            double shortfall = average > gap ? average - gap : 0.0;
            sum = fma(shortfall, shortfall, sum);
        }
        free(hashes);
        factor = sum * objectCount * objectCount / (double)((uint64_t)slots * slots) /
                 (double)(int64_t)((uint64_t)count - 1);
    }
    free(heapObjects);
    return factor;
}

// Native -[__NSFrozenSetM clumpingInterestingThreshold], 0x1c02a6a38. Non-external. (1 - (2 count) / slots)
// + (count * count as a 64-bit integer) / slots / slots, in that operation order.
- (double)clumpingInterestingThreshold {
    uint32_t state = storage.state.word;
    uint32_t used = state & 0x3FFFFFF;
    double size = (double)setSizeAtIndex(state >> 26);
    double linear = 1.0 - (double)((uint64_t)used << 1) / size;
    return linear + (double)((uint64_t)used * used) / size / size;
}

// Native -[__NSFrozenSetM countByEnumeratingWithState:objects:count:], 0x1c02a6a90. Non-external. The
// argument checks (os_log pack buffer 0x16 each: the method name and the length). state->state is the next
// slot; the first call points mutationsPtr at the unit's constant const_mu (a frozen set never mutates). Past
// the table the answer is 0. Otherwise itemsPtr is the buffer, the occupied slots (the slot array reread for
// each) are written until the buffer is full or the table ends, and the next slot is stored once at the end
// (also for a zero length).
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    // Native _mset_countByEnumeratingWithStateObjectsCount.const_mu, 0x1c046c7a0 (__TEXT,__const,
    // non-external): the value 1.
    static const unsigned long const_mu = 1;
    FastEnumerationState *enumerationState = state;
    if (buffer == NULL && length != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(__ONSFrozenSetM) " countByEnumeratingWithState:objects:count:]",
            (unsigned long)length
        );
    }
    if ((length >> 61) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSFrozenSetM) " countByEnumeratingWithState:objects:count:]",
            (unsigned long)length
        );
    }
    unsigned long position = enumerationState->state;
    uint32_t size = setSizeAtIndex(storage.state.word >> 26);
    if (position == 0) {
        enumerationState->mutationsPtr = (unsigned long *)&const_mu;
    }
    if (position >= size) {
        return 0;
    }
    enumerationState->itemsPtr = buffer;
    NSUInteger produced = 0;
    if (length != 0) {
        do {
            id object = storage.list[position];
            if (object != nil && object != deletedMarker()) {
                buffer[produced++] = object;
            }
            position += 1;
        } while (position < size && produced < length);
    }
    enumerationState->state = position;
    return produced;
}

// Native -[__NSFrozenSetM enumerateObjectsWithOptions:usingBlock:], 0x1c02a6cd8. Non-external. A nil block
// raises (os_log pack buffer 0xC: the method name); then mset_enumerateObjectsWithOptionsUsingBlock on the
// frozen storage (the same block invoke as -[__NSSetM enumerateObjectsWithOptions:usingBlock:]).
- (void)enumerateObjectsWithOptions:(NSUInteger)options usingBlock:(void (^)(id object, BOOL *stop))block {
    if (block == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: block cannot be nil",
            "-[" OF_NAME_STRING(__ONSFrozenSetM) " enumerateObjectsWithOptions:usingBlock:]"
        );
    }
    mset_enumerateObjectsWithOptionsUsingBlock(&storage, options, block);
}

// Native -[__NSFrozenSetM getObjects:count:], 0x1c02a6ed8. Non-external. mset_getObjectsCount on the frozen
// storage.
- (void)getObjects:(id *)objects count:(NSUInteger)count {
    mset_getObjectsCount(&storage, objects, count);
}

// Native +[__NSFrozenSetM allocWithZone:], 0x1c02a7108. Non-external. The immutable set placeholder (a tail
// call); the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSSetImmutablePlaceholder();
}

// Native -[__NSFrozenSetM dealloc], 0x1c02a710c, with its .cold.1 (0x1c030fdfc). Non-external. With a control
// block: under its lock a block without an immutable reference crashes (__cow_failure with "Attempting to
// over-release (immutable)"); the immutable count drops, and when neither count remains the storage is
// released (__RELEASE_OBJECTS_IN_THE_SET__, kind 0) and the block freed, after the unlock. Without a block
// the storage is released. Then super's -dealloc.
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
            __RELEASE_OBJECTS_IN_THE_SET__(self, 0);
            free(block);
        }
    } else {
        __RELEASE_OBJECTS_IN_THE_SET__(self, 0);
    }
    [super dealloc];
}

// Native -[__NSFrozenSetM copy], 0x1c02a71ec. Non-external. The receiver retained (objc_retain, a tail call).
- (id)copy {
    return [self retain];
}

// Native -[__NSFrozenSetM copyWithZone:], 0x1c02a71f0. Non-external. The receiver retained (a tail call).
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native -[__NSFrozenSetM mutableCopy], 0x1c02a71f4. Non-external. __NSFrozenSetM_mutableCopy (a tail call).
- (id)mutableCopy {
    return __NSFrozenSetM_mutableCopy(self);
}

// Native __NSFrozenSetM_mutableCopy, 0x1c02a71f8. Non-external. The ThreadSanitizer read check (with the
// return address the tail-calling method leaves, its caller's). A receiver without a control block gets one
// (__cow_create mode 0, installed with a compare-and-swap from NULL, acquire-release; the loser's block is
// freed). The copy is __CFAllocateObject of +[__NSSetM self] (no extra bytes) sharing the storage through
// __cow_copy (kind 0, mode 1).
static id __NSFrozenSetM_mutableCopy(__ONSFrozenSetM *set) {
    setTSANRead(set, __builtin_return_address(0));
    if (__atomic_load_n(&set->cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create((id)set, 0);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &set->cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject([__ONSSetM self], 0);
    _cow_copy((id)set, 0, __atomic_load_n(&set->cow, __ATOMIC_ACQUIRE), &__NSSet_cowCallbacks, copy, 1);
    return copy;
}

// Native -[__NSFrozenSetM mutableCopyWithZone:], 0x1c02a72c0. Non-external. __NSFrozenSetM_mutableCopy (a
// tail call); the zone is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return __NSFrozenSetM_mutableCopy(self);
}

@end
