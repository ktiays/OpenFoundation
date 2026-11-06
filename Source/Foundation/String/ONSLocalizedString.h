//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSMutableString.h"

#ifdef __OBJC__
// Native __NSLocalizedString, 0x22e6dd5c8. Was a private external. class_ro_t 0x22285f9f8:
// NSMutableString superclass, instance start 8 and size 24.
@class ONSDictionary;

@interface __ONSLocalizedString : ONSMutableString {
    @private
    ONSMutableString *original;
    ONSDictionary *config;
}

- (id)initWithString:(id)string withFormatConfiguration:(id)configuration;
- (id)baseString;
- (id)formatConfiguration;

@end
#endif
