//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSStringFromRange.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

// Inferred organization: NSStringFromRange (0x1c05bd710) lies in the Foundation image's NSRange functions
// beside NSUnionRange (0x1c05bd644) and NSIntersectionRange (0x1c05bd664), which Range/ONSRange.c restores;
// following the FS-string ownership it has its own unit here. Objective-C compiled with manual reference
// counting. iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence:
// verified (assembly read in full, iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device
// 24A446 remains unread. Reading: Foundation/String/ONSStringReview.md.

// Native NSStringFromRange, 0x1c05bd710. External. +[NSString stringWithFormat:] with "{%lu, %lu}".
id ONSStringFromRange(ONSRange range) {
    return [ONSString stringWithFormat:(id)OF_STR("{%lu, %lu}"), range.location, range.length];
}
