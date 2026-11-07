//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Number/ONSJSONRoundTrippingNumber.h"

#import "CoreFoundation/Objective-C/OCFNSDictionary.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Error/ONSErrorConstants.h"
#import "Foundation/Number/ONSNumberKindOfAdditions.h"
#import "Foundation/Number/ONSUnrestoredInterfaces.h"

#import <objc/objc-exception.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified except the external class and exception bindings. iOS Device divergence:
// iOS Device 24A446 remains unread. Reading: Foundation/Number/Reading.md, round 2. Manual reference
// counting.

extern char jsonRoundTrippingNumberClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                     OF_NAME_STRING(_ONSJSONRoundTrippingNumber));
extern char jsonNumberClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSNumber));
extern char jsonStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));

@implementation _ONSJSONRoundTrippingNumber

// Native -[_NSJSONRoundTrippingNumber initWithNumber:representation:], 0x1c0630084. Non-external.
- (id)initWithNumber:(id)number representation:(id)representation {
    self = [super init];
    if (self != nil) {
        _number = objc_retain(number);
        _representation = [representation copy];
    }
    return self;
}

// Native -[_NSJSONRoundTrippingNumber dealloc], 0x1c063011c. Non-external.
- (void)dealloc {
    objc_release(_number);
    objc_release(_representation);
    [super dealloc];
}

// Native -[_NSJSONRoundTrippingNumber stringValue], 0x1c0630198. Non-external.
- (id)stringValue {
    return objc_autorelease([_representation copy]);
}

// Native -[_NSJSONRoundTrippingNumber isEqual:], 0x1c06301b0. Non-external.
// At 0x1c06301d4 the stub's symbol names the selector isNSNumber__ (the pseudocode drops the suffix).
- (BOOL)isEqual:(id)value {
    if (value == nil) {
        return NO;
    }
    if (self == value) {
        return YES;
    }
    if ([value isNSNumber__]) {
        return [self isEqualToNumber:value];
    }
    return NO;
}

// Native -[_NSJSONRoundTrippingNumber isEqualToNumber:], 0x1c0630208. Non-external.
- (BOOL)isEqualToNumber:(id)number {
    if (self == number) {
        return YES;
    }
    if (number != nil && object_getClass(number) == (Class)jsonRoundTrippingNumberClass) {
        return [_representation isEqualToString:((_ONSJSONRoundTrippingNumber *)number)->_representation];
    }
    return NO;
}

// Native -[_NSJSONRoundTrippingNumber hash], 0x1c063026c. Non-external.
- (NSUInteger)hash {
    return [_representation hash];
}

// Native -[_NSJSONRoundTrippingNumber initWithCoder:], 0x1c0630284. Non-external.
// The dictionary class reference at 0x22227fad0 is bound by use (medium confidence).
- (id)initWithCoder:(id)coder {
    if (!objc_opt_isKindOfClass(coder, (Class)jsonXPCCoderClass)) {
        objc_release(self);
        objc_exception_throw([ONSException
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:(id)OF_STR("_NSJSONRoundTrippingNumber can only be decoded by NSXPCCoder.")
                     userInfo:nil]);
    }
    id number = [coder decodeObjectOfClass:objc_opt_class((id)jsonNumberClass)
                                    forKey:(id)OF_STR("NS.number")];
    id detail;
    if (number != nil) {
        id representation = [coder decodeObjectOfClass:objc_opt_class((id)jsonStringClass)
                                                forKey:(id)OF_STR("NS.representation")];
        if (representation != nil) {
            return [self initWithNumber:number representation:representation];
        }
        objc_release(self);
        detail = (id)OF_STR("Unable to decode underlying string representation for round-tripping number.");
    } else {
        objc_release(self);
        detail = (id)OF_STR("Unable to decode underlying NSNumber instance for round-tripping number.");
    }
    id key = ONSDebugDescriptionErrorKey;
    id userInfo = [ONSDictionary dictionaryWithObjects:&detail forKeys:&key count:1];
    id error = [ONSError errorWithDomain:ONSCocoaErrorDomain code:4864 userInfo:userInfo];
    [coder failWithError:error];
    return nil;
}

// Native -[_NSJSONRoundTrippingNumber encodeWithCoder:], 0x1c0630454. Non-external.
// The exception path ends at objc_exception_throw, before the next function shown by Hex-Rays.
- (void)encodeWithCoder:(id)coder {
    if (!objc_opt_isKindOfClass(coder, (Class)jsonXPCCoderClass)) {
        objc_exception_throw([ONSException
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:(id)OF_STR("_NSJSONRoundTrippingNumber can only be encoded by NSXPCCoder.")
                     userInfo:nil]);
    }
    [coder encodeObject:_number forKey:(id)OF_STR("NS.number")];
    [coder encodeObject:_representation forKey:(id)OF_STR("NS.representation")];
}

// Native -[_NSJSONRoundTrippingNumber superclass], 0x1c06304d4. Non-external.
// This selector reports NSNumber while the class metadata names NSObject (kept).
- (Class)superclass {
    return objc_opt_class((id)jsonNumberClass);
}

// Native -[_NSJSONRoundTrippingNumber isNSNumber__], 0x1c06305d0. Non-external.
- (BOOL)isNSNumber__ {
    return YES;
}

// Native +[_NSJSONRoundTrippingNumber instancesRespondToSelector:], 0x1c06305d8. Non-external. Only the
// selectors the class implements itself (identity comparisons, in the native order).
+ (BOOL)instancesRespondToSelector:(SEL)selector {
    return selector == @selector(initWithNumber:representation:) || selector == @selector(dealloc) ||
           selector == @selector(stringValue) || selector == @selector(initWithCoder:) ||
           selector == @selector(encodeWithCoder:) || selector == @selector(_allowsDirectEncoding) ||
           selector == @selector(isEqual:) || selector == @selector(isEqualToNumber:) ||
           selector == @selector(hash);
}

// Native -[_NSJSONRoundTrippingNumber respondsToSelector:], 0x1c0630658. Non-external.
- (BOOL)respondsToSelector:(SEL)selector {
    return [[self class] instancesRespondToSelector:selector];
}

@end
