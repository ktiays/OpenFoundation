//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFString.h>

CF_EXTERN_C_BEGIN

typedef CFStringRef CFErrorDomain CF_EXTENSIBLE_STRING_ENUM;

typedef struct CF_BRIDGED_TYPE(NSError) __CFError *CFErrorRef;

CF_EXPORT CFTypeID CFErrorGetTypeID(void);

CF_EXPORT const CFErrorDomain kCFErrorDomainPOSIX;
CF_EXPORT const CFErrorDomain kCFErrorDomainOSStatus;
CF_EXPORT const CFErrorDomain kCFErrorDomainCocoa;

CF_EXPORT const CFStringRef kCFErrorLocalizedDescriptionKey;
CF_EXPORT const CFStringRef kCFErrorLocalizedFailureReasonKey;
CF_EXPORT const CFStringRef kCFErrorLocalizedRecoverySuggestionKey;
CF_EXPORT const CFStringRef kCFErrorDescriptionKey;
CF_EXPORT const CFStringRef kCFErrorUnderlyingErrorKey;

CF_EXPORT CFErrorRef CFErrorCreate(
    CFAllocatorRef allocator,
    CFErrorDomain domain,
    CFIndex code,
    CFDictionaryRef userInfo
);
CF_EXPORT CFErrorRef CFErrorCreateWithUserInfoKeysAndValues(
    CFAllocatorRef allocator,
    CFErrorDomain domain,
    CFIndex code,
    const void *const *userInfoKeys,
    const void *const *userInfoValues,
    CFIndex numUserInfoValues
);
CF_EXPORT CFErrorDomain CFErrorGetDomain(CFErrorRef err);
CF_EXPORT CFIndex CFErrorGetCode(CFErrorRef err);
CF_EXPORT CFDictionaryRef CFErrorCopyUserInfo(CFErrorRef err);
CF_EXPORT CFStringRef CFErrorCopyDescription(CFErrorRef err);

CF_EXTERN_C_END
