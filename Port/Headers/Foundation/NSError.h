//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>

#if defined(__OBJC__)
@class NSDictionary<KeyType, ObjectType>, NSArray<ObjectType>, NSString;

typedef NSString *NSErrorDomain;
typedef NSString *NSErrorUserInfoKey NS_TYPED_EXTENSIBLE_ENUM;

FOUNDATION_EXPORT NSErrorDomain const NSCocoaErrorDomain;
FOUNDATION_EXPORT NSErrorDomain const NSPOSIXErrorDomain;

FOUNDATION_EXPORT NSErrorUserInfoKey const NSUnderlyingErrorKey;
FOUNDATION_EXPORT NSErrorUserInfoKey const NSLocalizedDescriptionKey;
FOUNDATION_EXPORT NSErrorUserInfoKey const NSDebugDescriptionErrorKey;
FOUNDATION_EXPORT NSErrorUserInfoKey const NSStringEncodingErrorKey;
FOUNDATION_EXPORT NSErrorUserInfoKey const NSURLErrorKey;
FOUNDATION_EXPORT NSErrorUserInfoKey const NSFilePathErrorKey;

@interface NSError : NSObject <NSCopying, NSSecureCoding>

- (instancetype)initWithDomain:(NSErrorDomain)domain
                          code:(NSInteger)code
                      userInfo:(NSDictionary<NSErrorUserInfoKey, id> *)dict NS_DESIGNATED_INITIALIZER;
+ (instancetype)errorWithDomain:(NSErrorDomain)domain code:(NSInteger)code userInfo:(NSDictionary<NSErrorUserInfoKey, id> *)dict;

@property (readonly, copy) NSErrorDomain domain;
@property (readonly) NSInteger code;
@property (readonly, copy) NSDictionary<NSErrorUserInfoKey, id> *userInfo;
@property (readonly, copy) NSString *localizedDescription;

@end
#endif
