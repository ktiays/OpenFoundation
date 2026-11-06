//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "Foundation/Error/ONSError.h"

#ifdef __OBJC__
// Native category NSError(NSKeyedArchivalErrors) of the Foundation image (iOS Simulator 24A434, Foundation
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03).
@interface ONSError (NSKeyedArchivalErrors)

+ (id)_readCorruptErrorWithFormat:(id)format, ...;

@end
#endif
