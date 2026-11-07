//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// Availability and enumeration macros of the SDK's CFAvailability.h. Apple OS availability does not restrict
// declarations on WebAssembly: every availability macro expands to nothing.

#include <TargetConditionals.h>
#include <Availability.h>

#ifndef __has_feature
    #define __has_feature(x) 0
#endif
#ifndef __has_attribute
    #define __has_attribute(x) 0
#endif
#ifndef __has_extension
    #define __has_extension(x) 0
#endif

// The API_* family, completing what objc4's platform <Availability.h> defines.
#ifndef API_AVAILABLE
    #define API_AVAILABLE(...)
#endif
#ifndef API_UNAVAILABLE
    #define API_UNAVAILABLE(...)
#endif
#ifndef API_DEPRECATED
    #define API_DEPRECATED(...)
#endif
#ifndef API_DEPRECATED_WITH_REPLACEMENT
    #define API_DEPRECATED_WITH_REPLACEMENT(...)
#endif
#ifndef API_OBSOLETED
    #define API_OBSOLETED(...)
#endif
#ifndef API_TO_BE_DEPRECATED
    #define API_TO_BE_DEPRECATED 100000
#endif
#ifndef API_AVAILABLE_BEGIN
    #define API_AVAILABLE_BEGIN(...)
#endif
#ifndef API_AVAILABLE_END
    #define API_AVAILABLE_END
#endif
#ifndef API_UNAVAILABLE_BEGIN
    #define API_UNAVAILABLE_BEGIN(...)
#endif
#ifndef API_UNAVAILABLE_END
    #define API_UNAVAILABLE_END
#endif
#ifndef API_DEPRECATED_BEGIN
    #define API_DEPRECATED_BEGIN(...)
#endif
#ifndef API_DEPRECATED_END
    #define API_DEPRECATED_END
#endif
#ifndef SPI_AVAILABLE
    #define SPI_AVAILABLE(...)
#endif
#ifndef SPI_DEPRECATED
    #define SPI_DEPRECATED(...)
#endif
#ifndef SPI_DEPRECATED_WITH_REPLACEMENT
    #define SPI_DEPRECATED_WITH_REPLACEMENT(...)
#endif
#ifndef __API_AVAILABLE
    #define __API_AVAILABLE(...)
#endif
#ifndef __API_DEPRECATED
    #define __API_DEPRECATED(...)
#endif
#ifndef __API_UNAVAILABLE
    #define __API_UNAVAILABLE(...)
#endif

#define CF_AVAILABLE(_mac, _ios)
#define CF_AVAILABLE_MAC(_mac)
#define CF_AVAILABLE_IOS(_ios)
#define CF_DEPRECATED(_macIntro, _macDep, _iosIntro, _iosDep, ...)
#define CF_DEPRECATED_MAC(_macIntro, _macDep, ...)
#define CF_DEPRECATED_IOS(_iosIntro, _iosDep, ...)

// Enumerations with a fixed underlying type. The SDK's `enum E : T E;` form is rejected outside Objective-C
// (-Welaborated-enum-base), so a named enumeration is declared opaquely first and then named by a typedef; the
// leading `typedef` of `typedef CF_ENUM(T, E) { ... };` declares a placeholder type name.
#if __has_attribute(enum_extensibility)
    #define __CF_ENUM_ATTRIBUTES __attribute__((enum_extensibility(open)))
    #define __CF_CLOSED_ENUM_ATTRIBUTES __attribute__((enum_extensibility(closed)))
    #define __CF_OPTIONS_ATTRIBUTES __attribute__((flag_enum, enum_extensibility(open)))
#else
    #define __CF_ENUM_ATTRIBUTES
    #define __CF_CLOSED_ENUM_ATTRIBUTES
    #define __CF_OPTIONS_ATTRIBUTES
#endif

#define __CF_ENUM_GET_MACRO(_1, _2, NAME, ...) NAME

#define __CF_NAMED_ENUM(_type, _name)                                                                        \
    int __CFEnumPlaceholder_##_name;                                                                          \
    enum __CF_ENUM_ATTRIBUTES _name : _type;                                                                  \
    typedef enum _name _name;                                                                                 \
    enum _name : _type
#define __CF_ANON_ENUM(_type) enum __CF_ENUM_ATTRIBUTES : _type
#define CF_CLOSED_ENUM(_type, _name)                                                                         \
    int __CFEnumPlaceholder_##_name;                                                                          \
    enum __CF_CLOSED_ENUM_ATTRIBUTES _name : _type;                                                           \
    typedef enum _name _name;                                                                                 \
    enum _name : _type
#if defined(__cplusplus)
    #define CF_OPTIONS(_type, _name)                                                                         \
        _type _name;                                                                                          \
        enum __CF_OPTIONS_ATTRIBUTES : _name
#else
    #define CF_OPTIONS(_type, _name)                                                                         \
        int __CFEnumPlaceholder_##_name;                                                                      \
        enum __CF_OPTIONS_ATTRIBUTES _name : _type;                                                           \
        typedef enum _name _name;                                                                             \
        enum _name : _type
#endif

#define CF_ENUM(...) __CF_ENUM_GET_MACRO(__VA_ARGS__, __CF_NAMED_ENUM, __CF_ANON_ENUM, )(__VA_ARGS__)

// Swift interface attributes, kept for the SDK's spelling.
#if __has_attribute(swift_wrapper)
    #define _CF_TYPED_ENUM __attribute__((swift_wrapper(enum)))
    #define _CF_TYPED_EXTENSIBLE_ENUM __attribute__((swift_wrapper(struct)))
#else
    #define _CF_TYPED_ENUM
    #define _CF_TYPED_EXTENSIBLE_ENUM
#endif
#define CF_STRING_ENUM _CF_TYPED_ENUM
#define CF_EXTENSIBLE_STRING_ENUM _CF_TYPED_EXTENSIBLE_ENUM
#define CF_TYPED_ENUM _CF_TYPED_ENUM
#define CF_TYPED_EXTENSIBLE_ENUM _CF_TYPED_EXTENSIBLE_ENUM

#if __has_attribute(swift_name)
    #define CF_SWIFT_NAME(_name) __attribute__((swift_name(#_name)))
#else
    #define CF_SWIFT_NAME(_name)
#endif
#if __has_attribute(swift_private)
    #define CF_REFINED_FOR_SWIFT __attribute__((swift_private))
#else
    #define CF_REFINED_FOR_SWIFT
#endif
#define CF_SWIFT_UNAVAILABLE(_msg) __attribute__((availability(swift, unavailable, message = _msg)))
#define CF_SWIFT_UNAVAILABLE_FROM_ASYNC(_msg)
#define CF_SWIFT_NONISOLATED
#define CF_SWIFT_SENDABLE
#define CF_SWIFT_NONSENDABLE
