//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// Scenarios are written with native names. The reference build compiles them against the system
// framework; the replacement build compiles them in OF_SYSTEM_NAMES=YES against the restored framework,
// whose installed headers spell the same names (AGENTS.md, "Trace comparison").
#if OF_TRACE_REFERENCE
#include <CoreFoundation/CoreFoundation.h>
#else
#include <OpenCoreFoundation/OpenCoreFoundation.h>
#endif

// libobjc entry points, external to both builds.
void *objc_autoreleasePoolPush(void);

void objc_autoreleasePoolPop(void *context);
