//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFRunLoop.h>
#include <mach/port.h>

CF_EXTERN_C_BEGIN

typedef struct CF_BRIDGED_MUTABLE_TYPE(NSMachPort) __CFMachPort *CFMachPortRef;

typedef struct {
    CFIndex version;
    void *info;
    const void *(*retain)(const void *info);
    void (*release)(const void *info);
    CFStringRef (*copyDescription)(const void *info);
} CFMachPortContext;

typedef void (*CFMachPortCallBack)(CFMachPortRef port, void *msg, CFIndex size, void *info);
typedef void (*CFMachPortInvalidationCallBack)(CFMachPortRef port, void *info);

CF_EXPORT CFTypeID CFMachPortGetTypeID(void);
CF_EXPORT CFMachPortRef CFMachPortCreateWithPort(
    CFAllocatorRef allocator,
    mach_port_t portNum,
    CFMachPortCallBack callout,
    CFMachPortContext *context,
    Boolean *shouldFreeInfo
);
CF_EXPORT void CFMachPortInvalidate(CFMachPortRef port);
CF_EXPORT CFRunLoopSourceRef CFMachPortCreateRunLoopSource(
    CFAllocatorRef allocator,
    CFMachPortRef port,
    CFIndex order
);

CF_EXTERN_C_END
