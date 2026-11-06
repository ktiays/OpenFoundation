//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/URL/OCFURLAllowedCharacters.h"

#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"
#include "CoreFoundation/Collections/OCFData.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#include <OpenFoundation/Range/ONSRange.h>

#include <OpenCoreFoundation/OCFBuild.h>
#include <dispatch/dispatch.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <stdint.h>

// Shared declarations of the NSCharacterSet unit (String/ONSCharacterSet.m), iOS Simulator 24A434, arm64,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless a declaration says
// otherwise. iOS Device 24A446: unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

// The inline feature check _foundation_swift_characterSet_enabled (the native out-of-line copy 0x1c051a370,
// was a private external, is unread): its statics and its dispatch_once block are shared by every Foundation
// unit that expands it (the NSCharacterSet methods here and -[NSString(NSURLUtilities)
// stringByAddingPercentEncodingWithAllowedCharacters:] of the NSString family), so they are hidden symbols
// under their native names, defined by String/ONSCharacterSet.m.
//
// Native _foundation_swift_characterSet_enabled.onceToken, 0x22e7761f8 (__DATA,__common, was a private
// external): the dispatch_once predicate.
extern __attribute__((visibility("hidden"))) dispatch_once_t
    foundationSwiftCharacterSetOnceToken __asm("__foundation_swift_characterSet_enabled.onceToken");

// Native _foundation_swift_characterSet_enabled.featureEnabled, 0x22e776200 (__DATA,__common, was a private
// external): the flag the block stores; the callers load it as a byte and compare it with 1.
extern __attribute__((visibility("hidden"))) uint8_t
    foundationSwiftCharacterSetFeatureEnabled __asm("__foundation_swift_characterSet_enabled.featureEnabled");

// Native ___block_literal_global (IDA ___block_literal_global_10), 0x22281c8d8 (__DATA_CONST,__const,
// non-external): the global block the expansions pass to dispatch_once, {_NSConcreteGlobalBlock, 0x50000000,
// 0, invoke 0x1c0fdc9cc, descriptor 0x22281c8b8 {0x50000000, 32, "v8@?0", no layout}} (read with
// Local/Scratch/cf-core-fc/fmacho.py). Defined by String/ONSCharacterSet.m under this hidden name; declared
// as a byte array so that the header needs no block layout.
OF_EXTERN const uint8_t foundationSwiftCharacterSetEnabledBlock[] __asm("___block_literal_global_10");

// The expansion every caller inlines: the once check (dispatch_once with the shared block when the predicate
// is not yet -1) and the byte comparison with 1. foundationSwiftCharacterSetEnabled is a local descriptive
// name.
static inline BOOL foundationSwiftCharacterSetEnabled(void) {
    dispatch_once(
        &foundationSwiftCharacterSetOnceToken,
        (__bridge dispatch_block_t)(void *)foundationSwiftCharacterSetEnabledBlock
    );
    return foundationSwiftCharacterSetFeatureEnabled == 1;
}

#ifdef __OBJC__
#import <objc/NSObject.h>

// Classes of this image the unit references by their class symbols, restored by other families or not in
// scope: the Swift-backed character set the methods allocate when the feature is enabled, the cluster's
// placeholder (-invertedSet), the keyed coders the coding methods compare the coder's class with, the error
// and string classes, and the CoreFoundation classes reached through the shared cache's GOT (NSException,
// NSDictionary, NSData, NSMutableData).

// Native class NSCharacterSet (__DATA,__objc_data 0x22e6d6958, external), an NSObject subclass (the
// superclass pointer 0x22e6b6088 lies outside the image: libobjc's NSObject); class_ro_t 0x222849a98: flags
// 0, instance start and size 8, no ivars (iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified). Only the methods the scope reaches are
// restored (String/ONSCharacterSet.m), with the categories NSURLUtilities (+URLPathAllowedCharacterSet) and
// NSCharacterSetPortCoding (-replacementObjectForPortCoder:); the other methods, the protocol conformances
// and the property list are unread and not declared.
__attribute__((visibility("default")))
@interface ONSCharacterSet : NSObject

- (unsigned long)_cfTypeID;
- (OCFCharacterSetRef)_expandedCFCharacterSet;
+ (id)whitespaceCharacterSet;
+ (id)whitespaceAndNewlineCharacterSet;
+ (id)decimalDigitCharacterSet;
+ (id)alphanumericCharacterSet;
+ (id)characterSetWithCharactersInString:(id)aString;
- (id)init;
- (NSUInteger)hash;
- (id)copyWithZone:(struct _NSZone *)zone;
- (id)invertedSet;
- (BOOL)isEmpty;
- (NSUInteger)count;
- (BOOL)isMutable;
- (void)makeImmutable;
+ (id)lowercaseLetterCharacterSet;
- (BOOL)characterIsMember:(unsigned short)aCharacter;
- (BOOL)longCharacterIsMember:(uint32_t)theLongChar;
- (BOOL)isSupersetOfSet:(id)theOtherSet;
- (BOOL)isEqual:(id)object;
- (id)bitmapRepresentation;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
- (void)encodeWithCoder:(id)coder;
- (id)initWithCoder:(id)coder;
+ (BOOL)supportsSecureCoding;
- (id)_retainedBitmapRepresentation;

@end

// Native _NSPlaceholderCharacterSet, 0x22e6d6908, non-external. class_ro_t 0x222849cf0
// gives superclass NSCharacterSet, instance start 8 and size 28.
__attribute__((visibility("hidden")))
@interface _ONSPlaceholderCharacterSet : ONSCharacterSet {
    @private
    ONSCharacterSet *_original;
    ONSCharacterSet *_invertedSet;

    struct {
        unsigned _inverted : 1;
        unsigned _builtin : 1;
        unsigned _isCF : 1;
        unsigned _reserved : 29;
    } _flags;
}
- (id)initWithSet:(id)set options:(NSUInteger)options;
- (void)_expandInverted;
@end

@interface ONSCharacterSet (NSURLUtilities)
+ (id)URLPathAllowedCharacterSet;
@end

@interface ONSCharacterSet (NSCharacterSetPortCoding)
- (id)replacementObjectForPortCoder:(id)coder;
@end
#endif
