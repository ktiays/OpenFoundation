//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSRange.h>

#if defined(__OBJC__)
@class NSString, NSURL, NSError;

typedef NS_OPTIONS(NSUInteger, NSDataReadingOptions) {
    NSDataReadingMappedIfSafe = 1UL << 0,
    NSDataReadingUncached = 1UL << 1,
    NSDataReadingMappedAlways = 1UL << 3,
    NSDataReadingMapped = NSDataReadingMappedIfSafe,
    NSMappedRead = NSDataReadingMapped,
    NSUncachedRead = NSDataReadingUncached
};

typedef NS_OPTIONS(NSUInteger, NSDataWritingOptions) {
    NSDataWritingAtomic = 1UL << 0,
    NSDataWritingWithoutOverwriting = 1UL << 1,
    NSDataWritingFileProtectionNone = 0x10000000,
    NSDataWritingFileProtectionComplete = 0x20000000,
    NSDataWritingFileProtectionCompleteUnlessOpen = 0x30000000,
    NSDataWritingFileProtectionCompleteUntilFirstUserAuthentication = 0x40000000,
    NSDataWritingFileProtectionMask = 0xf0000000,
    NSAtomicWrite = NSDataWritingAtomic
};

typedef NS_OPTIONS(NSUInteger, NSDataSearchOptions) {
    NSDataSearchBackwards = 1UL << 0,
    NSDataSearchAnchored = 1UL << 1
};

typedef NS_OPTIONS(NSUInteger, NSDataBase64EncodingOptions) {
    NSDataBase64Encoding64CharacterLineLength = 1UL << 0,
    NSDataBase64Encoding76CharacterLineLength = 1UL << 1,
    NSDataBase64EncodingEndLineWithCarriageReturn = 1UL << 4,
    NSDataBase64EncodingEndLineWithLineFeed = 1UL << 5
};

typedef NS_OPTIONS(NSUInteger, NSDataBase64DecodingOptions) {
    NSDataBase64DecodingIgnoreUnknownCharacters = 1UL << 0
};

@interface NSData : NSObject <NSCopying, NSMutableCopying, NSSecureCoding>

@property (readonly) NSUInteger length;
@property (readonly) const void *bytes NS_RETURNS_INNER_POINTER;

@end

@interface NSData (NSExtendedData)

@property (readonly, copy) NSString *description;
- (void)getBytes:(void *)buffer length:(NSUInteger)length;
- (BOOL)writeToFile:(NSString *)path atomically:(BOOL)useAuxiliaryFile;
- (BOOL)writeToFile:(NSString *)path options:(NSDataWritingOptions)writeOptionsMask error:(NSError **)errorPtr;

@end

@interface NSData (NSDataCreation)

+ (instancetype)data;
+ (instancetype)dataWithBytes:(const void *)bytes length:(NSUInteger)length;
+ (instancetype)dataWithBytesNoCopy:(void *)bytes length:(NSUInteger)length;
+ (instancetype)dataWithBytesNoCopy:(void *)bytes length:(NSUInteger)length freeWhenDone:(BOOL)b;
- (instancetype)initWithBytes:(const void *)bytes length:(NSUInteger)length;
- (instancetype)initWithBytesNoCopy:(void *)bytes length:(NSUInteger)length;
- (instancetype)initWithBytesNoCopy:(void *)bytes length:(NSUInteger)length freeWhenDone:(BOOL)b;
- (instancetype)initWithContentsOfURL:(NSURL *)url options:(NSDataReadingOptions)readOptionsMask error:(NSError **)errorPtr;
- (instancetype)initWithContentsOfURL:(NSURL *)url;
- (instancetype)initWithData:(NSData *)data;
+ (instancetype)dataWithData:(NSData *)data;

@end

@interface NSData (NSDataBase64Encoding)

- (NSString *)base64EncodedStringWithOptions:(NSDataBase64EncodingOptions)options;
- (instancetype)initWithBase64EncodedData:(NSData *)base64Data options:(NSDataBase64DecodingOptions)options;

@end

@interface NSMutableData : NSData

@property (readonly) void *mutableBytes NS_RETURNS_INNER_POINTER;
@property NSUInteger length;

@end

@interface NSMutableData (NSExtendedMutableData)

- (void)appendBytes:(const void *)bytes length:(NSUInteger)length;
- (void)appendData:(NSData *)other;
- (void)increaseLengthBy:(NSUInteger)extraLength;

@end

@interface NSMutableData (NSMutableDataCreation)

+ (instancetype)dataWithCapacity:(NSUInteger)aNumItems;
+ (instancetype)dataWithLength:(NSUInteger)length;
- (instancetype)initWithCapacity:(NSUInteger)capacity;
- (instancetype)initWithLength:(NSUInteger)length;

@end
#endif
