//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <pthread.h>

// The enumeration and search helpers of NSOrderedSet's block methods (Objective-C/OCFNSOrderedSetHelpers.m);
// all were private externals natively. The options word is the NSEnumerationOptions of the caller (bit 0
// concurrent, bit 1 reverse) with the helper's own bits: bit 60 (the indexes argument restricts the walk),
// bit 61 (stop at the first passing index, kept in *result) and bit 62 (collect the passing indexes into
// resultIndexes).

// Native __NSORDEREDSET_IS_CALLING_OUT_TO_A_BLOCK__, 0x1c01f7158: calls the enumeration block (a framed call
// kept out of line for backtraces).
OF_EXTERN void __NSORDEREDSET_IS_CALLING_OUT_TO_A_BLOCK__(
    void (^block)(id object, NSUInteger index, BOOL *stop), id object, NSUInteger index, BOOL *stop
);

// Native __NSOrderedSetParameterCheckIterate, 0x1c01f7170: with bit 60 of options, raises unless indexes is
// not nil and its last index lies inside the ordered set (or is NSNotFound). block is not read.
OF_EXTERN void
__NSOrderedSetParameterCheckIterate(id orderedSet, SEL selector, id block, NSUInteger options, id indexes);

// Native __NSOrderedSetChunkIterate, 0x1c01f740c: walks the inclusive index range start..end of the ordered
// set; answers whether the block set its stop flag.
OF_EXTERN BOOL __NSOrderedSetChunkIterate(
    id orderedSet,
    NSUInteger options,
    NSUInteger start,
    NSUInteger end,
    id indexes,
    id block,
    NSUInteger *result,
    id resultIndexes,
    pthread_mutex_t *mutex
);

// Native __NSORDEREDSET_IS_CALLING_OUT_TO_A_BOOL_BLOCK__, 0x1c01f76c8: calls the test block (out of line).
OF_EXTERN BOOL __NSORDEREDSET_IS_CALLING_OUT_TO_A_BOOL_BLOCK__(
    BOOL (^block)(id object, NSUInteger index, BOOL *stop), id object, NSUInteger index, BOOL *stop
);

// Native __NSOrderedSetEnumerate, 0x1c01f76e0: the body of the -enumerateObjects... methods.
OF_EXTERN void __NSOrderedSetEnumerate(id orderedSet, id block, NSUInteger options, id indexes);

// Native __NSOrderedSetGetIndexPassingTest, 0x1c01f7b28: the body of the -indexOfObject...passingTest:
// methods.
OF_EXTERN NSUInteger
__NSOrderedSetGetIndexPassingTest(id orderedSet, id block, NSUInteger options, id indexes);

// Native __NSOrderedSetGetIndexesPassingTest, 0x1c01f7f70: the body of the -indexesOfObjects...passingTest:
// methods; answers an autoreleased index set.
OF_EXTERN id __NSOrderedSetGetIndexesPassingTest(id orderedSet, id block, NSUInteger options, id indexes);
