//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Number/ONSNumber.h"

#ifdef __OBJC__
// Native class NSPlaceholderValue (__DATA,__objc_data 0x22e6de568, non-external), an NSNumber subclass;
// class_ro_t 0x222862b98: flags 0x10, instance start 8, size 16, one ivar zoneForInstance (type
// "^{_NSZone=}", offset variable 0x22e6d4e00, offset 8, size 8, alignment 8). Native class
// NSPlaceholderNumber (0x22e6de518, non-external), an NSPlaceholderValue subclass; class_ro_t 0x222862c28:
// flags 0x10, instance start and size 16, no ivars (iOS Simulator 24A434, UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified). Their only instances are the static
// placeholders __placeholderValue (0x2227d7ad8) and
// __placeholderNumber (0x2227d7ac8), each the class followed by a NULL zone (__DATA_CONST,__const).
@interface ONSPlaceholderValue : ONSNumber {
    @public
    struct _NSZone *zoneForInstance;
}
@end

#endif
