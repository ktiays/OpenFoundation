//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// NSNotification, Foundation's class (outside the image: __CFNotification's superclass pointer 0x22e6db778 is
// not resolvable in it). Its identity is the class -[__CFNotification isEqual:] looks up by name
// ("NSNotification") and the SDK's NSNotification (confidence: medium); __CFNotification's instance start of
// 8 shows it has no ivars. Declared only as the superclass; never defined here.
@interface ONSNotification : NSObject
@end

// Native class __CFNotification (__DATA,__objc_data 0x22e6ccbe0, was a private external); class_ro_t
// 0x2227c0068: instance start 8, size 33; ivars _name (offset 8), _object (16), _userInfo (24), each eight
// bytes with eight-byte alignment, and _fouSemantics (32, one byte), their offset symbols
// _OBJC_IVAR_$___CFNotification._name ... (were private externals) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The ivar type encodings lie outside the
// extracted image (unread); the object ivars are spelled id and _fouSemantics BOOL (the byte
// -initWithName:object:userInfo:foundation: stores from its last argument).
@interface __OCFNotification : ONSNotification {
    id _name;
    id _object;
    id _userInfo;
    BOOL _fouSemantics;
}

- (id)name;
- (id)object;
- (id)userInfo;

@end
#endif
