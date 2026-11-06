//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSSingleObjectEnumerator.h"

#import "CoreFoundation/Runtime/OCFRuntime.h"
#import "CoreFoundation/String/OCFString.h"
#import "CoreFoundation/String/OCFStringFormat.h"

// Inferred organization: the __NSSingleObjectEnumerator methods (-initWithObject:collection: 0x1c02875c0
// through -description 0x1c0287738, with -init in the cold region at 0x1c030f254) lie in the iOS Simulator
// __text run of the NSEnumerator unit (NSEnumerator.m by the class names); following the owner's decision,
// the class has its own translation unit. Objective-C compiled with manual reference counting (objc_retain,
// objc_release, objc_autorelease). The native source path is not in the evidence. Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read in full; constant strings resolved from the ADRP/ADD encodings); iOS Device 24A446: unread. Reading:
// not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.
//
// The super sends pass the selector addresses 0x2387aa0ea (init), 0x2387a9f83 (dealloc) and 0x2387aa04d
// (description).

@implementation __ONSSingleObjectEnumerator

// Native -[__NSSingleObjectEnumerator initWithObject:collection:], 0x1c02875c0. Non-external.
// A nil object answers -init (a tail call, which crashes). Otherwise super's -init; on success the object
// and then the collection are retained into _theObjectToReturn and _theCollection.
- (id)initWithObject:(id)object collection:(id)collection {
    if (object == nil) {
        return [self init];
    }
    self = [super init];
    if (self != nil) {
        _theObjectToReturn = [object retain];
        _theCollection = [collection retain];
    }
    return self;
}

// Native -[__NSSingleObjectEnumerator dealloc], 0x1c0287684. Non-external. The object, then the collection,
// released; super's -dealloc.
- (void)dealloc {
    [_theObjectToReturn release];
    [_theCollection release];
    [super dealloc];
}

// Native -[__NSSingleObjectEnumerator nextObject], 0x1c0287700. Non-external. The object, autoreleased (its
// retain is handed over); both ivars are cleared and the collection released.
- (id)nextObject {
    id object = [_theObjectToReturn autorelease];
    id collection = _theCollection;
    _theCollection = nil;
    _theObjectToReturn = nil;
    [collection release];
    return object;
}

// Native -[__NSSingleObjectEnumerator description], 0x1c0287738. Non-external.
// A new mutable string (kCFAllocatorDefault, no maximum length) gets "%@ that will yield %@" with super's
// -description and the object while there is one, else "%@ that yielded its object" with super's
// -description; it is autoreleased (CFAutorelease).
- (id)description {
    OCFMutableStringRef description = OCFStringCreateMutable(kOCFAllocatorDefault, 0);
    if (_theObjectToReturn != nil) {
        id superDescription = [super description];
        OCFStringAppendFormat(
            description, NULL, OF_STR("%@ that will yield %@"), superDescription, _theObjectToReturn
        );
    } else {
        OCFStringAppendFormat(description, NULL, OF_STR("%@ that yielded its object"), [super description]);
    }
    return (id)OCFAutorelease(description);
}

// Native -[__NSSingleObjectEnumerator init], 0x1c030f254 (cold region). Non-external. Crashes "Attempt to
// create a single-object enumerator with no object/collection" (stored, then BRK #1); no designated
// initializer is reached, so that diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

- (id)init {
    crashWithMessage("Attempt to create a single-object enumerator with no object/collection");
}

#pragma clang diagnostic pop

@end
