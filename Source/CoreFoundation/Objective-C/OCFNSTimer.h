//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSTimer (__DATA,__objc_data 0x22e6ccca8, external), an NSObject subclass; class_ro_t
// 0x2227c02e8: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The class has no methods in this image (its
// methods are Foundation's); its class object is defined here, with an empty implementation. The class is
// external natively (default visibility).
__attribute__((visibility("default")))
@interface ONSTimer : NSObject
@end
#endif
