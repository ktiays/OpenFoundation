//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"

#ifdef __OBJC__
// The NSSharedKeySetDictionary categories on NSDictionary and NSMutableDictionary (native methods
// 0x1c02a7530 and 0x1c02a7608, between the CFListFormatter unit's OUTLINED_FUNCTION pieces and
// _CFListFormatterGetTypeID), defined by Objective-C/OCFNSDictionarySharedKeySet.m.
@interface ONSDictionary (NSSharedKeySetDictionary)

+ (id)sharedKeySetForKeys:(id)keys;

@end

@interface ONSMutableDictionary (NSSharedKeySetDictionary)

+ (id)dictionaryWithSharedKeySet:(id)keySet;

@end
#endif
