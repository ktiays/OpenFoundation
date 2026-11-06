//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"

#ifdef __OBJC__
// Native class __NSCFAttributedString (__DATA,__objc_data 0x22e6ccfc8, was a private external); class_ro_t
// 0x2227c0ba0: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its superclass (0x22e6d61b0) lies outside the
// extracted image; NSMutableAttributedString is the SDK hierarchy (confidence: medium).
@interface __ONSCFAttributedString : ONSMutableAttributedString
@end
#endif
