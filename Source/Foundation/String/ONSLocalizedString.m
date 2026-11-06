//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSLocalizedString.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Archiving/ONSKeyedArchiver.h"
#import "Foundation/Error/ONSError.h"
#import "Foundation/Error/ONSErrorConstants.h"
#import "Foundation/String/ONSPlaceholderString.h"
#import "Foundation/String/ONSStringOtherEncodings.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "CoreFoundation/Objective-C/OCFNSDictionary.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/Objective-C/OCFNSSet.h"
#include "CoreFoundation/String/OCFStringFormat.h"

#include <objc/objc-exception.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device 24A446: unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FB-bundle-paths-plist.
// Uniqued shared-cache GOT bindings (medium): NSArray 0x22227fa90, NSData 0x22227fab8,
// NSDate 0x22227fac0, NSDictionary 0x22227fad0, NSException 0x22227fae0, NSSet 0x22227fb68,
// and NSInvalidArgumentException 0x22227f500.
extern char localizedStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSLocalizedString));
extern char localizedMutableStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableString));
extern char localizedDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char localizedArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSArray));
extern char localizedNumberClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNumber));
extern char localizedBaseStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));
extern char localizedDateClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDate));
extern char localizedDataClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSData));

@implementation __ONSLocalizedString

// Native -[__NSLocalizedString _allowsDirectEncoding], 0x1c05d9554. Non-external.
- (BOOL)_allowsDirectEncoding {
    return NO;
}

// Native +[__NSLocalizedString supportsSecureCoding], 0x1c05d955c. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[__NSLocalizedString classForCoder], 0x1c05d9564. Non-external.
- (Class)classForCoder {
    return objc_opt_class((id)localizedStringClass);
}

// Native -[__NSLocalizedString initWithCoder:], 0x1c05d9570. Non-external.
// The assembly loads the exported error-domain and description-key objects, which Hex-Rays renders as
// literals.
- (id)initWithCoder:(ONSCoder *)coder {
    if (![coder allowsKeyedCoding]) {
        [self release];
        objc_exception_throw([ONSException
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:(id)OF_STR("__NSLocalizedString cannot be decoded by non-keyed archivers!")
                     userInfo:nil]);
    }

    id string = [coder decodeObjectOfClass:objc_opt_class((id)localizedMutableStringClass)
                                    forKey:(id)OF_STR("NS.originalString")];
    id description;
    ONSInteger code;
    if (string != nil) {
        Class dictionary = objc_opt_class((id)localizedDictionaryClass);
        Class array = objc_opt_class((id)localizedArrayClass);
        Class number = objc_opt_class((id)localizedNumberClass);
        Class baseString = objc_opt_class((id)localizedBaseStringClass);
        Class date = objc_opt_class((id)localizedDateClass);
        Class data = objc_opt_class((id)localizedDataClass);
        id classes = [ONSSet setWithObjects:dictionary, array, number, baseString, date, data, nil];
        id configuration = [coder decodeObjectOfClasses:classes forKey:(id)OF_STR("NS.configDict")];
        if (configuration != nil || ![coder containsValueForKey:(id)OF_STR("NS.configDict")]) {
            return [self initWithString:string withFormatConfiguration:configuration];
        }
        [self release];
        description =
            (id)OF_STR("Unable to decode NS.configDict as a plist type despite data being present.");
        code = 4864;
    } else {
        code = [coder containsValueForKey:(id)OF_STR("NS.originalString")] ? 4864 : 4865;
        [self release];
        description = (id)OF_STR("Unable to decode NS.originalString.");
    }
    id key = ONSDebugDescriptionErrorKey;
    id info = [ONSDictionary dictionaryWithObjects:&description forKeys:&key count:1];
    [coder failWithError:[ONSError errorWithDomain:ONSCocoaErrorDomain code:code userInfo:info]];
    return nil;
}

// Native -[__NSLocalizedString encodeWithCoder:], 0x1c05d9814. Non-external.
// The throw at 0x1c05d98b4 does not return into the next method as depicted by Hex-Rays.
- (void)encodeWithCoder:(id)coder {
    if (![coder allowsKeyedCoding]) {
        objc_exception_throw([ONSException
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:(id)OF_STR("Encoder does not allow keyed coding!")
                     userInfo:nil]);
    }
    [coder encodeObject:original forKey:(id)OF_STR("NS.originalString")];
    if (config != nil) {
        [coder encodeObject:config forKey:(id)OF_STR("NS.configDict")];
    }
}

// Native -[__NSLocalizedString initWithString:withFormatConfiguration:], 0x1c05d98b8. Non-external.
- (id)initWithString:(id)string withFormatConfiguration:(id)configuration {
    original = [string mutableCopyWithZone:NULL];
    config = [configuration copyWithZone:NULL];
    return self;
}

// Native -[__NSLocalizedString formatConfiguration], 0x1c05d9914. Non-external.
- (id)formatConfiguration {
    return config;
}

// Native -[__NSLocalizedString baseString], 0x1c05d9924. Non-external.
- (id)baseString {
    return original;
}

// Native -[__NSLocalizedString length], 0x1c05d9934. Non-external.
- (ONSUInteger)length {
    return [original length];
}

// Native -[__NSLocalizedString characterAtIndex:], 0x1c05d9944. Non-external.
- (unsigned short)characterAtIndex:(ONSUInteger)index {
    return [original characterAtIndex:index];
}

// Native -[__NSLocalizedString dealloc], 0x1c05d9954. Non-external.
- (void)dealloc {
    [original release];
    [config release];
    [super dealloc];
}

// Native -[__NSLocalizedString fastestEncoding], 0x1c05d99e0. Non-external.
- (ONSUInteger)fastestEncoding {
    return [original fastestEncoding];
}

// Native -[__NSLocalizedString getCharacters:range:], 0x1c05d99f0. Non-external.
- (void)getCharacters:(unsigned short *)buffer range:(ONSRange)range {
    [original getCharacters:buffer range:range];
}

// Native -[__NSLocalizedString substringFromIndex:], 0x1c05d9a00. Non-external.
- (id)substringFromIndex:(ONSUInteger)index {
    return [original substringFromIndex:index];
}

// Native -[__NSLocalizedString substringToIndex:], 0x1c05d9a10. Non-external.
- (id)substringToIndex:(ONSUInteger)index {
    return [original substringToIndex:index];
}

// Native -[__NSLocalizedString substringWithRange:], 0x1c05d9a20. Non-external.
- (id)substringWithRange:(ONSRange)range {
    return [original substringWithRange:range];
}

// Native -[__NSLocalizedString compare:options:range:locale:], 0x1c05d9a30. Non-external.
- (ONSComparisonResult)compare:(id)string
                       options:(ONSUInteger)options
                         range:(ONSRange)range
                        locale:(id)locale {
    return [original compare:string options:options range:range locale:locale];
}

// Native -[__NSLocalizedString isEqualToString:], 0x1c05d9a40. Non-external.
- (BOOL)isEqualToString:(id)string {
    return [original isEqualToString:string];
}

// Native -[__NSLocalizedString _fastCharacterContents], 0x1c05d9a50. Non-external.
- (const unsigned short *)_fastCharacterContents {
    return OCFStringGetCharactersPtr((OCFStringRef)original);
}

// Native -[__NSLocalizedString _fastCStringContents:], 0x1c05d9a60. Non-external.
- (const char *)_fastCStringContents:(BOOL)nullTerminationRequired {
    return [original _fastCStringContents:nullTerminationRequired];
}

// Native -[__NSLocalizedString _fastUTF8StringContents:utf8Length:], 0x1c05d9a70. Non-external.
- (const char *)_fastUTF8StringContents:(BOOL)nullTerminationRequired utf8Length:(ONSUInteger *)length {
    return [original _fastUTF8StringContents:nullTerminationRequired utf8Length:length];
}

// Native -[__NSLocalizedString copyWithZone:], 0x1c05d9a80. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return [[__ONSLocalizedString alloc] initWithString:original withFormatConfiguration:config];
}

// Native -[__NSLocalizedString mutableCopyWithZone:], 0x1c05d9abc. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return [[__ONSLocalizedString alloc] initWithString:original withFormatConfiguration:config];
}

// Native -[__NSLocalizedString replaceCharactersInRange:withString:], 0x1c05d9af8. Non-external.
- (void)replaceCharactersInRange:(ONSRange)range withString:(id)string {
    id old = config;
    config = nil;
    [old release];
    [original replaceCharactersInRange:range withString:string];
}

// Native -[__NSLocalizedString insertString:atIndex:], 0x1c05d9b5c. Non-external.
- (void)insertString:(id)string atIndex:(ONSUInteger)index {
    id old = config;
    config = nil;
    [old release];
    [original insertString:string atIndex:index];
}

// Native -[__NSLocalizedString appendString:], 0x1c05d9bb0. Non-external.
- (void)appendString:(id)string {
    id old = config;
    config = nil;
    [old release];
    [original appendString:string];
}

// Native -[__NSLocalizedString deleteCharactersInRange:], 0x1c05d9bfc. Non-external.
- (void)deleteCharactersInRange:(ONSRange)range {
    id old = config;
    config = nil;
    [old release];
    [original deleteCharactersInRange:range];
}

// Native -[__NSLocalizedString appendFormat:], 0x1c05d9c50. Non-external.
- (void)appendFormat:(id)format, ... {
    id old = config;
    config = nil;
    [old release];

    va_list arguments;
    va_start(arguments, format);
    _OCFStringAppendFormatAndArgumentsAux2(
        (OCFMutableStringRef)original,
        _ONSDescriptionWithLocaleFunc,
        _ONSDescriptionWithStringProxyFunc,
        NULL,
        NULL,
        (OCFStringRef)format,
        arguments
    );
    va_end(arguments);
}

// Native -[__NSLocalizedString setString:], 0x1c05d9cfc. Non-external.
- (void)setString:(id)string {
    id old = config;
    config = nil;
    [old release];
    [original setString:string];
}

// Native -[__NSLocalizedString appendCharacters:length:], 0x1c05d9d48. Non-external.
- (void)appendCharacters:(const unsigned short *)characters length:(ONSUInteger)length {
    id old = config;
    config = nil;
    [old release];
    ONSMutableString *string = original;
    [string replaceCharactersInRange:(ONSRange){[original length], 0}
                      withCharacters:characters
                              length:length];
}

// Native -[__NSLocalizedString replaceOccurrencesOfString:withString:options:range:], 0x1c05d9db0.
// Non-external.
- (ONSUInteger)replaceOccurrencesOfString:(id)target
                               withString:(id)replacement
                                  options:(ONSUInteger)options
                                    range:(ONSRange)range {
    id old = config;
    config = nil;
    [old release];
    return [original replaceOccurrencesOfString:target withString:replacement options:options range:range];
}

@end
