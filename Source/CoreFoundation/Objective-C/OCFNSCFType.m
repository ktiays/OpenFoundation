//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFType.h"

// Inferred organization: the __NSCFType methods (0x1c029e9bc-0x1c029eaf0) lie together in the iOS Simulator
// __text section beside the Objective-C runtime glue (__CFLookUpClass); following the owner's decision, each
// bridged class has its own translation unit. The unit is Objective-C compiled with manual reference counting
// (objc_autorelease and explicit -dealloc chaining). Unless a definition says otherwise: iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device
// 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written
// yet).

@implementation __ONSCFType

// Native -[__NSCFType isEqual:], 0x1c029e9bc. Non-external. nil answers NO and the receiver itself YES before
// _CFNonObjCEqual.
- (BOOL)isEqual:(id)object {
    if (object == nil) {
        return NO;
    }
    if (object == self) {
        return YES;
    }
    return _OCFNonObjCEqual(self, object) != 0;
}

// Native -[__NSCFType hash], 0x1c029e9f8. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[__NSCFType retain], 0x1c029e9fc. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFType release], 0x1c029ea00. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFType _tryRetain], 0x1c029ea04. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFType _isDeallocating], 0x1c029ea20. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFType retainCount], 0x1c029ea3c. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native -[__NSCFType description], 0x1c029ea40. Non-external.
- (id)description {
    return [(id)OCFCopyDescription(self) autorelease];
}

// Native -[__NSCFType dealloc], 0x1c029ea54. Non-external. A CFAllocator (type ID 2) is freed through
// __CFAllocatorDeallocate; any other type runs its class's finalize callback (offset 32 of the class record)
// when present. Then super's -dealloc.
- (void)dealloc {
    OCFTypeID typeID = OCFGetTypeID(self);
    if (typeID == 2) {
        __CFAllocatorDeallocate(self);
    } else {
        const OCFRuntimeClass *cls = _OCFRuntimeGetClassWithTypeID(typeID);
        if (cls->finalize != NULL) {
            cls->finalize(self);
        }
    }
    [super dealloc];
}

// Native +[__NSCFType automaticallyNotifiesObserversForKey:], 0x1c029eaec. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

@end
