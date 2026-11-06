//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Scanner/ONSScanner.h"

#ifdef __OBJC__
@interface ONSScanner (NSDecimalNumberScanning)
- (BOOL)scanDecimal:(ONSDecimal *)result;
@end
#endif

// Native __NSNumberSet, 0x1c100e0b0, was a private external; its sixteen-byte bitmap is image data.
OF_EXTERN const unsigned char __NSNumberSet[16];
