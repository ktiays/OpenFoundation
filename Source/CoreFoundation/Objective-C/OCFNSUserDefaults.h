//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSUserDefaults (__DATA,__objc_data 0x22e6cf188, external), an NSObject subclass; class_ro_t
// 0x2227c80f8: instance start 8, size 40; ivars _kvo_ (offset 8), _identifier_ (16), _container_ (24) and
// _volatileDomainNames (32), each eight bytes with eight-byte alignment, their offset symbols
// _OBJC_IVAR_$_NSUserDefaults._kvo_ ... (were private externals) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The ivar type encodings lie outside the
// extracted image (unread; _identifier_ and _container_ share one encoding); the types follow their uses: the
// identifier and container hold CF strings the setters create, _volatileDomainNames is read through an atomic
// object property. The other NSUserDefaults methods are Foundation's.
@interface ONSUserDefaults : NSObject {
    id _kvo_;
    OCFStringRef _identifier_;
    OCFStringRef _container_;
    id _volatileDomainNames;
}

@property(readonly, retain) id volatileDomainNames;

@end
#endif
