//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSDictionaryM.h"
#import "CoreFoundation/Objective-C/OCFNSFrozenDictionaryM.h"
#include "CoreFoundation/Objective-C/OCFNSDictionaryConcurrentEnumeration.h"
#include "CoreFoundation/Objective-C/OCFNSDictionaryEnumerate.h"

#include <malloc/malloc.h>
#include <math.h>
#include <os/lock.h>
#include <stdlib.h>

// Inferred organization: the __NSDictionaryM unit spans 0x1c02f46b8-0x1c02f8f13 of the iOS Simulator __text
// section, after the __NSPlaceholderDictionary methods and the __NSCFDictionaryCreate* helpers: the external
// mdict_keysOfEntriesWithOptionsPassingTest and its block, the __NSDictionaryM methods (with the non-external
// mdict_removeObjectForKey, __NSDictionaryM_new and cow_cleanup among them), the __NSFrozenDictionaryM
// methods and its cow_copy_instance, cow_copy_storage and cow_set_cow (0x1c02f7b58-0x1c02f8be7; the frozen
// class's methods have their own translation unit, Objective-C/OCFNSFrozenDictionaryM.m, and the three
// callbacks are defined here, read from iphone-simulator-corpus-delta10.json), then mdict_rehashd,
// the method blocks, _compare_clumpiness (listed as __compare_clumpiness_2) and OUTLINED_FUNCTION_0_61-_6_15
// (0x1c02f8e94, 0x1c02f8ea0, 0x1c02f8ecc, 0x1c02f8edc, 0x1c02f8ee8, 0x1c02f8ef4, 0x1c02f8f00; folded into
// their callers). The unit's cold-region pieces follow __NSCFDictionaryCreateMutable.cold.1:
// -[__NSDictionaryM copy] and -copyWithZone: (0x1c03171cc, 0x1c0317274),
// __NSMUTABLEDICTIONARY_CONCURRENT_MODIFICATION_DETECTED__ (0x1c031731c) and the methods' .cold.1 fragments
// (0x1c0317350-0x1c03173b0, folded into their methods). The native source path is not in the evidence.
// Objective-C compiled with manual reference counting (objc_retain, objc_release, objc_autorelease). The
// unit's statics: __NSDictionarySizes (0x1c046fb80) and __NSDictionaryCapacities (0x1c046fa40), 40 entries
// each (listed with an _0 suffix), ___NSDictionaryM_DeletedMarker (0x2227aba60) and
// ___NSDictionary_cowCallbacks (0x2227aba98); _mdict_countByEnumeratingWithStateObjectsCount.const_mu
// (0x1c046fcc0, the value 1) is not read by any restored function (presumably the frozen class's enumeration)
// and is not defined. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full,
// iphone-simulator-corpus-delta8.json). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// Class symbols sent to or allocated directly (class references off_2227CE390, off_2227CE3B8, off_2227CE440
// and ___SCR___NSFrozenDictionaryM).
extern char mutableSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableSet));
extern char setClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSSet));
extern char fastEnumerationEnumeratorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                       OF_NAME_STRING(__ONSFastEnumerationEnumerator));
extern char frozenDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSFrozenDictionaryM));

// The storage and cow ivar offsets of __NSFrozenDictionaryM (_OBJC_IVAR_$___NSFrozenDictionaryM.storage,
// 0x22e6cc4d8, and .cow, 0x22e6cc4dc), read by cow_cleanup and cow_copy_instance for that class; the frozen
// class is defined by Objective-C/OCFNSFrozenDictionaryM.m.
extern const int32_t
    frozenDictionaryStorageOffset __asm(OF_SYMBOL("OBJC_IVAR_$_")
                                            OF_NAME_STRING(__ONSFrozenDictionaryM) ".storage");
extern const int32_t frozenDictionaryCowOffset __asm(OF_SYMBOL("OBJC_IVAR_$_")
                                                         OF_NAME_STRING(__ONSFrozenDictionaryM) ".cow");

// NSObject, the isa of the deleted-slot marker (0x22e6b6088, libobjc's class, outside the extracted image).
extern char rootObjectClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_NSObject"));

// Selectors this unit sends that the shared declarations do not carry; declared for the message sends only.
@protocol DictionaryMMessages

+ (id)set;
- (void)_willChangeValuesForKeys:(id)keys;
- (void)_didChangeValuesForKeys:(id)keys;

@end

// The superclass method -setObservationInfo: sends to super (Foundation's NSKeyValueObserving, outside this
// restoration); declared only.
@interface ONSMutableDictionary (DictionaryMSuperclassMessages)

- (void)setObservationInfo:(void *)observationInfo;

@end

// Native __NSDictionarySizes, 0x1c046fb80 (__TEXT,__const, listed as ___NSDictionarySizes_0). Non-external.
// The bucket counts per size index; the methods read the low 32 bits (LDR W) except __NSDictionaryM_new.
const uint64_t __NSDictionarySizes[40] = {
    0,       3,        7,        13,       23,       41,       71,        127,       191,       251,
    383,     631,      1087,     1723,     2803,     4523,     7351,      11959,     19447,     31231,
    50683,   81919,    132607,   214519,   346607,   561109,   907759,    1468927,   2376191,   3845119,
    6221311, 10066421, 16287743, 26354171, 42641881, 68996069, 111638519, 180634607, 292272623, 472907251,
};

// Native __NSDictionaryCapacities, 0x1c046fa40 (__TEXT,__const, listed as ___NSDictionaryCapacities_0).
// Non-external. The largest entry count per size index before the table grows.
static const uint64_t __NSDictionaryCapacities[40] = {
    0,       3,       6,        11,       19,       32,       52,       85,        118,       155,
    237,     390,     672,      1065,     1732,     2795,     4543,     7391,      12019,     19302,
    31324,   50629,   81956,    132580,   214215,   346784,   561026,   907847,    1468567,   2376414,
    3844982, 6221390, 10066379, 16287773, 26354132, 42641916, 68996399, 111638327, 180634415, 292272755,
};

// Native ___NSDictionaryM_DeletedMarker, 0x2227aba60 (__DATA_CONST,__const, 16 bytes). Non-external. A static
// object (isa NSObject, a zero word) stored in the key slot of a removed entry.
const OCFRuntimeBase ___NSDictionaryM_DeletedMarker = {(uintptr_t)rootObjectClass, 0};

// The deleted marker as an object pointer. deletedMarker is a local descriptive name.
static inline id deletedMarker(void) {
    return (id)&___NSDictionaryM_DeletedMarker;
}

// The low 32 bits of a size, as the methods load them. sizeAtIndex is a local descriptive name.
static inline NSUInteger sizeAtIndex(NSUInteger index) {
    return (uint32_t)__NSDictionarySizes[index];
}

static void cow_copy_instance(id source, int kind, id copy, int mode);

static bool cow_copy_storage(id object);

static void cow_set_cow(id object, CopyOnWriteBlock *cow);

// Native ___NSDictionary_cowCallbacks, 0x2227aba98 (__DATA_CONST,__const). Non-external: cow_copy_instance,
// cow_copy_storage, cow_set_cow and cow_cleanup, this unit's own copies of the four callbacks (the array, set
// and ordered set units have theirs, of the same names).
const CopyOnWriteCallbacks __NSDictionary_cowCallbacks = {
    cow_copy_instance,
    cow_copy_storage,
    cow_set_cow,
    cow_cleanup,
};

static void mdict_removeObjectForKey(__ONSDictionaryM *self, id key);

static void mdict_rehashd(__ONSDictionaryM *self, uint32_t index);

static __attribute__((noinline, noreturn)) void
__NSMUTABLEDICTIONARY_CONCURRENT_MODIFICATION_DETECTED__(id object);

static int _compare_clumpiness(const void *first, const void *second);

// The probe every lookup inlines: from the key's -hash modulo the size, at most probes slots in order
// (wrapping to 0). An empty slot ends the search; a deleted marker is counted (when deletedCount is not NULL)
// and skipped; a slot holding the key itself or a key that answers -isEqual: (sent to the stored key) with
// the key ends it. The answer is that slot, else the first deleted or empty slot passed, else the size.
// findSlot is a local descriptive name.
static inline NSUInteger
findSlot(id *keys, NSUInteger size, NSUInteger probes, id key, NSUInteger *deletedCount) {
    NSUInteger index = [key hash] % size;
    NSUInteger slot = size;
    for (; probes != 0; probes--) {
        id candidate = keys[index];
        if (candidate == nil) {
            if (slot == size) {
                slot = index;
            }
            break;
        }
        if (candidate == deletedMarker()) {
            if (deletedCount != NULL) {
                *deletedCount += 1;
            }
            if (slot == size) {
                slot = index;
            }
        } else if (candidate == key || [candidate isEqual:key]) {
            slot = index;
            break;
        }
        NSUInteger next = index + 1;
        index = next - (next < size ? 0 : size);
    }
    return slot;
}

// The copy-on-write and mutation-counter prologue every mutating method inlines (and -_mutate is): the
// control block is loaded with acquire; the next counter is the current one plus one, or 0 once bits 9-30 of
// it reach 125. Under the block's lock a block without a mutable reference crashes (the methods' .cold.1:
// OUTLINED_FUNCTION_0_61, __cow_failure with "Attempting to mutate immutable collection!"); unless this
// dictionary is the block's only reference, its mutable count drops and __cow_mutate_slow gives the
// dictionary its own storage. The counter is then stored into the (possibly new) storage word, its bit 31 and
// above kept. mutateStorage is a local descriptive name.
static inline void mutateStorage(__ONSDictionaryM *self) {
    CopyOnWriteBlock *cow = __atomic_load_n(&self->cow, __ATOMIC_ACQUIRE);
    uint32_t mutations = self->storage.bits.mutations;
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
            _cow_mutate_slow((id)self, cow, &__NSDictionary_cowCallbacks);
        }
        os_unfair_lock_unlock(&cow->lock);
    }
    self->storage.bits.mutations = nextMutations;
}

// How the insertion the setters inline stores a new key. KeyStorage is a local descriptive name.
typedef enum {
    KeyStorageCopy,
    KeyStorageCopyWhenFlagged,
    KeyStorageRetain,
} KeyStorage;

// The insertion -setObject:forKey:, -setObject:forKeyedSubscript: and -__setObject:forKey: inline, after
// their argument checks and the mutation prologue: an empty dictionary gets size index 1 and a 48-byte buffer
// (malloc_type_calloc(1, 0x30, 0x80040B8603338); a failed allocation is BRK #1); the object is retained
// unless it is a tagged pointer (bit 63); the storage word is read once for the KVO bit. The probe is
// repeated after mdict_rehashd at the next size index when no slot was found, or at the same index when more
// than 15 deleted markers were passed. An existing entry keeps its key and takes the object; otherwise the
// key is copied
// (-copyWithZone: NULL) or retained (unless tagged) as keyStorage says, the count grows, and a count above
// the index's capacity rehashes to the next index. The KVO notifications bracket the store; the replaced
// object is released last (unless nil or tagged). setEntry is a local descriptive name.
static inline void setEntry(__ONSDictionaryM *self, id key, id object, KeyStorage keyStorage) {
    DictionaryMStorage *storage = &self->storage;
    uint32_t index = storage->bits.szidx;
    NSUInteger size;
    BOOL copyKeys = storage->bits.copyKeys;
    if (index == 0) {
        storage->bits.szidx = 1;
        id *buffer = malloc_type_calloc(1, 0x30, 0x80040B8603338);
        if (buffer == NULL) {
            __builtin_trap();
        }
        storage->buffer = buffer;
        index = 1;
        size = 3;
    } else {
        size = sizeAtIndex(index);
    }
    if ((intptr_t)object >= 0) {
        [object retain];
    }
    BOOL kvo = storage->bits.kvo;
    id *keys;
    NSUInteger slot;
    for (;;) {
        keys = storage->buffer;
        NSUInteger deletedCount = 0;
        slot = findSlot(keys, size, size, key, &deletedCount);
        if (slot < size && deletedCount <= 15) {
            break;
        }
        mdict_rehashd(self, index + (slot >= size ? 1 : 0));
        index = storage->bits.szidx;
        size = sizeAtIndex(index);
    }
    id *objects = keys + size;
    id oldKey = keys[slot];
    if (kvo) {
        [(id)self willChangeValueForKey:key];
    }
    id oldObject = objects[slot];
    if (oldKey != deletedMarker() && oldKey != nil) {
        objects[slot] = object;
    } else {
        id storedKey;
        if (keyStorage == KeyStorageCopy || (keyStorage == KeyStorageCopyWhenFlagged && copyKeys)) {
            storedKey = [key copyWithZone:NULL];
        } else {
            if ((intptr_t)key >= 0) {
                [key retain];
            }
            storedKey = key;
        }
        keys[slot] = storedKey;
        objects[slot] = object;
        storage->bits.used += 1;
        if (__NSDictionaryCapacities[index] < storage->bits.used) {
            mdict_rehashd(self, index + 1);
        }
    }
    if (kvo) {
        [(id)self didChangeValueForKey:key];
    }
    if ((intptr_t)oldObject >= 1) {
        [oldObject release];
    }
}

// The temporary object buffer the methods inline for count entries: on the stack for up to 0x100 entries
// (room for at least one), otherwise _CFCreateArrayStorage(count, false, &outCount), whose result the caller
// frees. temporaryBufferCount is a local descriptive name: the stack entry count.
static inline uint32_t temporaryBufferCount(NSUInteger count) {
    NSUInteger allocationCount = (count & 0x1FFFFFE) != 0 ? count : 1;
    return count >= 0x101 ? 1 : (uint32_t)allocationCount;
}

// Native mdict_keysOfEntriesWithOptionsPassingTest, 0x1c02f46b8, with its block
// ___mdict_keysOfEntriesWithOptionsPassingTest_block_invoke (0x1c02f484c). External natively; hidden until
// the owner records a naming decision. The set is +[NSMutableSet set]. With the concurrent option honored
// (__NSCollectionHandleConcurrentEnumerationIfSpecified, no per-call pool) the block adds passing keys under
// a stack os_unfair_lock and the mutable set itself is answered. Otherwise each live entry is tested in an
// autorelease pool until the predicate sets stop, and the answer is the set's -copy, autoreleased.
id mdict_keysOfEntriesWithOptionsPassingTest(
    DictionaryMStorage *storage, NSUInteger options, BOOL (^predicate)(id key, id object, BOOL *stop)
) {
    id *keys = storage->buffer;
    uint64_t word = storage->bits.word;
    NSUInteger size = sizeAtIndex(word >> 58);
    id *objects = keys + size;
    id set = [(Class)mutableSetClass set];
    os_unfair_lock lock = OS_UNFAIR_LOCK_INIT;
    os_unfair_lock_t lockRef = &lock;
    BOOL handled =
        __NSCollectionHandleConcurrentEnumerationIfSpecified(options, NO, size, ^(size_t index, BOOL *stop) {
          id key = keys[index];
          if (key == nil || key == deletedMarker()) {
              return;
          }
          id object = objects[index];
          void *pool = _OCFAutoreleasePoolPush();
          if (predicate(key, object, stop)) {
              os_unfair_lock_lock(lockRef);
              [set addObject:key];
              os_unfair_lock_unlock(lockRef);
          }
          _OCFAutoreleasePoolPop(pool);
        });
    if (handled) {
        return set;
    }
    BOOL stop = NO;
    if ((word >> 58) != 0) {
        for (NSUInteger index = 0; index < size; index++) {
            id key = keys[index];
            if (key == nil || key == deletedMarker()) {
                continue;
            }
            id object = objects[index];
            void *pool = _OCFAutoreleasePoolPush();
            if (predicate(key, object, &stop)) {
                [set addObject:key];
            }
            _OCFAutoreleasePoolPop(pool);
            if (stop) {
                break;
            }
        }
    }
    return [[set copy] autorelease];
}

@implementation __ONSDictionaryM

// Native -[__NSDictionaryM count], 0x1c02f48f8. Non-external.
- (NSUInteger)count {
    dictionaryTSANRead(self, __builtin_return_address(0));
    return storage.bits.used;
}

// Native -[__NSDictionaryM getObjects:andKeys:count:], 0x1c02f4954. Non-external. A count with any of its top
// three bits set raises when the objects array is given, then when the keys array is; both raises carry
// "count (%lu) of objects array is ridiculous". At most min(count, entries) live entries are copied in slot
// order, the key before the object.
- (void)getObjects:(id *)objects andKeys:(id *)keys count:(NSUInteger)count {
    dictionaryTSANRead(self, __builtin_return_address(0));
    if (objects != NULL && (OF_WORD_SHIFT(count, 61)) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSDictionaryM) " getObjects:andKeys:count:]",
            count
        );
    }
    if (keys != NULL && (OF_WORD_SHIFT(count, 61)) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSDictionaryM) " getObjects:andKeys:count:]",
            count
        );
    }
    NSUInteger index = storage.bits.szidx;
    NSUInteger used = storage.bits.used;
    NSUInteger remaining = used < count ? used : count;
    if (index == 0 || remaining == 0) {
        return;
    }
    NSUInteger size = sizeAtIndex(index);
    id *slots = storage.buffer;
    for (NSUInteger slot = 1;; slot++) {
        id *entry = slots;
        slots += 1;
        id key = *entry;
        if (key != nil && key != deletedMarker()) {
            if (keys != NULL) {
                *keys++ = key;
            }
            if (objects != NULL) {
                *objects++ = entry[size];
            }
            remaining -= 1;
        }
        if (slot >= size || remaining == 0) {
            break;
        }
    }
}

// Native -[__NSDictionaryM keyEnumerator], 0x1c02f4b84. Non-external. A fast-enumeration enumerator over the
// receiver (allocWithZone: NULL), autoreleased.
- (id)keyEnumerator {
    dictionaryTSANRead(self, __builtin_return_address(0));
    id enumerator = [(Class)fastEnumerationEnumeratorClass allocWithZone:NULL];
    return [[enumerator initWithObject:self] autorelease];
}

// Native -[__NSDictionaryM objectForKey:], 0x1c02f4be0. Non-external. An empty table answers nil without
// hashing; otherwise the object of the probed slot (nil for an empty or deleted slot) or nil when the probe
// found no slot. A nil key is not checked (it hashes as 0).
- (id)objectForKey:(id)key {
    dictionaryTSANRead(self, __builtin_return_address(0));
    NSUInteger index = storage.bits.szidx;
    if (index == 0) {
        return nil;
    }
    NSUInteger size = sizeAtIndex(index);
    id *keys = storage.buffer;
    NSUInteger slot = findSlot(keys, size, size, key, NULL);
    if (slot >= size) {
        return nil;
    }
    return keys[size + slot];
}

// Native -[__NSDictionaryM removeObjectForKey:], 0x1c02f4d08, with its .cold.1 (0x1c0317350). Non-external.
// The write check and the mutation prologue come before the nil-key raise; then mdict_removeObjectForKey (a
// tail call).
- (void)removeObjectForKey:(id)key {
    dictionaryTSANWrite(self, __builtin_return_address(0));
    mutateStorage(self);
    if (key == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: key cannot be nil",
            "-[" OF_NAME_STRING(__ONSDictionaryM) " removeObjectForKey:]"
        );
    }
    mdict_removeObjectForKey(self, key);
}

// Native mdict_removeObjectForKey, 0x1c02f4ee8. Non-external. Nothing for an empty table or a key the probe
// does not find (counting the deleted markers passed). Otherwise, inside the KVO notifications when bit 57 of
// the storage word is set: the slot gets the deleted marker and a nil object, and the count drops. More than
// 15 deleted markers passed rehash at the same size index; otherwise, when the next slot (wrapping) is empty,
// the slot and the run of deleted markers before it (wrapping backwards) are cleared to empty. The key
// (unless tagged) and the object (unless nil or tagged) are released last.
static void mdict_removeObjectForKey(__ONSDictionaryM *self, id key) {
    DictionaryMStorage *storage = &self->storage;
    uint32_t index = storage->bits.szidx;
    if (index == 0) {
        return;
    }
    NSUInteger size = sizeAtIndex(index);
    id *keys = storage->buffer;
    id *objects = keys + size;
    NSUInteger deletedCount = 0;
    NSUInteger slot = findSlot(keys, size, size, key, &deletedCount);
    if (slot >= size) {
        return;
    }
    id oldKey = keys[slot];
    if (oldKey == nil || oldKey == deletedMarker()) {
        return;
    }
    BOOL kvo = storage->bits.kvo;
    if (kvo) {
        [(id)self willChangeValueForKey:key];
    }
    id oldObject = objects[slot];
    keys[slot] = deletedMarker();
    objects[slot] = nil;
    storage->bits.used -= 1;
    if (deletedCount > 15) {
        mdict_rehashd(self, index);
    } else {
        NSUInteger next = slot + 1 >= size ? 0 : slot + 1;
        if (keys[next] == nil) {
            do {
                keys[slot] = nil;
                slot = (slot == 0 ? size : slot) - 1;
            } while (keys[slot] == deletedMarker());
        }
    }
    if (kvo) {
        [(id)self didChangeValueForKey:key];
    }
    if ((intptr_t)oldKey >= 0) {
        [oldKey release];
    }
    if ((intptr_t)oldObject >= 1) {
        [oldObject release];
    }
}

// Native -[__NSDictionaryM setObject:forKey:], 0x1c02f50e8, with its .cold.1 (0x1c031735c). Non-external.
// Order: the write check, a nil key, a nil object, the mutation prologue, then the insertion with the key
// always copied.
- (void)setObject:(id)object forKey:(id)key {
    dictionaryTSANWrite(self, __builtin_return_address(0));
    if (key == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: key cannot be nil",
            "-[" OF_NAME_STRING(__ONSDictionaryM) " setObject:forKey:]"
        );
    }
    if (object == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: object cannot be nil (key: %@)",
            "-[" OF_NAME_STRING(__ONSDictionaryM) " setObject:forKey:]",
            key
        );
    }
    mutateStorage(self);
    setEntry(self, key, object, KeyStorageCopy);
}

// Native -[__NSDictionaryM setObject:forKeyedSubscript:], 0x1c02f55b8, with its .cold.1 (0x1c0317368).
// Non-external. A nil key raises before the mutation prologue; a nil object then removes the key
// (mdict_removeObjectForKey, a tail call); otherwise the insertion copies the key only when bit 31 of the
// storage word is set and retains it otherwise.
- (void)setObject:(id)object forKeyedSubscript:(id)key {
    dictionaryTSANWrite(self, __builtin_return_address(0));
    if (key == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: key cannot be nil",
            "-[" OF_NAME_STRING(__ONSDictionaryM) " setObject:forKeyedSubscript:]"
        );
    }
    mutateStorage(self);
    if (object == nil) {
        mdict_removeObjectForKey(self, key);
        return;
    }
    setEntry(self, key, object, KeyStorageCopyWhenFlagged);
}

// Native -[__NSDictionaryM __setObject:forKey:], 0x1c02f5a24, with its .cold.1 (0x1c0317374). Non-external.
// The toll-free CFDictionarySetValue path: -_mutate is sent first, then the write check (with the method's
// return address), a nil key, a nil object and the inline mutation prologue. When keys are copied (bit 31)
// and the key does not respond to -copyWithZone:, CFLog (level 3) reports it, the key is retained instead,
// and after the insertion a set __setObject_throw_on_warning environment variable raises
// NSInvalidArgumentException "key should be copyable" (+exceptionWithName:reason:userInfo:). Otherwise the
// insertion is that of -setObject:forKeyedSubscript:.
- (void)__setObject:(id)object forKey:(id)key {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (key == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: key cannot be nil",
            "-[" OF_NAME_STRING(__ONSDictionaryM) " __setObject:forKey:]"
        );
    }
    if (object == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: object cannot be nil (key: %@)",
            "-[" OF_NAME_STRING(__ONSDictionaryM) " __setObject:forKey:]",
            key
        );
    }
    mutateStorage(self);
    if (storage.bits.copyKeys && ![key respondsToSelector:@selector(copyWithZone:)]) {
        OCFLog(
            3,
            OF_STR(
                "*** CFDictionarySetValue(): attempt to use this function to set a key which is not copyable "
                "into a non-CFDictionary via toll-free bridging"
            )
        );
        setEntry(self, key, object, KeyStorageRetain);
        if (getenv("__setObject_throw_on_warning") != NULL) {
            objc_exception_throw([(id)dictionaryExceptionClass
                exceptionWithName:(id)ONSInvalidArgumentException
                           reason:(id)OF_STR("key should be copyable")
                         userInfo:nil]);
        }
        return;
    }
    setEntry(self, key, object, KeyStorageCopyWhenFlagged);
}

// Native -[__NSDictionaryM _mutate], 0x1c02f6164, with its .cold.1 (0x1c0317380). Non-external. The mutation
// prologue alone.
- (void)_mutate {
    mutateStorage(self);
}

// Native +[__NSDictionaryM automaticallyNotifiesObserversForKey:], 0x1c02f6214. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSDictionaryM setObservationInfo:], 0x1c02f621c, with its .cold.1 (0x1c031738c). Non-external.
// The mutation prologue, with bit 57 of the storage word set exactly when the observation info is not NULL
// (stored together with the counter natively), then super stores it.
- (void)setObservationInfo:(void *)observationInfo {
    dictionaryTSANWrite(self, __builtin_return_address(0));
    mutateStorage(self);
    storage.bits.kvo = observationInfo != NULL;
    [super setObservationInfo:observationInfo];
}

// Native -[__NSDictionaryM __apply:context:], 0x1c02f636c. Non-external. A NULL function raises
// NSInvalidArgumentException "%@: function pointer is NULL" (the __CFExceptionProem of the receiver and
// selector, +exceptionWithName:reason:userInfo:). The function gets each live key, its object and the
// context, in slot order.
- (void)__apply:(void (*)(id key, id object, void *context))applier context:(void *)context {
    if (applier == NULL) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFStringRef reason = OCFStringCreateWithFormat(
            allocator, NULL, OF_STR("%@: function pointer is NULL"), __CFExceptionProem(self, _cmd)
        );
        objc_exception_throw([(id)dictionaryExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                     userInfo:nil]);
    }
    NSUInteger index = storage.bits.szidx;
    if (index == 0) {
        return;
    }
    NSUInteger size = sizeAtIndex(index);
    id *keys = storage.buffer;
    NSUInteger count = size > 1 ? size : 1;
    for (; count != 0; count--) {
        id *entry = keys;
        keys += 1;
        id key = *entry;
        if (key != nil && key != deletedMarker()) {
            applier(key, entry[size], context);
        }
    }
}

// Native -[__NSDictionaryM countByEnumeratingWithState:objects:count:], 0x1c02f6468. Non-external. A NULL
// buffer with a nonzero length raises "pointer to objects array is NULL but length is %lu", a length with any
// of its top three bits set "count (%lu) of objects array is ridiculous". The first call points the mutations
// pointer at the storage word. The state is the next slot; live keys are copied into the caller's buffer,
// which becomes the items pointer.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    dictionaryTSANRead(self, __builtin_return_address(0));
    if (buffer == NULL && length != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(__ONSDictionaryM) " countByEnumeratingWithState:objects:count:]",
            length
        );
    }
    if ((OF_WORD_SHIFT(length, 61)) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSDictionaryM) " countByEnumeratingWithState:objects:count:]",
            length
        );
    }
    DictionaryEnumerationState *enumerationState = state;
    unsigned long index = enumerationState->state;
    NSUInteger size = sizeAtIndex(storage.bits.szidx);
    if (index == 0) {
        enumerationState->mutationsPtr = (unsigned long *)&storage.bits.word;
    }
    if (index >= size) {
        return 0;
    }
    id *keys = storage.buffer;
    enumerationState->itemsPtr = (void **)buffer;
    NSUInteger count = 0;
    if (length != 0) {
        do {
            id key = keys[index];
            if (key != nil && key != deletedMarker()) {
                buffer[count] = key;
                count += 1;
            }
            index += 1;
        } while (index < size && count < length);
    }
    enumerationState->state = index;
    return count;
}

// Native -[__NSDictionaryM enumerateKeysAndObjectsWithOptions:usingBlock:], 0x1c02f66e4, with its block
// ___mdict_enumerateKeysAndObjectsWithOptionsUsingBlock_block_invoke (0x1c02f8d4c). Non-external. A nil block
// raises "block cannot be nil". With the concurrent option honored, each live entry is called out in its own
// autorelease pool from the dispatch_apply block; otherwise in slot order, each in its own pool, until the
// block sets stop.
- (void)enumerateKeysAndObjectsWithOptions:(NSUInteger)options
                                usingBlock:(void (^)(id key, id object, BOOL *stop))block {
    dictionaryTSANRead(self, __builtin_return_address(0));
    if (block == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            "-[" OF_NAME_STRING(__ONSDictionaryM) " enumerateKeysAndObjectsWithOptions:usingBlock:]"
        );
    }
    id *keys = storage.buffer;
    uint64_t word = storage.bits.word;
    NSUInteger size = sizeAtIndex(word >> 58);
    id *objects = keys + size;
    BOOL handled =
        __NSCollectionHandleConcurrentEnumerationIfSpecified(options, NO, size, ^(size_t index, BOOL *stop) {
          id key = keys[index];
          if (key == nil || key == deletedMarker()) {
              return;
          }
          void *pool = _OCFAutoreleasePoolPush();
          __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(block, key, objects[index], stop);
          _OCFAutoreleasePoolPop(pool);
        });
    if (handled) {
        return;
    }
    BOOL stop = NO;
    if ((word >> 58) == 0) {
        return;
    }
    for (NSUInteger index = 0; index < size; index++) {
        id key = keys[index];
        if (key == nil || key == deletedMarker()) {
            continue;
        }
        void *pool = _OCFAutoreleasePoolPush();
        __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(block, key, objects[index], &stop);
        _OCFAutoreleasePoolPop(pool);
        if (stop) {
            break;
        }
    }
}

// Native -[__NSDictionaryM keyOfEntryWithOptions:passingTest:], 0x1c02f6930, with its block
// ___mdict_keyOfEntryWithOptionsPassingTest_block_invoke (0x1c02f8dd4). Non-external. The predicate is not
// checked for nil. With the concurrent option honored, a passing key is stored with release semantics into a
// __block variable and stop is set; the answer is loaded with acquire. Otherwise the live entries are tested
// in slot order, each in its own autorelease pool, and the first passing key sets stop and is answered.
- (id)keyOfEntryWithOptions:(NSUInteger)options
                passingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate {
    dictionaryTSANRead(self, __builtin_return_address(0));
    id *keys = storage.buffer;
    uint64_t word = storage.bits.word;
    NSUInteger size = sizeAtIndex(word >> 58);
    id *objects = keys + size;
    __block id concurrentResult = nil;
    BOOL handled =
        __NSCollectionHandleConcurrentEnumerationIfSpecified(options, NO, size, ^(size_t index, BOOL *stop) {
          id key = keys[index];
          if (key == nil || key == deletedMarker()) {
              return;
          }
          id object = objects[index];
          void *pool = _OCFAutoreleasePoolPush();
          if (predicate(key, object, stop)) {
              __atomic_store_n((void **)&concurrentResult, (void *)key, __ATOMIC_RELEASE);
              *stop = YES;
          }
          _OCFAutoreleasePoolPop(pool);
        });
    id result;
    if (handled) {
        result = (id)__atomic_load_n((void **)&concurrentResult, __ATOMIC_ACQUIRE);
    } else {
        BOOL stop = NO;
        result = nil;
        if ((word >> 58) != 0) {
            NSUInteger count = size > 1 ? size : 1;
            for (id *entry = keys; count != 0; count--, entry++) {
                id key = *entry;
                if (key == nil || key == deletedMarker()) {
                    continue;
                }
                id object = entry[size];
                void *pool = _OCFAutoreleasePoolPush();
                if (predicate(key, object, &stop)) {
                    stop = YES;
                    result = key;
                }
                _OCFAutoreleasePoolPop(pool);
                if (stop) {
                    break;
                }
            }
        }
    }
    return result;
}

// Native -[__NSDictionaryM keysOfEntriesWithOptions:passingTest:], 0x1c02f6b20. Non-external. A nil predicate
// raises "predicate cannot be nil"; otherwise mdict_keysOfEntriesWithOptionsPassingTest on the storage (a
// tail call).
- (id)keysOfEntriesWithOptions:(NSUInteger)options
                   passingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate {
    dictionaryTSANRead(self, __builtin_return_address(0));
    if (predicate == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: predicate cannot be nil",
            "-[" OF_NAME_STRING(__ONSDictionaryM) " keysOfEntriesWithOptions:passingTest:]"
        );
    }
    return mdict_keysOfEntriesWithOptionsPassingTest(&storage, options, predicate);
}

// Native -[__NSDictionaryM objectForKeyedSubscript:], 0x1c02f6c80. Non-external. The body of -objectForKey:.
- (id)objectForKeyedSubscript:(id)key {
    dictionaryTSANRead(self, __builtin_return_address(0));
    NSUInteger index = storage.bits.szidx;
    if (index == 0) {
        return nil;
    }
    NSUInteger size = sizeAtIndex(index);
    id *keys = storage.buffer;
    NSUInteger slot = findSlot(keys, size, size, key, NULL);
    if (slot >= size) {
        return nil;
    }
    return keys[size + slot];
}

// Native -[__NSDictionaryM removeEntriesWithOptions:passingTest:], 0x1c02f6da8, with its .cold.1
// (0x1c0317398). Non-external. The mutation prologue, then -removeObjectForKey: for every key of
// -keysOfEntriesWithOptions:passingTest: (fast enumeration, 16 at a time, with objc_enumerationMutation on a
// changed mutations value).
- (void)removeEntriesWithOptions:(NSUInteger)options
                     passingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate {
    dictionaryTSANWrite(self, __builtin_return_address(0));
    mutateStorage(self);
    id keys = [self keysOfEntriesWithOptions:options passingTest:predicate];
    for (id key in keys) {
        [self removeObjectForKey:key];
    }
}

// Native -[__NSDictionaryM removeAllObjects], 0x1c02f6f60, with its .cold.1 (0x1c03173a4). Non-external.
// After the mutation prologue, with KVO (bit 57 of the storage word read after the prologue's copy-on-write)
// the live keys (up to the count, in slot order) are gathered in a temporary buffer into an NSSet
// (+alloc, -initWithObjects:count:) for -_willChangeValuesForKeys:. The storage is detached first (buffer
// NULL, count and size index cleared); the keys (unless tagged) and objects (unless nil or tagged) are
// released and every slot cleared, deleted markers included. When the storage got a size index meanwhile the
// old buffer is freed; otherwise the emptied buffer and the old size index are put back. Then
// -_didChangeValuesForKeys: and the set's release.
- (void)removeAllObjects {
    dictionaryTSANWrite(self, __builtin_return_address(0));
    mutateStorage(self);
    uint64_t oldWord = storage.bits.word;
    NSUInteger size = sizeAtIndex(oldWord >> 58);
    id *buffer = storage.buffer;
    NSUInteger index = oldWord >> 58;
    id changedKeys = nil;
    if ((oldWord >> 57) & 1) {
        NSUInteger used = (oldWord >> 32) & 0x1FFFFFF;
        id *temporary = alloca(((uint32_t)(temporaryBufferCount(used) * sizeof(id)) + 15) & 0x1FFFFFF0);
        size_t outCount = 0;
        id *heap = NULL;
        if (used >= 0x101) {
            NSUInteger allocationCount = (used & 0x1FFFFFE) != 0 ? used : 1;
            temporary = _OCFCreateArrayStorage(allocationCount, false, &outCount);
            heap = temporary;
        }
        NSUInteger count = 0;
        if (used != 0 && index != 0) {
            id *entry = buffer;
            for (NSUInteger slot = 1;; slot++) {
                id key = *entry;
                entry += 1;
                if (key != nil && key != deletedMarker()) {
                    temporary[count] = key;
                    count += 1;
                }
                if (count == used || slot >= size) {
                    break;
                }
            }
        }
        changedKeys = [[(Class)setClass alloc] initWithObjects:temporary count:count];
        [(id)self _willChangeValuesForKeys:changedKeys];
        free(heap);
    }
    uint64_t word = storage.bits.word & 0x3FFFFFFFFFFFFFF & 0xFE000000FFFFFFFF;
    storage.buffer = NULL;
    storage.bits.word = word;
    if (index != 0) {
        id *objects = buffer + size;
        NSUInteger count = size > 1 ? size : 1;
        for (NSUInteger slot = 0; slot != count; slot++) {
            id key = buffer[slot];
            if (key == nil || key == deletedMarker()) {
                if (key != nil) {
                    buffer[slot] = nil;
                }
                continue;
            }
            if ((intptr_t)key >= 0) {
                [key release];
            }
            id object = objects[slot];
            if ((intptr_t)object >= 1) {
                [object release];
            }
            buffer[slot] = nil;
            objects[slot] = nil;
        }
        word = storage.bits.word;
        if ((word >> 58) != 0) {
            free(buffer);
            goto notify;
        }
    }
    storage.buffer = buffer;
    storage.bits.word = word | (oldWord & 0xFC00000000000000);
notify:
    if ((oldWord >> 57) & 1) {
        [(id)self _didChangeValuesForKeys:changedKeys];
        [changedKeys release];
    }
}

// Native __NSDictionaryM_new, 0x1c02f7260. Hidden (was a private external). The size index is the first whose
// capacity holds the count (a vectorized search; none of the 40 is BRK #1). The instance is
// __CFAllocateObject(__NSDictionaryM, 0); the storage word's low 32 bits become 1 (a 32-bit store), then its
// size index is set, bit 31 cleared and set from option bit 1. A zero count leaves the buffer NULL; otherwise
// it is malloc_type_calloc(1, size << 4, 0x80040B8603338) (the size's low 32 bits), and with both arrays each
// pair is inserted in order: an existing equal key releases its object (unless nil or tagged) and keeps its
// key; a new key is copied with option bit 0 (a nil copy raises NSInvalidArgumentException "NSDictionary:
// -copyWithZone: for key %@ returned nil") or stored and retained (unless nil or tagged), and the count
// grows; the object is stored and retained unless option bit 2 is set or it is nil or tagged. The control
// block is cleared with a store-release.
OCFMutableDictionaryRef
__NSDictionaryM_new(const void **keys, const void **objects, OCFIndex capacity, uintptr_t options) {
    NSUInteger count = (NSUInteger)capacity;
    NSUInteger index;
    for (index = 0; index < 40; index++) {
        if (__NSDictionaryCapacities[index] >= count) {
            break;
        }
    }
    if (index == 40) {
        __builtin_trap();
    }
    uint64_t size = __NSDictionarySizes[index];
    __ONSDictionaryM *dictionary = __CFAllocateObject([__ONSDictionaryM self], 0);
    DictionaryMStorage *storage = &dictionary->storage;
    *(uint32_t *)&storage->bits.word = 1;
    uint64_t word = storage->bits.word;
    word &= 0x3FFFFFFFFFFFFFF;
    word &= 0xFFFFFFFF7FFFFFFF;
    word |= (uint64_t)((options >> 1) & 1) << 31;
    word |= (uint64_t)index << 58;
    storage->bits.word = word;
    if (count == 0) {
        storage->buffer = NULL;
    } else {
        uint32_t size32 = (uint32_t)size;
        id *buffer = malloc_type_calloc(1, (uint64_t)size32 << 4, 0x80040B8603338);
        storage->buffer = buffer;
        if (keys != NULL && objects != NULL) {
            id *storedObjects = buffer + size32;
            NSUInteger probes = (size & 0xFFFFFFFE) != 0 ? size32 : 1;
            for (NSUInteger pair = 0; pair != count; pair++) {
                id key = (id)keys[pair];
                NSUInteger slot = findSlot(buffer, size32, probes, key, NULL);
                if (buffer[slot] != nil) {
                    id oldObject = storedObjects[slot];
                    if ((intptr_t)oldObject >= 1) {
                        [oldObject release];
                    }
                } else {
                    key = (id)keys[pair];
                    if (options & 1) {
                        id copied = [key copyWithZone:NULL];
                        buffer[slot] = copied;
                        if (copied == nil) {
                            OCFStringRef reason = OCFStringCreateWithFormat(
                                kOCFAllocatorSystemDefault,
                                NULL,
                                OF_STR("NSDictionary: -copyWithZone: for key %@ returned nil"),
                                key
                            );
                            objc_exception_throw([(id)dictionaryExceptionClass
                                exceptionWithName:(id)ONSInvalidArgumentException
                                           reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                         userInfo:nil]);
                        }
                    } else {
                        buffer[slot] = key;
                        if ((intptr_t)key >= 1) {
                            [key retain];
                        }
                    }
                    storage->bits.used += 1;
                }
                id object = (id)objects[pair];
                storedObjects[slot] = object;
                if (!(options & 4) && (intptr_t)object >= 1) {
                    [object retain];
                }
            }
        }
    }
    __atomic_store_n(&dictionary->cow, NULL, __ATOMIC_RELEASE);
    return (OCFMutableDictionaryRef)dictionary;
}

// Native -[__NSDictionaryM dealloc], 0x1c02f757c, with its .cold.1 (0x1c03173b0). Non-external. With a
// control block: under its lock a block without a mutable reference crashes (__cow_failure with "Attempting
// to over-release (mutable)"); the mutable count drops, and when no reference remains the storage is cleaned
// and the block freed. Without one the storage is cleaned. Then super.
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
            cow_cleanup(self, 1);
            free(block);
        }
    } else {
        cow_cleanup(self, 1);
    }
    [super dealloc];
}

// Native cow_cleanup, 0x1c02f765c. Non-external; the cleanup slot of ___NSDictionary_cowCallbacks. Kind 1 is
// a
// __NSDictionaryM, any other a __NSFrozenDictionaryM (its storage ivar offset). The storage is detached
// (buffer NULL, count and size index cleared); with a size index the keys (unless nil, tagged or the deleted
// marker) and then the objects (unless nil or tagged) are released; the buffer is freed (a tail call).
void cow_cleanup(id object, int kind) {
    DictionaryMStorage *storage =
        kind == 1 ? &((__ONSDictionaryM *)object)->storage
                  : (DictionaryMStorage *)((char *)object + frozenDictionaryStorageOffset);
    id *buffer = storage->buffer;
    uint64_t word = storage->bits.word;
    NSUInteger index = word >> 58;
    NSUInteger size = sizeAtIndex(index);
    storage->buffer = NULL;
    storage->bits.word = word & 0x3FFFFFFFFFFFFFF & 0xFE000000FFFFFFFF;
    if (index != 0) {
        id *objects = buffer + size;
        for (NSUInteger slot = 0; slot != size; slot++) {
            id key = buffer[slot];
            if ((intptr_t)key >= 1 && key != deletedMarker()) {
                [key release];
            }
        }
        for (NSUInteger slot = 0; slot != size; slot++) {
            id value = objects[slot];
            if ((intptr_t)value >= 1) {
                [value release];
            }
        }
    }
    free(buffer);
}

// Native +[__NSDictionaryM allocWithZone:], 0x1c02f7718. Non-external. The mutable placeholder (a tail call).
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSDictionaryMutablePlaceholder();
}

// Native -[__NSDictionaryM mutableCopy], 0x1c02f771c. Non-external. A receiver without a control block gets
// one (__cow_create mode 1, installed with a compare-and-swap from NULL, acquire-release; the loser's block
// is freed). The copy is __CFAllocateObject(__NSDictionaryM, 0), sharing the storage through __cow_copy (kind
// 1, mode 1).
- (id)mutableCopy {
    dictionaryTSANRead(self, __builtin_return_address(0));
    if (__atomic_load_n(&cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create(self, 1);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject([__ONSDictionaryM self], 0);
    _cow_copy(self, 1, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSDictionary_cowCallbacks, copy, 1);
    return copy;
}

// Native -[__NSDictionaryM mutableCopyWithZone:], 0x1c02f77e4. Non-external. The body of -mutableCopy; the
// zone is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    dictionaryTSANRead(self, __builtin_return_address(0));
    if (__atomic_load_n(&cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create(self, 1);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject([__ONSDictionaryM self], 0);
    _cow_copy(self, 1, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSDictionary_cowCallbacks, copy, 1);
    return copy;
}

// Native -[__NSDictionaryM _cfMutableCopy], 0x1c02f78ac. Non-external. -mutableCopyWithZone: NULL, with bit
// 31 of the receiver's storage word (copied keys) carried into the copy's.
- (id)_cfMutableCopy {
    __ONSDictionaryM *copy = [self mutableCopyWithZone:NULL];
    copy->storage.bits.copyKeys = storage.bits.copyKeys;
    return copy;
}

// Native -[__NSDictionaryM _clumpingFactor], 0x1c02f78f4. Non-external. The keys (-getObjects:andKeys:count:
// into a temporary buffer); with at least two, their -hash modulo the size are sorted (qsort with
// _compare_clumpiness into malloc_type_malloc(8 count + 8, 0x100004000313F17)), the first plus the size
// closes the ring, and for every gap below the even spacing size/count the shortfall's square is accumulated
// (FMADD). The factor is sum * count * count / (size * size) / (count - 1); 0 below two keys.
- (double)_clumpingFactor {
    uint64_t word = storage.bits.word;
    NSUInteger used = (word >> 32) & 0x1FFFFFF;
    id *temporary = alloca(((uint32_t)(temporaryBufferCount(used) * sizeof(id)) + 15) & 0x1FFFFFF0);
    NSUInteger size = sizeAtIndex(word >> 58);
    size_t outCount = 0;
    id *heap = NULL;
    if (used >= 0x101) {
        NSUInteger allocationCount = (used & 0x1FFFFFE) != 0 ? used : 1;
        temporary = _OCFCreateArrayStorage(allocationCount, false, &outCount);
        heap = temporary;
    }
    [self getObjects:NULL andKeys:temporary count:used];
    double factor = 0.0;
    if (used >= 2) {
        NSUInteger *hashes = malloc_type_malloc(used * 8 + 8, 0x100004000313F17);
        for (NSUInteger slot = 0; slot != used; slot++) {
            hashes[slot] = [temporary[slot] hash] % size;
        }
        qsort(hashes, used, 8, _compare_clumpiness);
        hashes[used] = hashes[0] + size;
        double spacing = (double)size / (double)used;
        double sum = 0.0;
        for (NSUInteger slot = 1; slot <= used; slot++) {
            double gap = (double)(hashes[slot] - hashes[slot - 1]);
            double shortfall = spacing > gap ? spacing - gap : 0.0;
            sum = fma(shortfall, shortfall, sum);
        }
        free(hashes);
        factor = sum * (double)used * (double)used / (double)((uint64_t)(uint32_t)size * (uint32_t)size) /
                 (double)(int64_t)(used - 1);
    }
    free(heap);
    return factor;
}

// Native -[__NSDictionaryM _clumpingInterestingThreshold], 0x1c02f7b00. Non-external.
// 1 - 2 count / size + count * count / size / size, in that operation order.
- (double)_clumpingInterestingThreshold {
    uint64_t word = storage.bits.word;
    NSUInteger size = sizeAtIndex(word >> 58);
    uint32_t used = (word >> 32) & 0x1FFFFFF;
    double result = 1.0 - (double)((uint64_t)used << 1) / (double)size;
    double square = (double)((uint64_t)used * used) / (double)size / (double)size;
    return result + square;
}

// Native cow_copy_instance, 0x1c02f8a60 (listed as _cow_copy_instance_2). Non-external; the first slot of
// ___NSDictionary_cowCallbacks. Kind and mode 1 select __NSDictionaryM's ivars, any other value
// __NSFrozenDictionaryM's (their offset symbols): the source's control block is loaded with acquire and
// stored into the copy's with release, then the source's 16-byte storage is copied into the copy's (one
// 128-bit load and store).
static void cow_copy_instance(id source, int kind, id copy, int mode) {
    CopyOnWriteBlock **sourceCow = kind == 1
                                       ? &((__ONSDictionaryM *)source)->cow
                                       : (CopyOnWriteBlock **)((char *)source + frozenDictionaryCowOffset);
    DictionaryMStorage *sourceStorage =
        kind == 1 ? &((__ONSDictionaryM *)source)->storage
                  : (DictionaryMStorage *)((char *)source + frozenDictionaryStorageOffset);
    CopyOnWriteBlock *cow = __atomic_load_n(sourceCow, __ATOMIC_ACQUIRE);
    CopyOnWriteBlock **copyCow = mode == 1 ? &((__ONSDictionaryM *)copy)->cow
                                           : (CopyOnWriteBlock **)((char *)copy + frozenDictionaryCowOffset);
    DictionaryMStorage *copyStorage =
        mode == 1 ? &((__ONSDictionaryM *)copy)->storage
                  : (DictionaryMStorage *)((char *)copy + frozenDictionaryStorageOffset);
    __atomic_store_n(copyCow, cow, __ATOMIC_RELEASE);
    *copyStorage = *sourceStorage;
}

// Native cow_copy_storage, 0x1c02f8ac4 (listed as _cow_copy_storage_2). Non-external; the second slot of
// ___NSDictionary_cowCallbacks, for a __NSDictionaryM. An empty dictionary (no entries counted in bits 32-56)
// gets a NULL buffer and its count and size index cleared (bits 0-31 and 57 kept) and answers YES. Otherwise
// a new buffer of malloc_type_calloc(1, size << 4, 0x80040B8603338) for the low 32 bits of the size of its
// index; a failed allocation installs NULL and answers NO. With a size index, every non-nil key is copied and
// retained unless it is the deleted marker or a tagged pointer (bit 63), then every non-nil object is
// retained unless tagged and copied; the new buffer is installed and YES answered. The old buffer stays with
// the other holders of the control block.
static bool cow_copy_storage(id object) {
    DictionaryMStorage *storage = &((__ONSDictionaryM *)object)->storage;
    uint64_t word = storage->bits.word;
    id *buffer = NULL;
    if ((word & 0x1FFFFFF00000000) == 0) {
        storage->bits.word = word & 0x3FFFFFFFFFFFFFF & 0xFE000000FFFFFFFF;
        storage->buffer = buffer;
        return true;
    }
    uint32_t size = (uint32_t)__NSDictionarySizes[word >> 58];
    buffer = (id *)malloc_type_calloc(1, (size_t)size << 4, 0x80040B8603338);
    if (buffer == NULL) {
        storage->buffer = NULL;
        return false;
    }
    if ((word >> 58) != 0) {
        id *source = storage->buffer;
        for (NSUInteger slot = 0; slot != size; slot++) {
            id key = source[slot];
            if (key != nil) {
                buffer[slot] = key;
                if (key != deletedMarker() && (OF_WORD_SHIFT((uintptr_t)key, 63)) == 0) {
                    [key retain];
                }
            }
        }
        for (NSUInteger slot = size; slot != 2 * (NSUInteger)size; slot++) {
            id value = source[slot];
            if (value != nil) {
                if ((OF_WORD_SHIFT((uintptr_t)value, 63)) == 0) {
                    [value retain];
                }
                buffer[slot] = value;
            }
        }
    }
    storage->buffer = buffer;
    return true;
}

// Native cow_set_cow, 0x1c02f8bd4 (listed as _cow_set_cow_2). Non-external; the third slot of
// ___NSDictionary_cowCallbacks. The control block is stored into the __NSDictionaryM's cow ivar with release.
static void cow_set_cow(id object, CopyOnWriteBlock *cow) {
    __atomic_store_n(&((__ONSDictionaryM *)object)->cow, cow, __ATOMIC_RELEASE);
}

// Native mdict_rehashd, 0x1c02f8be8. Non-external. An index of 40 or more is BRK #1. The new buffer is
// malloc_type_calloc(1, size << 4, 0x80040B8603338) (not checked); every live entry of the old table is
// placed at the first empty slot from its key's -hash modulo the new size (at most max(size, 1) probes). No
// empty slot means the table was mutated meanwhile: __NSMUTABLEDICTIONARY_CONCURRENT_MODIFICATION_DETECTED__.
// The old buffer is freed and the new buffer and size index installed together (the count is kept).
static void mdict_rehashd(__ONSDictionaryM *self, uint32_t index) {
    if (index >= 40) {
        __builtin_trap();
    }
    NSUInteger size = sizeAtIndex(index);
    id *buffer = malloc_type_calloc(1, size << 4, 0x80040B8603338);
    DictionaryMStorage *storage = &self->storage;
    id *oldBuffer = storage->buffer;
    NSUInteger oldIndex = storage->bits.word >> 58;
    if (oldIndex != 0) {
        id *objects = buffer + size;
        NSUInteger oldSize = sizeAtIndex(oldIndex);
        id *oldObjects = oldBuffer + oldSize;
        NSUInteger probes = size > 1 ? size : 1;
        for (NSUInteger oldSlot = 0; oldSlot != oldSize; oldSlot++) {
            id key = oldBuffer[oldSlot];
            if (key == nil || key == deletedMarker()) {
                continue;
            }
            NSUInteger slot = [key hash] % size;
            NSUInteger found = LONG_MAX;
            for (NSUInteger remaining = probes; remaining != 0; remaining--) {
                if (buffer[slot] == nil) {
                    found = slot;
                    break;
                }
                NSUInteger next = slot + 1;
                slot = next - (size > next ? 0 : size);
            }
            if (found == LONG_MAX) {
                __NSMUTABLEDICTIONARY_CONCURRENT_MODIFICATION_DETECTED__((id)self);
            }
            buffer[found] = key;
            objects[found] = oldObjects[oldSlot];
        }
    }
    free(oldBuffer);
    uint64_t word = storage->bits.word;
    word = (word & 0x3FFFFFFFFFFFFFF) | ((uint64_t)index << 58);
    storage->buffer = buffer;
    storage->bits.word = word;
}

// Native _compare_clumpiness, 0x1c02f8e7c (listed as __compare_clumpiness_2). Non-external. The qsort order
// of the unsigned bucket indexes.
static int _compare_clumpiness(const void *first, const void *second) {
    uint64_t left = *(const uint64_t *)first;
    uint64_t right = *(const uint64_t *)second;
    if (left < right) {
        return -1;
    }
    return left > right;
}

// Native -[__NSDictionaryM copy], 0x1c03171cc (cold region), with OUTLINED_FUNCTION_1_43-_6_15 folded in.
// Non-external. The read check runs twice (the second reloads the hook). An empty receiver answers
// __NSDictionary0__ (not retained). Otherwise the receiver gets a control block as in -mutableCopy, and the
// copy is __CFAllocateObject(__NSFrozenDictionaryM, 0) sharing the storage through __cow_copy (kind 1, mode
// 0).
- (id)copy {
    void *returnAddress = __builtin_return_address(0);
    dictionaryTSANRead(self, returnAddress);
    dictionaryTSANRead(self, returnAddress);
    if ([self count] == 0) {
        return (id)__ONSDictionary0__;
    }
    if (__atomic_load_n(&cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create(self, 1);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject([(Class)frozenDictionaryClass self], 0);
    _cow_copy(self, 1, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSDictionary_cowCallbacks, copy, 0);
    return copy;
}

// Native -[__NSDictionaryM copyWithZone:], 0x1c0317274 (cold region). Non-external. The body of -copy; the
// zone is not read.
- (id)copyWithZone:(struct _NSZone *)zone {
    void *returnAddress = __builtin_return_address(0);
    dictionaryTSANRead(self, returnAddress);
    dictionaryTSANRead(self, returnAddress);
    if ([self count] == 0) {
        return (id)__ONSDictionary0__;
    }
    if (__atomic_load_n(&cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create(self, 1);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject([(Class)frozenDictionaryClass self], 0);
    _cow_copy(self, 1, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSDictionary_cowCallbacks, copy, 0);
    return copy;
}

// Native __NSMUTABLEDICTIONARY_CONCURRENT_MODIFICATION_DETECTED__, 0x1c031731c (cold region). Non-external.
// CFLog (level 3) "*** Collection <%s: %p> was mutated while rehashing." with the class name and the object,
// then BRK #1.
static void __NSMUTABLEDICTIONARY_CONCURRENT_MODIFICATION_DETECTED__(id object) {
    OCFLog(
        3,
        OF_STR("*** Collection <%s: %p> was mutated while rehashing."),
        class_getName([object class]),
        object
    );
    __builtin_trap();
}

@end
