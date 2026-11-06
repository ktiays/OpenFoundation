//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFCharacterSet.h"

#include <objc/objc-exception.h>
#include <objc/runtime.h>

// Inferred organization: the __NSCFCharacterSet methods (0x1c01b442c-0x1c01b481c) lie together in the iOS
// Simulator __text section, after the __NSCFError methods; following the owner's decision, the class has its
// own translation unit. Objective-C compiled with manual reference counting (objc_autorelease). Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The NSException class symbol, loaded natively through its class reference (off_2227CE300).
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));

// The keyed-archive keys "NSRange" and "NSString" are data, not runtime names: they keep their native
// spelling in both naming modes. Each is written as two adjacent literals because the prefix tool's
// runtime-name check matches whole literals equal to a mapped name (the split is reported to the owner as a
// false positive of that check).

@implementation __ONSCFCharacterSet

// Native -[__NSCFCharacterSet isEqual:], 0x1c01b442c. Non-external. nil answers NO and the receiver itself
// YES before _CFNonObjCEqual.
- (BOOL)isEqual:(id)object {
    if (object == nil) {
        return NO;
    }
    if (self == object) {
        return YES;
    }
    return _OCFNonObjCEqual(self, object) != 0;
}

// Native -[__NSCFCharacterSet hash], 0x1c01b4468. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[__NSCFCharacterSet retain], 0x1c01b446c. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFCharacterSet release], 0x1c01b4470. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFCharacterSet _tryRetain], 0x1c01b4474. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFCharacterSet _isDeallocating], 0x1c01b4490. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFCharacterSet retainCount], 0x1c01b44ac. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native +[__NSCFCharacterSet allocWithZone:], 0x1c01b44b0. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native +[__NSCFCharacterSet automaticallyNotifiesObserversForKey:], 0x1c01b44cc. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSCFCharacterSet copyWithZone:], 0x1c01b44d4. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return (id)OCFCharacterSetCreateCopy(NULL, (OCFCharacterSetRef)self);
}

// Native -[__NSCFCharacterSet mutableCopyWithZone:], 0x1c01b44e0. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return (id)OCFCharacterSetCreateMutableCopy(NULL, (OCFCharacterSetRef)self);
}

// Native -[__NSCFCharacterSet characterIsMember:], 0x1c01b44ec. Non-external.
- (BOOL)characterIsMember:(UniChar)character {
    return OCFCharacterSetIsCharacterMember((OCFCharacterSetRef)self, character) != 0;
}

// Native -[__NSCFCharacterSet longCharacterIsMember:], 0x1c01b450c. Non-external.
- (BOOL)longCharacterIsMember:(UTF32Char)character {
    return OCFCharacterSetIsLongCharacterMember((OCFCharacterSetRef)self, character) != 0;
}

// Native -[__NSCFCharacterSet isSupersetOfSet:], 0x1c01b452c. Non-external.
- (BOOL)isSupersetOfSet:(id)otherSet {
    return OCFCharacterSetIsSupersetOfSet((OCFCharacterSetRef)self, (OCFCharacterSetRef)otherSet) != 0;
}

// Native -[__NSCFCharacterSet hasMemberInPlane:], 0x1c01b454c. Non-external. The plane byte is zero-extended
// (MOV W1, W2).
- (BOOL)hasMemberInPlane:(uint8_t)plane {
    return OCFCharacterSetHasMemberInPlane((OCFCharacterSetRef)self, plane) != 0;
}

// Native -[__NSCFCharacterSet bitmapRepresentation], 0x1c01b456c. Non-external.
- (id)bitmapRepresentation {
    return [(id)OCFCharacterSetCreateBitmapRepresentation(NULL, (OCFCharacterSetRef)self) autorelease];
}

// Native -[__NSCFCharacterSet invertedSet], 0x1c01b4588. Non-external.
- (id)invertedSet {
    return [(id)OCFCharacterSetCreateInvertedSet(NULL, (OCFCharacterSetRef)self) autorelease];
}

// Native -[__NSCFCharacterSet addCharactersInRange:], 0x1c01b45a4. Non-external.
- (void)addCharactersInRange:(OCFRange)range {
    OCFCharacterSetAddCharactersInRange((OCFMutableCharacterSetRef)self, range);
}

// Native -[__NSCFCharacterSet removeCharactersInRange:], 0x1c01b45b0. Non-external.
- (void)removeCharactersInRange:(OCFRange)range {
    OCFCharacterSetRemoveCharactersInRange((OCFMutableCharacterSetRef)self, range);
}

// Native -[__NSCFCharacterSet addCharactersInString:], 0x1c01b45bc. Non-external.
- (void)addCharactersInString:(id)string {
    OCFCharacterSetAddCharactersInString((OCFMutableCharacterSetRef)self, (OCFStringRef)string);
}

// Native -[__NSCFCharacterSet removeCharactersInString:], 0x1c01b45c4. Non-external.
- (void)removeCharactersInString:(id)string {
    OCFCharacterSetRemoveCharactersInString((OCFMutableCharacterSetRef)self, (OCFStringRef)string);
}

// Native -[__NSCFCharacterSet formUnionWithCharacterSet:], 0x1c01b45cc. Non-external.
- (void)formUnionWithCharacterSet:(id)otherSet {
    OCFCharacterSetUnion((OCFMutableCharacterSetRef)self, (OCFCharacterSetRef)otherSet);
}

// Native -[__NSCFCharacterSet formIntersectionWithCharacterSet:], 0x1c01b45d4. Non-external.
- (void)formIntersectionWithCharacterSet:(id)otherSet {
    OCFCharacterSetIntersect((OCFMutableCharacterSetRef)self, (OCFCharacterSetRef)otherSet);
}

// Native -[__NSCFCharacterSet invert], 0x1c01b45dc. Non-external.
- (void)invert {
    OCFCharacterSetInvert((OCFMutableCharacterSetRef)self);
}

// Native -[__NSCFCharacterSet makeCharacterSetCompact], 0x1c01b45e0. Non-external.
- (void)makeCharacterSetCompact {
    _OCFCharacterSetCompact((OCFMutableCharacterSetRef)self);
}

// Native -[__NSCFCharacterSet makeCharacterSetFast], 0x1c01b45e4. Non-external.
- (void)makeCharacterSetFast {
    _OCFCharacterSetFast((OCFMutableCharacterSetRef)self);
}

// Native -[__NSCFCharacterSet classForCoder], 0x1c01b45e8. Non-external. Looked up by name.
- (Class)classForCoder {
    if (__CFCharacterSetIsMutable((OCFCharacterSetRef)self)) {
        return objc_lookUpClass(OF_NAME_STRING(ONSMutableCharacterSet));
    }
    return objc_lookUpClass(OF_NAME_STRING(ONSCharacterSet));
}

// Native +[__NSCFCharacterSet supportsSecureCoding], 0x1c01b4614. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[__NSCFCharacterSet initWithCoder:], 0x1c01b461c. Non-external. Always throws an
// NSInternalInconsistencyException made by +[NSException exceptionWithName:reason:userInfo:].
- (id)initWithCoder:(id)coder {
    id exception = [(id)exceptionClass exceptionWithName:(id)ONSInternalInconsistencyException
                                                  reason:(id)OF_STR(
                                                             "Bridged character sets should decode using "
                                                             "NSCharacterSet/NSMutableCharacterSet via "
                                                             "classForCoder"
                                                         )
                                                userInfo:nil];
    objc_exception_throw(exception);
}

// Native -[__NSCFCharacterSet encodeWithCoder:], 0x1c01b464c. Non-external. Only keyed coders are served. By
// the keyed-coding type: 2 encodes the builtin ID ("NSBuiltinID"), 3 the range packed as location << 32 |
// length ("NSRange"), 4 the coding string ("NSString" through -_encodePropertyList:forKey: for an
// NSKeyedArchiver, "NSStringObject" otherwise; the string is released after), each followed by "NSIsInverted"
// for an inverted set. Type 5 encodes "NSBuiltinID2" (and "NSIsInverted2") and then the bitmap, as every
// other type does: "NSBitmap" through -_encodePropertyList:forKey: for an NSKeyedArchiver (the coder's class
// compared with the class looked up by name), "NSBitmapObject" otherwise.
- (void)encodeWithCoder:(id)coder {
    if (![coder allowsKeyedCoding]) {
        return;
    }
    Boolean inverted = _OCFCharacterSetIsInverted((OCFCharacterSetRef)self);
    OCFIndex type = _OCFCharacterSetGetKeyedCodingType((OCFCharacterSetRef)self);
    if (type == 2) {
        OCFIndex builtinType = _OCFCharacterSetGetKeyedCodingBuiltinType((OCFCharacterSetRef)self);
        [coder encodeInteger:builtinType forKey:(id)OF_STR("NSBuiltinID")];
        if (inverted) {
            [coder encodeBool:YES forKey:(id)OF_STR("NSIsInverted")];
        }
        return;
    }
    if (type == 3) {
        OCFRange range = _OCFCharacterSetGetKeyedCodingRange((OCFCharacterSetRef)self);
        [coder encodeInt64:(int64_t)(range.length | (range.location << 32))
                    forKey:(id)OF_STR(
                               "NS"
                               "Range"
                           )];
        if (inverted) {
            [coder encodeBool:YES forKey:(id)OF_STR("NSIsInverted")];
        }
        return;
    }
    if (type == 4) {
        OCFStringRef string = _OCFCharacterSetCreateKeyedCodingString((OCFCharacterSetRef)self);
        Class coderClass = [coder class];
        if (coderClass == objc_lookUpClass(OF_NAME_STRING(ONSKeyedArchiver))) {
            [coder _encodePropertyList:(id)string
                                forKey:(id)OF_STR(
                                           "NS"
                                           "String"
                                       )];
        } else {
            [coder encodeObject:(id)string forKey:(id)OF_STR("NSStringObject")];
        }
        OCFRelease(string);
        if (inverted) {
            [coder encodeBool:YES forKey:(id)OF_STR("NSIsInverted")];
        }
        return;
    }
    if (type == 5) {
        OCFIndex builtinType = _OCFCharacterSetGetKeyedCodingBuiltinType((OCFCharacterSetRef)self);
        [coder encodeInteger:builtinType forKey:(id)OF_STR("NSBuiltinID2")];
        if (inverted) {
            [coder encodeBool:YES forKey:(id)OF_STR("NSIsInverted2")];
        }
    }
    Class coderClass = [coder class];
    Class archiverClass = objc_lookUpClass(OF_NAME_STRING(ONSKeyedArchiver));
    id bitmap = [self bitmapRepresentation];
    if (coderClass == archiverClass) {
        [coder _encodePropertyList:bitmap forKey:(id)OF_STR("NSBitmap")];
    } else {
        [coder encodeObject:bitmap forKey:(id)OF_STR("NSBitmapObject")];
    }
}

@end
