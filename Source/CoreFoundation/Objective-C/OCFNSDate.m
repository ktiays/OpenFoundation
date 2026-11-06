//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSDate.h"

#import "CoreFoundation/Date/OCFTimeZoneEntryPoints.h"
#import "CoreFoundation/Locale/OCFCalendarEntryPoints.h"
#import "CoreFoundation/Locale/OCFDateFormatter.h"
#import "CoreFoundation/Locale/OCFLocale.h"
#import "CoreFoundation/Objective-C/OCFNSObjectGlue.h"
#import "CoreFoundation/Objective-C/OCFNSPlaceholderDate.h"
#import "CoreFoundation/String/OCFString.h"

#include <assert.h>
#include <math.h>
#include <os/lock.h>
#include <time.h>

// Inferred organization: the __NSDate methods (0x1c027ff98-0x1c028001c, restored from
// Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json), the __NSTaggedDate methods
// (0x1c0280048-0x1c0280180, with decodeTaggedTimeInterval
// at 0x1c02800b4) and the NSDate methods (0x1c028058c-0x1c0281618, with the outlined fragments
// OUTLINED_FUNCTION_1_26 and _2_22 at 0x1c0281948 and 0x1c028195c, and -timeIntervalSinceReferenceDate and
// -initWithTimeIntervalSinceReferenceDate: in the cold region at 0x1c030f0a4 and 0x1c030f0c8) belong to one
// native unit, NSDate.m (the file name of decodeTaggedTimeInterval's assertion), which also holds the
// +[__NSPlaceholderDate initialize] block and the system clock callback that Date/OCFDate.c restores (another
// family's file). Objective-C compiled with manual reference counting (objc_retain, objc_autorelease). Unless
// a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full; constants read with Local/Scratch/cf-core/macho.py); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.
//
// Selector names: the native code passes the selector addresses 0x2387a9f83 (dealloc) and 0x2387a9fd4
// (allocWithZone:) for its super sends; the names follow from the methods that make them.

// libobjc's tagged-pointer tables (objc-internal.h; supplied). The GOT slots 0x222409408 and 0x222409420 lie
// outside the image and are bound by use, as Objective-C/OCFNSObject.m binds them (confidence: medium).
extern uintptr_t objc_debug_taggedpointer_obfuscator;
extern const uint8_t objc_debug_tag60_permutations[8];

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI, no SDK declaration); supplied. The
// native calls pass 0x10000 (data synchronization).
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

// Classes of this image the unit references through class references, not restored on this branch:
// NSConstantDate (the constant dates' isa), __NSPlaceholderDate (the placeholder's isa), NSCalendar,
// NSTimeZone and NSDateComponents; and the restored NSDate.
extern char constantDateClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSConstantDate));
extern char placeholderDateClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSPlaceholderDate));
extern char calendarClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSCalendar));
extern char timeZoneClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSTimeZone));
extern char dateComponentsClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDateComponents));
extern char dateClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDate));

// Selectors this unit sends to objects of classes outside it (NSCalendar, NSTimeZone, NSDateComponents and
// NSString), declared for the message sends only; no protocol object is emitted.
@protocol DateMessages

+ (id)currentCalendar;
- (BOOL)isDateInToday:(id)date;
- (BOOL)isDateInTomorrow:(id)date;
- (BOOL)isDateInYesterday:(id)date;
- (BOOL)isDate:(id)date1 inSameDayAsDate:(id)date2;
- (NSInteger)compareDate:(id)date1 toDate:(id)date2 toUnitGranularity:(NSUInteger)unit;
- (id)initWithCalendarIdentifier:(id)identifier;
- (void)setTimeZone:(id)timeZone;
- (id)dateFromComponents:(id)components;
+ (id)timeZoneForSecondsFromGMT:(NSInteger)seconds;
- (void)setYear:(NSInteger)year;
- (void)setMonth:(NSInteger)month;
- (void)setDay:(NSInteger)day;
- (void)setHour:(NSInteger)hour;
- (void)setMinute:(NSInteger)minute;
- (void)setSecond:(NSInteger)second;
- (const char *)UTF8String;
- (BOOL)hasSuffix:(id)suffix;

@end

// The constant date objects (NSConstantDate's layout as these records show it: the isa, then the time
// interval). ConstantDate and its field names are local descriptive spellings.
typedef struct {
    const void *isa;
    double timeInterval;
} ConstantDate;

_Static_assert(sizeof(ConstantDate) == 16, "A constant date is 16 bytes.");

// Native __immutablePlaceholderDate, 0x2227a82e0 (__DATA_CONST,__const, 16 bytes). Non-external: an object
// whose isa is __NSPlaceholderDate, then a zero word; the result of +[NSDate allocWithZone:] for NSDate
// itself and of +[__NSTaggedDate allocWithZone:]. Hidden rather than static, as
// -[__NSPlaceholderDate initWithTimeIntervalSinceReferenceDate:] (Objective-C/OCFNSPlaceholderDate.m)
// compares with its address. Objective-C/OCFNSPlaceholderDate.h declares it as a byte array; the definition
// carries that symbol through an assembler label, so this record is the object the declaration names.
const struct {
    const void *isa;
    uintptr_t unused;
} immutablePlaceholderDateObject __asm("_" OF_NAME_STRING(__immutablePlaceholderDate)) = {
    placeholderDateClass, 0
};

// Native _NSConstantDateDistantPast, 0x2227ab900, and _NSDateDistantPast, 0x2227ab910 (__DATA_CONST,__const).
// The constant is non-external (NSConstantDate, -63114076800.0, bits 0xc22d63c8c5000000); the pointer to it
// is external.
static const ConstantDate _NSConstantDateDistantPast = {constantDateClass, -63114076800.0};
const void *const _ONSDateDistantPast = &_NSConstantDateDistantPast;

// Native _NSConstantDateDistantFuture, 0x2227ab918, and _NSDateDistantFuture, 0x2227ab928
// (__DATA_CONST,__const). The constant is non-external (NSConstantDate, 63113904000.0, bits
// 0x422d63c37f000000); the pointer to it is external.
static const ConstantDate _NSConstantDateDistantFuture = {constantDateClass, 63113904000.0};
const void *const _ONSDateDistantFuture = &_NSConstantDateDistantFuture;

// Native descriptionWithLocale:.lock, .cached_loc, .cached_df and .changeCounter (0x22e6d3740-0x22e6d3758,
// __DATA,__bss). Non-external, function-local statics of -[NSDate descriptionWithLocale:]: the lock, the
// cached locale and formatter, and the counter of cache misses (32-bit).
static os_unfair_lock descriptionLock = OS_UNFAIR_LOCK_INIT;
static OCFLocaleRef descriptionCachedLocale;
static OCFDateFormatterRef descriptionCachedFormatter;
static int32_t descriptionChangeCounter;

// The current absolute time as the native methods inline it: clock_gettime(CLOCK_REALTIME) into a zeroed
// timespec, then fma(nanoseconds, 1e-9 (qword_1C0373650), seconds + -978307200.0 (qword_1C046C300)), the
// seconds converted and added first. currentAbsoluteTime is a local descriptive name.
static inline double currentAbsoluteTime(void) {
    struct timespec now = {0, 0};
    clock_gettime(CLOCK_REALTIME, &now);
    double seconds = (double)now.tv_sec + -978307200.0;
    return fma((double)now.tv_nsec, 1e-09, seconds);
}

@implementation __ONSDate

// Native -[__NSDate timeIntervalSinceReferenceDate], 0x1c027ff98. Non-external. The _time ivar, read through
// its offset variable.
- (double)timeIntervalSinceReferenceDate {
    return _time;
}

// Native -[__NSDate dealloc], 0x1c027ffa8. Non-external; only super's -dealloc (objc_msgSendSuper2 with the
// __NSDate class, selector 0x2387a9f83).
- (void)dealloc {
    [super dealloc];
}

// Native +[__NSDate automaticallyNotifiesObserversForKey:], 0x1c0280008. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native +[__NSDate allocWithZone:], 0x1c0280010. Non-external; the immutable placeholder.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return (id)&immutablePlaceholderDateObject;
}

// Native -[__NSDate initWithTimeIntervalSinceReferenceDate:], 0x1c028001c. Non-external. A new NSDate
// (objc_alloc of the NSDate class reference) initialized with the interval (a tail call); the receiver is not
// released.
- (id)initWithTimeIntervalSinceReferenceDate:(double)ti {
    return [[(id)dateClass alloc] initWithTimeIntervalSinceReferenceDate:ti];
}

@end

@implementation __ONSTaggedDate

// Native -[__NSTaggedDate timeIntervalSinceReferenceDate], 0x1c0280048. Non-external.
// A receiver that is not a tagged pointer traps (BRK #1). The tag is decoded with the full 64-bit obfuscator
// (skipped for pointers with bits 63, 62 and 0-2 set); a tag that is not in the seven-entry permutation
// table, or whose index is not 6, traps. The payload is the deobfuscated value shifted right by 3, masked to
// 52 bits for the extended tag (low bits 7) and to 60 bits otherwise, and decoded by decodeTaggedTimeInterval
// (a tail call).
- (double)timeIntervalSinceReferenceDate {
    uintptr_t pointer = (uintptr_t)self;
    if ((intptr_t)pointer >= 0) {
        __builtin_trap();
    }
    uintptr_t obfuscator =
        (pointer & 0xC000000000000007) == 0xC000000000000007 ? 0 : objc_debug_taggedpointer_obfuscator;
    uintptr_t value = obfuscator ^ pointer;
    uintptr_t tag = value & 7;
    uintptr_t index = 0;
    while (tag != objc_debug_tag60_permutations[index]) {
        index++;
        if (index == 7) {
            __builtin_trap();
        }
    }
    if (index != 6) {
        __builtin_trap();
    }
    uintptr_t mask = tag == 7 ? 0xFFFFFFFFFFFFF : 0xFFFFFFFFFFFFFFF;
    return decodeTaggedTimeInterval((value >> 3) & mask);
}

// Native decodeTaggedTimeInterval, 0x1c02800b4, with .cold.1 (0x1c030f0ec). Non-external; hidden rather than
// static, as -[__NSPlaceholderDate initWithTimeIntervalSinceReferenceDate:]
// (Objective-C/OCFNSPlaceholderDate.m) calls it. Zero decodes to 0.0 and all ones to -0.0. Bits above bit 59
// fail the assertion "encodedBits.unused == 0"
// (__assert_rtn, "NSDate.m", line 244). Otherwise bit 59 is the sign, bits 52-58 a signed exponent offset
// added to 0x3EF (the field shifted to bit 52 and added with 0x3EF0000000000000) and bits 0-51 the mantissa.
double decodeTaggedTimeInterval(uint64_t encodedBits) {
    uint64_t bits;
    if (encodedBits == 0) {
        return 0.0;
    }
    if (encodedBits == UINT64_MAX) {
        bits = 0x8000000000000000;
    } else {
        if ((encodedBits >> 60) != 0) {
            __assert_rtn("decodeTaggedTimeInterval", "NSDate.m", 244, "encodedBits.unused == 0");
        }
        bits = (encodedBits << 4) & 0x8000000000000000;
        bits |= encodedBits & 0xFFFFFFFFFFFFF;
        int64_t exponent = (int64_t)(encodedBits << 5) >> 57;
        bits += (uint64_t)exponent << 52;
        bits += 0x3EF0000000000000;
    }
    double result;
    __builtin_memcpy(&result, &bits, sizeof(result));
    return result;
}

// Native -[__NSTaggedDate dealloc], 0x1c028010c. Non-external; only super's -dealloc.
- (void)dealloc {
    [super dealloc];
}

// Native +[__NSTaggedDate automaticallyNotifiesObserversForKey:], 0x1c028016c. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native +[__NSTaggedDate allocWithZone:], 0x1c0280174. Non-external; the immutable placeholder.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return (id)&immutablePlaceholderDateObject;
}

// Native -[__NSTaggedDate initWithTimeIntervalSinceReferenceDate:], 0x1c0280180. Non-external. A new NSDate
// (objc_alloc of the NSDate class reference) initialized with the interval (a tail call); the receiver is not
// released.
- (id)initWithTimeIntervalSinceReferenceDate:(double)ti {
    return [[(id)dateClass alloc] initWithTimeIntervalSinceReferenceDate:ti];
}

@end

@implementation ONSDate

// Native -[NSDate isNSDate__], 0x1c028058c. Non-external.
- (BOOL)isNSDate__ {
    return YES;
}

// Native -[NSDate _cfTypeID], 0x1c0280594. Non-external; the CFDate type ID 42.
- (OCFTypeID)_cfTypeID {
    return 42;
}

// Native +[NSDate timeIntervalSinceReferenceDate], 0x1c028059c. Non-external; the current absolute time.
+ (double)timeIntervalSinceReferenceDate {
    return currentAbsoluteTime();
}

// Native +[NSDate distantFuture], 0x1c0280614. Non-external; the pointer _NSDateDistantFuture holds.
+ (id)distantFuture {
    return (id)_ONSDateDistantFuture;
}

// Native +[NSDate distantPast], 0x1c0280620. Non-external; the pointer _NSDateDistantPast holds.
+ (id)distantPast {
    return (id)_ONSDateDistantPast;
}

// Native +[NSDate now], 0x1c028062c. Non-external. objc_alloc of the NSDate class reference (not the
// receiver) before the clock is read, initialized and autoreleased.
+ (id)now {
    id date = [(id)dateClass alloc];
    return [[date initWithTimeIntervalSinceReferenceDate:currentAbsoluteTime()] autorelease];
}

// Native -[NSDate compare:], 0x1c02806c8. Non-external. The receiver itself is the same (0); a nil argument
// compares as a NaN. -1 when the receiver's interval is less, 1 when greater, 0 when equal or unordered.
- (NSInteger)compare:(id)other {
    if (self == other) {
        return 0;
    }
    double interval = [self timeIntervalSinceReferenceDate];
    double otherInterval = other != nil ? [other timeIntervalSinceReferenceDate] : __builtin_nan("");
    if (interval < otherInterval) {
        return -1;
    }
    return interval > otherInterval ? 1 : 0;
}

// Native -[NSDate earlierDate:], 0x1c0280728. Non-external. The argument when the receiver's interval is
// greater; otherwise (also unordered, and for nil, a NaN) the receiver.
- (id)earlierDate:(id)other {
    double interval = [self timeIntervalSinceReferenceDate];
    double otherInterval = other != nil ? [other timeIntervalSinceReferenceDate] : __builtin_nan("");
    return interval > otherInterval ? other : self;
}

// Native -[NSDate laterDate:], 0x1c0280778. Non-external. The argument when the receiver's interval is less;
// otherwise (also unordered, and for nil, a NaN) the receiver.
- (id)laterDate:(id)other {
    double interval = [self timeIntervalSinceReferenceDate];
    double otherInterval = other != nil ? [other timeIntervalSinceReferenceDate] : __builtin_nan("");
    return interval < otherInterval ? other : self;
}

// Native -[NSDate timeIntervalSinceDate:], 0x1c02807c8. Non-external; nil subtracts a NaN.
- (double)timeIntervalSinceDate:(id)other {
    double interval = [self timeIntervalSinceReferenceDate];
    double otherInterval = other != nil ? [other timeIntervalSinceReferenceDate] : __builtin_nan("");
    return interval - otherInterval;
}

// Native -[NSDate timeIntervalSinceNow], 0x1c0280810. Non-external. The receiver's interval is read before
// the clock.
- (double)timeIntervalSinceNow {
    double interval = [self timeIntervalSinceReferenceDate];
    return interval - currentAbsoluteTime();
}

// Native -[NSDate timeIntervalSince1970], 0x1c028089c. Non-external; plus 978307200.0 (qword_1C046C308).
- (double)timeIntervalSince1970 {
    return [self timeIntervalSinceReferenceDate] + 978307200.0;
}

// Native -[NSDate hash], 0x1c02808bc. Non-external.
// A NaN interval hashes to 0. Otherwise the magnitude (negated when below zero, so -0.0 stays) is rounded
// (floor(x + 0.5)); the integral part reduced modulo 2^64 (i - trunc(i * 2^-64) * 2^64, fused, FMSUB) and
// converted to an unsigned integer is multiplied by 2654435761, and the fraction scaled by 2^64 is subtracted
// (its magnitude, when negative) or added (when positive) after an unsigned conversion. The conversions are
// FCVTZU (saturating; NaN converts to 0).
- (NSUInteger)hash {
    double value = [self timeIntervalSinceReferenceDate];
    if (value != value) {
        return 0;
    }
    double magnitude = value < 0.0 ? -value : value;
    double integral = floor(magnitude + 0.5);
    double fraction = (magnitude - integral) * 0x1p64;
    double quotient = trunc(integral * 0x1p-64);
    double reduced = fma(-quotient, 0x1p64, integral);
    NSUInteger result = (NSUInteger)reduced * 2654435761u;
    if (fraction < 0.0) {
        result -= (NSUInteger)fabs(fraction);
    } else if (fraction > 0.0) {
        result += (NSUInteger)fraction;
    }
    return result;
}

// Native -[NSDate isEqual:], 0x1c0280950. Non-external. The receiver itself is equal, nil is not; an object
// that _NSIsNSDate rejects is not; otherwise -isEqualToDate: (a tail call).
- (BOOL)isEqual:(id)other {
    if (other == self) {
        return YES;
    }
    if (other == nil) {
        return NO;
    }
    if (!_ONSIsNSDate(other)) {
        return NO;
    }
    return [self isEqualToDate:other];
}

// Native -[NSDate isEqualToDate:], 0x1c02809a8. Non-external. nil is not equal; otherwise the intervals are
// equal, or both are NaN.
- (BOOL)isEqualToDate:(id)otherDate {
    if (otherDate == nil) {
        return NO;
    }
    double interval = [self timeIntervalSinceReferenceDate];
    double otherInterval = [otherDate timeIntervalSinceReferenceDate];
    BOOL bothNaN = interval != interval && otherInterval != otherInterval;
    return interval == otherInterval ? YES : bothNaN;
}

// Native -[NSDate descriptionWithLocale:], 0x1c0280a04. Non-external.
// With a locale (a CFLocale, else the current locale, released after use) a full-style formatter (styles 4
// and 4, kCFAllocatorSystemDefault) formats the receiver; the string is autoreleased and the formatter
// released. Without a locale, the current locale is compared with the cached locale under the lock (taken
// with option 0x10000): with the same locale and a cached formatter, the formatter is retained and used;
// otherwise the miss counter is incremented and its new value kept, and outside the lock a formatter for a
// GMT time zone (CFTimeZoneCreateWithTimeIntervalFromGMT, 0.0) with the format "uuuu-MM-dd HH:mm:ss '+0000'"
// is made (no time zone or no locale, or no formatter, leaves no result). On a miss, the cache is replaced
// under the lock only when the counter is unchanged: the old locale and formatter are released and the
// current locale and the new formatter (or NULL after a failure) retained. The formatter and the current
// locale are released at the end. A NULL result answers "<description unavailable>".
- (id)descriptionWithLocale:(id)locale {
    id result;
    if (locale != nil) {
        OCFDateFormatterRef formatter;
        OCFAllocatorRef allocator;
        if (OCFGetTypeID(locale) == OCFLocaleGetTypeID()) {
            allocator = kOCFAllocatorSystemDefault;
            formatter = OCFDateFormatterCreate(
                allocator, (OCFLocaleRef)locale, kOCFDateFormatterFullStyle, kOCFDateFormatterFullStyle
            );
            if (formatter == NULL) {
                result = nil;
                goto done;
            }
        } else {
            OCFLocaleRef current = OCFLocaleCopyCurrent();
            allocator = kOCFAllocatorSystemDefault;
            formatter = OCFDateFormatterCreate(
                allocator, current, kOCFDateFormatterFullStyle, kOCFDateFormatterFullStyle
            );
            OCFRelease(current);
            if (formatter == NULL) {
                result = nil;
                goto done;
            }
        }
        result = _OCFAutoreleasePoolAddObject(
            NULL, OCFDateFormatterCreateStringWithDate(allocator, formatter, (OCFDateRef)self)
        );
        OCFRelease(formatter);
        goto done;
    }
    OCFLocaleRef current = OCFLocaleCopyCurrent();
    os_unfair_lock_lock_with_options(&descriptionLock, 0x10000);
    bool cached = descriptionCachedLocale == current && descriptionCachedFormatter != NULL;
    OCFDateFormatterRef formatter;
    int32_t counter;
    bool failed;
    if (cached) {
        formatter = (OCFDateFormatterRef)OCFRetain(descriptionCachedFormatter);
        os_unfair_lock_unlock(&descriptionLock);
        if (formatter == NULL) {
            result = nil;
            goto releaseLocale;
        }
        counter = 0;
    } else {
        counter = ++descriptionChangeCounter;
        os_unfair_lock_unlock(&descriptionLock);
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFTimeZoneRef timeZone = OCFTimeZoneCreateWithTimeIntervalFromGMT(allocator, 0.0);
        if (timeZone == NULL || current == NULL) {
            if (timeZone != NULL) {
                OCFRelease(timeZone);
            }
            goto failure;
        }
        formatter = OCFDateFormatterCreate(
            allocator, current, kOCFDateFormatterFullStyle, kOCFDateFormatterFullStyle
        );
        if (formatter == NULL) {
            OCFRelease(timeZone);
            goto failure;
        }
        OCFDateFormatterSetProperty(formatter, kOCFDateFormatterTimeZoneKey, timeZone);
        OCFDateFormatterSetFormat(formatter, OF_STR("uuuu-MM-dd HH:mm:ss '+0000'"));
        OCFRelease(timeZone);
    }
    result = _OCFAutoreleasePoolAddObject(
        NULL, OCFDateFormatterCreateStringWithDate(kOCFAllocatorSystemDefault, formatter, (OCFDateRef)self)
    );
    if (cached) {
        OCFRelease(formatter);
        goto releaseLocale;
    }
    failed = false;
    goto updateCache;
failure:
    result = nil;
    formatter = NULL;
    failed = true;
updateCache:
    os_unfair_lock_lock_with_options(&descriptionLock, 0x10000);
    if (descriptionChangeCounter == counter) {
        if (descriptionCachedLocale != NULL) {
            OCFRelease(descriptionCachedLocale);
        }
        descriptionCachedLocale = current != NULL ? (OCFLocaleRef)OCFRetain(current) : NULL;
        if (descriptionCachedFormatter != NULL) {
            OCFRelease(descriptionCachedFormatter);
        }
        descriptionCachedFormatter = failed ? NULL : (OCFDateFormatterRef)OCFRetain(formatter);
    }
    os_unfair_lock_unlock(&descriptionLock);
    if (!failed) {
        OCFRelease(formatter);
    }
releaseLocale:
    if (current != NULL) {
        OCFRelease(current);
    }
done:
    return result != nil ? result : (id)OF_STR("<description unavailable>");
}

// Native -[NSDate description], 0x1c0280cc0. Non-external; -descriptionWithLocale: with nil (a tail call).
- (id)description {
    return [self descriptionWithLocale:nil];
}

// Native +[NSDate allocWithZone:], 0x1c0280cc8. Non-external. NSDate itself (compared with its self class
// reference, ___SCR_NSDate) answers the immutable placeholder; a subclass gets super's +allocWithZone:.
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == (Class)dateClass) {
        return (id)&immutablePlaceholderDateObject;
    }
    return [super allocWithZone:zone];
}

// Native +[NSDate date], 0x1c0280d44. Non-external. objc_alloc of the receiver before the clock is read,
// initialized and autoreleased.
+ (id)date {
    id date = [self alloc];
    return [[date initWithTimeIntervalSinceReferenceDate:currentAbsoluteTime()] autorelease];
}

// Native +[NSDate dateWithTimeIntervalSinceReferenceDate:], 0x1c0280dd8. Non-external.
+ (id)dateWithTimeIntervalSinceReferenceDate:(double)ti {
    return [[[self alloc] initWithTimeIntervalSinceReferenceDate:ti] autorelease];
}

// Native +[NSDate dateWithTimeIntervalSinceNow:], 0x1c0280e00. Non-external. objc_alloc before the clock is
// read; the interval plus the current time.
+ (id)dateWithTimeIntervalSinceNow:(double)secs {
    id date = [self alloc];
    return [[date initWithTimeIntervalSinceReferenceDate:secs + currentAbsoluteTime()] autorelease];
}

// Native -[NSDate initWithTimeIntervalSinceNow:], 0x1c0280ea4. Non-external; the interval plus the current
// time.
- (id)initWithTimeIntervalSinceNow:(double)secs {
    return [self initWithTimeIntervalSinceReferenceDate:secs + currentAbsoluteTime()];
}

// Native +[NSDate dateWithTimeIntervalSince1970:], 0x1c0280f40. Non-external; plus -978307200.0 after the
// allocation.
+ (id)dateWithTimeIntervalSince1970:(double)secs {
    id date = [self alloc];
    return [[date initWithTimeIntervalSinceReferenceDate:secs + -978307200.0] autorelease];
}

// Native -[NSDate initWithTimeIntervalSince1970:], 0x1c0280f70. Non-external; plus -978307200.0 (a tail
// call).
- (id)initWithTimeIntervalSince1970:(double)secs {
    return [self initWithTimeIntervalSinceReferenceDate:secs + -978307200.0];
}

// Native +[NSDate dateWithTimeInterval:sinceDate:], 0x1c0280f80. Non-external. The date's interval (a NaN for
// nil) is read before the allocation.
+ (id)dateWithTimeInterval:(double)secsToBeAdded sinceDate:(id)date {
    double interval = date != nil ? [date timeIntervalSinceReferenceDate] : __builtin_nan("");
    id result = [self alloc];
    return [[result initWithTimeIntervalSinceReferenceDate:secsToBeAdded + interval] autorelease];
}

// Native -[NSDate initWithTimeInterval:sinceDate:], 0x1c0280fd4. Non-external; nil adds a NaN.
- (id)initWithTimeInterval:(double)secsToBeAdded sinceDate:(id)date {
    double interval = date != nil ? [date timeIntervalSinceReferenceDate] : __builtin_nan("");
    return [self initWithTimeIntervalSinceReferenceDate:secsToBeAdded + interval];
}

// Native +[NSDate dateWithDate:], 0x1c028101c. Non-external. The date's interval (a NaN for nil) is read
// before the allocation.
+ (id)dateWithDate:(id)date {
    double interval = date != nil ? [date timeIntervalSinceReferenceDate] : __builtin_nan("");
    id result = [self alloc];
    return [[result initWithTimeIntervalSinceReferenceDate:interval] autorelease];
}

// Native -[NSDate initWithDate:], 0x1c028106c. Non-external; nil gives a NaN.
- (id)initWithDate:(id)date {
    double interval = date != nil ? [date timeIntervalSinceReferenceDate] : __builtin_nan("");
    return [self initWithTimeIntervalSinceReferenceDate:interval];
}

// Native +[NSDate dateWithString:], 0x1c02810a4. Non-external.
+ (id)dateWithString:(id)aString {
    return [[[self alloc] initWithString:aString] autorelease];
}

// Native -[NSDate initWithString:], 0x1c02810cc. Non-external.
// An NSDateComponents (objc_opt_new) receives the year, month, day, hour, minute and second parsed from the
// UTF-8 string: each field is a run of decimal digits (0 when the run is empty), and the parse fails at a NUL
// after any field. The field pointer alternates between two steps exactly as natively: after the year, day
// and minute the next field starts one byte past the terminating byte; after the month, hour and second, two
// bytes past the last digit (or past the separator when the field was empty). When _CFAppVersionCheck
// ("com.apple.ist.Radar", NULL) holds, a string ending in "GMT" fails. The zone is "GMT" at offset 2 from the
// pointer, followed by NUL or a space (offset 0) or by a sign at offset 5; otherwise a sign at offset 2. The
// offset digits after the sign (at most 1600 as a number; none is 0) are hours * 100 + minutes, and the
// offset in seconds is sign * 60 * (hours * 60 + minutes). A Gregorian NSCalendar (kCFGregorianCalendar,
// autoreleased) with the NSTimeZone for that offset converts the components; the receiver is initialized with
// the date's interval. Failure releases the components and the receiver and answers nil; success releases the
// components.
- (id)initWithString:(id)description {
    id components = [(id)dateComponentsClass new];
    const unsigned char *p = (const unsigned char *)[description UTF8String];
    const unsigned char *q;
    NSInteger value;
    unsigned char c;
    NSInteger seconds;
    if (p == NULL) {
        goto failure;
    }
    value = 0;
    c = p[0];
    if ((unsigned)(c - '0') <= 9) {
        do {
            value = value * 10 + (uint8_t)(c - '0');
            c = *++p;
        } while ((unsigned)(c - '0') < 10);
    }
    [components setYear:value];
    if (*p == 0) {
        goto failure;
    }
    q = p + 1;
    c = *q;
    value = 0;
    if ((unsigned)(c - '0') <= 9) {
        do {
            value = value * 10 + (uint8_t)(c - '0');
            c = *++q;
        } while ((unsigned)(c - '0') < 10);
        p = q - 1;
    }
    [components setMonth:value];
    if (*q == 0) {
        goto failure;
    }
    p += 2;
    c = *p;
    value = 0;
    if ((unsigned)(c - '0') <= 9) {
        do {
            value = value * 10 + (uint8_t)(c - '0');
            c = *++p;
        } while ((unsigned)(c - '0') < 10);
    }
    [components setDay:value];
    if (*p == 0) {
        goto failure;
    }
    q = p + 1;
    c = *q;
    value = 0;
    if ((unsigned)(c - '0') <= 9) {
        do {
            value = value * 10 + (uint8_t)(c - '0');
            c = *++q;
        } while ((unsigned)(c - '0') < 10);
        p = q - 1;
    }
    [components setHour:value];
    if (*q == 0) {
        goto failure;
    }
    p += 2;
    c = *p;
    value = 0;
    if ((unsigned)(c - '0') <= 9) {
        do {
            value = value * 10 + (uint8_t)(c - '0');
            c = *++p;
        } while ((unsigned)(c - '0') < 10);
    }
    [components setMinute:value];
    if (*p == 0) {
        goto failure;
    }
    q = p + 1;
    c = *q;
    value = 0;
    if ((unsigned)(c - '0') <= 9) {
        do {
            value = value * 10 + (uint8_t)(c - '0');
            c = *++q;
        } while ((unsigned)(c - '0') < 10);
        p = q - 1;
    }
    [components setSecond:value];
    if (*q == 0) {
        goto failure;
    }
    if (_OCFAppVersionCheck(OF_STR("com.apple.ist.Radar"), NULL) &&
        [description hasSuffix:(id)OF_STR("GMT")]) {
        goto failure;
    }
    {
        unsigned char zone = p[2];
        int difference;
        if (zone == 'G') {
            if (p[3] == 'M') {
                difference = 'T' - p[4];
            } else {
                difference = 'M' - p[3];
            }
        } else {
            difference = 'G' - zone;
        }
        size_t signIndex;
        if (difference == 0) {
            zone = p[5];
            if (zone <= '*') {
                seconds = 0;
                if (zone == 0 || zone == ' ') {
                    goto convert;
                }
                goto failure;
            }
            signIndex = 5;
            if (zone != '-' && zone != '+') {
                goto failure;
            }
        } else {
            signIndex = 2;
        }
        NSInteger sign;
        if (zone == '-') {
            sign = -60;
        } else if (zone == '+') {
            sign = 60;
        } else {
            goto failure;
        }
        NSInteger offset = 0;
        c = p[signIndex + 1];
        if ((unsigned)(c - '0') <= 9) {
            const unsigned char *digits = p + signIndex + 2;
            do {
                offset = offset * 10 + (uint8_t)(c - '0');
                c = *digits++;
            } while ((unsigned)(c - '0') < 10);
            if (offset > 1600) {
                goto failure;
            }
        }
        seconds = sign * (offset / 100 * 60 + offset % 100);
    }
convert:
    {
        id calendar =
            [[[(id)calendarClass alloc] initWithCalendarIdentifier:(id)kOCFGregorianCalendar] autorelease];
        [calendar setTimeZone:[(id)timeZoneClass timeZoneForSecondsFromGMT:seconds]];
        double interval = [[calendar dateFromComponents:components] timeIntervalSinceReferenceDate];
        id result = [self initWithTimeIntervalSinceReferenceDate:interval];
        [components release];
        return result;
    }
failure:
    [components release];
    [self release];
    return nil;
}

// Native -[NSDate dateByAddingTimeInterval:], 0x1c02814b8. Non-external. The receiver's interval, then its
// class (objc_opt_class), then +dateWithTimeIntervalSinceReferenceDate: with the argument plus the interval
// (a tail call).
- (id)dateByAddingTimeInterval:(double)ti {
    double interval = [self timeIntervalSinceReferenceDate];
    Class cls = [self class];
    return [cls dateWithTimeIntervalSinceReferenceDate:ti + interval];
}

// Native -[NSDate addTimeInterval:], 0x1c02814f4. Non-external; a tail call of -dateByAddingTimeInterval:.
- (id)addTimeInterval:(double)seconds {
    return [self dateByAddingTimeInterval:seconds];
}

// Native -[NSDate copyWithZone:], 0x1c02814f8. Non-external; the receiver retained (objc_retain).
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native +[NSDate supportsSecureCoding], 0x1c02814fc. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSDate initWithCoder:], 0x1c0281504. Non-external; nil (the receiver is not released).
- (id)initWithCoder:(id)coder {
    return nil;
}

// Native -[NSDate encodeWithCoder:], 0x1c028150c. Non-external; does nothing.
- (void)encodeWithCoder:(id)coder {
}

// Native -[NSDate isInToday], 0x1c0281510. Non-external; +[NSCalendar currentCalendar] -isDateInToday:.
- (BOOL)isInToday {
    return [[(id)calendarClass currentCalendar] isDateInToday:self];
}

// Native -[NSDate isInTomorrow], 0x1c028153c. Non-external.
- (BOOL)isInTomorrow {
    return [[(id)calendarClass currentCalendar] isDateInTomorrow:self];
}

// Native -[NSDate isInYesterday], 0x1c0281568. Non-external.
- (BOOL)isInYesterday {
    return [[(id)calendarClass currentCalendar] isDateInYesterday:self];
}

// Native -[NSDate isInSameDayAsDate:], 0x1c0281594. Non-external.
- (BOOL)isInSameDayAsDate:(id)date {
    return [[(id)calendarClass currentCalendar] isDate:self inSameDayAsDate:date];
}

// Native -[NSDate isEqual:toUnitGranularity:], 0x1c02815c8. Non-external; whether the current calendar's
// comparison is 0.
- (BOOL)isEqual:(id)date toUnitGranularity:(NSUInteger)unit {
    return [[(id)calendarClass currentCalendar] compareDate:self toDate:date toUnitGranularity:unit] == 0;
}

// Native -[NSDate compare:toUnitGranularity:], 0x1c0281618. Non-external. The receiver itself is the same
// (0); otherwise the current calendar's comparison (a tail call).
- (NSInteger)compare:(id)date toUnitGranularity:(NSUInteger)unit {
    if (self == date) {
        return 0;
    }
    return [[(id)calendarClass currentCalendar] compareDate:self toDate:date toUnitGranularity:unit];
}

// Native -[NSDate timeIntervalSinceReferenceDate], 0x1c030f0a4 (cold region), with OUTLINED_FUNCTION_1_26
// (0x1c0281948: __CFLookUpClass("NSDate")) and _2_22 (0x1c028195c). Non-external. The abstract method:
// __CFRequireConcreteImplementation, then 0.0.
- (double)timeIntervalSinceReferenceDate {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSDate)), self, _cmd);
    return 0.0;
}

// Native -[NSDate initWithTimeIntervalSinceReferenceDate:], 0x1c030f0c8 (cold region), with the same outlined
// fragments. Non-external. The abstract method: __CFRequireConcreteImplementation, then the receiver.
- (id)initWithTimeIntervalSinceReferenceDate:(double)ti {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSDate)), self, _cmd);
    return self;
}

@end
