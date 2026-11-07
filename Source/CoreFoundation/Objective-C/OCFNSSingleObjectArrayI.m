//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSSingleObjectArrayI.h"
#include "CoreFoundation/Objective-C/OCFNSArrayHelpers.h"
#include "CoreFoundation/Objective-C/OCFNSArrayM.h"
#include "CoreFoundation/Objective-C/OCFNSFastEnumerationEnumerator.h"
#include "CoreFoundation/Objective-C/OCFNSPlaceholderArray.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#import <objc/NSObject.h>

#include <alloca.h>
#include <mach-o/loader.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <stdint.h>

// Inferred organization: the __NSSingleObjectArrayI methods lie together in the iOS Simulator __text section
// (0x1c02cedbc-0x1c02cf74b), with __NSSingleObjectArrayI_new (0x1c02cf644) among them (after +allocWithZone:
// and before -dealloc). Following the owner's decision, the class has this translation unit; the methods
// follow in native address order, the C function at its native position inside the @implementation.
// Objective-C compiled with manual reference counting (objc_retain, objc_release, objc_autorelease).
// Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json (family H-array-concrete;
// __NSSingleObjectArrayI_new from iphone-simulator-corpus-delta8.json). Unless a definition says otherwise:
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in
// full). iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md
// (not written yet).
//
// Argument errors raise as the other bridged array classes do (an os_log pack built on the stack, the reason
// made with kCFAllocatorSystemDefault and autoreleased, +[NSException
// exceptionWithName:reason:userInfo:osLogPack:size:] thrown); the first format argument is the method's name,
// spelled with the class's selected name.

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// libobjc entry point without an SDK declaration (objc-internal.h); supplied, never reimplemented.
extern id objc_allocWithZone(Class cls);

// The NSException and __NSSingleObjectEnumerator class symbols (off_2227CE300 and the enumerator's class
// reference), sent to or allocated directly.
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char singleObjectEnumeratorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                    OF_NAME_STRING(__ONSSingleObjectEnumerator));

// The messages this unit sends that the shared declarations do not carry; declared for the sends only.
@protocol SingleObjectArrayIMessages

+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;
- (id)initWithObject:(id)object collection:(id)collection;

@end

#define methodName(selectorText) "-[" OF_NAME_STRING(__ONSSingleObjectArrayI) " " selectorText "]"

#define RAISE_LOGGED_EXCEPTION(name, bufferSize, format, ...)                                                \
    do {                                                                                                     \
        size_t packSize = _os_log_pack_size(bufferSize);                                                     \
        void *pack = alloca((packSize + 15) & ~(size_t)15);                                                  \
        uint8_t *packBuffer = _os_log_pack_fill(pack, packSize, 0, &__dso_handle, format);                   \
        __builtin_os_log_format(packBuffer, format, __VA_ARGS__);                                            \
        OCFStringRef reason =                                                                                \
            OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR(format), __VA_ARGS__);        \
        objc_exception_throw([(id)exceptionClass                                                             \
            exceptionWithName:(id)(name)                                                                     \
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)                                     \
                     userInfo:nil                                                                            \
                    osLogPack:pack                                                                           \
                         size:packSize]);                                                                    \
    } while (0)

@implementation __ONSSingleObjectArrayI

// Native -[__NSSingleObjectArrayI count], 0x1c02cedbc. Non-external. Answers 1.
- (NSUInteger)count {
    return 1;
}

// Native -[__NSSingleObjectArrayI objectAtIndex:], 0x1c02cedc4. Non-external. Index 0 answers the object;
// any other raises NSRangeException "*** %s: index %lu beyond bounds [0 .. %lu]" (buffer 0x20) with 0.
- (id)objectAtIndex:(NSUInteger)index {
    if (index != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x20,
            "*** %s: index %lu beyond bounds [0 .. %lu]",
            methodName("objectAtIndex:"),
            (unsigned long)index,
            (unsigned long)0
        );
    }
    return _object;
}

// Native -[__NSSingleObjectArrayI getObjects:range:], 0x1c02ceef8. Non-external. Order: the range check
// (location or length with bit 63 set, or their sum 2 or more; "*** %s: range {%lu, %lu} extends beyond
// bounds [0 .. %lu]", buffer 0x2A, with 0), then a NULL buffer with a nonzero length (buffer 0x16); a
// nonzero length stores the object.
- (void)getObjects:(id *)objects range:(OCFRange)range {
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if ((NSInteger)location < 0 || (NSInteger)length < 0 || location + length >= 2) {
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x2A,
            "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
            methodName("getObjects:range:"),
            (unsigned long)location,
            (unsigned long)length,
            (unsigned long)0
        );
    }
    if (objects == NULL && length != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            methodName("getObjects:range:"),
            (unsigned long)length
        );
    }
    if (length != 0) {
        objects[0] = _object;
    }
}

// Native +[__NSSingleObjectArrayI automaticallyNotifiesObserversForKey:], 0x1c02cf120. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSSingleObjectArrayI countByEnumeratingWithState:objects:count:], 0x1c02cf128. Non-external. The
// buffer checks on the objects buffer and its length (a NULL buffer with a nonzero length, a length of 2^61
// or more; buffer 0x16 each); then a state other than 0 answers 0, and a first call points the items at the
// object ivar and the mutations at the unit's const_mu, sets the state to -1 and answers 1. The const_mu is
// one of the image's "countByEnumeratingWithState:objects:count:.const_mu" constants (__TEXT,__const), named
// const_mu_7 in the capture without its address (unread); every one of them holds 1 (read with
// Local/Scratch/cf-core/macho.py).
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    static const unsigned long const_mu = 1;
    if (buffer == NULL && length != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            methodName("countByEnumeratingWithState:objects:count:"),
            (unsigned long)length
        );
    }
    if ((OF_WORD_SHIFT(length, 61)) != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            methodName("countByEnumeratingWithState:objects:count:"),
            (unsigned long)length
        );
    }
    FastEnumerationState *enumerationState = (FastEnumerationState *)state;
    if (enumerationState->state != 0) {
        return 0;
    }
    enumerationState->itemsPtr = &_object;
    enumerationState->mutationsPtr = (unsigned long *)&const_mu;
    enumerationState->state = (unsigned long)-1;
    return 1;
}

// Native -[__NSSingleObjectArrayI enumerateObjectsWithOptions:usingBlock:], 0x1c02cf314. Non-external. A nil
// block raises ("block cannot be nil", buffer 0xC); otherwise the block is called once with the object at
// index 0 inside an autorelease pool. The options are not read.
- (void)enumerateObjectsWithOptions:(NSUInteger)options
                         usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    if (block == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            methodName("enumerateObjectsWithOptions:usingBlock:")
        );
    }
    BOOL stop = NO;
    void *pool = _OCFAutoreleasePoolPush();
    __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__(block, _object, 0, &stop);
    _OCFAutoreleasePoolPop(pool);
}

// Native -[__NSSingleObjectArrayI firstObject], 0x1c02cf460. Non-external. The object.
- (id)firstObject {
    return _object;
}

// Native -[__NSSingleObjectArrayI isEqualToArray:], 0x1c02cf470. Non-external. nil answers whether the
// receiver is nil; an argument that is not an NSArray (_NSIsNSArray) raises NSInvalidArgumentException "***
// %s: array argument is not an NSArray" (buffer 0xC). The receiver itself answers YES; another count than 1
// answers NO; otherwise its -objectAtIndexedSubscript: 0 is compared by pointer, then with -isEqual: sent to
// the receiver's object.
- (BOOL)isEqualToArray:(id)otherArray {
    if (otherArray == nil) {
        return self == nil;
    }
    if (!_ONSIsNSArray(otherArray)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: array argument is not an NSArray",
            methodName("isEqualToArray:")
        );
    }
    if (otherArray == self) {
        return YES;
    }
    if ([otherArray count] != 1) {
        return NO;
    }
    id other = [otherArray objectAtIndexedSubscript:0];
    if (_object == other) {
        return YES;
    }
    return [_object isEqual:other] ? YES : NO;
}

// Native -[__NSSingleObjectArrayI lastObject], 0x1c02cf5f4. Non-external. The object.
- (id)lastObject {
    return _object;
}

// Native -[__NSSingleObjectArrayI objectEnumerator], 0x1c02cf604. Non-external. A __NSSingleObjectEnumerator
// (objc_allocWithZone) sent -initWithObject:collection: with the object and the receiver, autoreleased (a
// tail call).
- (id)objectEnumerator {
    id enumerator = objc_allocWithZone((Class)(void *)singleObjectEnumeratorClass);
    return [[enumerator initWithObject:_object collection:self] autorelease];
}

// Native +[__NSSingleObjectArrayI allocWithZone:], 0x1c02cf640. Non-external. The immutable placeholder (a
// tail call).
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSArrayImmutablePlaceholder();
}

// Native __NSSingleObjectArrayI_new, 0x1c02cf644. Hidden (was a private external). The object is
// __CFAllocateObject of the class (objc_opt_self of its class reference) with no extra bytes; when it is not
// NULL the object is stored and, when it is greater than 0 as a signed word (neither nil nor a tagged
// pointer) and noRetain (bit 0) is clear, retained. Evidence: iphone-simulator-corpus-delta8.json.
id __NSSingleObjectArrayI_new(id object, BOOL noRetain) {
    __ONSSingleObjectArrayI *array = __CFAllocateObject([__ONSSingleObjectArrayI self], 0);
    if (array != nil) {
        array->_object = object;
        if ((intptr_t)object >= 1 && !(noRetain & 1)) {
            [object retain];
        }
    }
    return array;
}

// Native -[__NSSingleObjectArrayI dealloc], 0x1c02cf6ac. Non-external. The object is released when it is
// greater than 0 as a signed word, then super.
- (void)dealloc {
    if ((intptr_t)_object >= 1) {
        [_object release];
    }
    [super dealloc];
}

// Native -[__NSSingleObjectArrayI copyWithZone:], 0x1c02cf730. Non-external. The receiver retained
// (objc_retain, a tail call).
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native -[__NSSingleObjectArrayI mutableCopyWithZone:], 0x1c02cf734. Non-external. __NSArrayM_new of the one
// object ivar, retaining (a tail call).
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return __NSArrayM_new(&_object, 1, NO);
}

@end
