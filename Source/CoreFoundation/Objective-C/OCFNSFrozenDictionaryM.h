//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSDictionaryM.h"

// Statics of the native __NSDictionaryM unit that the __NSFrozenDictionaryM methods (natively in that unit)
// read. Objective-C/OCFNSDictionaryM.m defines them static on this branch: the coordinator must drop `static`
// from those four definitions (and include this header there) for these hidden declarations to link; until
// then they are undefined.

// Native __NSDictionarySizes, 0x1c046fb80 (__TEXT,__const, listed as ___NSDictionarySizes_0), non-external:
// the bucket counts per size index of the mutable and frozen dictionaries (the frozen methods read the low 32
// bits).
OF_EXTERN const uint64_t __NSDictionarySizes[40];

// Native ___NSDictionaryM_DeletedMarker, 0x2227aba60 (__DATA_CONST,__const, 16 bytes), non-external: the
// marker a removed entry's key slot holds.
OF_EXTERN const OCFRuntimeBase ___NSDictionaryM_DeletedMarker;

// Native ___NSDictionary_cowCallbacks, 0x2227aba98 (__DATA_CONST,__const), non-external: the copy-on-write
// callback record of the mutable and frozen dictionaries.
OF_EXTERN const CopyOnWriteCallbacks __NSDictionary_cowCallbacks;

// Native cow_cleanup, 0x1c02f765c, non-external: releases and frees the storage of a __NSDictionaryM (kind 1)
// or __NSFrozenDictionaryM (any other kind).
OF_EXTERN void cow_cleanup(id object, int kind);

#ifdef __OBJC__
// Native class __NSFrozenDictionaryM (__DATA,__objc_data 0x22e6cf340, was a private external), the immutable
// copy of a __NSDictionaryM that shares its storage through the copy-on-write control block, an NSDictionary
// subclass; class_ro_t 0x2227c87e0: instance start 8, size 32; ivars (offsets from their _OBJC_IVAR_$
// symbols, were private externals) storage at 8 (16 bytes, the layout of __NSDictionaryM's storage) and cow
// at 24 (8 bytes, read with load-acquire) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified for offsets and sizes; the type encodings lie outside the extracted image, the types
// follow __NSDictionaryM's). The ivars are public so that the dictionary units' C helpers reach them.
@interface __ONSFrozenDictionaryM : ONSDictionary {
    @public
    DictionaryMStorage storage;
    CopyOnWriteBlock *cow;
}
@end
#endif
