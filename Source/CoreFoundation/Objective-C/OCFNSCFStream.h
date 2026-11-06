//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"

#ifdef __OBJC__
// Native classes __NSCFInputStream (__DATA,__objc_data 0x22e6cd298) and __NSCFOutputStream (0x22e6cd2e8),
// both were private externals, subclasses of NSInputStream and NSOutputStream; class_ro_t 0x2227c11d0 and
// 0x2227c1260: instance start and size 136, no ivars of their own (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified).
@interface __ONSCFInputStream : ONSInputStream

- (id)initWithURL:(id)url;

@end

@interface __ONSCFOutputStream : ONSOutputStream

- (id)initWithURL:(id)url append:(BOOL)shouldAppend;

@end
#endif
