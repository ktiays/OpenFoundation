//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSRange.h>
#import <Foundation/NSString.h>

#if defined(__OBJC__)
@class NSData;

@interface NSCharacterSet : NSObject <NSCopying, NSMutableCopying, NSSecureCoding>

@property (readonly, class, copy) NSCharacterSet *whitespaceCharacterSet;
@property (readonly, class, copy) NSCharacterSet *whitespaceAndNewlineCharacterSet;
@property (readonly, class, copy) NSCharacterSet *decimalDigitCharacterSet;
@property (readonly, class, copy) NSCharacterSet *lowercaseLetterCharacterSet;
@property (readonly, class, copy) NSCharacterSet *alphanumericCharacterSet;

+ (NSCharacterSet *)characterSetWithCharactersInString:(NSString *)aString;

- (BOOL)characterIsMember:(unichar)aCharacter;
@property (readonly, copy) NSData *bitmapRepresentation;
@property (readonly, copy) NSCharacterSet *invertedSet;
- (BOOL)longCharacterIsMember:(UTF32Char)theLongChar;
- (BOOL)isSupersetOfSet:(NSCharacterSet *)theOtherSet;

@end

@interface NSMutableCharacterSet : NSCharacterSet <NSCopying, NSMutableCopying, NSSecureCoding>

- (void)addCharactersInRange:(NSRange)aRange;
- (void)removeCharactersInRange:(NSRange)aRange;
- (void)addCharactersInString:(NSString *)aString;
- (void)removeCharactersInString:(NSString *)aString;
- (void)formUnionWithCharacterSet:(NSCharacterSet *)otherSet;
- (void)formIntersectionWithCharacterSet:(NSCharacterSet *)otherSet;
- (void)invert;

@end
#endif
