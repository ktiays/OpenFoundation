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

// Native class __NSOrderedSetArrayProxy (__DATA,__objc_data 0x22e6cefd0, was a private external), the array
// view -[NSOrderedSet array] answers, an NSArray subclass; class_ro_t 0x2227c7990: flags 0x10, instance start
// 8, size 16; ivar (instance variable list 0x2227c7968) _orderedSet at 8 (8 bytes, eight-byte aligned, the
// retained ordered set; offset variable 0x22e6cc3e8) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for offset and size; the ivar name comes from
// the nm symbol, the type encoding lies outside the extracted image). Its methods are restored in
// Objective-C/OCFNSOrderedSetArrayProxy.m.
@interface __ONSOrderedSetArrayProxy : ONSArray {
    id _orderedSet;
}

- (id)initWithOrderedSet:(id)orderedSet;

@end
#endif
