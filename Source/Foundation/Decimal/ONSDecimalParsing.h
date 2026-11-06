//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured assembly; iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Scratch/cf-core-fd-misc/Reading.md.

#include <OpenFoundation/Decimal/ONSDecimal.h>

// Native _NSCStringToDecimal 0x1c04f914c, private external: the decimal is returned indirectly in x8.
// Hex-Rays incorrectly presents the output as an ordinary argument; assembly settles the C struct return.
OF_EXTERN ONSDecimal _NSCStringToDecimal(const char *string, char **end);

// Native _NSStringToDecimal 0x1c08bf420, private external: Swift body with a C entry boundary.
OF_EXTERN void _NSStringToDecimal(const void *string, ONSUInteger *length, ONSDecimal *result);

#include "CoreFoundation/String/OCFString.h"

// Private importer spelling: native CFStringRef is renamed CFString by Swift; the pointer ABI is identical.
typedef OCFStringRef DecimalStringPointer;

// Native _CFNonObjCStringGetBytes, CoreFoundation 0x1c020fafc (external); called at Foundation 0x1c0b0ca84.
// Its owning CF header does not yet declare it. Eight C parameters occupy x0-x7 and one stack slot
// because OCFRange occupies two registers. The CoreFoundation body remains unread here.
OF_EXPORT OCFIndex _OCFNonObjCStringGetBytes(
    OCFStringRef string,
    OCFRange range,
    OCFStringEncoding encoding,
    UInt8 lossByte,
    Boolean external,
    UInt8 *buffer,
    OCFIndex capacity,
    OCFIndex *usedLength
);

#ifdef __OBJC__
#import "Foundation/Base/ONSUnrestoredInterfaces.h"
#include <OpenFoundation/Range/ONSRange.h>

// libobjc C ownership boundary used by the native tagged-string fast path.
extern void *objc_retainAutorelease(void *object);

// NSString bridge message interfaces (FS-string), called by native Swift 0x1c0b0b6c0.
@interface ONSString (DecimalBridgeInterfaces)
- (id)mutableCopy;
- (const unsigned char *)_fastUTF8StringContents:(BOOL)requireNull utf8Length:(ONSUInteger *)length;
- (ONSUInteger)lengthOfBytesUsingEncoding:(ONSUInteger)encoding;
- (BOOL)getBytes:(void *)buffer
         maxLength:(ONSUInteger)capacity
        usedLength:(ONSUInteger *)usedLength
          encoding:(ONSUInteger)encoding
           options:(ONSUInteger)options
             range:(ONSRange)range
    remainingRange:(ONSRange *)remainingRange;
@end
#endif

// Native libobjc inline _objc_getTaggedPointerTag, Foundation copy 0x1c06ea730 (non-external).
// Read through IDA capture; this inline supplies exactly the native runtime-global reads to Swift,
// whose concurrency checker cannot directly read libobjc's initialized obfuscator global.
static inline unsigned decimalStringTaggedPointerTag(const void *object) {
    uintptr_t pointer = (uintptr_t)object;
    uintptr_t obfuscator =
        (~pointer & UINT64_C(0xc000000000000007)) == 0 ? 0 : objc_debug_taggedpointer_obfuscator;
    uintptr_t decoded = pointer ^ obfuscator;
    unsigned index = 0;
    while (index != 7 && (decoded & 7) != objc_debug_tag60_permutations[index]) {
        index++;
    }
    return index == 7 ? (unsigned)(((decoded | index) >> 55) & 0xff) + 8 : index;
}
