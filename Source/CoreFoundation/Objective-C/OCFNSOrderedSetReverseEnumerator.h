//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSEnumerator.h"

// Native class __NSOrderedSetReverseEnumerator (__DATA,__objc_data 0x22e6cd680, was a private external), the
// enumerator -[NSOrderedSet reverseObjectEnumerator] answers, an NSEnumerator subclass; class_ro_t
// 0x2227c1f28: flags 0x10, instance start 8, size 24; ivars (instance variable list 0x2227c1ee0, both
// eight-byte aligned) _obj at 8 (8 bytes, the retained ordered set; offset variable 0x22e6cc060) and _idx at
// 16 (8 bytes, the next index plus one; offset variable 0x22e6cc064) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for offsets and sizes; the ivar names come from
// the nm symbols, the type encodings lie outside the extracted image: _obj shares its type string with
// __NSOrderedSetReversed's _orderedSet, _idx with its _cnt). Its methods are restored in
// Objective-C/OCFNSOrderedSetReverseEnumerator.m.
@interface __ONSOrderedSetReverseEnumerator : ONSEnumerator {
    id _obj;
    NSUInteger _idx;
}

- (id)initWithObject:(id)object;

@end
#endif
