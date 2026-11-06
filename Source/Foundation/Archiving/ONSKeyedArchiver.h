//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>

#include "CoreFoundation/PropertyList/OCFBinaryPList.h"
#include "CoreFoundation/PropertyList/OCFKeyedArchiverUID.h"

#include <stdint.h>

// Native __NSCoderCheckPotentiallyMangledClassNameAndWarn, 0x1c0556fbc (was a private external; defined in
// Archiving/ONSKeyedArchiver.m): for a class whose runtime name starts "_T" (a mangled Swift name), asks
// +[NSKeyedUnarchiver _swift_checkClassAndWarnForKeyedArchiving:operation:] to warn, the operation being the
// 32-bit value archiving XOR 1 (the captured calls pass 0 or 1).
OF_EXTERN void _NSCoderCheckPotentiallyMangledClassNameAndWarn(Class cls, int archiving);

#ifdef __OBJC__
#import <objc/NSObject.h>

#import "Foundation/Archiving/ONSCoder.h"

@class ONSKeyedArchiver;
@class ONSKeyedUnarchiver;
@class _ONSKeyedUnarchiverHelper;
@class ONSMutableArray;
@protocol ONSKeyedArchiverDelegate;

// Native class NSKeyedArchiver (__DATA,__objc_data 0x22e6dadf0, external), an NSCoder subclass; class_ro_t
// 0x222857290: instance start 8, size 128; fifteen eight-byte ivars with their native type encodings (iOS
// Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified). The native
// offset symbols are merged into __MergedGlobals (non-external). The ivars are @public so that the unit's C
// functions, which the native code places among both classes' methods, reach them; visibility has no ABI
// effect. iOS Device 24A446: unread. Reading: Source/Foundation/Archiving/Reading.md.
__attribute__((visibility("default")))
@interface ONSKeyedArchiver : ONSCoder {
    @public
    void *_stream;                               // 8, ^v
    _Atomic(unsigned long long) _flags;          // 16, AQ
    id<ONSKeyedArchiverDelegate> _delegate;      // 24, @"<NSKeyedArchiverDelegate>"
    ONSMutableArray *_containers;                // 32, @"NSMutableArray"
    ONSMutableArray *_objects;                   // 40, @"NSMutableArray"
    struct __OCFDictionary *_objRefMap;          // 48, ^{__CFDictionary=}
    struct __OCFDictionary *_replacementMap;     // 56, ^{__CFDictionary=}
    struct __OCFDictionary *_classNameMap;       // 64, ^{__CFDictionary=}
    struct __OCFDictionary *_conditionals;       // 72, ^{__CFDictionary=}
    struct __OCFDictionary *_classes;            // 80, ^{__CFDictionary=}
    unsigned long long _genericKey;              // 88, Q
    const struct __OCFKeyedArchiverUID **_cache; // 96, r^^{__CFKeyedArchiverUID}
    unsigned long long _cacheSize;               // 104, Q
    unsigned long long _estimatedCount;          // 112, Q
    struct __OCFSet *_visited;                   // 120, ^{__CFSet=}
}

- (id)_initWithOutput:(id)output;
- (BOOL)allowsKeyedCoding;
- (void)replaceObject:(id)object withObject:(id)replacement;
- (void)_encodePropertyList:(id)plist forKey:(id)key;
- (void)encodeObject:(id)object;
- (void)encodeConditionalObject:(id)object;
- (void)encodeBycopyObject:(id)object;
- (void)encodeBytes:(const void *)bytes length:(NSUInteger)length;
- (void)encodeValuesOfObjCTypes:(const char *)types, ...;
- (void)encodeValueOfObjCType:(const char *)type at:(const void *)address;
- (void)encodeArrayOfObjCType:(const char *)type count:(NSUInteger)count at:(const void *)address;
- (NSInteger)versionForClassName:(id)name;

- (id)initRequiringSecureCoding:(BOOL)requiresSecureCoding;
+ (id)archivedDataWithRootObject:(id)object
           requiringSecureCoding:(BOOL)requiresSecureCoding
                           error:(id *)error;
- (id)init;
- (id)delegate;
- (void)setDelegate:(id)delegate;
- (NSUInteger)outputFormat;
- (id)encodedData;
- (void)finishEncoding;
+ (void)setClassName:(id)codedName forClass:(Class)cls;
- (void)setClassName:(id)codedName forClass:(Class)cls;
+ (id)classNameForClass:(Class)cls;
- (id)classNameForClass:(Class)cls;
- (void)encodeObject:(id)object forKey:(id)key;
- (void)encodeConditionalObject:(id)object forKey:(id)key;
- (void)encodeBool:(BOOL)value forKey:(id)key;
- (void)encodeInt:(int)value forKey:(id)key;
- (void)encodeInt32:(int32_t)value forKey:(id)key;
- (void)encodeInt64:(int64_t)value forKey:(id)key;
- (void)encodeFloat:(float)value forKey:(id)key;
- (void)encodeDouble:(double)value forKey:(id)key;
- (void)encodeBytes:(const uint8_t *)bytes length:(NSUInteger)length forKey:(id)key;
- (BOOL)requiresSecureCoding;
- (void)setRequiresSecureCoding:(BOOL)requiresSecureCoding;

@end

#endif
