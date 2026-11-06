//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenFoundation/Base/ONSObjCRuntime.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

@class ONSHashTable;
@class ONSLock;
@class ONSMapTable;
@class ONSPointerArray;

// Native __NSBundleTables, 0x22e6dc858. Was a private external. class_ro_t 0x22285c688:
// NSObject superclass (shared-cache binding, medium confidence), instance start 8 and size 64.
@interface __ONSBundleTables : NSObject {
    @private
    ONSLock *_lock;
    ONSHashTable *_staticFrameworks;
    ONSHashTable *_loadedBundles;
    ONSHashTable *_loadedFrameworks;
    ONSMapTable *_resolvedPathToBundles;
    ONSMapTable *_bundleForClassMap;
    ONSPointerArray *_immortalBundles;
}

+ (id)bundleTables;
- (void)removeBundle:(id)bundle forPath:(id)path type:(uint64_t)type __attribute__((objc_direct));
- (id)bundleForPath:(id)path __attribute__((objc_direct));
- (id)addBundleIfNeeded:(id)bundle
                forPath:(id)path
               withType:(uint64_t)type
             isImmortal:(BOOL)immortal __attribute__((objc_direct));
- (id)addBundle:(id)bundle
        forPath:(id)path
       withType:(uint64_t)type
       forClass:(Class)cls
     isImmortal:(BOOL)immortal __attribute__((objc_direct));
- (id)bundleForClass:(Class)cls __attribute__((objc_direct));
- (id)loadedBundles __attribute__((objc_direct));

@end
#endif
