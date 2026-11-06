//
//  Created by ktiays on 2026/11/04.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Collections/ONSHashTable.h"
#import "Foundation/Archiving/ONSObjCRuntimeNames.h"
#import "Foundation/String/ONSAllocateObject.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdlib.h>

// Inferred organization: the NSHashTable unit spans 0x1c051ded0-0x1c05201c7 of the iOS Simulator Foundation
// __text section (the NSHashTable methods, then the NSConcreteHashTable methods with _hashProbe at its
// native place); its cold pieces are raiseError (0x1c0fdca18), allocateInitialCapacity (0x1c0fdca90) and
// empty (0x1c0fdcaec). Only the methods the captured groups reach are restored. Objective-C compiled with
// manual reference counting. Unless a definition says otherwise: iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. The shared-cache bindings are those of Collections/ONSPointerFunctions.m (confidence: medium).
//
// The table is open addressing with linear probing over a power-of-two capacity, the hash of an item mixed
// by the 64-bit Jenkins mix; it grows (doubles) when the count reaches half the capacity. A removed slot is
// cleared and its probe chain reinserted (-rehashAround:); weak slots that read nil are purged on mutation.

extern void objc_release(id object);
extern id objc_autorelease(id object);

extern char hashTableStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));
extern char hashTableMutableStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableString));
extern char hashTableErrorClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSError));
extern char hashTableExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char hashTableMutableArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char hashTableKeyedUnarchiverClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSKeyedUnarchiver));

// The messages the unit sends to objects of other classes, declared for the sends only.
@protocol HashTableMessages
+ (id)stringWithFormat:(id)format, ...;
+ (id)string;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)_readCorruptErrorWithFormat:(id)format, ...;
+ (id)array;
- (id)initWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
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

@implementation ONSHashTable

// Native +[NSHashTable allocWithZone:], 0x1c051ded0. Non-external. The concrete class, whatever the
// receiver; the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return ONSAllocateObject([ONSConcreteHashTable self], 0, NULL);
}

// Native +[NSHashTable hashTableWithOptions:], 0x1c051df44. Non-external.
+ (id)hashTableWithOptions:(NSUInteger)options {
    return objc_autorelease([[ONSConcreteHashTable alloc] initWithOptions:options capacity:0]);
}

// Native -[NSHashTable initWithCoder:], 0x1c051df78. Non-external.
- (id)initWithCoder:(id)coder {
    objc_release(self);
    return [[ONSConcreteHashTable alloc] initWithCoder:coder];
}

// Native -[NSHashTable classForCoder], 0x1c051dfb0. Non-external.
- (Class)classForCoder {
    return [ONSHashTable self];
}

// Native +[NSHashTable _subclassesMustBeExplicitlyMentionedWhenDecoded], 0x1c051dfbc. Non-external.
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded {
    return YES;
}

// Native -[NSHashTable initWithOptions:capacity:], 0x1c051dfc4. Non-external.
- (id)initWithOptions:(NSUInteger)options capacity:(NSUInteger)capacity {
    raiseError(self, _cmd);
}

// Native -[NSHashTable init], 0x1c051dfd0. Non-external.
- (id)init {
    raiseError(self, _cmd);
}

// Native -[NSHashTable description], 0x1c051dfdc. Non-external.
- (id)description {
    raiseError(self, _cmd);
}

// Native -[NSHashTable encodeWithCoder:], 0x1c051dff4. Non-external.
- (void)encodeWithCoder:(id)coder {
    raiseError(self, _cmd);
}

// Native -[NSHashTable countByEnumeratingWithState:objects:count:], 0x1c051e000. Non-external.
- (NSUInteger)countByEnumeratingWithState:(PointerCollectionEnumerationState *)state
                                  objects:(id *)buffer
                                    count:(NSUInteger)length {
    raiseError(self, _cmd);
}

// Native -[NSHashTable count], 0x1c051e00c. Non-external.
- (NSUInteger)count {
    raiseError(self, _cmd);
}

// Native -[NSHashTable allObjects], 0x1c051e018. Non-external.
- (id)allObjects {
    raiseError(self, _cmd);
}

// Native -[NSHashTable getItem:], 0x1c051e028. Non-external.
- (void *)getItem:(const void *)item {
    raiseError(self, _cmd);
}

// Native -[NSHashTable addObject:], 0x1c051e034. Non-external.
- (void)addObject:(id)object {
    raiseError(self, _cmd);
}

// Native -[NSHashTable removeItem:], 0x1c051e04c. Non-external.
- (void)removeItem:(const void *)item {
    raiseError(self, _cmd);
}

// Native -[NSHashTable copy], 0x1c051e058. Non-external.
- (id)copy {
    raiseError(self, _cmd);
}

// Native -[NSHashTable removeAllItems], 0x1c051e064. Non-external.
- (void)removeAllItems {
    raiseError(self, _cmd);
}

// Native -[NSHashTable pointerFunctions], 0x1c051e088. Non-external.
- (id)pointerFunctions {
    raiseError(self, _cmd);
}

// Native -[NSHashTable removeObject:], 0x1c051e098. Non-external.
- (void)removeObject:(id)object {
    [self removeItem:object];
}

// Native -[NSHashTable removeAllObjects], 0x1c051e09c. Non-external.
- (void)removeAllObjects {
    [self removeAllItems];
}

// Native -[NSHashTable containsObject:], 0x1c051e10c. Non-external.
- (BOOL)containsObject:(id)object {
    return [self getItem:object] != NULL;
}

// Native -[NSHashTable unionHashTable:], 0x1c051e630. Non-external. Adds every object of the other table;
// a table united with itself is unchanged.
- (void)unionHashTable:(id)table {
    if (table != self) {
        for (id object in table) {
            [self addObject:object];
        }
    }
}

// Native -[NSHashTable copyWithZone:], 0x1c051ea34. Non-external. The zone is not read.
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self copy];
}

// Native -[NSHashTable mutableCopyWithZone:], 0x1c051ea38. Non-external. The zone is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return [self copy];
}

@end

// Native _hashProbe, 0x1c051f69c. Non-external. The slot holding an item equal to item (found set; the item
// handed out through foundItem after continueRead, or finished), else the first never-used slot. With purge,
// a weak slot read as nil is cleared, uncounted and its chain reinserted, and the probe restarts.
static NSUInteger
hashProbe(ONSConcreteHashTable *table, const void *item, void **foundItem, BOOL *found, BOOL purge);

@implementation ONSConcreteHashTable

// Native -[NSConcreteHashTable init], 0x1c051ea3c. Non-external.
- (id)init {
    return [self initWithOptions:0 capacity:16];
}

// Native -[NSConcreteHashTable copy], 0x1c051ea48. Non-external. A table with the receiver's capacity and
// options (a dynamic slice is copied, a static one shared), refilled by -insertItem:.
- (id)copy {
    ONSConcreteHashTable *copy = [ONSConcreteHashTable alloc];
    copy->count = 0;
    copy->capacity = capacity;
    copy->options = options;
    copy->mutations = 0;
    if (hasDynamicSlice) {
        NSSliceInitWithSlice(&copy->slice, &slice);
        copy->hasDynamicSlice = 1;
    } else {
        copy->slice = slice;
    }
    allocateInitialCapacity(&copy->slice, capacity);
    for (id object in self) {
        [copy insertItem:object];
    }
    return copy;
}

// Native -[NSConcreteHashTable initWithOptions:capacity:], 0x1c051eb98. Non-external. The capacity is the
// next power of two (computed with a 32-bit shift and sign-extended), 4 for 0; the store uses the sentinel.
- (id)initWithOptions:(NSUInteger)tableOptions capacity:(NSUInteger)requestedCapacity {
    NSUInteger roundedCapacity;
    if (requestedCapacity == 0) {
        roundedCapacity = 4;
    } else if ((requestedCapacity & (requestedCapacity - 1)) == 0) {
        roundedCapacity = requestedCapacity;
    } else {
        unsigned int shift = (unsigned int)-__builtin_clzll(requestedCapacity >> 1) & 31;
        roundedCapacity = (NSUInteger)(NSInteger)(int)(2u << shift);
    }
    count = 0;
    capacity = roundedCapacity;
    mutations = 0;
    options = tableOptions;
    if (![ONSConcretePointerFunctions initializeSlice:&slice withOptions:options]) {
        id reason = [(id)hashTableStringClass
            stringWithFormat:(id)OF_STR("*** -[NSHashTable %@] Requested configuration not supported."),
                             ONSStringFromSelector(_cmd)];
        objc_release(self);
        objc_exception_throw([(id)hashTableExceptionClass
            exceptionWithName:(id)ONSInternalInconsistencyException
                       reason:reason
                     userInfo:nil]);
    }
    [ONSConcretePointerFunctions initializeBackingStore:&slice
                                               sentinel:YES
                                      weakAutoreleasing:slice.acquisitionProps->weakAutoreleasing
                                                dynamic:NO];
    allocateInitialCapacity(&slice, capacity);
    return self;
}

// Native -[NSConcreteHashTable initWithCoder:], 0x1c051edb4. Non-external. Keyed archives may carry
// NS.count, which bounds the decoded elements (nil elements are tolerated for weak memory); otherwise
// elements are decoded up to a nil terminator.
- (id)initWithCoder:(id)coder {
    NSUInteger expected;
    BOOL counted;
    if ([coder isKindOfClass:(Class)hashTableKeyedUnarchiverClass] &&
        [coder containsValueForKey:(id)OF_STR("NS.count")]) {
        int64_t decodedCount = [coder decodeInt64ForKey:(id)OF_STR("NS.count")];
        if (decodedCount < 0) {
            objc_release(self);
            [coder failWithError:[(id)hashTableErrorClass
                                     _readCorruptErrorWithFormat:
                                         (id)OF_STR("NSHashTable archive contains negative count")]];
            return nil;
        }
        expected = (NSUInteger)decodedCount;
        counted = YES;
    } else {
        expected = 0;
        counted = NO;
    }
    int decodedOptions = 0;
    [coder decodeValueOfObjCType:"i" at:&decodedOptions size:4];
    if (!_NSPointerFunctionCoding_isValidOptions((NSUInteger)(NSInteger)decodedOptions)) {
        objc_release(self);
        [coder failWithError:[(id)hashTableErrorClass
                                 _readCorruptErrorWithFormat:(id)OF_STR(
                                                                 "Decoded unsupported NSHashTable options %x"
                                                             ),
                                                             decodedOptions]];
        return nil;
    }
    id table = [self initWithOptions:(NSUInteger)(NSInteger)decodedOptions capacity:16];
    if (!counted) {
        for (id object = [coder decodeObject]; object != nil; object = [coder decodeObject]) {
            [table addObject:object];
        }
        return table;
    }
    if (expected == 0) {
        return table;
    }
    int weakMemory = decodedOptions & 5;
    while (1) {
        if (![coder _containsNextUnkeyedObject]) {
            id error = [(id)hashTableErrorClass
                _readCorruptErrorWithFormat:(id)OF_STR("Missing expected key while decoding NSHashTable")];
            objc_release(table);
            [coder failWithError:error];
            return nil;
        }
        id object = [coder decodeObject];
        if (object != nil) {
            [table addObject:object];
        } else if (weakMemory == 0) {
            objc_release(table);
            [coder
                failWithError:[(id)hashTableErrorClass
                                  _readCorruptErrorWithFormat:(id)OF_STR(
                                                                  "Unexpected nil object encountered while "
                                                                  "decoding NSHashTable."
                                                              )]];
            return nil;
        }
        if (--expected == 0) {
            return table;
        }
    }
}

// Native -[NSConcreteHashTable encodeWithCoder:], 0x1c051f024. Non-external. The options as a 32-bit integer,
// the elements (conditionally for weak memory in keyed archives), a nil terminator, then NS.count when keyed.
// A nil element raises without freeing the buffer (native).
- (void)encodeWithCoder:(id)coder {
    NSUInteger memoryOptions = options;
    BOOL keyed = [coder allowsKeyedCoding];
    id exceptionName;
    id reason;
    if (!_NSPointerFunctionCoding_isValidOptions(options)) {
        exceptionName = (id)ONSInvalidArgumentException;
        reason = (id)OF_STR("NSHashTable has invalid options for encoding");
        goto raise;
    }
    NSUInteger expected = count;
    void **buffer = _NSPointerFunctionCoding_bufferForCount(expected);
    if (buffer == NULL) {
        exceptionName = (id)ONSMallocException;
        reason = (id)OF_STR("NSHashTable is too big to encode");
        goto raise;
    }
    NSUInteger encodedCount = 0;
    for (id object in self) {
        if (encodedCount > expected) {
            goto modified;
        }
        if (object == nil) {
            exceptionName = (id)ONSInvalidArgumentException;
            reason = (id)OF_STR("NSHashTable contained an invalid nil element that was found while encoding");
            goto raise;
        }
        buffer[encodedCount++] = object;
    }
    bool complete = (memoryOptions & 7) == 5;
    if (encodedCount >= expected) {
        complete = true;
    }
    if (encodedCount > expected || !complete) {
        goto modified;
    }
    BOOL conditional = (memoryOptions & 7) == 5 ? keyed : NO;
    NSUInteger encodedOptions = options;
    [coder encodeValueOfObjCType:"i" at:&encodedOptions];
    for (NSUInteger index = 0; index < encodedCount; ++index) {
        if (conditional) {
            [coder encodeConditionalObject:(id)buffer[index]];
        } else {
            [coder encodeObject:(id)buffer[index]];
        }
    }
    [coder encodeObject:nil];
    if (keyed) {
        [coder encodeInt64:(int64_t)encodedCount forKey:(id)OF_STR("NS.count")];
    }
    free(buffer);
    return;

modified:
    free(buffer);
    exceptionName = (id)ONSInvalidArgumentException;
    reason = (id)OF_STR("NSHashTable was modified while encoding");
raise:
    objc_exception_throw([(id)hashTableExceptionClass exceptionWithName:exceptionName
                                                                 reason:reason
                                                               userInfo:nil]);
}

// Native -[NSConcreteHashTable pointerFunctions], 0x1c051f298. Non-external.
- (id)pointerFunctions {
    ONSConcretePointerFunctions *functions = [ONSConcretePointerFunctions alloc];
    NSSliceInitWithSlice(&functions->slice, &slice);
    return objc_autorelease(functions);
}

// Native -[NSConcreteHashTable countByEnumeratingWithState:objects:count:], 0x1c051f2dc. Non-external. One
// item per call through the state's extra[0]; the previous read is finished (or a mutation reported) first.
- (NSUInteger)countByEnumeratingWithState:(PointerCollectionEnumerationState *)state
                                  objects:(id *)buffer
                                    count:(NSUInteger)length {
    state->mutationsPtr = (unsigned long *)&mutations;
    if (state->state != 0) {
        if (state->extra[1] == mutations) {
            slice.internalProps->finishRead((void *)*state->itemsPtr);
        } else {
            objc_enumerationMutation(self);
        }
    } else {
        state->extra[1] = mutations;
    }
    NSUInteger index = state->state;
    while (index < capacity) {
        void *item = slice.internalProps->startRead(&slice.items[index], NULL);
        index = state->state + 1;
        state->state = index;
        if (item != NULL) {
            state->extra[0] = (unsigned long)item;
            state->itemsPtr = (id *)&state->extra[0];
            return 1;
        }
    }
    return 0;
}

// Native -[NSConcreteHashTable count], 0x1c051f394. Non-external.
- (NSUInteger)count {
    return count;
}

// Native -[NSConcreteHashTable hash], 0x1c051f39c. Non-external. The count, or the receiver itself for weak
// storage.
- (NSUInteger)hash {
    if (slice.internalProps->isWeak != 1) {
        return count;
    }
    return (NSUInteger)self;
}

// Native -[NSConcreteHashTable isEqual:], 0x1c051f3b4. Non-external. A concrete table with the same count and
// isEqual and size functions holding an equal item for each of the receiver's; weak tables are never equal.
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    if (![object isKindOfClass:[ONSConcreteHashTable class]]) {
        return NO;
    }
    if ((slice.internalProps->isWeak & 1) != 0) {
        return NO;
    }
    ONSConcreteHashTable *other = object;
    if (count != other->count) {
        return NO;
    }
    if (slice.personalityProps->isEqual != other->slice.personalityProps->isEqual ||
        slice.personalityProps->size != other->slice.personalityProps->size) {
        return NO;
    }
    if (capacity != 0) {
        NSUInteger offset = 0;
        for (NSUInteger index = 0; index < capacity; ++index) {
            void *item = slice.internalProps->startRead((void **)((char *)slice.items + offset), NULL);
            if (item != NULL) {
                void *otherItem = [other getItem:item];
                if (otherItem == NULL) {
                    slice.internalProps->finishRead(item);
                    return NO;
                }
                BOOL equal = slice.personalityProps->isEqual(item, otherItem, slice.personalityProps->size);
                slice.internalProps->finishRead(item);
                if (!equal) {
                    return NO;
                }
            }
            offset += 8;
        }
    }
    return YES;
}

// Native -[NSConcreteHashTable raiseCountUnderflowException], 0x1c051f4f4. Non-external. The selector
// argument of the format is the receiver (native).
- (void)raiseCountUnderflowException {
    objc_exception_throw([(id)hashTableExceptionClass
        exceptionWithName:(id)ONSInternalInconsistencyException
                   reason:[(id)hashTableStringClass
                              stringWithFormat:(id)OF_STR("*** -[NSHashTable %@] count underflow"), self]
                 userInfo:nil]);
}

// Native -[NSConcreteHashTable rehashAround:], 0x1c051f540. Non-external. From the first never-used slot
// before index, reinserts every item after it up to the next never-used slot (skipping index itself),
// uncounting weak slots read as nil; returns that never-used slot.
- (NSUInteger)rehashAround:(NSUInteger)index {
    NSUInteger currentCapacity = capacity;
    NSUInteger slot = index;
    do {
        slot = (slot - 1) & (currentCapacity - 1);
    } while ((uintptr_t)slice.items[slot] != slice.internalProps->sentinel);
    while (1) {
        NSUInteger mask = currentCapacity - 1;
        do {
            slot = (slot + 1) & mask;
        } while (slot == index);
        bool isEmpty = false;
        void *item = slice.internalProps->startRead(&slice.items[slot], &isEmpty);
        if (item != NULL) {
            slice.internalProps->clear(&slice.items[slot]);
            NSUInteger target = hashProbe(self, item, NULL, NULL, NO);
            slice.internalProps->store(slice.items, target, item);
            slice.internalProps->finishRead(item);
        } else {
            if (isEmpty) {
                return slot;
            }
            slice.internalProps->clear(&slice.items[slot]);
            if (count != 0) {
                count = count - 1;
            } else {
                [self raiseCountUnderflowException];
            }
        }
        currentCapacity = capacity;
    }
}

static NSUInteger
hashProbe(ONSConcreteHashTable *table, const void *item, void **foundItem, BOOL *found, BOOL purge) {
    uint64_t hash = table->slice.personalityProps->hash(item, table->slice.personalityProps->size);
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
    if (found != NULL) {
        *found = NO;
    }
restart:
    for (NSUInteger index = start;; index = (table->capacity - 1) & (index + 1)) {
        bool isEmpty = true;
        void *existing = table->slice.internalProps->startRead(&table->slice.items[index], &isEmpty);
        if (existing != NULL) {
            if (table->slice.personalityProps->isEqual(item, existing, table->slice.personalityProps->size)) {
                ONSSliceInternalProperties *internal = table->slice.internalProps;
                if (foundItem != NULL) {
                    internal->continueRead(existing);
                    *foundItem = existing;
                } else {
                    internal->finishRead(existing);
                }
                if (found != NULL) {
                    *found = YES;
                }
                return index;
            }
            table->slice.internalProps->finishRead(existing);
            continue;
        }
        if (isEmpty) {
            return index;
        }
        if (purge) {
            table->slice.internalProps->clear(&table->slice.items[index]);
            if (table->count != 0) {
                table->count = table->count - 1;
            } else {
                [table raiseCountUnderflowException];
            }
            [table rehashAround:index];
            goto restart;
        }
    }
}

// Native -[NSConcreteHashTable hashGrow], 0x1c051f8ec. Non-external. Doubles the capacity and reinserts every
// readable item into the new store, recounting them, then frees the old store with the old capacity.
- (void)hashGrow {
    void **oldItems = slice.items;
    NSUInteger oldCapacity = capacity;
    capacity = 2 * oldCapacity;
    slice.items = slice.internalProps->allocate(capacity);
    if (slice.items == NULL) {
        objc_exception_throw([(id)hashTableExceptionClass
            exceptionWithName:(id)ONSMallocException
                       reason:(id)OF_STR(
                                  "*** NSHashTable unable to allocate memory -- allocation function failed"
                              )
                     userInfo:nil]);
    }
    count = 0;
    if (oldCapacity != 0) {
        void **slot = oldItems;
        for (NSUInteger remaining = oldCapacity; remaining != 0; --remaining) {
            void *item = slice.internalProps->startRead(slot, NULL);
            if (item != NULL) {
                NSUInteger index = hashProbe(self, item, NULL, NULL, NO);
                slice.internalProps->store(slice.items, index, item);
                slice.internalProps->clear(slot);
                slice.internalProps->finishRead(item);
                ++count;
            }
            ++slot;
        }
    }
    slice.internalProps->free(oldItems, oldCapacity);
}

// Native -[NSConcreteHashTable assign:key:], 0x1c051fa08. Non-external. Counts a mutation, acquires the key
// and stores it at index.
- (void)assign:(NSUInteger)index key:(const void *)key {
    ++mutations;
    ONSSliceExternalAcquisitionProperties *acquisition = slice.acquisitionProps;
    void *(*acquire)(const void *, NSUInteger (*)(const void *), BOOL) = acquisition->acquire;
    void *stored = (void *)key;
    if (acquire != NULL) {
        stored = acquire(key, slice.personalityProps->size, acquisition->copyIn);
    }
    slice.internalProps->store(slice.items, index, stored);
}

// Native -[NSConcreteHashTable rehash], 0x1c051fa6c. Non-external. Reinserts the chains of the used slots in
// the first count slots; stops when a chain wraps behind the scan.
- (void)rehash {
    NSUInteger limit = count;
    if (limit == 0) {
        return;
    }
    NSUInteger index = 0;
    uintptr_t sentinel = slice.internalProps->sentinel;
    do {
        if ((uintptr_t)slice.items[index] == sentinel) {
            ++index;
        } else {
            NSUInteger next = [self rehashAround:index];
            if (next < index) {
                return;
            }
            limit = count;
            index = next;
        }
    } while (index < limit);
}

// Native -[NSConcreteHashTable getItem:], 0x1c051fae0. Non-external.
- (void *)getItem:(const void *)item {
    if (item == NULL) {
        return NULL;
    }
    void *foundItem = NULL;
    hashProbe(self, item, &foundItem, NULL, NO);
    return foundItem;
}

// Native -[NSConcreteHashTable insertItem:], 0x1c051fb4c. Non-external. Stores the item even when an equal
// one exists (replacing it without relinquishing the old one); counts and grows only for a new item.
- (void)insertItem:(const void *)item {
    if (item == NULL) {
        return;
    }
    BOOL found = NO;
    [self assign:hashProbe(self, item, NULL, &found, YES) key:item];
    if (!found) {
        NSUInteger currentCapacity = capacity;
        count = count + 1;
        if (currentCapacity < 2 * count) {
            [self hashGrow];
        }
    }
}

// Native -[NSConcreteHashTable addObject:], 0x1c051fbf0. Non-external. An object equal to one present is not
// added.
- (void)addObject:(id)object {
    if (object == nil) {
        return;
    }
    BOOL found = NO;
    NSUInteger index = hashProbe(self, object, NULL, &found, YES);
    if (!found) {
        [self assign:index key:object];
        NSUInteger currentCapacity = capacity;
        count = count + 1;
        if (currentCapacity < 2 * count) {
            [self hashGrow];
        }
    }
}

// Native -[NSConcreteHashTable removeItem:], 0x1c051fd78. Non-external. The found item is relinquished, its
// slot cleared and uncounted, and its chain reinserted.
- (void)removeItem:(const void *)item {
    if (item == NULL) {
        return;
    }
    void *foundItem = NULL;
    NSUInteger index = hashProbe(self, item, &foundItem, NULL, YES);
    if (foundItem == NULL) {
        return;
    }
    void (*relinquish)(const void *, NSUInteger (*)(const void *)) = slice.acquisitionProps->relinquish;
    if (relinquish != NULL) {
        relinquish(foundItem, slice.personalityProps->size);
    }
    slice.internalProps->clear(&slice.items[index]);
    if (count != 0) {
        count = count - 1;
    } else {
        [self raiseCountUnderflowException];
    }
    ++mutations;
    [self rehashAround:index];
}

// Native -[NSConcreteHashTable removeAllItems], 0x1c051fe50. Non-external.
- (void)removeAllItems {
    empty(&slice, capacity);
    ++mutations;
    count = 0;
}

// Native -[NSConcreteHashTable allObjects], 0x1c051fe88. Non-external.
- (id)allObjects {
    id objects = [(id)hashTableMutableArrayClass array];
    for (id object in self) {
        [objects addObject:object];
    }
    return objects;
}

// Native -[NSConcreteHashTable dealloc], 0x1c052002c. Non-external. Empties the store when it has read and
// clear functions, frees it, then the dynamic slice.
- (void)dealloc {
    ONSSliceInternalProperties *internal = slice.internalProps;
    if (internal != NULL) {
        NSUInteger storeCapacity = capacity;
        if (internal->startRead != NULL && internal->clear != NULL) {
            empty(&slice, capacity);
            internal = slice.internalProps;
        }
        if (internal->free != NULL) {
            internal->free(slice.items, storeCapacity);
        }
        slice.items = NULL;
    }
    if (hasDynamicSlice) {
        NSSliceDealloc(&slice);
    }
    [super dealloc];
}

// Native -[NSConcreteHashTable description], 0x1c05200ec. Non-external. One line per used slot, with the slot
// index.
- (id)description {
    id description = [(id)hashTableMutableStringClass string];
    [description appendString:(id)OF_STR("NSHashTable {\n")];
    if (capacity != 0) {
        NSUInteger offset = 0;
        NSUInteger index = 0;
        do {
            void *item = slice.internalProps->startRead((void **)((char *)slice.items + offset), NULL);
            if (item != NULL) {
                [description
                    appendFormat:(id)OF_STR("[%lu] %@\n"), index, slice.personalityProps->description(item)];
                slice.internalProps->finishRead(item);
            }
            ++index;
            offset += 8;
        } while (index < capacity);
    }
    [description appendString:(id)OF_STR("}\n")];
    return description;
}

@end

// Native raiseError, 0x1c0fdca18 (cold region). Non-external.
static void raiseError(id object, SEL selector) {
    id exception = [(id)hashTableExceptionClass alloc];
    const char *name = sel_getName(selector);
    id reason = [(id)hashTableStringClass
        stringWithFormat:(id)OF_STR("abstract method %s called directly on class %@"), name, [object class]];
    objc_exception_throw([exception initWithName:(id)OF_STR("NSHashTableAbstractImplementationError")
                                          reason:reason
                                        userInfo:nil]);
}

// Native allocateInitialCapacity, 0x1c0fdca90 (cold region). Non-external.
static void allocateInitialCapacity(ONSSlice *slice, NSUInteger capacity) {
    slice->items = slice->internalProps->allocate(capacity);
    if (slice->items == NULL) {
        objc_exception_throw([(id)hashTableExceptionClass
            exceptionWithName:(id)ONSMallocException
                       reason:(id)OF_STR(
                                  "*** NSHashTable unable to allocate memory -- allocation function failed"
                              )
                     userInfo:nil]);
    }
}

// Native empty, 0x1c0fdcaec (cold region). Non-external. Relinquishes each readable item, clears its slot,
// then finishes the read.
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
        slice->internalProps->clear((void **)((char *)slice->items + offset));
        slice->internalProps->finishRead(item);
        offset += 8;
        --remaining;
    } while (remaining != 0);
}
