//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSString.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Locale/OCFLocale.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"
#import "Foundation/String/ONSAllocateObject.h"
#import "Foundation/String/ONSFileUtilities.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSMutableString.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSStringFromRange.h"
#import "Foundation/String/ONSStringOtherEncodings.h"
#import "Foundation/String/ONSStringRegularExpressionSupport.h"
#import "Foundation/String/ONSStringScanning.h"

#include <malloc/malloc.h>
#include <math.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xlocale.h>

// Inferred organization: the NSString methods lie together in the iOS Simulator __text section
// (0x1c05d1e14-0x1c05d8428, followed by the NSPlaceholderString methods from 0x1c05d8430), and the abstract
// methods -length, -characterAtIndex:, -initWithCharactersNoCopy:length:freeWhenDone:,
// -initWithBytesNoCopy:length:encoding:freeWhenDone: and -initWithFormat:locale:arguments: in the outlined
// region (0x1c0fe53e0-0x1c0fe54f4); following the owner's decision, the class has its own translation unit
// and its categories have theirs. The unit is Objective-C compiled with manual reference counting
// (objc_autorelease, explicit releases). Unless a definition says otherwise: Foundation image, iOS Simulator
// 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// iphone-simulator-foundation-link.json; where the IDA pseudocode drops a check, the assembly settles it).
// iOS Device divergence: iOS Device 24A446 remains unread. Reading: Foundation/String/ONSStringReview.md.
//
// The GOT slots the methods load lie outside the extracted image; they are bound by their use and by the
// alphabetical order of the shared cache's import slots (confidence: medium): 0x22227F500
// NSInvalidArgumentException and 0x22227F5E0 NSRangeException (the exception names of the nil-argument and
// out-of-bounds reasons), 0x22227FA90 NSArray, 0x22227FAE0 NSException (the receiver of
// +exceptionWithName:reason:userInfo:), 0x22227FB00 NSLocale (the -isKindOfClass: argument of the locale
// check), 0x22227FB18 NSMutableArray (+array, then -addObject:), 0x222280B50 kCFAllocatorDefault (the
// creation allocator, ahead of kCFAllocatorMalloc and kCFAllocatorNull at 0x222280B58 and 0x222280B68),
// 0x2222806A8
// __CFDefaultEightBitStringEncoding and 0x222417EE8 __stack_chk_guard.

// The class symbols the methods reference through GOT slots or class references.

// dyld's SPI (libdyld, re-exported by libSystem); +[NSString initialize] passes 0x07DF0901FFFFFFFF in X0.
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);
static const OCFBuildVersion kTaggedStringDefaultVersion = {0xFFFFFFFF, 0x07DF0901};

// NSNotFound.
#define kNotFound ((ONSUInteger)0x7FFFFFFFFFFFFFFF)

Class __NSCFStringClass = Nil;
Class __NSMutableStringClass = Nil;
Class __NSStringClass = Nil;

// Native ___placeholder (0x2227d76d8, __DATA_CONST,__const, non-external): the shared placeholder instance
// +[NSString allocWithZone:] answers for NSString itself, 16 bytes: the NSPlaceholderString class pointer,
// then zero. PlaceholderObject is a local name for its layout.
typedef struct {
    const void *isa;
    uintptr_t reserved;
} PlaceholderObject;

static const PlaceholderObject __placeholder = {placeholderStringClassSymbol, 0};

// The warn-once flags of the invalid-range logs (__DATA,__bss, one byte each, non-external natively):
// _rangeOfString:options:range:locale:.warnonce (0x22e6fcc88) and
// _rangeOfCharacterFromSet:options:range:.warnonce (0x22e6fcc89).
static bool rangeOfStringWarnOnce = false;
static bool rangeOfCharacterFromSetWarnOnce = false;

// The character buffer the native methods build on their stack (0x70 bytes, zeroed first): the count of
// buffered characters, the index after the current one, the string, its length, the location of the buffer in
// the string, 32 buffered characters and the current character (0xFFFF past either end). Every refill sends
// -getCharacters:range: through objc_msgSend with the selector from sel_getUid. StringCharacterBuffer and its
// field names are local descriptive spellings.
typedef struct {
    ONSUInteger bufferLength;
    ONSUInteger index;
    id string;
    ONSUInteger length;
    ONSUInteger location;
    unsigned short buffer[32];
    unsigned short current;
} StringCharacterBuffer;

_Static_assert(sizeof(StringCharacterBuffer) == 0x70, "The character buffer spans 0x6a bytes plus padding.");

static inline void
stringCharacterBufferFill(StringCharacterBuffer *characters, ONSUInteger location, ONSUInteger count) {
    ((void (*)(id, SEL, unsigned short *, ONSRange))objc_msgSend)(
        characters->string,
        sel_getUid("getCharacters:range:"),
        characters->buffer,
        (ONSRange){location, count}
    );
}

// The throws the argument checks inline: the reason is formatted with +[NSString stringWithFormat:] around
// the method's _NSMethodExceptionProem, then +[NSException exceptionWithName:reason:userInfo:] (nil user
// info) is thrown.
static void throwException(OCFStringRef name, id reason) __attribute__((noreturn));

static void throwException(OCFStringRef name, id reason) {
    objc_exception_throw([(Class)exceptionClassSymbol exceptionWithName:(id)name reason:reason userInfo:nil]);
}

static void throwNilArgument(id object, SEL selector) __attribute__((noreturn));

static void throwNilArgument(id object, SEL selector) {
    id proem = _ONSMethodExceptionProem(object, selector);
    id reason = [ONSString stringWithFormat:(id)OF_STR("%@: nil argument"), proem];
    throwException(ONSInvalidArgumentException, reason);
}

static void throwNullCString(id object, SEL selector) __attribute__((noreturn));

static void throwNullCString(id object, SEL selector) {
    id proem = _ONSMethodExceptionProem(object, selector);
    id reason = [ONSString stringWithFormat:(id)OF_STR("%@: NULL cString"), proem];
    throwException(ONSInvalidArgumentException, reason);
}

static void throwRangeOutOfBounds(id object, SEL selector, ONSRange range, ONSUInteger length)
    __attribute__((noreturn));

static void throwRangeOutOfBounds(id object, SEL selector, ONSRange range, ONSUInteger length) {
    id proem = _ONSMethodExceptionProem(object, selector);
    id reason =
        [ONSString stringWithFormat:(id)OF_STR("%@: Range {%lu, %lu} out of bounds; string length %lu"),
                                    proem,
                                    range.location,
                                    range.length,
                                    length];
    throwException(ONSRangeException, reason);
}

static void throwIndexOutOfBounds(id object, SEL selector, ONSUInteger index, ONSUInteger length)
    __attribute__((noreturn));

static void throwIndexOutOfBounds(id object, SEL selector, ONSUInteger index, ONSUInteger length) {
    id proem = _ONSMethodExceptionProem(object, selector);
    id reason = [ONSString
        stringWithFormat:(id)OF_STR("%@: Index %lu out of bounds; string length %lu"), proem, index, length];
    throwException(ONSRangeException, reason);
}

// The invalid-range check of -rangeOfString:options:range:locale: and -rangeOfCharacterFromSet:options:range:
// (inlined natively): an out-of-bounds range throws NSRangeException for executables linked on or after
// CoreFoundation version 6 and otherwise logs once per flag.
static void checkSearchRange(id object, SEL selector, ONSRange range, ONSUInteger length, bool *warnOnce) {
    if (length < range.length || range.location > length - range.length) {
        if (_OCFExecutableLinkedOnOrAfter(6)) {
            throwRangeOutOfBounds(object, selector, range, length);
        }
        if (!*warnOnce) {
            *warnOnce = true;
            id proem = _ONSMethodExceptionProem(object, selector);
            ONSLog(
                (id)OF_STR(
                    "*** %@: Invalid range %@; this will become an exception for apps linked on "
                    "SnowLeopard. Warning shown once per app execution."
                ),
                proem,
                ONSStringFromRange(range)
            );
        }
    }
}

// Includes part B needs beyond String/ONSString.h.

// Class symbols part B compares with or sends to, through the image's GOT slots. NSCharacterSet
// (off_22289ECA0), NSKeyedArchiver (___SCR_NSKeyedArchiver) and NSError (off_22289EE28) are Foundation's
// (other families); NSException, NSData, NSMutableData, NSDictionary and NSURL are classes of the
// CoreFoundation image reached through shared-cache GOT slots outside the extracted image (0x22227FAE0,
// 0x22227FAB8, 0x22227FB20, 0x22227FAD0, 0x22227FB88). Those slots are identified by use and by their order:
// the slots 0x22227F4F8-0x22227F5E0 follow the alphabetical order of Foundation's CoreFoundation imports
// (NSInternalInconsistencyException, NSInvalidArgumentException, ... NSMallocException at 0x22227F5D8,
// NSRangeException at 0x22227F5E0), and the class slots keep the same order (NSArray 0x22227FA90 < NSData
// < NSDictionary < NSException < NSMutableData < NSURL); confidence: medium. NSHTTPURLResponse
// (0x222278DA8), NSURLConnection (0x222278DE0) and NSURLRequest (0x222278E08) are CFNetwork's, identified the
// same way (confidence: medium); their class symbols stay undefined.

#ifdef __OBJC__
// Selectors part B sends that String/ONSString.h's StringForeignMessages lacks, declared for the message
// sends only; no protocol object is emitted.

#endif

// The forward step: the next buffered character; past the buffer, the next block of at most 32 characters
// (the index restarts at 1); past the string, 0xFFFF with the index one past the count.
static inline unsigned short readerNext(StringCharacterBuffer *reader) {
    if (reader->index < reader->bufferLength) {
        return reader->buffer[reader->index++];
    }
    ONSUInteger location = reader->location + reader->bufferLength;
    if (reader->length > location) {
        ONSUInteger remaining = reader->length - location;
        ONSUInteger count = remaining < 32 ? remaining : 32;
        reader->location = location;
        reader->bufferLength = count;
        stringCharacterBufferFill(reader, location, count);
        reader->index = 1;
        return reader->buffer[0];
    }
    reader->index = reader->bufferLength + 1;
    return 0xFFFF;
}

// The backward step: the character before the current one; before the buffer, the previous block of at most
// 32 characters (the index restarts at the count); before the string, 0xFFFF with the index 0.
static inline unsigned short readerPrevious(StringCharacterBuffer *reader) {
    if (reader->index >= 2) {
        reader->index--;
        return reader->buffer[reader->index - 1];
    }
    ONSUInteger location = reader->location;
    if (location == 0) {
        reader->index = 0;
        return 0xFFFF;
    }
    ONSUInteger count = location < 32 ? location : 32;
    reader->bufferLength = count;
    reader->index = count;
    reader->location = location - count;
    stringCharacterBufferFill(reader, location - count, count);
    return reader->buffer[reader->index - 1];
}

// The block-separator test both -_getBlockStart:... loops inline: with stopAtLineSeparators, LF, CR, NEL
// (0x85), LINE SEPARATOR (0x2028) and PARAGRAPH SEPARATOR (0x2029), skipping 0x0E-0x84 first; otherwise LF,
// PARAGRAPH SEPARATOR and CR, skipping 0x0E-0x2028 first.
static inline bool isBlockSeparator(unsigned short character, BOOL stopAtLineSeparators) {
    if (stopAtLineSeparators) {
        if ((unsigned short)(character - 0xE) < 0x77) {
            return false;
        }
        if (character > 0x2027) {
            return (unsigned int)(character - 0x2028) < 2;
        }
        return character == 0xA || character == 0xD || character == 0x85;
    }
    if ((unsigned short)(character - 0xE) < 0x201B) {
        return false;
    }
    return character == 0xA || character == 0x2029 || character == 0xD;
}

// Native writeStringToURLOrPath, 0x1c05d6df8 (non-external); defined in native order after
// -getExternalRepresentation:extendedAttributes:forWritingToURLOrPath:usingEncoding:error:.
static BOOL
writeStringToURLOrPath(id string, id urlOrPath, unsigned int options, ONSUInteger encoding, id *error);

@implementation ONSString

// Native +[NSString initialize], 0x1c05d1e14. Non-external. Only for NSString itself (objc_opt_class): the
// default C string encoding is initialized unless ___NSDefaultStringEncodingFullyInited has bit 0 set; then,
// when the runtime has the NSTaggedPointerString class (looked up by its mapped name, so a prefixed build
// never finds the system's), it becomes an NSString subclass (class_setSuperclass) and is sent
// +_setAsTaggedStringClass if the program SDK is at least 2015.9.1 (0x07DF0901, all platforms) and
// NSStringDisableTagged is unset, or, for older SDKs, NSStringEnableTagged is set.
+ (void)initialize {
    if (self != [ONSString class]) {
        return;
    }
    if (!(__ONSDefaultStringEncodingFullyInited & 1)) {
        _ONSDefaultCStringEncoding();
    }
    Class taggedClass = objc_lookUpClass(OF_NAME_STRING(ONSTaggedPointerString));
    if (taggedClass == Nil) {
        return;
    }
    if (dyld_program_sdk_at_least(kTaggedStringDefaultVersion)) {
        if (getenv("NSStringDisableTagged") != NULL) {
            return;
        }
    } else if (getenv("NSStringEnableTagged") == NULL) {
        return;
    }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    class_setSuperclass(taggedClass, self);
#pragma clang diagnostic pop
    [taggedClass _setAsTaggedStringClass];
}

// Native +[NSString allocWithZone:], 0x1c05d1ebc. Non-external. NSString itself (__NSStringClass) answers the
// shared placeholder; any other class gets NSAllocateObject(self, 0, zone).
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == __NSStringClass) {
        return (id)&__placeholder;
    }
    return ONSAllocateObject(self, 0, zone);
}

// Native -[NSString init], 0x1c05d1ee0. Non-external. Answers self.
- (id)init {
    return self;
}

// Native +[NSString string], 0x1c05d1ee4. Non-external. objc_allocWithZone, -init, autoreleased.
+ (id)string {
    return [[[self allocWithZone:NULL] init] autorelease];
}

// Native +[NSString stringWithString:], 0x1c05d1efc. Non-external.
+ (id)stringWithString:(id)string {
    return [[[self allocWithZone:NULL] initWithString:string] autorelease];
}

// Native +[NSString stringWithCharacters:length:], 0x1c05d1f24. Non-external.
+ (id)stringWithCharacters:(const unsigned short *)characters length:(ONSUInteger)length {
    return [[[self allocWithZone:NULL] initWithCharacters:characters length:length] autorelease];
}

// Native +[NSString stringWithCString:encoding:], 0x1c05d1ffc. Non-external. A NULL C string throws
// NSInvalidArgumentException ("%@: NULL cString"); otherwise strlen precedes the allocation and
// -initWithBytes:length:encoding: gets the bytes.
+ (id)stringWithCString:(const char *)cString encoding:(ONSUInteger)encoding {
    if (cString == NULL) {
        throwNullCString(self, _cmd);
    }
    size_t length = strlen(cString);
    return [[[self allocWithZone:NULL] initWithBytes:cString length:length encoding:encoding] autorelease];
}

// Native +[NSString stringWithUTF8String:], 0x1c05d20a0. Non-external. As +stringWithCString:encoding: with
// NSUTF8StringEncoding (4).
+ (id)stringWithUTF8String:(const char *)nullTerminatedCString {
    if (nullTerminatedCString == NULL) {
        throwNullCString(self, _cmd);
    }
    size_t length = strlen(nullTerminatedCString);
    return [[[self allocWithZone:NULL] initWithBytes:nullTerminatedCString length:length
                                            encoding:4] autorelease];
}

// Native +[NSString stringWithBytes:length:encoding:], 0x1c05d2140. Non-external. NULL bytes raise.
+ (id)stringWithBytes:(const void *)bytes length:(NSUInteger)length encoding:(NSUInteger)encoding {
    if (bytes == NULL) {
        throwException(
            ONSInvalidArgumentException,
            [ONSString stringWithFormat:(id)OF_STR("%@: NULL cString"), _ONSMethodExceptionProem(self, _cmd)]
        );
    }
    return objc_autorelease([[(Class)self allocWithZone:NULL] initWithBytes:bytes
                                                                     length:length
                                                                   encoding:encoding]);
}

// Native +[NSString stringWithFormat:], 0x1c05d2378. Non-external. -initWithFormat:locale:arguments: with a
// nil locale over the variadic arguments, autoreleased.
+ (id)stringWithFormat:(id)format, ... {
    va_list arguments;
    va_start(arguments, format);
    id string = [[self allocWithZone:NULL] initWithFormat:format locale:nil arguments:arguments];
    va_end(arguments);
    return [string autorelease];
}

// Native -[NSString _fastCharacterContents], 0x1c05d2640. Non-external. Answers NULL.
- (const unsigned short *)_fastCharacterContents {
    return NULL;
}

// Native -[NSString _fastCStringContents:], 0x1c05d2648. Non-external. Answers NULL.
- (const char *)_fastCStringContents:(BOOL)nullTerminationRequired {
    (void)nullTerminationRequired;
    return NULL;
}

// Native -[NSString _fastUTF8StringContents:utf8Length:], 0x1c05d2650. Non-external.
// _CFStringGetCStringPtrWithOptions with kCFStringEncodingASCII (0x600), the caller's null-termination flag
// (X2 passed through) and 1; a pointer stores -length into *length.
- (const char *)_fastUTF8StringContents:(BOOL)nullTerminationRequired utf8Length:(ONSUInteger *)length {
    const char *contents =
        _OCFStringGetCStringPtrWithOptions((OCFStringRef)self, 0x600, nullTerminationRequired, 1);
    if (contents != NULL) {
        *length = [self length];
    }
    return contents;
}

// Native -[NSString _isCString], 0x1c05d269c. Non-external. Whether -_fastCStringContents:NO answers a
// pointer.
- (BOOL)_isCString {
    return [self _fastCStringContents:NO] != NULL;
}

// Native -[NSString _stringRepresentation], 0x1c05d26bc. Non-external. The empty string answers the constant
// @"\"\"". Otherwise the characters are read through the stack character buffer while they are ASCII letters
// or digits ((c & 0xFFDF) - 'A' < 26 or c - '0' < 10, unsigned): a string made only of them answers an
// autoreleased -copyWithZone:NULL; the first other character answers -quotedStringRepresentation.
- (id)_stringRepresentation {
    ONSUInteger length = [self length];
    if (length == 0) {
        return (id)OF_STR("\"\"");
    }
    StringCharacterBuffer characters = {0};
    characters.string = self;
    characters.length = length;
    characters.bufferLength = length < 0x20 ? length : 0x20;
    stringCharacterBufferFill(&characters, 0, characters.bufferLength);
    unsigned short character = characters.buffer[0];
    characters.current = character;
    characters.index = 1;
    for (;;) {
        if (characters.index - 1 >= characters.bufferLength) {
            return [[self copyWithZone:NULL] autorelease];
        }
        bool alphanumeric =
            (uint32_t)((character & 0xFFDF) - 'A') < 0x1A || (uint32_t)(character - '0') < 0xA;
        if (characters.index < characters.bufferLength) {
            character = characters.buffer[characters.index];
            characters.index++;
        } else {
            ONSUInteger next = characters.location + characters.bufferLength;
            if (characters.length > next) {
                ONSUInteger remaining = characters.length - next;
                ONSUInteger count = remaining < 0x20 ? remaining : 0x20;
                characters.location = next;
                characters.bufferLength = count;
                stringCharacterBufferFill(&characters, next, count);
                character = characters.buffer[0];
                characters.index = 1;
            } else {
                characters.index = characters.bufferLength + 1;
                character = 0xFFFF;
            }
        }
        characters.current = character;
        if (!alphanumeric) {
            return [self quotedStringRepresentation];
        }
    }
}

// Native -[NSString quotedStringRepresentation], 0x1c05d287c. Non-external. The empty string answers the
// constant @"\"\"". Otherwise an ASCII buffer of length * 6 + 3 bytes (length + 9 when that exceeds 100000)
// from CFAllocatorAllocateTyped (default allocator, type 0x100004077774924, hint 0) gets an opening quote and
// each character in turn: BEL, BS, HT, LF, VT, FF, '\\' and '"' as backslash escapes (a b t n v f \ "),
// characters from 0x80 as "\U" and four lower-case hex digits, NUL dropped, the rest as one byte. Before each
// character, when the output index exceeds the capacity - 8, the capacity grows by min(capacity,
// (length + 1 - index - location) * 6 + 2) through __CFSafelyReallocateWithAllocatorTyped (X3 0x528098FA). A
// closing quote and NUL end the buffer, which is shrunk to its used size (X3 0x930A1E3F) and adopted by
// CFStringCreateWithCStringNoCopy (kCFAllocatorDefault, ASCII, default deallocator); autoreleased. A failed
// allocation or growth answers nil.
- (id)quotedStringRepresentation {
    ONSUInteger length = [self length];
    if (length == 0) {
        return (id)OF_STR("\"\"");
    }
    ONSUInteger capacity = length * 6 + 3;
    if (capacity > 100000) {
        capacity = length + 9;
    }
    char *output = OCFAllocatorAllocateTyped(NULL, capacity, 0x100004077774924, 0);
    if (output == NULL) {
        return nil;
    }
    output[0] = '"';
    StringCharacterBuffer characters = {0};
    characters.string = self;
    characters.length = length;
    characters.bufferLength = length < 0x20 ? length : 0x20;
    stringCharacterBufferFill(&characters, 0, characters.bufferLength);
    unsigned short character = characters.buffer[0];
    characters.current = character;
    characters.index = 1;
    ONSUInteger position = 1;
    if (characters.bufferLength != 0) {
        do {
            if (position > capacity - 8) {
                ONSUInteger needed = (length + 1 - characters.index - characters.location) * 6 + 2;
                capacity += capacity < needed ? capacity : needed;
                char *grown =
                    __OCFSafelyReallocateWithAllocatorTyped(NULL, output, capacity, 0x528098FA, 0, NULL);
                if (grown == NULL) {
                    return nil;
                }
                output = grown;
                character = characters.current;
            }
            char escape = 0;
            switch (character) {
                case 7:
                    escape = 'a';
                    break;
                case 8:
                    escape = 'b';
                    break;
                case 9:
                    escape = 't';
                    break;
                case 10:
                    escape = 'n';
                    break;
                case 11:
                    escape = 'v';
                    break;
                case 12:
                    escape = 'f';
                    break;
                case '\\':
                    escape = '\\';
                    break;
                case '"':
                    escape = '"';
                    break;
                default:
                    break;
            }
            if (escape != 0) {
                output[position] = '\\';
                output[position + 1] = escape;
                position += 2;
            } else if (character >= 0x80) {
                static const char hexDigits[] = "0123456789abcdef";
                output[position] = '\\';
                output[position + 1] = 'U';
                output[position + 2] = hexDigits[character >> 12];
                output[position + 3] = hexDigits[(character >> 8) & 0xF];
                output[position + 4] = hexDigits[(character >> 4) & 0xF];
                output[position + 5] = hexDigits[character & 0xF];
                position += 6;
            } else if ((character & 0xFF) != 0) {
                output[position] = (char)character;
                position++;
            }
            if (characters.index < characters.bufferLength) {
                character = characters.buffer[characters.index];
                characters.index++;
            } else {
                ONSUInteger next = characters.location + characters.bufferLength;
                if (characters.length > next) {
                    ONSUInteger remaining = characters.length - next;
                    ONSUInteger count = remaining < 0x20 ? remaining : 0x20;
                    characters.location = next;
                    characters.bufferLength = count;
                    stringCharacterBufferFill(&characters, next, count);
                    character = characters.buffer[0];
                    characters.index = 1;
                } else {
                    characters.index = characters.bufferLength + 1;
                    character = 0xFFFF;
                }
            }
            characters.current = character;
        } while (characters.index - 1 < characters.bufferLength);
    }
    output[position] = '"';
    output[position + 1] = '\0';
    output = __OCFSafelyReallocateWithAllocatorTyped(NULL, output, position + 2, 0x930A1E3F, 0, NULL);
    return [(id)OCFStringCreateWithCStringNoCopy(kOCFAllocatorDefault, output, 0x600, NULL) autorelease];
}

// Native -[NSString description], 0x1c05d2c1c. Non-external. Answers self.
- (id)description {
    return self;
}

// Native -[NSString copyWithZone:], 0x1c05d2c20. Non-external. +[NSString allocWithZone:zone], then
// -initWithString:self.
- (id)copyWithZone:(struct _NSZone *)zone {
    return [[ONSString allocWithZone:zone] initWithString:self];
}

// Native -[NSString mutableCopyWithZone:], 0x1c05d2c4c. Non-external. +[NSMutableString allocWithZone:zone],
// then -initWithString:self.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return [[ONSMutableString allocWithZone:zone] initWithString:self];
}

// Native -[NSString isEqual:], 0x1c05d2c78. Non-external. The same object answers YES; an object that is not
// an NSString (_NSIsNSString, bit 0 of W0 by CBZ W0) answers NO; otherwise -isEqualToString:.
- (BOOL)isEqual:(id)object {
    if (self == object) {
        return YES;
    }
    if (!_ONSIsNSString(object)) {
        return NO;
    }
    return [self isEqualToString:object];
}

// Native -[NSString hash], 0x1c05d2cc4. Non-external. A tail call of CFStringHashNSString.
- (ONSUInteger)hash {
    return OCFStringHashNSString((OCFStringRef)self);
}

// Native -[NSString isEqualToString:], 0x1c05d2cc8. Non-external. The same object answers YES; otherwise
// whether -compare:options:NSLiteralSearch (2) range:{0, -length} answers NSOrderedSame.
- (BOOL)isEqualToString:(id)string {
    if (self == string) {
        return YES;
    }
    return [self compare:string options:2 range:(ONSRange){0, [self length]}] == ONSOrderedSame;
}

// Native -[NSString compare:options:range:], 0x1c05d2d1c. Non-external. The same object answers
// NSOrderedSame; otherwise -compare:options:range:locale: with a nil locale.
- (ONSComparisonResult)compare:(id)string options:(ONSUInteger)mask range:(ONSRange)range {
    if (self == string) {
        return ONSOrderedSame;
    }
    return [self compare:string options:mask range:range locale:nil];
}

// Native -[NSString compare:options:range:locale:], 0x1c05d2d34. Non-external. The same object answers
// NSOrderedSame and a nil string NSOrderedDescending. A locale that is not an NSLocale (bit 0 of
// objc_opt_isKindOfClass) is dropped and adds kCFCompareLocalized (0x20). CFStringCompareWithOptionsAndLocale
// gets kCFCompareNonliteral (0x10) added unless NSLiteralSearch (2) is set.
- (ONSComparisonResult)compare:(id)string options:(ONSUInteger)mask range:(ONSRange)range locale:(id)locale {
    if (self == string) {
        return ONSOrderedSame;
    }
    if (string == nil) {
        return ONSOrderedDescending;
    }
    if (locale != nil) {
        if (![locale isKindOfClass:(Class)localeClassSymbol]) {
            locale = nil;
            mask |= 0x20;
        }
    }
    return (ONSComparisonResult)OCFStringCompareWithOptionsAndLocale(
        (OCFStringRef)self,
        (OCFStringRef)string,
        (OCFRange){(OCFIndex)range.location, (OCFIndex)range.length},
        mask | (~(mask << 3) & 0x10),
        (OCFLocaleRef)locale
    );
}

// Native -[NSString compare:], 0x1c05d2e40. Non-external. The same object answers NSOrderedSame; otherwise
// -compare:options:0 range:{0, -length}.
- (ONSComparisonResult)compare:(id)string {
    if (self == string) {
        return ONSOrderedSame;
    }
    return [self compare:string options:0 range:(ONSRange){0, [self length]}];
}

// Native -[NSString caseInsensitiveCompare:], 0x1c05d2e88. Non-external.
// -compare:options:NSCaseInsensitiveSearch (1) range:{0, -length}, without the identity check.
- (ONSComparisonResult)caseInsensitiveCompare:(id)string {
    return [self compare:string options:1 range:(ONSRange){0, [self length]}];
}

// Native -[NSString rangeOfString:options:range:locale:], 0x1c05d30c0. Non-external. The search string's
// -length is read first, then the receiver's. An out-of-bounds range throws NSRangeException for executables
// linked on or after CoreFoundation version 6, otherwise logs once
// (_rangeOfString:options:range:locale:.warnonce) and continues. A nil search string throws
// NSInvalidArgumentException. NSRegularExpressionSearch (0x400) tail-calls
// -_rangeOfRegularExpressionPattern:options:range:locale:. An empty range or search string answers
// {NSNotFound, 0}; otherwise CFStringFindWithOptionsAndLocale (kCFCompareNonliteral added unless
// NSLiteralSearch) answers the found range or {NSNotFound, 0}.
- (ONSRange)rangeOfString:(id)searchString options:(ONSUInteger)mask range:(ONSRange)range locale:(id)locale {
    OCFRange result = {0, 0};
    ONSUInteger searchLength = [searchString length];
    ONSUInteger length = [self length];
    checkSearchRange(self, _cmd, range, length, &rangeOfStringWarnOnce);
    if (searchString == nil) {
        throwNilArgument(self, _cmd);
    }
    if ((mask & 0x400) != 0) {
        return [self _rangeOfRegularExpressionPattern:searchString options:mask range:range locale:locale];
    }
    if (range.length == 0 || searchLength == 0) {
        return (ONSRange){kNotFound, 0};
    }
    if (!OCFStringFindWithOptionsAndLocale(
            (OCFStringRef)self,
            (OCFStringRef)searchString,
            (OCFRange){(OCFIndex)range.location, (OCFIndex)range.length},
            mask | (~(mask << 3) & 0x10),
            (OCFLocaleRef)locale,
            &result
        )) {
        return (ONSRange){kNotFound, 0};
    }
    return (ONSRange){(ONSUInteger)result.location, (ONSUInteger)result.length};
}

// Native -[NSString rangeOfString:options:range:], 0x1c05d32f8. Non-external. A nil locale.
- (ONSRange)rangeOfString:(id)searchString options:(ONSUInteger)mask range:(ONSRange)range {
    return [self rangeOfString:searchString options:mask range:range locale:nil];
}

// Native -[NSString rangeOfString:options:], 0x1c05d3300. Non-external. The whole string, a nil locale.
- (ONSRange)rangeOfString:(id)searchString options:(NSUInteger)options {
    return [self rangeOfString:searchString options:options range:(ONSRange){0, [self length]} locale:nil];
}

// Native -[NSString rangeOfString:], 0x1c05d3348. Non-external. No options, the whole string, a nil locale.
- (ONSRange)rangeOfString:(id)searchString {
    return [self rangeOfString:searchString options:0 range:(ONSRange){0, [self length]} locale:nil];
}

// Native -[NSString rangeOfCharacterFromSet:options:range:], 0x1c05d3384. Non-external. The range check of
// -rangeOfString:options:range:locale: (its own warn-once flag), then a nil set throws
// NSInvalidArgumentException; CFStringFindCharacterFromSet gets the options unchanged and answers the found
// range or {NSNotFound, 0} (W0 compared with zero).
- (ONSRange)rangeOfCharacterFromSet:(id)searchSet options:(ONSUInteger)mask range:(ONSRange)range {
    OCFRange result = {0, 0};
    ONSUInteger length = [self length];
    checkSearchRange(self, _cmd, range, length, &rangeOfCharacterFromSetWarnOnce);
    if (searchSet == nil) {
        throwNilArgument(self, _cmd);
    }
    Boolean found = OCFStringFindCharacterFromSet(
        (OCFStringRef)self,
        (OCFCharacterSetRef)searchSet,
        (OCFRange){(OCFIndex)range.location, (OCFIndex)range.length},
        mask,
        &result
    );
    if (!found) {
        return (ONSRange){kNotFound, 0};
    }
    return (ONSRange){(ONSUInteger)result.location, (ONSUInteger)result.length};
}

// Native -[NSString rangeOfCharacterFromSet:options:], 0x1c05d352c. Non-external. The whole string.
- (ONSRange)rangeOfCharacterFromSet:(id)searchSet options:(ONSUInteger)mask {
    return [self rangeOfCharacterFromSet:searchSet options:mask range:(ONSRange){0, [self length]}];
}

// Native -[NSString rangeOfCharacterFromSet:], 0x1c05d3570. Non-external. No options, the whole string.
- (ONSRange)rangeOfCharacterFromSet:(id)searchSet {
    return [self rangeOfCharacterFromSet:searchSet options:0 range:(ONSRange){0, [self length]}];
}

// Native -[NSString componentsSeparatedByString:], 0x1c05d35a8. Non-external. Without a match (zero length),
// +[NSArray arrayWithObject:] of a -copy, which is released. Otherwise +[NSMutableArray array] collects the
// -_newSubstringWithRange:zone: pieces (the receiver's -zone) between matches of -rangeOfString:options:0
// range:, each released after -addObject:, and the remainder after the last match.
- (id)componentsSeparatedByString:(id)separator {
    ONSRange found = [self rangeOfString:separator options:0 range:(ONSRange){0, [self length]}];
    id last;
    id array;
    if (found.length == 0) {
        last = [self copy];
        array = [(Class)arrayClassSymbol arrayWithObject:last];
    } else {
        ONSUInteger length = [self length];
        array = [(Class)mutableArrayClassSymbol array];
        struct _NSZone *zone = [self zone];
        ONSUInteger start = 0;
        ONSUInteger remaining;
        do {
            id piece = [self _newSubstringWithRange:(ONSRange){start, found.location - start} zone:zone];
            [array addObject:piece];
            [piece release];
            start = found.location + found.length;
            remaining = length - start;
            found = [self rangeOfString:separator options:0 range:(ONSRange){start, remaining}];
        } while (found.length != 0);
        last = [self _newSubstringWithRange:(ONSRange){start, remaining} zone:zone];
        [array addObject:last];
    }
    [last release];
    return array;
}

// Native -[NSString hasPrefix:], 0x1c05d3a2c. Non-external. -rangeOfString:options:NSAnchoredSearch (8)
// range:{0, -length} found (location not NSNotFound).
- (BOOL)hasPrefix:(id)string {
    return [self rangeOfString:string options:8 range:(ONSRange){0, [self length]}].location != kNotFound;
}

// Native -[NSString hasSuffix:], 0x1c05d3a74. Non-external. As -hasPrefix: with NSAnchoredSearch |
// NSBackwardsSearch (0xC).
- (BOOL)hasSuffix:(id)string {
    return [self rangeOfString:string options:0xC range:(ONSRange){0, [self length]}].location != kNotFound;
}

// Native -[NSString containsString:], 0x1c05d3b84. Non-external. -rangeOfString:options:0 range:{0, -length}
// locale:nil found.
- (BOOL)containsString:(id)string {
    return [self rangeOfString:string options:0 range:(ONSRange){0, [self length]} locale:nil].location !=
           kNotFound;
}

// Native -[NSString stringByTrimmingCharactersInSet:], 0x1c05d3cec. Non-external. A nil set throws
// NSInvalidArgumentException. The stack character buffer reads forward while -characterIsMember: answers YES
// (an empty string starts with the buffer count 0 and index 1, xmmword_1C13CD8B0, and the current character
// 0xFFFF). Reaching the end answers the constant @""; a start at the last character answers
// -substringWithRange:{start, 1}. Otherwise the buffer is positioned on the last character (kept when it is
// already buffered; else location 0 for indexes up to 15, length - 32 or 0 when the index lies in the last 16
// characters, and index - 16 otherwise) and reads backward, refilling up to 32 characters before the current
// location (0xFFFF before the start), while the position stays at or after the start and the character is a
// member; the answer is -substringWithRange: from the start through the last non-member.
- (id)stringByTrimmingCharactersInSet:(id)set {
    if (set == nil) {
        throwNilArgument(self, _cmd);
    }
    StringCharacterBuffer characters = {0};
    ONSUInteger length = [self length];
    characters.string = self;
    characters.length = length;
    characters.location = 0;
    ONSUInteger position;
    if (length == 0) {
        characters.bufferLength = 0;
        characters.index = 1;
        characters.current = 0xFFFF;
        position = 0;
    } else {
        characters.bufferLength = length < 0x20 ? length : 0x20;
        stringCharacterBufferFill(&characters, 0, characters.bufferLength);
        unsigned short character = characters.buffer[0];
        characters.index = 1;
        characters.current = character;
        position = characters.bufferLength;
        if (characters.bufferLength != 0) {
            for (;;) {
                if (![set characterIsMember:character]) {
                    position = characters.index - 1;
                    break;
                }
                if (characters.index < characters.bufferLength) {
                    character = characters.buffer[characters.index];
                    characters.index++;
                } else {
                    ONSUInteger next = characters.location + characters.bufferLength;
                    if (characters.length > next) {
                        ONSUInteger remaining = characters.length - next;
                        ONSUInteger count = remaining < 0x20 ? remaining : 0x20;
                        characters.location = next;
                        characters.bufferLength = count;
                        stringCharacterBufferFill(&characters, next, count);
                        character = characters.buffer[0];
                        characters.index = 1;
                    } else {
                        characters.index = characters.bufferLength + 1;
                        character = 0xFFFF;
                    }
                }
                characters.current = character;
                if (characters.index - 1 >= characters.bufferLength) {
                    position = characters.index - 1;
                    break;
                }
            }
        }
    }
    ONSUInteger start = position + characters.location;
    if (start == length) {
        return (id)OF_STR("");
    }
    ONSUInteger last = length - 1;
    if (start >= last) {
        return [self substringWithRange:(ONSRange){start, 1}];
    }
    if (last < characters.location || last >= characters.bufferLength + characters.location) {
        ONSUInteger location;
        if (last <= 0xF) {
            location = 0;
        } else if (last > characters.length - 0x10) {
            location = characters.length >= 0x20 ? characters.length - 0x20 : 0;
        } else {
            location = length - 0x11;
        }
        characters.location = location;
        ONSUInteger remaining = characters.length - location;
        ONSUInteger count = remaining < 0x20 ? remaining : 0x20;
        characters.bufferLength = count;
        stringCharacterBufferFill(&characters, location, count);
        characters.current = characters.buffer[0];
    }
    ONSUInteger offset = last - characters.location;
    characters.index = offset + 1;
    unsigned short character = characters.buffer[offset];
    characters.current = character;
    if ([set characterIsMember:character]) {
        do {
            if (characters.index - 1 + characters.location < start) {
                break;
            }
            if (characters.index >= 2) {
                characters.index--;
                character = characters.buffer[characters.index - 1];
            } else if (characters.location != 0) {
                ONSUInteger count = characters.location < 0x20 ? characters.location : 0x20;
                ONSUInteger location = characters.location - count;
                characters.bufferLength = count;
                characters.index = count;
                characters.location = location;
                stringCharacterBufferFill(&characters, location, count);
                character = characters.buffer[characters.index - 1];
            } else {
                characters.index = 0;
                character = 0xFFFF;
            }
            characters.current = character;
        } while ([set characterIsMember:character]);
    }
    return [self substringWithRange:(ONSRange){start, characters.location - start + characters.index}];
}

// Native -[NSString stringByReplacingOccurrencesOfString:withString:options:range:], 0x1c05d4208.
// Non-external. Only a nil replacement throws NSInvalidArgumentException. NSRegularExpressionSearch (0x400)
// tail-calls
// -_stringByReplacingOccurrencesOfRegularExpressionPattern:withTemplate:options:range:. Otherwise a
// -mutableCopy gets -replaceOccurrencesOfString:withString:options:range:; with replacements it is answered,
// without them it is released and self is retained; autoreleased either way.
- (id)stringByReplacingOccurrencesOfString:(id)target
                                withString:(id)replacement
                                   options:(ONSUInteger)options
                                     range:(ONSRange)searchRange {
    if (replacement == nil) {
        throwNilArgument(self, _cmd);
    }
    if ((options & 0x400) != 0) {
        return [self _stringByReplacingOccurrencesOfRegularExpressionPattern:target
                                                                withTemplate:replacement
                                                                     options:options
                                                                       range:searchRange];
    }
    id copy = [self mutableCopy];
    id result = copy;
    if ([copy replaceOccurrencesOfString:target withString:replacement options:options
                                   range:searchRange] == 0) {
        [copy release];
        result = [self retain];
    }
    return [result autorelease];
}

// Native -[NSString getCharacters:range:], 0x1c05d43f4. Non-external. One -characterAtIndex: per character,
// in order; an empty range sends nothing.
- (void)getCharacters:(unsigned short *)buffer range:(ONSRange)range {
    if (range.length == 0) {
        return;
    }
    ONSUInteger index = range.location;
    ONSUInteger remaining = range.length;
    do {
        unsigned short character = [self characterAtIndex:index];
        index++;
        *buffer++ = character;
        remaining--;
    } while (remaining != 0);
}

// Native -[NSString getCharacters:], 0x1c05d4444. Non-external.
- (void)getCharacters:(unsigned short *)buffer {
    ONSRange range = {0, [self length]};
    [self getCharacters:buffer range:range];
}

// Native -[NSString _newSubstringWithRange:zone:], 0x1c05d4478. Non-external. -length is read first. An empty
// range answers the constant @"" (not retained); the whole string answers -copyWithZone:zone. A
// -_fastCStringContents:NO pointer gives CFStringCreateWithBytes (kCFAllocatorDefault, the bytes from the
// range's location, the eight-bit encoding, not an external representation). Otherwise length * 2 bytes from
// malloc_type_zone_malloc (the zone, or malloc_default_zone() for NULL; type 0x409734D7) receive
// -getCharacters:range: and are adopted by +[NSString allocWithZone:zone]
// -initWithCharactersNoCopy:length:freeWhenDone:YES. The result is retained (+1).
- (id)_newSubstringWithRange:(ONSRange)range zone:(struct _NSZone *)zone {
    ONSUInteger length = [self length];
    if (range.length == 0) {
        return (id)OF_STR("");
    }
    if (range.location == 0 && range.length == length) {
        return [self copyWithZone:zone];
    }
    const char *contents = [self _fastCStringContents:NO];
    if (contents != NULL) {
        OCFAllocatorRef allocator = kOCFAllocatorDefault;
        OCFStringEncoding encoding = stringEightBitEncoding();
        return (id)OCFStringCreateWithBytes(
            allocator, (const UInt8 *)contents + range.location, (OCFIndex)range.length, encoding, false
        );
    }
    malloc_zone_t *mallocZone = (malloc_zone_t *)zone;
    if (mallocZone == NULL) {
        mallocZone = malloc_default_zone();
    }
    unsigned short *characters = malloc_type_zone_malloc(mallocZone, range.length * 2, 0x409734D7);
    [self getCharacters:characters range:range];
    return [[ONSString allocWithZone:zone] initWithCharactersNoCopy:characters
                                                             length:range.length
                                                       freeWhenDone:YES];
}

// Native -[NSString substringWithRange:], 0x1c05d45b0. Non-external. An out-of-bounds range throws
// NSRangeException; otherwise -_newSubstringWithRange:zone: (the receiver's -zone), autoreleased.
- (id)substringWithRange:(ONSRange)range {
    ONSUInteger length = [self length];
    if (length < range.length || range.location > length - range.length) {
        throwRangeOutOfBounds(self, _cmd, range, length);
    }
    return [[self _newSubstringWithRange:range zone:[self zone]] autorelease];
}

// Native -[NSString substringFromIndex:], 0x1c05d4674. Non-external. An index past -length throws
// NSRangeException ("%@: Index %lu out of bounds; string length %lu"); otherwise -substringWithRange:{from,
// length - from}.
- (id)substringFromIndex:(ONSUInteger)from {
    ONSUInteger length = [self length];
    if (length < from) {
        throwIndexOutOfBounds(self, _cmd, from, length);
    }
    return [self substringWithRange:(ONSRange){from, length - from}];
}

// Native -[NSString substringToIndex:], 0x1c05d4718. Non-external. An index past -length throws
// NSRangeException; otherwise -substringWithRange:{0, to}.
- (id)substringToIndex:(ONSUInteger)to {
    ONSUInteger length = [self length];
    if (to > length) {
        throwIndexOutOfBounds(self, _cmd, to, length);
    }
    return [self substringWithRange:(ONSRange){0, to}];
}

// Native -[NSString stringByAppendingString:], 0x1c05d47c0. Non-external. A nil string throws
// NSInvalidArgumentException. An empty receiver answers the argument's -copyWithZone:NULL and an empty
// argument the receiver's; otherwise __NSNewStringByAppendingStrings(-zone, self, string, nil). Autoreleased.
- (id)stringByAppendingString:(id)string {
    if (string == nil) {
        throwNilArgument(self, _cmd);
    }
    id result;
    if ([self length] == 0) {
        result = [string copyWithZone:NULL];
    } else if ([string length] == 0) {
        result = [self copyWithZone:NULL];
    } else {
        result = _ONSNewStringByAppendingStrings([self zone], self, string, nil);
    }
    return [result autorelease];
}

// ---- Part B (inside @implementation ONSString, after -stringByAppendingString:) ----

// Native -[NSString doubleValue], 0x1c05d4908. Non-external. Scans with _NSScanDoubleFromString from location
// 0, skipping +[NSCharacterSet whitespaceCharacterSet], with no locale; 0.0 when the scan fails.
- (double)doubleValue {
    ONSUInteger location = 0;
    double value = 0.0;
    BOOL scanned = _NSScanDoubleFromString(
        self, [(id)characterSetClassSymbol whitespaceCharacterSet], &value, &location, nil
    );
    return scanned ? value : 0.0;
}

// Native -[NSString floatValue], 0x1c05d4990. Non-external. As -doubleValue; the double is converted to float
// (FCVT) before the failure select.
- (float)floatValue {
    ONSUInteger location = 0;
    double value = 0.0;
    BOOL scanned = _NSScanDoubleFromString(
        self, [(id)characterSetClassSymbol whitespaceCharacterSet], &value, &location, nil
    );
    float result = (float)value;
    return scanned ? result : 0.0f;
}

// Native -[NSString intValue], 0x1c05d4a1c. Non-external. _NSScanIntFromString skipping whitespace; 0 when
// the scan fails.
- (int)intValue {
    int value = 0;
    ONSUInteger location = 0;
    BOOL scanned =
        _NSScanIntFromString(self, [(id)characterSetClassSymbol whitespaceCharacterSet], &value, &location);
    return scanned ? value : 0;
}

// Native -[NSString longLongValue], 0x1c05d4aa0. Non-external. _NSScanLongLongFromString skipping whitespace;
// 0 when the scan fails.
- (long long)longLongValue {
    ONSUInteger location = 0;
    long long value = 0;
    BOOL scanned = _NSScanLongLongFromString(
        self, [(id)characterSetClassSymbol whitespaceCharacterSet], &value, &location
    );
    return scanned ? value : 0;
}

// Native -[NSString integerValue], 0x1c05d4b20. Non-external. A tail call of -longLongValue.
- (ONSInteger)integerValue {
    return (ONSInteger)[self longLongValue];
}

// Native -[NSString boolValue], 0x1c05d4b24. Non-external. Skips leading whitespace (+[NSCharacterSet
// whitespaceCharacterSet], tested with -characterIsMember: on each character, 0xFFFF past the end) through
// the inlined 32-character reader; then 'T', 'Y', 't' or 'y' answer YES; '+' or '-' is skipped; then any
// run of '0' is skipped; the result is whether the next character is '1'-'9'. The reader starts with the
// first block (or 0xFFFF for an empty string) and the index 1.
- (BOOL)boolValue {
    id whitespace = [(id)characterSetClassSymbol whitespaceCharacterSet];
    ONSUInteger length = [self length];
    StringCharacterBuffer reader;
    memset(&reader, 0, sizeof(reader));
    reader.string = self;
    reader.length = length;
    unsigned short character;
    if (length != 0) {
        ONSUInteger count = length < 32 ? length : 32;
        reader.bufferLength = count;
        stringCharacterBufferFill(&reader, 0, count);
        character = reader.buffer[0];
    } else {
        character = 0xFFFF;
    }
    reader.index = 1;
    reader.current = character;
    while ([whitespace characterIsMember:reader.current]) {
        reader.current = readerNext(&reader);
    }
    character = reader.current;
    if (character <= 'X') {
        if (character == '+' || character == '-') {
            character = readerNext(&reader);
            reader.current = character;
        } else if (character == 'T') {
            return YES;
        }
    } else if (character == 'Y' || character == 't' || character == 'y') {
        return YES;
    }
    while (character == '0') {
        character = readerNext(&reader);
        reader.current = character;
    }
    return (unsigned int)(character - '1') < 9;
}

// Native -[NSString uppercaseString], 0x1c05d4e44. Non-external. -uppercaseStringWithLocale: nil.
- (id)uppercaseString {
    return [self uppercaseStringWithLocale:nil];
}

// Native -[NSString lowercaseString], 0x1c05d4e4c. Non-external. -lowercaseStringWithLocale: nil.
- (id)lowercaseString {
    return [self lowercaseStringWithLocale:nil];
}

// Native -[NSString capitalizedString], 0x1c05d4e54. Non-external. -capitalizedStringWithLocale: nil.
- (id)capitalizedString {
    return [self capitalizedStringWithLocale:nil];
}

// Native -[NSString uppercaseStringWithLocale:], 0x1c05d4eec. Non-external. A mutable copy made with
// CFStringCreateMutable(kCFAllocatorDefault, 0) and CFStringReplaceAll, uppercased with the locale;
// autoreleased. The allocator is the shared-cache GOT slot 0x222280B50, read as kCFAllocatorDefault: the
// slots 0x222280B50-0x222280B70 follow the alphabetical order of the kCFAllocator imports (Default, Malloc,
// MallocZone, Null, SystemDefault), matching their uses across the unit (confidence: medium).
- (id)uppercaseStringWithLocale:(id)locale {
    OCFMutableStringRef string = OCFStringCreateMutable(kOCFAllocatorDefault, 0);
    OCFStringReplaceAll(string, (OCFStringRef)self);
    OCFStringUppercase(string, (OCFLocaleRef)locale);
    return [(id)string autorelease];
}

// Native -[NSString lowercaseStringWithLocale:], 0x1c05d4f44. Non-external. As -uppercaseStringWithLocale:
// with CFStringLowercase.
- (id)lowercaseStringWithLocale:(id)locale {
    OCFMutableStringRef string = OCFStringCreateMutable(kOCFAllocatorDefault, 0);
    OCFStringReplaceAll(string, (OCFStringRef)self);
    OCFStringLowercase(string, (OCFLocaleRef)locale);
    return [(id)string autorelease];
}

// Native -[NSString capitalizedStringWithLocale:], 0x1c05d4f9c. Non-external. As -uppercaseStringWithLocale:
// with CFStringCapitalize.
- (id)capitalizedStringWithLocale:(id)locale {
    OCFMutableStringRef string = OCFStringCreateMutable(kOCFAllocatorDefault, 0);
    OCFStringReplaceAll(string, (OCFStringRef)self);
    OCFStringCapitalize(string, (OCFLocaleRef)locale);
    return [(id)string autorelease];
}

// Native -[NSString _getBlockStart:end:contentsEnd:forRange:stopAtLineSeparators:], 0x1c05d5204.
// Non-external. An out-of-bounds range throws NSRangeException when _CFExecutableLinkedOnOrAfter(6);
// otherwise it logs once per process ("Invalid range"; the once flag is the function-local warnonce byte in
// __bss) and continues. A zero location with no contents-end pointer and the whole string's length answers
// start 0 and end length. Otherwise the start (when requested and the location is not 0) is found by reading
// backwards from the character before the location (a CR before an LF at the location is stepped over) to
// the character after the nearest separator, or 0. The end and contents end (when either is requested) are
// found by reading forwards from the range's last character (its location when empty): an LF there ends the
// block (contents end before a preceding CR); otherwise the first separator gives the contents end, and the
// terminator is two characters for CR LF; reaching the string's end gives both the length. The separator
// test is isBlockSeparator (lines with stopAtLineSeparators, paragraphs otherwise). The reader record is
// zeroed once, and its count and index start at {0, 1} when the forward read starts past the end
// (xmmword_1C13CD8B0).
- (void)_getBlockStart:(ONSUInteger *)startPtr
                     end:(ONSUInteger *)endPtr
             contentsEnd:(ONSUInteger *)contentsEndPtr
                forRange:(ONSRange)range
    stopAtLineSeparators:(BOOL)stopAtLineSeparators {
    static bool warnonce;
    ONSUInteger stringLength = [self length];
    StringCharacterBuffer reader;
    memset(&reader, 0, sizeof(reader));
    if (stringLength < range.length || range.location > stringLength - range.length) {
        if (_OCFExecutableLinkedOnOrAfter(6)) {
            id reason = [ONSString
                stringWithFormat:(id)OF_STR("%@: Range {%lu, %lu} out of bounds; string length %lu"),
                                 _ONSMethodExceptionProem(self, _cmd),
                                 range.location,
                                 range.length,
                                 stringLength];
            objc_exception_throw([(id)exceptionClassSymbol exceptionWithName:(id)ONSRangeException
                                                                      reason:reason
                                                                    userInfo:nil]);
        }
        if (!warnonce) {
            warnonce = true;
            id proem = _ONSMethodExceptionProem(self, _cmd);
            ONSLog(
                (id)OF_STR(
                    "*** %@: Invalid range %@; this will become an exception for apps linked on "
                    "SnowLeopard. Warning shown once per app execution."
                ),
                proem,
                ONSStringFromRange(range)
            );
        }
    }
    if (range.location == 0) {
        if (contentsEndPtr == NULL && stringLength == range.length) {
            if (startPtr != NULL) {
                *startPtr = 0;
            }
            if (endPtr != NULL) {
                *endPtr = range.length;
            }
            return;
        }
        if (startPtr != NULL) {
            *startPtr = 0;
        }
    } else if (startPtr != NULL) {
        reader.string = self;
        reader.length = stringLength;
        reader.location = range.location;
        bool followedByNewline;
        ONSUInteger position;
        if (stringLength > range.location) {
            ONSUInteger remaining = stringLength - range.location;
            ONSUInteger count = remaining < 32 ? remaining : 32;
            reader.bufferLength = count;
            stringCharacterBufferFill(&reader, range.location, count);
            reader.current = reader.buffer[0];
            position = reader.location;
            followedByNewline = reader.current == '\n';
        } else {
            followedByNewline = false;
            reader.current = 0xFFFF;
            position = range.location;
        }
        ONSUInteger count = position < 32 ? position : 32;
        reader.bufferLength = count;
        reader.index = count;
        reader.location = position - count;
        stringCharacterBufferFill(&reader, position - count, count);
        unsigned short character = reader.buffer[reader.index - 1];
        reader.current = character;
        if (followedByNewline && character == '\r') {
            character = readerPrevious(&reader);
            reader.current = character;
        }
        ONSUInteger start;
        for (;;) {
            if (isBlockSeparator(character, stopAtLineSeparators)) {
                start = reader.location + reader.index;
                break;
            }
            if ((OCFIndex)(reader.location + reader.index - 1) < 1) {
                start = 0;
                break;
            }
            character = readerPrevious(&reader);
            reader.current = character;
        }
        *startPtr = start;
    }
    if (endPtr == NULL && contentsEndPtr == NULL) {
        return;
    }
    ONSUInteger position = range.location + range.length - (range.length != 0);
    reader.length = stringLength;
    reader.location = position;
    reader.string = self;
    ONSUInteger contentsEnd;
    ONSUInteger terminatorLength;
    unsigned short character;
    if (stringLength > position) {
        ONSUInteger remaining = stringLength - position;
        ONSUInteger count = remaining < 32 ? remaining : 32;
        reader.bufferLength = count;
        stringCharacterBufferFill(&reader, position, count);
        character = reader.buffer[0];
        reader.current = character;
        reader.index = 1;
        if (character == '\n') {
            ONSUInteger location = reader.location;
            if (location == 0) {
                contentsEnd = 0;
                terminatorLength = 1;
            } else {
                ONSUInteger previousCount = location < 32 ? location : 32;
                reader.bufferLength = previousCount;
                reader.index = previousCount;
                reader.location = location - previousCount;
                stringCharacterBufferFill(&reader, location - previousCount, previousCount);
                bool precededByReturn = reader.buffer[reader.index - 1] == '\r';
                terminatorLength = precededByReturn ? 2 : 1;
                contentsEnd = location - precededByReturn;
            }
            if (contentsEndPtr != NULL) {
                *contentsEndPtr = contentsEnd;
            }
            if (endPtr != NULL) {
                *endPtr = terminatorLength + contentsEnd;
            }
            return;
        }
    } else {
        character = 0xFFFF;
        reader.current = character;
        reader.bufferLength = 0;
        reader.index = 1;
    }
    for (;;) {
        if (isBlockSeparator(character, stopAtLineSeparators)) {
            contentsEnd = reader.index + reader.location - 1;
            unsigned short next = readerNext(&reader);
            reader.current = next;
            terminatorLength = (character == '\r' && next == '\n') ? 2 : 1;
            break;
        }
        if (reader.index + reader.location - 1 == stringLength) {
            contentsEnd = stringLength;
            terminatorLength = 0;
            break;
        }
        character = readerNext(&reader);
        reader.current = character;
    }
    if (contentsEndPtr != NULL) {
        *contentsEndPtr = contentsEnd;
    }
    if (endPtr != NULL) {
        *endPtr = terminatorLength + contentsEnd;
    }
}

// Native -[NSString enumerateSubstringsInRange:options:usingBlock:], 0x1c05d5a48. Non-external, with its
// block functions ___58-[NSString enumerateSubstringsInRange:options:usingBlock:]_block_invoke (0x1c05d6198),
// _block_invoke_2 (0x1c05d61e0), _block_invoke_3 (0x1c05d6220) and _block_invoke_4 (0x1c05d6410).
// The enumeration type is the low byte of the options: 0 lines and 1 paragraphs (-_getBlockStart:...,
// stopping at line separators for 0), 2 composed characters, 5 caret positions and 6 deletion clusters
// (CFStringGetRangeOfCharacterClusterAtIndex with cluster type 2, 3 and 4), 3 words and 4 sentences (a
// CFStringTokenizer with kCFStringTokenizerUnitWordBoundary (4) or kCFStringTokenizerUnitSentence (1), the
// system locale unless 0x400 (localized) passes NULL); any type above 6 calls the block once with nil and
// {NSNotFound, 0}. 0x100 reverses, 0x200 passes nil substrings, and without 0x800 the ranges are clamped to
// the given range (with 0x800 the tokenizer covers the whole string and starts at the token at the location).
// Reverse word and sentence enumeration collects the ranges of windows of at most 100000 characters
// (100000 or the first collected range's length plus 100000 for the next window, ending at that range's end)
// by a forward enumeration in the block of _block_invoke_3 (the stack block descriptor 0x2227D7680), which
// collects into malloc'd arrays (type 0x1000040451B5BE8; capacity ceil(length / 4.0) for words, / 64.0 for
// sentences, doubled with __CFSafelyReallocateTyped) through the block of _block_invoke_4 (descriptor
// 0x2227D7658); the ranges are consumed backwards, the first range of each window being used only for the
// last window. Forward enumeration notices a length change after each block call: the ranges move by the
// difference and the tokenizer is reset to the rest of the string. An autorelease pool is pushed around the
// loop and recycled every 1001 iterations. The word and sentence advance blocks are the global literals
// ___block_literal_global_50 and ___block_literal_global.46 (0x2227d7630).
- (void)enumerateSubstringsInRange:(ONSRange)range
                           options:(ONSUInteger)options
                        usingBlock:
                            (void (^)(
                                id substring, ONSRange substringRange, ONSRange enclosingRange, BOOL *stop
                            ))block {
    ONSUInteger location = range.location;
    ONSUInteger length = range.length;
    BOOL clampsToRange = (options & 0x800) == 0;
    ONSUInteger end = location + length;
    ONSUInteger index = (options & 0x100) != 0 ? end - 1 : location;
    BOOL stop = NO;
    ONSUInteger stringLength = [self length];
    if (stringLength < length || location > stringLength - length) {
        id reason =
            [ONSString stringWithFormat:(id)OF_STR("%@: Range {%lu, %lu} out of bounds; string length %lu"),
                                        _ONSMethodExceptionProem(self, _cmd),
                                        location,
                                        length,
                                        stringLength];
        objc_exception_throw([(id)exceptionClassSymbol exceptionWithName:(id)ONSRangeException
                                                                  reason:reason
                                                                userInfo:nil]);
    }
    if (length == 0) {
        return;
    }
    ONSRange *substringRanges = NULL;
    ONSRange *enclosingRanges = NULL;
    ONSUInteger type = options & 0xFF;
    OCFStringTokenizerTokenType (^advance)(OCFStringTokenizerRef) = nil;
    ONSUInteger (^collectRanges)(ONSRange **, ONSRange **, ONSRange) = nil;
    OCFStringTokenizerRef tokenizer = NULL;
    OCFIndex tokenLocation = 0;
    OCFIndex tokenLength = 0;
    ONSUInteger pending = 0;
    ONSUInteger windowLocation = 0;
    if (type - 3 <= 1) {
        if ((options & 0x100) != 0) {
            ONSUInteger windowStart = length > 100000 ? end - 100000 : location;
            ONSUInteger windowLength = length > 100000 ? 100000 : length;
            // _block_invoke_3, 0x1c05d6220: collects the substring and enclosing ranges of one window by a
            // forward enumeration with nil substrings (0x200 set, 0x100 cleared); when the outer enumeration
            // is not clamped (0x800) and the window touches neither end of the range, the inner one is
            // clamped (0x800 cleared). Answers the number collected.
            // Native block invoke 0x1c05d6220, non-external.
            collectRanges = ^ONSUInteger(ONSRange **substrings, ONSRange **enclosings, ONSRange window) {
              __block ONSUInteger capacity =
                  (ONSUInteger)ceil((double)window.length / (type == 3 ? 4.0 : 64.0));
              __block ONSUInteger count = 0;
              if (*substrings != NULL) {
                  free(*substrings);
              }
              if (*enclosings != NULL) {
                  free(*enclosings);
              }
              *substrings = malloc_type_malloc(16 * capacity, 0x1000040451B5BE8);
              *enclosings = malloc_type_malloc(16 * capacity, 0x1000040451B5BE8);
              ONSUInteger innerOptions = (options & ~(ONSUInteger)0x300) | 0x200;
              if (!clampsToRange && window.location != location && window.location + window.length != end) {
                  innerOptions &= ~(ONSUInteger)0x900;
              }
              // _block_invoke_4, 0x1c05d6410: appends both ranges, doubling both arrays when full.
              [self enumerateSubstringsInRange:window
                                       options:innerOptions
                                    // Native nested block invoke 0x1c05d6410, non-external.
                                    usingBlock:^(
                                        __unused id substring,
                                        ONSRange substringRange,
                                        ONSRange enclosingRange,
                                        __unused BOOL *innerStop
                                    ) {
                                      if (count == capacity) {
                                          capacity = count * 2;
                                          *substrings = __OCFSafelyReallocateTyped(
                                              *substrings, 16 * capacity, 0x1000040451B5BE8, NULL
                                          );
                                          *enclosings = __OCFSafelyReallocateTyped(
                                              *enclosings, 16 * capacity, 0x1000040451B5BE8, NULL
                                          );
                                      }
                                      (*substrings)[count] = substringRange;
                                      (*enclosings)[count] = enclosingRange;
                                      count++;
                                    }];
              return count;
            };
            windowLocation = windowStart;
            ONSUInteger collected =
                collectRanges(&substringRanges, &enclosingRanges, (ONSRange){windowStart, windowLength});
            if (collected != 0) {
                pending = collected - 1;
            } else {
                stop = YES;
            }
        } else {
            // _block_invoke (0x1c05d6198), the word advance: the next token, skipping tokens with
            // kCFStringTokenizerTokenHasNonLettersMask (0x10) and empty results while a token is current.
            // _block_invoke_2 (0x1c05d61e0), the sentence advance: the next token, skipping empty results
            // while a token is current. Native word/sentence block invokes 0x1c05d6198 and 0x1c05d61e0,
            // non-external.
            advance = type == 3 ? ^OCFStringTokenizerTokenType(OCFStringTokenizerRef wordTokenizer) {
                for (;;) {
                    OCFStringTokenizerTokenType tokenType = OCFStringTokenizerAdvanceToNextToken(wordTokenizer);
                    if (tokenType != 0) {
                        if ((tokenType & 0x10) == 0) {
                            return tokenType;
                        }
                        continue;
                    }
                    if (OCFStringTokenizerGetCurrentTokenRange(wordTokenizer).location == -1) {
                        return tokenType;
                    }
                }
            }
                                : ^OCFStringTokenizerTokenType(OCFStringTokenizerRef sentenceTokenizer) {
                                      OCFStringTokenizerTokenType tokenType;
                                      do {
                                          tokenType = OCFStringTokenizerAdvanceToNextToken(sentenceTokenizer);
                                      } while (tokenType == 0 &&
                                               OCFStringTokenizerGetCurrentTokenRange(sentenceTokenizer).location !=
                                                   -1);
                                      return tokenType;
                                  };
            OCFOptionFlags unit = type == 3 ? 4 : 1;
            OCFLocaleRef locale = (options & 0x400) != 0 ? NULL : OCFLocaleGetSystem();
            if ((options & 0x800) != 0) {
                tokenizer = OCFStringTokenizerCreate(
                    NULL, (OCFStringRef)self, (OCFRange){0, (OCFIndex)stringLength}, unit, locale
                );
                if (OCFStringTokenizerGoToTokenAtIndex(tokenizer, (OCFIndex)location) == 0) {
                    advance(tokenizer);
                }
            } else {
                tokenizer = OCFStringTokenizerCreate(
                    NULL, (OCFStringRef)self, (OCFRange){(OCFIndex)location, (OCFIndex)length}, unit, locale
                );
                advance(tokenizer);
            }
            OCFRange token = OCFStringTokenizerGetCurrentTokenRange(tokenizer);
            tokenLength = token.length;
            tokenLocation = token.location;
            if (token.location == -1) {
                stop = YES;
                tokenLocation = -1;
            }
        }
    }
    void *pool = _OCFAutoreleasePoolPush();
    OCFIndex clusterType = type == 2 ? 2 : (type == 5 ? 3 : 4);
    if (!stop) {
        ONSUInteger iterations = 0;
        // LSL W23, W8, W9: the shift count is taken modulo 32, as on the native register.
        uint32_t typeMask = (uint32_t)1 << (type & 31);
        do {
            ONSRange substringRange = {0, 0};
            ONSUInteger blockEnd = 0;
            ONSUInteger contentsEnd = 0;
            ONSUInteger enclosingLocation;
            ONSUInteger enclosingLength;
            id substring;
            if (type > 6) {
                enclosingLength = 0;
                substring = nil;
                enclosingLocation = 0x7FFFFFFFFFFFFFFF;
                substringRange = (ONSRange){0x7FFFFFFFFFFFFFFF, 0};
                goto callBlock;
            }
            if ((typeMask & 0x64) != 0) {
                OCFRange cluster =
                    OCFStringGetRangeOfCharacterClusterAtIndex((OCFStringRef)self, index, clusterType);
                enclosingLocation = cluster.location;
                enclosingLength = cluster.length;
                if ((options & 0x800) == 0) {
                    ONSUInteger cut = location < enclosingLocation ? 0 : location - enclosingLocation;
                    enclosingLocation = enclosingLocation > location ? enclosingLocation : location;
                    ONSUInteger clusterLength = enclosingLength - cut;
                    enclosingLength =
                        clusterLength + enclosingLocation > end ? end - enclosingLocation : clusterLength;
                }
                substringRange = (ONSRange){enclosingLocation, enclosingLength};
            } else if ((typeMask & 3) != 0) {
                [self _getBlockStart:&substringRange.location
                                     end:&blockEnd
                             contentsEnd:&contentsEnd
                                forRange:(ONSRange){index, 0}
                    stopAtLineSeparators:type == 0];
                if ((options & 0x800) == 0) {
                    if (substringRange.location < location) {
                        substringRange.location = location;
                        if (contentsEnd < location) {
                            contentsEnd = location;
                        }
                    }
                    if (blockEnd > end) {
                        blockEnd = end;
                        if (contentsEnd > end) {
                            contentsEnd = end;
                        }
                    }
                }
                substringRange.length = contentsEnd - substringRange.location;
                enclosingLocation = substringRange.location;
                enclosingLength = blockEnd - substringRange.location;
            } else if ((options & 0x100) == 0) {
                ONSUInteger tokenStart;
                ONSUInteger tokenSize;
                if ((options & 0x800) != 0) {
                    tokenStart = tokenLocation;
                    tokenSize = tokenLength;
                } else {
                    tokenStart =
                        (ONSUInteger)tokenLocation > location ? (ONSUInteger)tokenLocation : location;
                    tokenSize = (ONSUInteger)(tokenLocation + tokenLength) > end ? end - tokenStart
                                                                                 : (ONSUInteger)tokenLength;
                }
                substringRange = (ONSRange){tokenStart, tokenSize};
                enclosingLocation = tokenStart < index ? tokenStart : index;
                OCFStringTokenizerTokenType tokenType = advance(tokenizer);
                OCFRange next = OCFStringTokenizerGetCurrentTokenRange(tokenizer);
                tokenLength = next.length;
                ONSUInteger limit;
                if (tokenType != 0) {
                    limit = next.location;
                    if ((ONSUInteger)next.location > end) {
                        ONSUInteger substringEnd = substringRange.length + substringRange.location;
                        limit = substringEnd > end ? substringEnd : end;
                    }
                } else {
                    limit = end;
                    if ((options & 0x800) != 0) {
                        ONSUInteger substringEnd = substringRange.length + substringRange.location;
                        limit = substringEnd > end ? substringEnd : end;
                    }
                }
                enclosingLength = limit - enclosingLocation;
                tokenLocation = next.location;
            } else {
                if (pending == 0 && windowLocation != location) {
                    ONSRange first = enclosingRanges[0];
                    ONSUInteger span = first.length < 100000 ? 100000 : first.length + 100000;
                    ONSUInteger firstEnd = first.length + first.location;
                    ONSUInteger nextLocation;
                    ONSUInteger nextLength;
                    if (firstEnd > span && firstEnd - span > location) {
                        nextLocation = firstEnd - span;
                        nextLength = span;
                    } else {
                        nextLocation = location;
                        nextLength = firstEnd - location;
                    }
                    windowLocation = nextLocation;
                    ONSUInteger collected = collectRanges(
                        &substringRanges, &enclosingRanges, (ONSRange){nextLocation, nextLength}
                    );
                    if (collected == 0) {
                        enclosingLength = 0;
                        substring = nil;
                        pending = 0;
                        enclosingLocation = 0x7FFFFFFFFFFFFFFF;
                        substringRange = (ONSRange){0x7FFFFFFFFFFFFFFF, 0};
                        goto callBlock;
                    }
                    pending = collected - 1;
                    continue;
                }
                substringRange = substringRanges[pending];
                enclosingLocation = enclosingRanges[pending].location;
                enclosingLength = enclosingRanges[pending].length;
                pending--;
                if (enclosingLength + enclosingLocation > index) {
                    enclosingLength = index + 1 - enclosingLocation;
                    if (substringRange.length + substringRange.location > index) {
                        substringRange.length = index + 1 - substringRange.location;
                    }
                }
            }
            substring = (options & 0x200) != 0 ? nil : [self _newSubstringWithRange:substringRange zone:NULL];
        callBlock:
            block(substring, substringRange, (ONSRange){enclosingLocation, enclosingLength}, &stop);
            if (substring != nil) {
                [substring release];
            }
            if (stop) {
                break;
            }
            if ((options & 0x100) != 0) {
                if (enclosingLocation <= location) {
                    break;
                }
                index = enclosingLocation - 1;
            } else {
                ONSUInteger newLength = [self length];
                ONSUInteger difference = newLength - stringLength;
                if (difference != 0) {
                    enclosingLength += difference;
                    end += difference;
                    if (tokenizer != NULL) {
                        ONSUInteger limit = (options & 0x800) == 0 ? end : newLength;
                        ONSUInteger restart = enclosingLength + enclosingLocation;
                        OCFStringTokenizerSetString(
                            tokenizer,
                            (OCFStringRef)self,
                            (OCFRange){(OCFIndex)restart, (OCFIndex)(limit - restart)}
                        );
                        advance(tokenizer);
                        OCFRange next = OCFStringTokenizerGetCurrentTokenRange(tokenizer);
                        tokenLocation = next.location;
                        if (next.location == -1) {
                            break;
                        }
                        tokenLength = next.length;
                    }
                }
                index = enclosingLength + enclosingLocation;
                stringLength = newLength;
                if (index >= end) {
                    break;
                }
            }
            if (iterations == 1000) {
                _OCFAutoreleasePoolPop(pool);
                pool = _OCFAutoreleasePoolPush();
                iterations = 0;
            } else {
                iterations++;
            }
        } while (!stop);
    }
    _OCFAutoreleasePoolPop(pool);
    if (tokenizer != NULL) {
        OCFRelease(tokenizer);
    }
    if (substringRanges != NULL) {
        free(substringRanges);
    }
    if (enclosingRanges != NULL) {
        free(enclosingRanges);
    }
}

// Native __NSEncodingToNewDataForExtendedAttribute, 0x1c05d69d0. External. The com.apple.TextEncoding value
// of an encoding: the CFStringEncoding's IANA name (UTF-8 external representation through CFStringGetBytes
// with
// '?' as loss byte, into at most 70 bytes of a 100-byte buffer; nothing when the name is missing or does not
// convert) followed by ";%u" with the CFStringEncoding (snprintf_l, 99 bytes, C locale), as a new (+1)
// NSData; nil for an encoding CFStringConvertNSStringEncodingToEncoding rejects (kCFStringEncodingInvalidId).
id _ONSEncodingToNewDataForExtendedAttribute(ONSUInteger encoding) {
    OCFStringEncoding cfEncoding = OCFStringConvertNSStringEncodingToEncoding(encoding);
    if (cfEncoding == 0xFFFFFFFF) {
        return nil;
    }
    OCFIndex usedLength = 0;
    UInt8 buffer[100];
    OCFStringRef name = OCFStringConvertEncodingToIANACharSetName(cfEncoding);
    OCFIndex offset;
    if (name != NULL) {
        OCFIndex nameLength = OCFStringGetLength(name);
        if (OCFStringGetBytes(
                name, (OCFRange){0, nameLength}, 0x8000100, '?', true, buffer, 70, &usedLength
            ) != 0) {
            offset = usedLength;
        } else {
            offset = 0;
            usedLength = 0;
        }
    } else {
        offset = 0;
    }
    snprintf_l((char *)buffer + offset, 0x63, NULL, ";%u", cfEncoding);
    id data = [(id)dataClassSymbol alloc];
    return [data initWithBytes:buffer length:strlen((const char *)buffer)];
}

// Native -[NSString getExternalRepresentation:extendedAttributes:forWritingToURLOrPath:usingEncoding:error:],
// 0x1c05d6c4c. Non-external. With an attributes pointer, the encoding's extended-attribute data goes into an
// autoreleased dictionary under com.apple.TextEncoding (an unsupported encoding fails with
// NSFileWriteInapplicableStringEncodingError, 517). With a representation pointer, the encoded size is
// measured with -getBytes:maxLength:filledLength:... (NULL buffer, maximum length 0x7FFFFFFFFFFFFFFE, no
// loss) and the bytes go into +[NSMutableData dataWithLength:]; a failed measure is error 517, a failed fill
// NSFileWriteUnknownError (512) through _NSErrorWithFilePath.
- (BOOL)getExternalRepresentation:(id *)representation
               extendedAttributes:(id *)extendedAttributes
            forWritingToURLOrPath:(id)urlOrPath
                    usingEncoding:(ONSUInteger)encoding
                            error:(id *)error {
    if (extendedAttributes != NULL) {
        id attributeData = _ONSEncodingToNewDataForExtendedAttribute(encoding);
        if (attributeData == nil) {
            if (error != NULL) {
                *error = _ONSErrorWithFilePathAndEncoding(517, urlOrPath, encoding);
            }
            return NO;
        }
        *extendedAttributes = [[[(id)dictionaryClassSymbol alloc]
            initWithObjectsAndKeys:attributeData, (id)OF_STR("com.apple.TextEncoding"), nil] autorelease];
        [attributeData release];
    }
    if (representation == NULL) {
        return YES;
    }
    ONSUInteger filledLength = 0;
    ONSUInteger neededLength = 0;
    ONSUInteger length = [self length];
    if (![self getBytes:NULL
                       maxLength:0x7FFFFFFFFFFFFFFE
                    filledLength:&neededLength
                        encoding:encoding
            allowLossyConversion:NO
                           range:(ONSRange){0, length}
                  remainingRange:NULL]) {
        if (error != NULL) {
            *error = _ONSErrorWithFilePathAndEncoding(517, urlOrPath, encoding);
        }
        return NO;
    }
    id data = [(id)mutableDataClassSymbol dataWithLength:neededLength];
    if ([self getBytes:[data mutableBytes]
                       maxLength:neededLength
                    filledLength:&filledLength
                        encoding:encoding
            allowLossyConversion:NO
                           range:(ONSRange){0, length}
                  remainingRange:NULL]) {
        *representation = data;
        return YES;
    }
    if (error != NULL) {
        *error = _ONSErrorWithFilePath(512, urlOrPath);
    }
    return NO;
}

// Native writeStringToURLOrPath, 0x1c05d6df8. Non-external. The external representation and attributes
// (-getExternalRepresentation:...); a non-file NSURL is written with -[NSData writeToURL:options:error:]; a
// file URL without a path fails with NSFileNoSuchFileError (4); a path (or file URL's path) is written with
// +[NSData _writeDataToPath:data:options:...reportProgress:NO error:], passing the com.apple.TextEncoding
// attribute data when present. The options are the 32-bit argument (the callers' atomically flag).
static BOOL
writeStringToURLOrPath(id string, id urlOrPath, unsigned int options, ONSUInteger encoding, id *error) {
    id attributes = nil;
    id representation = nil;
    if (![string getExternalRepresentation:&representation
                        extendedAttributes:&attributes
                     forWritingToURLOrPath:urlOrPath
                             usingEncoding:encoding
                                     error:error]) {
        return NO;
    }
    id path;
    if ([urlOrPath isKindOfClass:[(id)urlClassSymbol class]]) {
        if (![urlOrPath isFileURL]) {
            return [representation writeToURL:urlOrPath options:options error:error];
        }
        path = [urlOrPath path];
        if (path == nil) {
            if (error != NULL) {
                *error = _ONSErrorWithFilePath(4, urlOrPath);
            }
            return NO;
        }
    } else {
        path = urlOrPath;
    }
    id attributeData = [attributes objectForKey:(id)OF_STR("com.apple.TextEncoding")];
    if (attributeData != nil) {
        return [(id)dataClassSymbol _writeDataToPath:path
                                                data:representation
                                             options:options
                         stringEncodingAttributeData:attributeData
                                      reportProgress:NO
                                               error:error];
    }
    return [(id)dataClassSymbol _writeDataToPath:path
                                            data:representation
                                         options:options
                                  reportProgress:NO
                                           error:error];
}

// Native -[NSString writeToFile:atomically:], 0x1c05d6f50. Non-external. -writeToURL:atomically: with
// +[NSURL fileURLWithPath:].
- (BOOL)writeToFile:(id)path atomically:(BOOL)useAuxiliaryFile {
    return [self writeToURL:[(id)urlClassSymbol fileURLWithPath:path] atomically:useAuxiliaryFile];
}

// Native -[NSString writeToURL:atomically:], 0x1c05d6f88. Non-external. Writes in the default C string
// encoding
// (_NSDefaultStringEncoding once __NSDefaultStringEncodingFullyInited is 1, otherwise
// _NSDefaultCStringEncoding); when that fails with NSFileWriteInapplicableStringEncodingError (517), retries
// in Unicode (10) without an error pointer.
- (BOOL)writeToURL:(id)url atomically:(BOOL)atomically {
    ONSUInteger encoding;
    if (__ONSDefaultStringEncodingFullyInited == 1) {
        encoding = _ONSDefaultStringEncoding;
    } else {
        encoding = _ONSDefaultCStringEncoding();
    }
    id error = nil;
    if (writeStringToURLOrPath(self, url, atomically, encoding, &error)) {
        return YES;
    }
    if ([error code] == 517) {
        return writeStringToURLOrPath(self, url, atomically, 10, NULL);
    }
    return NO;
}

// Native -[NSString initWithCString:length:], 0x1c05d7068. Non-external. -initWithBytes:length:encoding: in
// the default C string encoding (as -writeToURL:atomically:).
- (id)initWithCString:(const char *)bytes length:(ONSUInteger)length {
    ONSUInteger encoding;
    if (__ONSDefaultStringEncodingFullyInited == 1) {
        encoding = _ONSDefaultStringEncoding;
    } else {
        encoding = _ONSDefaultCStringEncoding();
    }
    return [self initWithBytes:bytes length:length encoding:encoding];
}

// Native -[NSString initWithCString:encoding:], 0x1c05d718c. Non-external. NULL throws
// NSInvalidArgumentException
// ("%@: NULL cString"); otherwise -initWithBytes:length:encoding: with strlen.
- (id)initWithCString:(const char *)nullTerminatedCString encoding:(ONSUInteger)encoding {
    if (nullTerminatedCString == NULL) {
        id reason =
            [ONSString stringWithFormat:(id)OF_STR("%@: NULL cString"), _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([(id)exceptionClassSymbol exceptionWithName:(id)ONSInvalidArgumentException
                                                                  reason:reason
                                                                userInfo:nil]);
    }
    return [self initWithBytes:nullTerminatedCString length:strlen(nullTerminatedCString) encoding:encoding];
}

// Native -[NSString initWithUTF8String:], 0x1c05d7224. Non-external. NULL throws NSInvalidArgumentException.
// The shared placeholder (__placeholder) or any receiver when _CFExecutableLinkedOnOrAfter(8) answers
// CFStringCreateWithCString(kCFAllocatorDefault, string, kCFStringEncodingUTF8) without consuming the
// receiver; otherwise -initWithBytes:length:encoding: with strlen and NSUTF8StringEncoding (4).
- (id)initWithUTF8String:(const char *)nullTerminatedCString {
    if (nullTerminatedCString == NULL) {
        id reason =
            [ONSString stringWithFormat:(id)OF_STR("%@: NULL cString"), _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([(id)exceptionClassSymbol exceptionWithName:(id)ONSInvalidArgumentException
                                                                  reason:reason
                                                                userInfo:nil]);
    }
    if ((const void *)self == (const void *)&__placeholder || _OCFExecutableLinkedOnOrAfter(8)) {
        return (id)OCFStringCreateWithCString(kOCFAllocatorDefault, nullTerminatedCString, 0x8000100);
    }
    return [self initWithBytes:nullTerminatedCString length:strlen(nullTerminatedCString) encoding:4];
}

// Native -[NSString initWithCharacters:length:], 0x1c05d72f4. Non-external. A non-empty copy goes into memory
// from the receiver's -zone (malloc_default_zone when NULL; type 0x409734D7) and is adopted with
// -initWithCharactersNoCopy:length:freeWhenDone:YES; an empty one passes NULL, 0 and NO.
- (id)initWithCharacters:(const unsigned short *)characters length:(ONSUInteger)length {
    if (length == 0) {
        return [self initWithCharactersNoCopy:NULL length:length freeWhenDone:NO];
    }
    malloc_zone_t *zone = (malloc_zone_t *)[self zone];
    if (zone == NULL) {
        zone = malloc_default_zone();
    }
    unsigned short *copy = malloc_type_zone_malloc(zone, length * 2, 0x409734D7);
    memmove(copy, characters, length * 2);
    return [self initWithCharactersNoCopy:copy length:length freeWhenDone:YES];
}

// Native -[NSString initWithString:], 0x1c05d7380. Non-external. The string's characters are copied into
// memory from the receiver's -zone (type 0x409734D7) and adopted with freeWhenDone:YES; an empty string
// passes NULL, 0 and NO; a failed allocation sends -dealloc to the receiver and answers nil.
- (id)initWithString:(id)string {
    ONSUInteger length = [string length];
    if (length == 0) {
        return [self initWithCharactersNoCopy:NULL length:0 freeWhenDone:NO];
    }
    malloc_zone_t *zone = (malloc_zone_t *)[self zone];
    if (zone == NULL) {
        zone = malloc_default_zone();
    }
    unsigned short *characters = malloc_type_zone_malloc(zone, length * 2, 0x409734D7);
    if (characters == NULL) {
        [self dealloc];
        return nil;
    }
    [string getCharacters:characters range:(ONSRange){0, length}];
    return [self initWithCharactersNoCopy:characters length:length freeWhenDone:YES];
}

// Native -[NSString initWithData:encoding:], 0x1c05d7438. Non-external. -initWithBytes:length:encoding: with
// the data's -bytes and -length.
- (id)initWithData:(id)data encoding:(ONSUInteger)encoding {
    const void *bytes = [data bytes];
    return [self initWithBytes:bytes length:[data length] encoding:encoding];
}

// Native -[NSString _initWithDataOfUnknownEncoding:], 0x1c05d7544. Non-external. nil data answers nil (after
// -length and -bytes are sent). An even length of at least 2 starting with the byte-order mark FF FE or FE FF
// is Unicode (10); otherwise a length above 2 starting with EF BB BF is UTF-8 (4); otherwise the default C
// string encoding. -initWithData:encoding:.
- (id)_initWithDataOfUnknownEncoding:(id)data {
    ONSUInteger length = [data length];
    const uint8_t *bytes = [data bytes];
    if (data == nil) {
        return nil;
    }
    ONSUInteger encoding;
    if (length < 2) {
        goto defaultEncoding;
    }
    if ((length & 1) == 0) {
        if (bytes[0] == 0xFF) {
            if (bytes[1] == 0xFE) {
                return [self initWithData:data encoding:10];
            }
        } else if (bytes[0] == 0xFE && bytes[1] == 0xFF) {
            return [self initWithData:data encoding:10];
        }
    }
    if (length != 2 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        return [self initWithData:data encoding:4];
    }
defaultEncoding:
    if (__ONSDefaultStringEncodingFullyInited == 1) {
        encoding = _ONSDefaultStringEncoding;
    } else {
        encoding = _ONSDefaultCStringEncoding();
    }
    return [self initWithData:data encoding:encoding];
}

// Native -[NSString initWithContentsOfFile:], 0x1c05d7640. Non-external. +[NSData allocWithZone:NULL]
// -initWithContentsOfMappedFile:, then -_initWithDataOfUnknownEncoding:; the data is released.
- (id)initWithContentsOfFile:(id)path {
    id data = [[(id)dataClassSymbol allocWithZone:NULL] initWithContentsOfMappedFile:path];
    id result = [self _initWithDataOfUnknownEncoding:data];
    [data release];
    return result;
}

// Native -[NSString initWithContentsOfURL:], 0x1c05d7690. Non-external. As -initWithContentsOfFile: with
// -[NSData initWithContentsOfURL:].
- (id)initWithContentsOfURL:(id)url {
    id data = [[(id)dataClassSymbol allocWithZone:NULL] initWithContentsOfURL:url];
    id result = [self _initWithDataOfUnknownEncoding:data];
    [data release];
    return result;
}

// Native -[NSString initWithContentsOfURL:encoding:error:], 0x1c05d76e0. Non-external. A file URL reads its
// -_securePath with -initWithContentsOfFile:encoding:error: (no path: NSFileReadInvalidFileNameError, 260).
// Any other URL is loaded with +[NSURLConnection sendSynchronousRequest:returningResponse:error:NULL] of an
// NSURLRequest (cache policy 1, timeout 60.0, released after the send); no data, or an NSHTTPURLResponse
// whose status code +[NSHTTPURLResponse isErrorStatusCode:] rejects, fails with NSFileReadUnknownError (256);
// data that does not decode fails with NSFileReadInapplicableStringEncodingError (261). Every failure
// releases the receiver (also after a nil -initWithBytes:length:encoding:) and answers nil.
- (id)initWithContentsOfURL:(id)url encoding:(ONSUInteger)encoding error:(id *)error {
    if ([url isFileURL]) {
        id path = [url _securePath];
        if (path != nil) {
            return [self initWithContentsOfFile:path encoding:encoding error:error];
        }
        if (error != NULL) {
            *error = _ONSErrorWithFilePath(260, url);
        }
        [self release];
        return nil;
    }
    id request = [[(id)urlRequestClassSymbol alloc] initWithURL:url cachePolicy:1 timeoutInterval:60.0];
    id response = nil;
    id data = [(id)urlConnectionClassSymbol sendSynchronousRequest:request
                                                 returningResponse:&response
                                                             error:NULL];
    [request release];
    BOOL failed;
    if (response != nil && data != nil) {
        failed = [response isKindOfClass:[(id)httpURLResponseClassSymbol self]] &&
                 [(id)httpURLResponseClassSymbol isErrorStatusCode:[response statusCode]];
    } else {
        failed = data == nil;
    }
    if (failed) {
        if (error != NULL) {
            *error = _ONSErrorWithFilePath(256, url);
        }
        [self release];
        return nil;
    }
    id result = [self initWithBytes:[data bytes] length:[data length] encoding:encoding];
    if (error != NULL && result == nil) {
        *error = _ONSErrorWithFilePathAndEncoding(261, url, encoding);
        [self release];
        return nil;
    }
    if (result == nil) {
        [self release];
        return nil;
    }
    return result;
}

// Native -[NSString initWithContentsOfFile:encoding:error:], 0x1c05d78b4. Non-external. An NSURL argument
// (_NSIsNSURL) goes to -initWithContentsOfURL:encoding:error:. The file is read with +[NSData
// _readBytesFromPath:maxLength:0x7FFFFFFFFFFFFFFF bytes:length:didMap:options:0 reportProgress:NO error:]; no
// bytes and no length answer -init; otherwise the bytes are adopted with
// -initWithBytesNoCopy:length:encoding:freeWhenDone:YES, and on failure freed with error 261. Every failure
// releases the receiver and answers nil.
- (id)initWithContentsOfFile:(id)path encoding:(ONSUInteger)encoding error:(id *)error {
    if (_ONSIsNSURL(path)) {
        return [self initWithContentsOfURL:path encoding:encoding error:error];
    }
    ONSUInteger length = 0;
    void *bytes = NULL;
    BOOL didMap = NO;
    if ([(id)dataClassSymbol _readBytesFromPath:path
                                      maxLength:0x7FFFFFFFFFFFFFFF
                                          bytes:&bytes
                                         length:&length
                                         didMap:&didMap
                                        options:0
                                 reportProgress:NO
                                          error:error]) {
        if (length == 0 && bytes == NULL) {
            id result = [self init];
            if (result != nil) {
                return result;
            }
        } else {
            id result = [self initWithBytesNoCopy:bytes length:length encoding:encoding freeWhenDone:YES];
            if (result != nil) {
                return result;
            }
            free(bytes);
            if (error != NULL) {
                *error = _ONSErrorWithFilePathAndEncoding(261, path, encoding);
            }
        }
    }
    [self release];
    return nil;
}

// Native -[NSString initWithFormat:], 0x1c05d7d54. Non-external. -initWithFormat:locale:nil arguments:.
- (id)initWithFormat:(id)format, ... {
    va_list arguments;
    va_start(arguments, format);
    id result = [self initWithFormat:format locale:nil arguments:arguments];
    va_end(arguments);
    return result;
}

// Native -[NSString initWithFormat:locale:], 0x1c05d7dac. Non-external. -initWithFormat:locale:arguments:.
- (id)initWithFormat:(id)format locale:(id)locale, ... {
    va_list arguments;
    va_start(arguments, locale);
    id result = [self initWithFormat:format locale:locale arguments:arguments];
    va_end(arguments);
    return result;
}

// Native -[NSString initWithFormat:arguments:], 0x1c05d7e00. Non-external. A tail call of
// -initWithFormat:locale:nil arguments:.
- (id)initWithFormat:(id)format arguments:(va_list)arguments {
    return [self initWithFormat:format locale:nil arguments:arguments];
}

// Native -[NSString classForCoder], 0x1c05d8058. Non-external. __NSStringClass.
- (Class)classForCoder {
    return __NSStringClass;
}

// Native -[NSString _allowsDirectEncoding], 0x1c05d8064. Non-external. Answers YES.
- (BOOL)_allowsDirectEncoding {
    return YES;
}

// Native +[NSString supportsSecureCoding], 0x1c05d806c. Non-external. Answers YES.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSString encodeWithCoder:], 0x1c05d8074. Non-external. A keyed coder whose class is exactly
// NSKeyedArchiver (object_getClass) gets -_encodePropertyList:self forKey:@"NS.string". Otherwise the UTF-8
// bytes (NSUTF8StringEncoding 4, no loss, at most six bytes per character, into a 100-byte stack buffer or
// malloc'd memory of type 0x100004077774924) go to -encodeBytes:length:forKey:@"NS.bytes" (keyed) or
// -encodeBytes:length: (unkeyed); a failed conversion throws NSInternalInconsistencyException ("couldn't
// encode string %@"; the exception-name slot 0x22227F4F8, confidence: medium). The exceptional heap-buffer
// leak is fixed below.
- (void)encodeWithCoder:(id)coder {
    BOOL keyed = [coder allowsKeyedCoding];
    if (keyed && object_getClass(coder) == (Class)keyedArchiverClassSymbol) {
        [coder _encodePropertyList:self forKey:(id)OF_STR("NS.string")];
        return;
    }
    uint8_t stackBuffer[100];
    ONSUInteger length = [self length];
    ONSUInteger size = length * 6;
    ONSUInteger filledLength = length;
    uint8_t *buffer = size < 0x65 ? stackBuffer : malloc_type_malloc(size, 0x100004077774924);
    if (![self getBytes:buffer
                       maxLength:size
                    filledLength:&filledLength
                        encoding:4
            allowLossyConversion:NO
                           range:(ONSRange){0, length}
                  remainingRange:NULL]) {
        id reason = [ONSString stringWithFormat:(id)OF_STR("couldn't encode string %@"), self];
        // Foundation defect: 0x1c05d828c throws past the heap-buffer free at 0x1c05d821c.
        // Release that buffer after formatting the reason and before creating the exception.
        // Confidence: verified; iOS Device divergence: unread.
        if (buffer != stackBuffer) {
            free(buffer);
        }
        objc_exception_throw([(id)exceptionClassSymbol exceptionWithName:(id)ONSInternalInconsistencyException
                                                                  reason:reason
                                                                userInfo:nil]);
    }
    if (keyed) {
        [coder encodeBytes:buffer length:filledLength forKey:(id)OF_STR("NS.bytes")];
    } else {
        [coder encodeBytes:buffer length:filledLength];
    }
    if (buffer != stackBuffer) {
        free(buffer);
    }
}

// Native -[NSString initWithCoder:], 0x1c05d8290. Non-external. Keyed: an NS.string property list that is a
// string (_NSIsNSString) goes to -initWithString:, any other value fails the coder with +[NSError
// _readCorruptErrorWithFormat:@"Decoded object is not a string"] and releases the receiver; without NS.string
// the NS.bytes are decoded as UTF-8. Unkeyed: class version 1 decodes -decodeBytesWithReturnedLength: as
// UTF-8; any other version releases the receiver and throws ("NSString cannot decode class version %lu") with
// the exception name of the shared-cache GOT slot 0x22227F4E0. That slot is unresolved: by the alphabetical
// order of Foundation's CoreFoundation imports it lies near NSDateFormatterPatternStringKey,
// NSDefaultRunLoopMode and NSGenericException; NSGenericException, the only exception name among them, is
// used (confidence: low; needs an IDA reading of the slot).
- (id)initWithCoder:(id)coder {
    ONSUInteger length;
    const void *bytes;
    if ([coder allowsKeyedCoding]) {
        if ([coder containsValueForKey:(id)OF_STR("NS.string")]) {
            id string = [coder _decodePropertyListForKey:(id)OF_STR("NS.string")];
            if (_ONSIsNSString(string)) {
                return [self initWithString:string];
            }
            [coder
                failWithError:[(id)errorClassSymbol
                                  _readCorruptErrorWithFormat:(id)OF_STR("Decoded object is not a string")]];
            [self release];
            return nil;
        }
        length = 0;
        bytes = [coder decodeBytesForKey:(id)OF_STR("NS.bytes") returnedLength:&length];
    } else {
        ONSInteger version = [coder versionForClassName:(id)OF_STR(OF_NAME_STRING(ONSString))];
        if (version != 1) {
            id reason =
                [ONSString stringWithFormat:(id)OF_STR("NSString cannot decode class version %lu"), version];
            [self release];
            objc_exception_throw([(id)exceptionClassSymbol exceptionWithName:(id)ONSGenericException
                                                                      reason:reason
                                                                    userInfo:nil]);
        }
        length = 0;
        bytes = [coder decodeBytesWithReturnedLength:&length];
    }
    return [self initWithBytes:bytes length:length encoding:4];
}

// Native +[NSString _subclassesMustBeExplicitlyMentionedWhenDecoded], 0x1c05d8428. Non-external. Answers YES.
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded {
    return YES;
}

// ---- end of part B ----

// The abstract methods below lie in the outlined region (0x1c0fe53e0-0x1c0fe54f4).

// Native -[NSString length], 0x1c0fe53e0. Non-external. NSRequestConcreteImplementation(self, _cmd,
// __NSStringClass); answers 0.
- (ONSUInteger)length {
    ONSRequestConcreteImplementation(self, _cmd, __NSStringClass);
    return 0;
}

// Native -[NSString characterAtIndex:], 0x1c0fe5400. Non-external. As -length; answers 0.
- (unsigned short)characterAtIndex:(ONSUInteger)index {
    (void)index;
    ONSRequestConcreteImplementation(self, _cmd, __NSStringClass);
    return 0;
}

// Native -[NSString initWithCharactersNoCopy:length:freeWhenDone:], 0x1c0fe5420. Non-external.
// _NSRequestConcreteObject(self, _cmd); answers self.
- (id)initWithCharactersNoCopy:(unsigned short *)characters
                        length:(ONSUInteger)length
                  freeWhenDone:(BOOL)freeBuffer {
    (void)characters;
    (void)length;
    (void)freeBuffer;
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSString initWithBytesNoCopy:length:encoding:freeWhenDone:], 0x1c0fe548c. Non-external. As
// -initWithCharactersNoCopy:length:freeWhenDone:.
- (id)initWithBytesNoCopy:(void *)bytes
                   length:(ONSUInteger)length
                 encoding:(ONSUInteger)encoding
             freeWhenDone:(BOOL)freeBuffer {
    (void)bytes;
    (void)length;
    (void)encoding;
    (void)freeBuffer;
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSString initWithFormat:locale:arguments:], 0x1c0fe54d4. Non-external. As
// -initWithCharactersNoCopy:length:freeWhenDone:.
- (id)initWithFormat:(id)format locale:(id)locale arguments:(va_list)arguments {
    (void)format;
    (void)locale;
    (void)arguments;
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

@end
