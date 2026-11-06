//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Number/ONSWeakObjectValue.h"

#import "CoreFoundation/Objective-C/OCFNSObjectGlue.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "Foundation/Number/ONSUnrestoredInterfaces.h"

#import <objc/objc-exception.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Foundation/Number/Reading.md, round 2. Manual reference counting.

@implementation ONSWeakObjectValue

// Native -[NSWeakObjectValue initWithObject:], 0x1c05f8aa4. Non-external.
- (id)initWithObject:(id)object {
    self = [super init];
    if (self != nil) {
        _value = object;
        _ONSObjectStoreWeak(&_object, object, &_useFallback);
    }
    return self;
}

// Native -[NSWeakObjectValue dealloc], 0x1c05f8b40. Non-external.
- (void)dealloc {
    _ONSObjectStoreWeak(&_object, nil, &_useFallback);
    [super dealloc];
}

// Native -[NSWeakObjectValue isEqualToValue:], 0x1c05f8bc8. Non-external.
- (BOOL)isEqualToValue:(id)value {
    if (self == value) {
        return YES;
    }
    if (value == nil) {
        return NO;
    }
    void *identity = _value;
    return identity == [value nonretainedObjectValue];
}

// Native -[NSWeakObjectValue hash], 0x1c05f8c18. Non-external.
- (NSUInteger)hash {
    return (NSUInteger)_value;
}

// Native -[NSWeakObjectValue objCType], 0x1c05f8c28. Non-external.
- (const char *)objCType {
    return "@";
}

// Native -[NSWeakObjectValue nonretainedObjectValue], 0x1c05f8c34. Non-external.
// The raw pointer is the original identity, even after the weak location clears (kept).
- (id)nonretainedObjectValue {
    return (id)_value;
}

// Native -[NSWeakObjectValue getValue:], 0x1c05f8c44. Non-external.
- (void)getValue:(void *)value {
    *(id *)value = _ONSObjectLoadWeak(&_object, &_useFallback);
}

// Native -[NSWeakObjectValue getValue:size:], 0x1c05f8c80. Non-external.
// The assembly ends the failure path at objc_exception_throw. The next method in the pseudocode is separate.
- (void)getValue:(void *)value size:(NSUInteger)size {
    if (size == sizeof(id)) {
        *(id *)value = _ONSObjectLoadWeak(&_object, &_useFallback);
        return;
    }
    id reason = [ONSString
        stringWithFormat:
            (id)OF_STR("Cannot get value with size %zu. The type encoded as %s is expected to be %zu bytes"),
            size,
            [self objCType],
            sizeof(id)];
    objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                  reason:reason
                                                userInfo:nil]);
}

// Native -[NSWeakObjectValue weakObjectValue], 0x1c05f8d20. Non-external.
- (id)weakObjectValue {
    return _ONSObjectLoadWeak(&_object, &_useFallback);
}

// Native -[NSWeakObjectValue copyWithZone:], 0x1c05f8d3c. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

@end
