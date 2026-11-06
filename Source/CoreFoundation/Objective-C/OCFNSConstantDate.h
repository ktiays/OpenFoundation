//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSDate.h"

#ifdef __OBJC__
// Native class NSConstantDate (__DATA,__objc_data 0x22e6cf278, external), an NSDate subclass; class_ro_t
// 0x2227c85a0: instance start 8, size 16; ivar _ti at offset 8 (its offset symbol
// _OBJC_IVAR_$_NSConstantDate._ti, 0x22e6cc4cc), eight bytes with eight-byte alignment (iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The ivar's name and type encoding lie
// outside the extracted image (unread); the name is the offset symbol's, the type follows its use (an
// NSTimeInterval, spelled double). The class is external natively (default visibility); the constant dates
// of Objective-C/OCFNSDate.m are its instances.
__attribute__((visibility("default")))
@interface ONSConstantDate : ONSDate {
    double _ti;
}
@end
#endif
