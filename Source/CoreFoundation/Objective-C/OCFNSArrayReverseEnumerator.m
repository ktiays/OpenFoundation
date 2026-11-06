//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSArrayReverseEnumerator.h"

// Inferred organization: the __NSArrayReverseEnumerator methods lie together in the iOS Simulator __text
// section (0x1c028eebc-0x1c028f033), after the __NSArrayReversed methods; following the owner's decision, the
// class has its own translation unit. The methods follow in native address order. Objective-C compiled with
// manual reference counting (objc_retain, objc_release). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json (family H-array-concrete). Unless a definition
// says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The messages the enumerator sends to the array; declared for the sends only.
@protocol ArrayReverseEnumeratorMessages

- (NSUInteger)count;
- (id)objectAtIndex:(NSUInteger)index;

@end

@implementation __ONSArrayReverseEnumerator

// Native -[__NSArrayReverseEnumerator initWithObject:], 0x1c028eebc. Non-external. super's -init (selector
// reference 0x2387aa0ea); a non-nil result retains the array into _obj and takes its -count into _idx.
- (id)initWithObject:(id)object {
    self = [super init];
    if (self != nil) {
        _obj = [object retain];
        _idx = [object count];
    }
    return self;
}

// Native -[__NSArrayReverseEnumerator dealloc], 0x1c028ef58. Non-external. _obj released, then super's
// -dealloc (selector reference 0x2387a9f83).
- (void)dealloc {
    [_obj release];
    [super dealloc];
}

// Native -[__NSArrayReverseEnumerator nextObject], 0x1c028efd4. Non-external. Without an array nil; an
// exhausted index releases the array, clears _obj and answers nil; otherwise _idx steps back by one and the
// array's -objectAtIndex: of it is answered (a tail call).
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
    return [_obj objectAtIndex:index - 1];
}

@end
