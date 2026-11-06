//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Preferences/OCFPreferences.h"
#include "CoreFoundation/Preferences/OCFXPreferences.h"

#include <os/lock.h>
#include <xpc/xpc.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

// The Foundation class whose -processInfo the default-arguments block of
// _addBackstopValuesForIdentifierAndSource messages (Preferences/OCFPrefsSource.m); declared for its runtime
// name only.
@class ONSProcessInfo;

// Native class CFPrefsSource (__DATA,__objc_data 0x22e6cd9f0), a hidden NSObject subclass with instance size
// 65. Ivar offsets and sizes from its class_ro_t (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified); the type encodings lie outside the extracted
// image, so the types below are the ones the restored code establishes (the shmem entry points at a 32-bit
// generation word of the shared-memory page), or size placeholders where no restored code reads the ivar
// (type unread). Only the direct methods this family restores are defined; the class's other methods are
// unread.
@interface OCFPrefsSource : NSObject {
    @protected
    _OCFXPreferences *_containingPreferences;
    OCFMutableDictionaryRef _dict;
    id _observers;
    int64_t _generationCount;
    uint32_t *shmemEntry;
    int64_t shmemIndex;
    uint32_t lastKnownShmemState;
    os_unfair_lock _lock;
    BOOL _isSearchList;
}

- (void)forEachObserver:(void (^)(id observer))block __attribute__((objc_direct));

- (void)setValues:(const void **)values
                forKeys:(OCFStringRef *)keys
                  count:(OCFIndex)count
             copyValues:(BOOL)copyValues
    removeValuesForKeys:(OCFStringRef *)removedKeys
                  count:(OCFIndex)removedCount
                   from:(id)from __attribute__((objc_direct));

- (BOOL)validateValue:(OCFPropertyListRef)value
               forKey:(OCFStringRef)key
               inDict:(OCFDictionaryRef)dict
           forWriting:(BOOL)forWriting __attribute__((objc_direct));

- (void)setValue:(OCFPropertyListRef)value
          forKey:(OCFStringRef)key
            from:(id)from __attribute__((objc_direct));

- (instancetype)initWithContainingPreferences:(_OCFXPreferences *)containingPreferences;
- (OCFPrefsSource *)copyVolatileSourceWithContainingPreferences:(_OCFXPreferences *)containingPreferences;
- (int)alreadylocked_updateObservingRemoteChanges;
- (xpc_object_t)alreadylocked_createObserverUpdateMessageWithOperation:(int)operation forRole:(int *)role;
- (void)alreadylocked_setPrecopiedValues:(const void **)values
                                 forKeys:(OCFStringRef *)keys
                                   count:(OCFIndex)count
                                    from:(id)from;
- (void)_notifyObserversOfChangeFromValuesForKeys:(OCFDictionaryRef)oldValues
                                  toValuesForKeys:(OCFDictionaryRef)newValues;
- (OCFPropertyListRef)alreadylocked_copyValueForKey:(OCFStringRef)key;
- (OCFPropertyListRef)copyValueForKey:(OCFStringRef)key;
- (BOOL)synchronize;
- (void)alreadylocked_recordError:(OCFErrorRef)error;
- (void)setConfigurationPath:(OCFStringRef)configurationPath;
- (void)setStoreName:(OCFStringRef)storeName;
- (void)setEnabled:(BOOL)enabled;
- (OCFMutableDictionaryRef)alreadylocked_copyDictionary;
- (void)mergeIntoDictionary:(OCFMutableDictionaryRef)dictionary
           sourceDictionary:(OCFMutableDictionaryRef)sourceDictionary
          cloudKeyEvaluator:(BOOL (^)(OCFStringRef key))cloudKeyEvaluator;
- (xpc_object_t)createRequestNewContentMessageForDaemon:(int)role;
- (void)handleReply:(xpc_object_t)reply
    toRequestNewDataMessage:(xpc_object_t)message
               onConnection:(xpc_connection_t)connection
                 retryCount:(int)retryCount
                      error:(bool *)error;
- (int64_t)generationCount;
- (void)lock;
- (void)unlock;
- (BOOL)managed;
- (BOOL)isVolatile;
- (void)alreadylocked_clearCache;
- (OCFStringRef)userIdentifier;
- (BOOL)servedByUserSessionDaemon;
- (OCFStringRef)domainIdentifier;
- (BOOL)isByHost;
- (OCFStringRef)container;
- (void)setValues:(const void **)values
          forKeys:(OCFStringRef *)keys
            count:(OCFIndex)count
       copyValues:(BOOL)copyValues
             from:(id)from __attribute__((objc_direct));
- (OCFDictionaryRef)copyDictionary __attribute__((objc_direct));

@end

// This selector remains unread on CFPrefsSource.
@interface OCFPrefsSource (NativeUnrestored)

- (OCFArrayRef)alreadylocked_copyKeyList;

@end

// Native category SourceAdditions (CFPrefsSource.m, from the block symbols and the assertion's file name).
// withSources: has no block of its own; it sits with CFPrefsSource's description methods (inferred).
@interface _OCFXPreferences (SourceAdditions)

- (void)withSourceForIdentifier:(OCFStringRef)identifier
                           user:(OCFStringRef)user
                         byHost:(BOOL)byHost
                      container:(OCFStringRef)container
                          cloud:(BOOL)cloud
                        perform:(void (^)(OCFPrefsSource *source))block __attribute__((objc_direct));

- (void)withNamedVolatileSourceForIdentifier:(OCFStringRef)identifier
                                     perform:(void (^)(OCFPrefsSource *source))block
    __attribute__((objc_direct));

- (void)withSources:(void (^)(OCFMutableDictionaryRef sources))block __attribute__((objc_direct));

- (void)withNamedVolatileSources:(void (^)(OCFMutableDictionaryRef namedVolatileSources))block
    __attribute__((objc_direct));

@end
#endif
