//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFURL.h>

CF_EXTERN_C_BEGIN

CF_EXPORT const CFStringRef kCFBundleVersionKey;
CF_EXPORT const CFStringRef kCFBundleIdentifierKey;
CF_EXPORT const CFStringRef kCFBundleDevelopmentRegionKey;

typedef struct CF_BRIDGED_TYPE(id) __CFBundle *CFBundleRef;

CF_EXPORT CFBundleRef CFBundleGetMainBundle(void);
CF_EXPORT CFBundleRef CFBundleGetBundleWithIdentifier(CFStringRef bundleID);
CF_EXPORT CFBundleRef CFBundleCreate(CFAllocatorRef allocator, CFURLRef bundleURL);
CF_EXPORT CFURLRef CFBundleCopyBundleURL(CFBundleRef bundle);
CF_EXPORT CFTypeRef CFBundleGetValueForInfoDictionaryKey(CFBundleRef bundle, CFStringRef key);
CF_EXPORT CFDictionaryRef CFBundleGetInfoDictionary(CFBundleRef bundle);
CF_EXPORT CFDictionaryRef CFBundleGetLocalInfoDictionary(CFBundleRef bundle);
CF_EXPORT CFStringRef CFBundleGetIdentifier(CFBundleRef bundle);
CF_EXPORT UInt32 CFBundleGetVersionNumber(CFBundleRef bundle);
CF_EXPORT CFStringRef CFBundleGetDevelopmentRegion(CFBundleRef bundle);
CF_EXPORT CFURLRef CFBundleCopyResourcesDirectoryURL(CFBundleRef bundle);
CF_EXPORT CFURLRef CFBundleCopyResourceURL(
    CFBundleRef bundle,
    CFStringRef resourceName,
    CFStringRef resourceType,
    CFStringRef subDirName
);
CF_EXPORT CFArrayRef CFBundleCopyResourceURLsOfType(
    CFBundleRef bundle,
    CFStringRef resourceType,
    CFStringRef subDirName
);
CF_EXPORT CFURLRef CFBundleCopyExecutableURL(CFBundleRef bundle);
CF_EXPORT Boolean CFBundleIsExecutableLoaded(CFBundleRef bundle);

CF_EXTERN_C_END
