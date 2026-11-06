//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Notification/ONSConcreteNotification.h"

#include "CoreFoundation/Objective-C/OCFNSObject.h"

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

@implementation ONSConcreteNotification

// Native -[NSConcreteNotification name], 0x1c0572318. Non-external.
- (id)name {
    return objc_autorelease(objc_retain(name));
}

// Native -[NSConcreteNotification object], 0x1c0572330. Non-external.
- (id)object {
    return object;
}

// Native -[NSConcreteNotification userInfo], 0x1c0572338. Non-external.
- (id)userInfo {
    return objc_autorelease(objc_retain(userInfo));
}

// Native -[NSConcreteNotification initWithName:object:userInfo:], 0x1c0572350. Non-external.
- (id)initWithName:(id)aName object:(id)anObject userInfo:(id)info {
    if (aName == nil) {
        objc_release(self);
        return nil;
    }
    name = [aName copy];
    object = objc_retain(anObject);
    userInfo = objc_retain(info);
    return self;
}

// Native -[NSConcreteNotification dealloc], 0x1c05723ac. Non-external.
- (void)dealloc {
    objc_release(name);
    objc_release(object);
    objc_release(userInfo);
    [super dealloc];
}

@end
