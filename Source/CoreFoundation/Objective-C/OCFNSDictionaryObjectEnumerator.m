//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSDictionaryObjectEnumerator.h"
#import "CoreFoundation/Objective-C/OCFNSDictionary.h"

// Inferred organization: -[__NSDictionaryObjectEnumerator nextObject] (0x1c01cc470) is the class's only
// method, after the dictionary enumeration helpers (Objective-C/OCFNSDictionaryEnumerate.m) in the iOS
// Simulator __text section; following the owner's decision, the class has its own translation unit.
// Objective-C compiled with manual reference counting (the method establishes none). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json. iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The offset of __NSFastEnumerationEnumerator's _origObj ivar
// (_OBJC_IVAR_$___NSFastEnumerationEnumerator._origObj, read with LDRSW), the enumerated collection; the ivar
// is not visible to a subclass in another unit's interface, so the offset symbol is read as natively.
extern const int32_t objectEnumeratorOriginalObjectOffset __asm(
    OF_SYMBOL("OBJC_IVAR_$_") OF_NAME_STRING(__ONSFastEnumerationEnumerator) "._origObj"
);

// The superclass method -nextObject sends to super (defined by Objective-C/OCFNSFastEnumerationEnumerator.m,
// whose interface does not declare it); declared only.
@interface __ONSFastEnumerationEnumerator (DictionaryObjectEnumeratorSuperclassMessages)

- (id)nextObject;

@end

@implementation __ONSDictionaryObjectEnumerator

// Native -[__NSDictionaryObjectEnumerator nextObject], 0x1c01cc470. Non-external. The superclass's next key;
// nil answers nil, otherwise the enumerated dictionary's -objectForKey: for it (a tail call).
- (id)nextObject {
    id key = [super nextObject];
    if (key == nil) {
        return nil;
    }
    id dictionary = *(id *)((char *)self + objectEnumeratorOriginalObjectOffset);
    return [(id<DictionaryClassMessages>)dictionary objectForKey:key];
}

@end
