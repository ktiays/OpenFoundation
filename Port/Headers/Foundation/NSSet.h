//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSEnumerator.h>

#if defined(__OBJC__)
@class NSArray<ObjectType>, NSDictionary<KeyType, ObjectType>, NSString;

@interface NSSet<__covariant ObjectType> : NSObject <NSCopying, NSMutableCopying, NSSecureCoding, NSFastEnumeration>

@property (readonly) NSUInteger count;
- (ObjectType)member:(ObjectType)object;
- (NSEnumerator<ObjectType> *)objectEnumerator;
- (instancetype)init NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithObjects:(const ObjectType[])objects count:(NSUInteger)cnt NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithCoder:(NSCoder *)coder NS_DESIGNATED_INITIALIZER;

@end

@interface NSSet<ObjectType> (NSExtendedSet)

@property (readonly, copy) NSArray<ObjectType> *allObjects;
- (ObjectType)anyObject;
- (BOOL)containsObject:(ObjectType)anObject;
@property (readonly, copy) NSString *description;
- (BOOL)intersectsSet:(NSSet<ObjectType> *)otherSet;
- (BOOL)isEqualToSet:(NSSet<ObjectType> *)otherSet;
- (BOOL)isSubsetOfSet:(NSSet<ObjectType> *)otherSet;
- (void)makeObjectsPerformSelector:(SEL)aSelector;
- (NSSet<ObjectType> *)setByAddingObject:(ObjectType)anObject;
- (NSSet<ObjectType> *)setByAddingObjectsFromSet:(NSSet<ObjectType> *)other;
- (NSSet<ObjectType> *)setByAddingObjectsFromArray:(NSArray<ObjectType> *)other;
- (void)enumerateObjectsUsingBlock:(void (^)(ObjectType obj, BOOL *stop))block;
- (NSSet<ObjectType> *)objectsPassingTest:(BOOL (^)(ObjectType obj, BOOL *stop))predicate;

@end

@interface NSSet<ObjectType> (NSSetCreation)

+ (instancetype)set;
+ (instancetype)setWithObject:(ObjectType)object;
+ (instancetype)setWithObjects:(const ObjectType[])objects count:(NSUInteger)cnt;
+ (instancetype)setWithObjects:(ObjectType)firstObj, ... NS_REQUIRES_NIL_TERMINATION;
+ (instancetype)setWithSet:(NSSet<ObjectType> *)set;
+ (instancetype)setWithArray:(NSArray<ObjectType> *)array;
- (instancetype)initWithObjects:(ObjectType)firstObj, ... NS_REQUIRES_NIL_TERMINATION;
- (instancetype)initWithSet:(NSSet<ObjectType> *)set;
- (instancetype)initWithSet:(NSSet<ObjectType> *)set copyItems:(BOOL)flag;
- (instancetype)initWithArray:(NSArray<ObjectType> *)array;

@end

@interface NSMutableSet<ObjectType> : NSSet<ObjectType>

- (void)addObject:(ObjectType)object;
- (void)removeObject:(ObjectType)object;
- (instancetype)initWithCoder:(NSCoder *)coder NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithCapacity:(NSUInteger)numItems NS_DESIGNATED_INITIALIZER;

@end

@interface NSMutableSet<ObjectType> (NSExtendedMutableSet)

- (void)addObjectsFromArray:(NSArray<ObjectType> *)array;
- (void)intersectSet:(NSSet<ObjectType> *)otherSet;
- (void)minusSet:(NSSet<ObjectType> *)otherSet;
- (void)removeAllObjects;
- (void)unionSet:(NSSet<ObjectType> *)otherSet;
- (void)setSet:(NSSet<ObjectType> *)otherSet;

@end

@interface NSMutableSet<ObjectType> (NSMutableSetCreation)

+ (instancetype)setWithCapacity:(NSUInteger)numItems;

@end

@interface NSCountedSet<ObjectType> : NSMutableSet<ObjectType>

- (instancetype)initWithCapacity:(NSUInteger)numItems NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithArray:(NSArray<ObjectType> *)array;
- (instancetype)initWithSet:(NSSet<ObjectType> *)set;
- (NSUInteger)countForObject:(ObjectType)object;
- (NSEnumerator<ObjectType> *)objectEnumerator;
- (void)addObject:(ObjectType)object;
- (void)removeObject:(ObjectType)object;

@end
#endif
