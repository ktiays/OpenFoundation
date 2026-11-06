//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSConstantDate.h"

// Inferred organization: the NSConstantDate methods of this image (-timeIntervalSinceReferenceDate
// 0x1c02f2f10 through +automaticallyNotifiesObserversForKey: 0x1c02f2f64, with -init and -dealloc in the cold
// region at 0x1c03171b0 and 0x1c03171b4) lie together in the iOS Simulator __text section among the constant
// collection classes; following the owner's decision, the class has its own translation unit. Objective-C,
// manual reference counting (the unit's MRC is inherited from the other Objective-C units; these methods do
// not establish it). The native source path is not in the evidence. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full);
// iOS Device 24A446: unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.

@implementation ONSConstantDate

// Native -[NSConstantDate timeIntervalSinceReferenceDate], 0x1c02f2f10. Non-external. The _ti ivar (through
// its offset symbol).
- (double)timeIntervalSinceReferenceDate {
    return _ti;
}

// Native +[NSConstantDate new], 0x1c02f2f20. Non-external. objc_opt_new of the NSDate class reference (a tail
// call), not of the receiver.
+ (id)new {
    return (id)[ONSDate new];
}

// Native +[NSConstantDate allocWithZone:], 0x1c02f2f2c. Non-external. +allocWithZone: of the NSDate class
// reference (a tail call with the zone).
+ (id)allocWithZone:(struct _NSZone *)zone {
    return (id)[ONSDate allocWithZone:zone];
}

// Native -[NSConstantDate autorelease], 0x1c02f2f38. Non-external. The receiver (RET).
- (instancetype)autorelease {
    return self;
}

// Native -[NSConstantDate retain], 0x1c02f2f3c. Non-external. The receiver (RET).
- (instancetype)retain {
    return self;
}

// Native -[NSConstantDate _tryRetain], 0x1c02f2f40. Non-external. YES.
- (BOOL)_tryRetain {
    return YES;
}

// Native -[NSConstantDate _isDeallocating], 0x1c02f2f48. Non-external. NO.
- (BOOL)_isDeallocating {
    return NO;
}

// Native -[NSConstantDate retainCount], 0x1c02f2f50. Non-external.
- (NSUInteger)retainCount {
    return UINTPTR_MAX;
}

// Native -[NSConstantDate release], 0x1c02f2f58. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[NSConstantDate copyWithZone:], 0x1c02f2f5c. Non-external. The receiver, not retained (RET).
- (id)copyWithZone:(struct _NSZone *)zone {
    return self;
}

// Native -[NSConstantDate copy], 0x1c02f2f60. Non-external. The receiver, not retained (RET).
- (id)copy {
    return self;
}

// Native +[NSConstantDate automaticallyNotifiesObserversForKey:], 0x1c02f2f64. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[NSConstantDate init], 0x1c03171b0 (cold region). Non-external. A bare BRK #1, never reaching a
// designated initializer, so that diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

- (id)init {
    __builtin_trap();
}

#pragma clang diagnostic pop

// Native -[NSConstantDate dealloc], 0x1c03171b4 (cold region). Non-external. A bare BRK #1; super is never
// reached, so the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
    __builtin_trap();
}

#pragma clang diagnostic pop

@end
