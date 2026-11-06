//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#ifdef __OBJC__
// Native __NSSET_IS_CALLING_OUT_TO_A_BLOCK__, 0x1c01960d4 (was a private external): calls the enumeration
// block with the object and the stop flag, in its own frame (the name marks the callout in backtraces).
OF_EXTERN void
__NSSET_IS_CALLING_OUT_TO_A_BLOCK__(void (^block)(id object, BOOL *stop), id object, BOOL *stop);

// Native __NSSetEnumerate, 0x1c01960ec (was a private external): the block enumeration of a set, concurrent
// when bit 0 of the options is set and more than one processor is active.
OF_EXTERN void __NSSetEnumerate(id set, NSUInteger options, void (^block)(id object, BOOL *stop));
#endif
