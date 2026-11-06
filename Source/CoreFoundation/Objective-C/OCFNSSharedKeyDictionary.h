//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"
#include "CoreFoundation/Objective-C/OCFNSSharedKeySet.h"

#ifdef __OBJC__
// Native class NSSharedKeyDictionary (__DATA,__objc_data 0x22e6ced00, was a private external), the mutable
// dictionary over a shared key set, an NSMutableDictionary subclass; class_ro_t 0x2227c6a60: instance start
// 8, size 57; ivars (instance variable list 0x2227c6978, offsets from their _OBJC_IVAR_$ symbols, which were
// private externals) _keyMap at 8, _count at 16, _values at 24, _ifkIMP at 32, _sideDic at 40 and _mutations
// at 48 (8 bytes each) and _doKVO at 56 (1 byte) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for names, offsets and sizes). The type
// encodings lie outside the extracted image: the types are read from the methods' use (confidence: medium):
// _keyMap the key set, _count the number of values in _values (one slot per key-set index), _ifkIMP the key
// set's -indexForKey: implementation, _sideDic the NSMutableDictionary of the keys outside the key set,
// _mutations the fast-enumeration mutation counter, _doKVO whether observation info is set.
@interface ONSSharedKeyDictionary : ONSMutableDictionary {
    ONSSharedKeySet *_keyMap;
    NSUInteger _count;
    id *_values;
    NSUInteger (*_ifkIMP)(id keySet, SEL selector, id key);
    id _sideDic;
    unsigned long _mutations;
    BOOL _doKVO;
}

+ (id)sharedKeyDictionaryWithKeySet:(id)keySet;
+ (BOOL)supportsSecureCoding;
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded;

- (id)initWithKeySet:(id)keySet;
- (id)keySet;

@end
#endif
