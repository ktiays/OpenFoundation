//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSDate.h>
#import <Foundation/NSNotification.h>

#if defined(__OBJC__)
@class NSArray<ObjectType>, NSMutableDictionary, NSString, NSNumber;

@interface NSThread : NSObject

@property (class, readonly, strong) NSThread *currentThread;
@property (readonly, retain) NSMutableDictionary *threadDictionary;
@property (copy) NSString *name;
@property (readonly) BOOL isMainThread;
@property (class, readonly) BOOL isMainThread;
@property (class, readonly, strong) NSThread *mainThread;
- (instancetype)init NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithTarget:(id)target selector:(SEL)selector object:(id)argument;
- (instancetype)initWithBlock:(void (^)(void))block;

@end

FOUNDATION_EXPORT NSNotificationName const NSWillBecomeMultiThreadedNotification;

@interface NSObject (NSThreadPerformAdditions)

- (void)performSelectorOnMainThread:(SEL)aSelector withObject:(id)arg waitUntilDone:(BOOL)wait;

@end
#endif
