//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Preferences/OCFPrefsConfigurationFileSource.h"
#import "CoreFoundation/Preferences/OCFPreferences.h"

// Inferred organization: -[CFPrefsConfigurationFileSource initWithConfigurationPropertyList:
// containingPreferences:] (0x1c01b5a28) and mergeFunc (0x1c01b5ab8) are contiguous in the iOS Simulator
// __text section between the NSLocale cluster and the pasteboard functions; the source path is not in the
// evidence (CFPrefsConfigurationFileSource.m inferred from the class). The unit is Objective-C compiled with
// manual reference counting. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationPreferences.md.

// The builtin type ID of CFDictionary (18).
#define kDictionaryTypeID ((OCFTypeID)18)

static void mergeFunc(const void *key, const void *value, void *context);

@implementation OCFPrefsConfigurationFileSource

// Native -[CFPrefsConfigurationFileSource initWithConfigurationPropertyList:containingPreferences:],
// 0x1c01b5a28. The superclass initializer is sent with the containing preferences (selector reference
// 0x2387af80c, outside the extracted image; -initWithContainingPreferences: inferred from the argument and
// the CFPrefsSource method list: confidence medium); the property list is then applied through mergeFunc.
- (id)initWithConfigurationPropertyList:(OCFDictionaryRef)propertyList
                  containingPreferences:(_OCFXPreferences *)containingPreferences {
    self = [super initWithContainingPreferences:containingPreferences];
    if (self != nil) {
        OCFDictionaryApplyFunction(propertyList, mergeFunc, self);
    }
    return self;
}

@end

// Native mergeFunc, 0x1c01b5ab8. Non-external. A dictionary entry's NSUserDefaultsDefaultValue is set as the
// key's value (copied, from no sender) on the configuration-file source.
static void mergeFunc(const void *key, const void *value, void *context) {
    if (OCFGetTypeID(value) != kDictionaryTypeID) {
        return;
    }
    OCFPropertyListRef defaultValue = OCFDictionaryGetValue(value, OF_STR("NSUserDefaultsDefaultValue"));
    if (defaultValue != NULL) {
        [(OCFPrefsSource *)context setValue:defaultValue forKey:key from:nil];
    }
}
