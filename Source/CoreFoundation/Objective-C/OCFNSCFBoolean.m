//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFBoolean.h"

// Inferred organization: the __NSCFBoolean methods (0x1c01b3c40-0x1c01b42b4) lie together in the iOS
// Simulator __text section, after the __NSCFNumber methods; -_cfNumberType, -_getValue:forType:,
// -_reverseCompare: and -compare: repeat __NSCFNumber's bodies instruction for instruction. Following the
// owner's decision, the class has its own translation unit. Objective-C, manual reference counting (the
// bridged-class units'). -compare: (0x1c01b414c) and its .cold.1 (0x1c0308ea4) are restored from the IDA
// capture of the fragment (iphone-simulator-corpus-delta2.json). Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full).
// iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not
// written yet).

// The private 128-bit CFNumber type (17), a high signed word followed by a low unsigned word, as
// -_cfNumberType and -_getValue:forType: use it. SInt128Value is a local descriptive spelling.
enum {
    kNumberSInt128Type = 17,
};

typedef struct {
    int64_t high;
    uint64_t low;
} SInt128Value;

@implementation __ONSCFBoolean

// Native +[__NSCFBoolean allocWithZone:], 0x1c01b3c40. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native -[__NSCFBoolean isEqualToNumber:], 0x1c01b3c5c. Non-external. The receiver itself is equal;
// otherwise -compare: must answer NSOrderedSame.
- (BOOL)isEqualToNumber:(id)number {
    if (self == number) {
        return YES;
    }
    return [self compare:number] == 0;
}

// Native -[__NSCFBoolean isEqual:], 0x1c01b3c88. Non-external. nil answers NO, the receiver YES, a non-number
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

// Native -[__NSCFBoolean hash], 0x1c01b3ce0. Non-external. CFHash, not _CFNonObjCHash.
- (NSUInteger)hash {
    return OCFHash(self);
}

// Native -[__NSCFBoolean retain], 0x1c01b3ce4. Non-external. A tagged receiver (bit 63) is answered as is.
- (id)retain {
    if ((intptr_t)self < 0) {
        return self;
    }
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFBoolean release], 0x1c01b3cf0. Non-external. A tagged receiver is left alone.
- (oneway void)release {
    if ((intptr_t)self < 0) {
        return;
    }
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFBoolean _tryRetain], 0x1c01b3cfc. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFBoolean _isDeallocating], 0x1c01b3d18. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFBoolean retainCount], 0x1c01b3d34. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native +[__NSCFBoolean automaticallyNotifiesObserversForKey:], 0x1c01b3d38. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSCFBoolean descriptionWithLocale:], 0x1c01b3d40. Non-external. "1" or "0" by -boolValue.
- (id)descriptionWithLocale:(id)locale {
    if ([self boolValue]) {
        return (id)OF_STR("1");
    }
    return (id)OF_STR("0");
}

// Native -[__NSCFBoolean description], 0x1c01b3d6c. Non-external. A tail message with nil.
- (id)description {
    return [self descriptionWithLocale:nil];
}

// Native -[__NSCFBoolean stringValue], 0x1c01b3d74. Non-external. A tail message with nil.
- (id)stringValue {
    return [self descriptionWithLocale:nil];
}

// Native -[__NSCFBoolean charValue], 0x1c01b3d7c. Non-external.
- (char)charValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0;
}

// Native -[__NSCFBoolean unsignedCharValue], 0x1c01b3d98. Non-external.
- (unsigned char)unsignedCharValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0;
}

// Native -[__NSCFBoolean shortValue], 0x1c01b3db4. Non-external.
- (short)shortValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0;
}

// Native -[__NSCFBoolean unsignedShortValue], 0x1c01b3dd0. Non-external.
- (unsigned short)unsignedShortValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0;
}

// Native -[__NSCFBoolean intValue], 0x1c01b3dec. Non-external.
- (int)intValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0;
}

// Native -[__NSCFBoolean unsignedIntValue], 0x1c01b3e08. Non-external.
- (unsigned int)unsignedIntValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0;
}

// Native -[__NSCFBoolean longValue], 0x1c01b3e24. Non-external.
- (long)longValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0;
}

// Native -[__NSCFBoolean unsignedLongValue], 0x1c01b3e40. Non-external.
- (unsigned long)unsignedLongValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0;
}

// Native -[__NSCFBoolean longLongValue], 0x1c01b3e5c. Non-external.
- (long long)longLongValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0;
}

// Native -[__NSCFBoolean unsignedLongLongValue], 0x1c01b3e78. Non-external.
- (unsigned long long)unsignedLongLongValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0;
}

// Native -[__NSCFBoolean floatValue], 0x1c01b3e94. Non-external. 1.0 or 0.0.
- (float)floatValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0 ? 1.0f : 0.0f;
}

// Native -[__NSCFBoolean doubleValue], 0x1c01b3eb8. Non-external. 1.0 or 0.0.
- (double)doubleValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0 ? 1.0 : 0.0;
}

// Native -[__NSCFBoolean integerValue], 0x1c01b3edc. Non-external. A tail message.
- (NSInteger)integerValue {
    return [self longValue];
}

// Native -[__NSCFBoolean unsignedIntegerValue], 0x1c01b3ee0. Non-external. A tail message.
- (NSUInteger)unsignedIntegerValue {
    return [self unsignedLongValue];
}

// Native -[__NSCFBoolean boolValue], 0x1c01b3ee4. Non-external.
- (BOOL)boolValue {
    return OCFBooleanGetValue((OCFBooleanRef)self) != 0;
}

// Native -[__NSCFBoolean _cfTypeID], 0x1c01b3f00. Non-external.
- (OCFTypeID)_cfTypeID {
    return OCFBooleanGetTypeID();
}

// Native -[__NSCFBoolean getValue:], 0x1c01b3f04. Non-external. Stores one byte, 1 or 0.
- (void)getValue:(void *)value {
    *(uint8_t *)value = OCFBooleanGetValue((OCFBooleanRef)self) != 0;
}

// Native -[__NSCFBoolean objCType], 0x1c01b3f30. Non-external.
- (const char *)objCType {
    return "c";
}

// Native -[__NSCFBoolean copyWithZone:], 0x1c01b3f3c. Non-external. A tagged receiver is answered as is.
- (id)copyWithZone:(struct _NSZone *)zone {
    if ((intptr_t)self < 0) {
        return self;
    }
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFBoolean _cfNumberType], 0x1c01b3f48. Non-external. The same body as
// -[__NSCFNumber _cfNumberType]: the CFNumber type for the first character of -objCType (c and B 7, C and s
// 8, S and i 9, I 9 or 11 by the sign of -unsignedIntValue as an int, l 10, L 10 or 11 by the sign of
// -unsignedLongValue as a long, q 11, Q 17, f 12, d 13); any other character executes BRK #1.
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

// Native -[__NSCFBoolean _getValue:forType:], 0x1c01b4068. Non-external. The same body as
// -[__NSCFNumber _getValue:forType:].
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

// Native -[__NSCFBoolean _reverseCompare:], 0x1c01b4128. Non-external. -compare:'s result negated.
- (NSInteger)_reverseCompare:(id)other {
    NSInteger result = [self compare:other];
    if (result == -1) {
        return 1;
    }
    return result == 1 ? -1 : 0;
}

// Native -[__NSCFBoolean compare:], 0x1c01b414c, with its .cold.1 (0x1c0308ea4). Non-external. numberCompare
// in Objective-C/OCFBridgedClasses.h.
- (NSInteger)compare:(id)other {
    return numberCompare(self, _cmd, other);
}

@end
