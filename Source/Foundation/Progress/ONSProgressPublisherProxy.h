//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>

// The NSProgress publisher proxy (Progress/ONSProgressPublisherProxy.m), iOS Simulator 24A434, arm64,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device 24A446: unread.

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native NSProgressPublisherProxy, 0x22e6d5e68 (non-external); class_ro_t flags 0x10 (not reproduced),
// instance start 8, size 56. Ivars: _forwarder (8, @"<NSProgressPublisher>"), _publisherID (16, @),
// _lowerCaseCategoryName (24, @"NSString"), _itemLocation (32, @"NSFileAccessNode"), _connection (40,
// @"NSXPCConnection") and _values (48, @"NSProgressValues"); the superclass is libobjc's NSObject. Only the
// methods the captured groups reach are declared.
__attribute__((visibility("hidden")))
@interface ONSProgressPublisherProxy : NSObject {
    id _forwarder;
    id _publisherID;
    id _lowerCaseCategoryName;
    id _itemLocation;
    id _connection;
    id _values;
}

- (id)initWithForwarder:(id)forwarder
           onConnection:(id)connection
            publisherID:(id)publisherID
                 values:(id)values;
- (void)dealloc;
- (id)descriptionWithIndenting:(id)indenting;
- (id)description;
@end

#endif
