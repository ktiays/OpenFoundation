//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSDictionaryCopyOnWrite.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <stdbool.h>
#include <stdint.h>

// The storage ivar of __NSArrayM (24 bytes at 16), a circular buffer of object pointers, as the unit's
// methods read it: the buffer (capacity slots; NULL for an array that never held a buffer), the slot of the
// first object (offset), the number of slots (capacity), a mutation counter (incremented by every mutation,
// reset to 0 once bits 9-31 reach 125; the fast-enumeration mutations pointer points at it) and the object
// count (used). Object i lives in slot offset + i, less capacity when that reaches capacity (confidence:
// verified for the positions and widths; the native field names and the ivar's type encoding lie outside the
// extracted image, the names are descriptive). ArrayMStorage is a local descriptive name.
typedef struct {
    id *list;
    uint32_t offset;
    uint32_t capacity;
    uint32_t mutations;
    uint32_t used;
} ArrayMStorage;

OF_ABI_ASSERT(sizeof(ArrayMStorage) == 24, "The __NSArrayM storage ivar is 24 bytes.");

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSMutableArray.h"

// Native class __NSArrayM (__DATA,__objc_data 0x22e6cc6e0, was a private external), the mutable array, an
// NSMutableArray subclass; class_ro_t 0x2227bf330: flags 0x10, instance start 8, size 40; ivars (instance
// variable list 0x2227bf2e8, both eight-byte aligned) cow at 8 (8 bytes, the copy-on-write control block,
// read with load-acquire; offset variable 0x22e6cbf08) and storage at 16 (24 bytes; offset variable
// 0x22e6cbf04) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for
// offsets and sizes; the ivar names come from the nm symbols _OBJC_IVAR_$___NSArrayM.cow and .storage, the
// type encodings lie outside the extracted image). The ivars are public so that the unit's C functions and
// the copy-on-write callbacks reach them, as the native functions do.
@interface __ONSArrayM : ONSMutableArray {
    @public
    CopyOnWriteBlock *cow;
    ArrayMStorage storage;
}
@end

// Native class __NSFrozenArrayM (__DATA,__objc_data 0x22e6cc730, was a private external), the immutable copy
// of a __NSArrayM that shares its storage through the copy-on-write control block (the class
// __NSArrayM_copy allocates), an NSArray subclass; class_ro_t 0x2227bf408: flags 0x10, instance start 8, size
// 40; ivars (instance variable list 0x2227bf3c0, both eight-byte aligned) cow at 8 (8 bytes, read with
// load-acquire; offset variable 0x22e6cbf10) and storage at 16 (24 bytes, the layout of __NSArrayM's
// storage; offset variable 0x22e6cbf0c) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified for offsets and sizes; the ivar names come from the nm symbols
// _OBJC_IVAR_$___NSFrozenArrayM.cow and .storage, the type encodings lie outside the extracted image, the
// types follow __NSArrayM's). The ivars are public so that the unit's C functions and the copy-on-write
// callbacks reach them, as the native functions do. Its methods are defined by Objective-C/OCFNSArrayM.m.
@interface __ONSFrozenArrayM : ONSArray {
    @public
    CopyOnWriteBlock *cow;
    ArrayMStorage storage;
}
@end
#endif

// Native __NSArrayM_new, 0x1c015f954 (was a private external; Objective-C/OCFNSArrayM.m): a new mutable array
// of count objects copied from objects (retained unless noRetain), or of capacity count and no objects when
// objects is NULL.
OF_EXTERN id __NSArrayM_new(const id *objects, NSUInteger count, BOOL noRetain);

// Native __NSArrayM_transferNew, 0x1c015fa8c (was a private external; Objective-C/OCFNSArrayM.m): a new
// mutable array that adopts the malloc'd buffer list of capacity slots holding count objects (already owned);
// frees the buffer and answers NULL when the object cannot be allocated.
OF_EXTERN id __NSArrayM_transferNew(id *list, NSUInteger count, NSUInteger capacity);
