//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"

// The 32-byte info record of a bridged timer (typed allocation 0x10800406BDE4C18): a 32-bit word cleared at 0
// (the reference count _timerRetain and _timerRelease maintain atomically), the
// retained target at 8, the selector at 16 (both read by __CFFireTimer) and the retained user info at 24.
// TimerInfo and its field names are local descriptive spellings.
typedef struct {
    uint32_t count;
    id target;
    SEL selector;
    id userInfo;
} TimerInfo;

#ifdef __OBJC__
// Native class __NSCFTimer (__DATA,__objc_data 0x22e6cd1f8, was a private external), an NSTimer subclass;
// class_ro_t 0x2227c1068: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified).
@interface __ONSCFTimer : ONSTimer

- (BOOL)isValid;
- (double)timeInterval;
- (void)invalidate;

@end
#endif
