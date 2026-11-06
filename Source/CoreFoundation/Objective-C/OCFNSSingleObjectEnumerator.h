//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSEnumerator.h"

#ifdef __OBJC__
// Native class __NSSingleObjectEnumerator (__DATA,__objc_data 0x22e6cdf40, was a private external), an
// NSEnumerator subclass; class_ro_t 0x2227c3f90: instance start 8, size 24; ivars _theCollection (offset 8)
// and _theObjectToReturn (16), eight bytes each with eight-byte alignment, their offset symbols
// _OBJC_IVAR_$___NSSingleObjectEnumerator._theCollection (0x22e6cc19c) and ._theObjectToReturn (0x22e6cc1a0)
// (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The ivar type
// encodings lie outside the extracted image (unread); both are objects by their use.
@interface __ONSSingleObjectEnumerator : ONSEnumerator {
    id _theCollection;
    id _theObjectToReturn;
}

- (id)initWithObject:(id)object collection:(id)collection;

@end
#endif
