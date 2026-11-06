//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <pthread.h>

// The enumeration and search helpers of NSArray's block methods (Objective-C/OCFNSArrayHelpers.m); all were
// private externals natively. The options word is the NSEnumerationOptions of the caller (bit 0 concurrent,
// bit 1 reverse) with the helper's own bits: bit 60 (the indexes argument restricts the walk), bit 61 (stop
// at the first passing index, kept in *result) and bit 62 (collect the passing indexes into resultIndexes).

// Native __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__, 0x1c01e8134: calls the enumeration block (a framed call kept
// out of line for backtraces).
OF_EXTERN void __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__(
    void (^block)(id object, NSUInteger index, BOOL *stop), id object, NSUInteger index, BOOL *stop
);

// Native __NSArrayParameterCheckIterate, 0x1c01e814c: with bit 60 of options, raises unless indexes is not
// nil and its last index lies inside the array (or is NSNotFound). block is not read.
OF_EXTERN void
__NSArrayParameterCheckIterate(id array, SEL selector, id block, NSUInteger options, id indexes);

// Native __NSArrayChunkIterate, 0x1c01e8448: walks the inclusive index range start..end of the array; answers
// whether the block set its stop flag.
OF_EXTERN BOOL __NSArrayChunkIterate(
    id array,
    NSUInteger options,
    NSUInteger start,
    NSUInteger end,
    id indexes,
    id block,
    NSUInteger *result,
    id resultIndexes,
    pthread_mutex_t *mutex
);

// Native __NSARRAY_IS_CALLING_OUT_TO_A_BOOL_BLOCK__, 0x1c01e8704: calls the test block (out of line).
OF_EXTERN BOOL __NSARRAY_IS_CALLING_OUT_TO_A_BOOL_BLOCK__(
    BOOL (^block)(id object, NSUInteger index, BOOL *stop), id object, NSUInteger index, BOOL *stop
);

// Native __NSArrayEnumerate, 0x1c01e871c: the body of the -enumerateObjects... methods.
OF_EXTERN void __NSArrayEnumerate(id array, id block, NSUInteger options, id indexes);

// Native __NSArrayGetIndexPassingTest, 0x1c01e8b64: the body of the -indexOfObject...passingTest: methods.
OF_EXTERN NSUInteger __NSArrayGetIndexPassingTest(id array, id block, NSUInteger options, id indexes);

// Native __NSArrayGetIndexesPassingTest, 0x1c01e8fb0: the body of the -indexesOfObjects...passingTest:
// methods; answers an autoreleased index set.
OF_EXTERN id __NSArrayGetIndexesPassingTest(id array, id block, NSUInteger options, id indexes);
