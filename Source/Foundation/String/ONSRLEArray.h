//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenFoundation/Range/ONSRange.h>

#import <objc/NSObject.h>
#include <os/lock.h>
#include <stddef.h>
#include <stdint.h>

// Native _NSRunArrayItem and _NSRefCountedRunArray. The NSRLEArray ivar encoding and the instructions
// establish the 40-byte header and 16-byte item. The field names are descriptive.
typedef struct _NSRunArrayItem {
    NSUInteger length;
    id object;
} NSRunArrayItem;

typedef struct _NSRefCountedRunArray {
    os_unfair_lock lock;
    NSUInteger length;
    NSUInteger cachedLocation;
    uint32_t references;
    uint32_t used;
    uint32_t capacity;
    uint32_t cachedIndex;
    NSRunArrayItem runs[];
} NSRefCountedRunArray;

OF_ABI_ASSERT(offsetof(NSRefCountedRunArray, runs) == 40, "Native run-array header size.");
OF_ABI_ASSERT(sizeof(NSRunArrayItem) == 16, "Native run-array item size.");

// Native NSRLEArray, 0x22e6d62f0, external. class_ro_t 0x2228483e8 gives instance start 8,
// size 16 and the single pointer ivar. Superclass 0x22e6b6088 is NSObject (confidence: medium).
__attribute__((visibility("default")))
@interface ONSRLEArray : NSObject {
    @protected
    NSRefCountedRunArray *theList;
}
- (id)initWithRefCountedRunArray:(NSRefCountedRunArray *)list;
- (void)_makeNewListFrom:(NSRefCountedRunArray *)list;
- (id)objectAtIndex:(NSUInteger)index effectiveRange:(ONSRange *)range;
- (id)objectAtIndex:(NSUInteger)index effectiveRange:(ONSRange *)range runIndex:(NSUInteger *)runIndex;
- (NSUInteger)count;
- (id)copyWithZone:(struct _NSZone *)zone;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
@end

// Native NSMutableRLEArray, 0x22e6d6200, external. class_ro_t 0x222848478 gives superclass
// NSRLEArray, instance start and size 16, and no additional ivars.
__attribute__((visibility("default")))
@interface ONSMutableRLEArray : ONSRLEArray
- (void)_setBlockCapacity:(NSUInteger)capacity;
- (void)insertObject:(id)object range:(ONSRange)range;
- (void)replaceObjectsInRange:(ONSRange)range withObject:(id)object length:(NSUInteger)length;
- (void)deleteObjectsInRange:(ONSRange)range;
@end
