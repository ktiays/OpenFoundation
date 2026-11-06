//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/XPC/ONSXPCClasses.h"

#import <objc/NSObject.h>
#include <xpc/xpc.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FX-swift-and-functions.

extern Class xpc_get_class4NSXPC(xpc_type_t type);

// Native __XPCObjectClass, 0x22e6fcdc0. Non-external.
Class _XPCObjectClass;

// Native ____initializeClasses_block_invoke, 0x1c06a9e34. Non-external.
// The shared-cache slot at 0x22241b9e8 binds to XPC_TYPE_NULL (confidence: medium).
dispatch_block_t const xpcInitializeClassesBlock = ^{
  _XPCObjectClass = [(id)xpc_get_class4NSXPC(XPC_TYPE_NULL) superclass];
};
