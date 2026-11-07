//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSString.h>
#import <Foundation/NSDictionary.h>

#if defined(__OBJC__)
typedef NSString *NSAttributedStringKey NS_TYPED_EXTENSIBLE_ENUM;

typedef NS_OPTIONS(NSUInteger, NSAttributedStringEnumerationOptions) {
    NSAttributedStringEnumerationReverse = (1UL << 1),
    NSAttributedStringEnumerationLongestEffectiveRangeNotRequired = (1UL << 20)
};

@interface NSAttributedString : NSObject <NSCopying, NSMutableCopying, NSSecureCoding>

@property (readonly, copy) NSString *string;
- (NSDictionary<NSAttributedStringKey, id> *)attributesAtIndex:(NSUInteger)location effectiveRange:(NSRangePointer)range;

@end

@interface NSAttributedString (NSExtendedAttributedString)

@property (readonly) NSUInteger length;
- (id)attribute:(NSAttributedStringKey)attrName atIndex:(NSUInteger)location effectiveRange:(NSRangePointer)range;
- (NSAttributedString *)attributedSubstringFromRange:(NSRange)range;
- (NSDictionary<NSAttributedStringKey, id> *)attributesAtIndex:(NSUInteger)location
                                         longestEffectiveRange:(NSRangePointer)range
                                                       inRange:(NSRange)rangeLimit;
- (id)attribute:(NSAttributedStringKey)attrName
                  atIndex:(NSUInteger)location
    longestEffectiveRange:(NSRangePointer)range
                  inRange:(NSRange)rangeLimit;
- (BOOL)isEqualToAttributedString:(NSAttributedString *)other;
- (instancetype)initWithString:(NSString *)str;
- (instancetype)initWithString:(NSString *)str attributes:(NSDictionary<NSAttributedStringKey, id> *)attrs;
- (instancetype)initWithAttributedString:(NSAttributedString *)attrStr;
- (void)enumerateAttributesInRange:(NSRange)enumerationRange
                           options:(NSAttributedStringEnumerationOptions)opts
                        usingBlock:(void (^)(NSDictionary<NSAttributedStringKey, id> *attrs, NSRange range, BOOL *stop))block;
- (void)enumerateAttribute:(NSAttributedStringKey)attrName
                   inRange:(NSRange)enumerationRange
                   options:(NSAttributedStringEnumerationOptions)opts
                usingBlock:(void (^)(id value, NSRange range, BOOL *stop))block;

@end

@interface NSMutableAttributedString : NSAttributedString

- (void)replaceCharactersInRange:(NSRange)range withString:(NSString *)str;
- (void)setAttributes:(NSDictionary<NSAttributedStringKey, id> *)attrs range:(NSRange)range;

@end

@interface NSMutableAttributedString (NSExtendedMutableAttributedString)

@property (readonly, retain) NSMutableString *mutableString;
- (void)addAttribute:(NSAttributedStringKey)name value:(id)value range:(NSRange)range;
- (void)addAttributes:(NSDictionary<NSAttributedStringKey, id> *)attrs range:(NSRange)range;
- (void)removeAttribute:(NSAttributedStringKey)name range:(NSRange)range;
- (void)replaceCharactersInRange:(NSRange)range withAttributedString:(NSAttributedString *)attrString;
- (void)insertAttributedString:(NSAttributedString *)attrString atIndex:(NSUInteger)loc;
- (void)appendAttributedString:(NSAttributedString *)attrString;
- (void)deleteCharactersInRange:(NSRange)range;
- (void)setAttributedString:(NSAttributedString *)attrString;
- (void)beginEditing;
- (void)endEditing;

@end
#endif
