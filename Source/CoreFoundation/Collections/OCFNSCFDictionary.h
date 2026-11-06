//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Collections/OCFDictionary.h"

#include <objc/objc.h>

// __NSCFDictionaryCreateTransfer, __NSCFDictionaryCreate and __NSCFDictionaryCreateMutable, this unit's
// functions, are declared in Collections/OCFDictionary.h, which their CFDictionary callers include.

// The C interfaces of the Foundation dictionary classes that live in CoreFoundation, declared here so that C
// units (this one, Error/OCFError.c) reach them without the classes' Objective-C interfaces
// (Objective-C/OCFNSDictionary*.h).

// Native __NSDictionaryImmutablePlaceholder (0x1c02f337c) and __NSDictionaryMutablePlaceholder (0x1c02f3388),
// non-external (were private externals), among the __NSPlaceholderDictionary methods: the addresses of the
// static immutable and mutable placeholder instances. Defined by this unit.
OF_EXTERN id __NSDictionaryImmutablePlaceholder(void);
OF_EXTERN id __NSDictionaryMutablePlaceholder(void);

// Native ___immutablePlaceholderDictionary (0x2227ab930) and ___mutablePlaceholderDictionary (0x2227ab940),
// __DATA_CONST,__const, non-external: the static placeholder instances those functions answer, hidden so that
// the __NSPlaceholderDictionary methods (Objective-C/OCFNSPlaceholderDictionary.m) compare their receiver
// with them, as the native methods of the same unit do. Defined by this unit.
OF_EXTERN const OCFRuntimeBase __immutablePlaceholderDictionary;
OF_EXTERN const OCFRuntimeBase __mutablePlaceholderDictionary;

// Native __NSDictionaryI_new, 0x1c018bfa8 (was a private external): the keys and objects as parallel arrays,
// or (third argument) one interleaved array, the count, and the option bits (1 copy the keys, 2 record copied
// keys in _copyKeys, 4 do not retain the objects, 8 do not retain the keys; confidence: medium for the bit
// roles beyond the values the callers pass, see Objective-C/OCFNSDictionaryI.m). Defined by
// Objective-C/OCFNSDictionaryI.m.
OF_EXTERN OCFDictionaryRef __NSDictionaryI_new(
    const void **keys, const void **objects, const void **keysAndObjects, OCFIndex count, uintptr_t options
);

// Native __NSSingleEntryDictionaryI_new, 0x1c019eb8c (was a private external): the key, the object and the
// option bits of __NSDictionaryI_new. Defined by Objective-C/OCFNSSingleEntryDictionaryI.m.
OF_EXTERN OCFDictionaryRef
__NSSingleEntryDictionaryI_new(const void *key, const void *object, uintptr_t options);

// Native __NSDictionaryM_new, 0x1c02f7260 (was a private external): the keys and objects as parallel arrays
// (or NULL), the capacity, and the option bits of __NSDictionaryI_new. Defined by
// Objective-C/OCFNSDictionaryM.m.
OF_EXTERN OCFMutableDictionaryRef
__NSDictionaryM_new(const void **keys, const void **objects, OCFIndex capacity, uintptr_t options);

// Native __NSDictionary0__struct, 0x2227cc280 (__DATA_CONST,__const_cfobj2, 16 bytes: the isa __NSDictionary0
// and a zero word), external: the empty immutable dictionary singleton. Defined by
// Objective-C/OCFNSDictionary0.m.
OF_EXPORT const OCFRuntimeBase __ONSDictionary0__struct;

// Native __NSDictionary0__, 0x2227a4378 (__DATA_CONST,__const), external: the address of the empty immutable
// dictionary singleton, __NSDictionary0__struct. Defined by Objective-C/OCFNSDictionary0.m.
OF_EXPORT const OCFDictionaryRef __ONSDictionary0__;
