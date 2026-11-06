//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSPlaceholderTimeZone.h"

#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/String/OCFString.h"

#include <objc/objc-exception.h>

// Inferred organization: the __NSPlaceholderTimeZone methods (-initWithName:data: 0x1c02aabb0 through
// -nextDaylightSavingTimeTransitionAfterDate: 0x1c02aaf5c) lie in the iOS Simulator __text section just
// before the NSTimeZone methods that Objective-C/OCFNSTimeZone.m restores; following the owner's decision,
// the class has its own translation unit. Objective-C compiled with manual reference counting (objc_retain,
// objc_release). The native source path is not in the evidence. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full;
// constant strings resolved from the ADRP/ADD encodings); iOS Device 24A446: unread. Reading: not yet
// written; the evidence is the assembly in Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json.

// Class methods the placeholder sends through the NSTimeZone class reference (off_2227CE3D8); NSTimeZone's
// private factory methods are implemented outside this image. Declarations only, no category is emitted.
@interface ONSTimeZone (PlaceholderTimeZoneMessages)

+ (id)_timeZoneWithName:(id)tzName data:(id)aData;
+ (id)_timeZoneWithName:(id)tzName;

@end

// The raise every primitive accessor of the placeholder inlines: kCFAllocatorSystemDefault, then the
// __CFFullMethodName of NSTimeZone (looked up by name with __CFLookUpClass), the receiver and the selector
// form the reason "*** %@: method sent to an uninitialized time zone object" (the constant string
// cfstr_MethodSentToAn_3); the reason is added to the current autorelease pool, and
// +[NSException exceptionWithName:reason:userInfo:] with NSInvalidArgumentException and no user info is
// thrown. raiseUninitializedTimeZone is a local descriptive name for the inlined code.
static inline __attribute__((always_inline, noreturn)) void raiseUninitializedTimeZone(id self, SEL _cmd) {
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef methodName =
        __CFFullMethodName((Class)__CFLookUpClass(OF_NAME_STRING(ONSTimeZone)), (const void *)self, _cmd);
    OCFStringRef reason = OCFStringCreateWithFormat(
        allocator, NULL, OF_STR("*** %@: method sent to an uninitialized time zone object"), methodName
    );
    id exception = [ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                            reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                          userInfo:nil];
    objc_exception_throw(exception);
}

@implementation __ONSPlaceholderTimeZone

// Native -[__NSPlaceholderTimeZone initWithName:data:], 0x1c02aabb0. Non-external. nil for a nil name;
// otherwise +[NSTimeZone _timeZoneWithName:data:], retained (objc_retain, a tail call). The placeholder
// receiver is not released.
- (id)initWithName:(id)tzName data:(id)aData {
    if (tzName == nil) {
        return nil;
    }
    return [[ONSTimeZone _timeZoneWithName:tzName data:aData] retain];
}

// Native -[__NSPlaceholderTimeZone initWithName:], 0x1c02aabd8. Non-external. nil for a nil name; otherwise
// +[NSTimeZone _timeZoneWithName:], retained (objc_retain, a tail call). The placeholder receiver is not
// released.
- (id)initWithName:(id)tzName {
    if (tzName == nil) {
        return nil;
    }
    return [[ONSTimeZone _timeZoneWithName:tzName] retain];
}

// Native -[__NSPlaceholderTimeZone init], 0x1c02aac00. Non-external. The receiver is released (objc_release)
// and nil returned.
- (id)init {
    [self release];
    return nil;
}

// Native -[__NSPlaceholderTimeZone retain], 0x1c02aac18. Non-external. The receiver (RET).
- (instancetype)retain {
    return self;
}

// Native -[__NSPlaceholderTimeZone release], 0x1c02aac1c. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[__NSPlaceholderTimeZone retainCount], 0x1c02aac20. Non-external. All ones (MOV X0, #-1).
- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

// Native -[__NSPlaceholderTimeZone dealloc], 0x1c02aac28. Non-external. Does nothing (RET); it never reaches
// super's -dealloc, so the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
}

#pragma clang diagnostic pop

// Native -[__NSPlaceholderTimeZone name], 0x1c02aac2c. Non-external. Raises (raiseUninitializedTimeZone).
- (id)name {
    raiseUninitializedTimeZone(self, _cmd);
}

// Native -[__NSPlaceholderTimeZone data], 0x1c02aacb4. Non-external. Raises (raiseUninitializedTimeZone).
- (id)data {
    raiseUninitializedTimeZone(self, _cmd);
}

// Native -[__NSPlaceholderTimeZone secondsFromGMTForDate:], 0x1c02aad3c. Non-external. Raises
// (raiseUninitializedTimeZone).
- (NSInteger)secondsFromGMTForDate:(id)date {
    raiseUninitializedTimeZone(self, _cmd);
}

// Native -[__NSPlaceholderTimeZone abbreviationForDate:], 0x1c02aadc4. Non-external. Raises
// (raiseUninitializedTimeZone).
- (id)abbreviationForDate:(id)date {
    raiseUninitializedTimeZone(self, _cmd);
}

// Native -[__NSPlaceholderTimeZone isDaylightSavingTimeForDate:], 0x1c02aae4c. Non-external. Raises
// (raiseUninitializedTimeZone).
- (BOOL)isDaylightSavingTimeForDate:(id)date {
    raiseUninitializedTimeZone(self, _cmd);
}

// Native -[__NSPlaceholderTimeZone daylightSavingTimeOffsetForDate:], 0x1c02aaed4. Non-external. Raises
// (raiseUninitializedTimeZone).
- (double)daylightSavingTimeOffsetForDate:(id)date {
    raiseUninitializedTimeZone(self, _cmd);
}

// Native -[__NSPlaceholderTimeZone nextDaylightSavingTimeTransitionAfterDate:], 0x1c02aaf5c. Non-external.
// Raises (raiseUninitializedTimeZone).
- (id)nextDaylightSavingTimeTransitionAfterDate:(id)date {
    raiseUninitializedTimeZone(self, _cmd);
}

@end
