//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>

typedef signed long OCFIndex;
typedef unsigned long OCFHashCode;
typedef unsigned long OCFOptionFlags;

typedef struct {
    OCFIndex location;
    OCFIndex length;
} OCFRange;

OF_EXPORT OCFRange __OCFRangeMake(OCFIndex location, OCFIndex length);
