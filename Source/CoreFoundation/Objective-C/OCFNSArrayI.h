//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <stdarg.h>

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSArray.h"

// Native class __NSArrayI (__DATA,__objc_data 0x22e6ce620, was a private external), the immutable array, an
// NSArray subclass; class_ro_t 0x2227c5598: flags 0x10, instance start 8, size 16; ivars (instance variable
// list 0x2227c5550, both eight-byte aligned) used at 8 (8 bytes, the count; offset variable 0x22e6cc22c) and
// list at 16 (size 0, the object's trailing array of object pointers; offset variable 0x22e6cc228) (iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for offsets and sizes;
// the ivar names come from the nm symbols _OBJC_IVAR_$___NSArrayI.used and .list, the type encodings lie
// outside the extracted image; the methods read both at fixed offsets).
@interface __ONSArrayI : ONSArray {
    @public
    NSUInteger used;
    id list[0];
}
@end

// Native class __NSArrayI_Transfer (__DATA,__objc_data 0x22e6ce670, was a private external), the immutable
// array that adopts a malloc'd buffer (the class __NSArrayI_transferNew allocates), an NSArray subclass;
// class_ro_t 0x2227c5670: flags 0x10, instance start 8, size 24; ivars (instance variable list 0x2227c5628,
// both eight-byte aligned) _used at 8 (8 bytes, the count; offset variable 0x22e6cc230) and _list at 16 (8
// bytes, the buffer; offset variable 0x22e6cc234) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for names, offsets and sizes; the type
// encodings lie outside the extracted image, the types follow the methods' 64-bit loads). The ivars are
// public so that __NSArrayI_transferNew reaches them, as the native function does. Its methods are defined
// by Objective-C/OCFNSArrayI.m.
@interface __ONSArrayI_Transfer : ONSArray {
    @public
    NSUInteger _used;
    id *_list;
}
@end
#endif

// Native __NSArrayI_new, 0x1c02abd0c (was a private external; Objective-C/OCFNSArrayI.m): a new immutable
// array of count objects, taken from objects or, when arguments is not NULL, the first from *objects and the
// rest from the variable arguments; retained unless noRetain.
OF_EXTERN id __NSArrayI_new(const id *objects, va_list arguments, NSUInteger count, BOOL noRetain);

// Native __NSArrayI_transferNew, 0x1c02acb00 (was a private external; Objective-C/OCFNSArrayI.m): a new
// __NSArrayI_Transfer that adopts the malloc'd buffer list holding count objects (already owned); frees the
// buffer and answers NULL when the object cannot be allocated. The capacity is not read.
OF_EXTERN id __NSArrayI_transferNew(id *list, NSUInteger count, NSUInteger capacity);
