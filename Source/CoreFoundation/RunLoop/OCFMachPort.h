//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/RunLoop/OCFMachPortLifetime.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <dispatch/dispatch.h>
#include <mach/mach.h>
#include <os/lock.h>
#include <stddef.h>

// Public CFMachPort interface, iOS Simulator SDK CFMachPort.h, spelled with the OCF prefix.
typedef struct __OCFMachPort *OCFMachPortRef;

typedef struct {
    OCFIndex version;
    void *info;
    const void *(*retain)(const void *info);
    void (*release)(const void *info);
    OCFStringRef (*copyDescription)(const void *info);
} OCFMachPortContext;

typedef void (*OCFMachPortCallBack)(OCFMachPortRef port, void *msg, OCFIndex size, void *info);

// The CFMachPort object (type 36), iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified for the offsets read by RunLoop/OCFMachPort.c (state 16, port 20, dispatch source 24,
// invalidation callback 32, run loop source 40, callout 48, context 56, lock 96, retain 104, release 112);
// the object's size is unread. The state is 0 while valid, 1 while invalidating, 2 invalidated and 3
// deallocated. The field names are local descriptive spellings.
typedef void (*OCFMachPortInvalidationCallBack)(OCFMachPortRef port, void *info);

struct __OCFMachPort {
    OCFRuntimeBase base;
    int32_t state;
    mach_port_t port;
    dispatch_source_t dispatchSource;
    OCFMachPortInvalidationCallBack invalidationCallBack;
    struct __OCFRunLoopSource *runLoopSource;
    OCFMachPortCallBack callout;
    OCFMachPortContext context;
    os_unfair_lock lock;
    uint32_t unread1;
    const void *(*retain)(const void *info);
    void (*release)(const void *info);
};

_Static_assert(offsetof(struct __OCFMachPort, port) == 20, "The port name is at 20.");
_Static_assert(offsetof(struct __OCFMachPort, runLoopSource) == 40, "The run loop source is at 40.");
_Static_assert(offsetof(struct __OCFMachPort, lock) == 96, "The mach port lock is at 96.");
_Static_assert(offsetof(struct __OCFMachPort, release) == 112, "The release callback is at 112.");

OF_EXPORT OCFTypeID OCFMachPortGetTypeID(void);

// Hidden (were private externals): the version 1 source perform callback of mach ports and its describer.
OF_EXTERN void *__CFMachPortPerform(void *msg, OCFIndex size, OCFAllocatorRef allocator, void *info);
OF_EXTERN void __CFDescribeCFMachPortPerformCallout(
    void *info, char *libraryName, size_t libraryNameSize, char *functionName, size_t functionNameSize
);

// The builtin class record of this unit (type 36).
OF_EXTERN const OCFRuntimeClass __CFMachPortClass;

// External natively (the mach messaging helper of this cluster).
OF_EXPORT void __OCFMachMessageCheckForAndDestroyUnsentMessage(kern_return_t ret, mach_msg_header_t *msg);
