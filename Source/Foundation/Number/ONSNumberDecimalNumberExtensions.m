//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Number/ONSNumberDecimalNumberExtensions.h"

#import "Foundation/Number/ONSUnrestoredInterfaces.h"

#import "CoreFoundation/Objective-C/OCFNSObject.h"

#include <string.h>

// Inferred organization: -[NSNumber(NSDecimalNumberExtensions) decimalValue] (0x1c04fb228) lies among the
// NSDecimalNumber methods in the iOS Simulator __text section, in the decimal-number unit, which no family of
// this track restores; this file holds the category, named for the native cluster. Objective-C compiled with
// manual reference counting. Foundation, iOS Simulator 24A434, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Source/Foundation/Number/Reading.md.

// Class objects the method loads directly (GOT slots off_22289F330 and off_22289EDD8).
extern char scannerClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSScanner));
extern char decimalNumberClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDecimalNumber));

@implementation ONSNumber (NSDecimalNumberExtensions)

// Native -[NSNumber(NSDecimalNumberExtensions) decimalValue], 0x1c04fb228. Non-external. Inside an
// autorelease pool (_CFAutoreleasePoolPush/Pop): an NSScanner over -stringValue scans a decimal into the
// zeroed result; when the scan fails, the result is +[NSDecimalNumber notANumber]'s -decimalValue (all zero
// when that is nil).
- (ONSDecimal)decimalValue {
    void *pool = _OCFAutoreleasePoolPush();
    id scanner = [(id)scannerClass scannerWithString:[self stringValue]];
    ONSDecimal result = {0};
    if (![scanner scanDecimal:&result]) {
        id notANumber = [(id)decimalNumberClass notANumber];
        ONSDecimal decimal;
        if (notANumber != nil) {
            decimal = [notANumber decimalValue];
        } else {
            memset(&decimal, 0, sizeof(decimal));
        }
        result = decimal;
    }
    _OCFAutoreleasePoolPop(pool);
    return result;
}

@end
