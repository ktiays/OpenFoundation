//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"

#ifdef __OBJC__
// Native class __NSCFData (__DATA,__objc_data 0x22e6cd130, was a private external), an NSMutableData
// subclass; class_ro_t 0x2227c0f48: instance start 8, size 48, five 8-byte ivars at 8, 16, 24, 32 and 40 (iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The ivar names and type
// encodings lie outside the extracted image and no restored method reads them: unread, size placeholders
// (they cover the CFData object's fields after the isa).
@interface __ONSCFData : ONSMutableData {
    uint64_t _unreadIvarAt8;
    uint64_t _unreadIvarAt16;
    uint64_t _unreadIvarAt24;
    uint64_t _unreadIvarAt32;
    uint64_t _unreadIvarAt40;
}
@end
#endif
