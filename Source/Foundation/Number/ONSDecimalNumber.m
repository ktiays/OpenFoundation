//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Number/ONSDecimalNumber.h"

#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Decimal/ONSDecimalInternal.h"
#import "Foundation/Number/ONSDecimalNumberHandler.h"
#import "Foundation/Number/ONSValueGeometry.h"
#import "Foundation/Number/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSStringOtherEncodings.h"
#import "Foundation/Thread/ONSThread.h"

#include <math.h>
#include <string.h>
#include <objc/objc-exception.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified except the exception GOT bindings (medium). iOS Device divergence:
// iOS Device 24A446 remains unread. Reading: Foundation/Number/Reading.md, round 2.
// The Objective-C units are grouped by the adjacent native decimal-number methods. Manual reference counting.

extern char decimalBaseClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDecimalNumber));
extern char decimalPlaceholderClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                OF_NAME_STRING(ONSDecimalNumberPlaceholder));
extern char staticDecimalClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(_ONSStatic_NSDecimalNumber));
extern char decimalScannerClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSScanner));

// Native static records at 0x2227d0ec0, 0x2227d0ee0, and 0x2227d0f30, __DATA_CONST,__const.
// The payload starts at offset 8 and the mantissa limbs at offset 12.
static const struct {
    const void *isa;
    uint32_t header;
    uint16_t limbs[8];
} minimumDecimal = {staticDecimalClass, 0x387f, {65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535}},
  maximumDecimal = {staticDecimalClass, 0x287f, {65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535}};

static const struct {
    const void *isa;
    uint32_t header;
} cachedPlaceholder = {decimalPlaceholderClass, 0};

static _Atomic(ONSDecimalNumber *) notANumber;

@implementation ONSDecimalNumberPlaceholder

// Native +[NSDecimalNumberPlaceholder allocWithZone:], 0x1c04f921c. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return (id)&cachedPlaceholder;
}

// Native -[NSDecimalNumberPlaceholder retain], 0x1c04f9228. Non-external.
- (id)retain {
    return self;
}

// Native -[NSDecimalNumberPlaceholder release], 0x1c04f922c. Non-external.
- (oneway void)release {
}

// Native -[NSDecimalNumberPlaceholder autorelease], 0x1c04f9230. Non-external.
- (id)autorelease {
    return self;
}

// Native -[NSDecimalNumberPlaceholder retainCount], 0x1c04f9234. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

// Native -[NSDecimalNumberPlaceholder init], 0x1c04f923c. Non-external.
- (id)init {
    objc_release(self);
    return [[ONSDecimalNumber notANumber] copyWithZone:NULL];
}

// Native -[NSDecimalNumberPlaceholder initWithDecimal:], 0x1c04f9260. Non-external.
- (id)initWithDecimal:(ONSDecimal)decimal {
    _ONSDecimalCompact(&decimal);
    Class cls = (Class)objc_opt_self((id)decimalBaseClass);
    id number = ONSAllocateObject(cls, 2 * decimal._length, NULL);
    ONSDecimalCopy((ONSDecimal *)((unsigned char *)number + 8), &decimal);
    return number;
}

// Native -[NSDecimalNumberPlaceholder initWithString:], 0x1c04f92b8. Non-external.
- (id)initWithString:(id)string {
    return [self initWithString:string locale:nil];
}

// Native -[NSDecimalNumberPlaceholder initWithString:locale:], 0x1c04f92c0. Non-external.
- (id)initWithString:(id)string locale:(id)locale {
    ONSScanner *scanner = [objc_allocWithZone((Class)decimalScannerClass) initWithString:string];
    ONSDecimal decimal = {0};
    if (locale != nil) {
        [scanner setLocale:locale];
    }
    id result = [scanner scanDecimal:&decimal] ? [self initWithDecimal:decimal] : [self init];
    objc_release(scanner);
    return result;
}

// Native -[NSDecimalNumberPlaceholder initWithMantissa:exponent:isNegative:], 0x1c04f9394. Non-external.
- (id)initWithMantissa:(unsigned long long)mantissa exponent:(short)exponent isNegative:(BOOL)negative {
    ONSDecimal decimal = {0};
    decimal._exponent = (int8_t)exponent;
    decimal._isNegative = negative != NO;
    unsigned int count = 0;
    while (mantissa != 0) {
        decimal._mantissa[count++] = (unsigned short)mantissa;
        mantissa >>= 16;
    }
    decimal._length = count;
    return [self initWithDecimal:decimal];
}

// Native -[NSDecimalNumberPlaceholder classForCoder], 0x1c04f9440. Non-external.
- (Class)classForCoder {
    return objc_opt_class((id)decimalBaseClass);
}

// Native +[NSDecimalNumberPlaceholder supportsSecureCoding], 0x1c04f944c. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSDecimalNumberPlaceholder initWithCoder:], 0x1c04f9454. Non-external.
- (id)initWithCoder:(id)coder {
    ONSDecimal decimal = {0};
    if ([coder allowsKeyedCoding]) {
        NSUInteger size = 0;
        decimal._exponent = (int8_t)[coder decodeInt32ForKey:(id)OF_STR("NS.exponent")];
        decimal._length = (unsigned int)[coder decodeInt32ForKey:(id)OF_STR("NS.length")] & 0xf;
        if (decimal._length <= 8) {
            decimal._isNegative = [coder decodeBoolForKey:(id)OF_STR("NS.negative")] != NO;
            decimal._isCompact = [coder decodeBoolForKey:(id)OF_STR("NS.compact")] != NO;
            int byteOrder = [coder decodeInt32ForKey:(id)OF_STR("NS.mantissa.bo")];
            const uint8_t *bytes = [coder decodeBytesForKey:(id)OF_STR("NS.mantissa") returnedLength:&size];
            if (size == 16) {
                if (bytes == NULL) {
                    id reason = [ONSString
                        stringWithFormat:(id)OF_STR("%@: critical NSDecimalNumber archived data is missing"),
                                         self];
                    objc_exception_throw([ONSException
                        exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                   reason:reason
                                 userInfo:nil]);
                }
                for (unsigned int index = 0; index < 8; ++index) {
                    uint16_t limb;
                    memcpy(&limb, bytes + 2 * index, sizeof(limb));
                    decimal._mantissa[index] = byteOrder == 1 ? limb : __builtin_bswap16(limb);
                }
                return [self initWithDecimal:decimal];
            }
        }
        id reason = [ONSString
            stringWithFormat:(id)OF_STR("%@: critical NSDecimalNumber archived data is wrong size"), self];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }

    int32_t exponent = 0;
    unsigned short length = 0;
    unsigned char negative = 0;
    unsigned char compact = 0;
    unsigned int count = 0;
    [coder decodeValueOfObjCType:"i" at:&exponent size:4];
    [coder decodeValueOfObjCType:"S" at:&length size:2];
    [coder decodeValueOfObjCType:"C" at:&negative size:1];
    [coder decodeValueOfObjCType:"C" at:&compact size:1];
    decimal._length = length & 0xf;
    if (decimal._length <= 8) {
        decimal._exponent = (int8_t)exponent;
        decimal._isNegative = negative != 0;
        decimal._isCompact = compact != 0;
        [coder decodeValueOfObjCType:"I" at:&count size:4];
        if (count <= 8) {
            [coder decodeArrayOfObjCType:"S" count:count at:decimal._mantissa];
            return [self initWithDecimal:decimal];
        }
    }
    id reason = [ONSString
        stringWithFormat:(id)OF_STR("%@: critical NSDecimalNumber archived data is wrong size"), self];
    objc_release(self);
    objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                  reason:reason
                                                userInfo:nil]);
}

@end

@implementation _ONSStatic_NSDecimalNumber

// Native -[_NSStatic_NSDecimalNumber retain], 0x1c04f974c. Non-external.
- (id)retain {
    return self;
}

// Native -[_NSStatic_NSDecimalNumber release], 0x1c04f9750. Non-external.
- (oneway void)release {
}

// Native -[_NSStatic_NSDecimalNumber retainCount], 0x1c04f9754. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

// Native -[_NSStatic_NSDecimalNumber classForCoder], 0x1c04f975c. Non-external.
- (Class)classForCoder {
    return objc_opt_class((id)decimalBaseClass);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

// Native -[_NSStatic_NSDecimalNumber dealloc], 0x1c0fe2cd0. Non-external.
- (void)dealloc {
    __builtin_arm_trap(1);
}

#pragma clang diagnostic pop

@end

@implementation ONSDecimalNumber

// Native +[NSDecimalNumber initialize], 0x1c04f9768. Non-external.
+ (void)initialize {
    if (self == (Class)decimalBaseClass) {
        objc_opt_class((id)staticDecimalClass);
    }
}

// Native +[NSDecimalNumber allocWithZone:], 0x1c04f9788. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == objc_opt_class((id)decimalBaseClass)) {
        return (id)&cachedPlaceholder;
    }
    return ONSAllocateObject(self, 0, zone);
}

// Native -[NSDecimalNumber copy], 0x1c04f97dc. Non-external.
- (id)copy {
    return objc_retain(self);
}

// Native -[NSDecimalNumber copyWithZone:], 0x1c04f97e0. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    if (ONSShouldRetainWithZone(self, zone)) {
        return objc_retain(self);
    }
    ONSDecimal decimal = {0};
    if (self != nil) {
        decimal = [self decimalValue];
    }
    return [(id)&cachedPlaceholder initWithDecimal:decimal];
}

// Native +[NSDecimalNumber decimalNumberWithDecimal:], 0x1c04f98a4. Non-external.
+ (id)decimalNumberWithDecimal:(ONSDecimal)decimal {
    return objc_autorelease([(id)&cachedPlaceholder initWithDecimal:decimal]);
}

// Native -[NSDecimalNumber initWithMantissa:exponent:isNegative:], 0x1c04f9944. Non-external.
- (id)initWithMantissa:(unsigned long long)mantissa exponent:(short)exponent isNegative:(BOOL)negative {
    objc_release(self);
    objc_exception_throw([ONSException
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:(id)OF_STR(
                              "Did you forget to nest alloc and initWithMantissa:exponent:isNegative ?"
                          )
                 userInfo:nil]);
}

// Native -[NSDecimalNumber initWithDecimal:], 0x1c04f9978. Non-external.
- (id)initWithDecimal:(ONSDecimal)decimal {
    objc_release(self);
    objc_exception_throw([ONSException
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:(id)OF_STR("Did you forget to nest alloc and initWithDecimal: ?")
                 userInfo:nil]);
}

// Native -[NSDecimalNumber initWithString:], 0x1c04f99ac. Non-external.
- (id)initWithString:(id)string {
    objc_release(self);
    objc_exception_throw([ONSException
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:(id)OF_STR("Did you forget to nest alloc and initWithString: ?")
                 userInfo:nil]);
}

// Native -[NSDecimalNumber initWithString:locale:], 0x1c04f99e0. Non-external.
- (id)initWithString:(id)string locale:(id)locale {
    objc_release(self);
    objc_exception_throw([ONSException
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:(id)OF_STR("Did you forget to nest alloc and initWithString:locale: ?")
                 userInfo:nil]);
}

// Native +[NSDecimalNumber supportsSecureCoding], 0x1c04f9a14. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSDecimalNumber initWithCoder:], 0x1c04f9a1c. Non-external.
- (id)initWithCoder:(id)coder {
    objc_release(self);
    objc_exception_throw([ONSException
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:(id)OF_STR("Did you forget to nest alloc and initWithCoder: ?")
                 userInfo:nil]);
}

// Native -[NSDecimalNumber description], 0x1c04f9a50. Non-external.
- (id)description {
    return [self descriptionWithLocale:nil];
}

// Native -[NSDecimalNumber descriptionWithLocale:], 0x1c04f9a58. Non-external.
- (id)descriptionWithLocale:(id)locale {
    ONSDecimal decimal = {0};
    if (self != nil) {
        decimal = [self decimalValue];
    }
    return _ONSDecimalString(&decimal, locale);
}

// Native -[NSDecimalNumber decimalValue], 0x1c04f9ac8. Non-external.
- (ONSDecimal)decimalValue {
    ONSDecimal result = {0};
    ONSDecimalCopy(&result, (const ONSDecimal *)((const unsigned char *)self + 8));
    return result;
}

// Native +[NSDecimalNumber minimumDecimalNumber], 0x1c04f9adc. Non-external.
+ (id)minimumDecimalNumber {
    return (id)&minimumDecimal;
}

// Native +[NSDecimalNumber maximumDecimalNumber], 0x1c04f9ae8. Non-external.
+ (id)maximumDecimalNumber {
    return (id)&maximumDecimal;
}

// Native +[NSDecimalNumber notANumber], 0x1c04f9b0c. Non-external.
// Foundation defect: the native load at 0x1c04f9b1c races with the store at 0x1c04f9b44,
// which publishes the object before setting the NaN sign bit at 0x1c04f9b64. Concurrent
// first calls can also leak. Publish the initialized object and release any losing allocation.
+ (id)notANumber {
    ONSDecimalNumber *number = atomic_load_explicit(&notANumber, memory_order_acquire);
    if (number != nil) {
        return number;
    }
    Class cls = (Class)objc_opt_self((id)decimalBaseClass);
    ONSDecimalNumber *candidate = ONSAllocateObject(cls, 0, NULL);
    candidate->_length = 0;
    candidate->_isNegative = 1;
    if (atomic_compare_exchange_strong_explicit(
            &notANumber, &number, candidate, memory_order_acq_rel, memory_order_acquire
        )) {
        return candidate;
    }
    objc_release(candidate);
    return number;
}

// Native -[NSDecimalNumber compare:], 0x1c04f9b74. Non-external.
- (NSInteger)compare:(id)number {
    if (self == number) {
        return 0;
    }
    if (number == nil) {
        return 1;
    }
    if (objc_opt_isKindOfClass(number, (Class)objc_opt_self((id)decimalBaseClass))) {
        ONSDecimal left = {0};
        if (self != nil) {
            left = [self decimalValue];
        }
        ONSDecimal right = [number decimalValue];
        return _ONSDecimalCompare(&left, &right);
    }
    NSInteger result = [number compare:self];
    return result == 1 ? -1 : result == -1;
}

// Native -[NSDecimalNumber isEqual:], 0x1c04f9c58. Non-external.
- (BOOL)isEqual:(id)object {
    return self == object || (_ONSIsNSNumber(object) && [self compare:object] == 0);
}

// Native -[NSDecimalNumber objCType], 0x1c04f9ca8. Non-external.
- (const char *)objCType {
    return "d";
}

// Native -[NSDecimalNumber doubleValue], 0x1c04f9cb4. Non-external.
// 0x1c04f9cf4 fuses the multiply and add, which are shown separately in the pseudocode.
- (double)doubleValue {
    double value = 0.0;
    if (_length == 0 && _isNegative) {
        return __builtin_nan("");
    }
    const uint16_t *limbs = _mantissa;
    for (unsigned int length = _length; length != 0; --length) {
        value = __builtin_fma(value, 65536.0, (double)limbs[length - 1]);
    }
    int exponent = _exponent;
    while (exponent < 0) {
        value /= 10.0;
        ++exponent;
    }
    while (exponent > 0) {
        value *= 10.0;
        --exponent;
    }
    return _isNegative ? -value : value;
}

// Native -[NSDecimalNumber getValue:], 0x1c04f9d74. Non-external.
- (void)getValue:(void *)value {
    *(double *)value = [self doubleValue];
}

// Native __checkErrorAndRound, 0x1c04f9ed4. Non-external.
static id _checkErrorAndRound(
    ONSCalculationError error,
    SEL operation,
    id left,
    id right,
    short scale,
    ONSRoundingMode mode,
    ONSDecimal *decimal,
    id behavior
) {
    id replacement = nil;
    if (error != ONSCalculationNoError) {
        replacement = [behavior exceptionDuringOperation:operation
                                                   error:error
                                             leftOperand:left
                                            rightOperand:right];
    }
    if (replacement != nil) {
        if (scale == INT16_MAX) {
            return replacement;
        }
        ONSDecimal value = [replacement decimalValue];
        _ONSDecimalRound(decimal, &value, scale, mode);
    } else if (scale != INT16_MAX) {
        _ONSDecimalRound(decimal, decimal, scale, mode);
    }
    return [objc_opt_self((id)decimalBaseClass) decimalNumberWithDecimal:*decimal];
}

// Native -[NSDecimalNumber decimalNumberByMultiplyingBy:], 0x1c04fa164. Non-external.
- (id)decimalNumberByMultiplyingBy:(id)number {
    id behavior = [objc_opt_self((id)decimalBaseClass) defaultBehavior];
    return [self decimalNumberByMultiplyingBy:number withBehavior:behavior];
}

// Native -[NSDecimalNumber decimalNumberByMultiplyingBy:withBehavior:], 0x1c04fa1a0. Non-external.
- (id)decimalNumberByMultiplyingBy:(id)number withBehavior:(id)behavior {
    ONSDecimal product = {0};
    short scale = [behavior scale];
    ONSRoundingMode mode = [behavior roundingMode];
    ONSDecimal left = {0};
    if (self != nil) {
        left = [self decimalValue];
    }
    ONSDecimal right = {0};
    if (number != nil) {
        right = [number decimalValue];
    }
    ONSCalculationError error = _ONSDecimalMultiply(&product, &left, &right, mode);
    return _checkErrorAndRound(error, _cmd, self, number, scale, mode, &product, behavior);
}

// Native +[NSDecimalNumber defaultBehavior], 0x1c04fa764. Non-external.
+ (id)defaultBehavior {
    id thread = [ONSThread currentThread];
    id behavior = [[thread threadDictionary] objectForKey:(id)OF_STR("NSDecimalNumberBehaviors")];
    return behavior != nil ? behavior : [ONSDecimalNumberHandler defaultDecimalNumberHandler];
}

// Native -[NSDecimalNumber classForCoder], 0x1c04fa7a4. Non-external.
- (Class)classForCoder {
    return objc_opt_class((id)decimalBaseClass);
}

// Native -[NSDecimalNumber encodeWithCoder:], 0x1c04fa7b0. Non-external.
- (void)encodeWithCoder:(id)coder {
    ONSDecimal decimal = {0};
    if (self != nil) {
        decimal = [self decimalValue];
    }
    if (decimal._length < 8) {
        memset(decimal._mantissa + decimal._length, 0, 16 - 2 * decimal._length);
    }
    if ([coder allowsKeyedCoding]) {
        [coder encodeInt32:decimal._exponent forKey:(id)OF_STR("NS.exponent")];
        [coder encodeInt32:decimal._length forKey:(id)OF_STR("NS.length")];
        [coder encodeBool:decimal._isNegative forKey:(id)OF_STR("NS.negative")];
        [coder encodeBool:decimal._isCompact forKey:(id)OF_STR("NS.compact")];
        [coder encodeInt32:1 forKey:(id)OF_STR("NS.mantissa.bo")];
        [coder encodeBytes:(const uint8_t *)decimal._mantissa length:16 forKey:(id)OF_STR("NS.mantissa")];
    } else {
        int32_t exponent = decimal._exponent;
        unsigned short length = decimal._length;
        unsigned char negative = decimal._isNegative;
        unsigned char compact = decimal._isCompact;
        uint64_t count = 8;
        [coder encodeValueOfObjCType:"i" at:&exponent];
        [coder encodeValueOfObjCType:"S" at:&length];
        [coder encodeValueOfObjCType:"C" at:&negative];
        [coder encodeValueOfObjCType:"C" at:&compact];
        [coder encodeValueOfObjCType:"I" at:&count];
        [coder encodeArrayOfObjCType:"S" count:count at:decimal._mantissa];
    }
}

// Native -[NSDecimalNumber _allowsDirectEncoding], 0x1c04fa998. Non-external.
- (BOOL)_allowsDirectEncoding {
    return NO;
}

// Native -[NSDecimalNumber charValue], 0x1c04fa9a0. Non-external.
- (char)charValue {
    double number = [self doubleValue];
    int32_t value;
    __asm__("fcvtzs %w0, %d1" : "=r"(value) : "w"(number));
    return (char)value;
}

// Native -[NSDecimalNumber unsignedCharValue], 0x1c04fa9b8. Non-external.
- (unsigned char)unsignedCharValue {
    double number = [self doubleValue];
    int32_t value;
    __asm__("fcvtzs %w0, %d1" : "=r"(value) : "w"(number));
    return (unsigned char)value;
}

// Native -[NSDecimalNumber shortValue], 0x1c04fa9d0. Non-external.
- (short)shortValue {
    double number = [self doubleValue];
    int32_t value;
    __asm__("fcvtzs %w0, %d1" : "=r"(value) : "w"(number));
    return (short)value;
}

// Native -[NSDecimalNumber unsignedShortValue], 0x1c04fa9e8. Non-external.
- (unsigned short)unsignedShortValue {
    double number = [self doubleValue];
    int32_t value;
    __asm__("fcvtzs %w0, %d1" : "=r"(value) : "w"(number));
    return (unsigned short)value;
}

// Native -[NSDecimalNumber intValue], 0x1c04faa00. Non-external.
- (int)intValue {
    double number = [self doubleValue];
    int32_t value;
    __asm__("fcvtzs %w0, %d1" : "=r"(value) : "w"(number));
    return (int)value;
}

// Native -[NSDecimalNumber unsignedIntValue], 0x1c04faa18. Non-external.
- (unsigned int)unsignedIntValue {
    double number = [self doubleValue];
    uint32_t value;
    __asm__("fcvtzu %w0, %d1" : "=r"(value) : "w"(number));
    return (unsigned int)value;
}

// Native -[NSDecimalNumber longValue], 0x1c04faa30. Non-external.
- (long)longValue {
    unsigned int length = _length;
    if (length == 0 && _isNegative) {
        return 9;
    }
    const uint16_t *limbs = _mantissa;
    uint64_t value = 0;
    while (length != 0) {
        value = (value << 16) | limbs[--length];
    }
    int exponent = _exponent;
    while (exponent < 0) {
        value = (uint64_t)((int64_t)value / 10);
        ++exponent;
    }
    while (exponent > 0) {
        value *= 10;
        --exponent;
    }
    return (long)(_isNegative ? -value : value);
}

// Native -[NSDecimalNumber unsignedLongValue], 0x1c04faae4. Non-external.
- (unsigned long)unsignedLongValue {
    unsigned int length = _length;
    if (length == 0 && _isNegative) {
        return 9;
    }
    const uint16_t *limbs = _mantissa;
    uint64_t value = 0;
    while (length != 0) {
        value = (value << 16) | limbs[--length];
    }
    int exponent = _exponent;
    while (exponent < 0) {
        value = value / 10;
        ++exponent;
    }
    while (exponent > 0) {
        value *= 10;
        --exponent;
    }
    return (unsigned long)(_isNegative ? -value : value);
}

// Native -[NSDecimalNumber longLongValue], 0x1c04fab94. Non-external.
- (long long)longLongValue {
    unsigned int length = _length;
    if (length == 0 && _isNegative) {
        return 9;
    }
    const uint16_t *limbs = _mantissa;
    uint64_t value = 0;
    while (length != 0) {
        value = (value << 16) | limbs[--length];
    }
    int exponent = _exponent;
    while (exponent < 0) {
        value = (uint64_t)((int64_t)value / 10);
        ++exponent;
    }
    while (exponent > 0) {
        value *= 10;
        --exponent;
    }
    return (long long)(_isNegative ? -value : value);
}

// Native -[NSDecimalNumber unsignedLongLongValue], 0x1c04fac48. Non-external.
- (unsigned long long)unsignedLongLongValue {
    unsigned int length = _length;
    if (length == 0 && _isNegative) {
        return 9;
    }
    const uint16_t *limbs = _mantissa;
    uint64_t value = 0;
    while (length != 0) {
        value = (value << 16) | limbs[--length];
    }
    int exponent = _exponent;
    while (exponent < 0) {
        value = value / 10;
        ++exponent;
    }
    while (exponent > 0) {
        value *= 10;
        --exponent;
    }
    return (unsigned long long)(_isNegative ? -value : value);
}

// Native -[NSDecimalNumber floatValue], 0x1c04facf8. Non-external.
- (float)floatValue {
    return (float)[self doubleValue];
}

// Native -[NSDecimalNumber boolValue], 0x1c04fad10. Non-external.
- (BOOL)boolValue {
    return [self doubleValue] != 0.0;
}

// Native -[NSDecimalNumber _getCString:length:multiplier:], 0x1c04fad2c. Non-external.
- (BOOL)_getCString:(char *)buffer length:(int)length multiplier:(double)multiplier {
    ONSDecimalNumber *number = self;
    if (multiplier != 1.0) {
        id factor = [objc_alloc((Class)decimalBaseClass) initWithDouble:multiplier];
        number = [self decimalNumberByMultiplyingBy:factor];
        objc_release(factor);
    }
    return [[number description] getCString:buffer maxLength:(ONSUInteger)(int64_t)length encoding:1];
}

@end

@implementation ONSDecimalNumberPlaceholder (NSNumberCompatibility)

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithLongLong:], 0x1c04fb89c. Non-external.
- (id)initWithLongLong:(long long)value {
    BOOL negative = value < 0;
    uint64_t mantissa = negative ? -(uint64_t)value : (uint64_t)value;
    return [self initWithMantissa:mantissa exponent:0 isNegative:negative];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithBool:], 0x1c04fb8b4. Non-external.
- (id)initWithBool:(BOOL)value {
    return [self initWithLongLong:value];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithChar:], 0x1c04fb8bc. Non-external.
- (id)initWithChar:(char)value {
    return [self initWithLongLong:value];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithShort:], 0x1c04fb8c4. Non-external.
- (id)initWithShort:(short)value {
    return [self initWithLongLong:value];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithInt:], 0x1c04fb8cc. Non-external.
- (id)initWithInt:(int)value {
    return [self initWithLongLong:value];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithInteger:], 0x1c04fb8d4. Non-external.
- (id)initWithInteger:(NSInteger)value {
    return [self initWithLongLong:value];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithLong:], 0x1c04fb8d8. Non-external.
- (id)initWithLong:(long)value {
    return [self initWithLongLong:value];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithUnsignedLongLong:], 0x1c04fb8dc.
// Non-external.
- (id)initWithUnsignedLongLong:(unsigned long long)value {
    return [self initWithMantissa:value exponent:0 isNegative:NO];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithUnsignedChar:], 0x1c04fb8e8.
// Non-external.
- (id)initWithUnsignedChar:(unsigned char)value {
    return [self initWithUnsignedLongLong:value];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithUnsignedShort:], 0x1c04fb8f0.
// Non-external.
- (id)initWithUnsignedShort:(unsigned short)value {
    return [self initWithUnsignedLongLong:value];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithUnsignedInt:], 0x1c04fb8f8.
// Non-external.
- (id)initWithUnsignedInt:(unsigned int)value {
    return [self initWithUnsignedLongLong:value];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithUnsignedInteger:], 0x1c04fb900.
// Non-external.
- (id)initWithUnsignedInteger:(NSUInteger)value {
    return [self initWithUnsignedLongLong:value];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithUnsignedLong:], 0x1c04fb904.
// Non-external.
- (id)initWithUnsignedLong:(unsigned long)value {
    return [self initWithUnsignedLongLong:value];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithDouble:], 0x1c04fb908. Non-external.
- (id)initWithDouble:(double)value {
    if (isnan(value)) {
        return [self initWithMantissa:0 exponent:0 isNegative:YES];
    }
    BOOL negative = value < 0.0;
    double magnitude = negative ? -value : value;
    if (magnitude < 0x1.80c903f7379f2p-366) {
        return [self initWithMantissa:0 exponent:0 isNegative:NO];
    }
    if (magnitude > 0x1.cd47aa7bead34p+485) {
        return [self initWithMantissa:UINT64_MAX exponent:127 isNegative:negative];
    }
    short exponent = 0;
    while (magnitude < 0x1p64) {
        magnitude *= 10.0;
        --exponent;
    }
    while (magnitude > 0x1p64) {
        magnitude /= 10.0;
        ++exponent;
    }
    uint64_t mantissa;
    __asm__("fcvtzu %0, %d1" : "=r"(mantissa) : "w"(magnitude));
    return [self initWithMantissa:mantissa exponent:exponent isNegative:negative];
}

// Native -[NSDecimalNumberPlaceholder(NSNumberCompatibility) initWithFloat:], 0x1c04fb9dc. Non-external.
- (id)initWithFloat:(float)value {
    return [self initWithDouble:value];
}

@end

@implementation ONSDecimalNumber (NSNumberCompatibility)

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithChar:], 0x1c04fb9e4. Non-external.
+ (id)numberWithChar:(char)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithChar:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithUnsignedChar:], 0x1c04fba14. Non-external.
+ (id)numberWithUnsignedChar:(unsigned char)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithUnsignedChar:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithShort:], 0x1c04fba44. Non-external.
+ (id)numberWithShort:(short)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithShort:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithUnsignedShort:], 0x1c04fba74. Non-external.
+ (id)numberWithUnsignedShort:(unsigned short)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithUnsignedShort:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithInt:], 0x1c04fbaa4. Non-external.
+ (id)numberWithInt:(int)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithInt:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithInteger:], 0x1c04fbad4. Non-external.
+ (id)numberWithInteger:(NSInteger)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithInteger:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithUnsignedInt:], 0x1c04fbb04. Non-external.
+ (id)numberWithUnsignedInt:(unsigned int)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithUnsignedInt:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithUnsignedInteger:], 0x1c04fbb34. Non-external.
+ (id)numberWithUnsignedInteger:(NSUInteger)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithUnsignedInteger:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithLong:], 0x1c04fbb64. Non-external.
+ (id)numberWithLong:(long)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithLong:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithUnsignedLong:], 0x1c04fbb94. Non-external.
+ (id)numberWithUnsignedLong:(unsigned long)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithUnsignedLong:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithLongLong:], 0x1c04fbbc4. Non-external.
+ (id)numberWithLongLong:(long long)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithLongLong:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithUnsignedLongLong:], 0x1c04fbbf4. Non-external.
+ (id)numberWithUnsignedLongLong:(unsigned long long)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithUnsignedLongLong:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithFloat:], 0x1c04fbc24. Non-external.
+ (id)numberWithFloat:(float)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithFloat:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithDouble:], 0x1c04fbc54. Non-external.
+ (id)numberWithDouble:(double)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithDouble:value]);
}

// Native +[NSDecimalNumber(NSNumberCompatibility) numberWithBool:], 0x1c04fbc84. Non-external.
+ (id)numberWithBool:(BOOL)value {
    return objc_autorelease([objc_allocWithZone((Class)decimalBaseClass) initWithBool:value]);
}

@end
