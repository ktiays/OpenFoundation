//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSFastEnumerationEnumerator.h"

// Native class __NSArrayReverseEnumerator (__DATA,__objc_data 0x22e6ce1c0, was a private external), the
// enumerator -[NSArray reverseObjectEnumerator] answers, an NSEnumerator subclass; class_ro_t 0x2227c4838:
// flags 0x10, instance start 8, size 24; ivars (instance variable list 0x2227c47f0, both eight-byte aligned)
// _obj at 8 (8 bytes, the retained array; offset variable 0x22e6cc1f4) and _idx at 16 (8 bytes, the next
// index plus one; offset variable 0x22e6cc1f8) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for offsets and sizes; the ivar names come from
// the nm symbols, the type encodings lie outside the extracted image). Its methods are restored in
// Objective-C/OCFNSArrayReverseEnumerator.m.
@interface __ONSArrayReverseEnumerator : ONSEnumerator {
    id _obj;
    NSUInteger _idx;
}

- (id)initWithObject:(id)object;

@end
#endif
