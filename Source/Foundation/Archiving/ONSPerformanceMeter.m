//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Archiving/ONSPerformanceMeter.h"

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

@implementation _ONSPerformanceMeter

// Native +[_NSPerformanceMeter allocWithZone:], 0x1c04df334. Non-external.
// The native implementation disables allocation (kept).
+ (id)allocWithZone:(struct _NSZone *)zone {
    return nil;
}

// Native -[_NSPerformanceMeter initWithTarget:], 0x1c04df33c. Non-external.
- (id)initWithTarget:(id)target {
    return [super init];
}

// Native -[_NSPerformanceMeter invalidate], 0x1c04df39c. Non-external.
// The native body is a single RET instruction (kept).
- (void)invalidate {
}

@end
