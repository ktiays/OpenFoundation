//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"

#ifdef __OBJC__
// Native class __NSCFError (__DATA,__objc_data 0x22e6cd0b8, was a private external); class_ro_t 0x2227c0d50:
// instance start and size 40, no ivars of its own (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its superclass (0x22e6d78d0) lies outside the
// extracted image; NSError is the SDK hierarchy and the class looked up by name (confidence: medium).
@interface __ONSCFError : ONSError

- (id)domain;
- (NSInteger)code;
- (id)userInfo;

@end
#endif
