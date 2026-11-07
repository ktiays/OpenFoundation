//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCalendar.h"

#import "CoreFoundation/Base/OCFUtilities.h"
#import "CoreFoundation/Locale/OCFCalendarEntryPoints.h"
#import "CoreFoundation/Locale/OCFDateFormatter.h"
#import "CoreFoundation/Locale/OCFLocaleKeys.h"
#import "CoreFoundation/Objective-C/OCFNSDate.h"
#import "CoreFoundation/Objective-C/OCFNSDateComponents.h"
#import "CoreFoundation/Objective-C/OCFRequireConcreteImplementation.h"
#import "CoreFoundation/String/OCFString.h"

#include <math.h>
#include <objc/runtime.h>

// Inferred organization: the NSCalendar methods of this image (+autoupdatingCurrentCalendar 0x1c02a8364
// through -shortStandaloneQuarterSymbols 0x1c02aa8d8, with the byref helpers and block invokes at
// 0x1c02a9860-0x1c02a9be0, the outlined fragments OUTLINED_FUNCTION_0_47 through _5_12 at
// 0x1c02aab6c-0x1c02aaba8, and the abstract methods in the cold region at 0x1c0310110-0x1c0310370) lie
// together in the iOS Simulator __text section after the day-changed notification functions that
// Locale/OCFCalendar.c restores; following the owner's decision, the class has its own translation unit.
// Objective-C compiled with manual reference counting (objc_retain, objc_autorelease, objc_release; the
// __block result variables have the MRC unretained byref layout, flags 0x52000000, and copy and dispose
// helpers passing 0x83). The native source path is not in the evidence. Unless a definition says otherwise:
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in
// full; constant strings resolved from the ADRP/ADD encodings and constants read with
// Local/Scratch/cf-core/macho.py); iOS Device 24A446: unread. Reading: not yet written; the evidence is the
// assembly in Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.
//
// The super send of +allocWithZone: passes the selector address 0x2387a9fd4. The abstract methods share the
// outlined fragments OUTLINED_FUNCTION_0_47 (__CFLookUpClass("NSCalendar"), saving the receiver and
// selector), _1_33 and _4_12 (__CFRequireConcreteImplementation), _2_29 (the argument moves) and _3_14/_5_12
// (the epilogues), restored inline in each method.

// The NSCalendar class reference (off_2227CE2B8, and the self class reference ___SCR_NSCalendar, the same
// class).
extern char calendarClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSCalendar));

// Selectors this unit sends to objects of other classes (NSDate, NSString, NSThread's class), declared for
// the message sends only; no protocol object is emitted.
@protocol CalendarMessages

+ (id)callStackSymbols;
- (id)dateByAddingTimeInterval:(double)ti;
- (NSInteger)compare:(id)other;
- (BOOL)isEqualToString:(id)string;

@end

// Class methods this unit sends through the NSCalendar and NSDate class references that their classes'
// interfaces do not declare (the private calendar factories, implemented outside this image, and +[NSDate
// date]); declarations only, no category is emitted.
@interface ONSCalendar (CalendarFactoryMessages)

+ (id)_current;
+ (id)_autoupdatingCurrent;

@end

@interface ONSDate (CalendarDateMessages)

+ (id)date;

@end

// The native lower and upper bounds of the time intervals that -compareDate:toDate:toUnitGranularity:
// compares through calendar components (qword_1C046C7A8 and qword_1C046C7B0, bits 0xc248a97998e00000 and
// 0x42acf8ab468f0000); outside them the intervals are compared directly.
static const double kComponentComparisonLowerBound = -211845067200.0;
static const double kComponentComparisonUpperBound = 15927175497600.0;

// The calendar units -date:matchesComponents: examines (unk_1C046C830, 14 entries, __TEXT,__const): era,
// year, month, day, hour, minute, second, weekday, weekdayOrdinal, quarter, weekOfMonth, weekOfYear,
// yearForWeekOfYear and nanosecond.
static const NSUInteger kMatchUnits[14] = {
    0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000
};

// The "*** %s: <argument> cannot be nil" warning the methods inline for a nil argument (natively each method
// has its own __count__ static): the first 14 times (the count is at most 13 before the call) CFLog at level
// 3 with the method name and the -description of +[NSThread callStackSymbols] (the class looked up by name);
// the count is incremented every time. logNilArgument is a local descriptive name.
static inline void logNilArgument(int32_t *count, OCFStringRef format, const char *method) {
    if (*count <= 13) {
        id thread = (id)__CFLookUpClass(OF_NAME_STRING(ONSThread));
        id symbols = [thread callStackSymbols];
        OCFLog(3, format, method, [symbols description]);
    }
    *count = *count + 1;
}

// The format strings of the nil-argument warnings (0x2227b93c0, 0x2227b93e0 and 0x2227b9400).
#define kDateCannotBeNilFormat                                                                               \
    OF_STR(                                                                                                  \
        "*** %s: date cannot be nil\nFuture exception.\nA few of these errors are going to be reported "     \
        "with "                                                                                              \
        "this complaint, then further violations will simply be ignored.\nHere is the backtrace where this " \
        "occurred this time (some frames may be missing due to compiler optimizations):\n%@"                 \
    )
#define kDate1CannotBeNilFormat                                                                              \
    OF_STR(                                                                                                  \
        "*** %s: date1 cannot be nil\nFuture exception.\nA few of these errors are going to be reported "    \
        "with "                                                                                              \
        "this complaint, then further violations will simply be ignored.\nHere is the backtrace where this " \
        "occurred this time (some frames may be missing due to compiler optimizations):\n%@"                 \
    )
#define kDate2CannotBeNilFormat                                                                              \
    OF_STR(                                                                                                  \
        "*** %s: date2 cannot be nil\nFuture exception.\nA few of these errors are going to be reported "    \
        "with "                                                                                              \
        "this complaint, then further violations will simply be ignored.\nHere is the backtrace where this " \
        "occurred this time (some frames may be missing due to compiler optimizations):\n%@"                 \
    )

// The symbol getters' shared body (inlined natively in each): a date formatter (system default allocator,
// the calendar's -locale, no date or time style) gets the calendar as kCFDateFormatterCalendarKey, the key's
// property is copied and autoreleased, and the formatter is released. formatterSymbols is a local
// descriptive name.
static inline id formatterSymbols(ONSCalendar *calendar, OCFDateFormatterKey key) {
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFDateFormatterRef formatter = OCFDateFormatterCreate(
        allocator, (OCFLocaleRef)[calendar locale], kOCFDateFormatterNoStyle, kOCFDateFormatterNoStyle
    );
    OCFDateFormatterSetProperty(formatter, kOCFDateFormatterCalendarKey, (OCFTypeRef)calendar);
    id symbols = [(id)OCFDateFormatterCopyProperty(formatter, key) autorelease];
    OCFRelease(formatter);
    return symbols;
}

@implementation ONSCalendar

// Native +[NSCalendar autoupdatingCurrentCalendar], 0x1c02a8364. Non-external; a tail call of
// +[NSCalendar _autoupdatingCurrent] (the class reference, not the receiver).
+ (id)autoupdatingCurrentCalendar {
    return [ONSCalendar _autoupdatingCurrent];
}

// Native +[NSCalendar allocWithZone:], 0x1c02a8370. Non-external. NSCalendar itself (its self class
// reference) allocates the class named "_NSSwiftCalendar" (objc_lookUpClass, a tail call of
// +allocWithZone:); a subclass gets super's +allocWithZone:.
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == (Class)calendarClass) {
        return [(id)objc_lookUpClass(OF_NAME_STRING(_ONSSwiftCalendar)) allocWithZone:zone];
    }
    return [super allocWithZone:zone];
}

// Native -[NSCalendar init], 0x1c02a8428. Non-external; nil (the receiver is not released).
- (id)init {
    return nil;
}

// Native -[NSCalendar _cfTypeID], 0x1c02a8430. Non-external; a tail call of CFCalendarGetTypeID.
- (OCFTypeID)_cfTypeID {
    return OCFCalendarGetTypeID();
}

// Native -[NSCalendar hash], 0x1c02a8434. Non-external; the identifier's -hash (a tail call).
- (NSUInteger)hash {
    return [[self calendarIdentifier] hash];
}

// Native -[NSCalendar isEqual:], 0x1c02a8448. Non-external.
// The receiver itself is equal and nil is not; an object that is not a kind of NSCalendar (the self class
// reference) is not. The identifiers, time zones and locales must be -isEqual:, and the first weekdays and
// minimum days equal. The Gregorian start dates are then compared, each fetched again for every test: when
// both are present, -isEqual: decides; when only the receiver's is present the answer is NO; otherwise the
// answer is YES when the receiver's or neither is present.
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    if (![object isKindOfClass:(Class)calendarClass]) {
        return NO;
    }
    ONSCalendar *other = object;
    if (![[self calendarIdentifier] isEqual:[other calendarIdentifier]]) {
        return NO;
    }
    if (![[self timeZone] isEqual:[other timeZone]]) {
        return NO;
    }
    if (![[self locale] isEqual:[other locale]]) {
        return NO;
    }
    if ([self firstWeekday] != [other firstWeekday]) {
        return NO;
    }
    if ([self minimumDaysInFirstWeek] != [other minimumDaysInFirstWeek]) {
        return NO;
    }
    if ([self gregorianStartDate] != nil && [other gregorianStartDate] != nil) {
        return [[self gregorianStartDate] isEqual:[other gregorianStartDate]] ? YES : NO;
    }
    if ([self gregorianStartDate] != nil && [other gregorianStartDate] == nil) {
        return NO;
    }
    if ([self gregorianStartDate] != nil) {
        return YES;
    }
    return [other gregorianStartDate] == nil;
}

// Native -[NSCalendar copyWithZone:], 0x1c02a85b8. Non-external.
// A new calendar of the receiver's class (objc_opt_class, +allocWithZone:) initialized with the identifier,
// given the locale, time zone, first weekday, minimum days and Gregorian start date, in that order.
- (id)copyWithZone:(struct _NSZone *)zone {
    ONSCalendar *copy =
        [[[self class] allocWithZone:zone] initWithCalendarIdentifier:[self calendarIdentifier]];
    [copy setLocale:[self locale]];
    [copy setTimeZone:[self timeZone]];
    [copy setFirstWeekday:[self firstWeekday]];
    [copy setMinimumDaysInFirstWeek:[self minimumDaysInFirstWeek]];
    [copy setGregorianStartDate:[self gregorianStartDate]];
    return copy;
}

// Native +[NSCalendar supportsSecureCoding], 0x1c02a8668. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSCalendar initWithCoder:], 0x1c02a8670. Non-external; nil (the receiver is not released).
- (id)initWithCoder:(id)coder {
    return nil;
}

// Native -[NSCalendar encodeWithCoder:], 0x1c02a8678. Non-external; does nothing.
- (void)encodeWithCoder:(id)coder {
}

// Native +[NSCalendar currentCalendar], 0x1c02a867c. Non-external; a tail call of +[NSCalendar _current]
// (the class reference).
+ (id)currentCalendar {
    return [ONSCalendar _current];
}

// Native +[NSCalendar calendarWithIdentifier:], 0x1c02a8688. Non-external. objc_alloc of the receiver,
// -initWithCalendarIdentifier:, autoreleased (a tail call).
+ (id)calendarWithIdentifier:(id)identifier {
    return [[[self alloc] initWithCalendarIdentifier:identifier] autorelease];
}

// Native -[NSCalendar getEra:year:month:day:fromDate:], 0x1c02a86b0. Non-external.
// A nil date in an executable linked on or after system version 9 logs "date cannot be nil" (count
// 0x22e6d38f8) and returns; otherwise the components 0x1E (era, year, month, day) of the date fill the
// non-NULL pointers in order.
- (void)getEra:(NSInteger *)eraValuePointer
          year:(NSInteger *)yearValuePointer
         month:(NSInteger *)monthValuePointer
           day:(NSInteger *)dayValuePointer
      fromDate:(id)date {
    static int32_t __count__;
    if (date == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(&__count__, kDateCannotBeNilFormat, "-[NSCalendar getEra:year:month:day:fromDate:]");
        return;
    }
    ONSDateComponents *components = [self components:0x1E fromDate:date];
    if (eraValuePointer != NULL) {
        *eraValuePointer = [components era];
    }
    if (yearValuePointer != NULL) {
        *yearValuePointer = [components year];
    }
    if (monthValuePointer != NULL) {
        *monthValuePointer = [components month];
    }
    if (dayValuePointer != NULL) {
        *dayValuePointer = [components day];
    }
}

// Native -[NSCalendar getEra:yearForWeekOfYear:weekOfYear:weekday:fromDate:], 0x1c02a87ac. Non-external.
// As -getEra:year:month:day:fromDate: (count 0x22e6d38fc) with the components 0x6202 (era,
// yearForWeekOfYear, weekOfYear, weekday).
- (void)getEra:(NSInteger *)eraValuePointer
    yearForWeekOfYear:(NSInteger *)yearValuePointer
           weekOfYear:(NSInteger *)weekValuePointer
              weekday:(NSInteger *)weekdayValuePointer
             fromDate:(id)date {
    static int32_t __count__;
    if (date == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(
            &__count__,
            kDateCannotBeNilFormat,
            "-[NSCalendar getEra:yearForWeekOfYear:weekOfYear:weekday:fromDate:]"
        );
        return;
    }
    ONSDateComponents *components = [self components:0x6202 fromDate:date];
    if (eraValuePointer != NULL) {
        *eraValuePointer = [components era];
    }
    if (yearValuePointer != NULL) {
        *yearValuePointer = [components yearForWeekOfYear];
    }
    if (weekValuePointer != NULL) {
        *weekValuePointer = [components weekOfYear];
    }
    if (weekdayValuePointer != NULL) {
        *weekdayValuePointer = [components weekday];
    }
}

// Native -[NSCalendar getHour:minute:second:nanosecond:fromDate:], 0x1c02a88a8. Non-external.
// As -getEra:year:month:day:fromDate: (count 0x22e6d3900) with the components 0x80E0 (hour, minute, second,
// nanosecond).
- (void)getHour:(NSInteger *)hourValuePointer
         minute:(NSInteger *)minuteValuePointer
         second:(NSInteger *)secondValuePointer
     nanosecond:(NSInteger *)nanosecondValuePointer
       fromDate:(id)date {
    static int32_t __count__;
    if (date == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(
            &__count__, kDateCannotBeNilFormat, "-[NSCalendar getHour:minute:second:nanosecond:fromDate:]"
        );
        return;
    }
    ONSDateComponents *components = [self components:0x80E0 fromDate:date];
    if (hourValuePointer != NULL) {
        *hourValuePointer = [components hour];
    }
    if (minuteValuePointer != NULL) {
        *minuteValuePointer = [components minute];
    }
    if (secondValuePointer != NULL) {
        *secondValuePointer = [components second];
    }
    if (nanosecondValuePointer != NULL) {
        *nanosecondValuePointer = [components nanosecond];
    }
}

// Native -[NSCalendar component:fromDate:], 0x1c02a89a4. Non-external.
// A nil date in an executable linked on or after system version 9 logs "date cannot be nil" (count
// 0x22e6d3904) and answers 0; otherwise the unit's value of the date's components for that unit (a tail
// call of -valueForComponent:).
- (NSInteger)component:(NSUInteger)unit fromDate:(id)date {
    static int32_t __count__;
    if (date == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(&__count__, kDateCannotBeNilFormat, "-[NSCalendar component:fromDate:]");
        return 0;
    }
    return [[self components:unit fromDate:date] valueForComponent:unit];
}

// Native -[NSCalendar dateWithEra:year:month:day:hour:minute:second:nanosecond:], 0x1c02a8a60. Non-external.
// A new autoreleased NSDateComponents (objc_opt_new) gets the eight values in order; -dateFromComponents:
// (a tail call).
- (id)dateWithEra:(NSInteger)eraValue
             year:(NSInteger)yearValue
            month:(NSInteger)monthValue
              day:(NSInteger)dayValue
             hour:(NSInteger)hourValue
           minute:(NSInteger)minuteValue
           second:(NSInteger)secondValue
       nanosecond:(NSInteger)nanosecondValue {
    ONSDateComponents *components = [[ONSDateComponents new] autorelease];
    [components setEra:eraValue];
    [components setYear:yearValue];
    [components setMonth:monthValue];
    [components setDay:dayValue];
    [components setHour:hourValue];
    [components setMinute:minuteValue];
    [components setSecond:secondValue];
    [components setNanosecond:nanosecondValue];
    return [self dateFromComponents:components];
}

// Native -[NSCalendar dateWithEra:yearForWeekOfYear:weekOfYear:weekday:hour:minute:second:nanosecond:],
// 0x1c02a8b30. Non-external. As the method above with the week-based values.
- (id)dateWithEra:(NSInteger)eraValue
    yearForWeekOfYear:(NSInteger)yearValue
           weekOfYear:(NSInteger)weekValue
              weekday:(NSInteger)weekdayValue
                 hour:(NSInteger)hourValue
               minute:(NSInteger)minuteValue
               second:(NSInteger)secondValue
           nanosecond:(NSInteger)nanosecondValue {
    ONSDateComponents *components = [[ONSDateComponents new] autorelease];
    [components setEra:eraValue];
    [components setYearForWeekOfYear:yearValue];
    [components setWeekOfYear:weekValue];
    [components setWeekday:weekdayValue];
    [components setHour:hourValue];
    [components setMinute:minuteValue];
    [components setSecond:secondValue];
    [components setNanosecond:nanosecondValue];
    return [self dateFromComponents:components];
}

// Native -[NSCalendar startOfDayForDate:], 0x1c02a8c00. Non-external. The start date of the day containing
// the date (-rangeOfUnit:startDate:interval:forDate: with 0x10, its result ignored; nil when not found).
- (id)startOfDayForDate:(id)date {
    id start = nil;
    double interval = 0.0;
    [self rangeOfUnit:0x10 startDate:&start interval:&interval forDate:date];
    return start;
}

// Native -[NSCalendar componentsInTimeZone:fromDate:], 0x1c02a8c60. Non-external.
// The calendar's time zone is replaced by the given one around -components:fromDate: with 0x31FEFE (every
// component, the calendar and the time zone), then restored (the old time zone is not retained).
- (id)componentsInTimeZone:(id)timezone fromDate:(id)date {
    id timeZone = [self timeZone];
    [self setTimeZone:timezone];
    id components = [self components:0x31FEFE fromDate:date];
    [self setTimeZone:timeZone];
    return components;
}

// Native -[NSCalendar compareDate:toDate:toUnitGranularity:], 0x1c02a8cc8. Non-external.
// The two intervals since the reference date are read first. When either lies outside the component bounds
// (at or below the lower bound, at or above the upper one; a NaN is inside), they are compared directly:
// equal 0, else 1 when the first is greater, else -1 (also unordered). Otherwise, by unit:
// - day (0x10) and hour (0x20): the unit's range containing the first date (start nil and interval 0
//   before the call); 0 when the second interval lies in [start, start + interval), else 1 when it is before
//   the start, else -1;
// - minute (0x40): the integral parts (modf, each date read again) divided by 60 and floored are compared
//   (equal 0, else 1 when the second is less, else -1);
// - second (0x80) and nanosecond (0x8000): the integral parts are compared the same way;
// - time zone (0x100000) and calendar (0x200000): 0;
// - any other unit: a list of units (weekday 0x200 and weekOfMonth 0x1000: era, year, month, weekOfMonth,
//   weekday; weekdayOrdinal 0x400: era, year, month, weekdayOrdinal, day; weekOfYear 0x2000 and
//   yearForWeekOfYear 0x4000: era, yearForWeekOfYear, weekOfYear, weekday; otherwise era, year, month, day)
//   whose union is fetched for both dates; the values are compared in list order (1 or -1 at the first
//   difference), the Chinese calendar's months also by leap month (a leap month is later), and the comparison
//   ends with 0 at the given unit or after the list.
- (NSInteger)compareDate:(id)date1 toDate:(id)date2 toUnitGranularity:(NSUInteger)unit {
    double interval1 = [date1 timeIntervalSinceReferenceDate];
    double interval2 = [date2 timeIntervalSinceReferenceDate];
    if (interval1 <= kComponentComparisonLowerBound || interval1 >= kComponentComparisonUpperBound ||
        interval2 >= kComponentComparisonUpperBound || interval2 <= kComponentComparisonLowerBound) {
        if (interval1 == interval2) {
            return 0;
        }
        return interval1 > interval2 ? 1 : -1;
    }
    switch (unit) {
        case 0x10:
        case 0x20: {
            id start = nil;
            double interval = 0.0;
            [self rangeOfUnit:unit startDate:&start interval:&interval forDate:date1];
            double startInterval = [start timeIntervalSinceReferenceDate];
            double otherInterval = [date2 timeIntervalSinceReferenceDate];
            if (startInterval <= otherInterval && otherInterval < startInterval + interval) {
                return 0;
            }
            return otherInterval < startInterval ? 1 : -1;
        }
        case 0x40: {
            double integral1;
            double integral2;
            modf([date1 timeIntervalSinceReferenceDate], &integral1);
            modf([date2 timeIntervalSinceReferenceDate], &integral2);
            double minutes1 = floor(integral1 / 60.0);
            double minutes2 = floor(integral2 / 60.0);
            if (minutes1 == minutes2) {
                return 0;
            }
            return minutes2 < minutes1 ? 1 : -1;
        }
        case 0x80:
        case 0x8000: {
            double integral1;
            double integral2;
            modf([date1 timeIntervalSinceReferenceDate], &integral1);
            modf([date2 timeIntervalSinceReferenceDate], &integral2);
            if (integral1 == integral2) {
                return 0;
            }
            return integral2 < integral1 ? 1 : -1;
        }
        case 0x100000:
        case 0x200000:
            return 0;
        default:
            break;
    }
    const NSUInteger ordinalUnits[5] = {0x2, 0x4, 0x8, 0x400, 0x10};
    const NSUInteger weekOfMonthUnits[5] = {0x2, 0x4, 0x8, 0x1000, 0x200};
    static const NSUInteger dayUnits[4] = {0x2, 0x4, 0x8, 0x10};
    static const NSUInteger weekOfYearUnits[4] = {0x2, 0x4000, 0x2000, 0x200};
    const NSUInteger *units;
    NSUInteger count;
    if (unit <= 0xFFF) {
        if (unit == 0x200) {
            units = weekOfMonthUnits;
            count = 5;
        } else if (unit == 0x400) {
            units = ordinalUnits;
            count = 5;
        } else {
            units = dayUnits;
            count = 4;
        }
    } else if (unit == 0x1000) {
        units = weekOfMonthUnits;
        count = 5;
    } else if (unit == 0x2000 || unit == 0x4000) {
        units = weekOfYearUnits;
        count = 4;
    } else {
        units = dayUnits;
        count = 4;
    }
    NSUInteger unitFlags = 0;
    for (NSUInteger index = 0; index < count; index++) {
        unitFlags |= units[index];
    }
    ONSDateComponents *components1 = [self components:unitFlags fromDate:date1];
    ONSDateComponents *components2 = [self components:unitFlags fromDate:date2];
    OCFCalendarIdentifier chinese = kOCFChineseCalendar;
    for (NSUInteger index = 0; index < count; index++) {
        NSInteger value1 = [components1 valueForComponent:units[index]];
        NSInteger value2 = [components2 valueForComponent:units[index]];
        if (value1 > value2) {
            return 1;
        }
        if (value1 < value2) {
            return -1;
        }
        if (units[index] == 0x8 && [[self calendarIdentifier] isEqualToString:(id)chinese]) {
            BOOL leap1 = [components1 isLeapMonth];
            BOOL leap2 = [components2 isLeapMonth];
            if (!leap1 && leap2) {
                return -1;
            }
            if (leap1 && !leap2) {
                return 1;
            }
        }
        if (units[index] == unit) {
            return 0;
        }
    }
    return 0;
}

// Native -[NSCalendar isDate:equalToDate:toUnitGranularity:], 0x1c02a9084. Non-external.
// In an executable linked on or after system version 9 a nil first date logs "date1 cannot be nil" (count
// 0x22e6d3908) and a nil second date "date2 cannot be nil" (count 0x22e6d390c), answering NO; otherwise
// whether -compareDate:toDate:toUnitGranularity: answers 0.
- (BOOL)isDate:(id)date1 equalToDate:(id)date2 toUnitGranularity:(NSUInteger)unit {
    static int32_t __count__;
    static int32_t __count__18;
    if (date1 == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(
            &__count__, kDate1CannotBeNilFormat, "-[NSCalendar isDate:equalToDate:toUnitGranularity:]"
        );
        return NO;
    }
    if (date2 == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(
            &__count__18, kDate2CannotBeNilFormat, "-[NSCalendar isDate:equalToDate:toUnitGranularity:]"
        );
        return NO;
    }
    return [self compareDate:date1 toDate:date2 toUnitGranularity:unit] == 0;
}

// Native -[NSCalendar isDate:inSameDayAsDate:], 0x1c02a91a0. Non-external. As the method above (counts
// 0x22e6d3910 and 0x22e6d3914) with the day unit (0x10).
- (BOOL)isDate:(id)date1 inSameDayAsDate:(id)date2 {
    static int32_t __count__;
    static int32_t __count__21;
    if (date1 == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(&__count__, kDate1CannotBeNilFormat, "-[NSCalendar isDate:inSameDayAsDate:]");
        return NO;
    }
    if (date2 == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(&__count__21, kDate2CannotBeNilFormat, "-[NSCalendar isDate:inSameDayAsDate:]");
        return NO;
    }
    return [self compareDate:date1 toDate:date2 toUnitGranularity:0x10] == 0;
}

// Native -[NSCalendar isDateInToday:], 0x1c02a92b8. Non-external.
// A nil date in an executable linked on or after system version 9 logs "date cannot be nil" (count
// 0x22e6d3918) and answers NO; otherwise whether the date compares equal to +[NSDate date] by day.
- (BOOL)isDateInToday:(id)date {
    static int32_t __count__;
    if (date == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(&__count__, kDateCannotBeNilFormat, "-[NSCalendar isDateInToday:]");
        return NO;
    }
    return [self compareDate:date toDate:[ONSDate date] toUnitGranularity:0x10] == 0;
}

// Native -[NSCalendar isDateInYesterday:], 0x1c02a936c. Non-external.
// A nil date in an executable linked on or after system version 9 logs "date cannot be nil" (count
// 0x22e6d391c) and answers NO. Otherwise the day containing +[NSDate date] is found (start nil, interval 0;
// NO when -rangeOfUnit:startDate:interval:forDate: fails), and the answer is whether the date compares equal
// by day to its start moved by -60 seconds.
- (BOOL)isDateInYesterday:(id)date {
    static int32_t __count__;
    if (date == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(&__count__, kDateCannotBeNilFormat, "-[NSCalendar isDateInYesterday:]");
        return NO;
    }
    double interval = 0.0;
    id start = nil;
    if (![self rangeOfUnit:0x10 startDate:&start interval:&interval forDate:[ONSDate date]]) {
        return NO;
    }
    return [self compareDate:date toDate:[start dateByAddingTimeInterval:-60.0] toUnitGranularity:0x10] == 0;
}

// Native -[NSCalendar isDateInTomorrow:], 0x1c02a947c. Non-external. As -isDateInYesterday: (count
// 0x22e6d3920) with the start moved by the day's interval plus 60 seconds.
- (BOOL)isDateInTomorrow:(id)date {
    static int32_t __count__;
    if (date == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(&__count__, kDateCannotBeNilFormat, "-[NSCalendar isDateInTomorrow:]");
        return NO;
    }
    double interval = 0.0;
    id start = nil;
    if (![self rangeOfUnit:0x10 startDate:&start interval:&interval forDate:[ONSDate date]]) {
        return NO;
    }
    return [self compareDate:date
                          toDate:[start dateByAddingTimeInterval:interval + 60.0]
               toUnitGranularity:0x10] == 0;
}

// Native -[NSCalendar rangeOfWeekendStartDate:interval:containingDate:], 0x1c02a9594. Non-external.
// The next weekend after the date (options 0) and, from its start, the previous one (options 4,
// NSCalendarSearchBackwards) are found, all four outputs nil and 0 first; the date's and the second start's
// intervals are read even when a search failed. When both searches succeed and the date lies in [start,
// start + interval) of the second weekend, its start and interval are stored through the non-NULL pointers
// and the answer is YES; otherwise NO.
- (BOOL)rangeOfWeekendStartDate:(id *)datep interval:(double *)tip containingDate:(id)date {
    id previousStart = nil;
    id nextStart = nil;
    double previousInterval = 0.0;
    double nextInterval = 0.0;
    BOOL failed;
    if ([self nextWeekendStartDate:&nextStart interval:&nextInterval options:0 afterDate:date]) {
        failed = ![self nextWeekendStartDate:&previousStart
                                    interval:&previousInterval
                                     options:4
                                   afterDate:nextStart];
    } else {
        failed = YES;
    }
    double dateInterval = [date timeIntervalSinceReferenceDate];
    double startInterval = [previousStart timeIntervalSinceReferenceDate];
    if (failed) {
        return NO;
    }
    double interval = previousInterval;
    if (!(startInterval <= dateInterval && dateInterval < startInterval + interval)) {
        return NO;
    }
    if (datep != NULL) {
        *datep = previousStart;
    }
    if (tip != NULL) {
        *tip = interval;
    }
    return YES;
}

// Native -[NSCalendar components:fromDateComponents:toDateComponents:options:], 0x1c02a9690. Non-external.
// Each components' date is made by its own calendar or, without one, the receiver; then
// -components:fromDate:toDate:options: (a tail call).
- (id)components:(NSUInteger)unitFlags
    fromDateComponents:(id)startingDateComp
      toDateComponents:(id)resultDateComp
               options:(NSUInteger)options {
    ONSCalendar *startCalendar = [startingDateComp calendar];
    id startDate = [(startCalendar != nil ? startCalendar : self) dateFromComponents:startingDateComp];
    ONSCalendar *resultCalendar = [resultDateComp calendar];
    id resultDate = [(resultCalendar != nil ? resultCalendar : self) dateFromComponents:resultDateComp];
    return [self components:unitFlags fromDate:startDate toDate:resultDate options:options];
}

// Native -[NSCalendar dateByAddingUnit:value:toDate:options:], 0x1c02a9714. Non-external. A new autoreleased
// NSDateComponents (objc_opt_new) with the value for the unit; -dateByAddingComponents:toDate:options: (a
// tail call).
- (id)dateByAddingUnit:(NSUInteger)unit value:(NSInteger)value toDate:(id)date options:(NSUInteger)options {
    ONSDateComponents *components = [[ONSDateComponents new] autorelease];
    [components setValue:value forComponent:unit];
    return [self dateByAddingComponents:components toDate:date options:options];
}

// Native -[NSCalendar nextDateAfterDate:matchingComponents:options:], 0x1c02a9780, with
// ___Block_byref_object_copy__3 (0x1c02a9860), ___Block_byref_object_dispose__3 (0x1c02a9870) and
// ___59-[NSCalendar nextDateAfterDate:matchingComponents:options:]_block_invoke (0x1c02a987c). Non-external.
// The first date the enumeration yields (stored unretained in a __block variable, then the enumeration
// stopped), or nil.
- (id)nextDateAfterDate:(id)date matchingComponents:(id)comps options:(NSUInteger)options {
    __block id result = nil;
    [self enumerateDatesStartingAfterDate:date
                       matchingComponents:comps
                                  options:options
                               usingBlock:^(id found, BOOL exactMatch, BOOL *stop) {
                                 (void)exactMatch;
                                 result = found;
                                 *stop = YES;
                               }];
    return result;
}

// Native -[NSCalendar nextDateAfterDate:matchingUnit:value:options:], 0x1c02a9894. Non-external.
// A nil date in an executable linked on or after system version 9 logs "date cannot be nil" (count
// 0x22e6d3924) and answers nil. Otherwise a new autoreleased NSDateComponents with the value for the unit;
// -nextDateAfterDate:matchingComponents:options: (a tail call).
- (id)nextDateAfterDate:(id)date
           matchingUnit:(NSUInteger)unit
                  value:(NSInteger)value
                options:(NSUInteger)options {
    static int32_t __count__;
    if (date == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(
            &__count__, kDateCannotBeNilFormat, "-[NSCalendar nextDateAfterDate:matchingUnit:value:options:]"
        );
        return nil;
    }
    ONSDateComponents *components = [[ONSDateComponents new] autorelease];
    [components setValue:value forComponent:unit];
    return [self nextDateAfterDate:date matchingComponents:components options:options];
}

// Native -[NSCalendar nextDateAfterDate:matchingHour:minute:second:options:], 0x1c02a9980. Non-external.
// As the method above (count 0x22e6d3928) with the hour, minute and second set.
- (id)nextDateAfterDate:(id)date
           matchingHour:(NSInteger)hourValue
                 minute:(NSInteger)minuteValue
                 second:(NSInteger)secondValue
                options:(NSUInteger)options {
    static int32_t __count__;
    if (date == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(
            &__count__,
            kDateCannotBeNilFormat,
            "-[NSCalendar nextDateAfterDate:matchingHour:minute:second:options:]"
        );
        return nil;
    }
    ONSDateComponents *components = [[ONSDateComponents new] autorelease];
    [components setHour:hourValue];
    [components setMinute:minuteValue];
    [components setSecond:secondValue];
    return [self nextDateAfterDate:date matchingComponents:components options:options];
}

// Native -[NSCalendar dateBySettingUnit:value:ofDate:options:], 0x1c02a9a90, with
// ___53-[NSCalendar dateBySettingUnit:value:ofDate:options:]_block_invoke (0x1c02a9be0) and the byref helpers
// above. Non-external.
// When the date's value for the unit already equals the value, the date itself (not retained). Otherwise a
// new NSDateComponents (objc_alloc_init) with the value for the unit drives an enumeration after the date
// (options 0 become 0x400, NSCalendarMatchNextTime), released afterwards; the first date yielded (stored
// unretained, the enumeration stopped) or nil.
- (id)dateBySettingUnit:(NSUInteger)unit value:(NSInteger)v ofDate:(id)date options:(NSUInteger)opts {
    if ([self component:unit fromDate:date] == v) {
        return date;
    }
    NSUInteger options = opts == 0 ? 0x400 : opts;
    ONSDateComponents *components = [[ONSDateComponents alloc] init];
    [components setValue:v forComponent:unit];
    __block id result = nil;
    [self enumerateDatesStartingAfterDate:date
                       matchingComponents:components
                                  options:options
                               usingBlock:^(id found, BOOL exactMatch, BOOL *stop) {
                                 (void)exactMatch;
                                 result = found;
                                 *stop = YES;
                               }];
    [components release];
    return result;
}

// Native -[NSCalendar dateBySettingHour:minute:second:ofDate:options:], 0x1c02a9bf8. Non-external.
// A nil date in an executable linked on or after system version 9 logs "date cannot be nil" (count
// 0x22e6d392c) and answers nil. Otherwise the day containing the date is found (start nil and interval 0
// first); no start answers nil. A new NSDateComponents (objc_alloc_init) with the hour, minute and second
// searches after the start moved by -0.5 seconds with the options 2 (NSCalendarMatchStrictly) when the given
// options have bit 1, else 0x400 (NSCalendarMatchNextTime), combined with 0x2000 (NSCalendarMatchLast) when
// they have bit 13, else 0x1000 (NSCalendarMatchFirst); a result that compares before the start
// (NSOrderedAscending) is searched again after the start itself. The components are released.
- (id)dateBySettingHour:(NSInteger)h
                 minute:(NSInteger)m
                 second:(NSInteger)s
                 ofDate:(id)date
                options:(NSUInteger)opts {
    static int32_t __count__;
    if (date == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        logNilArgument(
            &__count__,
            kDateCannotBeNilFormat,
            "-[NSCalendar dateBySettingHour:minute:second:ofDate:options:]"
        );
        return nil;
    }
    double interval = 0.0;
    id start = nil;
    [self rangeOfUnit:0x10 startDate:&start interval:&interval forDate:date];
    if (start == nil) {
        return nil;
    }
    ONSDateComponents *components = [[ONSDateComponents alloc] init];
    [components setHour:h];
    [components setMinute:m];
    [components setSecond:s];
    NSUInteger strictness = (opts & 2) == 0 ? 0x400 : 2;
    NSUInteger match = (opts & 0x2000) == 0 ? 0x1000 : 0x2000;
    id result = [self nextDateAfterDate:[start dateByAddingTimeInterval:-0.5]
                     matchingComponents:components
                                options:strictness | match];
    if ([result compare:start] == -1) {
        result = [self nextDateAfterDate:start matchingComponents:components options:strictness | match];
    }
    [components release];
    return result;
}

// Native -[NSCalendar date:matchesComponents:], 0x1c02a9da0. Non-external.
// The units of kMatchUnits whose value in the components is set (not NSDateComponentUndefined) are
// collected. With none, a components object whose leap month is set answers the date's month components'
// -isLeapMonthSet (a tail call). Otherwise the date's components for those units are compared with an
// autoreleased copy of the given components: the copy takes the date's leap month and repeated day when the
// date's components have them and the given ones do not; when the date's nanosecond is set, the two
// nanoseconds must differ by at most 500 (inclusive) and are then both cleared, otherwise the answer is NO;
// finally the copy's -isEqual: with the date's components (a tail call).
- (BOOL)date:(id)date matchesComponents:(id)components {
    NSUInteger unitFlags = 0;
    for (NSUInteger index = 0; index < 14; index++) {
        NSUInteger unit = kMatchUnits[index];
        NSInteger value = [components valueForComponent:unit];
        unitFlags |= value == LONG_MAX ? 0 : unit;
    }
    if (unitFlags == 0 && [components isLeapMonthSet]) {
        return [[self components:0x8 fromDate:date] isLeapMonthSet];
    }
    ONSDateComponents *dateComponents = [self components:unitFlags fromDate:date];
    ONSDateComponents *copy = [[components copy] autorelease];
    if ([dateComponents isLeapMonthSet] && ![components isLeapMonthSet]) {
        [copy setLeapMonth:[dateComponents isLeapMonth]];
    }
    if ([dateComponents isRepeatedDaySet] && ![components isRepeatedDaySet]) {
        [copy setRepeatedDay:[dateComponents isRepeatedDay]];
    }
    NSInteger nanosecond = [dateComponents valueForComponent:0x8000];
    if (nanosecond != LONG_MAX) {
        NSInteger copyNanosecond = [copy valueForComponent:0x8000];
        if ((uint64_t)nanosecond - (uint64_t)copyNanosecond - 501 < (uint64_t)-1001) {
            return NO;
        }
        [dateComponents setNanosecond:0];
        [copy setNanosecond:0];
    }
    return [copy isEqual:dateComponents];
}

// Native -[NSCalendar initWithCalendarIdentifier:], 0x1c02a9f40. Non-external; the receiver (RET).
- (id)initWithCalendarIdentifier:(id)identifier {
    return self;
}

// Native -[NSCalendar setGregorianStartDate:], 0x1c02a9f44. Non-external; does nothing.
- (void)setGregorianStartDate:(id)date {
}

// Native -[NSCalendar gregorianStartDate], 0x1c02a9f48. Non-external; nil.
- (id)gregorianStartDate {
    return nil;
}

// Native -[NSCalendar rangeOfUnit:startDate:interval:forDate:], 0x1c02a9f50. Non-external; NO (the outputs
// are not written).
- (BOOL)rangeOfUnit:(NSUInteger)unit startDate:(id *)datep interval:(double *)tip forDate:(id)date {
    return NO;
}

// Native -[NSCalendar eraSymbols], 0x1c02a9f58. Non-external. kCFDateFormatterEraSymbolsKey of a formatter
// for the calendar (formatterSymbols).
- (id)eraSymbols {
    return formatterSymbols(self, kOCFDateFormatterEraSymbolsKey);
}

// Native -[NSCalendar monthSymbols], 0x1c02a9fd8. Non-external. kCFDateFormatterMonthSymbolsKey.
- (id)monthSymbols {
    return formatterSymbols(self, kOCFDateFormatterMonthSymbolsKey);
}

// Native -[NSCalendar shortMonthSymbols], 0x1c02aa058. Non-external. kCFDateFormatterShortMonthSymbolsKey.
- (id)shortMonthSymbols {
    return formatterSymbols(self, kOCFDateFormatterShortMonthSymbolsKey);
}

// Native -[NSCalendar weekdaySymbols], 0x1c02aa0d8. Non-external. kCFDateFormatterWeekdaySymbolsKey.
- (id)weekdaySymbols {
    return formatterSymbols(self, kOCFDateFormatterWeekdaySymbolsKey);
}

// Native -[NSCalendar shortWeekdaySymbols], 0x1c02aa158. Non-external.
// kCFDateFormatterShortWeekdaySymbolsKey.
- (id)shortWeekdaySymbols {
    return formatterSymbols(self, kOCFDateFormatterShortWeekdaySymbolsKey);
}

// Native -[NSCalendar AMSymbol], 0x1c02aa1d8. Non-external. kCFDateFormatterAMSymbolKey.
- (id)AMSymbol {
    return formatterSymbols(self, kOCFDateFormatterAMSymbolKey);
}

// Native -[NSCalendar PMSymbol], 0x1c02aa258. Non-external. kCFDateFormatterPMSymbolKey.
- (id)PMSymbol {
    return formatterSymbols(self, kOCFDateFormatterPMSymbolKey);
}

// Native -[NSCalendar longEraSymbols], 0x1c02aa2d8. Non-external. kCFDateFormatterLongEraSymbolsKey.
- (id)longEraSymbols {
    return formatterSymbols(self, kOCFDateFormatterLongEraSymbolsKey);
}

// Native -[NSCalendar veryShortMonthSymbols], 0x1c02aa358. Non-external.
// kCFDateFormatterVeryShortMonthSymbolsKey.
- (id)veryShortMonthSymbols {
    return formatterSymbols(self, kOCFDateFormatterVeryShortMonthSymbolsKey);
}

// Native -[NSCalendar standaloneMonthSymbols], 0x1c02aa3d8. Non-external.
// kCFDateFormatterStandaloneMonthSymbolsKey.
- (id)standaloneMonthSymbols {
    return formatterSymbols(self, kOCFDateFormatterStandaloneMonthSymbolsKey);
}

// Native -[NSCalendar shortStandaloneMonthSymbols], 0x1c02aa458. Non-external.
// kCFDateFormatterShortStandaloneMonthSymbolsKey.
- (id)shortStandaloneMonthSymbols {
    return formatterSymbols(self, kOCFDateFormatterShortStandaloneMonthSymbolsKey);
}

// Native -[NSCalendar veryShortStandaloneMonthSymbols], 0x1c02aa4d8. Non-external.
// kCFDateFormatterVeryShortStandaloneMonthSymbolsKey.
- (id)veryShortStandaloneMonthSymbols {
    return formatterSymbols(self, kOCFDateFormatterVeryShortStandaloneMonthSymbolsKey);
}

// Native -[NSCalendar veryShortWeekdaySymbols], 0x1c02aa558. Non-external.
// kCFDateFormatterVeryShortWeekdaySymbolsKey.
- (id)veryShortWeekdaySymbols {
    return formatterSymbols(self, kOCFDateFormatterVeryShortWeekdaySymbolsKey);
}

// Native -[NSCalendar standaloneWeekdaySymbols], 0x1c02aa5d8. Non-external.
// kCFDateFormatterStandaloneWeekdaySymbolsKey.
- (id)standaloneWeekdaySymbols {
    return formatterSymbols(self, kOCFDateFormatterStandaloneWeekdaySymbolsKey);
}

// Native -[NSCalendar shortStandaloneWeekdaySymbols], 0x1c02aa658. Non-external.
// kCFDateFormatterShortStandaloneWeekdaySymbolsKey.
- (id)shortStandaloneWeekdaySymbols {
    return formatterSymbols(self, kOCFDateFormatterShortStandaloneWeekdaySymbolsKey);
}

// Native -[NSCalendar veryShortStandaloneWeekdaySymbols], 0x1c02aa6d8. Non-external.
// kCFDateFormatterVeryShortStandaloneWeekdaySymbolsKey.
- (id)veryShortStandaloneWeekdaySymbols {
    return formatterSymbols(self, kOCFDateFormatterVeryShortStandaloneWeekdaySymbolsKey);
}

// Native -[NSCalendar quarterSymbols], 0x1c02aa758. Non-external. kCFDateFormatterQuarterSymbolsKey.
- (id)quarterSymbols {
    return formatterSymbols(self, kOCFDateFormatterQuarterSymbolsKey);
}

// Native -[NSCalendar shortQuarterSymbols], 0x1c02aa7d8. Non-external.
// kCFDateFormatterShortQuarterSymbolsKey.
- (id)shortQuarterSymbols {
    return formatterSymbols(self, kOCFDateFormatterShortQuarterSymbolsKey);
}

// Native -[NSCalendar standaloneQuarterSymbols], 0x1c02aa858. Non-external.
// kCFDateFormatterStandaloneQuarterSymbolsKey.
- (id)standaloneQuarterSymbols {
    return formatterSymbols(self, kOCFDateFormatterStandaloneQuarterSymbolsKey);
}

// Native -[NSCalendar shortStandaloneQuarterSymbols], 0x1c02aa8d8. Non-external.
// kCFDateFormatterShortStandaloneQuarterSymbolsKey.
- (id)shortStandaloneQuarterSymbols {
    return formatterSymbols(self, kOCFDateFormatterShortStandaloneQuarterSymbolsKey);
}

// Native -[NSCalendar isDateInWeekend:], 0x1c0310110 (cold region). Non-external. The abstract method:
// __CFRequireConcreteImplementation of NSCalendar (looked up by name), then NO.
// Folds the compiler-outlined pieces OUTLINED_FUNCTION_1_33 (0x1c02aab80).
- (BOOL)isDateInWeekend:(id)date {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return NO;
}

// Native -[NSCalendar nextWeekendStartDate:interval:options:afterDate:], 0x1c0310134 (cold region).
// Non-external. The abstract method, then NO.
- (BOOL)nextWeekendStartDate:(id *)datep
                    interval:(double *)tip
                     options:(NSUInteger)options
                   afterDate:(id)date {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return NO;
}

// Native -[NSCalendar enumerateDatesStartingAfterDate:matchingComponents:options:usingBlock:], 0x1c0310158
// (cold region). Non-external. The abstract method (a tail call of __CFRequireConcreteImplementation).
// Folds the compiler-outlined pieces OUTLINED_FUNCTION_2_29 (0x1c02aab8c), OUTLINED_FUNCTION_4_12
// (0x1c02aaba0).
- (void)
    enumerateDatesStartingAfterDate:(id)start
                 matchingComponents:(id)comps
                            options:(NSUInteger)opts
                         usingBlock:
                             (void(__attribute__((noescape)) ^)(id date, BOOL exactMatch, BOOL *stop))block {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
}

// Native -[NSCalendar calendarIdentifier], 0x1c0310174 (cold region). Non-external. The abstract method,
// then nil.
// Folds the compiler-outlined pieces OUTLINED_FUNCTION_3_14 (0x1c02aab98).
- (id)calendarIdentifier {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return nil;
}

// Native -[NSCalendar setLocale:], 0x1c0310194 (cold region). Non-external. The abstract method (a tail
// call).
- (void)setLocale:(id)locale {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
}

// Native -[NSCalendar locale], 0x1c03101b0 (cold region). Non-external. The abstract method, then nil.
- (id)locale {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return nil;
}

// Native -[NSCalendar setTimeZone:], 0x1c03101d0 (cold region). Non-external. The abstract method (a tail
// call).
- (void)setTimeZone:(id)timeZone {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
}

// Native -[NSCalendar timeZone], 0x1c03101ec (cold region). Non-external. The abstract method, then nil.
- (id)timeZone {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return nil;
}

// Native -[NSCalendar setFirstWeekday:], 0x1c031020c (cold region). Non-external. The abstract method (a
// tail call).
- (void)setFirstWeekday:(NSUInteger)weekday {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
}

// Native -[NSCalendar firstWeekday], 0x1c0310228 (cold region). Non-external. The abstract method, then 0.
- (NSUInteger)firstWeekday {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return 0;
}

// Native -[NSCalendar setMinimumDaysInFirstWeek:], 0x1c0310248 (cold region). Non-external. The abstract
// method (a tail call).
- (void)setMinimumDaysInFirstWeek:(NSUInteger)days {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
}

// Native -[NSCalendar minimumDaysInFirstWeek], 0x1c0310264 (cold region). Non-external. The abstract method,
// then 0.
- (NSUInteger)minimumDaysInFirstWeek {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return 0;
}

// Native -[NSCalendar minimumRangeOfUnit:], 0x1c0310284 (cold region). Non-external. The abstract method,
// then {0, 0}.
- (OCFRange)minimumRangeOfUnit:(NSUInteger)unit {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return (OCFRange){0, 0};
}

// Native -[NSCalendar maximumRangeOfUnit:], 0x1c03102a8 (cold region). Non-external. The abstract method,
// then {0, 0}.
- (OCFRange)maximumRangeOfUnit:(NSUInteger)unit {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return (OCFRange){0, 0};
}

// Native -[NSCalendar rangeOfUnit:inUnit:forDate:], 0x1c03102cc (cold region). Non-external. The abstract
// method, then {0, 0}.
- (OCFRange)rangeOfUnit:(NSUInteger)smaller inUnit:(NSUInteger)larger forDate:(id)date {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return (OCFRange){0, 0};
}

// Native -[NSCalendar ordinalityOfUnit:inUnit:forDate:], 0x1c03102f0 (cold region). Non-external. The
// abstract method, then 0.
- (NSUInteger)ordinalityOfUnit:(NSUInteger)smaller inUnit:(NSUInteger)larger forDate:(id)date {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return 0;
}

// Native -[NSCalendar dateFromComponents:], 0x1c0310310 (cold region). Non-external. The abstract method,
// then nil.
- (id)dateFromComponents:(id)components {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return nil;
}

// Native -[NSCalendar components:fromDate:], 0x1c0310330 (cold region). Non-external. The abstract method,
// then nil.
- (id)components:(NSUInteger)unitFlags fromDate:(id)date {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return nil;
}

// Native -[NSCalendar dateByAddingComponents:toDate:options:], 0x1c0310350 (cold region). Non-external. The
// abstract method, then nil.
- (id)dateByAddingComponents:(id)comps toDate:(id)date options:(NSUInteger)opts {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return nil;
}

// Native -[NSCalendar components:fromDate:toDate:options:], 0x1c0310370 (cold region). Non-external. The
// abstract method, then nil.
- (id)components:(NSUInteger)unitFlags
        fromDate:(id)startingDate
          toDate:(id)resultDate
         options:(NSUInteger)opts {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSCalendar)), self, _cmd);
    return nil;
}

@end
