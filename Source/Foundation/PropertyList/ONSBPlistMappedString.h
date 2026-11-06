//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

#ifdef __OBJC__
// Native _NSBPlistMappedString, 0x22e6d80f0. Was a private external. class_ro_t 0x22284ee30:
// NSString superclass, instance start 8 and size 16.
@interface _ONSBPlistMappedString : ONSString {
    @private
    uint64_t payload;
}

+ (id)createStringWithOffset:(uint64_t)offset intoMappingData:(id)data;

@end
#endif
