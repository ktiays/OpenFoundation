//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Thread/ONSThread.h"

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

// Native NSWillBecomeMultiThreadedNotification, 0x2227d77b0. External.
static const OCFConstantString kWillBecomeMultiThreadedNotification = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NS"
    "WillBecomeMultiThreadedNotification",
    sizeof("NS"
           "WillBecomeMultiThreadedNotification") -
        1
};
id const ONSWillBecomeMultiThreadedNotification = (id)&kWillBecomeMultiThreadedNotification;

@implementation _ONSThreadData

// Native -[_NSThreadData init], 0x1c05e2b78. Non-external.
- (id)init {
    self = [super init];
    _selector = NULL;
    _argument = nil;
    _target = nil;
    return self;
}

@end

@implementation _ONSMainThread

// Native -[_NSMainThread retain], 0x1c05e2be0. Non-external. The main thread is never retained: the receiver.
- (id)retain {
    return self;
}

// Native -[_NSMainThread release], 0x1c05e2be4. Non-external.
- (oneway void)release {
}

// Native -[_NSMainThread retainCount], 0x1c05e2be8. Non-external.
- (NSUInteger)retainCount {
    return NSUIntegerMax;
}

// Native -[_NSMainThread _tryRetain], 0x1c05e2bf0. Non-external.
- (BOOL)_tryRetain {
    return YES;
}

// Native -[_NSMainThread _isDeallocating], 0x1c05e2bf8. Non-external.
- (BOOL)_isDeallocating {
    return NO;
}

@end
