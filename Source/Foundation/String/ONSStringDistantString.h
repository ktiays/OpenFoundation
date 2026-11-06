//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSMutableString.h"

// The NSDistantString categories of NSString and NSMutableString (String/ONSStringDistantString.m), iOS
// Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device
// 24A446: unread.

#ifdef __OBJC__
@interface ONSMutableString (NSDistantString)

- (id)replacementObjectForPortCoder:(id)coder;

@end

@interface ONSString (NSDistantString)

- (id)_getCharactersAsStringInRange:(ONSRange)range;
- (BOOL)_getBytesAsData:(id *)data
              maxLength:(ONSUInteger)maxLength
             usedLength:(ONSUInteger *)usedLength
               encoding:(ONSUInteger)encoding
                options:(ONSUInteger)options
                  range:(ONSRange)range
         remainingRange:(ONSRange *)remainingRange;

@end

// Selectors the categories send to the port coder (outside this family), declared for the message sends
// only; no protocol object is emitted.

#endif
