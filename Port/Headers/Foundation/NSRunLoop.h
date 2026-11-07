//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSDate.h>

#if defined(__OBJC__)
@class NSTimer, NSPort, NSArray<ObjectType>, NSString;

FOUNDATION_EXPORT NSRunLoopMode const NSRunLoopCommonModes;

@interface NSRunLoop : NSObject

@property (class, readonly, strong) NSRunLoop *mainRunLoop;
- (CFRunLoopRef)getCFRunLoop CF_RETURNS_NOT_RETAINED;
- (void)addTimer:(NSTimer *)timer forMode:(NSRunLoopMode)mode;

@end
#endif
