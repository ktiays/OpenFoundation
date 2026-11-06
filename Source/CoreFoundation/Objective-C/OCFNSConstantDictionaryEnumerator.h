//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSConstantDictionary.h"
#include "CoreFoundation/Objective-C/OCFNSFastEnumerationEnumerator.h"

#ifdef __OBJC__
// Native class __NSConstantDictionaryEnumerator (__DATA,__objc_data 0x22e6cd810, was a private external), the
// key and object enumerator of NSConstantDictionary, an NSEnumerator subclass; class_ro_t 0x2227c2310:
// instance start 8, size 25; ivars (offsets from their _OBJC_IVAR_$ symbols, were private externals)
// _underlyingDictionary at 8 (8 bytes), _currentIndex at 16 (8 bytes) and _isForKeys at 24 (1 byte) (iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for names, offsets and
// sizes). The type encodings lie outside the extracted image (unread); the types follow the sizes and the
// methods' use (the dictionary is stored without a retain).
@interface __ONSConstantDictionaryEnumerator : ONSEnumerator {
    ONSConstantDictionary *_underlyingDictionary;
    NSUInteger _currentIndex;
    BOOL _isForKeys;
}

- (id)initWithConstantDictionary:(ONSConstantDictionary *)dictionary enumerateKeys:(BOOL)enumerateKeys;

@end
#endif
