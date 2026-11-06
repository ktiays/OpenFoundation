//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>
#include "CoreFoundation/Locale/OCFLocale.h"
#include "CoreFoundation/String/OCFString.h"
#include <OpenFoundation/Base/ONSObjCRuntime.h>
#include <OpenFoundation/Range/ONSRange.h>

#include <objc/objc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __OBJC__
#import <objc/NSObject.h>
#endif

// NSString declarations and class state shared by the string family of the Foundation image (iOS Simulator
// 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03). NSMutableString, NSPlaceholderString and
// NSPlaceholderMutableString have their own owner headers; only the methods the scope reaches are declared.
// Confidence: verified unless a declaration says otherwise. iOS Device 24A446: unread.
// Reading: Foundation/String/ONSStringReview.md.
//
// The restored methods spell NSRange as ONSRange (two 64-bit words passed in X registers), NSStringEncoding,
// NSStringCompareOptions and the other NSUInteger option types as ONSUInteger, unichar as unsigned short and
// NSZone * as struct _NSZone * (libobjc's tag). Objects of the classes outside this family are spelled id.

// The NSString unit's class state (String/ONSString.m): __NSCFStringClass (0x22e775b08),
// __NSMutableStringClass (0x22e775b18) and __NSStringClass (0x22e775b20), each eight bytes in __DATA,__common
// (non-external natively, were private externals). None of the captured functions stores them; their writer
// lies outside the capture (unread), so they stay zero-initialized here.
OF_EXTERN Class __NSCFStringClass;
OF_EXTERN Class __NSMutableStringClass;
OF_EXTERN Class __NSStringClass;

// Native __NSEncodingToNewDataForExtendedAttribute, 0x1c05d69d0, external (String/ONSString.m): the
// com.apple.TextEncoding extended-attribute data for the encoding.
OF_EXPORT id _ONSEncodingToNewDataForExtendedAttribute(ONSUInteger encoding);

#ifdef __OBJC__
// Native class NSString (__DATA,__objc_data 0x22e6dd528, external), an NSObject subclass; class_ro_t
// 0x22285f860: flags 0, instance start and size 8, no ivars. The superclass pointer 0x22e6b6088 lies outside
// the extracted image; NSObject is the SDK's hierarchy (confidence: medium).
__attribute__((visibility("default")))
@interface ONSString : NSObject

+ (id)allocWithZone:(struct _NSZone *)zone;
+ (id)string;
+ (id)stringWithString:(id)string;
+ (id)stringWithCharacters:(const unsigned short *)characters length:(ONSUInteger)length;
+ (id)stringWithCString:(const char *)cString encoding:(ONSUInteger)encoding;
+ (id)stringWithUTF8String:(const char *)nullTerminatedCString;
+ (id)stringWithFormat:(id)format, ...;
+ (BOOL)supportsSecureCoding;
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded;

- (ONSUInteger)length;
- (unsigned short)characterAtIndex:(ONSUInteger)index;
- (id)init;
- (const unsigned short *)_fastCharacterContents;
- (const char *)_fastCStringContents:(BOOL)nullTerminationRequired;
- (const char *)_fastUTF8StringContents:(BOOL)nullTerminationRequired utf8Length:(ONSUInteger *)length;
- (BOOL)_isCString;
- (id)_stringRepresentation;
- (id)quotedStringRepresentation;
- (id)description;
- (id)copyWithZone:(struct _NSZone *)zone;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
- (BOOL)isEqual:(id)object;
- (ONSUInteger)hash;
- (BOOL)isEqualToString:(id)string;
- (ONSComparisonResult)compare:(id)string options:(ONSUInteger)mask range:(ONSRange)range;
- (ONSComparisonResult)compare:(id)string options:(ONSUInteger)mask range:(ONSRange)range locale:(id)locale;
- (ONSComparisonResult)compare:(id)string;
- (ONSComparisonResult)caseInsensitiveCompare:(id)string;
- (ONSRange)rangeOfString:(id)searchString options:(ONSUInteger)mask range:(ONSRange)range locale:(id)locale;
- (ONSRange)rangeOfString:(id)searchString options:(ONSUInteger)mask range:(ONSRange)range;
- (ONSRange)rangeOfString:(id)searchString;
- (ONSRange)rangeOfCharacterFromSet:(id)searchSet options:(ONSUInteger)mask range:(ONSRange)range;
- (ONSRange)rangeOfCharacterFromSet:(id)searchSet options:(ONSUInteger)mask;
- (ONSRange)rangeOfCharacterFromSet:(id)searchSet;
- (id)componentsSeparatedByString:(id)separator;
- (BOOL)hasPrefix:(id)string;
- (BOOL)hasSuffix:(id)string;
- (BOOL)containsString:(id)string;
- (id)stringByTrimmingCharactersInSet:(id)set;
- (id)stringByReplacingOccurrencesOfString:(id)target
                                withString:(id)replacement
                                   options:(ONSUInteger)options
                                     range:(ONSRange)searchRange;
- (void)getCharacters:(unsigned short *)buffer range:(ONSRange)range;
- (void)getCharacters:(unsigned short *)buffer;
- (id)_newSubstringWithRange:(ONSRange)range zone:(struct _NSZone *)zone;
- (id)substringWithRange:(ONSRange)range;
- (id)substringFromIndex:(ONSUInteger)from;
- (id)substringToIndex:(ONSUInteger)to;
- (id)stringByAppendingString:(id)string;
- (double)doubleValue;
- (float)floatValue;
- (int)intValue;
- (long long)longLongValue;
- (ONSInteger)integerValue;
- (BOOL)boolValue;
- (id)uppercaseString;
- (id)lowercaseString;
- (id)capitalizedString;
- (id)uppercaseStringWithLocale:(id)locale;
- (id)lowercaseStringWithLocale:(id)locale;
- (id)capitalizedStringWithLocale:(id)locale;
- (void)_getBlockStart:(ONSUInteger *)startPtr
                     end:(ONSUInteger *)endPtr
             contentsEnd:(ONSUInteger *)contentsEndPtr
                forRange:(ONSRange)range
    stopAtLineSeparators:(BOOL)stopAtLineSeparators;
- (void)enumerateSubstringsInRange:(ONSRange)range
                           options:(ONSUInteger)options
                        usingBlock:
                            (void (^)(
                                id substring, ONSRange substringRange, ONSRange enclosingRange, BOOL *stop
                            ))block;
- (BOOL)getExternalRepresentation:(id *)representation
               extendedAttributes:(id *)extendedAttributes
            forWritingToURLOrPath:(id)urlOrPath
                    usingEncoding:(ONSUInteger)encoding
                            error:(id *)error;
- (BOOL)writeToFile:(id)path atomically:(BOOL)useAuxiliaryFile;
- (BOOL)writeToURL:(id)url atomically:(BOOL)atomically;
- (id)initWithCString:(const char *)bytes length:(ONSUInteger)length;
- (id)initWithCString:(const char *)nullTerminatedCString encoding:(ONSUInteger)encoding;
- (id)initWithUTF8String:(const char *)nullTerminatedCString;
- (id)initWithCharacters:(const unsigned short *)characters length:(ONSUInteger)length;
- (id)initWithString:(id)string;
- (id)initWithData:(id)data encoding:(ONSUInteger)encoding;
- (id)_initWithDataOfUnknownEncoding:(id)data;
- (id)initWithContentsOfFile:(id)path;
- (id)initWithContentsOfURL:(id)url;
- (id)initWithContentsOfURL:(id)url encoding:(ONSUInteger)encoding error:(id *)error;
- (id)initWithContentsOfFile:(id)path encoding:(ONSUInteger)encoding error:(id *)error;
- (id)initWithFormat:(id)format, ...;
- (id)initWithFormat:(id)format locale:(id)locale, ...;
- (id)initWithFormat:(id)format arguments:(va_list)arguments;
- (Class)classForCoder;
- (BOOL)_allowsDirectEncoding;
- (void)encodeWithCoder:(id)coder;
- (id)initWithCoder:(id)coder;
- (id)initWithCharactersNoCopy:(unsigned short *)characters
                        length:(ONSUInteger)length
                  freeWhenDone:(BOOL)freeBuffer;
- (id)initWithBytesNoCopy:(void *)bytes
                   length:(ONSUInteger)length
                 encoding:(ONSUInteger)encoding
             freeWhenDone:(BOOL)freeBuffer;
- (id)initWithFormat:(id)format locale:(id)locale arguments:(va_list)arguments;

@end

// -initWithBytes:length:encoding:, which the restored methods send; NSString's own implementation lies
// outside the scope (only NSPlaceholderString's override is restored), so it is declared in the SDK's
// category.
@interface ONSString (NSStringExtensionMethods)

- (id)initWithBytes:(const void *)bytes length:(ONSUInteger)length encoding:(ONSUInteger)encoding;

@end

// Selectors the restored methods send to objects of classes outside this family, declared for the message
// sends only; no protocol object is emitted. A unit that needs another selector declares it in its own
// header's protocol after checking this list.

#endif
