//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSOrderedSet.h"

// Native class __NSOrderedSetI (__DATA,__objc_data 0x22e6cc5a0, was a private external), the immutable
// ordered set, an NSOrderedSet subclass; class_ro_t 0x2227bf080: flags 0x10, instance start 8, size 16;
// ivars (instance variable list 0x2227beff8, in this order) _used at 8 (8 bytes, 64-bit alignment; offset
// variable 0x22e6cbef0), _stride at 15 (4 bytes, 32-bit alignment; offset variable 0x22e6cbefc), _szidx at 15
// (8 bytes, 64-bit alignment; offset variable 0x22e6cbef4) and _list at 16 (size 0; offset variable
// 0x22e6cbef8) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for
// offsets, sizes and alignments; the names come from the nm symbols, the type encodings lie outside the
// extracted image). The shared offset 15 and the accesses establish bit-fields: _used the low 56 bits of the
// word at 8 (read as a word, a halfword and a byte), _stride the next two bits and _szidx the top six (the
// byte at 15, read with & 3 and >> 2); the declarations below give clang's layout those offsets
// (confidence: medium for the declared types, from the reported sizes). _list holds the objects in order,
// followed by the open-addressing index table. Its methods are restored in Objective-C/OCFNSOrderedSetI.m.
@interface __ONSOrderedSetI : ONSOrderedSet {
    @public
    NSUInteger _used : 56;
    unsigned int _stride : 2;
    NSUInteger _szidx : 6;
    id _list[0];
}
@end

// Native __NSOrderedSetI_new, 0x1c0151b5c (was a private external; Objective-C/OCFNSOrderedSetI.m): a new
// __NSOrderedSetI holding the distinct objects of objects[0 ..< count] in order (later duplicates by identity
// or -isEqual: are dropped); bit 0 of noRetain skips retaining.
OF_EXTERN id __NSOrderedSetI_new(const id *objects, NSUInteger count, BOOL noRetain);
#endif
