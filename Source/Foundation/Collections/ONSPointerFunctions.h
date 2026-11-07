//
//  Created by ktiays on 2026/11/04.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <stdbool.h>
#include <stdint.h>

// Shared declarations of the pointer functions unit (Collections/ONSPointerFunctions.m), iOS Simulator
// 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless a
// declaration says otherwise. iOS Device 24A446: unread. Reading:
// Source/Foundation/Collections/PointerCollectionsReading.md.

// The slice: the storage of NSHashTable, NSMapTable (one per keys and values), NSPointerArray and
// NSPointerFunctions. The struct and member names come from the ivar type encoding (string 0x1c1553c6b):
// {NSSlice="items"^^v"acquisitionProps"^{NSSliceExternalAcquisitionProperties}"personalityProps"
// ^{NSSliceExternalPersonalityProperties}"internalProps"^{NSSliceInternalProperties}}. The three property
// structs have no member names in the image; their layouts are read from the constant tables (__const
// 0x1c100dc70-0x1c100dccf and 0x2227d61d8-0x2227d64c7) and their uses, and the member names below are
// local descriptive names (confidence: verified for offsets, widths and use).
typedef struct ONSSlice ONSSlice;

// The memory side of the options (24 bytes). acquire is called with the item, the personality's size
// function and copyIn; relinquish with the item and the size function.
typedef struct ONSSliceExternalAcquisitionProperties {
    // Byte 0: no constant table sets it; -[NSConcreteMapTable _setBackingStore] tests it.
    uint8_t flag0;
    // Byte 1: weak memory (NSPointerFunctionsZeroingWeakMemory, NSPointerFunctionsWeakMemory).
    uint8_t isWeak;
    // Byte 2: weak memory stored through the ARC weak functions.
    uint8_t usesARCWeak;
    // Byte 3: weak reads autorelease (NSPointerFunctionsWeakMemory before the non-autoreleasing variant).
    uint8_t weakAutoreleasing;
    // Byte 4: NSPointerFunctionsCopyIn.
    uint8_t copyIn;
    void *(*acquire)(const void *item, NSUInteger (*size)(const void *item), BOOL copyIn);
    void (*relinquish)(const void *item, NSUInteger (*size)(const void *item));
} ONSSliceExternalAcquisitionProperties;

// The personality side of the options (40 bytes). hash and isEqual receive the size function last.
typedef struct ONSSliceExternalPersonalityProperties {
    // Byte 0: identity hashing (object pointer and opaque personalities).
    uint8_t usesIdentity;
    // Byte 1: integer personality (the item is its own hash).
    uint8_t isInteger;
    NSUInteger (*size)(const void *item);
    NSUInteger (*hash)(const void *item, NSUInteger (*size)(const void *item));
    BOOL (*isEqual)(const void *item1, const void *item2, NSUInteger (*size)(const void *item));
    id (*description)(const void *item);
} ONSSliceExternalPersonalityProperties;

// The backing store's barrier functions (64 bytes): strong (noWriteBarriers) or ARC weak, with or without
// the never-used sentinel 1 and autoreleasing reads. startRead returns the item of a slot (retained for
// weak non-autoreleasing reads) and, when isEmpty is not NULL, whether the slot is empty; finishRead balances
// a read; continueRead hands a read item to the caller.
typedef struct ONSSliceInternalProperties {
    uint8_t flag0;
    // Byte 1: weak storage (hashes become identities, equality fails).
    uint8_t isWeak;
    uint8_t usesARCWeak;
    // Byte 3: the value of a never-used slot (1 with the sentinel, else 0).
    uint8_t sentinel;
    // Byte 4: slots are plain pointers that may be read directly.
    uint8_t readsDirectly;
    void **(*allocate)(NSUInteger count);
    void (*free)(void **storage, NSUInteger count);
    void *(*startRead)(void **slot, bool *isEmpty);
    void (*finishRead)(void *item);
    void *(*continueRead)(void *item);
    void (*clear)(void **slot);
    void (*store)(void **storage, NSUInteger index, void *item);
} ONSSliceInternalProperties;

struct ONSSlice {
    void **items;
    ONSSliceExternalAcquisitionProperties *acquisitionProps;
    ONSSliceExternalPersonalityProperties *personalityProps;
    ONSSliceInternalProperties *internalProps;
};

OF_ABI_ASSERT(
    sizeof(ONSSliceExternalAcquisitionProperties) == 24, "The acquisition properties are 24 bytes."
);
OF_ABI_ASSERT(
    sizeof(ONSSliceExternalPersonalityProperties) == 40, "The personality properties are 40 bytes."
);
OF_ABI_ASSERT(sizeof(ONSSliceInternalProperties) == 64, "The internal properties are 64 bytes.");
OF_ABI_ASSERT(sizeof(ONSSlice) == 32, "The slice is 32 bytes.");

// NSFastEnumerationState's 64-byte ABI as the pointer collections' enumeration methods use it. A local
// spelling, not an exported type.
typedef struct {
    unsigned long state;
    id *itemsPtr;
    unsigned long *mutationsPtr;
    unsigned long extra[5];
} PointerCollectionEnumerationState;

// Native NSSliceInitWithSlice, 0x1c0597508 (hidden, was a private external): copies the three property
// structs of source into malloc'd copies and clears items.
OF_EXTERN void NSSliceInitWithSlice(ONSSlice *slice, const ONSSlice *source);

// Native NSSliceDealloc, 0x1c05975c4 (hidden, was a private external): frees the three property copies.
OF_EXTERN void NSSliceDealloc(ONSSlice *slice);

// Native __NSPointerFunctionCoding_isValidOptions, 0x1c05980bc (hidden, was a private external): whether the
// options are archivable (strong or weak memory; object or object pointer personality).
OF_EXTERN BOOL _NSPointerFunctionCoding_isValidOptions(NSUInteger options);

// Native __NSPointerFunctionCoding_bufferForCount, 0x1c0598178 (hidden, was a private external): a buffer of
// count pointers, or NULL.
OF_EXTERN void **_NSPointerFunctionCoding_bufferForCount(NSUInteger count);

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native NSPointerFunctions, 0x22e6dc560 (external); class_ro_t 0x22285bd08: flags 0, instance start and
// size 8, no ivars; the superclass (0x22e6b6088) is libobjc's NSObject. None of its own methods is reached.
__attribute__((visibility("default")))
@interface ONSPointerFunctions : NSObject
@end

// Native NSConcretePointerFunctions, 0x22e6dc4e8 (non-external); class_ro_t 0x22285bbd8: flags 0x10,
// instance start 8, size 40; ivar slice at 8 (offset variable 0x22e6d4a84).
__attribute__((visibility("hidden")))
@interface ONSConcretePointerFunctions : ONSPointerFunctions {
    @public
    ONSSlice slice;
}

+ (void)initializeBackingStore:(ONSSlice *)slice
                      sentinel:(BOOL)sentinel
             weakAutoreleasing:(BOOL)weakAutoreleasing
                       dynamic:(BOOL)dynamic;
+ (BOOL)initializeSlice:(ONSSlice *)slice withOptions:(NSUInteger)options;
- (id)init;
- (id)initWithOptions:(NSUInteger)options;
- (void)dealloc;
- (id)copyWithZone:(struct _NSZone *)zone;
- (NSUInteger)hash;
- (BOOL)isEqual:(id)object;
@end

// Native NSPointerArray, 0x22e6dc420 (external); class_ro_t 0x22285b8d0: flags 0, instance start and size 8,
// no ivars; the superclass is libobjc's NSObject.
__attribute__((visibility("default")))
@interface ONSPointerArray : NSObject

- (id)init;
- (id)initWithOptions:(NSUInteger)options;
- (id)initWithPointerFunctions:(id)functions;
+ (id)allocWithZone:(struct _NSZone *)zone;
- (id)initWithCoder:(id)coder;
- (Class)classForCoder;
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded;
- (id)pointerFunctions;
- (NSUInteger)countByEnumeratingWithState:(PointerCollectionEnumerationState *)state
                                  objects:(id *)buffer
                                    count:(NSUInteger)length;
- (void)addPointer:(void *)pointer;
- (NSUInteger)count;
- (id)copyWithZone:(struct _NSZone *)zone;
- (void)encodeWithCoder:(id)coder;
- (id)allObjects;
@end

// Native NSConcretePointerArray, 0x22e6dc3f8 (non-external); class_ro_t 0x22285ba48: flags 0x10, instance
// start 8, size 74. Ivars: slice at 8, count at 40, capacity at 48, options at 56, mutations at 64,
// needsCompaction at 72 (B) and hasDynamicSlice at 73 (B); offset variables 0x22e6d4a68-0x22e6d4a80.
__attribute__((visibility("hidden")))
@interface ONSConcretePointerArray : ONSPointerArray {
    @public
    ONSSlice slice;
    NSUInteger count;
    NSUInteger capacity;
    NSUInteger options;
    NSUInteger mutations;
    BOOL needsCompaction;
    BOOL hasDynamicSlice;
}

- (void)arrayGrow:(NSUInteger)newCapacity;
@end

#endif
