//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSConstantDictionary.h"
#import "CoreFoundation/Objective-C/OCFNSDictionaryEnumerate.h"

#include <stdlib.h>
#include <string.h>

// Inferred organization: the NSConstantDictionary methods (0x1c01f8520-0x1c01f9370) lie together in the iOS
// Simulator __text section, followed by the unit's static comparators comparisonUsingOrderingForStringKeys
// (0x1c01f9378) and comparisonUsingOrderingForNumericKeys (0x1c01f93cc), after which the CFURLComponents unit
// begins; the class's trapping methods -initWithObjects:forKeys:count: (0x1c030c390), -init (0x1c030c3a4) and
// -dealloc (0x1c030c3a8) lie in the cold region. Following the owner's decision, the class has its own
// translation unit, which keeps that order (the comparators, used by -objectForKey: and
// -objectForKeyedSubscript:, are declared before the class). The const_mu of
// -countByEnumeratingWithState:objects:count: lies at 0x1c03771b8 (__TEXT,__const, value 1, read with
// Local/Scratch/cf-core/macho.py). Objective-C compiled with manual reference counting (objc_autorelease).
// Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json. Unless a definition says otherwise:
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in
// full). iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md
// (not written yet).

// The class symbols this unit sends to directly: NSArray (+arrayWithObjects:count:), NSMutableSet (+set),
// NSSet (+setWithSet:), NSDictionary (+new) and __NSConstantDictionaryEnumerator (objc_alloc).
extern char constantArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSArray));
extern char constantMutableSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableSet));
extern char constantSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSSet));
extern char constantEnumeratorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                OF_NAME_STRING(__ONSConstantDictionaryEnumerator));

// Selectors this unit sends that no included header declares, for the message sends only.
@protocol ConstantDictionaryMessages

- (id)initWithConstantDictionary:(id)dictionary enumerateKeys:(BOOL)enumerateKeys;
- (NSInteger)compare:(id)other;
- (NSInteger)compare:(id)string options:(NSUInteger)mask range:(OCFRange)range locale:(id)locale;
+ (id)arrayWithObjects:(const id *)objects count:(NSUInteger)count;
+ (id)set;
+ (id)setWithSet:(id)set;

@end

// The comparators, defined after the class in native address order.
static int comparisonUsingOrderingForStringKeys(const void *key, const void *element);

static int comparisonUsingOrderingForNumericKeys(const void *key, const void *element);

// The text of the method names in the exception reasons, the native __PRETTY_FUNCTION__ strings, spelled
// through the naming switch. constantDictionaryMethodName is a local descriptive name.
#define constantDictionaryMethodName(selector) "-[" OF_NAME_STRING(ONSConstantDictionary) " " selector "]"

@implementation ONSConstantDictionary

// Native -[NSConstantDictionary count], 0x1c01f8520. Non-external.
- (NSUInteger)count {
    return _count;
}

// Native -[NSConstantDictionary objectForKey:], 0x1c01f8530. Non-external. The keys, objects, count and
// options are read first. No entries answers nil; one entry whose key is the argument (pointer equality)
// answers its object. Option bit 1 (numeric keys) requires an NSNumber argument (_NSIsNSNumber, else nil);
// otherwise an NSString argument (_NSIsNSString) takes the string path, and any other argument answers nil
// when any option bit is set, or is found by the linear scan (pointer equality, then -isEqual: sent to the
// stored key). On the typed paths one entry is compared with -isEqualToString: or -isEqualToNumber: sent to
// the argument; more entries are found with bsearch over the keys (8-byte elements, the comparator for the
// key kind) when option bit 0 (sorted) is set, else by a linear scan (pointer equality, then the typed
// comparison sent to the stored key). The object at the found key's index answers.
- (id)objectForKey:(id)key {
    NSUInteger options = _options;
    NSUInteger count = _count;
    id *keys = _keys;
    id *objects = _objects;
    id searchKey = key;
    if (count == 0) {
        return nil;
    }
    if (count == 1 && key == keys[0]) {
        return objects[0];
    }
    int (*comparator)(const void *, const void *);
    if ((options & 2) != 0) {
        if (!_ONSIsNSNumber(key)) {
            return nil;
        }
        if (count == 1) {
            return [key isEqualToNumber:keys[0]] ? objects[0] : nil;
        }
        if ((options & 1) != 0) {
            comparator = comparisonUsingOrderingForNumericKeys;
            goto search;
        }
    } else {
        if (!_ONSIsNSString(key)) {
            if (options != 0) {
                return nil;
            }
            do {
                id candidate = *keys++;
                if (candidate == key || [candidate isEqual:key]) {
                    return *objects;
                }
                objects++;
            } while (--count != 0);
            return nil;
        }
        if (count == 1) {
            return [key isEqualToString:keys[0]] ? objects[0] : nil;
        }
        if ((options & 1) != 0) {
            comparator = comparisonUsingOrderingForStringKeys;
            goto search;
        }
    }
    do {
        id candidate = *keys++;
        if (candidate == key) {
            return *objects;
        }
        if ((options & 2) != 0) {
            if ([candidate isEqualToNumber:key]) {
                return *objects;
            }
        } else if ([candidate isEqualToString:key]) {
            return *objects;
        }
        objects++;
    } while (--count != 0);
    return nil;
search:;
    id *found = bsearch(&searchKey, keys, count, 8, comparator);
    if (found == NULL) {
        return nil;
    }
    return *(id *)((char *)objects + ((char *)found - (char *)keys));
}

// Native -[NSConstantDictionary keyEnumerator], 0x1c01f86d8. Non-external. A key enumerator over the
// receiver, autoreleased (a tail call).
- (id)keyEnumerator {
    return [[(id)[(Class)constantEnumeratorClass alloc] initWithConstantDictionary:self
                                                                     enumerateKeys:YES] autorelease];
}

// Native -[NSConstantDictionary getObjects:andKeys:count:], 0x1c01f870c. Non-external. A count whose top
// three bits are not clear raises "count (%lu) of objects array is ridiculous" (os_log pack buffer 0x16: the
// method name and the count) when the objects pointer, then when the keys pointer, is not NULL. The keys,
// then the objects (a tail call), are copied with memmove for the receiver's own count, not the argument's
// (native behavior kept: the callers pass the receiver's count).
- (void)getObjects:(id *)objects andKeys:(id *)keys count:(NSUInteger)count {
    if (objects != NULL && (OF_WORD_SHIFT(count, 61)) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            constantDictionaryMethodName("getObjects:andKeys:count:"),
            count
        );
    }
    if (keys != NULL && (OF_WORD_SHIFT(count, 61)) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            constantDictionaryMethodName("getObjects:andKeys:count:"),
            count
        );
    }
    size_t length = _count * 8;
    if (keys != NULL) {
        memmove(keys, _keys, length);
    }
    if (objects != NULL) {
        memmove(objects, _objects, length);
    }
}

// Native -[NSConstantDictionary __apply:context:], 0x1c01f88f4. Non-external. A NULL function raises
// NSInvalidArgumentException "%@: function pointer is NULL" (kCFAllocatorSystemDefault, the
// __CFExceptionProem of the receiver and selector, no os_log pack); otherwise the function is called with
// each key, object and the context, the count reread after each call.
- (void)__apply:(void (*)(const void *key, const void *value, void *context))applier context:(void *)context {
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
    for (NSUInteger index = 0; index < _count; index++) {
        applier(_keys[index], _objects[index], context);
    }
}

// Native -[NSConstantDictionary allKeys], 0x1c01f89e4. Non-external. +[NSArray arrayWithObjects:count:] over
// the keys (a tail call).
- (id)allKeys {
    return [(id)constantArrayClass arrayWithObjects:_keys count:_count];
}

// Native -[NSConstantDictionary allValues], 0x1c01f8a0c. Non-external. +[NSArray arrayWithObjects:count:]
// over the objects (a tail call).
- (id)allValues {
    return [(id)constantArrayClass arrayWithObjects:_objects count:_count];
}

// Native -[NSConstantDictionary countByEnumeratingWithState:objects:count:], 0x1c01f8a34. Non-external. A
// NULL buffer with a nonzero length raises "pointer to objects array is NULL but length is %lu", then a
// length whose top three bits are not clear raises "count (%lu) of objects array is ridiculous" (os_log pack
// buffer 0x16 each: the method name and the length). A first call (state 0) stores const_mu as the mutations
// pointer. While the state is below the count, the buffer becomes the items pointer and the non-nil keys from
// the state index on are copied into it, the state advancing past each key, until the count or the length is
// reached.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    // Native countByEnumeratingWithState:objects:count:.const_mu, 0x1c03771b8 (__TEXT,__const).
    static const unsigned long const_mu = 1;
    DictionaryEnumerationState *enumerationState = state;
    if (buffer == NULL && length != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            constantDictionaryMethodName("countByEnumeratingWithState:objects:count:"),
            length
        );
    }
    if ((OF_WORD_SHIFT(length, 61)) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            constantDictionaryMethodName("countByEnumeratingWithState:objects:count:"),
            length
        );
    }
    NSUInteger count = _count;
    unsigned long index = enumerationState->state;
    if (index == 0) {
        enumerationState->mutationsPtr = (unsigned long *)&const_mu;
    }
    if (count <= index) {
        return 0;
    }
    enumerationState->itemsPtr = (void **)buffer;
    if (length == 0) {
        return 0;
    }
    NSUInteger produced = 0;
    do {
        id key = _keys[index];
        index++;
        enumerationState->state = index;
        if (key != nil) {
            buffer[produced] = key;
            produced++;
            index = enumerationState->state;
        }
        if (index >= count) {
            break;
        }
    } while (produced < length);
    return produced;
}

// Native -[NSConstantDictionary enumerateKeysAndObjectsWithOptions:usingBlock:], 0x1c01f8c64. Non-external. A
// nil block raises "block cannot be nil" (os_log pack buffer 0xC: the method name). The options are not read.
// Each entry with a non-nil key is passed, inside its own autorelease pool, through
// __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__ with the shared stop flag, which ends the enumeration once set.
- (void)enumerateKeysAndObjectsWithOptions:(NSUInteger)options usingBlock:(void (^)(id, id, BOOL *))block {
    if (block == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            constantDictionaryMethodName("enumerateKeysAndObjectsWithOptions:usingBlock:")
        );
    }
    NSUInteger count = _count;
    BOOL stop = NO;
    for (NSUInteger index = 0; index < count; index++) {
        id key = _keys[index];
        if (key == nil) {
            continue;
        }
        void *pool = _OCFAutoreleasePoolPush();
        __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(block, key, _objects[index], &stop);
        _OCFAutoreleasePoolPop(pool);
        if (stop) {
            break;
        }
    }
}

// Native -[NSConstantDictionary keyOfEntryWithOptions:passingTest:], 0x1c01f8e08. Non-external. No nil check
// and the options are not read. Each entry with a non-nil key is tested inside its own autorelease pool (the
// predicate called directly); the first key passing sets the stop flag and answers; a predicate setting the
// stop flag itself ends the search with nil.
- (id)keyOfEntryWithOptions:(NSUInteger)options passingTest:(BOOL (^)(id, id, BOOL *))predicate {
    NSUInteger count = _count;
    BOOL stop = NO;
    id result = nil;
    for (NSUInteger index = 0; index < count; index++) {
        id key = _keys[index];
        if (key == nil) {
            continue;
        }
        id object = _objects[index];
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
    return result;
}

// Native -[NSConstantDictionary keysOfEntriesWithOptions:passingTest:], 0x1c01f8f14. Non-external. A nil
// predicate raises "predicate cannot be nil" (os_log pack buffer 0xC: the method name). The options are not
// read. The keys passing the predicate (each test inside its own autorelease pool, the stop flag ending the
// scan) are added to +[NSMutableSet set]; the result is +[NSSet setWithSet:] of it.
- (id)keysOfEntriesWithOptions:(NSUInteger)options passingTest:(BOOL (^)(id, id, BOOL *))predicate {
    if (predicate == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: predicate cannot be nil",
            constantDictionaryMethodName("keysOfEntriesWithOptions:passingTest:")
        );
    }
    NSUInteger count = _count;
    id keys = [(id)constantMutableSetClass set];
    BOOL stop = NO;
    for (NSUInteger index = 0; index < count; index++) {
        id key = _keys[index];
        if (key == nil) {
            continue;
        }
        id object = _objects[index];
        void *pool = _OCFAutoreleasePoolPush();
        if (predicate(key, object, &stop)) {
            [keys addObject:key];
        }
        _OCFAutoreleasePoolPop(pool);
        if (stop) {
            break;
        }
    }
    return [(id)constantSetClass setWithSet:keys];
}

// Native -[NSConstantDictionary objectForKeyedSubscript:], 0x1c01f90f8. Non-external. The body of
// -objectForKey:, instruction for instruction.
- (id)objectForKeyedSubscript:(id)key {
    NSUInteger options = _options;
    NSUInteger count = _count;
    id *keys = _keys;
    id *objects = _objects;
    id searchKey = key;
    if (count == 0) {
        return nil;
    }
    if (count == 1 && key == keys[0]) {
        return objects[0];
    }
    int (*comparator)(const void *, const void *);
    if ((options & 2) != 0) {
        if (!_ONSIsNSNumber(key)) {
            return nil;
        }
        if (count == 1) {
            return [key isEqualToNumber:keys[0]] ? objects[0] : nil;
        }
        if ((options & 1) != 0) {
            comparator = comparisonUsingOrderingForNumericKeys;
            goto search;
        }
    } else {
        if (!_ONSIsNSString(key)) {
            if (options != 0) {
                return nil;
            }
            do {
                id candidate = *keys++;
                if (candidate == key || [candidate isEqual:key]) {
                    return *objects;
                }
                objects++;
            } while (--count != 0);
            return nil;
        }
        if (count == 1) {
            return [key isEqualToString:keys[0]] ? objects[0] : nil;
        }
        if ((options & 1) != 0) {
            comparator = comparisonUsingOrderingForStringKeys;
            goto search;
        }
    }
    do {
        id candidate = *keys++;
        if (candidate == key) {
            return *objects;
        }
        if ((options & 2) != 0) {
            if ([candidate isEqualToNumber:key]) {
                return *objects;
            }
        } else if ([candidate isEqualToString:key]) {
            return *objects;
        }
        objects++;
    } while (--count != 0);
    return nil;
search:;
    id *found = bsearch(&searchKey, keys, count, 8, comparator);
    if (found == NULL) {
        return nil;
    }
    return *(id *)((char *)objects + ((char *)found - (char *)keys));
}

// Native -[NSConstantDictionary objectEnumerator], 0x1c01f92a0. Non-external. An object enumerator over the
// receiver, autoreleased (a tail call).
- (id)objectEnumerator {
    return [[(id)[(Class)constantEnumeratorClass alloc] initWithConstantDictionary:self
                                                                     enumerateKeys:NO] autorelease];
}

// Native +[NSConstantDictionary new], 0x1c01f92d4. Non-external. +[NSDictionary new] (objc_opt_new, a tail
// call).
+ (instancetype)new {
    return (id)[ONSDictionary new];
}

// Native +[NSConstantDictionary allocWithZone:], 0x1c01f92e0. Non-external. The immutable placeholder (a tail
// call); the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSDictionaryImmutablePlaceholder();
}

// Native -[NSConstantDictionary mutableCopy], 0x1c01f92e4. Non-external. __NSDictionaryM_new over the keys,
// objects and count with option 2 (a tail call).
- (id)mutableCopy {
    return (id)__NSDictionaryM_new((const void **)_keys, (const void **)_objects, (OCFIndex)_count, 2);
}

// Native -[NSConstantDictionary mutableCopyWithZone:], 0x1c01f9314. Non-external. The body of -mutableCopy;
// the zone is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return (id)__NSDictionaryM_new((const void **)_keys, (const void **)_objects, (OCFIndex)_count, 2);
}

// Native -[NSConstantDictionary autorelease], 0x1c01f9344. Non-external. The receiver.
- (instancetype)autorelease {
    return self;
}

// Native -[NSConstantDictionary retain], 0x1c01f9348. Non-external. The receiver.
- (instancetype)retain {
    return self;
}

// Native -[NSConstantDictionary _tryRetain], 0x1c01f934c. Non-external.
- (BOOL)_tryRetain {
    return YES;
}

// Native -[NSConstantDictionary _isDeallocating], 0x1c01f9354. Non-external.
- (BOOL)_isDeallocating {
    return NO;
}

// Native -[NSConstantDictionary retainCount], 0x1c01f935c. Non-external.
- (NSUInteger)retainCount {
    return UINTPTR_MAX;
}

// Native -[NSConstantDictionary release], 0x1c01f9364. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[NSConstantDictionary copyWithZone:], 0x1c01f9368. Non-external. The receiver, not retained.
- (id)copyWithZone:(struct _NSZone *)zone {
    return self;
}

// Native -[NSConstantDictionary copy], 0x1c01f936c. Non-external. The receiver, not retained.
- (id)copy {
    return self;
}

// Native +[NSConstantDictionary automaticallyNotifiesObserversForKey:], 0x1c01f9370. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[NSConstantDictionary initWithObjects:forKeys:count:], 0x1c030c390 (cold region). Non-external.
// Stores "attempting to allocate a constant object" as the crash message and traps.
- (id)initWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count {
    crashWithMessage("attempting to allocate a constant object");
}

// Native -[NSConstantDictionary init], 0x1c030c3a4 (cold region). Non-external. A bare BRK #1.
- (id)init {
    __builtin_trap();
}

// Native -[NSConstantDictionary dealloc], 0x1c030c3a8 (cold region). Non-external. A bare BRK #1; super is
// never reached, so the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
    __builtin_trap();
}

#pragma clang diagnostic pop

@end

// Native comparisonUsingOrderingForStringKeys, 0x1c01f9378. Non-external. The bsearch comparator of the
// sorted string keys: equal pointers compare 0; otherwise -compare:options:range:locale: sent to the searched
// key with the stored key, NSLiteralSearch (2), the searched key's whole range and no locale.
static int comparisonUsingOrderingForStringKeys(const void *key, const void *element) {
    id searchKey = *(id const *)key;
    id storedKey = *(id const *)element;
    if (searchKey == storedKey) {
        return 0;
    }
    NSUInteger length = [searchKey length];
    return (int)[searchKey compare:storedKey options:2 range:(OCFRange){0, (OCFIndex)length} locale:nil];
}

// Native comparisonUsingOrderingForNumericKeys, 0x1c01f93cc. Non-external. The bsearch comparator of the
// sorted number keys: equal pointers compare 0; otherwise -compare: sent to the searched key (a tail call).
static int comparisonUsingOrderingForNumericKeys(const void *key, const void *element) {
    id searchKey = *(id const *)key;
    id storedKey = *(id const *)element;
    if (searchKey == storedKey) {
        return 0;
    }
    return (int)[searchKey compare:storedKey];
}
