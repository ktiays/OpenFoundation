//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

// The _NSOriginalAttributedString category of NSString (String/ONSStringOriginalAttributedString.m), iOS
// Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device
// 24A446: unread.

#ifdef __OBJC__
@interface ONSString (_NSOriginalAttributedString)

- (id)__baseAttributedString;

@end
#endif
