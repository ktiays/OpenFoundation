//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSDictionaryCopyOnWrite.h"
#include "CoreFoundation/Objective-C/OCFNSMutableSet.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <stdint.h>

// The storage ivar of __NSSetM (16 bytes at 16), as the unit's methods read it: the slot array (empty slots
// nil, removed slots the deleted marker), a 32-bit mutation counter at 8 (the word -countByEnumerating...
// points mutationsPtr at; 1 for a new set; reset to 0 once its bits 9-31 reach 125), and at 12 a 32-bit word
// whose low 26 bits count the occupied slots and whose top six bits hold the size index (confidence: verified
// for positions and widths; the native field names and the ivar's type encoding lie outside the extracted
// image, the names are descriptive). SetMStorage is a local descriptive name.
typedef struct {
    id *list;
    uint32_t mutations;

    union {
        uint32_t word;

        struct {
            uint32_t used : 26;
            uint32_t szidx : 6;
        };
    } state;
} SetMStorage;

_Static_assert(sizeof(SetMStorage) == 16, "The __NSSetM storage ivar is 16 bytes.");

#ifdef __OBJC__
// Native class __NSSetM (__DATA,__objc_data 0x22e6ce490, was a private external), the mutable hashed set, an
// NSMutableSet subclass; class_ro_t 0x2227c4fe8: instance start 8, size 32; ivars (offsets from their
// _OBJC_IVAR_$ symbols, were private externals) cow at 8 (8 bytes, the copy-on-write control block, read with
// load-acquire; 0x22e6cc21c) and storage at 16 (16 bytes; 0x22e6cc218) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for names, offsets and sizes; the type encodings
// lie outside the extracted image, the types follow the methods' use, confidence: medium). The ivars are
// public so that the unit's C functions reach them, as the native functions do. Its methods are defined by
// Objective-C/OCFNSSetM.m.
@interface __ONSSetM : ONSMutableSet {
    @public
    CopyOnWriteBlock *cow;
    SetMStorage storage;
}
@end

// Native class __NSFrozenSetM (__DATA,__objc_data 0x22e6ce4e0, was a private external), the immutable copy of
// a __NSSetM sharing its storage through the copy-on-write control block (the class __NSSetM_copy
// allocates), an NSSet subclass; class_ro_t 0x2227c50c0: flags 0x10, instance start 8, size 32; ivars
// (instance variable list 0x2227c5078, both eight-byte aligned) cow at 8 (8 bytes, read with load-acquire;
// offset variable 0x22e6cc224) and storage at 16 (16 bytes, the layout of __NSSetM's; offset variable
// 0x22e6cc220) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for
// names, offsets and sizes; the type encodings lie outside the extracted image, the types follow __NSSetM's).
// The ivars are public so that the unit's C functions reach them, as the native functions do. Its methods
// are defined by Objective-C/OCFNSSetM.m.
@interface __ONSFrozenSetM : ONSSet {
    @public
    CopyOnWriteBlock *cow;
    SetMStorage storage;
}
@end

// Native __NSSetM_new, 0x1c02a5edc (was a private external): a new __NSSetM with room for count objects,
// holding the objects of the array (when it is not NULL); bit 0 of noRetain skips retaining.
OF_EXTERN id __NSSetM_new(const id *objects, NSUInteger count, BOOL noRetain);
#endif
