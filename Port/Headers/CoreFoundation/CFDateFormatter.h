//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDate.h>
#include <CoreFoundation/CFLocale.h>

CF_EXTERN_C_BEGIN

typedef CFStringRef CFDateFormatterKey CF_STRING_ENUM;

typedef struct __CFDateFormatter *CFDateFormatterRef;

typedef CF_ENUM(CFIndex, CFDateFormatterStyle) {
    kCFDateFormatterNoStyle = 0,
    kCFDateFormatterShortStyle = 1,
    kCFDateFormatterMediumStyle = 2,
    kCFDateFormatterLongStyle = 3,
    kCFDateFormatterFullStyle = 4
};

CF_EXPORT CFDateFormatterRef CFDateFormatterCreate(
    CFAllocatorRef allocator,
    CFLocaleRef locale,
    CFDateFormatterStyle dateStyle,
    CFDateFormatterStyle timeStyle
);
CF_EXPORT void CFDateFormatterSetFormat(CFDateFormatterRef formatter, CFStringRef formatString);
CF_EXPORT CFStringRef CFDateFormatterCreateStringWithDate(
    CFAllocatorRef allocator,
    CFDateFormatterRef formatter,
    CFDateRef date
);
CF_EXPORT CFStringRef CFDateFormatterCreateStringWithAbsoluteTime(
    CFAllocatorRef allocator,
    CFDateFormatterRef formatter,
    CFAbsoluteTime at
);
CF_EXPORT void CFDateFormatterSetProperty(CFDateFormatterRef formatter, CFStringRef key, CFTypeRef value);
CF_EXPORT CFTypeRef CFDateFormatterCopyProperty(CFDateFormatterRef formatter, CFDateFormatterKey key);

CF_EXTERN_C_END
