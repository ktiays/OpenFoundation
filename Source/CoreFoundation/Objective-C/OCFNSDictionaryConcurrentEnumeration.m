//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSDictionaryConcurrentEnumeration.h"

#include <dispatch/dispatch.h>

// Inferred organization: __NSCollectionHandleConcurrentEnumerationIfSpecified (0x1c02acc44) and its block
// ____NSCollectionHandleConcurrentEnumerationIfSpecified_block_invoke (0x1c02acd28) follow the
// __NSArrayI_Transfer methods in the iOS Simulator __text section and precede that unit's OUTLINED_FUNCTION
// pieces and _CFIsMatchingLibrary; the native unit (shared with the array classes) is not restored on this
// branch, so the function is kept in a translation unit of the dictionary family, which calls it. Objective-C
// blocks; no message sends. Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json. iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full).
// iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not
// written yet).

// Native __NSCollectionHandleConcurrentEnumerationIfSpecified, 0x1c02acc44, with its block
// ____NSCollectionHandleConcurrentEnumerationIfSpecified_block_invoke (0x1c02acd28, non-external). Hidden
// (was a private external). Without option bit 0, or with fewer than two active processors, NO. Otherwise
// dispatch_apply(iterations, NULL, ...) runs a block (the caller's block, a __block stop flag and usePool)
// that returns at once while the shared flag is set (acquire load), else calls the caller's block with the
// index and its own stop flag, inside an autorelease pool when usePool is 1, and sets the shared flag
// (release store) when the block set its own; then YES.
BOOL __NSCollectionHandleConcurrentEnumerationIfSpecified(
    NSUInteger options, BOOL usePool, size_t iterations, void (^block)(size_t index, BOOL *stop)
) {
    if ((options & 1) == 0 || __CFActiveProcessorCount() < 2) {
        return NO;
    }
    __block BOOL stop = NO;
    dispatch_apply(iterations, DISPATCH_APPLY_AUTO, ^(size_t index) {
      if (__atomic_load_n(&stop, __ATOMIC_ACQUIRE)) {
          return;
      }
      BOOL innerStop = NO;
      if (usePool == YES) {
          void *pool = _OCFAutoreleasePoolPush();
          block(index, &innerStop);
          _OCFAutoreleasePoolPop(pool);
      } else {
          block(index, &innerStop);
      }
      if (innerStop == YES) {
          __atomic_store_n(&stop, YES, __ATOMIC_RELEASE);
      }
    });
    return YES;
}
