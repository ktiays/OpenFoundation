//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSStringURLUtilities.h"
#import "Foundation/String/ONSCharacterSet.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSLog.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <objc/objc.h>

// Inferred organization: the NSURLUtilities methods lie in the Foundation image's URL utilities region
// (0x1c05ebbc4-0x1c05ebdd0); following the FS-string ownership the category has its own unit here.
// Objective-C compiled with manual reference counting. Unless a definition says otherwise:
// iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Foundation/String/ONSStringReview.md.

// libobjc's objc_autorelease, which the native code imports (the SDK installs no declaration for it).
extern id objc_autorelease(id object);

@implementation ONSString (NSURLUtilities)

// Native -[NSString(NSURLUtilities) stringByAddingPercentEncodingWithAllowedCharacters:], 0x1c05ebbc4.
// Non-external. The shared feature gate's exact once token, block and byte are used; only a byte equal to 1
// selects the Swift message. The other path calls restored CoreFoundation with kCFAllocatorDefault, then
// both paths autorelease the +1 result. The once initializer is shared with NSCharacterSet (0x1c0fdc9cc).
- (id)stringByAddingPercentEncodingWithAllowedCharacters:(id)allowedCharacters {
    id string;
    if (foundationSwiftCharacterSetEnabled()) {
        string = [(id)self __copySwiftStringByAddingPercentEncodingWithAllowedCharacters:allowedCharacters];
    } else {
        string = (id)_OCFStringCreateByAddingPercentEncodingWithAllowedCharacters(
            kOCFAllocatorDefault, (OCFStringRef)self, (OCFCharacterSetRef)allowedCharacters
        );
    }
    return objc_autorelease(string);
}

// Native -[NSString(NSURLUtilities) stringByReplacingPercentEscapesUsingEncoding:], 0x1c05ebd44.
// Non-external. An encoding CFStringConvertNSStringEncodingToEncoding cannot map (kCFStringEncodingInvalidId,
// a 32-bit compare) becomes kCFStringEncodingInvalidId again unless it is 0 or 0x8000100: those log (on
// programs linked on or after version 6) and become 0x600 (ASCII). The result of
// CFURLCreateStringByReplacingPercentEscapes- UsingEncoding (default allocator, nothing left escaped) is
// autoreleased.
- (id)stringByReplacingPercentEscapesUsingEncoding:(ONSUInteger)encoding {
    OCFStringEncoding cfEncoding = OCFStringConvertNSStringEncodingToEncoding(encoding);
    if (cfEncoding == 0xFFFFFFFF) {
        if (encoding == 0x8000100 || encoding == 0) {
            if (_OCFExecutableLinkedOnOrAfter(6)) {
                ONSLog(
                    (id)OF_STR(
                        "Incorrect NSStringEncoding value 0x%04lX detected. Assuming NSASCIIStringEncoding. "
                        "Will stop this compatibility mapping behavior in the near future."
                    ),
                    encoding
                );
            }
            cfEncoding = 0x600;
        } else {
            cfEncoding = 0xFFFFFFFF;
        }
    }
    return objc_autorelease((id)OCFURLCreateStringByReplacingPercentEscapesUsingEncoding(
        NULL, (OCFStringRef)self, OF_STR(""), cfEncoding
    ));
}

@end
