//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSStream.h"

#include <stdint.h>

#ifdef __OBJC__
// Native class NSOutputStream (__DATA,__objc_data 0x22e6cce38, external), an NSStream subclass; class_ro_t
// 0x2227c0750: instance start 8, size 136; one ivar, _reserved (offset 8, its offset symbol
// _OBJC_IVAR_$_NSOutputStream._reserved at 0x22e6cbfa0), 128 bytes with one-byte alignment (iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The class has no methods in this
// image (its methods are Foundation's); its class object is defined here, with an empty implementation. The
// ivar's type encoding lies outside the extracted image (unread); it is spelled as its 128 bytes. The class
// is external natively (default visibility).
__attribute__((visibility("default")))
@interface ONSOutputStream : ONSStream {
    uint8_t _reserved[128];
}
@end
#endif
