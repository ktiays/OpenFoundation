//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Number/ONSNumber.h"

// Native NSConstantIntegerNumber, 0x22e6df030. External.
// class_ro_t 0x222864e38: superclass NSNumber, instance start 8, size 24.
// The encoding (r*) is at offset 8, and the signed integer (q) at 16.
__attribute__((visibility("default")))
@interface ONSConstantIntegerNumber : ONSNumber {
    @public
    const char *_encoding;
    int64_t _value;
}

@end
