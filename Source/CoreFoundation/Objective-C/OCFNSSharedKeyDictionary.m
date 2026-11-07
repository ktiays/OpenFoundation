//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSSharedKeyDictionary.h"
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Objective-C/OCFNSDictionaryConcurrentEnumeration.h"
#include "CoreFoundation/Objective-C/OCFNSDictionaryEnumerate.h"
#include "CoreFoundation/Error/OCFError.h"

#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <stdatomic.h>
#include <stdlib.h>

// Inferred organization: the NSSharedKeyDictionary methods (0x1c02d606c-0x1c02d879f) lie together in the iOS
// Simulator __text section, before the NSSharedKeySet unit, with the unit's C functions among them (the
// -enumerateKeysAndObjectsWithOptions:usingBlock: block at 0x1c02d6ba0; ___archiveIsCorrupt,
// _cleanup_sentinel_pair and the -initWithCoder: block at 0x1c02d85ac-0x1c02d871b after -initWithCoder:); the
// class has its own translation unit, in native order. The unit's statics: the -initWithCoder: once token and
// property-list class set (__DATA,__bss, listed as _initWithCoder:.onceToken and .oPlistClasses; one of the
// two pairs at 0x22e6d3bd8-0x22e6d3bf0). Objective-C compiled with manual reference counting (objc_retain,
// objc_release, objc_autorelease). Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.
// Unless a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified (assembly read in full). iOS Device 24A446: unread. The selector the unit passes to
// the key set's -indexForKey: implementation and to -methodForSelector: (0x2387af30d) lies outside the
// extracted image; it is -indexForKey: by the key set's methods and the callers' use of the answer
// (confidence: medium). Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not
// written yet).

// Class symbols sent to directly (class references off_2227CE2A0, off_2227CE2F8, off_2227CE378 and
// off_2227CE3B8); NSException is dictionaryExceptionClass (Objective-C/OCFNSDictionary.h).
extern char sharedKeyDictionaryArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSArray));
extern char sharedKeyDictionaryDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                           OF_NAME_STRING(ONSDictionary));
extern char sharedKeyDictionaryMutableDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                                  OF_NAME_STRING(ONSMutableDictionary));
extern char sharedKeyDictionarySetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSSet));
extern char sharedKeyDictionaryMutableArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                             OF_NAME_STRING(ONSMutableArray));

// Classes looked up by name in -initWithCoder:, so that the prefix tool maps them.
@class ONSData;

// Selectors this unit sends that no included header declares (the coders', the collections'), declared for
// the message sends only; the protocol is never adopted and no protocol object is emitted.
@protocol SharedKeyDictionaryMessages

- (id)_initByAdoptingBuffer:(id *)buffer count:(NSUInteger)count size:(NSUInteger)size;
- (void)enumerateKeysAndObjectsWithOptions:(NSUInteger)options
                                usingBlock:(void (^)(id key, id object, BOOL *stop))block;
- (BOOL)allowsKeyedCoding;
- (void)encodeInt64:(int64_t)value forKey:(id)key;
- (void)encodeObject:(id)object forKey:(id)key;
- (int64_t)decodeInt64ForKey:(id)key;
- (id)decodeObjectOfClass:(Class)aClass forKey:(id)key;
- (id)decodeObjectOfClasses:(id)classes forKey:(id)key;
- (NSInteger)decodingFailurePolicy;
- (void)failWithError:(id)error;
- (id)allowedClasses;
- (void)replaceObject:(id)object withObject:(id)newObject;
- (id)setByAddingObjectsFromSet:(id)set;
- (id)setByAddingObject:(id)object;
- (id)setWithObjects:(id)firstObject, ...;
- (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
- (id)array;

@end

// The superclass method -setObservationInfo: sends to super (Foundation's NSKeyValueObserving, outside this
// restoration); declared only.
@interface ONSMutableDictionary (SharedKeyDictionarySuperclassMessages)

- (void)setObservationInfo:(void *)observationInfo;

@end

// The record -initWithCoder: keeps per placeholder for _cleanup_sentinel_pair (24 bytes on the stack: the
// unarchiver, the decoded object and the placeholder that stands for it in the unarchiver). SentinelPair and
// its field names are local descriptive names.
typedef struct {
    id coder;
    id object;
    id placeholder;
} SentinelPair;

static id __archiveIsCorrupt(id reason);

static void cleanup_sentinel_pair(SentinelPair *pair);

// The cleanup of -countByEnumeratingWithState:objects:count:'s key buffer, freed when an exception unwinds
// before the array adopts it. freeKeyStorage is a local descriptive name.
static inline void freeKeyStorage(id **storage) {
    free(*storage);
}

// The failure every -initWithCoder: check after the first inlines: the reason autoreleased through
// _CFAutoreleasePoolAddObject, the receiver released when the coder's -decodingFailurePolicy is 1 (set
// error), then -failWithError: of the ___archiveIsCorrupt error. failDecoding is a local descriptive name.
static inline void failDecoding(id object, id coder, OCFStringRef reason) {
    id failureReason = _OCFAutoreleasePoolAddObject(NULL, reason);
    if ([coder decodingFailurePolicy] == 1) {
        [object release];
    }
    [coder failWithError:__archiveIsCorrupt(failureReason)];
}

@implementation ONSSharedKeyDictionary

// Native -[NSSharedKeyDictionary count], 0x1c02d606c. Non-external. _count plus the side dictionary's -count
// (none when it is nil).
- (NSUInteger)count {
    NSUInteger count = _count;
    NSUInteger sideCount = 0;
    if (_sideDic != nil) {
        sideCount = [_sideDic count];
    }
    return sideCount + count;
}

// Native -[NSSharedKeyDictionary objectForKey:], 0x1c02d60a8. Non-external. A non-nil key with a key set is
// looked up through _ifkIMP (-indexForKey:); an index other than NSNotFound answers its value. Otherwise the
// side dictionary's -objectForKey: (a tail call).
- (id)objectForKey:(id)key {
    if (key != nil && _keyMap != nil) {
        NSUInteger index = _ifkIMP(_keyMap, @selector(indexForKey:), key);
        if (index != LONG_MAX) {
            return _values[index];
        }
    }
    return [_sideDic objectForKey:key];
}

// Native -[NSSharedKeyDictionary getObjects:andKeys:count:], 0x1c02d6130. Non-external. A count with any of
// its top three bits set raises "count (%lu) of objects array is ridiculous" when objects, then when keys, is
// not NULL (os_log pack buffer 0x16: the method name and the count). The side dictionary fills the first
// min(count, its -count) entries (-getObjects:andKeys:count:, sent even to nil); the rest come from the
// key-set values, from the last index down, each non-nil value with its key (-keyAtIndex:), until the count
// is reached.
- (void)getObjects:(id *)objects andKeys:(id *)keys count:(NSUInteger)count {
    if (objects != NULL && (OF_WORD_SHIFT(count, 61)) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(ONSSharedKeyDictionary) " getObjects:andKeys:count:]",
            count
        );
    }
    if (keys != NULL && (OF_WORD_SHIFT(count, 61)) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(ONSSharedKeyDictionary) " getObjects:andKeys:count:]",
            count
        );
    }
    NSUInteger sideCount = 0;
    if (_sideDic != nil) {
        sideCount = [_sideDic count];
    }
    NSUInteger fromSide = count < sideCount ? count : sideCount;
    [_sideDic getObjects:objects andKeys:keys count:fromSide];
    NSUInteger remaining = count - fromSide;
    if (remaining == 0) {
        return;
    }
    NSUInteger keySetCount = [_keyMap count];
    if (keySetCount == 0) {
        return;
    }
    id *keysOut = keys != NULL ? keys + fromSide : NULL;
    id *objectsOut = objects != NULL ? objects + fromSide : NULL;
    NSUInteger index = keySetCount - 1;
    do {
        if (_values[index] != nil) {
            if (keysOut != NULL) {
                *keysOut++ = [_keyMap keyAtIndex:index];
            }
            if (objectsOut != NULL) {
                *objectsOut++ = _values[index];
            }
            if (--remaining == 0) {
                return;
            }
        }
        index -= 1;
    } while (index != (NSUInteger)-1);
}

// Native -[NSSharedKeyDictionary keyEnumerator], 0x1c02d6378. Non-external. The -count (_count first, then
// the side dictionary's) keys (the new_id_array overflow raise; always _CFCreateArrayStorage of at least one
// entry) from -getObjects:andKeys:count:; a buffer has its keys retained and is adopted by [[NSArray alloc]
// _initByAdoptingBuffer:count:size:] (NULL: -initWithObjects:NULL count:); the answer is the array's
// -objectEnumerator, the array released.
- (id)keyEnumerator {
    NSUInteger count = _count;
    NSUInteger sideCount = 0;
    if (_sideDic != nil) {
        sideCount = [_sideDic count];
    }
    count = sideCount + count;
    if ((OF_WORD_SHIFT(count, 60)) != 0) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            NULL,
            NULL,
            OF_STR(
                "*** attempt to create a temporary id buffer which is too large or with a negative count "
                "(%lu) -- "
                "possibly data is corrupt"
            ),
            count
        );
        id exception = [(id)dictionaryExceptionClass exceptionWithName:(id)ONSGenericException
                                                                reason:(id)reason
                                                              userInfo:nil];
        OCFRelease(reason);
        objc_exception_throw(exception);
    }
    size_t storageSize = 0;
    id *keys = (id *)_OCFCreateArrayStorage(count > 1 ? count : 1, false, &storageSize);
    [self getObjects:NULL andKeys:keys count:count];
    id array;
    if (keys != NULL) {
        for (NSUInteger index = 0; index < count; index++) {
            [keys[index] retain];
        }
        array = [[(Class)sharedKeyDictionaryArrayClass alloc] _initByAdoptingBuffer:keys
                                                                              count:count
                                                                               size:count];
    } else {
        array = [[(Class)sharedKeyDictionaryArrayClass alloc] initWithObjects:NULL count:count];
    }
    id enumerator = [array objectEnumerator];
    [array release];
    return enumerator;
}

// Native -[NSSharedKeyDictionary countByEnumeratingWithState:objects:count:], 0x1c02d6518. Non-external. A
// NULL buffer with a nonzero length raises first, then a length with any of its top three bits set (os_log
// pack buffer 0x16 each: the method name and the length). The first call (state 0) points mutationsPtr at
// _mutations and stores the -count (_count plus the side dictionary's) in extra[0]; a zero count stores state
// NSUIntegerMax and answers 0. Otherwise the keys (the new_id_array overflow raise; _CFCreateArrayStorage,
// freed if an exception unwinds) from -getObjects:andKeys:count: become an autoreleased array (retained and
// adopted, or -initWithObjects:NULL count:) kept in extra[1]. Each call copies min(length, extra[0] - state)
// keys of that array (-getObjects:range:) into the buffer, points itemsPtr at it and advances the state; none
// left answers 0 without a store.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    DictionaryEnumerationState *enumerationState = state;
    if (buffer == NULL && length != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(ONSSharedKeyDictionary) " countByEnumeratingWithState:objects:count:]",
            length
        );
    }
    if ((OF_WORD_SHIFT(length, 61)) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(ONSSharedKeyDictionary) " countByEnumeratingWithState:objects:count:]",
            length
        );
    }
    unsigned long position = enumerationState->state;
    if (position == 0) {
        enumerationState->mutationsPtr = &_mutations;
        NSUInteger total = _count;
        NSUInteger sideCount = 0;
        if (_sideDic != nil) {
            sideCount = [_sideDic count];
            position = enumerationState->state;
        }
        total = sideCount + total;
        enumerationState->extra[0] = total;
        if (total <= position) {
            enumerationState->state = (unsigned long)-1;
            return 0;
        }
        __attribute__((cleanup(freeKeyStorage))) id *keys = NULL;
        if ((OF_WORD_SHIFT(total, 60)) != 0) {
            OCFStringRef reason = OCFStringCreateWithFormat(
                NULL,
                NULL,
                OF_STR(
                    "*** attempt to create a temporary id buffer which is too large or with a negative count "
                    "(%lu) -- possibly data is corrupt"
                ),
                total
            );
            id exception = [(id)dictionaryExceptionClass exceptionWithName:(id)ONSGenericException
                                                                    reason:(id)reason
                                                                  userInfo:nil];
            OCFRelease(reason);
            objc_exception_throw(exception);
        }
        size_t storageSize = 0;
        keys = (id *)_OCFCreateArrayStorage(total, false, &storageSize);
        [self getObjects:NULL andKeys:keys count:total];
        id array;
        if (keys != NULL) {
            for (NSUInteger index = 0; index != total; index++) {
                [keys[index] retain];
            }
            array = [[(Class)sharedKeyDictionaryArrayClass alloc] _initByAdoptingBuffer:keys
                                                                                  count:total
                                                                                   size:total];
        } else {
            array = [[(Class)sharedKeyDictionaryArrayClass alloc] initWithObjects:NULL count:total];
        }
        keys = NULL;
        enumerationState->extra[1] = (unsigned long)[array autorelease];
        position = enumerationState->state;
    }
    if (enumerationState->extra[0] <= position) {
        return 0;
    }
    NSUInteger left = enumerationState->extra[0] - position;
    NSUInteger copied = length < left ? length : left;
    [(id)enumerationState->extra[1] getObjects:buffer range:(OCFRange){(OCFIndex)position, (OCFIndex)copied}];
    enumerationState->itemsPtr = (void **)buffer;
    enumerationState->state = enumerationState->state + copied;
    return copied;
}

// Native -[NSSharedKeyDictionary enumerateKeysAndObjectsWithOptions:usingBlock:], 0x1c02d68f4, with its block
// ___71-[NSSharedKeyDictionary enumerateKeysAndObjectsWithOptions:usingBlock:]_block_invoke (0x1c02d6ba0).
// Non-external. A nil block raises "*** %s: block cannot be nil" (os_log pack buffer 0xC). The key-set slots
// (-count of the key set) go first: concurrently through __NSCollectionHandleConcurrentEnumerationIfSpecified
// (no pool there; the block skips nil values, calls out in its own autorelease pool with the key from
// -keyAtIndex:, and on a stop sets the shared flag with a release store and the slot's stop), after which a
// set shared flag (load-acquire) ends the enumeration; otherwise in order, each non-nil value in its own
// autorelease pool, a stop ending the enumeration. Unless stopped, the side dictionary's
// -enumerateKeysAndObjectsWithOptions:usingBlock: follows with the same options and block.
- (void)enumerateKeysAndObjectsWithOptions:(NSUInteger)options
                                usingBlock:(void (^)(id key, id object, BOOL *stop))block {
    if (block == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            "-[" OF_NAME_STRING(ONSSharedKeyDictionary) " enumerateKeysAndObjectsWithOptions:usingBlock:]"
        );
    }
    NSUInteger keyCount = [_keyMap count];
    __block _Atomic(BOOL) sharedStop = NO;
    BOOL handled = __NSCollectionHandleConcurrentEnumerationIfSpecified(
        options, NO, keyCount, ^(size_t index, BOOL *stop) {
          id value = self->_values[index];
          if (value == nil) {
              return;
          }
          BOOL slotStop = NO;
          void *pool = _OCFAutoreleasePoolPush();
          __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(
              block, [self->_keyMap keyAtIndex:index], value, &slotStop
          );
          _OCFAutoreleasePoolPop(pool);
          if (slotStop == YES) {
              atomic_store_explicit(&sharedStop, YES, memory_order_release);
              *stop = YES;
          }
        }
    );
    if (handled) {
        if (!(atomic_load_explicit(&sharedStop, memory_order_acquire) & 1)) {
            [_sideDic enumerateKeysAndObjectsWithOptions:options usingBlock:block];
        }
        return;
    }
    BOOL stop = NO;
    if (keyCount != 0) {
        for (NSUInteger index = 0;; index++) {
            id value = _values[index];
            if (value != nil) {
                void *pool = _OCFAutoreleasePoolPush();
                __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(block, [_keyMap keyAtIndex:index], value, &stop);
                _OCFAutoreleasePoolPop(pool);
                if (stop) {
                    break;
                }
            }
            if (index + 1 >= keyCount) {
                break;
            }
        }
        if (stop) {
            return;
        }
    }
    [_sideDic enumerateKeysAndObjectsWithOptions:options usingBlock:block];
}

// Native -[NSSharedKeyDictionary setObject:forKey:], 0x1c02d6c84. Non-external. _mutations grows first. A nil
// key raises "*** %s: key cannot be nil" (os_log pack buffer 0xC), a nil object "*** %s: object cannot be nil
// (key: %@)" (0x16). The key's index comes from _ifkIMP (NSNotFound without a key set). With _doKVO (exactly
// 1) -willChangeValueForKey: brackets the store before and -didChangeValueForKey: after (a tail call). A key
// outside the key set goes to the side dictionary (made with +[NSMutableDictionary new] on first use). In the
// key set a different object is retained (unless a tagged pointer); it replaces an old value, which is then
// released (unless a tagged pointer), or fills an empty slot and _count grows.
- (void)setObject:(id)object forKey:(id)key {
    _mutations += 1;
    if (key == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: key cannot be nil",
            "-[" OF_NAME_STRING(ONSSharedKeyDictionary) " setObject:forKey:]"
        );
    }
    if (object == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: object cannot be nil (key: %@)",
            "-[" OF_NAME_STRING(ONSSharedKeyDictionary) " setObject:forKey:]",
            key
        );
    }
    NSUInteger index;
    if (_keyMap != nil) {
        index = _ifkIMP(_keyMap, @selector(indexForKey:), key);
    } else {
        index = LONG_MAX;
    }
    if (_doKVO == 1) {
        [(id)self willChangeValueForKey:key];
    }
    if (index == LONG_MAX) {
        if (_sideDic == nil) {
            _sideDic = [(Class)sharedKeyDictionaryMutableDictionaryClass new];
        }
        [_sideDic setObject:object forKey:key];
    } else {
        id oldObject = _values[index];
        if (oldObject != object) {
            if ((intptr_t)object >= 0) {
                [object retain];
            }
            if (oldObject != nil) {
                _values[index] = object;
                if ((intptr_t)oldObject >= 0) {
                    [oldObject release];
                }
            } else {
                _count += 1;
                _values[index] = object;
            }
        }
    }
    if (_doKVO == 1) {
        [(id)self didChangeValueForKey:key];
    }
}

// Native -[NSSharedKeyDictionary removeObjectForKey:], 0x1c02d6fcc. Non-external. _mutations grows first; a
// nil key raises "*** %s: key cannot be nil" (os_log pack buffer 0xC). The KVO bracket of -setObject:forKey:.
// A key outside the key set is removed from the side dictionary (when there is one); in the key set the slot
// is cleared and a value that was there decrements _count and is released (unless a tagged pointer).
- (void)removeObjectForKey:(id)key {
    _mutations += 1;
    if (key == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: key cannot be nil",
            "-[" OF_NAME_STRING(ONSSharedKeyDictionary) " removeObjectForKey:]"
        );
    }
    NSUInteger index;
    if (_keyMap != nil) {
        index = _ifkIMP(_keyMap, @selector(indexForKey:), key);
    } else {
        index = LONG_MAX;
    }
    if (_doKVO == 1) {
        [(id)self willChangeValueForKey:key];
    }
    if (index == LONG_MAX) {
        if (_sideDic != nil) {
            [_sideDic removeObjectForKey:key];
        }
    } else {
        id oldObject = _values[index];
        _values[index] = nil;
        if (oldObject != nil) {
            _count -= 1;
            if ((intptr_t)oldObject >= 0) {
                [oldObject release];
            }
        }
    }
    if (_doKVO == 1) {
        [(id)self didChangeValueForKey:key];
    }
}

// Native -[NSSharedKeyDictionary keySet], 0x1c02d71f0. Non-external. _keyMap.
- (id)keySet {
    return _keyMap;
}

// Native -[NSSharedKeyDictionary initWithKeySet:], 0x1c02d7200. Non-external. Does not send super an
// initializer. A nil key set answers the receiver as is. Otherwise the key set is retained (unless a tagged
// pointer) into _keyMap, _ifkIMP takes its -methodForSelector: of -indexForKey:, and _values is
// malloc_type_calloc(-count of the key set, 8, 0x80040B8603338); a failed allocation releases the receiver
// and throws NSMallocException "%@: unable to unarchive - memory failure" (the __CFExceptionProem of the
// receiver and selector, kCFAllocatorSystemDefault, autoreleased through _CFAutoreleasePoolAddObject).
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

- (id)initWithKeySet:(id)keySet {
    if (keySet == nil) {
        return self;
    }
    if ((intptr_t)keySet >= 0) {
        [keySet retain];
    }
    _keyMap = keySet;
    _ifkIMP = (NSUInteger (*)(id, SEL, id))[keySet methodForSelector:@selector(indexForKey:)];
    _values = (id *)malloc_type_calloc([_keyMap count], 8, 0x80040B8603338);
    if (_values == NULL) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("%@: unable to unarchive - memory failure"),
            __CFExceptionProem(self, _cmd)
        );
        id failureReason = _OCFAutoreleasePoolAddObject(NULL, reason);
        [self release];
        objc_exception_throw([(id)dictionaryExceptionClass exceptionWithName:(id)ONSMallocException
                                                                      reason:failureReason
                                                                    userInfo:nil]);
    }
    return self;
}

#pragma clang diagnostic pop

// Native +[NSSharedKeyDictionary sharedKeyDictionaryWithKeySet:], 0x1c02d7308. Non-external. [[self alloc]
// initWithKeySet:], autoreleased.
+ (id)sharedKeyDictionaryWithKeySet:(id)keySet {
    return [[[self alloc] initWithKeySet:keySet] autorelease];
}

// Native -[NSSharedKeyDictionary dealloc], 0x1c02d7330. Non-external. With values, the first -count (of the
// key set) slots are released (unless nil or tagged pointers) and the slots freed; then the key set and the
// side dictionary are released (unless nil or tagged pointers), then super.
- (void)dealloc {
    id *values = _values;
    if (values != NULL) {
        NSUInteger count = [_keyMap count];
        for (; count != 0; count--) {
            id value = *values++;
            if ((intptr_t)value >= 0) {
                [value release];
            }
        }
        free(_values);
    }
    if ((intptr_t)_keyMap >= 1) {
        [_keyMap release];
    }
    if ((intptr_t)_sideDic >= 1) {
        [_sideDic release];
    }
    [super dealloc];
}

// Native -[NSSharedKeyDictionary copyWithZone:], 0x1c02d7418. Non-external. [[[self class]
// allocWithZone:zone] initWithKeySet:_keyMap] takes _count, every slot from the last (retained when a
// positive pointer) and the side dictionary's -mutableCopyWithZone: (stored without a further retain).
- (id)copyWithZone:(struct _NSZone *)zone {
    ONSSharedKeyDictionary *copy = [[[self class] allocWithZone:zone] initWithKeySet:_keyMap];
    copy->_count = _count;
    NSUInteger count = [_keyMap count];
    if (count != 0) {
        NSUInteger index = count - 1;
        do {
            id value = _values[index];
            if ((intptr_t)value >= 1) {
                [value retain];
                value = _values[index];
            }
            copy->_values[index] = value;
            index -= 1;
        } while (index != (NSUInteger)-1);
    }
    copy->_sideDic = [_sideDic mutableCopyWithZone:zone];
    return copy;
}

// Native -[NSSharedKeyDictionary mutableCopyWithZone:], 0x1c02d74e0. Non-external. -copyWithZone: (a tail
// call).
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return [self copyWithZone:zone];
}

// Native -[NSSharedKeyDictionary classForCoder], 0x1c02d74e4. Non-external. +[NSSharedKeyDictionary class]
// (objc_opt_class of the class symbol, addressed directly).
- (Class)classForCoder {
    return [ONSSharedKeyDictionary class];
}

// Native -[NSSharedKeyDictionary encodeWithCoder:], 0x1c02d74f0. Non-external. A coder that does not allow
// keyed coding throws NSInvalidArgumentException "%@: this object can only be encoded by a keyed coder".
// NS.skkeyset and NS.sideDic (-encodeObject:forKey:), NS.count (-encodeInt64:forKey: of _count); with values
// also NS.keys and NS.values: two +[NSMutableArray array]s of the key set's keys (fast enumeration) whose
// -objectForKey: is not nil, and those objects.
- (void)encodeWithCoder:(id)coder {
    if (![coder allowsKeyedCoding]) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("%@: this object can only be encoded by a keyed coder"),
            __CFExceptionProem(self, _cmd)
        );
        objc_exception_throw([(id)dictionaryExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                     userInfo:nil]);
    }
    [coder encodeObject:_keyMap forKey:(id)OF_STR("NS.skkeyset")];
    [coder encodeObject:_sideDic forKey:(id)OF_STR("NS.sideDic")];
    [coder encodeInt64:(int64_t)_count forKey:(id)OF_STR("NS.count")];
    if (_count == 0) {
        return;
    }
    id keys = [(Class)sharedKeyDictionaryMutableArrayClass array];
    id values = [(Class)sharedKeyDictionaryMutableArrayClass array];
    for (id key in _keyMap) {
        id value = [self objectForKey:key];
        if (value != nil) {
            [keys addObject:key];
            [values addObject:value];
        }
    }
    [coder encodeObject:keys forKey:(id)OF_STR("NS.keys")];
    [coder encodeObject:values forKey:(id)OF_STR("NS.values")];
}

// Native -[NSSharedKeyDictionary initWithCoder:], 0x1c02d7720. Non-external. Decoding reads the key set, the
// side dictionary and the key-set entries. A coder that does not allow keyed coding fails at once ("%@: this
// object can only be decoded by a keyed coder": the receiver released, -failWithError: of the
// ___archiveIsCorrupt error, nil). Every later failure goes through the failDecoding sequence (the receiver
// released only under decoding failure policy 1) and answers nil after the two placeholder records are
// cleaned up
// (_cleanup_sentinel_pair, the side dictionary's record first). NS.skkeyset (-decodeObjectOfClass:
// NSSharedKeySet) is stored retained (when a positive pointer); a class other than NSSharedKeySet fails ("%@:
// invalid archive (unexpected keyset: %@)"). With a key set, _ifkIMP and _values (malloc_type_calloc(-count,
// 8, 0x80040B8603338); NULL: "%@: unable to unarchive - memory failure") are set up and, for an
// NSKeyedUnarchiver (objc_getClass), a placeholder [NSSharedKeySet new] (nil: the memory failure) stands for
// the key set in the unarchiver (-replaceObject:withObject:) until the cleanup. NS.sideDic is decoded with
// the coder's -allowedClasses, NSString, NSNumber, NSDate, NSURL and NSData (a set made once, dispatch_once)
// and NSMutableDictionary, and stored retained (when a positive pointer); one that is not an
// NSMutableDictionary ("%@: invalid archive (must be mutable)") or is an NSSharedKeyDictionary ("%@: invalid
// archive (invalid dictionary)") fails; for an unarchiver a placeholder +[NSMutableDictionary new] stands for
// it; a positive count above 0x1000000 fails ("%@: unreasonably sized collection") and an NSArray or
// NSDictionary key fails
// ("unexpected recursive keys (%@)"; the keys read into a temporary id buffer). NS.count: negative fails
// ("%@: negative count"), 0 answers the receiver, above 0x1000000 fails; without a key set "%@: invalid
// archive (encoded count but missing keyset)". NS.keys and NS.values are decoded with the class set plus
// NSArray, each kept only when _NSIsNSArray; an empty side ("%@: encoded keys/values are empty"), different
// counts ("%@: encoded keys/values length does not match") or a count other than NS.count ("%@: more entries
// than expected") fails. Both are read into temporary id buffers (stack up to 0x100 entries, otherwise two
// _CFCreateArrayStorage blocks freed at the end); recursive keys fail as above, a key the key set does not
// index fails ("%@: encoded key not present in keyset"), and each pair is stored with -setObject:forKey:.
// Defect kept: under any decoding failure policy other than 1 a failure answers nil without releasing the
// receiver (native leak).
- (id)initWithCoder:(id)coder {
    if (![coder allowsKeyedCoding]) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("%@: this object can only be decoded by a keyed coder"),
            __CFExceptionProem(self, _cmd)
        );
        id failureReason = _OCFAutoreleasePoolAddObject(NULL, reason);
        [self release];
        [coder failWithError:__archiveIsCorrupt(failureReason)];
        return nil;
    }
    Class keySetClass = [ONSSharedKeySet class];
    BOOL isUnarchiver = [coder isKindOfClass:objc_getClass(OF_NAME_STRING(ONSKeyedUnarchiver))];
    id unarchiver = isUnarchiver ? coder : nil;
    __attribute__((cleanup(cleanup_sentinel_pair))) SentinelPair keySetPair = {unarchiver, nil, nil};
    __attribute__((cleanup(cleanup_sentinel_pair))) SentinelPair sideDictionaryPair = {unarchiver, nil, nil};
    id keySet = [coder decodeObjectOfClass:keySetClass forKey:(id)OF_STR("NS.skkeyset")];
    keySetPair.object = keySet;
    _keyMap = keySet;
    if ((intptr_t)keySet >= 1) {
        [keySet retain];
    }
    Class decodedClass = [_keyMap class];
    if (decodedClass != keySetClass) {
        OCFStringRef proem = __CFExceptionProem(self, _cmd);
        failDecoding(
            self,
            coder,
            OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("%@: invalid archive (unexpected keyset: %@)"),
                proem,
                decodedClass
            )
        );
        return nil;
    }
    if (_keyMap != nil) {
        _ifkIMP = (NSUInteger (*)(id, SEL, id))[_keyMap methodForSelector:@selector(indexForKey:)];
        _values = (id *)malloc_type_calloc([_keyMap count], 8, 0x80040B8603338);
        if (_values == NULL) {
            failDecoding(
                self,
                coder,
                OCFStringCreateWithFormat(
                    kOCFAllocatorSystemDefault,
                    NULL,
                    OF_STR("%@: unable to unarchive - memory failure"),
                    __CFExceptionProem(self, _cmd)
                )
            );
            return nil;
        }
        if (isUnarchiver) {
            id placeholder = [ONSSharedKeySet new];
            keySetPair.placeholder = placeholder;
            if (placeholder == nil) {
                failDecoding(
                    self,
                    coder,
                    OCFStringCreateWithFormat(
                        kOCFAllocatorSystemDefault,
                        NULL,
                        OF_STR("%@: unable to unarchive - memory failure"),
                        __CFExceptionProem(self, _cmd)
                    )
                );
                return nil;
            }
            if (keySet != nil) {
                [coder replaceObject:keySet withObject:placeholder];
            }
        }
    }
    Class arrayClass = objc_lookUpClass(OF_NAME_STRING(ONSArray));
    Class dictionaryClass = objc_lookUpClass(OF_NAME_STRING(ONSDictionary));
    Class mutableDictionaryClass = objc_lookUpClass(OF_NAME_STRING(ONSMutableDictionary));
    Class stringClass = objc_lookUpClass(OF_NAME_STRING(ONSString));
    // Native _initWithCoder:.onceToken and _initWithCoder:.oPlistClasses, __DATA,__bss, non-external.
    static dispatch_once_t onceToken;
    static id oPlistClasses;
    dispatch_once(&onceToken, ^{
      // Native ___39-[NSSharedKeyDictionary initWithCoder:]_block_invoke, 0x1c02d8684. Non-external. The
      // captured NSString class, then NSNumber, NSDate, NSURL and NSData (objc_lookUpClass), as +[NSSet
      // setWithObjects:], retained after the store.
      oPlistClasses =
          [[(Class)sharedKeyDictionarySetClass setWithObjects:stringClass,
                                                              objc_lookUpClass(OF_NAME_STRING(ONSNumber)),
                                                              objc_lookUpClass(OF_NAME_STRING(ONSDate)),
                                                              objc_lookUpClass(OF_NAME_STRING(ONSURL)),
                                                              objc_lookUpClass(OF_NAME_STRING(ONSData)),
                                                              nil] retain];
    });
    id classes = [[coder allowedClasses] setByAddingObjectsFromSet:oPlistClasses];
    id sideDictionary = [coder decodeObjectOfClasses:[classes setByAddingObject:mutableDictionaryClass]
                                              forKey:(id)OF_STR("NS.sideDic")];
    sideDictionaryPair.object = sideDictionary;
    _sideDic = sideDictionary;
    if ((intptr_t)sideDictionary >= 1) {
        [sideDictionary retain];
    }
    if (_sideDic != nil) {
        if (![_sideDic isKindOfClass:mutableDictionaryClass]) {
            failDecoding(
                self,
                coder,
                OCFStringCreateWithFormat(
                    kOCFAllocatorSystemDefault,
                    NULL,
                    OF_STR("%@: invalid archive (must be mutable)"),
                    __CFExceptionProem(self, _cmd)
                )
            );
            return nil;
        }
        if ([_sideDic isKindOfClass:[ONSSharedKeyDictionary class]]) {
            failDecoding(
                self,
                coder,
                OCFStringCreateWithFormat(
                    kOCFAllocatorSystemDefault,
                    NULL,
                    OF_STR("%@: invalid archive (invalid dictionary)"),
                    __CFExceptionProem(self, _cmd)
                )
            );
            return nil;
        }
        if (isUnarchiver) {
            id placeholder = [(Class)sharedKeyDictionaryMutableDictionaryClass new];
            sideDictionaryPair.placeholder = placeholder;
            if (placeholder == nil) {
                failDecoding(
                    self,
                    coder,
                    OCFStringCreateWithFormat(
                        kOCFAllocatorSystemDefault,
                        NULL,
                        OF_STR("%@: unable to unarchive - memory failure"),
                        __CFExceptionProem(self, _cmd)
                    )
                );
                return nil;
            }
            if (sideDictionary != nil) {
                [coder replaceObject:sideDictionary withObject:placeholder];
            }
        }
        NSInteger sideCount = (NSInteger)[_sideDic count];
        if (sideCount >= 1) {
            if (sideCount >= 0x1000001) {
                failDecoding(
                    self,
                    coder,
                    OCFStringCreateWithFormat(
                        kOCFAllocatorSystemDefault,
                        NULL,
                        OF_STR("%@: unreasonably sized collection"),
                        __CFExceptionProem(self, _cmd)
                    )
                );
                return nil;
            }
            id stackKeys[sideCount < 0x101 ? sideCount : 1];
            size_t storageSize = 0;
            id *heapKeys = NULL;
            if (sideCount >= 0x101) {
                heapKeys = (id *)_OCFCreateArrayStorage(sideCount, false, &storageSize);
            }
            id *sideKeys = sideCount < 0x101 ? stackKeys : heapKeys;
            [_sideDic getObjects:NULL andKeys:sideKeys count:sideCount];
            for (NSInteger remaining = sideCount; remaining != 0; remaining--) {
                id key = *sideKeys++;
                if ([key isKindOfClass:arrayClass] || [key isKindOfClass:dictionaryClass]) {
                    failDecoding(
                        self,
                        coder,
                        OCFStringCreateWithFormat(
                            kOCFAllocatorSystemDefault,
                            NULL,
                            OF_STR("unexpected recursive keys (%@)"),
                            [key class]
                        )
                    );
                    free(heapKeys);
                    return nil;
                }
            }
            free(heapKeys);
        }
    }
    int64_t count = [coder decodeInt64ForKey:(id)OF_STR("NS.count")];
    if (count < 0) {
        failDecoding(
            self,
            coder,
            OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault, NULL, OF_STR("%@: negative count"), __CFExceptionProem(self, _cmd)
            )
        );
        return nil;
    }
    if (count == 0) {
        return self;
    }
    if (count >= 0x1000001) {
        failDecoding(
            self,
            coder,
            OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("%@: unreasonably sized collection"),
                __CFExceptionProem(self, _cmd)
            )
        );
        return nil;
    }
    if (_keyMap == nil) {
        failDecoding(
            self,
            coder,
            OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("%@: invalid archive (encoded count but missing keyset)"),
                __CFExceptionProem(self, _cmd)
            )
        );
        return nil;
    }
    id entryClasses = [classes setByAddingObject:arrayClass];
    id keys = [coder decodeObjectOfClasses:entryClasses forKey:(id)OF_STR("NS.keys")];
    id values = [coder decodeObjectOfClasses:entryClasses forKey:(id)OF_STR("NS.values")];
    keys = _ONSIsNSArray(keys) ? keys : nil;
    values = _ONSIsNSArray(values) ? values : nil;
    NSUInteger keyCount = [keys count];
    NSUInteger valueCount = [values count];
    if (keyCount == 0 || valueCount == 0) {
        failDecoding(
            self,
            coder,
            OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("%@: encoded keys/values are empty"),
                __CFExceptionProem(self, _cmd)
            )
        );
        return nil;
    }
    if (keyCount != valueCount) {
        failDecoding(
            self,
            coder,
            OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("%@: encoded keys/values length does not match"),
                __CFExceptionProem(self, _cmd)
            )
        );
        return nil;
    }
    if (keyCount != (NSUInteger)count) {
        failDecoding(
            self,
            coder,
            OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("%@: more entries than expected"),
                __CFExceptionProem(self, _cmd)
            )
        );
        return nil;
    }
    id keyStack[count < 0x101 ? count : 1];
    id *keyBuffer = keyStack;
    size_t keyStorageSize = 0;
    void *keyHeap = NULL;
    if (count > 0x100) {
        keyHeap = _OCFCreateArrayStorage(count, false, &keyStorageSize);
        keyBuffer = (id *)keyHeap;
    }
    id valueStack[count < 0x101 ? count : 1];
    size_t valueStorageSize = 0;
    void *valueHeap = NULL;
    if (count > 0x100) {
        valueHeap = _OCFCreateArrayStorage(count, false, &valueStorageSize);
    }
    id *valueBuffer = count < 0x101 ? valueStack : (id *)valueHeap;
    [keys getObjects:keyBuffer range:(OCFRange){0, (OCFIndex)count}];
    for (int64_t index = 0; index != count; index++) {
        id key = keyBuffer[index];
        if ([key isKindOfClass:arrayClass] || [key isKindOfClass:dictionaryClass]) {
            failDecoding(
                self,
                coder,
                OCFStringCreateWithFormat(
                    kOCFAllocatorSystemDefault, NULL, OF_STR("unexpected recursive keys (%@)"), [key class]
                )
            );
            free(valueHeap);
            free(keyHeap);
            return nil;
        }
    }
    [values getObjects:valueBuffer range:(OCFRange){0, (OCFIndex)count}];
    for (int64_t index = 0; index != count; index++) {
        id key = keyBuffer[index];
        if (_ifkIMP(_keyMap, @selector(indexForKey:), key) == LONG_MAX) {
            failDecoding(
                self,
                coder,
                OCFStringCreateWithFormat(
                    kOCFAllocatorSystemDefault,
                    NULL,
                    OF_STR("%@: encoded key not present in keyset"),
                    __CFExceptionProem(self, _cmd)
                )
            );
            free(valueHeap);
            free(keyHeap);
            return nil;
        }
        [self setObject:valueBuffer[index] forKey:key];
    }
    free(valueHeap);
    free(keyHeap);
    return self;
}

// Native ___archiveIsCorrupt, 0x1c02d85ac. Non-external. An autoreleased CFError of kCFErrorDomainCocoa, code
// 4864, whose user info is +[NSDictionary dictionaryWithObjects:forKeys:count:] of the reason under
// kCFErrorLocalizedDescriptionKey.
static id __archiveIsCorrupt(id reason) {
    OCFStringRef domain = kOCFErrorDomainCocoa;
    id key = (id)kOCFErrorLocalizedDescriptionKey;
    id userInfo = [(Class)sharedKeyDictionaryDictionaryClass dictionaryWithObjects:&reason
                                                                           forKeys:&key
                                                                             count:1];
    return [(id)OCFErrorCreate(NULL, domain, 0x1300, (OCFDictionaryRef)userInfo) autorelease];
}

// Native _cleanup_sentinel_pair, 0x1c02d8644. Non-external. The cleanup of -initWithCoder:'s placeholder
// records: when both the decoded object and its placeholder are set, the unarchiver gets the object back
// (-replaceObject:placeholder withObject:object); the placeholder is released and both fields cleared.
static void cleanup_sentinel_pair(SentinelPair *pair) {
    id object = pair->object;
    id placeholder = pair->placeholder;
    if (object != nil && placeholder != nil) {
        [pair->coder replaceObject:placeholder withObject:object];
    }
    [placeholder release];
    pair->object = nil;
    pair->placeholder = nil;
}

// Native +[NSSharedKeyDictionary supportsSecureCoding], 0x1c02d871c. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native +[NSSharedKeyDictionary _subclassesMustBeExplicitlyMentionedWhenDecoded], 0x1c02d8724. Non-external.
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded {
    return YES;
}

// Native -[NSSharedKeyDictionary setObservationInfo:], 0x1c02d872c. Non-external. _doKVO records whether the
// observation info is not NULL, then super stores it.
- (void)setObservationInfo:(void *)observationInfo {
    _doKVO = observationInfo != NULL;
    [super setObservationInfo:observationInfo];
}

@end
