//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <stdbool.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class __NSCFStreamWeakDelegateWrapper (__DATA,__objc_data 0x22e6cd220, was a private external), an
// NSObject subclass; class_ro_t 0x2227c1140: instance start 8, size 17; ivars _object (offset 8, eight bytes)
// and _useFallback (16, one byte), their offset symbols _OBJC_IVAR_$___NSCFStreamWeakDelegateWrapper._object
// and ._useFallback (were private externals) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The ivar type encodings lie outside the
// extracted image (unread); _object is the weak location _NSObjectStoreWeak manages, _useFallback its flag.
@interface __ONSCFStreamWeakDelegateWrapper : NSObject {
    id _object;
    bool _useFallback;
}

- (id)initWithDelegate:(id)delegate;
- (id)retainedDelegate;

@end
#endif
