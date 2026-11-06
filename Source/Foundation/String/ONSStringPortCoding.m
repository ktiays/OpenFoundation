//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSStringPortCoding.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

// Inferred organization: -[NSString(NSStringPortCoding) replacementObjectForPortCoder:] (0x1c064e804) lies in
// the Foundation image's port coding region; following the FS-string ownership the category has its own unit
// here. Objective-C compiled with manual reference counting. iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Foundation/String/ONSStringReview.md.

@implementation ONSString (NSStringPortCoding)

// Native -[NSString(NSStringPortCoding) replacementObjectForPortCoder:], 0x1c064e804. Non-external. A bare
// RET: the receiver (X0) is the result.
- (id)replacementObjectForPortCoder:(id)coder {
    (void)coder;
    return self;
}

@end
