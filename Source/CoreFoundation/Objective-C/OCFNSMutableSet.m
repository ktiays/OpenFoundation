//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSMutableSet.h"
#include "CoreFoundation/Objective-C/OCFNSSetEnumerate.h"

#include <objc/runtime.h>
#include <pthread.h>

// Inferred organization: the NSMutableSet methods lie together in the iOS Simulator __text section
// (0x1c0151fc4-0x1c0156d5c, after the __NSOrderedSetI methods, with OUTLINED_FUNCTION_0 at 0x1c0156d48), and
// the three abstract methods -addObject:, -removeObject: and -initWithCapacity: in the outlined region
// (0x1c0306074-0x1c03060e4); following the owner's decision, the class has its own translation unit. The
// unit is Objective-C compiled with manual reference counting (objc_autorelease, explicit retain/release
// calls). Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full,
// iphone-simulator-corpus-delta8.json). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).
//
// The mutation methods send -_mutate before the ThreadSanitizer write check. The argument checks raise
// through raiseWithLogPack (Objective-C/OCFNSSet.h) with the method's name as the %s argument; a nil
// argument passes the type checks and behaves as an empty collection.

@implementation ONSMutableSet

// Native -[NSMutableSet _mutate], 0x1c0151fc4. Non-external. Does nothing.
- (void)_mutate {
}

// Native -[NSMutableSet addObjects:count:], 0x1c0151fc8. Non-external. A NULL array with a non-zero count, a
// count above 2^61 - 1 and a nil object (all objects are checked before any is added) raise
// NSInvalidArgumentException; then each object is added with -addObject:.
- (void)addObjects:(const id *)objects count:(NSUInteger)count {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[NSMutableSet addObjects:count:]",
            (unsigned long)count
        );
    }
    if (count >> 61 != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[NSMutableSet addObjects:count:]",
            (unsigned long)count
        );
    }
    for (NSUInteger index = 0; index < count; index++) {
        if (objects[index] == nil) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                "*** %s: attempt to insert nil object from objects[%lu]",
                "-[NSMutableSet addObjects:count:]",
                (unsigned long)index
            );
        }
    }
    for (NSUInteger index = 0; index < count; index++) {
        [self addObject:objects[index]];
    }
}

// Native -[NSMutableSet addObjectsFromArray:range:], 0x1c01522b4. Non-external. A non-NSArray raises
// NSInvalidArgumentException; a negative location or length, or a range past the count, raises
// NSRangeException (the bounds message for a non-empty array, else the empty-set message). The objects are
// copied with -getObjects:range: into a temporary id buffer and added with -addObjects:count:.
- (void)addObjectsFromArray:(id)array range:(OCFRange)range {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: array argument is not an NSArray",
            "-[NSMutableSet addObjectsFromArray:range:]"
        );
    }
    NSUInteger count = [array count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        if (count != 0) {
            raiseWithLogPack(
                ONSRangeException,
                "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
                "-[NSMutableSet addObjectsFromArray:range:]",
                (unsigned long)range.location,
                (unsigned long)range.length,
                (unsigned long)(count - 1)
            );
        }
        raiseWithLogPack(
            ONSRangeException,
            "*** %s: range {%lu, %lu} extends beyond bounds for empty set",
            "-[NSMutableSet addObjectsFromArray:range:]",
            (unsigned long)range.location,
            (unsigned long)range.length
        );
    }
    temporaryIDBuffer(objects, range.length);
    [array getObjects:objects range:range];
    [self addObjects:objects count:(NSUInteger)range.length];
    free(objectsHeap);
}

// Native -[NSMutableSet addObjectsFromArray:], 0x1c01526dc. Non-external. Before the mutation, an argument
// that is not kind of objc_lookUpClass("NSArray") logs (level 3) the reason "*** %s: array argument is not an
// NSArray" (kCFAllocatorSystemDefault, autoreleased) and, when it is kind of objc_lookUpClass("NSSet"), is
// replaced by its -allObjects. Then a non-NSArray raises NSInvalidArgumentException, and each object is
// added.
- (void)addObjectsFromArray:(id)array {
    if (array != nil && ![array isKindOfClass:objc_lookUpClass(OF_NAME_STRING(ONSArray))]) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("*** %s: array argument is not an NSArray"),
            "-[NSMutableSet addObjectsFromArray:]"
        );
        OCFLog(3, OF_STR("%@"), _OCFAutoreleasePoolAddObject(NULL, reason));
        if ([array isKindOfClass:objc_lookUpClass(OF_NAME_STRING(ONSSet))]) {
            array = [array allObjects];
        }
    }
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: array argument is not an NSArray",
            "-[NSMutableSet addObjectsFromArray:]"
        );
    }
    for (id object in array) {
        [self addObject:object];
    }
}

// Native -[NSMutableSet addObjectsFromOrderedSet:range:], 0x1c0152964. Non-external. The pattern of
// -addObjectsFromArray:range: with _NSIsNSOrderedSet and the ordered-set message.
- (void)addObjectsFromOrderedSet:(id)orderedSet range:(OCFRange)range {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSMutableSet addObjectsFromOrderedSet:range:]"
        );
    }
    NSUInteger count = [orderedSet count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        if (count != 0) {
            raiseWithLogPack(
                ONSRangeException,
                "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
                "-[NSMutableSet addObjectsFromOrderedSet:range:]",
                (unsigned long)range.location,
                (unsigned long)range.length,
                (unsigned long)(count - 1)
            );
        }
        raiseWithLogPack(
            ONSRangeException,
            "*** %s: range {%lu, %lu} extends beyond bounds for empty set",
            "-[NSMutableSet addObjectsFromOrderedSet:range:]",
            (unsigned long)range.location,
            (unsigned long)range.length
        );
    }
    temporaryIDBuffer(objects, range.length);
    [orderedSet getObjects:objects range:range];
    [self addObjects:objects count:(NSUInteger)range.length];
    free(objectsHeap);
}

// Native -[NSMutableSet addObjectsFromOrderedSet:], 0x1c0152d8c. Non-external. A non-NSOrderedSet raises
// NSInvalidArgumentException; each object is added.
- (void)addObjectsFromOrderedSet:(id)orderedSet {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSMutableSet addObjectsFromOrderedSet:]"
        );
    }
    for (id object in orderedSet) {
        [self addObject:object];
    }
}

// Native -[NSMutableSet addObjectsFromSet:], 0x1c0152f78. Non-external. The argument is forgiven
// (__checkForAndForgiveClientSin); a remaining non-NSSet raises NSInvalidArgumentException; each object is
// added.
- (void)addObjectsFromSet:(id)otherSet {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (otherSet != nil) {
        otherSet = __checkForAndForgiveClientSin(self, _cmd, otherSet);
        if (otherSet != nil && !_ONSIsNSSet(otherSet)) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                "*** %s: set argument is not an NSSet",
                "-[NSMutableSet addObjectsFromSet:]"
            );
        }
    }
    for (id object in otherSet) {
        [self addObject:object];
    }
}

// Native -[NSMutableSet intersectOrderedSet:], 0x1c0153294. Non-external. A non-NSOrderedSet raises
// NSInvalidArgumentException. The receiver's total multiplicity sizes a temporary id buffer; each object of
// the receiver is entered as many times as the receiver's -countForObject: exceeds the ordered set's (the
// ordered set is asked first); then each entry is removed with -removeObject:.
- (void)intersectOrderedSet:(id)orderedSet {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSMutableSet intersectOrderedSet:]"
        );
    }
    NSUInteger total = 0;
    for (id object in self) {
        total += [self countForObject:object];
    }
    temporaryIDBuffer(objects, total);
    id *end = objects;
    for (id object in self) {
        NSUInteger otherCount = [orderedSet countForObject:object];
        NSUInteger selfCount = [self countForObject:object];
        if (selfCount > otherCount) {
            for (NSUInteger extra = selfCount - otherCount; extra != 0; extra--) {
                *end++ = object;
            }
        }
    }
    while (objects < end) {
        [self removeObject:*objects++];
    }
    free(objectsHeap);
}

// Native -[NSMutableSet intersectSet:], 0x1c0153668. Non-external. The argument is forgiven; a remaining
// non-NSSet raises NSInvalidArgumentException. Intersecting with the receiver does nothing; otherwise the
// pattern of -intersectOrderedSet:.
- (void)intersectSet:(id)otherSet {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (otherSet != nil) {
        otherSet = __checkForAndForgiveClientSin(self, _cmd, otherSet);
        if (otherSet != nil && !_ONSIsNSSet(otherSet)) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                "*** %s: set argument is not an NSSet",
                "-[NSMutableSet intersectSet:]"
            );
        }
    }
    if (self == otherSet) {
        return;
    }
    NSUInteger total = 0;
    for (id object in self) {
        total += [self countForObject:object];
    }
    temporaryIDBuffer(objects, total);
    id *end = objects;
    for (id object in self) {
        NSUInteger otherCount = [otherSet countForObject:object];
        NSUInteger selfCount = [self countForObject:object];
        if (selfCount > otherCount) {
            for (NSUInteger extra = selfCount - otherCount; extra != 0; extra--) {
                *end++ = object;
            }
        }
    }
    while (objects < end) {
        [self removeObject:*objects++];
    }
    free(objectsHeap);
}

// Native -[NSMutableSet minusOrderedSet:], 0x1c0153b64. Non-external. A non-NSOrderedSet raises
// NSInvalidArgumentException; each object of the ordered set is removed -countForObject: times.
- (void)minusOrderedSet:(id)orderedSet {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSMutableSet minusOrderedSet:]"
        );
    }
    for (id object in orderedSet) {
        for (NSUInteger times = [orderedSet countForObject:object]; times != 0; times--) {
            [self removeObject:object];
        }
    }
}

// Native -[NSMutableSet minusSet:], 0x1c0153d78. Non-external. The argument is forgiven; a remaining
// non-NSSet raises NSInvalidArgumentException. The receiver itself empties it (-removeAllObjects); otherwise
// each object of the set is removed -countForObject: times.
- (void)minusSet:(id)otherSet {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (otherSet != nil) {
        otherSet = __checkForAndForgiveClientSin(self, _cmd, otherSet);
        if (otherSet != nil && !_ONSIsNSSet(otherSet)) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                "*** %s: set argument is not an NSSet",
                "-[NSMutableSet minusSet:]"
            );
        }
    }
    if (self == otherSet) {
        [self removeAllObjects];
        return;
    }
    for (id object in otherSet) {
        for (NSUInteger times = [otherSet countForObject:object]; times != 0; times--) {
            [self removeObject:object];
        }
    }
}

// Native -[NSMutableSet removeAllObjects], 0x1c01540d0. Non-external. The objects are copied with
// -getObjects:count: into a temporary id buffer of -count entries; each is removed -countForObject: times.
- (void)removeAllObjects {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    NSUInteger count = [self count];
    temporaryIDBuffer(objects, count);
    [self getObjects:objects count:count];
    for (NSUInteger index = 0; index != count; index++) {
        for (NSUInteger times = [self countForObject:objects[index]]; times != 0; times--) {
            [self removeObject:objects[index]];
        }
    }
    free(objectsHeap);
}

// Native -[NSMutableSet removeObjectsInArray:range:], 0x1c0154294. Non-external. The checks of
// -addObjectsFromArray:range:; then each object of the range (-objectAtIndex:, ascending) is removed.
- (void)removeObjectsInArray:(id)array range:(OCFRange)range {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: array argument is not an NSArray",
            "-[NSMutableSet removeObjectsInArray:range:]"
        );
    }
    NSUInteger count = [array count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        if (count != 0) {
            raiseWithLogPack(
                ONSRangeException,
                "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
                "-[NSMutableSet removeObjectsInArray:range:]",
                (unsigned long)range.location,
                (unsigned long)range.length,
                (unsigned long)(count - 1)
            );
        }
        raiseWithLogPack(
            ONSRangeException,
            "*** %s: range {%lu, %lu} extends beyond bounds for empty set",
            "-[NSMutableSet removeObjectsInArray:range:]",
            (unsigned long)range.location,
            (unsigned long)range.length
        );
    }
    NSUInteger index = (NSUInteger)range.location;
    for (NSUInteger remaining = (NSUInteger)range.length; remaining != 0; remaining--) {
        [self removeObject:[array objectAtIndex:index++]];
    }
}

// Native -[NSMutableSet removeObjectsInArray:], 0x1c01545a0. Non-external. A non-NSArray raises
// NSInvalidArgumentException; each object is removed.
- (void)removeObjectsInArray:(id)array {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: array argument is not an NSArray",
            "-[NSMutableSet removeObjectsInArray:]"
        );
    }
    for (id object in array) {
        [self removeObject:object];
    }
}

// Native -[NSMutableSet removeObjectsInOrderedSet:range:], 0x1c015478c. Non-external. The pattern of
// -removeObjectsInArray:range: with _NSIsNSOrderedSet and the ordered-set message.
- (void)removeObjectsInOrderedSet:(id)orderedSet range:(OCFRange)range {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSMutableSet removeObjectsInOrderedSet:range:]"
        );
    }
    NSUInteger count = [orderedSet count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        if (count != 0) {
            raiseWithLogPack(
                ONSRangeException,
                "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
                "-[NSMutableSet removeObjectsInOrderedSet:range:]",
                (unsigned long)range.location,
                (unsigned long)range.length,
                (unsigned long)(count - 1)
            );
        }
        raiseWithLogPack(
            ONSRangeException,
            "*** %s: range {%lu, %lu} extends beyond bounds for empty set",
            "-[NSMutableSet removeObjectsInOrderedSet:range:]",
            (unsigned long)range.location,
            (unsigned long)range.length
        );
    }
    NSUInteger index = (NSUInteger)range.location;
    for (NSUInteger remaining = (NSUInteger)range.length; remaining != 0; remaining--) {
        [self removeObject:[orderedSet objectAtIndex:index++]];
    }
}

// Native -[NSMutableSet removeObjectsInOrderedSet:], 0x1c0154a98. Non-external. A non-NSOrderedSet raises
// NSInvalidArgumentException; each object is removed.
- (void)removeObjectsInOrderedSet:(id)orderedSet {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSMutableSet removeObjectsInOrderedSet:]"
        );
    }
    for (id object in orderedSet) {
        [self removeObject:object];
    }
}

// Native -[NSMutableSet removeObjectsInSet:], 0x1c0154c84. Non-external. The argument is forgiven; a
// remaining non-NSSet raises NSInvalidArgumentException. The receiver itself empties it; otherwise each
// object of the set is removed once.
- (void)removeObjectsInSet:(id)otherSet {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (otherSet != nil) {
        otherSet = __checkForAndForgiveClientSin(self, _cmd, otherSet);
        if (otherSet != nil && !_ONSIsNSSet(otherSet)) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                "*** %s: set argument is not an NSSet",
                "-[NSMutableSet removeObjectsInSet:]"
            );
        }
    }
    if (self == otherSet) {
        [self removeAllObjects];
        return;
    }
    for (id object in otherSet) {
        [self removeObject:object];
    }
}

// Native -[NSMutableSet removeObjectsWithOptions:passingTest:], 0x1c0154fb4, with
// ___53-[NSMutableSet removeObjectsWithOptions:passingTest:]_block_invoke (0x1c0155218). Non-external. A nil
// predicate raises NSInvalidArgumentException. The objects passing the predicate (called with the object and
// the enumeration's stop flag) are collected into +[NSMutableSet set] under an error-checking mutex (never
// destroyed) by __NSSetEnumerate with the options less NSEnumerationReverse (bit 1); then each is removed.
- (void)removeObjectsWithOptions:(NSUInteger)options passingTest:(BOOL (^)(id object, BOOL *stop))predicate {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (predicate == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSMutableSet removeObjectsWithOptions:passingTest:]"
        );
    }
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
    for (id object in passing) {
        [self removeObject:object];
    }
}

// Native -[NSMutableSet removeObjectsPassingTest:], 0x1c015526c. Non-external. A nil predicate raises
// NSInvalidArgumentException; otherwise -removeObjectsWithOptions:passingTest: with options 0 (no _mutate or
// ThreadSanitizer check here).
- (void)removeObjectsPassingTest:(BOOL (^)(id object, BOOL *stop))predicate {
    if (predicate == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: predicate cannot be nil",
            "-[NSMutableSet removeObjectsPassingTest:]"
        );
    }
    [self removeObjectsWithOptions:0 passingTest:predicate];
}

// Native -[NSMutableSet replaceObject:], 0x1c0155388. Non-external. A nil object raises
// NSInvalidArgumentException. A member that is present and not identical is removed and the object added.
- (void)replaceObject:(id)object {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (object == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, "*** %s: object cannot be nil", "-[NSMutableSet replaceObject:]"
        );
    }
    id member = [self member:object];
    if (member != object && member != nil) {
        [self removeObject:member];
        [self addObject:object];
    }
}

// Native -[NSMutableSet setObject:], 0x1c0155530. Non-external. A nil object raises
// NSInvalidArgumentException. Unless the member is the object itself, a present member is removed and the
// object added.
- (void)setObject:(id)object {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (object == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, "*** %s: object cannot be nil", "-[NSMutableSet setObject:]"
        );
    }
    id member = [self member:object];
    if (member != object) {
        if (member != nil) {
            [self removeObject:member];
        }
        [self addObject:object];
    }
}

// Native -[NSMutableSet setArray:], 0x1c01556d8. Non-external. A non-NSArray raises
// NSInvalidArgumentException; the receiver is emptied and each object added.
- (void)setArray:(id)array {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: array argument is not an NSArray",
            "-[NSMutableSet setArray:]"
        );
    }
    [self removeAllObjects];
    for (id object in array) {
        [self addObject:object];
    }
}

// Native -[NSMutableSet setOrderedSet:], 0x1c01558cc. Non-external. A non-NSOrderedSet raises
// NSInvalidArgumentException; the receiver is emptied and each object added -countForObject: times.
- (void)setOrderedSet:(id)orderedSet {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSMutableSet setOrderedSet:]"
        );
    }
    [self removeAllObjects];
    for (id object in orderedSet) {
        for (NSUInteger times = [orderedSet countForObject:object]; times != 0; times--) {
            [self addObject:object];
        }
    }
}

// Native -[NSMutableSet setSet:], 0x1c0155ae8. Non-external. The argument is forgiven; a remaining non-NSSet
// raises NSInvalidArgumentException. Unless the set is the receiver, the receiver is emptied and each object
// of the set added -countForObject: times.
- (void)setSet:(id)otherSet {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (otherSet != nil) {
        otherSet = __checkForAndForgiveClientSin(self, _cmd, otherSet);
        if (otherSet != nil && !_ONSIsNSSet(otherSet)) {
            raiseWithLogPack(
                ONSInvalidArgumentException, "*** %s: set argument is not an NSSet", "-[NSMutableSet setSet:]"
            );
        }
    }
    if (self != otherSet) {
        [self removeAllObjects];
        for (id object in otherSet) {
            for (NSUInteger times = [otherSet countForObject:object]; times != 0; times--) {
                [self addObject:object];
            }
        }
    }
}

// Native -[NSMutableSet unionOrderedSet:], 0x1c0155e3c. Non-external. A non-NSOrderedSet raises
// NSInvalidArgumentException; each object is added -countForObject: times.
- (void)unionOrderedSet:(id)orderedSet {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: ordered set argument is not an NSOrderedSet",
            "-[NSMutableSet unionOrderedSet:]"
        );
    }
    for (id object in orderedSet) {
        for (NSUInteger times = [orderedSet countForObject:object]; times != 0; times--) {
            [self addObject:object];
        }
    }
}

// Native -[NSMutableSet unionSet:], 0x1c0156050. Non-external. The argument is forgiven; a remaining
// non-NSSet raises NSInvalidArgumentException. The set's objects are copied with -getObjects:count: into a
// temporary id buffer of its -count entries; each is added -countForObject: times (no test for the receiver
// itself).
- (void)unionSet:(id)otherSet {
    [self _mutate];
    setTSANWrite(self, __builtin_return_address(0));
    if (otherSet != nil) {
        otherSet = __checkForAndForgiveClientSin(self, _cmd, otherSet);
        if (otherSet != nil && !_ONSIsNSSet(otherSet)) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                "*** %s: set argument is not an NSSet",
                "-[NSMutableSet unionSet:]"
            );
        }
    }
    NSUInteger count = [otherSet count];
    temporaryIDBuffer(objects, count);
    [otherSet getObjects:objects count:count];
    for (NSUInteger index = 0; index != count; index++) {
        for (NSUInteger times = [otherSet countForObject:objects[index]]; times != 0; times--) {
            [self addObject:objects[index]];
        }
    }
    free(objectsHeap);
}

// Native -[NSMutableSet initWithObjects:count:], 0x1c0156420. Non-external. The checks of -addObjects:count:
// (NULL array, ridiculous count, then each object for nil); no objects answers -initWithCapacity: 0;
// otherwise the result of -initWithCapacity: count receives each object with -addObject:.
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count {
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[NSMutableSet initWithObjects:count:]",
            (unsigned long)count
        );
    }
    if (count >> 61 != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[NSMutableSet initWithObjects:count:]",
            (unsigned long)count
        );
    }
    if (count == 0) {
        return [self initWithCapacity:0];
    }
    for (NSUInteger index = 0; index != count; index++) {
        if (objects[index] == nil) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                "*** %s: attempt to insert nil object from objects[%lu]",
                "-[NSMutableSet initWithObjects:count:]",
                (unsigned long)index
            );
        }
    }
    self = [self initWithCapacity:count];
    for (NSUInteger index = 0; index != count; index++) {
        [self addObject:objects[index]];
    }
    return self;
}

// Native -[NSMutableSet setByAddingObjectsFromArray:], 0x1c0156710. Non-external. A non-NSArray raises
// NSInvalidArgumentException. The array's -count is read before the receiver's; a temporary id buffer of the
// sum receives the receiver's objects (-getObjects:count:, when it has any) and then the array's
// (-getObjects:range: {0, count}, when it has any); the answer is +[NSSet setWithObjects:count:]. No _mutate
// or ThreadSanitizer check.
- (id)setByAddingObjectsFromArray:(id)array {
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: array argument is not an NSArray",
            "-[NSMutableSet setByAddingObjectsFromArray:]"
        );
    }
    NSUInteger arrayCount = [array count];
    NSUInteger selfCount = [self count];
    NSUInteger total = arrayCount + selfCount;
    temporaryIDBuffer(objects, total);
    if (selfCount != 0) {
        [self getObjects:objects count:selfCount];
    }
    if (arrayCount != 0) {
        [array getObjects:objects + selfCount range:(OCFRange){0, (OCFIndex)arrayCount}];
    }
    id result = [ONSSet setWithObjects:objects count:total];
    free(objectsHeap);
    return result;
}

// Native -[NSMutableSet setByAddingObjectsFromSet:], 0x1c0156984. Non-external. The argument is forgiven; a
// remaining non-NSSet raises NSInvalidArgumentException. The pattern of -setByAddingObjectsFromArray: with
// the set's -getObjects:count:.
- (id)setByAddingObjectsFromSet:(id)otherSet {
    if (otherSet != nil) {
        otherSet = __checkForAndForgiveClientSin(self, _cmd, otherSet);
        if (otherSet != nil && !_ONSIsNSSet(otherSet)) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                "*** %s: set argument is not an NSSet",
                "-[NSMutableSet setByAddingObjectsFromSet:]"
            );
        }
    }
    NSUInteger otherCount = [otherSet count];
    NSUInteger selfCount = [self count];
    NSUInteger total = otherCount + selfCount;
    temporaryIDBuffer(objects, total);
    if (selfCount != 0) {
        [self getObjects:objects count:selfCount];
    }
    if (otherCount != 0) {
        [otherSet getObjects:objects + selfCount count:otherCount];
    }
    id result = [ONSSet setWithObjects:objects count:total];
    free(objectsHeap);
    return result;
}

// Native +[NSMutableSet setWithCapacity:], 0x1c0156d20. Non-external. objc_alloc, -initWithCapacity:,
// autoreleased.
+ (id)setWithCapacity:(NSUInteger)capacity {
    return [[[self alloc] initWithCapacity:capacity] autorelease];
}

// Native -[NSMutableSet addObject:], 0x1c0306074, with OUTLINED_FUNCTION_0 (0x1c0156d48:
// __CFLookUpClass("NSMutableSet")). Non-external. The abstract method: __CFRequireConcreteImplementation (a
// tail call).
- (void)addObject:(id)object {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableSet)), self, _cmd);
}

// Native -[NSMutableSet removeObject:], 0x1c0306098, with OUTLINED_FUNCTION_0. Non-external. The abstract
// method (a tail call).
- (void)removeObject:(id)object {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableSet)), self, _cmd);
}

// Native -[NSMutableSet initWithCapacity:], 0x1c03060bc, with OUTLINED_FUNCTION_0. Non-external. The abstract
// method; answers nil.
- (id)initWithCapacity:(NSUInteger)capacity {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableSet)), self, _cmd);
    return nil;
}

@end
