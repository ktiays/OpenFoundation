//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSArray.h"

// The enumerator class -objectEnumerator allocates: native class __NSSingleObjectEnumerator
// (__DATA,__objc_data 0x22e6cdf40, was a private external); not restored on this branch.
@class __ONSSingleObjectEnumerator;

// Native class __NSSingleObjectArrayI (__DATA,__objc_data 0x22e6cecb0, was a private external), the immutable
// array of one object, an NSArray subclass; class_ro_t 0x2227c68e8: flags 0x10, instance start 8, size 16;
// one ivar (instance variable list 0x2227c68c0, eight-byte aligned): _object at 8 (8 bytes; offset variable
// 0x22e6cc2ac) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for the
// offset and size; the ivar name comes from the nm symbol _OBJC_IVAR_$___NSSingleObjectArrayI._object, the
// type encoding lies outside the extracted image).
@interface __ONSSingleObjectArrayI : ONSArray {
    @public
    id _object;
}
@end
#endif

// Native __NSSingleObjectArrayI_new, 0x1c02cf644 (was a private external;
// Objective-C/OCFNSSingleObjectArrayI.m): a new immutable array of the one object, retained unless noRetain.
OF_EXTERN id __NSSingleObjectArrayI_new(id object, BOOL noRetain);
