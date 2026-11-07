//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDate.h>
#include <CoreFoundation/CFNotificationCenter.h>

CF_EXTERN_C_BEGIN

CF_EXPORT CFTypeID CFTimeZoneGetTypeID(void);
CF_EXPORT CFTimeZoneRef CFTimeZoneCopyDefault(void);
CF_EXPORT CFTimeZoneRef CFTimeZoneCreateWithTimeIntervalFromGMT(CFAllocatorRef allocator, CFTimeInterval ti);
CF_EXPORT CFStringRef CFTimeZoneGetName(CFTimeZoneRef tz);
CF_EXPORT CFTimeInterval CFTimeZoneGetSecondsFromGMT(CFTimeZoneRef tz, CFAbsoluteTime at);

CF_EXPORT const CFNotificationName kCFTimeZoneSystemTimeZoneDidChangeNotification;

CF_EXTERN_C_END
