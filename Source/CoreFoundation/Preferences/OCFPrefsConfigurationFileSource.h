//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Preferences/OCFPrefsSource.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class CFPrefsConfigurationFileSource (__DATA,__objc_data 0x22e6cd360), a hidden CFPrefsSource
// subclass with instance start and size 65 (no ivars of its own), from its class_ro_t (iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its single method is restored in
// Preferences/OCFPrefsConfigurationFileSource.m.
@interface OCFPrefsConfigurationFileSource : OCFPrefsSource

- (id)initWithConfigurationPropertyList:(OCFDictionaryRef)propertyList
                  containingPreferences:(_OCFXPreferences *)containingPreferences;

@end
#endif
