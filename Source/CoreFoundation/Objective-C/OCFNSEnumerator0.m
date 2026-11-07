//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSEnumerator0.h"

// Inferred organization: the __NSEnumerator0 methods (+sharedInstance 0x1c0287814 through -autorelease
// 0x1c028786c) close the iOS Simulator __text run of the NSEnumerator unit (NSEnumerator.m by the class
// names); following the owner's decision, the class has its own translation unit, with its singleton.
// Objective-C compiled with manual reference counting (objc_retain, objc_release). The native source path is
// not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; the singleton read with
// Local/Scratch/cf-core/macho.py); iOS Device 24A446: unread. Reading: not yet written; the evidence is the
// assembly in Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.

// The __NSEnumerator0 class symbol, the singleton's isa.
extern char enumerator0Class[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSEnumerator0));

// Native __NSEnumerator0__struct, 0x2227a8428 (__DATA_CONST,__const, 16 bytes). Non-external: the empty
// enumerator singleton, an object whose isa is __NSEnumerator0, then a zero word.
static const struct {
    const void *isa;
    uintptr_t unused;
} __NSEnumerator0__struct = {enumerator0Class, 0};

// Native __NSEnumerator0__, 0x2227a8438 (__DATA_CONST,__const). Hidden (was a private external): the
// singleton's address.
id const __NSEnumerator0__ = (id)&__NSEnumerator0__struct;

@implementation __ONSEnumerator0

// Native +[__NSEnumerator0 sharedInstance], 0x1c0287814. Non-external. The singleton.
+ (id)sharedInstance {
    return (id)&__NSEnumerator0__struct;
}

// Native +[__NSEnumerator0 new], 0x1c0287820. Non-external. The singleton, retained (objc_retain, a tail
// call).
+ (id)new {
    return [(id)&__NSEnumerator0__struct retain];
}

// Native +[__NSEnumerator0 allocWithZone:], 0x1c028782c. Non-external. The singleton, retained (objc_retain,
// a tail call); the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return [(id)&__NSEnumerator0__struct retain];
}

// Native -[__NSEnumerator0 init], 0x1c0287838. Non-external. The receiver is released (objc_release); the
// singleton. No designated initializer is reached, so that diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

- (id)init {
    [self release];
    return (id)&__NSEnumerator0__struct;
}

#pragma clang diagnostic pop

// Native -[__NSEnumerator0 nextObject], 0x1c0287854. Non-external. nil.
- (id)nextObject {
    return nil;
}

// Native -[__NSEnumerator0 retainCount], 0x1c028785c. Non-external.
- (NSUInteger)retainCount {
    return UINTPTR_MAX;
}

// Native -[__NSEnumerator0 retain], 0x1c0287864. Non-external. The receiver (RET).
- (instancetype)retain {
    return self;
}

// Native -[__NSEnumerator0 release], 0x1c0287868. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[__NSEnumerator0 autorelease], 0x1c028786c. Non-external. The receiver (RET).
- (instancetype)autorelease {
    return self;
}

@end
