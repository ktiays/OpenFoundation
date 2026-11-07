//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>

CF_EXTERN_C_BEGIN

typedef const struct CF_BRIDGED_TYPE(NSData) __CFData *CFDataRef;
typedef struct CF_BRIDGED_MUTABLE_TYPE(NSMutableData) __CFData *CFMutableDataRef;

CF_EXPORT CFTypeID CFDataGetTypeID(void);
CF_EXPORT CFDataRef CFDataCreate(CFAllocatorRef allocator, const UInt8 *bytes, CFIndex length);
CF_EXPORT CFDataRef CFDataCreateWithBytesNoCopy(
    CFAllocatorRef allocator,
    const UInt8 *bytes,
    CFIndex length,
    CFAllocatorRef bytesDeallocator
);
CF_EXPORT CFDataRef CFDataCreateCopy(CFAllocatorRef allocator, CFDataRef theData);
CF_EXPORT CFMutableDataRef CFDataCreateMutable(CFAllocatorRef allocator, CFIndex capacity);
CF_EXPORT CFMutableDataRef CFDataCreateMutableCopy(CFAllocatorRef allocator, CFIndex capacity, CFDataRef theData);
CF_EXPORT CFIndex CFDataGetLength(CFDataRef theData);
CF_EXPORT const UInt8 *CFDataGetBytePtr(CFDataRef theData);
CF_EXPORT UInt8 *CFDataGetMutableBytePtr(CFMutableDataRef theData);
CF_EXPORT void CFDataGetBytes(CFDataRef theData, CFRange range, UInt8 *buffer);
CF_EXPORT void CFDataSetLength(CFMutableDataRef theData, CFIndex length);
CF_EXPORT void CFDataAppendBytes(CFMutableDataRef theData, const UInt8 *bytes, CFIndex length);
CF_EXPORT void CFDataReplaceBytes(
    CFMutableDataRef theData,
    CFRange range,
    const UInt8 *newBytes,
    CFIndex newLength
);

CF_EXTERN_C_END
