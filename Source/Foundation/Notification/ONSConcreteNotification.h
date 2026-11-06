//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Notification/ONSNotification.h"

@class ONSString;
@class ONSDictionary;

// Native NSConcreteNotification, 0x22e6db7a0, non-external. class_ro_t 0x2228595c8:
// instance start/size 8/32, ivars at 8, 16 and 24.
@interface ONSConcreteNotification : ONSNotification {
    @private
    ONSString *name;
    id object;
    ONSDictionary *userInfo;
}
@end
