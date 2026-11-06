//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSPlaceholderString.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSStringOriginalAttributedString.h"
#import "Foundation/String/ONSStringOtherEncodings.h"

#include <malloc/malloc.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdint.h>

// Inferred organization: the NSPlaceholderString methods lie in the NSString run of the iOS Simulator __text
// section (0x1c05d8430-0x1c05d92d8, with __NSDescriptionWithLocaleFunc and __NSDescriptionWithStringProxyFunc
// at 0x1c05d8df8-0x1c05d8eec between them), and -length and -characterAtIndex: in the outlined region
// (0x1c0fe5564-0x1c0fe55e8); following the owner's decision, the class has its own translation unit. Methods
// of the run that the scope does not reach (0x1c05d8444-0x1c05d8450, 0x1c05d849c-0x1c05d85cc,
// 0x1c05d867c-0x1c05d86e4, 0x1c05d87b0-0x1c05d8944, 0x1c05d8994-0x1c05d89c8, 0x1c05d8d38-0x1c05d8df4,
// 0x1c05d8ff4-0x1c05d9190) are not restored. The unit is Objective-C compiled with manual reference counting
// (objc_release, the immortal placeholder's memory-management methods). Unless a definition says otherwise:
// iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly
// read in full, Local/Evidence/cf-core/iphone-simulator-foundation-link.json). iOS Device divergence: iOS
// Device 24A446 remains unread. Reading: Foundation/String/ONSStringReview.md.
//
// The initializers answer a new CFString and never release the receiver (the shared placeholder). The
// allocator GOT slot 0x222280b50 lies outside the extracted image; it is kCFAllocatorDefault by the run of
// CoreFoundation allocator imports it begins (0x222280b50, 0x222280b58 and 0x222280b68 as
// kCFAllocatorDefault, kCFAllocatorMalloc and kCFAllocatorNull, the order of their names; confidence:
// medium). Likewise 0x22227fae0 is the NSException class (the receiver of
// +exceptionWithName:reason:userInfo:) and 0x22227f500 NSInvalidArgumentException (confidence: medium).

// The NSException class (through the GOT slot 0x22227fae0).

// The NSAttributedString class (through the GOT slot 0x22289ec00, __SCR_NSAttributedString).

// The encoding check every initializer of this unit inlines: the CFStringEncoding of the NSStringEncoding;
// when there is none (kCFStringEncodingInvalidId, -1) and the value is 0 or 0x8000100 (kCFStringEncodingUTF8
// passed as an NSStringEncoding), logs "Incorrect NSStringEncoding value..." when the executable is linked on
// or after version 6 and answers kCFStringEncodingASCII (0x600); any other value answers -1.
// placeholderCFEncoding is a local descriptive name.
static inline OCFStringEncoding placeholderCFEncoding(ONSUInteger encoding) {
    OCFStringEncoding cfEncoding = OCFStringConvertNSStringEncodingToEncoding(encoding);
    if (cfEncoding != (OCFStringEncoding)-1) {
        return cfEncoding;
    }
    if (encoding == 0x8000100 || encoding == 0) {
        if (_OCFExecutableLinkedOnOrAfter(6)) {
            ONSLog(
                (id)OF_STR(
                    "Incorrect NSStringEncoding value 0x%04lX detected. Assuming NSASCIIStringEncoding. "
                    "Will stop this compatibility mapping behavior in the near future."
                ),
                encoding
            );
        }
        return 0x600;
    }
    return (OCFStringEncoding)-1;
}

@implementation ONSPlaceholderString

// Native -[NSPlaceholderString autorelease], 0x1c05d8430. Non-external. Answers the receiver.
- (id)autorelease {
    return self;
}

// Native -[NSPlaceholderString retain], 0x1c05d8434. Non-external. Answers the receiver.
- (id)retain {
    return self;
}

// Native -[NSPlaceholderString retainCount], 0x1c05d8438. Non-external. Answers NSUIntegerMax.
- (ONSUInteger)retainCount {
    return (ONSUInteger)-1;
}

// Native -[NSPlaceholderString release], 0x1c05d8440. Non-external. Does nothing.
- (oneway void)release {
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

// Native -[NSPlaceholderString dealloc], 0x1c05d8454. Non-external. Does nothing (no [super dealloc]).
- (void)dealloc {
}

#pragma clang diagnostic pop

// Native -[NSPlaceholderString init], 0x1c05d8458. Non-external. Answers the empty constant string
// (stru_22281e130, flags 0x7c8, length 0).
- (id)init {
    return (id)OF_STR("");
}

// Native -[NSPlaceholderString initWithCharactersNoCopy:length:freeWhenDone:], 0x1c05d8464. Non-external.
// CFStringCreateWithCharactersNoCopy with kCFAllocatorMalloc as the contents deallocator when freeBuffer is
// set (a 32-bit test), otherwise kCFAllocatorNull.
- (id)initWithCharactersNoCopy:(unsigned short *)characters
                        length:(ONSUInteger)length
                  freeWhenDone:(BOOL)freeBuffer {
    return (id)OCFStringCreateWithCharactersNoCopy(
        kOCFAllocatorDefault,
        characters,
        (OCFIndex)length,
        freeBuffer ? kOCFAllocatorMalloc : kOCFAllocatorNull
    );
}

// Native -[NSPlaceholderString initWithCharacters:length:], 0x1c05d85d0. Non-external.
- (id)initWithCharacters:(const unsigned short *)characters length:(ONSUInteger)length {
    return (id)OCFStringCreateWithCharacters(kOCFAllocatorDefault, characters, (OCFIndex)length);
}

// Native -[NSPlaceholderString initWithCString:encoding:], 0x1c05d85e8. Non-external.
// CFStringCreateWithCString in the checked encoding.
- (id)initWithCString:(const char *)nullTerminatedCString encoding:(ONSUInteger)encoding {
    OCFAllocatorRef allocator = kOCFAllocatorDefault;
    return (id)OCFStringCreateWithCString(allocator, nullTerminatedCString, placeholderCFEncoding(encoding));
}

// Native -[NSPlaceholderString initWithBytesNoCopy:length:encoding:freeWhenDone:], 0x1c05d86e8. Non-external.
// CFStringCreateWithBytesNoCopy as an external representation in the checked encoding, with
// kCFAllocatorMalloc as the contents deallocator when freeBuffer is set (a 32-bit test), otherwise
// kCFAllocatorNull.
- (id)initWithBytesNoCopy:(void *)bytes
                   length:(ONSUInteger)length
                 encoding:(ONSUInteger)encoding
             freeWhenDone:(BOOL)freeBuffer {
    OCFAllocatorRef allocator = kOCFAllocatorDefault;
    OCFStringEncoding cfEncoding = placeholderCFEncoding(encoding);
    return (id)OCFStringCreateWithBytesNoCopy(
        allocator,
        (const UInt8 *)bytes,
        (OCFIndex)length,
        cfEncoding,
        true,
        freeBuffer ? kOCFAllocatorMalloc : kOCFAllocatorNull
    );
}

// Native -[NSPlaceholderString initWithCString:length:], 0x1c05d8948. Non-external. CFStringCreateWithBytes
// in the system encoding, not an external representation.
- (id)initWithCString:(const char *)bytes length:(ONSUInteger)length {
    OCFAllocatorRef allocator = kOCFAllocatorDefault;
    return (id)OCFStringCreateWithBytes(
        allocator, (const UInt8 *)bytes, (OCFIndex)length, OCFStringGetSystemEncoding(), false
    );
}

// Native -[NSPlaceholderString initWithString:], 0x1c05d89cc. Non-external. nil raises
// NSInvalidArgumentException ("%@: nil argument"). A tagged pointer (bit 63 set) is answered as is, without a
// retain. A string whose class or superclass is __NSCFString is copied with CFStringCreateCopy. Otherwise the
// string's fastest encoding, -_fastCStringContents:NO and -lengthOfBytesUsingEncoding: are read in that
// order: C string contents become CFStringCreateWithBytes in the checked encoding; else UTF-16 contents from
// -_fastCharacterContents become CFStringCreateWithCharacters of half the byte length; else the bytes are
// copied with -getBytes:maxLength:usedLength:encoding:options:range:remainingRange: (options 0, the whole
// string) into a buffer of length + 1 bytes, on the stack up to 0x100 bytes (CFStringCreateWithBytes) or from
// malloc_type_malloc (type 0x100004077774924) beyond, which CFStringCreateWithBytesNoCopy adopts with
// kCFAllocatorMalloc (0x222280b58).
- (id)initWithString:(id)string {
    if (string == nil) {
        id reason =
            [ONSString stringWithFormat:(id)OF_STR("%@: nil argument"), _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([(id)placeholderExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                       reason:reason
                                                                     userInfo:nil]);
    }
    if ((intptr_t)string < 0) {
        return string;
    }
    Class cls = [string class];
    if (cls == __NSCFStringClass || class_getSuperclass(cls) == __NSCFStringClass) {
        return (id)OCFStringCreateCopy(kOCFAllocatorDefault, (OCFStringRef)string);
    }
    ONSUInteger encoding = [string fastestEncoding];
    const char *cString = [string _fastCStringContents:NO];
    ONSUInteger byteLength = [string lengthOfBytesUsingEncoding:encoding];
    if (cString != NULL) {
        OCFStringEncoding cfEncoding = placeholderCFEncoding(encoding);
        return (id)OCFStringCreateWithBytes(
            kOCFAllocatorDefault, (const UInt8 *)cString, (OCFIndex)byteLength, cfEncoding, false
        );
    }
    const unsigned short *characters = [string _fastCharacterContents];
    if (characters != NULL) {
        return (id)OCFStringCreateWithCharacters(
            kOCFAllocatorDefault, characters, (OCFIndex)(byteLength >> 1)
        );
    }
    ONSUInteger bufferLength = byteLength + 1;
    ONSUInteger usedLength = 0;
    uint8_t stackBuffer[bufferLength >= 0x101 ? 1 : bufferLength];
    uint8_t *buffer = stackBuffer;
    if (bufferLength >= 0x101) {
        buffer = malloc_type_malloc(bufferLength, 0x100004077774924);
    }
    [string getBytes:buffer
             maxLength:bufferLength
            usedLength:&usedLength
              encoding:encoding
               options:0
                 range:(ONSRange){0, [string length]}
        remainingRange:NULL];
    OCFAllocatorRef allocator = kOCFAllocatorDefault;
    OCFStringEncoding cfEncoding = placeholderCFEncoding(encoding);
    if (bufferLength <= 0x100) {
        return (id)OCFStringCreateWithBytes(allocator, buffer, (OCFIndex)usedLength, cfEncoding, false);
    }
    return (id)OCFStringCreateWithBytesNoCopy(
        allocator, buffer, (OCFIndex)usedLength, cfEncoding, false, kOCFAllocatorMalloc
    );
}

// Native __NSDescriptionWithLocaleFunc, 0x1c05d8df8. External. nil describes as "*nil*"; otherwise
// -descriptionWithLocale: when the object responds to it (the selector reference 0x2387ae4cd lies outside
// the extracted image; descriptionWithLocale: by the message that follows; confidence: medium), else
// -description; a nil description becomes "*nil description*". The result is retained with CFRetain.
OCFStringRef _ONSDescriptionWithLocaleFunc(void *object, const void *locale) {
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

// Native __NSDescriptionWithStringProxyFunc, 0x1c05d8e60. External. When the object responds to
// -_dynamicContextEvaluation:patternString: (the selector reference 0x2387abbdf lies outside the extracted
// image; that selector by the message that follows; confidence: medium), sets *handled (when non-NULL) to
// true and, when evaluate is set, answers the retained evaluation if it is non-nil; otherwise sets *handled
// to false. Every other case answers NULL.
OCFStringRef _ONSDescriptionWithStringProxyFunc(
    void *object, const void *context, const void *patternString, bool evaluate, bool *handled
) {
    if (![(id)object respondsToSelector:@selector(_dynamicContextEvaluation:patternString:)]) {
        if (handled != NULL) {
            *handled = false;
        }
        return NULL;
    }
    if (handled != NULL) {
        *handled = true;
    }
    if (evaluate) {
        id result = [(id)object _dynamicContextEvaluation:context patternString:patternString];
        if (result != nil) {
            return (OCFStringRef)OCFRetain((OCFTypeRef)result);
        }
    }
    return NULL;
}

// Native -[NSPlaceholderString initWithFormat:locale:arguments:], 0x1c05d8ef0. Non-external. A nil format
// raises NSInvalidArgumentException ("%@: nil argument"). A format with a base attributed string
// (-__baseAttributedString) is formatted by a new NSAttributedString (-_initWithFormat:options:locale:
// arguments:, options 0), whose -string is copied before the attributed string is released. Otherwise
// _CFStringCreateWithFormatAndArgumentsAux2 with __NSDescriptionWithLocaleFunc and
// __NSDescriptionWithStringProxyFunc, the locale as the format options and no implicit argument.
- (id)initWithFormat:(id)format locale:(id)locale arguments:(va_list)arguments {
    if (format == nil) {
        id reason =
            [ONSString stringWithFormat:(id)OF_STR("%@: nil argument"), _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([(id)placeholderExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                       reason:reason
                                                                     userInfo:nil]);
    }
    id baseAttributedString = [format __baseAttributedString];
    if (baseAttributedString != nil) {
        id attributedString =
            [[(id)placeholderAttributedStringClass alloc] _initWithFormat:baseAttributedString
                                                                  options:0
                                                                   locale:locale
                                                                arguments:arguments];
        id result = [[attributedString string] copy];
        [attributedString release];
        return result;
    }
    return (id)_OCFStringCreateWithFormatAndArgumentsAux2(
        kOCFAllocatorDefault,
        (CopyDescriptionFunction)_ONSDescriptionWithLocaleFunc,
        (ContextDescriptionFunction)_ONSDescriptionWithStringProxyFunc,
        (OCFDictionaryRef)locale,
        NULL,
        (OCFStringRef)format,
        arguments
    );
}

// Native -[NSPlaceholderString initWithData:encoding:], 0x1c05d9194. Non-external. nil data answers the empty
// constant string (stru_22281e130); otherwise CFStringCreateFromExternalRepresentation in the checked
// encoding.
- (id)initWithData:(id)data encoding:(ONSUInteger)encoding {
    if (data == nil) {
        return (id)OF_STR("");
    }
    OCFAllocatorRef allocator = kOCFAllocatorDefault;
    return (id)OCFStringCreateFromExternalRepresentation(
        allocator, (OCFDataRef)data, placeholderCFEncoding(encoding)
    );
}

// Native -[NSPlaceholderString initWithBytes:length:encoding:], 0x1c05d9238. Non-external.
// CFStringCreateWithBytes as an external representation in the checked encoding.
- (id)initWithBytes:(const void *)bytes length:(ONSUInteger)length encoding:(ONSUInteger)encoding {
    OCFAllocatorRef allocator = kOCFAllocatorDefault;
    OCFStringEncoding cfEncoding = placeholderCFEncoding(encoding);
    return (id)OCFStringCreateWithBytes(allocator, (const UInt8 *)bytes, (OCFIndex)length, cfEncoding, true);
}

// Clang recognizes the system spelling NSLog as a format function, but cannot recognize our OF_STR
// constant-string record as an Objective-C format literal. These five fixed, substitution-free native
// messages are constant formats. Suppress only that diagnostic while preserving the native calls.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"

// Native -[NSPlaceholderString length], 0x1c0fe5564, in the outlined region. Non-external. Logs "Did you
// forget to nest alloc and init?", reports the abstract method through NSRequestConcreteImplementation with
// __NSStringClass and answers 0.
- (ONSUInteger)length {
    ONSLog((id)OF_STR("Did you forget to nest alloc and init?"));
    ONSRequestConcreteImplementation(self, _cmd, __NSStringClass);
    return 0;
}

// Native -[NSPlaceholderString characterAtIndex:], 0x1c0fe55a8, in the outlined region. Non-external. As
// -length; answers 0.
- (unsigned short)characterAtIndex:(ONSUInteger)index {
    (void)index;
    ONSLog((id)OF_STR("Did you forget to nest alloc and init?"));
    ONSRequestConcreteImplementation(self, _cmd, __NSStringClass);
    return 0;
}

#pragma clang diagnostic pop

@end
