//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Base/ONSInitializePlatform.h"
#import "Foundation/Collections/ONSHashTable.h"
#import "Foundation/Collections/ONSMapTable.h"
#import "Foundation/Collections/ONSPointerFunctions.h"
#import "Foundation/Lock/ONSLock.h"
#import "Foundation/Notification/ONSNotificationCenter.h"

#include <OpenFoundation/Base/ONSObjCRuntime.h>

#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/PlugIn/OCFBundleResources.h"
#include "CoreFoundation/PlugIn/OCFBundleStrings.h"
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Notification/OCFNotificationCenter.h"

#ifdef __OBJC__
#import <objc/NSObject.h>
#import "CoreFoundation/Objective-C/OCFNSMutableArray.h"

// Mutable-array selectors supplied by the CoreFoundation collection family.
@interface ONSMutableArray (BundlePayloadMessages)
- (id)initWithCapacity:(ONSUInteger)capacity;
- (void)addObject:(id)object;
@end

// FO-notifications-locks-json-coders.

@class _NotificationCenterActorQueueManager;

// Native class 0x22e6db908, class_ro_t 0x222859888. Instance start/size 8/32.
// Replace this temporary interface with the FO class header at integration.
__attribute__((visibility("default")))

@interface ONSNotificationCenter(BundleMethods)
+ (id)defaultCenter;
- (void)postNotification:(id)notification;
@end

@protocol BundleClassMessages
- (id)bundleForClass;
- (id)initFileURLWithPath:(id)path isDirectory:(BOOL)isDirectory;
@end

#endif
