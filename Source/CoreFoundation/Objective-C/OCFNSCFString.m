//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFString.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"

#include <objc/runtime.h>
#include <stdarg.h>

// Inferred organization: the __NSCFString methods (0x1c01b1930-0x1c01b2b30) lie together in the iOS Simulator
// __text section, after the __NSCFConstantString methods and before __NSCFAttributedString's; the unit's
// statics __DescriptionWithLocaleFunc (0x1c01b27b8) and __DescriptionWithStringProxyFunc (0x1c01b2820) sit
// between -appendFormat: and -setString:. Following the owner's decision, the class has its own translation
// unit. Objective-C compiled with manual reference counting (objc_autorelease). The seven methods
// whose .cold fragments were missing earlier (-characterAtIndex:, -getCharacters:range:, -hasPrefix:,
// -hasSuffix:, -appendFormat:, -appendCharacters:length:, -replaceOccurrencesOfString:withString:options:
// range:) are restored from the IDA capture of the fragments (iphone-simulator-corpus-delta2.json); their
// .cold fragments, and the static mutateError (0x1c0308c84, in the cold region), are folded into this unit.
// The fragments end in shared OUTLINED_FUNCTION pieces whose assembly is read in the IDA capture
// iphone-simulator-corpus-delta3.json (confidence: verified): _0_17 (0x1c01b59c0, NSInvalidArgumentException
// into X19 then __CFExceptionProem(X0, X1)), _4_3 (0x1c01b5a00, the same name then sel_getName(X0)), _1_12
// (0x1c01b59d0, "%@: nil argument"), _2_10 (0x1c01b59e4, "Attempt to mutate immutable object with %s"), _3_5
// (0x1c01b59f8, the raise with the format already in X1), _5_3 (0x1c01b5a10, __CFExceptionProem(object,
// selector)) and _6_3 (0x1c01b5a1c, object_getClass(object)); the restored code matches them. The statics
// __DescriptionWithLocaleFunc and __DescriptionWithStringProxyFunc are restored from the same capture,
// between -appendFormat: and -setString:. Unless
// a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The address of the class itself, as the native code loads it from its GOT slot (___SCR___NSCFString).
extern char stringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSCFString));

// Native enc (__DATA,__data 0x22e6cf708, non-external): the system encoding, cached on first use; it starts
// as 0xFFFFFFFF (kCFStringEncodingInvalidId).
static OCFStringEncoding enc = 0xFFFFFFFF;

// The cache read every method inlines: a cached value of 0xFFFFFFFF is replaced by
// CFStringGetSystemEncoding(). cachedSystemEncoding is a local descriptive name.
static inline OCFStringEncoding cachedSystemEncoding(void) {
    OCFStringEncoding encoding = enc;
    if (encoding == 0xFFFFFFFF) {
        encoding = OCFStringGetSystemEncoding();
        enc = encoding;
    }
    return encoding;
}

// The NSStringEncoding mapping two methods inline: an encoding CF cannot map is accepted only as 0 or as
// kCFStringEncodingUTF8's value passed by mistake; for those, apps linked on or after version 6 log the
// compatibility warning, and ASCII is used. Any other unmappable encoding answers false.
// convertNSStringEncoding is a local descriptive name.
static inline bool convertNSStringEncoding(unsigned long encoding, OCFStringEncoding *cfEncoding) {
    OCFStringEncoding converted = OCFStringConvertNSStringEncodingToEncoding(encoding);
    if (converted == 0xFFFFFFFF) {
        if (encoding != kOCFStringEncodingUTF8 && encoding != 0) {
            return false;
        }
        if (_OCFExecutableLinkedOnOrAfter(6)) {
            OCFLog(
                4,
                OF_STR(
                    "Incorrect NSStringEncoding value 0x%04lX detected. Assuming NSStringEncodingASCII. Will "
                    "stop this compatibility mapping behavior in the near future."
                ),
                encoding
            );
        }
        converted = kOCFStringEncodingASCII;
    }
    *cfEncoding = converted;
    return true;
}

// The range check three methods inline: a range ending past the string raises NSInvalidArgumentException
// (with the note about a shortened range when the length is 64); a range whose end wrapped around logs the
// warning once per method (the method's own warnonce flag).
// checkRange is a local descriptive name.
static inline void checkRange(id object, SEL selector, OCFRange range, OCFIndex length, bool *warnonce) {
    if ((uintptr_t)(range.location + range.length) > (uintptr_t)length) {
        OCFStringRef exceptionName = ONSInvalidArgumentException;
        OCFStringRef proem = __CFExceptionProem(object, selector);
        _CFThrowFormattedException(
            exceptionName,
            OF_STR("%@: Range {%lu, %lu} out of bounds; string length %lu%s"),
            proem,
            range.location,
            range.length,
            length,
            range.length == 0x40
                ? " (Note that the indicated range may be smaller than the original range passed to the API)"
                : ""
        );
    }
    if (((uintptr_t)length < (uintptr_t)range.length ||
         (uintptr_t)range.location > (uintptr_t)(length - range.length)) &&
        !*warnonce) {
        *warnonce = true;
        OCFStringRef proem = __CFExceptionProem(object, selector);
        OCFLog(
            4,
            OF_STR(
                "*** %@: Range {%lu, %lu} out of bounds; string length %lu. This will become an exception "
                "for "
                "apps linked after 10.10 and iOS 8. Warning shown once per app execution."
            ),
            proem,
            range.location,
            range.length,
            length
        );
    }
}

// The comparison -isEqual: and -isEqualToString: share (two identical native bodies). The receiver and nil
// are answered first. A tagged argument is decoded once: index 2 (an inline tagged string) compares through
// _NSTaggedPointerStringEqualCFString, the extended tag 22 sends -isEqualToString: back with the receiver,
// index 0 (an indirect tagged string) compares through _NSIndirectTaggedPointerStringIsEqualToCFString, and
// any other tag is unequal. An object whose isa is the constant-string class or the receiver's own compares
// through _CFStringEqual; any other object must answer -isNSString__ and the same -length, and then compares
// through _CFStringEqual when its class or its superclass is __NSCFString, else through -isEqualToString:.
// stringEqual is a local descriptive name.
static inline BOOL stringEqual(id self, id other) {
    if (self == other) {
        return YES;
    }
    if (other == nil) {
        return NO;
    }
    if ((intptr_t)other < 0) {
        uintptr_t pointer = (uintptr_t)other;
        uintptr_t obfuscator = (~pointer & 0xC000000000000007) == 0 ? 0 : objc_debug_taggedpointer_obfuscator;
        uintptr_t value = obfuscator ^ pointer;
        uintptr_t index = 0;
        while (index != 7 && (value & 7) != objc_debug_tag60_permutations[index]) {
            ++index;
        }
        if (index == kStringTagInline) {
            return _NSTaggedPointerStringEqualCFString(other, (OCFStringRef)self);
        }
        if (index == kStringTagNotFound) {
            if (((index | value) & 0x7F80000000000000) == 0x0700000000000000) {
                return [other isEqualToString:self];
            }
            return NO;
        }
        if (index == kStringTagIndirect) {
            return _NSIndirectTaggedPointerStringIsEqualToCFString(other, (OCFStringRef)self) != 0;
        }
        return NO;
    }
    uintptr_t isa = *(const uintptr_t *)other;
    if (isa != (uintptr_t)__OCFConstantStringClassReferencePtr && isa != *(const uintptr_t *)self) {
        if (![other isNSString__]) {
            return NO;
        }
        if ((NSUInteger)_OCFStringGetLength2((OCFStringRef)self) != [other length]) {
            return NO;
        }
        Class cls = [other class];
        if (cls != (Class)stringClass && class_getSuperclass(cls) != (Class)stringClass) {
            return [other isEqualToString:self];
        }
    }
    return _CFStringEqual((OCFStringRef)self, (OCFStringRef)other) != 0;
}

// Native mutateError, 0x1c0308c84; defined after the class, in native address order.
static __attribute__((noreturn)) void mutateError(SEL selector, OCFIndex status, const void *object);

// Native __DescriptionWithLocaleFunc (0x1c01b27b8) and __DescriptionWithStringProxyFunc (0x1c01b2820);
// defined after -appendFormat:, in native address order.
static OCFStringRef _DescriptionWithLocaleFunc(void *object, const void *locale);
static OCFStringRef _DescriptionWithStringProxyFunc(
    void *object, const void *context, const void *patternString, bool replacing, bool *isDeferred
);

// Selectors the description statics send to a format argument, declared for @selector and the message sends
// only; no protocol object is emitted.
@protocol DescriptionCallbackMessages

- (id)descriptionWithLocale:(id)locale;
- (id)_dynamicContextEvaluation:(id)context patternString:(id)patternString;

@end

@implementation __ONSCFString

// Native +[__NSCFString allocWithZone:], 0x1c01b1930. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native -[__NSCFString isNSString__], 0x1c01b194c. Non-external.
- (BOOL)isNSString__ {
    return YES;
}

// Native -[__NSCFString retain], 0x1c01b1954. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFString release], 0x1c01b1958. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFString _tryRetain], 0x1c01b195c. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFString _isDeallocating], 0x1c01b1978. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFString retainCount], 0x1c01b1994. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native -[__NSCFString hash], 0x1c01b1998. Non-external.
- (NSUInteger)hash {
    return __OCFStringHash(self);
}

// Native +[__NSCFString automaticallyNotifiesObserversForKey:], 0x1c01b199c. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSCFString length], 0x1c01b19a4. Non-external.
- (NSUInteger)length {
    return (NSUInteger)_OCFStringGetLength2((OCFStringRef)self);
}

// Native -[__NSCFString characterAtIndex:], 0x1c01b19a8, with its .cold.1 (0x1c0308db4). Non-external. A
// failed check raises NSInvalidArgumentException "%@: Range or index out of bounds" with the proem.
- (UniChar)characterAtIndex:(NSUInteger)index {
    UniChar character = 0;
    if (_OCFStringCheckAndGetCharacterAtIndex((OCFStringRef)self, (OCFIndex)index, &character) != 0) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException,
            OF_STR("%@: Range or index out of bounds"),
            __CFExceptionProem(self, _cmd)
        );
    }
    return character;
}

// Native -[__NSCFString getCharacters:range:], 0x1c01b1a24, with its .cold.1 (0x1c0308dd8). Non-external.
- (void)getCharacters:(UniChar *)buffer range:(OCFRange)range {
    if (_OCFStringCheckAndGetCharacters((OCFStringRef)self, range.location, range.length, buffer) != 0) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException,
            OF_STR("%@: Range or index out of bounds"),
            __CFExceptionProem(self, _cmd)
        );
    }
}

// Native -[__NSCFString _fastCharacterContents], 0x1c01b1a68. Non-external.
- (const UniChar *)_fastCharacterContents {
    return OCFStringGetCharactersPtr((OCFStringRef)self);
}

// Native -[__NSCFString _fastCStringContents:], 0x1c01b1a6c. Non-external. The argument is passed through as
// the null-termination requirement, in the cached system encoding.
- (const char *)_fastCStringContents:(BOOL)nullTerminated {
    OCFStringEncoding encoding = cachedSystemEncoding();
    return _CFNonObjCStringGetCStringPtr((OCFStringRef)self, encoding, nullTerminated);
}

// Native -[__NSCFString _fastUTF8StringContents:utf8Length:], 0x1c01b1ab8. Non-external. BMP is not required.
- (const char *)_fastUTF8StringContents:(BOOL)nullTerminated utf8Length:(NSUInteger *)utf8Length {
    return _CFStringGetUTF8PtrWithOptions((OCFStringRef)self, (OCFIndex *)utf8Length, nullTerminated, 0);
}

// Native -[__NSCFString cString], 0x1c01b1ac4. Non-external. A mutable string, or one without a
// null-terminated pointer in the system encoding, goes to super.
- (const char *)cString {
    if (!__OCFStringMtbl((OCFStringRef)self)) {
        OCFStringEncoding encoding = cachedSystemEncoding();
        const char *contents = _CFNonObjCStringGetCStringPtr((OCFStringRef)self, encoding, 1);
        if (contents != NULL) {
            return contents;
        }
    }
    return [super cString];
}

// Native -[__NSCFString UTF8String], 0x1c01b1b68. Non-external. A mutable string, or one without a
// null-terminated UTF-8 pointer, goes to super.
- (const char *)UTF8String {
    if (!__OCFStringMtbl((OCFStringRef)self)) {
        const char *contents = _CFNonObjCStringGetCStringPtr((OCFStringRef)self, kOCFStringEncodingUTF8, 1);
        if (contents != NULL) {
            return contents;
        }
    }
    return [super UTF8String];
}

// Native -[__NSCFString cStringLength], 0x1c01b1bf8. Non-external. When the contents are directly available
// in the system encoding (no null termination required) the length is the string's length; otherwise super's.
- (NSUInteger)cStringLength {
    OCFStringEncoding encoding = cachedSystemEncoding();
    if (_CFNonObjCStringGetCStringPtr((OCFStringRef)self, encoding, 0) != NULL) {
        return (NSUInteger)_OCFStringGetLength2((OCFStringRef)self);
    }
    return [super cStringLength];
}

// Native -[__NSCFString cStringUsingEncoding:], 0x1c01b1cbc. Non-external. A mutable string goes to super; an
// unmappable encoding other than 0 or UTF-8's CF value answers NULL; a string without a null-terminated
// pointer in the encoding goes to super.
- (const char *)cStringUsingEncoding:(NSUInteger)encoding {
    if (!__OCFStringMtbl((OCFStringRef)self)) {
        OCFStringEncoding cfEncoding;
        if (!convertNSStringEncoding(encoding, &cfEncoding)) {
            return NULL;
        }
        const char *contents = _CFNonObjCStringGetCStringPtr((OCFStringRef)self, cfEncoding, 1);
        if (contents != NULL) {
            return contents;
        }
    }
    return [super cStringUsingEncoding:encoding];
}

// Native -[__NSCFString getCString:maxLength:encoding:], 0x1c01b1da0. Non-external.
- (BOOL)getCString:(char *)buffer maxLength:(NSUInteger)maxLength encoding:(NSUInteger)encoding {
    OCFStringEncoding cfEncoding;
    if (!convertNSStringEncoding(encoding, &cfEncoding)) {
        return NO;
    }
    return OCFStringGetCString((OCFStringRef)self, buffer, (OCFIndex)maxLength, cfEncoding) != 0;
}

// Native -[__NSCFString _newSubstringWithRange:zone:], 0x1c01b1e44, with its function static
// _newSubstringWithRange:zone:.warnonce. Non-external. Answers a retained substring.
- (id)_newSubstringWithRange:(OCFRange)range zone:(struct _NSZone *)zone {
    static bool warnonce;
    checkRange(self, _cmd, range, _OCFStringGetLength2((OCFStringRef)self), &warnonce);
    return (id)OCFStringCreateWithSubstring(kOCFAllocatorDefault, (OCFStringRef)self, range);
}

// Native -[__NSCFString substringWithRange:], 0x1c01b1f48, with its function static
// substringWithRange:.warnonce. Non-external. Answers an autoreleased substring.
- (id)substringWithRange:(OCFRange)range {
    static bool warnonce;
    checkRange(self, _cmd, range, _OCFStringGetLength2((OCFStringRef)self), &warnonce);
    return [(id)OCFStringCreateWithSubstring(kOCFAllocatorDefault, (OCFStringRef)self, range) autorelease];
}

// Native -[__NSCFString isEqual:], 0x1c01b2050. Non-external.
- (BOOL)isEqual:(id)object {
    return stringEqual(self, object);
}

// Native -[__NSCFString isEqualToString:], 0x1c01b2210. Non-external. The same body as -isEqual:.
- (BOOL)isEqualToString:(id)string {
    return stringEqual(self, string);
}

// Native -[__NSCFString hasPrefix:], 0x1c01b23d0, with its .cold.1 (0x1c0308dfc). Non-external. A nil
// prefix raises NSInvalidArgumentException "%@: nil argument".
- (BOOL)hasPrefix:(id)prefix {
    if (prefix == nil) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException, OF_STR("%@: nil argument"), __CFExceptionProem(self, _cmd)
        );
    }
    return OCFStringHasPrefix((OCFStringRef)self, (OCFStringRef)prefix) != 0;
}

// Native -[__NSCFString hasSuffix:], 0x1c01b23f8, with its .cold.1 (0x1c0308e14). Non-external.
- (BOOL)hasSuffix:(id)suffix {
    if (suffix == nil) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException, OF_STR("%@: nil argument"), __CFExceptionProem(self, _cmd)
        );
    }
    return OCFStringHasSuffix((OCFStringRef)self, (OCFStringRef)suffix) != 0;
}

// Native -[__NSCFString getLineStart:end:contentsEnd:forRange:], 0x1c01b2420, with its function static
// getLineStart:end:contentsEnd:forRange:.warnonce. Non-external.
- (void)getLineStart:(NSUInteger *)lineStart
                 end:(NSUInteger *)lineEnd
         contentsEnd:(NSUInteger *)contentsEnd
            forRange:(OCFRange)range {
    static bool warnonce;
    checkRange(self, _cmd, range, _OCFStringGetLength2((OCFStringRef)self), &warnonce);
    OCFStringGetLineBounds(
        (OCFStringRef)self, range, (OCFIndex *)lineStart, (OCFIndex *)lineEnd, (OCFIndex *)contentsEnd
    );
}

// Native -[__NSCFString copyWithZone:], 0x1c01b2538. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return (id)_CFNonObjCStringCreateCopy(kOCFAllocatorDefault, (OCFStringRef)self);
}

// Native -[__NSCFString copy], 0x1c01b254c. Non-external.
- (id)copy {
    return (id)_CFNonObjCStringCreateCopy(kOCFAllocatorDefault, (OCFStringRef)self);
}

// Native -[__NSCFString mutableCopyWithZone:], 0x1c01b2560. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return (id)OCFStringCreateMutableCopy(kOCFAllocatorDefault, 0, (OCFStringRef)self);
}

// Native -[__NSCFString fastestEncoding], 0x1c01b2578. Non-external.
- (NSUInteger)fastestEncoding {
    return OCFStringConvertEncodingToNSStringEncoding(OCFStringGetFastestEncoding((OCFStringRef)self));
}

// Native -[__NSCFString smallestEncoding], 0x1c01b258c. Non-external.
- (NSUInteger)smallestEncoding {
    return OCFStringConvertEncodingToNSStringEncoding(OCFStringGetSmallestEncoding((OCFStringRef)self));
}

// Native -[__NSCFString classForCoder], 0x1c01b25a0. Non-external. Looked up by name.
- (Class)classForCoder {
    if (__OCFStringMtbl((OCFStringRef)self)) {
        return objc_lookUpClass(OF_NAME_STRING(ONSMutableString));
    }
    return objc_lookUpClass(OF_NAME_STRING(ONSString));
}

// Native -[__NSCFString _isCString], 0x1c01b25cc. Non-external.
- (BOOL)_isCString {
    return __OCFStringIsEightBit((OCFStringRef)self) != 0;
}

// Native -[__NSCFString replaceCharactersInRange:withString:], 0x1c01b25e8. Non-external. A nonzero status
// raises through mutateError.
- (void)replaceCharactersInRange:(OCFRange)range withString:(id)string {
    int status = __OCFStringCheckAndReplace(
        (OCFMutableStringRef)self, range.location, range.length, (OCFStringRef)string
    );
    if (status != 0) {
        mutateError(_cmd, status, self);
    }
}

// Native -[__NSCFString insertString:atIndex:], 0x1c01b262c. Non-external.
- (void)insertString:(id)string atIndex:(NSUInteger)index {
    int status =
        __OCFStringCheckAndReplace((OCFMutableStringRef)self, (OCFIndex)index, 0, (OCFStringRef)string);
    if (status != 0) {
        mutateError(_cmd, status, self);
    }
}

// Native -[__NSCFString appendString:], 0x1c01b2674. Non-external.
- (void)appendString:(id)string {
    OCFIndex length = _OCFStringGetLength2((OCFStringRef)self);
    int status = __OCFStringCheckAndReplace((OCFMutableStringRef)self, length, 0, (OCFStringRef)string);
    if (status != 0) {
        mutateError(_cmd, status, self);
    }
}

// Native -[__NSCFString deleteCharactersInRange:], 0x1c01b26cc. Non-external. The replacement is the
// constant empty string (__cfstring 0x2227ac180).
- (void)deleteCharactersInRange:(OCFRange)range {
    int status =
        __OCFStringCheckAndReplace((OCFMutableStringRef)self, range.location, range.length, OF_STR(""));
    if (status != 0) {
        mutateError(_cmd, status, self);
    }
}

// Native -[__NSCFString appendFormat:], 0x1c01b2718, with its .cold.1 (0x1c0308e2c). Non-external. An
// immutable string raises NSInvalidArgumentException "Attempt to mutate immutable object with %s" (the
// selector name); otherwise the format and the variadic arguments are appended through
// _CFStringAppendFormatAndArgumentsAux2 with the two description statics and no options or implicit
// argument.
- (void)appendFormat:(id)format, ... {
    if (!__OCFStringMtbl((OCFStringRef)self)) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException,
            OF_STR("Attempt to mutate immutable object with %s"),
            sel_getName(_cmd)
        );
    }
    va_list arguments;
    va_start(arguments, format);
    _OCFStringAppendFormatAndArgumentsAux2(
        (OCFMutableStringRef)self,
        _DescriptionWithLocaleFunc,
        _DescriptionWithStringProxyFunc,
        NULL,
        NULL,
        (OCFStringRef)format,
        arguments
    );
    va_end(arguments);
}

// Native __DescriptionWithLocaleFunc, 0x1c01b27b8. Non-external (a static of this unit), the copy-description
// callback -appendFormat: passes. A nil object is "*nil*"; otherwise -descriptionWithLocale: with the locale
// when the object responds to it (objc_opt_respondsToSelector), else -description. A nil result is "*nil
// description*". The result is retained (CFRetain, a tail call); the description itself is not owned.
// Evidence: iphone-simulator-corpus-delta3.json.
static OCFStringRef _DescriptionWithLocaleFunc(void *object, const void *locale) {
    id description;
    if (object == NULL) {
        description = (id)OF_STR("*nil*");
    } else if ([(id)object respondsToSelector:@selector(descriptionWithLocale:)]) {
        description = [(id)object descriptionWithLocale:(id)locale];
    } else {
        description = [(id)object description];
    }
    if (description == nil) {
        description = (id)OF_STR("*nil description*");
    }
    return (OCFStringRef)OCFRetain((OCFTypeRef)description);
}

// Native __DescriptionWithStringProxyFunc, 0x1c01b2820. Non-external (a static of this unit), the
// context-description callback -appendFormat: passes. An object that responds to
// -_dynamicContextEvaluation:patternString: sets *isDeferred (when given) to true and, when replacing (a
// 32-bit test), answers that message's result retained (CFRetain, a tail call) or NULL for nil; an object
// that does not respond sets *isDeferred (when given) to false. Every other path answers NULL. Evidence:
// iphone-simulator-corpus-delta3.json.
static OCFStringRef _DescriptionWithStringProxyFunc(
    void *object, const void *context, const void *patternString, bool replacing, bool *isDeferred
) {
    if ([(id)object respondsToSelector:@selector(_dynamicContextEvaluation:patternString:)]) {
        if (isDeferred != NULL) {
            *isDeferred = true;
        }
        if (replacing) {
            id result = [(id)object _dynamicContextEvaluation:(id)context patternString:(id)patternString];
            if (result != nil) {
                return (OCFStringRef)OCFRetain((OCFTypeRef)result);
            }
        }
    } else if (isDeferred != NULL) {
        *isDeferred = false;
    }
    return NULL;
}

// Native -[__NSCFString setString:], 0x1c01b28b0. Non-external.
- (void)setString:(id)string {
    OCFIndex length = _OCFStringGetLength2((OCFStringRef)self);
    int status = __OCFStringCheckAndReplace((OCFMutableStringRef)self, 0, length, (OCFStringRef)string);
    if (status != 0) {
        mutateError(_cmd, status, self);
    }
}

// Native -[__NSCFString appendCharacters:length:], 0x1c01b2908, with its .cold.1 (0x1c0308e44).
// Non-external. An immutable string raises as -appendFormat: does; otherwise a tail call.
- (void)appendCharacters:(const UniChar *)characters length:(NSUInteger)length {
    if (!__OCFStringMtbl((OCFStringRef)self)) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException,
            OF_STR("Attempt to mutate immutable object with %s"),
            sel_getName(_cmd)
        );
    }
    OCFStringAppendCharacters((OCFMutableStringRef)self, characters, (OCFIndex)length);
}

// Native -[__NSCFString replaceOccurrencesOfString:withString:options:range:], 0x1c01b2954, with its
// function static replaceOccurrencesOfString:withString:options:range:.warnonce and its .cold.1
// (0x1c0308e5c) and .cold.2 (0x1c0308e74). Non-external. Order: the mutability check (.cold.2, "Attempt to
// mutate immutable object with %s"), the length, the nil checks of the target and the replacement (.cold.1,
// "%@: nil argument"), the range check (an end past the length raises, with the shortened-range note when the
// length is 64), the once-per-method warning of a wrapped range, then NSRegularExpressionSearch (0x400) goes
// to super; otherwise CFStringFindAndReplace with kCFCompareNonliteral (0x10) added unless
// NSLiteralSearch (2) is set (~(options << 3) & 0x10 | options).
- (NSUInteger)replaceOccurrencesOfString:(id)target
                              withString:(id)replacement
                                 options:(NSUInteger)options
                                   range:(OCFRange)range {
    static bool warnonce;
    if (!__OCFStringMtbl((OCFStringRef)self)) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException,
            OF_STR("Attempt to mutate immutable object with %s"),
            sel_getName(_cmd)
        );
    }
    OCFIndex length = _OCFStringGetLength2((OCFStringRef)self);
    if (target == nil || replacement == nil) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException, OF_STR("%@: nil argument"), __CFExceptionProem(self, _cmd)
        );
    }
    if ((uintptr_t)(range.location + range.length) > (uintptr_t)length) {
        OCFStringRef exceptionName = ONSInvalidArgumentException;
        OCFStringRef proem = __CFExceptionProem(self, _cmd);
        _CFThrowFormattedException(
            exceptionName,
            OF_STR("%@: Range {%lu, %lu} out of bounds; string length %lu%s"),
            proem,
            range.location,
            range.length,
            length,
            range.length == 0x40
                ? " (Note that the indicated range may be smaller than the original range passed to the API)"
                : ""
        );
    }
    if (((uintptr_t)length < (uintptr_t)range.length ||
         (uintptr_t)range.location > (uintptr_t)(length - range.length)) &&
        !warnonce) {
        warnonce = true;
        OCFStringRef proem = __CFExceptionProem(self, _cmd);
        OCFLog(
            4,
            OF_STR(
                "*** %@: Range {%lu, %lu} out of bounds; string length %lu. This will become an exception "
                "for "
                "apps linked after 10.10 and iOS 8. Warning shown once per app execution."
            ),
            proem,
            range.location,
            range.length,
            length
        );
    }
    if ((options & 0x400) != 0) {
        return [super replaceOccurrencesOfString:target withString:replacement options:options range:range];
    }
    return (NSUInteger)OCFStringFindAndReplace(
        (OCFMutableStringRef)self,
        (OCFStringRef)target,
        (OCFStringRef)replacement,
        range,
        (~(options << 3) & 0x10) | options
    );
}

@end

// Native mutateError, 0x1c0308c84. Non-external. Raises NSInvalidArgumentException (loaded first) for a
// failed mutation status of __CFStringCheckAndReplace, compared as a 64-bit value: 3 "%@: Range or index out
// of bounds" and 2 "%@: nil argument" (with the proem), 1 "Attempt to mutate immutable object with %s" (the
// selector name), any other "Can't call %s in %@" (the selector name and the object's class). The proem
// (OUTLINED_FUNCTION_5_3, 0x1c01b5a10: __CFExceptionProem(object, selector)), the class argument
// (OUTLINED_FUNCTION_6_3, 0x1c01b5a1c: object_getClass(object) after sel_getName) and the raise
// (OUTLINED_FUNCTION_3_5, 0x1c01b59f8: _CFThrowFormattedException with the name loaded first) are read in
// iphone-simulator-corpus-delta3.json (confidence: verified).
static void mutateError(SEL selector, OCFIndex status, const void *object) {
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
