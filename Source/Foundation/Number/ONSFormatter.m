//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Number/ONSFormatter.h"

#import "CoreFoundation/Objective-C/OCFNSSet.h"

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Foundation/Number/Reading.md, round 2. Manual reference counting.

@implementation ONSFormatter

// Native -[NSFormatter copyWithZone:], 0x1c0515480. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

// Native -[NSFormatter encodeWithCoder:], 0x1c0515484. Non-external.
- (void)encodeWithCoder:(id)coder {
}

// Native -[NSFormatter initWithCoder:], 0x1c0515488. Non-external. RET preserves the receiver.
- (id)initWithCoder:(id)coder {
    return self;
}

@end
