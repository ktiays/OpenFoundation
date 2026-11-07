//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSStringOtherEncodings.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFData.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringByteStream.h"
#include "CoreFoundation/StringEncoding/OCFSingleByte.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSZone.h"
#import "Foundation/Base/ONSFoundationSupport.h"

// The Foundation bundle message of +localizedNameOfStringEncoding:, declared for the send only.
@protocol OtherEncodingsBundleMessages
- (id)localizedStringForKey:(id)key value:(id)value table:(id)table;
@end

#include <malloc/malloc.h>
#include <objc/objc-exception.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the __NSLocalizedStringAddition pair (0x1c05d9e2c-0x1c05d9e90), the C string
// character mapping (___NSSetCStringCharToUnichar, __NSDefaultCStringEncoding) and the NSStringOtherEncodings
// methods with _bytesInEncoding and __NSConvertToASCIIorUnicode2 (0x1c05d9ea4-0x1c05db07c) follow the
// NSPlaceholderString run in the iOS Simulator __text section; ___NSASCIICharToUnichar and
// ___NSCharToUnicharCFWrapper lie in the outlined region (0x1c0fdedf0-0x1c0fdee6c). The categories and their
// state have their own translation unit, named for NSStringOtherEncodings. Methods of the run that the scope
// does not reach (0x1c05da088-0x1c05da280, 0x1c05da4e0-0x1c05da5dc, 0x1c05da5e8-0x1c05da72c,
// 0x1c05daa14-0x1c05daaa8) are not restored. The unit is Objective-C compiled with manual reference counting
// (objc_autorelease, objc_release). Unless a definition says otherwise: iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device 24A446
// remains unread. Reading: Foundation/String/ONSStringReview.md.
//
// GOT slots outside the extracted image, identified by use (confidence: medium): 0x22227fae0 the NSException
// class (the receiver of +exceptionWithName:reason:userInfo:); 0x22227f4f8 NSInternalInconsistencyException
// (the name of "didn't convert all characters", the slot before NSInvalidArgumentException's 0x22227f500 in
// name order); 0x22227fad0 the NSDictionary class (+dictionaryWithObjectsAndKeys:); 0x22227fb20 the
// NSMutableData class (+dataWithLength:, -initWithLength:, -mutableBytes); 0x222280698 CoreFoundation's
// __CFCharToUniCharFunc; 0x2222806c0 and 0x2222806d0 CoreFoundation's __CFIdempotentCharToUniCharTable and
// __CFMacRomanCharToUnicharTable (512-byte tables copied as such; the second when the default encoding is
// NSMacOSRomanStringEncoding, 30; the order of their names among the CoreFoundation data imports).

// The __NSLocalizedString, NSException, NSDictionary and NSMutableData classes.

// Native _NSDefaultStringEncoding, 0x22e6e46f0, external (__DATA,__data, initially -1).
ONSUInteger _ONSDefaultStringEncoding = (ONSUInteger)-1;

// Native _NSCStringEncoding, 0x22e6e46f8, external (__DATA,__data, initially -1).
ONSUInteger _ONSCStringEncoding = (ONSUInteger)-1;

// Native ___NSDefaultStringEncodingFullyInited, 0x22e775b30, external (__DATA,__common).
BOOL __ONSDefaultStringEncodingFullyInited;

// Native __NSCStringCharToUnichar, 0x22e775b38, external (__DATA,__common).
CStringCharToUnicharFunction _ONSCStringCharToUnichar;

// Native __NSCStringCharToUnicharTable, 0x22e775b40, was a private external (__DATA,__common).
unsigned short *_NSCStringCharToUnicharTable;

// The encoding check the methods of this unit inline: the CFStringEncoding of the NSStringEncoding; when
// there is none (kCFStringEncodingInvalidId, -1) and the value is 0 or 0x8000100 (kCFStringEncodingUTF8
// passed as an NSStringEncoding), logs "Incorrect NSStringEncoding value..." when the executable is linked on
// or after version 6 and answers kCFStringEncodingASCII (0x600); any other value answers -1.
// encodingsCFEncoding is a local descriptive name.
static inline OCFStringEncoding encodingsCFEncoding(ONSUInteger encoding) {
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

@implementation ONSString (__NSLocalizedStringAddition)

// Native -[NSString(__NSLocalizedStringAddition) _copyFormatStringWithConfiguration:], 0x1c05d9e2c.
// Non-external. A new __NSLocalizedString of the receiver (its -baseString when the receiver is a
// __NSLocalizedString) with the configuration; retained (a copy method).
- (id)_copyFormatStringWithConfiguration:(id)configuration {
    id string = self;
    if ([self isKindOfClass:[(id)encodingsLocalizedStringClass class]]) {
        string = [(id)self baseString];
    }
    return [[(id)encodingsLocalizedStringClass alloc] initWithString:string
                                             withFormatConfiguration:configuration];
}

// Native -[NSString(__NSLocalizedStringAddition) formatConfiguration], 0x1c05d9e8c. Non-external. Answers
// nil.
- (id)formatConfiguration {
    return nil;
}

@end

// Native ___NSSetCStringCharToUnichar, 0x1c05d9ea4. Hidden (was a private external). Installs the C string
// character mapping function. With a table already built for the same function nothing changes. A previous
// table is freed unless it is NULL or one of CoreFoundation's two static tables. __NSASCIICharToUnichar and
// NULL use __CFIdempotentCharToUniCharTable, as does __NSCharToUnicharCFWrapper unless the default encoding
// is NSMacOSRomanStringEncoding (30), which uses __CFMacRomanCharToUnicharTable; any other function gets a
// new 512-byte table (malloc_type_malloc, type 0x1000040BDFB0063) with the first 128 entries copied from
// __CFIdempotentCharToUniCharTable and entries 0x80-0xFF from the function. The table, then the function,
// are stored.
void __NSSetCStringCharToUnichar(CStringCharToUnicharFunction function) {
    unsigned short *table = _NSCStringCharToUnicharTable;
    if (table != NULL && function == _ONSCStringCharToUnichar) {
        _ONSCStringCharToUnichar = function;
        return;
    }
    if (table != __OCFIdempotentCharToUniCharTable && table != __OCFMacRomanCharToUnicharTable &&
        table != NULL) {
        free(table);
    }
    unsigned short *newTable = (unsigned short *)__OCFIdempotentCharToUniCharTable;
    if (function != NULL && function != __NSASCIICharToUnichar) {
        if (function == __NSCharToUnicharCFWrapper && _ONSDefaultStringEncoding == 0x1E) {
            newTable = (unsigned short *)__OCFMacRomanCharToUnicharTable;
        } else {
            unsigned short *built = malloc_type_malloc(0x200, 0x1000040BDFB0063);
            memcpy(built, __OCFIdempotentCharToUniCharTable, 0x100);
            for (unsigned long character = 0x80; character != 0x100; character++) {
                built[character] = function((unsigned char)character);
            }
            newTable = built;
        }
    }
    _NSCStringCharToUnicharTable = newTable;
    _ONSCStringCharToUnichar = function;
}

// Native __NSDefaultCStringEncoding, 0x1c05d9fdc. External. Stores the NSStringEncoding of the system
// encoding in _NSDefaultStringEncoding. When CoreFoundation's __CFCharToUniCharFunc is set,
// _NSCStringEncoding becomes that encoding, the mapping function __NSCharToUnicharCFWrapper and the state
// fully initialized; otherwise the state is fully initialized only when the system encoding is not
// kCFStringEncodingMacRoman (0), _NSCStringEncoding becomes NSASCIIStringEncoding (1) and the mapping
// function __NSASCIICharToUnichar. Answers _NSDefaultStringEncoding, reloaded.
ONSUInteger _ONSDefaultCStringEncoding(void) {
    ONSUInteger encoding = OCFStringConvertEncodingToNSStringEncoding(OCFStringGetSystemEncoding());
    _ONSDefaultStringEncoding = encoding;
    if (__OCFCharToUniCharFunc != NULL) {
        _ONSCStringEncoding = encoding;
        __NSSetCStringCharToUnichar(__NSCharToUnicharCFWrapper);
        __ONSDefaultStringEncodingFullyInited = YES;
    } else {
        __ONSDefaultStringEncodingFullyInited = OCFStringGetSystemEncoding() != 0;
        _ONSCStringEncoding = 1;
        __NSSetCStringCharToUnichar(__NSASCIICharToUnichar);
    }
    return _ONSDefaultStringEncoding;
}

// Native _bytesInEncoding, 0x1c05da298. Non-external. The NUL-terminated bytes of the receiver in the
// encoding, valid for the autorelease pool, or NULL. The conversion options are 6 with
// externalRepresentation, otherwise 4, ORed with extraOptions. A receiver of at most 0xA5 characters is
// converted into a 1,008-byte stack buffer (maxLength 1,000) and, on success, NUL-terminated and copied into
// an autoreleased CFData whose byte pointer is answered when non-NULL. Otherwise the converted length is
// measured (maxLength 0x7FFFFFFFFFFFFFFE, no buffer), an NSMutableData of that length + 1 is made with
// +dataWithLength:, and the bytes are converted into its -mutableBytes; the result is answered NUL-terminated
// when the second conversion succeeds with the same length. On failure, raiseOnFailure raises
// NSCharacterConversionException ("Conversion to encoding %ld failed", user info {_NSString: receiver});
// otherwise NULL. raiseOnFailure, externalRepresentation and extraOptions are descriptive names (the native
// callers pass 0/0/0, 1/1/0 for -cString).
static const char *bytesInEncoding(
    id string,
    ONSUInteger encoding,
    BOOL raiseOnFailure,
    BOOL externalRepresentation,
    unsigned int extraOptions
) {
    ONSUInteger length = [string length];
    ONSUInteger secondUsedLength = 0;
    ONSUInteger usedLength = 0;
    ONSUInteger options = (externalRepresentation ? 6 : 4) | extraOptions;
    uint8_t bytes[1008];
    if (length <= 0xA5) {
        if ([string getBytes:bytes
                     maxLength:1000
                    usedLength:&usedLength
                      encoding:encoding
                       options:options
                         range:(ONSRange){0, length}
                remainingRange:NULL]) {
            bytes[usedLength] = 0;
            OCFDataRef data = OCFDataCreate(NULL, bytes, (OCFIndex)(usedLength + 1));
            const UInt8 *pointer = OCFDataGetBytePtr(data);
            [(id)data autorelease];
            if (pointer != NULL) {
                return (const char *)pointer;
            }
        }
    }
    if ([string getBytes:NULL
                 maxLength:(LONG_MAX - 1)
                usedLength:&usedLength
                  encoding:encoding
                   options:options
                     range:(ONSRange){0, length}
            remainingRange:NULL]) {
        uint8_t *buffer = [[(id)encodingsMutableDataClass dataWithLength:usedLength + 1] mutableBytes];
        if (buffer != NULL) {
            if ([string getBytes:buffer
                         maxLength:usedLength
                        usedLength:&secondUsedLength
                          encoding:encoding
                           options:options
                             range:(ONSRange){0, length}
                    remainingRange:NULL] &&
                secondUsedLength == usedLength) {
                buffer[secondUsedLength] = 0;
                return (const char *)buffer;
            }
        }
    }
    if (raiseOnFailure) {
        id reason = [ONSString stringWithFormat:(id)OF_STR("Conversion to encoding %ld failed"), encoding];
        id userInfo =
            [(id)encodingsDictionaryClass dictionaryWithObjectsAndKeys:string, OF_STR("_NSString"), nil];
        objc_exception_throw([(id)encodingsExceptionClass exceptionWithName:ONSCharacterConversionException
                                                                     reason:reason
                                                                   userInfo:userInfo]);
    }
    return NULL;
}

@implementation ONSString (NSStringOtherEncodings)

// Native +[NSString(NSStringOtherEncodings) defaultCStringEncoding], 0x1c05da068. Non-external.
// _NSDefaultStringEncoding once the state is fully initialized (the byte equals 1), otherwise
// __NSDefaultCStringEncoding.
+ (ONSUInteger)defaultCStringEncoding {
    if (__ONSDefaultStringEncodingFullyInited == 1) {
        return _ONSDefaultStringEncoding;
    }
    return _ONSDefaultCStringEncoding();
}

// Native -[NSString(NSStringOtherEncodings) UTF8String], 0x1c05da284. Non-external. _bytesInEncoding in
// NSUTF8StringEncoding (4) without raising.
- (const char *)UTF8String {
    return bytesInEncoding(self, 4, NO, NO, 0);
}

// Native -[NSString(NSStringOtherEncodings) cString], 0x1c05da490. Non-external. _bytesInEncoding in the
// default C string encoding (as +defaultCStringEncoding), raising, as an external representation.
- (const char *)cString {
    ONSUInteger encoding;
    if (__ONSDefaultStringEncodingFullyInited == 1) {
        encoding = _ONSDefaultStringEncoding;
    } else {
        encoding = _ONSDefaultCStringEncoding();
    }
    return bytesInEncoding(self, encoding, YES, YES, 0);
}

// Native -[NSString(NSStringOtherEncodings) fastestEncoding], 0x1c05da5e0. Non-external. Answers
// NSUnicodeStringEncoding (10).
- (ONSUInteger)fastestEncoding {
    return 10;
}

// Native -[NSString(NSStringOtherEncodings) dataUsingEncoding:allowLossyConversion:], 0x1c05da730.
// Non-external. An encoding that CFStringIsEncodingAvailable rejects (after the inlined encoding check) logs
// "Unknown encoding %ld" and answers nil. The loss byte is 0 without lossy, otherwise 0xFF for
// NSASCIIStringEncoding (1) and '?' for the others. __CFStringEncodeByteStream measures the external
// representation of the whole receiver; a conversion of fewer characters answers nil. A new NSMutableData of
// the measured length (+allocWithZone: NULL, -initWithLength:) receives the bytes when the length is
// non-zero; a second conversion of fewer characters releases it and raises NSInternalInconsistencyException
// ("%@: didn't convert all characters"); otherwise -setLength: trims it. Autoreleased.
- (id)dataUsingEncoding:(ONSUInteger)encoding allowLossyConversion:(BOOL)lossy {
    OCFIndex length = (OCFIndex)[self length];
    OCFIndex usedLength = 0;
    OCFStringEncoding cfEncoding = encodingsCFEncoding(encoding);
    if (!OCFStringIsEncodingAvailable(cfEncoding)) {
        ONSLog((id)OF_STR("Unknown encoding %ld"), encoding);
        return nil;
    }
    UInt8 lossByte = lossy ? (encoding == 1 ? 0xFF : '?') : 0;
    if (__OCFStringEncodeByteStream(
            (OCFStringRef)self, 0, length, true, cfEncoding, lossByte, NULL, 0, &usedLength
        ) != length) {
        return nil;
    }
    id data = [[(id)encodingsMutableDataClass allocWithZone:NULL] initWithLength:(ONSUInteger)usedLength];
    if (usedLength != 0) {
        UInt8 *bytes = [data mutableBytes];
        if (__OCFStringEncodeByteStream(
                (OCFStringRef)self, 0, length, true, cfEncoding, lossByte, bytes, usedLength, &usedLength
            ) != length) {
            [data release];
            id reason = [ONSString stringWithFormat:(id)OF_STR("%@: didn't convert all characters"),
                                                    _ONSMethodExceptionProem(self, _cmd)];
            objc_exception_throw([(id)encodingsExceptionClass
                exceptionWithName:(id)ONSInternalInconsistencyException
                           reason:reason
                         userInfo:nil]);
        }
        [data setLength:(ONSUInteger)usedLength];
    }
    return [data autorelease];
}

// Native -[NSString(NSStringOtherEncodings) dataUsingEncoding:], 0x1c05da92c. Non-external. Without lossy
// conversion.
- (id)dataUsingEncoding:(ONSUInteger)encoding {
    return [self dataUsingEncoding:encoding allowLossyConversion:NO];
}

// Native -[NSString(NSStringOtherEncodings) cStringUsingEncoding:], 0x1c05da934. Non-external.
// _bytesInEncoding without raising.
- (const char *)cStringUsingEncoding:(ONSUInteger)encoding {
    return bytesInEncoding(self, encoding, NO, NO, 0);
}

// Native -[NSString(NSStringOtherEncodings) getCString:maxLength:encoding:], 0x1c05da948. Non-external. A
// maximum of 0 or 1 (with 1 the buffer is NUL-terminated) answers whether the receiver is empty. Otherwise
// the whole receiver is converted into maxLength - 1 bytes with options 4; success NUL-terminates after the
// used length and answers YES, failure stores NUL at the start and answers NO.
- (BOOL)getCString:(char *)buffer maxLength:(ONSUInteger)maxBufferCount encoding:(ONSUInteger)encoding {
    if (maxBufferCount <= 1) {
        if (maxBufferCount == 1) {
            buffer[0] = 0;
        }
        return [self length] == 0;
    }
    ONSUInteger usedLength = 0;
    if ([self getBytes:buffer
                 maxLength:maxBufferCount - 1
                usedLength:&usedLength
                  encoding:encoding
                   options:4
                     range:(ONSRange){0, [self length]}
            remainingRange:NULL]) {
        buffer[usedLength] = 0;
        return YES;
    }
    buffer[0] = 0;
    return NO;
}

// Native -[NSString(NSStringOtherEncodings) lengthOfBytesUsingEncoding:], 0x1c05daaac. Non-external. An
// encoding without a CFStringEncoding (after the inlined check) answers 0; otherwise the measured byte length
// of the whole receiver (no external representation, no loss byte) when every character converts, else 0.
- (ONSUInteger)lengthOfBytesUsingEncoding:(ONSUInteger)encoding {
    OCFIndex length = (OCFIndex)[self length];
    OCFIndex usedLength = 0;
    OCFStringEncoding cfEncoding = encodingsCFEncoding(encoding);
    if (cfEncoding == (OCFStringEncoding)-1) {
        return 0;
    }
    OCFIndex converted = __OCFStringEncodeByteStream(
        (OCFStringRef)self, 0, length, false, cfEncoding, 0, NULL, 0, &usedLength
    );
    return converted == length ? (ONSUInteger)usedLength : 0;
}

// Native -[NSString(NSStringOtherEncodings) getBytes:maxLength:usedLength:encoding:options:range:
// remainingRange:], 0x1c05dab98. Non-external. An empty receiver answers YES with nothing converted; an
// encoding CFStringIsEncodingAvailable rejects answers NO with nothing converted. Otherwise
// __CFStringEncodeByteStream converts the range: option bit 1 is the external representation, bit 0 the loss
// byte (0xFF for NSASCIIStringEncoding, '?' otherwise), and a NULL buffer has a maximum of 0. With option bit
// 2 a partial conversion answers NO; any conversion of no characters answers NO; otherwise YES. The used
// length (when requested) and the remaining range {location + converted, length - converted} (when
// requested) are stored in every case.
- (BOOL)getBytes:(void *)buffer
         maxLength:(ONSUInteger)maxBufferCount
        usedLength:(ONSUInteger *)usedBufferCount
          encoding:(ONSUInteger)encoding
           options:(ONSUInteger)options
             range:(ONSRange)range
    remainingRange:(ONSRange *)leftover {
    OCFIndex usedLength = 0;
    OCFStringEncoding cfEncoding = encodingsCFEncoding(encoding);
    OCFIndex converted;
    BOOL result;
    if ([self length] == 0) {
        converted = 0;
        result = YES;
    } else if (!OCFStringIsEncodingAvailable(cfEncoding)) {
        converted = 0;
        result = NO;
    } else {
        UInt8 lossByte = (options & 1) == 0 ? 0 : (encoding == 1 ? 0xFF : '?');
        converted = __OCFStringEncodeByteStream(
            (OCFStringRef)self,
            (OCFIndex)range.location,
            (OCFIndex)range.length,
            (options >> 1) & 1,
            cfEncoding,
            lossByte,
            buffer,
            buffer == NULL ? 0 : (OCFIndex)maxBufferCount,
            &usedLength
        );
        if ((options & 4) != 0 && (ONSUInteger)converted < range.length) {
            result = NO;
        } else {
            result = converted != 0;
        }
    }
    if (usedBufferCount != NULL) {
        *usedBufferCount = (ONSUInteger)usedLength;
    }
    if (leftover != NULL) {
        *leftover =
            (ONSRange){(ONSUInteger)converted + range.location, range.length - (ONSUInteger)converted};
    }
    return result;
}

// Native -[NSString(NSStringOtherEncodings) getBytes:maxLength:filledLength:encoding:allowLossyConversion:
// range:remainingRange:], 0x1c05dad24. Non-external. The options are the 32-bit lossy flag ORed with 6.
- (BOOL)getBytes:(void *)buffer
               maxLength:(ONSUInteger)maxLength
            filledLength:(ONSUInteger *)filledLength
                encoding:(ONSUInteger)encoding
    allowLossyConversion:(BOOL)lossy
                   range:(ONSRange)range
          remainingRange:(ONSRange *)remainingRange {
    return [self getBytes:buffer
                maxLength:maxLength
               usedLength:filledLength
                 encoding:encoding
                  options:(unsigned int)lossy | 6
                    range:range
           remainingRange:remainingRange];
}

// Native +[NSString(NSStringOtherEncodings) localizedNameOfStringEncoding:], 0x1c05db2bc. Non-external. (It
// follows __NSConvertToASCIIorUnicode2 natively; it is kept in its category here.) The
// encoding's CoreFoundation name localized from Foundation's EncodingNames table, or the empty string. An
// unknown encoding of 0 or 0x8000100 (kCFStringEncodingUTF8 passed as an NSStringEncoding) is taken as ASCII,
// logged for executables linked on or after version 6.
+ (id)localizedNameOfStringEncoding:(NSUInteger)encoding {
    OCFStringEncoding converted = OCFStringConvertNSStringEncodingToEncoding(encoding);
    if (converted == 0xFFFFFFFF) {
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
            converted = 0x600;
        } else {
            converted = 0xFFFFFFFF;
        }
    }
    OCFStringRef name = OCFStringGetNameOfEncoding(converted);
    if (name == NULL) {
        return (id)OF_STR("");
    }
    return [(id<OtherEncodingsBundleMessages>)_ONSFoundationBundle()
        localizedStringForKey:(id)name
                        value:(id)OF_STR("")
                        table:(id)OF_STR("EncodingNames")];
}

@end

// Native __NSConvertToASCIIorUnicode2, 0x1c05dad38. External. Initializes the record (isASCII = alwaysUnicode
// ^ 1, not freed, no eight-bit characters, count 0); no bytes answer YES. NSUnicodeStringEncoding (10) and 0
// read UTF-16: a leading 0xFEFF or 0xFFFE unit (in host order) is skipped, the count is half the length
// less the mark, and the characters are copied as is after 0xFEFF and byte-swapped otherwise (also without a
// mark), into the local buffer up to 0x1F8 characters or from zone (malloc_default_zone when NULL;
// malloc_type_zone_malloc, type 0x409734D7) beyond; YES. Other encodings drop a UTF-8 byte order mark (an
// empty remainder answers YES), check the encoding (inlined) and decode with __CFStringDecodeByteStream3 into
// a VarWidthCharBuffer whose first 512 bytes are zeroed; the record takes isASCII and the count. A failed
// decode clears shouldFreeChars and the count. Eight-bit results are copied into the local buffer up to 0x3F0
// bytes or from zone (as above) beyond, UTF-16 results into the local buffer up to 0x1F8 characters or from
// NSZoneMalloc beyond; the decoder's own buffer is then deallocated when it is to be freed. Answers whether
// the encoding is available and the decode succeeded. No allocation result is checked (native behavior).
BOOL _ONSConvertToASCIIorUnicode2(
    const void *bytes,
    ONSUInteger length,
    ONSUInteger encoding,
    BOOL alwaysUnicode,
    StringConversionBuffer *buffer,
    struct _NSZone *zone
) {
    buffer->isASCII = (BOOL)(alwaysUnicode ^ 1);
    buffer->shouldFreeChars = NO;
    buffer->ascii = NULL;
    buffer->numChars = 0;
    if (length == 0) {
        return YES;
    }
    if (encoding == 10 || encoding == 0) {
        const unsigned short *characters = bytes;
        // Foundation defect: 0x1c05dad9c reads a UTF-16 unit even for a one-byte input.
        // Do not inspect a BOM unless a full unit exists; the truncated input still yields zero units.
        // Confidence: verified from the halfword load and the only preceding length==0 guard.
        unsigned short mark = length >= 2 ? characters[0] : 0;
        const unsigned short *source;
        if (mark == 0xFFFE || mark == 0xFEFF) {
            source = characters + 1;
        } else {
            source = characters;
            mark = 0;
        }
        ONSUInteger count = (length >> 1) - (mark != 0);
        buffer->isASCII = NO;
        buffer->numChars = count;
        size_t size = count << 1;
        unsigned short *destination;
        if (count > 0x1F8) {
            if (zone == NULL) {
                zone = (struct _NSZone *)malloc_default_zone();
            }
            destination = malloc_type_zone_malloc((malloc_zone_t *)zone, size, 0x409734D7);
        } else {
            destination = (unsigned short *)buffer->localBuffer;
        }
        buffer->unicode = destination;
        buffer->shouldFreeChars = count > 0x1F8;
        if (mark == 0xFEFF) {
            memmove(destination, source, size);
        } else {
            for (; count != 0; count--) {
                *destination++ = __builtin_bswap16(*source++);
            }
        }
        return YES;
    }
    const uint8_t *source = bytes;
    if (length >= 3 && encoding == 4 && source[0] == 0xEF && source[1] == 0xBB && source[2] == 0xBF) {
        length -= 3;
        if (length == 0) {
            return YES;
        }
        source += 3;
    }
    OCFStringEncoding cfEncoding = encodingsCFEncoding(encoding);
    Boolean available = OCFStringIsEncodingAvailable(cfEncoding);
    Boolean decoded = false;
    if (available) {
        VarWidthCharBuffer decodeBuffer;
        memset(&decodeBuffer, 0, 0x200);
        decoded = __OCFStringDecodeByteStream3(
            source, (OCFIndex)length, cfEncoding, alwaysUnicode, &decodeBuffer, NULL, 0
        );
        buffer->isASCII = decodeBuffer.isASCII != 0;
        OCFIndex count = decodeBuffer.numChars;
        buffer->numChars = (ONSUInteger)count;
        if (!decoded) {
            buffer->shouldFreeChars = NO;
            buffer->numChars = 0;
        } else {
            if (decodeBuffer.isASCII) {
                buffer->unicode = NULL;
                uint8_t *destination;
                BOOL allocated;
                if (count > 0x3F0) {
                    if (zone == NULL) {
                        zone = (struct _NSZone *)malloc_default_zone();
                    }
                    destination = malloc_type_zone_malloc((malloc_zone_t *)zone, (size_t)count, 0x409734D7);
                    allocated = YES;
                    count = decodeBuffer.numChars;
                } else {
                    destination = buffer->localBuffer;
                    allocated = NO;
                }
                buffer->ascii = destination;
                buffer->shouldFreeChars = allocated;
                memmove(destination, decodeBuffer.chars.ascii, (size_t)count);
            } else {
                buffer->ascii = NULL;
                unsigned short *destination;
                BOOL allocated;
                if (count > 0x1F8) {
                    destination = ONSZoneMalloc(zone, (ONSUInteger)count << 1);
                    allocated = YES;
                    count = decodeBuffer.numChars;
                } else {
                    destination = (unsigned short *)buffer->localBuffer;
                    allocated = NO;
                }
                buffer->unicode = destination;
                buffer->shouldFreeChars = allocated;
                memmove(destination, decodeBuffer.chars.unicode, (size_t)count << 1);
            }
            if (decodeBuffer.shouldFreeChars) {
                OCFAllocatorDeallocate(decodeBuffer.allocator, decodeBuffer.chars.ascii);
            }
        }
    }
    return available != 0 && decoded;
}

// Native ___NSASCIICharToUnichar, 0x1c0fdedf0, in the outlined region. Hidden (was a private external).
// Answers the character unchanged (a bare RET).
unsigned short __NSASCIICharToUnichar(unsigned char character) {
    return character;
}

// Native ___NSCharToUnicharCFWrapper, 0x1c0fdedf4, in the outlined region. Hidden (was a private external).
// CoreFoundation's __CFCharToUniCharFunc with flags 0; its 32-bit result selects the converted unit or
// U+FFFD.
unsigned short __NSCharToUnicharCFWrapper(unsigned char character) {
    UniChar unicodeCharacter = 0;
    if (__OCFCharToUniCharFunc(0, character, &unicodeCharacter) != 0) {
        return unicodeCharacter;
    }
    return 0xFFFD;
}
