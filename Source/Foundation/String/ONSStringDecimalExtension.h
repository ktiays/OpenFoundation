//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

#include <OpenFoundation/Decimal/ONSDecimal.h>

// The NSDecimalExtension category of NSString (String/ONSStringDecimalExtension.m), iOS Simulator 24A434,
// arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device 24A446: unread.

#ifdef __OBJC__
@interface ONSString (NSDecimalExtension)

- (ONSDecimal)decimalValue;

@end

// Selectors the category sends to NSScanner and NSDecimalNumber (outside this family), declared for the
// message sends only; no protocol object is emitted.

#endif
