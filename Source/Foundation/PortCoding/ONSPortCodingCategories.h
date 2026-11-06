//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "Foundation/Error/ONSError.h"
#include "Foundation/Notification/ONSNotification.h"

#ifdef __OBJC__
// Native categories NSError(NSErrorPortCoding) and NSNotification(NSNotificationPortCoding) of the Foundation
// image (iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03).
@interface ONSError (NSErrorPortCoding)

- (id)replacementObjectForPortCoder:(id)coder;

@end

@interface ONSNotification (NSNotificationPortCoding)

- (id)replacementObjectForPortCoder:(id)coder;

@end
#endif
