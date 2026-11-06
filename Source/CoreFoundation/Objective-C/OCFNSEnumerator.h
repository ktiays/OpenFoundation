//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSEnumerator (__DATA,__objc_data 0x22e6cdec8, external), an NSObject subclass; class_ro_t
// 0x2227c3d40: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The method lists and the protocol list are in
// the shared cache's optimized lists, outside the extracted image (unread); the methods come from the IDA
// capture (Objective-C/OCFNSEnumerator.m). The class is external natively (default visibility).
__attribute__((visibility("default")))
@interface ONSEnumerator : NSObject

- (id)nextObject;
- (id)allObjects;
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length;

@end
#endif
