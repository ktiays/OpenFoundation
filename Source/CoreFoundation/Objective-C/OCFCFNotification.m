//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFCFNotification.h"

#include <objc/runtime.h>

// Inferred organization: the __CFNotification methods (0x1c01a8c94-0x1c01a8e8c) lie together in the iOS
// Simulator __text section among the notification center functions that Notification/OCFNotificationCenter.c
// restores (another family's file); following the owner's decision, the class has its own translation unit.
// Objective-C compiled with manual reference counting (objc_retain, objc_release). The native source path is
// not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.

// libobjc's objc_opt_isKindOfClass (objc-internal.h, no SDK declaration); supplied.
extern BOOL objc_opt_isKindOfClass(id object, Class cls);

@implementation __OCFNotification

// Native -[__CFNotification name], 0x1c01a8c94. Non-external; the ivar, unretained.
- (id)name {
    return _name;
}

// Native -[__CFNotification object], 0x1c01a8ca4. Non-external; the ivar, unretained.
- (id)object {
    return _object;
}

// Native -[__CFNotification userInfo], 0x1c01a8cb4. Non-external; the ivar, unretained.
- (id)userInfo {
    return _userInfo;
}

// Native -[__CFNotification hash], 0x1c01a8cc4. Non-external; the name's hash (a tail call).
- (NSUInteger)hash {
    return [[self name] hash];
}

// Native -[__CFNotification isEqual:], 0x1c01a8cd8. Non-external.
// The receiver itself is equal and nil is not. The classes "__CFNotification" and "NSNotification" are looked
// up (objc_lookUpClass) on every call; the argument must be a kind of the first or, when the second exists,
// of the second. Then the names must be equal (-isEqual:), the objects identical, and the user infos equal
// (-isEqual:, a tail call; a nil receiver's user info answers NO even when both are nil).
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    Class notificationClass = objc_lookUpClass(OF_NAME_STRING(__OCFNotification));
    Class foundationClass = objc_lookUpClass(OF_NAME_STRING(ONSNotification));
    if (!objc_opt_isKindOfClass(object, notificationClass)) {
        if (foundationClass == Nil) {
            return NO;
        }
        if (!objc_opt_isKindOfClass(object, foundationClass)) {
            return NO;
        }
    }
    if (![[self name] isEqual:[object name]]) {
        return NO;
    }
    if ([self object] != [object object]) {
        return NO;
    }
    return [[self userInfo] isEqual:[object userInfo]];
}

// Native -[__CFNotification copyWithZone:], 0x1c01a8dc8. Non-external; the receiver retained (objc_retain).
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native -[__CFNotification initWithName:object:userInfo:foundation:], 0x1c01a8dcc. Non-external.
// No super initializer is called. A nil name releases the receiver and answers nil. With bit 0 of foundation
// set, the name is copied (-copy), and the user info and then the object retained (objc_retain); otherwise
// the name is retained with CFRetain, the user info with CFRetain when it is not NULL, and the object is not
// retained. The foundation byte is stored as given.
- (id)initWithName:(id)aName object:(id)anObject userInfo:(id)aUserInfo foundation:(BOOL)foundation {
    if (aName == nil) {
        [self release];
        return nil;
    }
    if ((foundation & 1) != 0) {
        _name = [aName copy];
        _userInfo = [aUserInfo retain];
        anObject = [anObject retain];
    } else {
        _name = (id)OCFRetain(aName);
        _userInfo = aUserInfo != nil ? (id)OCFRetain(aUserInfo) : nil;
    }
    _object = anObject;
    _fouSemantics = foundation;
    return self;
}

// Native -[__CFNotification dealloc], 0x1c01a8e8c. Non-external. With _fouSemantics equal to 1 the name,
// object and user info are released (objc_release); otherwise the name and the user info, when not NULL, are
// released with CFRelease (the object was not retained). Then super's -dealloc.
- (void)dealloc {
    if (_fouSemantics == 1) {
        [_name release];
        [_object release];
        [_userInfo release];
    } else {
        if (_name != nil) {
            OCFRelease(_name);
        }
        if (_userInfo != nil) {
            OCFRelease(_userInfo);
        }
    }
    [super dealloc];
}

@end
