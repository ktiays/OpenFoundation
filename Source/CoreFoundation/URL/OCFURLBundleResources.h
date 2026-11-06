//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/URL/OCFURL.h"

// Original private-external name is preserved (hidden; no process-shared symbol is exported).
OF_EXTERN OCFURLRef
_CFURLCreateResolvedDirectoryWithString(OCFAllocatorRef allocator, OCFStringRef URLString, OCFURLRef baseURL);
