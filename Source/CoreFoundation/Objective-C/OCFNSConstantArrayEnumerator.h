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

// Native class __NSConstantArrayEnumerator (__DATA,__objc_data 0x22e6ccb68, was a private external), the
// enumerator -[NSConstantArray objectEnumerator] answers, an NSEnumerator subclass; class_ro_t 0x2227bfe58:
// flags 0x10, instance start 8, size 24; ivars (instance variable list 0x2227bfe10, both eight-byte aligned)
// _underlyingArray at 8 (8 bytes, the constant array, not retained; offset variable 0x22e6cbf64) and _index
// at 16 (8 bytes, the next index; offset variable 0x22e6cbf70) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for offsets and sizes; the ivar names come from
// the nm symbols, the type encodings lie outside the extracted image). Its methods are restored in
// Objective-C/OCFNSConstantArrayEnumerator.m.
@interface __ONSConstantArrayEnumerator : ONSEnumerator {
    id _underlyingArray;
    NSUInteger _index;
}

- (id)initWithConstantArray:(id)array;

@end
#endif
