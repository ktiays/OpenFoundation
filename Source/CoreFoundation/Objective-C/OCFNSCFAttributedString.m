//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFAttributedString.h"

#include <objc/runtime.h>

// Inferred organization: the __NSCFAttributedString methods (0x1c01b2b34-0x1c01b2fac) lie together in the iOS
// Simulator __text section, between the __NSCFString and __NSCFNumber methods; following the owner's
// decision, the class has its own translation unit. Objective-C compiled with manual reference counting
// (objc_autorelease). The static mutateError2 (0x1c0308d1c, in the cold region beside the __NSCFString
// fragments) is restored here, after the class, from the IDA capture (iphone-simulator-corpus-delta2.json).
// Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// Native mutateError2, 0x1c0308d1c; defined after the class, in native address order.
static __attribute__((noreturn)) void mutateError2(SEL selector, OCFIndex status, const void *object);

@implementation __ONSCFAttributedString

// Native -[__NSCFAttributedString retain], 0x1c01b2b34. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFAttributedString release], 0x1c01b2b38. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFAttributedString _tryRetain], 0x1c01b2b3c. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFAttributedString _isDeallocating], 0x1c01b2b58. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFAttributedString retainCount], 0x1c01b2b74. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native -[__NSCFAttributedString isEqual:], 0x1c01b2b78. Non-external. nil answers NO; there is no identity
// shortcut.
- (BOOL)isEqual:(id)object {
    if (object == nil) {
        return NO;
    }
    return _OCFNonObjCEqual(self, object) != 0;
}

// Native +[__NSCFAttributedString automaticallyNotifiesObserversForKey:], 0x1c01b2ba4. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native +[__NSCFAttributedString allocWithZone:], 0x1c01b2bac. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native -[__NSCFAttributedString string], 0x1c01b2bc8. Non-external.
- (id)string {
    return (id)OCFAttributedStringGetString((OCFAttributedStringRef)self);
}

// Native -[__NSCFAttributedString attributesAtIndex:effectiveRange:], 0x1c01b2bcc. Non-external.
- (id)attributesAtIndex:(NSUInteger)location effectiveRange:(OCFRange *)range {
    return (id)OCFAttributedStringGetAttributes((OCFAttributedStringRef)self, (OCFIndex)location, range);
}

// Native -[__NSCFAttributedString length], 0x1c01b2bd8. Non-external.
- (NSUInteger)length {
    return (NSUInteger)_OCFAttributedStringGetLength((OCFAttributedStringRef)self);
}

// Native -[__NSCFAttributedString attribute:atIndex:effectiveRange:], 0x1c01b2bdc. Non-external.
- (id)attribute:(id)name atIndex:(NSUInteger)location effectiveRange:(OCFRange *)range {
    return (id)OCFAttributedStringGetAttribute(
        (OCFAttributedStringRef)self, (OCFIndex)location, (OCFStringRef)name, range
    );
}

// Native -[__NSCFAttributedString attributedSubstringFromRange:], 0x1c01b2be8. Non-external.
- (id)attributedSubstringFromRange:(OCFRange)range {
    return
        [(id)OCFAttributedStringCreateWithSubstring(NULL, (OCFAttributedStringRef)self, range) autorelease];
}

// Native -[__NSCFAttributedString attributesAtIndex:longestEffectiveRange:inRange:], 0x1c01b2c04.
// Non-external.
- (id)attributesAtIndex:(NSUInteger)location
    longestEffectiveRange:(OCFRange *)range
                  inRange:(OCFRange)rangeLimit {
    return (id)OCFAttributedStringGetAttributesAndLongestEffectiveRange(
        (OCFAttributedStringRef)self, (OCFIndex)location, rangeLimit, range
    );
}

// Native -[__NSCFAttributedString attribute:atIndex:longestEffectiveRange:inRange:], 0x1c01b2c1c.
// Non-external.
- (id)attribute:(id)name
                  atIndex:(NSUInteger)location
    longestEffectiveRange:(OCFRange *)range
                  inRange:(OCFRange)rangeLimit {
    return (id)OCFAttributedStringGetAttributeAndLongestEffectiveRange(
        (OCFAttributedStringRef)self, (OCFIndex)location, (OCFStringRef)name, rangeLimit, range
    );
}

// Native -[__NSCFAttributedString isEqualToAttributedString:], 0x1c01b2c34. Non-external. nil answers NO.
- (BOOL)isEqualToAttributedString:(id)other {
    if (other == nil) {
        return NO;
    }
    return OCFEqual(self, other) != 0;
}

// Native -[__NSCFAttributedString copyWithZone:], 0x1c01b2c60. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return (id)OCFAttributedStringCreateCopy(NULL, (OCFAttributedStringRef)self);
}

// Native -[__NSCFAttributedString mutableCopyWithZone:], 0x1c01b2c6c. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return (id)OCFAttributedStringCreateMutableCopy(NULL, 0, (OCFAttributedStringRef)self);
}

// Native -[__NSCFAttributedString classForCoder], 0x1c01b2c7c. Non-external. Looked up by name.
- (Class)classForCoder {
    if (__OCFAttributedStringMtbl((OCFAttributedStringRef)self)) {
        return objc_lookUpClass(OF_NAME_STRING(ONSMutableAttributedString));
    }
    return objc_lookUpClass(OF_NAME_STRING(ONSAttributedString));
}

// Native -[__NSCFAttributedString replaceCharactersInRange:withString:], 0x1c01b2ca8. Non-external. A nonzero
// status raises through mutateError2.
- (void)replaceCharactersInRange:(OCFRange)range withString:(id)string {
    int status = _OCFAttributedStringCheckAndReplace(
        (OCFMutableAttributedStringRef)self, range.location, range.length, (OCFStringRef)string
    );
    if (status != 0) {
        mutateError2(_cmd, status, self);
    }
}

// Native -[__NSCFAttributedString setAttributes:range:], 0x1c01b2cec. Non-external. Other attributes are
// cleared (true).
- (void)setAttributes:(id)attributes range:(OCFRange)range {
    int status = _OCFAttributedStringCheckAndSetAttributes(
        (OCFMutableAttributedStringRef)self, range.location, range.length, (OCFDictionaryRef)attributes, 1
    );
    if (status != 0) {
        mutateError2(_cmd, status, self);
    }
}

// Native -[__NSCFAttributedString replaceCharactersInRange:withAttributedString:], 0x1c01b2d38. Non-external.
- (void)replaceCharactersInRange:(OCFRange)range withAttributedString:(id)attributedString {
    int status = _OCFAttributedStringCheckAndReplaceAttributed(
        (OCFMutableAttributedStringRef)self,
        range.location,
        range.length,
        (OCFAttributedStringRef)attributedString
    );
    if (status != 0) {
        mutateError2(_cmd, status, self);
    }
}

// Native -[__NSCFAttributedString insertAttributedString:atIndex:], 0x1c01b2d7c. Non-external.
- (void)insertAttributedString:(id)attributedString atIndex:(NSUInteger)location {
    int status = _OCFAttributedStringCheckAndReplaceAttributed(
        (OCFMutableAttributedStringRef)self, (OCFIndex)location, 0, (OCFAttributedStringRef)attributedString
    );
    if (status != 0) {
        mutateError2(_cmd, status, self);
    }
}

// Native -[__NSCFAttributedString appendAttributedString:], 0x1c01b2dc4. Non-external.
- (void)appendAttributedString:(id)attributedString {
    OCFIndex length = _OCFAttributedStringGetLength((OCFAttributedStringRef)self);
    int status = _OCFAttributedStringCheckAndReplaceAttributed(
        (OCFMutableAttributedStringRef)self, length, 0, (OCFAttributedStringRef)attributedString
    );
    if (status != 0) {
        mutateError2(_cmd, status, self);
    }
}

// Native -[__NSCFAttributedString deleteCharactersInRange:], 0x1c01b2e1c. Non-external. The replacement is
// NULL.
- (void)deleteCharactersInRange:(OCFRange)range {
    int status = _OCFAttributedStringCheckAndReplaceAttributed(
        (OCFMutableAttributedStringRef)self, range.location, range.length, NULL
    );
    if (status != 0) {
        mutateError2(_cmd, status, self);
    }
}

// Native -[__NSCFAttributedString setAttributedString:], 0x1c01b2e60. Non-external.
- (void)setAttributedString:(id)attributedString {
    OCFIndex length = _OCFAttributedStringGetLength((OCFAttributedStringRef)self);
    int status = _OCFAttributedStringCheckAndReplaceAttributed(
        (OCFMutableAttributedStringRef)self, 0, length, (OCFAttributedStringRef)attributedString
    );
    if (status != 0) {
        mutateError2(_cmd, status, self);
    }
}

// Native -[__NSCFAttributedString addAttributes:range:], 0x1c01b2eb8. Non-external. Other attributes are kept
// (false).
- (void)addAttributes:(id)attributes range:(OCFRange)range {
    int status = _OCFAttributedStringCheckAndSetAttributes(
        (OCFMutableAttributedStringRef)self, range.location, range.length, (OCFDictionaryRef)attributes, 0
    );
    if (status != 0) {
        mutateError2(_cmd, status, self);
    }
}

// Native -[__NSCFAttributedString addAttribute:value:range:], 0x1c01b2f04. Non-external. A nil value raises
// through mutateError2 with status 2 before any check.
- (void)addAttribute:(id)name value:(id)value range:(OCFRange)range {
    if (value == nil) {
        mutateError2(_cmd, 2, self);
    }
    int status = _OCFAttributedStringCheckAndSetAttribute(
        (OCFMutableAttributedStringRef)self, range.location, range.length, (OCFStringRef)name, value
    );
    if (status != 0) {
        mutateError2(_cmd, status, self);
    }
}

// Native -[__NSCFAttributedString removeAttribute:range:], 0x1c01b2f64. Non-external. The value is NULL.
- (void)removeAttribute:(id)name range:(OCFRange)range {
    int status = _OCFAttributedStringCheckAndSetAttribute(
        (OCFMutableAttributedStringRef)self, range.location, range.length, (OCFStringRef)name, NULL
    );
    if (status != 0) {
        mutateError2(_cmd, status, self);
    }
}

@end

// Native mutateError2, 0x1c0308d1c. Non-external. An instruction-for-instruction twin of the __NSCFString
// unit's mutateError (0x1c0308c84): NSInvalidArgumentException (loaded first); status 3 "%@: Range or index
// out of bounds" and 2 "%@: nil argument" (with the proem), 1 "Attempt to mutate immutable object with %s"
// (the selector name), any other "Can't call %s in %@" (the selector name and the object's class). The
// argument set-up and the raise are OUTLINED_FUNCTION_5_3 (0x1c01b5a10), _6_3 (0x1c01b5a1c) and _3_5
// (0x1c01b59f8), shared with mutateError and read in iphone-simulator-corpus-delta3.json (confidence:
// verified).
static void mutateError2(SEL selector, OCFIndex status, const void *object) {
    OCFStringRef name = ONSInvalidArgumentException;
    if (status == 3) {
        _CFThrowFormattedException(
            name, OF_STR("%@: Range or index out of bounds"), __CFExceptionProem(object, selector)
        );
    }
    if (status == 2) {
        _CFThrowFormattedException(name, OF_STR("%@: nil argument"), __CFExceptionProem(object, selector));
    }
    if (status == 1) {
        _CFThrowFormattedException(
            name, OF_STR("Attempt to mutate immutable object with %s"), sel_getName(selector)
        );
    }
    const char *selectorName = sel_getName(selector);
    _CFThrowFormattedException(
        name, OF_STR("Can't call %s in %@"), selectorName, object_getClass((id)object)
    );
}
