//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSNull.h"

// Inferred organization: the NSNull methods (0x1c0291d64-0x1c0291dc8) lie together in the iOS Simulator
// __text section, followed by -[NSMutableArray _mutate]; following the owner's decision, each bridged class
// has its own translation unit. The unit is Objective-C compiled with manual reference counting (it overrides
// -retain, -release, -autorelease and -dealloc). Unless a definition says otherwise: iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device
// 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written
// yet).

@implementation ONSNull

// Native -[NSNull _cfTypeID], 0x1c0291d64. Non-external.
- (OCFTypeID)_cfTypeID {
    return OCFNullGetTypeID();
}

// Native +[NSNull allocWithZone:], 0x1c0291d68. Non-external. Answers kCFNull.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return (id)kOCFNull;
}

// Native +[NSNull null], 0x1c0291d78. Non-external. Answers kCFNull.
+ (id)null {
    return (id)kOCFNull;
}

// Native -[NSNull retain], 0x1c0291d88. Non-external. Answers the receiver.
- (id)retain {
    return self;
}

// Native -[NSNull release], 0x1c0291d8c. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[NSNull autorelease], 0x1c0291d90. Non-external. Answers the receiver.
- (id)autorelease {
    return self;
}

// Native -[NSNull retainCount], 0x1c0291d94. Non-external. Answers NSUIntegerMax.
- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

// Native -[NSNull dealloc], 0x1c0291d9c. Non-external. Returns at once; the native method does not call
// super's -dealloc, so the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
}

#pragma clang diagnostic pop

// Native -[NSNull copyWithZone:], 0x1c0291da0. Non-external. Answers the receiver, unretained.
- (id)copyWithZone:(struct _NSZone *)zone {
    return self;
}

// Native -[NSNull description], 0x1c0291da4. Non-external.
- (id)description {
    return (id)OF_STR("<null>");
}

// Native -[NSNull hash], 0x1c0291db0. Non-external. Answers 0x7c0.
- (NSUInteger)hash {
    return 0x7C0;
}

// Native +[NSNull supportsSecureCoding], 0x1c0291db8. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSNull initWithCoder:], 0x1c0291dc0. Non-external. Answers nil without releasing the receiver
// (well-defined: the receiver is kCFNull, whose release does nothing).
- (id)initWithCoder:(id)coder {
    return nil;
}

// Native -[NSNull encodeWithCoder:], 0x1c0291dc8. Non-external. Does nothing.
- (void)encodeWithCoder:(id)coder {
}

@end
