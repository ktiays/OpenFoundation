//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFTimer.h"

#include <malloc/malloc.h>
#include <stdlib.h>

// Inferred organization: the __NSCFTimer methods (0x1c01b4a34-0x1c01b4f54) lie together in the iOS Simulator
// __text section with __CFFireTimer (0x1c01b4bb8), __CFDescribeTimer (0x1c01b4c0c), _timerRetain and
// _timerRelease; __CFFireTimer and __CFDescribeTimer belong to the run-loop restoration's list; the two
// callbacks, restored from the IDA capture (iphone-simulator-corpus-delta2.json), are defined here in native
// order. Following the owner's
// decision, the class has its own translation unit. Objective-C compiled with manual reference counting
// (objc_retain and objc_release around -fire, retained target and user info). Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The NSDate class symbol, loaded natively through its class reference (off_2227CE2D0).
extern char dateClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDate));

// Native _timerRetain and _timerRelease; defined below, in native address order.
static const void *_timerRetain(const void *info);
static void _timerRelease(const void *info);

@implementation __ONSCFTimer

// Native -[__NSCFTimer isEqual:], 0x1c01b4a34. Non-external. nil answers NO and the receiver itself YES
// before _CFNonObjCEqual.
- (BOOL)isEqual:(id)object {
    if (object == nil) {
        return NO;
    }
    if (self == object) {
        return YES;
    }
    return _OCFNonObjCEqual(self, object) != 0;
}

// Native -[__NSCFTimer hash], 0x1c01b4a70. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[__NSCFTimer retain], 0x1c01b4a74. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFTimer release], 0x1c01b4a78. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFTimer _tryRetain], 0x1c01b4a7c. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFTimer _isDeallocating], 0x1c01b4a98. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFTimer retainCount], 0x1c01b4ab4. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native +[__NSCFTimer automaticallyNotifiesObserversForKey:], 0x1c01b4ab8. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native +[__NSCFTimer allocWithZone:], 0x1c01b4ac0. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native -[__NSCFTimer isValid], 0x1c01b4adc. Non-external.
- (BOOL)isValid {
    return OCFRunLoopTimerIsValid((OCFRunLoopTimerRef)self) != 0;
}

// Native -[__NSCFTimer timeInterval], 0x1c01b4af8. Non-external.
- (double)timeInterval {
    return OCFRunLoopTimerGetInterval((OCFRunLoopTimerRef)self);
}

// Native -[__NSCFTimer tolerance], 0x1c01b4afc. Non-external.
- (double)tolerance {
    return OCFRunLoopTimerGetTolerance((OCFRunLoopTimerRef)self);
}

// Native -[__NSCFTimer setTolerance:], 0x1c01b4b00. Non-external.
- (void)setTolerance:(double)tolerance {
    OCFRunLoopTimerSetTolerance((OCFRunLoopTimerRef)self, tolerance);
}

// Native -[__NSCFTimer invalidate], 0x1c01b4b04. Non-external.
- (void)invalidate {
    OCFRunLoopTimerInvalidate((OCFRunLoopTimerRef)self);
}

// Native -[__NSCFTimer fireDate], 0x1c01b4b08. Non-external. The NSDate class reference is loaded before the
// fire date is read.
- (id)fireDate {
    id cls = (id)dateClass;
    double fireDate = OCFRunLoopTimerGetNextFireDate((OCFRunLoopTimerRef)self);
    return [cls dateWithTimeIntervalSinceReferenceDate:fireDate];
}

// Native -[__NSCFTimer setFireDate:], 0x1c01b4b30. Non-external.
- (void)setFireDate:(id)date {
    double fireDate = [date timeIntervalSinceReferenceDate];
    OCFRunLoopTimerSetNextFireDate((OCFRunLoopTimerRef)self, fireDate);
}

// Native -[__NSCFTimer userInfo], 0x1c01b4b58. Non-external. The user info of the context's info record,
// unretained.
- (id)userInfo {
    RunLoopTimerContext context = {0, NULL, NULL, NULL, NULL};
    OCFRunLoopTimerGetContext((OCFRunLoopTimerRef)self, &context);
    return ((TimerInfo *)context.info)->userInfo;
}

// Native -[__NSCFTimer fire], 0x1c01b4d3c. Non-external. A valid timer is retained, fired through
// __CFFireTimer with its context's info, invalidated when its interval is 0.0, and released.
- (void)fire {
    if (![self isValid]) {
        return;
    }
    RunLoopTimerContext context = {0, NULL, NULL, NULL, NULL};
    [self retain];
    OCFRunLoopTimerGetContext((OCFRunLoopTimerRef)self, &context);
    __CFFireTimer((OCFRunLoopTimerRef)self, context.info);
    if ([self timeInterval] == 0.0) {
        [self invalidate];
    }
    [self release];
}

// Native -[__NSCFTimer _cfTypeID], 0x1c01b4ddc. Non-external.
- (OCFTypeID)_cfTypeID {
    return OCFRunLoopTimerGetTypeID();
}

// Native -[__NSCFTimer initWithFireDate:interval:target:selector:userInfo:repeats:], 0x1c01b4de0.
// Non-external. The receiver is ignored (not released). The info record is calloc'd in the default zone with
// a cleared count, the retained target, the selector and the retained user info; the context carries
// _timerRetain and _timerRelease. A non-positive (or NaN) interval becomes 0.0001 (qword 0x1c0375818); a
// timer that does not repeat gets interval 0.0. The new CFRunLoopTimer, firing __CFFireTimer, is answered.
- (id)initWithFireDate:(id)date
              interval:(double)interval
                target:(id)target
              selector:(SEL)selector
              userInfo:(id)userInfo
               repeats:(BOOL)repeats {
    TimerInfo *info = malloc_type_zone_calloc(malloc_default_zone(), 1, 0x20, 0x10800406BDE4C18);
    RunLoopTimerContext context = {0, info, _timerRetain, _timerRelease, NULL};
    double positiveInterval = interval > 0.0 ? interval : 0.0001;
    info->count = 0;
    info->target = [target retain];
    info->selector = selector;
    info->userInfo = [userInfo retain];
    double fireDate = [date timeIntervalSinceReferenceDate];
    return (id)OCFRunLoopTimerCreate(
        NULL, fireDate, repeats ? positiveInterval : 0.0, 0, 0, __CFFireTimer, &context
    );
}

// Native _timerRetain, 0x1c01b4ef0. Non-external. Atomically increments the 32-bit count (LDADDAL) and
// answers the record.
static const void *_timerRetain(const void *info) {
    __atomic_fetch_add(&((TimerInfo *)info)->count, 1, __ATOMIC_SEQ_CST);
    return info;
}

// Native _timerRelease, 0x1c01b4efc. Non-external. Atomically decrements the count; when the previous value
// was at most 1 (signed), the target and the user info are released inside an autorelease pool, the record is
// freed, and the pool popped (a tail call).
static void _timerRelease(const void *info) {
    TimerInfo *record = (TimerInfo *)info;
    if ((int32_t)__atomic_fetch_add(&record->count, (uint32_t)-1, __ATOMIC_SEQ_CST) > 1) {
        return;
    }
    void *pool = _OCFAutoreleasePoolPush();
    [record->target release];
    [record->userInfo release];
    free(record);
    _OCFAutoreleasePoolPop(pool);
}

// Native -[__NSCFTimer init], 0x1c01b4f50. Non-external. Answers nil without releasing the receiver.
- (id)init {
    return nil;
}

@end
