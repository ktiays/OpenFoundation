//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Locale/ONSAutoLocale.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/objc-exception.h>

// Inferred organization: the NSAutoLocale coding methods lie at 0x1c05652c0-0x1c056535f of the iOS Simulator
// Foundation __text section. Objective-C compiled with manual reference counting. Unless a definition says
// otherwise: iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified
// (assembly read in full). iOS Device 24A446: unread. Bindings (confidence: medium): 0x22227FAE0 NSException,
// 0x22227F500 NSInvalidArgumentException, 0x22227FB00 the NSLocale class (+autoupdatingCurrentLocale).

extern id objc_retain(id object);
extern void objc_release(id object);

extern char autoLocaleExceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char autoLocaleLocaleClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSLocale));

// The messages the unit sends to objects of other classes, declared for the sends only.
@protocol AutoLocaleMessages
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)autoupdatingCurrentLocale;
- (BOOL)allowsKeyedCoding;
@end

@implementation ONSAutoLocale

// Native -[NSAutoLocale encodeWithCoder:], 0x1c05652c0. Non-external. Always raises.
- (void)encodeWithCoder:(id)coder {
    objc_exception_throw([(id)autoLocaleExceptionClass
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:(id)OF_STR("NSAutoLocale should not be encoded directly")
                 userInfo:nil]);
}

// Native -[NSAutoLocale initWithCoder:], 0x1c05652f0. Non-external. The receiver is released; a keyed coder
// answers the autoupdating current locale (retained), any other raises.
- (id)initWithCoder:(id)coder {
    BOOL keyed = [coder allowsKeyedCoding];
    objc_release(self);
    if (keyed) {
        return objc_retain([(id)autoLocaleLocaleClass autoupdatingCurrentLocale]);
    }
    objc_exception_throw([(id)autoLocaleExceptionClass
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:(id)OF_STR("NSLocales cannot be decoded by non-keyed archivers")
                 userInfo:nil]);
}

// Native -[NSAutoLocale classForCoder], 0x1c0565354. Non-external.
- (Class)classForCoder {
    return [ONSAutoLocale self];
}

@end
