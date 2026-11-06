//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Stream/OCFStream.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class _CFStreamDelegate (__DATA,__objc_data 0x22e6cd8b0, was a private external), an NSObject
// subclass; class_ro_t 0x2227c26a8: instance start 8, size 64; ivars _flags (offset 8, eight bytes), _client
// (16, 40 bytes) and _cb (56, eight bytes), all with eight-byte alignment, their offset symbols
// _OBJC_IVAR_$__CFStreamDelegate._flags ... (were private externals) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The ivar type encodings lie outside the
// extracted image (unread); the types follow their sizes and uses: the event mask, the client context
// (CFStreamClientContext, 40 bytes) and the client callback (stream, event, info).
@interface _OCFStreamDelegate : NSObject {
    OCFOptionFlags _flags;
    OCFStreamClientContext _client;
    void (*_cb)(void *stream, OCFStreamEventType type, void *info);
}

- (id)initWithStreamEvents:(OCFOptionFlags)streamEvents
                  callback:(void (*)(void *stream, OCFStreamEventType type, void *info))callback
                   context:(OCFStreamClientContext *)context;

@end
#endif
