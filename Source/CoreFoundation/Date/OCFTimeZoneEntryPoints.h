//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>
#include <OpenCoreFoundation/Date/OCFDate.h>

// CFTimeZone.h (iOS Simulator SDK interface), spelled OCF: the CFTimeZone entry points that forward to
// NSTimeZone, restored in Date/OCFTimeZoneEntryPoints.c (Date/OCFTimeZone.h declares the rest of the unit).
OF_EXPORT OCFTypeID OCFTimeZoneGetTypeID(void);
OF_EXPORT OCFTimeZoneRef OCFTimeZoneCopyDefault(void);
OF_EXPORT OCFTimeZoneRef
OCFTimeZoneCreateWithTimeIntervalFromGMT(OCFAllocatorRef allocator, OCFTimeInterval ti);
OF_EXPORT OCFStringRef OCFTimeZoneGetName(OCFTimeZoneRef tz);
