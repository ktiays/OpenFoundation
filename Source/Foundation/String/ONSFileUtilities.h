//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

#include <sys/stat.h>

// File error and existence helpers (String/ONSFileUtilities.m), iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless a declaration says otherwise. iOS Device
// 24A446: unread.

// Native __NSErrorWithFilePath (0x1c05051ec) and __NSErrorWithFilePathAndEncoding (0x1c0505288), external:
// an NSCocoaErrorDomain error with the code, whose user info holds the path or URL (and the encoding).
OF_EXPORT id _ONSErrorWithFilePath(ONSInteger code, id pathOrURL);
OF_EXPORT id _ONSErrorWithFilePathAndEncoding(ONSInteger code, id pathOrURL, ONSUInteger encoding);

// Native __NSFileExistsAtPath, 0x1c050de18 (hidden, was a private external): whether stat() succeeds on the
// file system representation the file manager forms of the characters; the status is stored.
OF_EXTERN BOOL _NSFileExistsAtPath(
    id fileManager, const unsigned short *characters, ONSUInteger length, struct stat *status
);

#ifdef __OBJC__
// Classes the file and path units reference by name (class symbols and runtime names), so that the prefix
// tool maps them.

// Selectors the file and path units send to objects of classes outside this family (NSFileManager,
// NSCheapMutableString, NSPathStore2), declared for the message sends only; no protocol object is emitted.

#endif

// Native __NSFileSystemRepresentationString, 0x1c0593764. Was a private external.
OF_EXTERN id _NSFileSystemRepresentationString(const char *bytes, ONSUInteger length);
