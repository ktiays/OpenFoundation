//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSEnumerator.h"

#import "CoreFoundation/Objective-C/OCFNSMutableArray.h"
#import "CoreFoundation/Objective-C/OCFRequireConcreteImplementation.h"

// Inferred organization: -[NSEnumerator allObjects] (0x1c0287210) and -countByEnumeratingWithState:objects:
// count: (0x1c0287308) lie in the iOS Simulator __text section before __NSCollectionsShouldCopy, the
// __NSSingleObjectEnumerator and the __NSEnumerator0 methods (one native unit, NSEnumerator.m by the class
// names); -nextObject is in the cold region (0x1c030f218). Following the owner's decision, each class has its
// own translation unit. Objective-C compiled with manual reference counting (fast enumeration with
// objc_enumerationMutation). The native source path is not in the evidence. Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read in full; constants read with Local/Scratch/cf-core/macho.py); iOS Device 24A446: unread. Reading: not
// yet written; the evidence is the assembly in Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.

// The fast-enumeration state -countByEnumeratingWithState:objects:count: fills (the layout of
// NSFastEnumerationState; EnumeratorEnumerationState and its field names are local spellings).
typedef struct {
    unsigned long state;
    id *itemsPtr;
    unsigned long *mutationsPtr;
    unsigned long extra[5];
} EnumeratorEnumerationState;

// Native countByEnumeratingWithState:objects:count:.const_mu, 0x1c046c320 (__TEXT,__const; value 1, read
// with Local/Scratch/cf-core/macho.py). Non-external: the constant mutations word the enumeration reports.
static const unsigned long countByEnumeratingMutations = 1;

// The mutable array's -addObject:, declared for the message send only; no protocol object is emitted.
@protocol EnumeratorMessages

- (void)addObject:(id)object;

@end

@implementation ONSEnumerator

// Native -[NSEnumerator allObjects], 0x1c0287210. Non-external. A new +[NSMutableArray array] gets every
// object of a fast enumeration of the receiver (16 at a time, objc_enumerationMutation on a mutation).
- (id)allObjects {
    id array = [ONSMutableArray array];
    for (id object in (id)self) {
        [array addObject:object];
    }
    return array;
}

// Native -[NSEnumerator countByEnumeratingWithState:objects:count:], 0x1c0287308. Non-external.
// A finished state (state ~0) answers 0. A fresh state (0) reports the constant mutations word. Each call
// yields one -nextObject in the buffer's first slot (itemsPtr is the buffer) and advances the state; no
// object marks the state finished and answers 0.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    EnumeratorEnumerationState *enumerationState = (EnumeratorEnumerationState *)state;
    if (enumerationState->state == (unsigned long)-1) {
        return 0;
    }
    if (enumerationState->state == 0) {
        enumerationState->mutationsPtr = (unsigned long *)&countByEnumeratingMutations;
    }
    id object = [self nextObject];
    if (object == nil) {
        enumerationState->state = (unsigned long)-1;
        return 0;
    }
    enumerationState->itemsPtr = buffer;
    buffer[0] = object;
    enumerationState->state = enumerationState->state + 1;
    return 1;
}

// Native -[NSEnumerator nextObject], 0x1c030f218 (cold region). Non-external. The abstract method:
// __CFRequireConcreteImplementation of NSEnumerator (looked up by name), then nil.
- (id)nextObject {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSEnumerator)), self, _cmd);
    return nil;
}

@end
