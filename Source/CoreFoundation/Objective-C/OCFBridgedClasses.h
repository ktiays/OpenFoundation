//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Locale/OCFLocale.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/RunLoop/OCFRunLoop.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/Stream/OCFConcreteStreams.h"
#include "CoreFoundation/Stream/OCFStream.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/URL/OCFURL.h"
#include "CoreFoundation/URL/OCFURLCoreServicesForwarding.h"
#include "CoreFoundation/String/OCFAttributedString.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"

#include <objc/objc.h>
#include <stdbool.h>
#include <stdint.h>

// Shared declarations of the bridged Objective-C classes that live in CoreFoundation (the __NSCF* classes,
// the NSBlock family, NSNull, NSURL's CoreFoundation part, NSInvocation, NSMachPort and __NSArray0), iOS
// Simulator 24A434, arm64, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. Each class has its own translation unit
// under Objective-C/; see each unit's header comment for its organization. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet on this branch).

// The check every bridged collection method inlines: load the hook, and when it is set, call it with the
// object, the method's return address (X30 natively) and the tag. tsanRead and tsanWrite are local
// descriptive names.
static inline void tsanRead(const void *object, void *returnAddress, void *tag) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanReadFunction;
    if (hook != NULL) {
        hook(object, returnAddress, tag);
    }
}

static inline void tsanWrite(const void *object, void *returnAddress, void *tag) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanWriteFunction;
    if (hook != NULL) {
        hook(object, returnAddress, tag);
    }
}

// Interfaces of families not yet restored on this branch.

// String: native _CFNonObjCStringGetCStringPtr, 0x1c0210350 (was a private external).

// Collections (external natively unless noted).
// Native __CFDictionaryGetKeyCallbacks and __CFDictionaryGetValueCallbacks (were private externals); the
// records are returned by value (through X8).
// Native _CFNonObjCArrayGetCount, 0x1c014c0b0 (was a private external).
// Native __CFSetGetCallbacks (was a private external); the record is returned by value (through X8).
// Native _CFDataGetBytePtrNonObjC (was a private external).

// Native __CFFireTimer, 0x1c01b4bb8 (was a private external), the callout of the timers -[__NSCFTimer
// initWithFireDate:...] creates; RunLoop/OCFRunLoop.c declares it for its callout check.
OF_EXTERN void __CFFireTimer(OCFRunLoopTimerRef rlt, void *info);

// Blocks runtime (libsystem_blocks, supplied through libSystem; never reimplemented). Block.h declares
// _Block_copy and _Block_release; the SDK installs no declarations for these three.
extern bool _Block_tryRetain(const void *aBlock);
extern bool _Block_isDeallocating(const void *aBlock);
extern const char *_Block_signature(void *aBlock);

#ifdef __OBJC__
#import <objc/NSObject.h>

#import "CoreFoundation/Objective-C/OCFNSData.h"
#import "CoreFoundation/Objective-C/OCFNSInputStream.h"
#import "CoreFoundation/Objective-C/OCFNSLocale.h"
#import "CoreFoundation/Objective-C/OCFNSMutableArray.h"
#import "CoreFoundation/Objective-C/OCFNSMutableData.h"
#import "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"
#import "CoreFoundation/Objective-C/OCFNSMutableSet.h"
#import "CoreFoundation/Objective-C/OCFNSOutputStream.h"
#import "CoreFoundation/Objective-C/OCFNSPort.h"
#import "CoreFoundation/Objective-C/OCFNSTimer.h"

// The restored methods spell NSRange parameters and results as OCFRange: two 64-bit words passed in X
// registers, the layout and passing the native code uses (NSRange is Foundation's; the method type strings
// that would spell it lie outside the extracted image, unread).

// Classes outside this restoration that the restored methods reference by name (class symbols and runtime
// names), so that the prefix tool maps them.
@class __ONSFastEnumerationEnumerator;
@class __ONSCFStreamWeakDelegateWrapper;
@class ONSKeyedArchiver;
@class ONSMethodSignature;

// Superclasses of the restored classes. Those in this image (NSArray, NSMutableArray, NSDictionary,
// NSMutableDictionary, NSSet, NSMutableSet, NSData, NSMutableData, NSLocale, NSTimer, NSStream,
// NSInputStream, NSOutputStream and NSPort) are declared by their own headers, imported above. NSString,
// NSMutableString, NSValue, NSNumber, NSAttributedString, NSMutableAttributedString, NSCharacterSet,
// NSMutableCharacterSet and NSError live outside the image (the subclasses' superclass pointers 0x22e6db6b0,
// 0x22e6de400, 0x22e6d61b0, 0x22e6d6980 and 0x22e6d78d0 are not resolvable in it); their identity is the
// SDK's class hierarchy (confidence: medium) and NSError's 32 bytes of ivars come from
// __NSCFError's instance start of 40. Only the methods the restored code sends to super or to these classes
// are declared.
@interface ONSString : NSObject

- (const char *)cString;
- (const char *)UTF8String;
- (NSUInteger)cStringLength;
- (const char *)cStringUsingEncoding:(NSUInteger)encoding;

@end

@interface ONSMutableString : ONSString

- (NSUInteger)replaceOccurrencesOfString:(id)target
                              withString:(id)replacement
                                 options:(NSUInteger)options
                                   range:(OCFRange)range;

@end

@interface ONSAttributedString : NSObject
@end

@interface ONSMutableAttributedString : ONSAttributedString
@end

@interface ONSValue : NSObject
@end

@interface ONSNumber : ONSValue

- (id)descriptionWithLocale:(id)locale;
- (id)stringValue;

@end

@interface ONSCharacterSet : NSObject
@end

@interface ONSMutableCharacterSet : ONSCharacterSet
@end

@interface ONSError : NSObject {
    uint8_t _unreadIvars[32];
}
@end

// -copyWithZone: of NSData (Foundation's NSCopying method on this image's class); the bridged data's super
// sends are declared here.
@interface ONSData (CopyingDeclarations)
- (id)copyWithZone:(struct _NSZone *)zone;
@end

// Selectors the restored methods send to objects of classes outside this restoration (or to their own
// objects through id), declared for the message sends only; no protocol object is emitted.
@protocol BridgedClassMessages

- (id)initWithObject:(id)object;
- (id)initWithCoder:(id)coder;
- (id)initWithDomain:(id)domain code:(NSInteger)code userInfo:(id)userInfo;
- (id)domain;
- (NSInteger)code;
- (id)userInfo;
- (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
- (BOOL)allowsKeyedCoding;
- (void)encodeInt64:(int64_t)value forKey:(id)key;
- (void)encodeInteger:(NSInteger)value forKey:(id)key;
- (void)encodeBool:(BOOL)value forKey:(id)key;
- (void)encodeObject:(id)object forKey:(id)key;
- (void)_encodePropertyList:(id)propertyList forKey:(id)key;
- (id)bitmapRepresentation;
- (BOOL)isNSString__;
- (NSUInteger)length;
- (BOOL)isEqualToString:(id)string;
- (NSInteger)compare:(id)other;
- (BOOL)isEqualToNumber:(id)number;
- (const char *)objCType;
- (char)charValue;
- (short)shortValue;
- (int)intValue;
- (unsigned int)unsignedIntValue;
- (long)longValue;
- (unsigned long)unsignedLongValue;
- (long long)longLongValue;
- (unsigned long long)unsignedLongLongValue;
- (float)floatValue;
- (double)doubleValue;
- (BOOL)boolValue;
- (id)descriptionWithLocale:(id)locale;
- (id)dateWithTimeIntervalSinceReferenceDate:(double)interval;
- (double)timeIntervalSinceReferenceDate;
- (BOOL)isValid;
- (double)timeInterval;
- (void)invalidate;
- (id)initFileURLWithPath:(id)path;
- (id)initWithURL:(id)url;
- (id)initWithURL:(id)url append:(BOOL)shouldAppend;
- (id)initWithDelegate:(id)delegate;
- (id)retainedDelegate;
- (void)stream:(id)stream handleEvent:(NSUInteger)eventCode;
- (OCFRunLoopRef)getCFRunLoop;
- (void)insertObject:(id)object atIndex:(NSUInteger)index;
- (void)removeObjectAtIndex:(NSUInteger)index;
- (NSUInteger)frameLength;
- (BOOL)isFileURL;
- (BOOL)isFileReferenceURL;
- (id)query;
- (id)absoluteString;
- (id)_cfurl;
- (unsigned int)_queryResolveFlags;
- (unsigned int)_pathResolveFlags;
- (id)_urlStringByInsertingQueryResolveFlags:(unsigned int)flags;
- (id)_urlStringByInsertingPathResolveFlags:(unsigned int)flags;
- (id)_urlStringByRemovingResolveFlags;

@end

// The body -[__NSCFNumber compare:] (0x1c01b3ad4) and -[__NSCFBoolean compare:] (0x1c01b414c) share
// instruction for instruction, with their .cold.1 fragments (0x1c0308e8c, 0x1c0308ea4: "%@: nil argument"
// through OUTLINED_FUNCTION_0_17 (0x1c01b59c0: NSInvalidArgumentException into X19, then
// __CFExceptionProem(X0, X1), the receiver and selector the parent leaves there) and _1_12 (0x1c01b59d0: the
// raise with "%@: nil argument"); assembly read in iphone-simulator-corpus-delta3.json, confidence:
// verified). The argument's -objCType is read before the receiver's. When either type
// begins with 'd' or 'f' ((c & ~2) == 'd') the -doubleValue results (receiver first) are compared: a NaN
// receiver is equal to a NaN argument, else greater when the argument's sign bit is set and less otherwise; a
// NaN argument answers -1 for a receiver whose sign bit is set and 1 otherwise. Two 'Q' types compare their
// -unsignedLongLongValue results; otherwise a 'Q' receiver above INT64_MAX is greater and a 'Q' argument
// above INT64_MAX is greater than the receiver, and the -longLongValue results compare signed. The IDA
// pseudocode drops the NaN branches; the assembly settles them. numberCompare is a local descriptive name.
static inline NSInteger numberCompare(id self, SEL _cmd, id other) {
    if (other == nil) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException, OF_STR("%@: nil argument"), __CFExceptionProem(self, _cmd)
        );
    }
    if (self == other) {
        return 0;
    }
    unsigned char otherType = *(const unsigned char *)[other objCType];
    const unsigned char *selfType = (const unsigned char *)[self objCType];
    if ((otherType & 0xFFFFFFFD) == 'd' || (selfType[0] & 0xFFFFFFFD) == 'd') {
        double value = [self doubleValue];
        double otherValue = [other doubleValue];
        if (value != value) {
            if (otherValue != otherValue) {
                return 0;
            }
            int64_t bits;
            __builtin_memcpy(&bits, &otherValue, sizeof(bits));
            return bits <= -1 ? 1 : -1;
        }
        if (otherValue != otherValue) {
            int64_t bits;
            __builtin_memcpy(&bits, &value, sizeof(bits));
            return (bits >> 63) | 1;
        }
        if (value < otherValue) {
            return -1;
        }
        return otherValue < value ? 1 : 0;
    }
    unsigned char type = selfType[0];
    if (otherType == 'Q' && type == 'Q') {
        unsigned long long value = [self unsignedLongLongValue];
        unsigned long long otherValue = [other unsignedLongLongValue];
        if (value < otherValue) {
            return -1;
        }
        return value > otherValue ? 1 : 0;
    }
    if (type == 'Q' && (int64_t)[self unsignedLongLongValue] < 0) {
        return 1;
    }
    if (otherType == 'Q' && (int64_t)[other unsignedLongLongValue] < 0) {
        return -1;
    }
    long long value = [self longLongValue];
    long long otherValue = [other longLongValue];
    if (value < otherValue) {
        return -1;
    }
    return value > otherValue ? 1 : 0;
}
#endif
