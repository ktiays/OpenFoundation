//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFString.h>

CF_EXTERN_C_BEGIN

typedef const struct __CFUUID *CFUUIDRef;

typedef struct {
    UInt8 byte0;
    UInt8 byte1;
    UInt8 byte2;
    UInt8 byte3;
    UInt8 byte4;
    UInt8 byte5;
    UInt8 byte6;
    UInt8 byte7;
    UInt8 byte8;
    UInt8 byte9;
    UInt8 byte10;
    UInt8 byte11;
    UInt8 byte12;
    UInt8 byte13;
    UInt8 byte14;
    UInt8 byte15;
} CFUUIDBytes;

CF_EXPORT CFTypeID CFUUIDGetTypeID(void);
CF_EXPORT CFUUIDRef CFUUIDCreate(CFAllocatorRef alloc);
CF_EXPORT CFUUIDRef CFUUIDCreateFromString(CFAllocatorRef alloc, CFStringRef uuidStr);
CF_EXPORT CFStringRef CFUUIDCreateString(CFAllocatorRef alloc, CFUUIDRef uuid);
CF_EXPORT CFUUIDBytes CFUUIDGetUUIDBytes(CFUUIDRef uuid);
CF_EXPORT CFUUIDRef CFUUIDCreateFromUUIDBytes(CFAllocatorRef alloc, CFUUIDBytes bytes);

CF_EXTERN_C_END
