//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSMutableArray.h"

// Native class __NSPlaceholderArray (__DATA,__objc_data 0x22e6cc780, was a private external), the class of
// the placeholder objects +[NSArray allocWithZone:] answers, an NSMutableArray subclass; class_ro_t
// 0x2227bf498: flags 0x10, instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified).
@interface __ONSPlaceholderArray : ONSMutableArray
@end
#endif

// Native __NSArrayImmutablePlaceholder (0x1c0161f5c) and __NSArrayMutablePlaceholder (0x1c0161f68), both
// private externals (Objective-C/OCFNSPlaceholderArray.m): the immutable and mutable placeholder arrays.
OF_EXTERN id __NSArrayImmutablePlaceholder(void);
OF_EXTERN id __NSArrayMutablePlaceholder(void);
