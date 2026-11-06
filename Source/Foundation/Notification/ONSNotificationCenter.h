//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>
#include <os/lock.h>

#import <objc/NSObject.h>

#include "CoreFoundation/Notification/OCFNotificationCenter.h"

@class _NotificationCenterActorQueueManager;

// Native NSNotificationCenter, 0x22e6db908, external. class_ro_t 0x222859888:
// instance start/size 8/32, ivars at 8, 16 and 24.
__attribute__((visibility("default")))
@interface ONSNotificationCenter : NSObject {
    @private
    OCFNotificationCenterRef _impl;
    os_unfair_lock actorQueueManagerLock;
    _NotificationCenterActorQueueManager *_actorQueueManager;
}
+ (id)_defaultCenterWithoutCreating;
- (void)addObserver:(id)observer selector:(SEL)selector name:(id)name object:(id)object;
- (void)removeObserver:(id)observer;
- (void)removeObserver:(id)observer name:(id)name object:(id)object;
- (void)postNotificationName:(id)name object:(id)object;
- (void)postNotificationName:(id)name object:(id)object userInfo:(id)userInfo;
- (BOOL)isEmpty;
@end

// Native __NSObserver, 0x22e6db8b8, non-external. class_ro_t 0x222859778:
// instance start/size 8/32, ivars at 8, 16 and 24.
@interface __ONSObserver : NSObject {
    @package
    ONSNotificationCenter *nc;
    uint64_t _token;
    void (^_block)(void);
}
+ (id)observerWithCenter:(ONSNotificationCenter *)center __attribute__((objc_direct));
- (uint64_t)token;
@end
