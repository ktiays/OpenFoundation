//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"

#ifdef __OBJC__
// Native class NSNull (__DATA,__objc_data 0x22e6ce288, external), an NSObject subclass whose only instance is
// kCFNull; class_ro_t 0x2227c49f8: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified).
__attribute__((visibility("default")))
@interface ONSNull : NSObject

+ (id)null;

@end
#endif
