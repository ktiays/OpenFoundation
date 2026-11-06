//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSDate.h"

#include <stdint.h>

// Native __immutablePlaceholderDate (0x2227a82e0, __DATA_CONST,__const, non-external) and
// decodeTaggedTimeInterval (0x1c02800b4, non-external), defined hidden in Objective-C/OCFNSDate.m, the native
// unit (NSDate.m) of __NSPlaceholderDate as well. -initWithTimeIntervalSinceReferenceDate: compares the
// receiver with the placeholder's address and calls the decoder. The placeholder is spelled as its 16 bytes
// here; only its address is used.
OF_EXTERN const uint8_t __immutablePlaceholderDate[16];
OF_EXTERN double decodeTaggedTimeInterval(uint64_t encodedBits);

#ifdef __OBJC__
// Native class __NSPlaceholderDate (__DATA,__objc_data 0x22e6cddd8, was a private external), an NSDate
// subclass; class_ro_t 0x2227c38a8: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). Its only instance is the static immutable
// placeholder.
@interface __ONSPlaceholderDate : ONSDate
@end

// The concrete date class of this image that the placeholder allocates (__NSDate, restored in
// Objective-C/OCFNSDate.m), declared for the prefix tool.
@class __ONSDate;
#endif
