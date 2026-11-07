//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Bundle/ONSBundleTables.h"
#import "Foundation/Bundle/ONSUnrestoredInterfaces.h"
#import "Foundation/Base/ONSUnrestoredInterfaces.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"

#include <dispatch/dispatch.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device 24A446: comparison pending.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FB-bundle-paths-plist.

extern id objc_opt_self(id object);
extern id objc_opt_new(Class cls);
extern char bundleTablesClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSBundleTables));
extern char bundleLockClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSLock));

// Native __MergedGlobals_101 and qword_22E76C960.
static __ONSBundleTables *tables;
static dispatch_once_t tablesOnce;

@interface __ONSBundleTables ()
- (id)_addBundleLocked:(id)bundle
               forPath:(id)path
              withType:(uint64_t)type
              forClass:(Class)cls
            isImmortal:(BOOL)immortal __attribute__((objc_direct));
@end

@implementation __ONSBundleTables

// Native +[__NSBundleTables bundleTables], 0x1c059edfc. Non-external.
+ (id)bundleTables {
    objc_opt_self(self);
    dispatch_once(&tablesOnce, ^{
      // Native ___32+[__NSBundleTables bundleTables]_block_invoke, 0x1c059ee44. Non-external.
      tables = objc_alloc_init((Class)bundleTablesClass);
    });
    return objc_autorelease(objc_retain(tables));
}

// Native -[__NSBundleTables init], 0x1c059ee68. Non-external.
- (id)init {
    self = [super init];
    _immortalBundles = [[ONSPointerArray alloc] initWithOptions:258];
    _lock = objc_opt_new((Class)bundleLockClass);
    _staticFrameworks = [[ONSHashTable alloc] initWithOptions:5 capacity:0];
    _loadedBundles = [[ONSHashTable alloc] initWithOptions:5 capacity:0];
    _loadedFrameworks = [[ONSHashTable alloc] initWithOptions:5 capacity:0];
    _resolvedPathToBundles = [[ONSMapTable alloc] initWithKeyOptions:0 valueOptions:5 capacity:0];
    _bundleForClassMap = [[ONSMapTable alloc] initWithKeyOptions:256 valueOptions:5 capacity:0];
    return self;
}

// Native -[__NSBundleTables dealloc], 0x1c059ef94. Non-external.
- (void)dealloc {
    objc_release(_staticFrameworks);
    objc_release(_loadedBundles);
    objc_release(_loadedFrameworks);
    objc_release(_resolvedPathToBundles);
    objc_release(_bundleForClassMap);
    objc_release(_immortalBundles);
    objc_release(_lock);
    [super dealloc];
}

// Native -[__NSBundleTables removeBundle:forPath:type:], 0x1c059f038. Non-external, direct.
- (void)removeBundle:(id)bundle forPath:(id)path type:(uint64_t)type {
    [_lock lock];
    if (path != nil && [_resolvedPathToBundles objectForKey:path] != nil) {
        [_resolvedPathToBundles removeObjectForKey:path];
        objc_release(bundle);
    }
    switch (type) {
        case 0x10000:
        case 0x20000:
            if ([_loadedBundles containsObject:bundle]) {
                [_loadedBundles removeObject:bundle];
                objc_release(bundle);
            }
            break;
        case 0x40000:
            if ([_loadedFrameworks containsObject:bundle]) {
                [_loadedFrameworks removeObject:bundle];
                objc_release(bundle);
            }
            break;
        default:
            break;
    }
    [_lock unlock];
}

// Native -[__NSBundleTables bundleForPath:], 0x1c059f0f4. Non-external, direct.
- (id)bundleForPath:(id)path {
    [_lock lock];
    id bundle = objc_autorelease(objc_retain([_resolvedPathToBundles objectForKey:path]));
    [_lock unlock];
    return bundle;
}

// Native -[__NSBundleTables addBundleIfNeeded:forPath:withType:isImmortal:], 0x1c059f144.
// Non-external, direct. Autorelease a hit after unlocking, unlike the simple lookup.
- (id)addBundleIfNeeded:(id)bundle forPath:(id)path withType:(uint64_t)type isImmortal:(BOOL)immortal {
    [_lock lock];
    id existing = objc_retain([_resolvedPathToBundles objectForKey:path]);
    if (existing != nil) {
        [_lock unlock];
        return objc_autorelease(existing);
    }
    id result = [self _addBundleLocked:bundle forPath:path withType:type forClass:Nil isImmortal:immortal];
    [_lock unlock];
    return result;
}

// Native -[__NSBundleTables _addBundleLocked:forPath:withType:forClass:isImmortal:], 0x1c059f1f0.
// Non-external, direct. The native type dispatch rotates (type - 0x10000) right by 16 bits.
- (id)_addBundleLocked:(id)bundle
               forPath:(id)path
              withType:(uint64_t)type
              forClass:(Class)cls
            isImmortal:(BOOL)immortal {
    ONSHashTable **table = NULL;
    switch (type) {
        case 0x10000:
        case 0x40000:
            table = &_loadedFrameworks;
            break;
        case 0x20000:
            table = &_loadedBundles;
            break;
        case 0x30000:
            table = &_staticFrameworks;
            break;
        default:
            break;
    }
    if (table != NULL && ![*table containsObject:bundle]) {
        objc_retain(bundle);
        [*table addObject:bundle];
    }

    id existing = nil;
    if (path != nil) {
        existing = [_resolvedPathToBundles objectForKey:path];
        if (existing == nil) {
            objc_retain(bundle);
            [_resolvedPathToBundles setObject:bundle forKey:path];
        }
    }
    if (cls != Nil) {
        existing = [_bundleForClassMap objectForKey:cls];
        if (existing == nil) {
            objc_retain(bundle);
            [_bundleForClassMap setObject:bundle forKey:cls];
        }
    }
    if (immortal && existing == nil) {
        [_immortalBundles addPointer:bundle];
    }
    return existing;
}

// Native -[__NSBundleTables addBundle:forPath:withType:forClass:isImmortal:], 0x1c059f314.
// Non-external, direct.
- (id)addBundle:(id)bundle
        forPath:(id)path
       withType:(uint64_t)type
       forClass:(Class)cls
     isImmortal:(BOOL)immortal {
    [_lock lock];
    id result = [self _addBundleLocked:bundle forPath:path withType:type forClass:cls isImmortal:immortal];
    [_lock unlock];
    return result;
}

// Native -[__NSBundleTables bundleForClass:], 0x1c059f50c. Non-external, direct.
- (id)bundleForClass:(Class)cls {
    [_lock lock];
    id bundle = objc_autorelease(objc_retain([_bundleForClassMap objectForKey:cls]));
    [_lock unlock];
    return bundle;
}

// Native -[__NSBundleTables loadedBundles], 0x1c059f640. Non-external, direct.
- (id)loadedBundles {
    ONSHashTable *combined = [ONSHashTable hashTableWithOptions:0];
    id main = [ONSBundle mainBundle];
    if (main != nil) {
        [combined addObject:main];
    }
    [_lock lock];
    [combined unionHashTable:_loadedBundles];
    [combined unionHashTable:_staticFrameworks];
    [combined unionHashTable:_loadedFrameworks];
    [_lock unlock];
    return [combined allObjects];
}

@end
