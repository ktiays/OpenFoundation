//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFNumber.h"

#include <objc/runtime.h>

// Inferred organization: the __NSCFNumber methods (0x1c01b2fb0-0x1c01b3c3c) lie together in the iOS Simulator
// __text section, between the __NSCFAttributedString and __NSCFBoolean methods; following the owner's
// decision, the class has its own translation unit. Objective-C compiled with manual reference counting
// (objc_autorelease). -compare: (0x1c01b3ad4) and its .cold.1 (0x1c0308e8c) are restored from the IDA capture
// of the fragment (iphone-simulator-corpus-delta2.json). Unless a definition says otherwise: iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device
// 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written
// yet).

// CFNumber types as the native call sites pass them: kCFNumberSInt64Type (4), kCFNumberFloatType (12),
// kCFNumberDoubleType (13) and the private 128-bit type 17 (kCFNumberSInt128Type), whose value is a high
// signed word followed by a low unsigned word. SInt128Value is a local descriptive spelling.
enum {
    kNumberSInt128Type = 17,
};

typedef struct {
    int64_t high;
    uint64_t low;
} SInt128Value;

// Native off_2227A26D0 (__DATA_CONST,__const): the type encodings by CFNumber type minus one; only types 1-6
// and 17 have entries.
static const char *const numberTypeEncodings[17] = {
    "c",
    "s",
    "i",
    "q",
    "f",
    "d",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "Q",
};

@implementation __ONSCFNumber

// Native +[__NSCFNumber allocWithZone:], 0x1c01b2fb0. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native -[__NSCFNumber isNSNumber__], 0x1c01b2fcc. Non-external.
- (BOOL)isNSNumber__ {
    return YES;
}

// Native -[__NSCFNumber isEqualToNumber:], 0x1c01b2fd4. Non-external. The receiver itself is equal; otherwise
// -compare: must answer NSOrderedSame.
- (BOOL)isEqualToNumber:(id)number {
    if (self == number) {
        return YES;
    }
    return [self compare:number] == 0;
}

// Native -[__NSCFNumber isEqual:], 0x1c01b3000. Non-external. nil answers NO, the receiver YES, a non-number
// NO; any other number goes to -isEqualToNumber:.
- (BOOL)isEqual:(id)object {
    if (object == nil) {
        return NO;
    }
    if (self == object) {
        return YES;
    }
    if (!_ONSIsNSNumber(object)) {
        return NO;
    }
    return [self isEqualToNumber:object];
}

// Native -[__NSCFNumber hash], 0x1c01b3058. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[__NSCFNumber retain], 0x1c01b305c. Non-external. A tagged receiver (bit 63) is answered as is.
- (id)retain {
    if ((intptr_t)self < 0) {
        return self;
    }
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFNumber release], 0x1c01b3068. Non-external. A tagged receiver is left alone.
- (oneway void)release {
    if ((intptr_t)self < 0) {
        return;
    }
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFNumber _tryRetain], 0x1c01b3074. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFNumber _isDeallocating], 0x1c01b3090. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFNumber retainCount], 0x1c01b30ac. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native +[__NSCFNumber automaticallyNotifiesObserversForKey:], 0x1c01b30b0. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSCFNumber descriptionWithLocale:], 0x1c01b30b8. Non-external. With the NSNumber class present
// super answers; otherwise CF's formatting description, autoreleased.
- (id)descriptionWithLocale:(id)locale {
    if (objc_lookUpClass(OF_NAME_STRING(ONSNumber)) != Nil) {
        return [super descriptionWithLocale:locale];
    }
    return [(id)__CFNumberCopyFormattingDescription(self, locale) autorelease];
}

// Native -[__NSCFNumber description], 0x1c01b3170. Non-external. With the NSNumber class present super
// answers; otherwise -descriptionWithLocale: with nil.
- (id)description {
    if (objc_lookUpClass(OF_NAME_STRING(ONSNumber)) != Nil) {
        return [super description];
    }
    return [self descriptionWithLocale:nil];
}

// Native -[__NSCFNumber stringValue], 0x1c01b321c. Non-external. With the NSNumber class present super
// answers; otherwise CF's formatting description in the system default allocator, autoreleased.
- (id)stringValue {
    if (objc_lookUpClass(OF_NAME_STRING(ONSNumber)) != Nil) {
        return [super stringValue];
    }
    return [(id)__CFNumberCreateFormattingDescription(kOCFAllocatorSystemDefault, self, NULL) autorelease];
}

// Native -[__NSCFNumber copyWithZone:], 0x1c01b32d8. Non-external. A tagged receiver is answered as is.
- (id)copyWithZone:(struct _NSZone *)zone {
    if ((intptr_t)self < 0) {
        return self;
    }
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFNumber getValue:], 0x1c01b32e4. Non-external. A tagged receiver stores its value in its
// CFNumberGetType type. Otherwise the storage type decides: a 128-bit value stores its low word when the high
// word is zero and all ones otherwise; any other type stores the value in that type.
- (void)getValue:(void *)value {
    if ((intptr_t)self < 0) {
        OCFNumberType type = OCFNumberGetType((OCFNumberRef)self);
        OCFNumberGetValue((OCFNumberRef)self, type, value);
        return;
    }
    OCFNumberType type = _OCFNumberGetType2((OCFNumberRef)self);
    if (type == kNumberSInt128Type) {
        SInt128Value wide = {0, 0};
        OCFNumberGetValue((OCFNumberRef)self, kNumberSInt128Type, &wide);
        *(uint64_t *)value = wide.high == 0 ? wide.low : UINT64_MAX;
        return;
    }
    OCFNumberGetValue((OCFNumberRef)self, type, value);
}

// Native -[__NSCFNumber objCType], 0x1c01b33c8. Non-external. The encoding of the storage type; a type
// without an entry executes BRK #1.
- (const char *)objCType {
    OCFNumberType type = _OCFNumberGetType2((OCFNumberRef)self);
    uintptr_t index = (uintptr_t)type - 1;
    if (index > 16 || ((0x1003Fu >> index) & 1) == 0) {
        __builtin_trap();
    }
    return numberTypeEncodings[index];
}

// Native -[__NSCFNumber charValue], 0x1c01b3408. Non-external. The value as a 64-bit integer, truncated.
- (char)charValue {
    int64_t value = 0;
    OCFNumberGetValue((OCFNumberRef)self, kOCFNumberSInt64Type, &value);
    return (char)value;
}

// Native -[__NSCFNumber unsignedCharValue], 0x1c01b3460. Non-external.
- (unsigned char)unsignedCharValue {
    int64_t value = 0;
    OCFNumberGetValue((OCFNumberRef)self, kOCFNumberSInt64Type, &value);
    return (unsigned char)value;
}

// Native -[__NSCFNumber shortValue], 0x1c01b34b8. Non-external.
- (short)shortValue {
    int64_t value = 0;
    OCFNumberGetValue((OCFNumberRef)self, kOCFNumberSInt64Type, &value);
    return (short)value;
}

// Native -[__NSCFNumber unsignedShortValue], 0x1c01b3510. Non-external.
- (unsigned short)unsignedShortValue {
    int64_t value = 0;
    OCFNumberGetValue((OCFNumberRef)self, kOCFNumberSInt64Type, &value);
    return (unsigned short)value;
}

// Native -[__NSCFNumber intValue], 0x1c01b3568. Non-external.
- (int)intValue {
    int64_t value = 0;
    OCFNumberGetValue((OCFNumberRef)self, kOCFNumberSInt64Type, &value);
    return (int)value;
}

// Native -[__NSCFNumber unsignedIntValue], 0x1c01b35c0. Non-external.
- (unsigned int)unsignedIntValue {
    int64_t value = 0;
    OCFNumberGetValue((OCFNumberRef)self, kOCFNumberSInt64Type, &value);
    return (unsigned int)value;
}

// Native -[__NSCFNumber longValue], 0x1c01b3618. Non-external.
- (long)longValue {
    int64_t value = 0;
    OCFNumberGetValue((OCFNumberRef)self, kOCFNumberSInt64Type, &value);
    return (long)value;
}

// Native -[__NSCFNumber unsignedLongValue], 0x1c01b3670. Non-external. The value as a 128-bit integer; the
// low word is answered.
- (unsigned long)unsignedLongValue {
    SInt128Value value = {0, 0};
    OCFNumberGetValue((OCFNumberRef)self, kNumberSInt128Type, &value);
    return (unsigned long)value.low;
}

// Native -[__NSCFNumber longLongValue], 0x1c01b36c8. Non-external.
- (long long)longLongValue {
    int64_t value = 0;
    OCFNumberGetValue((OCFNumberRef)self, kOCFNumberSInt64Type, &value);
    return (long long)value;
}

// Native -[__NSCFNumber unsignedLongLongValue], 0x1c01b3720. Non-external. The low word of the 128-bit value.
- (unsigned long long)unsignedLongLongValue {
    SInt128Value value = {0, 0};
    OCFNumberGetValue((OCFNumberRef)self, kNumberSInt128Type, &value);
    return (unsigned long long)value.low;
}

// Native -[__NSCFNumber floatValue], 0x1c01b3778. Non-external.
- (float)floatValue {
    float value = 0;
    OCFNumberGetValue((OCFNumberRef)self, kOCFNumberFloatType, &value);
    return value;
}

// Native -[__NSCFNumber doubleValue], 0x1c01b37d0. Non-external.
- (double)doubleValue {
    double value = 0;
    OCFNumberGetValue((OCFNumberRef)self, kOCFNumberDoubleType, &value);
    return value;
}

// Native -[__NSCFNumber integerValue], 0x1c01b3828. Non-external. A tail message.
- (NSInteger)integerValue {
    return [self longValue];
}

// Native -[__NSCFNumber unsignedIntegerValue], 0x1c01b382c. Non-external. A tail message.
- (NSUInteger)unsignedIntegerValue {
    return [self unsignedLongValue];
}

// Native -[__NSCFNumber boolValue], 0x1c01b3830. Non-external. A floating-point number is true when its
// double value is not 0.0 (NaN is true); any other number when its 64-bit value is not 0.
- (BOOL)boolValue {
    if (OCFNumberIsFloatType((OCFNumberRef)self)) {
        double value = 0;
        OCFNumberGetValue((OCFNumberRef)self, kOCFNumberDoubleType, &value);
        return value != 0.0;
    }
    int64_t value = 0;
    OCFNumberGetValue((OCFNumberRef)self, kOCFNumberSInt64Type, &value);
    return value != 0;
}

// Native -[__NSCFNumber _cfTypeID], 0x1c01b38c8. Non-external. The CFNumber type ID, 22.
- (OCFTypeID)_cfTypeID {
    return 0x16;
}

// Native -[__NSCFNumber _cfNumberType], 0x1c01b38d0. Non-external. The CFNumber type for the first character
// of -objCType: c and B kCFNumberCharType (7), C and s kCFNumberShortType (8), S and i kCFNumberIntType (9),
// l kCFNumberLongType (10), q kCFNumberLongLongType (11), f kCFNumberFloatType (12), d kCFNumberDoubleType
// (13), Q the 128-bit type (17); I is 9 when -unsignedIntValue fits a signed int, else 11; L is 10 when
// -unsignedLongValue fits a signed long, else 11. Any other character executes BRK #1.
- (OCFNumberType)_cfNumberType {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return kOCFNumberCharType;
        case 'C':
        case 's':
            return kOCFNumberShortType;
        case 'S':
        case 'i':
            return kOCFNumberIntType;
        case 'I':
            return (int)[self unsignedIntValue] > -1 ? kOCFNumberIntType : kOCFNumberLongLongType;
        case 'l':
            return kOCFNumberLongType;
        case 'L':
            return (long)[self unsignedLongValue] <= -1 ? kOCFNumberLongLongType : kOCFNumberLongType;
        case 'q':
            return kOCFNumberLongLongType;
        case 'Q':
            return (OCFNumberType)kNumberSInt128Type;
        case 'f':
            return kOCFNumberFloatType;
        case 'd':
            return kOCFNumberDoubleType;
        default:
            __builtin_trap();
    }
}

// Native -[__NSCFNumber _getValue:forType:], 0x1c01b39f0. Non-external. Stores the value through the matching
// accessor for types 1-6; type 17 stores the 128-bit value of -longLongValue (sign word, then the value). Any
// other type executes BRK #1. Answers YES.
- (BOOL)_getValue:(void *)value forType:(OCFNumberType)type {
    switch ((OCFIndex)type) {
        case kOCFNumberSInt8Type:
            *(char *)value = [self charValue];
            break;
        case kOCFNumberSInt16Type:
            *(short *)value = [self shortValue];
            break;
        case kOCFNumberSInt32Type:
            *(int *)value = [self intValue];
            break;
        case kOCFNumberSInt64Type:
            *(long long *)value = [self longLongValue];
            break;
        case kOCFNumberFloat32Type:
            *(float *)value = [self floatValue];
            break;
        case kOCFNumberFloat64Type:
            *(double *)value = [self doubleValue];
            break;
        case kNumberSInt128Type: {
            long long low = [self longLongValue];
            ((SInt128Value *)value)->high = low >> 63;
            ((SInt128Value *)value)->low = (uint64_t)low;
            break;
        }
        default:
            __builtin_trap();
    }
    return YES;
}

// Native -[__NSCFNumber _reverseCompare:], 0x1c01b3ab0. Non-external. -compare:'s result negated
// (NSOrderedAscending and NSOrderedDescending swap; anything else is NSOrderedSame).
- (NSInteger)_reverseCompare:(id)other {
    NSInteger result = [self compare:other];
    if (result == -1) {
        return 1;
    }
    return result == 1 ? -1 : 0;
}

// Native -[__NSCFNumber compare:], 0x1c01b3ad4, with its .cold.1 (0x1c0308e8c). Non-external. numberCompare
// in Objective-C/OCFBridgedClasses.h.
- (NSInteger)compare:(id)other {
    return numberCompare(self, _cmd, other);
}

@end
