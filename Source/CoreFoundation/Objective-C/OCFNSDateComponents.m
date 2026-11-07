//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSDateComponents.h"

#import "CoreFoundation/Base/OCFUtilities.h"
#import "CoreFoundation/Locale/OCFDateComponents.h"
#import "CoreFoundation/Objective-C/OCFNSCalendar.h"
#import "CoreFoundation/String/OCFString.h"

#include <objc/runtime.h>

// Inferred organization: the NSDateComponents methods of this image (-_dateComponents 0x1c0281968 through
// -description 0x1c02824f4) lie together in the iOS Simulator __text section after the NSDate methods that
// Objective-C/OCFNSDate.m restores; following the owner's decision, the class has its own translation unit.
// Objective-C compiled with manual reference counting (objc_autorelease). The native source path is not in
// the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; constant strings
// resolved from the ADRP/ADD encodings); iOS Device 24A446: unread. Reading: not yet written; the evidence is
// the assembly in Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.
//
// The super sends pass the selector addresses 0x2387a9f83 (dealloc) and 0x2387aa04d (description).

// The NSDateComponents class reference (off_2227CE2E0, and the self class reference ___SCR_NSDateComponents).
extern char dateComponentsClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDateComponents));

// libobjc's __CFAllocateObject is CoreFoundation's (Objective-C/OCFNSObject.h); object_getIndexedIvars is
// objc/runtime.h's.

// The calendar's -dateFromComponents: and NSThread's +callStackSymbols, declared for the message sends only;
// no protocol object is emitted.
@protocol DateComponentsMessages

+ (id)callStackSymbols;

@end

// The CFDateComponents an instance carries (its first indexed ivar). dateComponentsOf is a local descriptive
// name for the load each method inlines (object_getIndexedIvars, then the first word).
static inline OCFDateComponentsRef dateComponentsOf(id object) {
    return *(OCFDateComponentsRef *)object_getIndexedIvars(object);
}

@implementation ONSDateComponents

// Native -[NSDateComponents _dateComponents], 0x1c0281968. Non-external. The carried CFDateComponents.
- (OCFDateComponentsRef)_dateComponents {
    return dateComponentsOf(self);
}

// Native +[NSDateComponents allocWithZone:], 0x1c0281980. Non-external. __CFAllocateObject of the receiver
// with 8 extra bytes (a tail call); the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __CFAllocateObject(self, 8);
}

// Native -[NSDateComponents init], 0x1c0281988. Non-external. A new CFDateComponents (system default
// allocator) is stored in the indexed ivar (the slot fetched first); the receiver.
- (id)init {
    OCFDateComponentsRef *slot = (OCFDateComponentsRef *)object_getIndexedIvars(self);
    *slot = OCFDateComponentsCreate(kOCFAllocatorSystemDefault);
    return self;
}

// Native -[NSDateComponents _initWithCFDateComponents:], 0x1c02819c8. Non-external. As -init with a copy of
// the given components (CFDateComponentsCreateCopy, system default allocator).
- (id)_initWithCFDateComponents:(OCFDateComponentsRef)dateComponents {
    OCFDateComponentsRef *slot = (OCFDateComponentsRef *)object_getIndexedIvars(self);
    *slot = OCFDateComponentsCreateCopy(kOCFAllocatorSystemDefault, dateComponents);
    return self;
}

// Native -[NSDateComponents dealloc], 0x1c0281a18. Non-external. The carried components are released when
// present, then super's -dealloc.
- (void)dealloc {
    OCFDateComponentsRef dateComponents = dateComponentsOf(self);
    if (dateComponents != NULL) {
        OCFRelease(dateComponents);
    }
    [super dealloc];
}

// Native -[NSDateComponents hash], 0x1c0281a94. Non-external. CFHash of the carried components (a tail call).
- (NSUInteger)hash {
    return OCFHash(dateComponentsOf(self));
}

// Native -[NSDateComponents isEqual:], 0x1c0281aac. Non-external. The receiver itself is equal and nil is
// not; an object that is not a kind of NSDateComponents (the self class reference) is not; otherwise CFEqual
// of the carried components (the receiver's fetched first).
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    if (![object isKindOfClass:(Class)dateComponentsClass]) {
        return NO;
    }
    OCFDateComponentsRef dateComponents = dateComponentsOf(self);
    return OCFEqual(dateComponents, dateComponentsOf(object)) ? YES : NO;
}

// Native -[NSDateComponents copyWithZone:], 0x1c0281b24. Non-external.
// An instance of exactly NSDateComponents (objc_opt_class compared with the self class reference) answers a
// new NSDateComponents (+allocWithZone: of the class reference) initialized with -_initWithCFDateComponents:
// (a tail call; the indexed ivars are fetched before the allocation, the components loaded after it). Any
// other class allocates an instance of its own class (+allocWithZone:, -init) and copies
// the calendar, time zone, era, year, quarter, month, day, hour, minute, second, nanosecond, week,
// weekOfYear, weekOfMonth, yearForWeekOfYear, weekday, weekdayOrdinal and dayOfYear in that order, then the
// leap month and repeated day when they are set.
- (id)copyWithZone:(struct _NSZone *)zone {
    Class cls = [self class];
    if (cls == (Class)dateComponentsClass) {
        OCFDateComponentsRef *slot = (OCFDateComponentsRef *)object_getIndexedIvars(self);
        ONSDateComponents *copy = [ONSDateComponents allocWithZone:zone];
        return [copy _initWithCFDateComponents:*slot];
    }
    ONSDateComponents *copy = [[cls allocWithZone:zone] init];
    [copy setCalendar:[self calendar]];
    [copy setTimeZone:[self timeZone]];
    [copy setEra:[self era]];
    [copy setYear:[self year]];
    [copy setQuarter:[self quarter]];
    [copy setMonth:[self month]];
    [copy setDay:[self day]];
    [copy setHour:[self hour]];
    [copy setMinute:[self minute]];
    [copy setSecond:[self second]];
    [copy setNanosecond:[self nanosecond]];
    [copy setWeek:[self week]];
    [copy setWeekOfYear:[self weekOfYear]];
    [copy setWeekOfMonth:[self weekOfMonth]];
    [copy setYearForWeekOfYear:[self yearForWeekOfYear]];
    [copy setWeekday:[self weekday]];
    [copy setWeekdayOrdinal:[self weekdayOrdinal]];
    [copy setDayOfYear:[self dayOfYear]];
    if ([self isLeapMonthSet]) {
        [copy setLeapMonth:[self isLeapMonth]];
    }
    if ([self isRepeatedDaySet]) {
        [copy setRepeatedDay:[self isRepeatedDay]];
    }
    return copy;
}

// Native +[NSDateComponents supportsSecureCoding], 0x1c0281d40. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSDateComponents initWithCoder:], 0x1c0281d48. Non-external; nil (the receiver is not released).
- (id)initWithCoder:(id)coder {
    return nil;
}

// Native -[NSDateComponents encodeWithCoder:], 0x1c0281d50. Non-external; does nothing.
- (void)encodeWithCoder:(id)coder {
}

// Native -[NSDateComponents calendar], 0x1c0281d54. Non-external. CFDateComponentsCopyCalendar, autoreleased
// (a tail call of objc_autorelease).
- (id)calendar {
    return [(id)OCFDateComponentsCopyCalendar(dateComponentsOf(self)) autorelease];
}

// Native -[NSDateComponents timeZone], 0x1c0281d70. Non-external. CFDateComponentsCopyTimeZone,
// autoreleased.
- (id)timeZone {
    return [(id)OCFDateComponentsCopyTimeZone(dateComponentsOf(self)) autorelease];
}

// Native -[NSDateComponents era], 0x1c0281d8c. Non-external. CFDateComponentsGetValue with 2 (a tail call);
// the getters below differ only in the unit.
- (NSInteger)era {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x2);
}

// Native -[NSDateComponents year], 0x1c0281da8. Non-external. Unit 4.
- (NSInteger)year {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x4);
}

// Native -[NSDateComponents month], 0x1c0281dc4. Non-external. Unit 8.
- (NSInteger)month {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x8);
}

// Native -[NSDateComponents day], 0x1c0281de0. Non-external. Unit 0x10.
- (NSInteger)day {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x10);
}

// Native -[NSDateComponents hour], 0x1c0281dfc. Non-external. Unit 0x20.
- (NSInteger)hour {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x20);
}

// Native -[NSDateComponents minute], 0x1c0281e18. Non-external. Unit 0x40.
- (NSInteger)minute {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x40);
}

// Native -[NSDateComponents second], 0x1c0281e34. Non-external. Unit 0x80.
- (NSInteger)second {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x80);
}

// Native -[NSDateComponents week], 0x1c0281e50. Non-external. Unit 0x100.
- (NSInteger)week {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x100);
}

// Native -[NSDateComponents weekday], 0x1c0281e6c. Non-external. Unit 0x200.
- (NSInteger)weekday {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x200);
}

// Native -[NSDateComponents weekdayOrdinal], 0x1c0281e88. Non-external. Unit 0x400.
- (NSInteger)weekdayOrdinal {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x400);
}

// Native -[NSDateComponents quarter], 0x1c0281ea4. Non-external. Unit 0x800.
- (NSInteger)quarter {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x800);
}

// Native -[NSDateComponents nanosecond], 0x1c0281ec0. Non-external. Unit 0x8000.
- (NSInteger)nanosecond {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x8000);
}

// Native -[NSDateComponents weekOfYear], 0x1c0281edc. Non-external. Unit 0x2000.
- (NSInteger)weekOfYear {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x2000);
}

// Native -[NSDateComponents weekOfMonth], 0x1c0281ef8. Non-external. Unit 0x1000.
- (NSInteger)weekOfMonth {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x1000);
}

// Native -[NSDateComponents yearForWeekOfYear], 0x1c0281f14. Non-external. Unit 0x4000.
- (NSInteger)yearForWeekOfYear {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x4000);
}

// Native -[NSDateComponents dayOfYear], 0x1c0281f30. Non-external. Unit 0x10000.
- (NSInteger)dayOfYear {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x10000);
}

// Native -[NSDateComponents isLeapMonth], 0x1c0281f4c. Non-external. The leap-month value (unit 0x40000000)
// is neither kCFUndefined nor 0.
- (BOOL)isLeapMonth {
    NSInteger value = OCFDateComponentsGetValue(dateComponentsOf(self), 0x40000000);
    return value != LONG_MAX && value != 0;
}

// Native -[NSDateComponents isRepeatedDay], 0x1c0281f7c. Non-external. The repeated-day value (unit
// 0xffffffff80000000) is neither kCFUndefined nor 0.
- (BOOL)isRepeatedDay {
    NSInteger value = OCFDateComponentsGetValue(dateComponentsOf(self), (OCFCalendarUnit)(int64_t)INT32_MIN);
    return value != LONG_MAX && value != 0;
}

// Native -[NSDateComponents isLeapMonthSet], 0x1c0281fac. Non-external. The leap-month value is not
// kCFUndefined.
- (BOOL)isLeapMonthSet {
    return OCFDateComponentsGetValue(dateComponentsOf(self), 0x40000000) != LONG_MAX;
}

// Native -[NSDateComponents isRepeatedDaySet], 0x1c0281fd8. Non-external. The repeated-day value is not
// kCFUndefined.
- (BOOL)isRepeatedDaySet {
    return OCFDateComponentsGetValue(dateComponentsOf(self), (OCFCalendarUnit)(int64_t)INT32_MIN) !=
           0x7fffffffffffffff;
}

// Native -[NSDateComponents setCalendar:], 0x1c0282004. Non-external. CFDateComponentsSetCalendar (a tail
// call).
- (void)setCalendar:(id)calendar {
    OCFDateComponentsSetCalendar(dateComponentsOf(self), (OCFCalendarRef)calendar);
}

// Native -[NSDateComponents setTimeZone:], 0x1c028202c. Non-external. CFDateComponentsSetTimeZone (a tail
// call).
- (void)setTimeZone:(id)timeZone {
    OCFDateComponentsSetTimeZone(dateComponentsOf(self), (OCFTimeZoneRef)timeZone);
}

// Native -[NSDateComponents setEra:], 0x1c0282054. Non-external. CFDateComponentsSetValue with 2 (a tail
// call); the setters below differ only in the unit.
- (void)setEra:(NSInteger)era {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x2, era);
}

// Native -[NSDateComponents setYear:], 0x1c0282080. Non-external. Unit 4.
- (void)setYear:(NSInteger)year {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x4, year);
}

// Native -[NSDateComponents setMonth:], 0x1c02820ac. Non-external. Unit 8.
- (void)setMonth:(NSInteger)month {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x8, month);
}

// Native -[NSDateComponents setDay:], 0x1c02820d8. Non-external. Unit 0x10.
- (void)setDay:(NSInteger)day {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x10, day);
}

// Native -[NSDateComponents setHour:], 0x1c0282104. Non-external. Unit 0x20.
- (void)setHour:(NSInteger)hour {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x20, hour);
}

// Native -[NSDateComponents setMinute:], 0x1c0282130. Non-external. Unit 0x40.
- (void)setMinute:(NSInteger)minute {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x40, minute);
}

// Native -[NSDateComponents setSecond:], 0x1c028215c. Non-external. Unit 0x80.
- (void)setSecond:(NSInteger)second {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x80, second);
}

// Native -[NSDateComponents setWeek:], 0x1c0282188. Non-external. Unit 0x100.
- (void)setWeek:(NSInteger)week {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x100, week);
}

// Native -[NSDateComponents setWeekday:], 0x1c02821b4. Non-external. Unit 0x200.
- (void)setWeekday:(NSInteger)weekday {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x200, weekday);
}

// Native -[NSDateComponents setWeekdayOrdinal:], 0x1c02821e0. Non-external. Unit 0x400.
- (void)setWeekdayOrdinal:(NSInteger)weekdayOrdinal {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x400, weekdayOrdinal);
}

// Native -[NSDateComponents setQuarter:], 0x1c028220c. Non-external. Unit 0x800.
- (void)setQuarter:(NSInteger)quarter {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x800, quarter);
}

// Native -[NSDateComponents setNanosecond:], 0x1c0282238. Non-external. Unit 0x8000.
- (void)setNanosecond:(NSInteger)nanosecond {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x8000, nanosecond);
}

// Native -[NSDateComponents setWeekOfYear:], 0x1c0282264. Non-external. Unit 0x2000.
- (void)setWeekOfYear:(NSInteger)weekOfYear {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x2000, weekOfYear);
}

// Native -[NSDateComponents setWeekOfMonth:], 0x1c0282290. Non-external. Unit 0x1000.
- (void)setWeekOfMonth:(NSInteger)weekOfMonth {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x1000, weekOfMonth);
}

// Native -[NSDateComponents setYearForWeekOfYear:], 0x1c02822bc. Non-external. Unit 0x4000.
- (void)setYearForWeekOfYear:(NSInteger)yearForWeekOfYear {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x4000, yearForWeekOfYear);
}

// Native -[NSDateComponents setDayOfYear:], 0x1c02822e8. Non-external. Unit 0x10000.
- (void)setDayOfYear:(NSInteger)dayOfYear {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x10000, dayOfYear);
}

// Native -[NSDateComponents setLeapMonth:], 0x1c0282314. Non-external. Unit 0x40000000 with the flag
// zero-extended from 32 bits (MOV W2, W19).
- (void)setLeapMonth:(BOOL)leapMonth {
    OCFDateComponentsSetValue(dateComponentsOf(self), 0x40000000, (uint32_t)leapMonth);
}

// Native -[NSDateComponents setRepeatedDay:], 0x1c0282340. Non-external. Unit 0xffffffff80000000 with the
// flag zero-extended from 32 bits.
- (void)setRepeatedDay:(BOOL)repeatedDay {
    OCFDateComponentsSetValue(
        dateComponentsOf(self), (OCFCalendarUnit)(int64_t)INT32_MIN, (uint32_t)repeatedDay
    );
}

// Native -[NSDateComponents date], 0x1c028236c. Non-external. A time zone, when set, is given to the
// calendar first; then the calendar's -dateFromComponents: with the receiver (a tail call; the calendar is
// fetched again).
- (id)date {
    id timeZone = [self timeZone];
    if (timeZone != nil) {
        [(ONSCalendar *)[self calendar] setTimeZone:timeZone];
    }
    return [(ONSCalendar *)[self calendar] dateFromComponents:self];
}

// Native -[NSDateComponents setValue:forComponent:], 0x1c02823b0. Non-external. The calendar (0x200000) and
// leap-month (0x40000000) units change nothing; any other unit is CFDateComponentsSetValue (a tail call).
- (void)setValue:(NSInteger)value forComponent:(NSUInteger)unit {
    if (unit == 0x200000 || unit == 0x40000000) {
        return;
    }
    OCFDateComponentsSetValue(dateComponentsOf(self), unit, value);
}

// Native -[NSDateComponents valueForComponent:], 0x1c0282400. Non-external. CFDateComponentsGetValue (a tail
// call).
- (NSInteger)valueForComponent:(NSUInteger)unit {
    return OCFDateComponentsGetValue(dateComponentsOf(self), unit);
}

// Native -[NSDateComponents isValidDate], 0x1c0282428. Non-external. CFDateComponentsIsValidDate, as a BOOL.
- (BOOL)isValidDate {
    return OCFDateComponentsIsValidDate(dateComponentsOf(self)) ? YES : NO;
}

// Native -[NSDateComponents isValidDateInCalendar:], 0x1c028244c. Non-external.
// A nil calendar in an executable linked on or after system version 9 logs "calendar cannot be nil" (the
// first 14 times, count _isValidDateInCalendar:.__count__ at 0x22e6d3778; CFLog level 3 with the method name
// and the -description of +[NSThread callStackSymbols]), counts and answers NO; otherwise
// CFDateComponentsIsValidDateInCalendar, as a BOOL (a nil calendar there fails its assertion).
- (BOOL)isValidDateInCalendar:(id)calendar {
    static int32_t __count__;
    if (calendar == nil && _OCFExecutableLinkedOnOrAfter(9)) {
        if (__count__ <= 13) {
            id thread = (id)__CFLookUpClass(OF_NAME_STRING(ONSThread));
            OCFLog(
                3,
                OF_STR(
                    "*** %s: calendar cannot be nil\nFuture exception.\nA few of these errors are going to "
                    "be "
                    "reported with this complaint, then further violations will simply be ignored.\nHere is "
                    "the "
                    "backtrace where this occurred this time (some frames may be missing due to compiler "
                    "optimizations):\n%@"
                ),
                "-[NSDateComponents isValidDateInCalendar:]",
                [[thread callStackSymbols] description]
            );
        }
        __count__ = __count__ + 1;
        return NO;
    }
    return OCFDateComponentsIsValidDateInCalendar(dateComponentsOf(self), (OCFCalendarRef)calendar) ? YES
                                                                                                    : NO;
}

// Native -[NSDateComponents description], 0x1c02824f4. Non-external. "%@ %@" with super's -description and
// __CFDateComponentsCopyDescriptionInner of the carried components (fetched first), a new string (system
// default allocator), autoreleased after the inner description is released.
- (id)description {
    OCFStringRef inner = __CFDateComponentsCopyDescriptionInner(dateComponentsOf(self));
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef description =
        OCFStringCreateWithFormat(allocator, NULL, OF_STR("%@ %@"), [super description], inner);
    OCFRelease(inner);
    return [(id)description autorelease];
}

@end
