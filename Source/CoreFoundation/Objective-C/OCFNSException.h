//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <stddef.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSException (__DATA,__objc_data 0x22e6cdfe0, external), an NSObject subclass; class_ro_t
// 0x2227c41c8: instance start 8, size 40; ivars name (offset 8), reason (16), userInfo (24) and reserved
// (32), each eight bytes with eight-byte alignment, their offset symbols _OBJC_IVAR_$_NSException.name ...
// (were private externals) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified). The ivar type encodings lie outside the extracted image (unread); the types follow the SDK's
// NSException.h (NSString *, NSString *, NSDictionary *, id), spelled id here because those classes are
// Foundation's.
// Exported, as the native image exports the class and its exception type record (_OBJC_EHTYPE_$_NSException).
__attribute__((visibility("default"), objc_exception))
@interface ONSException : NSObject {
    id name;
    id reason;
    id userInfo;
    id reserved;
}

- (id)initWithName:(id)aName reason:(id)aReason userInfo:(id)aUserInfo;
- (id)initWithName:(id)aName
            reason:(id)aReason
          userInfo:(id)aUserInfo
         osLogPack:(void *)osLogPack
              size:(size_t)size;
+ (id)exceptionWithName:(id)aName reason:(id)aReason userInfo:(id)aUserInfo;

@end
#endif
