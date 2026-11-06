//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>
#import <objc/NSObject.h>

// Native NSAssertionHandler, 0x22e6d79e8, external; class_ro_t 0x22284d438.
// Instance start and size 8, no ivars. The superclass is libobjc's NSObject.
__attribute__((visibility("default")))
@interface ONSAssertionHandler : NSObject
+ (id)currentHandler;
- (void)handleFailureInFunction:(id)function
                           file:(id)file
                     lineNumber:(NSInteger)line
                    description:(id)description, ...;
@end
