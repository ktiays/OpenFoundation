//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdint.h>

// Original names are preserved. All three were private externals and are non-external in the pinned
// image; no process-shared symbol is exported. Paths are UTF-16 unit buffers.
OF_EXTERN uint16_t _CFGetSlash(void);

OF_EXTERN OCFIndex _CFStartOfLastPathComponent(const uint16_t *unichars, OCFIndex length);

OF_EXTERN OCFIndex _CFLengthAfterDeletingLastPathComponent(const uint16_t *unichars, OCFIndex length);
