//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdint.h>

OF_EXPORT uint8_t OCFUniCharIsMemberOf(uint32_t character, uint32_t characterSet);

OF_EXPORT const uint8_t *OCFUniCharGetBitmapPtrForPlane(uint32_t characterSet, uint32_t plane);

OF_EXPORT OCFIndex OCFUniCharDecomposeCharacter(uint32_t character, uint32_t *converted, OCFIndex capacity);

OF_EXPORT uint8_t OCFUniCharFillDestinationBuffer(
    const uint32_t *characters,
    OCFIndex length,
    void **destination,
    OCFIndex capacity,
    OCFIndex *filledLength,
    uint32_t format
);
