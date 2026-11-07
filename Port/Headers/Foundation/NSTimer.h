//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSDate.h>

#if defined(__OBJC__)
@interface NSTimer : NSObject

+ (NSTimer *)timerWithTimeInterval:(NSTimeInterval)ti
                            target:(id)aTarget
                          selector:(SEL)aSelector
                          userInfo:(id)userInfo
                           repeats:(BOOL)yesOrNo;
- (instancetype)initWithFireDate:(NSDate *)date
                        interval:(NSTimeInterval)ti
                          target:(id)t
                        selector:(SEL)s
                        userInfo:(id)ui
                         repeats:(BOOL)rep NS_DESIGNATED_INITIALIZER;

- (void)fire;
@property (copy) NSDate *fireDate;
@property (readonly) NSTimeInterval timeInterval;
@property NSTimeInterval tolerance;
- (void)invalidate;
@property (readonly, getter=isValid) BOOL valid;
@property (readonly, retain) id userInfo;

@end
#endif
