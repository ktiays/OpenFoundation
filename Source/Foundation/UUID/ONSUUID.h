//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenFoundation/Base/ONSObjCRuntime.h>
#include "Foundation/Base/ONSFoundationSupport.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native NSUUID class 0x22e6e2320, class_ro_t 0x222871b28: instance start/size 8/8, no ivars.
// Superclass 0x22e6b6088 is an external class; NSObject (SDK hierarchy; confidence: medium).
// iOS Simulator Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; ivar reading verified.
@class __ONSConcreteUUID;
@class ONSCoder;
@class ONSString;

// The string and coder parameters carry their class so the Swift subclass (UUID/UUID.swift) overrides them
// with the native parameter types.
__attribute__((visibility("default")))
@interface ONSUUID : NSObject
+ (BOOL)_compatibilityBehavior;
+ (id)UUID;
- (id)init;
- (id)initWithUUIDString:(ONSString *)string;
- (id)initWithUUIDBytes:(const unsigned char *)bytes;
- (void)getUUIDBytes:(unsigned char *)bytes;
- (ONSString *)UUIDString;
- (ONSComparisonResult)compare:(ONSUUID *)other;
+ (BOOL)supportsSecureCoding;
- (id)initWithCoder:(ONSCoder *)coder;
- (void)encodeWithCoder:(ONSCoder *)coder;
- (Class)classForCoder;
@end

// Native class _NSUUIDBridge (__DATA,__objc_data 0x22e6e2370, class_ro_t 0x222871bf0, flags 0x10 hidden):
// instance start/size 8/8, no ivars; the superclass of the Swift __NSConcreteUUID (UUID/UUID.swift).
// iOS Simulator Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified.
@interface _ONSUUIDBridge : ONSUUID
@end
#endif
