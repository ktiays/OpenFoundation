//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSTimeZone.h"

#import "CoreFoundation/Base/OCFUtilities.h"
#import "CoreFoundation/Date/OCFTimeZoneEntryPoints.h"
#import "CoreFoundation/Objective-C/OCFNSDate.h"
#import "CoreFoundation/Objective-C/OCFNSLocale.h"
#import "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#import "CoreFoundation/Objective-C/OCFRequireConcreteImplementation.h"
#import "CoreFoundation/String/OCFString.h"

// Inferred organization: the NSTimeZone methods of this image (-daylightSavingTimeOffsetForDate: 0x1c02aafe4
// through -encodeWithCoder: 0x1c02ab43c, with the outlined fragments OUTLINED_FUNCTION_0_48, _1_34 and _2_30
// at 0x1c02ab48c-0x1c02ab4ac and the abstract methods in the cold region at 0x1c03103a4-0x1c0310468) lie
// together in the iOS Simulator __text section after the __NSPlaceholderTimeZone methods (0x1c02aabb0-,
// outside the evidence); following the owner's decision, the class has its own translation unit. Objective-C
// compiled with manual reference counting (objc_retain, objc_autorelease). The native source path is not in
// the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; constant strings
// resolved from the ADRP/ADD encodings and constants read with Local/Scratch/cf-core/macho.py); iOS Device
// 24A446: unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.
//
// The super send of +allocWithZone: passes the selector address 0x2387a9fd4. The abstract methods share the
// outlined fragments OUTLINED_FUNCTION_0_48 (__CFLookUpClass("NSTimeZone"), saving the receiver and
// selector), _1_34 (__CFRequireConcreteImplementation) and _2_30 (the epilogue), restored inline in each
// method.

// The NSTimeZone class reference (off_2227CE3D8, and the self class reference ___SCR_NSTimeZone) and the
// placeholder's class (__NSPlaceholderTimeZone, restored in Objective-C/OCFNSPlaceholderTimeZone.m).
extern char timeZoneClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSTimeZone));
extern char placeholderTimeZoneClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSPlaceholderTimeZone));

// Class methods this unit sends through the NSTimeZone, NSLocale and NSDate class references that their
// classes' interfaces do not declare (NSTimeZone's private class methods are implemented outside this
// image); declarations only, no category is emitted.
@interface ONSTimeZone (TimeZonePrivateMessages)

+ (id)_current;
+ (id)_default;
+ (void)_setDefaultTimeZone:(id)aTimeZone;
+ (id)_autoupdating;
+ (void)_resetSystemTimeZone;
+ (id)_knownTimeZoneIdentifiers;
+ (id)_abbreviationDictionary;
+ (void)_setAbbreviationDictionary:(id)dict;
+ (id)_timeZoneDataVersion;
+ (id)_timeZoneWithAbbreviation:(id)abbreviation;
+ (id)_timeZoneWithSecondsFromGMT:(NSInteger)seconds;

@end

@interface ONSLocale (TimeZoneLocaleMessages)

+ (void)_resetCurrent;

@end

@interface ONSDate (TimeZoneDateMessages)

+ (id)date;

@end

// Native __immutablePlaceholderTimeZone, 0x2227a8820 (__DATA_CONST,__const, 16 bytes; read with
// Local/Scratch/cf-core/macho.py). Non-external: an object whose isa is __NSPlaceholderTimeZone, then a zero
// word; the result of +[NSTimeZone allocWithZone:] for NSTimeZone itself.
static const struct {
    const void *isa;
    uintptr_t unused;
} __immutablePlaceholderTimeZone = {placeholderTimeZoneClass, 0};

@implementation ONSTimeZone

// Native -[NSTimeZone daylightSavingTimeOffsetForDate:], 0x1c02aafe4. Non-external; 0.0.
- (double)daylightSavingTimeOffsetForDate:(id)date {
    return 0.0;
}

// Native -[NSTimeZone nextDaylightSavingTimeTransitionAfterDate:], 0x1c02aafec. Non-external; nil.
- (id)nextDaylightSavingTimeTransitionAfterDate:(id)date {
    return nil;
}

// Native -[NSTimeZone isNSTimeZone__], 0x1c02aaff4. Non-external; YES.
- (BOOL)isNSTimeZone__ {
    return YES;
}

// Native -[NSTimeZone _cfTypeID], 0x1c02aaffc. Non-external; a tail call of CFTimeZoneGetTypeID.
- (OCFTypeID)_cfTypeID {
    return OCFTimeZoneGetTypeID();
}

// Native +[NSTimeZone systemTimeZone], 0x1c02ab000. Non-external; a tail call of +[NSTimeZone _current] (the
// class reference, not the receiver; the class methods below send to it the same way).
+ (id)systemTimeZone {
    return [ONSTimeZone _current];
}

// Native +[NSTimeZone resetSystemTimeZone], 0x1c02ab014. Non-external. +[NSLocale _resetCurrent], then
// +[NSTimeZone _resetSystemTimeZone] (a tail call).
+ (void)resetSystemTimeZone {
    [ONSLocale _resetCurrent];
    [ONSTimeZone _resetSystemTimeZone];
}

// Native +[NSTimeZone defaultTimeZone], 0x1c02ab038. Non-external; +[NSTimeZone _default].
+ (id)defaultTimeZone {
    return [ONSTimeZone _default];
}

// Native +[NSTimeZone setDefaultTimeZone:], 0x1c02ab044. Non-external; +[NSTimeZone _setDefaultTimeZone:].
+ (void)setDefaultTimeZone:(id)aTimeZone {
    [ONSTimeZone _setDefaultTimeZone:aTimeZone];
}

// Native +[NSTimeZone localTimeZone], 0x1c02ab050. Non-external.
// +[NSTimeZone _autoupdating], retained once more (objc_retain, the result is still returned unowned) when
// _CFAppVersionCheckLessThan("com.ea.simpsonssocial.inc2", 0xffffffff, 4.7) holds (qword_1C046C968, bits
// 0x4012cccccccccccd): a compatibility leak for that application.
+ (id)localTimeZone {
    id timeZone = [ONSTimeZone _autoupdating];
    if (_OCFAppVersionCheckLessThan(OF_STR("com.ea.simpsonssocial.inc2"), 0xffffffff, 4.7)) {
        [timeZone retain];
    }
    return timeZone;
}

// Native +[NSTimeZone knownTimeZoneNames], 0x1c02ab09c. Non-external; +[NSTimeZone
// _knownTimeZoneIdentifiers].
+ (id)knownTimeZoneNames {
    return [ONSTimeZone _knownTimeZoneIdentifiers];
}

// Native +[NSTimeZone abbreviationDictionary], 0x1c02ab0a8. Non-external; +[NSTimeZone
// _abbreviationDictionary].
+ (id)abbreviationDictionary {
    return [ONSTimeZone _abbreviationDictionary];
}

// Native +[NSTimeZone setAbbreviationDictionary:], 0x1c02ab0b4. Non-external; +[NSTimeZone
// _setAbbreviationDictionary:].
+ (void)setAbbreviationDictionary:(id)dict {
    [ONSTimeZone _setAbbreviationDictionary:dict];
}

// Native +[NSTimeZone timeZoneDataVersion], 0x1c02ab0c0. Non-external; +[NSTimeZone _timeZoneDataVersion].
+ (id)timeZoneDataVersion {
    return [ONSTimeZone _timeZoneDataVersion];
}

// Native -[NSTimeZone secondsFromGMT], 0x1c02ab0cc. Non-external; -secondsFromGMTForDate: with +[NSDate date]
// (a tail call). The four methods below follow the same pattern.
- (NSInteger)secondsFromGMT {
    return [self secondsFromGMTForDate:[ONSDate date]];
}

// Native -[NSTimeZone abbreviation], 0x1c02ab0fc. Non-external; -abbreviationForDate: with now.
- (id)abbreviation {
    return [self abbreviationForDate:[ONSDate date]];
}

// Native -[NSTimeZone isDaylightSavingTime], 0x1c02ab12c. Non-external; -isDaylightSavingTimeForDate: with
// now.
- (BOOL)isDaylightSavingTime {
    return [self isDaylightSavingTimeForDate:[ONSDate date]];
}

// Native -[NSTimeZone daylightSavingTimeOffset], 0x1c02ab15c. Non-external; -daylightSavingTimeOffsetForDate:
// with now.
- (double)daylightSavingTimeOffset {
    return [self daylightSavingTimeOffsetForDate:[ONSDate date]];
}

// Native -[NSTimeZone nextDaylightSavingTimeTransition], 0x1c02ab18c. Non-external;
// -nextDaylightSavingTimeTransitionAfterDate: with now.
- (id)nextDaylightSavingTimeTransition {
    return [self nextDaylightSavingTimeTransitionAfterDate:[ONSDate date]];
}

// Native -[NSTimeZone localizedName:locale:], 0x1c02ab1bc. Non-external; nil. NSTimeZoneNameStyle is spelled
// NSInteger.
- (id)localizedName:(NSInteger)style locale:(id)locale {
    return nil;
}

// Native -[NSTimeZone hash], 0x1c02ab1c4. Non-external; CFHash of the name (a tail call).
- (NSUInteger)hash {
    return OCFHash((OCFTypeRef)[self name]);
}

// Native -[NSTimeZone isEqual:], 0x1c02ab1d8. Non-external. The receiver itself is equal and nil is not; an
// object _NSIsNSTimeZone rejects is not; otherwise -isEqualToTimeZone: (a tail call).
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    if (!_ONSIsNSTimeZone(object)) {
        return NO;
    }
    return [self isEqualToTimeZone:object];
}

// Native -[NSTimeZone isEqualToTimeZone:], 0x1c02ab230. Non-external. nil is not equal; the receiver itself
// is; otherwise both names (the receiver's first) must be present and CFEqual.
- (BOOL)isEqualToTimeZone:(id)aTimeZone {
    if (aTimeZone == nil) {
        return NO;
    }
    if (aTimeZone == self) {
        return YES;
    }
    id name = [self name];
    id otherName = [aTimeZone name];
    if (name == nil || otherName == nil) {
        return NO;
    }
    return OCFEqual((OCFTypeRef)name, (OCFTypeRef)otherName) ? YES : NO;
}

// Native -[NSTimeZone description], 0x1c02ab298. Non-external.
// "%@ (%@) offset %ld%s" (the constant string 0x2227b9480) with the name, abbreviation, -secondsFromGMT and
// " (Daylight)" or "" by -isDaylightSavingTime, in that order of messages, a new string (system default
// allocator) added to the current autorelease pool (_CFAutoreleasePoolAddObject with NULL, a tail call).
- (id)description {
    id name = [self name];
    id abbreviation = [self abbreviation];
    NSInteger secondsFromGMT = [self secondsFromGMT];
    const char *daylight = [self isDaylightSavingTime] ? " (Daylight)" : "";
    OCFStringRef description = OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault,
        NULL,
        OF_STR("%@ (%@) offset %ld%s"),
        name,
        abbreviation,
        secondsFromGMT,
        daylight
    );
    return _OCFAutoreleasePoolAddObject(NULL, description);
}

// Native +[NSTimeZone allocWithZone:], 0x1c02ab330. Non-external. NSTimeZone itself (its self class
// reference) answers the immutable placeholder; a subclass gets super's +allocWithZone:.
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == (Class)timeZoneClass) {
        return (id)&__immutablePlaceholderTimeZone;
    }
    return [super allocWithZone:zone];
}

// Native +[NSTimeZone timeZoneWithName:data:], 0x1c02ab3ac. Non-external. objc_alloc of the receiver,
// -initWithName:data:, autoreleased (a tail call).
+ (id)timeZoneWithName:(id)tzName data:(id)aData {
    return [[[self alloc] initWithName:tzName data:aData] autorelease];
}

// Native +[NSTimeZone timeZoneWithName:], 0x1c02ab3dc. Non-external. objc_alloc of the receiver,
// -initWithName:, autoreleased (a tail call).
+ (id)timeZoneWithName:(id)tzName {
    return [[[self alloc] initWithName:tzName] autorelease];
}

// Native +[NSTimeZone timeZoneWithAbbreviation:], 0x1c02ab404. Non-external. nil for nil; otherwise
// +[NSTimeZone _timeZoneWithAbbreviation:] (a tail call).
+ (id)timeZoneWithAbbreviation:(id)abbreviation {
    if (abbreviation == nil) {
        return nil;
    }
    return [ONSTimeZone _timeZoneWithAbbreviation:abbreviation];
}

// Native +[NSTimeZone timeZoneForSecondsFromGMT:], 0x1c02ab41c. Non-external; +[NSTimeZone
// _timeZoneWithSecondsFromGMT:] (a tail call).
+ (id)timeZoneForSecondsFromGMT:(NSInteger)seconds {
    return [ONSTimeZone _timeZoneWithSecondsFromGMT:seconds];
}

// Native -[NSTimeZone copyWithZone:], 0x1c02ab428. Non-external; the receiver retained (objc_retain, a tail
// call).
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native +[NSTimeZone supportsSecureCoding], 0x1c02ab42c. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSTimeZone initWithCoder:], 0x1c02ab434. Non-external; nil (the receiver is not released).
- (id)initWithCoder:(id)coder {
    return nil;
}

// Native -[NSTimeZone encodeWithCoder:], 0x1c02ab43c. Non-external; does nothing.
- (void)encodeWithCoder:(id)coder {
}

// Native -[NSTimeZone name], 0x1c03103a4 (cold region). Non-external. The abstract method:
// __CFRequireConcreteImplementation of NSTimeZone (looked up by name), then nil.
// Folds the compiler-outlined pieces OUTLINED_FUNCTION_1_34 (0x1c02ab4a0).
- (id)name {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSTimeZone)), self, _cmd);
    return nil;
}

// Native -[NSTimeZone data], 0x1c03103c4 (cold region). Non-external. The abstract method, then nil.
- (id)data {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSTimeZone)), self, _cmd);
    return nil;
}

// Native -[NSTimeZone secondsFromGMTForDate:], 0x1c03103e4 (cold region). Non-external. The abstract method,
// then 0.
- (NSInteger)secondsFromGMTForDate:(id)aDate {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSTimeZone)), self, _cmd);
    return 0;
}

// Native -[NSTimeZone abbreviationForDate:], 0x1c0310404 (cold region). Non-external. The abstract method,
// then nil.
- (id)abbreviationForDate:(id)aDate {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSTimeZone)), self, _cmd);
    return nil;
}

// Native -[NSTimeZone isDaylightSavingTimeForDate:], 0x1c0310424 (cold region). Non-external. The abstract
// method, then NO.
- (BOOL)isDaylightSavingTimeForDate:(id)aDate {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSTimeZone)), self, _cmd);
    return NO;
}

// Native -[NSTimeZone initWithName:data:], 0x1c0310448 (cold region). Non-external. The abstract method, then
// nil (the receiver is not released).
- (id)initWithName:(id)tzName data:(id)aData {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSTimeZone)), self, _cmd);
    return nil;
}

// Native -[NSTimeZone initWithName:], 0x1c0310468 (cold region). Non-external. The abstract method, then nil.
- (id)initWithName:(id)tzName {
    __CFRequireConcreteImplementation((Class)__CFLookUpClass(OF_NAME_STRING(ONSTimeZone)), self, _cmd);
    return nil;
}

@end
