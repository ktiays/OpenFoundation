//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSSet.h"

// Native class __NSOrderedSetSetProxy (__DATA,__objc_data 0x22e6cf020, was a private external), the set view
// -[NSOrderedSet set] answers, an NSSet subclass; class_ro_t 0x2227c7a48: flags 0x10, instance start 8, size
// 16; ivar (instance variable list 0x2227c7a20) _orderedSet at 8 (8 bytes, eight-byte aligned, the retained
// ordered set; offset variable 0x22e6cc3ec) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified for offset and size; the ivar name comes from the nm symbol, the type encoding lies
// outside the extracted image). Its methods are restored in Objective-C/OCFNSOrderedSetSetProxy.m.
@interface __ONSOrderedSetSetProxy : ONSSet {
    id _orderedSet;
}

- (id)initWithOrderedSet:(id)orderedSet;

@end
#endif
