//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>
#include <pthread.h>

#import <objc/NSObject.h>

@class ONSString;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} LockCondition;

// Native NSLock, 0x22e6db0e8, external. class_ro_t 0x222857f90, instance start/size 8/88.
// Its _priv ivar has mutex, condition pointer and name at instance offsets 8, 72 and 80.
// The root superclass is bound to libobjc's NSObject, 0x22e6b6088.
__attribute__((visibility("default")))
@interface ONSLock : NSObject {
    @private
    struct {
        pthread_mutex_t m;
        LockCondition *cd;
        ONSString *n;
    } _priv;
}
- (void)lock;
- (void)unlock;
- (void)setName:(id)name;
- (id)name;
@end

// Native NSRecursiveLock, 0x22e6db188, external. class_ro_t 0x2228581d0, instance start/size 8/104.
// Its _priv ivar has mutex, thread, count, condition and name at instance offsets 8, 72, 80, 88 and 96.
__attribute__((visibility("default")))
@interface ONSRecursiveLock : NSObject {
    @private
    struct {
        pthread_mutex_t m;
        pthread_t t;
        NSUInteger cnt;
        LockCondition *cd;
        ONSString *n;
    } _priv;
}
- (void)lock;
- (void)unlock;
- (void)setName:(id)name;
- (id)name;
@end
