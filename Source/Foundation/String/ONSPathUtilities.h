//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

// The NSPathUtilities category and the path functions of its unit (String/ONSPathUtilities.m), iOS Simulator
// 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless a
// declaration says otherwise. iOS Device 24A446: unread. The unit's other path functions are non-external
// natively and static in the unit.

// Native NSHomeDirectoryForUser, 0x1c0591f0c, external (SDK NSPathUtilities.h).
OF_EXPORT id ONSHomeDirectoryForUser(id userName);

// Swift calling convention for the restored standardizingPath file-manager call at 0x1c0b63918.
OF_EXTERN __attribute__((swiftcall)) bool
swiftStandardizingPathFileExists(const uint8_t *bytes, OCFIndex count);

#ifdef __OBJC__
@interface ONSString (NSPathUtilities)

+ (id)pathWithComponents:(id)components;
- (id)pathComponents;
- (id)lastPathComponent;
- (id)stringByDeletingLastPathComponent;
- (id)stringByAppendingPathComponent:(id)string;
- (id)pathExtension;
- (id)stringByDeletingPathExtension;
- (id)_stringByStandardizingPathUsingCache:(BOOL)useCache;
- (id)stringByStandardizingPath;
- (id)_stringByResolvingSymlinksInPathUsingCache:(BOOL)useCache;
- (const char *)fileSystemRepresentation;
- (BOOL)getFileSystemRepresentation:(char *)buffer maxLength:(ONSUInteger)maxLength;

@end
#endif
