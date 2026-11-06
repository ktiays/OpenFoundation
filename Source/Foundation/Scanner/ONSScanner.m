//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Scanner/ONSScanner.h"

#include "CoreFoundation/String/OCFString.h"
#include <objc/runtime.h>

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured assembly; iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Scratch/cf-core-fd-misc/Reading.md; native source paths unread, organization inferred.

extern id objc_opt_self(id object);
extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *pool);
extern char scannerClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSScanner));
extern char concreteScannerClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSConcreteScanner));

// Native _NSScanDoubleFromString 0x1c05c18d0 and _NSScanIntFromString 0x1c05c11f8, both private externals.
// Missing from the supplied capture; these signatures follow the argument registers, and stay undefined.
extern int _NSScanDoubleFromString(id string, id skip, double *result, NSUInteger *location, id locale);
extern int _NSScanIntFromString(id string, id skip, int *result, NSUInteger *location);

@implementation ONSScanner

// Native +[NSScanner allocWithZone:], 0x1c05c0a94. External class, non-external method.
+ (id)allocWithZone:(struct _NSZone *)zone {
    Class cls = self;
    if (self == (Class)objc_opt_self((id)scannerClass)) {
        cls = (Class)objc_opt_self((id)concreteScannerClass);
    }
    return ONSAllocateObject(cls, 0, zone);
}

// Native -[NSScanner initWithString:], 0x1c05c0ae4. Non-external method; the superclass selector is init.
- (id)initWithString:(id)string {
    return [super init];
}

// Native +[NSScanner scannerWithString:], 0x1c05c0b44. Non-external method; ignores the receiving class.
+ (id)scannerWithString:(id)string {
    return [[[(id)concreteScannerClass allocWithZone:NULL] initWithString:string] autorelease];
}

// Native -[NSScanner copyWithZone:], 0x1c05c0bc8. Non-external method; ignores the requested zone (kept).
- (id)copyWithZone:(struct _NSZone *)zone {
    id allocated = [(id)concreteScannerClass allocWithZone:NULL];
    id string = [self string];
    id result = [allocated initWithString:string];
    [result setCharactersToBeSkipped:[self charactersToBeSkipped]];
    [result setScanLocation:[self scanLocation]];
    [result setCaseSensitive:[self caseSensitive]];
    [result setLocale:[self locale]];
    return result;
}

// Native -[NSScanner scanDouble:], 0x1c05c226c. Non-external method.
- (BOOL)scanDouble:(double *)result {
    void *pool = objc_autoreleasePoolPush();
    NSUInteger location = [self scanLocation];
    id string = [self string];
    id skip = [self charactersToBeSkipped];
    id locale = [self locale];
    int scanned = _NSScanDoubleFromString(string, skip, result, &location, locale);
    if (scanned != 0) {
        [self setScanLocation:location];
    }
    objc_autoreleasePoolPop(pool);
    return scanned;
}

// Native -[NSScanner scanInt:], 0x1c05c2c14. Non-external method.
- (BOOL)scanInt:(int *)result {
    void *pool = objc_autoreleasePoolPush();
    NSUInteger location = [self scanLocation];
    id string = [self string];
    id skip = [self charactersToBeSkipped];
    int scanned = _NSScanIntFromString(string, skip, result, &location);
    if (scanned != 0) {
        [self setScanLocation:location];
    }
    objc_autoreleasePoolPop(pool);
    return scanned;
}

// Native -[NSScanner charactersToBeSkipped], 0x1c05c36a8. Non-external method.
- (id)charactersToBeSkipped {
    return [ONSCharacterSet whitespaceAndNewlineCharacterSet];
}

// Native -[NSScanner caseSensitive], 0x1c05c36b4. Non-external method; native base-class default.
- (BOOL)caseSensitive {
    return NO;
}

// Native -[NSScanner locale], 0x1c05c36bc. Non-external method; native base-class default.
- (id)locale {
    return nil;
}

// Native -[NSScanner string], 0x1c0fe47b8. Non-external method.
- (id)string {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSScanner)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
    return nil;
}

// Native -[NSScanner setScanLocation:], 0x1c0fe47f8. Non-external method.
- (void)setScanLocation:(NSUInteger)location {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSScanner)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
}

// Native -[NSScanner scanLocation], 0x1c0fe4830. Non-external method.
- (NSUInteger)scanLocation {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSScanner)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
    return 0;
}

// Native -[NSScanner setCharactersToBeSkipped:], 0x1c0fe4870. Non-external method.
- (void)setCharactersToBeSkipped:(id)set {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSScanner)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
}

// Native -[NSScanner setCaseSensitive:], 0x1c0fe48a8. Non-external method.
- (void)setCaseSensitive:(BOOL)value {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSScanner)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
}

// Native -[NSScanner setLocale:], 0x1c0fe48e0. Non-external method.
- (void)setLocale:(id)locale {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSScanner)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
}

@end
