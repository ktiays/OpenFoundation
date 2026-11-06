//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFError.h"

#include <objc/runtime.h>

// Inferred organization: the __NSCFError methods (0x1c01b42b8-0x1c01b4428) lie together in the iOS Simulator
// __text section, after the __NSCFBoolean methods; following the owner's decision, the class has its own
// translation unit. Objective-C compiled with manual reference counting (objc_release of the receiver in
// -initWithCoder:, objc_autorelease). Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

@implementation __ONSCFError

// Native -[__NSCFError isEqual:], 0x1c01b42b8. Non-external. nil answers NO and the receiver itself YES
// before _CFNonObjCEqual.
- (BOOL)isEqual:(id)object {
    if (object == nil) {
        return NO;
    }
    if (self == object) {
        return YES;
    }
    return _OCFNonObjCEqual(self, object) != 0;
}

// Native -[__NSCFError hash], 0x1c01b42f4. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[__NSCFError retain], 0x1c01b42f8. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFError release], 0x1c01b42fc. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFError _tryRetain], 0x1c01b4300. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFError _isDeallocating], 0x1c01b431c. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFError retainCount], 0x1c01b4338. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native +[__NSCFError allocWithZone:], 0x1c01b433c. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native -[__NSCFError initWithCoder:], 0x1c01b4358. Non-external. The receiver is released and a new NSError
// (looked up by name) decodes instead.
- (id)initWithCoder:(id)coder {
    [self release];
    Class errorClass = objc_lookUpClass(OF_NAME_STRING(ONSError));
    return [[errorClass alloc] initWithCoder:coder];
}

// Native +[__NSCFError automaticallyNotifiesObserversForKey:], 0x1c01b438c. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSCFError code], 0x1c01b4394. Non-external.
- (NSInteger)code {
    return OCFErrorGetCode((OCFErrorRef)self);
}

// Native -[__NSCFError domain], 0x1c01b4398. Non-external.
- (id)domain {
    return (id)OCFErrorGetDomain((OCFErrorRef)self);
}

// Native -[__NSCFError userInfo], 0x1c01b439c. Non-external.
- (id)userInfo {
    return [(id)OCFErrorCopyUserInfo((OCFErrorRef)self) autorelease];
}

// Native -[__NSCFError classForCoder], 0x1c01b43b0. Non-external. Looked up by name.
- (Class)classForCoder {
    return objc_lookUpClass(OF_NAME_STRING(ONSError));
}

// Native -[__NSCFError copyWithZone:], 0x1c01b43bc. Non-external. A new NSError (looked up by name) is
// allocated in the zone first, then initialized with the receiver's domain, code and user info.
- (id)copyWithZone:(struct _NSZone *)zone {
    id copy = [objc_lookUpClass(OF_NAME_STRING(ONSError)) allocWithZone:zone];
    id domain = [self domain];
    NSInteger code = [self code];
    id userInfo = [self userInfo];
    return [copy initWithDomain:domain code:code userInfo:userInfo];
}

@end
