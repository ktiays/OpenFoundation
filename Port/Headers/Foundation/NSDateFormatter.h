//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSFormatter.h>
#import <Foundation/NSDate.h>

#if defined(__OBJC__)
@class NSLocale, NSTimeZone, NSCalendar, NSString;

typedef NS_ENUM(NSUInteger, NSDateFormatterStyle) {
    NSDateFormatterNoStyle = kCFDateFormatterNoStyle,
    NSDateFormatterShortStyle = kCFDateFormatterShortStyle,
    NSDateFormatterMediumStyle = kCFDateFormatterMediumStyle,
    NSDateFormatterLongStyle = kCFDateFormatterLongStyle,
    NSDateFormatterFullStyle = kCFDateFormatterFullStyle
};

@interface NSDateFormatter : NSFormatter

- (NSString *)stringFromDate:(NSDate *)date;
@property (copy) NSString *dateFormat;

@end
#endif
