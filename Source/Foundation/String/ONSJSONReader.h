//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

// Native class_ro_t 0x222868550, instance start 8, size 8.
@interface _ONSJSONReader : NSObject
- (id)parseData:(id)data options:(NSUInteger)options error:(id *)error;
@end
