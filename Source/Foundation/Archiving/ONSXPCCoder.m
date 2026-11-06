//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/XPC/ONSXPCClasses.h"
#import "Foundation/Archiving/ONSXPCCoder.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

extern id objc_getProperty(id object, SEL selector, ptrdiff_t offset, BOOL atomic);

// Native initializeClasses.onceToken. Its global block literal (0x22281ded8) is FX's
// xpcInitializeClassesBlock.
static dispatch_once_t initializeClassesToken;

@implementation ONSXPCCoder

// Native -[NSXPCCoder requiresSecureCoding], 0x1c06a60b8. Non-external.
- (BOOL)requiresSecureCoding {
    return YES;
}

// Native -[NSXPCCoder init], 0x1c06a60dc. Non-external.
- (id)init {
    self = [super init];
    if (self != nil && initializeClassesToken != -1) {
        dispatch_once(&initializeClassesToken, xpcInitializeClassesBlock);
    }
    return self;
}

// Native -[NSXPCCoder dealloc], 0x1c06a6178. Non-external.
- (void)dealloc {
    objc_release(_userInfo);
    [super dealloc];
}

// Native -[NSXPCCoder userInfo], 0x1c06a64e4. Non-external.
- (id)userInfo {
    return objc_getProperty(self, _cmd, (char *)&_userInfo - (char *)self, YES);
}

@end
