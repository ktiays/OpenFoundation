//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#include <uuid/uuid.h>

#if defined(__OBJC__)
@class NSString;

@interface NSUUID : NSObject <NSCopying, NSSecureCoding>

+ (instancetype)UUID;
- (instancetype)init NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithUUIDString:(NSString *)string;
- (instancetype)initWithUUIDBytes:(const uuid_t)bytes;
- (void)getUUIDBytes:(uuid_t)uuid;
- (NSComparisonResult)compare:(NSUUID *)otherUUID;
@property (readonly, copy) NSString *UUIDString;

@end
#endif
