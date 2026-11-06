//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSPlaceholderMutableString.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSPlaceholderString.h"
#import "Foundation/String/ONSRuntimeSupport.h"

#include "CoreFoundation/Base/OCFUtilities.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"

#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the NSPlaceholderMutableString methods lie together in the iOS Simulator __text
// section (0x1c057104c-0x1c0571793), after the NSMutableString methods (String/ONSMutableString.m); the three
// primitive methods -length, -characterAtIndex: and -replaceCharactersInRange:withString: lie in the outlined
// region (0x1c0fe39e0-0x1c0fe3aa3). Following the owner's decision, the class has its own translation unit,
// which keeps the native order. Objective-C compiled with manual reference counting. Unless a definition says
// otherwise: Foundation image, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified (assembly read in full, Local/Evidence/cf-core/iphone-simulator-foundation-link.json;
// strings read from the image's __cfstring and __cstring sections). iOS Device divergence: iOS Device 24A446
// remains unread. Reading: Foundation/String/ONSStringReview.md.
//
// The initializers answer new CFMutableStrings (+1) and never release the receiver (the static placeholder).
// The GOT slots outside the extracted image are bound by their use (confidence: medium): 0x22227FAE0 is the
// NSException class and 0x22227F500 NSInvalidArgumentException; 0x222280B50, 0x222280B58 and 0x222280B68 are
// kCFAllocatorDefault, kCFAllocatorMalloc and kCFAllocatorNull (the slots follow the alphabetical order of
// the imported names, as the exception slots do; the deallocator choice of -initWithBytesNoCopy:... agrees).
//
// Every initializer with an encoding converts it with CFStringConvertNSStringEncodingToEncoding; an invalid
// result (kCFStringEncodingInvalidId) for 0 or 0x8000100 logs "Incorrect NSStringEncoding value 0x%04lX
// detected. ..." (only when _CFExecutableLinkedOnOrAfter(6)) and falls back to kCFStringEncodingASCII
// (0x600); any other invalid encoding stays kCFStringEncodingInvalidId. The check is inlined at each call
// site; placeholderMutableStringEncoding spells it once.

// The NSException class symbol (CoreFoundation), loaded natively through GOT slot 0x22227FAE0.

// The inlined encoding conversion with its compatibility mapping (see above). A local descriptive name.
static inline __attribute__((always_inline)) OCFStringEncoding
placeholderMutableStringEncoding(ONSUInteger encoding) {
    OCFStringEncoding cfEncoding = OCFStringConvertNSStringEncodingToEncoding(encoding);
    if (cfEncoding == 0xFFFFFFFF && (encoding == 0x8000100 || encoding == 0)) {
        if (_OCFExecutableLinkedOnOrAfter(6)) {
            ONSLog(
                (id)OF_STR(
                    "Incorrect NSStringEncoding value 0x%04lX detected. Assuming NSASCIIStringEncoding. Will "
                    "stop this compatibility mapping behavior in the near future."
                ),
                encoding
            );
        }
        cfEncoding = 0x600;
    }
    return cfEncoding;
}

// The nil-argument raise of the initializers: NSInvalidArgumentException with "<proem>: nil argument" (or
// "<proem>: NULL cString"). A local descriptive name for the inlined code.
static inline __attribute__((always_inline, noreturn)) void
raisePlaceholderMutableStringArgument(id self, SEL _cmd, OCFStringRef format) {
    id reason = [ONSString stringWithFormat:(id)format, _ONSMethodExceptionProem(self, _cmd)];
    objc_exception_throw([(id)placeholderMutableStringExceptionClass
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:reason
                 userInfo:nil]);
}

@implementation ONSPlaceholderMutableString

// Native -[NSPlaceholderMutableString autorelease], 0x1c057104c. Non-external. The receiver.
- (instancetype)autorelease {
    return self;
}

// Native -[NSPlaceholderMutableString retain], 0x1c0571050. Non-external. The receiver.
- (instancetype)retain {
    return self;
}

// Native -[NSPlaceholderMutableString retainCount], 0x1c0571054. Non-external. UINTPTR_MAX.
- (ONSUInteger)retainCount {
    return UINTPTR_MAX;
}

// Native -[NSPlaceholderMutableString release], 0x1c057105c. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[NSPlaceholderMutableString dealloc], 0x1c0571070. Non-external. Does nothing; super is never
// reached, so the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
}

#pragma clang diagnostic pop

// The initializers reach no designated initializer of super, so that diagnostic is silenced for them.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

// Native -[NSPlaceholderMutableString init], 0x1c0571074. Non-external. A tail call of
// CFStringCreateMutable(kCFAllocatorDefault, 0).
- (id)init {
    return (id)OCFStringCreateMutable(kOCFAllocatorDefault, 0);
}

// Native -[NSPlaceholderMutableString initWithCapacity:], 0x1c0571088. Non-external. The capacity is ignored:
// CFStringCreateMutable(kCFAllocatorDefault, 0).
- (id)initWithCapacity:(ONSUInteger)capacity {
    (void)capacity;
    return (id)OCFStringCreateMutable(kOCFAllocatorDefault, 0);
}

// Native -[NSPlaceholderMutableString initWithString:], 0x1c057109c. Non-external. nil raises "%@: nil
// argument". A string whose class (objc_opt_class) or its superclass is __NSCFStringClass is copied with
// CFStringCreateMutableCopy(kCFAllocatorDefault, 0, string); any other gets an empty mutable string with
// CFStringAppend.
- (id)initWithString:(id)string {
    if (string == nil) {
        raisePlaceholderMutableStringArgument(self, _cmd, OF_STR("%@: nil argument"));
    }
    Class cls = [string class];
    if (cls == __NSCFStringClass || class_getSuperclass(cls) == __NSCFStringClass) {
        return (id)OCFStringCreateMutableCopy(kOCFAllocatorDefault, 0, (OCFStringRef)string);
    }
    OCFMutableStringRef result = OCFStringCreateMutable(kOCFAllocatorDefault, 0);
    OCFStringAppend(result, (OCFStringRef)string);
    return (id)result;
}

// Native -[NSPlaceholderMutableString initWithBytes:length:encoding:], 0x1c0571178. Non-external.
// CFStringCreateWithBytes(NULL, ..., isExternalRepresentation true); NULL answers NULL; otherwise its
// CFStringCreateMutableCopy(kCFAllocatorDefault, 0, ...), and the immutable string is released.
- (id)initWithBytes:(const void *)bytes length:(ONSUInteger)length encoding:(ONSUInteger)encoding {
    OCFStringEncoding cfEncoding = placeholderMutableStringEncoding(encoding);
    OCFStringRef string = OCFStringCreateWithBytes(NULL, bytes, (OCFIndex)length, cfEncoding, true);
    if (string == NULL) {
        return nil;
    }
    OCFMutableStringRef result = OCFStringCreateMutableCopy(kOCFAllocatorDefault, 0, string);
    OCFRelease(string);
    return (id)result;
}

// Native -[NSPlaceholderMutableString initWithUTF8String:], 0x1c0571240. Non-external. NULL raises "%@: NULL
// cString". For __mutablePlaceholder, or when _CFExecutableLinkedOnOrAfter(8): an empty
// CFStringCreateMutable(kCFAllocatorDefault, 0) string with CFStringAppendCString(kCFStringEncodingUTF8) (no
// append when the creation fails); otherwise a tail call of -initWithBytes:length:strlen
// encoding:NSUTF8StringEncoding.
- (id)initWithUTF8String:(const char *)nullTerminatedCString {
    if (nullTerminatedCString == NULL) {
        raisePlaceholderMutableStringArgument(self, _cmd, OF_STR("%@: NULL cString"));
    }
    if (self == (id)&__mutablePlaceholder || _OCFExecutableLinkedOnOrAfter(8)) {
        OCFMutableStringRef result = OCFStringCreateMutable(kOCFAllocatorDefault, 0);
        if (result != NULL) {
            OCFStringAppendCString(result, nullTerminatedCString, 0x8000100);
        }
        return (id)result;
    }
    return [self initWithBytes:nullTerminatedCString length:strlen(nullTerminatedCString) encoding:4];
}

// Native -[NSPlaceholderMutableString initWithBytesNoCopy:length:encoding:freeWhenDone:], 0x1c057132c.
// Non-external. CFStringCreateWithBytesNoCopy(NULL, ..., isExternalRepresentation true, kCFAllocatorMalloc
// when freeBuffer, else kCFAllocatorNull); then as -initWithBytes:length:encoding:.
- (id)initWithBytesNoCopy:(void *)bytes
                   length:(ONSUInteger)length
                 encoding:(ONSUInteger)encoding
             freeWhenDone:(BOOL)freeBuffer {
    OCFStringEncoding cfEncoding = placeholderMutableStringEncoding(encoding);
    OCFStringRef string = OCFStringCreateWithBytesNoCopy(
        NULL, bytes, (OCFIndex)length, cfEncoding, true, freeBuffer ? kOCFAllocatorMalloc : kOCFAllocatorNull
    );
    if (string == NULL) {
        return nil;
    }
    OCFMutableStringRef result = OCFStringCreateMutableCopy(kOCFAllocatorDefault, 0, string);
    OCFRelease(string);
    return (id)result;
}

// Native -[NSPlaceholderMutableString initWithCharactersNoCopy:length:freeWhenDone:], 0x1c0571490.
// Non-external. The characters are appended to CFStringCreateMutable(kCFAllocatorDefault, 0) (not checked for
// NULL) and freed with free() when freeBuffer.
- (id)initWithCharactersNoCopy:(unsigned short *)characters
                        length:(ONSUInteger)length
                  freeWhenDone:(BOOL)freeBuffer {
    OCFMutableStringRef result = OCFStringCreateMutable(kOCFAllocatorDefault, 0);
    OCFStringAppendCharacters(result, characters, (OCFIndex)length);
    if (freeBuffer) {
        free(characters);
    }
    return (id)result;
}

// Native -[NSPlaceholderMutableString initWithCString:encoding:], 0x1c057155c. Non-external. NULL raises "%@:
// nil argument". The mutable string is created before the encoding is converted; CFStringAppendCString
// appends with the converted encoding.
- (id)initWithCString:(const char *)nullTerminatedCString encoding:(ONSUInteger)encoding {
    if (nullTerminatedCString == NULL) {
        raisePlaceholderMutableStringArgument(self, _cmd, OF_STR("%@: nil argument"));
    }
    OCFMutableStringRef result = OCFStringCreateMutable(kOCFAllocatorDefault, 0);
    OCFStringEncoding cfEncoding = placeholderMutableStringEncoding(encoding);
    OCFStringAppendCString(result, nullTerminatedCString, cfEncoding);
    return (id)result;
}

// Native -[NSPlaceholderMutableString initWithFormat:locale:arguments:], 0x1c05716d8. Non-external. nil
// format raises "%@: nil argument". _CFStringAppendFormatAndArgumentsAux2 into
// CFStringCreateMutable(kCFAllocatorDefault, 0) with __NSDescriptionWithLocaleFunc,
// __NSDescriptionWithStringProxyFunc, the locale as the format options and no implicit argument.
- (id)initWithFormat:(id)format locale:(id)locale arguments:(va_list)arguments {
    if (format == nil) {
        raisePlaceholderMutableStringArgument(self, _cmd, OF_STR("%@: nil argument"));
    }
    OCFMutableStringRef result = OCFStringCreateMutable(kOCFAllocatorDefault, 0);
    _OCFStringAppendFormatAndArgumentsAux2(
        result,
        (CopyDescriptionFunction)_ONSDescriptionWithLocaleFunc,
        (ContextDescriptionFunction)_ONSDescriptionWithStringProxyFunc,
        (OCFDictionaryRef)locale,
        NULL,
        (OCFStringRef)format,
        arguments
    );
    return (id)result;
}

#pragma clang diagnostic pop

// Clang recognizes the system spelling NSLog as a format function, but cannot recognize our OF_STR
// constant-string record as an Objective-C format literal. These five fixed, substitution-free native
// messages are constant formats. Suppress only that diagnostic while preserving the native calls.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"

// Native -[NSPlaceholderMutableString length], 0x1c0fe39e0 (outlined region). Non-external. Logs "Did you
// forget to nest alloc and init?", then NSRequestConcreteImplementation(self, _cmd, __NSMutableStringClass);
// answers 0.
- (ONSUInteger)length {
    ONSLog((id)OF_STR("Did you forget to nest alloc and init?"));
    ONSRequestConcreteImplementation(self, _cmd, __NSMutableStringClass);
    return 0;
}

// Native -[NSPlaceholderMutableString characterAtIndex:], 0x1c0fe3a24 (outlined region). Non-external. As
// -length; answers 0.
- (unsigned short)characterAtIndex:(ONSUInteger)index {
    (void)index;
    ONSLog((id)OF_STR("Did you forget to nest alloc and init?"));
    ONSRequestConcreteImplementation(self, _cmd, __NSMutableStringClass);
    return 0;
}

// Native -[NSPlaceholderMutableString replaceCharactersInRange:withString:], 0x1c0fe3a68 (outlined region).
// Non-external. As -length, with NSRequestConcreteImplementation as a tail call.
- (void)replaceCharactersInRange:(ONSRange)range withString:(id)string {
    (void)range;
    (void)string;
    ONSLog((id)OF_STR("Did you forget to nest alloc and init?"));
    ONSRequestConcreteImplementation(self, _cmd, __NSMutableStringClass);
}

#pragma clang diagnostic pop

@end
