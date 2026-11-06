//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSArrayHelpers.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/Objective-C/OCFRequireConcreteImplementation.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

#import "CoreFoundation/Objective-C/OCFNSArray.h"

// Native class NSMutableArray (__DATA,__objc_data 0x22e6ce2b0, external), an NSArray subclass; class_ro_t
// 0x2227c4a88: flags 0, instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The image defines no category on it (no
// "-[NSMutableArray(" symbol in the nm listing). Its methods are restored in Objective-C/OCFNSMutableArray.m.
__attribute__((visibility("default")))
@interface ONSMutableArray : ONSArray
@end
#endif
