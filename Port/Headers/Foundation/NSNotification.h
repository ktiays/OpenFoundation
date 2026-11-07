//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>

#if defined(__OBJC__)
@class NSString, NSDictionary<KeyType, ObjectType>, NSOperationQueue;

typedef NSString *NSNotificationName NS_TYPED_EXTENSIBLE_ENUM;

@interface NSNotification : NSObject <NSCopying, NSCoding>

@property (readonly, copy) NSNotificationName name;
@property (readonly, retain) id object;
@property (readonly, copy) NSDictionary *userInfo;
- (instancetype)initWithName:(NSNotificationName)name object:(id)object userInfo:(NSDictionary *)userInfo NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithCoder:(NSCoder *)coder NS_DESIGNATED_INITIALIZER;

@end

@interface NSNotification (NSNotificationCreation)

+ (instancetype)notificationWithName:(NSNotificationName)aName object:(id)anObject userInfo:(NSDictionary *)aUserInfo;

@end

@interface NSNotificationCenter : NSObject

@property (class, readonly, strong) NSNotificationCenter *defaultCenter;
- (void)addObserver:(id)observer selector:(SEL)aSelector name:(NSNotificationName)aName object:(id)anObject;
- (void)postNotification:(NSNotification *)notification;
- (void)postNotificationName:(NSNotificationName)aName object:(id)anObject;
- (void)postNotificationName:(NSNotificationName)aName object:(id)anObject userInfo:(NSDictionary *)aUserInfo;
- (void)removeObserver:(id)observer;
- (void)removeObserver:(id)observer name:(NSNotificationName)aName object:(id)anObject;

@end
#endif
