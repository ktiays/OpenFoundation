//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Number/ONSValuePortCoding.h"

// Inferred organization: the port-coding categories (-replacementObjectForPortCoder: of NSAttributedString at
// 0x1c064e3a0, NSValue at 0x1c064e890 and others) lie together in the iOS Simulator __text section, in the
// port-coder unit; this file holds the NSValue category, named for the native cluster. Objective-C compiled
// with manual reference counting. Foundation, iOS Simulator 24A434, UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Source/Foundation/Number/Reading.md.

@implementation ONSValue (NSValuePortCoding)

// Native -[NSValue(NSValuePortCoding) replacementObjectForPortCoder:], 0x1c064e890. Non-external. Answers the
// receiver (RET).
- (id)replacementObjectForPortCoder:(id)coder {
    return self;
}

@end
