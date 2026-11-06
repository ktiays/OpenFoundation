//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Decimal/ONSDecimalParsing.h"
#import "Foundation/Base/ONSUnrestoredInterfaces.h"

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified; iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Scratch/cf-core-fd-misc/Reading.md.

// Native _NSCStringToDecimal, 0x1c04f914c. Was a private external.
// Objective-C/MRC: objc_alloc + initWithCString:encoding: (UTF-8 = 4), then _NSStringToDecimal, then release.
// x8 is saved before allocation and receives the 20-byte returned struct; this is not a third C argument.
ONSDecimal _NSCStringToDecimal(const char *string, char **end) {
    id object = [[ONSString alloc] initWithCString:string encoding:4];
    ONSUInteger length = 0;
    ONSDecimal result = {0};
    _NSStringToDecimal(object, &length, &result);
    *end = (char *)string + length;
    [object release];
    return result;
}
