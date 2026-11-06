//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSDictionary.h"
#import "CoreFoundation/Collections/OCFNSCFDictionary.h"
#import "CoreFoundation/Collections/OCFSortFunctions.h"
#import "CoreFoundation/Objective-C/OCFNSDictionaryEnumerate.h"
#import "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"

#include <malloc/malloc.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the NSDictionary methods (0x1c02825b0-0x1c02871bc) lie together in the iOS Simulator
// __text section, with ___createDictionary (0x1c02867dc) between +dictionaryWithObject:forKey: and
// +dictionaryWithObjects:forKeys:count:, the block invokes after their methods, and the unit's
// OUTLINED_FUNCTION_0_39 (0x1c02871c0: __CFLookUpClass("NSDictionary") with the receiver and selector kept),
// _1_27 (0x1c02871d4: __CFRequireConcreteImplementation of that class, the receiver and the selector) and
// _2_23 (0x1c02871e0: the epilogue) after +supportsSecureCoding and the coding methods; the four abstract
// methods -count, -objectForKey:, -keyEnumerator and -initWithObjects:forKeys:count:
// (0x1c030f198-0x1c030f1f8) lie in the cold region and use those pieces, which are folded into them here.
// Following the owner's decision, the class has its own translation unit. Objective-C compiled with manual
// reference counting (objc_retain, objc_release, objc_autorelease). Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full,
// IDA capture Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json). iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The literal method names the native raises format ("-[NSDictionary selector]"), spelled through the naming
// switch. dictionaryMethodName is a local descriptive name.
#define dictionaryMethodName(kind, selector) kind "[" OF_NAME_STRING(ONSDictionary) " " selector "]"

// libobjc's autorelease pool interfaces, which the native image imports; the SDK installs no declarations.
extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *pool);

// Class symbols the methods load through class references or address directly.
extern char dictionaryArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSArray));
extern char dictionarySetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSSet));
extern char dictionaryMutableSetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableSet));
extern char
    dictionaryObjectEnumeratorClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSDictionaryObjectEnumerator));

// Selectors the methods send that no other visible declaration makes, with Foundation's signatures (objects
// spelled id). Declared for the message sends only; no class adopts the protocol.
@protocol DictionaryMethodMessages

- (id)_initByAdoptingBuffer:(id *)buffer count:(NSUInteger)count size:(NSUInteger)size;
- (id)_stringRepresentation;
- (NSInteger)compare:(id)other;
- (id)array;
- (id)set;
- (id)setWithSet:(id)set;
- (id)allKeys;
- (id)allObjects;
- (BOOL)isEqualToDictionary:(id)dictionary;
- (id)dictionaryWithObjects:(id)objects forKeys:(id)keys;
- (id)initWithObjects:(id)objects forKeys:(id)keys;
- (void)enumerateKeysAndObjectsWithOptions:(NSUInteger)options
                                usingBlock:(void (^)(id key, id object, BOOL *stop))block;
- (id)keyOfEntryWithOptions:(NSUInteger)options
                passingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate;
- (id)keysOfEntriesWithOptions:(NSUInteger)options
                   passingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate;
- (id)keysSortedByValueWithOptions:(NSUInteger)options usingComparator:(NSInteger (^)(id, id))comparator;

@end

// The temporary object-buffer raise the methods inline when a count does not fit the buffer (the top four
// bits set): the reason formatted with the default allocator, +[NSException
// exceptionWithName:reason:userInfo:] with NSGenericException and no user info, the reason released, the
// exception thrown. raiseTemporaryBufferException is a local descriptive name.
static __attribute__((noreturn)) void raiseTemporaryBufferException(NSUInteger count) {
    OCFStringRef reason = OCFStringCreateWithFormat(
        NULL,
        NULL,
        OF_STR(
            "*** attempt to create a temporary id buffer which is too large or with a negative count (%lu) "
            "-- "
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

// The proem-formatted invalid-argument raise: the reason formatted with kCFAllocatorSystemDefault (loaded
// before __CFExceptionProem) from the format and the "-[Class selector]" proem, autoreleased through
// _CFAutoreleasePoolAddObject, +[NSException exceptionWithName:reason:userInfo:] with
// NSInvalidArgumentException and no user info, thrown. raiseWithProem is a local descriptive name.
static __attribute__((noreturn)) void raiseWithProem(id object, SEL selector, OCFStringRef format) {
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef reason =
        OCFStringCreateWithFormat(allocator, NULL, format, __CFExceptionProem(object, selector));
    id exception = [(id)dictionaryExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                            reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                                          userInfo:nil];
    objc_exception_throw(exception);
}

@implementation ONSDictionary

// Native -[NSDictionary _cfTypeID], 0x1c02825b0. Non-external. The CFDictionary type ID, 18.
- (OCFTypeID)_cfTypeID {
    return 0x12;
}

// Native -[NSDictionary isNSDictionary__], 0x1c02825b8. Non-external.
- (BOOL)isNSDictionary__ {
    return YES;
}

// Native -[NSDictionary allKeysForObject:], 0x1c02825c0. Non-external. The keys whose objects are identical
// to or -isEqual: (sent to the argument) the object, collected by fast enumeration into a stack buffer of the
// count (at least one) or, from 257 entries, _CFCreateArrayStorage; a heap buffer is retained element by
// element and adopted by the array, a stack buffer copied by -initWithObjects:count:.
- (id)allKeysForObject:(id)anObject {
    dictionaryTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferException(count);
    }
    NSUInteger size = count > 1 ? count : 1;
    id *keys = alloca((count < 0x101 ? size : 1) * sizeof(id));
    size_t storageCount = 0;
    id *storage = NULL;
    if (count >= 0x101) {
        keys = _OCFCreateArrayStorage(size, false, &storageCount);
        storage = keys;
    }
    id *cursor = keys;
    for (id key in self) {
        id object = [self objectForKey:key];
        if (object == anObject || [anObject isEqual:object]) {
            *cursor++ = key;
        }
    }
    OCFIndex found = cursor - keys;
    id array;
    if (storage != NULL) {
        for (OCFIndex index = 0; index < found; index++) {
            [storage[index] retain];
        }
        array = [[(Class)dictionaryArrayClass alloc] _initByAdoptingBuffer:storage count:found size:found];
    } else {
        array = [[(Class)dictionaryArrayClass alloc] initWithObjects:keys count:found];
    }
    return [array autorelease];
}

// Native -[NSDictionary allKeys], 0x1c0282870. Non-external. The keys through -getObjects:andKeys:count:
// into _CFCreateArrayStorage of the count (at least one), retained and adopted by the array; a NULL storage
// makes -initWithObjects:NULL count:.
- (id)allKeys {
    dictionaryTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferException(count);
    }
    size_t storageCount = 0;
    id *keys = _OCFCreateArrayStorage(count > 1 ? count : 1, false, &storageCount);
    [self getObjects:NULL andKeys:keys count:count];
    id array;
    if (keys != NULL) {
        for (NSUInteger index = 0; index < count; index++) {
            [keys[index] retain];
        }
        array = [[(Class)dictionaryArrayClass alloc] _initByAdoptingBuffer:keys count:count size:count];
    } else {
        array = [[(Class)dictionaryArrayClass alloc] initWithObjects:NULL count:count];
    }
    return [array autorelease];
}

// Native -[NSDictionary allObjects], 0x1c0282a14. Non-external. -allKeys with the objects.
- (id)allObjects {
    dictionaryTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferException(count);
    }
    size_t storageCount = 0;
    id *objects = _OCFCreateArrayStorage(count > 1 ? count : 1, false, &storageCount);
    [self getObjects:objects andKeys:NULL count:count];
    id array;
    if (objects != NULL) {
        for (NSUInteger index = 0; index < count; index++) {
            [objects[index] retain];
        }
        array = [[(Class)dictionaryArrayClass alloc] _initByAdoptingBuffer:objects count:count size:count];
    } else {
        array = [[(Class)dictionaryArrayClass alloc] initWithObjects:NULL count:count];
    }
    return [array autorelease];
}

// Native -[NSDictionary allValues], 0x1c0282bb8. Non-external. The body of -allObjects instruction for
// instruction.
- (id)allValues {
    dictionaryTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferException(count);
    }
    size_t storageCount = 0;
    id *objects = _OCFCreateArrayStorage(count > 1 ? count : 1, false, &storageCount);
    [self getObjects:objects andKeys:NULL count:count];
    id array;
    if (objects != NULL) {
        for (NSUInteger index = 0; index < count; index++) {
            [objects[index] retain];
        }
        array = [[(Class)dictionaryArrayClass alloc] _initByAdoptingBuffer:objects count:count size:count];
    } else {
        array = [[(Class)dictionaryArrayClass alloc] initWithObjects:NULL count:count];
    }
    return [array autorelease];
}

// Native -[NSDictionary containsKey:], 0x1c0282d5c. Non-external. A nil key answers NO after the check.
- (BOOL)containsKey:(id)key {
    dictionaryTSANRead(self, __builtin_return_address(0));
    if (key == nil) {
        return NO;
    }
    return [self objectForKey:key] != nil;
}

// Native -[NSDictionary containsObject:], 0x1c0282dc8. Non-external. Fast enumeration of the keys; an object
// identical to or -isEqual: (sent to the argument) the argument answers YES.
- (BOOL)containsObject:(id)anObject {
    dictionaryTSANRead(self, __builtin_return_address(0));
    for (id key in self) {
        id object = [self objectForKey:key];
        if (object == anObject || [anObject isEqual:object]) {
            return YES;
        }
    }
    return NO;
}

// Native -[NSDictionary countByEnumeratingWithState:objects:count:], 0x1c0282efc. Non-external. Order: a NULL
// buffer with a nonzero length, then a length with any of the top three bits set, raise (os_log pack of 22
// bytes, the method name and the length); a state of -1 answers 0 before the read check. The first call
// records the count in extra[0] (mutationsPtr points there) and the key enumerator in extra[1] (none answers
// 0 without updating the state); each call takes up to the remaining count of -nextObject results, a nil
// result marking the state -1 and answering the number taken; otherwise the state advances, becoming -1 once
// it reaches the count.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    DictionaryEnumerationState *enumerationState = state;
    if (buffer == NULL && length != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            dictionaryMethodName("-", "countByEnumeratingWithState:objects:count:"),
            length
        );
    }
    if ((length >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            dictionaryMethodName("-", "countByEnumeratingWithState:objects:count:"),
            length
        );
    }
    if (enumerationState->state == (unsigned long)-1) {
        return 0;
    }
    dictionaryTSANRead(self, __builtin_return_address(0));
    id enumerator;
    if (enumerationState->state == 0) {
        enumerationState->mutationsPtr = &enumerationState->extra[0];
        enumerationState->extra[0] = [self count];
        enumerator = [self keyEnumerator];
        enumerationState->extra[1] = (unsigned long)enumerator;
        if (enumerator == nil) {
            return 0;
        }
    } else {
        enumerator = (id)enumerationState->extra[1];
    }
    enumerationState->itemsPtr = (void **)buffer;
    unsigned long total = enumerationState->extra[0];
    unsigned long remaining = total - enumerationState->state;
    NSUInteger taken = length < remaining ? length : remaining;
    for (NSUInteger index = 0; index < taken; index++) {
        id key = [enumerator nextObject];
        if (key == nil) {
            enumerationState->state = (unsigned long)-1;
            return index;
        }
        buffer[index] = key;
    }
    unsigned long next = enumerationState->state + taken;
    enumerationState->state = total > next ? next : (unsigned long)-1;
    return taken;
}

// Native -[NSDictionary countForKey:], 0x1c028319c. Non-external. 1 when the key has an object, else 0.
- (NSUInteger)countForKey:(id)key {
    dictionaryTSANRead(self, __builtin_return_address(0));
    return [self objectForKey:key] != nil;
}

// Native -[NSDictionary countForObject:], 0x1c02831fc. Non-external. The number of objects identical to or
// -isEqual: (sent to the argument) the argument.
- (NSUInteger)countForObject:(id)anObject {
    dictionaryTSANRead(self, __builtin_return_address(0));
    NSUInteger found = 0;
    for (id key in self) {
        id object = [self objectForKey:key];
        if (object == anObject || [anObject isEqual:object]) {
            found++;
        }
    }
    return found;
}

// Native -[NSDictionary descriptionWithLocale:indent:], 0x1c0283344, with its two block invokes (0x1c02839f0,
// 0x1c0283a00). Non-external. No read check. The level is clamped to 100. The keys come from
// -getObjects:andKeys:count: into a stack buffer (up to 256 entries) or _CFCreateArrayStorage; when every key
// is an NSString responding to the selector the native code loads from 0x2387adb3c (outside the extracted
// image: compare:, the selector the sort block sends; confidence: medium), CFSortIndexes orders them through
// -compare: (indexes on the stack up to 4096 entries, else malloc_type_malloc with type ID
// 0x100004000313F17), otherwise they keep their order. Inside an autorelease pool (objc_autoreleasePoolPush),
// each key, then its object (-objectForKey: first), is described: an NSString by -_stringRepresentation; an
// NSDictionary or NSArray by -descriptionWithLocale:indent: with the clamped level plus one; NSData by
// -description; anything else by -description through the second block, which records once whether NSString
// is present (objc_lookUpClass) and, for executables linked on or after version 6 with NSString present,
// answers its
// -_stringRepresentation; nil becomes "(null)". The text is "{\n", then "<indent>    key = value;\n" per
// entry, then "<indent>}", the indent being four spaces per clamped level (none for level 0), built with
// kCFAllocatorSystemDefault and autoreleased after the pool pops.
- (id)descriptionWithLocale:(id)locale indent:(NSUInteger)level {
    NSUInteger clamped = level < 100 ? level : 100;
    NSUInteger count = [self count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferException(count);
    }
    NSUInteger size = count > 1 ? count : 1;
    id *keys = alloca((count < 0x101 ? size : 1) * sizeof(id));
    size_t keysCount = 0;
    size_t valuesCount = 0;
    size_t rawKeysCount = 0;
    id *values;
    id *rawKeys;
    id *heapKeys = NULL;
    id *heapValues = NULL;
    id *heapRawKeys = NULL;
    if (count <= 0x100) {
        values = alloca(size * sizeof(id));
        rawKeys = alloca(size * sizeof(id));
    } else {
        keys = _OCFCreateArrayStorage(size, false, &keysCount);
        values = _OCFCreateArrayStorage(size, false, &valuesCount);
        heapRawKeys = _OCFCreateArrayStorage(size, false, &rawKeysCount);
        rawKeys = heapRawKeys;
        heapValues = values;
        heapKeys = keys;
    }
    [self getObjects:NULL andKeys:rawKeys count:count];
    NSUInteger checked = 0;
    while (checked < count) {
        id key = rawKeys[checked];
        if (!_ONSIsNSString(key) || ![key respondsToSelector:@selector(compare:)]) {
            break;
        }
        checked++;
    }
    if (checked < count) {
        memmove(keys, rawKeys, count * sizeof(id));
    } else {
        OCFIndex *stackIndexes = alloca((count > 0x1000 ? 1 : count) * sizeof(OCFIndex));
        OCFIndex *indexes = stackIndexes;
        if (count > 0x1000) {
            indexes = malloc_type_malloc(count * sizeof(OCFIndex), 0x100004000313F17);
        }
        OCFSortIndexes(indexes, count, 0, ^OCFComparisonResult(OCFIndex first, OCFIndex second) {
          return [rawKeys[first] compare:rawKeys[second]];
        });
        for (NSUInteger index = 0; index < count; index++) {
            keys[index] = rawKeys[indexes[index]];
        }
        if (stackIndexes != indexes) {
            free(indexes);
        }
    }
    free(heapRawKeys);
    __block int stringPresent = -1;
    id (^describe)(id) = ^id(id description) {
      if (stringPresent == -1) {
          stringPresent = objc_lookUpClass(OF_NAME_STRING(ONSString)) != Nil;
      }
      if (_OCFExecutableLinkedOnOrAfter(6) && stringPresent != 0) {
          return [description _stringRepresentation];
      }
      return description;
    };
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableStringRef string = OCFStringCreateMutable(allocator, 0);
    void *pool = objc_autoreleasePoolPush();
    for (NSUInteger index = 0; index < count; index++) {
        id key = keys[index];
        id object = [self objectForKey:key];
        id keyDescription;
        if (_ONSIsNSString(key)) {
            keyDescription = [key _stringRepresentation];
        } else if (_ONSIsNSDictionary(key) || _ONSIsNSArray(key)) {
            keyDescription = [key descriptionWithLocale:locale indent:clamped + 1];
        } else if (_ONSIsNSData(key)) {
            keyDescription = [key description];
        } else {
            keyDescription = describe([key description]);
        }
        id objectDescription;
        if (_ONSIsNSString(object)) {
            objectDescription = [object _stringRepresentation];
        } else if (_ONSIsNSDictionary(object) || _ONSIsNSArray(object)) {
            objectDescription = [object descriptionWithLocale:locale indent:clamped + 1];
        } else if (_ONSIsNSData(object)) {
            objectDescription = [object description];
        } else {
            objectDescription = describe([object description]);
        }
        keys[index] = keyDescription != nil ? keyDescription : (id)OF_STR("(null)");
        values[index] = objectDescription != nil ? objectDescription : (id)OF_STR("(null)");
    }
    OCFMutableStringRef indent = OCFStringCreateMutable(allocator, 0);
    if (level != 0) {
        do {
            OCFStringAppend(indent, OF_STR("    "));
        } while (--clamped != 0);
    }
    OCFStringAppend(string, indent);
    OCFStringAppend(string, OF_STR("{\n"));
    for (NSUInteger index = 0; index < count; index++) {
        OCFStringAppend(string, indent);
        OCFStringAppend(string, OF_STR("    "));
        OCFStringAppend(string, (OCFStringRef)keys[index]);
        OCFStringAppend(string, OF_STR(" = "));
        OCFStringAppend(string, (OCFStringRef)values[index]);
        OCFStringAppend(string, OF_STR(";\n"));
    }
    OCFStringAppend(string, indent);
    OCFStringAppend(string, OF_STR("}"));
    OCFRelease(indent);
    objc_autoreleasePoolPop(pool);
    id result = [(id)string autorelease];
    free(heapValues);
    free(heapKeys);
    return result;
}

// Native -[NSDictionary descriptionWithLocale:], 0x1c0283a84. Non-external.
- (id)descriptionWithLocale:(id)locale {
    dictionaryTSANRead(self, __builtin_return_address(0));
    return [self descriptionWithLocale:locale indent:0];
}

// Native -[NSDictionary description], 0x1c0283adc. Non-external.
- (id)description {
    dictionaryTSANRead(self, __builtin_return_address(0));
    return [self descriptionWithLocale:nil indent:0];
}

// Native -[NSDictionary enumerateKeysAndObjectsWithOptions:usingBlock:], 0x1c0283b30. Non-external. A nil
// block raises before the read check; then __NSDictionaryParameterCheckIterate and __NSDictionaryEnumerate
// with the options less bit 1 (passed to both).
- (void)enumerateKeysAndObjectsWithOptions:(NSUInteger)options
                                usingBlock:(void (^)(id key, id object, BOOL *stop))block {
    if (block == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            dictionaryMethodName("-", "enumerateKeysAndObjectsWithOptions:usingBlock:")
        );
    }
    dictionaryTSANRead(self, __builtin_return_address(0));
    __NSDictionaryParameterCheckIterate(self, _cmd, block, options & ~(NSUInteger)2);
    __NSDictionaryEnumerate(self, options & ~(NSUInteger)2, block);
}

// Native -[NSDictionary enumerateKeysAndObjectsUsingBlock:], 0x1c0283ca0. Non-external.
- (void)enumerateKeysAndObjectsUsingBlock:(void (^)(id key, id object, BOOL *stop))block {
    if (block == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            dictionaryMethodName("-", "enumerateKeysAndObjectsUsingBlock:")
        );
    }
    dictionaryTSANRead(self, __builtin_return_address(0));
    [self enumerateKeysAndObjectsWithOptions:0 usingBlock:block];
}

// Native -[NSDictionary getObjects:andKeys:count:], 0x1c0283df4. Non-external. A count with any of the top
// three bits set raises when the objects buffer, then when the keys buffer, is given; then fast enumeration
// stores up to count keys (when wanted) and their -objectForKey: results (when wanted), keys first.
- (void)getObjects:(id *)objects andKeys:(id *)keys count:(NSUInteger)count {
    if (objects != NULL && (count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            dictionaryMethodName("-", "getObjects:andKeys:count:"),
            count
        );
    }
    if (keys != NULL && (count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            dictionaryMethodName("-", "getObjects:andKeys:count:"),
            count
        );
    }
    dictionaryTSANRead(self, __builtin_return_address(0));
    NSUInteger index = 0;
    for (id key in self) {
        if (index == count) {
            return;
        }
        if (keys != NULL) {
            *keys++ = key;
        }
        if (objects != NULL) {
            *objects++ = [self objectForKey:key];
        }
        index++;
    }
}

// Native -[NSDictionary _getObjects:andKeys:count:], 0x1c0284048. Non-external.
- (void)_getObjects:(id *)objects andKeys:(id *)keys count:(NSUInteger)count {
    [self getObjects:objects andKeys:keys count:count];
}

// Native -[NSDictionary getObjects:andKeys:], 0x1c028404c. Non-external. The count's top three bits raise
// when the objects buffer, then when the keys buffer, is given.
- (void)getObjects:(id *)objects andKeys:(id *)keys {
    dictionaryTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (objects != NULL && (count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            dictionaryMethodName("-", "getObjects:andKeys:"),
            count
        );
    }
    if (keys != NULL && (count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            dictionaryMethodName("-", "getObjects:andKeys:"),
            count
        );
    }
    [self getObjects:objects andKeys:keys count:count];
}

// Native -[NSDictionary getKeys:], 0x1c028420c. Non-external.
- (void)getKeys:(id *)keys {
    dictionaryTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (keys != NULL && (count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            dictionaryMethodName("-", "getKeys:"),
            count
        );
    }
    [self getObjects:NULL andKeys:keys count:count];
}

// Native -[NSDictionary getObjects:], 0x1c0284384. Non-external.
- (void)getObjects:(id *)objects {
    dictionaryTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (objects != NULL && (count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            dictionaryMethodName("-", "getObjects:"),
            count
        );
    }
    [self getObjects:objects andKeys:NULL count:count];
}

// Native -[NSDictionary hash], 0x1c02844fc. Non-external. The count.
- (NSUInteger)hash {
    dictionaryTSANRead(self, __builtin_return_address(0));
    return [self count];
}

// Native -[NSDictionary invertedDictionary], 0x1c0284548. Non-external. +[NSDictionary
// dictionaryWithObjects:forKeys:] with -allKeys as the objects and -allObjects as the keys (-allKeys sent
// first).
- (id)invertedDictionary {
    dictionaryTSANRead(self, __builtin_return_address(0));
    id keys = [self allKeys];
    id objects = [self allObjects];
    return [(id)[ONSDictionary class] dictionaryWithObjects:keys forKeys:objects];
}

// Native -[NSDictionary isEqualToDictionary:], 0x1c02845c0. Non-external. A non-nil argument that is not an
// NSDictionary raises before the read check. Identical answers YES, nil NO; different counts NO; then for
// every key the argument's object, then the receiver's: both nil continues, one nil answers NO, otherwise the
// receiver's object's -isEqual: decides.
- (BOOL)isEqualToDictionary:(id)otherDictionary {
    void *returnAddress = __builtin_return_address(0);
    if (otherDictionary != nil && !_ONSIsNSDictionary(otherDictionary)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: dictionary argument is not an NSDictionary",
            dictionaryMethodName("-", "isEqualToDictionary:")
        );
    }
    dictionaryTSANRead(self, returnAddress);
    if (otherDictionary == self) {
        return YES;
    }
    if (otherDictionary == nil) {
        return NO;
    }
    NSUInteger count = [self count];
    if ([otherDictionary count] != count) {
        return NO;
    }
    for (id key in self) {
        id otherObject = [otherDictionary objectForKey:key];
        id object = [self objectForKey:key];
        if (otherObject == nil && object == nil) {
            continue;
        }
        if (otherObject == nil || object == nil) {
            return NO;
        }
        if (![object isEqual:otherObject]) {
            return NO;
        }
    }
    return YES;
}

// Native -[NSDictionary isEqual:], 0x1c0284818. Non-external.
- (BOOL)isEqual:(id)object {
    dictionaryTSANRead(self, __builtin_return_address(0));
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    if (!_ONSIsNSDictionary(object)) {
        return NO;
    }
    return [self isEqualToDictionary:object];
}

// Native -[NSDictionary keyOfEntryWithOptions:passingTest:], 0x1c028489c, with its block invoke
// (0x1c0284a4c). Non-external. A nil predicate raises before the read check. The block captures the predicate
// and the address of the result: a passing entry stores its key and stops; a stop the predicate sets also
// stops. The result is answered unretained.
- (id)keyOfEntryWithOptions:(NSUInteger)options
                passingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate {
    if (predicate == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: predicate cannot be nil",
            dictionaryMethodName("-", "keyOfEntryWithOptions:passingTest:")
        );
    }
    dictionaryTSANRead(self, __builtin_return_address(0));
    __NSDictionaryParameterCheckIterate(self, _cmd, predicate, options & ~(NSUInteger)2);
    id result = nil;
    id *resultPointer = &result;
    __NSDictionaryEnumerate(self, options & ~(NSUInteger)2, ^(id key, id object, BOOL *stop) {
      BOOL predicateStop = NO;
      if (predicate(key, object, &predicateStop)) {
          *resultPointer = key;
          *stop = YES;
      }
      if (predicateStop) {
          *stop = YES;
      }
    });
    return result;
}

// Native -[NSDictionary keyOfEntryPassingTest:], 0x1c0284ae4. Non-external.
- (id)keyOfEntryPassingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate {
    if (predicate == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: predicate cannot be nil",
            dictionaryMethodName("-", "keyOfEntryPassingTest:")
        );
    }
    dictionaryTSANRead(self, __builtin_return_address(0));
    return [self keyOfEntryWithOptions:0 passingTest:predicate];
}

// Native -[NSDictionary keysOfEntriesWithOptions:passingTest:], 0x1c0284c38, with its block invoke
// (0x1c0284e20). Non-external. Passing keys are added to +[NSMutableSet set] under a stack mutex
// (PTHREAD_MUTEX_INITIALIZER; never destroyed); the answer is +[NSSet setWithSet:] of it.
- (id)keysOfEntriesWithOptions:(NSUInteger)options
                   passingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate {
    if (predicate == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: predicate cannot be nil",
            dictionaryMethodName("-", "keysOfEntriesWithOptions:passingTest:")
        );
    }
    dictionaryTSANRead(self, __builtin_return_address(0));
    __NSDictionaryParameterCheckIterate(self, _cmd, predicate, options & ~(NSUInteger)2);
    id set = [(id)dictionaryMutableSetClass set];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t *mutexPointer = &mutex;
    __NSDictionaryEnumerate(self, options & ~(NSUInteger)2, ^(id key, id object, BOOL *stop) {
      if (predicate(key, object, stop)) {
          pthread_mutex_lock(mutexPointer);
          [set addObject:key];
          pthread_mutex_unlock(mutexPointer);
      }
    });
    return [(id)dictionarySetClass setWithSet:set];
}

// Native -[NSDictionary keysOfEntriesPassingTest:], 0x1c0284e74. Non-external.
- (id)keysOfEntriesPassingTest:(BOOL (^)(id key, id object, BOOL *stop))predicate {
    if (predicate == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: predicate cannot be nil",
            dictionaryMethodName("-", "keysOfEntriesPassingTest:")
        );
    }
    dictionaryTSANRead(self, __builtin_return_address(0));
    return [self keysOfEntriesWithOptions:0 passingTest:predicate];
}

// The free of the native exception cleanups (landing pads 0x1c02853b0-0x1c02853fc) of
// -keysSortedByValueWithOptions:usingComparator:. freeBuffer is a local descriptive name.
static void freeBuffer(id **buffer) {
    free(*buffer);
}

// Native -[NSDictionary keysSortedByValueWithOptions:usingComparator:], 0x1c0284fc8, with its block invoke
// (0x1c0285400). Non-external. A nil comparator raises before the read check; an empty dictionary answers
// +[NSArray array]. Keys and objects come from -getObjects:andKeys:count: (stack buffers up to 256 entries,
// else _CFCreateArrayStorage), CFSortIndexes orders the indexes with the full options through the comparator
// applied to the objects (indexes on the stack up to 4096 entries, else malloc_type_malloc with type ID
// 0x100004000313F17), and the sorted keys, in a _CFCreateArrayStorage buffer, are retained and adopted by the
// array. The heap buffers are freed on the normal path and by the exception cleanups.
- (id)keysSortedByValueWithOptions:(NSUInteger)options usingComparator:(NSInteger (^)(id, id))comparator {
    if (comparator == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: comparator cannot be nil",
            dictionaryMethodName("-", "keysSortedByValueWithOptions:usingComparator:")
        );
    }
    dictionaryTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (count == 0) {
        return [(id)dictionaryArrayClass array];
    }
    if ((count >> 60) != 0) {
        raiseTemporaryBufferException(count);
    }
    id *keys = alloca((count < 0x101 ? count : 1) * sizeof(id));
    size_t keysCount = 0;
    size_t valuesCount = 0;
    size_t sortedCount = 0;
    __attribute__((cleanup(freeBuffer))) id *heapKeys = NULL;
    __attribute__((cleanup(freeBuffer))) id *heapValues = NULL;
    id *stackValues;
    if (count <= 0x100) {
        stackValues = alloca(count * sizeof(id));
    } else {
        keys = _OCFCreateArrayStorage(count, false, &keysCount);
        stackValues = alloca(2 * sizeof(id));
        heapKeys = keys;
        heapValues = _OCFCreateArrayStorage(count, false, &valuesCount);
    }
    id *values = count < 0x101 ? stackValues : heapValues;
    __attribute__((cleanup(freeBuffer))) id *sorted = _OCFCreateArrayStorage(count, false, &sortedCount);
    [self getObjects:values andKeys:keys count:count];
    OCFIndex *stackIndexes = alloca((count > 0x1000 ? 1 : count) * sizeof(OCFIndex));
    OCFIndex *indexes = stackIndexes;
    if (count > 0x1000) {
        indexes = malloc_type_malloc(count * sizeof(OCFIndex), 0x100004000313F17);
    }
    OCFSortIndexes(indexes, count, options, ^OCFComparisonResult(OCFIndex first, OCFIndex second) {
      return comparator(values[first], values[second]);
    });
    NSUInteger index = 0;
    do {
        sorted[index] = keys[indexes[index]];
        index++;
    } while (count != index);
    if (stackIndexes != indexes) {
        free(indexes);
    }
    index = 0;
    do {
        [sorted[index] retain];
        index++;
    } while (count != index);
    id array = [[(Class)dictionaryArrayClass alloc] _initByAdoptingBuffer:sorted count:count size:count];
    sorted = NULL;
    return [array autorelease];
}

// Native -[NSDictionary keysSortedByValueUsingComparator:], 0x1c0285418. Non-external.
- (id)keysSortedByValueUsingComparator:(NSInteger (^)(id, id))comparator {
    if (comparator == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: comparator cannot be nil",
            dictionaryMethodName("-", "keysSortedByValueUsingComparator:")
        );
    }
    dictionaryTSANRead(self, __builtin_return_address(0));
    return [self keysSortedByValueWithOptions:0 usingComparator:comparator];
}

// Native -[NSDictionary keysSortedByValueUsingSelector:], 0x1c028556c, with its block invoke (0x1c0285644).
// Non-external. A NULL selector is sent to -doesNotRecognizeSelector: (before the read check); the block
// sends the selector to the first object with the second.
- (id)keysSortedByValueUsingSelector:(SEL)comparator {
    void *returnAddress = __builtin_return_address(0);
    if (comparator == NULL) {
        [self doesNotRecognizeSelector:comparator];
    }
    dictionaryTSANRead(self, returnAddress);
    return [self keysSortedByValueWithOptions:0
                              usingComparator:^NSInteger(id first, id second) {
                                return ((NSInteger (*)(id, SEL, id))objc_msgSend)(first, comparator, second);
                              }];
}

// Native -[NSDictionary objectEnumerator], 0x1c0285654. Non-external. An autoreleased
// __NSDictionaryObjectEnumerator over the receiver.
- (id)objectEnumerator {
    dictionaryTSANRead(self, __builtin_return_address(0));
    return [[[(Class)dictionaryObjectEnumeratorClass alloc] initWithObject:self] autorelease];
}

// Native -[NSDictionary objectForKeyedSubscript:], 0x1c02856b0. Non-external.
- (id)objectForKeyedSubscript:(id)key {
    dictionaryTSANRead(self, __builtin_return_address(0));
    return [self objectForKey:key];
}

// Native -[NSDictionary objectsForKeys:notFoundMarker:], 0x1c0285704. Non-external. A nil marker raises
// first; the keys' -count sizes the buffer (stack up to 256 entries, else _CFCreateArrayStorage) before the
// read check; fast enumeration of the keys stores each -objectForKey: result or the marker.
- (id)objectsForKeys:(id)keys notFoundMarker:(id)marker {
    void *returnAddress = __builtin_return_address(0);
    if (marker == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: marker cannot be nil",
            dictionaryMethodName("-", "objectsForKeys:notFoundMarker:")
        );
    }
    NSUInteger count = [keys count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferException(count);
    }
    NSUInteger size = count > 1 ? count : 1;
    id *objects = alloca((count < 0x101 ? size : 1) * sizeof(id));
    size_t storageCount = 0;
    id *storage = NULL;
    if (count >= 0x101) {
        objects = _OCFCreateArrayStorage(size, false, &storageCount);
        storage = objects;
    }
    dictionaryTSANRead(self, returnAddress);
    id *cursor = objects;
    for (id key in keys) {
        id object = [self objectForKey:key];
        *cursor++ = object != nil ? object : marker;
    }
    OCFIndex found = cursor - objects;
    id array;
    if (storage != NULL) {
        for (OCFIndex index = 0; index < found; index++) {
            [storage[index] retain];
        }
        array = [[(Class)dictionaryArrayClass alloc] _initByAdoptingBuffer:storage count:found size:found];
    } else {
        array = [[(Class)dictionaryArrayClass alloc] initWithObjects:objects count:found];
    }
    return [array autorelease];
}

// Native -[NSDictionary __apply:context:], 0x1c0285a70, with its block invoke (0x1c0285b54). Non-external. A
// NULL function raises "%@: function pointer is NULL"; the block calls the function with the key, the object
// and the context through -enumerateKeysAndObjectsWithOptions:usingBlock: (options 0).
- (void)__apply:(void (*)(const void *key, const void *value, void *context))applier context:(void *)context {
    if (applier == NULL) {
        raiseWithProem(self, _cmd, OF_STR("%@: function pointer is NULL"));
    }
    [self enumerateKeysAndObjectsWithOptions:0
                                  usingBlock:^(id key, id object, BOOL *stop) {
                                    (void)stop;
                                    applier(key, object, context);
                                  }];
}

// Native -[NSDictionary __getValue:forKey:], 0x1c0285b68. Non-external. Stores a found object when the
// pointer is given.
- (BOOL)__getValue:(id *)value forKey:(id)key {
    id object = [self objectForKey:key];
    if (value != NULL && object != nil) {
        *value = object;
    }
    return object != nil;
}

// Native +[NSDictionary allocWithZone:], 0x1c0285ba0. Non-external. NSDictionary itself answers the immutable
// placeholder, NSMutableDictionary the mutable one (the classes compared by their class references);
// subclasses go to super.
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == [ONSDictionary class]) {
        return __NSDictionaryImmutablePlaceholder();
    }
    if (self == [ONSMutableDictionary class]) {
        return __NSDictionaryMutablePlaceholder();
    }
    return [super allocWithZone:zone];
}

// Native -[NSDictionary initWithDictionary:copyItems:], 0x1c0285c68. Non-external. A non-nil argument that is
// not an NSDictionary raises. Keys and objects come from the argument's -getObjects:andKeys:count: (stack
// buffers up to 256 entries, else _CFCreateArrayStorage); with copyItems exactly 1 (and a nonzero count) each
// key, then its object, is replaced by -copyWithZone:NULL; after -initWithObjects:forKeys:count:, a nonzero
// copyItems (and count) releases each key and then its object.
- (id)initWithDictionary:(id)otherDictionary copyItems:(BOOL)flag {
    if (otherDictionary != nil && !_ONSIsNSDictionary(otherDictionary)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: dictionary argument is not an NSDictionary",
            dictionaryMethodName("-", "initWithDictionary:copyItems:")
        );
    }
    NSUInteger count = [otherDictionary count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferException(count);
    }
    NSUInteger size = count > 1 ? count : 1;
    id *keys = alloca((count < 0x101 ? size : 1) * sizeof(id));
    size_t keysCount = 0;
    size_t objectsCount = 0;
    id *objects;
    id *heapKeys = NULL;
    id *heapObjects = NULL;
    if (count <= 0x100) {
        objects = alloca(size * sizeof(id));
    } else {
        heapKeys = _OCFCreateArrayStorage(size, false, &keysCount);
        heapObjects = _OCFCreateArrayStorage(size, false, &objectsCount);
        keys = heapKeys;
        objects = heapObjects;
    }
    [otherDictionary getObjects:objects andKeys:keys count:count];
    BOOL copyItems = count == 0 ? NO : flag;
    if (copyItems == 1) {
        for (NSUInteger index = 0; index < count; index++) {
            keys[index] = [keys[index] copyWithZone:NULL];
            objects[index] = [objects[index] copyWithZone:NULL];
        }
    }
    id result = [self initWithObjects:objects forKeys:keys count:count];
    if (copyItems) {
        for (NSUInteger index = 0; index < count; index++) {
            [keys[index] release];
            [objects[index] release];
        }
    }
    free(heapObjects);
    free(heapKeys);
    return result;
}

// Native -[NSDictionary initWithDictionary:], 0x1c0285f88. Non-external.
- (id)initWithDictionary:(id)otherDictionary {
    return [self initWithDictionary:otherDictionary copyItems:NO];
}

// Native -[NSDictionary initWithObject:forKey:], 0x1c0285f90. Non-external.
- (id)initWithObject:(id)object forKey:(id)key {
    id objects[1] = {object};
    id keys[1] = {key};
    return [self initWithObjects:objects forKeys:keys count:1];
}

// Native -[NSDictionary initWithObjectsAndKeys:], 0x1c0285fe8. Non-external. The variadic arguments after the
// first object are counted up to and including the terminating nil; an odd number raises "second object of
// each pair". When -class is NSDictionary (compared with its class reference): no arguments answer the
// __NSDictionary0__ singleton (the receiver is neither released nor the singleton retained); otherwise the
// receiver is released and __NSDictionaryI_new builds from the first object and the argument list (options
// 1, copy the keys). Other classes copy the pairs (objects then keys) into stack buffers (up to 256 pairs,
// else _CFCreateArrayStorage; a count of 2^61 - 1 or more arguments raises) for
// -initWithObjects:forKeys:count:.
- (id)initWithObjectsAndKeys:(id)firstObject, ... {
    va_list arguments;
    va_start(arguments, firstObject);
    id first = firstObject;
    Class cls = [self class];
    NSUInteger pairs;
    if (firstObject != nil) {
        NSUInteger total = 0;
        va_list counting;
        va_copy(counting, arguments);
        id argument;
        do {
            argument = va_arg(counting, id);
            total++;
        } while (argument != nil);
        va_end(counting);
        if ((total & 1) != 0) {
            raiseWithProem(
                self,
                _cmd,
                OF_STR(
                    "%@: second object of each pair must be non-nil.  Or, did you forget to nil-terminate "
                    "your parameter list?"
                )
            );
        }
        pairs = total >> 1;
        if (cls == [ONSDictionary class]) {
            [self release];
            id result =
                (id)__NSDictionaryI_new((const void **)&first, NULL, (const void **)arguments, pairs, 1);
            va_end(arguments);
            return result;
        }
        if (total >= 0x1FFFFFFFFFFFFFFF) {
            raiseTemporaryBufferException(pairs);
        }
    } else {
        if (cls == [ONSDictionary class]) {
            va_end(arguments);
            return (id)__ONSDictionary0__;
        }
        pairs = 0;
    }
    NSUInteger size = pairs > 1 ? pairs : 1;
    id *keys = alloca((pairs < 0x101 ? size : 1) * sizeof(id));
    size_t keysCount = 0;
    size_t objectsCount = 0;
    id *objects;
    id *heapKeys = NULL;
    id *heapObjects = NULL;
    if (pairs >= 0x101) {
        heapKeys = _OCFCreateArrayStorage(size, false, &keysCount);
        heapObjects = _OCFCreateArrayStorage(size, false, &objectsCount);
        heapObjects[0] = firstObject;
        heapKeys[0] = va_arg(arguments, id);
        keys = heapKeys;
        objects = heapObjects;
    } else {
        objects = alloca(size * sizeof(id));
        if (pairs != 0) {
            objects[0] = firstObject;
            keys[0] = va_arg(arguments, id);
        }
    }
    for (NSUInteger index = 1; index < pairs; index++) {
        objects[index] = va_arg(arguments, id);
        keys[index] = va_arg(arguments, id);
    }
    va_end(arguments);
    id result = [self initWithObjects:objects forKeys:keys count:pairs];
    free(heapObjects);
    free(heapKeys);
    return result;
}

// Native -[NSDictionary initWithObjects:forKeys:], 0x1c0286320. Non-external. Non-nil arguments that are not
// NSArrays raise (objects first). Counts are compared twice: objects against keys raises through
// _CFThrowFormattedException ("count of objects differs"), then keys against objects through the
// __CFExceptionProem reason ("number of objects not equal"; reachable only if the counts change between the
// two reads). The keys' count sizes stack buffers (up to 256 entries, else _CFCreateArrayStorage) filled by
// -getObjects:range: (keys first) for -initWithObjects:forKeys:count:.
- (id)initWithObjects:(id)objects forKeys:(id)keys {
    if (objects != nil && !_ONSIsNSArray(objects)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: objects argument is not an NSArray",
            dictionaryMethodName("-", "initWithObjects:forKeys:")
        );
    }
    if (keys != nil && !_ONSIsNSArray(keys)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: keys argument is not an NSArray",
            dictionaryMethodName("-", "initWithObjects:forKeys:")
        );
    }
    NSUInteger objectsCount = [objects count];
    NSUInteger keysCount = [keys count];
    if (objectsCount != keysCount) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException,
            OF_STR("*** %s: count of objects (%lu) differs from count of keys (%lu)"),
            dictionaryMethodName("-", "initWithObjects:forKeys:"),
            objectsCount,
            keysCount
        );
    }
    NSUInteger count = [keys count];
    objectsCount = [objects count];
    if (count != objectsCount) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFStringRef reason = OCFStringCreateWithFormat(
            allocator,
            NULL,
            OF_STR("%@: number of objects (%lu) not equal to number of keys (%lu)"),
            __CFExceptionProem(self, _cmd),
            objectsCount,
            count
        );
        id exception =
            [(id)dictionaryExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                     reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                                   userInfo:nil];
        objc_exception_throw(exception);
    }
    if ((count >> 60) != 0) {
        raiseTemporaryBufferException(count);
    }
    NSUInteger size = count > 1 ? count : 1;
    id *keyBuffer = alloca((count < 0x101 ? size : 1) * sizeof(id));
    size_t keyStorageCount = 0;
    size_t objectStorageCount = 0;
    id *objectBuffer;
    id *heapKeys = NULL;
    id *heapObjects = NULL;
    if (count <= 0x100) {
        objectBuffer = alloca(size * sizeof(id));
    } else {
        heapKeys = _OCFCreateArrayStorage(size, false, &keyStorageCount);
        heapObjects = _OCFCreateArrayStorage(size, false, &objectStorageCount);
        keyBuffer = heapKeys;
        objectBuffer = heapObjects;
    }
    [keys getObjects:keyBuffer range:(OCFRange){0, (OCFIndex)count}];
    [objects getObjects:objectBuffer range:(OCFRange){0, (OCFIndex)count}];
    id result = [self initWithObjects:objectBuffer forKeys:keyBuffer count:count];
    free(heapObjects);
    free(heapKeys);
    return result;
}

// Native +[NSDictionary dictionary], 0x1c0286788. Non-external.
+ (id)dictionary {
    return [[[self alloc] initWithObjects:NULL forKeys:NULL count:0] autorelease];
}

// Native ___createDictionary, 0x1c02867dc. Non-external (a static C function between
// +dictionaryWithObject:forKey: and +dictionaryWithObjects:forKeys:count:; declared before its caller here).
// -initWithObjects:forKeys:count: with the one object and key.
static id __createDictionary(id dictionary, id object, id key) {
    id objects[1] = {object};
    id keys[1] = {key};
    return [dictionary initWithObjects:objects forKeys:keys count:1];
}

// Native +[NSDictionary dictionaryWithObject:forKey:], 0x1c02867ac. Non-external.
+ (id)dictionaryWithObject:(id)object forKey:(id)key {
    return [__createDictionary([self alloc], object, key) autorelease];
}

// Native +[NSDictionary dictionaryWithObjects:forKeys:count:], 0x1c0286834. Non-external.
+ (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count {
    return [[[self alloc] initWithObjects:objects forKeys:keys count:count] autorelease];
}

// Native +[NSDictionary dictionaryWithObjectsAndKeys:], 0x1c0286874. Non-external. The pattern of
// -initWithObjectsAndKeys: on the class: NSDictionary itself (compared with its class reference) answers the
// __NSDictionary0__ singleton for no arguments (not autoreleased) or the autoreleased __NSDictionaryI_new
// result (options 1); other classes use [[self alloc] initWithObjects:forKeys:count:], autoreleased.
+ (id)dictionaryWithObjectsAndKeys:(id)firstObject, ... {
    va_list arguments;
    va_start(arguments, firstObject);
    id first = firstObject;
    NSUInteger pairs;
    if (firstObject != nil) {
        NSUInteger total = 0;
        va_list counting;
        va_copy(counting, arguments);
        id argument;
        do {
            argument = va_arg(counting, id);
            total++;
        } while (argument != nil);
        va_end(counting);
        if ((total & 1) != 0) {
            raiseWithProem(
                self,
                _cmd,
                OF_STR(
                    "%@: second object of each pair must be non-nil.  Or, did you forget to nil-terminate "
                    "your parameter list?"
                )
            );
        }
        pairs = total >> 1;
        if (self == [ONSDictionary class]) {
            id result =
                (id)__NSDictionaryI_new((const void **)&first, NULL, (const void **)arguments, pairs, 1);
            va_end(arguments);
            return [result autorelease];
        }
        if (total >= 0x1FFFFFFFFFFFFFFF) {
            raiseTemporaryBufferException(pairs);
        }
    } else {
        if (self == [ONSDictionary class]) {
            va_end(arguments);
            return (id)__ONSDictionary0__;
        }
        pairs = 0;
    }
    NSUInteger size = pairs > 1 ? pairs : 1;
    id *keys = alloca((pairs < 0x101 ? size : 1) * sizeof(id));
    size_t keysCount = 0;
    size_t objectsCount = 0;
    id *objects;
    id *heapKeys = NULL;
    id *heapObjects = NULL;
    if (pairs >= 0x101) {
        heapKeys = _OCFCreateArrayStorage(size, false, &keysCount);
        heapObjects = _OCFCreateArrayStorage(size, false, &objectsCount);
        heapObjects[0] = firstObject;
        heapKeys[0] = va_arg(arguments, id);
        keys = heapKeys;
        objects = heapObjects;
    } else {
        objects = alloca(size * sizeof(id));
        if (pairs != 0) {
            objects[0] = firstObject;
            keys[0] = va_arg(arguments, id);
        }
    }
    for (NSUInteger index = 1; index < pairs; index++) {
        objects[index] = va_arg(arguments, id);
        keys[index] = va_arg(arguments, id);
    }
    va_end(arguments);
    id result = [[self alloc] initWithObjects:objects forKeys:keys count:pairs];
    free(heapObjects);
    free(heapKeys);
    return [result autorelease];
}

// Native +[NSDictionary dictionaryWithObjects:forKeys:], 0x1c0286bb8. Non-external.
+ (id)dictionaryWithObjects:(id)objects forKeys:(id)keys {
    return [[[self alloc] initWithObjects:objects forKeys:keys] autorelease];
}

// Native +[NSDictionary newDictionaryWithObjects:forKeys:count:], 0x1c0286be8. Non-external. Order: NULL
// objects with a nonzero count; a count with any of the top three bits set; a nil object (its index); NULL
// keys with a nonzero count; a nil key (its index, with the objects message) all raise. NSDictionary itself
// (compared with its class reference) answers __NSSingleEntryDictionaryI_new (options 4) for one entry, the
// retained __NSDictionary0__ singleton for none, otherwise __NSDictionaryI_new (options 5);
// NSMutableDictionary answers __NSDictionaryM_new (options 3); any other class reaches
// __CFRequireConcreteImplementation and answers nil.
+ (id)newDictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count {
    if (objects == NULL && count != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            dictionaryMethodName("+", "newDictionaryWithObjects:forKeys:count:"),
            count
        );
    }
    if ((count >> 61) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            dictionaryMethodName("+", "newDictionaryWithObjects:forKeys:count:"),
            count
        );
    }
    if (count != 0) {
        for (NSUInteger index = 0; index < count; index++) {
            if (objects[index] == nil) {
                DICTIONARY_RAISE_WITH_LOG_PACK(
                    ONSInvalidArgumentException,
                    0x16,
                    "*** %s: attempt to insert nil object from objects[%lu]",
                    dictionaryMethodName("+", "newDictionaryWithObjects:forKeys:count:"),
                    index
                );
            }
        }
        if (keys == NULL && count != 0) {
            DICTIONARY_RAISE_WITH_LOG_PACK(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: pointer to objects array is NULL but length is %lu",
                dictionaryMethodName("+", "newDictionaryWithObjects:forKeys:count:"),
                count
            );
        }
        for (NSUInteger index = 0; index < count; index++) {
            if (keys[index] == nil) {
                DICTIONARY_RAISE_WITH_LOG_PACK(
                    ONSInvalidArgumentException,
                    0x16,
                    "*** %s: attempt to insert nil object from objects[%lu]",
                    dictionaryMethodName("+", "newDictionaryWithObjects:forKeys:count:"),
                    index
                );
            }
        }
    }
    if (self == [ONSDictionary class]) {
        if (count == 1) {
            return (id)__NSSingleEntryDictionaryI_new(keys[0], objects[0], 4);
        }
        if (count == 0) {
            return [(id)__ONSDictionary0__ retain];
        }
        return (id)__NSDictionaryI_new((const void **)keys, (const void **)objects, NULL, count, 5);
    }
    if (self == [ONSMutableDictionary class]) {
        return (id)__NSDictionaryM_new((const void **)keys, (const void **)objects, count, 3);
    }
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSDictionary)), self, _cmd);
    return nil;
}

// Native +[NSDictionary dictionaryWithDictionary:copyItems:], 0x1c0287018. Non-external.
+ (id)dictionaryWithDictionary:(id)otherDictionary copyItems:(BOOL)flag {
    return [[[self alloc] initWithDictionary:otherDictionary copyItems:flag] autorelease];
}

// Native +[NSDictionary dictionaryWithDictionary:], 0x1c0287048. Non-external.
+ (id)dictionaryWithDictionary:(id)otherDictionary {
    return [[[self alloc] initWithDictionary:otherDictionary copyItems:NO] autorelease];
}

// Native -[NSDictionary copyWithZone:], 0x1c0287074. Non-external. __NSCollectionsShouldCopy (called with the
// receiver in X0) before the read check decides copyItems of [[NSDictionary allocWithZone:zone]
// initWithDictionary:self copyItems:].
- (id)copyWithZone:(struct _NSZone *)zone {
    void *returnAddress = __builtin_return_address(0);
    BOOL shouldCopy = __NSCollectionsShouldCopy();
    dictionaryTSANRead(self, returnAddress);
    return [[ONSDictionary allocWithZone:zone] initWithDictionary:self copyItems:shouldCopy];
}

// Native -[NSDictionary mutableCopyWithZone:], 0x1c02870ec. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    dictionaryTSANRead(self, __builtin_return_address(0));
    return [[ONSMutableDictionary allocWithZone:zone] initWithDictionary:self copyItems:NO];
}

// Native -[NSDictionary _cfMutableCopy], 0x1c0287150. Non-external. objc_allocWithZone of
// NSMutableDictionary.
- (id)_cfMutableCopy {
    dictionaryTSANRead(self, __builtin_return_address(0));
    return [[ONSMutableDictionary allocWithZone:NULL] initWithDictionary:self copyItems:NO];
}

// Native +[NSDictionary supportsSecureCoding], 0x1c02871ac. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSDictionary initWithCoder:], 0x1c02871b4. Non-external. Answers nil without releasing the
// receiver.
- (id)initWithCoder:(id)coder {
    return nil;
}

// Native -[NSDictionary encodeWithCoder:], 0x1c02871bc. Non-external. Does nothing.
- (void)encodeWithCoder:(id)coder {
}

// Native -[NSDictionary count], 0x1c030f198. Non-external. The abstract method: OUTLINED_FUNCTION_0_39
// (__CFLookUpClass("NSDictionary")), _1_27 (__CFRequireConcreteImplementation with the receiver and selector)
// and _2_23 (the epilogue); answers 0.
- (NSUInteger)count {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSDictionary)), self, _cmd);
    return 0;
}

// Native -[NSDictionary objectForKey:], 0x1c030f1b8. Non-external. Abstract, as -count; answers nil.
- (id)objectForKey:(id)key {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSDictionary)), self, _cmd);
    return nil;
}

// Native -[NSDictionary keyEnumerator], 0x1c030f1d8. Non-external. Abstract, as -count; answers nil.
- (id)keyEnumerator {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSDictionary)), self, _cmd);
    return nil;
}

// Native -[NSDictionary initWithObjects:forKeys:count:], 0x1c030f1f8. Non-external. Abstract, as -count;
// answers nil without releasing the receiver.
- (id)initWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSDictionary)), self, _cmd);
    return nil;
}

@end
