//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFData.h"
#include "CoreFoundation/Base/OCFUtilities.h"

#include <malloc/malloc.h>
#include <string.h>

// Inferred organization: the __NSCFData methods (0x1c01b4820-0x1c01b4a30) lie together in the iOS Simulator
// __text section, after the __NSCFCharacterSet methods; following the owner's decision, the class has its own
// translation unit. Objective-C, manual reference counting (the bridged-class units'). Unless a definition
// says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

@implementation __ONSCFData

// Native -[__NSCFData isEqual:], 0x1c01b4820. Non-external. nil answers NO and the receiver itself YES before
// _CFNonObjCEqual.
- (BOOL)isEqual:(id)object {
    if (object == nil) {
        return NO;
    }
    if (self == object) {
        return YES;
    }
    return _OCFNonObjCEqual(self, object) != 0;
}

// Native -[__NSCFData hash], 0x1c01b485c. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[__NSCFData retain], 0x1c01b4860. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFData release], 0x1c01b4864. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFData _tryRetain], 0x1c01b4868. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFData _isDeallocating], 0x1c01b4884. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFData retainCount], 0x1c01b48a0. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native +[__NSCFData allocWithZone:], 0x1c01b48a4. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native +[__NSCFData automaticallyNotifiesObserversForKey:], 0x1c01b48c0. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSCFData copyWithZone:], 0x1c01b48c8. Non-external. Immutable data in an app linked on or after
// version 6 is retained; mutable data, or any data in an older app, goes to super.
- (id)copyWithZone:(struct _NSZone *)zone {
    if (!__OCFDataMtbl((OCFDataRef)self) && _OCFExecutableLinkedOnOrAfter(6)) {
        return (id)_OCFNonObjCRetain(self);
    }
    return [super copyWithZone:zone];
}

// Native -[__NSCFData length], 0x1c01b497c. Non-external.
- (NSUInteger)length {
    return (NSUInteger)OCFDataGetLength((OCFDataRef)self);
}

// Native -[__NSCFData bytes], 0x1c01b4980. Non-external.
- (const void *)bytes {
    return _CFDataGetBytePtrNonObjC((OCFDataRef)self);
}

// Native -[__NSCFData mutableBytes], 0x1c01b4984. Non-external.
- (void *)mutableBytes {
    return _CFDataGetBytePtrNonObjC((OCFDataRef)self);
}

// Native -[__NSCFData setLength:], 0x1c01b4988. Non-external.
- (void)setLength:(NSUInteger)length {
    OCFDataSetLength((OCFMutableDataRef)self, (OCFIndex)length);
}

// Native -[__NSCFData _bridgingCopy:length:], 0x1c01b4990. Non-external. Stores the length, and a malloc copy
// of the bytes (typed allocation 0x1F347CF7) or NULL for empty data; answers NULL. The result's type is
// unread beyond its value (confidence: medium).
- (void *)_bridgingCopy:(void **)bytes length:(NSUInteger *)length {
    OCFIndex count = OCFDataGetLength((OCFDataRef)self);
    *length = (NSUInteger)count;
    void *copy;
    if (count != 0) {
        copy = malloc_type_malloc((size_t)count, 0x1F347CF7);
        memcpy(copy, _CFDataGetBytePtrNonObjC((OCFDataRef)self), (size_t)count);
    } else {
        copy = NULL;
    }
    *bytes = copy;
    return NULL;
}

// Native -[__NSCFData classForCoder], 0x1c01b4a04. Non-external. The class references of NSMutableData and
// NSData (0x2227ce368, 0x2227ce2c8), sent +self.
- (Class)classForCoder {
    if (__OCFDataMtbl((OCFDataRef)self)) {
        return [ONSMutableData self];
    }
    return [ONSData self];
}

@end
