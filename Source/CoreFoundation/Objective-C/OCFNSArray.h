//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

// NSNotFound (Foundation's NSIntegerMax, 0x7FFFFFFFFFFFFFFF at every native use), the index the array methods
// answer for no match. kNotFound is a local name.
static const NSUInteger kNotFound = NSIntegerMax;

// Interfaces of families not yet restored on this branch

// Native NSRangeException (__DATA_CONST,__const 0x2227a8448, external), the exception name constant of the
// NSException unit, between NSGenericException and NSInvalidArgumentException; not restored on this branch.

// Native __CFExceptionProem, 0x1c029ec78 (was a private external; defined in Objective-C/OCFNSObject.m): the
// "-[Class selector]" prefix of an exception reason for the object and selector. Also declared by
// Objective-C/OCFBridgedClasses.h, which this header cannot include while that header holds the ONSArray
// stub.

#ifdef __OBJC__
#import <objc/NSObject.h>

// Classes the NSArray methods reference by their class symbols, not restored on this branch: the enumerators
// -objectEnumerator, -reverseObjectEnumerator and -reversedArray allocate, and the class clusters'
// placeholder.
@class __ONSArrayReverseEnumerator;
@class __ONSArrayReversed;
@class __ONSFastEnumerationEnumerator;
@class __ONSPlaceholderArray;
@class ONSMutableArray;
@class ONSSet;

// Native class NSArray (__DATA,__objc_data 0x22e6cdc70, external), an NSObject subclass; class_ro_t
// 0x2227c36d0: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its method lists, protocol list (0x23d88c949)
// and property list (0x23d88ca41) lie in the shared cache outside the extracted image; the methods are the
// 110 nm symbols "-[NSArray ...]" and "+[NSArray ...]" of the image, all restored in
// Objective-C/OCFNSArray.m. The image defines no category on NSArray. The protocol conformances are unread
// and not declared. NSRange parameters and results are spelled OCFRange (two 64-bit words), and
// NSEnumerationOptions, NSBinarySearchingOptions and NSSortOptions NSUInteger, the passing the native code
// uses.
__attribute__((visibility("default")))
@interface ONSArray : NSObject

+ (id)allocWithZone:(struct _NSZone *)zone;
+ (id)array;
+ (id)arrayWithObject:(id)object;
+ (id)arrayWithObjects:(const id *)objects count:(NSUInteger)count;
+ (id)arrayWithObjects:(id)firstObject, ...;
+ (id)newArrayWithObjects:(const id *)objects count:(NSUInteger)count;
+ (id)arrayWithArray:(id)array;
+ (id)arrayWithArray:(id)array copyItems:(BOOL)copyItems;
+ (id)arrayWithArray:(id)array range:(OCFRange)range;
+ (id)arrayWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)copyItems;
+ (id)arrayWithOrderedSet:(id)orderedSet;
+ (id)arrayWithOrderedSet:(id)orderedSet copyItems:(BOOL)copyItems;
+ (id)arrayWithOrderedSet:(id)orderedSet range:(OCFRange)range;
+ (id)arrayWithOrderedSet:(id)orderedSet range:(OCFRange)range copyItems:(BOOL)copyItems;
+ (id)arrayWithSet:(id)set;
+ (id)arrayWithSet:(id)set copyItems:(BOOL)copyItems;
+ (BOOL)supportsSecureCoding;

- (unsigned long)_cfTypeID;
- (BOOL)isNSArray__;
- (NSUInteger)count;
- (id)objectAtIndex:(NSUInteger)index;
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count;
- (id)allObjects;
- (id)componentsJoinedByString:(id)separator;
- (BOOL)containsObject:(id)object inRange:(OCFRange)range;
- (BOOL)containsObject:(id)object;
- (BOOL)containsObjectIdenticalTo:(id)object inRange:(OCFRange)range;
- (BOOL)containsObjectIdenticalTo:(id)object;
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length;
- (NSUInteger)countForObject:(id)object inRange:(OCFRange)range;
- (NSUInteger)countForObject:(id)object;
- (id)descriptionWithLocale:(id)locale indent:(NSUInteger)level;
- (id)descriptionWithLocale:(id)locale;
- (id)description;
- (void)enumerateObjectsAtIndexes:(id)indexes
                          options:(NSUInteger)options
                       usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block;
- (void)enumerateObjectsWithOptions:(NSUInteger)options
                         usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block;
- (void)enumerateObjectsUsingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block;
- (id)firstObject;
- (id)firstObjectCommonWithArray:(id)otherArray;
- (void)getObjects:(id *)objects range:(OCFRange)range;
- (void)getObjects:(id *)objects;
- (NSUInteger)hash;
- (NSUInteger)indexOfObject:(id)object inRange:(OCFRange)range;
- (NSUInteger)indexOfObject:(id)object;
- (NSUInteger)indexOfObjectIdenticalTo:(id)object inRange:(OCFRange)range;
- (NSUInteger)indexOfObjectIdenticalTo:(id)object;
- (id)indexesOfObject:(id)object inRange:(OCFRange)range;
- (id)indexesOfObject:(id)object;
- (id)indexesOfObjectIdenticalTo:(id)object inRange:(OCFRange)range;
- (id)indexesOfObjectIdenticalTo:(id)object;
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
- (BOOL)isEqualToArray:(id)otherArray;
- (BOOL)isEqual:(id)object;
- (id)lastObject;
- (void)makeObjectsPerformSelector:(SEL)selector;
- (void)makeObjectsPerformSelector:(SEL)selector withObject:(id)argument;
- (id)objectAtIndexedSubscript:(NSUInteger)index;
- (id)objectsAtIndexes:(id)indexes;
- (id)objectAtIndexes:(id)indexes
              options:(NSUInteger)options
          passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)objectWithOptions:(NSUInteger)options
            passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)objectPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)objectsAtIndexes:(id)indexes
               options:(NSUInteger)options
           passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)objectsWithOptions:(NSUInteger)options
             passingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)objectsPassingTest:(BOOL (^)(id object, NSUInteger index, BOOL *stop))predicate;
- (id)objectEnumerator;
- (id)reverseObjectEnumerator;
- (id)reversedArray;
- (id)sortedArrayFromRange:(OCFRange)range
                   options:(NSUInteger)options
           usingComparator:(NSInteger (^)(id object1, id object2))comparator;
- (id)sortedArrayWithOptions:(NSUInteger)options
             usingComparator:(NSInteger (^)(id object1, id object2))comparator;
- (id)sortedArrayUsingComparator:(NSInteger (^)(id object1, id object2))comparator;
- (id)sortedArrayUsingFunction:(NSInteger (*)(id object1, id object2, void *context))comparator
                       context:(void *)context;
- (id)sortedArrayUsingSelector:(SEL)comparator;
- (id)subarrayWithRange:(OCFRange)range;
- (id)initWithArray:(id)array;
- (id)initWithArray:(id)array copyItems:(BOOL)copyItems;
- (id)initWithArray:(id)array range:(OCFRange)range;
- (id)initWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)copyItems;
- (id)initWithObject:(id)object;
- (id)initWithObjects:(id)firstObject, ...;
- (id)initWithSet:(id)set copyItems:(BOOL)copyItems;
- (id)initWithSet:(id)set;
- (id)initWithOrderedSet:(id)orderedSet range:(OCFRange)range copyItems:(BOOL)copyItems;
- (id)initWithOrderedSet:(id)orderedSet range:(OCFRange)range;
- (id)initWithOrderedSet:(id)orderedSet copyItems:(BOOL)copyItems;
- (id)initWithOrderedSet:(id)orderedSet;
- (id)_initByAdoptingBuffer:(id *)buffer count:(NSUInteger)count size:(NSUInteger)size;
- (id)arrayByAddingObject:(id)object;
- (id)arrayByAddingObjectsFromArray:(id)otherArray;
- (id)arrayByApplyingSelector:(SEL)selector;
- (id)arrayByExcludingObjectsInArray:(id)otherArray;
- (id)arrayByExcludingToObjectsInArray:(id)otherArray;
- (id)copyWithZone:(struct _NSZone *)zone;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
- (id)initWithCoder:(id)coder;
- (void)encodeWithCoder:(id)coder;

@end
#endif
