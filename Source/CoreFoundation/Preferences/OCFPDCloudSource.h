//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Preferences/OCFPreferences.h"

// C2 supplies this native CFSet applier, 0x1c01cbb44.
OF_EXTERN void remoteStoreChanged(const void *source, void *context);

#ifdef __OBJC__
#import "CoreFoundation/Preferences/OCFPDSource.h"

// Native CFPDCloudSource, class_ro_t 0x2227c1bb0. CFPDContainerSource subclass, size 168.
// Its two added ivars are at offsets 152 and 160; native type strings are outside the extracted image.
@interface OCFPDCloudSource : OCFPDContainerSource {
    @package
    id cloudSource;
    OCFStringRef _configurationPath;
}

- (instancetype)initWithDomain:(OCFStringRef)domain
                      userName:(OCFStringRef)user
                     storeName:(xpc_object_t)store
             configurationPath:(OCFStringRef)configuration
                 containerPath:(OCFStringRef)container
                    shmemIndex:(int16_t)index
                        daemon:(OCFPrefsDaemon *)daemon;

- (BOOL)synchronizeWithCloud;
- (id)copyPropertyListWithoutDrainingPendingChangesValidatingPlist:(BOOL)validate
    __attribute__((objc_direct));

@end
#endif
