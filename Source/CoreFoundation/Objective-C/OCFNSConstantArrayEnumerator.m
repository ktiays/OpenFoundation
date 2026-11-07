//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSConstantArrayEnumerator.h"
#include "CoreFoundation/Objective-C/OCFNSArrayI.h"

#include <stdint.h>

// Inferred organization: the __NSConstantArrayEnumerator methods lie together in the iOS Simulator __text
// section (0x1c01a36f8-0x1c01a3797), just before the NSConstantArray methods; following the owner's decision,
// the class has its own translation unit. The methods follow in native address order. Objective-C compiled
// with manual reference counting (objc_autorelease). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json (family H-array-concrete). Unless a definition
// says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The offset variables of NSConstantArray's ivars (_OBJC_IVAR_$_NSConstantArray._count, 0x22e6cbf6c, and
// ._objects, 0x22e6cbf68; read with LDRSW), defined by Objective-C/OCFNSConstantArray.m; the ivars are not
// public in that unit's interface, so they are read through their offsets as the native code does.
extern const int32_t constantArrayCountOffset __asm(OF_SYMBOL("OBJC_IVAR_$_")
                                                        OF_NAME_STRING(ONSConstantArray) "._count");
extern const int32_t constantArrayObjectsOffset __asm(OF_SYMBOL("OBJC_IVAR_$_")
                                                          OF_NAME_STRING(ONSConstantArray) "._objects");

// The constant array's count and objects. constantArrayCount and constantArrayObjects are local descriptive
// names.
static inline NSUInteger constantArrayCount(id array) {
    return *(NSUInteger *)((char *)array + constantArrayCountOffset);
}

static inline const id *constantArrayObjects(id array) {
    return *(const id **)((char *)array + constantArrayObjectsOffset);
}

@implementation __ONSConstantArrayEnumerator

// Native -[__NSConstantArrayEnumerator initWithConstantArray:], 0x1c01a36f8. Non-external. Stores the array
// (not retained) and answers the receiver; super's initializer is not sent.
- (id)initWithConstantArray:(id)array {
    _underlyingArray = array;
    return self;
}

// Native -[__NSConstantArrayEnumerator allObjects], 0x1c01a3708. Non-external. __NSArrayI_new of all the
// constant array's objects (from index 0, not from _index; retaining), autoreleased (a tail call).
- (id)allObjects {
    id array = _underlyingArray;
    return [__NSArrayI_new(constantArrayObjects(array), NULL, constantArrayCount(array), NO) autorelease];
}

// Native -[__NSConstantArrayEnumerator nextObject], 0x1c01a3748. Non-external. Below the count the object at
// _index is answered and _index advances; otherwise nil.
- (id)nextObject {
    NSUInteger index = _index;
    id array = _underlyingArray;
    if (index >= constantArrayCount(array)) {
        return nil;
    }
    const id *objects = constantArrayObjects(array);
    _index = index + 1;
    return objects[index];
}

@end
