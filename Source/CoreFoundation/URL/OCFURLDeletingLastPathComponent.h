//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/URL/OCFURL.h"

// Native CFURLCreateCopyDeletingLastPathComponent, 0x1c024b248 (CFURL.c native
// unit; isolated here in URL/OCFURLDeletingLastPathComponent.c). External; SDK CFURL.h signature.
OF_EXPORT OCFURLRef OCFURLCreateCopyDeletingLastPathComponent(OCFAllocatorRef allocator, OCFURLRef url);
