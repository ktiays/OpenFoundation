//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <objc/objc.h>

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSMutableOrderedSet.h"

// Native class __NSPlaceholderOrderedSet (__DATA,__objc_data 0x22e6ceb20, was a private external), the isa of
// the two static placeholder objects +[NSOrderedSet allocWithZone:] answers, an NSMutableOrderedSet
// subclass; class_ro_t 0x2227c64d8: flags 0x10, instance start and size 8, no ivars (iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its methods are restored in
// Objective-C/OCFNSOrderedSetPlaceholder.m.
@interface __ONSPlaceholderOrderedSet : ONSMutableOrderedSet
@end

// Native __NSOrderedSetImmutablePlaceholder (0x1c02be6c4) and __NSOrderedSetMutablePlaceholder (0x1c02be6d0),
// were private externals (Objective-C/OCFNSOrderedSetPlaceholder.m): the static placeholder objects
// +[NSOrderedSet allocWithZone:] answers for NSOrderedSet and NSMutableOrderedSet.
OF_EXTERN id __NSOrderedSetImmutablePlaceholder(void);
OF_EXTERN id __NSOrderedSetMutablePlaceholder(void);
#endif
