//
//  Created by ktiays on 2026/11/04.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Collections/ONSMapTable.h"
#import "Foundation/Archiving/ONSObjCRuntimeNames.h"
#import "Foundation/String/ONSAllocateObject.h"
#import "Foundation/String/ONSLog.h"
#import "CoreFoundation/Objective-C/OCFNSFastEnumerationEnumerator.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <stdlib.h>

// Inferred organization: the NSMapTable unit spans 0x1c0567a5c-0x1c056a957 of the iOS Simulator Foundation
// __text section (the NSMapTable methods, then the NSConcreteMapTable methods with _probeGC,
// _NSConcreteMapTable_reclaim and _dealloc at their native places); its cold pieces are raiseError
// (0x1c0fddc84, linked as _raiseError_0), allocateInitialCapacity (0x1c0fddcfc, linked as
// _allocateInitialCapacity_0) and empty (0x1c0fddd58, linked as _empty_0). Only the methods the captured
// groups reach are restored. Objective-C compiled with manual reference counting. Unless a definition says
// otherwise: iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. The shared-cache bindings are those of
// Collections/ONSPointerFunctions.m (confidence: medium), and 0x22227FB98, the class of the key enumerator
// (+allocWithZone: through objc_allocWithZone, then -initWithObject:), bound as CoreFoundation's
// __NSFastEnumerationEnumerator from its position after the NSURL slot (0x22227FB88) in the alphabetical GOT
// order of Foundation's CoreFoundation imports and the selector (confidence: medium).
//
// The keys and the values are two parallel stores, probed by the key (open addressing with linear probing
// over a power-of-two capacity, the 64-bit Jenkins mix). A pair whose key or value was cleared by a weak
// reference is reclaimed when the table grows or when a probe purges.
//
// The relinquish of a dangling value in _NSConcreteMapTable_reclaim and -grow is passed the keys' size
// function, not the values' (native; the two coincide for every object personality).

extern void objc_release(id object);
extern id objc_autorelease(id object);
extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *pool);

extern char mapTableStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char mapTableMutableStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableString));
extern char mapTableErrorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSError));
extern char mapTableExceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char mapTableMutableArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableArray));
extern char mapTableKeyedUnarchiverClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                     OF_NAME_STRING(ONSKeyedUnarchiver));

// The messages the unit sends to objects of other classes, declared for the sends only.
@protocol MapTableMessages
+ (id)stringWithFormat:(id)format, ...;
+ (id)string;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)_readCorruptErrorWithFormat:(id)format, ...;
+ (id)array;
- (id)initWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
- (id)initWithObject:(id)object;
- (void)addObject:(id)object;
- (void)appendString:(id)string;
- (void)appendFormat:(id)format, ...;
- (BOOL)containsValueForKey:(id)key;
- (int64_t)decodeInt64ForKey:(id)key;
- (void)encodeInt64:(int64_t)value forKey:(id)key;
- (BOOL)allowsKeyedCoding;
- (void)decodeValueOfObjCType:(const char *)type at:(void *)address size:(NSUInteger)size;
- (void)encodeValueOfObjCType:(const char *)type at:(const void *)address;
- (BOOL)_containsNextUnkeyedObject;
- (id)decodeObject;
- (void)encodeObject:(id)object;
- (void)encodeConditionalObject:(id)object;
- (void)failWithError:(id)error;
@end

static void raiseError(id object, SEL selector) __attribute__((noreturn));
static void allocateInitialCapacity(ONSSlice *slice, NSUInteger capacity);
static void empty(ONSSlice *slice, NSUInteger capacity);
static NSUInteger
probeGC(ONSConcreteMapTable *table, const void *key, BOOL *found, void **foundKey, BOOL purge);
static NSUInteger NSConcreteMapTable_reclaim(ONSConcreteMapTable *table);
static void dealloc(ONSSlice *slice, NSUInteger capacity);

@implementation ONSMapTable

// Native +[NSMapTable allocWithZone:], 0x1c0567a5c. Non-external. The concrete class, whatever the receiver;
// the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return ONSAllocateObject([ONSConcreteMapTable self], 0, NULL);
}

// Native -[NSMapTable initWithCoder:], 0x1c0567c1c. Non-external.
- (id)initWithCoder:(id)coder {
    objc_release(self);
    return [[ONSConcreteMapTable alloc] initWithCoder:coder];
}

// Native -[NSMapTable classForCoder], 0x1c0567c54. Non-external.
- (Class)classForCoder {
    return [ONSMapTable self];
}

// Native +[NSMapTable _subclassesMustBeExplicitlyMentionedWhenDecoded], 0x1c0567c60. Non-external.
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded {
    return YES;
}

// Native -[NSMapTable initWithKeyOptions:valueOptions:capacity:], 0x1c0567c68. Non-external.
- (id)initWithKeyOptions:(NSUInteger)keyOptions
            valueOptions:(NSUInteger)valueOptions
                capacity:(NSUInteger)capacity {
    raiseError(self, _cmd);
}

// Native -[NSMapTable init], 0x1c0567c80. Non-external.
- (id)init {
    raiseError(self, _cmd);
}

// Native -[NSMapTable description], 0x1c0567c8c. Non-external.
- (id)description {
    raiseError(self, _cmd);
}

// Native -[NSMapTable encodeWithCoder:], 0x1c0567c98. Non-external.
- (void)encodeWithCoder:(id)coder {
    raiseError(self, _cmd);
}

// Native -[NSMapTable count], 0x1c0567ca4. Non-external.
- (NSUInteger)count {
    raiseError(self, _cmd);
}

// Native -[NSMapTable keyEnumerator], 0x1c0567cbc. Non-external.
- (id)keyEnumerator {
    raiseError(self, _cmd);
}

// Native -[NSMapTable copy], 0x1c0567cd4. Non-external.
- (id)copy {
    raiseError(self, _cmd);
}

// Native -[NSMapTable countByEnumeratingWithState:objects:count:], 0x1c0567ce0. Non-external.
- (NSUInteger)countByEnumeratingWithState:(PointerCollectionEnumerationState *)state
                                  objects:(id *)buffer
                                    count:(NSUInteger)length {
    raiseError(self, _cmd);
}

// Native -[NSMapTable objectForKey:], 0x1c0567d04. Non-external.
- (id)objectForKey:(id)key {
    raiseError(self, _cmd);
}

// Native -[NSMapTable setObject:forKey:], 0x1c0567d10. Non-external.
- (void)setObject:(id)object forKey:(id)key {
    raiseError(self, _cmd);
}

// Native -[NSMapTable removeObjectForKey:], 0x1c0567d1c. Non-external.
- (void)removeObjectForKey:(id)key {
    raiseError(self, _cmd);
}

// Native -[NSMapTable allKeys], 0x1c0567d64. Non-external.
- (id)allKeys {
    raiseError(self, _cmd);
}

// Native -[NSMapTable allValues], 0x1c0567d70. Non-external.
- (id)allValues {
    raiseError(self, _cmd);
}

// Native -[NSMapTable removeAllItems], 0x1c0567d7c. Non-external.
- (void)removeAllItems {
    raiseError(self, _cmd);
}

// Native -[NSMapTable removeAllObjects], 0x1c0567dac. Non-external.
- (void)removeAllObjects {
    [self removeAllItems];
}

// Native -[NSMapTable copyWithZone:], 0x1c0567fe0. Non-external. The zone is not read.
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self copy];
}

// Native -[NSMapTable mutableCopyWithZone:], 0x1c0567fe4. Non-external. The zone is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return [self copy];
}

@end

@implementation ONSConcreteMapTable

// Native -[NSConcreteMapTable init], 0x1c0567fe8. Non-external.
- (id)init {
    return [self initWithKeyOptions:0 valueOptions:0 capacity:16];
}

// Native -[NSConcreteMapTable raiseCountUnderflowException], 0x1c0567ff8. Non-external. The selector argument
// of the format is the receiver (native).
- (void)raiseCountUnderflowException {
    objc_exception_throw([(id)mapTableExceptionClass
        exceptionWithName:(id)ONSInternalInconsistencyException
                   reason:[(id)mapTableStringClass
                              stringWithFormat:(id)OF_STR("*** -[NSMapTable %@] count underflow"), self]
                 userInfo:nil]);
}

// Native -[NSConcreteMapTable realCount], 0x1c0568044. Non-external. The slots whose key and value both read.
- (NSUInteger)realCount {
    if (capacity == 0) {
        return 0;
    }
    NSUInteger offset = 0;
    NSUInteger live = 0;
    NSUInteger index = 0;
    do {
        void *key = keys.internalProps->startRead((void **)((char *)keys.items + offset), NULL);
        void *value = values.internalProps->startRead((void **)((char *)values.items + offset), NULL);
        if (key != NULL && value != NULL) {
            ++live;
        }
        keys.internalProps->finishRead(key);
        values.internalProps->finishRead(value);
        ++index;
        offset += 8;
    } while (index < capacity);
    return live;
}

// Native -[NSConcreteMapTable _setBackingStore], 0x1c0568188. Non-external. The key store uses the sentinel
// for weak keys (or, when the key memory's flag0 is 1, for weak values); the value store never does.
- (void)_setBackingStore {
    ONSSliceExternalAcquisitionProperties *keyAcquisition = keys.acquisitionProps;
    BOOL sentinel;
    if ((keyAcquisition->isWeak & 1) != 0) {
        sentinel = 1;
    } else if (keyAcquisition->flag0 == 1) {
        sentinel = values.acquisitionProps->isWeak;
    } else {
        sentinel = 0;
    }
    [ONSConcretePointerFunctions initializeBackingStore:&keys
                                               sentinel:sentinel & 1
                                      weakAutoreleasing:keyAcquisition->weakAutoreleasing
                                                dynamic:hasDynamicSlices];
    [ONSConcretePointerFunctions initializeBackingStore:&values
                                               sentinel:NO
                                      weakAutoreleasing:values.acquisitionProps->weakAutoreleasing
                                                dynamic:hasDynamicSlices];
}

// Native -[NSConcreteMapTable copy], 0x1c0568208. Non-external. A table with the receiver's capacity and
// options refilled with every live pair; a dangling pair marks the receiver for rehashing, done at the end.
- (id)copy {
    ONSConcreteMapTable *copy = [ONSConcreteMapTable alloc];
    copy->count = 0;
    copy->capacity = capacity;
    copy->keyOptions = keyOptions;
    copy->valueOptions = valueOptions;
    copy->mutations = 0;
    if (hasDynamicSlices == YES) {
        NSSliceInitWithSlice(&copy->keys, &keys);
        NSSliceInitWithSlice(&copy->values, &values);
        copy->hasDynamicSlices = YES;
    } else {
        copy->keys = keys;
        copy->values = values;
    }
    allocateInitialCapacity(&copy->keys, capacity);
    allocateInitialCapacity(&copy->values, capacity);
    if (capacity != 0) {
        NSUInteger offset = 0;
        NSUInteger index = 0;
        do {
            void *key = keys.internalProps->startRead((void **)((char *)keys.items + offset), NULL);
            void *value = values.internalProps->startRead((void **)((char *)values.items + offset), NULL);
            if (((uintptr_t)key | (uintptr_t)value) != 0) {
                if (key != NULL && value != NULL) {
                    [copy setObject:(id)value forKey:(id)key];
                } else {
                    shouldRehash = YES;
                }
                keys.internalProps->finishRead(key);
                values.internalProps->finishRead(value);
            }
            ++index;
            offset += 8;
        } while (index < capacity);
    }
    if ((shouldRehash & 1) != 0) {
        [self rehash];
    }
    return copy;
}

// Native -[NSConcreteMapTable initWithKeyOptions:valueOptions:capacity:], 0x1c05683a0. Non-external. Below 5
// the capacity is 4; a power of two is kept; otherwise the bit length (counted over at most 24 halvings)
// gives a 32-bit power of two, at least 4.
- (id)initWithKeyOptions:(NSUInteger)tableKeyOptions
            valueOptions:(NSUInteger)tableValueOptions
                capacity:(NSUInteger)requestedCapacity {
    NSUInteger roundedCapacity = requestedCapacity;
    if (requestedCapacity < 5) {
        roundedCapacity = 4;
    } else if ((requestedCapacity & (requestedCapacity - 1)) != 0) {
        NSUInteger halvings = 0;
        NSUInteger bits;
        while (1) {
            bits = halvings + 1;
            if (roundedCapacity < 2) {
                break;
            }
            roundedCapacity >>= 1;
            if (!(halvings++ < 0x17)) {
                break;
            }
        }
        unsigned int power = 1u << (unsigned int)bits;
        roundedCapacity = power > 4 ? power : 4;
    }
    keyOptions = tableKeyOptions;
    valueOptions = tableValueOptions;
    count = 0;
    capacity = roundedCapacity;
    mutations = 0;
    growLock = OS_UNFAIR_LOCK_INIT;
    if (![ONSConcretePointerFunctions initializeSlice:&keys withOptions:tableKeyOptions] ||
        ![ONSConcretePointerFunctions initializeSlice:&values withOptions:valueOptions]) {
        id reason = [(id)mapTableStringClass
            stringWithFormat:(id)OF_STR("*** -[NSMapTable %@] Requested configuration not supported."),
                             ONSStringFromSelector(_cmd)];
        objc_release(self);
        objc_exception_throw([(id)mapTableExceptionClass
            exceptionWithName:(id)ONSInternalInconsistencyException
                       reason:reason
                     userInfo:nil]);
    }
    [self _setBackingStore];
    allocateInitialCapacity(&keys, capacity);
    allocateInitialCapacity(&values, capacity);
    return self;
}

// Native -[NSConcreteMapTable initWithCoder:], 0x1c0568620. Non-external. Keyed archives may carry NS.count,
// which bounds the decoded pairs (nil keys or values are tolerated for weak memory); otherwise pairs are
// decoded up to a nil key.
- (id)initWithCoder:(id)coder {
    NSUInteger expected;
    BOOL counted;
    if ([coder isKindOfClass:(Class)mapTableKeyedUnarchiverClass] &&
        [coder containsValueForKey:(id)OF_STR("NS.count")]) {
        int64_t decodedCount = [coder decodeInt64ForKey:(id)OF_STR("NS.count")];
        if (decodedCount < 0) {
            objc_release(self);
            [coder
                failWithError:[(id)mapTableErrorClass
                                  _readCorruptErrorWithFormat:(id)OF_STR(
                                                                  "NSMapTable archive contains negative count"
                                                              )]];
            return nil;
        }
        expected = (NSUInteger)decodedCount;
        counted = YES;
    } else {
        expected = 0;
        counted = NO;
    }
    int decodedOptions[2] = {0, 0};
    [coder decodeValueOfObjCType:"i" at:&decodedOptions[1] size:4];
    [coder decodeValueOfObjCType:"i" at:&decodedOptions[0] size:4];
    int decodedKeyOptions = decodedOptions[1];
    if (!_NSPointerFunctionCoding_isValidOptions((NSUInteger)(NSInteger)decodedKeyOptions)) {
        objc_release(self);
        [coder
            failWithError:[(id)mapTableErrorClass
                              _readCorruptErrorWithFormat:(id)OF_STR(
                                                              "Decoded unsupported NSMapTable key options %x"
                                                          ),
                                                          decodedOptions[1]]];
        return nil;
    }
    int decodedValueOptions = decodedOptions[0];
    if (!_NSPointerFunctionCoding_isValidOptions((NSUInteger)(NSInteger)decodedValueOptions)) {
        objc_release(self);
        [coder failWithError:[(id)mapTableErrorClass
                                 _readCorruptErrorWithFormat:
                                     (id)OF_STR("Decoded unsupported NSMapTable value options %x"),
                                     decodedOptions[0]]];
        return nil;
    }
    id table = [self initWithKeyOptions:(NSUInteger)(NSInteger)decodedKeyOptions
                           valueOptions:(NSUInteger)(NSInteger)decodedValueOptions
                               capacity:16];
    if (counted) {
        if (expected == 0) {
            return table;
        }
        int weakKeys = decodedOptions[1] & 5;
        int weakValues = decodedOptions[0] & 5;
        while (1) {
            if (![coder _containsNextUnkeyedObject]) {
                id error = [(id)mapTableErrorClass
                    _readCorruptErrorWithFormat:(id)OF_STR(
                                                    "Missing expected key for key while decoding NSMapTable"
                                                )];
                objc_release(table);
                [coder failWithError:error];
                return nil;
            }
            id key = [coder decodeObject];
            if (![coder _containsNextUnkeyedObject]) {
                id error = [(id)mapTableErrorClass
                    _readCorruptErrorWithFormat:(id)OF_STR(
                                                    "Missing expected key for value while decoding NSMapTable"
                                                )];
                objc_release(table);
                [coder failWithError:error];
                return nil;
            }
            id value = [coder decodeObject];
            if (key != nil) {
                if (value != nil) {
                    [table setObject:value forKey:key];
                } else if (weakValues == 0) {
                    goto missing;
                }
            } else if (weakKeys == 0) {
                goto missing;
            }
            if (--expected == 0) {
                return table;
            }
        }
    missing:
        objc_release(table);
        [coder failWithError:[(id)mapTableErrorClass
                                 _readCorruptErrorWithFormat:(id)OF_STR(
                                                                 "Missing key while decoding NSMapTable"
                                                             )]];
        return nil;
    }
    for (id key = [coder decodeObject]; key != nil; key = [coder decodeObject]) {
        id value = [coder decodeObject];
        if (value == nil) {
            objc_release(table);
            [coder
                failWithError:[(id)mapTableErrorClass
                                  _readCorruptErrorWithFormat:(id)OF_STR(
                                                                  "Missing object for non-nil key %@ while "
                                                                  "decoding NSMapTable"
                                                              ),
                                                              key]];
            return nil;
        }
        [table setObject:value forKey:key];
    }
    return table;
}

// Native -[NSConcreteMapTable encodeWithCoder:], 0x1c05689ac. Non-external. Both options as 32-bit integers,
// then each key and value (conditionally for weak memory in keyed archives), a nil terminator, then NS.count
// when keyed. The pairs are buffered keys first, values from the count on; the loop does not bound the
// enumerated pairs by the count (native).
- (void)encodeWithCoder:(id)coder {
    NSUInteger *keyOptionsAddress = &keyOptions;
    NSUInteger encodedKeyOptions = keyOptions;
    NSUInteger *valueOptionsAddress = &valueOptions;
    NSUInteger encodedValueOptions = valueOptions;
    BOOL keyed = [coder allowsKeyedCoding];
    id exceptionName = (id)ONSInvalidArgumentException;
    id reason;
    if (!_NSPointerFunctionCoding_isValidOptions(*keyOptionsAddress)) {
        reason = (id)OF_STR("NSMapTable has invalid key options for encoding");
        goto raise;
    }
    if (!_NSPointerFunctionCoding_isValidOptions(*valueOptionsAddress)) {
        reason = (id)OF_STR("NSMapTable has invalid value options for encoding");
        goto raise;
    }
    NSUInteger expected = count;
    if ((NSInteger)expected < 0) {
        goto tooBig;
    }
    void **buffer = _NSPointerFunctionCoding_bufferForCount(2 * expected);
    if (buffer == NULL) {
        goto tooBig;
    }
    NSUInteger encodedCount = 0;
    for (id key in self) {
        id value = [self objectForKey:key];
        if (key == nil || value == nil) {
            free(buffer);
            reason = (id)OF_STR("NSMapTable has nil item that cannot be encoded");
            goto raise;
        }
        buffer[encodedCount] = key;
        buffer[expected + encodedCount++] = value;
    }
    NSUInteger valueMemory = encodedValueOptions & 7;
    bool complete = (encodedKeyOptions & 7) == 5 || valueMemory == 5;
    if (encodedCount >= expected) {
        complete = true;
    }
    if (encodedCount > expected || !complete) {
        free(buffer);
        reason = (id)OF_STR("NSMapTable was modified during archival");
        goto raise;
    }
    BOOL conditionalKeys = (encodedKeyOptions & 7) == 5 ? keyed : NO;
    BOOL conditionalValues = valueMemory == 5 ? keyed : NO;
    [coder encodeValueOfObjCType:"i" at:keyOptionsAddress];
    [coder encodeValueOfObjCType:"i" at:valueOptionsAddress];
    void **pair = buffer;
    for (NSUInteger remaining = encodedCount; remaining != 0; --remaining) {
        if (conditionalKeys) {
            [coder encodeConditionalObject:(id)pair[0]];
        } else {
            [coder encodeObject:(id)pair[0]];
        }
        if (conditionalValues) {
            [coder encodeConditionalObject:(id)pair[expected]];
        } else {
            [coder encodeObject:(id)pair[expected]];
        }
        ++pair;
    }
    [coder encodeObject:nil];
    free(buffer);
    if (keyed) {
        [coder encodeInt64:(int64_t)encodedCount forKey:(id)OF_STR("NS.count")];
    }
    return;

tooBig:
    reason = (id)OF_STR("NSMapTable is too big to archive");
raise:
    objc_exception_throw([(id)mapTableExceptionClass exceptionWithName:exceptionName
                                                                reason:reason
                                                              userInfo:nil]);
}

// Native -[NSConcreteMapTable count], 0x1c0568d40. Non-external. A table marked for rehashing is rehashed and
// recounted first.
- (NSUInteger)count {
    if ((shouldRehash & 1) == 0) {
        return count;
    }
    [self rehash];
    count = [self realCount];
    return count;
}

// Native _probeGC, 0x1c0568eb4. Non-external. The slot holding a key equal to key (found set; the key handed
// out through foundKey after continueRead, or finished), else the first never-used slot. Integer keys are
// their own hash and identity keys hash by address; a plain store is read directly. With purge, a slot whose
// key reads nil is cleared with its value, uncounted and its chain reinserted, and the probe restarts.
static NSUInteger
probeGC(ONSConcreteMapTable *table, const void *key, BOOL *found, void **foundKey, BOOL purge) {
    ONSSliceExternalPersonalityProperties *personality = table->keys.personalityProps;
    uint64_t hash;
    if (personality->isInteger == 1) {
        hash = (uintptr_t)key;
    } else if (personality->usesIdentity == 1) {
        hash = (uintptr_t)key >> 3;
    } else {
        hash = personality->hash(key, personality->size);
    }
    uint64_t a = hash + 0xDB0300492813010AULL;
    uint64_t b = (0x8D6271D7234E6D5AULL - hash) ^ (a << 9);
    uint64_t c = (0x24FCFFB6D7ECFEF7ULL - hash - b) ^ (b >> 8);
    a = (a - b - c) ^ (c >> 38);
    b = (b - c - a) ^ (a << 23);
    c = (c - a - b) ^ (b >> 5);
    a = (a - b - c) ^ (c >> 35);
    b = (b - c - a) ^ (a << 49);
    c = (c - a - b) ^ (b >> 11);
    a = (a - b - c) ^ (c >> 12);
    b = (b - c - a) ^ (a << 18);
    c = (c - a - b) ^ (b >> 22);
    NSUInteger start = c & (table->capacity - 1);
restart:
    for (NSUInteger index = start;; index = (table->capacity - 1) & (index + 1)) {
        bool isEmpty = true;
        ONSSliceInternalProperties *internal = table->keys.internalProps;
        uint8_t readsDirectly = internal->readsDirectly;
        void *existing;
        if (readsDirectly == 1) {
            existing = table->keys.items[index];
        } else {
            existing = internal->startRead(&table->keys.items[index], &isEmpty);
        }
        if (existing != NULL) {
            if (table->keys.personalityProps->isEqual(key, existing, table->keys.personalityProps->size)) {
                if (found != NULL) {
                    *found = YES;
                }
                if (foundKey != NULL) {
                    *foundKey = existing;
                    if ((readsDirectly & 1) == 0) {
                        table->keys.internalProps->continueRead(existing);
                    }
                } else if ((readsDirectly & 1) == 0) {
                    table->keys.internalProps->finishRead(existing);
                }
                return index;
            }
        } else {
            if (isEmpty) {
                return index;
            }
            if (purge) {
                table->keys.internalProps->clear(&table->keys.items[index]);
                void *value = table->values.internalProps->startRead(&table->values.items[index], NULL);
                void (*relinquish)(const void *, NSUInteger (*)(const void *)) =
                    table->values.acquisitionProps->relinquish;
                if (relinquish != NULL && value != NULL) {
                    relinquish(value, table->values.personalityProps->size);
                }
                table->values.internalProps->finishRead(value);
                table->values.internalProps->clear(&table->values.items[index]);
                if (table->count != 0) {
                    table->count = table->count - 1;
                } else {
                    [table raiseCountUnderflowException];
                }
                [table rehashAround:index];
                goto restart;
            }
        }
        if ((readsDirectly & 1) == 0) {
            table->keys.internalProps->finishRead(existing);
        }
    }
}

// Native _NSConcreteMapTable_reclaim, 0x1c0569248. Non-external. Clears every dangling key or value (one of
// the pair read nil), marking the table for rehashing, and counts the live pairs; each slot in its own
// autorelease pool.
static NSUInteger NSConcreteMapTable_reclaim(ONSConcreteMapTable *table) {
    if (table->capacity == 0) {
        return 0;
    }
    NSUInteger offset = 0;
    NSUInteger live = 0;
    NSUInteger index = 0;
    do {
        void *pool = objc_autoreleasePoolPush();
        void **keySlot = (void **)((char *)table->keys.items + offset);
        void **valueSlot = (void **)((char *)table->values.items + offset);
        void *key = table->keys.internalProps->startRead(keySlot, NULL);
        void *value = table->values.internalProps->startRead(valueSlot, NULL);
        if (((uintptr_t)key | (uintptr_t)value) != 0) {
            if (key != NULL && value == NULL) {
                void (*relinquish)(const void *, NSUInteger (*)(const void *)) =
                    table->keys.acquisitionProps->relinquish;
                if (relinquish != NULL) {
                    relinquish(key, table->keys.personalityProps->size);
                }
                table->keys.internalProps->clear((void **)((char *)table->keys.items + offset));
                table->shouldRehash = YES;
                table->keys.internalProps->finishRead(key);
            } else if (key != NULL || value == NULL) {
                table->keys.internalProps->finishRead(key);
                table->values.internalProps->finishRead(value);
                ++live;
            } else {
                void (*relinquish)(const void *, NSUInteger (*)(const void *)) =
                    table->values.acquisitionProps->relinquish;
                if (relinquish != NULL) {
                    relinquish(value, table->keys.personalityProps->size);
                }
                table->values.internalProps->clear((void **)((char *)table->values.items + offset));
                table->shouldRehash = YES;
                table->values.internalProps->finishRead(value);
            }
        }
        objc_autoreleasePoolPop(pool);
        ++index;
        offset += 8;
    } while (index < table->capacity);
    return live;
}

// Native -[NSConcreteMapTable grow], 0x1c05693cc. Non-external. A table marked for rehashing whose rehash
// leaves it under half full does not grow. Otherwise the capacity doubles when the live pairs fill half of it
// (else stays), both stores are replaced under growLock and every live pair is reinserted (dangling halves
// are relinquished and dropped), each slot in its own autorelease pool; then the old stores are freed. A
// failed value-store allocation leaves the new key store allocated (native).
- (void)grow {
    void **oldKeys = keys.items;
    void **oldValues = values.items;
    NSUInteger oldCapacity = capacity;
    if (shouldRehash == 1) {
        [self rehash];
        if (capacity > 2 * count) {
            return;
        }
    }
    NSUInteger newCapacity = oldCapacity << (capacity <= 2 * NSConcreteMapTable_reclaim(self));
    void **newKeys = keys.internalProps->allocate(newCapacity);
    void **newValues;
    if (newKeys == NULL || (newValues = values.internalProps->allocate(newCapacity)) == NULL) {
        objc_exception_throw([(id)mapTableExceptionClass
            exceptionWithName:(id)ONSMallocException
                       reason:(id)OF_STR(
                                  "*** NSMapTable unable to allocate memory -- allocation function failed"
                              )
                     userInfo:nil]);
    }
    os_unfair_lock_lock(&growLock);
    keys.items = newKeys;
    values.items = newValues;
    capacity = newCapacity;
    os_unfair_lock_unlock(&growLock);
    count = 0;
    if (oldCapacity != 0) {
        void **keySlot = oldKeys;
        void **valueSlot = oldValues;
        for (NSUInteger remaining = oldCapacity; remaining != 0; --remaining) {
            void *pool = objc_autoreleasePoolPush();
            void *key = keys.internalProps->startRead(keySlot, NULL);
            void *value = values.internalProps->startRead(valueSlot, NULL);
            if (((uintptr_t)key | (uintptr_t)value) != 0) {
                if (key != NULL && value == NULL) {
                    void (*relinquish)(const void *, NSUInteger (*)(const void *)) =
                        keys.acquisitionProps->relinquish;
                    if (relinquish != NULL) {
                        relinquish(key, keys.personalityProps->size);
                    }
                    keys.internalProps->clear(keySlot);
                    keys.internalProps->finishRead(key);
                } else if (key != NULL || value == NULL) {
                    NSUInteger index = probeGC(self, key, NULL, NULL, YES);
                    keys.internalProps->store(keys.items, index, key);
                    values.internalProps->store(values.items, index, value);
                    keys.internalProps->finishRead(key);
                    values.internalProps->finishRead(value);
                    ++count;
                } else {
                    void (*relinquish)(const void *, NSUInteger (*)(const void *)) =
                        values.acquisitionProps->relinquish;
                    if (relinquish != NULL) {
                        relinquish(value, keys.personalityProps->size);
                    }
                    values.internalProps->clear(valueSlot);
                    values.internalProps->finishRead(value);
                }
            }
            objc_autoreleasePoolPop(pool);
            ++valueSlot;
            ++keySlot;
        }
    }
    keys.internalProps->free(oldKeys, oldCapacity);
    values.internalProps->free(oldValues, oldCapacity);
}

// Native -[NSConcreteMapTable assign:key:value:isNew:], 0x1c0569688. Non-external. Counts a mutation; a new
// key is acquired and stored. The value is acquired, then the old value (when not new) is relinquished and
// cleared, and the value stored.
- (void)assign:(NSUInteger)index key:(const void *)key value:(const void *)value isNew:(BOOL)isNew {
    ++mutations;
    if (isNew) {
        ONSSliceExternalAcquisitionProperties *keyAcquisition = keys.acquisitionProps;
        void *(*acquireKey)(const void *, NSUInteger (*)(const void *), BOOL) = keyAcquisition->acquire;
        if (acquireKey != NULL) {
            key = acquireKey(key, keys.personalityProps->size, keyAcquisition->copyIn);
        }
        keys.internalProps->store(keys.items, index, (void *)key);
    }
    ONSSliceExternalAcquisitionProperties *valueAcquisition = values.acquisitionProps;
    void *(*acquireValue)(const void *, NSUInteger (*)(const void *), BOOL) = valueAcquisition->acquire;
    if (acquireValue != NULL) {
        value = acquireValue(value, values.personalityProps->size, valueAcquisition->copyIn);
    }
    if ((isNew & 1) == 0) {
        if (values.acquisitionProps->relinquish != NULL) {
            void *old = values.internalProps->startRead(&values.items[index], NULL);
            values.acquisitionProps->relinquish(old, values.personalityProps->size);
            values.internalProps->finishRead(old);
        }
        values.internalProps->clear(&values.items[index]);
    }
    values.internalProps->store(values.items, index, (void *)value);
}

// Native -[NSConcreteMapTable objectForKey:], 0x1c056979c. Non-external.
- (id)objectForKey:(id)key {
    if (key == nil) {
        return nil;
    }
    BOOL found = NO;
    NSUInteger index = probeGC(self, key, &found, NULL, NO);
    void *value = NULL;
    if (found == 1) {
        value = values.internalProps->startRead(&values.items[index], NULL);
        values.internalProps->continueRead(value);
    }
    return (id)value;
}

// Native -[NSConcreteMapTable setObject:forKey:], 0x1c056984c. Non-external. A nil object or key is ignored;
// a new pair is counted and may grow the table.
- (void)setObject:(id)object forKey:(id)key {
    if (object == nil || key == nil) {
        return;
    }
    BOOL found = NO;
    NSUInteger index = probeGC(self, key, &found, NULL, YES);
    BOOL existed = found;
    [self assign:index key:key value:object isNew:existed ^ 1];
    if ((existed & 1) == 0) {
        NSUInteger currentCapacity = capacity;
        count = count + 1;
        if (currentCapacity < 2 * count) {
            [self grow];
        }
    }
}

// Native -[NSConcreteMapTable rehashAround:], 0x1c0569d60. Non-external. From the first never-used key slot
// before index, reinserts every live pair up to the next never-used key slot, dropping and uncounting
// dangling pairs; a value left beside a never-used key is logged and dropped. Returns that slot (scanning on
// when it is index itself).
- (NSUInteger)rehashAround:(NSUInteger)index {
    ONSSliceInternalProperties *keyInternal = keys.internalProps;
    NSUInteger currentCapacity = capacity;
    void **keyItems = keys.items;
    NSUInteger slot = index;
    do {
        slot = (slot - 1) & (currentCapacity - 1);
    } while ((uintptr_t)keyItems[slot] != keyInternal->sentinel);
    while (1) {
        slot = (slot + 1) & (currentCapacity - 1);
        bool isEmpty = false;
        void *key = keyInternal->startRead(&keyItems[slot], &isEmpty);
        void *value = values.internalProps->startRead(&values.items[slot], NULL);
        if (key == NULL && isEmpty) {
            if (value != NULL) {
                ONSLog(
                    (id)OF_STR("NSMaptable %p isEmpty key but not value %p at index %ld"), self, value, slot
                );
                void (*relinquish)(const void *, NSUInteger (*)(const void *)) =
                    values.acquisitionProps->relinquish;
                if (relinquish != NULL) {
                    relinquish(value, values.personalityProps->size);
                }
                values.internalProps->clear(&values.items[slot]);
            }
            keys.internalProps->finishRead(NULL);
            values.internalProps->finishRead(value);
            if (slot != index) {
                return slot;
            }
        } else {
            if (key == NULL || value == NULL) {
                if (key != NULL) {
                    void (*relinquishKey)(const void *, NSUInteger (*)(const void *)) =
                        keys.acquisitionProps->relinquish;
                    if (relinquishKey != NULL) {
                        relinquishKey(key, keys.personalityProps->size);
                    }
                }
                keys.internalProps->clear(&keys.items[slot]);
                void (*relinquishValue)(const void *, NSUInteger (*)(const void *)) =
                    values.acquisitionProps->relinquish;
                if (relinquishValue != NULL && value != NULL) {
                    relinquishValue(value, values.personalityProps->size);
                }
                values.internalProps->clear(&values.items[slot]);
                if (count != 0) {
                    count = count - 1;
                } else {
                    [self raiseCountUnderflowException];
                }
            } else {
                keys.internalProps->clear(&keys.items[slot]);
                values.internalProps->clear(&values.items[slot]);
                NSUInteger target = probeGC(self, key, NULL, NULL, NO);
                keys.internalProps->store(keys.items, target, key);
                values.internalProps->store(values.items, target, value);
            }
            keys.internalProps->finishRead(key);
            values.internalProps->finishRead(value);
        }
        currentCapacity = capacity;
        keyInternal = keys.internalProps;
        keyItems = keys.items;
    }
}

// Native -[NSConcreteMapTable rehash], 0x1c0569ffc. Non-external. Reinserts the chains of the used key slots
// over the whole capacity (stopping when a chain wraps behind the scan), then clears the rehash mark.
- (void)rehash {
    NSUInteger limit = capacity;
    if (limit != 0) {
        NSUInteger index = 0;
        uintptr_t sentinel = keys.internalProps->sentinel;
        do {
            if ((uintptr_t)keys.items[index] == sentinel) {
                ++index;
            } else {
                NSUInteger next = [self rehashAround:index];
                if (next < index) {
                    break;
                }
                limit = capacity;
                index = next;
            }
        } while (index < limit);
    }
    shouldRehash = NO;
}

// Native -[NSConcreteMapTable removeObjectForKey:], 0x1c056a078. Non-external. The count is decremented
// without the underflow check.
- (void)removeObjectForKey:(id)key {
    if (key == nil) {
        return;
    }
    BOOL found = NO;
    void *foundKey = NULL;
    NSUInteger index = probeGC(self, key, &found, &foundKey, YES);
    if (found != 1) {
        return;
    }
    void (*relinquishKey)(const void *, NSUInteger (*)(const void *)) = keys.acquisitionProps->relinquish;
    if (relinquishKey != NULL) {
        relinquishKey(foundKey, keys.personalityProps->size);
    }
    if (values.acquisitionProps->relinquish != NULL) {
        void *value = values.internalProps->startRead(&values.items[index], NULL);
        values.acquisitionProps->relinquish(value, values.personalityProps->size);
        values.internalProps->finishRead(value);
    }
    keys.internalProps->clear(&keys.items[index]);
    values.internalProps->clear(&values.items[index]);
    --count;
    ++mutations;
    [self rehashAround:index];
}

// Native -[NSConcreteMapTable removeAllItems], 0x1c056a1b0. Non-external.
- (void)removeAllItems {
    ++mutations;
    empty(&keys, capacity);
    empty(&values, capacity);
    count = 0;
}

// Native -[NSConcreteMapTable dealloc], 0x1c056a1f4. Non-external.
- (void)dealloc {
    dealloc(&keys, capacity);
    dealloc(&values, capacity);
    if (hasDynamicSlices == YES) {
        NSSliceDealloc(&keys);
        NSSliceDealloc(&values);
    }
    [super dealloc];
}

// Native _dealloc, 0x1c056a294. Non-external. Empties a store that has read and clear functions, then frees
// it.
static void dealloc(ONSSlice *slice, NSUInteger capacity) {
    ONSSliceInternalProperties *internal = slice->internalProps;
    if (internal == NULL) {
        return;
    }
    if (internal->startRead != NULL && internal->clear != NULL) {
        empty(slice, capacity);
        internal = slice->internalProps;
    }
    if (internal->free != NULL) {
        internal->free(slice->items, capacity);
    }
}

// Native -[NSConcreteMapTable description], 0x1c056a2f8. Non-external. One line per live pair, with the slot
// index; the header is appended as a format.
- (id)description {
    id description = [(id)mapTableMutableStringClass string];
    [description appendFormat:(id)OF_STR("NSMapTable {\n")];
    if (capacity != 0) {
        NSUInteger offset = 0;
        NSUInteger index = 0;
        do {
            void *key = keys.internalProps->startRead((void **)((char *)keys.items + offset), NULL);
            void *value = values.internalProps->startRead((void **)((char *)values.items + offset), NULL);
            if (key != NULL && value != NULL) {
                id keyDescription = keys.personalityProps->description(key);
                [description appendFormat:(id)OF_STR("[%lu] %@ -> %@\n"),
                                          index,
                                          keyDescription,
                                          values.personalityProps->description(value)];
            }
            keys.internalProps->finishRead(key);
            values.internalProps->finishRead(value);
            ++index;
            offset += 8;
        } while (index < capacity);
    }
    [description appendString:(id)OF_STR("}\n")];
    return description;
}

// Native -[NSConcreteMapTable countByEnumeratingWithState:objects:count:], 0x1c056a42c. Non-external. One key
// per call through the state's extra[1] (its value kept in extra[2], the mutation count in extra[0]); a slot
// with only one half readable is skipped without finishing its reads (native).
- (NSUInteger)countByEnumeratingWithState:(PointerCollectionEnumerationState *)state
                                  objects:(id *)buffer
                                    count:(NSUInteger)length {
    state->mutationsPtr = (unsigned long *)&mutations;
    if (state->state != 0) {
        if (state->extra[0] == mutations) {
            keys.internalProps->finishRead((void *)state->extra[1]);
            values.internalProps->finishRead((void *)state->extra[2]);
        } else {
            objc_enumerationMutation(self);
        }
    } else {
        state->extra[0] = mutations;
    }
    NSUInteger index = state->state;
    while (index < capacity) {
        void *key = keys.internalProps->startRead(&keys.items[index], NULL);
        void *value = values.internalProps->startRead(&values.items[state->state], NULL);
        bool skip = key == NULL || value == NULL;
        index = state->state + 1;
        state->state = index;
        if (!skip) {
            state->extra[1] = (unsigned long)key;
            state->itemsPtr = (id *)&state->extra[1];
            state->extra[2] = (unsigned long)value;
            return 1;
        }
    }
    return 0;
}

// Native -[NSConcreteMapTable keyEnumerator], 0x1c056a524. Non-external.
- (id)keyEnumerator {
    return objc_autorelease([[__ONSFastEnumerationEnumerator allocWithZone:NULL] initWithObject:self]);
}

// Native -[NSConcreteMapTable hash], 0x1c056a65c. Non-external. The count, or the receiver itself when the
// keys or the values are weak.
- (NSUInteger)hash {
    if ((keys.internalProps->isWeak & 1) == 0 && values.internalProps->isWeak != 1) {
        return count;
    }
    return (NSUInteger)self;
}

// Native -[NSConcreteMapTable isEqual:], 0x1c056a680. Non-external. Same class, neither side weak, the same
// count and isEqual and size functions, and for each used key slot an equal value under the other table
// (the raw slot contents are compared).
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    if ([object class] != [self class]) {
        return NO;
    }
    if ((keys.internalProps->isWeak & 1) != 0 || (values.internalProps->isWeak & 1) != 0) {
        return NO;
    }
    ONSConcreteMapTable *other = object;
    if (count != other->count) {
        return NO;
    }
    if (keys.personalityProps->isEqual != other->keys.personalityProps->isEqual ||
        values.personalityProps->isEqual != other->values.personalityProps->isEqual ||
        keys.personalityProps->size != other->keys.personalityProps->size ||
        values.personalityProps->size != other->values.personalityProps->size) {
        return NO;
    }
    NSUInteger limit = capacity;
    for (NSUInteger index = 0; index < limit; ++index) {
        if (keys.items[index] != NULL) {
            id otherValue = [other objectForKey:(id)keys.items[index]];
            if (otherValue == nil) {
                return NO;
            }
            if (!values.personalityProps->isEqual(
                    values.items[index], otherValue, values.personalityProps->size
                )) {
                return NO;
            }
            limit = capacity;
        }
    }
    return YES;
}

// Native -[NSConcreteMapTable allKeys], 0x1c056a7b0. Non-external. The keys of the live pairs.
- (id)allKeys {
    id result = [(id)mapTableMutableArrayClass array];
    if (capacity != 0) {
        NSUInteger offset = 0;
        NSUInteger index = 0;
        do {
            void *key = keys.internalProps->startRead((void **)((char *)keys.items + offset), NULL);
            void *value = values.internalProps->startRead((void **)((char *)values.items + offset), NULL);
            if (key != NULL && value != NULL) {
                [result addObject:(id)key];
            }
            keys.internalProps->finishRead(key);
            values.internalProps->finishRead(value);
            ++index;
            offset += 8;
        } while (index < capacity);
    }
    return result;
}

// Native -[NSConcreteMapTable allValues], 0x1c056a884. Non-external. The values of the live pairs.
- (id)allValues {
    id result = [(id)mapTableMutableArrayClass array];
    if (capacity != 0) {
        NSUInteger offset = 0;
        NSUInteger index = 0;
        do {
            void *key = keys.internalProps->startRead((void **)((char *)keys.items + offset), NULL);
            void *value = values.internalProps->startRead((void **)((char *)values.items + offset), NULL);
            if (key != NULL && value != NULL) {
                [result addObject:(id)value];
            }
            keys.internalProps->finishRead(key);
            values.internalProps->finishRead(value);
            ++index;
            offset += 8;
        } while (index < capacity);
    }
    return result;
}

@end

// Native raiseError, 0x1c0fddc84 (cold region, linked as _raiseError_0). Non-external.
static void raiseError(id object, SEL selector) {
    id exception = [(id)mapTableExceptionClass alloc];
    const char *name = sel_getName(selector);
    id reason = [(id)mapTableStringClass
        stringWithFormat:(id)OF_STR("abstract method %s called directly on class %@"), name, [object class]];
    objc_exception_throw([exception initWithName:(id)OF_STR("NSMapTableAbstractImplementationError")
                                          reason:reason
                                        userInfo:nil]);
}

// Native allocateInitialCapacity, 0x1c0fddcfc (cold region, linked as _allocateInitialCapacity_0).
// Non-external.
static void allocateInitialCapacity(ONSSlice *slice, NSUInteger capacity) {
    slice->items = slice->internalProps->allocate(capacity);
    if (slice->items == NULL) {
        objc_exception_throw([(id)mapTableExceptionClass
            exceptionWithName:(id)ONSMallocException
                       reason:(id)OF_STR(
                                  "*** NSMapTable unable to allocate memory -- allocation function failed"
                              )
                     userInfo:nil]);
    }
}

// Native empty, 0x1c0fddd58 (cold region, linked as _empty_0). Non-external. Relinquishes each readable item,
// finishes the read, then clears its slot (the hash table's empty clears first).
static void empty(ONSSlice *slice, NSUInteger capacity) {
    if (capacity == 0) {
        return;
    }
    NSUInteger remaining = capacity;
    NSUInteger offset = 0;
    do {
        void *item = slice->internalProps->startRead((void **)((char *)slice->items + offset), NULL);
        if (item != NULL) {
            void (*relinquish)(const void *, NSUInteger (*)(const void *)) =
                slice->acquisitionProps->relinquish;
            if (relinquish != NULL) {
                relinquish(item, slice->personalityProps->size);
            }
        }
        slice->internalProps->finishRead(item);
        slice->internalProps->clear((void **)((char *)slice->items + offset));
        offset += 8;
        --remaining;
    } while (remaining != 0);
}
