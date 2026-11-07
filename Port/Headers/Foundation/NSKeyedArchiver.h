//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSCoder.h>
#import <Foundation/NSPropertyList.h>
#import <Foundation/NSException.h>

#if defined(__OBJC__)
@class NSArray<ObjectType>, NSMutableData, NSData, NSString, NSSet<ObjectType>;
@protocol NSKeyedArchiverDelegate, NSKeyedUnarchiverDelegate;

FOUNDATION_EXPORT NSExceptionName const NSInvalidArchiveOperationException;
FOUNDATION_EXPORT NSExceptionName const NSInvalidUnarchiveOperationException;

@interface NSKeyedArchiver : NSCoder

- (instancetype)initRequiringSecureCoding:(BOOL)requiresSecureCoding;
+ (NSData *)archivedDataWithRootObject:(id)object requiringSecureCoding:(BOOL)requiresSecureCoding error:(NSError **)error;
- (instancetype)init;

@property (assign) id<NSKeyedArchiverDelegate> delegate;
@property (readonly, strong) NSData *encodedData;
- (void)finishEncoding;
+ (void)setClassName:(NSString *)codedName forClass:(Class)cls;
- (void)setClassName:(NSString *)codedName forClass:(Class)cls;
+ (NSString *)classNameForClass:(Class)cls;
- (NSString *)classNameForClass:(Class)cls;

- (void)encodeObject:(id)object forKey:(NSString *)key;
- (void)encodeConditionalObject:(id)object forKey:(NSString *)key;
- (void)encodeBool:(BOOL)value forKey:(NSString *)key;
- (void)encodeInt:(int)value forKey:(NSString *)key;
- (void)encodeInt32:(int32_t)value forKey:(NSString *)key;
- (void)encodeInt64:(int64_t)value forKey:(NSString *)key;
- (void)encodeFloat:(float)value forKey:(NSString *)key;
- (void)encodeDouble:(double)value forKey:(NSString *)key;
- (void)encodeBytes:(const uint8_t *)bytes length:(NSUInteger)length forKey:(NSString *)key;
@property (readwrite) BOOL requiresSecureCoding;

@end

@interface NSKeyedUnarchiver : NSCoder

- (instancetype)initForReadingFromData:(NSData *)data error:(NSError **)error;
+ (id)unarchivedObjectOfClass:(Class)cls fromData:(NSData *)data error:(NSError **)error;
+ (id)unarchivedObjectOfClasses:(NSSet<Class> *)classes fromData:(NSData *)data error:(NSError **)error;
- (instancetype)init;

@property (assign) id<NSKeyedUnarchiverDelegate> delegate;
- (void)finishDecoding;
+ (void)setClass:(Class)cls forClassName:(NSString *)codedName;
- (void)setClass:(Class)cls forClassName:(NSString *)codedName;
+ (Class)classForClassName:(NSString *)codedName;
- (Class)classForClassName:(NSString *)codedName;

- (BOOL)containsValueForKey:(NSString *)key;
- (id)decodeObjectForKey:(NSString *)key;
- (BOOL)decodeBoolForKey:(NSString *)key;
- (int)decodeIntForKey:(NSString *)key;
- (int32_t)decodeInt32ForKey:(NSString *)key;
- (int64_t)decodeInt64ForKey:(NSString *)key;
- (float)decodeFloatForKey:(NSString *)key;
- (double)decodeDoubleForKey:(NSString *)key;
- (const uint8_t *)decodeBytesForKey:(NSString *)key returnedLength:(NSUInteger *)lengthp NS_RETURNS_INNER_POINTER;
@property (readwrite) BOOL requiresSecureCoding;
@property (readwrite) NSDecodingFailurePolicy decodingFailurePolicy;

@end

@protocol NSKeyedArchiverDelegate <NSObject>
@optional
- (id)archiver:(NSKeyedArchiver *)archiver willEncodeObject:(id)object;
- (void)archiver:(NSKeyedArchiver *)archiver didEncodeObject:(id)object;
- (void)archiver:(NSKeyedArchiver *)archiver willReplaceObject:(id)object withObject:(id)newObject;
- (void)archiverWillFinish:(NSKeyedArchiver *)archiver;
- (void)archiverDidFinish:(NSKeyedArchiver *)archiver;

@end

@protocol NSKeyedUnarchiverDelegate <NSObject>
@optional
- (Class)unarchiver:(NSKeyedUnarchiver *)unarchiver
    cannotDecodeObjectOfClassName:(NSString *)name
                  originalClasses:(NSArray<NSString *> *)classNames;
- (id)unarchiver:(NSKeyedUnarchiver *)unarchiver didDecodeObject:(id)NS_RELEASES_ARGUMENT object NS_RETURNS_RETAINED;
- (void)unarchiver:(NSKeyedUnarchiver *)unarchiver willReplaceObject:(id)object withObject:(id)newObject;
- (void)unarchiverWillFinish:(NSKeyedUnarchiver *)unarchiver;
- (void)unarchiverDidFinish:(NSKeyedUnarchiver *)unarchiver;

@end

@interface NSObject (NSKeyedArchiverObjectSubstitution)

@property (readonly) Class classForKeyedArchiver;

@end
#endif
