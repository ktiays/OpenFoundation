//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSSingleObjectSetI.h"
#import "CoreFoundation/Objective-C/OCFNSFastEnumerationEnumerator.h"
#import "CoreFoundation/Objective-C/OCFNSPlaceholderSet.h"
#import "CoreFoundation/Objective-C/OCFNSSetEnumerate.h"

#include <stdint.h>

// Inferred organization: the __NSSingleObjectSetI methods (0x1c018d588-0x1c018dca7) lie together in the iOS
// Simulator __text section, with the unit's C function __NSSingleObjectSetI_new (0x1c018dbb4) between
// -getObjects:count: and +allocWithZone:; following the owner's decision, the class has its own translation
// unit, which keeps that order (the function, which writes the instance variable, is defined inside the
// @implementation). The const_mu of -countByEnumeratingWithState:objects:count: (listed as const_mu_1) is
// placed at 0x1c0373498 (__TEXT,__const, value 1, read with Local/Scratch/cf-core/macho.py) by the order of
// the __TEXT,__const statics, which follows the __text order of their units (confidence: medium for the
// address). Objective-C compiled with manual reference counting (objc_retain, objc_release,
// objc_autorelease). Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json (the constructor)
// and iphone-simulator-corpus-delta10.json (the methods). Unless a definition says otherwise: iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device
// 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written
// yet).

// The __NSSingleObjectSetI class symbol (loaded through the __SCR___NSSingleObjectSetI GOT slot) and the
// offset of its ivar element (8), defined by this unit's @implementation.
extern char singleObjectSetIClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSSingleObjectSetI));
extern const int32_t
    singleObjectSetIElementOffset __asm(OF_SYMBOL("OBJC_IVAR_$_")
                                            OF_NAME_STRING(__ONSSingleObjectSetI) ".element");

// The __NSSingleObjectEnumerator class symbol, referenced natively by address (ADRP/ADD) and sent
// +allocWithZone: (objc_allocWithZone); a class of this image not restored on this branch (undefined).
extern char singleObjectSetEnumeratorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                       OF_NAME_STRING(__ONSSingleObjectEnumerator));

// Selectors this unit sends that no included header declares, for the message sends only.
@protocol SingleObjectSetIMessages

- (id)initWithObject:(id)object collection:(id)collection;

@end

@implementation __ONSSingleObjectSetI

// Native -[__NSSingleObjectSetI count], 0x1c018d588. Non-external.
- (NSUInteger)count {
    return 1;
}

// Native -[__NSSingleObjectSetI member:], 0x1c018d590. Non-external. The element when it is the argument
// itself or the argument's -isEqual: (sent to the argument) answers YES with it, otherwise nil.
- (id)member:(id)object {
    id stored = self->element;
    if (object != stored && ![object isEqual:stored]) {
        return nil;
    }
    return stored;
}

// Native -[__NSSingleObjectSetI objectEnumerator], 0x1c018d5d4. Non-external. A new
// __NSSingleObjectEnumerator (objc_allocWithZone, then -initWithObject:collection: with the element and the
// receiver), autoreleased (a tail call of objc_autorelease).
- (id)objectEnumerator {
    id enumerator = [(Class)singleObjectSetEnumeratorClass allocWithZone:NULL];
    enumerator = [(id<SingleObjectSetIMessages>)enumerator initWithObject:self->element collection:self];
    return [enumerator autorelease];
}

// Native +[__NSSingleObjectSetI automaticallyNotifiesObserversForKey:], 0x1c018d610. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSSingleObjectSetI containsObject:], 0x1c018d618. Non-external. Whether the element is non-nil
// and either the argument itself or equal to it by the argument's -isEqual:.
- (BOOL)containsObject:(id)object {
    id stored = self->element;
    if (object != stored && ![object isEqual:stored]) {
        return NO;
    }
    return stored != nil;
}

// Native -[__NSSingleObjectSetI countForObject:], 0x1c018d65c. Non-external. The body of -containsObject: as
// a count (1 or 0).
- (NSUInteger)countForObject:(id)object {
    id stored = self->element;
    if (object != stored && ![object isEqual:stored]) {
        return 0;
    }
    return stored != nil;
}

// Native -[__NSSingleObjectSetI countByEnumeratingWithState:objects:count:], 0x1c018d6a8. Non-external. A
// NULL buffer with a nonzero length raises first, then a length with any of its top three bits set (os_log
// pack buffer 0x16 each: the method name and the length). A started enumeration (nonzero state) answers 0.
// The first call stores the buffer as itemsPtr and the constant const_mu (1) as mutationsPtr; with a buffer
// and a nonzero length it writes the element to the buffer's first slot, sets state 1 and answers 1,
// otherwise it answers 0 and leaves the state at 0.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    // Native countByEnumeratingWithState:objects:count:.const_mu (listed as const_mu_1), 0x1c0373498
    // (__TEXT,__const).
    static const unsigned long const_mu = 1;
    FastEnumerationState *enumerationState = state;
    if (buffer == NULL && length != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(__ONSSingleObjectSetI) " countByEnumeratingWithState:objects:count:]",
            (unsigned long)length
        );
    }
    if ((OF_WORD_SHIFT(length, 61)) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSSingleObjectSetI) " countByEnumeratingWithState:objects:count:]",
            (unsigned long)length
        );
    }
    if (enumerationState->state != 0) {
        return 0;
    }
    enumerationState->itemsPtr = buffer;
    enumerationState->mutationsPtr = (unsigned long *)&const_mu;
    if (buffer == NULL || length == 0) {
        return 0;
    }
    buffer[0] = self->element;
    enumerationState->state = 1;
    return 1;
}

// Native -[__NSSingleObjectSetI enumerateObjectsWithOptions:usingBlock:], 0x1c018d8a0. Non-external. A nil
// block raises (os_log pack buffer 0xC: the method name); otherwise the block is called out once with the
// element and a stop flag, inside an autorelease pool. The options are not read.
- (void)enumerateObjectsWithOptions:(NSUInteger)options usingBlock:(void (^)(id object, BOOL *stop))block {
    if (block == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: block cannot be nil",
            "-[" OF_NAME_STRING(__ONSSingleObjectSetI) " enumerateObjectsWithOptions:usingBlock:]"
        );
    }
    BOOL stop = NO;
    void *pool = _OCFAutoreleasePoolPush();
    __NSSET_IS_CALLING_OUT_TO_A_BLOCK__(block, self->element, &stop);
    _OCFAutoreleasePoolPop(pool);
}

// Native -[__NSSingleObjectSetI getObjects:count:], 0x1c018d9e8. Non-external. The argument checks of
// -countByEnumeratingWithState:objects:count: (with this method's name); a nonzero count receives the
// element in the first slot.
- (void)getObjects:(id *)objects count:(NSUInteger)count {
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(__ONSSingleObjectSetI) " getObjects:count:]",
            (unsigned long)count
        );
    }
    if ((OF_WORD_SHIFT(count, 61)) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSSingleObjectSetI) " getObjects:count:]",
            (unsigned long)count
        );
    }
    if (count != 0) {
        objects[0] = self->element;
    }
}

// Native __NSSingleObjectSetI_new, 0x1c018dbb4. Hidden (was a private external). __CFAllocateObject of
// +[__NSSingleObjectSetI self] with no extra bytes; when that answers an object, the element is stored and
// retained unless it is a tagged pointer or nil (signed value below 1) or bit 0 of noRetain is set.
id __NSSingleObjectSetI_new(id object, BOOL noRetain) {
    id set = __CFAllocateObject([(Class)singleObjectSetIClass self], 0);
    if (set != nil) {
        *(id *)((uint8_t *)set + singleObjectSetIElementOffset) = object;
        if ((intptr_t)object >= 1 && !noRetain) {
            objc_retain(object);
        }
    }
    return set;
}

// Native +[__NSSingleObjectSetI allocWithZone:], 0x1c018dc1c. Non-external. The immutable set placeholder (a
// tail call of __NSSetImmutablePlaceholder); the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSSetImmutablePlaceholder();
}

// Native -[__NSSingleObjectSetI copyWithZone:], 0x1c018dc20. Non-external. A tail call of objc_retain.
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native -[__NSSingleObjectSetI dealloc], 0x1c018dc24. Non-external. The element is released unless it is a
// tagged pointer or nil (signed value below 1), then super's -dealloc.
- (void)dealloc {
    if ((intptr_t)self->element >= 1) {
        [self->element release];
    }
    [super dealloc];
}

@end
