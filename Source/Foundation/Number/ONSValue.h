//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include <OpenFoundation/Range/ONSRange.h>

#import <objc/NSObjCRuntime.h>
#import <objc/objc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

// The value unit of the Foundation image (iOS Simulator 24A434, arm64, UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03): NSValue, NSNumber, NSPlaceholderValue and NSPlaceholderNumber,
// restored in Foundation/Number/ONSValue.m. See that file's header comment for the organization.

// The zone type of the SDK's NSZone.h (struct _NSZone), declared at file scope for the prototypes below.
struct _NSZone;

// The geometry structures the NSValue methods take and return, laid out as the SDK's CGPoint, CGSize and
// CGRect (CGFloat is double on arm64). ValuePoint, ValueSize and ValueRect are local spellings: the
// CoreGraphics names are not this project's, and the type encodings the methods store use the native
// "{CGPoint=dd}" strings regardless.
typedef struct {
    double x;
    double y;
} ValuePoint;

typedef struct {
    double width;
    double height;
} ValueSize;

typedef struct {
    ValuePoint origin;
    ValueSize size;
} ValueRect;

// Native NSCachedTypeInfo records (__NSCachedTypeInfoRect64 0x2227d7a38 ... __NSCachedTypeInfoUIOffset64
// 0x2227d7ab8, __DATA_CONST,__const) and the records __NSNewValue allocates for other encodings: the byte
// size of the encoding, then the encoding. An allocated record stores its copy of the encoding right after
// these two words. CachedTypeInfo is a local descriptive name.
typedef struct {
    NSUInteger size;
    const char *type;
} CachedTypeInfo;

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSValue (__DATA,__objc_data 0x22e6de478, external), an NSObject subclass; class_ro_t
// 0x222862a50: flags 0, instance start and size 8, no ivars (iOS Simulator
// 24A434, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified). The method lists lie in the
// shared cache's optimized method lists; the methods come from the IDA capture
// (Local/Evidence/cf-core/iphone-simulator-foundation-link.json). The declarations below are the methods the
// restored units define or send; NSZone is spelled struct _NSZone, NSComparisonResult NSInteger.
// The native protocol-list reference 0x23d88fcf9 is outside the extracted image and remains unread;
// no guessed protocol-conformance metadata is emitted. iOS Device divergence: this family remains unread.
__attribute__((visibility("default")))
@interface ONSValue : NSObject

+ (id)allocWithZone:(struct _NSZone *)zone;
+ (id)value:(const void *)value withObjCType:(const char *)type;
+ (id)valueWithBytes:(const void *)value objCType:(const char *)type;
+ (id)valueWithWeakObject:(id)object;
+ (id)valueWithPoint:(ValuePoint)point;
+ (id)valueWithSize:(ValueSize)size;
+ (id)valueWithRect:(ValueRect)rect;
+ (id)valueWithRange:(ONSRange)range;
+ (BOOL)supportsSecureCoding;
- (id)init;
- (id)initWithBytes:(const void *)value objCType:(const char *)type;
- (id)initWithCoder:(id)coder;
- (const char *)objCType;
- (void)getValue:(void *)value;
- (void)getValue:(void *)value size:(NSUInteger)size;
- (BOOL)_matchType:(const char *)type size:(NSUInteger)size;
- (BOOL)_matchType:(const char *)type size:(NSUInteger)size strict:(BOOL)strict;
- (id)copyWithZone:(struct _NSZone *)zone;
- (BOOL)isEqualToValue:(id)value;
- (id)nonretainedObjectValue;
- (ValuePoint)pointValue;
- (ValueSize)sizeValue;
- (ValueRect)rectValue;
- (ONSRange)rangeValue;
- (Class)classForCoder;
- (void)encodeWithCoder:(id)coder;

@end

#endif

// Native __NSConcreteValueClass, 0x22e775b10 (__DATA,__common, was a private external): the NSConcreteValue
// class, cached by __NSNewValue on first use. Defined in Foundation/Number/ONSValue.m.
// Atomic pointer-width storage fixes the native cache race without changing the symbol layout.
OF_EXTERN _Atomic(uintptr_t) __NSConcreteValueClass;
