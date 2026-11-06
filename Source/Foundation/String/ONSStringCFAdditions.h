//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSMutableString.h"

// The NSCFAdditions categories of NSString and NSMutableString (String/ONSStringCFAdditions.m), Foundation
// image iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; signatures from the methods'
// register use (confidence: verified). iOS Device 24A446: unread.

#ifdef __OBJC__
@interface ONSString (NSCFAdditions)

- (OCFTypeID)_cfTypeID;
- (id)_createSubstringWithRange:(ONSRange)range __attribute__((ns_returns_retained));
- (BOOL)_encodingCantBeStoredInEightBitCFString;
- (OCFStringEncoding)_fastestEncodingInCFStringEncoding;
- (BOOL)_getCString:(char *)buffer maxLength:(ONSUInteger)maxLength encoding:(OCFStringEncoding)encoding;

@end

@interface ONSMutableString (NSCFAdditions)

- (void)appendCharacters:(const unsigned short *)characters length:(ONSUInteger)length;
- (void)_cfAppendCString:(const char *)cString length:(ONSInteger)length;
- (void)_cfLowercase:(const void *)locale;
- (void)_cfUppercase:(const void *)locale;
- (void)_cfCapitalize:(const void *)locale;
- (void)_cfPad:(OCFStringRef)padString length:(uint32_t)length padIndex:(uint32_t)indexIntoPad;

@end
#endif
