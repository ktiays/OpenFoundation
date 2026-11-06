//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSMutableSet.h"

#include <objc/objc.h>

#ifdef __OBJC__
// Native class __NSPlaceholderSet (__DATA,__objc_data 0x22e6cc8c0, was a private external), the class of the
// two static placeholder objects +[NSSet allocWithZone:] answers, an NSMutableSet subclass; class_ro_t
// 0x2227bf788: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its methods are defined by
// Objective-C/OCFNSPlaceholderSet.m.
@interface __ONSPlaceholderSet : ONSMutableSet
@end

// Native __NSSetImmutablePlaceholder (0x1c019713c) and __NSSetMutablePlaceholder (0x1c0197148), were private
// externals: the static placeholder objects +[NSSet allocWithZone:] answers for NSSet and NSMutableSet.
OF_EXTERN id __NSSetImmutablePlaceholder(void);
OF_EXTERN id __NSSetMutablePlaceholder(void);
#endif
