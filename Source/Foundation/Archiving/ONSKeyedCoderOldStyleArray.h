//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

// Native _NSKeyedCoderOldStyleArray, 0x22e6dae18, non-external. class_ro_t 0x222857858:
// instance start/size 8/36; offsets 8, 16, 24, 32, 33 and 34.
@interface _ONSKeyedCoderOldStyleArray : NSObject {
    @private
    void *_addr;
    NSUInteger _count;
    NSUInteger _size;
    char _type;
    BOOL _decoded;
    char _padding[2];
}
- (id)initWithObjCType:(char)type count:(NSUInteger)count at:(const void *)address;
- (void)fillObjCType:(char)type count:(NSUInteger)count at:(void *)address;
@end
