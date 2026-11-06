//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/String/OCFString.h"

// Native __CFStringContainsNullCharacter, 0x1c01d8110: non-external in the property-list unit; hidden here
// with its native name until it joins that unit as a static function.
OF_EXTERN Boolean __CFStringContainsNullCharacter(OCFStringRef string);
