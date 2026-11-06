//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSPort.h"

// Inferred organization: the two NSPort methods of this image (0x1c01b17e4, 0x1c01b17f0) lie in the iOS
// Simulator __text section after +[NSData supportsSecureCoding], among the bridged-class methods; following
// the owner's decision, the class has its own translation unit. Objective-C compiled with manual reference
// counting. The native source path is not in the evidence. iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.

@implementation ONSPort

// Native -[NSPort isValid], 0x1c01b17e4. Non-external; bit 0 of the _valid byte (read at its fixed offset 8).
- (BOOL)isValid {
    return _valid & 1;
}

// Native -[NSPort reservedSpaceLength], 0x1c01b17f0. Non-external; the ivar (read at its fixed offset 16).
- (NSUInteger)reservedSpaceLength {
    return _reservedSpaceLength;
}

@end
