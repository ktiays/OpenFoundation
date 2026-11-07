//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>

#if defined(__OBJC__)
@class NSString, NSDictionary<KeyType, ObjectType>, NSArray<ObjectType>, NSNumber;

FOUNDATION_EXPORT NSExceptionName const NSGenericException;
FOUNDATION_EXPORT NSExceptionName const NSRangeException;
FOUNDATION_EXPORT NSExceptionName const NSInvalidArgumentException;
FOUNDATION_EXPORT NSExceptionName const NSInternalInconsistencyException;
FOUNDATION_EXPORT NSExceptionName const NSMallocException;

__attribute__((__objc_exception__))
@interface NSException : NSObject <NSCopying, NSSecureCoding>

+ (NSException *)exceptionWithName:(NSExceptionName)name reason:(NSString *)reason userInfo:(NSDictionary *)userInfo;
- (instancetype)initWithName:(NSExceptionName)aName reason:(NSString *)aReason userInfo:(NSDictionary *)aUserInfo NS_DESIGNATED_INITIALIZER;

@property (readonly, copy) NSExceptionName name;
@property (readonly, copy) NSString *reason;
@property (readonly, copy) NSDictionary *userInfo;
@property (readonly, copy) NSArray<NSNumber *> *callStackReturnAddresses;
@property (readonly, copy) NSArray<NSString *> *callStackSymbols;

- (void)raise;

@end

@interface NSException (NSExceptionRaisingConveniences)

+ (void)raise:(NSExceptionName)name format:(NSString *)format, ... NS_FORMAT_FUNCTION(2, 3);
+ (void)raise:(NSExceptionName)name format:(NSString *)format arguments:(va_list)argList NS_FORMAT_FUNCTION(2, 0);

@end

@class NSAssertionHandler;

@interface NSAssertionHandler : NSObject

@property (class, readonly, strong) NSAssertionHandler *currentHandler;
- (void)handleFailureInFunction:(NSString *)functionName
                           file:(NSString *)fileName
                     lineNumber:(NSInteger)line
                    description:(NSString *)format, ... NS_FORMAT_FUNCTION(4, 5);

@end

#if !defined(NS_BLOCK_ASSERTIONS)
    #define NSAssert(condition, desc, ...)                                                                    \
        do {                                                                                                  \
            if (__builtin_expect(!(condition), 0)) {                                                          \
                NSString *__assert_file__ = [NSString stringWithUTF8String:__FILE__];                         \
                [[NSAssertionHandler currentHandler] handleFailureInMethod:_cmd                               \
                                                                    object:self                               \
                                                                      file:__assert_file__                    \
                                                                lineNumber:__LINE__                           \
                                                               description:(desc), ##__VA_ARGS__];            \
            }                                                                                                 \
        } while (0)
    #define NSCAssert(condition, desc, ...)                                                                   \
        do {                                                                                                  \
            if (__builtin_expect(!(condition), 0)) {                                                          \
                NSString *__assert_fn__ = [NSString stringWithUTF8String:__PRETTY_FUNCTION__];                \
                NSString *__assert_file__ = [NSString stringWithUTF8String:__FILE__];                         \
                [[NSAssertionHandler currentHandler] handleFailureInFunction:__assert_fn__                    \
                                                                        file:__assert_file__                  \
                                                                  lineNumber:__LINE__                         \
                                                                 description:(desc), ##__VA_ARGS__];          \
            }                                                                                                 \
        } while (0)
#else
    #define NSAssert(condition, desc, ...)                                                                    \
        do {                                                                                                  \
        } while (0)
    #define NSCAssert(condition, desc, ...)                                                                   \
        do {                                                                                                  \
        } while (0)
#endif
#define NSParameterAssert(condition) NSAssert((condition), @"Invalid parameter not satisfying: %@", @ #condition)
#define NSCParameterAssert(condition) NSCAssert((condition), @"Invalid parameter not satisfying: %@", @ #condition)
#endif
