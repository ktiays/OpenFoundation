//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <Foundation/NSObjCRuntime.h>
#include <Foundation/NSZone.h>

#if defined(__OBJC__)
#import <objc/NSObject.h>

@class NSString, NSInvocation, NSMethodSignature, NSCoder, NSEnumerator;

@protocol NSCopying

- (id)copyWithZone:(NSZone *)zone;

@end

@protocol NSMutableCopying

- (id)mutableCopyWithZone:(NSZone *)zone;

@end

@protocol NSCoding

- (void)encodeWithCoder:(NSCoder *)coder;
- (instancetype)initWithCoder:(NSCoder *)coder;

@end

@protocol NSSecureCoding <NSCoding>
@required
@property (class, readonly) BOOL supportsSecureCoding;

@end

@interface NSObject (NSCoderMethods)

+ (NSInteger)version;
+ (void)setVersion:(NSInteger)aVersion;
- (id)replacementObjectForCoder:(NSCoder *)coder;

@end

@protocol NSDiscardableContent
@required
- (BOOL)beginContentAccess;
- (void)endContentAccess;
- (void)discardContentIfPossible;
- (BOOL)isContentDiscarded;

@end

FOUNDATION_EXPORT id NSAllocateObject(Class aClass, NSUInteger extraBytes, NSZone *zone);
FOUNDATION_EXPORT void NSDeallocateObject(id object);
FOUNDATION_EXPORT BOOL NSShouldRetainWithZone(id anObject, NSZone *requestedZone);

NS_INLINE CF_RETURNS_RETAINED CFTypeRef CFBridgingRetain(id X) {
    return X ? CFRetain((CFTypeRef)X) : NULL;
}

NS_INLINE id CFBridgingRelease(CFTypeRef CF_CONSUMED X) {
    return [(id)X autorelease];
}
#endif
