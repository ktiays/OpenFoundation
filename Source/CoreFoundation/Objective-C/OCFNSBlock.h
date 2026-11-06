//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"

#ifdef __OBJC__
// Native classes of the block family (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified): NSBlock (__DATA,__objc_data 0x22e6ce878, external, an NSObject subclass,
// class_ro_t 0x2227c5d40) and its subclasses __NSStackBlock (0x22e6ce8c8), __NSMallocBlock (0x22e6ce918),
// __NSAutoBlock (0x22e6ce968), __NSGlobalBlock (0x22e6cea08) and __NSAutoBlock's subclass __NSFinalizingBlock
// (0x22e6ce9b8), all were private externals. Every class_ro_t gives instance start and size 8 and no ivars;
// the block's fields are the blocks ABI layout, not Objective-C ivars.
__attribute__((visibility("default")))
@interface ONSBlock : NSObject

- (void)invoke;
- (void)performAfterDelay:(double)delay;

@end

@interface __ONSStackBlock : ONSBlock
@end

@interface __ONSMallocBlock : ONSBlock
@end

@interface __ONSAutoBlock : ONSBlock
@end

@interface __ONSFinalizingBlock : __ONSAutoBlock
@end

@interface __ONSGlobalBlock : ONSBlock
@end
#endif
