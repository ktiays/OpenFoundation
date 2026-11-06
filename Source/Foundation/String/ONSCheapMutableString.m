//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSCheapMutableString.h"
#import "Foundation/String/ONSStringOtherEncodings.h"

#include <stdlib.h>
#include <string.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device 24A446: unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FB-bundle-paths-plist.

@implementation ONSCheapMutableString

// Native -[NSCheapMutableString setContentsNoCopy:length:freeWhenDone:isUnicode:], 0x1c0570bbc.
// Non-external. The freeWhenDone argument in W4 is unused; W5 replaces bit 0 and bit 1 is preserved (kept).
- (void)setContentsNoCopy:(void *)buffer
                   length:(ONSUInteger)length
             freeWhenDone:(BOOL)freeWhenDone
                isUnicode:(BOOL)isUnicode {
    if (flags.freeWhenDone) {
        free(contents.thin);
    }
    contents.thin = buffer;
    flags.isFat = isUnicode;
    numCharacters = length;
}

// Native -[NSCheapMutableString dealloc], 0x1c0570c30. Non-external.
- (void)dealloc {
    if (flags.freeWhenDone) {
        free(contents.thin);
    }
    [super dealloc];
}

// Native -[NSCheapMutableString characterAtIndex:], 0x1c0570cbc. Non-external.
- (unsigned short)characterAtIndex:(ONSUInteger)index {
    if (flags.isFat) {
        return contents.fat[index];
    }
    return ((const unsigned char *)contents.thin)[index];
}

// Native -[NSCheapMutableString length], 0x1c0570ce4. Non-external.
- (ONSUInteger)length {
    return numCharacters;
}

// Native -[NSCheapMutableString getBytes:maxLength:usedLength:encoding:options:range:remainingRange:],
// 0x1c0570cf4. Non-external. The assembly forwards range as two words to the superclass.
- (BOOL)getBytes:(void *)buffer
         maxLength:(ONSUInteger)maxLength
        usedLength:(ONSUInteger *)usedLength
          encoding:(ONSUInteger)encoding
           options:(ONSUInteger)options
             range:(ONSRange)range
    remainingRange:(ONSRange *)remainingRange {
    if (flags.isFat || encoding != [ONSString defaultCStringEncoding]) {
        return [super getBytes:buffer
                     maxLength:maxLength
                    usedLength:usedLength
                      encoding:encoding
                       options:options
                         range:range
                remainingRange:remainingRange];
    }

    ONSUInteger count = range.length < maxLength ? range.length : maxLength;
    if (buffer != NULL) {
        for (ONSUInteger i = 0; i < count; ++i) {
            ((unsigned char *)buffer)[i] = ((const unsigned char *)contents.thin)[range.location + i];
        }
    }
    if (usedLength != NULL) {
        *usedLength = count;
    }
    if (remainingRange != NULL) {
        *remainingRange = (ONSRange){range.location + count, range.length - count};
    }
    return YES;
}

// Native -[NSCheapMutableString getCharacters:range:], 0x1c0570e34. Non-external.
- (void)getCharacters:(unsigned short *)buffer range:(ONSRange)range {
    if (flags.isFat) {
        memmove(buffer, contents.fat + range.location, range.length * sizeof(*buffer));
        return;
    }
    for (ONSUInteger i = 0; i < range.length; ++i) {
        buffer[i] = ((const unsigned char *)contents.thin)[range.location + i];
    }
}

// Native -[NSCheapMutableString fastestEncoding], 0x1c0570e8c. Non-external.
- (ONSUInteger)fastestEncoding {
    if (flags.isFat) {
        return 10;
    }
    if (__ONSDefaultStringEncodingFullyInited == 1) {
        return _ONSDefaultStringEncoding;
    }
    return _ONSDefaultCStringEncoding();
}

// Native -[NSCheapMutableString cString], 0x1c0570ecc. Non-external.
- (const char *)cString {
    if (flags.isFat) {
        return [super cString];
    }
    return contents.thin;
}

@end
