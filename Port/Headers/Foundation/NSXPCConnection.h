//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSCoder.h>
#include <xpc/xpc.h>

#if defined(__OBJC__)
@class NSString;

@interface NSXPCCoder : NSCoder

- (void)encodeXPCObject:(xpc_object_t)xpcObject forKey:(NSString *)key;
- (xpc_object_t)decodeXPCObjectOfType:(xpc_type_t)type forKey:(NSString *)key;

@end
#endif
