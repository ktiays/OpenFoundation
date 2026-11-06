//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

__attribute__((visibility("default")))
@interface ONSLocalizedNumberFormatRule : NSObject
- (instancetype)_init;
+ (id)automatic;
- (id)copyWithZone:(struct _NSZone *)zone;
- (void)encodeWithCoder:(id)coder;
- (instancetype)initWithCoder:(id)coder;
+ (BOOL)supportsSecureCoding;
- (id)_formattedString:(id)string withStringLocale:(id)stringLocale andDisplayLocale:(id)locale;
@end

// Native class 0x22e6def40, class_ro_t 0x222864c00, size 8.
@interface _ONSLocalizedNumberFormatRuleAutomatic : ONSLocalizedNumberFormatRule
@end
