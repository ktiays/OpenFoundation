//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSLocale.h"

#ifdef __OBJC__
// Native class _NSPlaceholderLocale (__DATA,__objc_data 0x22e6ce738, was a private external), an NSLocale
// subclass; class_ro_t 0x2227c5700: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its only instance is the static placeholder of
// Objective-C/OCFNSLocale.m.
@interface _ONSPlaceholderLocale : ONSLocale
@end
#endif
