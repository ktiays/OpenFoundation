//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/PropertyList/ONSBPlistMappedString.h"
#import "Foundation/PropertyList/ONSBPlistMappedData.h"
#include "CoreFoundation/PlugIn/OCFBundleStrings.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/String/ONSStringFromRange.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#include "CoreFoundation/Collections/OCFData.h"

#include <objc/objc-exception.h>

// Native NSBPlistMappedClasses.m, Foundation, iOS Simulator 24A434, arm64,
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device 24A446: unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FB-bundle-paths-plist;
// +initialize: Local/Scratch/cf-core-r2-fb/ida-supplement.json.
// Uniqued GOT bindings (medium): NSInternalInconsistencyException 0x22227f4f8, NSRangeException
// 0x22227f5e0, NSException 0x22227fae0, objc_debug_tag60_permutations 0x222409408,
// objc_debug_taggedpointer_mask 0x222409418, and objc_debug_taggedpointer_obfuscator 0x222409420.

extern uintptr_t objc_debug_taggedpointer_mask;
extern void _objc_registerTaggedPointerClass(uint16_t tag, Class cls);
extern char mappedPlistStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(_ONSBPlistMappedString));
extern char mappedPlistDataClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(_ONSBPlistMappedData));

// Native _enableTaggedPointerBPlistMappedStrings, 0x22e6fcb58. Non-external. When set, use heap objects.
static BOOL enableTaggedPointerBPlistMappedStrings;

// Native __NSCreateBPlistMappedString, 0x1c051a7d8. External.
OCFStringRef __ONSCreateBPlistMappedString(uint64_t offset, OCFDataRef backingData) {
    return (OCFStringRef)[_ONSBPlistMappedString createStringWithOffset:offset
                                                        intoMappingData:(id)backingData];
}

@interface _ONSBPlistMappedString ()
- (void)getBPlistPtr:(const uint8_t **)base
          bplistSize:(OCFIndex *)size
           stringPtr:(const uint8_t **)string
           isUTF16BE:(BOOL *)isUTF16BE __attribute__((objc_direct));
- (BOOL)isUTF16BE __attribute__((objc_direct));
@end

@implementation _ONSBPlistMappedString

// Native +[_NSBPlistMappedString initialize], 0x1c051ab74. Non-external.
// This initializer's tagged-pointer registration requires the isolated object runtime.
+ (void)initialize {
    if ((Class)self == (Class)mappedPlistStringClass) {
        if (objc_debug_taggedpointer_mask != 0 &&
            dyld_program_sdk_at_least((OCFBuildVersion){UINT32_MAX, 0x07e30301})) {
            _objc_registerTaggedPointerClass(22, (Class)self);
        } else {
            enableTaggedPointerBPlistMappedStrings = YES;
        }
    }
}

// Native +[_NSBPlistMappedString createStringWithOffset:intoMappingData:], 0x1c051abe4. Non-external.
// The assembly's two B.GT branches use signed comparisons. The special tag keeps X0 unobfuscated.
// Foundation defect: at 0x1c051ac98 the marker is loaded before the parser validates its address.
// Reject a negative or out-of-mapping offset before the marker load, as for offsets above the native limit.
+ (id)createStringWithOffset:(uint64_t)offset intoMappingData:(id)data {
    if (objc_opt_class(data) != (Class)mappedPlistDataClass) {
        return nil;
    }

    _ONSBPlistMappedData *mapping = data;
    int64_t index = (int64_t)[mapping mappingIndex];
    if (offset > 0xfffffff || index == -1 || index > 1023) {
        return nil;
    }

    const uint8_t *marker = [mapping bytes];
    if (offset >= [mapping length]) {
        return nil;
    }
    marker += offset;
    BOOL utf16 = (*marker & 0xf0) == 0x60;
    const uint8_t *base = [mapping bytes];
    uint64_t limit = [mapping _bplistObjectsRangeEnd];
    __block uint64_t length = 0;
    bool parsed;
    if (utf16) {
        parsed = __OCFBinaryPlistParseUnicode16String(
            base, limit, marker, ^bool(const uint8_t *bytes, uint64_t count) {
              // Native ____getStringLength_block_invoke_2, 0x1c051b6dc. Non-external.
              (void)bytes;
              length = count;
              return true;
            }
        );
    } else {
        parsed = __OCFBinaryPlistParseASCIIString(
            base, limit, marker, ^bool(const uint8_t *bytes, uint64_t count) {
              // Native ____getStringLength_block_invoke, 0x1c051b6c8. Non-external.
              (void)bytes;
              length = count;
              return true;
            }
        );
    }
    if (!parsed) {
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInternalInconsistencyException
                                                      reason:(id)OF_STR("Plist data is corrupt!")
                                                    userInfo:nil]);
    }
    if (length >> 13 != 0) {
        return nil;
    }

    uint64_t value = (offset << 24) | ((uint64_t)index << 14) | (length << 1);
    value += utf16;
    if (enableTaggedPointerBPlistMappedStrings) {
        _ONSBPlistMappedString *string = objc_alloc_init((Class)mappedPlistStringClass);
        if (string != nil) {
            string->payload = value;
        }
        return string;
    }

    uintptr_t tagged = ((value & 0xfffffffffffff) << 3) | UINT64_C(0x8700000000000007);
    uintptr_t obfuscated = tagged ^ objc_debug_taggedpointer_obfuscator;
    if ((~obfuscated & UINT64_C(0xc000000000000007)) != 0) {
        tagged = (obfuscated & ~(uintptr_t)7) | objc_debug_tag60_permutations[obfuscated & 7];
    }
    return (id)tagged;
}

// Native -[_NSBPlistMappedString characterAtIndex:], 0x1c051ae48. Non-external.
- (unsigned short)characterAtIndex:(ONSUInteger)index {
    unsigned short character = 0;
    [self getCharacters:&character range:(ONSRange){index, 1}];
    return character;
}

// Native -[_NSBPlistMappedString getBPlistPtr:bplistSize:stringPtr:isUTF16BE:], 0x1c051aea4. Non-external.
// Direct, with no selector register. The throw does not fall through to getCharacters:range:.
- (void)getBPlistPtr:(const uint8_t **)base
          bplistSize:(OCFIndex *)size
           stringPtr:(const uint8_t **)string
           isUTF16BE:(BOOL *)isUTF16BE {
    uintptr_t pointer = (uintptr_t)self;
    uint64_t value;
    if ((intptr_t)pointer >= 0 || enableTaggedPointerBPlistMappedStrings) {
        value = payload;
    } else {
        uintptr_t obfuscator =
            (~pointer & UINT64_C(0xc000000000000007)) == 0 ? 0 : objc_debug_taggedpointer_obfuscator;
        uintptr_t decoded = pointer ^ obfuscator;
        uint64_t mask = (decoded & 7) == 7 ? 0xfffffffffffff : 0xfffffffffffffff;
        value = (decoded >> 3) & mask;
    }

    *isUTF16BE = (value & 1) != 0;
    OCFDataRef mapping = _OCFBundleGetMappedStringsFile((value >> 14) & 0x3ff);
    if (mapping == NULL) {
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInternalInconsistencyException
                                                      reason:(id)OF_STR("Invalid tagged pointer reference")
                                                    userInfo:nil]);
    }
    *base = OCFDataGetBytePtr(mapping);
    *size = OCFDataGetLength(mapping);
    *string = *base + ((value >> 24) & 0xfffffff);
}

// Native -[_NSBPlistMappedString getCharacters:range:], 0x1c051af94. Non-external.
- (void)getCharacters:(unsigned short *)buffer range:(ONSRange)range {
    const uint8_t *base = NULL;
    const uint8_t *marker = NULL;
    OCFIndex size = 0;
    BOOL utf16 = NO;
    [self getBPlistPtr:&base bplistSize:&size stringPtr:&marker isUTF16BE:&utf16];

    __block uint64_t actualLength = 0;
    __block BOOL outOfRange = NO;
    bool parsed;
    if (utf16) {
        parsed = __OCFBinaryPlistParseUnicode16String(
            base, size, marker, ^bool(const uint8_t *bytes, uint64_t length) {
              // Native ___45-[_NSBPlistMappedString getCharacters:range:]_block_invoke_2, 0x1c051b26c.
              // Foundation defect: the sum at 0x1c051b2cc can wrap. Check the remaining length before
              // copying.
              BOOL strict = dyld_program_sdk_at_least((OCFBuildVersion){UINT32_MAX, 0x07e60901});
              uint64_t start = range.location;
              uint64_t count = range.length;
              if (!strict) {
                  if (start >= length) {
                      start = count = 0;
                  } else {
                      uint64_t end = range.location + range.length;
                      count = end < length ? end - start : length - start;
                  }
              }
              if (start > length || count > length - start) {
                  actualLength = length;
                  outOfRange = YES;
              } else {
                  for (uint64_t i = 0; i < count; ++i) {
                      const uint8_t *character = bytes + 2 * (start + i);
                      buffer[i] = (uint16_t)((character[0] << 8) | character[1]);
                  }
              }
              return true;
            }
        );
    } else {
        parsed = __OCFBinaryPlistParseASCIIString(
            base, size, marker, ^bool(const uint8_t *bytes, uint64_t length) {
              // Native ___45-[_NSBPlistMappedString getCharacters:range:]_block_invoke, 0x1c051b1c0.
              // Foundation defect: the sum at 0x1c051b220 can wrap. Check the remaining length before
              // copying.
              BOOL strict = dyld_program_sdk_at_least((OCFBuildVersion){UINT32_MAX, 0x07e60901});
              uint64_t start = range.location;
              uint64_t count = range.length;
              if (!strict) {
                  if (start >= length) {
                      start = count = 0;
                  } else {
                      uint64_t end = range.location + range.length;
                      count = end < length ? end - start : length - start;
                  }
              }
              if (start > length || count > length - start) {
                  actualLength = length;
                  outOfRange = YES;
              } else {
                  __OCFStrConvertBytesToUnicode(bytes + start, buffer, count);
              }
              return true;
            }
        );
    }
    if (!parsed) {
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInternalInconsistencyException
                                                      reason:(id)OF_STR("Plist data is corrupt!")
                                                    userInfo:nil]);
    }
    if (outOfRange) {
        id rangeString = ONSStringFromRange(range);
        ONSUInteger encodedLength = [self length];
        id reason = [ONSString
            stringWithFormat:(id)OF_STR(
                                 "Out of range. Requested range: %@, reported length: %ld, encoded length: "
                                 "%ld, isUTF16BE: %d"
                             ),
                             rangeString,
                             actualLength,
                             encodedLength,
                             (int)utf16];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSRangeException reason:reason
                                                    userInfo:nil]);
    }
}

// Native -[_NSBPlistMappedString length], 0x1c051b330. Non-external.
- (ONSUInteger)length {
    uintptr_t pointer = (uintptr_t)self;
    uint64_t value;
    if ((intptr_t)pointer >= 0 || enableTaggedPointerBPlistMappedStrings) {
        value = payload;
    } else {
        uintptr_t obfuscator =
            (~pointer & UINT64_C(0xc000000000000007)) == 0 ? 0 : objc_debug_taggedpointer_obfuscator;
        uintptr_t decoded = pointer ^ obfuscator;
        uint64_t mask = (decoded & 7) == 7 ? 0xfffffffffffff : 0xfffffffffffffff;
        value = (decoded >> 3) & mask;
    }
    return (value >> 1) & 0x1fff;
}

// Native -[_NSBPlistMappedString _fastCStringContents:], 0x1c051b398. Non-external.
- (const char *)_fastCStringContents:(BOOL)nullTerminationRequired {
    __block const char *result = NULL;
    if (!nullTerminationRequired) {
        ONSUInteger length = 0;
        ONSUInteger *lengthOut = &length;
        const uint8_t *base = NULL;
        const uint8_t *marker = NULL;
        OCFIndex size = 0;
        BOOL utf16 = NO;
        [self getBPlistPtr:&base bplistSize:&size stringPtr:&marker isUTF16BE:&utf16];
        if (!utf16 && !__OCFBinaryPlistParseASCIIString(
                          base, size, marker, ^bool(const uint8_t *bytes, uint64_t count) {
                            // Native ____fastASCIIContents_block_invoke, 0x1c051b6f0. Non-external.
                            *lengthOut = count;
                            result = (const char *)bytes;
                            return true;
                          }
                      )) {
            objc_exception_throw([ONSException exceptionWithName:(id)ONSInternalInconsistencyException
                                                          reason:(id)OF_STR("Plist data is corrupt!")
                                                        userInfo:nil]);
        }
    }
    return result;
}

// Native -[_NSBPlistMappedString _fastUTF8StringContents:utf8Length:], 0x1c051b4dc. Non-external.
- (const char *)_fastUTF8StringContents:(BOOL)nullTerminationRequired utf8Length:(ONSUInteger *)length {
    __block const char *result = NULL;
    if (!nullTerminationRequired) {
        const uint8_t *base = NULL;
        const uint8_t *marker = NULL;
        OCFIndex size = 0;
        BOOL utf16 = NO;
        [self getBPlistPtr:&base bplistSize:&size stringPtr:&marker isUTF16BE:&utf16];
        if (!utf16 && !__OCFBinaryPlistParseASCIIString(
                          base, size, marker, ^bool(const uint8_t *bytes, uint64_t count) {
                            // Native ____fastASCIIContents_block_invoke, 0x1c051b6f0. Non-external, inlined
                            // parent.
                            *length = count;
                            result = (const char *)bytes;
                            return true;
                          }
                      )) {
            objc_exception_throw([ONSException exceptionWithName:(id)ONSInternalInconsistencyException
                                                          reason:(id)OF_STR("Plist data is corrupt!")
                                                        userInfo:nil]);
        }
    }
    return result;
}

// Native -[_NSBPlistMappedString isUTF16BE], 0x1c051b61c. Non-external, direct.
- (BOOL)isUTF16BE {
    uintptr_t pointer = (uintptr_t)self;
    uint64_t value;
    if ((intptr_t)pointer >= 0 || enableTaggedPointerBPlistMappedStrings) {
        value = payload;
    } else {
        uintptr_t obfuscator =
            (~pointer & UINT64_C(0xc000000000000007)) == 0 ? 0 : objc_debug_taggedpointer_obfuscator;
        uintptr_t decoded = pointer ^ obfuscator;
        uint64_t mask = (decoded & 7) == 7 ? 0xfffffffffffff : 0xfffffffffffffff;
        value = (decoded >> 3) & mask;
    }
    return (value & 1) != 0;
}

// Native -[_NSBPlistMappedString fastestEncoding], 0x1c051b67c. Non-external.
- (ONSUInteger)fastestEncoding {
    return [self isUTF16BE] ? 0x90000100 : 1;
}

// Native -[_NSBPlistMappedString copyWithZone:], 0x1c051b6c4. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

@end
