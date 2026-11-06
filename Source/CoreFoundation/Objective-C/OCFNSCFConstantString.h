//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSCFString.h"

#ifdef __OBJC__
// Native class __NSCFConstantString (__DATA,__objc_data 0x22e6ccf78, was a private external; metaclass
// 0x22e6ccf28); class_ro_t 0x2227c0a80: flags 0x10 (RO_HIDDEN), instance start and size 8, no ivars, eight
// instance methods (relative method list 0x1c031f020), one protocol (list 0x2227c0a08) and one property (list
// 0x2227c0a68). Superclass __NSCFString (0x22e6ccf50). iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for the layout, read with
// Local/Scratch/cf-core/macho.py and the IDA capture
// Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json
// ("constant_string_class"). The protocol and the property name strings lie outside the extracted image;
// they are unread and not declared. The class object is also __CFConstantStringClassReference, the isa of
// every constant string (Objective-C/OCFNSCFConstantString.m).
@interface __ONSCFConstantString : __ONSCFString

- (id)copyWithZone:(struct _NSZone *)zone;
- (id)copy;
- (id)retain;
- (oneway void)release;
- (NSUInteger)retainCount;
- (id)autorelease;
- (BOOL)isNSCFConstantString__;
- (id)redactedDescription;

@end
#endif
