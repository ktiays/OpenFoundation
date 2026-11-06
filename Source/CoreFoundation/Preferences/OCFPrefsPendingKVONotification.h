//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Preferences/OCFPreferences.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class _CFPrefsPendingKVONotification (__DATA,__objc_data 0x22e6cf110, was a private external), an
// NSObject subclass; class_ro_t 0x2227c7fb0: instance start 8, size 32; ivars source (offset 8), oldValues
// (16) and values (24), eight bytes each with eight-byte alignment, their offset symbols
// _OBJC_IVAR_$__CFPrefsPendingKVONotification.source ... (were private externals) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The ivar type encodings and visibility lie
// outside the extracted image (unread); the types follow their uses (the source an object, the value sets
// dictionaries, as -[_CFXPreferences _deliverPendingKVONotifications] reads them directly, hence @public).
// The three properties are atomic, retaining and direct: the native setters (-setSource:, -setOldValues:,
// -setValues:, was private externals) are the compiler's objc_setProperty_atomic calls with offsets 8, 16 and
// 24 after a nil-receiver check, and the getters are inlined into their callers (no getter is in the image).
@interface _OCFPrefsPendingKVONotification : NSObject {
    @public
    id source;
    OCFDictionaryRef oldValues;
    OCFDictionaryRef values;
}

@property(atomic, retain, direct) id source;
@property(atomic, retain, direct) __attribute__((NSObject)) OCFDictionaryRef oldValues;
@property(atomic, retain, direct) __attribute__((NSObject)) OCFDictionaryRef values;

@end
#endif
