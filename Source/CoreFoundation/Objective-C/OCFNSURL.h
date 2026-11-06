//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"

// Interfaces of families not yet restored on this branch
// Native __CFFullMethodName, 0x1c029eb84 (was a private external), in the Objective-C runtime glue: the
// "-[Class selector]" name of a method for the class, the object and the selector. Hidden; the parameter
// types are the NSURL fragments' call sites (confidence: medium).

#ifdef __OBJC__
// Native class NSURL (__DATA,__objc_data 0x22e6ce800, external), an NSObject subclass; class_ro_t
// 0x2227c5c98: instance start 8, size 40, four 8-byte ivars at 8, 16, 24 and 32 (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The ivar names and type encodings lie outside
// the extracted image and no restored method reads them: unread, size placeholders. Only the methods this
// track restores are defined; the class's other methods (Foundation's and the rest of CoreFoundation's) are
// unread.
__attribute__((visibility("default")))
@interface ONSURL : NSObject {
    uint64_t _unreadIvarAt8;
    uint64_t _unreadIvarAt16;
    uint64_t _unreadIvarAt24;
    uint64_t _unreadIvarAt32;
}

+ (id)__unurl;

@end

// Methods of NSURL the restored methods send to the receiver; defined outside this restoration (Foundation's
// part of the class and the unrestored CoreFoundation methods). Declarations only; the category is never
// implemented.
@interface ONSURL (NativeUnrestored)

- (id)_cfurl;
- (BOOL)isFileURL;
- (id)query;
- (id)absoluteString;
- (id)initFileURLWithPath:(id)path;

@end
#endif
