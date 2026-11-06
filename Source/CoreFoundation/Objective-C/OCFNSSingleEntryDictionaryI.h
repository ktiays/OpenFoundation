//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Collections/OCFNSCFDictionary.h"
#include "CoreFoundation/Objective-C/OCFNSDictionary.h"

#ifdef __OBJC__
// Native class __NSSingleEntryDictionaryI (__DATA,__objc_data 0x22e6cca50, was a private external), the
// one-entry immutable dictionary, an NSDictionary subclass; class_ro_t 0x2227bfb98: instance start 8, size
// 24; ivars (instance variable list 0x2227bfb50, offsets from their _OBJC_IVAR_$ symbols) _obj at 8 and _key
// at 16, 8 bytes each (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified;
// the type encodings lie outside the extracted image).
@interface __ONSSingleEntryDictionaryI : ONSDictionary {
    id _obj;
    id _key;
}
@end
#endif
