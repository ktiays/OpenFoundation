//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

// Runtime support functions of the Foundation image (String/ONSRuntimeSupport.m), iOS Simulator 24A434,
// arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless a declaration says otherwise.
// iOS Device 24A446: unread.

// Native __NSNameOfClass, 0x1c05a1038 (hidden, was a private external): the class's name, or "nil".
OF_EXTERN const char *_NSNameOfClass(Class cls);

// Native __NSFullMethodName, 0x1c05a104c, external: "-[Class selector]" ('+' when the object is a class),
// autoreleased, or nil when asprintf fails.
OF_EXPORT id _ONSFullMethodName(id object, SEL selector);

// Native __NSMethodExceptionProem, 0x1c05a1124, external: the "*** -[Class selector]" prefix of an exception
// reason ('+' when the object is a class), autoreleased, or nil when asprintf fails.
OF_EXPORT id _ONSMethodExceptionProem(id object, SEL selector);

// Native NSRequestConcreteImplementation (0x1c0fe4408) and __NSRequestConcreteObject (0x1c0fe44bc),
// external: raise NSInvalidArgumentException for an abstract method or initializer that reached the class
// cluster's abstract class. They never return.
OF_EXPORT void ONSRequestConcreteImplementation(id object, SEL selector, Class abstractClass)
    __attribute__((noreturn));
OF_EXPORT void _ONSRequestConcreteObject(id object, SEL selector) __attribute__((noreturn));

#ifdef __OBJC__
@interface ONSString (NSKindOfAdditions)

- (BOOL)isNSString__;

@end
#endif
