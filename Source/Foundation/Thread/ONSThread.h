//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "Foundation/Base/ONSFoundationSupport.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

@class ONSMutableDictionary;

// Foundation's _NSCallStackArray (__DATA,__objc_data 0x22e6dd938, non-external) and NSNotificationCenter (the
// GOT slot 0x22289f148); the full declarations are in their owner headers.
@class _ONSCallStackArray;
@class _ONSMainThread;
@class ONSNotificationCenter;

// Foundation's _NSThreadData (__DATA,__objc_data 0x22e6dd9d8, non-external), NSThread's private state, whose
// ivars the restored NSThread methods read and write directly. class_ro_t 0x2228608c0: instance start 8, size
// 160; ivars _target (8, "@"), _selector (16, ":"), _argument (24, "@"), dict (32, "@"), name (40, "@"),
// seqNum (48, "i"), qstate (52, "C"), qos (53, "c"), cancel (54, "C"), status (55, "AC", an atomic unsigned
// char), performQ (56, "@"), performD (64,
// "@\"NSMutableDictionary\""), attr (72, pthread_attr_t), tid (136, pthread_t), pri (144, "d") and defpri
// (152, "d") (iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence:
// verified). Its superclass (0x22e6b6088) lies outside the image; NSObject (confidence: medium).
@interface _ONSThreadData : NSObject {
    @public
    id _target;
    SEL _selector;
    id _argument;
    id dict;
    id name;
    int seqNum;
    unsigned char qstate;
    char qos;
    unsigned char cancel;
    _Atomic(unsigned char) status;
    id performQ;
    ONSMutableDictionary *performD;
    pthread_attr_t attr;
    pthread_t tid;
    double pri;
    double defpri;
}
@end

// Native class NSThread (__DATA,__objc_data 0x22e6dda78, external); class_ro_t 0x222860b18: instance start 8,
// size 60; ivars _private (offset 8, "@") and _bytes (16, "[44C]") (iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified). The restored methods read _private as the
// thread's _NSThreadData. Its superclass (0x22e6b6088) lies outside the image; NSObject (confidence: medium).
__attribute__((visibility("default")))
// Native ivar-offset symbols are non-external in nm. @package preserves hidden linkage while allowing
// same-image helpers to use the read layout; the original source access-specifier spelling is unread.
@interface ONSThread : NSObject {
    @package
    id _private;
    uint8_t _bytes[44];
}

+ (BOOL)isMainThread;
+ (id)callStackSymbols;
+ (id)currentThread;
+ (id)mainThread;
- (id)initWithBlock:(void (^)(void))block;
- (id)threadDictionary;
- (void)setName:(id)value;
- (id)name;
- (BOOL)isMainThread;
- (id)initWithTarget:(id)target selector:(SEL)selector object:(id)argument;

@end
// Native _NSMainThread, 0x22e6ddaa0, non-external. class_ro_t 0x222860950:
// instance start and size 60, no added ivars.
@interface _ONSMainThread : ONSThread
@end

#endif

// Native _NSIsMultiThreaded (__DATA,__common 0x22e775b48, was a private external): set once a thread other
// than the main thread asks for +currentThread or +mainThread. Defined by Thread/ONSThread.m.
OF_EXTERN _Atomic(uint8_t) _NSIsMultiThreaded;

// Native NSWillBecomeMultiThreadedNotification, 0x2227d77b0. External.
OF_EXPORT id const ONSWillBecomeMultiThreadedNotification;
