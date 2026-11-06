//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSStringDistantString.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSStringOtherEncodings.h"

// Inferred organization: the NSDistantString methods lie together in the Foundation image's distributed
// objects region (0x1c0500628-0x1c050076c); following the FS-string ownership the categories have their own
// unit here. Objective-C compiled with manual reference counting. Unless a definition says otherwise:
// iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Foundation/String/ONSStringReview.md.

// The class behind GOT slot 0x22227FB20, outside the extracted image, which receives +dataWithLength: and
// whose instances receive -mutableBytes and -setLength: (bound as NSMutableData, confidence: medium).

@implementation ONSMutableString (NSDistantString)

// Native -[NSMutableString(NSDistantString) replacementObjectForPortCoder:], 0x1c0500628. Non-external. A
// by-reference coder gets the coder's -_proxyForLocalObject: of the receiver (a tail call); otherwise the
// receiver.
- (id)replacementObjectForPortCoder:(id)coder {
    if ([coder isByref]) {
        return [coder _proxyForLocalObject:self];
    }
    return self;
}

@end

@implementation ONSString (NSDistantString)

// Native -[NSString(NSDistantString) _getCharactersAsStringInRange:], 0x1c050066c. Non-external. A tail call
// of -substringWithRange:.
- (id)_getCharactersAsStringInRange:(ONSRange)range {
    return [self substringWithRange:range];
}

// Native -[NSString(NSDistantString)
// _getBytesAsData:maxLength:usedLength:encoding:options:range:remainingRange:], 0x1c0500670. Non-external.
// The bytes go into +dataWithLength:maxLength's -mutableBytes through
// -getBytes:maxLength:usedLength:encoding:options:range:remainingRange: (the used length starts at 0). On YES
// the data is cut to the used length with -setLength:, the used length is stored when usedLength is not NULL
// and *data is the (autoreleased) data; on NO *data is nil.
- (BOOL)_getBytesAsData:(id *)data
              maxLength:(ONSUInteger)maxLength
             usedLength:(ONSUInteger *)usedLength
               encoding:(ONSUInteger)encoding
                options:(ONSUInteger)options
                  range:(ONSRange)range
         remainingRange:(ONSRange *)remainingRange {
    id bytes = [(id)mutableDataClassSymbol dataWithLength:maxLength];
    ONSUInteger used = 0;
    BOOL result = [self getBytes:[bytes mutableBytes]
                       maxLength:maxLength
                      usedLength:&used
                        encoding:encoding
                         options:options
                           range:range
                  remainingRange:remainingRange];
    id stored = nil;
    if (result) {
        [bytes setLength:used];
        if (usedLength != NULL) {
            *usedLength = used;
        }
        stored = bytes;
    }
    *data = stored;
    return result;
}

@end
