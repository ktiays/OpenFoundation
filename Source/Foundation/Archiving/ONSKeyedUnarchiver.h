//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/PropertyList/OCFBinaryPList.h"

#include <stdint.h>

// The record NSKeyedUnarchiver's _offsetData points to: its type encoding ^{?={?=[5C]CCCQQQ}QQ} is the binary
// property-list trailer as __CFBinaryPlistGetTopLevelInfo fills it (BinaryPlistTrailer), then two 64-bit
// words. KeyedUnarchiverOffsetData and its field names are local descriptive spellings; the two trailing
// words are named in Archiving/ONSKeyedUnarchiver.m where they are read.
typedef struct {
    BinaryPlistTrailer trailer;
    uint64_t containerOffset;
    uint64_t objectsOffset;
} KeyedUnarchiverOffsetData;

_Static_assert(sizeof(KeyedUnarchiverOffsetData) == 48, "The unarchiver offset record is 48 bytes.");

#ifdef __OBJC__
#import "Foundation/Archiving/ONSCoder.h"

@class _ONSKeyedUnarchiverHelper;
@protocol ONSKeyedUnarchiverDelegate;

// Native class NSKeyedUnarchiver (__DATA,__objc_data 0x22e6dadc8, external), an NSCoder subclass; class_ro_t
// 0x2228576d0: instance start 8, size 136; sixteen ivars with their native type encodings (_genericKey a
// four-byte int at 64, the others eight bytes) (iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified). @public as for NSKeyedArchiver.
// iOS Device 24A446: unread. Reading: Source/Foundation/Archiving/Reading.md.
__attribute__((visibility("default")))
@interface ONSKeyedUnarchiver : ONSCoder {
    @public
    id<ONSKeyedUnarchiverDelegate> _delegate; // 8, @"<NSKeyedUnarchiverDelegate>"
    _Atomic(unsigned long long) _flags;       // 16, AQ
    struct __OCFDictionary *_objRefMap;       // 24, ^{__CFDictionary=}
    struct __OCFDictionary *_replacementMap;  // 32, ^{__CFDictionary=}
    struct __OCFDictionary *_nameClassMap;    // 40, ^{__CFDictionary=}
    struct __OCFDictionary *_tmpRefObjMap;    // 48, ^{__CFDictionary=}
    struct __OCFDictionary *_refObjMap;       // 56, ^{__CFDictionary=}
    int _genericKey;                          // 64, i
    KeyedUnarchiverOffsetData *_offsetData;   // 72, ^{?={?=[5C]CCCQQQ}QQ}
    struct __OCFArray *_containers;           // 80, ^{__CFArray=}
    struct __OCFArray *_objects;              // 88, ^{__CFArray=}
    struct __OCFData *_data;                  // 96, ^{__CFData=}
    const char *_bytes;                       // 104, r*
    unsigned long long _len;                  // 112, Q
    _ONSKeyedUnarchiverHelper *_helper;       // 120, @"_NSKeyedUnarchiverHelper"
    struct __OCFDictionary *_offsetObjMap;    // 128, ^{__CFDictionary=}
}

- (id)_initWithStream:(void *)stream data:(id)data topDict:(id)topDict;
- (id)_initForReadingFromData:(id)data error:(id *)error throwLegacyExceptions:(BOOL)throwLegacyExceptions;
- (BOOL)allowsKeyedCoding;
- (void)_temporaryMapReplaceObject:(id)object withObject:(id)replacement;
- (void)replaceObject:(id)object withObject:(id)replacement;
- (void)_replaceObject:(id)object withObject:(id)replacement;
- (BOOL)_validateAllowedClassesContainsClass:(Class)cls forKey:(id)key;
- (BOOL)_validateAllowedClassesContainsClassOfReference:(id)object forKey:(id)key;
- (BOOL)_validatePropertyListClass:(Class)cls forKey:(id)key __attribute__((objc_direct));
- (id)_decodeObjectOfClass:(Class)cls;
- (id)_decodeCollectionOfClass:(Class)cls
                allowedClasses:(id)classes
                        forKey:(id)key __attribute__((objc_direct));
- (BOOL)_strictSecureDecodingEnabled;
- (void)_allowDecodingCyclesInSecureMode;
- (void)__setError:(id)error;
- (id)_decodePropertyListForKey:(id)key;
- (BOOL)_containsNextUnkeyedObject;
- (id)decodeObject;
- (id)decodeDataObject;
- (void *)decodeBytesWithReturnedLength:(NSUInteger *)length;
- (void)decodeValuesOfObjCTypes:(const char *)types, ...;
- (void)decodeValueOfObjCType:(const char *)type at:(void *)address;
- (void)decodeArrayOfObjCType:(const char *)type count:(NSUInteger)count at:(void *)address;
- (NSInteger)versionForClassName:(id)name;

- (id)init;
- (id)initForReadingFromData:(id)data error:(id *)error;
+ (id)unarchivedObjectOfClass:(Class)cls fromData:(id)data error:(id *)error;
+ (id)unarchivedObjectOfClasses:(id)classes fromData:(id)data error:(id *)error;
- (id)delegate;
- (void)setDelegate:(id)delegate;
- (void)finishDecoding;
+ (void)setClass:(Class)cls forClassName:(id)codedName;
- (void)setClass:(Class)cls forClassName:(id)codedName;
+ (Class)classForClassName:(id)codedName;
- (Class)classForClassName:(id)codedName;
- (BOOL)containsValueForKey:(id)key;
- (id)decodeObjectForKey:(id)key;
- (id)decodeObjectOfClass:(Class)cls forKey:(id)key;
- (id)decodeObjectOfClasses:(id)classes forKey:(id)key;
- (id)decodeArrayOfObjectsOfClasses:(id)classes forKey:(id)key;
- (BOOL)decodeBoolForKey:(id)key;
- (int)decodeIntForKey:(id)key;
- (int32_t)decodeInt32ForKey:(id)key;
- (int64_t)decodeInt64ForKey:(id)key;
- (float)decodeFloatForKey:(id)key;
- (double)decodeDoubleForKey:(id)key;
- (const uint8_t *)decodeBytesForKey:(id)key returnedLength:(NSUInteger *)lengthp;
- (id)allowedClasses;
- (BOOL)requiresSecureCoding;
- (void)setRequiresSecureCoding:(BOOL)requiresSecureCoding;
- (id)error;
- (NSInteger)decodingFailurePolicy;
- (void)setDecodingFailurePolicy:(NSInteger)decodingFailurePolicy;

@end
#endif
