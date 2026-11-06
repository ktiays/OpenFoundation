//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSTimeZone.h"

#ifdef __OBJC__
// Native class __NSPlaceholderTimeZone (__DATA,__objc_data 0x22e6ce5f8, was a private external), an
// NSTimeZone subclass; class_ro_t 0x2227c53d0: instance start and size 8, no ivars (iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its only instance is the static immutable
// placeholder of Objective-C/OCFNSTimeZone.m (__immutablePlaceholderTimeZone).
@interface __ONSPlaceholderTimeZone : ONSTimeZone
@end
#endif
