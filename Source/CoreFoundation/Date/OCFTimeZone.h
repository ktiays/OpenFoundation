//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>
#include <OpenCoreFoundation/Date/OCFDate.h>

// CFTimeZone.h (iOS Simulator SDK interface), spelled OCF; OCFTimeZoneRef is declared by the installed
// Date/OCFDate.h. Only CFTimeZoneGetSecondsFromGMT is restored on this branch (Date/OCFTimeZone.c).
OF_EXPORT OCFTimeInterval OCFTimeZoneGetSecondsFromGMT(OCFTimeZoneRef tz, OCFAbsoluteTime at);

// The Foundation class the system time zone change handler messages (natively NSTimeZone).
#ifdef __OBJC__
@class ONSTimeZone;
#endif

// Native kCFTimeZoneSystemTimeZoneDidChangeNotification, 0x2227a87e0. External.
OF_EXPORT const OCFStringRef kOCFTimeZoneSystemTimeZoneDidChangeNotification;
