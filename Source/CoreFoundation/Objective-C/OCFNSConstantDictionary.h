//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSDictionary.h"

#ifdef __OBJC__
// Native class NSConstantDictionary (__DATA,__objc_data 0x22e6cd888, external), an NSDictionary subclass, the
// class of the compiler's constant dictionary literals; class_ro_t 0x2227c2448: instance start 8, size 40;
// ivars (instance variable list 0x2227c23c0, offsets from their external _OBJC_IVAR_$ symbols) _options at
// 8, _count at 16, _keys at 24 and _objects at 32, 8 bytes each with 8-byte alignment (iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for names, offsets and sizes). The type
// encodings lie outside the extracted image: the types are read from the methods' use (confidence: medium
// until a method fixes them).
__attribute__((visibility("default")))
@interface ONSConstantDictionary : ONSDictionary {
    NSUInteger _options;
    NSUInteger _count;
    id *_keys;
    id *_objects;
}
@end
#endif
