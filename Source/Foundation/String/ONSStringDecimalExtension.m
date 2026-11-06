//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSStringDecimalExtension.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"

#include <string.h>

// Inferred organization: -[NSString(NSDecimalExtension) decimalValue] (0x1c04fb7d8) lies in the Foundation
// image's NSDecimal region; following the FS-string ownership the category has its own unit here.
// Objective-C compiled with manual reference counting. iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Foundation/String/ONSStringReview.md.

// The NSScanner and NSDecimalNumber class symbols (through the __SCR GOT slots 0x22289F330 and 0x22289EDD8);
// neither class is restored on this branch.

@implementation ONSString (NSDecimalExtension)

// Native -[NSString(NSDecimalExtension) decimalValue], 0x1c04fb7d8. Non-external. The result (returned
// through X8) is zeroed after +[NSScanner scannerWithString:] and filled by -scanDecimal:; when that answers
// NO it becomes -decimalValue of +[NSDecimalNumber notANumber] (zero when that is nil; the nil check is
// explicit in the native code). The work runs inside an autorelease pool.
- (ONSDecimal)decimalValue {
    ONSDecimal result;
    void *pool = _OCFAutoreleasePoolPush();
    id scanner = [(id)scannerClassSymbol scannerWithString:self];
    memset(&result, 0, sizeof(result));
    if (![scanner scanDecimal:&result]) {
        id notANumber = [(id)decimalNumberClassSymbol notANumber];
        ONSDecimal value;
        if (notANumber != nil) {
            value = [notANumber decimalValue];
        } else {
            memset(&value, 0, sizeof(value));
        }
        result = value;
    }
    _OCFAutoreleasePoolPop(pool);
    return result;
}

@end
