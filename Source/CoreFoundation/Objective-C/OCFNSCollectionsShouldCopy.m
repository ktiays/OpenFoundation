//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCollectionsShouldCopy.h"

// Inferred organization: __NSCollectionsShouldCopy (0x1c0287374) lies among the fast-enumeration helpers of
// the CoreFoundation image, between the methods that precede __NSFastEnumerationMutationHandler (0x1c028737c,
// restored in Objective-C/OCFException.m) and the __NSFastEnumerationEnumerator methods; that unit belongs to
// another sub-track, so the function has this translation unit, named for it, until the owner integrates it.
// Objective-C (the unit's language), manual reference counting. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json. iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// Native __NSCollectionsShouldCopy, 0x1c0287374. Hidden (was a private external). Answers NO (MOV W0, #0).
BOOL __NSCollectionsShouldCopy(void) {
    return NO;
}
