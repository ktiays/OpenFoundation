//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

// Native _NSPerformanceMeter 0x22e6d6b88; defined in Archiving/ONSPerformanceMeter.m.
@interface _ONSPerformanceMeter : NSObject
- (id)initWithTarget:(id)target;
- (void)invalidate;
@end
