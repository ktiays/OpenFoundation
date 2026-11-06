//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "CoreFoundation/Objective-C/OCFNSArray.h"

#include <mach-o/loader.h>
#include <os/lock.h>

#ifdef __OBJC__
// Native _NSBundleDidLoadNotificationPayloadArray, 0x22e6d6688. Non-external. class_ro_t 0x222849078:
// NSArray superclass, instance start 8 and size 28.
@interface _ONSBundleDidLoadNotificationPayloadArray : ONSArray {
    @private
    const struct mach_header *_libraryHeader;
    ONSArray *_storage;
    os_unfair_lock _lock;
}

- (id)initWithHeader:(const struct mach_header *)header;

@end
#endif
