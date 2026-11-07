//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFConstantString.h"

// Inferred organization: the __NSCFConstantString methods (0x1c01b1908-0x1c01b192c) lie together in the iOS
// Simulator __text section, directly before the __NSCFString methods (0x1c01b1930); following the owner's
// decision, the class has its own translation unit. Objective-C compiled with manual reference counting (the
// bridged-class units'). Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json. Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).
//
// Native __CFConstantStringClassReference (0x22e6ccf78, __DATA,__objc_data, external) is the class object
// itself: nm lists it and _OBJC_CLASS_$___NSCFConstantString (was a private external) at the same address,
// two names for one class_t, as the capture's "data" section records it. The restoration reproduces that
// arrangement with an assembler alias: the module-level .set below makes __OCFConstantStringClassReference
// (spelled through OF_NAME_STRING, so the system-name build defines __CFConstantStringClassReference) a
// global symbol whose value is the class object clang emits for the class implementation, in the same
// section at the same offset. An alias keeps one object, so a constant string's isa is the restored class in
// both naming modes; a separate variable initialized with the class address would add a level of indirection
// the native constant strings do not have, and a linker -alias option would move the arrangement out of the
// source.
__asm__(".globl " OF_SYMBOL(OF_NAME_STRING(__OCFConstantStringClassReference)));
__asm__(".set " OF_SYMBOL(OF_NAME_STRING(__OCFConstantStringClassReference)) ", " OF_SYMBOL("OBJC_CLASS_$_")
            OF_NAME_STRING(__ONSCFConstantString));

@implementation __ONSCFConstantString

// Native -[__NSCFConstantString copyWithZone:], 0x1c01b1908. Non-external. Returns the receiver (RET).
- (id)copyWithZone:(struct _NSZone *)zone {
    return self;
}

// Native -[__NSCFConstantString copy], 0x1c01b190c. Non-external. Returns the receiver (RET).
- (id)copy {
    return self;
}

// Native -[__NSCFConstantString retain], 0x1c01b1910. Non-external. Returns the receiver (RET).
- (id)retain {
    return self;
}

// Native -[__NSCFConstantString release], 0x1c01b1914. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[__NSCFConstantString retainCount], 0x1c01b1918. Non-external. Answers NSUIntegerMax (MOV X0, #-1).
- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

// Native -[__NSCFConstantString autorelease], 0x1c01b1920. Non-external. Returns the receiver (RET).
- (id)autorelease {
    return self;
}

// Native -[__NSCFConstantString isNSCFConstantString__], 0x1c01b1924. Non-external. Answers YES.
- (BOOL)isNSCFConstantString__ {
    return YES;
}

// Native -[__NSCFConstantString redactedDescription], 0x1c01b192c. Non-external. Returns the receiver (RET).
- (id)redactedDescription {
    return self;
}

@end
