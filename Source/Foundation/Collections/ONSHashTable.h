//
//  Created by ktiays on 2026/11/04.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Collections/ONSPointerFunctions.h"

// Shared declarations of the NSHashTable unit (Collections/ONSHashTable.m), iOS Simulator 24A434, arm64,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device 24A446: unread.

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native NSHashTable, 0x22e6d85a0 (external); class_ro_t 0x22284fb28: flags 0, instance start and size 8,
// no ivars; the superclass is libobjc's NSObject. Only the methods the captured groups reach are declared.
__attribute__((visibility("default")))
@interface ONSHashTable : NSObject

+ (id)allocWithZone:(struct _NSZone *)zone;
+ (id)hashTableWithOptions:(NSUInteger)options;
- (id)initWithCoder:(id)coder;
- (Class)classForCoder;
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded;
- (id)initWithOptions:(NSUInteger)options capacity:(NSUInteger)capacity;
- (id)init;
- (id)description;
- (void)encodeWithCoder:(id)coder;
- (NSUInteger)countByEnumeratingWithState:(PointerCollectionEnumerationState *)state
                                  objects:(id *)buffer
                                    count:(NSUInteger)length;
- (NSUInteger)count;
- (id)allObjects;
- (void *)getItem:(const void *)item;
- (void)addObject:(id)object;
- (void)removeItem:(const void *)item;
- (id)copy;
- (void)removeAllItems;
- (id)pointerFunctions;
- (void)removeObject:(id)object;
- (void)removeAllObjects;
- (BOOL)containsObject:(id)object;
- (void)unionHashTable:(id)table;
- (id)copyWithZone:(struct _NSZone *)zone;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
@end

// Native NSConcreteHashTable, 0x22e6d8578 (non-external); class_ro_t 0x22284fcb0: flags 0x10, instance start
// 8, size 72. Ivars: slice at 8, count at 40, capacity at 48, mutations at 56, and one eight-byte unit at 64
// holding options (b63) and hasDynamicSlice (b1, the top bit); offset variables 0x22e6d468c-0x22e6d46a0.
__attribute__((visibility("hidden")))
@interface ONSConcreteHashTable : ONSHashTable {
    @public
    ONSSlice slice;
    NSUInteger count;
    NSUInteger capacity;
    NSUInteger mutations;
    NSUInteger options : 63;
    NSUInteger hasDynamicSlice : 1;
}

- (NSUInteger)hash;
- (BOOL)isEqual:(id)object;
- (void)raiseCountUnderflowException;
- (NSUInteger)rehashAround:(NSUInteger)index;
- (void)hashGrow;
- (void)assign:(NSUInteger)index key:(const void *)key;
- (void)rehash;
- (void)insertItem:(const void *)item;
- (void)dealloc;
@end

#endif
