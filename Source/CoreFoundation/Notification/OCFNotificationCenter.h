//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"

#include <dispatch/dispatch.h>
#include <stdint.h>

// CFNotificationCenter.h (iOS Simulator SDK interface), spelled OCF. Restored: the functions of the
// misc-services task list (Notification/OCFNotificationCenter.c); the rest of the unit (observer registration
// and removal, the distributed and Darwin centers, suspension) is not restored on this branch.
typedef struct __OCFNotificationCenter *OCFNotificationCenterRef;

typedef void (*OCFNotificationCallback)(
    OCFNotificationCenterRef center,
    void *observer,
    OCFStringRef name,
    const void *object,
    OCFDictionaryRef userInfo
);

OF_EXPORT const OCFStringRef kOCFNotificationAnyName;
OF_EXPORT const OCFStringRef kOCFNotificationAnyObject;
OF_EXPORT const OCFStringRef kOCFNotificationAnyObserver;

OF_EXPORT OCFNotificationCenterRef OCFNotificationCenterGetLocalCenter(void);
OF_EXPORT void OCFNotificationCenterPostNotification(
    OCFNotificationCenterRef center,
    OCFStringRef name,
    const void *object,
    OCFDictionaryRef userInfo,
    Boolean deliverImmediately
);
OF_EXPORT void OCFNotificationCenterPostNotificationWithOptions(
    OCFNotificationCenterRef center,
    OCFStringRef name,
    const void *object,
    OCFDictionaryRef userInfo,
    OCFOptionFlags options
);

// A notification center configuration (48 bytes; private, no SDK header). Layout read from
// _CFXNotificationCenterCreate (iOS Simulator 24A434, 0x1c01a886c, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC); confidence: verified for the fields it reads (kind, privileged,
// serviceName, endpoint). The kind is 0 for a local, 1 for a distributed and 2 for a Darwin center. The
// standard flag (1 in the standard configurations) and the alternate service name are read by functions
// outside this restoration (unread). Field names are local descriptive spellings.
typedef struct {
    uint32_t kind;
    uint32_t standard;
    uint8_t reserved8;
    uint8_t privileged;
    uint8_t reserved10[6];
    const char *serviceName;
    const char *alternateServiceName;
    void *endpoint;
    uint64_t reserved40;
} NotificationConfiguration;

_Static_assert(sizeof(NotificationConfiguration) == 48, "The native configuration is 48 bytes.");

// Native _kCFXNotificationConfigurationStandardLocalConfiguration, 0x1c0375170 (__TEXT,__const). External.
OF_EXPORT const NotificationConfiguration _kOCFXNotificationConfigurationStandardLocalConfiguration;

// Native _kCFXNotificationConfigurationStandardDarwinConfiguration, 0x1c03751d0. External.
OF_EXPORT const NotificationConfiguration _kOCFXNotificationConfigurationStandardDarwinConfiguration;

// Native _kCFXNotificationConfigurationStandardDistributedConfiguration, 0x2227a2258 (__DATA_CONST,__const).
// External.
OF_EXPORT const NotificationConfiguration _kOCFXNotificationConfigurationStandardDistributedConfiguration;

// A dependent notification's sub-notification (16 bytes; private, no SDK header): the name the center
// observes and where (0 the local, 1 the distributed, 2 the Darwin center, 3 a notify(3) registration), then
// padding. Read from __CFNotificationCenterInitializeDependentNotificationIfNecessary's block (0x1c01a93d4)
// and the records' data (iOS Simulator 24A434); confidence: verified. Field names are local descriptive
// spellings.
typedef struct {
    OCFStringRef name;
    uint8_t kind;
    uint8_t reserved[7];
} DependentSubnotification;

_Static_assert(sizeof(DependentSubnotification) == 16, "The native sub-notification is 16 bytes.");

// A dependent notification (56 bytes): the address of the notification name it stands for (its set key), the
// once token of its first registration, an optional initializer, the callback each sub-notification invokes,
// the suspension behavior those observers use, and its sub-notifications.
typedef struct DependentNotification {
    const OCFStringRef *name;
    dispatch_once_t *initOnce;
    void (*initialize)(void);
    OCFNotificationCallback callback;
    uint64_t suspensionBehavior;
    uint8_t count;
    uint8_t reserved41[7];
    const DependentSubnotification *subnotifications;
} DependentNotification;

_Static_assert(sizeof(DependentNotification) == 56, "The native dependent notification is 56 bytes.");

// CoreFoundation's own dependent notifications, defined beside their callbacks (Locale/OCFCalendar.c,
// Locale/OCFLocale.c, Date/OCFDate.c and Date/OCFTimeZone.c) and listed by __cfDependentNotifications.
// Non-external (each was a private external).
OF_EXTERN const DependentNotification __CFCalendarDayChangedDependentNotification;
OF_EXTERN const DependentNotification __CFLocaleDidChangeDependentNotification;
OF_EXTERN const DependentNotification __CFSystemClockDidChangeDependentNotification;
OF_EXTERN const DependentNotification __CFTimeZoneDidChangeDependentNotification;

// Native _CFXNotificationCenterCreate, 0x1c01a886c. External.
OF_EXPORT OCFNotificationCenterRef
_OCFXNotificationCenterCreate(OCFAllocatorRef allocator, const NotificationConfiguration *configuration);

// Native _CFXNotificationPostXPC, 0x1c01a9bf0. External. The object is a string; the user info is the binary
// property list data.
OF_EXPORT void _OCFXNotificationPostXPC(
    OCFNotificationCenterRef center,
    OCFStringRef name,
    OCFStringRef object,
    OCFDataRef userInfo,
    OCFStringRef entitlementKey,
    uint64_t options,
    uint64_t audience
);

// Native _CFXNotificationPost, 0x1c01aa0e0. External. The notification is an Objective-C object answering
// name, object and userInfo.
OF_EXPORT void
_OCFXNotificationPost(OCFNotificationCenterRef center, void *notification, OCFOptionFlags options);

// The CFNotificationCenter class record (type 52), for the builtin class table.
OF_EXTERN const struct __OCFRuntimeClass __CFNotificationCenterClass;

// The Objective-C classes the center unit messages: the notification class of this image (natively
// __CFNotification, not restored on this branch) and the Foundation operation classes it looks up by name.
#ifdef __OBJC__
@class __OCFNotification;
@class ONSOperationQueue;
@class ONSBlockOperation;
#endif

OF_EXPORT OCFNotificationCenterRef OCFNotificationCenterGetDarwinNotifyCenter(void);

// Native CFNotificationCenterGetDistributedCenter, 0x1c01a8b38. External.
OF_EXPORT OCFNotificationCenterRef OCFNotificationCenterGetDistributedCenter(void);

// Native _CFXNotificationGetTaskCenter, 0x1c01a8c60. External.
OF_EXPORT OCFNotificationCenterRef _OCFXNotificationGetTaskCenter(void);

// Native _CFNotificationCenterRegisterDependentNotificationList, 0x1c01a9200. External.
OF_EXPORT void
_OCFNotificationCenterRegisterDependentNotificationList(const DependentNotification *const *list);

// Native _CFNotificationCenterInitializeDependentNotificationIfNecessary, 0x1c01a9258. External.
OF_EXPORT void _OCFNotificationCenterInitializeDependentNotificationIfNecessary(OCFStringRef name);

// Native _CFXNotificationRegisterObserver, 0x1c01a961c. External.
OF_EXPORT uint64_t _OCFXNotificationRegisterObserver(
    OCFNotificationCenterRef center,
    OCFStringRef name,
    const void *object,
    OCFOptionFlags options,
    const void *observer,
    const void *callout,
    const void *context
);

// Native _CFXNotificationCancelToken, 0x1c01a9b44. External.
OF_EXPORT void _OCFXNotificationCancelToken(OCFNotificationCenterRef center, uint64_t token);

// Native _CFXNotificationRemoveObservers, 0x1c01a9b4c. External.
OF_EXPORT void _OCFXNotificationRemoveObservers(
    OCFNotificationCenterRef center, OCFStringRef name, const void *object, const void *observer
);

// Native _CFXNotificationCenterIsEmpty, 0x1c01aa688. External.
OF_EXPORT Boolean _OCFXNotificationCenterIsEmpty(OCFNotificationCenterRef center);

// Native _CFXNotificationCenterCopyDebugInfo, 0x1c01a8f5c. External.
OF_EXPORT OCFStringRef _OCFXNotificationCenterCopyDebugInfo(OCFNotificationCenterRef center);

// Native CFNotificationCenterAddObserver, 0x1c01aa920. External.
OF_EXPORT void OCFNotificationCenterAddObserver(
    OCFNotificationCenterRef center,
    const void *observer,
    OCFNotificationCallback callBack,
    OCFStringRef name,
    const void *object,
    OCFIndex suspensionBehavior
);
