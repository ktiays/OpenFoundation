//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSEnumerator.h"

// Native __NSEnumerator0__, 0x2227a8438 (__DATA_CONST,__const, was a private external): the address of the
// empty enumerator singleton __NSEnumerator0__struct (0x2227a8428), defined in
// Objective-C/OCFNSEnumerator0.m.
OF_EXTERN id const __NSEnumerator0__;

#ifdef __OBJC__
// Native class __NSEnumerator0 (__DATA,__objc_data 0x22e6cdfb8, was a private external), the empty
// enumerator, an NSEnumerator subclass; class_ro_t 0x2227c4020: instance start and size 8, no ivars (iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its only instance is
// the static singleton __NSEnumerator0__struct.
@interface __ONSEnumerator0 : ONSEnumerator

+ (id)sharedInstance;

@end
#endif
