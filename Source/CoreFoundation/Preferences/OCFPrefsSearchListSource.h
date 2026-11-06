//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Preferences/OCFPreferences.h"
#include "CoreFoundation/Preferences/OCFPrefsCloudSource.h"
#include "CoreFoundation/Preferences/OCFPrefsConfigurationFileSource.h"
#include "CoreFoundation/Preferences/OCFPrefsSource.h"
#include "CoreFoundation/Preferences/OCFXPreferences.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class CFPrefsSearchListSource (__DATA,__objc_data 0x22e6ce760), a hidden CFPrefsSource subclass with
// instance start 72 and size 129. Ivar offsets and sizes from its class_ro_t (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified); the type encodings lie outside the extracted
// image, so the types below are the ones the restored code establishes. The ivars are public because the
// _CFXPreferences(SearchListAdditions) blocks read and write them directly (offset loads and stores); their
// declared visibility is unread. Only the direct methods this family restores are defined; the class's other
// methods are unread.
@interface OCFPrefsSearchListSource : OCFPrefsSource {
    @public
    OCFMutableDictionaryRef _keysToSources;
    OCFStringRef _identifier;
    OCFMutableArrayRef _sourceList;
    OCFMutableSetRef _cloudKeys;
    OCFMutableArrayRef _cloudPrefixKeys;
    OCFPrefsSource *_standardSetTarget;
    OCFPrefsSource *_cloudSetTarget;
    BOOL initialized;
}

- (OCFStringRef)copyCloudConfigurationWithURL:(OCFURLRef)url
                          outConfigFileSource:(OCFPrefsSource **)outConfigFileSource
                                 outStoreName:(OCFStringRef *)outStoreName __attribute__((objc_direct));

- (void)addSource:(OCFPrefsSource *)source __attribute__((objc_direct));

- (void)setCloudEnabled:(BOOL)enabled forKeyPrefix:(OCFStringRef)prefix __attribute__((objc_direct));

- (void)setCloudEnabled:(BOOL)enabled forKey:(OCFStringRef)key __attribute__((objc_direct));

- (void)addManagedSourceForIdentifier:(OCFStringRef)identifier
                                 user:(OCFStringRef)user __attribute__((objc_direct));

- (void)addSimulatorBackstopSourceForIdentifier:(OCFStringRef)identifier __attribute__((objc_direct));

- (void)addSourceForIdentifier:(OCFStringRef)identifier
                          user:(OCFStringRef)user
                        byHost:(BOOL)byHost
                     container:(OCFStringRef)container __attribute__((objc_direct));

- (void)addCloudSourceForIdentifier:(OCFStringRef)identifier
                  configurationPath:(OCFStringRef)configurationPath
                          storeName:(OCFStringRef)storeName
                          container:(OCFStringRef)container __attribute__((objc_direct));

- (void)addSuiteSourceForIdentifier:(OCFStringRef)identifier
                               user:(OCFStringRef)user __attribute__((objc_direct));

- (void)addNamedVolatileSourceForIdentifier:(OCFStringRef)identifier __attribute__((objc_direct));

- (instancetype)initWithIdentifier:(OCFStringRef)identifier
             containingPreferences:(_OCFXPreferences *)containingPreferences;
- (void)deferredNotifyCausedByLoadingOfChangesFromDictionary:(OCFDictionaryRef)oldValues
                                                toDictionary:(OCFDictionaryRef)newValues;
- (void)handleChangeNotificationForDomainIdentifier:(OCFStringRef)identifier isRemote:(BOOL)isRemote;
- (void)deferredNotifyCausedByLocalWriteOfChangesToKey:(OCFStringRef)key
                                             fromValue:(OCFPropertyListRef)oldValue
                                               toValue:(OCFPropertyListRef)newValue;

@end

// C2 supplies the native destructor for the pending KVO thread-specific slot.
OF_EXTERN void pendingKVODestructor(void *pending);

// Native class CFPrefsSuiteSearchListSource (__DATA,__objc_data 0x22e6ce7b0), a hidden
// CFPrefsSearchListSource subclass with instance start and size 129 (no ivars of its own), from its
// class_ro_t (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its
// four methods are restored in Preferences/OCFPrefsSearchListSource.m.
@interface OCFPrefsSuiteSearchListSource : OCFPrefsSearchListSource

- (void)deferredNotifyCausedByLoadingOfChangesFromDictionary:(OCFDictionaryRef)oldValues
                                                toDictionary:(OCFDictionaryRef)newValues;
- (void)_notifyObserversOfChangeFromValuesForKeys:(OCFDictionaryRef)oldValues
                                  toValuesForKeys:(OCFDictionaryRef)newValues;
- (void)handleChangeNotificationForDomainIdentifier:(OCFStringRef)identifier isRemote:(BOOL)isRemote;
- (void)deferredNotifyCausedByLocalWriteOfChangesToKey:(OCFStringRef)key
                                             fromValue:(OCFPropertyListRef)oldValue
                                               toValue:(OCFPropertyListRef)newValue;

@end

// Native category SearchListAdditions (CFPrefsSearchListSource.m, from the block symbols); withSearchLists:
// has no block of its own and sits with the other search-list direct methods (inferred).
@interface _OCFXPreferences (SearchListAdditions)

- (void)withSuiteSearchListForIdentifier:(OCFStringRef)identifier
                                    user:(OCFStringRef)user
                                  locked:(BOOL)locked
                                 perform:(void (^)(OCFPrefsSearchListSource *searchList))block
    __attribute__((objc_direct));

- (void)withSearchLists:(void (^)(OCFMutableDictionaryRef searchLists))block __attribute__((objc_direct));

- (void)updateSearchListsForIdentifier:(OCFStringRef)identifier __attribute__((objc_direct));

- (void)withSearchListForIdentifier:(OCFStringRef)identifier
                          container:(OCFStringRef)container
              cloudConfigurationURL:(OCFURLRef)cloudConfigurationURL
                            perform:(void (^)(OCFPrefsSearchListSource *searchList))block
    __attribute__((objc_direct));

@end
#endif

// Pending notification TLS destructor, installed by C1-prefs-sources.
OF_EXTERN void pendingKVODestructor(void *value) __attribute__((noreturn));
