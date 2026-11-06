//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSOrderedSetReverseEnumerator.h"

// Inferred organization: the __NSOrderedSetReverseEnumerator methods lie together in the iOS Simulator
// __text section (0x1c01d33b8-0x1c01d34db), followed by the __NSOrderedSetReversed methods; following the
// owner's decision, the class has its own translation unit. The methods follow in native address order.
// Objective-C compiled with manual reference counting (objc_retain, objc_release). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json (family K-mutable-ordered-set). Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The messages the enumerator sends to the ordered set; declared for the sends only.
@protocol OrderedSetReverseEnumeratorMessages

- (NSUInteger)count;
- (id)objectAtIndex:(NSUInteger)index;

@end

@implementation __ONSOrderedSetReverseEnumerator

// Native -[__NSOrderedSetReverseEnumerator initWithObject:], 0x1c01d33b8. Non-external. No super initializer
// (unlike the array enumerator): _obj takes the retained ordered set (objc_retain), _idx its -count; the
// receiver is answered. The native init reaches no designated initializer of super, so that diagnostic is
// silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

- (id)initWithObject:(id)object {
    _obj = [object retain];
    _idx = [(id<OrderedSetReverseEnumeratorMessages>)object count];
    return self;
}

#pragma clang diagnostic pop

// Native -[__NSOrderedSetReverseEnumerator dealloc], 0x1c01d3400. Non-external. _obj released, then super's
// -dealloc (selector reference 0x2387a9f83).
- (void)dealloc {
    [_obj release];
    [super dealloc];
}

// Native -[__NSOrderedSetReverseEnumerator nextObject], 0x1c01d347c. Non-external. Without an ordered set
// nil; an exhausted index releases the ordered set, clears _obj and answers nil; otherwise _idx steps back by
// one and the ordered set's -objectAtIndex: of it is answered (_obj reread; a tail call).
- (id)nextObject {
    if (_obj == nil) {
        return nil;
    }
    NSUInteger index = _idx;
    if (index == 0) {
        [_obj release];
        _obj = nil;
        return nil;
    }
    _idx = index - 1;
    return [(id<OrderedSetReverseEnumeratorMessages>)_obj objectAtIndex:index - 1];
}

@end
