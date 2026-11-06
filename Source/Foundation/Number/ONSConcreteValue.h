//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Number/ONSValue.h"

// Native NSConcreteValue, 0x22e6de3d8. External. class_ro_t 0x222862878:
// superclass NSValue, instance start 8, size 24. Both ivar offsets are external.
__attribute__((visibility("default")))
@interface ONSConcreteValue : ONSValue {
    @public
    uint64_t _specialFlags;
    void *typeInfo;
}

- (void *)_value;

@end
