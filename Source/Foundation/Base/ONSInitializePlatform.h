//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>
#include <objc/objc.h>
#include <stdint.h>

// Foundation, iOS Simulator 24A434, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
OF_EXTERN void _NSInitializePlatform(void);
OF_EXTERN void _NSToDoAtProcessStart(void);
OF_EXTERN void _NSSetupDispatchDataBridge(void);

OF_EXPORT uint8_t ONSDebugEnabled;
OF_EXPORT uint8_t ONSZombieEnabled;
OF_EXPORT uint8_t ONSDeallocateZombies;

OF_EXTERN uint8_t _NSDoAPCache;
OF_EXTERN uint8_t _NSDOLoggingEnabled;
OF_EXTERN uint8_t _NSDOLoggingEnabled2;
