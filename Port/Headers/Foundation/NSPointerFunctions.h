//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>

#if defined(__OBJC__)
typedef NS_OPTIONS(NSUInteger, NSPointerFunctionsOptions) {
    NSPointerFunctionsStrongMemory = (0UL << 0),
    NSPointerFunctionsOpaqueMemory = (2UL << 0),
    NSPointerFunctionsMallocMemory = (3UL << 0),
    NSPointerFunctionsMachVirtualMemory = (4UL << 0),
    NSPointerFunctionsWeakMemory = (5UL << 0),
    NSPointerFunctionsObjectPersonality = (0UL << 8),
    NSPointerFunctionsOpaquePersonality = (1UL << 8),
    NSPointerFunctionsObjectPointerPersonality = (2UL << 8),
    NSPointerFunctionsCStringPersonality = (3UL << 8),
    NSPointerFunctionsStructPersonality = (4UL << 8),
    NSPointerFunctionsIntegerPersonality = (5UL << 8),
    NSPointerFunctionsCopyIn = (1UL << 16)
};

@interface NSPointerFunctions : NSObject <NSCopying>

- (instancetype)initWithOptions:(NSPointerFunctionsOptions)options NS_DESIGNATED_INITIALIZER;

@end
#endif
