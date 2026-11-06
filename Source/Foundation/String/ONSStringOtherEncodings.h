//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Collections/OCFData.h"
#import "Foundation/String/ONSString.h"

// The NSStringOtherEncodings and __NSLocalizedStringAddition categories and the default C string encoding
// state (String/ONSStringOtherEncodings.m).

// Native _NSDefaultStringEncoding (0x22e6e46f0) and _NSCStringEncoding (0x22e6e46f8), external, eight bytes
// each in __DATA,__data, initially -1; ___NSDefaultStringEncodingFullyInited (0x22e775b30, external,
// __DATA,__common), accessed as a byte.
OF_EXPORT ONSUInteger _ONSDefaultStringEncoding;
OF_EXPORT ONSUInteger _ONSCStringEncoding;
OF_EXPORT BOOL __ONSDefaultStringEncodingFullyInited;

// Native __NSCStringCharToUnichar (0x22e775b38, external) and __NSCStringCharToUnicharTable (0x22e775b40,
// was a private external), __DATA,__common: the C string character mapping function (an eight-bit character
// in W0, the UTF-16 unit in W0) and the 256-entry table built from it.
typedef unsigned short (*CStringCharToUnicharFunction)(unsigned char character);
OF_EXPORT CStringCharToUnicharFunction _ONSCStringCharToUnichar;
OF_EXTERN unsigned short *_NSCStringCharToUnicharTable;

// Native ___NSSetCStringCharToUnichar, 0x1c05d9ea4 (hidden, was a private external).
OF_EXTERN void __NSSetCStringCharToUnichar(CStringCharToUnicharFunction function);

// Native __NSDefaultCStringEncoding, 0x1c05d9fdc, external: sets and answers _NSDefaultStringEncoding.
OF_EXPORT ONSUInteger _ONSDefaultCStringEncoding(void);

// Native ___NSASCIICharToUnichar (0x1c0fdedf0) and ___NSCharToUnicharCFWrapper (0x1c0fdedf4), hidden (were
// private externals), in the outlined region.
OF_EXTERN unsigned short __NSASCIICharToUnichar(unsigned char character);
OF_EXTERN unsigned short __NSCharToUnicharCFWrapper(unsigned char character);

// The output record of __NSConvertToASCIIorUnicode2 (1,040 bytes): isASCII at 0, shouldFreeChars at 1, the
// eight-bit characters at 8, the UTF-16 characters at 0x10, the character count at 0x18 and a 1,008-byte
// local buffer at 0x20 (0x3f0 eight-bit or 0x1f8 UTF-16 characters). StringConversionBuffer and its field
// names are local descriptive names; the layout is read from the stores of __NSConvertToASCIIorUnicode2.
typedef struct {
    BOOL isASCII;
    BOOL shouldFreeChars;
    uint8_t *ascii;
    unsigned short *unicode;
    ONSUInteger numChars;
    uint8_t localBuffer[1008];
} StringConversionBuffer;

_Static_assert(offsetof(StringConversionBuffer, ascii) == 8, "The eight-bit characters are at 8.");
_Static_assert(offsetof(StringConversionBuffer, numChars) == 0x18, "The character count is at 0x18.");
_Static_assert(sizeof(StringConversionBuffer) == 0x410, "The conversion buffer is 1,040 bytes.");

// Native __NSConvertToASCIIorUnicode2, 0x1c05dad38, external: decodes length bytes in the NSStringEncoding
// into buffer (eight-bit unless alwaysUnicode, or UTF-16), allocating from zone beyond the local buffer;
// answers whether the decoding succeeded.
OF_EXPORT BOOL _ONSConvertToASCIIorUnicode2(
    const void *bytes,
    ONSUInteger length,
    ONSUInteger encoding,
    BOOL alwaysUnicode,
    StringConversionBuffer *buffer,
    struct _NSZone *zone
);

#ifdef __OBJC__
@interface ONSString (NSStringOtherEncodings)

+ (ONSUInteger)defaultCStringEncoding;
- (const char *)UTF8String;
- (const char *)cString;
- (ONSUInteger)fastestEncoding;
- (id)dataUsingEncoding:(ONSUInteger)encoding allowLossyConversion:(BOOL)lossy;
- (id)dataUsingEncoding:(ONSUInteger)encoding;
- (const char *)cStringUsingEncoding:(ONSUInteger)encoding;
- (BOOL)getCString:(char *)buffer maxLength:(ONSUInteger)maxBufferCount encoding:(ONSUInteger)encoding;
- (ONSUInteger)lengthOfBytesUsingEncoding:(ONSUInteger)encoding;
- (BOOL)getBytes:(void *)buffer
         maxLength:(ONSUInteger)maxBufferCount
        usedLength:(ONSUInteger *)usedBufferCount
          encoding:(ONSUInteger)encoding
           options:(ONSUInteger)options
             range:(ONSRange)range
    remainingRange:(ONSRange *)leftover;
- (BOOL)getBytes:(void *)buffer
               maxLength:(ONSUInteger)maxLength
            filledLength:(ONSUInteger *)filledLength
                encoding:(ONSUInteger)encoding
    allowLossyConversion:(BOOL)lossy
                   range:(ONSRange)range
          remainingRange:(ONSRange *)remainingRange;

@end

@interface ONSString (__NSLocalizedStringAddition)

- (id)_copyFormatStringWithConfiguration:(id)configuration;
- (id)formatConfiguration;

@end

// Selectors the categories send to objects outside this family, declared for the message sends only; no
// protocol object is emitted.

#endif
