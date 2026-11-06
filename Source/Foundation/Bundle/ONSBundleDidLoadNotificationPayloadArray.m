//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Bundle/ONSBundleDidLoadNotificationPayloadArray.h"
#import "Foundation/Bundle/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSString.h"
#import "CoreFoundation/Objective-C/OCFNSMutableArray.h"

#include <stdlib.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device 24A446: comparison pending.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FB-bundle-paths-plist.
// Uniqued GOT binding (medium): NSMutableArray 0x22227fb18.

extern const char **objc_copyClassNamesForImageHeader(const struct mach_header *header, unsigned int *count);

@implementation _ONSBundleDidLoadNotificationPayloadArray

// Native -[_NSBundleDidLoadNotificationPayloadArray initWithHeader:], 0x1c04cd47c. Non-external.
- (id)initWithHeader:(const struct mach_header *)header {
    self = [super init];
    _lock = OS_UNFAIR_LOCK_INIT;
    _libraryHeader = header;
    return self;
}

// Native -[_NSBundleDidLoadNotificationPayloadArray dealloc], 0x1c04cd500. Non-external.
- (void)dealloc {
    [_storage release];
    [super dealloc];
}

// Native -[_NSBundleDidLoadNotificationPayloadArray _generateStorageLocked], 0x1c04cd57c. Non-external.
- (void)_generateStorageLocked {
    unsigned int count = 0;
    const char **names = objc_copyClassNamesForImageHeader(_libraryHeader, &count);
    ONSMutableArray *storage = [[ONSMutableArray alloc] initWithCapacity:count];
    for (ONSUInteger index = 0; index < count; ++index) {
        const char *name = names[index];
        if (name != NULL) {
            id string = [[ONSString alloc] initWithUTF8String:name];
            if (string != nil) {
                [storage addObject:string];
                [string release];
            }
        }
    }
    free(names);
    _storage = storage;
}

// Native -[_NSBundleDidLoadNotificationPayloadArray count], 0x1c04cd674. Non-external.
- (ONSUInteger)count {
    os_unfair_lock_lock(&_lock);
    if (_storage == nil) {
        [self _generateStorageLocked];
    }
    ONSUInteger count = [_storage count];
    os_unfair_lock_unlock(&_lock);
    return count;
}

// Native -[_NSBundleDidLoadNotificationPayloadArray objectAtIndex:], 0x1c04cd6d8. Non-external.
- (id)objectAtIndex:(ONSUInteger)index {
    os_unfair_lock_lock(&_lock);
    if (_storage == nil) {
        [self _generateStorageLocked];
    }
    id object = [_storage objectAtIndex:index];
    os_unfair_lock_unlock(&_lock);
    return object;
}

@end
