//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Collections/OCFNSCFDictionary.h"
#include "CoreFoundation/Objective-C/OCFNSDictionary.h"

#ifdef __OBJC__
// Native class __NSDictionaryI (__DATA,__objc_data 0x22e6cc820, was a private external), the immutable hashed
// dictionary, an NSDictionary subclass; class_ro_t 0x2227bf640: instance start 8, size 16; ivars (instance
// variable list 0x2227bf5b8, offsets from their _OBJC_IVAR_$ symbols): _used at 8, _copyKeys and _szidx at 15
// (bit-fields of the word at 8: _used bits 0-56, _copyKeys bit 57, _szidx bits 58-63, as the methods' masks
// and BFI/BFXIL stores read them) and _list at 16, a flexible array of key and object pairs (size 0) (iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for offsets and widths;
// the type encodings lie outside the extracted image).
@interface __ONSDictionaryI : ONSDictionary {
    uint64_t _used : 57;
    uint64_t _copyKeys : 1;
    uint64_t _szidx : 6;
    id _list[0];
}
@end
#endif
