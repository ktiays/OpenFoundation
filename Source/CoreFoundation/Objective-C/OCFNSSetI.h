//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSSet.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <stdarg.h>

#ifdef __OBJC__
// Native class __NSSetI (__DATA,__objc_data 0x22e6cd770, was a private external), the immutable hashed set,
// an NSSet subclass; class_ro_t 0x2227c2188: instance start 8, size 16; ivars (instance variable list read
// with Local/Scratch/cf-core/macho.py, offsets from their _OBJC_IVAR_$ symbols 0x22e6cc070-0x22e6cc078, were
// private externals) _used at 8 (the low 58 bits of a 64-bit word), _szidx at 15 (the top six bits of that
// word: the last byte's bits 2-7) and _list at 16 (size 0: the slot array allocated past the instance, as
// __NSSetI_new sizes it) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified for names, offsets, sizes and the bit positions the methods use; the type encodings lie outside
// the extracted image, the bit-field declaration follows the offsets and the methods' masks, confidence:
// medium). Its methods are defined by Objective-C/OCFNSSetI.m.
@interface __ONSSetI : ONSSet {
    @public
    OF_UINT64_BITFIELD(NSUInteger) _used : 58;
    OF_UINT64_BITFIELD(NSUInteger) _szidx : 6;
    id _list[0];
}
@end

// Native __NSSetI_new, 0x1c01ea2e8 (was a private external): a new __NSSetI holding count objects, the first
// read from objects[0] and, when args is not NULL, the rest from args (the variadic tail of
// -initWithObjects: and +setWithObjects:), otherwise from objects[1...]; bit 0 of noRetain skips retaining.
OF_EXTERN id __NSSetI_new(const id *objects, va_list args, NSUInteger count, BOOL noRetain);
#endif
