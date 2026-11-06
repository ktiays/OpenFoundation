//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "Foundation/Base/ONSFoundationSupport.h"

// NSProcessInfoThermalState, the SDK's NS_ENUM(NSInteger), spelled as the macro expands (the Swift process
// info's state holds it as its imported type, So25NSProcessInfoThermalStateV in the reflection metadata).
typedef enum __attribute__((
    enum_extensibility(open)
)) ONSProcessInfoThermalState : NSInteger ONSProcessInfoThermalState;

enum ONSProcessInfoThermalState : NSInteger {
    ONSProcessInfoThermalStateNominal = 0,
    ONSProcessInfoThermalStateFair = 1,
    ONSProcessInfoThermalStateSerious = 2,
    ONSProcessInfoThermalStateCritical = 3,
};
#ifdef __OBJC__
#import <objc/NSObject.h>

@class ONSArray;
@class ONSDictionary;
@class ONSString;

// Foundation's _NSSwiftProcessInfo (__DATA,__objc_data 0x22e6e2b70, was a private external), the Swift
// subclass that serves +processInfo; referenced by class symbol only and not restored on this branch.
@class _ONSSwiftProcessInfo;

// Native class NSProcessInfo (__DATA,__objc_data 0x22e6dc9c0, external); class_ro_t 0x22285ca60: instance
// start 8, size 48; ivars environment (offset 8, "@\"NSDictionary\""), arguments (16, "@\"NSArray\""),
// hostName (24,
// "@\"NSString\""), name (32, "@\"NSString\"") and automaticTerminationOptOutCounter (40, "q"), each eight
// bytes with eight-byte alignment (iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified). Its superclass (0x22e6b6088) lies outside the
// image; NSObject is the SDK hierarchy (confidence: medium). The property list (0x23d8930c1) lies outside the
// extracted image (unread).
__attribute__((visibility("default")))
// Native ivar-offset symbols are non-external in nm. @package preserves hidden linkage while allowing
// same-image helpers to use the read layout; the original source access-specifier spelling is unread.
@interface ONSProcessInfo : NSObject {
    @package
    ONSDictionary *environment;
    ONSArray *arguments;
    ONSString *hostName;
    ONSString *name;
    NSInteger automaticTerminationOptOutCounter;
}

+ (id)processInfo;
- (id)arguments;
- (id)processName;

@end

// Native category NSProcessInfo(NSProcessInfoHardwareState).
@interface ONSProcessInfo (NSProcessInfoHardwareState)

- (BOOL)isLowPowerModeEnabled;

@end
#endif

// Native __NSProcessInfoGetPowerState (0x1c05a2b7c) and __NSProcessInfoGetThermalState (0x1c05a2c70), hidden
// (were private externals): the low power mode state (0 or 1) and NSProcessInfoThermalState, -1 after a
// notify failure (ProcessInfo/ONSProcessInfo.m).
OF_EXTERN NSInteger _NSProcessInfoGetPowerState(void);
OF_EXTERN NSInteger _NSProcessInfoGetThermalState(void);
