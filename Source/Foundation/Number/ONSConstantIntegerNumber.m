//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Number/ONSConstantIntegerNumber.h"

#import "Foundation/Number/ONSUnrestoredInterfaces.h"

#import <objc/objc-exception.h>
#include <string.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Foundation/Number/Reading.md, round 2. Manual reference counting.

// Native gCRAnnotations, 0x22e6fc8c8, was a private external, __DATA,__crash_info.
// The native trap paths store the message in its second word (0x22e6fc8d0).
static volatile uint64_t foundationCrashAnnotations[41]
    __attribute__((used, section("__DATA,__crash_info"))) = {7};

extern char constantDecimalNumberClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDecimalNumber));

@implementation ONSConstantIntegerNumber

// Native -[NSConstantIntegerNumber objCType], 0x1c0615af8. Non-external.
- (const char *)objCType {
    return _encoding;
}

// Native -[NSConstantIntegerNumber getValue:], 0x1c0615b08. Non-external.
- (void)getValue:(void *)value {
    size_t expectedSize;
    switch ((unsigned char)_encoding[0]) {
        case 'B':
        case 'C':
        case 'c':
            expectedSize = 1;
            break;
        case 'S':
        case 's':
            expectedSize = 2;
            break;
        case 'I':
        case 'i':
            expectedSize = 4;
            break;
        case 'L':
        case 'Q':
        case 'l':
        case 'q':
            expectedSize = 8;
            break;
        default:
            foundationCrashAnnotations[1] = (uintptr_t)"Should not be possible to have a non supported type "
                                                       "encoding! Please file a radar!";
            __builtin_arm_trap(1);
    }
    memmove(value, &_value, expectedSize);
}

// Native -[NSConstantIntegerNumber getValue:size:], 0x1c0615bd0. Non-external.
// The native exception always reports the encoding as "d" (kept).
- (void)getValue:(void *)value size:(NSUInteger)size {
    size_t expectedSize;
    switch ((unsigned char)_encoding[0]) {
        case 'B':
        case 'C':
        case 'c':
            expectedSize = 1;
            break;
        case 'S':
        case 's':
            expectedSize = 2;
            break;
        case 'I':
        case 'i':
            expectedSize = 4;
            break;
        case 'L':
        case 'Q':
        case 'l':
        case 'q':
            expectedSize = 8;
            break;
        default:
            foundationCrashAnnotations[1] = (uintptr_t)"Should not be possible to have a non supported type "
                                                       "encoding! Please file a radar!";
            __builtin_arm_trap(1);
    }
    if (size != expectedSize) {
        id reason = [ONSString
            stringWithFormat:
                (id)OF_STR(
                    "Cannot get value with size %zu. The type encoded as %s is expected to be %zu bytes"
                ),
                size,
                "d",
                expectedSize];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    memmove(value, &_value, size);
}

// Native -[NSConstantIntegerNumber decimalValue], 0x1c0615cf4. Non-external.
- (ONSDecimal)decimalValue {
    id number = [objc_alloc((Class)constantDecimalNumberClass) initWithLongLong:_value];
    ONSDecimal result = {0};
    if (number != nil) {
        result = [number decimalValue];
    }
    objc_release(number);
    return result;
}

// Native -[NSConstantIntegerNumber boolValue], 0x1c0615d54. Non-external.
- (BOOL)boolValue {
    unsigned int encoding = (unsigned char)_encoding[0] - (unsigned int)'B';
    if (encoding <= 49 && ((UINT64_C(1) << encoding) & UINT64_C(0x2848200028483)) != 0) {
        return _value != 0;
    }
    foundationCrashAnnotations[1] =
        (uintptr_t)"Should not be possible to have a non supported type encoding! Please file a radar!";
    __builtin_arm_trap(1);
}

// Native +[NSConstantIntegerNumber allocWithZone:], 0x1c0615dc4. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return (id)[ONSNumber allocWithZone:zone];
}

// Native -[NSConstantIntegerNumber charValue], 0x1c0615dd0. Non-external.
- (char)charValue {
    return (char)_value;
}

// Native -[NSConstantIntegerNumber unsignedCharValue], 0x1c0615de0. Non-external.
- (unsigned char)unsignedCharValue {
    return (unsigned char)_value;
}

// Native -[NSConstantIntegerNumber shortValue], 0x1c0615df0. Non-external.
- (short)shortValue {
    return (short)_value;
}

// Native -[NSConstantIntegerNumber unsignedShortValue], 0x1c0615e00. Non-external.
- (unsigned short)unsignedShortValue {
    return (unsigned short)_value;
}

// Native -[NSConstantIntegerNumber intValue], 0x1c0615e10. Non-external.
- (int)intValue {
    return (int)_value;
}

// Native -[NSConstantIntegerNumber unsignedIntValue], 0x1c0615e20. Non-external.
- (unsigned int)unsignedIntValue {
    return (unsigned int)_value;
}

// Native -[NSConstantIntegerNumber longValue], 0x1c0615e30. Non-external.
- (long)longValue {
    return (long)_value;
}

// Native -[NSConstantIntegerNumber unsignedLongValue], 0x1c0615e40. Non-external.
- (unsigned long)unsignedLongValue {
    return (unsigned long)_value;
}

// Native -[NSConstantIntegerNumber longLongValue], 0x1c0615e50. Non-external.
- (long long)longLongValue {
    return (long long)_value;
}

// Native -[NSConstantIntegerNumber unsignedLongLongValue], 0x1c0615e60. Non-external.
- (unsigned long long)unsignedLongLongValue {
    return (unsigned long long)_value;
}

// Native -[NSConstantIntegerNumber floatValue], 0x1c0615e70. Non-external.
- (float)floatValue {
    return (float)_value;
}

// Native -[NSConstantIntegerNumber doubleValue], 0x1c0615e84. Non-external.
- (double)doubleValue {
    return (double)_value;
}

// Native -[NSConstantIntegerNumber integerValue], 0x1c0615e98. Non-external.
- (NSInteger)integerValue {
    return (NSInteger)_value;
}

// Native -[NSConstantIntegerNumber unsignedIntegerValue], 0x1c0615ea8. Non-external.
- (NSUInteger)unsignedIntegerValue {
    return (NSUInteger)_value;
}

// Native -[NSConstantIntegerNumber autorelease], 0x1c0615eb8. Non-external.
// The bare RET preserves the receiver in X0.
- (id)autorelease {
    return self;
}

// Native -[NSConstantIntegerNumber retain], 0x1c0615ebc. Non-external.
// The bare RET preserves the receiver in X0.
- (id)retain {
    return self;
}

// Native -[NSConstantIntegerNumber retainCount], 0x1c0615ed0. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

// Native -[NSConstantIntegerNumber release], 0x1c0615ed8. Non-external.
- (oneway void)release {
}

// Native -[NSConstantIntegerNumber copyWithZone:], 0x1c0615edc. Non-external.
// The bare RET preserves the receiver in X0.
- (id)copyWithZone:(struct _NSZone *)zone {
    return self;
}

// Native -[NSConstantIntegerNumber copy], 0x1c0615ee0. Non-external.
// The bare RET preserves the receiver in X0.
- (id)copy {
    return self;
}

// Native +[NSConstantIntegerNumber automaticallyNotifiesObserversForKey:], 0x1c0615ee4. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[NSConstantIntegerNumber init], 0x1c0fe788c. Non-external.
- (id)init {
    __builtin_arm_trap(1);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

// Native -[NSConstantIntegerNumber dealloc], 0x1c0fe7890. Non-external.
- (void)dealloc {
    __builtin_arm_trap(1);
}

#pragma clang diagnostic pop

@end
