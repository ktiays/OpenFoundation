//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#include <OpenFoundation/Range/ONSRange.h>

#include <OpenCoreFoundation/OCFBuild.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <stdbool.h>
#include <stdint.h>

// Shared declarations of the NSIndexSet unit (Collections/ONSIndexSet.m), iOS Simulator 24A434, arm64,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless a declaration says
// otherwise. iOS Device 24A446: unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

// The out-of-line storage of an index set holding several ranges (the _data member of the _internal ivar's
// _multipleRanges arm), as the unit's functions read and write it: a 64-byte header followed by capacity
// ranges, of which rangeCount, starting at firstRange, are in use. The cache fields remember the index the
// last sequential lookup returned (cachedIndex), the range holding it (cachedRange) and its offset in that
// range (cachedOffset); cacheOwner is the pthread_t of the thread that claimed the cache (0 when free), read
// with load-acquire and claimed with compare-and-swap (confidence: verified for offsets, widths and use; the
// native field names are not in the image, the names here are local descriptive names, as is
// IndexSetRangeStorage).
typedef struct {
    NSUInteger capacity;
    NSUInteger rangeCount;
    NSUInteger indexCount;
    NSUInteger firstRange;
    NSUInteger cachedIndex;
    NSUInteger cachedRange;
    NSUInteger cachedOffset;
    uintptr_t cacheOwner;
    ONSRange ranges[];
} IndexSetRangeStorage;

_Static_assert(sizeof(IndexSetRangeStorage) == 64, "The index set range storage header is 64 bytes.");

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSIndexSet (__DATA,__objc_data 0x22e6d97c0, external); class_ro_t 0x222852ce0: flags 0,
// instance start 8, size 32. Ivars (instance variable list 0x222852c60): _indexSetFlags at 8 (8 bytes,
// eight-byte aligned, offset variable 0x22e6d46fc; encoding {?="_hasSingleRange"b1"_hasBitfield"b1
// "_reservedArrayBinderController"b30}) and _internal at 16 (16 bytes, offset variable 0x22e6d4700; encoding
// (?="_singleRange"{?="_range"{_NSRange="location"Q"length"Q}}"_multipleRanges"{?="_data"^v"_reserved"^v}
// "_singleBitfield"{?="_bitfield"Q})). The superclass pointer (0x22e6b6088) lies outside the image: NSObject,
// libobjc's root class (confidence: verified). The bit-fields' storage unit is spelled NSUInteger to give the
// ivar its native eight bytes; the methods read and write the low 32-bit word. The three representations the
// methods use: both flag bits set, _singleBitfield holds a 64-bit membership mask; only _hasSingleRange set,
// _singleRange holds one range (an empty set when its length is 0); neither set, _multipleRanges._data points
// to an IndexSetRangeStorage. The ivars are public so that the unit's C functions reach them, as the native
// functions do. Its methods are restored in Collections/ONSIndexSet.m.
__attribute__((visibility("default")))
@interface ONSIndexSet : NSObject {
    @public
    struct {
        NSUInteger _hasSingleRange : 1;
        NSUInteger _hasBitfield : 1;
        NSUInteger _reservedArrayBinderController : 30;
    } _indexSetFlags;

    union {
        struct {
            ONSRange _range;
        } _singleRange;

        struct {
            void *_data;
            void *_reserved;
        } _multipleRanges;

        struct {
            NSUInteger _bitfield;
        } _singleBitfield;
    } _internal;
}

+ (id)_alloc;
+ (id)allocWithZone:(struct _NSZone *)zone;
+ (id)indexSet;
- (id)_init;
- (id)init;
- (id)initWithIndex:(NSUInteger)value;
- (id)initWithIndexesInRange:(ONSRange)range;
- (void)_setContentToContentFromIndexSet:(id)indexSet;
- (id)initWithIndexSet:(id)indexSet;
- (id)initWithBitfield:(NSUInteger)bitfield;
- (void)dealloc;
+ (BOOL)supportsSecureCoding;
- (void)encodeWithCoder:(id)coder;
- (id)initWithCoder:(id)coder;
- (Class)classForCoder;
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded;
- (NSUInteger)_indexOfRangeBeforeOrContainingIndex:(NSUInteger)index;
- (NSUInteger)_indexOfRangeAfterOrContainingIndex:(NSUInteger)index;
- (NSUInteger)hash;
- (BOOL)isEqualToIndexSet:(id)indexSet;
- (BOOL)isEqual:(id)object;
- (id)copyWithZone:(struct _NSZone *)zone;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
- (NSUInteger)count;
- (NSUInteger)firstIndex;
- (NSUInteger)lastIndex;
- (NSUInteger)_indexClosestToIndex:(NSUInteger)index
                      equalAllowed:(BOOL)equalAllowed
                         following:(BOOL)following __attribute__((objc_direct));
- (NSUInteger)indexGreaterThanIndex:(NSUInteger)value;
- (NSUInteger)indexLessThanIndex:(NSUInteger)value;
- (NSUInteger)indexGreaterThanOrEqualToIndex:(NSUInteger)value;
- (NSUInteger)getIndexes:(NSUInteger *)indexBuffer
                maxCount:(NSUInteger)bufferSize
            inIndexRange:(ONSRange *)range;
- (NSUInteger)__getContainmentVector:(BOOL *)vector inRange:(ONSRange)range;
- (NSUInteger)rangeCount;
- (ONSRange)rangeAtIndex:(NSUInteger)rangeIndex;
- (BOOL)getBitfield:(NSUInteger *)bitfield;
- (id)description;
- (void)enumerateRangesUsingBlock:(void (^)(ONSRange range, BOOL *stop))block;
@end

// Native _NSPlaceholderIndexSet, 0x22e6d9838, non-external; class_ro_t 0x222852b00.
// Native _NSConstantIndexSet, 0x22e6d9798, was private external; class_ro_t 0x222852b90.
// Both subclass NSIndexSet with instance start and size 32 and no additional ivars.
__attribute__((visibility("hidden")))
@interface _ONSPlaceholderIndexSet : ONSIndexSet
@end

__attribute__((visibility("hidden")))
@interface _ONSConstantIndexSet : ONSIndexSet
@end

_Static_assert(sizeof(((ONSIndexSet *)0)->_indexSetFlags) == 8, "NSIndexSet's _indexSetFlags is 8 bytes.");
_Static_assert(sizeof(((ONSIndexSet *)0)->_internal) == 16, "NSIndexSet's _internal is 16 bytes.");

#endif

// Native data _NSTaggedIndexSetAllowed, 0x22e6fcb5a, non-external. The writer remains unread.
extern __attribute__((visibility("hidden"))) bool indexSetTaggedAllowed __asm("__NSTaggedIndexSetAllowed");
