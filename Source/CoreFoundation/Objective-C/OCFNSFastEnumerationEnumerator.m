//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSFastEnumerationEnumerator.h"

// Inferred organization: the __NSFastEnumerationEnumerator methods (0x1c02873ec-0x1c0287544) lie together in
// the iOS Simulator __text section after __NSFastEnumerationMutationHandler (0x1c028737c), which
// Objective-C/OCFException.m restores (another family's file); following the owner's decision, the class has
// its own translation unit. Objective-C compiled with manual reference counting (objc_retain, objc_release).
// The native source path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.
//
// The super sends pass the selector addresses 0x2387aa0ea (init) and 0x2387a9f83 (dealloc).

// The fast enumeration message, declared for the send only; no protocol object is emitted.
@protocol FastEnumerationMessages

- (NSUInteger)countByEnumeratingWithState:(FastEnumerationState *)state
                                  objects:(id *)buffer
                                    count:(NSUInteger)len;

@end

@implementation __ONSFastEnumerationEnumerator

// Native +[__NSFastEnumerationEnumerator allocWithZone:], 0x1c02873ec. Non-external. __CFAllocateObject of
// the receiver's class (objc_opt_class) with 0x40 extra bytes (a tail call); the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __CFAllocateObject([self class], 0x40);
}

// Native -[__NSFastEnumerationEnumerator initWithObject:], 0x1c0287404. Non-external. After super's -init,
// the object is retained into _obj and again into _origObj.
- (id)initWithObject:(id)object {
    self = [super init];
    if (self != nil) {
        _obj = [object retain];
        _origObj = [object retain];
    }
    return self;
}

// Native -[__NSFastEnumerationEnumerator nextObject], 0x1c028748c. Non-external.
// When the buffered objects are used up (_index equals _count), the old state word is kept, _index is cleared
// and -countByEnumeratingWithState:objects:count: (16) refills _count. A zero count ends the enumeration.
// After the first call (an old state of zero) the mutations value is recorded in _mut; on later calls a
// different mutations value calls __NSFastEnumerationMutationHandler with _obj and then ends the enumeration.
// Ending releases _obj and _origObj, clears both and answers nil. Otherwise the next item is answered.
- (id)nextObject {
    if (_index == _count) {
        unsigned long oldState = _state.state;
        _index = 0;
        _count = [_obj countByEnumeratingWithState:&_state objects:_objects count:16];
        if (_count == 0) {
            goto end;
        }
        if (oldState == 0) {
            _mut = *_state.mutationsPtr;
        } else if (_mut != *_state.mutationsPtr) {
            __NSFastEnumerationMutationHandler(_obj);
            goto end;
        }
    }
    return _state.itemsPtr[_index++];
end:
    [_obj release];
    _obj = nil;
    [_origObj release];
    _origObj = nil;
    return nil;
}

// Native -[__NSFastEnumerationEnumerator dealloc], 0x1c0287544. Non-external; _obj and _origObj released,
// then super's -dealloc.
- (void)dealloc {
    [_obj release];
    [_origObj release];
    [super dealloc];
}

@end
