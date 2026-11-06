//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSStream (__DATA,__objc_data 0x22e6ccd98, external), an NSObject subclass; class_ro_t
// 0x2227c05b0: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The class has no methods in this image (its
// methods are Foundation's); its class object is defined here, with an empty implementation. It is the
// superclass of NSInputStream and NSOutputStream (not on family J's list; defined because they need it). The
// class is external natively (default visibility).
__attribute__((visibility("default")))
@interface ONSStream : NSObject
@end
#endif
