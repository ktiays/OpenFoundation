//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/PortCoding/ONSPortCodingCategories.h"

// Inferred organization: the port-coding categories of NSError (0x1c064e550) and NSNotification (0x1c064e6ec)
// lie together among the port-coder code of the iOS Simulator __text section; the native source path is not
// in the evidence, and the two categories share this translation unit, named for that cluster. Objective-C
// compiled with manual reference counting natively. iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: Local/Scratch/cf-core-fd-misc/Reading.md; original assembly in
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json (family FM-misc).

// The selector these categories send to the port coder, declared for the message send only; no protocol
// object is emitted.
@protocol PortCodingMessages

- (BOOL)isByref;

@end

// NSObject's -replacementObjectForPortCoder: (Foundation's NSObject port-coding category, not restored on
// this branch), declared for the super send only; no category is emitted.
@interface NSObject (PortCodingDeclarations)

- (id)replacementObjectForPortCoder:(id)coder;

@end

@implementation ONSError (NSErrorPortCoding)

// Native -[NSError(NSErrorPortCoding) replacementObjectForPortCoder:], 0x1c064e550. External class,
// non-external method. A by-reference coder gets the superclass's replacement; otherwise the receiver itself
// (passed by copy).
- (id)replacementObjectForPortCoder:(id)coder {
    if ([coder isByref]) {
        return [super replacementObjectForPortCoder:coder];
    }
    return self;
}

@end

@implementation ONSNotification (NSNotificationPortCoding)

// Native -[NSNotification(NSNotificationPortCoding) replacementObjectForPortCoder:], 0x1c064e6ec. External
// class, non-external method. The receiver itself (a bare RET).
- (id)replacementObjectForPortCoder:(id)coder {
    return self;
}

@end
