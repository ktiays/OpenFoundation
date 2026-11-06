//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSArray.h"

// Native class __NSArrayReversed (__DATA,__objc_data 0x22e6ce170, was a private external), the reversed view
// -[NSArray reversedArray] answers, an NSArray subclass; class_ro_t 0x2227c4760: flags 0x10, instance start
// 8, size 24; ivars (instance variable list 0x2227c4718, both eight-byte aligned) _array at 8 (8 bytes, a
// copy of the reversed array; offset variable 0x22e6cc1f0) and _cnt at 16 (8 bytes, its count; offset
// variable 0x22e6cc1ec) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified for offsets and sizes; the ivar names come from the nm symbols, the type encodings lie outside the
// extracted image: _array shares its type string with the single-object array's object ivar, _cnt with
// __NSArrayI's used word). Its methods are restored in Objective-C/OCFNSArrayReversed.m.
@interface __ONSArrayReversed : ONSArray {
    id _array;
    NSUInteger _cnt;
}
@end
#endif
