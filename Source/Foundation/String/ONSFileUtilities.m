//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSFileUtilities.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

#include <objc/message.h>
#include <objc/runtime.h>

// Inferred organization: three file helpers the NSString unit and the path utilities call, which lie in other
// native clusters than the path utilities: __NSErrorWithFilePath and __NSErrorWithFilePathAndEncoding
// together at 0x1c05051ec-0x1c050535c, __NSFileExistsAtPath at 0x1c050de18 (both in the file manager's region
// of
// __text). Following the batch's ownership, they share this translation unit in native address order. The
// unit is Objective-C compiled with manual reference counting (objc_release of the temporary string). Unless
// a definition says otherwise: iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Foundation/String/ONSStringReview.md.
//
// GOT slots outside the extracted image are identified by their use and by the alphabetical order of the
// shared cache's import slots (confidence: medium): 0x22227FAD0 is the NSDictionary class and 0x22227FB88 the
// NSURL class.

// Class symbols the helpers reference: NSError and NSNumber (Foundation, outside this family), NSDictionary
// and NSURL (CoreFoundation), NSCheapMutableString (Foundation, not restored).

// Native __NSErrorWithFilePath, 0x1c05051ec. External. The key is NSURLErrorKey when the object is a kind of
// [NSURL class] (objc_opt_class, objc_opt_isKindOfClass, bit 0), otherwise NSFilePathErrorKey; the user info
// is +[NSDictionary dictionaryWithObjectsAndKeys:] of the object and the key, and the result is
// +[NSError errorWithDomain:code:userInfo:] with NSCocoaErrorDomain (a tail call).
id _ONSErrorWithFilePath(ONSInteger code, id pathOrURL) {
    id key = [pathOrURL isKindOfClass:[(id)fileURLClassSymbol class]] ? ONSURLErrorKey : ONSFilePathErrorKey;
    id userInfo = [(id)fileDictionaryClassSymbol dictionaryWithObjectsAndKeys:pathOrURL, key, nil];
    return [(id)fileErrorClassSymbol errorWithDomain:ONSCocoaErrorDomain code:code userInfo:userInfo];
}

// Native __NSErrorWithFilePathAndEncoding, 0x1c0505288. External. As __NSErrorWithFilePath, with the encoding
// as +[NSNumber numberWithUnsignedInteger:] under NSStringEncodingErrorKey after the path entry. The key
// constants are loaded before the number is created.
id _ONSErrorWithFilePathAndEncoding(ONSInteger code, id pathOrURL, ONSUInteger encoding) {
    id key = [pathOrURL isKindOfClass:[(id)fileURLClassSymbol class]] ? ONSURLErrorKey : ONSFilePathErrorKey;
    id domain = ONSCocoaErrorDomain;
    id number = [(id)fileNumberClassSymbol numberWithUnsignedInteger:encoding];
    id userInfo = [(id)fileDictionaryClassSymbol
        dictionaryWithObjectsAndKeys:pathOrURL, key, number, ONSStringEncodingErrorKey, nil];
    return [(id)fileErrorClassSymbol errorWithDomain:domain code:code userInfo:userInfo];
}

// Native __NSFileExistsAtPath, 0x1c050de18. Hidden (was a private external). The characters are wrapped
// without copying in an NSCheapMutableString (+allocWithZone: with no zone, -init,
// -setContentsNoCopy:length:freeWhenDone:NO isUnicode:YES); the file manager forms its file system
// representation into a 1026-byte stack buffer (-getFileSystemRepresentation:maxLength:1026 withPath:), the
// string is released, and on success the answer is whether stat() returns 0. A failed representation answers
// NO without calling stat().
BOOL _NSFileExistsAtPath(
    id fileManager, const unsigned short *characters, ONSUInteger length, struct stat *status
) {
    char buffer[1026];
    id string = [[(Class)fileCheapMutableStringClassSymbol allocWithZone:NULL] init];
    [string setContentsNoCopy:(void *)characters length:length freeWhenDone:NO isUnicode:YES];
    BOOL represented = [fileManager getFileSystemRepresentation:buffer maxLength:1026 withPath:string];
    [string release];
    if (!represented) {
        return NO;
    }
    return stat(buffer, status) == 0;
}

// Native __NSFileSystemRepresentationString, 0x1c0593764. Was a private external.
// iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device 24A446: unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FB-bundle-paths-plist.
id _NSFileSystemRepresentationString(const char *bytes, ONSUInteger length) {
    return [[[ONSString allocWithZone:NULL] initWithBytes:bytes length:length encoding:4] autorelease];
}
