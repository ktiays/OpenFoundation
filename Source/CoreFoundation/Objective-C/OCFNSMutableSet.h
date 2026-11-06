//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSSet.h"

#ifdef __OBJC__
// Native class NSMutableSet (__DATA,__objc_data 0x22e6cc5f0, external), an NSSet subclass; class_ro_t
// 0x2227bf110: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its method and protocol lists (0x23d88e551,
// 0x23d88e631) lie outside the extracted image: the categories are unread, so every method is declared and
// defined on the class.
__attribute__((visibility("default")))
@interface ONSMutableSet : ONSSet

+ (id)setWithCapacity:(NSUInteger)capacity;

- (void)_mutate;
- (void)addObject:(id)object;
- (void)removeObject:(id)object;
- (id)initWithCapacity:(NSUInteger)capacity;
- (void)addObjects:(const id *)objects count:(NSUInteger)count;
- (void)addObjectsFromArray:(id)array range:(OCFRange)range;
- (void)addObjectsFromArray:(id)array;
- (void)addObjectsFromOrderedSet:(id)orderedSet range:(OCFRange)range;
- (void)addObjectsFromOrderedSet:(id)orderedSet;
- (void)addObjectsFromSet:(id)otherSet;
- (void)intersectOrderedSet:(id)orderedSet;
- (void)intersectSet:(id)otherSet;
- (void)minusOrderedSet:(id)orderedSet;
- (void)minusSet:(id)otherSet;
- (void)removeAllObjects;
- (void)removeObjectsInArray:(id)array range:(OCFRange)range;
- (void)removeObjectsInArray:(id)array;
- (void)removeObjectsInOrderedSet:(id)orderedSet range:(OCFRange)range;
- (void)removeObjectsInOrderedSet:(id)orderedSet;
- (void)removeObjectsInSet:(id)otherSet;
- (void)removeObjectsWithOptions:(NSUInteger)options passingTest:(BOOL (^)(id object, BOOL *stop))predicate;
- (void)removeObjectsPassingTest:(BOOL (^)(id object, BOOL *stop))predicate;
- (void)replaceObject:(id)object;
- (void)setObject:(id)object;
- (void)setArray:(id)array;
- (void)setOrderedSet:(id)orderedSet;
- (void)setSet:(id)otherSet;
- (void)unionOrderedSet:(id)orderedSet;
- (void)unionSet:(id)otherSet;

@end

// Native __checkForAndForgiveClientSin, inlined into the set-taking methods of each unit, with its counter
// __checkForAndForgiveClientSin.cnt (0x22e6d38d0, __DATA,__bss) for the NSMutableSet unit and a second copy
// (listed as .cnt_0) for the NSSet unit: one static per translation unit. An argument that is an NSSet, or
// any argument in an app linked on or after CFSystemVersion 7 or 0x3F2, is answered unchanged; otherwise the
// first 100 calls log at level 3 (the 100th adds the last-log sentence) and the argument's objects are copied
// into a new NSMutableSet (+new, -addObject:), which is answered autoreleased. The callers pass a non-nil
// argument and check the answer with _NSIsNSSet when it is not nil.
static inline id __checkForAndForgiveClientSin(id self, SEL _cmd, id set) {
    static unsigned long cnt;
    if (_ONSIsNSSet(set) || _OCFExecutableLinkedOnOrAfter(7) || _OCFExecutableLinkedOnOrAfter(0x3F2)) {
        return set;
    }
    if (++cnt <= 100) {
        OCFStringRef proem = __CFExceptionProem(self, _cmd);
        OCFLog(
            3,
            OF_STR(
                "%@: This app appears to be calling this method with a non-set parameter.  Please wait while "
                "the system corrects this....  %s"
            ),
            proem,
            cnt == 100 ? "This is the last log that will appear on this subject, though there may be more "
                         "instances of this bug during further execution."
                       : ""
        );
    }
    id copy = [ONSMutableSet new];
    for (id object in set) {
        [copy addObject:object];
    }
    return [copy autorelease];
}
#endif
