//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSMutableString.h"

// The NSRegularExpressionSupport categories of NSString and NSMutableString, iOS Simulator 24A434, arm64,
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. iOS Device 24A446: unread.
//
// The shared cache's initializer (____createRegexForPattern_block_invoke, 0x1c063fe88) was read in IDA on
// 2026/11/03. It creates an NSCache, names it NSRegularExpressionCache and sets its count limit to 10.

#ifdef __OBJC__
@interface ONSString (NSRegularExpressionSupport)

- (ONSRange)_rangeOfRegularExpressionPattern:(id)pattern
                                     options:(ONSUInteger)options
                                       range:(ONSRange)range
                                      locale:(id)locale;
- (id)_stringByReplacingOccurrencesOfRegularExpressionPattern:(id)pattern
                                                 withTemplate:(id)replacementTemplate
                                                      options:(ONSUInteger)options
                                                        range:(ONSRange)range;

@end

@interface ONSMutableString (NSRegularExpressionSupport)

- (ONSUInteger)_replaceOccurrencesOfRegularExpressionPattern:(id)pattern
                                                withTemplate:(id)replacementTemplate
                                                     options:(ONSUInteger)options
                                                       range:(ONSRange)range;

@end
#endif
