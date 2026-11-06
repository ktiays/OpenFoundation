//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Number/OCFNumber.h"
#import "Foundation/Number/ONSNumber.h"

#ifdef __OBJC__
// Native category NSNumber(NSCFAdditions) of the Foundation image (iOS Simulator 24A434, UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03): the CFNumber-bridging methods CoreFoundation sends to a number that
// is not a CFNumber. Restored in Foundation/Number/ONSNumberCFAdditions.m. CFTypeID and CFNumberType are
// spelled with their OCF types.
@interface ONSNumber (NSCFAdditions)

- (OCFTypeID)_cfTypeID;
- (OCFNumberType)_cfNumberType;
- (BOOL)_getValue:(void *)value forType:(OCFNumberType)type;
- (NSInteger)_reverseCompare:(id)otherNumber;

@end
#endif
