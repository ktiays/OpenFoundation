//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSMutableString.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSAllocateObject.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSStringOtherEncodings.h"
#import "Foundation/String/ONSStringRegularExpressionSupport.h"

#include "CoreFoundation/Collections/OCFArray.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"

#include <malloc/malloc.h>
#include <objc/objc-exception.h>
#include <stdarg.h>
#include <stdint.h>

// Inferred organization: the NSMutableString methods lie together in the iOS Simulator __text section
// (0x1c0570720-0x1c0570bbb), followed by the NSPlaceholderMutableString methods (0x1c057104c-0x1c0571793;
// String/ONSPlaceholderMutableString.m) and __NSNewStringByAppendingStrings (0x1c0571a50); the two abstract
// methods -replaceCharactersInRange:withString: and -initWithCapacity: lie in the outlined region
// (0x1c0fe39b0-0x1c0fe39df). Following the owner's decision, the class has its own translation unit, which
// keeps the native order; __NSNewStringByAppendingStrings, the native neighbour that follows the placeholder
// methods, closes it. Objective-C compiled with manual reference counting (objc_release, objc_autorelease).
// Unless a definition says otherwise: Foundation image, iOS Simulator 24A434, arm64, UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json; data read from the image's __cfstring and
// __const sections). iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Foundation/String/ONSStringReview.md.
//
// The GOT slots outside the extracted image are bound by their use (confidence: medium): 0x22227FAE0 is the
// NSException class (the receiver of +exceptionWithName:reason:userInfo:), 0x22227F4F8, 0x22227F500 and
// 0x22227F5E0 are NSInternalInconsistencyException, NSInvalidArgumentException and NSRangeException (the
// names thrown with "Too many strings to append", "%@: nil argument" and "%@: Range {%lu, %lu} out of bounds
// ..."), and 0x222280B58 and 0x222280B70 are kCFAllocatorMalloc and kCFAllocatorSystemDefault (the slots
// follow the alphabetical order of the imported names, as the exception slots do).

// The NSPlaceholderMutableString class symbol, the isa of __mutablePlaceholder.
extern char
    mutableStringPlaceholderClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSPlaceholderMutableString));

// The NSException class symbol (CoreFoundation), loaded natively through GOT slot 0x22227FAE0.

// Native ___mutablePlaceholder, 0x2227d5ad0 (__DATA_CONST,__const, non-external): {
// NSPlaceholderMutableString, 0 }.
const StringPlaceholderObject __mutablePlaceholder = {mutableStringPlaceholderClass, 0};

@implementation ONSMutableString

// Native +[NSMutableString allocWithZone:], 0x1c0570720. Non-external. NSMutableString itself answers
// __mutablePlaceholder; any other class gets NSAllocateObject(self, 0, zone) (a tail call).
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == __NSMutableStringClass) {
        return (id)&__mutablePlaceholder;
    }
    return ONSAllocateObject(self, 0, zone);
}

// Native +[NSMutableString stringWithCapacity:], 0x1c0570744. Non-external. -initWithCapacity: sent to
// __mutablePlaceholder whatever the receiver, autoreleased.
+ (id)stringWithCapacity:(ONSUInteger)capacity {
    return [[(id)&__mutablePlaceholder initWithCapacity:capacity] autorelease];
}

// Native -[NSMutableString insertString:atIndex:], 0x1c0570760. Non-external. A tail call of
// -replaceCharactersInRange:{location, 0} withString:.
- (void)insertString:(id)string atIndex:(ONSUInteger)location {
    [self replaceCharactersInRange:(ONSRange){location, 0} withString:string];
}

// Native -[NSMutableString appendString:], 0x1c0570770. Non-external. Replaces {length, 0}.
- (void)appendString:(id)string {
    [self replaceCharactersInRange:(ONSRange){[self length], 0} withString:string];
}

// Native -[NSMutableString deleteCharactersInRange:], 0x1c05707a4. Non-external. Replaces the range with the
// empty constant string (0x22281E130).
- (void)deleteCharactersInRange:(ONSRange)range {
    [self replaceCharactersInRange:range withString:(id)OF_STR("")];
}

// Native -[NSMutableString replaceCharactersInRange:withCString:length:], 0x1c05707b0. Non-external. A
// temporary [[NSString allocWithZone:nil] initWithCString:length:] (objc_allocWithZone) replaces the range
// and is released.
- (void)replaceCharactersInRange:(ONSRange)range
                     withCString:(const char *)cString
                          length:(ONSUInteger)length {
    id string = [[ONSString allocWithZone:NULL] initWithCString:cString length:length];
    [self replaceCharactersInRange:range withString:string];
    [string release];
}

// Native -[NSMutableString replaceCharactersInRange:withCharacters:length:], 0x1c0570828. Non-external. As
// -replaceCharactersInRange:withCString:length: with -initWithCharacters:length:.
- (void)replaceCharactersInRange:(ONSRange)range
                  withCharacters:(const unsigned short *)characters
                          length:(ONSUInteger)length {
    id string = [[ONSString allocWithZone:NULL] initWithCharacters:characters length:length];
    [self replaceCharactersInRange:range withString:string];
    [string release];
}

// Native -[NSMutableString appendFormat:], 0x1c05708a0. Non-external. [[NSString allocWithZone:nil]
// initWithFormat:format locale:nil arguments:] over the variadic arguments, appended at {length, 0}, then
// released.
- (void)appendFormat:(id)format, ... {
    va_list arguments;
    va_start(arguments, format);
    id string = [[ONSString allocWithZone:NULL] initWithFormat:format locale:nil arguments:arguments];
    [self replaceCharactersInRange:(ONSRange){[self length], 0} withString:string];
    [string release];
    va_end(arguments);
}

// Native -[NSMutableString setString:], 0x1c057093c. Non-external. Replaces {0, length}.
- (void)setString:(id)string {
    [self replaceCharactersInRange:(ONSRange){0, [self length]} withString:string];
}

// Native -[NSMutableString replaceOccurrencesOfString:withString:options:range:], 0x1c0570970. Non-external.
// The length is read first. A nil target or replacement raises NSInvalidArgumentException ("%@: nil
// argument"); a range past the length raises NSRangeException. NSRegularExpressionSearch (0x400) tail-calls
// -_replaceOccurrencesOfRegularExpressionPattern:withTemplate:options:range:. Otherwise
// CFStringCreateArrayWithFindResults runs with the options plus kCFCompareNonliteral (0x10) unless
// NSLiteralSearch (2) is set; each found range is replaced, from the last to the first unless
// NSBackwardsSearch (4) is set (then from the first); the count is answered (0 without results).
- (ONSUInteger)replaceOccurrencesOfString:(id)target
                               withString:(id)replacement
                                  options:(ONSUInteger)options
                                    range:(ONSRange)searchRange {
    ONSUInteger length = [self length];
    if (target == nil || replacement == nil) {
        id reason =
            [ONSString stringWithFormat:(id)OF_STR("%@: nil argument"), _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([(id)mutableStringExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:reason
                     userInfo:nil]);
    }
    if (length < searchRange.length || searchRange.location > length - searchRange.length) {
        id reason =
            [ONSString stringWithFormat:(id)OF_STR("%@: Range {%lu, %lu} out of bounds; string length %lu"),
                                        _ONSMethodExceptionProem(self, _cmd),
                                        searchRange.location,
                                        searchRange.length,
                                        length];
        objc_exception_throw([(id)mutableStringExceptionClass exceptionWithName:(id)ONSRangeException
                                                                         reason:reason
                                                                       userInfo:nil]);
    }
    if ((options & 0x400) != 0) {
        return [self _replaceOccurrencesOfRegularExpressionPattern:target
                                                      withTemplate:replacement
                                                           options:options
                                                             range:searchRange];
    }
    OCFArrayRef results = OCFStringCreateArrayWithFindResults(
        NULL,
        (OCFStringRef)self,
        (OCFStringRef)target,
        (OCFRange){(OCFIndex)searchRange.location, (OCFIndex)searchRange.length},
        (OCFOptionFlags)((~(options << 3) & 0x10) | options)
    );
    if (results == NULL) {
        return 0;
    }
    OCFIndex count = OCFArrayGetCount(results);
    if (count != 0) {
        OCFIndex forward = 0;
        OCFIndex backward = count - 1;
        do {
            const OCFRange *range =
                (const OCFRange *)OCFArrayGetValueAtIndex(results, (options & 4) == 0 ? backward : forward);
            [self
                replaceCharactersInRange:(ONSRange){(ONSUInteger)range->location, (ONSUInteger)range->length}
                              withString:replacement];
            forward++;
            backward--;
        } while (backward != -1);
    }
    OCFRelease(results);
    return (ONSUInteger)count;
}

// Native +[NSMutableString _subclassesMustBeExplicitlyMentionedWhenDecoded], 0x1c0570ba8. Non-external.
// Answers YES.
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded {
    return YES;
}

// Native -[NSMutableString classForCoder], 0x1c0570bb0. Non-external. __NSMutableStringClass.
- (Class)classForCoder {
    return __NSMutableStringClass;
}

// Native __NSNewStringByAppendingStrings, 0x1c0571a50. External. The encoding is _NSDefaultStringEncoding
// once
// ___NSDefaultStringEncodingFullyInited is 1, else __NSDefaultCStringEncoding(). Whether that encoding is a
// superset of ASCII is tested inline: NSASCIIStringEncoding, NSISOLatin1StringEncoding and
// NSMacOSRomanStringEncoding (mask 0x40000022 over encodings up to 30) are; otherwise its CFStringEncoding is
// classified by its second byte exactly as the assembly branches (the classification CF's
// __CFStringEncodingIsSupersetOfASCII makes; only bit 0 of the result is used). At most 40 strings are read
// from the variadic area (the 41st raises NSInternalInconsistencyException "Too many strings to append").
// While either flag holds, each string's -fastestEncoding keeps "all ASCII" (fastest ==
// NSASCIIStringEncoding) and "all eight bit" (fastest equals the encoding, or the encoding is an ASCII
// superset and fastest is ASCII). The lengths are summed. With neither flag the characters are copied with
// -getCharacters:range: into a zone buffer (malloc_type_zone_malloc, type 0x409734D7; none for a total of 0;
// nil for a total with bit 63 set) adopted by
// -[NSString initWithCharactersNoCopy:length:freeWhenDone:YES] allocated in the original zone. Otherwise the
// bytes (ASCII when all are ASCII, else the encoding with the summed -lengthOfBytesUsingEncoding:) go through
// -getBytes:maxLength:usedLength:encoding:options:range:remainingRange: into a total + 1 byte buffer (NULL
// when that overflows), NUL-terminated, and
// _CFStringCreateWithNullTerminatedBytesNoCopy(kCFAllocatorSystemDefault,
// ..., kCFAllocatorMalloc) makes the string. No string at all yields an empty ASCII string over a one-byte
// buffer.
id _ONSNewStringByAppendingStrings(struct _NSZone *zone, ...) {
    ONSUInteger encoding =
        __ONSDefaultStringEncodingFullyInited == 1 ? _ONSDefaultStringEncoding : _ONSDefaultCStringEncoding();
    uint32_t notASCIISuperset;
    if (encoding <= 0x1E && ((1UL << encoding) & 0x40000022) != 0) {
        notASCIISuperset = 0;
    } else {
        uint32_t cfEncoding = OCFStringConvertNSStringEncodingToEncoding(encoding);
        uint32_t superset;
        switch ((cfEncoding >> 8) & 0xFF) {
            case 0:
                if (cfEncoding <= 0x22 && ((1ULL << cfEncoding) & 0x600000032ULL) != 0) {
                    superset = 0;
                } else if (cfEncoding == 0x98) {
                    superset = 0;
                } else {
                    superset = 1;
                }
                break;
            case 1:
                superset = cfEncoding == 0x8000100;
                break;
            case 2:
                superset = cfEncoding != 0x206;
                break;
            case 6:
                superset = cfEncoding == 0x600;
                break;
            case 8:
            case 0xC:
                superset = 0;
                break;
            case 0xA:
                if (cfEncoding - 0xA01 >= 0x10) {
                    superset = 1;
                } else {
                    superset = 0x7FEEu >> (cfEncoding - 0xA01);
                }
                break;
            case 0xB:
                if (cfEncoding == 0xBFF) {
                    superset = 0;
                } else {
                    superset = cfEncoding - 0xBF5 < 0xFFFFFFFEu;
                }
                break;
            default:
                superset = (cfEncoding & 0xFF00) < 0xC01;
                break;
        }
        notASCIISuperset = superset ^ 1;
    }

    va_list arguments;
    va_start(arguments, zone);
    id strings[40];
    ONSUInteger lengths[40];
    char *buffer;
    char *cursor;
    id string = va_arg(arguments, id);
    if (string == nil) {
        if (zone == NULL) {
            zone = (struct _NSZone *)malloc_default_zone();
        }
        encoding = 1;
        buffer = malloc_type_zone_malloc((malloc_zone_t *)zone, 1, 0x409734D7);
        cursor = buffer;
        va_end(arguments);
        goto terminate;
    }
    ONSUInteger count = 1;
    uint32_t allASCII = 1;
    uint32_t allEightBit = 1;
    for (;;) {
        if ((allEightBit & 1) != 0 || (allASCII & 1) != 0) {
            ONSUInteger fastest = [string fastestEncoding];
            allASCII &= fastest == 1;
            allEightBit &= (fastest == encoding) | ((notASCIISuperset | (fastest != 1)) ^ 1);
        } else {
            allASCII = 0;
            allEightBit = 0;
        }
        strings[count - 1] = string;
        string = va_arg(arguments, id);
        if (string == nil) {
            break;
        }
        count++;
        if (count == 41) {
            objc_exception_throw([(id)mutableStringExceptionClass
                exceptionWithName:(id)ONSInternalInconsistencyException
                           reason:(id)OF_STR("Too many strings to append")
                         userInfo:nil]);
        }
    }
    va_end(arguments);
    ONSUInteger total = 0;
    for (ONSUInteger index = 0; index != count; index++) {
        ONSUInteger length = [strings[index] length];
        total += length;
        lengths[index] = length;
    }
    if (((allEightBit | allASCII) & 1) == 0) {
        if ((total >> 63) != 0) {
            return nil;
        }
        unsigned short *characters;
        if (total == 0) {
            characters = NULL;
        } else {
            malloc_zone_t *mallocZone = (malloc_zone_t *)zone;
            if (zone == NULL) {
                mallocZone = malloc_default_zone();
            }
            characters = malloc_type_zone_malloc(mallocZone, total << 1, 0x409734D7);
        }
        unsigned short *destination = characters;
        for (ONSUInteger index = 0; index != count; index++) {
            ONSUInteger length = lengths[index];
            [strings[index] getCharacters:destination range:(ONSRange){0, length}];
            destination += length;
        }
        return [[ONSString allocWithZone:zone] initWithCharactersNoCopy:characters
                                                                 length:total
                                                           freeWhenDone:YES];
    }
    if ((allASCII & 1) == 0) {
        total = 0;
        for (ONSUInteger index = 0; index != count; index++) {
            total += [strings[index] lengthOfBytesUsingEncoding:encoding];
        }
    } else {
        encoding = 1;
    }
    if (total + 1 < total) {
        buffer = NULL;
    } else {
        if (zone == NULL) {
            zone = (struct _NSZone *)malloc_default_zone();
        }
        buffer = malloc_type_zone_malloc((malloc_zone_t *)zone, total + 1, 0x409734D7);
    }
    char *end = buffer + total;
    cursor = buffer;
    for (ONSUInteger index = 0; index != count; index++) {
        // STP XZR, XZR at native 0x1c0571dac initializes the output even when conversion fails.
        ONSUInteger used = 0;
        [strings[index] getBytes:cursor
                       maxLength:(ONSUInteger)(end - cursor)
                      usedLength:&used
                        encoding:encoding
                         options:0
                           range:(ONSRange){0, lengths[index]}
                  remainingRange:NULL];
        cursor += used;
    }
terminate:
    *cursor = 0;
    return (id)_OCFStringCreateWithNullTerminatedBytesNoCopy(
        kOCFAllocatorSystemDefault,
        (const uint8_t *)buffer,
        cursor - buffer,
        OCFStringConvertNSStringEncodingToEncoding(encoding),
        kOCFAllocatorMalloc
    );
}

// Native -[NSMutableString replaceCharactersInRange:withString:], 0x1c0fe39b0 (outlined region).
// Non-external. The abstract primitive: a tail call of NSRequestConcreteImplementation(self, _cmd,
// __NSMutableStringClass).
- (void)replaceCharactersInRange:(ONSRange)range withString:(id)string {
    (void)range;
    (void)string;
    ONSRequestConcreteImplementation(self, _cmd, __NSMutableStringClass);
}

// Native -[NSMutableString initWithCapacity:], 0x1c0fe39bc (outlined region). Non-external.
// _NSRequestConcreteObject(self, _cmd), then the receiver.
- (id)initWithCapacity:(ONSUInteger)capacity {
    (void)capacity;
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

@end
