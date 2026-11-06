//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Error/OCFError.h"

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/RunLoop/OCFRunLoop.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <os/log.h>
#include <stdbool.h>
#include <stdint.h>

// Foundation-image functions of the family FM-misc that other units of the family call: iOS Simulator 24A434,
// arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.

// Native _NSFoundationBundle, 0x1c05a1480 (external; Base/ONSFoundationBundle.m): the Foundation bundle,
// created once.
OF_EXPORT id _ONSFoundationBundle(void);

// Native __NSPropertyListClasses_ForArchival_ErrorAndException, 0x1c05a6b0c (external;
// Base/ONSPropertyListClasses.m): the set of property-list classes allowed when an error or exception is
// decoded, created once.
OF_EXPORT id __ONSPropertyListClasses_ForArchival_ErrorAndException(void);

// -----------------------------------------------------------------------------------------------------------
// Interfaces of families not yet restored on this branch.

#include "Foundation/Base/ONSUnrestoredInterfaces.h"
