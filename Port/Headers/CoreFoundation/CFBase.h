//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// The SDK's CFBase.h for OpenFoundation's wasm32 CoreFoundation. Every declaration has the type of the restored
// definition (Source/CoreFoundation/Base, Runtime): CFIndex is long, CFTypeID, CFOptionFlags and CFHashCode are
// unsigned long, as on arm64.

#include <CoreFoundation/CFAvailability.h>

#include <TargetConditionals.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>

// As the SDK's CFBase.h: code compiled with blocks gets the blocks runtime's declarations (_Block_copy,
// _Block_release).
#if __BLOCKS__
#include <Block.h>
#endif

#if !defined(__MACTYPES__) && !defined(_OS_OSTYPES_H)
    // The MacTypes subset CFBase.h declares when MacTypes.h is absent; the widths are arm64's.
    typedef unsigned char Boolean;
    typedef unsigned char UInt8;
    typedef signed char SInt8;
    typedef unsigned short UInt16;
    typedef signed short SInt16;
    typedef unsigned int UInt32;
    typedef signed int SInt32;
    typedef uint64_t UInt64;
    typedef int64_t SInt64;
    typedef SInt32 OSStatus;
    typedef float Float32;
    typedef double Float64;
    typedef unsigned short UniChar;
    typedef unsigned long UniCharCount;
    typedef unsigned char *StringPtr;
    typedef const unsigned char *ConstStringPtr;
    typedef unsigned char Str255[256];
    typedef const unsigned char *ConstStr255Param;
    typedef SInt16 OSErr;
    typedef SInt16 RegionCode;
    typedef SInt16 LangCode;
    typedef SInt16 ScriptCode;
    typedef UInt32 FourCharCode;
    typedef FourCharCode OSType;
    typedef UInt8 Byte;
    typedef SInt8 SignedByte;
    typedef UInt32 UTF32Char;
    typedef UInt16 UTF16Char;
    typedef UInt8 UTF8Char;
#endif

#if !defined(TRUE)
    #define TRUE 1
#endif
#if !defined(FALSE)
    #define FALSE 0
#endif

#if defined(__cplusplus)
    #define CF_EXTERN_C_BEGIN extern "C" {
    #define CF_EXTERN_C_END }
    #define CF_EXTERN extern "C"
#else
    #define CF_EXTERN_C_BEGIN
    #define CF_EXTERN_C_END
    #define CF_EXTERN extern
#endif

#define CF_EXPORT CF_EXTERN __attribute__((visibility("default")))
#define CF_INLINE static __inline__ __attribute__((always_inline))

#if __has_feature(attribute_cf_returns_retained)
    #define CF_RETURNS_RETAINED __attribute__((cf_returns_retained))
#else
    #define CF_RETURNS_RETAINED
#endif
#if __has_feature(attribute_cf_returns_not_retained)
    #define CF_RETURNS_NOT_RETAINED __attribute__((cf_returns_not_retained))
#else
    #define CF_RETURNS_NOT_RETAINED
#endif
#if __has_feature(attribute_cf_consumed)
    #define CF_RELEASES_ARGUMENT __attribute__((cf_consumed))
    #define CF_CONSUMED __attribute__((cf_consumed))
#else
    #define CF_RELEASES_ARGUMENT
    #define CF_CONSUMED
#endif

#define CF_IMPLICIT_BRIDGING_ENABLED
#define CF_IMPLICIT_BRIDGING_DISABLED

// The classes the bridged types name, declared as the SDK's CFBase.h declares them: a `__bridge` cast from `id`
// to a bridged type looks its class up, and clang refuses the cast when no declaration is found.
#if defined(__OBJC__) && __has_attribute(objc_bridge) && __has_feature(objc_bridge_id) && \
    __has_feature(objc_bridge_id_on_typedefs)
@class NSArray;
@class NSAttributedString;
@class NSString;
@class NSNull;
@class NSCharacterSet;
@class NSData;
@class NSDate;
@class NSTimeZone;
@class NSDictionary;
@class NSError;
@class NSLocale;
@class NSNumber;
@class NSSet;
@class NSURL;
#endif

#if defined(__OBJC__) && __has_attribute(objc_bridge) && __has_attribute(objc_bridge_mutable)
    #define CF_BRIDGED_TYPE(T) __attribute__((objc_bridge(T)))
    #define CF_BRIDGED_MUTABLE_TYPE(T) __attribute__((objc_bridge_mutable(T)))
    #define CF_RELATED_TYPE(T, C, I) __attribute__((objc_bridge_related(T, C, I)))
#else
    #define CF_BRIDGED_TYPE(T)
    #define CF_BRIDGED_MUTABLE_TYPE(T)
    #define CF_RELATED_TYPE(T, C, I)
#endif

#if __has_feature(assume_nonnull)
    #define CF_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
    #define CF_ASSUME_NONNULL_END _Pragma("clang assume_nonnull end")
#else
    #define CF_ASSUME_NONNULL_BEGIN
    #define CF_ASSUME_NONNULL_END
#endif

#if __has_attribute(noescape)
    #define CF_NOESCAPE __attribute__((noescape))
#else
    #define CF_NOESCAPE
#endif
#if __has_attribute(warn_unused_result)
    #define CF_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
    #define CF_WARN_UNUSED_RESULT
#endif
#if __has_attribute(not_tail_called)
    #define CF_NO_TAIL_CALL __attribute__((not_tail_called))
#else
    #define CF_NO_TAIL_CALL
#endif
#define CF_FORMAT_FUNCTION(F, A) __attribute__((format(__CFString__, F, A)))
#define CF_FORMAT_ARGUMENT(A) __attribute__((format_arg(A)))
#define CF_AUTOMATED_REFCOUNT_UNAVAILABLE
#define CF_UNAVAILABLE(_msg) __attribute__((unavailable(_msg)))

CF_EXTERN_C_BEGIN

typedef unsigned long CFTypeID;
typedef unsigned long CFOptionFlags;
typedef unsigned long CFHashCode;
typedef signed long CFIndex;

typedef const void *CFTypeRef;
typedef const struct CF_BRIDGED_TYPE(NSString) __CFString *CFStringRef;
typedef struct CF_BRIDGED_MUTABLE_TYPE(NSMutableString) __CFString *CFMutableStringRef;
typedef CFTypeRef CFPropertyListRef;

typedef CF_ENUM(CFIndex, CFComparisonResult) {
    kCFCompareLessThan = -1L,
    kCFCompareEqualTo = 0,
    kCFCompareGreaterThan = 1
};

typedef CFComparisonResult (*CFComparatorFunction)(const void *val1, const void *val2, void *context);

static const CFIndex kCFNotFound = -1;

typedef struct {
    CFIndex location;
    CFIndex length;
} CFRange;

CF_INLINE CFRange CFRangeMake(CFIndex loc, CFIndex len) {
    CFRange range;
    range.location = loc;
    range.length = len;
    return range;
}

CF_EXPORT CFRange __CFRangeMake(CFIndex loc, CFIndex len);

typedef const struct CF_BRIDGED_TYPE(NSNull) __CFNull *CFNullRef;

CF_EXPORT CFTypeID CFNullGetTypeID(void);
CF_EXPORT const CFNullRef kCFNull;

typedef const struct __CFAllocator *CFAllocatorRef;

CF_EXPORT const CFAllocatorRef kCFAllocatorDefault;
CF_EXPORT const CFAllocatorRef kCFAllocatorSystemDefault;
CF_EXPORT const CFAllocatorRef kCFAllocatorMalloc;
CF_EXPORT const CFAllocatorRef kCFAllocatorMallocZone;
CF_EXPORT const CFAllocatorRef kCFAllocatorNull;

typedef const void *(*CFAllocatorRetainCallBack)(const void *info);
typedef void (*CFAllocatorReleaseCallBack)(const void *info);
typedef CFStringRef (*CFAllocatorCopyDescriptionCallBack)(const void *info);
typedef void *(*CFAllocatorAllocateCallBack)(CFIndex allocSize, CFOptionFlags hint, void *info);
typedef void *(*CFAllocatorReallocateCallBack)(void *ptr, CFIndex newsize, CFOptionFlags hint, void *info);
typedef void (*CFAllocatorDeallocateCallBack)(void *ptr, void *info);
typedef CFIndex (*CFAllocatorPreferredSizeCallBack)(CFIndex size, CFOptionFlags hint, void *info);

typedef struct {
    CFIndex version;
    void *info;
    CFAllocatorRetainCallBack retain;
    CFAllocatorReleaseCallBack release;
    CFAllocatorCopyDescriptionCallBack copyDescription;
    CFAllocatorAllocateCallBack allocate;
    CFAllocatorReallocateCallBack reallocate;
    CFAllocatorDeallocateCallBack deallocate;
    CFAllocatorPreferredSizeCallBack preferredSize;
} CFAllocatorContext;

typedef unsigned long long CFAllocatorTypeID;

CF_EXPORT CFAllocatorRef CFAllocatorGetDefault(void);
CF_EXPORT CFAllocatorRef CFAllocatorCreate(CFAllocatorRef allocator, CFAllocatorContext *context);
CF_EXPORT void *CFAllocatorAllocateTyped(
    CFAllocatorRef allocator,
    CFIndex size,
    CFAllocatorTypeID descriptor,
    CFOptionFlags hint
);
CF_EXPORT void CFAllocatorDeallocate(CFAllocatorRef allocator, void *ptr);
CF_EXPORT CFIndex CFAllocatorGetPreferredSizeForSize(CFAllocatorRef allocator, CFIndex size, CFOptionFlags hint);

CF_EXPORT CFTypeID CFGetTypeID(CFTypeRef cf);
CF_EXPORT CFStringRef CFCopyTypeIDDescription(CFTypeID type_id);
CF_EXPORT CFTypeRef CFRetain(CFTypeRef cf);
CF_EXPORT void CFRelease(CFTypeRef cf);
CF_EXPORT CFTypeRef CFAutorelease(CFTypeRef CF_RELEASES_ARGUMENT arg);
CF_EXPORT CFIndex CFGetRetainCount(CFTypeRef cf);
CF_EXPORT Boolean CFEqual(CFTypeRef cf1, CFTypeRef cf2);
CF_EXPORT CFHashCode CFHash(CFTypeRef cf);
CF_EXPORT CFStringRef CFCopyDescription(CFTypeRef cf);
CF_EXPORT CFAllocatorRef CFGetAllocator(CFTypeRef cf);

CF_EXTERN_C_END
