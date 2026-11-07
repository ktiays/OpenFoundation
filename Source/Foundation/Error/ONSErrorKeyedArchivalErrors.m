//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Error/ONSErrorKeyedArchivalErrors.h"

#include "CoreFoundation/String/OCFString.h"
#import "Foundation/Error/ONSErrorConstants.h"

#include <stdarg.h>

// Inferred organization: +[NSError(NSKeyedArchivalErrors) _readCorruptErrorWithFormat:] (0x1c0563b20) lies
// among the keyed-archiving code of the iOS Simulator __text section, apart from the NSError class; the
// native source path is not in the evidence, and the category has its own translation unit here. Objective-C
// compiled with manual reference counting natively. iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: Local/Scratch/cf-core-fd-misc/Reading.md; original assembly in
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json (family FM-misc).

// The NSString class (Foundation's, the GOT slot 0x22289f3d8) and the NSDictionary class (CoreFoundation's;
// the uniqued GOT slot 0x22227fad0 lies outside the image and is bound by the selector sent and the slots'
// alphabetical order; confidence: medium).
extern char stringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char dictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDictionary));

// Selectors this unit sends to NSString and NSDictionary, declared for the message sends only; no protocol
// object is emitted.
@protocol KeyedArchivalErrorMessages

- (id)initWithFormat:(id)format arguments:(va_list)arguments;
+ (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;

@end

@implementation ONSError (NSKeyedArchivalErrors)

// Native +[NSError(NSKeyedArchivalErrors) _readCorruptErrorWithFormat:], 0x1c0563b20. External class,
// non-external method. The formatted description (+alloc on NSString, -initWithFormat:arguments: over the
// variadic arguments) becomes the one-entry user info under NSDebugDescriptionErrorKey and is then released;
// a nil description leaves the user info nil. Returns +[NSError errorWithDomain:code:userInfo:] (the class
// NSError, not the receiver) with NSCocoaErrorDomain and code 4864 (NSCoderReadCorruptError, 0x1300).
+ (id)_readCorruptErrorWithFormat:(id)format, ... {
    va_list arguments;
    va_start(arguments, format);
    id description = [[(id)stringClass alloc] initWithFormat:format arguments:arguments];
    va_end(arguments);
    id userInfo = nil;
    if (description != nil) {
        id key = (id)ONSDebugDescriptionErrorKey;
        userInfo = [(id)dictionaryClass dictionaryWithObjects:&description forKeys:&key count:1];
        [description release];
    }
    return [ONSError errorWithDomain:(id)ONSCocoaErrorDomain code:0x1300 userInfo:userInfo];
}

@end
