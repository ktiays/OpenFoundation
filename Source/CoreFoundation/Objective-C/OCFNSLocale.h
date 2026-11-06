//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSLocale (__DATA,__objc_data 0x22e6ce710, external), an NSObject subclass; class_ro_t
// 0x2227c57c8: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The method lists are in the shared cache's
// optimized method lists, outside the extracted image; the methods come from the IDA capture
// (Objective-C/OCFNSLocale.m). This interface replaces the size placeholder of
// Objective-C/OCFBridgedClasses.h.
// Exported, as the native image exports the class.
__attribute__((visibility("default")))
@interface ONSLocale : NSObject

- (id)objectForKey:(id)key;
- (id)displayNameForKey:(id)key value:(id)value;
- (id)initWithLocaleIdentifier:(id)string;
- (id)localeIdentifier;
+ (id)currentLocale;

@end

// The placeholder class of this image whose instance +[NSLocale allocWithZone:] answers
// (_NSPlaceholderLocale, not restored on this branch), declared so that the prefix tool maps it.
@class _ONSPlaceholderLocale;
#endif
