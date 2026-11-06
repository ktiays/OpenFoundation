//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Preferences/OCFPrefsPlistSource.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class CFPrefsCloudSource (__DATA,__objc_data 0x22e6ce210), a hidden CFPrefsPlistSource subclass with
// instance start 144 and size 161: _configPath (144), _storeName (152) and _enabled (160, one byte), from its
// class_ro_t (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The two
// strings are malloc'd UTF-8 copies (strdup, freed by -dealloc); the type encodings lie outside the extracted
// image. The class's ten methods are restored in Preferences/OCFPrefsCloudSource.m.
@interface OCFPrefsCloudSource : OCFPrefsPlistSource {
    @protected
    char *_configPath;
    char *_storeName;
    BOOL _enabled;
}

- (id)initWithDomain:(OCFStringRef)domain
                     user:(OCFStringRef)user
                   byHost:(BOOL)byHost
            containerPath:(OCFStringRef)containerPath
    containingPreferences:(_OCFXPreferences *)containingPreferences;
- (xpc_object_t)createSynchronizeMessage;
- (int)alreadylocked_updateObservingRemoteChanges;
- (void)fullCloudSynchronizeWithCompletionHandler:(void (^)(OCFDictionaryRef changes))completionHandler;
- (void)mergeIntoDictionary:(OCFMutableDictionaryRef)dictionary
           sourceDictionary:(OCFMutableDictionaryRef)sourceDictionary
          cloudKeyEvaluator:(BOOL (^)(OCFStringRef key))cloudKeyEvaluator;
- (BOOL)enabled;
- (void)setEnabled:(BOOL)enabled;
- (void)setConfigurationPath:(OCFStringRef)configurationPath;
- (void)setStoreName:(OCFStringRef)storeName;
- (void)dealloc;

@end
#endif
