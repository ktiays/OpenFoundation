//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <Foundation/NSObjCRuntime.h>

typedef struct _NSRange {
    NSUInteger location;
    NSUInteger length;
} NSRange;

typedef NSRange *NSRangePointer;

NS_INLINE NSRange NSMakeRange(NSUInteger loc, NSUInteger len) {
    NSRange r;
    r.location = loc;
    r.length = len;
    return r;
}

NS_INLINE NSUInteger NSMaxRange(NSRange range) {
    return (range.location + range.length);
}

NS_INLINE BOOL NSLocationInRange(NSUInteger loc, NSRange range) {
    return (!(loc < range.location) && (loc - range.location) < range.length) ? YES : NO;
}

NS_INLINE BOOL NSEqualRanges(NSRange range1, NSRange range2) {
    return (range1.location == range2.location && range1.length == range2.length);
}

FOUNDATION_EXPORT NSRange NSUnionRange(NSRange range1, NSRange range2);
FOUNDATION_EXPORT NSRange NSIntersectionRange(NSRange range1, NSRange range2);

#if defined(__OBJC__)
#import <Foundation/NSValue.h>

@class NSString;

FOUNDATION_EXPORT NSString *NSStringFromRange(NSRange range);

@interface NSValue (NSValueRangeExtensions)

+ (NSValue *)valueWithRange:(NSRange)range;
@property (readonly) NSRange rangeValue;

@end
#endif
