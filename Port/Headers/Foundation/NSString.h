//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSRange.h>

typedef unsigned short unichar;

typedef NSUInteger NSStringEncoding;

#if defined(__OBJC__)
@class NSData, NSArray<ObjectType>, NSDictionary<KeyType, ObjectType>, NSCharacterSet, NSURL, NSError, NSLocale;

typedef NS_OPTIONS(NSUInteger, NSStringCompareOptions) {
    NSCaseInsensitiveSearch = 1,
    NSLiteralSearch = 2,
    NSBackwardsSearch = 4,
    NSAnchoredSearch = 8,
    NSNumericSearch = 64,
    NSDiacriticInsensitiveSearch = 128,
    NSWidthInsensitiveSearch = 256,
    NSForcedOrderingSearch = 512,
    NSRegularExpressionSearch = 1024
};

NS_ENUM(NSStringEncoding) {
    NSASCIIStringEncoding = 1,
    NSNEXTSTEPStringEncoding = 2,
    NSJapaneseEUCStringEncoding = 3,
    NSUTF8StringEncoding = 4,
    NSISOLatin1StringEncoding = 5,
    NSSymbolStringEncoding = 6,
    NSNonLossyASCIIStringEncoding = 7,
    NSShiftJISStringEncoding = 8,
    NSISOLatin2StringEncoding = 9,
    NSUnicodeStringEncoding = 10,
    NSWindowsCP1251StringEncoding = 11,
    NSWindowsCP1252StringEncoding = 12,
    NSWindowsCP1253StringEncoding = 13,
    NSWindowsCP1254StringEncoding = 14,
    NSWindowsCP1250StringEncoding = 15,
    NSISO2022JPStringEncoding = 21,
    NSMacOSRomanStringEncoding = 30,
    NSUTF16StringEncoding = NSUnicodeStringEncoding,
    NSUTF16BigEndianStringEncoding = 0x90000100,
    NSUTF16LittleEndianStringEncoding = 0x94000100,
    NSUTF32StringEncoding = 0x8c000100,
    NSUTF32BigEndianStringEncoding = 0x98000100,
    NSUTF32LittleEndianStringEncoding = 0x9c000100
};

typedef NS_OPTIONS(NSUInteger, NSStringEncodingConversionOptions) {
    NSStringEncodingConversionAllowLossy = 1,
    NSStringEncodingConversionExternalRepresentation = 2
};

typedef NS_OPTIONS(NSUInteger, NSStringEnumerationOptions) {
    NSStringEnumerationByLines = 0,
    NSStringEnumerationByParagraphs = 1,
    NSStringEnumerationByComposedCharacterSequences = 2,
    NSStringEnumerationByWords = 3,
    NSStringEnumerationBySentences = 4,
    NSStringEnumerationByCaretPositions = 5,
    NSStringEnumerationByDeletionClusters = 6,
    NSStringEnumerationReverse = 1UL << 8,
    NSStringEnumerationSubstringNotRequired = 1UL << 9,
    NSStringEnumerationLocalized = 1UL << 10
};

@interface NSString : NSObject <NSCopying, NSMutableCopying, NSSecureCoding>

@property (readonly) NSUInteger length;
- (unichar)characterAtIndex:(NSUInteger)index;
- (instancetype)init NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithCoder:(NSCoder *)coder NS_DESIGNATED_INITIALIZER;

@end

@interface NSString (NSStringExtensionMethods)

- (NSString *)substringFromIndex:(NSUInteger)from;
- (NSString *)substringToIndex:(NSUInteger)to;
- (NSString *)substringWithRange:(NSRange)range;
- (void)getCharacters:(unichar *)buffer range:(NSRange)range;

- (NSComparisonResult)compare:(NSString *)string;
- (NSComparisonResult)compare:(NSString *)string options:(NSStringCompareOptions)mask range:(NSRange)rangeOfReceiverToCompare;
- (NSComparisonResult)compare:(NSString *)string
                      options:(NSStringCompareOptions)mask
                        range:(NSRange)rangeOfReceiverToCompare
                       locale:(id)locale;
- (NSComparisonResult)caseInsensitiveCompare:(NSString *)string;

- (BOOL)isEqualToString:(NSString *)aString;
- (BOOL)hasPrefix:(NSString *)str;
- (BOOL)hasSuffix:(NSString *)str;
- (BOOL)containsString:(NSString *)str;

- (NSRange)rangeOfString:(NSString *)searchString;
- (NSRange)rangeOfString:(NSString *)searchString options:(NSStringCompareOptions)mask;
- (NSRange)rangeOfString:(NSString *)searchString options:(NSStringCompareOptions)mask range:(NSRange)rangeOfReceiverToSearch;
- (NSRange)rangeOfString:(NSString *)searchString
                 options:(NSStringCompareOptions)mask
                   range:(NSRange)rangeOfReceiverToSearch
                  locale:(NSLocale *)locale;
- (NSRange)rangeOfCharacterFromSet:(NSCharacterSet *)searchSet;
- (NSRange)rangeOfCharacterFromSet:(NSCharacterSet *)searchSet options:(NSStringCompareOptions)mask;
- (NSRange)rangeOfCharacterFromSet:(NSCharacterSet *)searchSet
                           options:(NSStringCompareOptions)mask
                             range:(NSRange)rangeOfReceiverToSearch;

- (NSString *)stringByAppendingString:(NSString *)aString;

@property (readonly) double doubleValue;
@property (readonly) float floatValue;
@property (readonly) int intValue;
@property (readonly) NSInteger integerValue;
@property (readonly) long long longLongValue;
@property (readonly) BOOL boolValue;

@property (readonly, copy) NSString *uppercaseString;
@property (readonly, copy) NSString *lowercaseString;
@property (readonly, copy) NSString *capitalizedString;
- (NSString *)uppercaseStringWithLocale:(NSLocale *)locale;
- (NSString *)lowercaseStringWithLocale:(NSLocale *)locale;
- (NSString *)capitalizedStringWithLocale:(NSLocale *)locale;

- (void)getLineStart:(NSUInteger *)startPtr
                 end:(NSUInteger *)lineEndPtr
         contentsEnd:(NSUInteger *)contentsEndPtr
            forRange:(NSRange)range;
- (void)enumerateSubstringsInRange:(NSRange)range
                           options:(NSStringEnumerationOptions)opts
                        usingBlock:(void (^)(NSString *substring, NSRange substringRange, NSRange enclosingRange, BOOL *stop))block;

@property (readonly) const char *UTF8String NS_RETURNS_INNER_POINTER;
@property (readonly) NSStringEncoding fastestEncoding;
@property (readonly) NSStringEncoding smallestEncoding;
- (NSData *)dataUsingEncoding:(NSStringEncoding)encoding allowLossyConversion:(BOOL)lossy;
- (NSData *)dataUsingEncoding:(NSStringEncoding)encoding;
- (const char *)cStringUsingEncoding:(NSStringEncoding)encoding NS_RETURNS_INNER_POINTER;
- (BOOL)getCString:(char *)buffer maxLength:(NSUInteger)maxBufferCount encoding:(NSStringEncoding)encoding;
- (BOOL)getBytes:(void *)buffer
         maxLength:(NSUInteger)maxBufferCount
        usedLength:(NSUInteger *)usedBufferCount
          encoding:(NSStringEncoding)encoding
           options:(NSStringEncodingConversionOptions)options
             range:(NSRange)range
    remainingRange:(NSRangePointer)leftover;
- (NSUInteger)lengthOfBytesUsingEncoding:(NSStringEncoding)enc;
+ (NSString *)localizedNameOfStringEncoding:(NSStringEncoding)encoding;
@property (class, readonly) NSStringEncoding defaultCStringEncoding;

- (NSArray<NSString *> *)componentsSeparatedByString:(NSString *)separator;
- (NSString *)stringByTrimmingCharactersInSet:(NSCharacterSet *)set;
- (NSString *)stringByReplacingOccurrencesOfString:(NSString *)target
                                        withString:(NSString *)replacement
                                           options:(NSStringCompareOptions)options
                                             range:(NSRange)searchRange;

@property (readonly, copy) NSString *description;
@property (readonly) NSUInteger hash;

- (instancetype)initWithCharactersNoCopy:(unichar *)characters length:(NSUInteger)length freeWhenDone:(BOOL)freeBuffer;
- (instancetype)initWithCharacters:(const unichar *)characters length:(NSUInteger)length;
- (instancetype)initWithUTF8String:(const char *)nullTerminatedCString;
- (instancetype)initWithString:(NSString *)aString;
- (instancetype)initWithFormat:(NSString *)format, ... NS_FORMAT_FUNCTION(1, 2);
- (instancetype)initWithFormat:(NSString *)format arguments:(va_list)argList NS_FORMAT_FUNCTION(1, 0);
- (instancetype)initWithFormat:(NSString *)format locale:(id)locale, ... NS_FORMAT_FUNCTION(1, 3);
- (instancetype)initWithFormat:(NSString *)format locale:(id)locale arguments:(va_list)argList NS_FORMAT_FUNCTION(1, 0);
- (instancetype)initWithData:(NSData *)data encoding:(NSStringEncoding)encoding;
- (instancetype)initWithBytes:(const void *)bytes length:(NSUInteger)len encoding:(NSStringEncoding)encoding;
- (instancetype)initWithBytesNoCopy:(void *)bytes
                             length:(NSUInteger)len
                           encoding:(NSStringEncoding)encoding
                       freeWhenDone:(BOOL)freeBuffer;

+ (instancetype)string;
+ (instancetype)stringWithString:(NSString *)string;
+ (instancetype)stringWithCharacters:(const unichar *)characters length:(NSUInteger)length;
+ (instancetype)stringWithUTF8String:(const char *)nullTerminatedCString;
+ (instancetype)stringWithFormat:(NSString *)format, ... NS_FORMAT_FUNCTION(1, 2);

- (instancetype)initWithCString:(const char *)nullTerminatedCString encoding:(NSStringEncoding)encoding;
+ (instancetype)stringWithCString:(const char *)cString encoding:(NSStringEncoding)enc;

- (instancetype)initWithContentsOfURL:(NSURL *)url encoding:(NSStringEncoding)enc error:(NSError **)error;
- (instancetype)initWithContentsOfFile:(NSString *)path encoding:(NSStringEncoding)enc error:(NSError **)error;

@end

@interface NSMutableString : NSString

- (void)replaceCharactersInRange:(NSRange)range withString:(NSString *)aString;

@end

@interface NSMutableString (NSMutableStringExtensionMethods)

- (void)insertString:(NSString *)aString atIndex:(NSUInteger)loc;
- (void)deleteCharactersInRange:(NSRange)range;
- (void)appendString:(NSString *)aString;
- (void)appendFormat:(NSString *)format, ... NS_FORMAT_FUNCTION(1, 2);
- (void)setString:(NSString *)aString;
- (NSUInteger)replaceOccurrencesOfString:(NSString *)target
                              withString:(NSString *)replacement
                                 options:(NSStringCompareOptions)options
                                   range:(NSRange)searchRange;
- (instancetype)initWithCapacity:(NSUInteger)capacity;
+ (NSMutableString *)stringWithCapacity:(NSUInteger)capacity;

@end

FOUNDATION_EXPORT NSExceptionName const NSCharacterConversionException;
#endif
