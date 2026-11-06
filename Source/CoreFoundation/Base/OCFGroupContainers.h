//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/URL/OCFURL.h"

#include <sys/types.h>

// The application-group container lookup of the iOS Simulator CoreFoundation image, restored in
// Base/OCFGroupContainers.c.

// Native _CFCreateContainerURLForSecurityApplicationIdentifierGroupIdentifierAndUser, 0x1c0235d0c. External.
// The directory URL of the application-group container named groupIdentifier among the group containers of
// identifier for the user, or NULL. The parameter names are local descriptive spellings.
OF_EXPORT OCFURLRef _OCFCreateContainerURLForSecurityApplicationIdentifierGroupIdentifierAndUser(
    OCFStringRef identifier, OCFStringRef groupIdentifier, uid_t user
);
