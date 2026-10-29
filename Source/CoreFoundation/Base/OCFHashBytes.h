//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdint.h>

// Exported by the reference image; not part of the installed public header surface.
OF_EXPORT OCFHashCode OCFHashBytes(const uint8_t *bytes, OCFIndex length);
