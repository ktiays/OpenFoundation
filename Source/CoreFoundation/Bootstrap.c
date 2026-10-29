//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Bootstrap.h"

// An empty translation unit builds the framework without inventing native APIs.
_Static_assert(sizeof(void *) == 8, "The pinned arm64 ABI requires 64-bit pointers.");
