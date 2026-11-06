//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Locale/OCFLocale.h"
#include "CoreFoundation/Locale/OCFLocaleKeys.h"

// Private externals of the CFLocale unit restored in Locale/OCFLocaleEntryPoints.c (hidden, native names).
// _CFLocaleCreateCopyWithNewCalendarIdentifier ignores the allocator; the preference getters answer the
// object -[NSLocale _prefForKey:] returns, as _CFLocaleGetAppleICUNumberFormatStringsPref does
// (Locale/OCFLocale.h).
OF_EXTERN OCFLocaleRef _CFLocaleCreateCopyWithNewCalendarIdentifier(
    OCFAllocatorRef allocator, OCFLocaleRef locale, OCFStringRef calendarIdentifier
);
OF_EXTERN OCFTypeRef _CFLocaleGetAppleFirstWeekdayPref(OCFLocaleRef locale);
OF_EXTERN OCFTypeRef _CFLocaleGetAppleICUDateTimeSymbolsPref(OCFLocaleRef locale);
OF_EXTERN OCFTypeRef _CFLocaleGetAppleMinDaysInFirstWeekPref(OCFLocaleRef locale);
OF_EXTERN OCFTypeRef _CFLocaleGetAppleICUForce24HourTimePref(OCFLocaleRef locale);
OF_EXTERN OCFTypeRef _CFLocaleGetAppleICUForce12HourTimePref(OCFLocaleRef locale);
OF_EXTERN OCFTypeRef _CFLocaleGetAppleICUDateFormatStringsPref(OCFLocaleRef locale);
OF_EXTERN OCFTypeRef _CFLocaleGetAppleICUTimeFormatStringsPref(OCFLocaleRef locale);
