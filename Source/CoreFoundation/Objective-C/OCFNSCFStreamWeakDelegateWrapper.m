//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFStreamWeakDelegateWrapper.h"

#import "CoreFoundation/Objective-C/OCFNSObjectGlue.h"

// Inferred organization: the __NSCFStreamWeakDelegateWrapper methods (0x1c01b4f58-0x1c01b505c) lie together
// in the iOS Simulator __text section before the __NSCFInputStream methods that Objective-C/OCFNSCFStream.m
// restores (another family's file); following the owner's decision, the class has its own translation unit.
// Objective-C compiled with manual reference counting. The native source path is not in the evidence. Unless
// a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full); iOS Device 24A446: unread. Reading: not yet written; the evidence is the
// assembly in Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.
//
// The super sends pass the selector addresses 0x2387aa0ea (init) and 0x2387a9f83 (dealloc).

@implementation __ONSCFStreamWeakDelegateWrapper

// Native -[__NSCFStreamWeakDelegateWrapper initWithDelegate:], 0x1c01b4f58. Non-external. After super's
// -init, the delegate is stored with _NSObjectStoreWeak.
- (id)initWithDelegate:(id)delegate {
    self = [super init];
    if (self != nil) {
        _ONSObjectStoreWeak(&_object, delegate, &_useFallback);
    }
    return self;
}

// Native -[__NSCFStreamWeakDelegateWrapper dealloc], 0x1c01b4fe0. Non-external. The location is cleared with
// _NSObjectStoreWeak (nil), then super's -dealloc.
- (void)dealloc {
    _ONSObjectStoreWeak(&_object, nil, &_useFallback);
    [super dealloc];
}

// Native -[__NSCFStreamWeakDelegateWrapper retainedDelegate], 0x1c01b505c. Non-external; a tail call of
// _NSObjectLoadWeakRetained.
- (id)retainedDelegate {
    return _ONSObjectLoadWeakRetained(&_object, &_useFallback);
}

@end
