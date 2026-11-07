//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// NSConstantFloatNumber and NSConstantDoubleNumber for the wasm port. Clang emits a compile-time constant
// object for a floating-point number literal (@1.5f, @2.5) when the deployment target has constant literals,
// as { NSConstantFloatNumber class, float } and { NSConstantDoubleNumber class, double }; the native classes
// are in Foundation beside the restored NSConstantIntegerNumber but are not restored (outside the consumers'
// scope). Each class here is a simple implementation shaped like the restored integer class: the value is the
// instance variable after isa (offset 4 for the float and 8 for the double on wasm32, matching the emitted
// records), -objCType answers "f" or "d", the accessors convert the value (integers through the port's
// saturating conversions, as the native FCVTZS/FCVTZU do), and the objects are immortal (retain, release,
// autorelease and copy answer the receiver; -retainCount is NSUIntegerMax).
//
// The class names are the native ones, which the compiler references; the port builds with system names.
// Manual reference counting, as the port's other Foundation units.

#import "Foundation/Number/ONSNumber.h"

#include <OFPortHookDeclarations.h>
#include <string.h>

__attribute__((visibility("default")))
@interface NSConstantFloatNumber : ONSNumber {
    @public
    float _value;
}
@end

__attribute__((visibility("default")))
@interface NSConstantDoubleNumber : ONSNumber {
    @public
    double _value;
}
@end

@implementation NSConstantFloatNumber

+ (id)allocWithZone:(struct _NSZone *)zone {
    return (id)[ONSNumber allocWithZone:zone];
}

+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

- (const char *)objCType {
    return "f";
}

- (void)getValue:(void *)value {
    memmove(value, &_value, sizeof(_value));
}

- (BOOL)boolValue {
    return _value != 0;
}

- (float)floatValue {
    return (float)_value;
}

- (double)doubleValue {
    return (double)_value;
}

- (id)autorelease {
    return self;
}

- (id)retain {
    return self;
}

- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

- (oneway void)release {
}

- (id)copyWithZone:(struct _NSZone *)zone {
    return self;
}

- (id)copy {
    return self;
}

- (char)charValue {
    return (char)OFPort_floatToInt32(_value);
}

- (unsigned char)unsignedCharValue {
    return (unsigned char)OFPort_floatToUInt32(_value);
}

- (short)shortValue {
    return (short)OFPort_floatToInt32(_value);
}

- (unsigned short)unsignedShortValue {
    return (unsigned short)OFPort_floatToUInt32(_value);
}

- (int)intValue {
    return OFPort_floatToInt32(_value);
}

- (unsigned int)unsignedIntValue {
    return OFPort_floatToUInt32(_value);
}

- (long)longValue {
    return (long)OFPort_floatToInt64(_value);
}

- (unsigned long)unsignedLongValue {
    return (unsigned long)OFPort_floatToUInt64(_value);
}

- (long long)longLongValue {
    return OFPort_floatToInt64(_value);
}

- (unsigned long long)unsignedLongLongValue {
    return OFPort_floatToUInt64(_value);
}

- (NSInteger)integerValue {
    return (NSInteger)OFPort_floatToInt64(_value);
}

- (NSUInteger)unsignedIntegerValue {
    return (NSUInteger)OFPort_floatToUInt64(_value);
}

@end

@implementation NSConstantDoubleNumber

+ (id)allocWithZone:(struct _NSZone *)zone {
    return (id)[ONSNumber allocWithZone:zone];
}

+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

- (const char *)objCType {
    return "d";
}

- (void)getValue:(void *)value {
    memmove(value, &_value, sizeof(_value));
}

- (BOOL)boolValue {
    return _value != 0;
}

- (float)floatValue {
    return (float)_value;
}

- (double)doubleValue {
    return (double)_value;
}

- (id)autorelease {
    return self;
}

- (id)retain {
    return self;
}

- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

- (oneway void)release {
}

- (id)copyWithZone:(struct _NSZone *)zone {
    return self;
}

- (id)copy {
    return self;
}

- (char)charValue {
    return (char)OFPort_doubleToInt32(_value);
}

- (unsigned char)unsignedCharValue {
    return (unsigned char)OFPort_doubleToUInt32(_value);
}

- (short)shortValue {
    return (short)OFPort_doubleToInt32(_value);
}

- (unsigned short)unsignedShortValue {
    return (unsigned short)OFPort_doubleToUInt32(_value);
}

- (int)intValue {
    return OFPort_doubleToInt32(_value);
}

- (unsigned int)unsignedIntValue {
    return OFPort_doubleToUInt32(_value);
}

- (long)longValue {
    return (long)OFPort_doubleToInt64(_value);
}

- (unsigned long)unsignedLongValue {
    return (unsigned long)OFPort_doubleToUInt64(_value);
}

- (long long)longLongValue {
    return OFPort_doubleToInt64(_value);
}

- (unsigned long long)unsignedLongLongValue {
    return OFPort_doubleToUInt64(_value);
}

- (NSInteger)integerValue {
    return (NSInteger)OFPort_doubleToInt64(_value);
}

- (NSUInteger)unsignedIntegerValue {
    return (NSUInteger)OFPort_doubleToUInt64(_value);
}

@end
