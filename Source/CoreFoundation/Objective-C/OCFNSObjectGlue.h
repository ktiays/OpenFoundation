//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once
#include "CoreFoundation/Objective-C/OCFRequireConcreteImplementation.h"

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <objc/objc.h>
#include <stdbool.h>

// The Objective-C runtime glue functions restored in Objective-C/OCFNSObjectGlue.m (external natively):
// whether an object is an NSDate, and the weak-reference store and load with a strong fallback flag that
// __NSCFStreamWeakDelegateWrapper uses.
OF_EXPORT BOOL _ONSIsNSDate(id object);
OF_EXPORT BOOL _ONSIsNSObject(id object);
OF_EXPORT id _ONSObjectLoadWeak(id *location, bool *useFallback);
OF_EXPORT id _ONSObjectStoreWeak(id *location, id object, bool *useFallback);
OF_EXPORT id _ONSObjectLoadWeakRetained(id *location, bool *useFallback);

// Interfaces of families not yet restored on this branch
// Native __CFRequireConcreteImplementation, 0x1c030fa8c (was a private external; family A-array of the
// batch): raises NSInvalidArgumentException for a method of the abstract class that the object's class does
// not override. Its callers continue after the call (it is not declared noreturn natively).
