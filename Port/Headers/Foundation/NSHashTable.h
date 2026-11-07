//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSPointerFunctions.h>
#import <Foundation/NSString.h>
#import <Foundation/NSEnumerator.h>

#if defined(__OBJC__)
@class NSArray<ObjectType>, NSSet<ObjectType>;

static const NSPointerFunctionsOptions NSHashTableStrongMemory = NSPointerFunctionsStrongMemory;
static const NSPointerFunctionsOptions NSHashTableCopyIn = NSPointerFunctionsCopyIn;
static const NSPointerFunctionsOptions NSHashTableObjectPointerPersonality = NSPointerFunctionsObjectPointerPersonality;
static const NSPointerFunctionsOptions NSHashTableWeakMemory = NSPointerFunctionsWeakMemory;

typedef NSUInteger NSHashTableOptions;

@interface NSHashTable<ObjectType> : NSObject <NSCopying, NSSecureCoding, NSFastEnumeration>

- (instancetype)initWithOptions:(NSPointerFunctionsOptions)options capacity:(NSUInteger)initialCapacity NS_DESIGNATED_INITIALIZER;
+ (NSHashTable<ObjectType> *)hashTableWithOptions:(NSPointerFunctionsOptions)options;
+ (NSHashTable<ObjectType> *)weakObjectsHashTable;

@property (readonly, copy) NSPointerFunctions *pointerFunctions;
@property (readonly) NSUInteger count;
- (void)addObject:(ObjectType)object;
- (void)removeObject:(ObjectType)object;
- (void)removeAllObjects;
@property (readonly, copy) NSArray<ObjectType> *allObjects;
- (BOOL)containsObject:(ObjectType)anObject;
- (void)unionHashTable:(NSHashTable<ObjectType> *)other;

@end
#endif
