//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "Foundation/Base/ONSFoundationSupport.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Foundation's NSConcreteNotification (__DATA,__objc_data 0x22e6db7a0, non-external), the concrete subclass
// +allocWithZone: substitutes; the class and its layout are defined in ONSConcreteNotification.h.
@class ONSConcreteNotification;

// Native class NSNotification (__DATA,__objc_data 0x22e6db778, external); class_ro_t 0x2228594d0: instance
// start and size 8, no ivars (iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified). Its superclass (0x22e6b6088) lies outside the image; NSObject is the SDK hierarchy
// (confidence: medium). The protocol list (0x222859430) and the property list (0x23d892951) are not adopted
// here (unread).
__attribute__((visibility("default")))
@interface ONSNotification : NSObject

+ (id)notificationWithName:(id)name object:(id)object userInfo:(id)userInfo;
- (id)initWithName:(id)name object:(id)object userInfo:(id)userInfo;
- (id)name;
- (id)object;
- (id)userInfo;

@end
#endif
