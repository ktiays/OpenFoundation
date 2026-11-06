//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSStringOriginalAttributedString.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

// Inferred organization: -[NSString(_NSOriginalAttributedString) __baseAttributedString] (0x1c04d10f8) lies
// in the Foundation image's attributed string region; following the FS-string ownership the category has its
// own unit here. Objective-C compiled with manual reference counting. iOS Simulator 24A434, arm64, Foundation
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Foundation/String/ONSStringReview.md.

@implementation ONSString (_NSOriginalAttributedString)

// Native -[NSString(_NSOriginalAttributedString) __baseAttributedString], 0x1c04d10f8. Non-external. Answers
// nil.
- (id)__baseAttributedString {
    return nil;
}

@end
