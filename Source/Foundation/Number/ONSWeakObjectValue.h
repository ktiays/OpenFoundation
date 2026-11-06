//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Number/ONSValue.h"

// Native NSWeakObjectValue, 0x22e6de4a0. Non-external.
// class_ro_t 0x222862970: superclass NSValue, instance start 8, size 25.
// Ivars at offsets 8 (^v), 16 (@), and 24 (B), respectively.
@interface ONSWeakObjectValue : ONSValue {
    @private
    void *_value;
    id _object;
    bool _useFallback;
}

- (id)initWithObject:(id)object;

@end
