//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFLocale.h"

// Inferred organization: the __NSCFLocale methods (0x1c01b58e0-0x1c01b59bc) lie together at the end of the
// bridged-class run of the iOS Simulator __text section, after the stream methods; following the owner's
// decision, the class has its own translation unit. Objective-C compiled with manual reference counting
// (objc_autorelease). Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

@implementation __ONSCFLocale

// Native -[__NSCFLocale isEqual:], 0x1c01b58e0. Non-external. nil answers NO and the receiver itself YES
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

// Native -[__NSCFLocale hash], 0x1c01b591c. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[__NSCFLocale retain], 0x1c01b5920. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFLocale release], 0x1c01b5924. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFLocale _tryRetain], 0x1c01b5928. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFLocale _isDeallocating], 0x1c01b5944. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFLocale retainCount], 0x1c01b5960. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native +[__NSCFLocale allocWithZone:], 0x1c01b5964. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native +[__NSCFLocale automaticallyNotifiesObserversForKey:], 0x1c01b5980. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSCFLocale objectForKey:], 0x1c01b5988. Non-external.
- (id)objectForKey:(id)key {
    return (id)OCFLocaleGetValue((OCFLocaleRef)self, (OCFLocaleKey)key);
}

// Native -[__NSCFLocale displayNameForKey:value:], 0x1c01b5990. Non-external.
- (id)displayNameForKey:(id)key value:(id)value {
    return [(id)OCFLocaleCopyDisplayNameForPropertyValue(
        (OCFLocaleRef)self, (OCFStringRef)key, (OCFStringRef)value
    ) autorelease];
}

// Native -[__NSCFLocale initWithLocaleIdentifier:], 0x1c01b59ac. Non-external. The receiver is ignored (not
// released); a new CFLocale in the system default allocator is answered.
- (id)initWithLocaleIdentifier:(id)identifier {
    return (id)OCFLocaleCreate(kOCFAllocatorSystemDefault, (OCFLocaleIdentifier)identifier);
}

@end
