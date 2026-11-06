//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSOrderedSetSetProxy.h"
#include "CoreFoundation/Objective-C/OCFNSArray.h"

// Inferred organization: the __NSOrderedSetSetProxy methods lie together in the iOS Simulator __text section
// (0x1c02e55dc-0x1c02e5727), after the __NSOrderedSetArrayProxy methods; following the owner's decision, the
// class has its own translation unit. The methods follow in native address order. Objective-C compiled with
// manual reference counting (objc_retain, objc_release). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json (family K-mutable-ordered-set). Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The NSSet class symbol (class reference off_2227CE3B8), sent +allocWithZone: by -copyWithZone:.
extern char setProxySetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSSet));

// The messages the proxy sends to the ordered set; declared for the sends only.
@protocol OrderedSetSetProxyMessages

- (NSUInteger)indexOfObject:(id)object;
- (id)objectAtIndex:(NSUInteger)index;

@end

@implementation __ONSOrderedSetSetProxy

// Native -[__NSOrderedSetSetProxy initWithOrderedSet:], 0x1c02e55dc. Non-external. No super initializer:
// _orderedSet takes the retained ordered set (objc_retain); the receiver is answered. The native init reaches
// no designated initializer of super, so that diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

- (id)initWithOrderedSet:(id)orderedSet {
    _orderedSet = [orderedSet retain];
    return self;
}

#pragma clang diagnostic pop

// Native -[__NSOrderedSetSetProxy dealloc], 0x1c02e560c. Non-external. _orderedSet released, then super.
- (void)dealloc {
    [_orderedSet release];
    [super dealloc];
}

// Native -[__NSOrderedSetSetProxy count], 0x1c02e5688. Non-external. The ordered set's -count (a tail call).
- (NSUInteger)count {
    return [_orderedSet count];
}

// Native -[__NSOrderedSetSetProxy member:], 0x1c02e5698. Non-external. The ordered set's -indexOfObject:;
// NSNotFound answers nil, any other index the ordered set's -objectAtIndex: of it (_orderedSet reread; a tail
// call).
- (id)member:(id)object {
    NSUInteger index = [(id<OrderedSetSetProxyMessages>)_orderedSet indexOfObject:object];
    if (index == kNotFound) {
        return nil;
    }
    return [(id<OrderedSetSetProxyMessages>)_orderedSet objectAtIndex:index];
}

// Native -[__NSOrderedSetSetProxy objectEnumerator], 0x1c02e56e8. Non-external. The ordered set's
// -objectEnumerator (a tail call).
- (id)objectEnumerator {
    return [_orderedSet objectEnumerator];
}

// Native -[__NSOrderedSetSetProxy copyWithZone:], 0x1c02e56f8. Non-external. +[NSSet allocWithZone:] with the
// zone, then -initWithSet:copyItems: of the receiver without copying items (a tail call).
- (id)copyWithZone:(struct _NSZone *)zone {
    return [[(id)setProxySetClass allocWithZone:zone] initWithSet:self copyItems:NO];
}

@end
