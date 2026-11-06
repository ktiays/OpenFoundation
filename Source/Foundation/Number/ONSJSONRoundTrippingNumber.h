//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Number/ONSNumber.h"
#import "Foundation/String/ONSString.h"

// Native _NSJSONRoundTrippingNumber, 0x22e6dff08. Hidden (was a private external).
// class_ro_t 0x2228682d0: instance start 8, size 24. Superclass binds to NSObject (medium confidence).
@interface _ONSJSONRoundTrippingNumber : NSObject {
    @private
    ONSNumber *_number;
    ONSString *_representation;
}

- (id)initWithNumber:(id)number representation:(id)representation;
- (id)stringValue;
- (BOOL)isEqualToNumber:(id)number;
- (id)initWithCoder:(id)coder;
- (void)encodeWithCoder:(id)coder;

@end
