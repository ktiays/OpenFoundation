//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSArray.h"

#ifdef __OBJC__
// The enumerator class -objectEnumerator allocates (__DATA,__objc_data 0x22e6ccb68, was a private external);
// not restored on this branch.
@class __ONSConstantArrayEnumerator;

// Native class NSConstantArray (__DATA,__objc_data 0x22e6ccbb8, external), an NSArray subclass, the class of
// the arrays the compiler emits for constant array literals; class_ro_t 0x2227bff30: flags 0, instance start
// 8, instance size 24, two 8-byte ivars, 8-byte aligned, _count at 8 (offset variable 0x22e6cbf6c) and
// _objects at 16 (offset variable 0x22e6cbf68); the ivar names come from the nm symbols
// _OBJC_IVAR_$_NSConstantArray._count and ._objects, while the names and type encodings in the ivar list lie
// in the shared cache outside the extracted image and are unread: the types are those of their use (a count
// read as a 64-bit word, a pointer to the object pointers). Its 18 instance and 3 class methods (the relative
// method lists 0x1c031b340 and 0x1c0319108, matched by implementation address) are restored in
// Objective-C/OCFNSConstantArray.m (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified). The image defines no category on NSConstantArray.
__attribute__((visibility("default")))
@interface ONSConstantArray : ONSArray {
    NSUInteger _count;
    const id *_objects;
}

+ (id)new;
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key;

- (id)retain;
- (oneway void)release;
- (id)autorelease;
- (BOOL)_tryRetain;
- (BOOL)_isDeallocating;
- (NSUInteger)retainCount;
- (id)copy;
- (id)mutableCopy;
- (id)init;
- (void)dealloc;

@end
#endif
