//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSNotification.h>

#if defined(__OBJC__)
@class NSArray<ObjectType>, NSString, NSDictionary<KeyType, ObjectType>;

@interface NSProcessInfo : NSObject

@property (class, readonly, strong) NSProcessInfo *processInfo;
@property (readonly, copy) NSDictionary<NSString *, NSString *> *environment;
@property (readonly, copy) NSArray<NSString *> *arguments;
@property (readonly, copy) NSString *processName;
@property (readonly, getter=isLowPowerModeEnabled) BOOL lowPowerModeEnabled;

@end

FOUNDATION_EXPORT NSNotificationName const NSProcessInfoThermalStateDidChangeNotification;
FOUNDATION_EXPORT NSNotificationName const NSProcessInfoPowerStateDidChangeNotification;
#endif
