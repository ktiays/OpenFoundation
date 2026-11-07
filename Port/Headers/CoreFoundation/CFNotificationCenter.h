//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDictionary.h>

CF_EXTERN_C_BEGIN

typedef CFStringRef CFNotificationName CF_EXTENSIBLE_STRING_ENUM;

typedef struct CF_BRIDGED_MUTABLE_TYPE(id) __CFNotificationCenter *CFNotificationCenterRef;

typedef void (*CFNotificationCallback)(
    CFNotificationCenterRef center,
    void *observer,
    CFNotificationName name,
    const void *object,
    CFDictionaryRef userInfo
);

typedef CF_ENUM(CFIndex, CFNotificationSuspensionBehavior) {
    CFNotificationSuspensionBehaviorDrop = 1,
    CFNotificationSuspensionBehaviorCoalesce = 2,
    CFNotificationSuspensionBehaviorHold = 3,
    CFNotificationSuspensionBehaviorDeliverImmediately = 4
};

CF_EXPORT CFNotificationCenterRef CFNotificationCenterGetLocalCenter(void);
CF_EXPORT CFNotificationCenterRef CFNotificationCenterGetDistributedCenter(void);
CF_EXPORT CFNotificationCenterRef CFNotificationCenterGetDarwinNotifyCenter(void);
CF_EXPORT void CFNotificationCenterAddObserver(
    CFNotificationCenterRef center,
    const void *observer,
    CFNotificationCallback callBack,
    CFStringRef name,
    const void *object,
    CFNotificationSuspensionBehavior suspensionBehavior
);
CF_EXPORT void CFNotificationCenterRemoveObserver(
    CFNotificationCenterRef center,
    const void *observer,
    CFNotificationName name,
    const void *object
);
CF_EXPORT void CFNotificationCenterPostNotification(
    CFNotificationCenterRef center,
    CFNotificationName name,
    const void *object,
    CFDictionaryRef userInfo,
    Boolean deliverImmediately
);

enum {
    kCFNotificationDeliverImmediately = (1UL << 0),
    kCFNotificationPostToAllSessions = (1UL << 1)
};

CF_EXPORT void CFNotificationCenterPostNotificationWithOptions(
    CFNotificationCenterRef center,
    CFNotificationName name,
    const void *object,
    CFDictionaryRef userInfo,
    CFOptionFlags options
);

CF_EXTERN_C_END
