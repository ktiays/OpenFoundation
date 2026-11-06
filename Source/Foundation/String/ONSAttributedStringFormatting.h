//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSAttributedString.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include <stdarg.h>

// iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified.
// iOS Device 24A446: unread. Only the three formatting entry points in the captured scope are declared.
@interface ONSAttributedString (NSAttributedStringFormattingSPI)
- (id)_initWithFormat:(id)format options:(NSUInteger)options locale:(id)locale arguments:(va_list)arguments;
- (id)__initWithFormat:(id)format
      attributeOptions:(NSUInteger)attributeOptions
     formattingOptions:(id)formattingOptions
                locale:(id)locale
             arguments:(va_list)arguments;
- (id)__initWithFormat:(id)format
      attributeOptions:(NSUInteger)attributeOptions
     formattingOptions:(id)formattingOptions
                locale:(id)locale
               context:(id)context
             arguments:(va_list)arguments;
@end
