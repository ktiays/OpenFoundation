//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSDictionary.h"

// The dictionary enumeration helpers (native cluster 0x1c01cbed4-0x1c01cc46c, between the preferences
// soft-link helpers and -[__NSDictionaryObjectEnumerator nextObject]), defined by
// Objective-C/OCFNSDictionaryEnumerate.m.

// Native __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__, 0x1c01cbed4 (was a private external): calls the block
// with the key, the object and the stop flag (the block's invoke pointer at offset 16).
OF_EXTERN void __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(
    void (^block)(id key, id object, BOOL *stop), id key, id object, BOOL *stop
);

// Native __NSDictionaryParameterCheckIterate, 0x1c01cbeec (was a private external): raises
// NSInvalidArgumentException "%@: block is nil" (the __CFExceptionProem of the object and selector) when the
// block is nil. The callers also pass their enumeration options (less bit 1) in the fourth argument, which
// the check does not read (confidence: medium for the parameter's role).
OF_EXTERN void __NSDictionaryParameterCheckIterate(id object, SEL selector, id block, NSUInteger options);

// Native __NSDictionaryEnumerate, 0x1c01cbf5c (was a private external): enumerates the dictionary's keys and
// objects with the NSEnumerationOptions bits (bit 0 concurrent) and the block.
OF_EXTERN void
__NSDictionaryEnumerate(id dictionary, NSUInteger options, void (^block)(id key, id object, BOOL *stop));
