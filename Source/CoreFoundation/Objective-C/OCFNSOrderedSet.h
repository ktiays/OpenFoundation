//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

// Classes the NSOrderedSet methods reference by their class symbols, not restored on this branch: the mutable
// subclass, the enumerators and views -reverseObjectEnumerator, -reversedOrderedSet, -array and -set
// allocate, and the fast-enumeration enumerator -objectEnumerator allocates
// (Objective-C/OCFNSFastEnumerationEnumerator.m).
@class ONSMutableOrderedSet;
@class __ONSOrderedSetReverseEnumerator;
@class __ONSOrderedSetReversed;
@class __ONSOrderedSetArrayProxy;
@class __ONSOrderedSetSetProxy;

// Native class NSOrderedSet (__DATA,__objc_data 0x22e6ceb70, external), an NSObject subclass; class_ro_t
// 0x2227c65c8: flags 0, instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The superclass pointer 0x22e6b6088 lies
// outside the extracted image; NSObject is the SDK's hierarchy (confidence: medium). The relative method
// lists
// __OBJC_$_INSTANCE_METHODS_NSOrderedSet (0x1c031cea0, 68 methods) and __OBJC_$_CLASS_METHODS_NSOrderedSet
// (0x1c03195a0, 17 methods) lie in the image and name exactly the methods restored in
// Objective-C/OCFNSOrderedSet.m; the image has no category list on NSOrderedSet (any categories were merged
// into these lists at build time and cannot be told apart), so every method is declared and defined on the
// class. The protocol list (0x2227c6520: four protocols whose records lie outside the image) and the property
// lists (0x2227c65b0, 0x2227c6550) are unread and not declared. NSRange parameters are spelled OCFRange (two
// 64-bit words), and NSEnumerationOptions, NSBinarySearchingOptions and NSSortOptions NSUInteger, the passing
// the native code uses.
__attribute__((visibility("default")))
@interface ONSOrderedSet : NSObject

+ (id)allocWithZone:(struct _NSZone *)zone;
+ (id)newOrderedSetWithObjects:(const id *)objects count:(NSUInteger)count;
+ (id)orderedSetWithObjects:(const id *)objects count:(NSUInteger)count;
+ (id)orderedSetWithObject:(id)object;
+ (id)orderedSet;
+ (id)orderedSetWithObjects:(id)firstObject, ...;
+ (id)orderedSetWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)flag;
+ (id)orderedSetWithArray:(id)array range:(OCFRange)range;
+ (id)orderedSetWithArray:(id)array copyItems:(BOOL)flag;
+ (id)orderedSetWithArray:(id)array;
+ (id)orderedSetWithOrderedSet:(id)orderedSet range:(OCFRange)range copyItems:(BOOL)flag;
+ (id)orderedSetWithOrderedSet:(id)orderedSet range:(OCFRange)range;
+ (id)orderedSetWithOrderedSet:(id)orderedSet copyItems:(BOOL)flag;
+ (id)orderedSetWithOrderedSet:(id)orderedSet;
+ (id)orderedSetWithSet:(id)set copyItems:(BOOL)flag;
+ (id)orderedSetWithSet:(id)set;
+ (BOOL)supportsSecureCoding;

- (BOOL)isNSOrderedSet__;
- (NSUInteger)count;
- (NSUInteger)indexOfObject:(id)object;
- (id)objectAtIndex:(NSUInteger)index;
- (id)allObjects;
- (BOOL)containsObject:(id)object inRange:(OCFRange)range;
- (BOOL)containsObject:(id)object;
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length;
- (NSUInteger)countForObject:(id)object inRange:(OCFRange)range;
- (NSUInteger)countForObject:(id)object;
- (void)enumerateObjectsAtIndexes:(id)indexes
                          options:(NSUInteger)options
                       usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block;
- (void)enumerateObjectsWithOptions:(NSUInteger)options
                         usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block;
- (void)enumerateObjectsUsingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block;
- (id)firstObject;
- (void)getObjects:(id *)objects range:(OCFRange)range;
- (void)getObjects:(id *)objects;
- (NSUInteger)indexOfObject:(id)object inRange:(OCFRange)range;
- (NSUInteger)indexOfObject:(id)object
              inSortedRange:(OCFRange)range
                    options:(NSUInteger)options
            usingComparator:(NSInteger (^)(id object1, id object2))comparator;
- (NSUInteger)indexOfObjectAtIndexes:(id)indexes
                             options:(NSUInteger)options
                         passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (NSUInteger)indexOfObjectWithOptions:(NSUInteger)options
                           passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (NSUInteger)indexOfObjectPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)indexesOfObjectsAtIndexes:(id)indexes
                        options:(NSUInteger)options
                    passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)indexesOfObjectsWithOptions:(NSUInteger)options
                      passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)indexesOfObjectsPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (BOOL)intersectsOrderedSet:(id)other;
- (BOOL)intersectsSet:(id)set;
- (BOOL)isSubsetOfOrderedSet:(id)other;
- (BOOL)isSubsetOfSet:(id)set;
- (id)lastObject;
- (id)objectAtIndexedSubscript:(NSUInteger)index;
- (id)objectAtIndexes:(id)indexes
              options:(NSUInteger)options
          passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)objectWithOptions:(NSUInteger)options
            passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)objectPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)objectsAtIndexes:(id)indexes;
- (id)objectsAtIndexes:(id)indexes
               options:(NSUInteger)options
           passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)objectsWithOptions:(NSUInteger)options
             passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)objectsPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)objectEnumerator;
- (id)reverseObjectEnumerator;
- (id)reversedOrderedSet;
- (id)sortedArrayFromRange:(OCFRange)range
                   options:(NSUInteger)options
           usingComparator:(NSInteger (^)(id object1, id object2))comparator;
- (id)sortedArrayWithOptions:(NSUInteger)options
             usingComparator:(NSInteger (^)(id object1, id object2))comparator;
- (id)sortedArrayUsingComparator:(NSInteger (^)(id object1, id object2))comparator;
- (id)array;
- (id)set;
- (id)descriptionWithLocale:(id)locale indent:(NSUInteger)level;
- (id)descriptionWithLocale:(id)locale;
- (id)description;
- (NSUInteger)hash;
- (BOOL)isEqualToOrderedSet:(id)other;
- (BOOL)isEqual:(id)object;
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count;
- (id)initWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)flag;
- (id)initWithArray:(id)array range:(OCFRange)range;
- (id)initWithArray:(id)array copyItems:(BOOL)flag;
- (id)initWithArray:(id)array;
- (id)initWithObject:(id)object;
- (id)initWithObjects:(id)firstObject, ...;
- (id)initWithOrderedSet:(id)orderedSet range:(OCFRange)range copyItems:(BOOL)flag;
- (id)initWithOrderedSet:(id)orderedSet range:(OCFRange)range;
- (id)initWithOrderedSet:(id)orderedSet copyItems:(BOOL)flag;
- (id)initWithOrderedSet:(id)orderedSet;
- (id)initWithSet:(id)set copyItems:(BOOL)flag;
- (id)initWithSet:(id)set;
- (id)copyWithZone:(struct _NSZone *)zone;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
- (id)initWithCoder:(id)coder;
- (void)encodeWithCoder:(id)coder;

@end
#endif
