//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSSet.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/Collections/OCFSortFunctions.h"
#include "CoreFoundation/Objective-C/OCFNSEmptyCollections.h"
#include "CoreFoundation/Objective-C/OCFNSMutableSet.h"
#include "CoreFoundation/Objective-C/OCFNSPlaceholderSet.h"
#include "CoreFoundation/Objective-C/OCFNSSetEnumerate.h"
#include "CoreFoundation/Objective-C/OCFNSSetI.h"
#include "CoreFoundation/Objective-C/OCFNSSetM.h"
#include "CoreFoundation/Objective-C/OCFNSSingleObjectSetI.h"

#include <malloc/malloc.h>
#include <objc/runtime.h>
#include <pthread.h>
#include <stdarg.h>
#include <string.h>

// Inferred organization: the NSSet methods lie together in the iOS Simulator __text section
// (0x1c029eee4-0x1c02a4706, with __createSet at 0x1c02a3440 and OUTLINED_FUNCTION_0_44, _1_31 and _2_27 at
// 0x1c02a46e0-0x1c02a4700), and the four abstract methods -count, -member:, -objectEnumerator and
// -initWithObjects:count: in the outlined region (0x1c030fc20-0x1c030fc9c); following the owner's decision,
// the class has its own translation unit. The unit is Objective-C compiled with manual reference counting
// (objc_autorelease, explicit retains). Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full,
// iphone-simulator-corpus-delta8.json; where the IDA pseudocode drops a check, the assembly settles it). iOS
// Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not
// written yet).
//
// Every ThreadSanitizer read check passes __CFTSANTagMutableSet. The argument checks raise through
// raiseWithLogPack (Objective-C/OCFNSSet.h) with the method's name as the %s argument.

// The fast-enumeration state of -countByEnumeratingWithState:objects:count: (64 bytes, zeroed by the caller).
// FastEnumerationState is a local name for the layout of NSFastEnumerationState.
typedef struct {
    unsigned long state;
    id *itemsPtr;
    unsigned long *mutationsPtr;
    unsigned long extra[5];
} FastEnumerationState;

// The NSSet and NSMutableSet class symbols, compared by address with the receiver (through the __SCR_NSSet
// and __SCR_NSMutableSet GOT slots).
extern char setClassSymbol[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSSet));
extern char mutableSetClassSymbol[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableSet));

// Native __createSet, 0x1c02a3440; defined after +[NSSet setWithObject:], in native address order.
static id __createSet(id set, id object);

@implementation ONSSet

// Native -[NSSet _cfTypeID], 0x1c029eee4. Non-external. A tail call of CFSetGetTypeID.
- (OCFTypeID)_cfTypeID {
    return OCFSetGetTypeID();
}

// Native -[NSSet isNSSet__], 0x1c029eee8. Non-external. Answers YES.
- (BOOL)isNSSet__ {
    return YES;
}

// Native -[NSSet allObjects], 0x1c029eef0. Non-external. The objects are copied with -getObjects:count: into
// _CFCreateArrayStorage(max(count, 1)) storage (a count with any of the top four bits set throws
// NSGenericException first); with storage, each object is retained and the storage is adopted by
// -[NSArray _initByAdoptingBuffer:count:size:]; without it, -[NSArray initWithObjects:count:] gets NULL.
// Autoreleased.
- (id)allObjects {
    setTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (OF_WORD_SHIFT(count, 60) != 0) {
        raiseTemporaryBufferTooLarge(count);
    }
    size_t size = 0;
    id *objects = (id *)_OCFCreateArrayStorage(count > 1 ? count : 1, false, &size);
    [self getObjects:objects count:count];
    id array;
    if (objects != NULL) {
        for (NSUInteger index = 0; index != count; index++) {
            [objects[index] retain];
        }
        array = [[(Class)setArrayClass alloc] _initByAdoptingBuffer:objects count:count size:count];
    } else {
        array = [[(Class)setArrayClass alloc] initWithObjects:NULL count:count];
    }
    return [array autorelease];
}

// Native -[NSSet anyObject], 0x1c029f090. Non-external. The first object of one fast-enumeration batch of 16,
// or nil.
- (id)anyObject {
    setTSANRead(self, __builtin_return_address(0));
    FastEnumerationState state = {0};
    id buffer[16];
    if ([self countByEnumeratingWithState:&state objects:buffer count:16] == 0) {
        return nil;
    }
    return state.itemsPtr[0];
}

// Native -[NSSet containsObject:], 0x1c029f138. Non-external. nil answers NO; otherwise whether -member:
// answers an object.
- (BOOL)containsObject:(id)object {
    setTSANRead(self, __builtin_return_address(0));
    if (object == nil) {
        return NO;
    }
    return [self member:object] != nil;
}

// Native -[NSSet countByEnumeratingWithState:objects:count:], 0x1c029f1a4. Non-external. A NULL buffer with a
// non-zero length and a length above 2^61 - 1 raise NSInvalidArgumentException (the IDA pseudocode drops the
// second check). A finished state (-1) answers 0 before the ThreadSanitizer check. The first call points the
// mutations pointer at extra[0], stores -count there and the -objectEnumerator in extra[1] (answering 0 when
// it is nil). Each call fills at most min(length, count - state) objects from -nextObject; a nil object
// finishes the state (-1) and answers the objects so far; otherwise the state advances, finishing when it
// reaches the count.
- (NSUInteger)countByEnumeratingWithState:(void *)enumerationState
                                  objects:(id *)buffer
                                    count:(NSUInteger)length {
    FastEnumerationState *state = enumerationState;
    if (buffer == NULL && length != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[NSSet countByEnumeratingWithState:objects:count:]",
            (unsigned long)length
        );
    }
    if (OF_WORD_SHIFT(length, 61) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[NSSet countByEnumeratingWithState:objects:count:]",
            (unsigned long)length
        );
    }
    if (state->state == (unsigned long)-1) {
        return 0;
    }
    setTSANRead(self, __builtin_return_address(0));
    id enumerator;
    if (state->state == 0) {
        state->mutationsPtr = &state->extra[0];
        state->extra[0] = [self count];
        enumerator = [self objectEnumerator];
        state->extra[1] = (unsigned long)enumerator;
        if (enumerator == nil) {
            return 0;
        }
    } else {
        enumerator = (id)state->extra[1];
    }
    state->itemsPtr = buffer;
    unsigned long total = state->extra[0];
    unsigned long remaining = total - state->state;
    NSUInteger batch = length < remaining ? length : remaining;
    for (NSUInteger index = 0; index != batch; index++) {
        id object = [enumerator nextObject];
        if (object == nil) {
            state->state = (unsigned long)-1;
            return index;
        }
        buffer[index] = object;
    }
    unsigned long next = state->state + batch;
    state->state = total > next ? next : (unsigned long)-1;
    return batch;
}

// Native -[NSSet countForObject:], 0x1c029f444. Non-external. 1 when -member: answers an object, else 0.
- (NSUInteger)countForObject:(id)object {
    setTSANRead(self, __builtin_return_address(0));
    return [self member:object] != nil;
}

// Native -[NSSet descriptionWithLocale:indent:], 0x1c029f4a4. Non-external. No ThreadSanitizer check. The
// level is capped at 100. The objects are copied with -getObjects:count: into a temporary id buffer; a
// mutable string is made with kCFAllocatorSystemDefault, and inside an autorelease pool each object is
// replaced by its description: an NSString's -_stringRepresentation, else -descriptionWithLocale:indent:
// (level
// + 1) when it responds, else -descriptionWithLocale: when it responds, else -description; nil becomes
// "(null)". The indent string holds four spaces per level; the result is the indent, "{(\n", then per object
// the indent, four spaces, the description and ",\n" (the last "\n"), then the indent and ")}". The indent
// string is released before the pool is popped; the result is autoreleased after it.
- (id)descriptionWithLocale:(id)locale indent:(NSUInteger)level {
    NSUInteger indentLevel = level < 100 ? level : 100;
    NSUInteger count = [self count];
    temporaryIDBuffer(objects, count);
    [self getObjects:objects count:count];
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableStringRef result = OCFStringCreateMutable(allocator, 0);
    @autoreleasepool {
        for (NSUInteger index = 0; index != count; index++) {
            id object = objects[index];
            id description;
            if (_ONSIsNSString(object)) {
                description = [object _stringRepresentation];
            } else if ([object respondsToSelector:@selector(descriptionWithLocale:indent:)]) {
                description = [object descriptionWithLocale:locale indent:indentLevel + 1];
            } else if ([object respondsToSelector:@selector(descriptionWithLocale:)]) {
                description = [object descriptionWithLocale:locale];
            } else {
                description = [object description];
            }
            objects[index] = description != nil ? description : (id)OF_STR("(null)");
        }
        OCFMutableStringRef indent = OCFStringCreateMutable(allocator, 0);
        if (level != 0) {
            do {
                OCFStringAppend(indent, OF_STR("    "));
            } while (--indentLevel != 0);
        }
        OCFStringAppend(result, indent);
        OCFStringAppend(result, OF_STR("{(\n"));
        for (NSUInteger index = 1; index <= count; index++) {
            OCFStringAppend(result, indent);
            OCFStringAppend(result, OF_STR("    "));
            OCFStringAppend(result, (OCFStringRef)objects[index - 1]);
            OCFStringAppend(result, index < count ? OF_STR(",\n") : OF_STR("\n"));
        }
        OCFStringAppend(result, indent);
        OCFStringAppend(result, OF_STR(")}"));
        OCFRelease(indent);
    }
    id description = [(id)result autorelease];
    free(objectsHeap);
    return description;
}

// Native -[NSSet descriptionWithLocale:], 0x1c029f7e8. Non-external. -descriptionWithLocale:indent: with
// level 0.
- (id)descriptionWithLocale:(id)locale {
    setTSANRead(self, __builtin_return_address(0));
    return [self descriptionWithLocale:locale indent:0];
}

// Native -[NSSet description], 0x1c029f840. Non-external. -descriptionWithLocale:indent: with nil and 0.
- (id)description {
    setTSANRead(self, __builtin_return_address(0));
    return [self descriptionWithLocale:nil indent:0];
}

// Native -[NSSet enumerateObjectsWithOptions:usingBlock:], 0x1c029f894. Non-external. A nil block raises
// NSInvalidArgumentException; otherwise __NSSetEnumerate with the options less NSEnumerationReverse (bit 1),
// a tail call.
- (void)enumerateObjectsWithOptions:(NSUInteger)options usingBlock:(void (^)(id object, BOOL *stop))block {
    if (block == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: block cannot be nil",
            "-[NSSet enumerateObjectsWithOptions:usingBlock:]"
        );
    }
    setTSANRead(self, __builtin_return_address(0));
    __NSSetEnumerate(self, options & ~(NSUInteger)2, block);
}

// Native -[NSSet enumerateObjectsUsingBlock:], 0x1c029f9ec. Non-external. A nil block raises
// NSInvalidArgumentException; otherwise -enumerateObjectsWithOptions:usingBlock: with options 0.
- (void)enumerateObjectsUsingBlock:(void (^)(id object, BOOL *stop))block {
    if (block == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, "*** %s: block cannot be nil", "-[NSSet enumerateObjectsUsingBlock:]"
        );
    }
    setTSANRead(self, __builtin_return_address(0));
    [self enumerateObjectsWithOptions:0 usingBlock:block];
}

// Native -[NSSet getObjects:count:], 0x1c029fb40. Non-external. A NULL array with a non-zero count and a
// count above 2^61 - 1 raise NSInvalidArgumentException (the IDA pseudocode drops the second check); then at
// most count objects are copied in fast-enumeration order.
- (void)getObjects:(id *)objects count:(NSUInteger)count {
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[NSSet getObjects:count:]",
            (unsigned long)count
        );
    }
    if (OF_WORD_SHIFT(count, 61) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[NSSet getObjects:count:]",
            (unsigned long)count
        );
    }
    setTSANRead(self, __builtin_return_address(0));
    for (id object in self) {
        if (count == 0) {
            break;
        }
        count--;
        *objects++ = object;
    }
}

// Native -[NSSet getObjects:], 0x1c029fdc4. Non-external. -count is read first; a NULL array with a non-zero
// count and a count above 2^61 - 1 raise NSInvalidArgumentException (the IDA pseudocode drops the second
// check); then -getObjects:count: with -count read again (a tail call).
- (void)getObjects:(id *)objects {
    NSUInteger count = [self count];
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[NSSet getObjects:]",
            (unsigned long)count
        );
    }
    if (OF_WORD_SHIFT(count, 61) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[NSSet getObjects:]",
            (unsigned long)count
        );
    }
    setTSANRead(self, __builtin_return_address(0));
    [self getObjects:objects count:[self count]];
}

// Native -[NSSet getObjects:range:], 0x1c029ffcc. Non-external. Logs (level 3) that an NSArray-taking method
// was passed an NSSet; with objects, -getObjects:count: fills a malloc_type_malloc(16 * count,
// 0x80040B8603338) buffer, and range.length objects from its start (the location is ignored) are moved into
// the array. CoreFoundation defect: the native buffer is never freed; this restoration frees it after the
// move (confidence: verified). The ignored location is kept.
- (void)getObjects:(id *)objects range:(OCFRange)range {
    OCFLog(
        3,
        OF_STR(
            "*** ERROR: this process has called an NSArray-taking method, such as initWithArray:, and passed "
            "in an "
            "NSSet object.  This is being worked-around for now, but will soon cause you grief."
        )
    );
    setTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (count == 0) {
        return;
    }
    id *buffer = (id *)malloc_type_malloc(count * 16, 0x80040B8603338);
    [self getObjects:buffer count:count];
    memmove(objects, buffer, (size_t)range.length * sizeof(id));
    free(buffer);
}

// Native -[NSSet hash], 0x1c02a009c. Non-external. The count.
- (NSUInteger)hash {
    setTSANRead(self, __builtin_return_address(0));
    return [self count];
}

// Native -[NSSet intersectsOrderedSet:], 0x1c02a00e8. Non-external. A non-NSOrderedSet raises
// NSInvalidArgumentException. The smaller collection (the receiver when its count is below the other's) is
// enumerated and each object tested with the other's -containsObject:.
- (BOOL)intersectsOrderedSet:(id)orderedSet {
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSSet intersectsOrderedSet:]"
        );
    }
    setTSANRead(self, __builtin_return_address(0));
    NSUInteger selfCount = [self count];
    NSUInteger otherCount = [orderedSet count];
    id enumerated = selfCount >= otherCount ? orderedSet : self;
    id tested = selfCount >= otherCount ? self : orderedSet;
    for (id object in enumerated) {
        if ([tested containsObject:object]) {
            return YES;
        }
    }
    return NO;
}

// Native -[NSSet intersectsSet:], 0x1c02a02f8. Non-external. The argument is forgiven
// (__checkForAndForgiveClientSin); a remaining non-NSSet raises NSInvalidArgumentException; then the pattern
// of -intersectsOrderedSet:.
- (BOOL)intersectsSet:(id)otherSet {
    if (otherSet != nil) {
        otherSet = __checkForAndForgiveClientSin(self, _cmd, otherSet);
        if (otherSet != nil && !_ONSIsNSSet(otherSet)) {
            raiseWithLogPack(
                ONSInvalidArgumentException, "*** %s: set argument is not an NSSet", "-[NSSet intersectsSet:]"
            );
        }
    }
    setTSANRead(self, __builtin_return_address(0));
    NSUInteger selfCount = [self count];
    NSUInteger otherCount = [otherSet count];
    id enumerated = selfCount >= otherCount ? otherSet : self;
    id tested = selfCount >= otherCount ? self : otherSet;
    for (id object in enumerated) {
        if ([tested containsObject:object]) {
            return YES;
        }
    }
    return NO;
}

// Native -[NSSet isEqualToSet:], 0x1c02a0640. Non-external. The argument is forgiven; a remaining non-NSSet
// raises NSInvalidArgumentException. The receiver itself answers YES and nil NO; otherwise the counts must
// match (the receiver's read first) and every object of the receiver must have the same -countForObject: in
// both (the other set asked first).
- (BOOL)isEqualToSet:(id)otherSet {
    if (otherSet != nil) {
        otherSet = __checkForAndForgiveClientSin(self, _cmd, otherSet);
        if (otherSet != nil && !_ONSIsNSSet(otherSet)) {
            raiseWithLogPack(
                ONSInvalidArgumentException, "*** %s: set argument is not an NSSet", "-[NSSet isEqualToSet:]"
            );
        }
    }
    setTSANRead(self, __builtin_return_address(0));
    if (otherSet == self) {
        return YES;
    }
    if (otherSet == nil) {
        return NO;
    }
    NSUInteger selfCount = [self count];
    if ([otherSet count] != selfCount) {
        return NO;
    }
    for (id object in self) {
        NSUInteger otherCount = [otherSet countForObject:object];
        if (otherCount != [self countForObject:object]) {
            return NO;
        }
    }
    return YES;
}

// Native -[NSSet isEqual:], 0x1c02a09bc. Non-external. The receiver itself answers YES, nil NO; an NSSet is
// compared with -isEqualToSet:, anything else answers NO.
- (BOOL)isEqual:(id)object {
    setTSANRead(self, __builtin_return_address(0));
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    if (!_ONSIsNSSet(object)) {
        return NO;
    }
    return [self isEqualToSet:object];
}

// Native -[NSSet isSubsetOfOrderedSet:], 0x1c02a0a40. Non-external. A non-NSOrderedSet raises
// NSInvalidArgumentException. A receiver larger than the ordered set answers NO; otherwise every object's
// -countForObject: in the ordered set (asked first) must not be below the receiver's, and a nil ordered set
// answers NO.
- (BOOL)isSubsetOfOrderedSet:(id)orderedSet {
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSSet isSubsetOfOrderedSet:]"
        );
    }
    setTSANRead(self, __builtin_return_address(0));
    NSUInteger selfCount = [self count];
    if (selfCount > [orderedSet count]) {
        return NO;
    }
    BOOL subset = YES;
    for (id object in self) {
        NSUInteger otherCount = [orderedSet countForObject:object];
        if (otherCount < [self countForObject:object]) {
            subset = NO;
            break;
        }
    }
    return orderedSet == nil ? NO : subset;
}

// Native -[NSSet isSubsetOfSet:], 0x1c02a0c80. Non-external. The argument is forgiven; a remaining non-NSSet
// raises NSInvalidArgumentException; then the pattern of -isSubsetOfOrderedSet:.
- (BOOL)isSubsetOfSet:(id)otherSet {
    if (otherSet != nil) {
        otherSet = __checkForAndForgiveClientSin(self, _cmd, otherSet);
        if (otherSet != nil && !_ONSIsNSSet(otherSet)) {
            raiseWithLogPack(
                ONSInvalidArgumentException, "*** %s: set argument is not an NSSet", "-[NSSet isSubsetOfSet:]"
            );
        }
    }
    setTSANRead(self, __builtin_return_address(0));
    NSUInteger selfCount = [self count];
    if (selfCount > [otherSet count]) {
        return NO;
    }
    for (id object in self) {
        NSUInteger otherCount = [otherSet countForObject:object];
        if (otherCount < [self countForObject:object]) {
            return NO;
        }
    }
    return otherSet != nil;
}

// Native -[NSSet makeObjectsPerformSelector:], 0x1c02a0fe4. Non-external. A NULL selector is reported with
// -doesNotRecognizeSelector: (passing NULL) before the enumeration; each object is sent -performSelector:.
- (void)makeObjectsPerformSelector:(SEL)selector {
    if (selector == NULL) {
        [self doesNotRecognizeSelector:selector];
    }
    setTSANRead(self, __builtin_return_address(0));
    for (id object in self) {
        [object performSelector:selector];
    }
}

// Native -[NSSet makeObjectsPerformSelector:withObject:], 0x1c02a1108. Non-external. As
// -makeObjectsPerformSelector:, with -performSelector:withObject:.
- (void)makeObjectsPerformSelector:(SEL)selector withObject:(id)argument {
    if (selector == NULL) {
        [self doesNotRecognizeSelector:selector];
    }
    setTSANRead(self, __builtin_return_address(0));
    for (id object in self) {
        [object performSelector:selector withObject:argument];
    }
}

// Native -[NSSet members:notFoundMarker:], 0x1c02a123c. Non-external. A nil marker raises
// NSInvalidArgumentException. A temporary id buffer of the objects' -count receives, per object, the
// receiver's -member: or the marker; heap storage is adopted (each entry retained) by
// -[NSArray _initByAdoptingBuffer:count:size:], a stack buffer is copied by -initWithObjects:count:.
// Autoreleased.
- (id)members:(id)objects notFoundMarker:(id)marker {
    if (marker == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, "*** %s: marker cannot be nil", "-[NSSet members:notFoundMarker:]"
        );
    }
    NSUInteger count = [objects count];
    temporaryIDBuffer(members, count);
    setTSANRead(self, __builtin_return_address(0));
    id *end = members;
    for (id object in objects) {
        id member = [self member:object];
        *end++ = member != nil ? member : marker;
    }
    NSInteger memberCount = end - members;
    id array;
    if (membersHeap != NULL) {
        for (NSInteger index = 0; index < memberCount; index++) {
            [membersHeap[index] retain];
        }
        array = [[(Class)setArrayClass alloc] _initByAdoptingBuffer:membersHeap
                                                              count:(NSUInteger)memberCount
                                                               size:(NSUInteger)memberCount];
    } else {
        array = [[(Class)setArrayClass alloc] initWithObjects:members count:(NSUInteger)memberCount];
    }
    membersHeap = NULL;
    id result = [array autorelease];
    free(membersHeap);
    return result;
}

// Native -[NSSet objectWithOptions:passingTest:], 0x1c02a15a8, with
// ___39-[NSSet objectWithOptions:passingTest:]_block_invoke (0x1c02a174c). Non-external. A nil predicate
// raises NSInvalidArgumentException. __NSSetEnumerate (options less bit 1) calls the predicate with the
// object and a flag of this frame (not the enumeration's); a passing object is recorded (not retained) and
// stops the enumeration, as does the predicate setting its flag (the byte compared with 1).
- (id)objectWithOptions:(NSUInteger)options passingTest:(BOOL (^)(id object, BOOL *stop))predicate {
    if (predicate == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSSet objectWithOptions:passingTest:]"
        );
    }
    setTSANRead(self, __builtin_return_address(0));
    id found = nil;
    BOOL predicateStop = NO;
    id *foundPointer = &found;
    BOOL *predicateStopPointer = &predicateStop;
    __NSSetEnumerate(self, options & ~(NSUInteger)2, ^(id object, BOOL *stop) {
      if (predicate(object, predicateStopPointer)) {
          *foundPointer = object;
          *stop = YES;
      }
      if (*(unsigned char *)predicateStopPointer == 1) {
          *stop = YES;
      }
    });
    return found;
}

// Native -[NSSet objectPassingTest:], 0x1c02a17b0. Non-external. A nil predicate raises
// NSInvalidArgumentException; otherwise -objectWithOptions:passingTest: with options 0.
- (id)objectPassingTest:(BOOL (^)(id object, BOOL *stop))predicate {
    if (predicate == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, "*** %s: predicate cannot be nil", "-[NSSet objectPassingTest:]"
        );
    }
    setTSANRead(self, __builtin_return_address(0));
    return [self objectWithOptions:0 passingTest:predicate];
}

// Native -[NSSet objectsWithOptions:passingTest:], 0x1c02a1904, with
// ___40-[NSSet objectsWithOptions:passingTest:]_block_invoke (0x1c02a1ad4). Non-external. A nil predicate
// raises NSInvalidArgumentException. The objects passing the predicate (called with the enumeration's stop
// flag) are collected into +[NSMutableSet set] under an error-checking mutex (never destroyed) by
// __NSSetEnumerate (options less bit 1); the answer is +[NSSet setWithSet:] of them.
- (id)objectsWithOptions:(NSUInteger)options passingTest:(BOOL (^)(id object, BOOL *stop))predicate {
    if (predicate == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSSet objectsWithOptions:passingTest:]"
        );
    }
    setTSANRead(self, __builtin_return_address(0));
    id passing = [ONSMutableSet set];
    pthread_mutex_t lock = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
    pthread_mutex_t *lockPointer = &lock;
    __NSSetEnumerate(self, options & ~(NSUInteger)2, ^(id object, BOOL *stop) {
      if (predicate(object, stop)) {
          pthread_mutex_lock(lockPointer);
          [passing addObject:object];
          pthread_mutex_unlock(lockPointer);
      }
    });
    return [ONSSet setWithSet:passing];
}

// Native -[NSSet objectsPassingTest:], 0x1c02a1b28. Non-external. A nil predicate raises
// NSInvalidArgumentException; otherwise -objectsWithOptions:passingTest: with options 0.
- (id)objectsPassingTest:(BOOL (^)(id object, BOOL *stop))predicate {
    if (predicate == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, "*** %s: predicate cannot be nil", "-[NSSet objectsPassingTest:]"
        );
    }
    setTSANRead(self, __builtin_return_address(0));
    return [self objectsWithOptions:0 passingTest:predicate];
}

// Native -[NSSet sortedArrayWithOptions:usingComparator:], 0x1c02a1c7c, with
// ___48-[NSSet sortedArrayWithOptions:usingComparator:]_block_invoke (0x1c02a2050). Non-external. A nil
// comparator raises NSInvalidArgumentException. An empty set answers +[NSArray array]. Otherwise the objects
// are copied with -getObjects:count: into a temporary id buffer (freed on unwinding too); a second
// _CFCreateArrayStorage(count) storage receives the sorted objects (freed on unwinding until adopted); the
// indexes live on the stack up to 4096 entries, else in malloc_type_malloc(8 * count, 0x100004000313F17),
// and are sorted by CFSortIndexes with the options and a block comparing the indexed objects. The sorted
// objects are retained and adopted by -[NSArray _initByAdoptingBuffer:count:size:], autoreleased.
- (id)sortedArrayWithOptions:(NSUInteger)options usingComparator:(OCFComparisonResult (^)(id, id))comparator {
    if (comparator == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: comparator cannot be nil",
            "-[NSSet sortedArrayWithOptions:usingComparator:]"
        );
    }
    setTSANRead(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    if (count == 0) {
        return [(id<SetUnrestoredMessages>)(id)setArrayClass array];
    }
    if (OF_WORD_SHIFT(count, 60) != 0) {
        raiseTemporaryBufferTooLarge(count);
    }
    id objectsStack[count < 0x101 ? count : 1];
    size_t objectsSize = 0;
    __attribute__((cleanup(freeTemporaryStorage))) id *objectsHeap = NULL;
    id *objects = objectsStack;
    if (count >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(count, false, &objectsSize);
        objectsHeap = objects;
    }
    size_t sortedSize = 0;
    __attribute__((cleanup(freeTemporaryStorage))) id *sorted =
        (id *)_OCFCreateArrayStorage(count, false, &sortedSize);
    [self getObjects:objects count:count];
    OCFIndex indexesStack[count > 0x1000 ? 1 : count];
    OCFIndex *indexes = indexesStack;
    if (count > 0x1000) {
        indexes = (OCFIndex *)malloc_type_malloc(count * sizeof(OCFIndex), 0x100004000313F17);
    }
    OCFSortIndexes(
        indexes, (OCFIndex)count, (OCFOptionFlags)options, ^OCFComparisonResult(OCFIndex i1, OCFIndex i2) {
          return comparator(objects[i1], objects[i2]);
        }
    );
    for (NSUInteger index = 0; index != count; index++) {
        sorted[index] = objects[indexes[index]];
    }
    if (indexesStack != indexes) {
        free(indexes);
    }
    for (NSUInteger index = 0; index != count; index++) {
        [sorted[index] retain];
    }
    id array = [[(Class)setArrayClass alloc] _initByAdoptingBuffer:sorted count:count size:count];
    sorted = NULL;
    return [array autorelease];
}

// Native -[NSSet sortedArrayUsingComparator:], 0x1c02a2068. Non-external. A nil comparator raises
// NSInvalidArgumentException; otherwise -sortedArrayWithOptions:usingComparator: with options 0.
- (id)sortedArrayUsingComparator:(OCFComparisonResult (^)(id, id))comparator {
    if (comparator == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: comparator cannot be nil",
            "-[NSSet sortedArrayUsingComparator:]"
        );
    }
    setTSANRead(self, __builtin_return_address(0));
    return [self sortedArrayWithOptions:0 usingComparator:comparator];
}

// Native -[NSSet __applyValues:context:], 0x1c02a21bc. Non-external. No ThreadSanitizer check. A NULL
// function throws +[NSException exceptionWithName:reason:userInfo:] (NSInvalidArgumentException, "%@:
// function pointer is NULL" with the proem, kCFAllocatorSystemDefault, autoreleased); otherwise the function
// gets each object and the context.
- (void)__applyValues:(void (*)(const void *value, void *context))applier context:(void *)context {
    if (applier == NULL) {
        OCFStringRef proem = __CFExceptionProem(self, _cmd);
        OCFStringRef reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault, NULL, OF_STR("%@: function pointer is NULL"), proem
        );
        objc_exception_throw([(id<SetUnrestoredMessages>)(id)setExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                     userInfo:nil]);
    }
    for (id object in self) {
        applier(object, context);
    }
}

// Native -[NSSet __getValue:forObj:], 0x1c02a2310. Non-external. No ThreadSanitizer check. The member is
// stored when both it and the out pointer are not nil; answers whether there is a member.
- (BOOL)__getValue:(id *)value forObj:(id)object {
    id member = [self member:object];
    if (value != NULL && member != nil) {
        *value = member;
    }
    return member != nil;
}

// Native +[NSSet allocWithZone:], 0x1c02a2348. Non-external. NSSet itself answers
// __NSSetImmutablePlaceholder() and NSMutableSet __NSSetMutablePlaceholder() (tail calls); other classes go
// to super.
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == (Class)setClassSymbol) {
        return __NSSetImmutablePlaceholder();
    }
    if (self == (Class)mutableSetClassSymbol) {
        return __NSSetMutablePlaceholder();
    }
    return [super allocWithZone:zone];
}

// Native -[NSSet initWithObject:], 0x1c02a2410. Non-external. -initWithObjects:count: with the one object.
- (id)initWithObject:(id)object {
    id objects[1] = {object};
    return [self initWithObjects:objects count:1];
}

// Native -[NSSet initWithObjects:], 0x1c02a2464. Non-external. The objects up to the first nil are counted.
// For a receiver whose class is exactly NSSet (object_getClass), no objects answer __NSSet0__ (not retained)
// and objects answer __NSSetI_new(&firstObject, the variadic list, count, NO). Otherwise a temporary id
// buffer receives the objects and -initWithObjects:count: is sent.
- (id)initWithObjects:(id)firstObject, ... {
    va_list args;
    va_start(args, firstObject);
    Class cls = object_getClass(self);
    NSUInteger count;
    if (firstObject != nil) {
        va_list counter;
        va_copy(counter, args);
        count = 0;
        do {
            count++;
        } while (va_arg(counter, id) != nil);
        va_end(counter);
        if (cls == (Class)setClassSymbol) {
            id set = __NSSetI_new(&firstObject, args, count, NO);
            va_end(args);
            return set;
        }
    } else {
        if (cls == (Class)setClassSymbol) {
            va_end(args);
            return __NSSet0__;
        }
        count = 0;
    }
    temporaryIDBuffer(objects, count);
    if (count != 0) {
        objects[0] = firstObject;
        for (NSUInteger index = 1; index != count; index++) {
            objects[index] = va_arg(args, id);
        }
    }
    va_end(args);
    self = [self initWithObjects:objects count:count];
    free(objectsHeap);
    return self;
}

// Native -[NSSet initWithArray:range:copyItems:], 0x1c02a267c. Non-external. A non-NSArray raises
// NSInvalidArgumentException; a negative location or length, or a range past the count, raises
// NSRangeException (the bounds message for a non-empty array, else the empty-set message). The range is
// copied with -getObjects:range: into a temporary id buffer; with flag (and objects) each entry is replaced
// by its -copyWithZone: NULL, -initWithObjects:count: is sent, and the copies are released.
- (id)initWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)flag {
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: array argument is not an NSArray",
            "-[NSSet initWithArray:range:copyItems:]"
        );
    }
    NSUInteger count = [array count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        if (count != 0) {
            raiseWithLogPack(
                ONSRangeException,
                "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
                "-[NSSet initWithArray:range:copyItems:]",
                (unsigned long)range.location,
                (unsigned long)range.length,
                (unsigned long)(count - 1)
            );
        }
        raiseWithLogPack(
            ONSRangeException,
            "*** %s: range {%lu, %lu} extends beyond bounds for empty set",
            "-[NSSet initWithArray:range:copyItems:]",
            (unsigned long)range.location,
            (unsigned long)range.length
        );
    }
    NSUInteger length = (NSUInteger)range.length;
    temporaryIDBuffer(objects, length);
    [array getObjects:objects range:range];
    BOOL copyItems = length != 0 && flag;
    if (copyItems) {
        for (NSUInteger index = 0; index != length; index++) {
            objects[index] = [objects[index] copyWithZone:NULL];
        }
    }
    self = [self initWithObjects:objects count:length];
    if (copyItems) {
        for (NSUInteger index = 0; index != length; index++) {
            [objects[index] release];
        }
    }
    free(objectsHeap);
    return self;
}

// Native -[NSSet initWithArray:range:], 0x1c02a2ac4. Non-external. Without copying (a tail call).
- (id)initWithArray:(id)array range:(OCFRange)range {
    return [self initWithArray:array range:range copyItems:NO];
}

// Native -[NSSet initWithArray:copyItems:], 0x1c02a2acc. Non-external. The whole array.
- (id)initWithArray:(id)array copyItems:(BOOL)flag {
    return [self initWithArray:array range:(OCFRange){0, (OCFIndex)[array count]} copyItems:flag];
}

// Native -[NSSet initWithArray:], 0x1c02a2b14. Non-external. The whole array, without copying.
- (id)initWithArray:(id)array {
    return [self initWithArray:array range:(OCFRange){0, (OCFIndex)[array count]} copyItems:NO];
}

// Native -[NSSet initWithOrderedSet:range:copyItems:], 0x1c02a2b50. Non-external. The pattern of
// -initWithArray:range:copyItems: with _NSIsNSOrderedSet and the ordered-set message.
- (id)initWithOrderedSet:(id)orderedSet range:(OCFRange)range copyItems:(BOOL)flag {
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSSet initWithOrderedSet:range:copyItems:]"
        );
    }
    NSUInteger count = [orderedSet count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        if (count != 0) {
            raiseWithLogPack(
                ONSRangeException,
                "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
                "-[NSSet initWithOrderedSet:range:copyItems:]",
                (unsigned long)range.location,
                (unsigned long)range.length,
                (unsigned long)(count - 1)
            );
        }
        raiseWithLogPack(
            ONSRangeException,
            "*** %s: range {%lu, %lu} extends beyond bounds for empty set",
            "-[NSSet initWithOrderedSet:range:copyItems:]",
            (unsigned long)range.location,
            (unsigned long)range.length
        );
    }
    NSUInteger length = (NSUInteger)range.length;
    temporaryIDBuffer(objects, length);
    [orderedSet getObjects:objects range:range];
    BOOL copyItems = length != 0 && flag;
    if (copyItems) {
        for (NSUInteger index = 0; index != length; index++) {
            objects[index] = [objects[index] copyWithZone:NULL];
        }
    }
    self = [self initWithObjects:objects count:length];
    if (copyItems) {
        for (NSUInteger index = 0; index != length; index++) {
            [objects[index] release];
        }
    }
    free(objectsHeap);
    return self;
}

// Native -[NSSet initWithOrderedSet:range:], 0x1c02a2f98. Non-external. Without copying (a tail call).
- (id)initWithOrderedSet:(id)orderedSet range:(OCFRange)range {
    return [self initWithOrderedSet:orderedSet range:range copyItems:NO];
}

// Native -[NSSet initWithOrderedSet:copyItems:], 0x1c02a2fa0. Non-external. The whole ordered set.
- (id)initWithOrderedSet:(id)orderedSet copyItems:(BOOL)flag {
    return [self initWithOrderedSet:orderedSet
                              range:(OCFRange){0, (OCFIndex)[orderedSet count]}
                          copyItems:flag];
}

// Native -[NSSet initWithOrderedSet:], 0x1c02a2fe8. Non-external. The whole ordered set, without copying.
- (id)initWithOrderedSet:(id)orderedSet {
    return [self initWithOrderedSet:orderedSet
                              range:(OCFRange){0, (OCFIndex)[orderedSet count]}
                          copyItems:NO];
}

// Native -[NSSet initWithSet:copyItems:], 0x1c02a3024. Non-external. The argument is forgiven; a remaining
// non-NSSet raises NSInvalidArgumentException. The set's objects are copied with -getObjects:count: into a
// temporary id buffer of its -count; with flag (and objects) each is replaced by its -copyWithZone: NULL,
// -initWithObjects:count: is sent, and the copies are released.
- (id)initWithSet:(id)set copyItems:(BOOL)flag {
    if (set != nil) {
        set = __checkForAndForgiveClientSin(self, _cmd, set);
        if (set != nil && !_ONSIsNSSet(set)) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                "*** %s: set argument is not an NSSet",
                "-[NSSet initWithSet:copyItems:]"
            );
        }
    }
    NSUInteger count = [set count];
    temporaryIDBuffer(objects, count);
    [set getObjects:objects count:count];
    BOOL copyItems = count != 0 && flag;
    if (copyItems) {
        for (NSUInteger index = 0; index != count; index++) {
            objects[index] = [objects[index] copyWithZone:NULL];
        }
    }
    self = [self initWithObjects:objects count:count];
    if (copyItems) {
        for (NSUInteger index = 0; index != count; index++) {
            [objects[index] release];
        }
    }
    free(objectsHeap);
    return self;
}

// Native -[NSSet initWithSet:], 0x1c02a33e0. Non-external. Without copying (a tail call).
- (id)initWithSet:(id)set {
    return [self initWithSet:set copyItems:NO];
}

// Native +[NSSet setWithObjects:count:], 0x1c02a33e8. Non-external. objc_alloc, -initWithObjects:count:,
// autoreleased.
+ (id)setWithObjects:(const id *)objects count:(NSUInteger)count {
    return [[[self alloc] initWithObjects:objects count:count] autorelease];
}

// Native +[NSSet setWithObject:], 0x1c02a3418. Non-external. __createSet of objc_alloc, autoreleased.
+ (id)setWithObject:(id)object {
    return [__createSet([self alloc], object) autorelease];
}

// Native __createSet, 0x1c02a3440. Non-external. -initWithObjects:count: with the one object.
static id __createSet(id set, id object) {
    id objects[1] = {object};
    return [set initWithObjects:objects count:1];
}

// Native +[NSSet set], 0x1c02a3494. Non-external. objc_alloc, -initWithObjects:count: (NULL, 0),
// autoreleased.
+ (id)set {
    return [[[self alloc] initWithObjects:NULL count:0] autorelease];
}

// Native +[NSSet newSetWithObjects:count:], 0x1c02a34b4. Non-external. A NULL array with a non-zero count, a
// count above 2^61 - 1 and a nil object raise NSInvalidArgumentException (the IDA pseudocode drops the nil
// check). NSSet itself answers the retained __NSSet0__ for no objects, __NSSingleObjectSetI_new(object, YES)
// for one and __NSSetI_new(objects, NULL, count, YES) for more (the objects' references are taken over);
// NSMutableSet answers __NSSetM_new(objects, count, NO); any other class reaches
// __CFRequireConcreteImplementation (__CFLookUpClass("NSSet")) and answers nil.
+ (id)newSetWithObjects:(const id *)objects count:(NSUInteger)count {
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "+[NSSet newSetWithObjects:count:]",
            (unsigned long)count
        );
    }
    if (OF_WORD_SHIFT(count, 61) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "+[NSSet newSetWithObjects:count:]",
            (unsigned long)count
        );
    }
    for (NSUInteger index = 0; index != count; index++) {
        if (objects[index] == nil) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                "*** %s: attempt to insert nil object from objects[%lu]",
                "+[NSSet newSetWithObjects:count:]",
                (unsigned long)index
            );
        }
    }
    if (self == (Class)setClassSymbol) {
        if (count == 1) {
            return __NSSingleObjectSetI_new(objects[0], YES);
        }
        if (count != 0) {
            return __NSSetI_new(objects, NULL, count, YES);
        }
        return objc_retain(__NSSet0__);
    }
    if (self == (Class)mutableSetClassSymbol) {
        return __NSSetM_new(objects, count, NO);
    }
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSSet)), self, _cmd);
    return nil;
}

// Native +[NSSet setWithObjects:], 0x1c02a383c. Non-external. The objects up to the first nil are counted.
// NSSet itself answers the unretained __NSSet0__ for no objects and an autoreleased
// __NSSetI_new(&firstObject, the variadic list, count, NO) otherwise; other classes get a temporary id buffer
// of the objects, objc_alloc and -initWithObjects:count:, autoreleased.
+ (id)setWithObjects:(id)firstObject, ... {
    va_list args;
    va_start(args, firstObject);
    NSUInteger count;
    if (firstObject != nil) {
        va_list counter;
        va_copy(counter, args);
        count = 0;
        do {
            count++;
        } while (va_arg(counter, id) != nil);
        va_end(counter);
        if (self == (Class)setClassSymbol) {
            id set = __NSSetI_new(&firstObject, args, count, NO);
            va_end(args);
            return [set autorelease];
        }
    } else {
        if (self == (Class)setClassSymbol) {
            va_end(args);
            return __NSSet0__;
        }
        count = 0;
    }
    temporaryIDBuffer(objects, count);
    if (count != 0) {
        objects[0] = firstObject;
        for (NSUInteger index = 1; index != count; index++) {
            objects[index] = va_arg(args, id);
        }
    }
    va_end(args);
    id set = [[self alloc] initWithObjects:objects count:count];
    free(objectsHeap);
    return [set autorelease];
}

// Native +[NSSet setWithArray:range:copyItems:], 0x1c02a3a5c. Non-external. objc_alloc,
// -initWithArray:range:copyItems:, autoreleased.
+ (id)setWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)flag {
    return [[[self alloc] initWithArray:array range:range copyItems:flag] autorelease];
}

// Native +[NSSet setWithArray:range:], 0x1c02a3aa4. Non-external. Without copying.
+ (id)setWithArray:(id)array range:(OCFRange)range {
    return [[[self alloc] initWithArray:array range:range copyItems:NO] autorelease];
}

// Native +[NSSet setWithArray:copyItems:], 0x1c02a3ae8. Non-external. The whole array (its -count read after
// objc_alloc).
+ (id)setWithArray:(id)array copyItems:(BOOL)flag {
    id set = [self alloc];
    return [[set initWithArray:array range:(OCFRange){0, (OCFIndex)[array count]}
                     copyItems:flag] autorelease];
}

// Native +[NSSet setWithArray:], 0x1c02a3b38. Non-external. The whole array, without copying.
+ (id)setWithArray:(id)array {
    id set = [self alloc];
    return [[set initWithArray:array range:(OCFRange){0, (OCFIndex)[array count]} copyItems:NO] autorelease];
}

// Native +[NSSet setWithOrderedSet:range:copyItems:], 0x1c02a3b7c. Non-external. objc_alloc,
// -initWithOrderedSet:range:copyItems:, autoreleased.
+ (id)setWithOrderedSet:(id)orderedSet range:(OCFRange)range copyItems:(BOOL)flag {
    return [[[self alloc] initWithOrderedSet:orderedSet range:range copyItems:flag] autorelease];
}

// Native +[NSSet setWithOrderedSet:range:], 0x1c02a3bc4. Non-external. Without copying.
+ (id)setWithOrderedSet:(id)orderedSet range:(OCFRange)range {
    return [[[self alloc] initWithOrderedSet:orderedSet range:range copyItems:NO] autorelease];
}

// Native +[NSSet setWithOrderedSet:copyItems:], 0x1c02a3c08. Non-external. The whole ordered set.
+ (id)setWithOrderedSet:(id)orderedSet copyItems:(BOOL)flag {
    id set = [self alloc];
    return [[set initWithOrderedSet:orderedSet
                              range:(OCFRange){0, (OCFIndex)[orderedSet count]}
                          copyItems:flag] autorelease];
}

// Native +[NSSet setWithOrderedSet:], 0x1c02a3c58. Non-external. The whole ordered set, without copying.
+ (id)setWithOrderedSet:(id)orderedSet {
    id set = [self alloc];
    return [[set initWithOrderedSet:orderedSet range:(OCFRange){0, (OCFIndex)[orderedSet count]}
                          copyItems:NO] autorelease];
}

// Native +[NSSet setWithSet:copyItems:], 0x1c02a3c9c. Non-external. objc_alloc, -initWithSet:copyItems:,
// autoreleased.
+ (id)setWithSet:(id)set copyItems:(BOOL)flag {
    return [[[self alloc] initWithSet:set copyItems:flag] autorelease];
}

// Native +[NSSet setWithSet:], 0x1c02a3ccc. Non-external. Without copying.
+ (id)setWithSet:(id)set {
    return [[[self alloc] initWithSet:set copyItems:NO] autorelease];
}

// Native -[NSSet setByAddingObject:], 0x1c02a3cf8. Non-external. No ThreadSanitizer check. A nil object
// raises NSInvalidArgumentException; a contained object answers the receiver retained and autoreleased.
// Otherwise a temporary id buffer of count + 1 (heap storage freed on unwinding too) receives the receiver's
// objects and the object; the answer is +[NSSet setWithObjects:count:].
- (id)setByAddingObject:(id)object {
    if (object == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, "*** %s: object cannot be nil", "-[NSSet setByAddingObject:]"
        );
    }
    if ([self containsObject:object]) {
        return [[self retain] autorelease];
    }
    NSUInteger count = [self count];
    NSUInteger total = count + 1;
    if (OF_WORD_SHIFT(total, 60) != 0) {
        raiseTemporaryBufferTooLarge(total);
    }
    NSUInteger bufferCount = total > 1 ? total : 1;
    id objectsStack[total < 0x101 ? bufferCount : 1];
    size_t objectsSize = 0;
    __attribute__((cleanup(freeTemporaryStorage))) id *objectsHeap = NULL;
    id *objects = objectsStack;
    if (total >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(bufferCount, false, &objectsSize);
        objectsHeap = objects;
    }
    [self getObjects:objects count:count];
    objects[count] = object;
    return [ONSSet setWithObjects:objects count:total];
}

// Native -[NSSet setByAddingObjectsFromArray:], 0x1c02a3f80. Non-external. No ThreadSanitizer check. A
// non-NSArray raises NSInvalidArgumentException; an empty array answers the receiver retained and
// autoreleased. Otherwise a temporary id buffer of the sum (heap storage freed on unwinding too) receives the
// receiver's objects (when it has any) and the array's (-getObjects:range: {0, count}); the answer is
// +[NSSet setWithObjects:count:].
- (id)setByAddingObjectsFromArray:(id)array {
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: array argument is not an NSArray",
            "-[NSSet setByAddingObjectsFromArray:]"
        );
    }
    NSUInteger arrayCount = [array count];
    if (arrayCount == 0) {
        return [[self retain] autorelease];
    }
    NSUInteger count = [self count];
    NSUInteger total = count + arrayCount;
    if (OF_WORD_SHIFT(total, 60) != 0) {
        raiseTemporaryBufferTooLarge(total);
    }
    NSUInteger bufferCount = total > 1 ? total : 1;
    id objectsStack[total < 0x101 ? bufferCount : 1];
    size_t objectsSize = 0;
    __attribute__((cleanup(freeTemporaryStorage))) id *objectsHeap = NULL;
    id *objects = objectsStack;
    if (total >= 0x101) {
        objects = (id *)_OCFCreateArrayStorage(bufferCount, false, &objectsSize);
        objectsHeap = objects;
    }
    if (count != 0) {
        [self getObjects:objects count:count];
    }
    [array getObjects:objects + count range:(OCFRange){0, (OCFIndex)arrayCount}];
    return [ONSSet setWithObjects:objects count:total];
}

// Native -[NSSet setByAddingObjectsFromSet:], 0x1c02a4234. Non-external. No ThreadSanitizer check. The
// argument is forgiven; a remaining non-NSSet raises NSInvalidArgumentException; an empty set answers the
// receiver retained and autoreleased. Otherwise a temporary id buffer of the sum (no unwinding cleanup here)
// receives the receiver's objects (when it has any) and the set's; the answer is
// +[NSSet setWithObjects:count:].
- (id)setByAddingObjectsFromSet:(id)otherSet {
    if (otherSet != nil) {
        otherSet = __checkForAndForgiveClientSin(self, _cmd, otherSet);
        if (otherSet != nil && !_ONSIsNSSet(otherSet)) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                "*** %s: set argument is not an NSSet",
                "-[NSSet setByAddingObjectsFromSet:]"
            );
        }
    }
    NSUInteger otherCount = [otherSet count];
    if (otherCount == 0) {
        return [[self retain] autorelease];
    }
    NSUInteger count = [self count];
    NSUInteger total = otherCount + count;
    temporaryIDBuffer(objects, total);
    if (count != 0) {
        [self getObjects:objects count:count];
    }
    [otherSet getObjects:objects + count count:otherCount];
    id result = [ONSSet setWithObjects:objects count:total];
    free(objectsHeap);
    return result;
}

// Native -[NSSet copyWithZone:], 0x1c02a45f0. Non-external. __NSCollectionsShouldCopy is asked before the
// ThreadSanitizer check; the answer is +[NSSet allocWithZone:] sent -initWithSet:copyItems: with it.
- (id)copyWithZone:(struct _NSZone *)zone {
    BOOL shouldCopy = __NSCollectionsShouldCopy();
    setTSANRead(self, __builtin_return_address(0));
    return [[ONSSet allocWithZone:zone] initWithSet:self copyItems:shouldCopy];
}

// Native -[NSSet mutableCopyWithZone:], 0x1c02a4668. Non-external. +[NSMutableSet allocWithZone:] sent
// -initWithSet:copyItems: NO.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    setTSANRead(self, __builtin_return_address(0));
    return [[ONSMutableSet allocWithZone:zone] initWithSet:self copyItems:NO];
}

// Native +[NSSet supportsSecureCoding], 0x1c02a46cc. Non-external. Answers YES.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSSet initWithCoder:], 0x1c02a46d4. Non-external. Answers nil (the receiver is not released).
- (id)initWithCoder:(id)coder {
    return nil;
}

// Native -[NSSet encodeWithCoder:], 0x1c02a46dc. Non-external. Does nothing.
- (void)encodeWithCoder:(id)coder {
}

// Native -[NSSet count], 0x1c030fc20, with OUTLINED_FUNCTION_0_44 (0x1c02a46e0: __CFLookUpClass("NSSet")),
// _1_31 (0x1c02a46f4: the tail call of __CFRequireConcreteImplementation with the receiver and selector) and
// _2_27 (0x1c02a4700: the epilogue). Non-external. The abstract method; answers 0.
- (NSUInteger)count {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSSet)), self, _cmd);
    return 0;
}

// Native -[NSSet member:], 0x1c030fc40, with the same outlined fragments. Non-external. The abstract method;
// answers nil.
- (id)member:(id)object {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSSet)), self, _cmd);
    return nil;
}

// Native -[NSSet objectEnumerator], 0x1c030fc60, with the same outlined fragments. Non-external. The abstract
// method; answers nil.
- (id)objectEnumerator {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSSet)), self, _cmd);
    return nil;
}

// Native -[NSSet initWithObjects:count:], 0x1c030fc80, with the same outlined fragments. Non-external. The
// abstract method; answers nil.
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSSet)), self, _cmd);
    return nil;
}

@end
