//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Notification/ONSNotification.h"

#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/objc-exception.h>

// Inferred organization: the NSNotification methods lie in two runs of the iOS Simulator __text section,
// 0x1c0571ebc-0x1c05722b8 and the abstract accessors and initializer at 0x1c0fe3aa4-0x1c0fe3b64 (the
// subclass-responsibility cluster); the native source path is not in the evidence, and the class has one
// translation unit here. Objective-C compiled with manual reference counting natively. Unless a definition
// says otherwise: iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified (assembly read in full); iOS Device 24A446: unread. Reading:
// Local/Scratch/cf-core-fd-misc/Reading.md; original evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json (family FM-misc).

// The classes this unit references by symbol: NSNotification itself where the native code loads its class
// reference without a message (___SCR_NSNotification), NSConcreteNotification, NSString and NSMutableString
// (Foundation's, GOT slots 0x22289f3d8 and 0x22289f118) and NSException (CoreFoundation's; the uniqued GOT
// slot 0x22227fae0 lies outside the image and is bound by the selector sent and the slots' alphabetical
// order, as is the exception name 0x22227f4e0, NSGenericException; confidence: medium).
extern char notificationClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNotification));
extern char concreteNotificationClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSConcreteNotification));
extern char stringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));
extern char mutableStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableString));
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));

// Selectors this unit sends to objects of classes outside it (NSString, NSMutableString, NSDictionary,
// NSException and NSCoder), declared for the message sends only; no protocol object is emitted.
@protocol NotificationMessages

+ (id)stringWithFormat:(id)format, ...;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
- (id)initWithCapacity:(NSUInteger)capacity;
- (void)appendFormat:(id)format, ...;
- (BOOL)isEqualToDictionary:(id)dictionary;
- (BOOL)allowsKeyedCoding;
- (void)encodeObject:(id)object forKey:(id)key;
- (void)encodeObject:(id)object;
- (id)decodeObjectForKey:(id)key;
- (id)decodeObject;

@end

@implementation ONSNotification

// Native -[NSNotification description], 0x1c0571ebc. External class, non-external method. The object and user
// info are read first, then an autoreleased NSMutableString of capacity 256, the receiver's class and name.
// One of four formats is appended by which of the object and user info are nil.
- (id)description {
    id object = [self object];
    id userInfo = [self userInfo];
    id string = [[[(id)mutableStringClass alloc] initWithCapacity:0x100] autorelease];
    Class cls = [self class];
    id name = [self name];
    if (object == nil && userInfo == nil) {
        [string appendFormat:(id)OF_STR("%@ %p {name = %@}"), cls, self, name];
    } else if (object == nil) {
        [string appendFormat:(id)OF_STR("%@ %p {name = %@; userInfo = %@}"), cls, self, name, userInfo];
    } else if (userInfo == nil) {
        [string appendFormat:(id)OF_STR("%@ %p {name = %@; object = %@}"), cls, self, name, object];
    } else {
        [string appendFormat:(id)OF_STR("%@ %p {name = %@; object = %@; userInfo = %@}"),
                             cls,
                             self,
                             name,
                             object,
                             userInfo];
    }
    return string;
}

// Native -[NSNotification hash], 0x1c0571fa0. The name's hash (a tail call).
- (NSUInteger)hash {
    return [[self name] hash];
}

// Native -[NSNotification isEqual:], 0x1c0571fb4. The receiver itself is equal and nil is not; the argument
// must be a kind of NSNotification (the class reference). Then equal names (-isEqual:) and identical objects;
// when the receiver's user info is not nil it is read again and compared with -isEqualToDictionary:,
// otherwise the argument's user info must be nil.
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    if (![object isKindOfClass:(Class)notificationClass]) {
        return NO;
    }
    if (![[self name] isEqual:[object name]]) {
        return NO;
    }
    if ([self object] != [object object]) {
        return NO;
    }
    if ([self userInfo] != nil) {
        if (![[self userInfo] isEqualToDictionary:[object userInfo]]) {
            return NO;
        }
        return YES;
    }
    if ([object userInfo] != nil) {
        return NO;
    }
    return YES;
}

// Native -[NSNotification copyWithZone:], 0x1c0572090. The receiver retained (objc_retain, a tail call).
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native -[NSNotification encodeWithCoder:], 0x1c0572094. The coder kind is read, then the name; a
// keyed coder gets "NS.name", "NS.object" and "NS.userinfo", any other coder the three objects in that order.
- (void)encodeWithCoder:(id)coder {
    BOOL keyed = [coder allowsKeyedCoding];
    id name = [self name];
    if (keyed) {
        [coder encodeObject:name forKey:(id)OF_STR("NS.name")];
        [coder encodeObject:[self object] forKey:(id)OF_STR("NS.object")];
        [coder encodeObject:[self userInfo] forKey:(id)OF_STR("NS.userinfo")];
        return;
    }
    [coder encodeObject:name];
    [coder encodeObject:[self object]];
    [coder encodeObject:[self userInfo]];
}

// Native -[NSNotification initWithCoder:], 0x1c0572158. The name, object and user info ("NS.name",
// "NS.object", "NS.userinfo" from a keyed coder, else three -decodeObject) are handed to
// -initWithName:object:userInfo: (a tail call).
- (id)initWithCoder:(id)coder {
    id name;
    id object;
    id userInfo;
    if ([coder allowsKeyedCoding]) {
        name = [coder decodeObjectForKey:(id)OF_STR("NS.name")];
        object = [coder decodeObjectForKey:(id)OF_STR("NS.object")];
        userInfo = [coder decodeObjectForKey:(id)OF_STR("NS.userinfo")];
    } else {
        name = [coder decodeObject];
        object = [coder decodeObject];
        userInfo = [coder decodeObject];
    }
    return [self initWithName:name object:object userInfo:userInfo];
}

// Native -[NSNotification classForCoder], 0x1c05721f8. +self of NSNotification (objc_opt_self, a tail call).
- (Class)classForCoder {
    return [ONSNotification self];
}

// Native +[NSNotification allocWithZone:], 0x1c0572204. NSNotification itself allocates
// NSConcreteNotification (objc_opt_self of the class symbol); any other receiver allocates itself.
// NSAllocateObject with no extra bytes (a tail call).
+ (id)allocWithZone:(struct _NSZone *)zone {
    Class cls = self;
    if (self == (Class)notificationClass) {
        cls = [(id)concreteNotificationClass self];
    }
    return ONSAllocateObject(cls, 0, zone);
}

// Native +[NSNotification notificationWithName:object:userInfo:], 0x1c0572278. objc_alloc, the initializer,
// then autoreleased (a tail call).
+ (id)notificationWithName:(id)name object:(id)object userInfo:(id)userInfo {
    return [[[self alloc] initWithName:name object:object userInfo:userInfo] autorelease];
}

// Native -[NSNotification init], 0x1c05722b8. Releases the receiver and throws NSGenericException ("%@:
// should never be used", the method proem computed first).
- (id)init {
    id reason = [(id)stringClass
        stringWithFormat:(id)OF_STR("%@: should never be used"), _ONSMethodExceptionProem(self, _cmd)];
    [self release];
    @throw [(id)exceptionClass exceptionWithName:(id)ONSGenericException reason:reason userInfo:nil];
}

// Native -[NSNotification name], 0x1c0fe3aa4. NSRequestConcreteImplementation with the class
// NSClassFromString("NSNotification") names; returns nil.
- (id)name {
    ONSRequestConcreteImplementation(
        self, _cmd, ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSNotification)))
    );
    return nil;
}

// Native -[NSNotification object], 0x1c0fe3ae4. As -name.
- (id)object {
    ONSRequestConcreteImplementation(
        self, _cmd, ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSNotification)))
    );
    return nil;
}

// Native -[NSNotification userInfo], 0x1c0fe3b24. As -name.
- (id)userInfo {
    ONSRequestConcreteImplementation(
        self, _cmd, ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSNotification)))
    );
    return nil;
}

// Native -[NSNotification initWithName:object:userInfo:], 0x1c0fe3b64. _NSRequestConcreteObject with the
// receiver and _cmd (the remaining argument registers pass through unchanged natively); returns nil.
- (id)initWithName:(id)name object:(id)object userInfo:(id)userInfo {
    _ONSRequestConcreteObject(self, _cmd);
    return nil;
}

@end
