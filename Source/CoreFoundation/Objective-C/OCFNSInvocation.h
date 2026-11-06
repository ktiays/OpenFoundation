//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"

#include <stdint.h>

// Fields of NSMethodSignature's frame records read by forwarding and NSInvocation retention.
// The producer is separate. The field names describe the captured register uses.
typedef struct ForwardingArgumentRecord {
    const struct ForwardingArgumentRecord *type;
    const struct ForwardingArgumentRecord *next;
    uint32_t size;
    uint32_t memoryOffset;
    uint8_t unreadBytesAt24[4];
    uint32_t offset;
    uint8_t extraOffset;
    uint8_t unreadByteAt21;
    uint16_t flags;
} ForwardingArgumentRecord;

_Static_assert(__builtin_offsetof(ForwardingArgumentRecord, offset) == 0x1C, "The frame offset is at 0x1C.");
_Static_assert(__builtin_offsetof(ForwardingArgumentRecord, flags) == 0x22, "The flags are at 0x22.");

typedef struct {
    const ForwardingArgumentRecord *returnRecord;
    const ForwardingArgumentRecord *firstArgument;
} ForwardingFrameDescriptor;

#ifdef __OBJC__
// Native class NSInvocation (__DATA,__objc_data 0x22e6ce0d0, external), an NSObject subclass; class_ro_t
// 0x2227c45f8: instance start 8, size 54, ivars at 8, 16, 24, 32, 40 (8 bytes each), 48 (4), 52 (1) and 53
// (1) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The ivar names
// and type encodings lie outside the extracted image; the names below are local descriptive spellings and the
// types reflect the captured initialization, retention and destruction paths.
__attribute__((visibility("default")))
@interface ONSInvocation : NSObject {
    @package
    void *_frame;
    void *_buffer;
    id _signature;
    id _container;
    uint8_t *_argumentBackingFlags;
    uint32_t _magic;
    uint8_t _byteAt52;
    uint8_t _byteAt53;
}

- (id)_initWithMethodSignature:(id)signature
                         frame:(const void *)frame
                        buffer:(void *)buffer
                          size:(size_t)size;

+ (id)_invocationWithMethodSignature:(id)signature frame:(void *)frame;
- (void)retainArguments;

@end
#endif
