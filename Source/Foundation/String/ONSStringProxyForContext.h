//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

// Native _NSStringProxyForContext, 0x22e6da968, was private external. class_ro_t 0x222855e98
// gives superclass NSString, instance start 8 and size 32.
__attribute__((visibility("hidden")))
@interface _ONSStringProxyForContext : ONSString {
    @private
    ONSString *_string;
    id _formatter;
    id _item;
}
@property(atomic, copy) ONSString *string;
@property(atomic, copy) id formatter;
@property(atomic, copy) id item;
- (void)_retainFormatter:(id)formatter;
- (id)_dynamicContextEvaluation:(const void *)context patternString:(const void *)patternString;
@end
