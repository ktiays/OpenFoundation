//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// The SDK's NSObjCRuntime.h for OpenFoundation's wasm32 Foundation: NSInteger is long (NS_BUILD_32_LIKE_64), as
// objc4's <objc/NSObjCRuntime.h> defines it and as the restored Foundation is compiled.

#include <CoreFoundation/CoreFoundation.h>
#include <TargetConditionals.h>
#include <objc/NSObjCRuntime.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>

#if defined(__cplusplus)
    #define FOUNDATION_EXTERN extern "C"
#else
    #define FOUNDATION_EXTERN extern
#endif
#define FOUNDATION_EXPORT FOUNDATION_EXTERN __attribute__((visibility("default")))
#define FOUNDATION_IMPORT FOUNDATION_EXTERN
#define FOUNDATION_STATIC_INLINE static __inline__
#define FOUNDATION_EXTERN_INLINE extern __inline__
#define NS_INLINE static __inline__ __attribute__((always_inline))

#define NS_ENUM(...) CF_ENUM(__VA_ARGS__)
#define NS_OPTIONS(_type, _name) CF_OPTIONS(_type, _name)
#define NS_CLOSED_ENUM(_type, _name) CF_CLOSED_ENUM(_type, _name)
#define NS_ERROR_ENUM(_type, _name, _domain) CF_ENUM(_type, _name)
#define NS_TYPED_ENUM _CF_TYPED_ENUM
#define NS_TYPED_EXTENSIBLE_ENUM _CF_TYPED_EXTENSIBLE_ENUM
#define NS_STRING_ENUM _CF_TYPED_ENUM
#define NS_EXTENSIBLE_STRING_ENUM _CF_TYPED_EXTENSIBLE_ENUM

#if __has_feature(attribute_ns_returns_retained)
    #define NS_RETURNS_RETAINED __attribute__((ns_returns_retained))
#else
    #define NS_RETURNS_RETAINED
#endif
#if __has_feature(attribute_ns_returns_not_retained)
    #define NS_RETURNS_NOT_RETAINED __attribute__((ns_returns_not_retained))
#else
    #define NS_RETURNS_NOT_RETAINED
#endif
#if __has_feature(attribute_ns_consumed)
    #define NS_RELEASES_ARGUMENT __attribute__((ns_consumed))
#else
    #define NS_RELEASES_ARGUMENT
#endif
#if __has_feature(attribute_ns_consumes_self)
    #define NS_REPLACES_RECEIVER __attribute__((ns_consumes_self)) NS_RETURNS_RETAINED
#else
    #define NS_REPLACES_RECEIVER
#endif
#if __has_feature(attribute_objc_method_family)
    #define NS_METHOD_FAMILY(_family) __attribute__((objc_method_family(_family)))
#else
    #define NS_METHOD_FAMILY(_family)
#endif
#define NS_RETURNS_INNER_POINTER __attribute__((objc_returns_inner_pointer))
#define NS_AUTOMATED_REFCOUNT_UNAVAILABLE
#define NS_AUTOMATED_REFCOUNT_WEAK_UNAVAILABLE
#define NS_REQUIRES_PROPERTY_DEFINITIONS __attribute__((objc_requires_property_definitions))
#define NS_REQUIRES_NIL_TERMINATION __attribute__((sentinel(0, 1)))
#define NS_REQUIRES_SUPER __attribute__((objc_requires_super))
#define NS_FORMAT_FUNCTION(F, A) __attribute__((format(__NSString__, F, A)))
#define NS_FORMAT_ARGUMENT(A) __attribute__((format_arg(A)))
#define NS_NOESCAPE CF_NOESCAPE
#define NS_WARN_UNUSED_RESULT CF_WARN_UNUSED_RESULT
#define NS_ASSUME_NONNULL_BEGIN CF_ASSUME_NONNULL_BEGIN
#define NS_ASSUME_NONNULL_END CF_ASSUME_NONNULL_END
#define NS_UNAVAILABLE UNAVAILABLE_ATTRIBUTE
#define NS_ROOT_CLASS __attribute__((objc_root_class))
#define NS_DIRECT __attribute__((objc_direct))
#define NS_DIRECT_MEMBERS __attribute__((objc_direct_members))
#define NS_SWIFT_NAME(_name) CF_SWIFT_NAME(_name)
#define NS_SWIFT_UNAVAILABLE(_msg) CF_SWIFT_UNAVAILABLE(_msg)
#define NS_SWIFT_UNAVAILABLE_FROM_ASYNC(_msg)
#define NS_SWIFT_NOTHROW
#define NS_SWIFT_SENDABLE
#define NS_SWIFT_NONSENDABLE
#define NS_SWIFT_NONISOLATED
#define NS_REFINED_FOR_SWIFT CF_REFINED_FOR_SWIFT
#define NS_SWIFT_UI_ACTOR
#define NS_HEADER_AUDIT_BEGIN(...)
#define NS_HEADER_AUDIT_END(...)
#define NS_AVAILABLE(_mac, _ios)
#define NS_AVAILABLE_MAC(_mac)
#define NS_AVAILABLE_IOS(_ios)
#define NS_DEPRECATED(_macIntro, _macDep, _iosIntro, _iosDep, ...)
#define NS_DEPRECATED_MAC(_macIntro, _macDep, ...)
#define NS_DEPRECATED_IOS(_iosIntro, _iosDep, ...)
#define NS_CLASS_AVAILABLE(_mac, _ios)
#define NS_ENUM_AVAILABLE(_mac, _ios)
#ifndef UNAVAILABLE_ATTRIBUTE
    #define UNAVAILABLE_ATTRIBUTE __attribute__((unavailable))
#endif
#ifndef NS_DESIGNATED_INITIALIZER
    #define NS_DESIGNATED_INITIALIZER __attribute__((objc_designated_initializer))
#endif
#ifndef NS_VALID_UNTIL_END_OF_SCOPE
    #define NS_VALID_UNTIL_END_OF_SCOPE
#endif
#if !defined(NS_BLOCKS_AVAILABLE)
    #define NS_BLOCKS_AVAILABLE 1
#endif
#define NS_NONATOMIC_IOSONLY nonatomic

#define NSFoundationVersionWithFileManagerResourceForkSupport 412

typedef NS_CLOSED_ENUM(NSInteger, NSComparisonResult) {
    NSOrderedAscending = -1L,
    NSOrderedSame,
    NSOrderedDescending
};

static const NSInteger NSNotFound = NSIntegerMax;

#if defined(__OBJC__)
@class NSString, Protocol;

typedef NSComparisonResult (^NSComparator)(id obj1, id obj2);

typedef NS_OPTIONS(NSUInteger, NSEnumerationOptions) {
    NSEnumerationConcurrent = (1UL << 0),
    NSEnumerationReverse = (1UL << 1)
};

typedef NS_OPTIONS(NSUInteger, NSSortOptions) {
    NSSortConcurrent = (1UL << 0),
    NSSortStable = (1UL << 4)
};

typedef NS_ENUM(NSInteger, NSQualityOfService) {
    NSQualityOfServiceUserInteractive = 0x21,
    NSQualityOfServiceUserInitiated = 0x19,
    NSQualityOfServiceUtility = 0x11,
    NSQualityOfServiceBackground = 0x09,
    NSQualityOfServiceDefault = -1
};

typedef NSString *NSExceptionName NS_TYPED_EXTENSIBLE_ENUM;
typedef NSString *NSRunLoopMode NS_TYPED_EXTENSIBLE_ENUM;

FOUNDATION_EXPORT NSString *NSStringFromSelector(SEL aSelector);
FOUNDATION_EXPORT SEL NSSelectorFromString(NSString *aSelectorName);
FOUNDATION_EXPORT NSString *NSStringFromClass(Class aClass);
FOUNDATION_EXPORT Class NSClassFromString(NSString *aClassName);
FOUNDATION_EXPORT const char *NSGetSizeAndAlignment(const char *typePtr, NSUInteger *sizep, NSUInteger *alignp);
FOUNDATION_EXPORT void NSLog(NSString *format, ...) NS_FORMAT_FUNCTION(1, 2);
#endif
