//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Collections/ONSObjectArray.h"
#import "Foundation/String/ONSAttributedStringInternal.h"
#include "CoreFoundation/String/OCFString.h"

#include <malloc/malloc.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the two object-array allocation functions in Foundation's allocation unit,
// required by attributed-string decoding. Evidence:
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json, FC-sets-attributed. iOS Simulator 24A434,
// arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device divergence:
// iOS Device 24A446 remains unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md. Shared-cache bindings to NSException at
// 0x22227fae0 and NSGenericException at 0x22227f4e0 have medium confidence.

// Native NSAllocateObjectArray, 0x1c0593b60. External. Counts with any of bits 61-63 set throw before
// calling the allocator; the good allocation size and type ID are preserved, including count zero.
id *ONSAllocateObjectArray(NSUInteger count) {
    if (OF_WORD_SHIFT(count, 61)) {
        id reason = [(id)attributedStringStringClass
            stringWithFormat:(id)OF_STR(
                                 "*** attempt to allocate id buffer with too large of a count (%lu) -- "
                                 "possibly data is corrupt"
                             ),
                             count];
        objc_exception_throw([(id)attributedStringExceptionClass exceptionWithName:(id)ONSGenericException
                                                                            reason:reason
                                                                          userInfo:nil]);
    }
    size_t size = malloc_good_size(sizeof(id) * count);
    return malloc_type_calloc(1, size, 0x1F34E4E5);
}

// Native NSReallocateObjectArray, 0x1c0593bd8. External. Hex-Rays prints a void return, but assembly
// 0x1c0593c70 returns the new pointer in X0. Allocation failure returns NULL and keeps the old buffer
// allocated (kept). Copy min(malloc_size(old), requested bytes), zero the remaining good-size bytes, then
// free the old allocation only on success.
id *ONSReallocateObjectArray(id *objects, NSUInteger count) {
    if (OF_WORD_SHIFT(count, 61)) {
        id reason = [(id)attributedStringStringClass
            stringWithFormat:(id)OF_STR(
                                 "*** attempt to allocate id buffer with too large of a count (%lu) -- "
                                 "possibly data is corrupt"
                             ),
                             count];
        objc_exception_throw([(id)attributedStringExceptionClass exceptionWithName:(id)ONSGenericException
                                                                            reason:reason
                                                                          userInfo:nil]);
    }
    if (objects == NULL) {
        return ONSAllocateObjectArray(count);
    }
    size_t oldSize = malloc_size(objects);
    size_t requestedSize = sizeof(id) * count;
    size_t allocationSize = malloc_good_size(requestedSize);
    malloc_zone_t *zone = malloc_default_zone();
    id *allocation = malloc_type_zone_calloc(zone, 1, allocationSize, 0x9DA5E11C);
    if (allocation != NULL) {
        size_t copiedSize = oldSize < requestedSize ? oldSize : requestedSize;
        memmove(allocation, objects, copiedSize);
        if (allocationSize > copiedSize) {
            bzero((uint8_t *)allocation + copiedSize, allocationSize - copiedSize);
        }
        free(objects);
    }
    return allocation;
}
