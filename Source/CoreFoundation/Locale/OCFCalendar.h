//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <dispatch/dispatch.h>
#include <stdint.h>

// The CFCalendar functions restored on this branch (Locale/OCFCalendar.c).

// Native _CFCalendarResetCurrent (0x1c02a8118), external.
OF_EXPORT void _OCFCalendarResetCurrent(void);

// Native __postAndResetMidnight (0x1c02a812c), was a private external. Its callers pass two Booleans (the
// day-changed notification passes a CFEqual result as the second).
OF_EXTERN void __postAndResetMidnight(Boolean postIfEarlier, Boolean postIfLater);

// Native __initDayChangedNotification, 0x1c02a82a4. Hidden (was a private external).
OF_EXTERN void __initDayChangedNotification(void);

// Native NSCalendarDayChangedNotification, 0x2227a8718. External.
OF_EXPORT const OCFStringRef ONSCalendarDayChangedNotification;
