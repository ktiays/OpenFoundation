//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSStringCFAdditions.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSStringOtherEncodings.h"

#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"

#include <malloc/malloc.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdlib.h>

// Inferred organization: the NSCFAdditions categories of NSString (0x1c04d1f64-0x1c04d21d7) and
// NSMutableString (0x1c04d21d8-0x1c04d24b7) lie together in the iOS Simulator __text section; the unit is
// named for the category. Objective-C compiled with manual reference counting (objc_retain). Unless a
// definition says otherwise: Foundation image, iOS Simulator 24A434, arm64, UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device 24A446
// remains unread. Reading: Foundation/String/ONSStringReview.md.
//
// The GOT slots outside the extracted image are bound by their use (confidence: medium): 0x22227FAE0 is the
// NSException class and 0x22227F5E0 NSRangeException; 0x2222806A8 is CoreFoundation's
// __CFDefaultEightBitStringEncoding (a 32-bit load compared with kCFStringEncodingInvalidId before
// __CFStringComputeEightBitStringEncoding, the pattern CoreFoundation inlines; stringEightBitEncoding spells
// it).

// The NSException class symbol (CoreFoundation), loaded natively through GOT slot 0x22227FAE0.

@implementation ONSString (NSCFAdditions)

// Native -[NSString(NSCFAdditions) _cfTypeID], 0x1c04d1f64. Non-external. A tail call of CFStringGetTypeID.
- (OCFTypeID)_cfTypeID {
    return OCFStringGetTypeID();
}

// Native -[NSString(NSCFAdditions) _createSubstringWithRange:], 0x1c04d1f68. Non-external. The static baseImp
// caches NSString's implementation of the selector at 0x2387B3255 (a selector reference outside the extracted
// image; -substringWithRange: by its use, confidence: medium), looked up with class_getMethodImplementation
// on objc_opt_class(NSString) when the cache is still 0. When the receiver's class implements the selector
// differently, the answer is [[self substringWithRange:range] retain]. Otherwise a range past the length
// raises NSRangeException ("%@: Range {%lu, %lu} out of bounds; string length %lu") and
// -_newSubstringWithRange:zone: with -zone is answered (a tail call).
- (id)_createSubstringWithRange:(ONSRange)range {
    static IMP baseImp;
    SEL selector = @selector(substringWithRange:);
    if (baseImp == NULL) {
        baseImp = class_getMethodImplementation([ONSString class], selector);
    }
    if (baseImp != class_getMethodImplementation([self class], selector)) {
        return [[self substringWithRange:range] retain];
    }
    ONSUInteger length = [self length];
    if (length < range.length || range.location > length - range.length) {
        id reason =
            [ONSString stringWithFormat:(id)OF_STR("%@: Range {%lu, %lu} out of bounds; string length %lu"),
                                        _ONSMethodExceptionProem(self, _cmd),
                                        range.location,
                                        range.length,
                                        length];
        objc_exception_throw([(id)stringCFAdditionsExceptionClass exceptionWithName:(id)ONSRangeException
                                                                             reason:reason
                                                                           userInfo:nil]);
    }
    return [self _newSubstringWithRange:range zone:[self zone]];
}

// Native -[NSString(NSCFAdditions) _encodingCantBeStoredInEightBitCFString], 0x1c04d20a8. Non-external.
// Whether -fastestEncoding differs from the NSStringEncoding of CoreFoundation's eight-bit encoding (read
// after the -fastestEncoding send).
- (BOOL)_encodingCantBeStoredInEightBitCFString {
    ONSUInteger fastest = [self fastestEncoding];
    return fastest != OCFStringConvertEncodingToNSStringEncoding(stringEightBitEncoding());
}

// Native -[NSString(NSCFAdditions) _fastestEncodingInCFStringEncoding], 0x1c04d2100. Non-external.
// CFStringConvertNSStringEncodingToEncoding of -fastestEncoding (a tail call).
- (OCFStringEncoding)_fastestEncodingInCFStringEncoding {
    return OCFStringConvertNSStringEncodingToEncoding([self fastestEncoding]);
}

// Native -[NSString(NSCFAdditions) _getCString:maxLength:encoding:], 0x1c04d2114. Non-external. The
// CFStringEncoding is converted; 0xFFFFFFFF (a 64-bit compare) stores a NUL at buffer[0] and answers NO.
// Otherwise -getBytes:maxLength:filledLength:encoding:allowLossyConversion:NO range:{0, length}
// remainingRange:NULL; on success the filled length is NUL-terminated and YES answered, on failure buffer[0]
// gets the NUL and NO is answered.
- (BOOL)_getCString:(char *)buffer maxLength:(ONSUInteger)maxLength encoding:(OCFStringEncoding)encoding {
    ONSUInteger nsEncoding = OCFStringConvertEncodingToNSStringEncoding(encoding);
    if (nsEncoding != 0xFFFFFFFF) {
        ONSUInteger filledLength = 0;
        if ([self getBytes:buffer
                           maxLength:maxLength
                        filledLength:&filledLength
                            encoding:nsEncoding
                allowLossyConversion:NO
                               range:(ONSRange){0, [self length]}
                      remainingRange:NULL]) {
            buffer[filledLength] = 0;
            return YES;
        }
    }
    buffer[0] = 0;
    return NO;
}

@end

@implementation ONSMutableString (NSCFAdditions)

// Native -[NSMutableString(NSCFAdditions) appendCharacters:length:], 0x1c04d21d8. Non-external. A tail call
// of -replaceCharactersInRange:{length, 0} withCharacters:length:.
- (void)appendCharacters:(const unsigned short *)characters length:(ONSUInteger)length {
    [self replaceCharactersInRange:(ONSRange){[self length], 0} withCharacters:characters length:length];
}

// Native -[NSMutableString(NSCFAdditions) _cfAppendCString:length:], 0x1c04d221c. Non-external. A tail call
// of -replaceCharactersInRange:{length, 0} withCString:length:.
- (void)_cfAppendCString:(const char *)cString length:(ONSInteger)length {
    [self replaceCharactersInRange:(ONSRange){[self length], 0}
                       withCString:cString
                            length:(ONSUInteger)length];
}

// Native -[NSMutableString(NSCFAdditions) _cfLowercase:], 0x1c04d2260. Non-external. The argument is not
// read: {0, length} (the length read first) is replaced with -lowercaseString.
- (void)_cfLowercase:(const void *)locale {
    (void)locale;
    ONSUInteger length = [self length];
    [self replaceCharactersInRange:(ONSRange){0, length} withString:[self lowercaseString]];
}

// Native -[NSMutableString(NSCFAdditions) _cfUppercase:], 0x1c04d229c. Non-external. As -_cfLowercase: with
// -uppercaseString.
- (void)_cfUppercase:(const void *)locale {
    (void)locale;
    ONSUInteger length = [self length];
    [self replaceCharactersInRange:(ONSRange){0, length} withString:[self uppercaseString]];
}

// Native -[NSMutableString(NSCFAdditions) _cfCapitalize:], 0x1c04d22d8. Non-external. As -_cfLowercase: with
// -capitalizedString.
- (void)_cfCapitalize:(const void *)locale {
    (void)locale;
    ONSUInteger length = [self length];
    [self replaceCharactersInRange:(ONSRange){0, length} withString:[self capitalizedString]];
}

// Native -[NSMutableString(NSCFAdditions) _cfPad:length:padIndex:], 0x1c04d2314. Non-external. The length and
// pad index are 32-bit unsigned (W registers, UXTW). A longer receiver loses {length, current - length}
// through -deleteCharactersInRange: (a tail call); an equal one is unchanged. A shorter one is extended by
// count = length - current characters gathered in a 512-character stack buffer (malloc_type_malloc(count * 2,
// type 0x1000040BDFB0063) for 512 or more): the first chunk is min(count, padLength - padIndex) (signed)
// characters from padIndex, written at buffer + (length - remaining - [self length]) characters (the length
// re-read for each chunk); after each chunk the remaining count drops by the chunk, and the next chunk is the
// remaining count when it is below the previous chunk, else CFStringGetLength(padString), from index 0, while
// the remaining count is positive (signed). The buffer is appended with -replaceCharactersInRange:{length, 0}
// withCharacters:length:count and freed when it was allocated.
- (void)_cfPad:(OCFStringRef)padString length:(uint32_t)length padIndex:(uint32_t)indexIntoPad {
    ONSUInteger current = [self length];
    ONSUInteger newLength = length;
    if (current > newLength) {
        [self deleteCharactersInRange:(ONSRange){newLength, current - newLength}];
        return;
    }
    if (current >= newLength) {
        return;
    }
    ONSUInteger count = newLength - current;
    unsigned short stackBuffer[512];
    unsigned short *buffer;
    if (count < 0x200) {
        buffer = stackBuffer;
    } else {
        buffer = malloc_type_malloc(count << 1, 0x1000040BDFB0063);
    }
    OCFIndex padLength = OCFStringGetLength(padString);
    OCFIndex location = indexIntoPad;
    OCFIndex chunk = (OCFIndex)count < padLength - (OCFIndex)indexIntoPad
                         ? (OCFIndex)count
                         : padLength - (OCFIndex)indexIntoPad;
    OCFIndex remaining = (OCFIndex)count;
    do {
        unsigned short *destination = buffer + (newLength - remaining);
        destination -= [self length];
        OCFStringGetCharacters(padString, (OCFRange){location, chunk}, destination);
        remaining -= chunk;
        if (remaining < chunk) {
            chunk = remaining;
        } else {
            chunk = OCFStringGetLength(padString);
            // Foundation defect: 0x1c04d2424 compares remaining with the previous chunk, then
            // 0x1c04d2434 loads the entire pad length. For pad length 3, index 2 and count 3,
            // the second copy writes 3 units into the 2-unit remainder. Clamp that copy only.
            // Confidence: verified; iOS Device divergence: unread.
            if (chunk > remaining) {
                chunk = remaining;
            }
        }
        location = 0;
    } while (remaining > 0);
    [self replaceCharactersInRange:(ONSRange){[self length], 0} withCharacters:buffer length:count];
    if (buffer != stackBuffer) {
        free(buffer);
    }
}

@end
