//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Error/ONSCFError.h"

#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

extern Class objc_opt_class(id object);
extern char cfErrorBaseClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSError));

@implementation ONSCFError

// Native -[NSCFError isEqual:], 0x1c0504a50. Non-external.
- (BOOL)isEqual:(id)object {
    return object != nil && (self == object || _OCFNonObjCEqual(self, object) != 0);
}

// Native -[NSCFError hash], 0x1c0504a8c. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[NSCFError retain], 0x1c0504a90. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[NSCFError release], 0x1c0504a94. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[NSCFError allowsWeakReference], 0x1c0504a98. Non-external.
- (BOOL)allowsWeakReference {
    return _OCFIsDeallocating(self) == 0;
}

// Native -[NSCFError retainWeakReference], 0x1c0504ab4. Non-external.
- (BOOL)retainWeakReference {
    return _OCFTryRetain(self) != NULL;
}

// Native -[NSCFError retainCount], 0x1c0504ad0. Non-external.
- (NSUInteger)retainCount {
    return OCFGetRetainCount(self);
}

// Native +[NSCFError automaticallyNotifiesObserversForKey:], 0x1c0504ad4. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[NSCFError initWithCoder:], 0x1c0504adc. Non-external.
- (id)initWithCoder:(id)coder {
    objc_release(self);
    return [objc_alloc((Class)cfErrorBaseClass) initWithCoder:coder];
}

// Native -[NSCFError code], 0x1c0504b0c. Non-external.
- (NSInteger)code {
    return OCFErrorGetCode((OCFErrorRef)self);
}

// Native -[NSCFError domain], 0x1c0504b10. Non-external.
- (id)domain {
    return (id)OCFErrorGetDomain((OCFErrorRef)self);
}

// Native -[NSCFError userInfo], 0x1c0504b14. Non-external.
- (id)userInfo {
    return objc_autorelease((id)OCFErrorCopyUserInfo((OCFErrorRef)self));
}

// Native -[NSCFError classForCoder], 0x1c0504b28. Non-external.
- (Class)classForCoder {
    return objc_opt_class((id)cfErrorBaseClass);
}

// Native -[NSCFError _callStackReturnAddresses], 0x1c0504b34. Non-external.
- (id)_callStackReturnAddresses {
    return objc_autorelease((id)_OCFErrorCopyCallStackReturnAddresses(self));
}

@end
