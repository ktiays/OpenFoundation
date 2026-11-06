//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Number/ONSValue.h"

#ifdef __OBJC__
// Native category NSValue(NSValuePortCoding) of the Foundation image (iOS Simulator 24A434, UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03). Restored in Foundation/Number/ONSValuePortCoding.m.
@interface ONSValue (NSValuePortCoding)

- (id)replacementObjectForPortCoder:(id)coder;

@end
#endif
