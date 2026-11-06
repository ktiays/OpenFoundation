//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Archiving/ONSCoder.h"

// Native NSXPCCoder, 0x22e6e1df8, external; class_ro_t 0x222870678. Instance start/size 8/16.
__attribute__((visibility("default")))
@interface ONSXPCCoder : ONSCoder {
    @private
    id<NSObject> _userInfo;
}
- (BOOL)requiresSecureCoding;
- (id)userInfo;
@end
