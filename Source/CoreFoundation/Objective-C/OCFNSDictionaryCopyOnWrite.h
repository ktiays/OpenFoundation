//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSDictionary.h"

#include <os/lock.h>
#include <stdbool.h>
#include <stdint.h>

// The copy-on-write helpers of the mutable collections (native cluster 0x1c019655c-0x1c01966cf, after the
// __NSSetEnumerate helpers and before _CFDoubledStringCreate; their cold-region pieces
// 0x1c0307f50-0x1c0307f97), defined by Objective-C/OCFNSDictionaryCopyOnWrite.c. iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC.

// The control block __cow_create allocates (8 bytes, malloc_type_calloc(8, 1, 0xF41EA2E1)): an
// os_unfair_lock at 0, and two 16-bit reference counts at 4 and 6 (confidence: verified for offsets and
// widths). The native names lie outside the extracted image; the field names are descriptive: the count at 4
// grows for an immutable copy (__cow_copy mode 0, as -[__NSDictionaryM copy] passes), the count at 6 for a
// mutable holder (mode 1, as -mutableCopy passes, and the creator when __cow_create's mode is 1); a mutation
// requires the count at 6 to be nonzero (confidence: medium for the roles).
typedef struct {
    os_unfair_lock lock;
    uint16_t immutableReferences;
    uint16_t mutableReferences;
} CopyOnWriteBlock;

// The callback record a collection passes (for the dictionaries ___NSDictionary_cowCallbacks, 0x2227aba98):
// copy an instance's storage reference into a copy (the source, the collection kind, the copy and the mode,
// as
// __cow_copy passes them), copy the storage so that the object owns it alone (returns false on failure), set
// the object's control block, and release the object's storage (the object and its kind) (confidence:
// verified for slots and arguments; the native field names are unread).
typedef struct {
    void (*copyInstance)(id source, int kind, id copy, int mode);
    bool (*copyStorage)(id object);
    void (*setCow)(id object, CopyOnWriteBlock *cow);
    void (*cleanup)(id object, int kind);
} CopyOnWriteCallbacks;

// Native __cow_create, 0x1c019655c, with .cold.1 (0x1c0307f6c). Hidden (was a private external). The object
// argument is not read.
OF_EXTERN CopyOnWriteBlock *_cow_create(id object, int mode);

// Native __cow_copy, 0x1c01965ac, with .cold.1 (0x1c0307f80). Hidden (was a private external).
OF_EXTERN void _cow_copy(
    id source, int kind, CopyOnWriteBlock *cow, const CopyOnWriteCallbacks *callbacks, id copy, int mode
);

// Native __cow_mutate_slow, 0x1c0196688, with .cold.1 (0x1c0307f8c). Hidden (was a private external). The
// control block argument is not read.
OF_EXTERN void _cow_mutate_slow(id object, CopyOnWriteBlock *cow, const CopyOnWriteCallbacks *callbacks);

// Native __cow_failure, 0x1c0307f50. Hidden (was a private external).
OF_EXTERN __attribute__((noreturn)) void _cow_failure(OCFStringRef message);
