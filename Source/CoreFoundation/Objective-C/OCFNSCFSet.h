//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"

#ifdef __OBJC__
// Native class __NSCFSet (__DATA,__objc_data 0x22e6cf390, was a private external), an NSMutableSet subclass;
// class_ro_t 0x2227c8918: instance start 8, size 48, ivars at 8 (4 bytes, alignment 1), 12 (4, alignment 4),
// 16 (16, alignment 4), 32 and 40 (8 each) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified). The ivar names and type encodings lie outside the extracted image and no restored
// method reads them: unread, size placeholders.
@interface __ONSCFSet : ONSMutableSet {
    uint8_t _unreadIvarAt8[4];
    uint32_t _unreadIvarAt12;
    uint32_t _unreadIvarAt16[4];
    uint64_t _unreadIvarAt32;
    uint64_t _unreadIvarAt40;
}
@end
#endif
