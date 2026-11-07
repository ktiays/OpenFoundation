//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSOrderedSetArrayProxy.h"

// Inferred organization: the __NSOrderedSetArrayProxy methods lie together in the iOS Simulator __text
// section (0x1c02e54c8-0x1c02e55db), followed by the __NSOrderedSetSetProxy methods; following the owner's
// decision, the class has its own translation unit. The methods follow in native address order.
// Objective-C compiled with manual reference counting (objc_retain, objc_release). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json (family K-mutable-ordered-set). Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The NSArray class symbol (class reference off_2227CE2A0), sent +allocWithZone: by -copyWithZone:.
extern char arrayProxyArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSArray));

@implementation __ONSOrderedSetArrayProxy

// Native -[__NSOrderedSetArrayProxy initWithOrderedSet:], 0x1c02e54c8. Non-external. No super initializer:
// _orderedSet takes the retained ordered set (objc_retain); the receiver is answered. The native init reaches
// no designated initializer of super, so that diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

- (id)initWithOrderedSet:(id)orderedSet {
    _orderedSet = [orderedSet retain];
    return self;
}

#pragma clang diagnostic pop

// Native -[__NSOrderedSetArrayProxy dealloc], 0x1c02e54f8. Non-external. _orderedSet released, then super.
- (void)dealloc {
    [_orderedSet release];
    [super dealloc];
}

// Native -[__NSOrderedSetArrayProxy count], 0x1c02e5574. Non-external. The ordered set's -count (a tail
// call).
- (NSUInteger)count {
    return [_orderedSet count];
}

// Native -[__NSOrderedSetArrayProxy objectAtIndex:], 0x1c02e5584. Non-external. The ordered set's
// -objectAtIndex: (a tail call).
- (id)objectAtIndex:(NSUInteger)index {
    return [_orderedSet objectAtIndex:index];
}

// Native -[__NSOrderedSetArrayProxy copyWithZone:], 0x1c02e5594. Non-external. +[NSArray allocWithZone:] with
// the zone, then the receiver's -count, and -initWithArray:range:copyItems: of the receiver over {0, count}
// without copying items (a tail call).
- (id)copyWithZone:(struct _NSZone *)zone {
    id array = [(id)arrayProxyArrayClass allocWithZone:zone];
    NSUInteger count = [self count];
    return [array initWithArray:self range:(OCFRange){0, (OCFIndex)count} copyItems:NO];
}

@end
