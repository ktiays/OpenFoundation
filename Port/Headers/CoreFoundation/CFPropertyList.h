//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFError.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFStream.h>

CF_EXTERN_C_BEGIN

typedef CF_OPTIONS(CFOptionFlags, CFPropertyListMutabilityOptions) {
    kCFPropertyListImmutable = 0,
    kCFPropertyListMutableContainers = 1 << 0,
    kCFPropertyListMutableContainersAndLeaves = 1 << 1
};

typedef CF_ENUM(CFIndex, CFPropertyListFormat) {
    kCFPropertyListOpenStepFormat = 1,
    kCFPropertyListXMLFormat_v1_0 = 100,
    kCFPropertyListBinaryFormat_v1_0 = 200
};

CF_EXPORT CFPropertyListRef CFPropertyListCreateDeepCopy(
    CFAllocatorRef allocator,
    CFPropertyListRef propertyList,
    CFOptionFlags mutabilityOption
);
CF_EXPORT Boolean CFPropertyListIsValid(CFPropertyListRef plist, CFPropertyListFormat format);
CF_EXPORT CFPropertyListRef CFPropertyListCreateWithData(
    CFAllocatorRef allocator,
    CFDataRef data,
    CFOptionFlags options,
    CFPropertyListFormat *format,
    CFErrorRef *error
);
CF_EXPORT CFIndex CFPropertyListWrite(
    CFPropertyListRef propertyList,
    CFWriteStreamRef stream,
    CFPropertyListFormat format,
    CFOptionFlags options,
    CFErrorRef *error
);
CF_EXPORT CFDataRef CFPropertyListCreateData(
    CFAllocatorRef allocator,
    CFPropertyListRef propertyList,
    CFPropertyListFormat format,
    CFOptionFlags options,
    CFErrorRef *error
);

CF_EXTERN_C_END
