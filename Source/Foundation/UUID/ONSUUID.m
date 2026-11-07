//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/UUID/ONSUUID.h"
#import "Foundation/Error/ONSError.h"
#import "Foundation/Error/ONSErrorConstants.h"

#include "CoreFoundation/Base/OCFHashBytes.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/OFPortHooks.h"
#include <objc/runtime.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured assembly, except explicitly marked bindings.
// iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Scratch/cf-core-fd-misc/Reading.md; native source paths are unread, organization inferred.

extern Class objc_opt_class(id object);
extern id objc_opt_self(id object);
extern BOOL objc_opt_isKindOfClass(id object, Class cls);
extern bool dyld_program_sdk_at_least(uint64_t version);

// Native __NSConcreteUUID (metadata accessor 0x1c0c46aec): the Swift class of UUID/UUID.swift.
extern char uuidClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSUUID));
extern char concreteUUIDClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSConcreteUUID));
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char dictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDictionary));

@protocol UUIDMessages
- (BOOL)allowsKeyedCoding;
- (void)encodeBytes:(const unsigned char *)bytes length:(NSUInteger)length forKey:(id)key;
- (const unsigned char *)decodeBytesForKey:(id)key returnedLength:(NSUInteger *)length;
- (void)failWithError:(id)error;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)info;
+ (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
@end

@implementation ONSUUID

// Native +[NSUUID _compatibilityBehavior], 0x1c06b1228. Non-external. YES for programs built with an SDK
// before the version 0x7E10901FFFFFFFF passes (dyld_program_sdk_at_least).
+ (BOOL)_compatibilityBehavior {
    return OF_PORT_ANSWER(
               programSDKAtLeastPacked, dyld_program_sdk_at_least(0x7E10901FFFFFFFFULL), 0x7E10901FFFFFFFFULL
           ) ^
           1;
}

// Native +[NSUUID allocWithZone:], 0x1c06b1248. External class, non-external method.
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == (Class)uuidClass || self == objc_opt_class((id)concreteUUIDClass)) {
        return ONSAllocateObject((Class)objc_opt_self((id)concreteUUIDClass), 0, zone);
    }
    return [super allocWithZone:zone];
}

// Native +[NSUUID UUID], 0x1c06b1324. Non-external method.
+ (id)UUID {
    return [[[self alloc] init] autorelease];
}

// Native -[NSUUID init], 0x1c06b1338. Non-external method.
- (id)init {
    return [super init];
}

// Native -[NSUUID initWithUUIDString:], 0x1c06b1398. Non-external method.
// Native abstract-class nil result (kept); no release occurs in the native body.
- (id)initWithUUIDString:(id)string {
    return nil;
}

// Native -[NSUUID initWithUUIDBytes:], 0x1c06b13a0. Non-external method.
// Native abstract-class nil result (kept), with ownership unchanged.
- (id)initWithUUIDBytes:(const unsigned char *)bytes {
    return nil;
}

// Native -[NSUUID hash], 0x1c06b13a8. Non-external method.
- (NSUInteger)hash {
    unsigned char bytes[16] = {0};
    [self getUUIDBytes:bytes];
    return OCFHashBytes(bytes, 16);
}

// Native -[NSUUID isEqual:], 0x1c06b1404. Non-external method.
- (BOOL)isEqual:(id)other {
    if (self == other) {
        return YES;
    }
    if ((objc_opt_isKindOfClass(other, (Class)uuidClass) & 1) == 0) {
        return NO;
    }
    uint64_t left[2] = {0};
    [self getUUIDBytes:(unsigned char *)left];
    uint64_t right[2] = {0};
    [other getUUIDBytes:(unsigned char *)right];
    return left[0] == right[0] && left[1] == right[1];
}

// Native -[NSUUID compare:], 0x1c06b14b8. Non-external method.
// Reverse traversal and signed arithmetic shift preserve the native comparison accumulator.
- (ONSComparisonResult)compare:(ONSUUID *)other {
    if (self == other) {
        return ONSOrderedSame;
    }
    unsigned char left[16] = {0};
    [self getUUIDBytes:left];
    unsigned char right[16] = {0};
    [other getUUIDBytes:right];
    int comparison = 0;
    for (int index = 15; index >= 0; index--) {
        int difference = left[index] - right[index];
        comparison = (comparison & (((difference - 1) & ~difference) >> 8)) | difference;
    }
    return comparison < 0 ? ONSOrderedAscending : (comparison > 0 ? ONSOrderedDescending : ONSOrderedSame);
}

// Native -[NSUUID getUUIDBytes:], 0x1c06b1580. Non-external method. Native base-class zero UUID (kept).
- (void)getUUIDBytes:(unsigned char *)bytes {
    memset(bytes, 0, 16);
}

// Native -[NSUUID UUIDString], 0x1c06b1588. Non-external method; constant object 0x22281e130 is empty.
- (id)UUIDString {
    return (id)OF_STR("");
}

// Native -[NSUUID copyWithZone:], 0x1c06b15c4. Non-external method.
- (id)copyWithZone:(struct _NSZone *)zone {
    unsigned char bytes[16] = {0};
    [self getUUIDBytes:bytes];
    return [[(id)objc_opt_class(self) allocWithZone:zone] initWithUUIDBytes:bytes];
}

// Native +[NSUUID supportsSecureCoding], 0x1c06b163c. Non-external method.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSUUID encodeWithCoder:], 0x1c06b1644. Non-external method.
// External uniqued GOT bindings 0x22227fae0/0x22227f500: NSException/NSInvalidArgumentException (medium).
- (void)encodeWithCoder:(id)coder {
    if (([coder allowsKeyedCoding] & 1) == 0) {
        @throw [(id)exceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:(id)OF_STR(
                                  "-[NSUUID encodeWithCoder]: NSUUIDs cannot be encoded by non-keyed coders"
                              )
                     userInfo:nil];
    }
    unsigned char bytes[16] = {0};
    [self getUUIDBytes:bytes];
    [coder encodeBytes:bytes length:16 forKey:(id)OF_STR("NS.uuidbytes")];
}

// Native -[NSUUID initWithCoder:], 0x1c06b16f4. Non-external method.
// SDK gate 0x007e10901ffffffff is passed byte-for-byte to libdyld. Dictionary GOT 0x22227fad0 (medium).
- (id)initWithCoder:(id)coder {
    if (([coder allowsKeyedCoding] & 1) == 0) {
        @throw [(id)exceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:(id)OF_STR(
                                  "-[NSUUID initWithCoder]: NSUUIDs cannot be decoded by non-keyed coders"
                              )
                     userInfo:nil];
    }
    NSUInteger length = 0;
    const unsigned char *bytes = [coder decodeBytesForKey:(id)OF_STR("NS.uuidbytes") returnedLength:&length];
    if (bytes != NULL && length == 16) {
        return [self initWithUUIDBytes:bytes];
    }
    if (!OF_PORT_ANSWER(
            programSDKAtLeastPacked,
            dyld_program_sdk_at_least(UINT64_C(0x7e10901ffffffff)),
            UINT64_C(0x7e10901ffffffff)
        )) {
        return [self init];
    }
    id key = (id)ONSLocalizedDescriptionKey;
    id description = (id)(bytes != NULL ? OF_STR("UUID bytes were not the expected length")
                                        : OF_STR("UUID bytes not found in archive"));
    id info = [(id)dictionaryClass dictionaryWithObjects:&description forKeys:&key count:1];
    [coder failWithError:[ONSError errorWithDomain:(id)ONSCocoaErrorDomain
                                              code:bytes != NULL ? 4864 : 4865
                                          userInfo:info]];
    [self release];
    return nil;
}

// Native -[NSUUID classForCoder], 0x1c06b18ac. Non-external method.
- (Class)classForCoder {
    return objc_opt_class((id)(Class)uuidClass);
}

@end

// Native _NSUUIDBridge methods, iOS Simulator Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified (assembly read in full). iOS Device divergence: iOS Device 24A446 remains unread.
@implementation _ONSUUIDBridge

// Native -[_NSUUIDBridge init], 0x1c06b18b8: answers self without messaging super. The Swift subclass's
// super.init calls reach it.
- (id)init {
    return self;
}

// Native -[_NSUUIDBridge copyWithZone:], 0x1c06b18bc: a tail call to objc_retain.
- (id)copyWithZone:(struct _NSZone *)zone {
    (void)zone;
    return [self retain];
}

// Native +[_NSUUIDBridge automaticallyNotifiesObserversForKey:], 0x1c06b18c0.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    (void)key;
    return NO;
}

// Native -[_NSUUIDBridge initWithUUIDString:], 0x1c0fe7ba4: a trap (BRK #1).
- (id)initWithUUIDString:(ONSString *)string {
    (void)string;
    __builtin_trap();
}

// Native -[_NSUUIDBridge initWithUUIDBytes:], 0x1c0fe7ba8: a trap (BRK #1).
- (id)initWithUUIDBytes:(const unsigned char *)bytes {
    (void)bytes;
    __builtin_trap();
}

// Native -[_NSUUIDBridge initWithCoder:], 0x1c0fe7bac: a trap (BRK #1).
- (id)initWithCoder:(ONSCoder *)coder {
    (void)coder;
    __builtin_trap();
}

@end
