//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>
#include <OpenCoreFoundation/Date/OCFDate.h>

// Native __CFCalendarCreateUCalendar, 0x1c02aa958 (was a private external; hidden, native name;
// Locale/OCFCalendarICU.c): opens an ICU calendar (UCalendar *, an opaque pointer, spelled void *) for the
// calendar identifier (NULL keeps the locale identifier's own), the locale identifier and the time zone.
OF_EXTERN void *
__CFCalendarCreateUCalendar(OCFStringRef calendarID, OCFStringRef localeID, OCFTimeZoneRef tz);
