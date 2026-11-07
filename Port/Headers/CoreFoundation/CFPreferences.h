//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFString.h>

CF_EXTERN_C_BEGIN

CF_EXPORT const CFStringRef kCFPreferencesAnyApplication;
CF_EXPORT const CFStringRef kCFPreferencesCurrentApplication;
CF_EXPORT const CFStringRef kCFPreferencesAnyHost;
CF_EXPORT const CFStringRef kCFPreferencesCurrentHost;
CF_EXPORT const CFStringRef kCFPreferencesAnyUser;
CF_EXPORT const CFStringRef kCFPreferencesCurrentUser;

CF_EXPORT CFPropertyListRef CFPreferencesCopyAppValue(CFStringRef key, CFStringRef applicationID);
CF_EXPORT Boolean CFPreferencesGetAppBooleanValue(
    CFStringRef key,
    CFStringRef applicationID,
    Boolean *keyExistsAndHasValidFormat
);
CF_EXPORT CFIndex CFPreferencesGetAppIntegerValue(
    CFStringRef key,
    CFStringRef applicationID,
    Boolean *keyExistsAndHasValidFormat
);
CF_EXPORT void CFPreferencesSetAppValue(CFStringRef key, CFPropertyListRef value, CFStringRef applicationID);
CF_EXPORT Boolean CFPreferencesAppSynchronize(CFStringRef applicationID);
CF_EXPORT CFPropertyListRef CFPreferencesCopyValue(
    CFStringRef key,
    CFStringRef applicationID,
    CFStringRef userName,
    CFStringRef hostName
);
CF_EXPORT CFDictionaryRef CFPreferencesCopyMultiple(
    CFArrayRef keysToFetch,
    CFStringRef applicationID,
    CFStringRef userName,
    CFStringRef hostName
);

CF_EXTERN_C_END
