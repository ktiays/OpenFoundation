//
//  Created by ktiays on 2026/11/04.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Collections/ONSPointerFunctions.h"

#include <os/lock.h>

// Shared declarations of the NSMapTable unit (Collections/ONSMapTable.m), iOS Simulator 24A434, arm64,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device 24A446: unread.

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native NSMapTable, 0x22e6db250 (external); class_ro_t 0x2228583a8: flags 0, instance start and size 8,
// no ivars; the superclass is libobjc's NSObject. Only the methods the captured groups reach are declared.
__attribute__((visibility("default")))
@interface ONSMapTable : NSObject

+ (id)allocWithZone:(struct _NSZone *)zone;
- (id)initWithCoder:(id)coder;
- (Class)classForCoder;
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded;
- (id)initWithKeyOptions:(NSUInteger)keyOptions
            valueOptions:(NSUInteger)valueOptions
                capacity:(NSUInteger)capacity;
- (id)init;
- (id)description;
- (void)encodeWithCoder:(id)coder;
- (NSUInteger)count;
- (id)keyEnumerator;
- (id)copy;
- (NSUInteger)countByEnumeratingWithState:(PointerCollectionEnumerationState *)state
                                  objects:(id *)buffer
                                    count:(NSUInteger)length;
- (id)objectForKey:(id)key;
- (void)setObject:(id)object forKey:(id)key;
- (void)removeObjectForKey:(id)key;
- (id)allKeys;
- (id)allValues;
- (void)removeAllItems;
- (void)removeAllObjects;
- (id)copyWithZone:(struct _NSZone *)zone;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
@end

// Native NSConcreteMapTable, 0x22e6db228 (non-external); class_ro_t 0x222858580: flags 0x10, instance start
// 8, size 118. Ivars: keys at 8, values at 40, count at 72, capacity at 80, keyOptions at 88, valueOptions at
// 96, mutations at 104, growLock at 112 (os_unfair_lock), shouldRehash at 116 (B) and hasDynamicSlices at 117
// (B); offset variables 0x22e6d4960-0x22e6d4984.
__attribute__((visibility("hidden")))
@interface ONSConcreteMapTable : ONSMapTable {
    @public
    ONSSlice keys;
    ONSSlice values;
    NSUInteger count;
    NSUInteger capacity;
    NSUInteger keyOptions;
    NSUInteger valueOptions;
    NSUInteger mutations;
    os_unfair_lock growLock;
    BOOL shouldRehash;
    BOOL hasDynamicSlices;
}

- (void)raiseCountUnderflowException;
- (NSUInteger)realCount;
- (void)_setBackingStore;
- (void)grow;
- (void)assign:(NSUInteger)index key:(const void *)key value:(const void *)value isNew:(BOOL)isNew;
- (NSUInteger)rehashAround:(NSUInteger)index;
- (void)rehash;
- (void)dealloc;
- (NSUInteger)hash;
- (BOOL)isEqual:(id)object;
@end

#endif
