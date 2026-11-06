//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

// Native NSAllocateObjectArray, 0x1c0593b60, and NSReallocateObjectArray, 0x1c0593bd8, external. iOS
// Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified.
// iOS Device 24A446: unread. These allocate unretained id slots, not Objective-C collection objects.
OF_EXPORT id *ONSAllocateObjectArray(NSUInteger count);
OF_EXPORT id *ONSReallocateObjectArray(id *objects, NSUInteger count);
