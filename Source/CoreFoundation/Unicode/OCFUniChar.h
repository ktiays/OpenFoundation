//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
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

OF_EXPORT const uint8_t *OCFUniCharGetUnicodePropertyDataForPlane(uint32_t propertyType, uint32_t plane);

OF_EXPORT void OCFUniCharPrioritySort(uint32_t *characters, OCFIndex length);

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN uint32_t CFUniCharGetNumberOfPlanes(uint32_t characterSet);

OF_EXTERN uint8_t
CFUniCharGetBitmapForPlane(uint32_t characterSet, uint32_t plane, void *bitmap, bool isInverted);
