//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFSet.h"
#include "CoreFoundation/Base/OCFUtilities.h"

// Inferred organization: the __NSCFSet methods (0x1c02fef5c-0x1c02ff5f4) lie together in the iOS Simulator
// __text section; following the owner's decision, the class has its own translation unit. Objective-C
// compiled with manual reference counting (objc_retain, objc_autorelease). -addObject:,
// -removeObject: and -removeAllObjects are restored from the IDA capture of their .cold fragments
// (iphone-simulator-corpus-delta2.json); the fragments use OUTLINED_FUNCTION_1_45 (0x1c02ff60c:
// NSInternalInconsistencyException into X19, then __CFExceptionProem(X0, X1)), _0_63 (0x1c02ff5f8: the raise
// with "%@: mutating method sent to immutable object") and _2_39 (0x1c02ff61c: NSInvalidArgumentException,
// then the proem, before the nil-argument raises), read in iphone-simulator-corpus-delta3.json (confidence:
// verified). Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The __NSFastEnumerationEnumerator class symbol, loaded natively through its class reference
// (off_2227CE440).
extern char
    fastEnumerationEnumeratorClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSFastEnumerationEnumerator));

// The mutability raise of the three mutation fragments.
// raiseImmutableSet is a local descriptive name.
static __attribute__((noreturn)) void raiseImmutableSet(id object, SEL selector) {
    _CFThrowFormattedException(
        ONSInternalInconsistencyException,
        OF_STR("%@: mutating method sent to immutable object"),
        __CFExceptionProem(object, selector)
    );
}

@implementation __ONSCFSet

// Native +[__NSCFSet allocWithZone:], 0x1c02fef5c. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native -[__NSCFSet isEqual:], 0x1c02fef78. Non-external. Apps linked on or after version 6 use super's
// comparison; older apps compare through _CFNonObjCEqual (nil is unequal; no identity shortcut).
- (BOOL)isEqual:(id)object {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableSet);
    if (_OCFExecutableLinkedOnOrAfter(6)) {
        return [super isEqual:object];
    }
    if (object == nil) {
        return NO;
    }
    return _OCFNonObjCEqual(self, object) != 0;
}

// Native -[__NSCFSet hash], 0x1c02ff048. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[__NSCFSet retain], 0x1c02ff04c. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFSet release], 0x1c02ff050. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFSet _tryRetain], 0x1c02ff054. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFSet _isDeallocating], 0x1c02ff070. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFSet retainCount], 0x1c02ff08c. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native -[__NSCFSet description], 0x1c02ff090. Non-external. A set whose copy-description callback is
// neither NULL nor CFCopyDescription describes through CF; the rest go to super.
- (id)description {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableSet);
    OCFSetCopyDescriptionCallBack copyDescription = __CFSetGetCallbacks((OCFSetRef)self).copyDescription;
    if (copyDescription != OCFCopyDescription && copyDescription != NULL) {
        return [(id)OCFCopyDescription(self) autorelease];
    }
    return [super description];
}

// Native +[__NSCFSet automaticallyNotifiesObserversForKey:], 0x1c02ff15c. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSCFSet classForCoder], 0x1c02ff164. Non-external. The class references of NSMutableSet and
// NSSet (0x2227ce390, 0x2227ce3b8), sent +self.
- (Class)classForCoder {
    if (_OCFSetIsMutable((OCFSetRef)self)) {
        return [ONSMutableSet self];
    }
    return [ONSSet self];
}

// Native -[__NSCFSet copyWithZone:], 0x1c02ff194. Non-external. A mutable set is copied (NULL allocator); an
// immutable one is retained.
- (id)copyWithZone:(struct _NSZone *)zone {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableSet);
    if (_OCFSetIsMutable((OCFSetRef)self)) {
        return (id)OCFSetCreateCopy(NULL, (OCFSetRef)self);
    }
    return [self retain];
}

// Native -[__NSCFSet mutableCopyWithZone:], 0x1c02ff200. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableSet);
    return (id)OCFSetCreateMutableCopy(NULL, 0, (OCFSetRef)self);
}

// Native -[__NSCFSet count], 0x1c02ff254. Non-external.
- (NSUInteger)count {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableSet);
    return (NSUInteger)OCFSetGetCount((OCFSetRef)self);
}

// Native -[__NSCFSet member:], 0x1c02ff2a0. Non-external. A nil object answers nil before the check.
- (id)member:(id)object {
    if (object == nil) {
        return nil;
    }
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableSet);
    const void *value = NULL;
    Boolean present = OCFSetGetValueIfPresent((OCFSetRef)self, object, &value);
    return present ? (id)value : nil;
}

// Native -[__NSCFSet _trueCount], 0x1c02ff34c. Non-external. No ThreadSanitizer check.
- (NSUInteger)_trueCount {
    return (NSUInteger)OCFSetGetCount((OCFSetRef)self);
}

// Native -[__NSCFSet countByEnumeratingWithState:objects:count:], 0x1c02ff350. Non-external.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableSet);
    return _OCFSetFastEnumeration((OCFSetRef)self, state, buffer, length);
}

// Native -[__NSCFSet objectEnumerator], 0x1c02ff3bc. Non-external. A fast-enumeration enumerator over the
// receiver, autoreleased.
- (id)objectEnumerator {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableSet);
    id enumerator = [(Class)fastEnumerationEnumeratorClass allocWithZone:NULL];
    return [[enumerator initWithObject:self] autorelease];
}

// Native -[__NSCFSet addObject:], 0x1c02ff418, with its .cold.1 (0x1c03175bc) and .cold.2 (0x1c03175d4).
// Non-external. Unlike the array and dictionary methods, the write check comes before the mutability check;
// a nil object raises NSInvalidArgumentException "%@: attempt to insert nil" (.cold.2).
- (void)addObject:(id)object {
    tsanWrite(self, __builtin_return_address(0), __OCFTSANTagMutableSet);
    if (!_OCFSetIsMutable((OCFSetRef)self)) {
        raiseImmutableSet(self, _cmd);
    }
    if (object == nil) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException, OF_STR("%@: attempt to insert nil"), __CFExceptionProem(self, _cmd)
        );
    }
    OCFSetAddValue((OCFMutableSetRef)self, object);
}

// Native -[__NSCFSet removeObject:], 0x1c02ff49c, with its .cold.1 (0x1c03175fc) and .cold.2 (0x1c0317614).
// Non-external. A nil object raises "%@: attempt to remove nil".
- (void)removeObject:(id)object {
    tsanWrite(self, __builtin_return_address(0), __OCFTSANTagMutableSet);
    if (!_OCFSetIsMutable((OCFSetRef)self)) {
        raiseImmutableSet(self, _cmd);
    }
    if (object == nil) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException, OF_STR("%@: attempt to remove nil"), __CFExceptionProem(self, _cmd)
        );
    }
    OCFSetRemoveValue((OCFMutableSetRef)self, object);
}

// Native -[__NSCFSet removeAllObjects], 0x1c02ff520, with its .cold.1 (0x1c031763c). Non-external.
- (void)removeAllObjects {
    tsanWrite(self, __builtin_return_address(0), __OCFTSANTagMutableSet);
    if (!_OCFSetIsMutable((OCFSetRef)self)) {
        raiseImmutableSet(self, _cmd);
    }
    OCFSetRemoveAllValues((OCFMutableSetRef)self);
}

// Native -[__NSCFSet getObjects:], 0x1c02ff588. Non-external. Nothing is copied for a NULL buffer or an empty
// set.
- (void)getObjects:(id *)objects {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableSet);
    OCFIndex count = OCFSetGetCount((OCFSetRef)self);
    if (objects == NULL || count == 0) {
        return;
    }
    OCFSetGetValues((OCFSetRef)self, (const void **)objects);
}

@end
