//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSDictionary.h"

#ifdef __OBJC__
// Native class NSMutableDictionary (__DATA,__objc_data 0x22e6cd7e8, external), an NSDictionary subclass;
// class_ro_t 0x2227c2218: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its methods are defined by
// Objective-C/OCFNSMutableDictionary.m.
__attribute__((visibility("default")))
@interface ONSMutableDictionary : ONSDictionary
@end
#endif
