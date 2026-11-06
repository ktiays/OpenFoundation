//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSFrozenDictionaryM.h"
#include "CoreFoundation/Objective-C/OCFNSDictionaryConcurrentEnumeration.h"
#include "CoreFoundation/Objective-C/OCFNSDictionaryEnumerate.h"

#include <os/lock.h>
#include <stdlib.h>

// Inferred organization: the __NSFrozenDictionaryM methods (0x1c02f7b58-0x1c02f8a5f) lie inside the native
// __NSDictionaryM unit (Objective-C/OCFNSDictionaryM.m), after cow_cleanup and +[__NSDictionaryM
// allocWithZone:] ... -[__NSDictionaryM _clumpingFactor] and before that unit's cow_copy_instance; the
// -dealloc .cold.1 lies at 0x1c03173c4 in the cold region. That unit belongs to another sub-track, so the
// class has this translation unit, named for the class; the unit's statics it reads are declared in
// Objective-C/OCFNSFrozenDictionaryM.h. Natively the frozen methods share the unit's block invoke functions
// (___mdict_enumerateKeysAndObjectsWithOptionsUsingBlock_block_invoke, 0x1c02f8d4c, and
// ___mdict_keyOfEntryWithOptionsPassingTest_block_invoke, 0x1c02f8dd4) with the __NSDictionaryM methods; this
// unit compiles its own copies of those blocks with the same bodies. The unit's static
// _mdict_countByEnumeratingWithStateObjectsCount.const_mu (0x1c046fcc0, __TEXT,__const, the value 1, read
// with Local/Scratch/cf-core/macho.py) is defined here, where its only reader lives. Objective-C compiled
// with manual reference counting (objc_retain). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full).
// iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not
// written yet).

// libobjc's objc_retain, which the native image imports; the SDK installs no declaration for it.
extern id objc_retain(id object);

// Class symbols allocated directly (class reference off_2227CE440 and ___SCR___NSDictionaryM).
extern char frozenFastEnumerationEnumeratorClass[] __asm(
    "_OBJC_CLASS_$_" OF_NAME_STRING(__ONSFastEnumerationEnumerator)
);

// The deleted marker as an object pointer. frozenDeletedMarker is a local descriptive name.
static inline id frozenDeletedMarker(void) {
    return (id)&___NSDictionaryM_DeletedMarker;
}

// The low 32 bits of a size, as the methods load them. frozenSizeAtIndex is a local descriptive name.
static inline NSUInteger frozenSizeAtIndex(NSUInteger index) {
    return (uint32_t)__NSDictionarySizes[index];
}

// The probe -objectForKey: and -objectForKeyedSubscript: inline: from the key's -hash modulo the size, at
// most size slots in order (wrapping to 0). An empty slot ends the search; a deleted marker is skipped (the
// first one seen remembered); a slot holding the key itself or a key that answers -isEqual: (sent to the
// stored key) with the key ends it. The answer is that slot, else the first deleted or empty slot passed,
// else the size. frozenFindSlot is a local descriptive name.
static inline NSUInteger frozenFindSlot(id *keys, NSUInteger size, id key) {
    NSUInteger index = [key hash] % size;
    NSUInteger slot = size;
    for (NSUInteger probes = size; probes != 0; probes--) {
        id candidate = keys[index];
        if (candidate == nil) {
            if (slot == size) {
                slot = index;
            }
            break;
        }
        if (candidate == frozenDeletedMarker()) {
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

// Native mdict_countByEnumeratingWithStateObjectsCount, inlined into -[__NSFrozenDictionaryM
// countByEnumeratingWithState:objects:count:] after its argument checks (no out-of-line copy is in the
// image). The first call points the mutations pointer at the constant const_mu (1): a frozen dictionary does
// not mutate. The state is the next slot; live keys are copied into the caller's buffer, which becomes the
// items pointer, and the state is stored once at the end.
static inline NSUInteger mdict_countByEnumeratingWithStateObjectsCount(
    DictionaryMStorage *storage, DictionaryEnumerationState *state, id *buffer, NSUInteger length
) {
    // Native _mdict_countByEnumeratingWithStateObjectsCount.const_mu, 0x1c046fcc0 (__TEXT,__const).
    static const unsigned long const_mu = 1;
    unsigned long index = state->state;
    NSUInteger size = frozenSizeAtIndex(storage->bits.szidx);
    if (index == 0) {
        state->mutationsPtr = (unsigned long *)&const_mu;
    }
    if (index >= size) {
        return 0;
    }
    id *keys = storage->buffer;
    state->itemsPtr = (void **)buffer;
    NSUInteger count = 0;
    if (length != 0) {
        do {
            id key = keys[index];
            if (key != nil && key != frozenDeletedMarker()) {
                buffer[count] = key;
                count += 1;
            }
            index += 1;
        } while (index < size && count < length);
    }
    state->state = index;
    return count;
}

@implementation __ONSFrozenDictionaryM

// Native -[__NSFrozenDictionaryM count], 0x1c02f7b58. Non-external. The 25-bit entry count.
- (NSUInteger)count {
    return storage.bits.used;
}

// Native -[__NSFrozenDictionaryM getObjects:andKeys:count:], 0x1c02f7b70. Non-external. A count with any of
// its top three bits set raises when the objects array is given, then when the keys array is; both raises
// carry "count (%lu) of objects array is ridiculous" (os_log pack buffer 0x16). At most min(count, entries)
// live entries are copied in slot order, the key before the object.
- (void)getObjects:(id *)objects andKeys:(id *)keys count:(NSUInteger)count {
    if (objects != NULL && (count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSFrozenDictionaryM) " getObjects:andKeys:count:]",
            count
        );
    }
    if (keys != NULL && (count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSFrozenDictionaryM) " getObjects:andKeys:count:]",
            count
        );
    }
    NSUInteger index = storage.bits.szidx;
    NSUInteger used = storage.bits.used;
    NSUInteger remaining = used < count ? used : count;
    if (index == 0 || remaining == 0) {
        return;
    }
    NSUInteger size = frozenSizeAtIndex(index);
    id *slots = storage.buffer;
    for (NSUInteger slot = 1;; slot++) {
        id *entry = slots;
        slots += 1;
        id key = *entry;
        if (key != nil && key != frozenDeletedMarker()) {
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

// Native -[__NSFrozenDictionaryM keyEnumerator], 0x1c02f7d6c. Non-external. A fast-enumeration enumerator
// over the receiver (allocWithZone: NULL), autoreleased.
- (id)keyEnumerator {
    id enumerator = [(Class)frozenFastEnumerationEnumeratorClass allocWithZone:NULL];
    return [[enumerator initWithObject:self] autorelease];
}

// Native -[__NSFrozenDictionaryM objectForKey:], 0x1c02f7d9c. Non-external. An empty table answers nil
// without hashing; otherwise the object of the probed slot (nil for an empty or deleted slot) or nil when the
// probe found no slot.
- (id)objectForKey:(id)key {
    NSUInteger index = storage.bits.szidx;
    if (index == 0) {
        return nil;
    }
    NSUInteger size = frozenSizeAtIndex(index);
    id *keys = storage.buffer;
    NSUInteger slot = frozenFindSlot(keys, size, key);
    if (slot >= size) {
        return nil;
    }
    return keys[size + slot];
}

// Native -[__NSFrozenDictionaryM __apply:context:], 0x1c02f7e9c. Non-external. A NULL function raises
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
    NSUInteger size = frozenSizeAtIndex(index);
    id *keys = storage.buffer;
    NSUInteger count = size > 1 ? size : 1;
    for (; count != 0; count--) {
        id *entry = keys;
        keys += 1;
        id key = *entry;
        if (key != nil && key != frozenDeletedMarker()) {
            applier(key, entry[size], context);
        }
    }
}

// Native -[__NSFrozenDictionaryM countByEnumeratingWithState:objects:count:], 0x1c02f7f98. Non-external. A
// NULL buffer with a nonzero length raises "pointer to objects array is NULL but length is %lu", a length
// with any of its top three bits set "count (%lu) of objects array is ridiculous" (os_log pack buffer 0x16
// each); then mdict_countByEnumeratingWithStateObjectsCount.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    if (buffer == NULL && length != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(__ONSFrozenDictionaryM) " countByEnumeratingWithState:objects:count:]",
            length
        );
    }
    if ((length >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSFrozenDictionaryM) " countByEnumeratingWithState:objects:count:]",
            length
        );
    }
    return mdict_countByEnumeratingWithStateObjectsCount(&storage, state, buffer, length);
}

// Native -[__NSFrozenDictionaryM enumerateKeysAndObjectsWithOptions:usingBlock:], 0x1c02f81e0, with the
// unit's block ___mdict_enumerateKeysAndObjectsWithOptionsUsingBlock_block_invoke (0x1c02f8d4c).
// Non-external. A nil block raises "block cannot be nil" (os_log pack buffer 0xC). With the concurrent option
// honored (__NSCollectionHandleConcurrentEnumerationIfSpecified, no per-call pool) each live entry is called
// out in its own autorelease pool from the dispatch_apply block; otherwise in slot order, each in its own
// pool, until the block sets stop.
- (void)enumerateKeysAndObjectsWithOptions:(NSUInteger)options
                                usingBlock:(void (^)(id key, id object, BOOL *stop))block {
    if (block == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            "-[" OF_NAME_STRING(__ONSFrozenDictionaryM) " enumerateKeysAndObjectsWithOptions:usingBlock:]"
        );
    }
    id *keys = storage.buffer;
    uint64_t word = storage.bits.word;
    NSUInteger size = frozenSizeAtIndex(word >> 58);
    id *objects = keys + size;
    BOOL handled =
        __NSCollectionHandleConcurrentEnumerationIfSpecified(options, NO, size, ^(size_t index, BOOL *stop) {
          id key = keys[index];
          if (key == nil || key == frozenDeletedMarker()) {
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
    for (NSUInteger slot = 1;; slot++) {
        id *entry = keys;
        keys += 1;
        id key = *entry;
        if (key != nil && key != frozenDeletedMarker()) {
            void *pool = _OCFAutoreleasePoolPush();
            __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(block, key, entry[size], &stop);
            _OCFAutoreleasePoolPop(pool);
            if (stop) {
                break;
            }
        }
        if (slot >= size) {
            break;
        }
    }
}

// Native -[__NSFrozenDictionaryM keyOfEntryWithOptions:passingTest:], 0x1c02f83f8, with the unit's block
// ___mdict_keyOfEntryWithOptionsPassingTest_block_invoke (0x1c02f8dd4). Non-external. The predicate is not
// checked for nil. With the concurrent option honored, a passing key is stored with release semantics into a
// __block variable and stop is set; the answer is loaded with acquire. Otherwise the live entries are tested
// in slot order (max(size, 1) slots), each in its own autorelease pool (the object loaded before the push),
// and the first passing key sets stop and is answered.
- (id)keyOfEntryWithOptions:(NSUInteger)options
                passingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate {
    id *keys = storage.buffer;
    uint64_t word = storage.bits.word;
    NSUInteger size = frozenSizeAtIndex(word >> 58);
    id *objects = keys + size;
    __block id concurrentResult = nil;
    BOOL handled =
        __NSCollectionHandleConcurrentEnumerationIfSpecified(options, NO, size, ^(size_t index, BOOL *stop) {
          id key = keys[index];
          if (key == nil || key == frozenDeletedMarker()) {
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
                if (key == nil || key == frozenDeletedMarker()) {
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

// Native -[__NSFrozenDictionaryM keysOfEntriesWithOptions:passingTest:], 0x1c02f85bc. Non-external. A nil
// predicate raises "predicate cannot be nil" (os_log pack buffer 0xC); otherwise
// mdict_keysOfEntriesWithOptionsPassingTest on the storage (a tail call).
- (id)keysOfEntriesWithOptions:(NSUInteger)options
                   passingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate {
    if (predicate == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: predicate cannot be nil",
            "-[" OF_NAME_STRING(__ONSFrozenDictionaryM) " keysOfEntriesWithOptions:passingTest:]"
        );
    }
    return mdict_keysOfEntriesWithOptionsPassingTest(&storage, options, predicate);
}

// Native -[__NSFrozenDictionaryM objectForKeyedSubscript:], 0x1c02f86e4. Non-external. The body of
// -objectForKey:.
- (id)objectForKeyedSubscript:(id)key {
    NSUInteger index = storage.bits.szidx;
    if (index == 0) {
        return nil;
    }
    NSUInteger size = frozenSizeAtIndex(index);
    id *keys = storage.buffer;
    NSUInteger slot = frozenFindSlot(keys, size, key);
    if (slot >= size) {
        return nil;
    }
    return keys[size + slot];
}

// Native +[__NSFrozenDictionaryM allocWithZone:], 0x1c02f87e4. Non-external. The immutable placeholder (a
// tail call).
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSDictionaryImmutablePlaceholder();
}

// Native -[__NSFrozenDictionaryM dealloc], 0x1c02f87e8, with its .cold.1 (0x1c03173c4). Non-external. With a
// control block: under its lock a block without an immutable reference crashes (__cow_failure with
// "Attempting to over-release (immutable)"); the immutable count drops, and when no reference remains the
// storage is cleaned (cow_cleanup kind 0) and the block freed. Without one the storage is cleaned. Then
// super.
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
            cow_cleanup(self, 0);
            free(block);
        }
    } else {
        cow_cleanup(self, 0);
    }
    [super dealloc];
}

// Native -[__NSFrozenDictionaryM copy], 0x1c02f88c8. Non-external. The receiver retained (objc_retain, a
// tail call).
- (id)copy {
    return objc_retain(self);
}

// Native -[__NSFrozenDictionaryM copyWithZone:], 0x1c02f88cc. Non-external. The body of -copy.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

// Native -[__NSFrozenDictionaryM mutableCopy], 0x1c02f88d0. Non-external. The ThreadSanitizer read check; a
// receiver without a control block gets one (__cow_create mode 0, installed with a compare-and-swap from
// NULL, acquire-release; the loser's block is freed). The copy is __CFAllocateObject(__NSDictionaryM, 0),
// sharing the storage through __cow_copy (kind 0, mode 1).
- (id)mutableCopy {
    dictionaryTSANRead(self, __builtin_return_address(0));
    if (__atomic_load_n(&cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create(self, 0);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject([__ONSDictionaryM self], 0);
    _cow_copy(self, 0, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSDictionary_cowCallbacks, copy, 1);
    return copy;
}

// Native -[__NSFrozenDictionaryM mutableCopyWithZone:], 0x1c02f8998. Non-external. The body of -mutableCopy;
// the zone is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    dictionaryTSANRead(self, __builtin_return_address(0));
    if (__atomic_load_n(&cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create(self, 0);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject([__ONSDictionaryM self], 0);
    _cow_copy(self, 0, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSDictionary_cowCallbacks, copy, 1);
    return copy;
}

@end
