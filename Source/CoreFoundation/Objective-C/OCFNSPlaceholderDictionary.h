//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Collections/OCFNSCFDictionary.h"
#include "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"

#ifdef __OBJC__
// Native class __NSPlaceholderDictionary (__DATA,__objc_data 0x22e6cf2a0, was a private external), the class
// of the two static placeholder instances +[NSDictionary allocWithZone:] answers, an NSMutableDictionary
// subclass; class_ro_t 0x2227c8630: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified).
@interface __ONSPlaceholderDictionary : ONSMutableDictionary
@end
#endif
