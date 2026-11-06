//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

struct _NSZone;

// Native NSFormatter, 0x22e6d7fb0. External. class_ro_t 0x22284e8e0: instance start
// and size 8, no ivars. The superclass slot binds to NSObject (medium confidence).
__attribute__((visibility("default")))
@interface ONSFormatter : NSObject

- (id)copyWithZone:(struct _NSZone *)zone;
- (void)encodeWithCoder:(id)coder;
- (id)initWithCoder:(id)coder;

@end
