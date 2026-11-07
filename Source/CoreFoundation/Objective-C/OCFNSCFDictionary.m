//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFDictionary.h"
#include "CoreFoundation/Base/OCFUtilities.h"

// Inferred organization: the __NSCFDictionary methods (0x1c01a2ffc-0x1c01a36c0) lie together in the iOS
// Simulator __text section, followed by an OUTLINED_FUNCTION_0; following the owner's decision, the class has
// its own translation unit. Objective-C compiled with manual reference counting (objc_retain, objc_release,
// objc_autorelease). -setObject:forKey:, -removeObjectForKey: and -removeAllObjects are restored from the
// IDA capture of their .cold.1 fragments (iphone-simulator-corpus-delta2.json); the fragments end in
// OUTLINED_FUNCTION_1_8 (0x1c01a36d8: NSInternalInconsistencyException into X19, then __CFExceptionProem(X0,
// X1)) and _0_12 (0x1c01a36c4: the raise with "%@: mutating method sent to immutable object"), read in
// iphone-simulator-corpus-delta3.json (confidence: verified). The cold-region static
// _NSDictionaryRaiseInsertNilKeyException (0x1c0308664), _NSDictionaryRaiseInsertNilValueException
// (0x1c030868c) and _NSDictionaryRaiseRemoveNilKeyException (0x1c03086c4) follow the class, from the same
// capture (OUTLINED_FUNCTION_2_6, 0x1c01a36e8, is folded into the nil-key raises). Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The __NSFastEnumerationEnumerator class symbol, loaded natively through its class reference
// (off_2227CE440).
extern char fastEnumerationEnumeratorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                       OF_NAME_STRING(__ONSFastEnumerationEnumerator));

// The three raises, defined after the class in native address order.
static __attribute__((noreturn)) void
_NSDictionaryRaiseInsertNilKeyException(const void *object, SEL selector);

static __attribute__((noreturn)) void
_NSDictionaryRaiseInsertNilValueException(const void *object, SEL selector, const void *key);

static __attribute__((noreturn)) void
_NSDictionaryRaiseRemoveNilKeyException(const void *object, SEL selector);

// The mutability raise of the three mutation fragments.
// raiseImmutableDictionary is a local descriptive name.
static __attribute__((noreturn)) void raiseImmutableDictionary(id object, SEL selector) {
    _CFThrowFormattedException(
        ONSInternalInconsistencyException,
        OF_STR("%@: mutating method sent to immutable object"),
        __CFExceptionProem(object, selector)
    );
}

// -setObservationInfo: is Foundation's NSObject(NSKeyValueObserving) method; NSMutableDictionary does not
// declare it, so the super send is declared here.
@interface ONSMutableDictionary (KeyValueObservingDeclarations)
- (void)setObservationInfo:(void *)observationInfo;
@end

@implementation __ONSCFDictionary

// Native +[__NSCFDictionary allocWithZone:], 0x1c01a2ffc. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native -[__NSCFDictionary count], 0x1c01a3018. Non-external.
- (NSUInteger)count {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableDictionary);
    return (NSUInteger)OCFDictionaryGetCount((OCFDictionaryRef)self);
}

// Native -[__NSCFDictionary objectForKey:], 0x1c01a3064. Non-external. A nil key answers nil before the
// check.
- (id)objectForKey:(id)key {
    if (key == nil) {
        return nil;
    }
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableDictionary);
    return (id)OCFDictionaryGetValue((OCFDictionaryRef)self, key);
}

// Native -[__NSCFDictionary keyEnumerator], 0x1c01a30c4. Non-external. A fast-enumeration enumerator over the
// receiver, autoreleased.
- (id)keyEnumerator {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableDictionary);
    id enumerator = [(Class)fastEnumerationEnumeratorClass allocWithZone:NULL];
    return [[enumerator initWithObject:self] autorelease];
}

// Native -[__NSCFDictionary setObject:forKey:], 0x1c01a3120, with its .cold.1 (0x1c03086ec). Non-external.
// Order: the mutability check, the write check with the method's return address, a nil key, a nil object;
// then the key is copied (-copyWithZone: NULL), set and released.
- (void)setObject:(id)object forKey:(id)key {
    void *returnAddress = __builtin_return_address(0);
    if (!_OCFDictionaryIsMutable((OCFDictionaryRef)self)) {
        raiseImmutableDictionary(self, _cmd);
    }
    tsanWrite(self, returnAddress, __OCFTSANTagMutableDictionary);
    if (key == nil) {
        _NSDictionaryRaiseInsertNilKeyException(self, _cmd);
    }
    if (object == nil) {
        _NSDictionaryRaiseInsertNilValueException(self, _cmd, key);
    }
    id copy = [key copyWithZone:NULL];
    OCFDictionarySetValue((OCFMutableDictionaryRef)self, copy, object);
    [copy release];
}

// Native -[__NSCFDictionary removeObjectForKey:], 0x1c01a31ec, with its .cold.1 (0x1c0308704). Non-external.
- (void)removeObjectForKey:(id)key {
    void *returnAddress = __builtin_return_address(0);
    if (!_OCFDictionaryIsMutable((OCFDictionaryRef)self)) {
        raiseImmutableDictionary(self, _cmd);
    }
    tsanWrite(self, returnAddress, __OCFTSANTagMutableDictionary);
    if (key == nil) {
        _NSDictionaryRaiseRemoveNilKeyException(self, _cmd);
    }
    OCFDictionaryRemoveValue((OCFMutableDictionaryRef)self, key);
}

// Native +[__NSCFDictionary automaticallyNotifiesObserversForKey:], 0x1c01a3270. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSCFDictionary setObservationInfo:], 0x1c01a3278. Non-external. The KVO bit records whether
// observation info is present; then super stores it.
- (void)setObservationInfo:(void *)observationInfo {
    _OCFDictionarySetKVOBit((OCFDictionaryRef)self, observationInfo != NULL);
    [super setObservationInfo:observationInfo];
}

// Native -[__NSCFDictionary classForCoder], 0x1c01a32f8. Non-external. The class references of
// NSMutableDictionary and NSDictionary (0x2227ce378, 0x2227ce2f8), sent +self.
- (Class)classForCoder {
    if (_OCFDictionaryIsMutable((OCFDictionaryRef)self)) {
        return [ONSMutableDictionary self];
    }
    return [ONSDictionary self];
}

// Native -[__NSCFDictionary hash], 0x1c01a3328. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[__NSCFDictionary isEqual:], 0x1c01a332c. Non-external. Apps linked on or after version 6 use
// super's comparison; older apps compare through _CFNonObjCEqual (nil is unequal; no identity shortcut).
- (BOOL)isEqual:(id)object {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableDictionary);
    if (_OCFExecutableLinkedOnOrAfter(6)) {
        return [super isEqual:object];
    }
    if (object == nil) {
        return NO;
    }
    return _OCFNonObjCEqual(self, object) != 0;
}

// Native -[__NSCFDictionary countByEnumeratingWithState:objects:count:], 0x1c01a33fc. Non-external.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableDictionary);
    return _OCFDictionaryFastEnumeration((OCFDictionaryRef)self, state, buffer, length);
}

// Native -[__NSCFDictionary removeAllObjects], 0x1c01a3468, with its .cold.1 (0x1c030871c). Non-external.
- (void)removeAllObjects {
    void *returnAddress = __builtin_return_address(0);
    if (!_OCFDictionaryIsMutable((OCFDictionaryRef)self)) {
        raiseImmutableDictionary(self, _cmd);
    }
    tsanWrite(self, returnAddress, __OCFTSANTagMutableDictionary);
    OCFDictionaryRemoveAllValues((OCFMutableDictionaryRef)self);
}

// Native -[__NSCFDictionary copyWithZone:], 0x1c01a34d8. Non-external. A mutable dictionary is copied (NULL
// allocator); an immutable one is retained.
- (id)copyWithZone:(struct _NSZone *)zone {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableDictionary);
    if (_OCFDictionaryIsMutable((OCFDictionaryRef)self)) {
        return (id)OCFDictionaryCreateCopy(NULL, (OCFDictionaryRef)self);
    }
    return [self retain];
}

// Native -[__NSCFDictionary mutableCopyWithZone:], 0x1c01a3544. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableDictionary);
    return (id)OCFDictionaryCreateMutableCopy(NULL, 0, (OCFDictionaryRef)self);
}

// Native -[__NSCFDictionary description], 0x1c01a3598. Non-external. A dictionary whose key or value
// copy-description callback is neither NULL nor CFCopyDescription describes through CF; the rest go to super.
// The key callbacks are read before the value callbacks.
- (id)description {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableDictionary);
    OCFDictionaryCopyDescriptionCallBack keyDescription =
        __CFDictionaryGetKeyCallbacks((OCFDictionaryRef)self).copyDescription;
    OCFDictionaryCopyDescriptionCallBack valueDescription =
        __CFDictionaryGetValueCallbacks((OCFDictionaryRef)self).copyDescription;
    if ((keyDescription != NULL && keyDescription != OCFCopyDescription) ||
        (valueDescription != NULL && valueDescription != OCFCopyDescription)) {
        return [(id)OCFCopyDescription(self) autorelease];
    }
    return [super description];
}

// Native -[__NSCFDictionary retain], 0x1c01a3680. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFDictionary release], 0x1c01a3684. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFDictionary _tryRetain], 0x1c01a3688. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFDictionary _isDeallocating], 0x1c01a36a4. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFDictionary retainCount], 0x1c01a36c0. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

@end

// Native _NSDictionaryRaiseInsertNilKeyException, 0x1c0308664. Non-external. OUTLINED_FUNCTION_2_6
// (0x1c01a36e8) loads NSInvalidArgumentException into X19 and tail-calls __CFExceptionProem with the object
// and selector still in X0/X1; the proem is the argument of "%@: attempt to insert nil key". Evidence:
// iphone-simulator-corpus-delta3.json.
static void _NSDictionaryRaiseInsertNilKeyException(const void *object, SEL selector) {
    OCFStringRef name = ONSInvalidArgumentException;
    _CFThrowFormattedException(
        name, OF_STR("%@: attempt to insert nil key"), __CFExceptionProem(object, selector)
    );
}

// Native _NSDictionaryRaiseInsertNilValueException, 0x1c030868c. Non-external. NSInvalidArgumentException
// (loaded before the proem), "%@: attempt to insert nil value (key: %@)" with the proem and the key.
// Evidence: iphone-simulator-corpus-delta3.json.
static void _NSDictionaryRaiseInsertNilValueException(const void *object, SEL selector, const void *key) {
    OCFStringRef name = ONSInvalidArgumentException;
    _CFThrowFormattedException(
        name, OF_STR("%@: attempt to insert nil value (key: %@)"), __CFExceptionProem(object, selector), key
    );
}

// Native _NSDictionaryRaiseRemoveNilKeyException, 0x1c03086c4. Non-external. As the insert nil-key raise
// (OUTLINED_FUNCTION_2_6), with "%@: attempt to remove nil key". Evidence:
// iphone-simulator-corpus-delta3.json.
static void _NSDictionaryRaiseRemoveNilKeyException(const void *object, SEL selector) {
    OCFStringRef name = ONSInvalidArgumentException;
    _CFThrowFormattedException(
        name, OF_STR("%@: attempt to remove nil key"), __CFExceptionProem(object, selector)
    );
}
