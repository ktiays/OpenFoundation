//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSSingleEntryDictionaryI.h"
#import "CoreFoundation/Objective-C/OCFNSDictionaryEnumerate.h"

// Inferred organization: the __NSSingleEntryDictionaryI methods (0x1c019e3a4-0x1c019ef83) lie together in the
// iOS Simulator __text section, with the unit's C function __NSSingleEntryDictionaryI_new (0x1c019eb8c) among
// them; following the owner's decision, the class has its own translation unit, which keeps that order (the
// function, which writes the instance variables, is defined inside the @implementation). The const_mu of
// -countByEnumeratingWithState:objects:count: lies at 0x1c0373668 (__TEXT,__const, value 1, read with
// Local/Scratch/cf-core/macho.py). Objective-C compiled with manual reference counting (objc_retain,
// objc_release, objc_autorelease) and Objective-C exceptions (the catch-all of
// __NSSingleEntryDictionaryI_new). Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.
// Unless a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The __NSSingleObjectEnumerator class symbol, sent +alloc (objc_alloc) directly.
extern char singleEntrySingleObjectEnumeratorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                               OF_NAME_STRING(__ONSSingleObjectEnumerator));

// Selectors this unit sends that no included header declares, for the message sends only.
@protocol SingleEntryDictionaryIMessages

- (id)initWithObject:(id)object collection:(id)collection;

@end

@implementation __ONSSingleEntryDictionaryI

// Native -[__NSSingleEntryDictionaryI count], 0x1c019e3a4. Non-external.
- (NSUInteger)count {
    return 1;
}

// Native -[__NSSingleEntryDictionaryI isEqualToDictionary:], 0x1c019e3ac. Non-external. A nil argument
// answers whether the receiver is nil (NO). An argument that is not an NSDictionary (_NSIsNSDictionary)
// raises (os_log pack buffer 0xC: the method name). The receiver itself answers YES; a count other than 1
// answers NO; otherwise the argument's object for the receiver's key, nil answering NO, compared with
// -isEqual: sent to the receiver's object (a tail call).
- (BOOL)isEqualToDictionary:(id)other {
    if (other == nil) {
        return self == nil;
    }
    if (!_ONSIsNSDictionary(other)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: dictionary argument is not an NSDictionary",
            "-[" OF_NAME_STRING(__ONSSingleEntryDictionaryI) " isEqualToDictionary:]"
        );
    }
    if (other == self) {
        return YES;
    }
    if ([other count] != 1) {
        return NO;
    }
    id object = [other objectForKey:self->_key];
    if (object == nil) {
        return NO;
    }
    return [self->_obj isEqual:object];
}

// Native -[__NSSingleEntryDictionaryI objectForKey:], 0x1c019e558. Non-external. The key matches when it is
// the same pointer or the stored key's -isEqual: answers YES.
- (id)objectForKey:(id)key {
    id stored = self->_key;
    if (stored != key && ![stored isEqual:key]) {
        return nil;
    }
    return self->_obj;
}

// Native -[__NSSingleEntryDictionaryI keyEnumerator], 0x1c019e5a4. Non-external. A single-object enumerator
// (+alloc, -initWithObject:collection:) over the key, autoreleased.
- (id)keyEnumerator {
    id enumerator = [(Class)singleEntrySingleObjectEnumeratorClass alloc];
    return [[enumerator initWithObject:self->_key collection:self] autorelease];
}

// Native -[__NSSingleEntryDictionaryI objectEnumerator], 0x1c019e5e0. Non-external. As -keyEnumerator, over
// the object.
- (id)objectEnumerator {
    id enumerator = [(Class)singleEntrySingleObjectEnumeratorClass alloc];
    return [[enumerator initWithObject:self->_obj collection:self] autorelease];
}

// Native -[__NSSingleEntryDictionaryI getObjects:andKeys:count:], 0x1c019e61c. Non-external. With an objects
// array: a count with any of its top three bits set raises (os_log pack buffer 0x16: the method name and the
// count), a zero count returns, otherwise the object is stored; then the same for the keys array and the key.
- (void)getObjects:(id *)objects andKeys:(id *)keys count:(NSUInteger)count {
    if (objects != NULL) {
        if ((OF_WORD_SHIFT(count, 61)) != 0) {
            DICTIONARY_RAISE_WITH_LOG_PACK(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: count (%lu) of objects array is ridiculous",
                "-[" OF_NAME_STRING(__ONSSingleEntryDictionaryI) " getObjects:andKeys:count:]",
                count
            );
        }
        if (count == 0) {
            return;
        }
        objects[0] = self->_obj;
    }
    if (keys != NULL) {
        if ((OF_WORD_SHIFT(count, 61)) != 0) {
            DICTIONARY_RAISE_WITH_LOG_PACK(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: count (%lu) of objects array is ridiculous",
                "-[" OF_NAME_STRING(__ONSSingleEntryDictionaryI) " getObjects:andKeys:count:]",
                count
            );
        }
        if (count == 0) {
            return;
        }
        keys[0] = self->_key;
    }
}

// Native -[__NSSingleEntryDictionaryI __apply:context:], 0x1c019e7b4. Non-external. A NULL function raises
// NSInvalidArgumentException "%@: function pointer is NULL" (the __CFExceptionProem of the receiver and
// selector; no os_log pack); otherwise a tail call of the function with the key, the object and the context.
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
    applier(self->_key, self->_obj, context);
}

// Native -[__NSSingleEntryDictionaryI countByEnumeratingWithState:objects:count:], 0x1c019e848. Non-external.
// A NULL buffer with a nonzero length raises first, then a length with any of its top three bits set (os_log
// pack buffer 0x16 each: the method name and the length). A started enumeration answers 0; the first call
// stores the buffer as itemsPtr and the constant const_mu (1) as mutationsPtr, writes the key to the buffer's
// first slot, sets state 1 and answers 1. The key is written even for a zero length (with a NULL buffer and a
// zero length the native store faults; kept).
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    // Native countByEnumeratingWithState:objects:count:.const_mu, 0x1c0373668 (__TEXT,__const).
    static const unsigned long const_mu = 1;
    DictionaryEnumerationState *enumerationState = state;
    if (buffer == NULL && length != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(__ONSSingleEntryDictionaryI) " countByEnumeratingWithState:objects:count:]",
            length
        );
    }
    if ((OF_WORD_SHIFT(length, 61)) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSSingleEntryDictionaryI) " countByEnumeratingWithState:objects:count:]",
            length
        );
    }
    if (enumerationState->state != 0) {
        return 0;
    }
    enumerationState->itemsPtr = (void **)buffer;
    enumerationState->mutationsPtr = (unsigned long *)&const_mu;
    buffer[0] = self->_key;
    enumerationState->state = 1;
    return 1;
}

// Native -[__NSSingleEntryDictionaryI enumerateKeysAndObjectsWithOptions:usingBlock:], 0x1c019ea34.
// Non-external. A nil block raises (os_log pack buffer 0xC: the method name). The options are not read: the
// block is called once, through __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__, inside an autorelease pool.
- (void)enumerateKeysAndObjectsWithOptions:(NSUInteger)options usingBlock:(void (^)(id, id, BOOL *))block {
    if (block == nil) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            "-[" OF_NAME_STRING(
                __ONSSingleEntryDictionaryI
            ) " enumerateKeysAndObjectsWithOptions:usingBlock:]"
        );
    }
    BOOL stop = NO;
    void *pool = _OCFAutoreleasePoolPush();
    __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(block, self->_key, self->_obj, &stop);
    _OCFAutoreleasePoolPop(pool);
}

// Native -[__NSSingleEntryDictionaryI copyWithZone:], 0x1c019eb88. Non-external. A tail call of objc_retain.
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native __NSSingleEntryDictionaryI_new, 0x1c019eb8c. Hidden (was a private external). A nil key raises, then
// a nil object (os_log pack buffer 0xC each: the function name "__NSSingleEntryDictionaryI_new"). The
// instance is __CFAllocateObject([__NSSingleEntryDictionaryI self], 0); NULL is answered as is. Option bit 1
// copies the key (-copyWithZone: NULL; a nil copy raises NSInvalidArgumentException "NSDictionary:
// -copyWithZone: for key
// %@ returned nil" with the original key, no os_log pack); otherwise the key is retained unless option bit 8
// is set or it is a tagged pointer (bit 63). The object is stored, then retained unless it is a tagged
// pointer or option bit 4 is set. An Objective-C exception raised after the allocation releases the instance
// (when it is a positive pointer) and is rethrown (objc_begin_catch, objc_exception_rethrow); other
// exceptions unwind through. Which calls the landing pads (0x1c019ee64, 0x1c019ee6c, 0x1c019ee74) cover is
// read from their register use, not from the LSDA (confidence: medium for the exact extent of the @try).
OCFDictionaryRef __NSSingleEntryDictionaryI_new(const void *key, const void *object, uintptr_t options) {
    if (key == NULL) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException, 0xC, "*** %s: key cannot be nil", "__NSSingleEntryDictionaryI_new"
        );
    }
    if (object == NULL) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException, 0xC, "*** %s: object cannot be nil", "__NSSingleEntryDictionaryI_new"
        );
    }
    __ONSSingleEntryDictionaryI *dictionary = nil;
    @try {
        dictionary = __CFAllocateObject([__ONSSingleEntryDictionaryI self], 0);
        if (dictionary == nil) {
            return NULL;
        }
        if ((options & 1) != 0) {
            dictionary->_key = [(id)key copyWithZone:NULL];
            if (dictionary->_key == nil) {
                OCFStringRef reason = OCFStringCreateWithFormat(
                    kOCFAllocatorSystemDefault,
                    NULL,
                    OF_STR("NSDictionary: -copyWithZone: for key %@ returned nil"),
                    key
                );
                id exception =
                    [(id)dictionaryExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                             reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                                           userInfo:nil];
                objc_exception_throw(exception);
            }
        } else {
            if ((options & 8) == 0 && (intptr_t)key >= 0) {
                [(id)key retain];
            }
            dictionary->_key = (id)key;
        }
        dictionary->_obj = (id)object;
        if ((intptr_t)object >= 0 && (options & 4) == 0) {
            [(id)object retain];
        }
    } @catch (id exception) {
        if ((intptr_t)dictionary >= 1) {
            [dictionary release];
        }
        @throw;
    }
    return (OCFDictionaryRef)dictionary;
}

// Native -[__NSSingleEntryDictionaryI dealloc], 0x1c019eeb4. Non-external. The object, then the key, each
// released when it is a positive pointer; then super's -dealloc.
- (void)dealloc {
    if ((intptr_t)self->_obj >= 1) {
        [self->_obj release];
    }
    if ((intptr_t)self->_key >= 1) {
        [self->_key release];
    }
    [super dealloc];
}

// Native +[__NSSingleEntryDictionaryI automaticallyNotifiesObserversForKey:], 0x1c019ef50. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native +[__NSSingleEntryDictionaryI allocWithZone:], 0x1c019ef58. Non-external. A tail call of
// __NSDictionaryImmutablePlaceholder.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSDictionaryImmutablePlaceholder();
}

// Native -[__NSSingleEntryDictionaryI mutableCopyWithZone:], 0x1c019ef5c. Non-external. A tail call of
// __NSDictionaryM_new with the addresses of the key and object instance variables as one-entry arrays, the
// capacity 1 and the options 2; the zone is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return (id)__NSDictionaryM_new((const void **)&self->_key, (const void **)&self->_obj, 1, 2);
}

@end
