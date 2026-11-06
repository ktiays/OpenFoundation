//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/PlugIn/OCFBundle.h"

#include <stdatomic.h>
#include <os/lock.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

@class ONSMutableDictionary;
@class ONSString;

// Native NSBundle, 0x22e6d66d8. External. class_ro_t 0x2228493d0:
// NSObject superclass (shared-cache binding, medium confidence), instance start 8 and size 68.
__attribute__((visibility("default")))
@interface ONSBundle : NSObject {
    @package
    _Atomic(uint64_t) _flags;
    _Atomic(OCFBundleRef) _cfBundle;
    ONSMutableDictionary *_attributedStringTable;
    Class _principalClass;
    id _initialPath;
    id _resolvedPath;
    ONSString *_firstClassName;
    os_unfair_lock _lock;
}

+ (id)bundleWithPath:(id)path;
- (id)initWithPath:(id)path;
+ (id)bundleForClass:(Class)cls;
+ (id)mainBundle;
- (id)bundleURL;
- (id)preferredLocalizations;
- (id)developmentLocalization;

@end

// Native helpers (was private external).
OF_EXTERN id _NSFrameworkPathFromLibraryPath(id path);
OF_EXTERN id _NSBundlePathFromExecutablePath(id path);
OF_EXTERN void _NSBundleDeallocatingImmortalBundle(void);
OF_EXTERN void _NSInitBundleDidLoadNotification(void);
#endif
