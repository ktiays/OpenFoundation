//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"
#include "CoreFoundation/String/OCFStringFormat.h"

#include <stdarg.h>
#include <stdbool.h>

#ifdef __OBJC__
// Native class __NSCFString (__DATA,__objc_data 0x22e6ccf50, was a private external); class_ro_t
// 0x2227c0b10: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its superclass (0x22e6db6b0) lies outside the
// extracted image; NSMutableString is the SDK hierarchy and the messages to super (confidence: medium).
@interface __ONSCFString : ONSMutableString

- (NSUInteger)length;

@end
#endif
