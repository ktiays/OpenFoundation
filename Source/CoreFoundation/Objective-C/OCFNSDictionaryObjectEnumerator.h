//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSFastEnumerationEnumerator.h"

#ifdef __OBJC__
// Native class __NSDictionaryObjectEnumerator (__DATA,__objc_data 0x22e6cd540, was a private external), the
// object enumerator of the dictionaries, a __NSFastEnumerationEnumerator subclass that enumerates the keys
// and answers their objects; class_ro_t 0x2227c1c40: instance start and size 240, no ivars of its own (iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified).
@interface __ONSDictionaryObjectEnumerator : __ONSFastEnumerationEnumerator
@end
#endif
