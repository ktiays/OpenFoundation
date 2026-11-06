//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Collections/OCFBasicHash.h"
#include "CoreFoundation/Objective-C/OCFNSDictionaryCopyOnWrite.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

// The storage ivar of __NSOrderedSetM (16 bytes at 16), as the unit's methods read it: the CFBasicHash of
// the objects (each object its own key and value; equality through __NSOrderedSetEquateKeys and
// __NSOrderedSetHashKey, no retain or release callbacks) and the NSMutableArray holding them in order (which
// owns the references) (confidence: verified for positions and widths; the native field names and the ivar's
// type encoding lie outside the extracted image, the names are descriptive). OrderedSetMStorage is a local
// descriptive name.
typedef struct {
    BasicHashRef hash;
    id array;
} OrderedSetMStorage;

_Static_assert(sizeof(OrderedSetMStorage) == 16, "The __NSOrderedSetM storage ivar is 16 bytes.");

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSMutableOrderedSet.h"

// Native class __NSOrderedSetM (__DATA,__objc_data 0x22e6cd950, was a private external), the mutable ordered
// set, an NSMutableOrderedSet subclass; class_ro_t 0x2227c2810: flags 0x10, instance start 8, size 32; ivars
// (instance variable list 0x2227c27c8, both eight-byte aligned) cow at 8 (8 bytes, the copy-on-write control
// block, read with load-acquire; offset variable 0x22e6cc0a8) and storage at 16 (16 bytes; offset variable
// 0x22e6cc0a4) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for
// offsets and sizes; the ivar names come from the nm symbols, the type encodings lie outside the extracted
// image, the types follow the methods' use, confidence: medium). The ivars are public so that the unit's C
// functions and the copy-on-write callbacks reach them, as the native functions do. Its methods are restored
// in Objective-C/OCFNSOrderedSetM.m.
@interface __ONSOrderedSetM : ONSMutableOrderedSet {
    @public
    CopyOnWriteBlock *cow;
    OrderedSetMStorage storage;
}
@end

// Native class __NSFrozenOrderedSetM (__DATA,__objc_data 0x22e6cd9a0, was a private external), the immutable
// copy of a __NSOrderedSetM sharing its storage through the copy-on-write control block (the class
// -[__NSOrderedSetM copy] allocates), an NSOrderedSet subclass; class_ro_t 0x2227c28e8: flags 0x10, instance
// start 8, size 32; ivars (instance variable list 0x2227c28a0, both eight-byte aligned) cow at 8 (8 bytes,
// read with load-acquire; offset variable 0x22e6cc0b0) and storage at 16 (16 bytes, the layout of
// __NSOrderedSetM's; offset variable 0x22e6cc0ac) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for offsets and sizes; the ivar names come from
// the nm symbols, the type encodings lie outside the extracted image, the types follow __NSOrderedSetM's,
// confidence: medium). The ivars are public so that the unit's C functions reach them, as the native
// functions do. Its methods are restored in Objective-C/OCFNSOrderedSetM.m.
@interface __ONSFrozenOrderedSetM : ONSOrderedSet {
    @public
    CopyOnWriteBlock *cow;
    OrderedSetMStorage storage;
}
@end

// Native __NSOrderedSetM_new, 0x1c0237948 (was a private external; Objective-C/OCFNSOrderedSetM.m): a new
// __NSOrderedSetM holding the distinct objects of objects[0 ..< count] in order; bit 0 of noRetain skips
// retaining. Answers nil when a part cannot be made.
OF_EXTERN id __NSOrderedSetM_new(const id *objects, NSUInteger count, BOOL noRetain);
#endif
