//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"

#include <os/lock.h>

#ifdef __OBJC__
// Native class NSMachPort (__DATA,__objc_data 0x22e6cced8, external), an NSPort subclass; class_ro_t
// 0x2227c09a0: instance start 24, size 52, four ivars at 24 (8 bytes), 32 (8), 40 (8) and 48 (4) (iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The ivar names and type
// encodings lie outside the extracted image; the names below are local descriptive spellings and the types
// are those the restored accessors establish (an object stored and loaded at 24, 64-bit words at 32 and 40,
// the 40 one counted with an all-ones saturation value, and a 32-bit Mach port name at 48). Only the methods
// this track restores are defined; the class's other methods are unread.
__attribute__((visibility("default")))
@interface ONSMachPort : ONSPort {
    id _portDelegate;
    uint64_t _portFlags;
    uint64_t _portRetainCount;
    uint32_t _portName;
}
@end
#endif
