//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSEnumerator.h>

#if defined(__OBJC__)
@class NSArray<ObjectType>, NSSet<ObjectType>, NSString, NSURL, NSError;

@interface NSDictionary<__covariant KeyType, __covariant ObjectType> : NSObject <NSCopying, NSMutableCopying, NSSecureCoding, NSFastEnumeration>

@property (readonly) NSUInteger count;
- (ObjectType)objectForKey:(KeyType)aKey;
- (NSEnumerator<KeyType> *)keyEnumerator;
- (instancetype)init NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithObjects:(const ObjectType[])objects
                        forKeys:(const KeyType<NSCopying>[])keys
                          count:(NSUInteger)cnt NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithCoder:(NSCoder *)coder NS_DESIGNATED_INITIALIZER;

@end

@interface NSDictionary<KeyType, ObjectType> (NSExtendedDictionary)

@property (readonly, copy) NSArray<KeyType> *allKeys;
- (NSArray<KeyType> *)allKeysForObject:(ObjectType)anObject;
@property (readonly, copy) NSArray<ObjectType> *allValues;
@property (readonly, copy) NSString *description;
- (NSString *)descriptionWithLocale:(id)locale;
- (BOOL)isEqualToDictionary:(NSDictionary<KeyType, ObjectType> *)otherDictionary;
- (NSEnumerator<ObjectType> *)objectEnumerator;
- (NSArray<ObjectType> *)objectsForKeys:(NSArray<KeyType> *)keys notFoundMarker:(ObjectType)marker;
- (NSArray<KeyType> *)keysSortedByValueUsingSelector:(SEL)comparator;
- (void)getObjects:(ObjectType __unsafe_unretained[])objects andKeys:(KeyType __unsafe_unretained[])keys count:(NSUInteger)count;
- (ObjectType)objectForKeyedSubscript:(KeyType)key;
- (void)enumerateKeysAndObjectsUsingBlock:(void (^)(KeyType key, ObjectType obj, BOOL *stop))block;
- (void)enumerateKeysAndObjectsWithOptions:(NSEnumerationOptions)opts
                                usingBlock:(void (^)(KeyType key, ObjectType obj, BOOL *stop))block;
- (NSArray<KeyType> *)keysSortedByValueUsingComparator:(NSComparator)cmptr;
- (NSSet<KeyType> *)keysOfEntriesPassingTest:(BOOL (^)(KeyType key, ObjectType obj, BOOL *stop))predicate;

@end

@interface NSDictionary<KeyType, ObjectType> (NSDictionaryCreation)

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithObject:(ObjectType)object forKey:(KeyType<NSCopying>)key;
+ (instancetype)dictionaryWithObjects:(const ObjectType[])objects
                              forKeys:(const KeyType<NSCopying>[])keys
                                count:(NSUInteger)cnt;
+ (instancetype)dictionaryWithObjectsAndKeys:(id)firstObject, ... NS_REQUIRES_NIL_TERMINATION;
+ (instancetype)dictionaryWithDictionary:(NSDictionary<KeyType, ObjectType> *)dict;
+ (instancetype)dictionaryWithObjects:(NSArray<ObjectType> *)objects forKeys:(NSArray<KeyType<NSCopying>> *)keys;
- (instancetype)initWithObjectsAndKeys:(id)firstObject, ... NS_REQUIRES_NIL_TERMINATION;
- (instancetype)initWithDictionary:(NSDictionary<KeyType, ObjectType> *)otherDictionary;
- (instancetype)initWithDictionary:(NSDictionary<KeyType, ObjectType> *)otherDictionary copyItems:(BOOL)flag;
- (instancetype)initWithObjects:(NSArray<ObjectType> *)objects forKeys:(NSArray<KeyType<NSCopying>> *)keys;

@end

@interface NSMutableDictionary<KeyType, ObjectType> : NSDictionary<KeyType, ObjectType>

- (void)removeObjectForKey:(KeyType)aKey;
- (void)setObject:(ObjectType)anObject forKey:(KeyType<NSCopying>)aKey;
- (instancetype)init NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithCapacity:(NSUInteger)numItems NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithCoder:(NSCoder *)coder NS_DESIGNATED_INITIALIZER;

@end

@interface NSMutableDictionary<KeyType, ObjectType> (NSExtendedMutableDictionary)

- (void)addEntriesFromDictionary:(NSDictionary<KeyType, ObjectType> *)otherDictionary;
- (void)removeAllObjects;
- (void)removeObjectsForKeys:(NSArray<KeyType> *)keyArray;
- (void)setDictionary:(NSDictionary<KeyType, ObjectType> *)otherDictionary;
- (void)setObject:(ObjectType)obj forKeyedSubscript:(KeyType<NSCopying>)key;

@end

@interface NSMutableDictionary<KeyType, ObjectType> (NSMutableDictionaryCreation)

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

@end
#endif
