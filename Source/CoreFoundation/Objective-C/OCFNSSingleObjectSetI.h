//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSSet.h"

#include <objc/objc.h>

#ifdef __OBJC__
// Native class __NSSingleObjectSetI (__DATA,__objc_data 0x22e6cc870, was a private external), the immutable
// set of one object, an NSSet subclass; class_ro_t 0x2227bf6f8: instance start 8, size 16; ivar element at 8
// (8 bytes, 8-byte alignment; offset symbol _OBJC_IVAR_$___NSSingleObjectSetI.element, 0x22e6cbf24, was a
// private external) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// for name, offset and size; the type encoding lies outside the extracted image, the type follows the
// methods' use, confidence: medium). Its methods are defined by Objective-C/OCFNSSingleObjectSetI.m.
@interface __ONSSingleObjectSetI : ONSSet {
    @public
    id element;
}
@end

// Native __NSSingleObjectSetI_new, 0x1c018dbb4 (was a private external): a new __NSSingleObjectSetI holding
// the object; bit 0 of noRetain skips retaining it.
OF_EXTERN id __NSSingleObjectSetI_new(id object, BOOL noRetain);
#endif
