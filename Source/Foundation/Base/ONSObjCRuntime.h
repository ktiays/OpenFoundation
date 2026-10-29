//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>

// Declaration ABI: Foundation NSObjCRuntime.h on the arm64 iOS SDK.
// The native range argument/return boundary is independently verified in
// NSUnionRange 0x1c05bd644 and NSIntersectionRange 0x1c05bd664, iOS Simulator
// 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// These declarations do not implement an Objective-C root class or runtime.
typedef signed long ONSInteger;
typedef unsigned long ONSUInteger;

typedef enum __attribute__((enum_extensibility(closed))) ONSComparisonResult : ONSInteger ONSComparisonResult;

enum ONSComparisonResult : ONSInteger {
    ONSOrderedAscending = -1,
    ONSOrderedSame = 0,
    ONSOrderedDescending = 1
};
