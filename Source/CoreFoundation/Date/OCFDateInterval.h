//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>
#include <OpenCoreFoundation/Date/OCFDate.h>

#include "CoreFoundation/Runtime/OCFRuntime.h"

// CFDateInterval.h (iOS Simulator SDK interface), spelled OCF. The unit's entry points (CFDateIntervalCreate
// and the rest, 0x1c028f154-0x1c028f418) are outside this restoration; only the class callbacks are restored.
typedef const struct __OCFDateInterval *OCFDateIntervalRef;

// The CFDateInterval object: the retained start date at offset 16 and the duration at offset 24 (the
// callbacks' loads). The struct tag follows OCFDateIntervalRef; the field names are local descriptive
// spellings.
struct __OCFDateInterval {
    OCFRuntimeBase base;
    OCFDateRef start;
    OCFTimeInterval duration;
};

// The CFDateInterval class record (type 71), iOS Simulator 24A434, 0x2227a84b8 (__DATA_CONST,__const). Hidden
// (was a private external); original name preserved.
OF_EXTERN const OCFRuntimeClass __CFDateIntervalClass;
