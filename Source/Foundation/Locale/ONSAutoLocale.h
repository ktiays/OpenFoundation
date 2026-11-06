//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "CoreFoundation/Objective-C/OCFNSLocale.h"

// NSAutoLocale's coding (Locale/ONSAutoLocale.m), iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device 24A446: unread.

#ifdef __OBJC__
// Native NSAutoLocale, 0x22e6daff8 (non-external); class_ro_t flags 0x10 (not reproduced), instance start and
// size 8, no ivars; the superclass (0x22e6ce710) is CoreFoundation's NSLocale. Only the coding methods are
// reached.
__attribute__((visibility("hidden")))
@interface ONSAutoLocale : ONSLocale
- (void)encodeWithCoder:(id)coder;
- (id)initWithCoder:(id)coder;
- (Class)classForCoder;
@end
#endif
