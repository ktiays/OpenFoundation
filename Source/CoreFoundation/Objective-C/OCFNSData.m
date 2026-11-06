//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSData.h"

// Inferred organization: +[NSData supportsSecureCoding] (0x1c01b17dc) is the only NSData method of this
// image; it lies in the iOS Simulator __text section before the two NSPort methods, among the bridged-class
// methods. Following the owner's decision, the class has its own translation unit. Objective-C compiled with
// manual reference counting. The native source path is not in the evidence. iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.

@implementation ONSData

// Native +[NSData supportsSecureCoding], 0x1c01b17dc. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

@end
