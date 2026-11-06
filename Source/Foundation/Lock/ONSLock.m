//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Lock/ONSLock.h"

#import "Foundation/String/ONSString.h"
#import "Foundation/String/ONSAllocateObject.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <dispatch/dispatch.h>
#include <stdlib.h>

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

extern void objc_release(id object);

_Static_assert(offsetof(LockCondition, condition) == 64, "The condition follows the native mutex.");

static dispatch_once_t _MergedGlobals_96;
static pthread_mutexattr_t stru_22E76C678;

@implementation ONSLock

// Native -[NSLock init], 0x1c0566914. Non-external.
- (id)init {
    if (pthread_mutex_init(&_priv.m, NULL) != 0) {
        [super dealloc];
        return nil;
    }
    _priv.n = nil;
    return self;
}

// Native -[NSLock dealloc], 0x1c05669a0. Non-external.
- (void)dealloc {
    pthread_mutex_destroy(&_priv.m);
    LockCondition *condition = _priv.cd;
    if (condition != NULL) {
        pthread_cond_destroy(&condition->condition);
        pthread_mutex_destroy(&condition->mutex);
        free(condition);
    }
    objc_release(_priv.n);
    [super dealloc];
}

// Native -[NSLock lock], 0x1c0566a3c. Non-external.
- (void)lock {
    pthread_mutex_lock(&_priv.m);
}

// Native -[NSLock unlock], 0x1c0566be4. Non-external.
- (void)unlock {
    pthread_mutex_unlock(&_priv.m);
    LockCondition *condition = _priv.cd;
    if (condition != NULL) {
        pthread_mutex_lock(&condition->mutex);
        pthread_cond_broadcast(&condition->condition);
        pthread_mutex_unlock(&condition->mutex);
    }
}

// Native -[NSLock description], 0x1c0566c30. Non-external.
- (id)description {
    id description = [super description];
    id name = _priv.n;
    const char *quote = "'";
    if (name == nil) {
        name = (id)OF_STR("nil");
        quote = "";
    }
    return [ONSString stringWithFormat:(id)OF_STR("%@{name = %s%@%s}"), description, quote, name, quote];
}

// Native -[NSLock setName:], 0x1c0566ce4. Non-external.
- (void)setName:(id)name {
    if (name != _priv.n) {
        objc_release(_priv.n);
        _priv.n = [name copy];
    }
}

// Native -[NSLock name], 0x1c0566d20. Non-external.
- (id)name {
    return _priv.n;
}

@end

@implementation ONSRecursiveLock

// Native +[NSRecursiveLock allocWithZone:], 0x1c0567180. Non-external.
// Native allocation requests 0x60 extra bytes beyond the class instance size (kept).
+ (id)allocWithZone:(struct _NSZone *)zone {
    return ONSAllocateObject(self, 0x60, zone);
}

// Native -[NSRecursiveLock init], 0x1c0567188. Non-external.
- (id)init {
    // Native ___23-[NSRecursiveLock init]_block_invoke, 0x1c0567244. Non-external.
    dispatch_once(&_MergedGlobals_96, ^{
      pthread_mutexattr_init(&stru_22E76C678);
      pthread_mutexattr_settype(&stru_22E76C678, PTHREAD_MUTEX_RECURSIVE);
    });
    if (pthread_mutex_init(&_priv.m, &stru_22E76C678) != 0) {
        [super dealloc];
        return nil;
    }
    _priv.n = nil;
    _priv.t = NULL;
    _priv.cnt = 0;
    return self;
}

// Native -[NSRecursiveLock dealloc], 0x1c0567274. Non-external.
- (void)dealloc {
    pthread_mutex_destroy(&_priv.m);
    LockCondition *condition = _priv.cd;
    if (condition != NULL) {
        pthread_cond_destroy(&condition->condition);
        pthread_mutex_destroy(&condition->mutex);
        free(condition);
    }
    objc_release(_priv.n);
    [super dealloc];
}

// Native -[NSRecursiveLock lock], 0x1c0567310. Non-external.
- (void)lock {
    pthread_mutex_lock(&_priv.m);
    _priv.t = pthread_self();
    ++_priv.cnt;
}

// Native -[NSRecursiveLock unlock], 0x1c0567518. Non-external.
- (void)unlock {
    if (--_priv.cnt == 0) {
        _priv.t = NULL;
    }
    pthread_mutex_unlock(&_priv.m);
    LockCondition *condition = _priv.cd;
    if (condition != NULL) {
        pthread_mutex_lock(&condition->mutex);
        pthread_cond_broadcast(&condition->condition);
        pthread_mutex_unlock(&condition->mutex);
    }
}

// Native -[NSRecursiveLock description], 0x1c0567578. Non-external.
- (id)description {
    id description = [super description];
    NSUInteger count = _priv.cnt;
    id name = _priv.n;
    const char *quote = "'";
    if (name == nil) {
        name = (id)OF_STR("nil");
        quote = "";
    }
    return [ONSString stringWithFormat:(id)OF_STR("%@{recursion count = %ld, name = %s%@%s}"),
                                       description,
                                       (NSInteger)count,
                                       quote,
                                       name,
                                       quote];
}

// Native -[NSRecursiveLock setName:], 0x1c0567634. Non-external.
- (void)setName:(id)name {
    if (name != _priv.n) {
        objc_release(_priv.n);
        _priv.n = [name copy];
    }
}

// Native -[NSRecursiveLock name], 0x1c0567670. Non-external.
- (id)name {
    return _priv.n;
}

@end
