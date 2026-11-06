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

// Native class __NSOrderedSetReversed (__DATA,__objc_data 0x22e6cd6d0, was a private external), the reversed
// view -[NSOrderedSet reversedOrderedSet] answers, an NSOrderedSet subclass; class_ro_t 0x2227c2000: flags
// 0x10, instance start 8, size 24; ivars (instance variable list 0x2227c1fb8, both eight-byte aligned)
// _orderedSet at 8 (8 bytes, a copy of the reversed ordered set; offset variable 0x22e6cc068) and _cnt at 16
// (8 bytes, its count; offset variable 0x22e6cc06c) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for offsets and sizes; the ivar names come from
// the nm symbols, the type encodings lie outside the extracted image). Its methods are restored in
// Objective-C/OCFNSOrderedSetReversed.m.
@interface __ONSOrderedSetReversed : ONSOrderedSet {
    id _orderedSet;
    NSUInteger _cnt;
}
@end
#endif
