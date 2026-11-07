//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>

#if defined(__OBJC__)
@class NSString, NSCharacterSet, NSDictionary<KeyType, ObjectType>;

@interface NSScanner : NSObject <NSCopying>

@property (readonly, copy) NSString *string;
@property NSUInteger scanLocation;
@property (copy) NSCharacterSet *charactersToBeSkipped;
@property BOOL caseSensitive;
@property (retain) id locale;
- (instancetype)initWithString:(NSString *)string NS_DESIGNATED_INITIALIZER;

@end

@interface NSScanner (NSExtendedScanner)

- (BOOL)scanInt:(int *)result;
- (BOOL)scanDouble:(double *)result;
+ (instancetype)scannerWithString:(NSString *)string;

@end
#endif
