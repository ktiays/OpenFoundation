//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/objc.h>

// Native __CFRequireConcreteImplementation, 0x1c030fa8c (was a private external;
// Objective-C/OCFRequireConcreteImplementation.m): raises the NSInvalidArgumentException of a primitive
// method that the abstract class abstractClass leaves to its concrete subclasses. Not declared noreturn: the
// native callers emit code after the call.
OF_EXTERN void __CFRequireConcreteImplementation(Class abstractClass, id object, SEL selector);
