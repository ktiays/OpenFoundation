//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenFoundation/Base/ONSObjCRuntime.h>

typedef struct _ONSRange {
    ONSUInteger location;
    ONSUInteger length;
} ONSRange;

OF_EXPORT ONSRange ONSUnionRange(ONSRange range1, ONSRange range2);

OF_EXPORT ONSRange ONSIntersectionRange(ONSRange range1, ONSRange range2);
