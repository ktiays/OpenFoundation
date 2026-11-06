//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Objective-C/OCFNSDictionaryCopyOnWrite.h"

#include <malloc/malloc.h>

// Inferred organization: __cow_create (0x1c019655c), __cow_copy (0x1c01965ac), __cow_mutate_slow
// (0x1c0196688) and their OUTLINED_FUNCTION_0 (0x1c01966c4, listed as _OUTLINED_FUNCTION_0_7) lie together in
// the iOS Simulator __text section after the __NSSetEnumerate helpers and before _CFDoubledStringCreate, so
// they form their own native unit (the shared copy-on-write support of the mutable collections); the cold
// region holds
// __cow_failure (0x1c0307f50) and the three .cold.1 pieces (0x1c0307f6c-0x1c0307f8c), after
// __ResetUDateFormat.cold.1 and before __CFAccentuatedStringCreate.cold.1. The native source path is not in
// the evidence; the unit holds no Objective-C construct, so it is compiled as C (confidence: medium for the
// source language). The file name follows the dictionary sub-track's ownership. Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read in full, iphone-simulator-corpus-delta8.json). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// Native __cow_create, 0x1c019655c. Hidden (was a private external). An 8-byte zeroed block
// (malloc_type_calloc(8, 1, 0xF41EA2E1)); the lock word is cleared, the count at 4 is 1 unless the mode is 1,
// the count at 6 is 1 when it is. A failed allocation reaches .cold.1 (0x1c0307f6c): __cow_failure with
// "Unable to create CoW control block".
CopyOnWriteBlock *_cow_create(id object, int mode) {
    (void)object;
    CopyOnWriteBlock *cow = malloc_type_calloc(8, 1, 0xF41EA2E1);
    if (cow == NULL) {
        _cow_failure(OF_STR("Unable to create CoW control block"));
    }
    *(uint32_t *)&cow->lock = 0;
    cow->immutableReferences = mode != 1;
    cow->mutableReferences = mode == 1;
    return cow;
}

// Native __cow_copy, 0x1c01965ac. Hidden (was a private external). Under the block's lock: mode 1 counts a
// mutable reference, any other mode an immutable one, and the instance callback shares the storage with the
// copy. When the count to raise is already 0xFFFF it is left unchanged and the copy instead takes its own
// storage: the instance callback, then the storage callback on the copy (failure reaches .cold.1,
// 0x1c0307f80, OUTLINED_FUNCTION_0_7: __cow_failure with "Failed to grow buffer"), then the copy's control
// block is cleared.
void _cow_copy(
    id source, int kind, CopyOnWriteBlock *cow, const CopyOnWriteCallbacks *callbacks, id copy, int mode
) {
    os_unfair_lock_lock(&cow->lock);
    uint16_t immutableReferences = cow->immutableReferences;
    uint16_t mutableReferences = cow->mutableReferences;
    if (mode == 1) {
        if (mutableReferences == 0xFFFF) {
            goto detach;
        }
        mutableReferences += 1;
    } else {
        if (immutableReferences == 0xFFFF) {
            goto detach;
        }
        immutableReferences += 1;
    }
    cow->immutableReferences = immutableReferences;
    cow->mutableReferences = mutableReferences;
    callbacks->copyInstance(source, kind, copy, mode);
    os_unfair_lock_unlock(&cow->lock);
    return;
detach:
    callbacks->copyInstance(source, kind, copy, mode);
    if (!callbacks->copyStorage(copy)) {
        _cow_failure(OF_STR("Failed to grow buffer"));
    }
    callbacks->setCow(copy, NULL);
    os_unfair_lock_unlock(&cow->lock);
}

// Native __cow_mutate_slow, 0x1c0196688. Hidden (was a private external). The object takes its own storage
// (failure reaches .cold.1, 0x1c0307f8c, OUTLINED_FUNCTION_0_7: __cow_failure with "Failed to grow buffer"),
// then its control block is cleared (a tail call).
void _cow_mutate_slow(id object, CopyOnWriteBlock *cow, const CopyOnWriteCallbacks *callbacks) {
    (void)cow;
    if (!callbacks->copyStorage(object)) {
        _cow_failure(OF_STR("Failed to grow buffer"));
    }
    callbacks->setCow(object, NULL);
}

// Native __cow_failure, 0x1c0307f50. Hidden (was a private external). Stores the message's ASCII C string
// pointer (CFStringGetCStringPtr with 0x600) as the crash annotation message (gCRAnnotations + 8,
// qword_22E6D2398) and executes BRK #1.
void _cow_failure(OCFStringRef message) {
    crashWithMessage(OCFStringGetCStringPtr(message, kOCFStringEncodingASCII));
}
