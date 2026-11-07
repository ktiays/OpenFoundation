//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSDictionary0.h"

#import "CoreFoundation/Objective-C/OCFNSEnumerator0.h"

// Inferred organization: the __NSDictionary0 methods (0x1c01e9448-0x1c01e94ac) lie together in the iOS
// Simulator __text section, and its trapping -init (0x1c030c26c) and -dealloc (0x1c030c270) in the cold
// region; following the owner's decision, the class has its own translation unit, which also defines the
// empty-dictionary singleton __NSDictionary0__struct (0x2227cc280, __DATA_CONST,__const_cfobj2: the isa
// __NSDictionary0 and a zero word, read with Local/Scratch/cf-core/macho.py; the section ends at 0x2227cc290)
// and its address __NSDictionary0__ (0x2227a4378, __DATA_CONST,__const). The restoration does not place the
// object in a __const_cfobj2 section. Objective-C compiled with manual reference counting (the unit's methods
// establish none; inherited from the dictionary units). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full).
// iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not
// written yet).

// The class symbols the singleton's isa and -keyEnumerator reference.
extern char dictionary0Class[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSDictionary0));
extern char enumerator0Class[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSEnumerator0));

// Native __NSDictionary0__struct, 0x2227cc280. External.
const OCFRuntimeBase __ONSDictionary0__struct = {(uintptr_t)dictionary0Class, 0};

// Native __NSDictionary0__, 0x2227a4378. External.
const OCFDictionaryRef __ONSDictionary0__ = (OCFDictionaryRef)&__ONSDictionary0__struct;

@implementation __ONSDictionary0

// Native +[__NSDictionary0 allocWithZone:], 0x1c01e9448. Non-external. The immutable placeholder (a tail
// call); the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSDictionaryImmutablePlaceholder();
}

// Native +[__NSDictionary0 automaticallyNotifiesObserversForKey:], 0x1c01e944c. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSDictionary0 getObjects:andKeys:count:], 0x1c01e9454. Non-external. Does nothing.
- (void)getObjects:(id *)objects andKeys:(id *)keys count:(NSUInteger)count {
}

// Native -[__NSDictionary0 copy], 0x1c01e9458. Non-external. The receiver, not retained.
- (id)copy {
    return self;
}

// Native -[__NSDictionary0 copyWithZone:], 0x1c01e945c. Non-external. The receiver, not retained.
- (id)copyWithZone:(struct _NSZone *)zone {
    return self;
}

// Native -[__NSDictionary0 retainCount], 0x1c01e9460. Non-external.
- (NSUInteger)retainCount {
    return UINTPTR_MAX;
}

// Native -[__NSDictionary0 retain], 0x1c01e9468. Non-external. The receiver.
- (instancetype)retain {
    return self;
}

// Native -[__NSDictionary0 release], 0x1c01e946c. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[__NSDictionary0 autorelease], 0x1c01e9470. Non-external. The receiver.
- (instancetype)autorelease {
    return self;
}

// Native -[__NSDictionary0 count], 0x1c01e9474. Non-external.
- (NSUInteger)count {
    return 0;
}

// Native -[__NSDictionary0 objectForKey:], 0x1c01e947c. Non-external.
- (id)objectForKey:(id)key {
    return nil;
}

// Native -[__NSDictionary0 keyEnumerator], 0x1c01e9484. Non-external. +[__NSEnumerator0 class]
// (objc_opt_class, its result unused), then the empty enumerator singleton __NSEnumerator0__.
- (id)keyEnumerator {
    [(Class)enumerator0Class class];
    return __NSEnumerator0__;
}

// Native -[__NSDictionary0 objectEnumerator], 0x1c01e94ac. Non-external. The body of -keyEnumerator.
- (id)objectEnumerator {
    [(Class)enumerator0Class class];
    return __NSEnumerator0__;
}

// Native -[__NSDictionary0 init], 0x1c030c26c (cold region). Non-external. A bare BRK #1, never reaching a
// designated initializer, so that diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

- (id)init {
    __builtin_trap();
}

#pragma clang diagnostic pop

// Native -[__NSDictionary0 dealloc], 0x1c030c270 (cold region). Non-external. A bare BRK #1; super is never
// reached, so the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
    __builtin_trap();
}

#pragma clang diagnostic pop

@end
