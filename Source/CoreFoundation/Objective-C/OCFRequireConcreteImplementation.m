//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFRequireConcreteImplementation.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/message.h>
#include <objc/objc-exception.h>

// Inferred organization: __CFRequireConcreteImplementation (0x1c030fa8c) lies in the cold region among the
// fragments of the Objective-C runtime glue unit restored in Objective-C/OCFNSObject.m (after
// __NSSetDeallocHandler and before +[NSObject(NSObject) load].cold.1), with its shared fragment
// OUTLINED_FUNCTION_0_43 (0x1c029eed8) at the end of that unit's __text range. That file is outside this
// track's ownership, so the function has this translation unit, named for it; it joins the native unit when
// the owner integrates it. Objective-C, manual reference counting. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json. iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// libobjc entry point without an SDK declaration (objc-internal.h); supplied, never reimplemented.
extern Class objc_opt_class(id object);

// The NSException class symbol (_OBJC_CLASS_$_NSException), sent to directly.
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

// Native __CFRequireConcreteImplementation, 0x1c030fa8c, with OUTLINED_FUNCTION_0_43 (0x1c029eed8: the tail
// call of __CFFullMethodName with the object and the selector kept in X19 and X21). Hidden (was a private
// external). The object's class (objc_opt_class) is read first; when it is the abstract class itself the
// reason is "*** %@: method sent to an instance (%p) of an abstract class.  Create a concrete instance!" with
// the method name of that class and the object; otherwise "*** %@: method only defined for abstract class.
// Define %@!" with the method names of the abstract class and then of the object's class. The reason is made
// with kCFAllocatorSystemDefault and autoreleased through _CFAutoreleasePoolAddObject, whose pool argument is
// the reason itself (X0 is left unchanged), and +[NSException exceptionWithName:reason:userInfo:]
// (NSInvalidArgumentException, no user info) is thrown.
void __CFRequireConcreteImplementation(Class abstractClass, id object, SEL selector) {
    Class cls = objc_opt_class(object);
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef reason;
    if (cls == abstractClass) {
        OCFStringRef methodName = __CFFullMethodName(cls, object, selector);
        reason = OCFStringCreateWithFormat(
            allocator,
            NULL,
            OF_STR(
                "*** %@: method sent to an instance (%p) of an abstract class.  Create a concrete instance!"
            ),
            methodName,
            object
        );
    } else {
        OCFStringRef abstractMethodName = __CFFullMethodName(abstractClass, object, selector);
        OCFStringRef methodName = __CFFullMethodName(cls, object, selector);
        reason = OCFStringCreateWithFormat(
            allocator,
            NULL,
            OF_STR("*** %@: method only defined for abstract class.  Define %@!"),
            abstractMethodName,
            methodName
        );
    }
    id exception = ((id (*)(id, SEL, id, id, id))objc_msgSend)(
        (id)exceptionClass,
        sel_getUid("exceptionWithName:reason:userInfo:"),
        (id)ONSInvalidArgumentException,
        _OCFAutoreleasePoolAddObject((void *)reason, reason),
        nil
    );
    objc_exception_throw(exception);
}
