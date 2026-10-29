//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdint.h>

// Scalar ABI verified in CFGregorianDateIsValid, iOS Simulator 24A434,
// 0x1c018dca8, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
typedef struct {
    int32_t year;
    int8_t month;
    int8_t day;
    int8_t hour;
    int8_t minute;
    double second;
} OCFGregorianDate;

OF_EXPORT uint8_t OCFGregorianDateIsValid(OCFGregorianDate gdate, OCFOptionFlags unitFlags);
