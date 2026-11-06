//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSData (__DATA,__objc_data 0x22e6cccf8, external), an NSObject subclass; class_ro_t
// 0x2227c0430: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its only method in this image is
// +supportsSecureCoding; the others are Foundation's. This interface replaces the size placeholder of
// Objective-C/OCFBridgedClasses.h.
// Exported, as the native image exports the class.
__attribute__((visibility("default")))
@interface ONSData : NSObject
@end
#endif
