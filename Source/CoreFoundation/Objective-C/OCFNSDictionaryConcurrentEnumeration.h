//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSDictionary.h"

// Native __NSCollectionHandleConcurrentEnumerationIfSpecified, 0x1c02acc44 (was a private external), the
// concurrent path of the immutable and mutable collections' block enumerations (native neighbours: the
// __NSArrayI_Transfer methods before, OUTLINED_FUNCTION_0/_1 and _CFIsMatchingLibrary after). With bit 0 of
// the options set and at least two active processors it runs the block for every index on
// dispatch_apply(iterations, NULL, ...), each call inside its own autorelease pool when usePool is 1, sharing
// a stop flag, and answers YES; otherwise NO without calling the block. Defined by
// Objective-C/OCFNSDictionaryConcurrentEnumeration.m.
OF_EXTERN BOOL __NSCollectionHandleConcurrentEnumerationIfSpecified(
    NSUInteger options, BOOL usePool, size_t iterations, void (^block)(size_t index, BOOL *stop)
);
