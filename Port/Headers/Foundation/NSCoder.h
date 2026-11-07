//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>

#if defined(__OBJC__)
@class NSString, NSData, NSSet<ObjectType>, NSArray<ObjectType>, NSDictionary<KeyType, ObjectType>, NSError;

typedef NS_ENUM(NSInteger, NSDecodingFailurePolicy) {
    NSDecodingFailurePolicyRaiseException,
    NSDecodingFailurePolicySetErrorAndReturn
};

@interface NSCoder : NSObject

- (void)encodeValueOfObjCType:(const char *)type at:(const void *)addr;
- (NSData *)decodeDataObject;
- (NSInteger)versionForClassName:(NSString *)className;

@end

@interface NSCoder (NSExtendedCoder)

- (void)encodeObject:(id)object;
- (void)encodeBycopyObject:(id)anObject;
- (void)encodeConditionalObject:(id)object;
- (void)encodeValuesOfObjCTypes:(const char *)types, ...;
- (void)encodeArrayOfObjCType:(const char *)type count:(NSUInteger)count at:(const void *)array;
- (void)encodeBytes:(const void *)byteaddr length:(NSUInteger)length;
- (id)decodeObject;
- (void)decodeValuesOfObjCTypes:(const char *)types, ...;
- (void)decodeArrayOfObjCType:(const char *)itemType count:(NSUInteger)count at:(void *)array;
- (void *)decodeBytesWithReturnedLength:(NSUInteger *)lengthp NS_RETURNS_INNER_POINTER;
@property (readonly) BOOL allowsKeyedCoding;

- (void)encodeObject:(id)object forKey:(NSString *)key;
- (void)encodeConditionalObject:(id)object forKey:(NSString *)key;
- (void)encodeBool:(BOOL)value forKey:(NSString *)key;
- (void)encodeInt:(int)value forKey:(NSString *)key;
- (void)encodeInt32:(int32_t)value forKey:(NSString *)key;
- (void)encodeInt64:(int64_t)value forKey:(NSString *)key;
- (void)encodeFloat:(float)value forKey:(NSString *)key;
- (void)encodeDouble:(double)value forKey:(NSString *)key;
- (void)encodeBytes:(const uint8_t *)bytes length:(NSUInteger)length forKey:(NSString *)key;

- (BOOL)containsValueForKey:(NSString *)key;
- (id)decodeObjectForKey:(NSString *)key;
- (BOOL)decodeBoolForKey:(NSString *)key;
- (int)decodeIntForKey:(NSString *)key;
- (int32_t)decodeInt32ForKey:(NSString *)key;
- (int64_t)decodeInt64ForKey:(NSString *)key;
- (float)decodeFloatForKey:(NSString *)key;
- (double)decodeDoubleForKey:(NSString *)key;
- (const uint8_t *)decodeBytesForKey:(NSString *)key returnedLength:(NSUInteger *)lengthp NS_RETURNS_INNER_POINTER;

- (NSInteger)decodeIntegerForKey:(NSString *)key;

@property (readonly) BOOL requiresSecureCoding;
- (id)decodeObjectOfClass:(Class)aClass forKey:(NSString *)key;
- (id)decodeObjectOfClasses:(NSSet<Class> *)classes forKey:(NSString *)key;
- (NSArray *)decodeArrayOfObjectsOfClasses:(NSSet<Class> *)classes forKey:(NSString *)key;
@property (readonly, copy) NSSet<Class> *allowedClasses;
@property (readonly) NSDecodingFailurePolicy decodingFailurePolicy;
@property (readonly, copy) NSError *error;

@end
#endif
