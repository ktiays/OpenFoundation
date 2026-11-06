//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSPort (__DATA,__objc_data 0x22e6cce88, external), an NSObject subclass; class_ro_t
// 0x2227c0870: instance start 8, size 24; ivars _valid (offset 8, one byte) and _reservedSpaceLength (16,
// eight bytes with eight-byte alignment), their offset symbols _OBJC_IVAR_$_NSPort._valid and
// _OBJC_IVAR_$_NSPort._reservedSpaceLength (were private externals) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The ivar type encodings lie outside the
// extracted image (unread); the types follow their sizes and the getters. The other NSPort methods are
// Foundation's. This interface replaces the size placeholder of Objective-C/OCFBridgedClasses.h.
@interface ONSPort : NSObject {
    BOOL _valid;
    NSUInteger _reservedSpaceLength;
}

- (BOOL)isValid;
- (NSUInteger)reservedSpaceLength;

@end
#endif
