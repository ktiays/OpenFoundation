//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>

#if defined(__OBJC__)
@class NSString, NSAttributedString, NSDictionary<KeyType, ObjectType>;

typedef NS_ENUM(NSInteger, NSFormattingContext) {
    NSFormattingContextUnknown = 0,
    NSFormattingContextDynamic = 1,
    NSFormattingContextStandalone = 2,
    NSFormattingContextListItem = 3,
    NSFormattingContextBeginningOfSentence = 4,
    NSFormattingContextMiddleOfSentence = 5
};

@interface NSFormatter : NSObject <NSCopying, NSCoding>

- (NSString *)stringForObjectValue:(id)obj;
- (BOOL)getObjectValue:(out id *)obj forString:(NSString *)string errorDescription:(out NSString **)error;

@end
#endif
