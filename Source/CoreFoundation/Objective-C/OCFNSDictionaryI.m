//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSDictionaryI.h"
#import "CoreFoundation/Objective-C/OCFNSDictionaryConcurrentEnumeration.h"
#import "CoreFoundation/Objective-C/OCFNSDictionaryEnumerate.h"
#import "CoreFoundation/Objective-C/OCFNSDictionaryM.h"

#include <math.h>
#include <os/lock.h>
#include <stdlib.h>

// Inferred organization: the __NSDictionaryI methods (0x1c018b074-0x1c018c86c) lie together in the iOS
// Simulator __text section, between the preceding unit's code and _CFBundleCopyResolvedURLForExecutableURL;
// following the owner's decision, the class has its own translation unit. The unit's C functions
// __NSDictionaryI_new (0x1c018bfa8, with its block 0x1c018c1b4), _NSDictionaryI_mutableCopyWithZone
// (0x1c018c480) and _compare_clumpiness (0x1c018c858) lie among the methods and are kept in native order;
// those that read the instance variables are defined inside the @implementation. The unit's constant data,
// __NSDictionarySizes (0x1c0373210, 40 words), the const_mu of -countByEnumeratingWithState:objects:count:
// (0x1c0373350) and __NSDictionaryCapacities (0x1c0373358, 40 words), lie together in __TEXT,__const; their
// values were read from the pinned image with Local/Scratch/cf-core/macho.py. Objective-C compiled with
// manual reference counting (objc_retain, objc_release, objc_autorelease). A key or object is retained or
// released only when it is a positive pointer (the native signed comparisons with 1, which skip nil and
// tagged pointers). Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json. Unless a definition
// says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// Native __NSDictionarySizes, 0x1c0373210 (__TEXT,__const, non-external): the table sizes in entries (key and
// object pairs) of each _szidx; the methods index it with _szidx (0-39).
static const uint64_t __NSDictionarySizes[40] = {
    0,       3,        7,        13,       23,       41,       71,        127,       191,       251,
    383,     631,      1087,     1723,     2803,     4523,     7351,      11959,     19447,     31231,
    50683,   81919,    132607,   214519,   346607,   561109,   907759,    1468927,   2376191,   3845119,
    6221311, 10066421, 16287743, 26354171, 42641881, 68996069, 111638519, 180634607, 292272623, 472907251,
};

// Native __NSDictionaryCapacities, 0x1c0373358 (__TEXT,__const, non-external): the largest count each
// _szidx holds.
static const uint64_t __NSDictionaryCapacities[40] = {
    0,       3,       6,        11,       19,       32,       52,       85,        118,       155,
    237,     390,     672,      1065,     1732,     2795,     4543,     7391,      12019,     19302,
    31324,   50629,   81956,    132580,   214215,   346784,   561026,   907847,    1468567,   2376414,
    3844982, 6221390, 10066379, 16287773, 26354132, 42641916, 68996399, 111638327, 180634415, 292272755,
};

// The __NSFastEnumerationEnumerator, NSMutableSet, NSSet and NSException class symbols, loaded natively
// through their class references (off_2227CE440, off_2227CE390, off_2227CE3B8, off_2227CE300).
extern char dictionaryIFastEnumerationEnumeratorClass[] __asm(
    "_OBJC_CLASS_$_" OF_NAME_STRING(__ONSFastEnumerationEnumerator)
);
extern char dictionaryIMutableSetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableSet));
extern char dictionaryISetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSSet));

// The offset of __NSDictionaryM's storage instance variable (_OBJC_IVAR_$___NSDictionaryM.storage), which
// -_cfMutableCopy writes in the copy it makes.
extern const int32_t
    dictionaryMStorageIvarOffset __asm("_OBJC_IVAR_$_" OF_NAME_STRING(__ONSDictionaryM) ".storage");

// Selectors this unit sends that no included header declares, for the message sends only.
@protocol DictionaryIMessages

+ (id)set;
+ (id)setWithSet:(id)set;

@end

// Native _compare_clumpiness, 0x1c018c858. Non-external. The qsort comparison of -_clumpingFactor: the
// unsigned words compare as 1 (greater), -1 (less) or 0.
static int _compare_clumpiness(const void *first, const void *second) {
    uint64_t a = *(const uint64_t *)first;
    uint64_t b = *(const uint64_t *)second;
    if (a < b) {
        return -1;
    }
    return a > b ? 1 : 0;
}

// Native _NSDictionaryI_mutableCopyWithZone, 0x1c018c480; defined inside the implementation below, in
// native address order.
static id _NSDictionaryI_mutableCopyWithZone(__ONSDictionaryI *self);

@implementation __ONSDictionaryI

// Native -[__NSDictionaryI count], 0x1c018b074. Non-external.
- (NSUInteger)count {
    return self->_used;
}

// Native -[__NSDictionaryI objectForKey:], 0x1c018b088. Non-external. An empty table (_szidx 0, tested as the
// byte at offset 15 below 4) answers nil. Otherwise linear probing from -hash modulo the size, at most size
// probes: a slot whose key is NULL, the same pointer or -isEqual: to the argument (the stored key is the
// receiver) ends the search and its object is answered (NULL for an empty slot); no match answers nil.
- (id)objectForKey:(id)key {
    if (self->_szidx == 0) {
        return nil;
    }
    uint64_t size = __NSDictionarySizes[self->_szidx];
    id *list = self->_list;
    uint64_t index = (uint64_t)[key hash] % size;
    uint64_t remaining = size;
    for (;;) {
        id stored = list[index * 2];
        if (stored == nil || stored == key || [stored isEqual:key]) {
            break;
        }
        uint64_t next = index + 1;
        index = next - (size > next ? 0 : size);
        remaining -= 1;
        if (remaining == 0) {
            return nil;
        }
    }
    if (index * 2 >= size * 2) {
        return nil;
    }
    return self->_list[index * 2 + 1];
}

// Native -[__NSDictionaryI keyEnumerator], 0x1c018b16c. Non-external. A fast-enumeration enumerator over the
// receiver (objc_allocWithZone), autoreleased.
- (id)keyEnumerator {
    id enumerator = [(Class)dictionaryIFastEnumerationEnumeratorClass allocWithZone:NULL];
    return [[enumerator initWithObject:self] autorelease];
}

// Native -[__NSDictionaryI getObjects:andKeys:count:], 0x1c018b19c. Non-external. A count with any of its top
// three bits set raises when the objects array is given, then when the keys array is given (os_log pack
// buffer 0x16: the method name and the count). Then the occupied slots are copied in table order, at most
// min(count, _used) entries.
- (void)getObjects:(id *)objects andKeys:(id *)keys count:(NSUInteger)count {
    if (objects != NULL && (count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSDictionaryI) " getObjects:andKeys:count:]",
            count
        );
    }
    if (keys != NULL && (count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSDictionaryI) " getObjects:andKeys:count:]",
            count
        );
    }
    uint64_t slots = __NSDictionarySizes[self->_szidx] * 2;
    uint64_t used = self->_used;
    uint64_t remaining = used < count ? used : count;
    if (slots == 0 || remaining == 0) {
        return;
    }
    id *entry = self->_list;
    for (uint64_t index = 2;; index += 2) {
        if (entry[0] != nil) {
            if (keys != NULL) {
                *keys++ = entry[0];
            }
            if (objects != NULL) {
                *objects++ = entry[1];
            }
            remaining -= 1;
        }
        if (index >= slots) {
            return;
        }
        entry += 2;
        if (remaining == 0) {
            return;
        }
    }
}

// Native +[__NSDictionaryI automaticallyNotifiesObserversForKey:], 0x1c018b398. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSDictionaryI __apply:context:], 0x1c018b3a0. Non-external. A NULL function raises
// NSInvalidArgumentException "%@: function pointer is NULL" (the __CFExceptionProem of the receiver and
// selector; no os_log pack). Otherwise the function is called with each occupied slot's key, object and the
// context, in table order.
- (void)__apply:(void (*)(const void *key, const void *object, void *context))applier
        context:(void *)context {
    if (applier == NULL) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFStringRef reason = OCFStringCreateWithFormat(
            allocator, NULL, OF_STR("%@: function pointer is NULL"), __CFExceptionProem(self, _cmd)
        );
        id exception =
            [(id)dictionaryExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                     reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                                   userInfo:nil];
        objc_exception_throw(exception);
    }
    uint64_t slots = __NSDictionarySizes[self->_szidx] * 2;
    if (slots == 0) {
        return;
    }
    id *list = self->_list;
    for (uint64_t index = 0; index < slots; index += 2) {
        if (list[index] != nil) {
            applier(list[index], list[index + 1], context);
        }
    }
}

// Native -[__NSDictionaryI countByEnumeratingWithState:objects:count:], 0x1c018b48c. Non-external. A NULL
// buffer with a nonzero length raises first, then a length with any of its top three bits set (os_log pack
// buffer 0x16 each: the method name and the length). state->state is the next slot; the first call points
// mutationsPtr at the constant const_mu (1). The keys of the occupied slots are written to the buffer, and
// state->state is stored after every slot and reread after every key, as natively.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    // Native countByEnumeratingWithState:objects:count:.const_mu, 0x1c0373350 (__TEXT,__const).
    static const unsigned long const_mu = 1;
    DictionaryEnumerationState *enumerationState = state;
    if (buffer == NULL && length != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(__ONSDictionaryI) " countByEnumeratingWithState:objects:count:]",
            length
        );
    }
    if ((length >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSDictionaryI) " countByEnumeratingWithState:objects:count:]",
            length
        );
    }
    uint64_t size = __NSDictionarySizes[self->_szidx];
    unsigned long position = enumerationState->state;
    if (position == 0) {
        enumerationState->mutationsPtr = (unsigned long *)&const_mu;
    }
    if (size <= position) {
        return 0;
    }
    enumerationState->itemsPtr = (void **)buffer;
    if (length == 0) {
        return 0;
    }
    NSUInteger count = 0;
    id *list = self->_list;
    do {
        id key = list[position * 2];
        position += 1;
        enumerationState->state = position;
        if (key != nil) {
            buffer[count] = key;
            count += 1;
            position = enumerationState->state;
        }
        if (position >= size) {
            break;
        }
    } while (count < length);
    return count;
}

// Native -[__NSDictionaryI enumerateKeysAndObjectsWithOptions:usingBlock:], 0x1c018b6d0, with its block
// ___65-[__NSDictionaryI enumerateKeysAndObjectsWithOptions:usingBlock:]_block_invoke (0x1c018b8d0,
// non-external). A nil block raises (os_log pack buffer 0xC: the method name). The block literal (receiver
// and block) is offered to __NSCollectionHandleConcurrentEnumerationIfSpecified with the options, no pool and
// the table size as the iteration count; it calls the block for an occupied slot inside an autorelease pool
// (the list reread after the push). When that answers NO and the table is not empty (_szidx tested as its
// byte, read before the call), the slots are visited in order, each occupied one inside its own pool, until
// the stop flag is set.
- (void)enumerateKeysAndObjectsWithOptions:(NSUInteger)options usingBlock:(void (^)(id, id, BOOL *))block {
    if (block == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            "-[" OF_NAME_STRING(__ONSDictionaryI) " enumerateKeysAndObjectsWithOptions:usingBlock:]"
        );
    }
    uint64_t szidx = self->_szidx;
    uint64_t size = __NSDictionarySizes[szidx];
    BOOL concurrent =
        __NSCollectionHandleConcurrentEnumerationIfSpecified(options, NO, size, ^(size_t index, BOOL *stop) {
          id key = self->_list[index * 2];
          if (key == nil) {
              return;
          }
          void *pool = _OCFAutoreleasePoolPush();
          __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(block, key, self->_list[index * 2 + 1], stop);
          _OCFAutoreleasePoolPop(pool);
        });
    if (concurrent) {
        return;
    }
    BOOL stop = NO;
    if (szidx == 0) {
        return;
    }
    id *entry = self->_list;
    for (uint64_t index = 1;; index += 1) {
        id key = entry[0];
        if (key != nil) {
            void *pool = _OCFAutoreleasePoolPush();
            __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(block, key, entry[1], &stop);
            _OCFAutoreleasePoolPop(pool);
            if (stop) {
                return;
            }
        }
        if (index >= size) {
            return;
        }
        entry += 2;
    }
}

// Native -[__NSDictionaryI keyOfEntryWithOptions:passingTest:], 0x1c018b95c, with its block
// ___53-[__NSDictionaryI keyOfEntryWithOptions:passingTest:]_block_invoke (0x1c018bb0c, non-external). No nil
// check of the predicate. The result is a __block word (the key's address): the concurrent block, for an
// occupied slot inside an autorelease pool, stores the key with release ordering (STLR) and sets the stop
// flag when the predicate answers YES, and the method reads it back with acquire ordering (LDAR). Serially
// (an empty table answers nil) at most max(size, 1) slots are visited, each occupied one inside its own pool;
// a YES sets the stop flag and the key. The predicate is called directly, not through
// __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__.
- (id)keyOfEntryWithOptions:(NSUInteger)options passingTest:(BOOL (^)(id, id, BOOL *))predicate {
    uint64_t szidx = self->_szidx;
    uint64_t size = __NSDictionarySizes[szidx];
    __block uintptr_t result = 0;
    id found;
    BOOL concurrent =
        __NSCollectionHandleConcurrentEnumerationIfSpecified(options, NO, size, ^(size_t index, BOOL *stop) {
          id *list = self->_list;
          id key = list[index * 2];
          if (key == nil) {
              return;
          }
          id object = list[index * 2 + 1];
          void *pool = _OCFAutoreleasePoolPush();
          if (predicate(key, object, stop)) {
              __atomic_store_n(&result, (uintptr_t)key, __ATOMIC_RELEASE);
              *stop = YES;
          }
          _OCFAutoreleasePoolPop(pool);
        });
    if (concurrent) {
        found = (id)__atomic_load_n(&result, __ATOMIC_ACQUIRE);
    } else {
        BOOL stop = NO;
        found = nil;
        if (szidx != 0) {
            uint64_t remaining = size > 1 ? size : 1;
            id *entry = self->_list;
            do {
                id key = entry[0];
                if (key != nil) {
                    id object = entry[1];
                    void *pool = _OCFAutoreleasePoolPush();
                    if (predicate(key, object, &stop)) {
                        stop = YES;
                        found = key;
                    }
                    _OCFAutoreleasePoolPop(pool);
                    if (stop) {
                        break;
                    }
                }
                entry += 2;
                remaining -= 1;
            } while (remaining != 0);
        }
    }
    return found;
}

// Native -[__NSDictionaryI keysOfEntriesWithOptions:passingTest:], 0x1c018bbb8, with its block
// ___56-[__NSDictionaryI keysOfEntriesWithOptions:passingTest:]_block_invoke (0x1c018be10, non-external). A
// nil predicate raises (os_log pack buffer 0xC: the method name). The keys go to +[NSMutableSet set]; the
// concurrent block adds a passing key (an occupied slot, inside an autorelease pool) under an os_unfair_lock
// on the method's stack. After a concurrent enumeration the mutable set itself is answered; otherwise the
// slots are visited in order (each occupied one inside its own pool, until the stop flag is set) and
// +[NSSet setWithSet:] of the keys is answered.
- (id)keysOfEntriesWithOptions:(NSUInteger)options passingTest:(BOOL (^)(id, id, BOOL *))predicate {
    if (predicate == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: predicate cannot be nil",
            "-[" OF_NAME_STRING(__ONSDictionaryI) " keysOfEntriesWithOptions:passingTest:]"
        );
    }
    uint64_t szidx = self->_szidx;
    uint64_t size = __NSDictionarySizes[szidx];
    id keys = [(Class)dictionaryIMutableSetClass set];
    os_unfair_lock lock = OS_UNFAIR_LOCK_INIT;
    os_unfair_lock_t lockPointer = &lock;
    BOOL concurrent =
        __NSCollectionHandleConcurrentEnumerationIfSpecified(options, NO, size, ^(size_t index, BOOL *stop) {
          id *list = self->_list;
          id key = list[index * 2];
          if (key == nil) {
              return;
          }
          id object = list[index * 2 + 1];
          void *pool = _OCFAutoreleasePoolPush();
          if (predicate(key, object, stop)) {
              os_unfair_lock_lock(lockPointer);
              [keys addObject:key];
              os_unfair_lock_unlock(lockPointer);
          }
          _OCFAutoreleasePoolPop(pool);
        });
    if (concurrent) {
        return keys;
    }
    BOOL stop = NO;
    if (szidx != 0) {
        id *entry = self->_list;
        for (uint64_t index = 1;; index += 1) {
            id key = entry[0];
            if (key != nil) {
                id object = entry[1];
                void *pool = _OCFAutoreleasePoolPush();
                if (predicate(key, object, &stop)) {
                    [keys addObject:key];
                }
                _OCFAutoreleasePoolPop(pool);
                if (stop) {
                    break;
                }
            }
            if (index >= size) {
                break;
            }
            entry += 2;
        }
    }
    return [(Class)dictionaryISetClass setWithSet:keys];
}

// Native -[__NSDictionaryI objectForKeyedSubscript:], 0x1c018bec0. Non-external. The body of
// -objectForKey:, instruction for instruction.
- (id)objectForKeyedSubscript:(id)key {
    if (self->_szidx == 0) {
        return nil;
    }
    uint64_t size = __NSDictionarySizes[self->_szidx];
    id *list = self->_list;
    uint64_t index = (uint64_t)[key hash] % size;
    uint64_t remaining = size;
    for (;;) {
        id stored = list[index * 2];
        if (stored == nil || stored == key || [stored isEqual:key]) {
            break;
        }
        uint64_t next = index + 1;
        index = next - (size > next ? 0 : size);
        remaining -= 1;
        if (remaining == 0) {
            return nil;
        }
    }
    if (index * 2 >= size * 2) {
        return nil;
    }
    return self->_list[index * 2 + 1];
}

// Native +[__NSDictionaryI allocWithZone:], 0x1c018bfa4. Non-external. A tail call of
// __NSDictionaryImmutablePlaceholder.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSDictionaryImmutablePlaceholder();
}

// Native __NSDictionaryI_new, 0x1c018bfa8, with its block ____NSDictionaryI_new_block_invoke (0x1c018c1b4,
// non-external). Hidden (was a private external). _szidx is the first index whose __NSDictionaryCapacities
// entry is at least the count; the instance is allocated with __CFAllocateObject([__NSDictionaryI self],
// size * 16) and gets _szidx, _used = count and _copyKeys = option bit 1, in that order. The entries are then
// inserted through the block literal (the instance, the list, the size and the options): with an interleaved
// array (the varargs form), the first pair is the array's first word as the key and keys[0] as the object,
// and every further pair is an object followed by its key; otherwise keys[i] and objects[i]. Option bits, as
// the block reads them: 1 copy each key (-copyWithZone: NULL), 4 the caller transfers the objects (no retain;
// released when the key is a duplicate), 8 the caller transfers the keys (no retain or copy; released when
// duplicated); bit 2 is only recorded in _copyKeys. A key equal to one already inserted keeps the first entry
// and decrements _used.
// CoreFoundation defect: the native search runs up to 64 indexes over the 40-entry capacity table, reading
// the constant data after it (another unit's const_mu, then unrelated words) for a count above 292272755,
// which selects a 0-entry table that the insertions then overrun. The restoration traps (BRK #1, as the
// native code does after 64 indexes) once the 40 entries are exhausted.
OCFDictionaryRef __NSDictionaryI_new(
    const void **keys, const void **objects, const void **keysAndObjects, OCFIndex count, uintptr_t options
) {
    uint64_t szidx = 0;
    while ((uint64_t)count > __NSDictionaryCapacities[szidx]) {
        szidx += 1;
        if (szidx == 40) {
            __builtin_trap();
        }
    }
    uint64_t size = __NSDictionarySizes[szidx];
    __ONSDictionaryI *dictionary = __CFAllocateObject([__ONSDictionaryI self], size << 4);
    dictionary->_szidx = szidx;
    dictionary->_used = (uint64_t)count;
    dictionary->_copyKeys = (options >> 1) & 1;
    if (count == 0) {
        return (OCFDictionaryRef)dictionary;
    }
    id *list = dictionary->_list;
    void (^insert)(id, id) = ^(id key, id object) {
      uint64_t index = (uint64_t)[key hash] % size;
      uint64_t remaining = size;
      uint64_t slot;
      for (;;) {
          slot = index * 2;
          id stored = list[slot];
          if (stored == nil || stored == key || [stored isEqual:key]) {
              break;
          }
          uint64_t next = index + 1;
          index = next - (size > next ? 0 : size);
          remaining -= 1;
          if (remaining == 0) {
              slot = size * 2;
              break;
          }
      }
      if (list[slot] != nil) {
          dictionary->_used = dictionary->_used - 1;
          if ((options & 8) != 0) {
              [key release];
          }
          if ((options & 4) != 0) {
              [object release];
          }
          return;
      }
      if ((options & 8) != 0) {
          list[slot] = key;
      } else if ((options & 1) != 0) {
          list[slot] = [key copyWithZone:NULL];
          if (list[slot] == nil) {
              OCFStringRef reason = OCFStringCreateWithFormat(
                  kOCFAllocatorSystemDefault,
                  NULL,
                  OF_STR("NSDictionary: -copyWithZone: for key %@ returned nil"),
                  NULL
              );
              id exception =
                  [(id)dictionaryExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                           reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                                         userInfo:nil];
              objc_exception_throw(exception);
          }
      } else {
          if ((intptr_t)key >= 1) {
              [key retain];
          }
          list[slot] = key;
      }
      list[slot + 1] = object;
      if ((intptr_t)object >= 1 && (options & 4) == 0) {
          [object retain];
      }
    };
    if (keysAndObjects != NULL) {
        const void **cursor = keysAndObjects;
        const void *firstObject = keys[0];
        const void *firstKey = *cursor++;
        insert((id)firstKey, (id)firstObject);
        while (--count != 0) {
            const void *object = *cursor++;
            const void *key = *cursor++;
            insert((id)key, (id)object);
        }
    } else {
        do {
            const void *key = *keys++;
            const void *object = *objects++;
            insert((id)key, (id)object);
        } while (--count != 0);
    }
    return (OCFDictionaryRef)dictionary;
}

// Native -[__NSDictionaryI dealloc], 0x1c018c384. Non-external. The empty singleton __NSDictionary0__ traps
// (BRK #1). Otherwise _szidx and _used are cleared (_copyKeys kept), every slot word of the table that is a
// positive pointer is released, keys and objects in table order, and super's -dealloc runs.
- (void)dealloc {
    if (self == (id)__ONSDictionary0__) {
        __builtin_trap();
    }
    uint64_t size = __NSDictionarySizes[self->_szidx];
    self->_szidx = 0;
    self->_used = 0;
    uint64_t slots = size * 2;
    id *list = self->_list;
    for (; slots != 0; slots -= 1) {
        id value = *list++;
        if ((intptr_t)value >= 1) {
            [value release];
        }
    }
    [super dealloc];
}

// Native -[__NSDictionaryI copy], 0x1c018c474. Non-external. A tail call of objc_retain.
- (id)copy {
    return [self retain];
}

// Native -[__NSDictionaryI copyWithZone:], 0x1c018c478. Non-external. A tail call of objc_retain.
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native -[__NSDictionaryI mutableCopy], 0x1c018c47c. Non-external. A tail call of
// _NSDictionaryI_mutableCopyWithZone.
- (id)mutableCopy {
    return _NSDictionaryI_mutableCopyWithZone(self);
}

// Native _NSDictionaryI_mutableCopyWithZone (symbol __NSDictionaryI_mutableCopyWithZone), 0x1c018c480.
// Non-external; the receiver is its only argument. The keys and objects are fetched with
// -getObjects:andKeys:count: into one buffer, keys first: on the stack for up to 0x80 entries (2 * _used
// words, at least one), otherwise from _CFCreateArrayStorage(2 * _used, false, ...), freed afterwards (free
// of NULL for the stack buffer); then __NSDictionaryM_new(keys, objects, _used, 2).
static id _NSDictionaryI_mutableCopyWithZone(__ONSDictionaryI *self) {
    uint64_t count = self->_used;
    uint64_t words = count != 0 ? count * 2 : 1;
    uint64_t stackWords = count >= 0x81 ? 1 : words;
    id stackBuffer[stackWords];
    id *buffer = stackBuffer;
    size_t storageCount = 0;
    void *storage = NULL;
    if (count >= 0x81) {
        buffer = _OCFCreateArrayStorage(count * 2, false, &storageCount);
        storage = buffer;
    }
    id *objects = buffer + count;
    [self getObjects:objects andKeys:buffer count:count];
    id copy = (id)__NSDictionaryM_new((const void **)buffer, (const void **)objects, (OCFIndex)count, 2);
    free(storage);
    return copy;
}

// Native -[__NSDictionaryI mutableCopyWithZone:], 0x1c018c598. Non-external. A tail call of
// _NSDictionaryI_mutableCopyWithZone; the zone is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return _NSDictionaryI_mutableCopyWithZone(self);
}

// Native -[__NSDictionaryI _cfMutableCopy], 0x1c018c59c. Non-external. The mutable copy, with the receiver's
// _copyKeys stored into bit 31 of the second word of the copy's storage instance variable (__NSDictionaryM).
- (id)_cfMutableCopy {
    id copy = _NSDictionaryI_mutableCopyWithZone(self);
    uint64_t copyKeys = self->_copyKeys;
    uint64_t *storage = (uint64_t *)((char *)copy + dictionaryMStorageIvarOffset);
    storage[1] = (storage[1] & ~(UINT64_C(1) << 31)) | (copyKeys << 31);
    return copy;
}

// Native -[__NSDictionaryI _clumpingFactor], 0x1c018c5e8. Non-external. The keys are fetched with
// -getObjects:NULL andKeys:count:_used (on the stack below 0x101 entries, at least one word; otherwise from
// _CFCreateArrayStorage(_used, false, ...), _used reread). With at least two entries, each key's -hash modulo
// the table size goes into a malloc_type_malloc buffer of _used + 1 words (type 0x100004000313F17), sorted
// with qsort and _compare_clumpiness, and closed with the first value plus the size. Over the gaps between
// neighbours the squared shortfall from the mean gap size / _used is summed (FMADD, a gap above the mean
// contributes +0.0); the factor is sum * n * n / (size * size) / (n - 1). Fewer entries answer 0.0.
- (double)_clumpingFactor {
    uint64_t used = self->_used;
    uint64_t words = used > 1 ? used : 1;
    uint64_t stackWords = used >= 0x101 ? 1 : words;
    id stackBuffer[stackWords];
    id *keys = stackBuffer;
    size_t storageCount = 0;
    void *storage = NULL;
    if (used >= 0x101) {
        keys = _OCFCreateArrayStorage(words, false, &storageCount);
        used = self->_used;
        storage = keys;
    }
    [self getObjects:NULL andKeys:keys count:used];
    uint64_t count = self->_used;
    double factor = 0.0;
    if (count >= 2) {
        uint64_t size = __NSDictionarySizes[self->_szidx];
        uint64_t *hashes = malloc_type_malloc(count * 8 + 8, 0x100004000313F17);
        for (uint64_t index = 0; index < count; index += 1) {
            hashes[index] = (uint64_t)[keys[index] hash] % size;
        }
        qsort(hashes, count, 8, _compare_clumpiness);
        hashes[count] = hashes[0] + size;
        double sizeValue = (double)size;
        double countValue = (double)count;
        double mean = sizeValue / countValue;
        double sum = 0.0;
        for (uint64_t index = 0; index < count; index += 1) {
            double gap = (double)(hashes[index + 1] - hashes[index]);
            double shortfall = mean > gap ? mean - gap : 0.0;
            sum = fma(shortfall, shortfall, sum);
        }
        free(hashes);
        double scaled = sum * countValue * countValue / (sizeValue * sizeValue);
        factor = scaled / (countValue + -1.0);
    }
    free(storage);
    return factor;
}

// Native -[__NSDictionaryI _clumpingInterestingThreshold], 0x1c018c800. Non-external. With u = _used and s
// the table size (as doubles): 1 - (u + u) / s + u * u / s / s, in that operation order.
- (double)_clumpingInterestingThreshold {
    double used = (double)self->_used;
    double size = (double)__NSDictionarySizes[self->_szidx];
    double linear = 1.0 - (used + used) / size;
    return linear + used * used / size / size;
}

@end
