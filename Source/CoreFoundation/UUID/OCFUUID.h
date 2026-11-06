//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Runtime/OCFRuntime.h"

// SDK CFUUID.h type, spelled OCF.
typedef const struct __OCFUUID *OCFUUIDRef;

// CFUUID.h (iOS Simulator SDK interface), spelled OCF.
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
} OCFUUIDBytes;

OF_EXPORT OCFUUIDRef OCFUUIDCreateFromString(OCFAllocatorRef alloc, OCFStringRef uuidStr);
OF_EXPORT OCFStringRef OCFUUIDCreateString(OCFAllocatorRef alloc, OCFUUIDRef uuid);

// The CFUUID object: the sixteen bytes at offset 16, right after the object header (the native copies, the
// dictionary key &uuid->bytes and the description reads). The struct tag follows OCFUUIDRef; the field names
// are local descriptive spellings.
struct __OCFUUID {
    OCFRuntimeBase base;
    OCFUUIDBytes bytes;
};

// The CFUUID class record (type 34), iOS Simulator 24A434, 0x2227a5a20 (__DATA_CONST,__const). Hidden (was a
// private external); original name preserved.
OF_EXTERN const OCFRuntimeClass __CFUUIDClass;

// Entry points of the CFUUID unit (UUID/OCFUUID.c; external natively).
OF_EXPORT OCFTypeID OCFUUIDGetTypeID(void);
OF_EXPORT OCFUUIDBytes OCFUUIDGetUUIDBytes(OCFUUIDRef uuid);
OF_EXPORT OCFUUIDRef OCFUUIDCreateFromUUIDBytes(OCFAllocatorRef alloc, OCFUUIDBytes bytes);
