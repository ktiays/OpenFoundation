//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <stdbool.h>

// The distant-past and distant-future date pointers (Objective-C/OCFNSDate.m; external natively as
// _NSDateDistantPast, 0x2227ab910, and _NSDateDistantFuture, 0x2227ab928): each points at its constant
// NSConstantDate object.
OF_EXPORT const void *const _ONSDateDistantPast;
OF_EXPORT const void *const _ONSDateDistantFuture;

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSDate (__DATA,__objc_data 0x22e6cdce8, external), an NSObject subclass; class_ro_t
// 0x2227c3988: instance start and size 8, no ivars. Native class __NSTaggedDate (0x22e6cdd60, was a private
// external), an NSDate subclass; class_ro_t 0x2227c3818: instance start and size 8, no ivars (iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The method lists are in the
// shared cache's optimized method lists, outside the extracted image; the methods come from the IDA capture.
// NSTimeInterval is spelled double, NSComparisonResult NSInteger and NSCalendarUnit NSUInteger.
// Exported, as the native image exports the class.
__attribute__((visibility("default")))
@interface ONSDate : NSObject

- (double)timeIntervalSinceReferenceDate;
- (id)initWithTimeIntervalSinceReferenceDate:(double)ti;
- (BOOL)isEqualToDate:(id)otherDate;
- (id)descriptionWithLocale:(id)locale;
- (id)dateByAddingTimeInterval:(double)ti;
- (id)initWithString:(id)description;
+ (id)dateWithTimeIntervalSinceReferenceDate:(double)ti;

@end

@interface __ONSTaggedDate : ONSDate
@end

// Native class __NSDate (__DATA,__objc_data 0x22e6cdcc0, was a private external), an NSDate subclass;
// class_ro_t 0x2227c3788: instance start 8, size 16, one ivar _time (offset variable 0x22e6cc17c, offset 8,
// size 8, alignment 8; iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified). The type encoding lies outside the extracted image; the type is the double the restored code
// reads (LDR D0). Restored in Objective-C/OCFNSDate.m; -[__NSPlaceholderDate
// initWithTimeIntervalSinceReferenceDate:] allocates its instances and stores the interval at offset 8.
@interface __ONSDate : ONSDate {
    @public
    double _time;
}
@end

// Classes of this image that Objective-C/OCFNSDate.m references by name and that are not restored on this
// branch (NSConstantDate, __NSPlaceholderDate, NSDateComponents), declared so that the prefix tool maps them.
@class ONSConstantDate;
@class __ONSPlaceholderDate;
@class ONSDateComponents;
#endif
