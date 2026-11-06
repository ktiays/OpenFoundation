//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdio.h>
#include <stdlib.h>

// Inferred organization: __NSNameOfClass (0x1c05a1038), __NSFullMethodName (0x1c05a104c),
// __NSMethodExceptionProem (0x1c05a1124) and
// -[NSString(NSKindOfAdditions) isNSString__] (0x1c05a19e0) lie in the Foundation image's Objective-C runtime
// additions (0x1c05a1...); NSRequestConcreteImplementation (0x1c0fe4408) and __NSRequestConcreteObject
// (0x1c0fe44bc) lie in the outlined region with the class clusters' abstract methods. Following the FS-string
// ownership, the five share this unit. Objective-C compiled with manual reference counting. Unless a
// definition says otherwise: iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Foundation/String/ONSStringReview.md.
//
// The exception class and names are loaded through GOT slots outside the extracted image: 0x22227FAE0 is the
// receiver of +exceptionWithName:reason:userInfo: (NSException) and 0x22227F500 the name of the
// abstract-method exceptions (NSInvalidArgumentException); both bindings confidence: medium.

// libobjc's objc_opt_class, which the native code imports (the SDK installs no declaration for it).
extern Class objc_opt_class(id object);

// Native __NSNameOfClass, 0x1c05a1038. Non-external (was a private external).
const char *_NSNameOfClass(Class cls) {
    if (cls == Nil) {
        return "nil";
    }
    return class_getName(cls);
}

// Native __NSFullMethodName, 0x1c05a104c. External. As __NSMethodExceptionProem without the "*** " prefix.
id _ONSFullMethodName(id object, SEL selector) {
    Class cls = objc_opt_class(object);
    char *text = NULL;
    int marker = (id)cls == object ? '+' : '-';
    const char *className = cls != Nil ? class_getName(cls) : "nil";
    asprintf(&text, "%c[%s %s]", marker, className, sel_getName(selector));
    if (text == NULL) {
        return nil;
    }
    id name = [ONSString stringWithUTF8String:text];
    free(text);
    return name;
}

// Native __NSMethodExceptionProem, 0x1c05a1124. External. The marker is '+' when objc_opt_class answers the
// object itself; a Nil class prints "nil". The buffer from asprintf is freed after +stringWithUTF8String:.
id _ONSMethodExceptionProem(id object, SEL selector) {
    Class cls = objc_opt_class(object);
    char *text = NULL;
    int marker = (id)cls == object ? '+' : '-';
    const char *className = cls != Nil ? class_getName(cls) : "nil";
    asprintf(&text, "*** %c[%s %s]", marker, className, sel_getName(selector));
    if (text == NULL) {
        return nil;
    }
    id proem = [ONSString stringWithUTF8String:text];
    free(text);
    return proem;
}

@implementation ONSString (NSKindOfAdditions)

// Native -[NSString(NSKindOfAdditions) isNSString__], 0x1c05a19e0. Non-external. Answers YES.
- (BOOL)isNSString__ {
    return YES;
}

@end

// Native NSRequestConcreteImplementation, 0x1c0fe4408. External. When objc_opt_class of the object is the
// abstract class itself, the reason names the abstract class; otherwise it names the object's class through
// __NSNameOfClass and the selector again (sel_getName is called a second time).
void ONSRequestConcreteImplementation(id object, SEL selector, Class abstractClass) {
    Class cls = objc_opt_class(object);
    const char *selectorName = sel_getName(selector);
    id reason;
    if (cls == abstractClass) {
        reason =
            [ONSString stringWithFormat:
                           (id)OF_STR(
                               "*** -%s cannot be sent to an abstract object of class %s: Create a concrete "
                               "instance!"
                           ),
                           selectorName,
                           class_getName(abstractClass)];
    } else {
        const char *className = _NSNameOfClass(objc_opt_class(object));
        reason = [ONSString
            stringWithFormat:(id)OF_STR("*** -%s only defined for abstract class.  Define -[%s %s]!"),
                             selectorName,
                             className,
                             sel_getName(selector)];
    }
    objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                  reason:reason
                                                userInfo:nil]);
}

// Native __NSRequestConcreteObject, 0x1c0fe44bc. External. sel_getName runs before objc_opt_class.
void _ONSRequestConcreteObject(id object, SEL selector) {
    const char *selectorName = sel_getName(selector);
    const char *className = _NSNameOfClass(objc_opt_class(object));
    id reason = [ONSString stringWithFormat:(id)OF_STR(
                                                "*** initialization method -%s cannot be sent to an abstract "
                                                "object of class %s: Create a concrete instance!"
                                            ),
                                            selectorName,
                                            className];
    objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                  reason:reason
                                                userInfo:nil]);
}
