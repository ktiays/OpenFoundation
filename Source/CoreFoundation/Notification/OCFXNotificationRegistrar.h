//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <xpc/xpc.h>

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Base/OCFUtilities.h"

#include <stdint.h>

// The CFXNotification registrar (private; no SDK header). Layouts read from the iOS Simulator image (24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC) in the registrar functions and their callers; confidence:
// verified for every offset named here. Type and field names are local descriptive spellings.

typedef struct __OCFXNotificationRegistrar *OCFXNotificationRegistrarRef;

// An observer's callout: the callback (a block copied with _Block_copy for block observers, flag 0x20000; a
// selector for 0x40000; a C callback for 0x80000) and, for block observers, a retained CF object (the
// operation queue).
typedef struct {
    void *callout;
    OCFTypeRef context;
} NotificationHandler;

_Static_assert(sizeof(NotificationHandler) == 16, "The native handler is 16 bytes.");

// One matched observer (64 bytes): the name and object it matched (copies owned by the record), the observer
// (retained when flag 0x400 marks a weak observer), the observer flags, the handler, the token (observer
// index in the low 32 bits, generation in the high 32 bits) and the observer's context word.
typedef struct {
    OCFStringRef name;
    OCFTypeRef object;
    void *observer;
    uint64_t flags;
    NotificationHandler handler;
    uint64_t token;
    uint64_t context;
} NotificationRegistration;

_Static_assert(sizeof(NotificationRegistration) == 64, "The native registration record is 64 bytes.");

// The registration buffer the find functions fill: a count, a capacity, whether the records own their object
// (the registrar's option bit 1), and the records, initially the caller's inline storage that follows the
// header.
typedef struct {
    uint32_t count;
    uint32_t capacity;
    uint8_t copiesObjects;
    NotificationRegistration *records;
} NotificationRegistrationBuffer;

_Static_assert(sizeof(NotificationRegistrationBuffer) == 24, "The native buffer header is 24 bytes.");

// Native CFXNotificationHandlerCopy, 0x1c02bb134. External.
OF_EXPORT void
OCFXNotificationHandlerCopy(NotificationHandler *handler, void *callout, OCFTypeRef context, uint32_t flags);

// Native CFXNotificationHandlerRelease, 0x1c02bb18c. External.
OF_EXPORT void OCFXNotificationHandlerRelease(NotificationHandler *handler, uint32_t flags);

// Native CFXNotificationRegistrarCreate, 0x1c02bb4c4. External.
OF_EXPORT OCFXNotificationRegistrarRef
OCFXNotificationRegistrarCreate(OCFAllocatorRef allocator, uint64_t options);

// Native _CFXNotificationRegistrationDispose, 0x1c02bb524. External.
OF_EXPORT void
_OCFXNotificationRegistrationDispose(NotificationRegistration *registration, Boolean releaseObject);

// Native _CFXNotificationRegistrationBufferDestroy, 0x1c02bb5ec. External.
OF_EXPORT void _OCFXNotificationRegistrationBufferDestroy(NotificationRegistrationBuffer *buffer);

// Native CFXNotificationRegistrarGetOptions, 0x1c02bb51c. External.
OF_EXPORT uint64_t OCFXNotificationRegistrarGetOptions(OCFXNotificationRegistrarRef registrar);

// Native CFXNotificationRegistrarFind, 0x1c02bb664. External.
OF_EXPORT void OCFXNotificationRegistrarFind(
    OCFXNotificationRegistrarRef registrar,
    OCFStringRef name,
    OCFTypeRef object,
    NotificationRegistrationBuffer *buffer
);

// Native CFXNotificationRegistrarFindToken, 0x1c02bbf94. External.
OF_EXPORT Boolean OCFXNotificationRegistrarFindToken(
    OCFXNotificationRegistrarRef registrar, uint64_t token, NotificationRegistrationBuffer *buffer
);

// Native CFXNotificationRegistrarRemove, 0x1c02bc1d8. External.
OF_EXPORT void OCFXNotificationRegistrarRemove(
    OCFXNotificationRegistrarRef registrar, OCFStringRef name, OCFTypeRef object, void *observer
);

// Native CFXNotificationRegistrarRemoveToken, 0x1c02bc7c4. External.
OF_EXPORT void OCFXNotificationRegistrarRemoveToken(OCFXNotificationRegistrarRef registrar, uint64_t token);

// Native CFXNotificationRegistrarAdd, 0x1c02bc880. External.
OF_EXPORT uint64_t OCFXNotificationRegistrarAdd(
    OCFXNotificationRegistrarRef registrar,
    OCFStringRef name,
    OCFTypeRef object,
    void *observer,
    OCFOptionFlags flags,
    void *callout,
    OCFTypeRef context
);

// Native CFXNotificationRegisteredObserverSetNotifyToken, 0x1c02bda00. External.
OF_EXPORT void OCFXNotificationRegisteredObserverSetNotifyToken(
    OCFXNotificationRegistrarRef registrar, uint64_t token, int notifyToken
);

// Native CFXNotificationRegisteredObserverSetDistributedConnection, 0x1c02bda70. External.
OF_EXPORT void OCFXNotificationRegisteredObserverSetDistributedConnection(
    OCFXNotificationRegistrarRef registrar, uint64_t token, xpc_object_t connection
);

// Native CFXNotificationRegistrarEnumerate, 0x1c02bdb28. External.
#ifdef __BLOCKS__
OF_EXPORT void OCFXNotificationRegistrarEnumerate(
    OCFXNotificationRegistrarRef registrar, void (^block)(const NotificationRegistration *registration)
);
#endif

// Native CFXNotificationRegistrarIsEmpty, 0x1c02bdf5c. External.
OF_EXPORT Boolean OCFXNotificationRegistrarIsEmpty(OCFXNotificationRegistrarRef registrar);

// Native CFXNotificationRegistrarValidateToken, 0x1c02bdfac. External.
OF_EXPORT Boolean
OCFXNotificationRegistrarValidateToken(OCFXNotificationRegistrarRef registrar, uint64_t token);

// The registrar class record (type 70), for the builtin class table.
OF_EXTERN const struct __OCFRuntimeClass __CFXNotificationRegistrarClass;
