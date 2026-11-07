//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFDictionary.h>

CF_EXTERN_C_BEGIN

typedef CFStringRef CFLocaleIdentifier CF_EXTENSIBLE_STRING_ENUM;
typedef CFStringRef CFLocaleKey CF_STRING_ENUM;

typedef const struct CF_BRIDGED_TYPE(NSLocale) __CFLocale *CFLocaleRef;

typedef CF_ENUM(CFIndex, CFLocaleLanguageDirection) {
    kCFLocaleLanguageDirectionUnknown = 0,
    kCFLocaleLanguageDirectionLeftToRight = 1,
    kCFLocaleLanguageDirectionRightToLeft = 2,
    kCFLocaleLanguageDirectionTopToBottom = 3,
    kCFLocaleLanguageDirectionBottomToTop = 4
};

CF_EXPORT CFTypeID CFLocaleGetTypeID(void);
CF_EXPORT CFLocaleRef CFLocaleGetSystem(void);
CF_EXPORT CFLocaleRef CFLocaleCopyCurrent(void);
CF_EXPORT CFLocaleIdentifier CFLocaleCreateCanonicalLanguageIdentifierFromString(
    CFAllocatorRef allocator,
    CFStringRef localeIdentifier
);
CF_EXPORT CFDictionaryRef CFLocaleCreateComponentsFromLocaleIdentifier(
    CFAllocatorRef allocator,
    CFLocaleIdentifier localeID
);
CF_EXPORT CFLocaleIdentifier CFLocaleCreateLocaleIdentifierFromComponents(
    CFAllocatorRef allocator,
    CFDictionaryRef dictionary
);
CF_EXPORT CFLocaleRef CFLocaleCreate(CFAllocatorRef allocator, CFLocaleIdentifier localeIdentifier);
CF_EXPORT CFTypeRef CFLocaleGetValue(CFLocaleRef locale, CFLocaleKey key);
CF_EXPORT CFLocaleIdentifier CFLocaleGetIdentifier(CFLocaleRef locale);
CF_EXPORT CFStringRef CFLocaleCopyDisplayNameForPropertyValue(
    CFLocaleRef displayLocale,
    CFLocaleKey key,
    CFStringRef value
);
CF_EXPORT CFLocaleLanguageDirection CFLocaleGetLanguageCharacterDirection(CFStringRef isoLangCode);

CF_EXTERN_C_END
