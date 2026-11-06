//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Archiving/ONSKeyedUnarchiverHelper.h"

#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Archiving/ONSObjCRuntimeNames.h"

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

extern void objc_setProperty_atomic_copy(id object, SEL selector, id value, ptrdiff_t offset);

@protocol KeyedUnarchiverAllowedNames
- (BOOL)containsObject:(id)object;
@end

@implementation _ONSKeyedUnarchiverHelper

// Native -[_NSKeyedUnarchiverHelper init], 0x1c055aa58. Non-external.
- (id)init {
    _allowedClasses = objc_opt_new((id)archivingMutableArrayClass);
    expectsUnnestedCollection = NO;
    decodedCollection = NO;
    return self;
}

// Native -[_NSKeyedUnarchiverHelper dealloc], 0x1c055aa8c. Non-external.
- (void)dealloc {
    objc_release(_allowed);
    objc_release(_allowedClasses);
    objc_release(_decodeError);
    [super dealloc];
}

// Native -[_NSKeyedUnarchiverHelper classNameAllowed:], 0x1c055ab64. Non-external.
- (BOOL)classNameAllowed:(Class)cls {
    if (_allowed == nil) {
        return YES;
    }
    BOOL allowed = NO;
    for (Class current = cls; current != Nil; current = class_getSuperclass(current)) {
        id<KeyedUnarchiverAllowedNames> names = (id)_allowed;
        allowed = [names containsObject:ONSStringFromClass(current)];
        if (allowed) {
            break;
        }
    }
    return allowed;
}

// Native -[_NSKeyedUnarchiverHelper setDecodeError:], 0x1c055abe8. Non-external.
- (void)setDecodeError:(id)error {
    objc_setProperty_atomic_copy(self, _cmd, error, (char *)&_decodeError - (char *)self);
}

@end
