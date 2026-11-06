//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFIndirectTaggedPointerString.h"

#include "CoreFoundation/Base/OCFUtilities.h"

#include <objc/message.h>
#include <objc/runtime.h>
#include <string.h>

// Inferred organization: _CFStringCreateIndirectASCIITaggedPointerString (0x1c02058fc),
// _CFIndirectTaggedPointerStringGetContents (0x1c0205b0c), the static helpers isEqualToTaggedPointer
// (0x1c0205f98) and isEqualToString (0x1c0206150; both listed as isEqualToTaggedPointer_0 and
// isEqualToString_0 in the evidence, which disambiguates them from the inline unit's statics),
// __NSIndirectTaggedPointerStringIsEqualToTaggedPointer (0x1c0206374) and
// _CFStringCreateWithASCIIBytesNoCopyOrIndirectTagged (0x1c0206b1c) sit among the
// NSIndirectTaggedPointerString methods in the iOS Simulator __text section (after boundPairWriteUnschedule,
// up to the EUC-CN converters); they are the C helpers of that Objective-C unit and are kept in this
// translation unit until the unit itself is restored. The native source path is not in the evidence. Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified; iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationString.md.

// An indirect tagged string (tag index 0) holds the address of immutable ASCII bytes in bits 3-49 of the
// decoded pointer and their length in bits 50 and up (thirteen bits, five for an extended tag).

// libdyld (supplied): the SDK-version query, with the build-version pair in X0 as OCFBuildVersion.
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);

// The build version of the indirect-string decision (MOV X0, #0x7E30901FFFFFFFF): any platform, 0x07e30901.
static const OCFBuildVersion kIndirectTaggedStringVersion = {0xffffffff, 0x07e30901};

// Native charToSixBitLookup, 0x1c03771d8 (__TEXT,__const, 256 bytes; the unit's own copy of the table also at
// 0x1c0372640 and 0x1c0385982). Non-external. The six-bit code of each byte, 0xff when it has none.
static const uint8_t charToSixBitLookup[256] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x2e, 0x07, 0x3c,
    0x1d, 0x1e, 0x28, 0x1f, 0x1c, 0x2b, 0x2c, 0x30, 0x2a, 0x31, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x32, 0x2f, 0x1b, 0x21, 0x35, 0x33, 0x3a, 0x3d, 0x0d, 0x3b, 0x34, 0x27, 0x13, 0x22, 0x29,
    0x2d, 0xff, 0x18, 0x14, 0x17, 0x26, 0x37, 0x36, 0x3f, 0x3e, 0xff, 0xff, 0xff, 0xff, 0xff, 0x38,
    0xff, 0x08, 0x20, 0x0e, 0x0a, 0x00, 0x11, 0x1a, 0x15, 0x01, 0x16, 0x12, 0x02, 0x06, 0x0b, 0x03,
    0x09, 0xff, 0x05, 0x0c, 0x04, 0x10, 0x23, 0x24, 0x19, 0x25, 0x39, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

// The compiled constant "" (0x2227ac180) is kOCFEmptyString's object; the native code takes its address
// directly.

// Native _CFStringCreateIndirectASCIITaggedPointerString, 0x1c02058fc. External. A length of 0 returns the
// empty constant string itself (not retained). While __CFTaggedStringClass is set, an inline tagged string is
// tried first: up to seven bytes without bit 7 set (copied through __memmove_chk into a zeroed word), eight
// or nine characters with six-bit codes, ten or eleven with five-bit codes; the value (code << 7) | (length
// << 3) | 0x8000000000000002 is encoded with stringEncodeTaggedPointer (for up to seven bytes a zero result
// also falls through). Otherwise, for bytes whose bits 56-59 are clear, linked on or after the 0x07e30901 SDK
// and a length below 0x2000, the indirect value 0x8000000000000000 | (length << 50) | (low 47 bits of bytes
// << 3) is encoded (the permutation is indexed by the obfuscator's low bits, which equal the obfuscated
// value's since the value's low bits are zero). Anything else returns NULL; dyld is asked even when the
// length is out of range.
OCFStringRef _OCFStringCreateIndirectASCIITaggedPointerString(const UInt8 *bytes, OCFIndex length) {
    if (length == 0) {
        return kOCFEmptyString;
    }
    if (__CFTaggedStringClass != 0) {
        if (length <= 7) {
            uint64_t word = 0;
            __builtin___memmove_chk(&word, bytes, (size_t)length, sizeof(word));
            if ((word & 0x8080808080808080) == 0) {
                uintptr_t value = ((uintptr_t)length << 3) | (word << 7) | 0x8000000000000002;
                uintptr_t tagged = stringEncodeTaggedPointer(value);
                if (tagged != 0) {
                    return (OCFStringRef)tagged;
                }
            }
        } else if (length <= 0xB) {
            uint64_t code = 0;
            bool encoded = true;
            if (length < 0xA) {
                for (OCFIndex index = 0; index < length; ++index) {
                    uint64_t sixBit = charToSixBitLookup[bytes[index]];
                    if (sixBit > 0x3F) {
                        encoded = false;
                        break;
                    }
                    code = sixBit | (code << 6);
                }
            } else {
                for (OCFIndex index = 0; index < length; ++index) {
                    uint64_t fiveBit = charToSixBitLookup[bytes[index]];
                    if (fiveBit > 0x1F) {
                        encoded = false;
                        break;
                    }
                    code = fiveBit | (code << 5);
                }
            }
            if (encoded) {
                uintptr_t value = (code << 7) | ((uintptr_t)length << 3) | 0x8000000000000002;
                uintptr_t tagged = stringEncodeTaggedPointer(value);
                if (tagged != 0) {
                    return (OCFStringRef)tagged;
                }
            }
        }
    }
    if (((uintptr_t)bytes & 0x0F00000000000000) != 0) {
        return NULL;
    }
    bool linked = dyld_program_sdk_at_least(kIndirectTaggedStringVersion);
    if (((uintptr_t)length >> 13) != 0 || !linked) {
        return NULL;
    }
    uintptr_t value =
        ((uintptr_t)length << 50) | (((uintptr_t)bytes & UINT64_C(0x7FFFFFFFFFFF)) << 3) | 0x8000000000000000;
    uintptr_t obfuscated = objc_debug_taggedpointer_obfuscator ^ value;
    if ((~obfuscated & 0xC000000000000007) == 0) {
        return (OCFStringRef)value;
    }
    return (OCFStringRef)((obfuscated & ~(uintptr_t)7) |
                          objc_debug_tag60_permutations[objc_debug_taggedpointer_obfuscator & 7]);
}

// Native _CFIndirectTaggedPointerStringGetContents, 0x1c0205b0c. External. When length is not NULL it
// receives bits 50 and up of the decoded pointer, masked to 13 bits (5 for a decoded value whose low three
// bits are all set). The contents address is bits 3-49 of the pointer exclusive-ored with the obfuscator, or
// with 0xC000000000000007 itself when the pointer has bits 63, 62 and 0-2 set (bits 3-49 are unaffected
// either way).
const UInt8 *_OCFIndirectTaggedPointerStringGetContents(OCFStringRef string, uint16_t *length) {
    uintptr_t pointer = (uintptr_t)string;
    uintptr_t flags = pointer & 0xC000000000000007;
    uintptr_t obfuscator = objc_debug_taggedpointer_obfuscator;
    if (length != NULL) {
        uintptr_t decoded = (flags == 0xC000000000000007 ? 0 : obfuscator) ^ pointer;
        uint32_t mask = (~(uint32_t)decoded & 7) == 0 ? 0x1F : 0x1FFF;
        *length = (uint16_t)(mask & (uint32_t)(decoded >> 50));
    }
    uintptr_t decoded = (flags == 0xC000000000000007 ? flags : obfuscator) ^ pointer;
    return (const UInt8 *)((decoded >> 3) & UINT64_C(0x7FFFFFFFFFFF));
}

// Native isEqualToString, 0x1c0206150 (defined below in native order).
static bool isEqualToString(OCFTypeRef string, OCFTypeRef other, bool isCF);

// Native isEqualToTaggedPointer, 0x1c0205f98 (isEqualToTaggedPointer_0 in the evidence). Non-external. string
// is an indirect tagged string, other another tagged pointer. The length of string is bits 50 and up of the
// decoded pointer (bits 47 and up of the masked value: 0xF800000000000 for an extended tag, else
// 0xFFF800000000000, applied to decoded >> 3). other's tag is searched in the first seven permutation entries
// (an extended tag is 8 plus bits 55-62): tag 22 is answered by isEqualToString(string, other, false) (a tail
// call); an inline tagged string (tag 2) of the same _NSTaggedPointerStringGetLength is copied into a zeroed
// 16-byte buffer with _NSTaggedPointerStringGetBytes and compared with memcmp; another indirect string (tag
// 0) is equal when its masked length field matches and memcmp of the two contents addresses (bits 3-49)
// answers zero. Any other tag is unequal.
static bool isEqualToTaggedPointer(OCFTypeRef string, OCFTypeRef other) {
    uintptr_t obfuscator = objc_debug_taggedpointer_obfuscator;
    uintptr_t pointer = (uintptr_t)string;
    uintptr_t flags = pointer & 0xC000000000000007;
    uintptr_t decoded = (flags == 0xC000000000000007 ? 0 : obfuscator) ^ pointer;
    uintptr_t lengthMask =
        (~(uint32_t)decoded & 7) == 0 ? UINT64_C(0xF800000000000) : UINT64_C(0xFFF800000000000);
    uintptr_t lengthField = lengthMask & (decoded >> 3);
    size_t length = lengthField >> 47;
    uintptr_t otherPointer = (uintptr_t)other;
    uintptr_t otherDecoded = ((~otherPointer & 0xC000000000000007) == 0 ? 0 : obfuscator) ^ otherPointer;
    uintptr_t otherLow = otherDecoded & 7;
    uintptr_t index = 0;
    do {
        if (otherLow == objc_debug_tag60_permutations[index]) {
            break;
        }
        ++index;
    } while (index != 7);
    uint32_t tag = (uint32_t)(index & 7);
    if (tag == 7) {
        tag = (uint32_t)(((index | otherDecoded) >> 55) & 0xFF) + 8;
    }
    if (tag == 22) {
        return isEqualToString(string, other, false);
    }
    if (tag == 2) {
        if ((size_t)_ONSTaggedPointerStringGetLength((OCFStringRef)other) != length) {
            return false;
        }
        uint64_t buffer[2] = {0, 0};
        _ONSTaggedPointerStringGetBytes((OCFStringRef)other, (UInt8 *)buffer);
        uintptr_t contentsObfuscator =
            flags == 0xC000000000000007 ? flags : objc_debug_taggedpointer_obfuscator;
        const void *bytes = (const void *)(((contentsObfuscator ^ pointer) >> 3) & UINT64_C(0x7FFFFFFFFFFF));
        return memcmp(bytes, buffer, length) == 0;
    }
    if (tag != 0) {
        return false;
    }
    uintptr_t otherMask = otherLow == 7 ? UINT64_C(0xF800000000000) : UINT64_C(0xFFF800000000000);
    if (lengthField != (otherMask & (otherDecoded >> 3))) {
        return false;
    }
    return memcmp(
               (const void *)((decoded >> 3) & UINT64_C(0x7FFFFFFFFFFF)),
               (const void *)((otherDecoded >> 3) & UINT64_C(0x7FFFFFFFFFFF)),
               length
           ) == 0;
}

// Native isEqualToString, 0x1c0206150 (isEqualToString_0 in the evidence). Non-external. string is an
// indirect tagged string (length: bits 50 and up of the decoded pointer, masked to 13 bits, 5 for an extended
// tag; contents: bits 3-49). With isCF set, other's length is _CFStringGetLength2; otherwise __CFStringIsCF
// decides. A CF string of a different length is unequal; its ASCII C string pointer
// (_CFNonObjCStringGetCStringPtr(other, kCFStringEncodingASCII, false)) is compared with memcmp, else its
// characters pointer unit by unit (each byte sign-extended and truncated to 16 bits, LDRSB and UXTH), else
// -compare:options:0 range:{0, [other length]} locale:nil answers equality by a zero result. An object that
// is not a CF string: [other length] must match, then -_fastCStringContents:NO is compared with memcmp, else
// CFStringGetCharactersPtr unit by unit, else -isEqualToString: decides.
static bool isEqualToString(OCFTypeRef string, OCFTypeRef other, bool isCF) {
    uintptr_t pointer = (uintptr_t)string;
    uintptr_t flags = pointer & 0xC000000000000007;
    uintptr_t decoded = (flags == 0xC000000000000007 ? 0 : objc_debug_taggedpointer_obfuscator) ^ pointer;
    uintptr_t lengthMask = (~(uint32_t)decoded & 7) == 0 ? 0x1F : 0x1FFF;
    size_t length = lengthMask & (decoded >> 50);
    OCFIndex otherLength = 0;
    Boolean isMutable = false;
    Boolean isEightBit = false;
    bool isConstant = false;
    const char *cString;
    const UniChar *characters;
    if (isCF) {
        otherLength = _OCFStringGetLength2((OCFStringRef)other);
    } else if (!__OCFStringIsCF((OCFStringRef)other, &isMutable, &otherLength, &isEightBit, &isConstant)) {
        if (((size_t (*)(id, SEL))objc_msgSend)((id)other, sel_getUid("length")) != length) {
            return false;
        }
        cString = ((const char *(*)(id, SEL, BOOL))objc_msgSend)(
            (id)other, sel_getUid("_fastCStringContents:"), NO
        );
        if (cString != NULL) {
            goto compareBytes;
        }
        characters = OCFStringGetCharactersPtr((OCFStringRef)other);
        if (characters != NULL) {
            goto compareCharacters;
        }
        return ((BOOL (*)(id, SEL, id))objc_msgSend)((id)other, sel_getUid("isEqualToString:"), (id)string);
    }
    if ((size_t)otherLength != length) {
        return false;
    }
    cString = _CFNonObjCStringGetCStringPtr((OCFStringRef)other, 0x600, false);
    if (cString == NULL) {
        characters = OCFStringGetCharactersPtr((OCFStringRef)other);
        if (characters == NULL) {
            OCFIndex fullLength = ((OCFIndex (*)(id, SEL))objc_msgSend)((id)other, sel_getUid("length"));
            return ((intptr_t (*)(id, SEL, id, unsigned long, OCFRange, id))objc_msgSend)(
                       (id)other,
                       sel_getUid("compare:options:range:locale:"),
                       (id)string,
                       0,
                       __OCFRangeMake(0, fullLength),
                       nil
                   ) == 0;
        }
        goto compareCharacters;
    }
compareBytes:
    {
        uintptr_t contentsObfuscator =
            flags == 0xC000000000000007 ? flags : objc_debug_taggedpointer_obfuscator;
        const void *bytes = (const void *)(((contentsObfuscator ^ pointer) >> 3) & UINT64_C(0x7FFFFFFFFFFF));
        return memcmp(bytes, cString, length) == 0;
    }
compareCharacters:
    {
        uintptr_t contentsObfuscator =
            flags == 0xC000000000000007 ? flags : objc_debug_taggedpointer_obfuscator;
        if (length == 0) {
            return true;
        }
        const int8_t *bytes =
            (const int8_t *)(((contentsObfuscator ^ pointer) >> 3) & UINT64_C(0x7FFFFFFFFFFF));
        bool equal;
        size_t index = 0;
        do {
            equal = characters[index] == (uint16_t)bytes[index];
            ++index;
        } while (equal && index < length);
        return equal;
    }
}

// Native __NSIndirectTaggedPointerStringIsEqualToTaggedPointer, 0x1c0206374. Hidden (was private external).
// Identical pointers are equal; otherwise the unit's static isEqualToTaggedPointer decides (a tail call).
Boolean __NSIndirectTaggedPointerStringIsEqualToTaggedPointer(OCFTypeRef string, OCFTypeRef other) {
    if (string == other) {
        return true;
    }
    return isEqualToTaggedPointer(string, other);
}

// Native _NSIndirectTaggedPointerStringIsEqualToCFString, 0x1c0206398 (corpus delta capture 7). Hidden (was
// private external). The unit's static isEqualToString with the comparison marked as against a CF string
// (W2 = 1).
Boolean _NSIndirectTaggedPointerStringIsEqualToCFString(OCFTypeRef taggedString, OCFStringRef string) {
    return isEqualToString(taggedString, string, true);
}

// Native _CFStringCreateWithASCIIBytesNoCopyOrIndirectTagged, 0x1c0206b1c. External. A tagged string when one
// can be made, else CFStringCreateWithBytesNoCopy(alloc, bytes, length, kCFStringEncodingASCII, false,
// kCFAllocatorNull) (a tail call).
OCFStringRef _OCFStringCreateWithASCIIBytesNoCopyOrIndirectTagged(
    OCFAllocatorRef alloc, const UInt8 *bytes, OCFIndex length
) {
    OCFStringRef string = _OCFStringCreateIndirectASCIITaggedPointerString(bytes, length);
    if (string != NULL) {
        return string;
    }
    return OCFStringCreateWithBytesNoCopy(alloc, bytes, length, 0x600, false, kOCFAllocatorNull);
}

// Native _CFStringCreateWithASCIICStringNoCopyOrIndirectTagged, 0x1c0206b88. External. The C string's bytes
// (strlen) through _CFStringCreateWithASCIIBytesNoCopyOrIndirectTagged (a tail call).
OCFStringRef
_OCFStringCreateWithASCIICStringNoCopyOrIndirectTagged(OCFAllocatorRef alloc, const char *cString) {
    return _OCFStringCreateWithASCIIBytesNoCopyOrIndirectTagged(
        alloc, (const UInt8 *)cString, strlen(cString)
    );
}
