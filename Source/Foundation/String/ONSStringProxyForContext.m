//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSStringProxyForContext.h"
#import "Foundation/Archiving/ONSObjCRuntimeNames.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/Locale/OCFLocale.h"
#include "CoreFoundation/Objective-C/OCFNSException.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/objc-exception.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

extern char contextProxyStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));
extern char contextProxyExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));

@protocol StringProxyFormatter
- (void)setFormattingContext:(NSInteger)context;
- (id)stringForObjectValue:(id)object;
@end

@protocol StringProxyCoder
- (BOOL)allowsKeyedCoding;
- (id)decodeObjectForKey:(id)key;
- (void)encodeObject:(id)object forKey:(id)key;
@end

@implementation _ONSStringProxyForContext

// Native -[_NSStringProxyForContext dealloc], 0x1c054df84. Non-external.
- (void)dealloc {
    if (_string != nil) {
        objc_release(_string);
    }
    if (_formatter != nil) {
        objc_release(_formatter);
    }
    if (_item != nil) {
        objc_release(_item);
    }
    [super dealloc];
}

// Native -[_NSStringProxyForContext length], 0x1c054e02c. Non-external.
- (ONSUInteger)length {
    return [_string length];
}

// Native -[_NSStringProxyForContext characterAtIndex:], 0x1c054e03c. Non-external.
- (unsigned short)characterAtIndex:(ONSUInteger)index {
    return [_string characterAtIndex:index];
}

// Native -[_NSStringProxyForContext getCharacters:range:], 0x1c054e04c. Non-external.
- (void)getCharacters:(unsigned short *)buffer range:(ONSRange)range {
    [_string getCharacters:buffer range:range];
}

// Native -[_NSStringProxyForContext _retainFormatter:], 0x1c054e05c. Non-external.
- (void)_retainFormatter:(id)formatter {
    if (_formatter != formatter) {
        objc_release(_formatter);
        _formatter = objc_retain(formatter);
    }
}

// Native -[_NSStringProxyForContext _dynamicContextEvaluation:patternString:], 0x1c054e0a4.
// Non-external. The CBZ at 0x1c054e0ec tests the range location. An absent pattern (NSNotFound)
// enters the tokenizer path, while a match at offset zero takes the immediate context-4 path (kept).
// The allocator slot at 0x222280b50 is kCFAllocatorDefault (confidence: medium).
- (id)_dynamicContextEvaluation:(const void *)context patternString:(const void *)patternString {
    ONSString *string = (ONSString *)context;
    NSInteger formattingContext;
    if ([string isEqualToString:(id)patternString]) {
        formattingContext = 2;
    } else if ([string rangeOfString:(id)patternString].location != 0) {
        OCFMutableStringRef uppercase = OCFStringCreateMutableCopy(NULL, 0, (OCFStringRef)context);
        OCFStringUppercase(uppercase, NULL);
        OCFStringRef copy = OCFStringCreateCopy(NULL, uppercase);
        OCFRelease(uppercase);
        OCFLocaleRef locale = OCFLocaleCopyCurrent();
        OCFAllocatorRef allocator = kOCFAllocatorDefault;
        OCFRange range = {0, OCFStringGetLength(copy)};
        OCFStringTokenizerRef tokenizer = OCFStringTokenizerCreate(allocator, copy, range, 1, locale);
        formattingContext = 5;
        while (OCFStringTokenizerAdvanceToNextToken(tokenizer) != 0) {
            OCFRange tokenRange = OCFStringTokenizerGetCurrentTokenRange(tokenizer);
            OCFStringRef token = OCFStringCreateWithSubstring(NULL, copy, tokenRange);
            OCFIndex location = OCFStringFind(token, (OCFStringRef)patternString, 0).location;
            if ((OCFOptionFlags)location + 1 >= 2) {
                OCFRelease(token);
                formattingContext = 5;
                break;
            }
            OCFRelease(token);
            if (location == 0) {
                formattingContext = 4;
                break;
            }
        }
        OCFRelease(copy);
        OCFRelease(tokenizer);
        OCFRelease(locale);
    } else {
        formattingContext = 4;
    }
    id<StringProxyFormatter> formatter = [self formatter];
    if (!objc_opt_respondsToSelector(formatter, @selector(setFormattingContext:))) {
        return nil;
    }
    [formatter setFormattingContext:formattingContext];
    return [formatter stringForObjectValue:[self item]];
}

// Native -[_NSStringProxyForContext copyWithZone:], 0x1c054e258. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    _ONSStringProxyForContext *copy = [[objc_opt_class(self) allocWithZone:zone] init];
    [copy setString:[self string]];
    [copy setFormatter:[self formatter]];
    [copy setItem:[self item]];
    return copy;
}

// Native -[_NSStringProxyForContext initWithCoder:], 0x1c054e2cc. Non-external.
// NSException at 0x22227fae0 and NSInvalidArgumentException at 0x22227f500 have medium confidence.
- (id)initWithCoder:(id<StringProxyCoder>)coder {
    if (![coder allowsKeyedCoding]) {
        id name = ONSStringFromClass(objc_opt_class(self));
        id reason = [(id)contextProxyStringClass
            stringWithFormat:(id)OF_STR("%@ can only be decoded by a keyed coder."), name];
        objc_release(self);
        objc_exception_throw([(id)contextProxyExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                        reason:reason
                                                                      userInfo:nil]);
    }
    id string = [coder decodeObjectForKey:(id)OF_STR("NSStringProxyForContext.string")];
    id formatter = [coder decodeObjectForKey:(id)OF_STR("NSStringProxyForContext.formatter")];
    id item = [coder decodeObjectForKey:(id)OF_STR("NSStringProxyForContext.item")];
    _string = objc_retain(string);
    _formatter = objc_retain(formatter);
    _item = objc_retain(item);
    return self;
}

// Native -[_NSStringProxyForContext encodeWithCoder:], 0x1c054e3c8. Non-external.
- (void)encodeWithCoder:(id<StringProxyCoder>)coder {
    if (![coder allowsKeyedCoding]) {
        id name = ONSStringFromClass(objc_opt_class(self));
        id reason = [(id)contextProxyStringClass
            stringWithFormat:(id)OF_STR("%@ can only be encoded by a keyed coder."), name];
        objc_exception_throw([(id)contextProxyExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                        reason:reason
                                                                      userInfo:nil]);
    }
    [coder encodeObject:_string forKey:(id)OF_STR("NSStringProxyForContext.string")];
    [coder encodeObject:_formatter forKey:(id)OF_STR("NSStringProxyForContext.formatter")];
    [coder encodeObject:_item forKey:(id)OF_STR("NSStringProxyForContext.item")];
}

// Native -[_NSStringProxyForContext string], 0x1c054e49c. Non-external.
// Native -[_NSStringProxyForContext setString:], 0x1c054e4ac. Non-external.
@synthesize string = _string;

// Native -[_NSStringProxyForContext formatter], 0x1c054e4b8. Non-external.
// Native -[_NSStringProxyForContext setFormatter:], 0x1c054e4c8. Non-external.
@synthesize formatter = _formatter;

// Native -[_NSStringProxyForContext item], 0x1c054e4d4. Non-external.
// Native -[_NSStringProxyForContext setItem:], 0x1c054e4e4. Non-external.
@synthesize item = _item;

@end
