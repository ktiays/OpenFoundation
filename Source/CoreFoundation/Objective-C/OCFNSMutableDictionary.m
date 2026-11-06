//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"
#import "CoreFoundation/Objective-C/OCFNSDictionaryEnumerate.h"

#include <pthread.h>
#include <stdlib.h>

// Inferred organization: the NSMutableDictionary methods (0x1c01ea744-0x1c01ed99b, with
// OUTLINED_FUNCTION_0_24 at 0x1c01ed988 and the block invoke of -removeEntriesWithOptions:passingTest: at
// 0x1c01eb974) lie together in the iOS Simulator __text section after the __NSDictionary0 methods;
// -removeObjectForKey:, -setObject:forKey: and -initWithCapacity: (0x1c030c274-0x1c030c2e7) lie in the cold
// region after
// -[__NSDictionary0 dealloc]. Following the owner's decision, the class has its own translation unit, in
// native address order. Objective-C compiled with manual reference counting (objc_autorelease, no ARC
// entry points); the native source path is not in the evidence. Message sends to self go through id where
// the method is not one of this unit's (the class interface declares no methods). Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read in full, iphone-simulator-corpus-delta8.json). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).
//
// The mutating methods share one prologue: the method's return address (X30) is kept, -_mutate is sent to
// the receiver, and then the ThreadSanitizer write hook is called with that return address. The invalid
// argument raises are the inlined os_log pack pattern of DICTIONARY_RAISE_WITH_LOG_PACK; their method names
// are the native string literals ("-[NSMutableDictionary ...]"), spelled with the class's selected name.

// Class symbols sent to directly (class references off_2227CE390, off_2227CE360).
extern char mutableSetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableSet));
extern char mutableArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));

// Selectors this unit sends that no included header declares, declared for the message sends only; the
// protocol is never adopted and no protocol object is emitted.
@protocol MutableDictionaryMethodMessages

+ (id)set;
+ (id)array;
- (id)allKeys;
- (id)allValues;
- (NSUInteger)countForKey:(id)key;

@end

// The method names the raises format, the native literals with the class's selected name.
#define MUTABLE_DICTIONARY_METHOD(selectorText) "-[" OF_NAME_STRING(ONSMutableDictionary) " " selectorText "]"

@implementation ONSMutableDictionary

// Native -[NSMutableDictionary _mutate], 0x1c01ea744. Non-external. Empty; subclasses override it.
- (void)_mutate {
}

// Native -[NSMutableDictionary addEntriesFromDictionary:], 0x1c01ea748. Non-external. A non-nil argument
// must pass _NSIsNSDictionary (os_log pack of 12 bytes). Adding the receiver to itself does nothing;
// otherwise every key of the argument (fast enumeration, 16 at a time) is set with the argument's object.
- (void)addEntriesFromDictionary:(id)otherDictionary {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (otherDictionary != nil && !_ONSIsNSDictionary(otherDictionary)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: dictionary argument is not an NSDictionary",
            MUTABLE_DICTIONARY_METHOD("addEntriesFromDictionary:")
        );
    }
    if (self == otherDictionary) {
        return;
    }
    for (id key in otherDictionary) {
        [self setObject:[otherDictionary objectForKey:key] forKey:key];
    }
}

// Native -[NSMutableDictionary addObject:forKey:], 0x1c01ea958. Non-external. Order: the object, then the key
// (os_log packs of 12 bytes); then the object is set only when -objectForKey: answers nil.
- (void)addObject:(id)object forKey:(id)key {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (object == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: object cannot be nil",
            MUTABLE_DICTIONARY_METHOD("addObject:forKey:")
        );
    }
    if (key == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: key cannot be nil",
            MUTABLE_DICTIONARY_METHOD("addObject:forKey:")
        );
    }
    if ([(id)self objectForKey:key] == nil) {
        [self setObject:object forKey:key];
    }
}

// Native -[NSMutableDictionary addObjects:forKeys:count:], 0x1c01eab70. Non-external. The argument checks
// (os_log packs of 22 bytes, the method name and one unsigned long): a NULL objects array with a nonzero
// count, a count with any of the top three bits set, a nil object (its index), a NULL keys array with a
// nonzero count (the native message names the objects array here too), a nil key (its index; the native
// message says "objects[%lu]" here too). Then each pair is added with -objectForKey: and -setObject:forKey:
// (the object only when the key is absent).
- (void)addObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (objects == NULL && count != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            MUTABLE_DICTIONARY_METHOD("addObjects:forKeys:count:"),
            count
        );
    }
    if ((count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            MUTABLE_DICTIONARY_METHOD("addObjects:forKeys:count:"),
            count
        );
    }
    if (count == 0) {
        return;
    }
    for (NSUInteger index = 0; index != count; ++index) {
        if (objects[index] == nil) {
            DICTIONARY_RAISE_WITH_LOG_PACK(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: attempt to insert nil object from objects[%lu]",
                MUTABLE_DICTIONARY_METHOD("addObjects:forKeys:count:"),
                index
            );
        }
    }
    if (keys == NULL) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            MUTABLE_DICTIONARY_METHOD("addObjects:forKeys:count:"),
            count
        );
    }
    for (NSUInteger index = 0; index != count; ++index) {
        if (keys[index] == nil) {
            DICTIONARY_RAISE_WITH_LOG_PACK(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: attempt to insert nil object from objects[%lu]",
                MUTABLE_DICTIONARY_METHOD("addObjects:forKeys:count:"),
                index
            );
        }
    }
    do {
        if ([(id)self objectForKey:*keys] == nil) {
            [self setObject:*objects forKey:*keys];
        }
        ++objects;
        ++keys;
    } while (--count != 0);
}

// Native -[NSMutableDictionary addObjects:forKeys:], 0x1c01eaf14. Non-external. Non-nil arguments must pass
// _NSIsNSArray (objects first; os_log packs of 12 bytes). Unequal counts raise through
// _CFThrowFormattedException (the method name, the objects' count, the keys' count). -count is sent to the
// objects array a second time for the loop, which sends -addObject:forKey:.
- (void)addObjects:(id)objects forKeys:(id)keys {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (objects != nil && !_ONSIsNSArray(objects)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: objects argument is not an NSArray",
            MUTABLE_DICTIONARY_METHOD("addObjects:forKeys:")
        );
    }
    if (keys != nil && !_ONSIsNSArray(keys)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: keys argument is not an NSArray",
            MUTABLE_DICTIONARY_METHOD("addObjects:forKeys:")
        );
    }
    NSUInteger objectCount = [objects count];
    NSUInteger keyCount = [keys count];
    if (objectCount != keyCount) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException,
            OF_STR("*** %s: count of objects (%lu) differs from count of keys (%lu)"),
            MUTABLE_DICTIONARY_METHOD("addObjects:forKeys:"),
            objectCount,
            keyCount
        );
    }
    NSUInteger count = [objects count];
    for (NSUInteger index = 0; index != count; ++index) {
        id object = [objects objectAtIndex:index];
        [self addObject:object forKey:[keys objectAtIndex:index]];
    }
}

// Native -[NSMutableDictionary invert], 0x1c01eb1a4. Non-external. The keys and objects are read (-allKeys,
// -allValues), the receiver is emptied, and each former object becomes the key of its former key (the index
// runs across fast-enumeration batches of the keys).
- (void)invert {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    id keys = [(id)self allKeys];
    id values = [(id)self allValues];
    [self removeAllObjects];
    NSUInteger index = 0;
    for (id key in keys) {
        [self setObject:key forKey:[values objectAtIndex:index]];
        ++index;
    }
}

// Native -[NSMutableDictionary removeAllObjects], 0x1c01eb308. Non-external. The keys are copied into a
// temporary id buffer (the native new_id_array pattern: a count with any of the top four bits set throws
// NSGenericException with a reason made with the default allocator and released after the exception is
// created; up to 256 entries on the stack, else _CFCreateArrayStorage), read with -getObjects:andKeys:count:
// (no objects array); each key is removed as many times as -countForKey: answers. The heap storage is freed
// at the end (no unwind cleanup natively).
- (void)removeAllObjects {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    NSUInteger count = [(id)self count];
    if ((count >> 60) != 0) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            NULL,
            NULL,
            OF_STR(
                "*** attempt to create a temporary id buffer which is too large or with a negative count "
                "(%lu) "
                "-- possibly data is corrupt"
            ),
            count
        );
        id exception = [(id)dictionaryExceptionClass exceptionWithName:(id)ONSGenericException
                                                                reason:(id)reason
                                                              userInfo:nil];
        OCFRelease(reason);
        objc_exception_throw(exception);
    }
    size_t bufferCount = count > 1 ? count : 1;
    id stackBuffer[count < 0x101 ? bufferCount : 1];
    size_t allocatedCount = 0;
    void *allocated = NULL;
    id *keys = stackBuffer;
    if (count >= 0x101) {
        keys = _OCFCreateArrayStorage(bufferCount, false, &allocatedCount);
        allocated = keys;
    }
    [(id)self getObjects:NULL andKeys:keys count:count];
    for (NSUInteger index = 0; index != count; ++index) {
        NSUInteger occurrences = [(id)self countForKey:keys[index]];
        for (; occurrences != 0; --occurrences) {
            [self removeObjectForKey:keys[index]];
        }
    }
    free(allocated);
}

// Native -[NSMutableDictionary removeEntriesInDictionary:], 0x1c01eb4d0. Non-external. A non-nil argument
// must pass _NSIsNSDictionary (os_log pack of 12 bytes). The receiver itself empties it (-removeAllObjects,
// a tail call); otherwise each key of the argument is removed.
- (void)removeEntriesInDictionary:(id)otherDictionary {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (otherDictionary != nil && !_ONSIsNSDictionary(otherDictionary)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: dictionary argument is not an NSDictionary",
            MUTABLE_DICTIONARY_METHOD("removeEntriesInDictionary:")
        );
    }
    if (self == otherDictionary) {
        [self removeAllObjects];
        return;
    }
    for (id key in otherDictionary) {
        [self removeObjectForKey:key];
    }
}

// Native -[NSMutableDictionary removeEntriesWithOptions:passingTest:], 0x1c01eb6f8, with its block
// ___60-[NSMutableDictionary removeEntriesWithOptions:passingTest:]_block_invoke (0x1c01eb974). Non-external.
// A nil predicate raises (os_log pack of 12 bytes); __NSDictionaryParameterCheckIterate then checks it again.
// The keys passing the predicate are collected into +[NSMutableSet set] under a stack pthread mutex
// (PTHREAD_MUTEX_INITIALIZER, never destroyed) by __NSDictionaryEnumerate with the options less
// NSEnumerationReverse (bit 1 cleared); then each collected key is removed. The block passes the key, object
// and stop pointer through to the predicate unchanged.
- (void)removeEntriesWithOptions:(NSUInteger)options
                     passingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (predicate == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: predicate cannot be nil",
            MUTABLE_DICTIONARY_METHOD("removeEntriesWithOptions:passingTest:")
        );
    }
    __NSDictionaryParameterCheckIterate(self, _cmd, (id)predicate, options & ~(NSUInteger)2);
    id keys = [(Class)mutableSetClass set];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t *lock = &mutex;
    __NSDictionaryEnumerate(self, options & ~(NSUInteger)2, ^(id key, id object, BOOL *stop) {
      if (predicate(key, object, stop)) {
          pthread_mutex_lock(lock);
          [keys addObject:key];
          pthread_mutex_unlock(lock);
      }
    });
    for (id key in keys) {
        [self removeObjectForKey:key];
    }
}

// Native -[NSMutableDictionary removeEntriesPassingTest:], 0x1c01eb9c8. Non-external. No -_mutate here: the
// write hook, the nil check (os_log pack of 12 bytes), then -removeEntriesWithOptions:0 passingTest: (a tail
// call).
- (void)removeEntriesPassingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate {
    dictionaryTSANWrite(self, __builtin_return_address(0));
    if (predicate == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: predicate cannot be nil",
            MUTABLE_DICTIONARY_METHOD("removeEntriesPassingTest:")
        );
    }
    [self removeEntriesWithOptions:0 passingTest:predicate];
}

// Native -[NSMutableDictionary removeKeysForObject:], 0x1c01ebb1c. Non-external. The keys whose object is
// identical (pointer comparison) to the argument are collected into +[NSMutableArray array] while the
// receiver is enumerated, then removed.
- (void)removeKeysForObject:(id)object {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    id keys = [(Class)mutableArrayClass array];
    for (id key in (id)self) {
        if ([(id)self objectForKey:key] == object) {
            [keys addObject:key];
        }
    }
    for (id key in keys) {
        [self removeObjectForKey:key];
    }
}

// Native -[NSMutableDictionary removeObjectsForKeys:], 0x1c01ebcec. Non-external. A non-nil argument must
// pass _NSIsNSArray (os_log pack of 12 bytes); each of its elements is removed.
- (void)removeObjectsForKeys:(id)keyArray {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (keyArray != nil && !_ONSIsNSArray(keyArray)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: array argument is not an NSArray",
            MUTABLE_DICTIONARY_METHOD("removeObjectsForKeys:")
        );
    }
    for (id key in keyArray) {
        [self removeObjectForKey:key];
    }
}

// Native -[NSMutableDictionary replaceObject:forKey:], 0x1c01ebed8. Non-external. Order: the object, then the
// key (os_log packs of 12 bytes); the object is set only when -objectForKey: answers non-nil.
- (void)replaceObject:(id)object forKey:(id)key {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (object == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: object cannot be nil",
            MUTABLE_DICTIONARY_METHOD("replaceObject:forKey:")
        );
    }
    if (key == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: key cannot be nil",
            MUTABLE_DICTIONARY_METHOD("replaceObject:forKey:")
        );
    }
    if ([(id)self objectForKey:key] != nil) {
        [self setObject:object forKey:key];
    }
}

// Native -[NSMutableDictionary replaceObjects:forKeys:count:], 0x1c01ec0f0. Non-external. The checks of
// -addObjects:forKeys:count: (the same messages and order); then -replaceObject:forKey: for each pair.
- (void)replaceObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (objects == NULL && count != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            MUTABLE_DICTIONARY_METHOD("replaceObjects:forKeys:count:"),
            count
        );
    }
    if ((count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            MUTABLE_DICTIONARY_METHOD("replaceObjects:forKeys:count:"),
            count
        );
    }
    if (count == 0) {
        return;
    }
    for (NSUInteger index = 0; index != count; ++index) {
        if (objects[index] == nil) {
            DICTIONARY_RAISE_WITH_LOG_PACK(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: attempt to insert nil object from objects[%lu]",
                MUTABLE_DICTIONARY_METHOD("replaceObjects:forKeys:count:"),
                index
            );
        }
    }
    if (keys == NULL) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            MUTABLE_DICTIONARY_METHOD("replaceObjects:forKeys:count:"),
            count
        );
    }
    for (NSUInteger index = 0; index != count; ++index) {
        if (keys[index] == nil) {
            DICTIONARY_RAISE_WITH_LOG_PACK(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: attempt to insert nil object from objects[%lu]",
                MUTABLE_DICTIONARY_METHOD("replaceObjects:forKeys:count:"),
                index
            );
        }
    }
    do {
        [self replaceObject:*objects++ forKey:*keys++];
    } while (--count != 0);
}

// Native -[NSMutableDictionary replaceObjects:forKeys:], 0x1c01ec47c. Non-external. The checks of
// -addObjects:forKeys:; then -replaceObject:forKey: for each pair (the objects array's -count is sent a
// second time for the loop).
- (void)replaceObjects:(id)objects forKeys:(id)keys {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (objects != nil && !_ONSIsNSArray(objects)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: objects argument is not an NSArray",
            MUTABLE_DICTIONARY_METHOD("replaceObjects:forKeys:")
        );
    }
    if (keys != nil && !_ONSIsNSArray(keys)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: keys argument is not an NSArray",
            MUTABLE_DICTIONARY_METHOD("replaceObjects:forKeys:")
        );
    }
    NSUInteger objectCount = [objects count];
    NSUInteger keyCount = [keys count];
    if (objectCount != keyCount) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException,
            OF_STR("*** %s: count of objects (%lu) differs from count of keys (%lu)"),
            MUTABLE_DICTIONARY_METHOD("replaceObjects:forKeys:"),
            objectCount,
            keyCount
        );
    }
    NSUInteger count = [objects count];
    for (NSUInteger index = 0; index != count; ++index) {
        id object = [objects objectAtIndex:index];
        [self replaceObject:object forKey:[keys objectAtIndex:index]];
    }
}

// Native -[NSMutableDictionary setEntriesFromDictionary:], 0x1c01ec70c. Non-external. The body of
// -addEntriesFromDictionary: with its own method name.
- (void)setEntriesFromDictionary:(id)otherDictionary {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (otherDictionary != nil && !_ONSIsNSDictionary(otherDictionary)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: dictionary argument is not an NSDictionary",
            MUTABLE_DICTIONARY_METHOD("setEntriesFromDictionary:")
        );
    }
    if (self == otherDictionary) {
        return;
    }
    for (id key in otherDictionary) {
        [self setObject:[otherDictionary objectForKey:key] forKey:key];
    }
}

// Native -[NSMutableDictionary setObject:forKeyedSubscript:], 0x1c01ec91c. Non-external. No -_mutate: the
// write hook, then -setObject:forKey: for a non-nil object or -removeObjectForKey: for nil (tail calls).
- (void)setObject:(id)object forKeyedSubscript:(id)key {
    dictionaryTSANWrite(self, __builtin_return_address(0));
    if (object == nil) {
        [self removeObjectForKey:key];
        return;
    }
    [self setObject:object forKey:key];
}

// Native -[NSMutableDictionary setObjects:forKeys:count:], 0x1c01ec998. Non-external. The checks of
// -addObjects:forKeys:count:; then -setObject:forKey: for each pair.
- (void)setObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (objects == NULL && count != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            MUTABLE_DICTIONARY_METHOD("setObjects:forKeys:count:"),
            count
        );
    }
    if ((count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            MUTABLE_DICTIONARY_METHOD("setObjects:forKeys:count:"),
            count
        );
    }
    if (count == 0) {
        return;
    }
    for (NSUInteger index = 0; index != count; ++index) {
        if (objects[index] == nil) {
            DICTIONARY_RAISE_WITH_LOG_PACK(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: attempt to insert nil object from objects[%lu]",
                MUTABLE_DICTIONARY_METHOD("setObjects:forKeys:count:"),
                index
            );
        }
    }
    if (keys == NULL) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            MUTABLE_DICTIONARY_METHOD("setObjects:forKeys:count:"),
            count
        );
    }
    for (NSUInteger index = 0; index != count; ++index) {
        if (keys[index] == nil) {
            DICTIONARY_RAISE_WITH_LOG_PACK(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: attempt to insert nil object from objects[%lu]",
                MUTABLE_DICTIONARY_METHOD("setObjects:forKeys:count:"),
                index
            );
        }
    }
    do {
        [self setObject:*objects++ forKey:*keys++];
    } while (--count != 0);
}

// Native -[NSMutableDictionary setObjects:forKeys:], 0x1c01ecd24. Non-external. The checks of
// -addObjects:forKeys:; then -setObject:forKey: for each pair. Unlike the add and replace forms, the loop
// uses the first -count of the objects array.
- (void)setObjects:(id)objects forKeys:(id)keys {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (objects != nil && !_ONSIsNSArray(objects)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: objects argument is not an NSArray",
            MUTABLE_DICTIONARY_METHOD("setObjects:forKeys:")
        );
    }
    if (keys != nil && !_ONSIsNSArray(keys)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: keys argument is not an NSArray",
            MUTABLE_DICTIONARY_METHOD("setObjects:forKeys:")
        );
    }
    NSUInteger count = [objects count];
    NSUInteger keyCount = [keys count];
    if (count != keyCount) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException,
            OF_STR("*** %s: count of objects (%lu) differs from count of keys (%lu)"),
            MUTABLE_DICTIONARY_METHOD("setObjects:forKeys:"),
            count,
            keyCount
        );
    }
    for (NSUInteger index = 0; index != count; ++index) {
        id object = [objects objectAtIndex:index];
        [self setObject:object forKey:[keys objectAtIndex:index]];
    }
}

// Native -[NSMutableDictionary setDictionary:], 0x1c01ecfa8. Non-external. A non-nil argument must pass
// _NSIsNSDictionary (os_log pack of 12 bytes). The receiver itself is left unchanged; otherwise the receiver
// is emptied and every entry of the argument is set.
- (void)setDictionary:(id)otherDictionary {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (otherDictionary != nil && !_ONSIsNSDictionary(otherDictionary)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: dictionary argument is not an NSDictionary",
            MUTABLE_DICTIONARY_METHOD("setDictionary:")
        );
    }
    if (self == otherDictionary) {
        return;
    }
    [self removeAllObjects];
    for (id key in otherDictionary) {
        [self setObject:[otherDictionary objectForKey:key] forKey:key];
    }
}

// Native -[NSMutableDictionary __addObject:forKey:], 0x1c01ed1c0. Non-external. The checks of
// -addObject:forKey: with its own method name; an absent key is set through -__setObject:forKey: (a tail
// call).
- (void)__addObject:(id)object forKey:(id)key {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (object == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: object cannot be nil",
            MUTABLE_DICTIONARY_METHOD("__addObject:forKey:")
        );
    }
    if (key == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: key cannot be nil",
            MUTABLE_DICTIONARY_METHOD("__addObject:forKey:")
        );
    }
    if ([(id)self objectForKey:key] == nil) {
        [self __setObject:object forKey:key];
    }
}

// Native -[NSMutableDictionary __setObject:forKey:], 0x1c01ed3d8. Non-external. The checks of
// -addObject:forKey: with its own method name; a key that does not respond to -copyWithZone: (the selector
// at 0x2387aa02e, outside the extracted image; its name is fixed by -[__NSCFData copyWithZone:]'s super
// send of the same selector reference in iphone-simulator-core-corpus.json) logs at level 3 through CFLog;
// then -setObject:forKey: (a tail call).
- (void)__setObject:(id)object forKey:(id)key {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    dictionaryTSANWrite(self, returnAddress);
    if (object == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: object cannot be nil",
            MUTABLE_DICTIONARY_METHOD("__setObject:forKey:")
        );
    }
    if (key == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: key cannot be nil",
            MUTABLE_DICTIONARY_METHOD("__setObject:forKey:")
        );
    }
    if (![key respondsToSelector:@selector(copyWithZone:)]) {
        OCFLog(
            3,
            OF_STR(
                "*** CFDictionarySetValue(): attempt to use this function to set a key which is not copyable "
                "into "
                "a non-CFDictionary via toll-free bridging"
            )
        );
    }
    [self setObject:object forKey:key];
}

// Native -[NSMutableDictionary initWithObjects:forKeys:count:], 0x1c01ed5d8. Non-external. No -_mutate and
// no write hook. The checks of -addObjects:forKeys:count: with its own method name, except that a zero count
// answers -initWithCapacity:0 (a tail call) before the element checks; otherwise -initWithCapacity: with the
// count, and each pair is set on the dictionary it answers, which is returned.
- (id)initWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count {
    if (objects == NULL && count != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            MUTABLE_DICTIONARY_METHOD("initWithObjects:forKeys:count:"),
            count
        );
    }
    if ((count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            MUTABLE_DICTIONARY_METHOD("initWithObjects:forKeys:count:"),
            count
        );
    }
    if (count == 0) {
        return [self initWithCapacity:0];
    }
    for (NSUInteger index = 0; index != count; ++index) {
        if (objects[index] == nil) {
            DICTIONARY_RAISE_WITH_LOG_PACK(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: attempt to insert nil object from objects[%lu]",
                MUTABLE_DICTIONARY_METHOD("initWithObjects:forKeys:count:"),
                index
            );
        }
    }
    if (keys == NULL) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            MUTABLE_DICTIONARY_METHOD("initWithObjects:forKeys:count:"),
            count
        );
    }
    for (NSUInteger index = 0; index != count; ++index) {
        if (keys[index] == nil) {
            DICTIONARY_RAISE_WITH_LOG_PACK(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: attempt to insert nil object from objects[%lu]",
                MUTABLE_DICTIONARY_METHOD("initWithObjects:forKeys:count:"),
                index
            );
        }
    }
    id dictionary = [self initWithCapacity:count];
    do {
        [dictionary setObject:*objects++ forKey:*keys++];
    } while (--count != 0);
    return dictionary;
}

// Native +[NSMutableDictionary dictionaryWithCapacity:], 0x1c01ed960. Non-external. +alloc (objc_alloc),
// -initWithCapacity:, autoreleased (a tail call to objc_autorelease).
+ (id)dictionaryWithCapacity:(NSUInteger)numItems {
    return [[[self alloc] initWithCapacity:numItems] autorelease];
}

// Native -[NSMutableDictionary removeObjectForKey:], 0x1c030c274, with OUTLINED_FUNCTION_0_24 (0x1c01ed988:
// __CFLookUpClass("NSMutableDictionary"), keeping the receiver and selector). Non-external. The abstract
// method: __CFRequireConcreteImplementation with the looked-up class, the receiver and the selector (a tail
// call).
- (void)removeObjectForKey:(id)key {
    __CFRequireConcreteImplementation(
        (Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableDictionary)), self, _cmd
    );
}

// Native -[NSMutableDictionary setObject:forKey:], 0x1c030c298, with OUTLINED_FUNCTION_0_24. Non-external.
// The abstract method, as -removeObjectForKey:.
- (void)setObject:(id)object forKey:(id)key {
    __CFRequireConcreteImplementation(
        (Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableDictionary)), self, _cmd
    );
}

// Native -[NSMutableDictionary initWithCapacity:], 0x1c030c2bc, with OUTLINED_FUNCTION_0_24. Non-external.
// The abstract method, as -removeObjectForKey:, answering nil when __CFRequireConcreteImplementation returns.
- (id)initWithCapacity:(NSUInteger)numItems {
    __CFRequireConcreteImplementation(
        (Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableDictionary)), self, _cmd
    );
    return nil;
}

@end
