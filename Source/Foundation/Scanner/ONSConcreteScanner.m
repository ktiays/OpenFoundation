//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Scanner/ONSConcreteScanner.h"

#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "Foundation/String/ONSCharacterSet.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSStringScanning.h"

#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include <objc/objc-exception.h>

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

@implementation ONSConcreteScanner

// Native -[NSConcreteScanner initWithString:], 0x1c05c36e4. Non-external.
- (id)initWithString:(id)string {
    if (string == nil) {
        // OF_STR supplies a static format object. Clang cannot see its literal through the cast.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
        ONSLog((id)OF_STR("NSScanner: nil string argument"));
#pragma clang diagnostic pop
        string = (id)OF_STR("");
    }
    self = [super initWithString:string];
    scanString = [string copyWithZone:NULL];
    skipSet = objc_retain([ONSCharacterSet whitespaceAndNewlineCharacterSet]);
    scanLocation = 0;
    return self;
}

// Native -[NSConcreteScanner string], 0x1c05c37a4. Non-external.
- (id)string {
    return objc_autorelease(objc_retain(scanString));
}

// Native -[NSConcreteScanner setScanLocation:], 0x1c05c384c. Non-external.
- (void)setScanLocation:(NSUInteger)location {
    NSUInteger length = [scanString length];
    if (location > length) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        id reason = [ONSString stringWithFormat:(id)OF_STR("%@: Index %lu out of bounds; string length %lu"),
                                                proem,
                                                location,
                                                length];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSRangeException reason:reason
                                                    userInfo:nil]);
    }
    scanLocation = location;
}

// Native -[NSConcreteScanner scanLocation], 0x1c05c38f0. Non-external.
- (NSUInteger)scanLocation {
    return scanLocation;
}

// Native -[NSConcreteScanner setCharactersToBeSkipped:], 0x1c05c38f8. Non-external.
- (void)setCharactersToBeSkipped:(id)set {
    objc_autorelease(skipSet);
    skipSet = [set copyWithZone:[self zone]];
    objc_release(invertedSkipSet);
    invertedSkipSet = nil;
}

// Native -[NSConcreteScanner charactersToBeSkipped], 0x1c05c3944. Non-external.
- (id)charactersToBeSkipped {
    return objc_autorelease(objc_retain(skipSet));
}

// Native -[NSConcreteScanner setCaseSensitive:], 0x1c05c395c. Non-external.
- (void)setCaseSensitive:(BOOL)value {
    flags.caseSensitive = value;
}

// Native -[NSConcreteScanner caseSensitive], 0x1c05c3970. Non-external.
- (BOOL)caseSensitive {
    return flags.caseSensitive;
}

// Native -[NSConcreteScanner setLocale:], 0x1c05c397c. Non-external.
- (void)setLocale:(id)value {
    if (locale != value) {
        objc_release(locale);
        locale = objc_retain(value);
    }
}

// Native -[NSConcreteScanner locale], 0x1c05c39b4. Non-external.
- (id)locale {
    return objc_autorelease(objc_retain(locale));
}

// Native -[NSConcreteScanner scanInt:], 0x1c05c39cc. Non-external.
- (BOOL)scanInt:(int *)result {
    return _NSScanIntFromString(scanString, skipSet, result, &scanLocation);
}

// Native -[NSConcreteScanner dealloc], 0x1c05c39ec. Non-external.
- (void)dealloc {
    objc_release(scanString);
    objc_release(skipSet);
    objc_release(invertedSkipSet);
    objc_release(locale);
    [super dealloc];
}

@end
